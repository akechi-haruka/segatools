#pragma once

#include <windows.h>

#include "gfxhook/gfx.h"

void gfx_dxgi_hook_init(const struct gfx_config *cfg, HINSTANCE self);
