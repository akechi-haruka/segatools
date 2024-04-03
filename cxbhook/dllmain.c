#include <windows.h>

#include <stddef.h>
#include <stdlib.h>

#include "amex/amex.h"
#include "api/api.h"

#include "board/sg-reader.h"

#include "cxbhook/config.h"

#include "hook/process.h"

#include "hooklib/gfx.h"
#include "hooklib/serial.h"
#include "hooklib/spike.h"

#include "platform/platform.h"

#include "util/dprintf.h"

static HMODULE cxb_hook_mod;
static process_entry_t cxb_startup;
static struct cxb_hook_config cxb_hook_cfg;

static DWORD CALLBACK cxb_pre_startup(void)
{
    HRESULT hr;

    dprintf("--- Begin crossbeats_pre_startup ---\n");

    /* Config load */

    cxb_hook_config_load(&cxb_hook_cfg, L".\\segatools.ini");

    /* Hook Win32 APIs */

    gfx_hook_init(&cxb_hook_cfg.gfx, cxb_hook_mod);
    serial_hook_init();

    /* Initialize emulation hooks */

    hr = platform_hook_init(
            &cxb_hook_cfg.platform,
            "SDCA",
            "AAV1",
            cxb_hook_mod);

    if (FAILED(hr)) {
        return EXIT_FAILURE;
    }


    hr = amex_hook_init(&cxb_hook_cfg.amex, NULL); //disabling JVS as it is handled by the CommIO DLL

    if (FAILED(hr)) {
        return EXIT_FAILURE;
    }

    hr = sg_reader_hook_init(&cxb_hook_cfg.aime, 12, cxb_hook_cfg.aime.gen, cxb_hook_mod);

    if (FAILED(hr)) {
        return EXIT_FAILURE;
    }

    hr = commio_hook_init(&cxb_hook_cfg.io, cxb_hook_mod);

    if (FAILED(hr)) {
        return EXIT_FAILURE;
    }

    /* Initialize debug helpers */

    spike_hook_init(L".\\segatools.ini");

    hr = api_init();

    if (FAILED(hr)) {
        return EXIT_FAILURE;
    }

    /* Loads the custom touch IO built by the team */

    dprintf("---  End crossbeats_pre_startup ---\n");

    /* Jump to EXE start address */

    return cxb_startup();
}

BOOL WINAPI DllMain(HMODULE mod, DWORD cause, void *ctx)
{
    HRESULT hr;

    if (cause != DLL_PROCESS_ATTACH) {
        return TRUE;
    }

    cxb_hook_mod = mod;

    hr = process_hijack_startup(cxb_pre_startup, &cxb_startup);

    if (!SUCCEEDED(hr)) {
        dprintf("Failed to hijack process startup: %x\n", (int) hr);
    }

    return SUCCEEDED(hr);
}
