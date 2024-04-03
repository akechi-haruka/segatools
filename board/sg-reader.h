#pragma once

#include <windows.h>

#include <stdbool.h>

#include "board/aime-dll.h"

struct aime_config {
    struct aime_dll_config dll;
    bool enable;
    bool high_baudrate;
    unsigned int gen;
};

HRESULT sg_reader_hook_init(
        const struct aime_config *cfg,
        unsigned int port_no,
        unsigned int gen,
        HINSTANCE self);
