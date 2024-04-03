#include <windows.h>
#include <winbase.h>

#include "hook/table.h"

#include "util/dprintf.h"

static BOOL WINAPI my_SetCommState(HANDLE hFile,  LPDCB lpDCB);
static BOOL (WINAPI *next_SetCommState)(HANDLE hFile,  LPDCB lpDCB);
static void com_hook_insert_hooks(HMODULE target);

static const struct hook_symbol win32_hooks[] = {
    {
        .name = "SetCommState",
        .patch = my_SetCommState,
        .link = (void **) &next_SetCommState
    }
};

void serial_init()
{
    com_hook_insert_hooks(NULL);
    dprintf("Serial: Spy init\n");
}

static void com_hook_insert_hooks(HMODULE target)
{
    hook_table_apply(
            target,
            "kernel32.dll",
            win32_hooks,
            _countof(win32_hooks));
}

static BOOL WINAPI my_SetCommState(HANDLE hFile,  LPDCB lpDCB)
{
    dprintf("Serial: my_SetCommState with baudrate %ld\n", lpDCB->BaudRate);
    return next_SetCommState(hFile, lpDCB);
}