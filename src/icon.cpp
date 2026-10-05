#include "icon.h"
#include "protocol.h"
#include <stdio.h>

HICON BatteryIcon(int percent, int size) {
    const int scale = 4, n = size * scale;
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = n;
    bi.bmiHeader.biHeight = -n;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    HDC dc = CreateCompatibleDC(nullptr);
    uint32_t* pixels = nullptr;
    HBITMAP high = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, (void**)&pixels, nullptr, 0);
    if (!dc || !high) { if (dc) DeleteDC(dc); return nullptr; }
    HGDIOBJ previous = SelectObject(dc, high);
    ZeroMemory(pixels, n * n * 4);
    COLORREF color = RGB(68, 214, 44);
    switch (battery::Color(percent)) {
    case battery::Tone::Red: color = RGB(255, 76, 76); break;
    case battery::Tone::Yellow: color = RGB(255, 205, 53); break;
    case battery::Tone::Unavailable: color = RGB(163, 171, 180); break;
    default: break;
    }
    // Horizontal battery leaves room for two readable digits at 16 logical pixels.
    const int left = n / 32, top = n * 2 / 16, right = n * 14 / 16, bottom = n * 14 / 16;
    HRGN body = CreateRoundRectRgn(left, top, right, bottom, scale * 2, scale * 2);
    HRGN terminal = CreateRectRgn(right - 1, n * 6 / 16, n - n / 32, n * 10 / 16);
    CombineRgn(body, body, terminal, RGN_OR);
    HBRUSH brush = CreateSolidBrush(color);
    FillRgn(dc, body, brush);
    DeleteObject(brush);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(15, 24, 14));
    if (battery::Full(percent)) {
        HPEN pen = CreatePen(PS_SOLID, n * 2 / 16, RGB(15, 24, 14));
        HGDIOBJ oldPen = SelectObject(dc, pen);
        MoveToEx(dc, n * 4 / 16, n * 8 / 16, nullptr);
        LineTo(dc, n * 6 / 16, n * 10 / 16);
        LineTo(dc, n * 11 / 16, n * 5 / 16);
        SelectObject(dc, oldPen);
        DeleteObject(pen);
    } else {
        wchar_t text[8] = L"?";
        if (percent >= 0) swprintf_s(text, L"%d", percent);
        HFONT font = CreateFontW(-n * 10 / 16, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
            DEFAULT_PITCH, L"Segoe UI");
        HGDIOBJ oldFont = SelectObject(dc, font);
        RECT rect = {n * 5 / 64, top, n * 53 / 64, bottom};
        DrawTextW(dc, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        SelectObject(dc, oldFont);
        DeleteObject(font);
    }
    GdiFlush();
    bi.bmiHeader.biWidth = size;
    bi.bmiHeader.biHeight = -size;
    uint32_t* output = nullptr;
    HBITMAP bitmap = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, (void**)&output, nullptr, 0);
    if (bitmap) {
        for (int y = 0; y < size; ++y) for (int x = 0; x < size; ++x) {
            unsigned a = 0, r = 0, g = 0, b = 0;
            for (int dy = 0; dy < scale; ++dy) for (int dx = 0; dx < scale; ++dx) {
                int sx = x * scale + dx, sy = y * scale + dy;
                if (PtInRegion(body, sx, sy)) {
                    uint32_t p = pixels[sy * n + sx];
                    a += 255; b += p & 255; g += (p >> 8) & 255; r += (p >> 16) & 255;
                }
            }
            output[y * size + x] = (a / 16 << 24) | (r / 16 << 16) | (g / 16 << 8) | b / 16;
        }
    }
    HBITMAP mask = CreateBitmap(size, size, 1, 1, nullptr);
    ICONINFO info = {TRUE, 0, 0, mask, bitmap};
    HICON icon = bitmap && mask ? CreateIconIndirect(&info) : nullptr;
    DeleteObject(mask);
    DeleteObject(bitmap);
    DeleteObject(body);
    DeleteObject(terminal);
    SelectObject(dc, previous);
    DeleteObject(high);
    DeleteDC(dc);
    return icon;
}

// Diagnostic export uses the same icon objects as the notification area.
bool SaveIcon(HICON icon, const wchar_t* path) {
    ICONINFO info = {};
    if (!GetIconInfo(icon, &info)) return false;
    BITMAP bm = {};
    GetObjectW(info.hbmColor, sizeof(bm), &bm);
    const DWORD colorBytes = bm.bmWidth * bm.bmHeight * 4;
    const DWORD maskBytes = ((bm.bmWidth + 31) / 32) * 4 * bm.bmHeight;
    const DWORD bytes = sizeof(BITMAPINFOHEADER) + colorBytes + maskBytes;
    BYTE* data = (BYTE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bytes);
    bool ok = false;
    if (data) {
        BITMAPINFO* bi = (BITMAPINFO*)data;
        bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi->bmiHeader.biWidth = bm.bmWidth;
        bi->bmiHeader.biHeight = bm.bmHeight;
        bi->bmiHeader.biPlanes = 1;
        bi->bmiHeader.biBitCount = 32;
        HDC dc = GetDC(nullptr);
        GetDIBits(dc, info.hbmColor, 0, bm.bmHeight, data + sizeof(BITMAPINFOHEADER), bi, DIB_RGB_COLORS);
        ReleaseDC(nullptr, dc);
        bi->bmiHeader.biHeight *= 2;
        // ICO stores straight alpha; CreateIconIndirect consumes premultiplied pixels.
        uint32_t* p = (uint32_t*)(data + sizeof(BITMAPINFOHEADER));
        for (int i = 0; i < bm.bmWidth * bm.bmHeight; ++i) {
            unsigned a = p[i] >> 24;
            if (a) p[i] = (a << 24) | ((((p[i] >> 16) & 255) * 255 / a) << 16) |
                ((((p[i] >> 8) & 255) * 255 / a) << 8) | ((p[i] & 255) * 255 / a);
        }
        BYTE header[22] = {0,0,1,0,1,0};
        header[6] = (BYTE)bm.bmWidth; header[7] = (BYTE)bm.bmHeight;
        header[10] = 1; header[12] = 32;
        CopyMemory(header + 14, &bytes, 4);
        DWORD offset = 22; CopyMemory(header + 18, &offset, 4);
        HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file != INVALID_HANDLE_VALUE) {
            DWORD written = 0;
            ok = WriteFile(file, header, sizeof(header), &written, nullptr) && written == sizeof(header);
            ok = ok && WriteFile(file, data, bytes, &written, nullptr) && written == bytes;
            CloseHandle(file);
        }
        HeapFree(GetProcessHeap(), 0, data);
    }
    DeleteObject(info.hbmColor); DeleteObject(info.hbmMask);
    return ok;
}
