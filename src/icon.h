#pragma once
#include <windows.h>
HICON BatteryIcon(int percent, int size);
bool SaveIcon(HICON icon, const wchar_t* path);
