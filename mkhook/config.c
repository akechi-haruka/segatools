#include <assert.h>
#include <stddef.h>

#include "board/config.h"

#include "hooklib/config.h"
#include "hooklib/gfx.h"

#include "mkhook/config.h"

#include "namco/config.h"

#include "platform/config.h"

void io_config_load(struct io_config *cfg, const wchar_t *filename){

    cfg->input_mode = GetPrivateProfileIntW(L"najv", L"input_mode", 1, filename);
    cfg->keyboard_relative = GetPrivateProfileIntW(L"najv", L"keyboard_relative", 0, filename);
    cfg->foreground_only = GetPrivateProfileIntW(L"najv", L"foreground_input_only", 1, filename);

    cfg->vk_exit = GetPrivateProfileIntW(L"najv", L"exit", VK_ESCAPE, filename);
    cfg->vk_test_switch = GetPrivateProfileIntW(L"najv", L"test_switch", 0x31, filename);
    cfg->vk_service = GetPrivateProfileIntW(L"najv", L"service", 0x32, filename);
    cfg->vk_coin = GetPrivateProfileIntW(L"najv", L"coin", 0x33, filename);
    cfg->vk_test_up = GetPrivateProfileIntW(L"najv", L"test_up", VK_UP, filename);
    cfg->vk_test_down = GetPrivateProfileIntW(L"najv", L"test_down", VK_DOWN, filename);
    cfg->vk_test_enter = GetPrivateProfileIntW(L"najv", L"test_enter", VK_RETURN, filename);

    cfg->vk_steering_left = GetPrivateProfileIntW(L"najv", L"steering_left", VK_LEFT, filename);
    cfg->vk_steering_right = GetPrivateProfileIntW(L"najv", L"steering_right", VK_RIGHT, filename);
    cfg->vk_accelerator = GetPrivateProfileIntW(L"najv", L"accelerator", VK_UP, filename);
    cfg->vk_brake = GetPrivateProfileIntW(L"najv", L"brake", VK_DOWN, filename);
    cfg->vk_mario_button = GetPrivateProfileIntW(L"najv", L"mario_button", 0x41, filename);
    cfg->vk_mario_button = GetPrivateProfileIntW(L"najv", L"mario_button_alt", VK_DOWN, filename);
    cfg->vk_item_button = GetPrivateProfileIntW(L"najv", L"item_button", 0x53, filename);
    cfg->vk_item_button_alt = GetPrivateProfileIntW(L"najv", L"item_button_alt", 0x44, filename);
}

void mk_hook_config_load(
        struct mk_hook_config *cfg,
        const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    platform_config_load(&cfg->platform, filename);
    gfx_config_load(&cfg->gfx, filename);
    namsec_config_load(&cfg->namsec, filename);
    bngrw_config_load(&cfg->bngrw, filename);
    io_config_load(&cfg->io, filename);
    clock_config_load(&cfg->clock, filename);
    window_config_load(&cfg->window, filename);
}
