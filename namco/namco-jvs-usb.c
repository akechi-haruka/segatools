#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#include <winternl.h>

#include <ntstatus.h>

#include <assert.h>
#include <stddef.h>

#include "hook/iobuf.h"
#include "hook/iohook.h"

#include "hooklib/setupapi.h"
#include "hooklib/uart.h"

#include "jvs/jvs-bus.h"

#include "namco/namco-jvs-usb.h"

#include "util/dprintf.h"
#include "util/dump.h"
#include "util/str.h"

typedef struct {
    CRITICAL_SECTION lock;
    struct uart boarduart;
    uint8_t written_bytes[4096];
    uint8_t readable_bytes[4096];
    bool enable_response;
} _namco3_per_board_vars;

_namco3_per_board_vars namco3_per_board_vars;

static HRESULT jvs_handle_irp(struct irp *irp);
static HRESULT jvs_handle_irp_locked(struct irp *irp);
static HRESULT jvs_handle_open(struct irp *irp);
static HRESULT jvs_handle_close(struct irp *irp);

static HRESULT jvs_ioctl_transact(struct uart *uart);

static HANDLE jvs_fd;
static struct jvs_node *jvs_root;
static jvs_provider_t jvs_provider;

HRESULT jvs_hook_init(jvs_provider_t provider)
{
    HRESULT hr = setupapi_add_phantom_dev(&jvs_guid, L"$jvs");

    if (FAILED(hr)) {
        return hr;
    }

    jvs_provider = provider;

    _namco3_per_board_vars *v = &namco3_per_board_vars;

    InitializeCriticalSection(&v->lock);

    uart_init(&v->boarduart, 3);
    v->boarduart.written.bytes = v->written_bytes;
    v->boarduart.written.nbytes = sizeof(v->written_bytes);
    v->boarduart.readable.bytes = v->readable_bytes;
    v->boarduart.readable.nbytes = sizeof(v->readable_bytes);
    v->enable_response = true;

    LeaveCriticalSection(&v->lock);

    dprintf("NAJV: initialized\n");

    return iohook_push_handler(jvs_handle_irp);
}

static HRESULT jvs_handle_irp(struct irp *irp)
{
    _namco3_per_board_vars *v = &namco3_per_board_vars;
    struct uart *boarduart = &v->boarduart;

    if (uart_match_irp(boarduart, irp)) {
        //dprintf("NAJV: IRP: %d, %p\n", irp->op, boarduart->fd);

        CRITICAL_SECTION lock = v->lock;

        EnterCriticalSection(&lock);
        HRESULT hr = jvs_handle_irp_locked(irp);
        LeaveCriticalSection(&lock);

        if (irp->op == IRP_OP_OPEN && !FAILED(hr)){
            dprintf("NAJV: NAJV->JVS link established: %p\n", irp->fd);
            boarduart->fd = irp->fd;
        }
        return hr;
    }

    return iohook_invoke_next(irp);
}

static HRESULT jvs_handle_irp_locked(struct irp *irp)
{
    assert(irp != NULL);

    struct uart *boarduart = &namco3_per_board_vars.boarduart;

    HRESULT uarthr = uart_handle_irp(boarduart, irp);

    if (FAILED(uarthr)) {
        dprintf("uart_handle_irp failed: %d, %lx\n", irp->op, uarthr);
        return uarthr;
    }

    switch (irp->op) {
        case IRP_OP_OPEN:   return jvs_handle_open(irp);
        case IRP_OP_CLOSE:  return jvs_handle_close(irp);
        case IRP_OP_WRITE:
            ;
            HRESULT hr = jvs_ioctl_transact(boarduart);
            //dprintf("transact result: %lx\n", hr);
            return hr;
        case IRP_OP_READ:
            //dprintf("read result: %lx N=%d\n", hr, boarduart->readable.pos);
            //dump_iobuf(&boarduart->readable);
            return S_OK;
        default:            return uarthr;
    }
}

static HRESULT jvs_handle_open(struct irp *irp)
{
    struct jvs_node *root;
    HRESULT hr;

    if (jvs_fd != NULL) {
        dprintf("JVS Port: Already open\n");

        return HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION);
    }

    hr = iohook_open_nul_fd(&jvs_fd);

    if (FAILED(hr)) {
        dprintf("JVS Port: Open failed: %lx\n", hr);
        return hr;
    }

    dprintf("JVS Port: Open device (%p <- %p)\n", irp->fd, jvs_fd);

    if (jvs_provider != NULL) {
        hr = jvs_provider(&root);

        if (SUCCEEDED(hr)) {
            jvs_root = root;
        } else {
            dprintf("JVS Port: Open failed: %lx\n", hr);
        }
    }

    irp->fd = jvs_fd;

    return S_OK;
}

static HRESULT jvs_handle_close(struct irp *irp)
{
    dprintf("JVS Port: Close device\n");
    jvs_fd = NULL;

    return iohook_invoke_next(irp);
}

static HRESULT jvs_ioctl_transact(struct uart *uart)
{
#if 0
    dprintf("\nJVS Port: From game frame (N:%d):\n", uart->written.pos);
    dump_iobuf(&uart->written);
#endif

    jvs_bus_transact(jvs_root, uart->written.bytes, uart->written.pos, &uart->readable);

#if 0
    dprintf("JVS Port: To game frame:\n");
    dump_iobuf(&uart->readable);
    dprintf("\n");
#endif

    uart->written.pos = 0;

    if (uart->readable.pos == 0) {
        /* The un-acked JVS reset command must return ERROR_NO_DATA_DETECTED,
           and this error must always be returned asynchronously. And since
           async I/O comes from the NT kernel, we have to return that win32
           error as the equivalent NTSTATUS. */

        return HRESULT_FROM_WIN32(ERROR_NO_DATA_DETECTED);
    } else {
        return S_OK;
    }
}
