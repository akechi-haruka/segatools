#pragma once

#include <windows.h>
#include <initguid.h>

#include <stdbool.h>
#include <stdint.h>

DEFINE_GUID(
        keychip_guid,
        0x3ABF6F2D,
        0x71C4,
        0x462a,
        0x8a, 0x92, 0x1e, 0x68, 0x61, 0xe6, 0xaf, 0x27);

struct namsec_config {
    bool enable;
    wchar_t keychip[16];
};

HRESULT keychip_init(struct namsec_config *cfg);
