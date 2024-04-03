#pragma once

#include <stddef.h>

#include "amex/amex.h"

#include "board/sg-reader.h"

#include "carolhook/carol-dll.h"

#include "platform/platform.h"

#include "gfxhook/gfx.h"

#include "carolhook/touch.h"
#include "carolhook/controlbd.h"

struct carol_hook_config {
    struct platform_config platform;
    struct amex_config amex;
    struct aime_config aime;
    struct carol_dll_config dll;
    struct gfx_config gfx;
    struct touch_config touch;
    struct controlbd_config controlbd;
};

void carol_dll_config_load(
        struct carol_dll_config *cfg,
        const wchar_t *filename);
void carol_hook_config_load(
        struct carol_hook_config *cfg,
        const wchar_t *filename);
