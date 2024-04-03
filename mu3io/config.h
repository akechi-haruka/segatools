#pragma once

#include <stddef.h>
#include <stdint.h>

struct mu3_io_config {
    uint8_t vk_test;
    uint8_t vk_service;
    uint8_t vk_coin;
    uint8_t vk_la;
    uint8_t vk_lb;
    uint8_t vk_lc;
    uint8_t vk_ra;
    uint8_t vk_rb;
    uint8_t vk_rc;
    uint8_t vk_lm;
    uint8_t vk_rm;
    uint8_t vk_ls;
    uint8_t vk_rs;
    uint8_t vk_sliderLeft;
    uint8_t vk_sliderRight;
    uint16_t sliderSpeed;
    uint8_t input_mode;
	uint8_t background_input_allowed;
	
};

void mu3_io_config_load(
        struct mu3_io_config *cfg,
        const wchar_t *filename);
