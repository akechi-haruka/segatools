#pragma once

#include "board/rfid-frame.h"

enum {
    RFID_CMD_RESET = 0x41,
    RFID_CMD_GET_BOOT_VERSION = 0x84,
    RFID_CMD_BOARD_INFO = 0x85,
    RFID_CMD_UNK_INIT_2 = 0x81,
    RFID_CMD_GET_APP_VERSION = 0x42,
    RFID_CMD_UNK_INIT_4 = 0x04,
    RFID_CMD_UNK_INIT_5 = 0x05,
    RFID_CMD_SCAN_OR_WRITE = 0x06,
    RFID_CMD_FW_UPDATE = 0x82,
    RFID_CMD_WRITE_DATA_START_STOP = 0x02,
    RFID_CMD_WRITE_DATA_BLOCK = 0x03,
};

enum {
    RFID_SCAN_START = 0x81,
    RFID_SCAN_CARD_DATA = 0x82,
    RFID_SCAN_END = 0x83,
};

struct rfid_req_any {
    struct rfid_hdr hdr;
    uint8_t cmd;
    uint8_t len;
    uint8_t payload[256];
};

struct rfid_resp_any {
    struct rfid_hdr hdr;
    uint8_t cmd;
    uint8_t unknown;
    uint8_t len;
    uint8_t data[256];
};

struct rfid_resp_empty {
    struct rfid_hdr hdr;
    uint8_t cmd;
    uint8_t unknown;
    uint8_t len;
};

struct rfid_resp_board_info {
    struct rfid_hdr hdr;
    uint8_t cmd;
    uint8_t unknown;
    uint8_t len;
    struct {
        char board_num[9];
    } data;
};

struct rfid_resp_unk1 {
    struct rfid_hdr hdr;
    uint8_t cmd;
    uint8_t unknown;
    uint8_t len;
    struct {
        uint8_t unk1;
    } data;
};

struct rfid_resp_card_start {
    struct rfid_hdr hdr;
    uint8_t cmd;
    uint8_t unknown;
    uint8_t len;
};

struct rfid_resp_card_data {
    struct rfid_hdr hdr;
    uint8_t cmd;
    uint8_t unknown;
    uint8_t len;
    struct {
        uint8_t unknown[44];
    } data;
};

struct rfid_resp_card_end {
    struct rfid_hdr hdr;
    uint8_t cmd;
    uint8_t unknown;
    uint8_t len;
};

struct rfid_resp_write_reset {
    struct rfid_hdr hdr;
    uint8_t cmd;
    uint8_t unknown;
    uint8_t len;
    struct {
        uint8_t unknown[80];
    } data;
};

struct rfid_req_write_start {
    struct rfid_hdr hdr;
    uint8_t cmd;
    uint8_t len;
    struct {
        uint8_t unk[4];
        uint8_t cardid[12];
    } data;
};

struct rfid_resp_write_start {
    struct rfid_hdr hdr;
    uint8_t cmd;
    uint8_t unknown;
    uint8_t len;
    struct {
        uint8_t unknown[32];
    } data;
};

struct rfid_req_write_block {
    struct rfid_hdr hdr;
    uint8_t cmd;
    uint8_t len;
    struct {
        uint8_t unk1[2];
        uint8_t block_num;
        uint8_t block_data[2];
        uint8_t unk2[27];
    } data;
};

struct rfid_resp_write_block {
    struct rfid_hdr hdr;
    uint8_t cmd;
    uint8_t unknown;
    uint8_t len;
    struct {
        uint8_t unknown[32];
    } data;
};
