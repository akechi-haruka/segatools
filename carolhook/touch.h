#pragma once
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>

struct touch_config {
    bool enable;
};

struct touch_req {
    uint8_t cmd; // First byte is the command byte
    uint8_t data[256]; // rest of the data goes here
    uint8_t data_length; // Size of the data including command byte
};

HRESULT touch_hook_init(const struct touch_config *cfg);