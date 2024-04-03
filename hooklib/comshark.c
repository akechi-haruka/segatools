#include <windows.h>
#include <shlwapi.h>

#include <assert.h>
#include <process.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "hook/iobuf.h"
#include "hook/iohook.h"

#include "hooklib/uart.h"

#include "util/dprintf.h"
#include "util/dump.h"

static HRESULT com_handle_irp(struct irp *irp);
static HRESULT com_handle_irp_locked(int board, struct irp *irp);

static int portlen;

typedef struct {
    CRITICAL_SECTION lock;
    struct uart boarduart;
    uint8_t written_bytes[520];
    uint8_t readable_bytes[520];
    bool enable_response;
    int port;
} _com_per_board_vars;

_com_per_board_vars com_per_board_vars[99];

HRESULT com_shark_init(int* ports, int len)
{

    portlen = len;
    for (int i = 0; i < len; i++)
    {
        _com_per_board_vars *v = &com_per_board_vars[i];

        InitializeCriticalSection(&v->lock);

        if (ports[i] <= 0){
            continue;
        }

        uart_init(&v->boarduart, ports[i]);
        v->boarduart.written.bytes = v->written_bytes;
        v->boarduart.written.nbytes = sizeof(v->written_bytes);
        v->boarduart.readable.bytes = v->readable_bytes;
        v->boarduart.readable.nbytes = sizeof(v->readable_bytes);
        v->port = ports[i];
        v->enable_response = true;
        dprintf("COMSHARK: initialized port %d\n", v->port);
    }

    return iohook_push_handler(com_handle_irp);
}

static HRESULT com_handle_irp(struct irp *irp)
{
    HRESULT hr;

    assert(irp != NULL);

    for (int i = 0; i < portlen; i++)
    {
        _com_per_board_vars *v = &com_per_board_vars[i];
        struct uart *boarduart = &v->boarduart;

        if (uart_match_irp(boarduart, irp))
        {

            CRITICAL_SECTION lock = v->lock;

            EnterCriticalSection(&lock);
            hr = com_handle_irp_locked(i, irp);
            LeaveCriticalSection(&lock);

            return hr;
        }
    }

    return iohook_invoke_next(irp);
}

static HRESULT com_handle_irp_locked(int board, struct irp *irp)
{
    HRESULT hr;

    struct uart *boarduart = &com_per_board_vars[board].boarduart;

    hr = uart_handle_irp(boarduart, irp);

    if (irp->op == IRP_OP_OPEN){
        dprintf("COMShark(%d): Open\n", boarduart->port_no);
    } else if (irp->op == IRP_OP_CLOSE){
        dprintf("COMShark(%d): Close\n", boarduart->port_no);
    }

    if (FAILED(hr) || irp->op != IRP_OP_WRITE) {
        return hr;
    }

    for (;;) {

        if (&boarduart->written.nbytes == 0){
            continue;
        }

        dprintf("COMShark(%d): TX Buffer:\n", boarduart->port_no);
        dump_iobuf(&boarduart->written);
    }
}
