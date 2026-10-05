#include "device.h"
#include "protocol.h"
#include <setupapi.h>
#include <hidsdi.h>
#include <hidclass.h>
#include <stdio.h>

namespace {
bool Transfer(HANDLE handle, DWORD code, BYTE* bytes, HANDLE stop, DWORD& error) {
    OVERLAPPED ov = {};
    ov.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!ov.hEvent) { error = GetLastError(); return false; }
    DWORD transferred = 0;
    bool ok = DeviceIoControl(handle, code, bytes, battery::ReportSize,
        bytes, battery::ReportSize, &transferred, &ov) != FALSE;
    if (!ok) {
        error = GetLastError();
        if (error == ERROR_IO_PENDING) {
            HANDLE events[] = {ov.hEvent, stop};
            DWORD wait = WaitForMultipleObjects(stop ? 2 : 1, events, FALSE, 750);
            if (wait == WAIT_OBJECT_0) {
                ok = GetOverlappedResult(handle, &ov, &transferred, FALSE) != FALSE;
                if (!ok) error = GetLastError();
            } else {
                error = wait == WAIT_TIMEOUT ? ERROR_TIMEOUT : ERROR_OPERATION_ABORTED;
                CancelIoEx(handle, &ov);
                // Drain cancellation before releasing the stack OVERLAPPED and its buffer.
                GetOverlappedResult(handle, &ov, &transferred, TRUE);
            }
        }
    }
    // SET_FEATURE has no output count; GET_FEATURE must provide the full payload.
    if (ok && code == IOCTL_HID_GET_FEATURE && !battery::FeatureLength(transferred, bytes[0])) {
        ok = false; error = ERROR_INVALID_DATA;
    }
    CloseHandle(ov.hEvent);
    if (ok) error = 0;
    return ok;
}
}

bool Device::Query(HANDLE handle, BYTE command, BYTE& value, HANDLE stop, DWORD& error) {
    for (int attempt = 0; attempt < 2; ++attempt) {
        BYTE bytes[battery::ReportSize];
        battery::Request(bytes, command);
        if (!Transfer(handle, IOCTL_HID_SET_FEATURE, bytes, stop, error)) return false;
        if (stop) { if (WaitForSingleObject(stop, 60) == WAIT_OBJECT_0) return false; }
        else Sleep(60);
        ZeroMemory(bytes, sizeof(bytes));
        if (!Transfer(handle, IOCTL_HID_GET_FEATURE, bytes, stop, error)) return false;
        if (battery::Valid(bytes, command)) { value = bytes[10]; return true; }
        error = ERROR_INVALID_DATA;
        // Busy, stale or colliding replies (e.g. Synapse): at most one retry.
        if (stop) { if (WaitForSingleObject(stop, 100) == WAIT_OBJECT_0) return false; }
        else Sleep(100);
    }
    return false;
}

bool Device::ReadPath(const wchar_t* path, WORD pid, Reading& reading, HANDLE stop) {
    HANDLE handle = CreateFileW(path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
        OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
    if (handle == INVALID_HANDLE_VALUE) { reading.error = GetLastError(); return false; }
    BYTE raw = 0;
    bool ok = Query(handle, 0x80, raw, stop, reading.error);
    if (ok) {
        reading.percent = battery::Percent(raw);
        reading.pid = pid;
        BYTE charging = 0;
        DWORD chargingError = 0;
        if (Query(handle, 0x84, charging, stop, chargingError) && charging <= 1)
            reading.charging = charging;
        swprintf_s(reading.status, L"Bateria: %d%% · Basilisk V3 Pro%s · %s%s", reading.percent,
            pid == 0x00cc || pid == 0x00cd ? L" 35K" : L"",
            battery::Wired(pid) ? L"USB" : L"Wireless",
            reading.charging == 1 ? L" · Carregando" : L"");
    }
    CloseHandle(handle);
    return ok;
}

Reading Device::Read(HANDLE stop) {
    Reading result;
    if (path_[0]) {
        if (ReadPath(path_, pid_, result, stop)) return result;
        Reset();
        // Preserve the failure as unavailable; rediscover on the next poll.
        wcscpy_s(result.status, L"Bateria indisponível · tentando reconectar");
        return result;
    }
    GUID guid; HidD_GetHidGuid(&guid);
    HDEVINFO devices = SetupDiGetClassDevsW(&guid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (devices == INVALID_HANDLE_VALUE) {
        result.error = GetLastError();
        wcscpy_s(result.status, L"Não foi possível procurar o mouse");
        return result;
    }
    bool found = false;
    // Prefer a cable when both cable and receiver are attached.
    for (int pass = 0; pass < 2 && result.percent < 0; ++pass) {
        for (DWORD i = 0; ; ++i) {
            if (stop && WaitForSingleObject(stop, 0) == WAIT_OBJECT_0) break;
            SP_DEVICE_INTERFACE_DATA iface = {}; iface.cbSize = sizeof(iface);
            if (!SetupDiEnumDeviceInterfaces(devices, nullptr, &guid, i, &iface)) break;
            BYTE buffer[4096] = {};
            auto detail = (SP_DEVICE_INTERFACE_DETAIL_DATA_W*)buffer;
            detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
            if (!SetupDiGetDeviceInterfaceDetailW(devices, &iface, detail, sizeof(buffer), nullptr, nullptr)) continue;
            HANDLE handle = CreateFileW(detail->DevicePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
            if (handle == INVALID_HANDLE_VALUE) continue;
            HIDD_ATTRIBUTES attrs = {}; attrs.Size = sizeof(attrs);
            bool supported = HidD_GetAttributes(handle, &attrs) && attrs.VendorID == 0x1532 &&
                battery::Supported(attrs.ProductID) && battery::Wired(attrs.ProductID) == (pass == 0);
            bool feature = false;
            if (supported) {
                found = true;
                PHIDP_PREPARSED_DATA data = nullptr;
                if (HidD_GetPreparsedData(handle, &data)) {
                    HIDP_CAPS caps = {};
                    feature = HidP_GetCaps(data, &caps) == HIDP_STATUS_SUCCESS &&
                        caps.FeatureReportByteLength == battery::ReportSize;
                    HidD_FreePreparsedData(data);
                }
            }
            CloseHandle(handle);
            if (feature && ReadPath(detail->DevicePath, attrs.ProductID, result, stop)) {
                wcscpy_s(path_, detail->DevicePath); pid_ = attrs.ProductID;
                break;
            }
        }
    }
    SetupDiDestroyDeviceInfoList(devices);
    if (result.percent < 0) wcscpy_s(result.status, found ?
        L"Mouse encontrado · bateria indisponível" : L"Mouse desconectado · aguardando conexão");
    return result;
}
