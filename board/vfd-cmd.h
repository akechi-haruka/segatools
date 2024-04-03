#pragma once

#include "board/vfd-frame.h"

enum {
    VFD_CMD_GET_VERSION = 0x5B,
    VFD_CMD_RESET = 0x0B,
    VFD_CMD_CLEAR_SCREEN = 0x0C,
    VFD_CMD_SET_BRIGHTNESS = 0x20,
    VFD_CMD_SET_SCREEN_ON = 0x21,
    VFD_CMD_SET_H_SCROLL = 0x22,
    VFD_CMD_DRAW_IMAGE = 0x2E,
    VFD_CMD_SET_CURSOR = 0x30,
    VFD_CMD_SET_ENCODING = 0x32,
    VFD_CMD_SET_TEXT_WND = 0x40,
    VFD_CMD_SET_TEXT_SPEED = 0x41,
    VFD_CMD_WRITE_TEXT = 0x50,
    VFD_CMD_ENABLE_SCROLL = 0x51,
    VFD_CMD_DISABLE_SCROLL = 0x52,
    VFD_CMD_ROTATE = 0x5D,
    VFD_CMD_CREATE_CHAR = 0xA3,
    VFD_CMD_CREATE_CHAR2 = 0xA4,
};

enum {
    VFD_ENC_GB2312    = 0,
    VFD_ENC_BIG5      = 1,
    VFD_ENC_SHIFT_JIS = 2,
    VFD_ENC_KSC5601   = 3,
    VFD_ENC_MAX = 3,
};

struct vfd_req_hdr {
    uint8_t sync;
    uint8_t cmd;
};

struct vfd_req_any {
    struct vfd_req_hdr hdr;
    uint8_t payload[2054];
};

struct vfd_req_board_info {
    struct vfd_req_hdr hdr;
    uint8_t unk1;
};

struct vfd_resp_board_info { // \x0201.20\x03
    uint8_t unk1;
    char version[5];
    uint8_t unk2;
};

struct vfd_req_reset {
    struct vfd_req_hdr hdr;
};

struct vfd_req_cls {
    struct vfd_req_hdr hdr;
};

struct vfd_req_brightness {
    struct vfd_req_hdr hdr;
    uint8_t brightness;
};

struct vfd_req_power {
    struct vfd_req_hdr hdr;
    uint8_t power_state;
};

struct vfd_req_hscroll {
    struct vfd_req_hdr hdr;
    uint8_t x_pos;
};

struct vfd_req_draw {
    struct vfd_req_hdr hdr;
    uint16_t x0;
    uint8_t y0;
    uint16_t x1;
    uint8_t y1;
    uint8_t image[2048];
};

struct vfd_req_cursor {
    struct vfd_req_hdr hdr;
    uint16_t x;
    uint8_t y;
};

struct vfd_req_encoding {
    struct vfd_req_hdr hdr;
    uint8_t encoding;
};

struct vfd_req_wnd {
    struct vfd_req_hdr hdr;
    uint16_t x0;
    uint8_t y0;
    uint16_t x1;
    uint8_t y1;
};

struct vfd_req_speed {
    struct vfd_req_hdr hdr;
    uint8_t encoding;
};

struct vfd_req_scroll {
    struct vfd_req_hdr hdr;
};

struct vfd_req_rotate {
    struct vfd_req_hdr hdr;
    uint8_t unk1;
};

struct vfd_req_create_char {
    struct vfd_req_hdr hdr;
    uint8_t type;
    uint8_t pixels[32];
};
