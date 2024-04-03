#include <windows.h>

#include <assert.h>
#include <stdlib.h>

#include "gfxhook/gfx.h"

#include "hook/table.h"

#include "util/dprintf.h"

typedef BOOL (WINAPI *ShowWindow_t)(HWND hWnd, int nCmdShow);

static BOOL WINAPI hook_ShowWindow(HWND hWnd, int nCmdShow);

static struct gfx_config gfx_config;
static ShowWindow_t next_ShowWindow;

static const struct hook_symbol gfx_hooks[] = {
    {
        .name = "ShowWindow",
        .patch = hook_ShowWindow,
        .link = (void **) &next_ShowWindow,
    },
};

void gfx_hook_init(const struct gfx_config *cfg)
{
    assert(cfg != NULL);

    if (!cfg->enable) {
        return;
    }

    memcpy(&gfx_config, cfg, sizeof(*cfg));
    hook_table_apply(NULL, "user32.dll", gfx_hooks, _countof(gfx_hooks));
}

static BOOL WINAPI hook_ShowWindow(HWND hWnd, int nCmdShow)
{
    dprintf("Gfx: ShowWindow hook hit\n");

    if (!gfx_config.framed && nCmdShow == SW_RESTORE) {
        nCmdShow = SW_SHOW;
    }

    return next_ShowWindow(hWnd, nCmdShow);
}
