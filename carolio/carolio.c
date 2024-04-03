#include <windows.h>

#include <process.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "carolio/carolio.h"
#include "carolio/config.h"

static bool carol_io_coin;
static uint16_t carol_io_coins;
static struct carol_io_config carol_io_cfg;

uint16_t carol_io_get_api_version(void)
{
    return 0x0100;
}

HRESULT carol_io_jvs_init(void)
{
    carol_io_config_load(&carol_io_cfg, L".\\segatools.ini");

    return S_OK;
}

void carol_io_jvs_poll(uint8_t *opbtn_out, uint8_t *gamebtn_out)
{
    uint8_t opbtn;
    uint8_t gamebtn;
    size_t i;

    opbtn = 0;

    if (GetAsyncKeyState(carol_io_cfg.vk_test) & 0x8000) {
        opbtn |= 1;
    }

    if (GetAsyncKeyState(carol_io_cfg.vk_service) & 0x8000) {
        opbtn |= 2;
    }

    for (i = 0 ; i < _countof(carol_io_cfg.vk_buttons) ; i++) {
        if (GetAsyncKeyState(carol_io_cfg.vk_buttons[i]) & 0x8000) {
            gamebtn |= 1 << i;
        }
    }

    *opbtn_out = opbtn;
    *gamebtn_out = gamebtn;
}

void carol_io_jvs_read_coin_counter(uint16_t *out)
{
    if (out == NULL) {
        return;
    }

    if (GetAsyncKeyState(carol_io_cfg.vk_coin) & 0x8000) {
        if (!carol_io_coin) {
            carol_io_coin = true;
            carol_io_coins++;
        }
    } else {
        carol_io_coin = false;
    }

    *out = carol_io_coins;
}

HRESULT carol_io_touch_init()
{
    return S_OK;
}

HRESULT carol_io_controlbd_init()
{
    return S_OK;
}