#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "amex/amex.h"

#include "board/sg-reader.h"

#include "cxbhook/commio.h"

#include "hooklib/gfx.h"

#include "platform/platform.h"

struct cxb_hook_config {
    struct platform_config platform;
    struct amex_config amex;
    struct aime_config aime;
    struct gfx_config gfx;
    struct io_config io;
};

void cxb_hook_config_load(
        struct cxb_hook_config *cfg,
        const wchar_t *filename);
