#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "api/config.h"


void api_config_load(
        struct api_config *cfg,
        const wchar_t *filename)
{

    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"api", L"enable", 0, filename);
	GetPrivateProfileStringW(
            L"api",
            L"password",
            L"1234",
            cfg->password,
            _countof(cfg->password),
            filename);
	wchar_t bindaddrw[16];
	GetPrivateProfileStringW(
            L"api",
            L"bindAddr",
            L"255.255.255.255",
            bindaddrw,
            _countof(bindaddrw),
            filename);
	wcstombs(cfg->bindAddr, bindaddrw, _countof(bindaddrw));
    cfg->log = GetPrivateProfileIntW(L"api", L"log", 1, filename);
    cfg->port = GetPrivateProfileIntW(L"api", L"port", 5364, filename);
    cfg->networkId = GetPrivateProfileIntW(L"api", L"networkId", 1, filename);
    cfg->allowShutdown = GetPrivateProfileIntW(L"api", L"allowShutdown", 0, filename);

}
