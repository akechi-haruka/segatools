#include <assert.h>
#include <stddef.h>

#include "amex/config.h"

#include "board/config.h"

#include "hooklib/config.h"
#include "hooklib/dvd.h"
#include "hooklib/gfx.h"

#include "mu3hook/config.h"

#include "platform/config.h"

void mu3_dll_config_load(
        struct mu3_dll_config *cfg,
        const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    GetPrivateProfileStringW(
            L"mu3io",
            L"path",
            L"",
            cfg->path,
            _countof(cfg->path),
            filename);
}

void mu3_hook_config_load(
        struct mu3_hook_config *cfg,
        const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    platform_config_load(&cfg->platform, filename);
    aime_config_load(&cfg->aime, filename);
    dvd_config_load(&cfg->dvd, filename);
    gpio_config_load(&cfg->gpio, filename);
    io4_config_load(&cfg->io4, filename);
    gfx_config_load(&cfg->gfx, filename);
    vfd_config_load(&cfg->vfd, filename);
    mu3_dll_config_load(&cfg->dll, filename);
}
