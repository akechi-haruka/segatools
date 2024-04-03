#include <windows.h>

#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include "mu3io/config.h"


void mu3_io_config_load(
        struct mu3_io_config *cfg,
        const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);
	
    cfg->input_mode = GetPrivateProfileIntW(L"io3", L"mode", 1, filename);
    cfg->vk_test = GetPrivateProfileIntW(L"io3", L"test", 0x31, filename);
    cfg->vk_service = GetPrivateProfileIntW(L"io3", L"service", 0x32, filename);
    cfg->vk_coin = GetPrivateProfileIntW(L"io3", L"coin", 0x33, filename);
	
    cfg->vk_la = GetPrivateProfileIntW(L"dinput", L"LEFT_A", 0x53, filename);
    cfg->vk_lb = GetPrivateProfileIntW(L"dinput", L"LEFT_B", 0x44, filename);
    cfg->vk_lc = GetPrivateProfileIntW(L"dinput", L"LEFT_C", 0x46, filename);
    cfg->vk_lm = GetPrivateProfileIntW(L"dinput", L"LEFT_MENU", 0x51, filename);
    cfg->vk_ls = GetPrivateProfileIntW(L"dinput", L"LEFT_SIDE", 0x52, filename);
    cfg->vk_ra = GetPrivateProfileIntW(L"dinput", L"RIGHT_A", 0x4A, filename);
    cfg->vk_rb = GetPrivateProfileIntW(L"dinput", L"RIGHT_B", 0x4B, filename);
    cfg->vk_rc = GetPrivateProfileIntW(L"dinput", L"RIGHT_C", 0x4C, filename);
    cfg->vk_rm = GetPrivateProfileIntW(L"dinput", L"RIGHT_MENU", 0x50, filename);
    cfg->vk_rs = GetPrivateProfileIntW(L"dinput", L"RIGHT_SIDE", 0x55, filename);
    cfg->vk_sliderLeft = GetPrivateProfileIntW(L"dinput", L"SLIDER_LEFT", 0x54, filename);
    cfg->vk_sliderRight = GetPrivateProfileIntW(L"dinput", L"SLIDER_RIGHT", 0x59, filename);
    cfg->sliderSpeed = GetPrivateProfileIntW(L"dinput", L"SLIDER_SPEED", 1000, filename);
    cfg->background_input_allowed = GetPrivateProfileIntW(L"dinput", L"background_input_allowed", 0, filename);

}
