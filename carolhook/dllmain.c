#include <windows.h>

#include <stdlib.h>

#include "amex/amex.h"
#include "gfxhook/gfx.h"
#include "gfxhook/d3d9.h"

#include "board/sg-reader.h"

#include "carolhook/config.h"
#include "carolhook/carol-dll.h"
#include "carolhook/jvs.h"
#include "carolhook/touch.h"
#include "carolhook/controlbd.h"
#include "carolhook/serial.h"

#include "hook/process.h"

#include "hooklib/serial.h"
#include "hooklib/spike.h"

#include "platform/platform.h"

#include "util/dprintf.h"

static HMODULE carol_hook_mod;
static process_entry_t carol_startup;
static struct carol_hook_config carol_hook_cfg;

/*
COM Layout
01:(?) Touchscreen
10: Aime reader
11: Control board
12(?): LED Board
*/

static DWORD CALLBACK carol_pre_startup(void)
{
    HRESULT hr;

    dprintf("--- Begin carol_pre_startup(hMod=%ld) ---\n", carol_hook_mod);

    /* Config load */

    carol_hook_config_load(&carol_hook_cfg, L".\\segatools.ini");

    /* Hook Win32 APIs */

    serial_hook_init();

    /* Initialize emulation hooks */

    hr = platform_hook_init(
            &carol_hook_cfg.platform,
            "SDAP",
            "AAV0",
            carol_hook_mod);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = carol_dll_init(&carol_hook_cfg.dll, carol_hook_mod);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = amex_hook_init(&carol_hook_cfg.amex, carol_jvs_init);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = sg_reader_hook_init(&carol_hook_cfg.aime, 10, carol_hook_cfg.aime.gen, carol_hook_mod);

    if (FAILED(hr)) {
        goto fail;
    }

    gfx_hook_init(&carol_hook_cfg.gfx);
    gfx_d3d9_hook_init(&carol_hook_cfg.gfx, carol_hook_mod);
    //serial_init();


    hr = touch_hook_init(&carol_hook_cfg.touch);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = controlbd_hook_init(&carol_hook_cfg.controlbd);

    if (FAILED(hr)) {
        goto fail;
    }

    /* Initialize debug helpers */

    spike_hook_init(L".\\segatools.ini");

    dprintf("---  End  carol_pre_startup ---\n");

    /* Jump to EXE start address */

    return carol_startup();

fail:
    ExitProcess(EXIT_FAILURE);
}

BOOL WINAPI DllMain(HMODULE mod, DWORD cause, void *ctx)
{
    HRESULT hr;

    if (cause != DLL_PROCESS_ATTACH) {
        return TRUE;
    }

    carol_hook_mod = mod;

    hr = process_hijack_startup(carol_pre_startup, &carol_startup);

    if (!SUCCEEDED(hr)) {
        dprintf("Failed to hijack process startup: %x\n", (int) hr);
    }

    return SUCCEEDED(hr);
}
