#pragma once

#include <windows.h>

#include <stdbool.h>

struct led1509306_config {
    bool enable;
    char board_number[8];
    char chip_number[5];
    uint8_t fw_ver;
    uint16_t fw_sum;
    uint8_t port1;
    uint8_t port2;
};

HRESULT led1509306_hook_init(const struct led1509306_config *cfg);
void led1509306_config_load(struct led1509306_config *cfg, const wchar_t *filename);
