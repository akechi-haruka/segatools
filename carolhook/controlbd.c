#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "hook/iobuf.h"
#include "hook/iohook.h"

#include "carolhook/carol-dll.h"
#include "carolhook/controlbd.h"

#include "hooklib/uart.h"

#include "util/dprintf.h"
#include "util/dump.h"

static HRESULT controlbd_handle_irp(struct irp *irp);
static HRESULT controlbd_handle_irp_locked(struct irp *irp);
static HRESULT controlbd_frame_decode(struct controlbd_req *dest, struct iobuf *iobuf);
static HRESULT controlbd_frame_dispatch(struct controlbd_req *dest);

static HRESULT controlbd_req_nop(uint8_t cmd);

static CRITICAL_SECTION controlbd_lock;
static struct uart controlbd_uart;
static uint8_t controlbd_written_bytes[520];
static uint8_t controlbd_readable_bytes[520];

HRESULT controlbd_hook_init(const struct controlbd_config *cfg)
{
    if (!cfg->enable) {
        return S_OK;
    }

    InitializeCriticalSection(&controlbd_lock);

    uart_init(&controlbd_uart, 11);
    controlbd_uart.written.bytes = controlbd_written_bytes;
    controlbd_uart.written.nbytes = sizeof(controlbd_written_bytes);
    controlbd_uart.readable.bytes = controlbd_readable_bytes;
    controlbd_uart.readable.nbytes = sizeof(controlbd_readable_bytes);

    dprintf("Control Board: Init\n");

    return iohook_push_handler(controlbd_handle_irp);
}

static HRESULT controlbd_handle_irp(struct irp *irp)
{
    HRESULT hr;

    assert(irp != NULL);

    if (!uart_match_irp(&controlbd_uart, irp)) {
        return iohook_invoke_next(irp);
    }

    EnterCriticalSection(&controlbd_lock);
    hr = controlbd_handle_irp_locked(irp);
    LeaveCriticalSection(&controlbd_lock);

    return hr;
}

static HRESULT controlbd_handle_irp_locked(struct irp *irp)
{
    struct controlbd_req req;
    HRESULT hr;

    assert(carol_dll.controlbd_init != NULL);

    if (irp->op == IRP_OP_OPEN) {
        dprintf("Control Board: Starting backend DLL\n");
        hr = carol_dll.controlbd_init();

        if (FAILED(hr)) {
            dprintf("Control Board: Backend DLL error: %x\n", (int) hr);

            return hr;
        }
    }

    hr = uart_handle_irp(&controlbd_uart, irp);

    if (FAILED(hr) || irp->op != IRP_OP_WRITE) {
        return hr;
    }

    for (;;) {
#if 0
        dprintf("Control Board: TX Buffer:\n");
        dump_iobuf(&controlbd_uart.written);
#endif
        hr = controlbd_frame_decode(&req, &controlbd_uart.written);

        if (FAILED(hr)) {
            dprintf("Control Board: Deframe Error: %x\n", (int) hr);

            return hr;
        }

        hr = controlbd_frame_dispatch(&req);
        if (FAILED(hr)) {
            dprintf("Control Board: Dispatch Error: %x\n", (int) hr);

            return hr;
        }

        return hr;
    }
}

static HRESULT controlbd_frame_dispatch(struct controlbd_req *req)
{
    switch (req->cmd) {
    case CONTROLBD_CMD_UNK_11:
        return controlbd_req_nop(req->cmd);
    default:
        dprintf("Unhandled command %#02x\n", req->cmd);

        return S_OK;
    }
}

static HRESULT controlbd_req_nop(uint8_t cmd)
{
    dprintf("Control Board: No-op cmd %#02x\n", cmd);

    controlbd_uart.readable.bytes[controlbd_uart.readable.pos++] = 0xE0;
    controlbd_uart.readable.bytes[controlbd_uart.readable.pos++] = 0x01;
    controlbd_uart.readable.bytes[controlbd_uart.readable.pos++] = 0x11;
    controlbd_uart.readable.bytes[controlbd_uart.readable.pos++] = 0x03;
    controlbd_uart.readable.bytes[controlbd_uart.readable.pos++] = 0x01;
    controlbd_uart.readable.bytes[controlbd_uart.readable.pos++] = 0x10;
    controlbd_uart.readable.bytes[controlbd_uart.readable.pos++] = 0x01;
    controlbd_uart.readable.bytes[controlbd_uart.readable.pos++] = 0x27;

    return S_OK;
}

/* Decodes the response into a struct that's easier to work with. */
static HRESULT controlbd_frame_decode(struct controlbd_req *dest, struct iobuf *iobuf)
{
    int initial_pos = iobuf->pos;
    uint8_t check = 0;

    dest->sync = iobuf->bytes[0];
    iobuf->pos--;

    dest->cmd = iobuf->bytes[1];
    iobuf->pos--;
    check += dest->cmd;

    dest->checksum = iobuf->bytes[initial_pos - 1];
    iobuf->pos--;

    dest->data_length = initial_pos - 3; // sync, cmd, checksum
    if (dest->data_length > 0) {
        for (int i = 0; i < dest->data_length; i++) {
            dest->data[i] = iobuf->bytes[i+2];
            check += dest->data[i];
        }
    }
    iobuf->pos -= dest->data_length;

    if (dest->sync != 0xe0) {
        dprintf("Control Board: Sync error, expected 0xe0, got %x\n", dest->sync);
        return E_FAIL;
    }
    if (dest->checksum != check) {
        dprintf("Control Board: Checksum error, expected %x, got %x\n", check, dest->checksum);
        return E_FAIL;
    }

    return S_OK;
}