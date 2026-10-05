#include <windows.h>
#include <stdio.h>
#include "startup.h"

int main() {
    // Exercise the same functions used by the menu against the real per-user Run key.
    // Preserve the exact existing value (type and bytes), including another installation's path.
    const wchar_t* keyPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    const wchar_t* name = L"BasiliskBattery";
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, keyPath, 0, nullptr, 0, KEY_QUERY_VALUE | KEY_SET_VALUE,
        nullptr, &key, nullptr) != ERROR_SUCCESS) return 1;
    BYTE saved[65536] = {};
    DWORD bytes = sizeof(saved), type = 0;
    LSTATUS savedStatus = RegQueryValueExW(key, name, nullptr, &type, saved, &bytes);
    if (savedStatus != ERROR_SUCCESS && savedStatus != ERROR_FILE_NOT_FOUND) { RegCloseKey(key); return 1; }
    bool passed = startup::SetEnabled(true) && startup::Enabled();
    wchar_t command[2048] = {}, path[1024] = {}, expected[2048] = {};
    DWORD commandBytes = sizeof(command);
    GetModuleFileNameW(nullptr, path, _countof(path));
    swprintf_s(expected, L"\"%s\"", path);
    passed = RegQueryValueExW(key, name, nullptr, nullptr, (BYTE*)command, &commandBytes) == ERROR_SUCCESS &&
        wcscmp(command, expected) == 0 && passed;
    passed = startup::SetEnabled(false) && !startup::Enabled() && passed;
    passed = startup::SetEnabled(false) && passed;
    LSTATUS restored = savedStatus == ERROR_SUCCESS ? RegSetValueExW(key, name, 0, type, saved, bytes) :
        RegDeleteValueW(key, name);
    if (restored == ERROR_FILE_NOT_FOUND && savedStatus == ERROR_FILE_NOT_FOUND) restored = ERROR_SUCCESS;
    RegCloseKey(key);
    printf("Windows startup integration: %s; original preference %s\n", passed ? "passed" : "FAILED",
        restored == ERROR_SUCCESS ? "restored" : "RESTORE FAILED");
    return passed && restored == ERROR_SUCCESS ? 0 : 1;
}
