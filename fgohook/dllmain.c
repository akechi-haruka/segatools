#include <windows.h>

#include "board/io4.h"
#include "board/sg-reader.h"
#include "board/vfd.h"
#include "board/led1509306.h"

#include "hook/process.h"

#include "hooklib/dvd.h"
#include "hooklib/touch.h"
#include "hooklib/serial.h"
#include "hooklib/spike.h"
#include "printer/printer.h"
#include "board/rfid-rw.h"

#include "fgohook/config.h"
#include "fgohook/io4.h"
#include "fgohook/fgo-dll.h"

#include "platform/platform.h"

#include "util/dprintf.h"

static HMODULE fgo_hook_mod;
static process_entry_t fgo_startup;
static struct fgo_hook_config fgo_hook_cfg;

static DWORD CALLBACK fgo_pre_startup(void)
{
    HRESULT hr;

    dprintf("--- Begin fgo_pre_startup ---\n");

    /* Load config */

    fgo_hook_config_load(&fgo_hook_cfg, L".\\segatools.ini");

    /* Hook Win32 APIs */

    dvd_hook_init(&fgo_hook_cfg.dvd, fgo_hook_mod);
    touch_hook_init(&fgo_hook_cfg.touch, fgo_hook_mod, 0, 0);
    printer_hook_init(&fgo_hook_cfg.printer, fgo_hook_mod, 330, true, true);
    //printerspy_hook_init(&fgo_hook_cfg.printer, fgo_hook_mod, 330);
    if (GetPrivateProfileIntW(L"serial", L"enable", 1, L".\\segatools.ini")){
        serial_hook_init();
    } else {
        dprintf("Serial: disabled\n");
    }

    /* Initialize emulation hooks */

    hr = platform_hook_init(
            &fgo_hook_cfg.platform,
            "SDEJ",
            "ACA1",
            fgo_hook_mod);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = sg_reader_hook_init(&fgo_hook_cfg.aime, 3, fgo_hook_cfg.aime.gen, fgo_hook_mod);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = vfd_hook_init(&fgo_hook_cfg.vfd);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = fgo_dll_init(&fgo_hook_cfg.dll, fgo_hook_mod);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = fgo_io4_hook_init(&fgo_hook_cfg.io4);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = rfid_hook_init(&fgo_hook_cfg.rfid);

    if (FAILED(hr)) {
        goto fail;
    }

    /*hr = rfidwriter_hook_init(&fgo_hook_cfg.rfidwriter);

    if (FAILED(hr)) {
        goto fail;
    }*/

    hr = led1509306_hook_init(&fgo_hook_cfg.led1509306);

    if (FAILED(hr)) {
        goto fail;
    }

    /* Initialize debug helpers */

    spike_hook_init(L".\\segatools.ini");

    dprintf("---  End  fgo_pre_startup ---\n");

    /* Jump to EXE start address */

    return fgo_startup();

fail:
    ExitProcess(EXIT_FAILURE);
}

BOOL WINAPI DllMain(HMODULE mod, DWORD cause, void *ctx)
{
    HRESULT hr;

    if (cause != DLL_PROCESS_ATTACH) {
        return TRUE;
    }

    fgo_hook_mod = mod;

    hr = process_hijack_startup(fgo_pre_startup, &fgo_startup);

    if (!SUCCEEDED(hr)) {
        dprintf("Failed to hijack process startup: %x\n", (int) hr);
    }

    return SUCCEEDED(hr);
}
