/*
 * ScrenKey - Transparent key overlay using pure Win32 + GDI
 * Compile: cl screnkey.c /link user32.lib gdi32.lib
 *    or:   gcc screnkey.c -o screnkey.exe -lgdi32 -luser32 -mwindows
 */

#define WIN32_LEAN_AND_MEAN
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <stdio.h>

/* --- Config --- */
#define WIN_SIZE      120
#define PADDING       10
#define CORNER_RADIUS 12
#define FONT_SIZE     32
#define TRANS_COLOR   RGB(1, 1, 1)    /* color key for transparency */
#define BLOCK_COLOR   RGB(34, 34, 34)
#define BORDER_COLOR  RGB(68, 68, 68)
#define TEXT_COLOR    RGB(255, 255, 255)

static HWND   g_hwnd;
static HHOOK  g_hook;
static WCHAR  g_label[16] = L"";
static BOOL   g_dragging = FALSE;
static POINT  g_drag_origin;

/* ---- Map virtual key to a short display label ---- */
static void vk_to_label(DWORD vk, WCHAR *buf, int buflen)
{
    /* printable characters */
    if ((vk >= '0' && vk <= '9') || (vk >= 'A' && vk <= 'Z')) {
        buf[0] = (WCHAR)vk;
        buf[1] = 0;
        return;
    }

    /* OEM / punctuation via MapVirtualKey + ToUnicode */
    if (vk >= VK_OEM_1 && vk <= VK_OEM_102) {
        BYTE ks[256] = {0};
        UINT sc = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
        WCHAR ch[4];
        int n = ToUnicode(vk, sc, ks, ch, 4, 0);
        if (n == 1) {
            buf[0] = towupper(ch[0]);
            buf[1] = 0;
            return;
        }
    }

    const WCHAR *name = NULL;
    switch (vk) {
    case VK_SPACE:      name = L"SP";   break;
    case VK_RETURN:     name = L"ENT";  break;
    case VK_BACK:       name = L"BS";   break;
    case VK_TAB:        name = L"TAB";  break;
    case VK_ESCAPE:     name = L"ESC";  break;
    case VK_DELETE:     name = L"DEL";  break;
    case VK_INSERT:     name = L"INS";  break;
    case VK_HOME:       name = L"HOM";  break;
    case VK_END:        name = L"END";  break;
    case VK_PRIOR:      name = L"PU";   break;
    case VK_NEXT:       name = L"PD";   break;
    case VK_SHIFT:
    case VK_LSHIFT:
    case VK_RSHIFT:     name = L"SHF";  break;
    case VK_CONTROL:
    case VK_LCONTROL:
    case VK_RCONTROL:   name = L"CTL";  break;
    case VK_MENU:
    case VK_LMENU:
    case VK_RMENU:      name = L"ALT";  break;
    case VK_LWIN:
    case VK_RWIN:       name = L"WIN";  break;
    case VK_CAPITAL:    name = L"CAP";  break;
    case VK_UP:         name = L"\x2191"; break;  /* arrow symbols */
    case VK_DOWN:       name = L"\x2193"; break;
    case VK_LEFT:       name = L"\x2190"; break;
    case VK_RIGHT:      name = L"\x2192"; break;
    default:
        /* F-keys */
        if (vk >= VK_F1 && vk <= VK_F24) {
            swprintf(buf, buflen, L"F%d", vk - VK_F1 + 1);
            return;
        }
        /* numpad digits */
        if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) {
            buf[0] = L'0' + (WCHAR)(vk - VK_NUMPAD0);
            buf[1] = 0;
            return;
        }
        swprintf(buf, buflen, L"%02X", vk);
        return;
    }
    wcsncpy(buf, name, buflen);
    buf[buflen - 1] = 0;
}

/* ---- Paint ---- */
static void paint(HDC hdc)
{
    RECT rc = {0, 0, WIN_SIZE, WIN_SIZE};

    /* fill transparent background */
    HBRUSH bg = CreateSolidBrush(TRANS_COLOR);
    FillRect(hdc, &rc, bg);
    DeleteObject(bg);

    if (g_label[0] == 0) return;

    /* draw rounded block */
    HBRUSH block = CreateSolidBrush(BLOCK_COLOR);
    HPEN   pen   = CreatePen(PS_SOLID, 2, BORDER_COLOR);
    HGDIOBJ oldBr = SelectObject(hdc, block);
    HGDIOBJ oldPn = SelectObject(hdc, pen);
    RoundRect(hdc,
              PADDING, PADDING,
              WIN_SIZE - PADDING, WIN_SIZE - PADDING,
              CORNER_RADIUS * 2, CORNER_RADIUS * 2);
    SelectObject(hdc, oldBr);
    SelectObject(hdc, oldPn);
    DeleteObject(block);
    DeleteObject(pen);

    /* draw text */
    HFONT font = CreateFontW(
        FONT_SIZE, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_MODERN, L"Consolas");
    HGDIOBJ oldFont = SelectObject(hdc, font);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, TEXT_COLOR);

    RECT textRc = {PADDING, PADDING, WIN_SIZE - PADDING, WIN_SIZE - PADDING};
    DrawTextW(hdc, g_label, -1, &textRc,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);

    SelectObject(hdc, oldFont);
    DeleteObject(font);
}

/* ---- Low-level keyboard hook ---- */
static LRESULT CALLBACK keyboard_proc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT *kb = (KBDLLHOOKSTRUCT *)lParam;
        vk_to_label(kb->vkCode, g_label, 16);
        InvalidateRect(g_hwnd, NULL, TRUE);
    }
    return CallNextHookEx(g_hook, nCode, wParam, lParam);
}

/* ---- Window proc ---- */
static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        /* double buffer to avoid flicker */
        HDC mem = CreateCompatibleDC(hdc);
        HBITMAP bmp = CreateCompatibleBitmap(hdc, WIN_SIZE, WIN_SIZE);
        HGDIOBJ old = SelectObject(mem, bmp);
        paint(mem);
        BitBlt(hdc, 0, 0, WIN_SIZE, WIN_SIZE, mem, 0, 0, SRCCOPY);
        SelectObject(mem, old);
        DeleteObject(bmp);
        DeleteDC(mem);
        EndPaint(hwnd, &ps);
        return 0;
    }

    /* drag with left-click */
    case WM_LBUTTONDOWN:
        g_dragging = TRUE;
        g_drag_origin.x = LOWORD(lParam);
        g_drag_origin.y = HIWORD(lParam);
        SetCapture(hwnd);
        return 0;

    case WM_MOUSEMOVE:
        if (g_dragging) {
            POINT cursor;
            GetCursorPos(&cursor);
            SetWindowPos(hwnd, NULL,
                         cursor.x - g_drag_origin.x,
                         cursor.y - g_drag_origin.y,
                         0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        return 0;

    case WM_LBUTTONUP:
        g_dragging = FALSE;
        ReleaseCapture();
        return 0;

    /* right-click to close */
    case WM_RBUTTONUP:
        PostQuitMessage(0);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

/* ---- Entry point ---- */
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR cmdLine, int nShow)
{
    (void)hPrev; (void)cmdLine; (void)nShow;

    WNDCLASSW wc = {0};
    wc.lpfnWndProc   = wnd_proc;
    wc.hInstance      = hInst;
    wc.lpszClassName  = L"ScrenKey";
    wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    g_hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"ScrenKey", L"ScrenKey",
        WS_POPUP,
        50, 50, WIN_SIZE, WIN_SIZE,
        NULL, NULL, hInst, NULL);

    /* make TRANS_COLOR pixels fully transparent */
    SetLayeredWindowAttributes(g_hwnd, TRANS_COLOR, 0, LWA_COLORKEY);

    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);

    /* install global keyboard hook */
    g_hook = SetWindowsHookExW(WH_KEYBOARD_LL, keyboard_proc, hInst, 0);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    UnhookWindowsHookEx(g_hook);
    return 0;
}
