#pragma once

#include <stddef.h>
#include <stdint.h>

#include <stdbool.h>

struct fgo_io_config {
    bool background_input_allowed;
    uint8_t vk_test;
    uint8_t vk_service;
    uint8_t vk_coin;
    uint8_t input_mode;
    uint8_t vk_treasure;
    uint8_t vk_target;
    uint8_t vk_dash;
    uint8_t vk_attack;
    uint8_t vk_camera;
    uint8_t vk_right;
    uint8_t vk_left;
    uint8_t vk_down;
    uint8_t vk_up;
};

void fgo_io_config_load(
        struct fgo_io_config *cfg,
        const wchar_t *filename);
