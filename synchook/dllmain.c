#include <windows.h>

#include "amex/gpio.h"

#include "api/api.h"

#include "hook/process.h"

#include "hooklib/serial.h"

#include "synchook/config.h"

#include "platform/platform.h"

#include "syncio/syncio.h"

#include "util/dprintf.h"

static HMODULE sync_hook_mod;
static process_entry_t sync_startup;
static struct sync_hook_config sync_hook_cfg;

static DWORD CALLBACK sync_pre_startup(void)
{
    HRESULT hr;

    dprintf("--- Begin sync_pre_startup ---\n");

    dprintf("Init: Sync hook config load\n");
    sync_hook_config_load(&sync_hook_cfg, L".\\segatools.ini");
	dprintf("Init: gfx_hook_init\n");
    gfx_hook_init(&sync_hook_cfg.gfx, sync_hook_mod);
	dprintf("Init: serial_hook_init\n");
    serial_hook_init();
	dprintf("Init: platform_hook_init\n");
    hr = platform_hook_init(
            &sync_hook_cfg.platform,
            "SDBB",
            "ACA1",
            sync_hook_mod);

    if (FAILED(hr)) {
		dprintf("Init: platform_hook_init failed\n");
        return hr;
    }

	dprintf("Init: sync_io_init\n");
	hr = sync_io_init();
	if (FAILED(hr)) {
        return hr;
    }

    dprintf("---  End  sync_pre_startup ---\n");

    return sync_startup();
}

BOOL WINAPI DllMain(HMODULE mod, DWORD cause, void *ctx)
{
    HRESULT hr;

    if (cause != DLL_PROCESS_ATTACH) {
        return TRUE;
    }

    sync_hook_mod = mod;

    hr = process_hijack_startup(sync_pre_startup, &sync_startup);

    if (!SUCCEEDED(hr)) {
        dprintf("Failed to hijack process startup: %x\n", (int) hr);
    }

    return SUCCEEDED(hr);
}
