#pragma once

#include <stdbool.h>
#include <stddef.h>

BOOL cCommIo_IsLoaded();
int __cdecl cCommIo_Open(char *Src); // idb
int cCommIo_Update();
int cCommIo_Close();
int cCommIo_GetStatus();
int cCommIo_Recieve();
LONG cCommIo_GetCoin();
int __cdecl cCommIo_SetCoin(int a1);
int cCommIo_GetSwitch();
LONG cCommIo_GetTrigger();
LONG cCommIo_GetRelease();
int __cdecl cCommIo_SetOutput(int a1);
LONG cCommIo_GetVolume();
int __cdecl cCommIo_SetAmpVolume(int, LONG Value); // idb
int __cdecl cCommIo_GetAmpVolume(unsigned int a1);
int __cdecl cCommIo_SetAmpMute(unsigned int a1, int a2);

struct io_config {
    bool enable;
    int vk_test;
    int vk_cancel;
    int vk_coin;
    int vk_up;
    int vk_down;
    int vk_headphones;
    int vk_hp_up;
    int vk_hp_down;
};

HRESULT commio_hook_init(struct io_config *cfg, HMODULE cxb_hook_mod);
void cxbio_config_load(struct io_config *cfg, const wchar_t *filename);
