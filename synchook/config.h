#pragma once

#include <stddef.h>

#include "amex/config.h"

#include "board/config.h"

#include "hooklib/gfx.h"

#include "platform/config.h"

struct sync_hook_config {
    struct platform_config platform;
    struct aime_config aime;
    struct gpio_config gpio;
    struct gfx_config gfx;
};

void sync_hook_config_load(
        struct sync_hook_config *cfg,
        const wchar_t *filename);
