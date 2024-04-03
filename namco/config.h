#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "namco/keychip.h"

struct bngrw_config {
    uint8_t enable;
	uint8_t vk_scan;
	wchar_t banapass_path[MAX_PATH];
};

void namsec_config_load(struct namsec_config *cfg, const wchar_t *filename);
void bngrw_config_load(struct bngrw_config *cfg, const wchar_t *filename);

