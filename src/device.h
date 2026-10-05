#pragma once
#include <windows.h>
struct Reading {
    int percent = -1;
    int charging = -1;
    WORD pid = 0;
    DWORD error = 0;
    wchar_t status[160] = L"Procurando mouse...";
};
class Device {
    wchar_t path_[1024] = {};
    WORD pid_ = 0;
    bool Query(HANDLE handle, BYTE command, BYTE& value, HANDLE stop, DWORD& error);
    bool ReadPath(const wchar_t* path, WORD pid, Reading& reading, HANDLE stop);
public:
    void Reset() { path_[0] = 0; pid_ = 0; }
    Reading Read(HANDLE stop);
};
