#include <windows.h>

#include <stdlib.h>

#include "amex/config.h"
#include "amex/ds.h"

#include "board/io3.h"
#include "board/io4.h"
#include "board/sg-reader.h"
#include "board/vfd.h"
#include "board/led1509306.h"

#include "hook/process.h"

#include "hooklib/config.h"
#include "hooklib/dvd.h"
#include "hooklib/touch.h"
#include "hooklib/serial.h"
#include "hooklib/spike.h"
#include "printer/printer.h"
#include "board/rfid-rw.h"

#include "fgohook/config.h"
#include "fgohook/io4.h"
#include "fgohook/fgo-dll.h"

#include "platform/platform.h"

#include "util/dprintf.h"

static process_entry_t app_startup;
static HMODULE mini_mod;

static HRESULT io4_poll(void* ctx, struct io4_state* state);

static const struct io4_ops io4_op_list = {
    .poll = io4_poll,
};

static struct clock_config clock_cfg;
static struct ds_config ds_cfg;
static struct nusec_config nusec_cfg;
static struct aime_config aime_cfg;
static struct dvd_config dvd_cfg;
static struct touch_config touch_cfg;
static struct platform_config platform_cfg;
static struct amex_config amex_cfg;
static struct led1509306_config led1509306_cfg;
static struct io4_config io4_cfg;
static struct gfx_config gfx_cfg;
static struct vfd_config vfd_cfg;

static int vk_test;
static int vk_service;
static int vk_coin;

static bool coin_counter_key_state;
static int coin_counter;

static DWORD CALLBACK app_pre_startup(void)
{

    int aime_port;
    char gameid[5];
    char platid[5];

    HRESULT hr;

    dprintf("--- Begin %s ---\n", __func__);

    GetPrivateProfileStringA("minihook", "gameid", "SSSS", gameid, 5, ".\\segatools.ini");
    GetPrivateProfileStringA("minihook", "platformid", "AAV0", platid, 5, ".\\segatools.ini");
    aime_port = GetPrivateProfileIntA("minihook", "aime_port", 3, ".\\segatools.ini");

    clock_config_load(&clock_cfg, L".\\segatools.ini");
    ds_config_load(&ds_cfg, L".\\segatools.ini");
    nusec_config_load(&nusec_cfg, L".\\segatools.ini");
    platform_config_load(&platform_cfg, L".\\segatools.ini");
    touch_config_load(&touch_cfg, L".\\segatools.ini");
    dvd_config_load(&dvd_cfg, L".\\segatools.ini");
    amex_config_load(&amex_cfg, L".\\segatools.ini");
    led1509306_config_load(&led1509306_cfg, L".\\segatools.ini");
    io4_config_load(&io4_cfg, L".\\segatools.ini");
    gfx_config_load(&gfx_cfg, L".\\segatools.ini");
    aime_config_load(&aime_cfg, L".\\segatools.ini");
    vfd_config_load(&vfd_cfg, L".\\segatools.ini");

    vk_test = GetPrivateProfileIntW(L"io4", L"test", '1', L".\\segatools.ini");
    vk_service = GetPrivateProfileIntW(L"io4", L"service", '2', L".\\segatools.ini");
    vk_coin = GetPrivateProfileIntW(L"io4", L"coin", '3', L".\\segatools.ini");

    spike_hook_init(L".\\segatools.ini");
    serial_hook_init();
    dvd_hook_init(&dvd_cfg, mini_mod);
    touch_hook_init(&touch_cfg, mini_mod, 0, 0);
    gfx_hook_init(&gfx_cfg, mini_mod);
    vfd_hook_init(&vfd_cfg);

    hr = amex_hook_init(&amex_cfg, NULL);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = clock_hook_init(&clock_cfg);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = platform_hook_init(
                &platform_cfg,
                gameid,
                platid,
                mini_mod);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = sg_reader_hook_init(&aime_cfg, aime_port, aime_cfg.gen, mini_mod);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = ds_hook_init(&ds_cfg);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = led1509306_hook_init(&led1509306_cfg);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = io4_hook_init(&io4_cfg, &io4_op_list, NULL);

    if (FAILED(hr)) {
        goto fail;
    }

    dprintf("---  End  %s ---\n", __func__);

    return app_startup();

fail:
    ExitProcess(EXIT_FAILURE);
}

BOOL WINAPI DllMain(HMODULE mod, DWORD cause, void *ctx)
{
    HRESULT hr;

    if (cause != DLL_PROCESS_ATTACH) {
        return TRUE;
    }

    mini_mod = mod;

    hr = process_hijack_startup(app_pre_startup, &app_startup);

    if (!SUCCEEDED(hr)) {
        dprintf("Failed to hijack process startup: %x\n", (int) hr);
    }

    return SUCCEEDED(hr);
}

static HRESULT io4_poll(void* ctx, struct io4_state* state)
{
    uint8_t opbtn;

    memset(state, 0, sizeof(*state));

    opbtn = 0;

    if (GetAsyncKeyState(vk_test)) {
        opbtn |= 0x01; /* Test */
    }

    if (GetAsyncKeyState(vk_service)) {
        opbtn |= 0x02; /* Service */
    }

    if (GetAsyncKeyState(vk_coin) && !coin_counter_key_state) {
        coin_counter++;
        coin_counter_key_state = true;
    } else if (!GetAsyncKeyState(vk_coin)) {
        coin_counter_key_state = false;
    }

    // ------------------------------------

    if (opbtn & 0x01) {
        state->buttons[0] |= IO4_BUTTON_TEST;
    }

    if (opbtn & 0x02) {
        state->buttons[0] |= IO4_BUTTON_SERVICE;
    }

    state->chutes[0] = coin_counter << 8;

    return S_OK;
}
