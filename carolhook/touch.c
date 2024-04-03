#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "carolhook/carol-dll.h"
#include "carolhook/touch.h"

#include "hooklib/uart.h"

#include "util/dprintf.h"
#include "util/dump.h"

static HRESULT touch_handle_irp(struct irp *irp);
static HRESULT touch_handle_irp_locked(struct irp *irp);
static HRESULT touch_frame_decode(struct touch_req *dest, struct iobuf *iobuf);

static CRITICAL_SECTION touch_lock;
static struct uart touch_uart;
static uint8_t touch_written_bytes[520];
static uint8_t touch_readable_bytes[520];

HRESULT touch_hook_init(const struct touch_config *cfg)
{
    if (!cfg->enable) {
        return S_OK;
    }
    
    InitializeCriticalSection(&touch_lock);

    uart_init(&touch_uart, 1);
    touch_uart.written.bytes = touch_written_bytes;
    touch_uart.written.nbytes = sizeof(touch_written_bytes);
    touch_uart.readable.bytes = touch_readable_bytes;
    touch_uart.readable.nbytes = sizeof(touch_readable_bytes);

    dprintf("Touchscreen: Init\n");

    return iohook_push_handler(touch_handle_irp);
    return S_OK;
}

static HRESULT touch_handle_irp(struct irp *irp)
{
    HRESULT hr;

    assert(irp != NULL);

    if (!uart_match_irp(&touch_uart, irp)) {
        return iohook_invoke_next(irp);
    }

    EnterCriticalSection(&touch_lock);
    hr = touch_handle_irp_locked(irp);
    LeaveCriticalSection(&touch_lock);

    return hr;
}

static HRESULT touch_handle_irp_locked(struct irp *irp)
{
    struct touch_req req;
    HRESULT hr;

    assert(carol_dll.touch_init != NULL);

    if (irp->op == IRP_OP_OPEN) {
        dprintf("Touchscreen: Starting backend DLL\n");
        hr = carol_dll.touch_init();

        if (FAILED(hr)) {
            dprintf("Touchscreen: Backend DLL error: %x\n", (int) hr);

            return hr;
        }
    }

    hr = uart_handle_irp(&touch_uart, irp);

    if (FAILED(hr) || irp->op != IRP_OP_WRITE) {
        return hr;
    }

    for (;;) {
#if 1
        dprintf("Touchscreen: TX Buffer:\n");
        dump_iobuf(&touch_uart.written);
#endif
        hr = touch_frame_decode(&req, &touch_uart.written);

        if (FAILED(hr)) {
            dprintf("Touchscreen: Deframe Error: %x\n", (int) hr);

            return hr;
        }

        return hr;
    }
}

/* Decodes the response into a struct that's easier to work with. */
static HRESULT touch_frame_decode(struct touch_req *dest, struct iobuf *iobuf)
{
    dest->cmd = iobuf->bytes[0];
    iobuf->pos--;
    dest->data_length = iobuf->pos;

    if (dest->data_length > 0) {
        for (int i = 1; i < dest->data_length; i++) {
            dest->data[i-1] = iobuf->bytes[i];
        }
    }
    iobuf->pos -= dest->data_length;

    return S_OK;
}