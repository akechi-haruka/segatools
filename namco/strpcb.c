#include <windows.h>
#include <shlwapi.h>

#include <assert.h>
#include <process.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "hook/iobuf.h"
#include "hook/iohook.h"
#include "hook/hr.h"
#include "hook/table.h"

#include "hooklib/uart.h"

#include "namco/strpcb.h"

#include "util/dprintf.h"
#include "util/dump.h"


static BOOL WINAPI iohook_ReadFileEx(
        HANDLE hFile,
        LPVOID lpBuffer,
        DWORD nNumberOfBytesToRead,
        LPOVERLAPPED lpOverlapped,
        LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);

static BOOL (WINAPI *next_ReadFileEx)(
        HANDLE hFile,
        LPVOID lpBuffer,
        DWORD nNumberOfBytesToRead,
        LPOVERLAPPED lpOverlapped,
        LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);

static const struct hook_symbol iohook_kernel32_syms[] = {
    {
        .name   = "ReadFileEx",
        .patch  = iohook_ReadFileEx,
        .link   = (void *) &next_ReadFileEx,
    },
};

static HRESULT strpcb_handle_irp(struct irp *irp);
static HRESULT strpcb_handle_irp_locked(struct irp *irp);

typedef struct {
    CRITICAL_SECTION lock;
    struct uart boarduart;
    uint8_t written_bytes[4096];
    uint8_t readable_bytes[4096];
    bool enable_response;
} _strpcb_per_board_vars;

_strpcb_per_board_vars strpcb_per_board_vars;

static bool strpcb_requested_once = false;

HRESULT strpcb_hook_init()
{
    _strpcb_per_board_vars *v = &strpcb_per_board_vars;

    InitializeCriticalSection(&v->lock);

    uart_init(&v->boarduart, 1);
    v->boarduart.written.bytes = v->written_bytes;
    v->boarduart.written.nbytes = sizeof(v->written_bytes);
    v->boarduart.readable.bytes = v->readable_bytes;
    v->boarduart.readable.nbytes = sizeof(v->readable_bytes);

    v->enable_response = true;

    hook_table_apply(
                NULL,
                "kernel32.dll",
                iohook_kernel32_syms,
                _countof(iohook_kernel32_syms));

    dprintf("STRPCB: initialized\n");

    return iohook_push_handler(strpcb_handle_irp);
}

static BOOL WINAPI iohook_ReadFileEx(
        HANDLE hFile,
        LPVOID lpBuffer,
        DWORD nNumberOfBytesToRead,
        LPOVERLAPPED lpOverlapped,
        LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    struct irp irp;
    HRESULT hr;

    if (hFile == NULL || hFile == INVALID_HANDLE_VALUE || lpBuffer == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);

        return FALSE;
    }

    memset(&irp, 0, sizeof(irp));
    irp.op = IRP_OP_READ;
    irp.fd = hFile;
    irp.ovl = lpOverlapped;
    irp.read.bytes = lpBuffer;
    irp.read.nbytes = nNumberOfBytesToRead;
    irp.read.pos = 0;

    hr = iohook_invoke_next(&irp);

    if (FAILED(hr)) {
        return hr_propagate_win32(hr, FALSE);
    }

    assert(irp.read.pos <= irp.read.nbytes);

    (*lpCompletionRoutine)(ERROR_SUCCESS, irp.read.pos, lpOverlapped);

    SetLastError(ERROR_SUCCESS);
    return TRUE;
}

static HRESULT strpcb_handle_irp(struct irp *irp)
{
    HRESULT hr;

    assert(irp != NULL);

    _strpcb_per_board_vars *v = &strpcb_per_board_vars;
    struct uart *boarduart = &v->boarduart;

    if (uart_match_irp(boarduart, irp))
    {

        if (irp->op == IRP_OP_OPEN){
            dprintf("STRPCB: Opened handle\n");
        } else if (irp->op == IRP_OP_CLOSE){
            dprintf("STRPCB: Closed handle\n");
        }

        CRITICAL_SECTION lock = v->lock;

        EnterCriticalSection(&lock);
        hr = strpcb_handle_irp_locked(irp);
        LeaveCriticalSection(&lock);

        return hr;
    }

    return iohook_invoke_next(irp);
}

static HRESULT strpcb_handle_irp_locked(struct irp *irp)
{
    HRESULT hr;

    struct uart *boarduart = &strpcb_per_board_vars.boarduart;

    if (irp->op == IRP_OP_READ){
        if (!strpcb_requested_once){
            dprintf("STRPCB: Start updates\n");
            strpcb_requested_once = true;
        }
        struct iobuf* dest = &strpcb_per_board_vars.boarduart.readable;
        const char* str = "C01";
        iobuf_write(dest, str, strlen(str) + 1);
    }

    hr = uart_handle_irp(boarduart, irp);

    if (FAILED(hr) || irp->op != IRP_OP_WRITE) {
        return hr;
    }

    return E_NOTIMPL;
}

