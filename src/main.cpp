#include <windows.h>
#include <shellapi.h>
#include <dbt.h>
#include <hidsdi.h>
#include <stdio.h>
#include "version.h"
#include "device.h"
#include "icon.h"
#include "startup.h"

namespace {
constexpr UINT TrayMessage = WM_APP + 1, ResultMessage = WM_APP + 2;
constexpr UINT Startup = 101, Refresh = 102, Quit = 103;
HANDLE stopEvent = nullptr, pollEvent = nullptr, threadHandle = nullptr;
volatile LONG topology = 0;
HWND window = nullptr;
HICON currentIcon = nullptr;
Reading current;
CRITICAL_SECTION resultLock;
Reading pending;
UINT taskbarCreated = 0;
bool trayAdded = false, suspended = false;
int iconPercent = -999, iconSize = 0;

int TraySize() {
    using GetDpi = UINT(WINAPI*)(HWND);
    using GetMetric = int(WINAPI*)(int, UINT);
    HMODULE user = GetModuleHandleW(L"user32.dll");
    auto dpi = (GetDpi)GetProcAddress(user, "GetDpiForWindow");
    auto metric = (GetMetric)GetProcAddress(user, "GetSystemMetricsForDpi");
    HWND taskbar = FindWindowW(L"Shell_TrayWnd", nullptr);
    int size = dpi && metric ? metric(SM_CXSMICON, dpi(taskbar ? taskbar : window)) : GetSystemMetrics(SM_CXSMICON);
    return size > 0 && size <= 128 ? size : 16;
}
void UpdateTray(bool force = false) {
    int size = TraySize();
    if (!force && trayAdded && iconPercent == current.percent && iconSize == size) {
        // The tooltip can change independently (charging/connection).
        NOTIFYICONDATAW tip = {}; tip.cbSize = sizeof(tip); tip.hWnd = window; tip.uID = 1;
        tip.uFlags = NIF_TIP | NIF_SHOWTIP;
        wcscpy_s(tip.szTip, current.status); Shell_NotifyIconW(NIM_MODIFY, &tip);
        return;
    }
    HICON next = BatteryIcon(current.percent, size);
    if (!next) return;
    NOTIFYICONDATAW data = {}; data.cbSize = sizeof(data); data.hWnd = window; data.uID = 1;
    data.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;
    data.uCallbackMessage = TrayMessage; data.hIcon = next;
    wcscpy_s(data.szTip, current.status);
    bool ok = Shell_NotifyIconW(trayAdded ? NIM_MODIFY : NIM_ADD, &data) != FALSE;
    if (!ok && trayAdded) ok = Shell_NotifyIconW(NIM_ADD, &data) != FALSE;
    if (ok) {
        trayAdded = true;
        data.uVersion = NOTIFYICON_VERSION_4;
        Shell_NotifyIconW(NIM_SETVERSION, &data);
        if (currentIcon) DestroyIcon(currentIcon);
        currentIcon = next; iconPercent = current.percent; iconSize = size;
    } else DestroyIcon(next);
}
DWORD WINAPI PollWorker(void*) {
    Device device;
    LONG seen = -1;
    HANDLE events[] = {stopEvent, pollEvent};
    while (WaitForMultipleObjects(2, events, FALSE, INFINITE) == WAIT_OBJECT_0 + 1) {
        LONG now = InterlockedCompareExchange(&topology, 0, 0);
        if (now != seen) { device.Reset(); seen = now; }
        Reading result = device.Read(stopEvent);
        EnterCriticalSection(&resultLock); pending = result; LeaveCriticalSection(&resultLock);
        PostMessageW(window, ResultMessage, 0, 0);
    }
    return 0;
}
void Menu() {
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING | MF_GRAYED, 0, current.status);
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING | (startup::Enabled() ? MF_CHECKED : 0), Startup, L"Iniciar com o Windows");
    AppendMenuW(menu, MF_STRING, Refresh, L"Atualizar agora");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING | MF_GRAYED, 0, L"Basilisk Battery · v" APP_VERSION_W);
    AppendMenuW(menu, MF_STRING, Quit, L"Sair");
    POINT point; GetCursorPos(&point);
    SetForegroundWindow(window);
    UINT selected = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, point.x, point.y, 0, window, nullptr);
    DestroyMenu(menu); PostMessageW(window, WM_NULL, 0, 0);
    if (selected == Startup) {
        if (!startup::SetEnabled(!startup::Enabled())) MessageBoxW(window,
            L"Não foi possível alterar a inicialização. Tente novamente.", L"Basilisk Battery", MB_OK | MB_ICONERROR);
    } else if (selected == Refresh) {
        InterlockedIncrement(&topology); SetEvent(pollEvent);
    } else if (selected == Quit) DestroyWindow(window);
}
LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    if (message == taskbarCreated && taskbarCreated) { trayAdded = false; UpdateTray(true); return 0; }
    switch (message) {
    case TrayMessage:
        if (LOWORD(lparam) == WM_CONTEXTMENU || LOWORD(lparam) == NIN_SELECT || LOWORD(lparam) == NIN_KEYSELECT) Menu();
        return 0;
    case ResultMessage:
        if (!suspended) {
            EnterCriticalSection(&resultLock); current = pending; LeaveCriticalSection(&resultLock);
            UpdateTray();
        }
        return 0;
    case WM_TIMER: if (!suspended) { if (!trayAdded) UpdateTray(true); SetEvent(pollEvent); } return 0;
    case WM_DEVICECHANGE:
        if (wparam == DBT_DEVICEARRIVAL || wparam == DBT_DEVICEREMOVECOMPLETE || wparam == DBT_DEVNODES_CHANGED) {
            InterlockedIncrement(&topology); if (!suspended) SetEvent(pollEvent);
        }
        return TRUE;
    case WM_POWERBROADCAST:
        if (wparam == PBT_APMSUSPEND) suspended = true;
        if (wparam == PBT_APMRESUMEAUTOMATIC || wparam == PBT_APMRESUMESUSPEND) {
            suspended = false; InterlockedIncrement(&topology); SetEvent(pollEvent);
        }
        return TRUE;
    case WM_SETTINGCHANGE: case WM_DISPLAYCHANGE: case WM_DPICHANGED: UpdateTray(true); return 0;
    case WM_CLOSE: DestroyWindow(hwnd); return 0;
    case WM_DESTROY: {
        KillTimer(hwnd, 1); SetEvent(stopEvent);
        NOTIFYICONDATAW data = {}; data.cbSize = sizeof(data); data.hWnd = hwnd; data.uID = 1;
        Shell_NotifyIconW(NIM_DELETE, &data);
        PostQuitMessage(0); return 0;
    }
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}
int Diagnose(const wchar_t* output) {
    Device device;
    Reading reading = device.Read(nullptr);
    FILE* file = nullptr;
    if (_wfopen_s(&file, output, L"wb") || !file) return 2;
    fprintf(file, "{\"version\":\"%s\",\"percent\":%d,\"charging\":%d,\"pid\":%u,\"win32Error\":%lu}\n",
        APP_VERSION, reading.percent, reading.charging, reading.pid, reading.error);
    fclose(file);
    return reading.percent >= 0 ? 0 : 1;
}
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) return 2;
    if (argc == 3 && wcscmp(argv[1], L"--diagnose") == 0) {
        int exitCode = Diagnose(argv[2]); LocalFree(argv); return exitCode;
    }
    if (argc == 3 && wcscmp(argv[1], L"--export-icons") == 0) {
        CreateDirectoryW(argv[2], nullptr);
        bool ok = true;
        const int sizes[] = {16, 20, 24, 32, 48};
        const int percentages[] = {-1, 0, 9, 24, 25, 49, 50, 76, 88, 99, 100};
        for (int size : sizes) for (int percent : percentages) {
            wchar_t path[1024]; swprintf_s(path, L"%s\\battery-%d-%d.ico", argv[2], percent, size);
            HICON icon = BatteryIcon(percent, size); ok = icon && SaveIcon(icon, path) && ok;
            if (icon) DestroyIcon(icon);
        }
        LocalFree(argv); return ok ? 0 : 2;
    }
    LocalFree(argv);
    HANDLE mutex = CreateMutexW(nullptr, FALSE, L"Local\\BasiliskBattery.SingleInstance");
    if (!mutex) return 2;
    if (GetLastError() == ERROR_ALREADY_EXISTS) { CloseHandle(mutex); return 0; }
    taskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
    InitializeCriticalSection(&resultLock);
    stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    pollEvent = CreateEventW(nullptr, FALSE, TRUE, nullptr);
    WNDCLASSW wc = {}; wc.lpfnWndProc = WindowProc; wc.hInstance = instance;
    wc.lpszClassName = L"BasiliskBattery.Window"; wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(1));
    if (!stopEvent || !pollEvent || !RegisterClassW(&wc)) return 2;
    window = CreateWindowExW(WS_EX_TOOLWINDOW, wc.lpszClassName, L"Basilisk Battery", WS_POPUP,
        0, 0, 0, 0, nullptr, nullptr, instance, nullptr);
    if (!window) return 2;
    DEV_BROADCAST_DEVICEINTERFACE_W notification = {};
    notification.dbcc_size = sizeof(notification); notification.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    HidD_GetHidGuid(&notification.dbcc_classguid);
    HDEVNOTIFY registration = RegisterDeviceNotificationW(window, &notification, DEVICE_NOTIFY_WINDOW_HANDLE);
    UpdateTray();
    threadHandle = CreateThread(nullptr, 0, PollWorker, nullptr, 0, nullptr);
    if (!threadHandle || !SetTimer(window, 1, 30000, nullptr)) { DestroyWindow(window); }
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    SetEvent(stopEvent);
    if (threadHandle) { WaitForSingleObject(threadHandle, INFINITE); CloseHandle(threadHandle); }
    if (registration) UnregisterDeviceNotification(registration);
    if (currentIcon) DestroyIcon(currentIcon);
    CloseHandle(stopEvent); CloseHandle(pollEvent); CloseHandle(mutex);
    DeleteCriticalSection(&resultLock);
    return 0;
}
