#pragma once

struct window_config {
    bool enable;
    int width;
    int height;
};

void window_hook_init(const struct window_config *cfg, const wchar_t *wnd);
