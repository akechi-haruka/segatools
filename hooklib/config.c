#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include "hooklib/config.h"
#include "hooklib/gfx.h"
#include "hooklib/dvd.h"
#include "hooklib/touch.h"
#include "hooklib/window.h"


void gfx_config_load(struct gfx_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"gfx", L"enable", 1, filename);
    cfg->windowed = GetPrivateProfileIntW(L"gfx", L"windowed", 0, filename);
    cfg->framed = GetPrivateProfileIntW(L"gfx", L"framed", 1, filename);
    cfg->monitor = GetPrivateProfileIntW(L"gfx", L"monitor", 0, filename);
    cfg->stretch = GetPrivateProfileIntW(L"gfx", L"stretch", 0, filename);
    cfg->width = GetPrivateProfileIntW(L"window", L"width", 0, filename);
    cfg->height = GetPrivateProfileIntW(L"window", L"height", 0, filename);
    cfg->dpi = GetPrivateProfileIntW(L"window", L"dpiaware", 1, filename);
}

void dvd_config_load(struct dvd_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"dvd", L"enable", 1, filename);
}

void touch_config_load(struct touch_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"touch", L"enable", 1, filename);
    cfg->passthru = GetPrivateProfileIntW(L"touch", L"passthru", 0, filename);
    cfg->cursor = GetPrivateProfileIntW(L"touch", L"cursor", 0, filename);
    cfg->touch_mode = GetPrivateProfileIntW(L"touch", L"touch_mode", 2, filename);
    cfg->focused_only = GetPrivateProfileIntW(L"touch", L"focused_only", 1, filename);
}

void window_config_load(struct window_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"window", L"enable", 0, filename);
    cfg->width = GetPrivateProfileIntW(L"window", L"width", 0, filename);
    cfg->height = GetPrivateProfileIntW(L"window", L"height", 0, filename);
}
