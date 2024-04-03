#pragma once

#include <windows.h>

#include <stddef.h>
#include <stdint.h>

#include "hook/iobuf.h"

enum {
    VFD_SYNC_BYTE = 0x1B,
    VFD_SYNC_BYTE2 = 0x1A,
};

void vfd_frame_sync(struct const_iobuf *src);

HRESULT vfd_frame_encode(
        struct iobuf *dest,
        const void *ptr,
        size_t nbytes);
