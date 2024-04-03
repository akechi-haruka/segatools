#include <windows.h>

#include "printer/printer.h"
#include "printer/printer-stdcall.h"

#include "hook/process.h"

#include "exdlls/printerex320a/config.h"

#include "util/dprintf.h"

static HMODULE printer_hook_mod = NULL;
static struct printer_hook_config printer_hook_cfg;

static void printer_pre_startup(void)
{

    dprintf("--- Begin printerA_pre_startup ---\n");

    /* Load config */
    printer_hook_config_load(&printer_hook_cfg, L".\\segatools.ini");
    printer_std_hook_init(&printer_hook_cfg.printer, printer_hook_mod, NULL, 320, false);

    dprintf("---  End  printerA_pre_startup ---\n");

}

BOOL WINAPI DllMain(HMODULE mod, DWORD cause, void *ctx)
{
    HRESULT hr;

    if (cause != DLL_PROCESS_ATTACH) {
        return TRUE;
    }

    printer_hook_mod = mod;

    printer_pre_startup();

    return SUCCEEDED(hr);
}
