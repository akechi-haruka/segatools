#pragma once

#include <windows.h>

#include "fgoio/fgoio.h"

struct fgo_dll {
    uint16_t api_version;
    HRESULT (*init)(void);
    HRESULT (*poll)(void);
    void (*get_opbtns)(uint8_t *opbtn);
    void (*get_gamebtns)(uint8_t *btn);
    void (*get_stick)(uint16_t *x, uint16_t *y);
    void (*get_coin)(bool *c);
};

struct fgo_dll_config {
    wchar_t path[MAX_PATH];
};

extern struct fgo_dll fgo_dll;

HRESULT fgo_dll_init(const struct fgo_dll_config *cfg, HINSTANCE self);
