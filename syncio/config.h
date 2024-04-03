#pragma once

#include <stddef.h>
#include <stdint.h>

struct wajv_config {
    uint8_t enable;
	uint8_t vk_test;
	uint8_t vk_service;
	uint8_t vk_coin;
	uint8_t vk_up;
	uint8_t vk_down;
	uint8_t vk_enter;
	uint8_t dipsw1;
	uint8_t dipsw2;
	uint8_t dipsw3;
	uint8_t dipsw4;
	uint8_t side;
};

void wajv_config_load(
        struct wajv_config *cfg,
        const wchar_t *filename);
