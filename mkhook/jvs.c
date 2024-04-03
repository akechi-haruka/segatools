#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <xinput.h>

#include "api/api.h"

#include "jvs/jvs-bus.h"

#include "mkhook/config.h"

#include "namco/najv.h"

#include "util/dprintf.h"
#include "util/foreground-detect.h"

static void mk_jvs_read_switches(void *ctx, struct najv_switch_state *out);
static void mk_jvs_read_coin_counter(
        void *ctx,
        uint8_t slot_no,
        uint16_t *out);
static void mk_jvs_read_analogs(
        void *ctx,
        uint16_t *analogs,
        uint8_t nanalogs);

static const struct najv_ops mk_jvs_najv_ops = {
    .read_switches      = mk_jvs_read_switches,
    .read_analogs       = mk_jvs_read_analogs,
    .read_coin_counter  = mk_jvs_read_coin_counter,
};

static uint16_t steering = 0x7FFF;
static uint16_t accelerator = 0;
static uint16_t brake = 0;

static struct io_config config;
static struct najv mk_jvs_najv;


#include <tlhelp32.h>
DWORD FindProcessId(char* processName)
{
    // strip path

    char* p = strrchr(processName, '\\');
    if(p)
        processName = p+1;

    PROCESSENTRY32 processInfo;
    processInfo.dwSize = sizeof(processInfo);

    HANDLE processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if ( processesSnapshot == INVALID_HANDLE_VALUE )
        return 0;

    Process32First(processesSnapshot, &processInfo);
    if ( !strcmp(processName, processInfo.szExeFile) )
    {
        CloseHandle(processesSnapshot);
        return processInfo.th32ProcessID;
    }

    while ( Process32Next(processesSnapshot, &processInfo) )
    {
        if ( !strcmp(processName, processInfo.szExeFile) )
        {
          CloseHandle(processesSnapshot);
          return processInfo.th32ProcessID;
        }
    }

    CloseHandle(processesSnapshot);
    return 0;
}

void mk_jvs_init(struct io_config* cfg){

    assert(cfg != NULL);

    config = *cfg;

    if (config.foreground_only){
        fgdet_init("mkart3");
    }
}

HRESULT mk_jvs_node(struct jvs_node **out)
{
    assert(out != NULL);

    dprintf("JVS I/O: Starting JVS\n");

    najv_init(&mk_jvs_najv, NULL, &mk_jvs_najv_ops, NULL);
    *out = najv_to_jvs_node(&mk_jvs_najv);

    return S_OK;
}

static void mk_jvs_read_switches(void *ctx, struct najv_switch_state *out) {

    assert(out != NULL);

    fgdet_poll();

    static bool test_mode_toggle = false;

    if (test_mode_toggle){
        out->system |= 1 << 7;
    }

    if (config.foreground_only && !fgdet_in_foreground()){
        return;
    }

    if (GetAsyncKeyState(config.vk_exit) & 0x8000){
        ExitProcess(0);
        return;
    }

    static bool test_mode_down_prev = false;
    bool test_mode_down_now = (GetAsyncKeyState(config.vk_test_switch) & 0x8000) || api_get_test_button();

    if (test_mode_down_now && !test_mode_down_prev){
        api_clear_buttons();
        test_mode_toggle = !test_mode_toggle;
    }
    test_mode_down_prev = test_mode_down_now;


    /*
    system:7 = test
    p1:9 = ok
    p1:12 = down
    p1:13 = up
    14=service
    1=mariobutton
    4 and 5=itembutton
    */



    if ((GetAsyncKeyState(config.vk_service) & 0x8000) || api_get_service_button()){
        api_clear_buttons();
        out->p1 |= 1 << 14;
    }
    if (GetAsyncKeyState(config.vk_test_enter) & 0x8000){
        out->p1 |= 1 << 9;
    }
    if (GetAsyncKeyState(config.vk_test_down) & 0x8000){
        out->p1 |= 1 << 12;
    }
    if (GetAsyncKeyState(config.vk_test_up) & 0x8000){
        out->p1 |= 1 << 13;
    }

    if (config.input_mode == 1){
        if (GetAsyncKeyState(config.vk_mario_button) & 0x8000){
            out->p1 |= 1 << 1;
        }
        if (GetAsyncKeyState(config.vk_mario_button_alt) & 0x8000){
            out->p1 |= 1 << 1;
        }
        if (GetAsyncKeyState(config.vk_item_button) & 0x8000){
            out->p1 |= 1 << 4;
        }
        if (GetAsyncKeyState(config.vk_item_button_alt) & 0x8000){
            out->p1 |= 1 << 5;
        }

        if (config.keyboard_relative){
            int speed = 0x100;
            if (GetAsyncKeyState(config.vk_steering_left) & 0x8000){
                if ((uint16_t)(steering - speed) < steering){
                    steering -= speed;
                } else {
                    steering = 0;
                }
            } else if (GetAsyncKeyState(config.vk_steering_right) & 0x8000){
                if ((uint16_t)(steering + speed) > steering){
                    steering += speed;
                } else {
                    steering = 0xFFFF;
                }
            }
        } else {
            if (GetAsyncKeyState(config.vk_steering_left) & 0x8000){
                steering = 0;
            } else if (GetAsyncKeyState(config.vk_steering_right) & 0x8000){
                steering = 0xFFFF;
            } else {
                steering = 0x7FFF;
            }
        }

        if (GetAsyncKeyState(config.vk_accelerator) & 0x8000){
            accelerator = 0xFFFF;
        } else {
            accelerator = 0;
        }
        if (GetAsyncKeyState(config.vk_brake) & 0x8000){
            brake = 0xFFFF;
        } else {
            brake = 0;
        }
    } else {
		XINPUT_STATE xi;
		memset(&xi, 0, sizeof(xi));
        XInputGetState(0, &xi);
        WORD xb = xi.Gamepad.wButtons;

        if (xb & (XINPUT_GAMEPAD_X | XINPUT_GAMEPAD_Y)) {
            out->p1 |= 1 << 1;
        }
        if (xb & (XINPUT_GAMEPAD_A)) {
            out->p1 |= 1 << 4;
        }
        if (xb & (XINPUT_GAMEPAD_B)) {
            out->p1 |= 1 << 5;
        }

        if (abs(xi.Gamepad.sThumbLX) > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
            steering = xi.Gamepad.sThumbLX + (USHRT_MAX / 2);
        } else {
            steering = 0x7FFF;
        }

        accelerator = xi.Gamepad.bRightTrigger * 257;
        brake = xi.Gamepad.bLeftTrigger * 257;
    }
}

static void mk_jvs_read_analogs(
        void *ctx,
        uint16_t *analogs,
        uint8_t nanalogs)
{

    if (config.foreground_only && !fgdet_in_foreground()){
        return;
    }

    if (nanalogs > 0) {
        analogs[0] = steering;
    }

    if (nanalogs > 1) {
        analogs[1] = accelerator;
    }

    if (nanalogs > 2) {
        analogs[2] = brake;
    }
}

static void mk_jvs_read_coin_counter(
        void *ctx,
        uint8_t slot_no,
        uint16_t *out)
{
    if (slot_no > 0) {
        return;
    }

    static uint16_t coins = 0;
    static bool was_button_down = false;

    if (!config.foreground_only || fgdet_in_foreground()){

        bool is_button_down = GetAsyncKeyState(config.vk_coin) & 0x8000;

        if ((was_button_down && !is_button_down)){
            coins++;
        }

        was_button_down = is_button_down;
    }

    *out = coins;
}
