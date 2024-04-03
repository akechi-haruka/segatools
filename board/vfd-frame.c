#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SUPER_VERBOSE 1

#include "board/vfd-frame.h"

#include "hook/iobuf.h"

#include "util/dprintf.h"

static HRESULT vfd_frame_encode_byte(struct iobuf *dest, uint8_t byte);

/* Frame structure:

    REQUEST:
   [0] Sync byte (0x1B)
   [1] Packet ID
   [2...n-1] Data/payload

   RESPONSE:
   This thing never responds, unless it's VFD_CMD_GET_VERSION
   */

void vfd_frame_sync(struct const_iobuf *src) {
    size_t i;

    for (i = 0 ; i < src->pos && (src->bytes[i] != VFD_SYNC_BYTE && src->bytes[i] != VFD_SYNC_BYTE2) ; i++);

    src->pos += i + 1;
    //memmove(&src->bytes[0], &src->bytes[i], i);
}

HRESULT vfd_frame_encode(
        struct iobuf *dest,
        const void *ptr,
        size_t nbytes)
{
    const uint8_t *src;
    uint8_t checksum;
    uint8_t byte;
    size_t i;
    HRESULT hr;

    assert(dest != NULL);
    assert(dest->bytes != NULL || dest->nbytes == 0);
    assert(dest->pos <= dest->nbytes);
    assert(ptr != NULL);

    src = ptr;

    if (dest->pos >= dest->nbytes) {
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

#if SUPER_VERBOSE
    dprintf("VFD: RX Buffer:\n");
#endif

    for (i = 1 ; i < nbytes ; i++) {
        byte = src[i];
        checksum += byte;
        #if SUPER_VERBOSE
        dprintf("%02x ", byte);
        #endif

        hr = vfd_frame_encode_byte(dest, byte);

        if (FAILED(hr)) {
            return hr;
        }
    }
    #if SUPER_VERBOSE
    dprintf("\n");
    #endif

    return vfd_frame_encode_byte(dest, checksum);
}

static HRESULT vfd_frame_encode_byte(struct iobuf *dest, uint8_t byte)
{
    if (dest->pos + 1 > dest->nbytes) {
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    dest->bytes[dest->pos++] = byte;

    return S_OK;
}
