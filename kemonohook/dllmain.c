#include <windows.h>

#include "board/io4.h"
#include "board/sg-reader.h"
#include "board/vfd.h"
#include "board/led1509306.h"

#include "hook/process.h"

#include "hooklib/comshark.h"
#include "hooklib/dll.h"
#include "hooklib/dvd.h"
#include "hooklib/touch.h"
#include "hooklib/serial.h"
#include "hooklib/spike.h"
#include "printer/printer-stdcall.h"
#include "board/rfid-rw.h"

#include "kemonohook/config.h"

#include "platform/platform.h"

#include "util/dprintf.h"

static HMODULE kemono_hook_mod;
static process_entry_t kemono_startup;
static struct kemono_hook_config kemono_hook_cfg;

static DWORD CALLBACK kemono_pre_startup(void)
{
    HRESULT hr;

    dprintf("--- Begin kemono_pre_startup ---\n");

    /* Load config */

    kemono_hook_config_load(&kemono_hook_cfg, L".\\segatools.ini");

    printer_std_hook_init(&kemono_hook_cfg.printer, kemono_hook_mod, kemono_hook_mod, 300, true);

    dprintf("---  End  kemono_pre_startup ---\n");

    /* Jump to EXE start address */

    return kemono_startup();

fail:
    ExitProcess(EXIT_FAILURE);
}

BOOL WINAPI DllMain(HMODULE mod, DWORD cause, void *ctx)
{
    HRESULT hr;

    if (cause != DLL_PROCESS_ATTACH) {
        return TRUE;
    }

    kemono_hook_mod = mod;

    hr = process_hijack_startup(kemono_pre_startup, &kemono_startup);

    if (!SUCCEEDED(hr)) {
        dprintf("Failed to hijack process startup: %x\n", (int) hr);
    }

    return SUCCEEDED(hr);
}
