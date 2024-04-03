#include <assert.h>
#include <stddef.h>

#include "amex/config.h"

#include "board/config.h"

#include "hooklib/config.h"
#include "hooklib/gfx.h"

#include "synchook/config.h"

#include "platform/config.h"

void sync_hook_config_load(
        struct sync_hook_config *cfg,
        const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    platform_config_load(&cfg->platform, filename);
    gfx_config_load(&cfg->gfx, filename);
}
