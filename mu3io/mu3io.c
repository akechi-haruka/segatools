#include <windows.h>
#include <xinput.h>

#include <limits.h>
#include <stdint.h>

#include "api/api.h"

#include "mu3io/mu3io.h"
#include "mu3io/config.h"

#include "util/dprintf.h"

static uint8_t mu3_opbtn;
static uint8_t mu3_left_btn;
static uint8_t mu3_right_btn;
static int16_t mu3_lever_pos;
static int16_t mu3_lever_xpos;
static uint8_t focus_state = 1;
static HWND game_wnd = NULL;

static struct mu3_io_config mu3_io_cfg;

static HWND get_game_window(){
	if (game_wnd != NULL){
		return game_wnd;
	}
	char wnd_title[256];
	HWND hwnd = GetForegroundWindow();
	GetWindowText(hwnd, wnd_title, sizeof(wnd_title));
	if (strcmp(wnd_title, "Otoge") == 0){
		dprintf("mu3io: Game Window detected\n");
		game_wnd = hwnd;
		return game_wnd;
	}
	return NULL;
}

uint16_t mu3_io_get_api_version(void)
{
    return 0x0100;
}

HRESULT mu3_io_init(void)
{
	mu3_lever_pos = 0;
    mu3_io_config_load(&mu3_io_cfg, L".\\segatools.ini");
    return S_OK;
}

HRESULT mu3_io_poll(void)
{
    if (mu3_io_cfg.background_input_allowed || GetForegroundWindow() == get_game_window()){
		if (!focus_state){
			dprintf("mu3io: Got focus\n");
			focus_state = 1;
		}

		int lever;
		int xlever;
		XINPUT_STATE xi;
		WORD xb;

		mu3_opbtn = 0;
		mu3_left_btn = 0;
		mu3_right_btn = 0;

		if (GetAsyncKeyState(mu3_io_cfg.vk_test) & 0x8000 || api_get_test_button()) {
			mu3_opbtn |= MU3_IO_OPBTN_TEST;
		    api_clear_buttons();
		}

		if (GetAsyncKeyState(mu3_io_cfg.vk_service) & 0x8000 || api_get_service_button()) {
			mu3_opbtn |= MU3_IO_OPBTN_SERVICE;
		    api_clear_buttons();
		}

		memset(&xi, 0, sizeof(xi));
		XInputGetState(0, &xi);
		xb = xi.Gamepad.wButtons;

		if(mu3_io_cfg.input_mode == 2){
			if (GetAsyncKeyState(mu3_io_cfg.vk_la) & 0x8000) {
				mu3_left_btn |= MU3_IO_GAMEBTN_1;
			}

			if (GetAsyncKeyState(mu3_io_cfg.vk_lb) & 0x8000) {
				mu3_left_btn |= MU3_IO_GAMEBTN_2;
			}

			if (GetAsyncKeyState(mu3_io_cfg.vk_lc) & 0x8000) {
				mu3_left_btn |= MU3_IO_GAMEBTN_3;
			}

			if (GetAsyncKeyState(mu3_io_cfg.vk_ra) & 0x8000) {
				mu3_right_btn |= MU3_IO_GAMEBTN_1;
			}

			if (GetAsyncKeyState(mu3_io_cfg.vk_rb) & 0x8000) {
				mu3_right_btn |= MU3_IO_GAMEBTN_2;
			}

			if (GetAsyncKeyState(mu3_io_cfg.vk_rc) & 0x8000) {
				mu3_right_btn |= MU3_IO_GAMEBTN_3;
			}

			if (GetAsyncKeyState(mu3_io_cfg.vk_lm) & 0x8000) {
				mu3_left_btn |= MU3_IO_GAMEBTN_MENU;
			}

			if (GetAsyncKeyState(mu3_io_cfg.vk_rm) & 0x8000) {
				mu3_right_btn |= MU3_IO_GAMEBTN_MENU;
			}

			if (GetAsyncKeyState(mu3_io_cfg.vk_ls) & 0x8000) {
				mu3_left_btn |= MU3_IO_GAMEBTN_SIDE;
			}

			if (GetAsyncKeyState(mu3_io_cfg.vk_rs) & 0x8000) {
				mu3_right_btn |= MU3_IO_GAMEBTN_SIDE;
			}
		}
		else if(mu3_io_cfg.input_mode == 1){
			if (xb & XINPUT_GAMEPAD_DPAD_LEFT) {
				mu3_left_btn |= MU3_IO_GAMEBTN_1;
			}

			if (xb & XINPUT_GAMEPAD_DPAD_UP) {
				mu3_left_btn |= MU3_IO_GAMEBTN_2;
			}

			if (xb & XINPUT_GAMEPAD_A) {
				mu3_left_btn |= MU3_IO_GAMEBTN_3;
			}

			if (xb & XINPUT_GAMEPAD_X) {
				mu3_right_btn |= MU3_IO_GAMEBTN_1;
			}

			if (xb & XINPUT_GAMEPAD_Y) {
				mu3_right_btn |= MU3_IO_GAMEBTN_2;
			}

			if (xb & XINPUT_GAMEPAD_B) {
				mu3_right_btn |= MU3_IO_GAMEBTN_3;
			}

			if (xb & XINPUT_GAMEPAD_BACK) {
				mu3_left_btn |= MU3_IO_GAMEBTN_MENU;
			}

			if (xb & XINPUT_GAMEPAD_START) {
				mu3_right_btn |= MU3_IO_GAMEBTN_MENU;
			}

			if (xb & XINPUT_GAMEPAD_LEFT_SHOULDER) {
				mu3_left_btn |= MU3_IO_GAMEBTN_SIDE;
			}

			if (xb & XINPUT_GAMEPAD_RIGHT_SHOULDER) {
				mu3_right_btn |= MU3_IO_GAMEBTN_SIDE;
			}
		}



		lever = mu3_lever_pos;

		if(mu3_io_cfg.input_mode == 2){
			if (GetAsyncKeyState(mu3_io_cfg.vk_sliderLeft) & 0x8000) {
				lever -= mu3_io_cfg.sliderSpeed;
			}

			if (GetAsyncKeyState(mu3_io_cfg.vk_sliderRight) & 0x8000) {
				lever += mu3_io_cfg.sliderSpeed;
			}
		}
		else if(mu3_io_cfg.input_mode == 1){
			if (abs(xi.Gamepad.sThumbLX) > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
				lever += xi.Gamepad.sThumbLX / 24;
			}

			if (abs(xi.Gamepad.sThumbRX) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) {
				lever += xi.Gamepad.sThumbRX / 24;
			}
		}

		if (lever < INT16_MIN) {
			lever = INT16_MIN;
		}
		if (lever > INT16_MAX) {
			lever = INT16_MAX;
		}

		mu3_lever_pos = lever;

		xlever = mu3_lever_pos
						- xi.Gamepad.bLeftTrigger * 64
						+ xi.Gamepad.bRightTrigger * 64;

		if (xlever < INT16_MIN) {
			xlever = INT16_MIN;
		}

		if (xlever > INT16_MAX) {
			xlever = INT16_MAX;
		}

		mu3_lever_xpos = xlever;

	} else {
		if (focus_state){
			dprintf("mu3io: Lost focus\n");
			focus_state = 0;
		}
	}

    return S_OK;
}

void mu3_io_get_opbtns(uint8_t *opbtn)
{
    if (opbtn != NULL) {
        *opbtn = mu3_opbtn;
    }
}

void mu3_io_get_gamebtns(uint8_t *left, uint8_t *right)
{
    if (left != NULL) {
        *left = mu3_left_btn;
    }

    if (right != NULL ){
        *right = mu3_right_btn;
    }
}

void mu3_io_get_lever(int16_t *pos)
{
    if (pos != NULL) {
        *pos = mu3_lever_xpos;
    }
}
