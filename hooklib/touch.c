/*
This part (touch screen hook) is mostly taken from spicetools, which is GPL.
This means there can be some license issues if you do use this code in some other places without including source code.
*/
#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "hook/com-proxy.h"
#include "hook/table.h"

#include "hooklib/config.h"
#include "hooklib/dll.h"
#include "hooklib/touch.h"

#include "util/dprintf.h"

/* API hooks */

static int WINAPI hook_GetSystemMetrics(
    int nIndex
);

static BOOL WINAPI hook_RegisterTouchWindow(
    HWND hwnd,
    ULONG ulFlags
);

static BOOL WINAPI hook_GetTouchInputInfo(
    HANDLE hTouchInput,
    UINT cInputs,
    PTOUCHINPUT pInputs,
    int cbSize
);

static BOOL WINAPI hook_SetCursor(
    HCURSOR hCursor
);

/* Link pointers */

static int (WINAPI *next_GetSystemMetrics)(
    int nIndex
);

static BOOL (WINAPI *next_RegisterTouchWindow)(
    HWND hwnd,
    ULONG ulFlags
);

static BOOL (WINAPI *next_GetTouchInputInfo)(
    HANDLE hTouchInput,
    UINT cInputs,
    PTOUCHINPUT pInputs,
    int cbSize
);
static BOOL (WINAPI *next_SetCursor)(
    HCURSOR hCursor
);

static bool touch_hook_initted;
static struct touch_config touch_config;
HWND game_wnd = NULL;
HWND emu_touch_wnd = NULL;
static uint8_t last_mouse_state = 0;
static POINTER_TOUCH_INFO contact;
static bool touch_window_initted = false;

static HWND get_game_window(){
	if (game_wnd != NULL){
		return game_wnd;
	}
	char wnd_title[256];
	HWND hwnd = GetForegroundWindow();
	GetWindowText(hwnd, wnd_title, sizeof(wnd_title));
	if (strcmp(wnd_title, "KANCOLLE Arcade") == 0){
		game_wnd = hwnd;
		return game_wnd;
	}
	return NULL;
}

static const struct hook_symbol touch_hooks[] = {
    {
        .name   = "GetSystemMetrics",
        .patch  = hook_GetSystemMetrics,
        .link   = (void **) &next_GetSystemMetrics
    },
    {
        .name   = "RegisterTouchWindow",
        .patch  = hook_RegisterTouchWindow,
        .link   = (void **) &next_RegisterTouchWindow
    },
    {
        .name   = "GetTouchInputInfo",
        .patch  = hook_GetTouchInputInfo,
        .link   = (void **) &next_GetTouchInputInfo
    },
    {
        .name   = "SetCursor",
        .patch  = hook_SetCursor,
        .link   = (void **) &next_SetCursor
    },
};

DWORD WINAPI touch_thread(void* data){
    do {
        Sleep(100);
    } while (!touch_window_initted);
    dprintf("touch: thread start\n");
    do {

        if (touch_config.touch_mode == 0){
            return 0;
        } else if (touch_config.touch_mode == 1 && (!touch_config.focused_only || GetForegroundWindow() == get_game_window())){

            bool mouse_clicked = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

            if (mouse_clicked){
                if (last_mouse_state >= 1){
                    contact.pointerInfo.pointerFlags = POINTER_FLAG_UPDATE | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT;
                    last_mouse_state = 3;
                } else {
                    contact.pointerInfo.pointerFlags = POINTER_FLAG_DOWN | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT;
                    last_mouse_state = 2;
                }

                POINT cursorPos;
                if (!GetCursorPos(&cursorPos)){
                    dprintf("touch: GetCursorPos failed: %lx\n", GetLastError());
                }
                contact.pointerInfo.ptPixelLocation.y = cursorPos.y; // Y co-ordinate of touch on screen
                contact.pointerInfo.ptPixelLocation.x = cursorPos.x; // X co-ordinate of touch on screen
                contact.rcContact.top = contact.pointerInfo.ptPixelLocation.y - 2;
                contact.rcContact.bottom = contact.pointerInfo.ptPixelLocation.y + 2;
                contact.rcContact.left = contact.pointerInfo.ptPixelLocation.x  - 2;
                contact.rcContact.right = contact.pointerInfo.ptPixelLocation.x  + 2;
            } else {
                if (last_mouse_state >= 2){
                    contact.pointerInfo.pointerFlags = POINTER_FLAG_UP;
                    last_mouse_state = 1;
                } else {
                    contact.pointerInfo.pointerFlags = POINTER_FLAG_NONE;
                    last_mouse_state = 0;
                }
            }

            /*if (last_mouse_state > 0) {
                WNDPROC wndProc = (WNDPROC) GetWindowLongPtr(emu_touch_wnd, GWLP_WNDPROC);
                wndProc(emu_touch_wnd, WM_TOUCH, MAKEWORD(1, 0), (LPARAM) hook_GetTouchInputInfo);
            }*/
            if (last_mouse_state > 0){
                if (!SendMessage(emu_touch_wnd, WM_TOUCH, MAKEWORD(1, 0), (LPARAM) hook_GetTouchInputInfo)){
                    // lets uhhh not question this
                    //dprintf("touch: failed update2(state=%d) %p: %lx\n", last_mouse_state, emu_touch_wnd, GetLastError());
                }
                if (!InjectTouchInput(1, &contact)){
                    //dprintf("touch: failed update: %lx\n", GetLastError());
                }
            }

            #if 0
            if (last_mouse_state != 0){
                dprintf("touch: state=%d\n", last_mouse_state);
            }
            #endif

        } else if (touch_config.touch_mode == 2){
            // all good
        }
        Sleep(1);
    } while(true);
    return 0;
}

void touch_hook_init(const struct touch_config *cfg, HINSTANCE self, long long mode_ptr, long long connected_ptr)
{
    assert(cfg != NULL);

    if (!cfg->enable) {
        return;
    }

    if (touch_hook_initted) {
        return;
    }

    touch_hook_initted = true;

    memcpy(&touch_config, cfg, sizeof(*cfg));
    hook_table_apply(NULL, "user32.dll", touch_hooks, _countof(touch_hooks));


    if (!InitializeTouchInjection(1, TOUCH_FEEDBACK_DEFAULT)){
        dprintf("touch: failed initialization: %lx\n", GetLastError());
    }

    memset(&contact, 0, sizeof(POINTER_TOUCH_INFO));

    contact.pointerInfo.pointerType = PT_TOUCH;
    contact.pointerInfo.pointerId = 0;
    contact.pointerInfo.ptPixelLocation.y = 200; // Y co-ordinate of touch on screen
    contact.pointerInfo.ptPixelLocation.x = 300; // X co-ordinate of touch on screen

    contact.touchFlags = TOUCH_FLAG_NONE;
    contact.touchMask = TOUCH_MASK_CONTACTAREA | TOUCH_MASK_ORIENTATION | TOUCH_MASK_PRESSURE;
    contact.orientation = 90; // Orientation of 90 means touching perpendicular to screen.
    contact.pressure = 32000;

    // defining contact area (I have taken area of 4 x 4 pixel)
    contact.rcContact.top = contact.pointerInfo.ptPixelLocation.y - 2;
    contact.rcContact.bottom = contact.pointerInfo.ptPixelLocation.y + 2;
    contact.rcContact.left = contact.pointerInfo.ptPixelLocation.x  - 2;
    contact.rcContact.right = contact.pointerInfo.ptPixelLocation.x  + 2;

    if (CreateThread(NULL, 0, touch_thread, NULL, 0, NULL) == NULL){
        dprintf("touch: CreateThread fail: %lx\n", GetLastError());
    }

    dprintf("touch: hook enabled (mode=%d, passthru=%d).\n", cfg->touch_mode, cfg->passthru);
}

// Spicetools misc/wintouchemu.cpp

static int WINAPI hook_GetSystemMetrics(
    int nIndex
)
{
    if (nIndex == 94) return 0x01 | 0x02 | 0x40 | 0x80;

    int orig = next_GetSystemMetrics(nIndex);

    return orig;
}

static BOOL WINAPI hook_RegisterTouchWindow(
    HWND hwnd,
    ULONG ulFlags
)
{
    dprintf("touch: registered touch window for emulation\n");
    emu_touch_wnd = hwnd;
    touch_window_initted = true;

    if (touch_config.cursor){
        if (!ShowCursor(TRUE)){
            dprintf("touch: ShowCursor failed: %lx\n", GetLastError());
        }
    }

    // requires the dll to be digitally signed (lmfao)
    /*if (touch_config.touch_mode == 1 && !RegisterPointerInputTarget(hwnd, PT_TOUCH)){
        dprintf("touch: RegisterPointerInputTarget failed: %lx\n", GetLastError());
    }*/

    if (touch_config.passthru){
        return next_RegisterTouchWindow(hwnd, ulFlags);
    }

    return true;
}

// Converting mouse event to touch event
// Does not work at current stage
static BOOL WINAPI hook_GetTouchInputInfo(
    HANDLE hTouchInput,
    UINT cInputs,
    PTOUCHINPUT pInputs,
    int cbSize
)
{

    //dprintf("GetTouchInputInfo\n");

    if (touch_config.passthru){
        return next_GetTouchInputInfo(hTouchInput, cInputs, pInputs, cbSize);
    }

    bool result = false;
    for (UINT input = 0; input < cInputs; input++) {
        TOUCHINPUT *touch_input = &pInputs[input];

        touch_input->x = 0;
        touch_input->y = 0;
        touch_input->hSource = NULL;
        touch_input->dwID = 0;
        touch_input->dwFlags = 0;
        touch_input->dwMask = 0;
        touch_input->dwTime = 0;
        touch_input->dwExtraInfo = 0;
        touch_input->cxContact = 0;
        touch_input->cyContact = 0;

        if (last_mouse_state > 0) {
            POINT cursorPos;
            GetCursorPos(&cursorPos);

            result = true;
            touch_input->x = cursorPos.x * 100;
            touch_input->y = cursorPos.y * 100;
            touch_input->hSource = hTouchInput;
            touch_input->dwID = 0;
            touch_input->dwFlags = 0;
            if (last_mouse_state == 2) {
                touch_input->dwFlags |= TOUCHEVENTF_DOWN;
            } else if (last_mouse_state == 3) {
                touch_input->dwFlags |= TOUCHEVENTF_MOVE;
            } else if (last_mouse_state == 1) {
                touch_input->dwFlags |= TOUCHEVENTF_UP;
            }
            touch_input->dwMask = 0;
            touch_input->dwTime = 0;
            touch_input->dwExtraInfo = 0;
            touch_input->cxContact = 0;
            touch_input->cyContact = 0;
        }
    }

    return result;
}

static BOOL WINAPI hook_SetCursor(
    HCURSOR hCursor
){

    if (touch_config.cursor){
        return TRUE;
    }

    return next_SetCursor(hCursor);

}
