#pragma once

#include <stddef.h>

#include "board/config.h"
#include "board/vfd.h"

#include "hooklib/dvd.h"
#include "hooklib/gfx.h"

#include "platform/config.h"

#include "chusanhook/chuni-dll.h"
#include "chusanhook/slider.h"
#include "chunihook/led1509306.h"

struct chusan_hook_config {
    struct platform_config platform;
    struct aime_config aime;
    struct io4_config io4;
    struct gfx_config gfx;
    struct vfd_config vfd;
    struct chuni_dll_config dll;
    struct dvd_config dvd;
    struct slider_config slider;
    struct led1509306_config led1509306;
};

void chuni_dll_config_load(
        struct chuni_dll_config *cfg,
        const wchar_t *filename);
void slider_config_load(struct slider_config *cfg, const wchar_t *filename);
void chusan_hook_config_load(
        struct chusan_hook_config *cfg,
        const wchar_t *filename);
