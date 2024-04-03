#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include "amex/amex.h"
#include "amex/config.h"

#include "board/config.h"
#include "board/sg-reader.h"

#include "cxbhook/config.h"
#include "cxbhook/commio.h"

#include "hooklib/config.h"
#include "hooklib/gfx.h"

#include "platform/config.h"
#include "platform/platform.h"

void cxb_hook_config_load(
        struct cxb_hook_config *cfg,
        const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    memset(cfg, 0, sizeof(*cfg));

    platform_config_load(&cfg->platform, filename);
    amex_config_load(&cfg->amex, filename);
    aime_config_load(&cfg->aime, filename);
    gfx_config_load(&cfg->gfx, filename);
    cxbio_config_load(&cfg->io, filename);
}
