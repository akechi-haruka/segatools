#include <windows.h>

#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include "syncio/config.h"

#include "util/dprintf.h"


void wajv_config_load(
        struct wajv_config *cfg,
        const wchar_t *filename){

    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"wajv", L"enable", 1, filename);
    cfg->vk_test = GetPrivateProfileIntW(L"wajv", L"test", 0x31, filename);
    cfg->vk_service = GetPrivateProfileIntW(L"wajv", L"service", 0x32, filename);
    cfg->vk_coin = GetPrivateProfileIntW(L"wajv", L"coin", 0x33, filename);
    cfg->vk_up = GetPrivateProfileIntW(L"wajv", L"up", 0x26, filename);
    cfg->vk_down = GetPrivateProfileIntW(L"wajv", L"down", 0x28, filename);
    cfg->vk_enter = GetPrivateProfileIntW(L"wajv", L"enter", 0x0D, filename);
    cfg->dipsw1 = GetPrivateProfileIntW(L"wajv", L"dipsw1", 0, filename);
    cfg->dipsw2 = GetPrivateProfileIntW(L"wajv", L"dipsw2", 0, filename);
    cfg->dipsw3 = GetPrivateProfileIntW(L"wajv", L"dipsw3", 0, filename);
    cfg->dipsw4 = GetPrivateProfileIntW(L"wajv", L"dipsw4", 0, filename);
	cfg->side = 0;

	wchar_t side[2];

	GetPrivateProfileStringW(
                L"wajv",
                L"side",
                L"L",
                side,
                _countof(side),
                filename);

	if (wcscmp(side, L"R") == 0){
		cfg->side = 1;
	} else if (wcscmp(side, L"L") != 0){
		dprintf("WARNING: Unknown side value \"%ls\", defaulting to L!\n", side);
	}

	dprintf("Synchronica: Side %ls (%d)\n", side, cfg->side);
}
