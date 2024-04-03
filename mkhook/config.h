#pragma once

#include <stddef.h>

#include "board/config.h"

#include "hooklib/gfx.h"
#include "hooklib/window.h"

#include "namco/config.h"
#include "namco/keychip.h"

#include "platform/config.h"

struct io_config {
    uint8_t input_mode;
    uint8_t keyboard_relative;
    uint8_t foreground_only;

    uint8_t vk_exit;
    uint8_t vk_test_switch;
    uint8_t vk_service;
    uint8_t vk_coin;
    uint8_t vk_test_up;
    uint8_t vk_test_down;
    uint8_t vk_test_enter;

    uint8_t vk_steering_left;
    uint8_t vk_steering_right;
    uint8_t vk_accelerator;
    uint8_t vk_brake;
    uint8_t vk_mario_button;
    uint8_t vk_mario_button_alt;
    uint8_t vk_item_button;
    uint8_t vk_item_button_alt;

};

struct mk_hook_config {
    struct platform_config platform;
    struct aime_config aime;
    struct gfx_config gfx;
    struct namsec_config namsec;
    struct bngrw_config bngrw;
    struct io_config io;
    struct clock_config clock;
    struct window_config window;
};

void mk_hook_config_load(
        struct mk_hook_config *cfg,
        const wchar_t *filename);
