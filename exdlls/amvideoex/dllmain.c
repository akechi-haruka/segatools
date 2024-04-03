#include <windows.h>

#include "platform/amvideo.h"
#include "platform/config.h"
#include "platform/platform.h"

#include "util/dprintf.h"

static HMODULE hook_mod = NULL;
static struct amvideo_config amvideo_config;

static HRESULT amvideoex_pre_startup(){

    dprintf("--- Begin amvideoex_pre_startup ---\n");

    amvideo_config_load(&amvideo_config, L".\\segatools.ini");
    HRESULT hr = amvideo_hook_init(&amvideo_config, hook_mod);

    dprintf("---  End  amvideoex_pre_startup ---\n");

    return hr;
}

BOOL WINAPI DllMain(HMODULE mod, DWORD cause, void *ctx){
    if (cause != DLL_PROCESS_ATTACH) {
        return TRUE;
    }

    hook_mod = mod;

    return SUCCEEDED(amvideoex_pre_startup());
}
