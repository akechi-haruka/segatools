#pragma once

#include <windows.h>

#include <stdbool.h>

struct rfid_config {
    bool enable;
    bool enable_bin_writing;
    uint8_t ports[2];
    char board_number[10];
    uint8_t boot_fw_ver[2];
    uint8_t app_fw_ver[2];
    wchar_t path[MAX_PATH];
};

HRESULT rfid_hook_init(const struct rfid_config *cfg);
void rfid_config_load(struct rfid_config *cfg, const wchar_t *filename);
