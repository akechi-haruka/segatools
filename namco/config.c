#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "namco/config.h"
#include "namco/keychip.h"

void namsec_config_load(struct namsec_config *cfg, const wchar_t *filename){
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"keychip", L"enable", 1, filename);
    GetPrivateProfileStringW(
                L"keychip",
                L"id",
                L"271022022456",
                cfg->keychip,
                _countof(cfg->keychip),
                filename);
}

void bngrw_config_load(
        struct bngrw_config *cfg,
        const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"bngrw", L"enable", 1, filename);
    cfg->vk_scan = GetPrivateProfileIntW(L"bngrw", L"scan", 0x0D, filename);
    GetPrivateProfileStringW(
                L"bngrw",
                L"banapassPath",
                L"DEVICE\\banapass.txt",
                cfg->banapass_path,
                _countof(cfg->banapass_path),
                filename);

}
