#pragma once

#include <stddef.h>

#include "amex/config.h"

#include "board/config.h"
#include "board/vfd.h"

#include "hooklib/dvd.h"
#include "hooklib/gfx.h"

#include "mu3hook/mu3-dll.h"

#include "platform/config.h"

struct mu3_hook_config {
    struct platform_config platform;
    struct aime_config aime;
    struct dvd_config dvd;
    struct io4_config io4;
    struct gpio_config gpio;
    struct gfx_config gfx;
    struct vfd_config vfd;
    struct mu3_dll_config dll;
};

void mu3_dll_config_load(
        struct mu3_dll_config *cfg,
        const wchar_t *filename);

void mu3_hook_config_load(
        struct mu3_hook_config *cfg,
        const wchar_t *filename);
