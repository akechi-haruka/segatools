#pragma once

#include <windows.h>

#include <stddef.h>
#include <stdint.h>

#include "hook/iobuf.h"

enum {
    RFID_FRAME_SYNC = 0xE0,
};

struct rfid_hdr {
    uint8_t sync;
};

HRESULT rfid_frame_decode(struct iobuf *dest, struct iobuf *src);

HRESULT rfid_frame_encode(
        struct iobuf *dest,
        const void *ptr,
        size_t nbytes);

HRESULT rfid_frame_encode_no_checksum(
        struct iobuf *dest,
        const void *ptr,
        size_t nbytes);
