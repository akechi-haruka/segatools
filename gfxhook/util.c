#include <windows.h>

#include "gfxhook/util.h"

#include "util/dprintf.h"

void gfx_util_ensure_win_visible(HWND hwnd)
{
    /*
     * Ensure window is maximized to avoid a Windows 10 issue where a
     * fullscreen swap chain is not created because the window is minimized
     * at the time of creation.
     */
    ShowWindow(hwnd, SW_RESTORE);
}

void gfx_util_borderless_fullscreen_windowed(HWND hwnd, UINT width, UINT height)
{
    BOOL ok;
    HRESULT hr;

    dprintf("Gfx: Resizing window to %ux%u\n", width, height);

    SetWindowLongPtrW(hwnd, GWL_STYLE, WS_POPUP);
    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, WS_EX_TOPMOST);

    ok = SetWindowPos(
            hwnd,
            HWND_TOP,
            0,
            0,
            (int) width,
            (int) height,
            SWP_FRAMECHANGED | SWP_NOSENDCHANGING);

    if (!ok) {
        /* come on... */
        hr = HRESULT_FROM_WIN32(GetLastError());
        dprintf("Gfx: SetWindowPos failed: %x\n", (int) hr);

        return;
    }

    ok = ShowWindow(hwnd, SW_SHOWMAXIMIZED);

    if (!ok) {
        /* come on... */
        hr = HRESULT_FROM_WIN32(GetLastError());
        dprintf("Gfx: ShowWindow failed: %x\n", (int) hr);

        return;
    }
}

HRESULT gfx_util_frame_window(HWND hwnd)
{
    HRESULT hr;
    DWORD error;
    LONG style;
    RECT rect;
    BOOL ok;

    SetLastError(ERROR_SUCCESS);
    style = GetWindowLongW(hwnd, GWL_STYLE);
    error = GetLastError();

    if (error != ERROR_SUCCESS) {
        hr = HRESULT_FROM_WIN32(error);
        dprintf("Gfx: GetWindowLongPtrW(%p, GWL_STYLE) failed: %x\n",
                hwnd,
                (int) hr);

        return hr;
    }

    ok = GetClientRect(hwnd, &rect);

    if (!ok) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        dprintf("Gfx: GetClientRect(%p) failed: %x\n", hwnd, (int) hr);

        return hr;
    }

    style |= WS_BORDER | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;
    ok = AdjustWindowRect(&rect, style, FALSE);

    if (!ok) {
        /* come on... */
        hr = HRESULT_FROM_WIN32(GetLastError());
        dprintf("Gfx: AdjustWindowRect failed: %x\n", (int) hr);

        return hr;
    }

    /* This... always seems to set an error, even though it works? idk */
    SetWindowLongW(hwnd, GWL_STYLE, style);

    ok = SetWindowPos(
            hwnd,
            HWND_TOP,
            rect.left,
            rect.top,
            rect.right - rect.left,
            rect.bottom - rect.top,
            SWP_FRAMECHANGED | SWP_NOMOVE);

    if (!ok) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        dprintf("Gfx: SetWindowPos(%p) failed: %x\n", hwnd, (int) hr);

        return hr;
    }

    return S_OK;
}
