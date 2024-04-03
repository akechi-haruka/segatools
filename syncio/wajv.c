#include <windows.h>
#include <stdint.h>

#include "api/api.h"

#include "hook/table.h"
#include "syncio/config.h"
#include "syncio/wajv.h"
#include "util/dprintf.h"

static __stdcall int hook_WAJVOpen(int x);
static __stdcall signed int hook_WAJVGetStatus(); // return 1
static __stdcall int hook_WAJVSetFunctionSettings(int a1, int a2, int a3);
static __stdcall int hook_WAJVUpdate();
static __stdcall WAJVInput* hook_WAJVGetInput(int x);
static __stdcall const char *hook_WAJVGetLibraryVersion();
static __stdcall int hook_WAJVClose();
static __stdcall int hook_WAJVEndUpdate(); // return 1 on ok
static __stdcall int hook_WAJVSetOutput(int a1, WAJVOutput* a2); // return 1 on ok
static __stdcall int hook_WAJVBeginUpdate();
static __stdcall signed int hook_WAJVGetNodeCount();
static __stdcall int hook_WAJVSetPLParm(unsigned __int8 a1, __int16 a2, unsigned __int8 a3);

static struct WAJVInput* input_emu;
static struct wajv_config* config;

static const struct hook_symbol wajv_syms[] = {
    {
        .name   = "WAJVOpen",
        .patch  = hook_WAJVOpen,
    },
    {
        .name   = "WAJVGetStatus",
        .patch  = hook_WAJVGetStatus,
    },
    {
        .name   = "WAJVSetFunctionSettings",
        .patch  = hook_WAJVSetFunctionSettings,
    },
    {
        .name   = "WAJVUpdate",
        .patch  = hook_WAJVUpdate,
    },
    {
        .name   = "WAJVGetInput",
        .patch  = hook_WAJVGetInput,
    },
    {
        .name   = "WAJVGetLibraryVersion",
        .patch  = hook_WAJVGetLibraryVersion,
    },
    {
        .name   = "WAJVClose",
        .patch  = hook_WAJVClose,
    },
    {
        .name   = "WAJVEndUpdate",
        .patch  = hook_WAJVEndUpdate,
    },
    {
        .name   = "WAJVSetOutput",
        .patch  = hook_WAJVSetOutput,
    },
    {
        .name   = "WAJVBeginUpdate",
        .patch  = hook_WAJVBeginUpdate,
    },
	{
        .name   = "WAJVGetNodeCount",
        .patch  = hook_WAJVGetNodeCount,
    },
	{
        .name   = "WAJVSetPLParm",
        .patch  = hook_WAJVSetPLParm,
    },
};

HRESULT wajv_init(struct wajv_config* cfg){
    dprintf("WAJV: initializing\n");

    config = cfg;

    input_emu = (WAJVInput*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WAJVInput));

    hook_table_apply(
        NULL,
        "wajvio.dll",
        wajv_syms,
        _countof(wajv_syms));

    return S_OK;
}

static __stdcall int hook_WAJVOpen(int x){
    dprintf("WAJV: hook_WAJVOpen\n");
    // immediately set the side to prevent race conditions with the second cab
    input_emu->SwIn[0][7] = config->side;
    return 1;
}
static __stdcall signed int hook_WAJVGetStatus(){
    //dprintf("WAJV: hook_WAJVGetStatus\n");
    return 1;
}
static __stdcall int hook_WAJVSetFunctionSettings(int a1, int a2, int a3){
    dprintf("WAJV: hook_WAJVSetFunctionSettings: %d, %d, %d\n", a1, a2, a3);
    return 1;
}
static __stdcall int hook_WAJVUpdate(){
    dprintf("WAJV: hook_WAJVUpdate\n");
    return 1;
}
static __stdcall WAJVInput* hook_WAJVGetInput(int x){
    //dprintf("WAJV: hook_WAJVGetInput\n");
    return input_emu;
}
static __stdcall const char *hook_WAJVGetLibraryVersion(){
    return "Bamcotools-WAJV-Emulation";
}
static __stdcall int hook_WAJVClose(){
    dprintf("WAJV: hook_WAJVClose\n");
    return 1;
}

static bool coin_was_down = false;
static bool service_was_down = false;
static bool test_was_down = false;

static __stdcall int hook_WAJVEndUpdate(){
    //dprintf("WAJV: hook_WAJVEndUpdate\n");
    input_emu->DipSw1 = config->dipsw1;
    input_emu->DipSw2 = config->dipsw2;
    input_emu->DipSw3 = config->dipsw3;
    input_emu->DipSw4 = config->dipsw4;

    if (GetAsyncKeyState(config->vk_test) & 0x8000 || api_get_test_button()){
        if (!test_was_down){
            input_emu->SwTest = !input_emu->SwTest;
            test_was_down = true;
        }
    } else {
        test_was_down = false;
    }

    if (GetAsyncKeyState(config->vk_coin) & 0x8000 || api_get_credit_button()){
        if (!coin_was_down){
            input_emu->Coin[0].Count++;
            input_emu->Coin[0].Status = WAJVCoinBusy;
            coin_was_down = true;
        }
    } else {
        coin_was_down = false;
        input_emu->Coin[0].Status = WAJVCoinOK;
    }

    if (GetAsyncKeyState(config->vk_service) & 0x8000 || api_get_service_button()){
        if (!service_was_down){
            input_emu->SwIn[0][1] = 1;
            input_emu->Service[0].Count = 1;
            service_was_down = true;
        } else {
            input_emu->Service[0].Count = 0;
        }
    } else {
        input_emu->SwIn[0][1] = 0;
        service_was_down = false;
        input_emu->Service[0].Count = 0;
    }

    input_emu->SwIn[0][2] = (GetAsyncKeyState(config->vk_up) & 0x8000) ? 1 : 0; // up
    input_emu->SwIn[0][3] = (GetAsyncKeyState(config->vk_down) & 0x8000) ? 1 : 0; // down
    input_emu->SwIn[1][2] = (GetAsyncKeyState(config->vk_enter) & 0x8000) ? 1 : 0; // enter

    for (int i = 0; i < 4; i++){
        input_emu->Service[i].Status = WAJVServiceOK;
    }

	input_emu->SwIn[0][7] = config->side;

	api_clear_buttons();

    return 1;
}
static __stdcall int hook_WAJVSetOutput(int a1, WAJVOutput* a2){
    //dprintf("WAJV: hook_WAJVSetOutput: %d, %p\n", a1, a2);
    //dprintf("AnalogOut: %d, %d, %d, %d, %d, %d, %d, %d", a2->AnalogOut[0], a2->AnalogOut[1], a2->AnalogOut[2], a2->AnalogOut[3], a2->AnalogOut[4], a2->AnalogOut[5], a2->AnalogOut[6], a2->AnalogOut[7]);
    return 1;
}
static __stdcall int hook_WAJVBeginUpdate(){
    //dprintf("WAJV: hook_WAJVBeginUpdate\n");
    return 1;
}
static __stdcall signed int hook_WAJVGetNodeCount(){
    //dprintf("WAJV: hook_WAJVGetNodeCount\n");
    return 99;
}
static __stdcall int hook_WAJVSetPLParm(unsigned __int8 a1, __int16 a2, unsigned __int8 a3){
    dprintf("WAJV: hook_WAJVSetPLParm: %d, %d, %d\n", a1, a2, a3);
    return 1;
}
