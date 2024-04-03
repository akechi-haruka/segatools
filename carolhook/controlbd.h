#pragma once
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>

struct controlbd_config {
    bool enable;
};
enum controlbd_cmd {
    CONTROLBD_CMD_UNK_11 = 0x11
};
struct controlbd_req {
    uint8_t sync; // First byte is the sync
    uint8_t cmd; // Command byte    
    uint8_t data[256]; // Request body goes here
    uint8_t checksum; // Final byte is all bytes added, except the sync
    uint8_t data_length; // Size of the data including command byte
};

HRESULT controlbd_hook_init(const struct controlbd_config *cfg);