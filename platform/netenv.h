#pragma once

#include <windows.h>

#include <stdbool.h>
#include <stdint.h>

#include "platform/nusec.h"

struct netenv_config {
    uint8_t enable;
    uint8_t addr_suffix;
    uint8_t router_suffix;
    uint8_t mac_addr[6];
    bool use_tracert_bypass;
    bool force;
    bool full_addr;
    uint32_t virtual_ipaddr;
    uint32_t virtual_router;
    uint32_t virtual_bcast;
};

HRESULT netenv_hook_init(
        const struct netenv_config *cfg,
        const struct nusec_config *kc_cfg);

