/*
IO part of fgohook code is developed and provided by Haruka @ discord with permission. Thanks.
*/
#include <windows.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "board/io4.h"

#include "fgohook/fgo-dll.h"

#include "util/dprintf.h"

static HRESULT fgo_io4_poll(void *ctx, struct io4_state *state);

static const struct io4_ops fgo_io4_ops = {
    .poll = fgo_io4_poll,
};
static int coin_count = 0;
static bool coin_prev_state = false;

HRESULT fgo_io4_hook_init(const struct io4_config *cfg)
{
    HRESULT hr;

    assert(fgo_dll.init != NULL);

    hr = io4_hook_init(cfg, &fgo_io4_ops, NULL);

    if (FAILED(hr)) {
        return hr;
    }

    return fgo_dll.init();
}

static HRESULT fgo_io4_poll(void *ctx, struct io4_state *state)
{
    uint8_t opbtn;
    uint8_t btn;
    uint16_t x, y;
    bool c;
    HRESULT hr;

    assert(fgo_dll.poll != NULL);
    assert(fgo_dll.get_opbtns != NULL);
    assert(fgo_dll.get_gamebtns != NULL);
    assert(fgo_dll.get_stick != NULL);
    assert(fgo_dll.get_coin != NULL);

    memset(state, 0, sizeof(*state));

    hr = fgo_dll.poll();

    if (FAILED(hr)) {
        return hr;
    }

    opbtn = 0;
    btn = 0;
    x = 0;
    y = 0;

    fgo_dll.get_opbtns(&opbtn);
    fgo_dll.get_gamebtns(&btn);
    fgo_dll.get_stick(&x, &y);
    fgo_dll.get_coin(&c);

    if (opbtn & FGO_IO_OPBTN_TEST) {
        state->buttons[0] |= IO4_BUTTON_TEST;
    }

    if (opbtn & FGO_IO_OPBTN_SERVICE) {
        state->buttons[0] |= IO4_BUTTON_SERVICE;
    }

    if (btn & IO_BTN_TREASURE) {
        state->buttons[0] |= 1 << 0;
    }

    if (btn & IO_BTN_ATTACK) {
        state->buttons[0] |= 1 << 1;
    }

    if (btn & IO_BTN_DASH) {
        state->buttons[0] |= 1 << 4;
    }

    if (btn & IO_BTN_TARGET) {
        state->buttons[0] |= 1 << 5;
    }

    if (btn & IO_BTN_CAMERA) {
        state->buttons[0] |= 1 << 14;
    }

    state->adcs[0] = x;
    state->adcs[4] = y;

    if (c){
        state->chutes[0] |= 1 << 8;
        if (!coin_prev_state){
            coin_count++;
            coin_prev_state = true;
        }
    } else {
        coin_prev_state = false;
    }
    state->chutes[0] = coin_count << 8;

    return S_OK;
}
