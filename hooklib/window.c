#include "hook/table.h"

#include "hooklib/config.h"
#include "hooklib/window.h"

#include "util/dprintf.h"
#include "util/str.h"

static struct window_config config;

HWND __stdcall hook_CreateWindowExW(
   DWORD     dwExStyle,
  LPCWSTR   lpClassName,
  LPCWSTR   lpWindowName,
   DWORD     dwStyle,
   int       X,
   int       Y,
   int       nWidth,
   int       nHeight,
  HWND      hWndParent,
  HMENU     hMenu,
  HINSTANCE hInstance,
  LPVOID    lpParam
);
HWND (__stdcall *next_CreateWindowExW)(
   DWORD     dwExStyle,
  LPCWSTR   lpClassName,
  LPCWSTR   lpWindowName,
   DWORD     dwStyle,
   int       X,
   int       Y,
   int       nWidth,
   int       nHeight,
  HWND      hWndParent,
  HMENU     hMenu,
  HINSTANCE hInstance,
  LPVOID    lpParam
);
BOOL __stdcall hook_SetWindowPos(
             HWND hWnd,
   HWND hWndInsertAfter,
             int  X,
             int  Y,
             int  cx,
             int  cy,
             UINT uFlags
);
BOOL __stdcall (*next_SetWindowPos)(
             HWND hWnd,
   HWND hWndInsertAfter,
             int  X,
             int  Y,
             int  cx,
             int  cy,
             UINT uFlags
);
BOOL __stdcall hook_AdjustWindowRect(
   LPRECT lpRect,
        DWORD  dwStyle,
        BOOL   bMenu
);
BOOL __stdcall (*next_AdjustWindowRect)(
   LPRECT lpRect,
        DWORD  dwStyle,
        BOOL   bMenu
);


static const struct hook_symbol syms[] = {
    {
        .name   = "CreateWindowExW",
        .patch  = hook_CreateWindowExW,
        .link   = (void **) &next_CreateWindowExW,
    },
    {
        .name   = "SetWindowPos",
        .patch  = hook_SetWindowPos,
        .link   = (void **) &next_SetWindowPos,
    },
    {
        .name   = "AdjustWindowRect",
        .patch  = hook_AdjustWindowRect,
        .link   = (void **) &next_AdjustWindowRect,
    },
};

static const wchar_t* wnd_name;

void window_hook_init(const struct window_config *cfg, const wchar_t *wnd){
    assert(cfg != NULL);
    assert(wnd != NULL);

    config = *cfg;
    wnd_name = wnd;

    if (config.enable){

        hook_table_apply(
            NULL,
            "user32.dll",
            syms,
            _countof(syms));

        dprintf("Wndhook: enabled\n");
    }
}

HWND __stdcall hook_CreateWindowExW(
   DWORD     dwExStyle,
  LPCWSTR   lpClassName,
  LPCWSTR   lpWindowName,
   DWORD     dwStyle,
   int       X,
   int       Y,
   int       nWidth,
   int       nHeight,
  HWND      hWndParent,
  HMENU     hMenu,
  HINSTANCE hInstance,
  LPVOID    lpParam
){

    dprintf("Wndhook: CreateWindowExW: %ls/%ls\n", lpClassName, lpWindowName);
    if (wstr_eq(lpWindowName, wnd_name)){
        dprintf("Wndhook: Found tracked window\n");
        int nw = config.width;
        int nh = config.height;
        dprintf("Wndhook: Resize %dx%d -> %dx%d\n", nWidth, nHeight, nw, nh);
        nWidth = nw;
        nHeight = nh;
    }

    return next_CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

}

BOOL __stdcall hook_SetWindowPos(
             HWND hWnd,
   HWND hWndInsertAfter,
             int  X,
             int  Y,
             int  cx,
             int  cy,
             UINT uFlags
){

    dprintf("Wndhook: SetWindowPos ignored\n");
    return TRUE;
}

BOOL __stdcall hook_AdjustWindowRect(
   LPRECT lpRect,
        DWORD  dwStyle,
        BOOL   bMenu
){
    int nw = config.width;
    int nh = config.height;
    lpRect->right = nw;
    lpRect->bottom = nh;
    dprintf("Wndhook: Manipulated AdjustWindowRect\n");
    return TRUE;
}
