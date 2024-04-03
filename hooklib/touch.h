#pragma once

#include <windows.h>

#include <stdbool.h>

struct touch_config {
    bool enable;
    bool passthru;
    bool cursor;
    bool focused_only;
    uint16_t touch_mode;
};

/* Init is not thread safe because API hook init is not thread safe blah
    blah blah you know the drill by now. */

extern HWND emu_touch_wnd;

void touch_hook_init(const struct touch_config *cfg, HINSTANCE self, long long mode, long long connected);
