#include <windows.h>

#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include "chuniio/config.h"

static const int chuni_io_default_cells[] = {
    'L', 'L', 'L', 'L',
    'K', 'K', 'K', 'K',
    'J', 'J', 'J', 'J',
    'H', 'H', 'H', 'H',
    'G', 'G', 'G', 'G',
    'F', 'F', 'F', 'F',
    'D', 'D', 'D', 'D',
    'S', 'S', 'S', 'S',
};

void chuni_io_config_load(
        struct chuni_io_config *cfg,
        const wchar_t *filename)
{
    wchar_t key[16];
    int i;

    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->vk_test = GetPrivateProfileIntW(L"io3", L"test", '1', filename);
    cfg->vk_service = GetPrivateProfileIntW(L"io3", L"service", '2', filename);
    cfg->vk_coin = GetPrivateProfileIntW(L"io3", L"coin", '3', filename);
    cfg->vk_ir = GetPrivateProfileIntW(L"io3", L"ir", VK_SPACE, filename);
    cfg->vk_enable_ir = GetPrivateProfileIntW(L"io3", L"enableirkeys", 1, filename);
    cfg->vk_ir_arr[0] = GetPrivateProfileIntW(L"io3", L"ir1", 0x62, filename);
    cfg->vk_ir_arr[1] = GetPrivateProfileIntW(L"io3", L"ir2", 0x61, filename);
    cfg->vk_ir_arr[2] = GetPrivateProfileIntW(L"io3", L"ir3", 0x64, filename);
    cfg->vk_ir_arr[3] = GetPrivateProfileIntW(L"io3", L"ir4", 0x63, filename);
    cfg->vk_ir_arr[4] = GetPrivateProfileIntW(L"io3", L"ir5", 0x66, filename);
    cfg->vk_ir_arr[5] = GetPrivateProfileIntW(L"io3", L"ir6", 0x65, filename);

    for (i = 0 ; i < 32 ; i++) {
        swprintf_s(key, _countof(key), L"cell%i", i + 1);
        cfg->vk_cell[i] = GetPrivateProfileIntW(
                L"slider",
                key,
                chuni_io_default_cells[i],
                filename);
    }

    cfg->led_port = GetPrivateProfileIntW(L"led", L"port", 4, filename);
    cfg->led_rate = GetPrivateProfileIntW(L"led", L"rate", 115200, filename);
    cfg->led_type = GetPrivateProfileIntW(L"led", L"type", 1, filename);
    cfg->led_brightness = GetPrivateProfileIntW(L"led", L"brightness", 50, filename);
    if (cfg->led_brightness < 0){
        cfg->led_brightness = 0;
    } else if (cfg->led_brightness > 100){
        cfg->led_brightness = 100;
    }
}
