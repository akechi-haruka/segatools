#include <windows.h>

#include <process.h>
#include <stdbool.h>
#include <stdint.h>

#include "chuniio/chuniio.h"
#include "chuniio/config.h"

#include "api/api.h"

#include "util/dprintf.h"

static unsigned int __stdcall chuni_io_slider_thread_proc(void *ctx);

static bool chuni_io_coin;
static uint16_t chuni_io_coins;
static uint8_t chuni_io_hand_pos;
static HANDLE chuni_io_slider_thread;
static bool chuni_io_slider_stop_flag;
static struct chuni_io_config chuni_io_cfg;

static HANDLE led_fd = NULL;
static uint8_t led_header[2] = {0xAA, 0xAA};
static uint8_t led_colors[96] = {0};
static volatile uint32_t led_colors_hash, led_colors_prevhash;

uint16_t chuni_io_get_api_version(void)
{
    return 0x0101;
}

HRESULT chuni_io_jvs_init(void)
{
    chuni_io_config_load(&chuni_io_cfg, L".\\segatools.ini");

    return S_OK;
}

void chuni_io_jvs_read_coin_counter(uint16_t *out)
{
    if (out == NULL) {
        return;
    }

    if (GetAsyncKeyState(chuni_io_cfg.vk_coin) || api_get_credit_button()) {
        if (!chuni_io_coin) {
            chuni_io_coin = true;
            chuni_io_coins++;
            dprintf("Chuni coin\n");
			api_clear_buttons();
        }
    } else {
        chuni_io_coin = false;
    }

    *out = chuni_io_coins;
}

void chuni_io_jvs_poll(uint8_t *opbtn, uint8_t *beams)
{
    size_t i;

    if (GetAsyncKeyState(chuni_io_cfg.vk_test) || api_get_test_button()) {
        *opbtn |= 0x01; /* Test */
		api_clear_buttons();
    }

    if (GetAsyncKeyState(chuni_io_cfg.vk_service) || api_get_service_button()) {
        *opbtn |= 0x02; /* Service */
		api_clear_buttons();
    }

    if (chuni_io_cfg.vk_enable_ir){

        for (i = 0 ; i < 6 ; i++) {
            if (GetAsyncKeyState(chuni_io_cfg.vk_ir_arr[i])){
                *beams |= (1 << i);
            }
        }

    } else {

        if (GetAsyncKeyState(chuni_io_cfg.vk_ir)) {
            if (chuni_io_hand_pos < 6) {
                chuni_io_hand_pos++;
            }
        } else {
            if (chuni_io_hand_pos > 0) {
                chuni_io_hand_pos--;
            }
        }

        for (i = 0 ; i < 6 ; i++) {
            if (chuni_io_hand_pos > i) {
                *beams |= (1 << i);
            }
        }

    }

}

HRESULT chuni_io_slider_init(void)
{
    return S_OK;
}

void chuni_io_slider_start(chuni_io_slider_callback_t callback)
{
    if (chuni_io_slider_thread != NULL) {
        return;
    }

    chuni_io_slider_thread = (HANDLE) _beginthreadex(
            NULL,
            0,
            chuni_io_slider_thread_proc,
            callback,
            0,
            NULL);
}

void chuni_io_slider_stop(void)
{
    if (chuni_io_slider_thread == NULL) {
        return;
    }

    chuni_io_slider_stop_flag = true;

    WaitForSingleObject(chuni_io_slider_thread, INFINITE);
    CloseHandle(chuni_io_slider_thread);
    chuni_io_slider_thread = NULL;
    chuni_io_slider_stop_flag = false;
}

void chuni_io_slider_set_leds(const uint8_t *rgb)
{
	int i;
    if (led_fd == NULL) {
        return;
    }
    // Remap GBR to RGB
    if (chuni_io_cfg.led_type == 1){
        for (int i = 0; i < 16; i++) {
            led_colors[i*3+0] = rgb[i*6+1];
            led_colors[i*3+1] = rgb[i*6+2];
            led_colors[i*3+2] = rgb[i*6+0];
        }
    } else if (chuni_io_cfg.led_type == 2){
       for (int i = 0; i < 16; i++) {
           led_colors[(i*2)*3+1] = rgb[i*6+1];
           led_colors[(i*2)*3+2] = rgb[i*6+2];
           led_colors[(i*2)*3+0] = rgb[i*6+0];
       }
   } else if (chuni_io_cfg.led_type == 3){
       for (int i = 0; i < 16; i++) {
           led_colors[(i*2)*3+1] = rgb[(15-i)*6+1];
           led_colors[(i*2)*3+2] = rgb[(15-i)*6+2];
           led_colors[(i*2)*3+0] = rgb[(15-i)*6+0];
       }
   }
    if (chuni_io_cfg.led_brightness < 100){
        for (i = 0; i < 96; i++) {
            led_colors[i] = (int)(led_colors[i] * (chuni_io_cfg.led_brightness / (float)100));
        }
    }
    // Detect change in current LED colors by simple FNV hash
    // Only send colors to COM port if changed to reduce bus load (useful?)
    uint32_t hash = 0x811c9dc5;
    for (i = 0; i < 96; i++) {
        hash ^= rgb[i];
        hash *= 0x01000193;
    }
    if (led_colors_hash != hash) {
        //dprintf("chuni_io_slider_set_leds: %d", hash);
        led_colors_hash = hash;
    }
    // Actual COM write will happen in slider thread below
}

static void chuni_io_led_init(struct chuni_io_config *cfg)
{
    dprintf("ChuniLED: LED Init called\n");
    if (!cfg->led_port) {
        return;
    }
    char comname[8];
    snprintf(comname, sizeof(comname), "COM%d", cfg->led_port);
    led_fd = CreateFile(comname,
                        GENERIC_READ | GENERIC_WRITE,
                        0,
                        0,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        0);
    if (led_fd == INVALID_HANDLE_VALUE) {
        dprintf("ChuniLED: Cannot open LED port %s: %lx\n", comname, GetLastError());
		led_fd = NULL;
        return;
    }
    // Set COM parameters
    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(led_fd, &dcbSerialParams)) {
        dprintf("ChuniLED: Cannot get LED COM port parameters: %lx\n", GetLastError());
        return;
    }
    dcbSerialParams.BaudRate = cfg->led_rate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    if (!SetCommState(led_fd, &dcbSerialParams)) {
        dprintf("ChuniLED: Cannot set LED COM port parameters: %lx\n", GetLastError());
        return;
    }
    // Set COM timeout to nonblocking reads
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    if (!SetCommTimeouts(led_fd, &timeouts)) {
        dprintf("ChuniLED: Cannot set LED COM timeouts: %lx\n", GetLastError());
        return;
    }
	dprintf("ChuniLED: Initialized successfully (COM%d, type:%d)\n", cfg->led_port, cfg->led_type);
}


static unsigned int __stdcall chuni_io_slider_thread_proc(void *ctx)
{

	chuni_io_led_init(&chuni_io_cfg);

    chuni_io_slider_callback_t callback;
    uint8_t pressure[32];
    size_t i;
    DWORD n;
    ssize_t ret;

    callback = ctx;

    while (!chuni_io_slider_stop_flag) {
        for (i = 0 ; i < _countof(pressure) ; i++) {
            if (GetAsyncKeyState(chuni_io_cfg.vk_cell[i]) & 0x8000) {
                pressure[i] = 128;
            } else {
                pressure[i] = 0;
            }
        }

        callback(pressure);

		if (led_fd != NULL) {
            if (led_colors_hash != led_colors_prevhash) {
                ret = WriteFile(led_fd, led_header, sizeof(led_header), &n, NULL);
                if (!ret || n != sizeof(led_header)) {
					dprintf("ChuniLED: LED COM Write failed: %lx\n", GetLastError());
					led_fd = NULL;
				} else {
					ret = WriteFile(led_fd, led_colors, sizeof(led_colors), &n, NULL);
					if (!ret || n != sizeof(led_colors)) {
						dprintf("ChuniLED: LED COM Write failed: %lx\n", GetLastError());
						led_fd = NULL;
					} else {
				        //dprintf("ChuniLED write ok: %d\n", led_colors_hash);
						led_colors_prevhash = led_colors_hash;
						if (chuni_io_cfg.led_type >= 2){
                            ret = WriteFile(led_fd, led_header, sizeof(led_header), &n, NULL);
                            if (!ret || n != sizeof(led_header)) {
                                dprintf("ChuniLED: LED COM Write failed: %lx\n", GetLastError());
                                led_fd = NULL;
                            } else {
                                //dprintf("ChuniLED write padding ok\n");
                            }
                        }
					}
				}
            }
        }

        Sleep(1);
    }

	if (led_fd != INVALID_HANDLE_VALUE && led_fd != NULL) {
        CloseHandle(led_fd);
    }

    return 0;
}
