#include <windows.h>
#include <stdio.h>
#include "startup.h"

namespace startup {
namespace {
const wchar_t* RunKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
const wchar_t* RunName = L"BasiliskBattery";
bool Command(wchar_t* command, size_t capacity) {
    wchar_t path[1024];
    DWORD n = GetModuleFileNameW(nullptr, path, _countof(path));
    if (!n || n >= _countof(path)) return false;
    return swprintf_s(command, capacity, L"\"%s\"", path) > 0;
}
}
bool Enabled() {
    wchar_t actual[2048] = {}, expected[2048] = {};
    DWORD bytes = sizeof(actual), type = 0;
    return Command(expected, _countof(expected)) &&
        RegGetValueW(HKEY_CURRENT_USER, RunKey, RunName, RRF_RT_REG_SZ, &type, actual, &bytes) == ERROR_SUCCESS &&
        _wcsicmp(actual, expected) == 0;
}
bool SetEnabled(bool enabled) {
    HKEY key;
    LSTATUS status = RegCreateKeyExW(HKEY_CURRENT_USER, RunKey, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr);
    if (status != ERROR_SUCCESS) return false;
    if (enabled) {
        wchar_t command[2048];
        status = Command(command, _countof(command)) ?
            RegSetValueExW(key, RunName, 0, REG_SZ, (BYTE*)command, (DWORD)((wcslen(command) + 1) * sizeof(wchar_t))) : ERROR_BAD_PATHNAME;
    } else {
        status = RegDeleteValueW(key, RunName);
        if (status == ERROR_FILE_NOT_FOUND) status = ERROR_SUCCESS;
    }
    RegCloseKey(key);
    return status == ERROR_SUCCESS && Enabled() == enabled;
}
}
