#include <stdbool.h>
#include <stddef.h>

#include "cxbhook/commio.h"
#include "hook/table.h"
#include "hooklib/config.h"
#include "hooklib/dll.h"
#include "util/dprintf.h"

static struct io_config io_config;

static int coin;
static long headphones_volume;
static long prev_btn;
static long cur_btn;
static int coin_btn_state;

BOOL cCommIo_IsLoaded(){
    return TRUE;
}
int __cdecl cCommIo_Open(char *Src){
    dprintf("cxbio: Open: %s ?\n", Src);
    return 1;
} // idb
int cCommIo_Close(){
    return 1;
}
int cCommIo_GetStatus(){

    prev_btn = cur_btn;
    cur_btn = (((GetAsyncKeyState(io_config.vk_headphones) & 0x8000) != 0) << 9)
    | (((GetAsyncKeyState(io_config.vk_headphones) & 0x8000) != 0) << 8)
    | (((GetAsyncKeyState(io_config.vk_up) & 0x8000) != 0) << 3)
    | (((GetAsyncKeyState(io_config.vk_cancel) & 0x8000) != 0) << 2)
    | (((GetAsyncKeyState(io_config.vk_down) & 0x8000) != 0) << 1)
    | (((GetAsyncKeyState(io_config.vk_test) & 0x8000) != 0) << 0);

    if (api_get_test_button()){
        cur_btn |= 1 << 0;
        api_clear_buttons();
    }

    if (GetAsyncKeyState(io_config.vk_hp_up) & 0x8000){
        headphones_volume+=1;
    } else if (GetAsyncKeyState(io_config.vk_hp_down) & 0x8000){
        headphones_volume-=1;
    }
    if (headphones_volume < 0){
        headphones_volume = 0;
    }

    return 1;
}
int cCommIo_Recieve(){
    return 1;
}
int cCommIo_Update(){
    return 1;
}
LONG cCommIo_GetCoin(){
    int coin_pressed = GetAsyncKeyState(io_config.vk_coin) & 0x8000;
    if (api_get_service_button()){
        coin_pressed = 1;
        api_clear_buttons();
    }
    bool result = !coin_pressed && coin_btn_state;
    coin_btn_state = coin_pressed;

    return result;
}
int __cdecl cCommIo_SetCoin(int a1){
    //dprintf("cxbio: SetCoin: %d ?\n", a1);
    coin = a1;
    return 1;
}
int cCommIo_GetSwitch(){
    return 0;
}
LONG cCommIo_GetTrigger(){
    return cur_btn & ~prev_btn;
}
LONG cCommIo_GetRelease(){
    return prev_btn & ~cur_btn;
}
int __cdecl cCommIo_SetOutput(int a1){
    dprintf("cxbio: SetOutput: %d ?\n", a1);
    return 1;
}
LONG cCommIo_GetVolume(){
    //dprintf("cCommIo_GetVolume: %ld", headphones_volume);
    return headphones_volume;
}
int __cdecl cCommIo_SetAmpVolume(int a1, LONG Value){
    //dprintf("cxbio: SetAmpVolume: %d ?\n", a1);
    //headphones_volume = Value;
    return 1;
} // idb
int __cdecl cCommIo_GetAmpVolume(unsigned int a1){
    return headphones_volume;
}
int __cdecl cCommIo_SetAmpMute(unsigned int a1, int a2){
    dprintf("cxbio: SetAmpMute: %d,%d ?\n", a1, a2);
    return 1;
}

static const struct hook_symbol io_hooks[] = {
    {
        .name   = "cCommIo_IsLoaded",
        .patch  = cCommIo_IsLoaded,
    },
	{
        .name   = "cCommIo_Open",
        .patch  = cCommIo_Open,
    },
	{
        .name   = "cCommIo_Update",
        .patch  = cCommIo_Open,
    },
	{
        .name   = "cCommIo_Close",
        .patch  = cCommIo_Close,
    },
	{
        .name   = "cCommIo_GetStatus",
        .patch  = cCommIo_GetStatus,
    },
	{
        .name   = "cCommIo_Recieve",
        .patch  = cCommIo_Recieve,
    },
	{
        .name   = "cCommIo_GetCoin",
        .patch  = cCommIo_GetCoin,
    },
	{
        .name   = "cCommIo_SetCoin",
        .patch  = cCommIo_SetCoin,
    },
	{
        .name   = "cCommIo_GetSwitch",
        .patch  = cCommIo_GetSwitch,
    },
	{
        .name   = "cCommIo_GetTrigger",
        .patch  = cCommIo_GetTrigger,
    },
	{
        .name   = "cCommIo_GetRelease",
        .patch  = cCommIo_GetRelease,
    },
	{
        .name   = "cCommIo_SetOutput",
        .patch  = cCommIo_SetOutput,
    },
	{
        .name   = "cCommIo_GetVolume",
        .patch  = cCommIo_GetVolume,
    },
	{
        .name   = "cCommIo_SetAmpVolume",
        .patch  = cCommIo_SetAmpVolume,
    },
	{
        .name   = "cCommIo_GetAmpVolume",
        .patch  = cCommIo_GetAmpVolume,
    },
	{
        .name   = "cCommIo_SetAmpMute",
        .patch  = cCommIo_SetAmpMute,
    },
};

HRESULT commio_hook_init(struct io_config *cfg, HMODULE cxb_hook_mod){
    assert(cfg != NULL);

    if (!cfg->enable) {
        return S_FALSE;
    }

    memcpy(&io_config, cfg, sizeof(*cfg));
    hook_table_apply(NULL, "CommIo.dll", io_hooks, _countof(io_hooks));
    if (cxb_hook_mod != NULL) {
        dll_hook_push(cxb_hook_mod, L"CommIo.dll");
    }

    return S_OK;
}

void cxbio_config_load(struct io_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"io", L"enable", 1, filename);
    cfg->vk_test = GetPrivateProfileIntW(L"io", L"test", '1', filename);
    cfg->vk_cancel = GetPrivateProfileIntW(L"io", L"cancel", '2', filename);
    cfg->vk_coin = GetPrivateProfileIntW(L"io", L"coin", '3', filename);
    cfg->vk_up = GetPrivateProfileIntW(L"io", L"up", 0x26, filename);
    cfg->vk_down = GetPrivateProfileIntW(L"io", L"down", 0x28, filename);
    cfg->vk_headphones = GetPrivateProfileIntW(L"io", L"headphones_conn", ' ', filename);
    cfg->vk_hp_up = GetPrivateProfileIntW(L"io", L"headphones_up", 0x27, filename);
    cfg->vk_hp_down = GetPrivateProfileIntW(L"io", L"headphones_down", 0x25, filename);
}
