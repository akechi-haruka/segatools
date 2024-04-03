#pragma once

#include <stddef.h>
#include <stdint.h>

struct api_config {
    uint8_t enable;
	uint8_t networkId;
    wchar_t password[64];
    uint8_t log;
    uint16_t port;
	char bindAddr[16];
	bool allowShutdown;
};

void api_config_load(
        struct api_config *cfg,
        const wchar_t *filename);
