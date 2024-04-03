#include <windows.h>

#include "util/dprintf.h"
#include "util/foreground-detect.h"

static HWND window_handle;
static const char* window_title = NULL;
static bool foreground_state;

static HWND get_game_window(){
	if (window_handle != NULL){
		return window_handle;
	}
	char wnd_title[256];
	HWND hwnd = GetForegroundWindow();
	GetWindowText(hwnd, wnd_title, sizeof(wnd_title));
	if (strcmp(wnd_title, window_title) == 0){
		dprintf("FG-Detect: Program Window detected\n");
		window_handle = hwnd;
		return window_handle;
	}
	return NULL;
}

void fgdet_init(const char* wnd_title){
    assert(wnd_title != NULL);

    window_handle = NULL;
    window_title = wnd_title;
    dprintf("FG-Detect: Searching for \"%s\"\n", window_title);
}

bool fgdet_in_foreground(){
    return foreground_state;
}

void fgdet_poll(){
    if (window_title == NULL){
        return;
    } else if (GetForegroundWindow() == get_game_window()){
        if (!foreground_state){
            dprintf("FG-Detect: Got focus\n");
            foreground_state = true;
        }
    } else {
        if (foreground_state){
            dprintf("FG-Detect: Lost focus\n");
            foreground_state = false;
        }
    }
}
