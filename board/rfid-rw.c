#include <windows.h>
#include <shlwapi.h>

#include <assert.h>
#include <process.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "board/rfid-cmd.h"
#include "board/rfid-frame.h"

#include "board/rfid-rw.h"

#include "hook/iobuf.h"
#include "hook/iohook.h"

#include "board/rfid-file.h"
#include "hooklib/uart.h"

#include "util/dprintf.h"
#include "util/dump.h"

#define SUPER_VERBOSE 1

#define IMAGE_SIZE 0x24FC00
#define CARD_ID_LEN 12

static HRESULT rfid_handle_irp(struct irp *irp);
static HRESULT rfid_handle_irp_locked(int board, struct irp *irp);

static HRESULT rfid_req_dispatch(int board, const struct rfid_req_any *req);
static HRESULT rfid_req_reset(int board, const struct rfid_req_any *req);
static HRESULT rfid_req_get_board_info(int board);
static HRESULT rfid_req_boot_version(int board, const struct rfid_req_any *req);
static HRESULT rfid_req_unk_init_2(int board, const struct rfid_req_any *req);
static HRESULT rfid_req_app_version(int board, const struct rfid_req_any *req);
static HRESULT rfid_req_unk_init_4(int board, const struct rfid_req_any *req);
static HRESULT rfid_req_unk_init_5(int board, const struct rfid_req_any *req);
static HRESULT rfid_req_scan(int board, const struct rfid_req_any *req);
static HRESULT rfid_req_fw_update(int board, const struct rfid_req_any *req);
static HRESULT rfid_req_write_reset(int board, const struct rfid_req_any *req);
static HRESULT rfid_req_write_start(int board, const struct rfid_req_any *req);
static HRESULT rfid_req_write_block(int board, const struct rfid_req_any *req);

static char rfid_board_num[10];
static const uint8_t* boot_fw_ver;
static const uint8_t* app_fw_ver;
static wchar_t path[MAX_PATH];

static int block_written_state = 0;
static HANDLE hFind = INVALID_HANDLE_VALUE;
static WIN32_FIND_DATAW ffd;

#define rfid_nboards 2

typedef struct {
    CRITICAL_SECTION lock;
    struct uart boarduart;
    uint8_t written_bytes[40960];
    uint8_t readable_bytes[40960];
    bool enable_response;
} _rfid_per_board_vars;

_rfid_per_board_vars rfid_per_board_vars[rfid_nboards];

void rfid_config_load(struct rfid_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    wchar_t tmpstr[16];

    memset(cfg->board_number, ' ', sizeof(cfg->board_number));

    cfg->enable = GetPrivateProfileIntW(L"rfid", L"enable", 1, filename);
    cfg->enable_bin_writing = GetPrivateProfileIntW(L"rfid", L"enable_bin_writing", 1, filename);
    cfg->ports[0] = GetPrivateProfileIntW(L"rfid", L"port_reader", 12, filename);
    cfg->ports[1] = GetPrivateProfileIntW(L"rfid", L"port_writer", 0, filename);
    cfg->boot_fw_ver[0] = GetPrivateProfileIntW(L"rfid", L"boot_fw_ver_reader", 0x90, filename);
    cfg->app_fw_ver[0] = GetPrivateProfileIntW(L"rfid", L"app_fw_ver_reader", 0x91, filename);
    cfg->boot_fw_ver[1] = GetPrivateProfileIntW(L"rfid", L"boot_fw_ver_writer", 0x90, filename);
    cfg->app_fw_ver[1] = GetPrivateProfileIntW(L"rfid", L"app_fw_ver_writer", 0x90, filename);

    GetPrivateProfileStringW(L"rfid", L"board_number_reader", L"837-15345", tmpstr, _countof(tmpstr), filename);
    GetPrivateProfileStringW(L"rfid", L"board_number_writer", L"837-15347", tmpstr, _countof(tmpstr), filename);
    size_t n = wcstombs(cfg->board_number, tmpstr, sizeof(cfg->board_number));
    for (int i = n; i < sizeof(cfg->board_number); i++)
    {
        cfg->board_number[i] = ' ';
    }

    GetPrivateProfileStringW(
                L"rfid",
                L"path",
                L"DEVICE\\card",
                cfg->path,
                _countof(cfg->path),
                filename);
}

HRESULT rfid_hook_init(const struct rfid_config *cfg)
{
    assert(cfg != NULL);

    if (!cfg->enable) {
        return S_FALSE;
    }

    memcpy(rfid_board_num, cfg->board_number, sizeof(rfid_board_num));
    memcpy(path, cfg->path, sizeof(path));
    boot_fw_ver = cfg->boot_fw_ver;
    app_fw_ver = cfg->app_fw_ver;

    for (int i = 0; i < rfid_nboards; i++)
    {
        _rfid_per_board_vars *v = &rfid_per_board_vars[i];

        InitializeCriticalSection(&v->lock);

        if (cfg->ports[i] > 0){
            uart_init(&v->boarduart, cfg->ports[i]);
            v->boarduart.written.bytes = v->written_bytes;
            v->boarduart.written.nbytes = sizeof(v->written_bytes);
            v->boarduart.readable.bytes = v->readable_bytes;
            v->boarduart.readable.nbytes = sizeof(v->readable_bytes);

            v->enable_response = true;
        }
    }
    dprintf("RFID: initialized (reader=%d, writer=%d)\n", cfg->ports[0], cfg->ports[1]);

    return iohook_push_handler(rfid_handle_irp);
}

char* rw(int board){
    if (board == 0){
        return "Reader";
    } else {
        return "Writer";
    }
}

static HRESULT rfid_handle_irp(struct irp *irp)
{
    HRESULT hr;

    assert(irp != NULL);

    for (int i = 0; i < rfid_nboards; i++)
    {
        _rfid_per_board_vars *v = &rfid_per_board_vars[i];
        struct uart *boarduart = &v->boarduart;

        if (uart_match_irp(boarduart, irp))
        {

            CRITICAL_SECTION lock = v->lock;

            EnterCriticalSection(&lock);
            hr = rfid_handle_irp_locked(i, irp);
            LeaveCriticalSection(&lock);

            return hr;
        }
    }

    return iohook_invoke_next(irp);
}

static HRESULT rfid_handle_irp_locked(int board, struct irp *irp)
{
    struct rfid_req_any req;
    struct iobuf req_iobuf;
    HRESULT hr;

    struct uart *boarduart = &rfid_per_board_vars[board].boarduart;

    hr = uart_handle_irp(boarduart, irp);

    if (irp->op == IRP_OP_READ && hFind != INVALID_HANDLE_VALUE){
        return rfid_req_scan(board, NULL);
    }

    if (FAILED(hr) || irp->op != IRP_OP_WRITE) {
        return hr;
    }

    for (;;) {

        if (&boarduart->written.nbytes == 0){
            continue;
        }

#if SUPER_VERBOSE
        dprintf("RFID[%s]: TX Buffer:\n", rw(board));
        dump_iobuf(&boarduart->written);
#endif

        req_iobuf.bytes = (byte*)&req;
        req_iobuf.nbytes = sizeof(req.hdr) + sizeof(req.cmd) + sizeof(req.payload);
        req_iobuf.pos = 0;

        hr = rfid_frame_decode(&req_iobuf, &boarduart->written);

        if (hr != S_OK) {
            if (hr != 1){
                dprintf("RFID[%s]: Deframe error: %x\n", rw(board), (int) hr);
            }
            return hr;
        }

#if 0
        dprintf("RFID[%s]: Deframe Buffer:\n", rw(board));
        dump_iobuf(&req_iobuf);
#endif

        hr = rfid_req_dispatch(board, &req);

        if (FAILED(hr)) {
            dprintf("RFID[%s]: Processing error: %x\n", rw(board), (int) hr);
        }
    }
}

static HRESULT rfid_req_dispatch(int board, const struct rfid_req_any *req)
{
    switch (req->cmd) {
    case RFID_CMD_RESET:
        return rfid_req_reset(board, req);

    case RFID_CMD_BOARD_INFO:
        return rfid_req_get_board_info(board);

    case RFID_CMD_GET_BOOT_VERSION:
        return rfid_req_boot_version(board, req);

    case RFID_CMD_UNK_INIT_2:
        return rfid_req_unk_init_2(board, req);

    case RFID_CMD_GET_APP_VERSION:
        return rfid_req_app_version(board, req);

    case RFID_CMD_UNK_INIT_4:
        return rfid_req_unk_init_4(board, req);

    case RFID_CMD_UNK_INIT_5:
        return rfid_req_unk_init_5(board, req);

    case RFID_CMD_SCAN_OR_WRITE:
        if (board == 0){
            return rfid_req_scan(board, req);
        } else {
            return rfid_req_write_reset(board, req);
        }

    case RFID_CMD_FW_UPDATE:
        return rfid_req_fw_update(board, req);

    case RFID_CMD_WRITE_DATA_START_STOP:
        return rfid_req_write_start(board, req);

    case RFID_CMD_WRITE_DATA_BLOCK:
        return rfid_req_write_block(board, req);

    default:
        dprintf("RFID[%s]: Unhandled command %02x\n", rw(board), req->cmd);

        return S_OK;
    }
}

static HRESULT rfid_req_reset(int board, const struct rfid_req_any *req)
{
    dprintf("RFID[%s]: Reset\n", rw(board));

    rfid_per_board_vars[board].enable_response = true;

    struct rfid_resp_empty resp;

    memset(&resp, 0, sizeof(resp));
    resp.hdr.sync = RFID_FRAME_SYNC;
    resp.cmd = RFID_CMD_RESET;
    resp.unknown = 0;
    resp.len = 0;

    return rfid_frame_encode(&rfid_per_board_vars[board].boarduart.readable, &resp, sizeof(resp));
}

static HRESULT rfid_req_get_board_info(int board)
{
    dprintf("RFID[%s]: Get board info\n", rw(board));

    struct rfid_resp_board_info resp;

    memset(&resp, 0, sizeof(resp));
    resp.hdr.sync = RFID_FRAME_SYNC;
    resp.cmd = RFID_CMD_BOARD_INFO;
    resp.unknown = 0;
    resp.len = sizeof(resp.data.board_num);

    memcpy(resp.data.board_num, rfid_board_num, sizeof(resp.data.board_num));

    return rfid_frame_encode(&rfid_per_board_vars[board].boarduart.readable, &resp, sizeof(resp));
}

static HRESULT rfid_req_boot_version(int board, const struct rfid_req_any *req)
{
    dprintf("RFID[%s]: Get boot firmware version\n", rw(board));

    struct rfid_resp_unk1 resp;

    memset(&resp, 0, sizeof(resp));
    resp.hdr.sync = RFID_FRAME_SYNC;
    resp.cmd = RFID_CMD_GET_BOOT_VERSION;
    resp.unknown = 0;
    resp.len = 1;
    resp.data.unk1 = boot_fw_ver[board];

    return rfid_frame_encode(&rfid_per_board_vars[board].boarduart.readable, &resp, sizeof(resp));
}

static HRESULT rfid_req_unk_init_2(int board, const struct rfid_req_any *req)
{
    dprintf("RFID[%s]: toRFIDNormalMode\n", rw(board));

    struct rfid_resp_empty resp;

    memset(&resp, 0, sizeof(resp));
    resp.hdr.sync = RFID_FRAME_SYNC;
    resp.cmd = RFID_CMD_UNK_INIT_2;
    resp.unknown = 0;
    resp.len = 0;

    return rfid_frame_encode(&rfid_per_board_vars[board].boarduart.readable, &resp, sizeof(resp));
}

static HRESULT rfid_req_app_version(int board, const struct rfid_req_any *req)
{
    dprintf("RFID[%s]: Get app firmware version\n", rw(board));

    struct rfid_resp_unk1 resp;

    memset(&resp, 0, sizeof(resp));
    resp.hdr.sync = RFID_FRAME_SYNC;
    resp.cmd = RFID_CMD_GET_APP_VERSION;
    resp.unknown = 0;
    resp.len = 1;
    resp.data.unk1 = app_fw_ver[board];

    return rfid_frame_encode(&rfid_per_board_vars[board].boarduart.readable, &resp, sizeof(resp));
}

static HRESULT rfid_req_unk_init_4(int board, const struct rfid_req_any *req)
{
    dprintf("RFID[%s]: Get unknown(4)\n", rw(board));

    struct rfid_resp_empty resp;

    memset(&resp, 0, sizeof(resp));
    resp.hdr.sync = RFID_FRAME_SYNC;
    resp.cmd = RFID_CMD_UNK_INIT_4;
    resp.unknown = 0;
    resp.len = 0;

    return rfid_frame_encode(&rfid_per_board_vars[board].boarduart.readable, &resp, sizeof(resp));
}

static HRESULT rfid_req_unk_init_5(int board, const struct rfid_req_any *req)
{
    dprintf("RFID[%s]: Get unknown(5)\n", rw(board));

    struct rfid_resp_empty resp;

    memset(&resp, 0, sizeof(resp));
    resp.hdr.sync = RFID_FRAME_SYNC;
    resp.cmd = RFID_CMD_UNK_INIT_5;
    resp.unknown = 0;
    resp.len = 0;

    return rfid_frame_encode(&rfid_per_board_vars[board].boarduart.readable, &resp, sizeof(resp));
}

static HRESULT rfid_req_scan(int board, const struct rfid_req_any *req)
{
    dprintf("RFID[%s]: Scanning request\n", rw(board));

    HRESULT hr = S_FALSE;

    wchar_t spath[MAX_PATH];
    wcscpy(spath, path);
    wcscat(spath, L"\\*.*");

    if (hFind == INVALID_HANDLE_VALUE){
        // start packet

        struct rfid_resp_card_start resp_start;

        memset(&resp_start, 0, sizeof(resp_start));
        resp_start.hdr.sync = RFID_FRAME_SYNC;
        resp_start.cmd = RFID_CMD_SCAN_OR_WRITE;
        resp_start.unknown = RFID_SCAN_START;
        resp_start.len = 0;

        hr = rfid_frame_encode(&rfid_per_board_vars[board].boarduart.readable, &resp_start, sizeof(resp_start));
        if (!SUCCEEDED(hr)){
            return hr;
        }

        hFind = FindFirstFileW(spath, &ffd);
        FindNextFileW(hFind, &ffd); // skip "."
        FindNextFileW(hFind, &ffd); // skip ".."
    }

    if (INVALID_HANDLE_VALUE != hFind) {

        //dprintf("RFID[%s]: Detected %ls\n", rw(board), ffd.cFileName);

        wchar_t* ext;
        ext = wcsrchr(ffd.cFileName, '.');
        bool is_image = wcscmp(ext, L".bmp") == 0;
        if (!is_image && wcscmp(ext, L".bin") != 0){
            dprintf("RFID[%s]: Invalid file (%ls): %ls\n", rw(board), ext, ffd.cFileName);
            goto SEND_END;
        }

        wchar_t fpath[MAX_PATH];
        PathCombineW(fpath, path, ffd.cFileName);
        dprintf("RFID[%s]: reading %ls...\n", rw(board), fpath);

        HANDLE hFile = CreateFileW(fpath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == NULL || hFile == INVALID_HANDLE_VALUE){
            dprintf("RFID[%s]: failed opening file: %lx\n", rw(board), GetLastError());
            goto SEND_END;
        }

        unsigned long nReadBytes;
        unsigned long skip = 0;

        if (is_image){

            skip = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + IMAGE_SIZE;
            DWORD ret = SetFilePointer(hFile, skip, 0, FILE_BEGIN);
            if (ret == INVALID_SET_FILE_POINTER){
                dprintf("RFID[%s]: failed reading image file: %lx\n", rw(board), GetLastError());
                CloseHandle(hFile);
                goto SEND_END;
            }

        }

        char data[255];

        //dprintf("ReadFile: size=%ld, skip=%ld\n", ffd.nFileSizeLow, skip);
        if (!ReadFile(hFile, data, ffd.nFileSizeLow - skip, &nReadBytes, NULL)){
            dprintf("RFID[%s]: failed reading file: %lx\n", rw(board), GetLastError());
            CloseHandle(hFile);
            goto SEND_END;
        }

        CloseHandle(hFile);

        if (nReadBytes == 0){
            dprintf("RFID[%s]: empty file, skipping\n", rw(board));
            CloseHandle(hFile);
            goto SEND_END;
        }
        if (nReadBytes != 44){
            dprintf("RFID[%s]: suspicious card size: %ld\n", rw(board), nReadBytes);
        }

        struct rfid_resp_card_data resp_data;

        memset(&resp_data, 0, sizeof(resp_data));
        resp_data.hdr.sync = RFID_FRAME_SYNC;
        resp_data.cmd = RFID_CMD_SCAN_OR_WRITE;
        resp_data.unknown = RFID_SCAN_CARD_DATA;
        resp_data.len = nReadBytes;

        memcpy(resp_data.data.unknown, data, nReadBytes);

        hr = rfid_frame_encode(&rfid_per_board_vars[board].boarduart.readable, &resp_data, sizeof(resp_data));
        if (!SUCCEEDED(hr)){
            dprintf("RFID[%s]: failed encoding individual card: %lx\n", rw(board), hr);
            return hr;
        }
    } else {
        dprintf("RFID[%s]: failed accessing card directory %ls: %lx\n", rw(board), path, GetLastError());
    }

    SEND_END:
    if (FindNextFileW(hFind, &ffd) == 0){
        dprintf("RFID[%s]: done reading all files: %lx\n", rw(board), GetLastError());
        FindClose(hFind);
        hFind = INVALID_HANDLE_VALUE;
    }

    if (hFind == INVALID_HANDLE_VALUE){
        // end packet

        struct rfid_resp_card_end resp_end;

        memset(&resp_end, 0, sizeof(resp_end));
        resp_end.hdr.sync = RFID_FRAME_SYNC;
        resp_end.cmd = RFID_CMD_SCAN_OR_WRITE;
        resp_end.unknown = RFID_SCAN_END;
        resp_end.len = 0;
        hr = rfid_frame_encode(&rfid_per_board_vars[board].boarduart.readable, &resp_end, sizeof(resp_end));

    }

    return hr;
}

static HRESULT rfid_req_fw_update(int board, const struct rfid_req_any *req)
{
    dprintf("RFID[%s]: Firmware update request?\n", rw(board));
    dprintf("------------------------------\n");
    dprintf("ERROR: Trying to update the firmware of the RFID %s.\n", rw(board));
    dprintf("Make sure boot_fw_ver and app_fw_ver are correct.\n");
    dprintf("------------------------------\n");

    return E_NOTIMPL;
}


static HRESULT rfid_req_write_reset(int board, const struct rfid_req_any *req)
{
    dprintf("RFID[%s]: Write Reset\n", rw(board));

    //rfid_file_close();

    struct rfid_resp_card_start resp_start;
    memset(&resp_start, 0, sizeof(resp_start));
    resp_start.hdr.sync = RFID_FRAME_SYNC;
    resp_start.cmd = RFID_CMD_SCAN_OR_WRITE;
    resp_start.unknown = RFID_SCAN_START;
    resp_start.len = 0;

    HRESULT hr = rfid_frame_encode(&rfid_per_board_vars[board].boarduart.readable, &resp_start, sizeof(resp_start));
    if (!SUCCEEDED(hr)){
        return hr;
    }

    struct rfid_resp_card_data resp_data;
    memset(&resp_data, 0, sizeof(resp_data));
    resp_data.hdr.sync = RFID_FRAME_SYNC;
    resp_data.cmd = RFID_CMD_SCAN_OR_WRITE;
    resp_data.unknown = RFID_SCAN_CARD_DATA;
    resp_data.len = sizeof(resp_data.data);

    uint8_t cardid[CARD_ID_LEN];
    rfid_get_random_id(cardid, CARD_ID_LEN);

    memcpy(resp_data.data.unknown, cardid, CARD_ID_LEN);

    hr = rfid_frame_encode(&rfid_per_board_vars[board].boarduart.readable, &resp_data, sizeof(resp_data));
    if (!SUCCEEDED(hr)){
        return hr;
    }

    struct rfid_resp_card_end resp_end;

    memset(&resp_end, 0, sizeof(resp_end));
    resp_end.hdr.sync = RFID_FRAME_SYNC;
    resp_end.cmd = RFID_CMD_SCAN_OR_WRITE;
    resp_end.unknown = RFID_SCAN_END;
    resp_end.len = 0;

    block_written_state = 0;

    return rfid_frame_encode(&rfid_per_board_vars[board].boarduart.readable, &resp_end, sizeof(resp_end));
}

static HRESULT rfid_req_write_start(int board, const struct rfid_req_any *req)
{
    dprintf("RFID[%s]: Write Start/Stop\n", rw(board));

    struct rfid_req_write_start* req_start = (struct rfid_req_write_start*)req;
    struct rfid_resp_write_start resp;

    memset(&resp, 0, sizeof(resp));
    resp.hdr.sync = RFID_FRAME_SYNC;
    resp.cmd = RFID_CMD_WRITE_DATA_START_STOP;
    resp.unknown = 0;
    resp.len = 32;

    if (rfid_file_is_open() && block_written_state == 2){

        uint8_t cardid[CARD_ID_LEN];
        wchar_t cardid_hex[80] = {0};
        int len = 0;

        if (!rfid_file_get_id(cardid, CARD_ID_LEN, &len)){
            dprintf("RFID[%s]: Failed retrieving stored card id\n", rw(board));
            return E_FAIL;
        }

        for (size_t i = 0; i < CARD_ID_LEN; i++) {
            swprintf_s(cardid_hex, 60, L"%ls%02x", cardid_hex, cardid[i]);
        }

        wchar_t filename[MAX_PATH];
        swprintf_s(filename, MAX_PATH, L"%ls\\%ls.bin", path, cardid_hex);

        rfid_file_get_buffer(resp.data.unknown, 32, &len, 0);

        rfid_file_commit(filename, false, NULL);

    } else if (block_written_state == 0) {

        HRESULT hr = rfid_file_open();
        if (hr != S_OK){
            dprintf("RFID[%s]: Failed opening RFID writer: %lx\n", rw(board), hr);
            return hr;
        }

        uint8_t cardid[CARD_ID_LEN];
        memcpy(cardid, req_start->data.cardid, CARD_ID_LEN);

        wchar_t cardid_hex[60] = {0};
        for (size_t i = 0; i < CARD_ID_LEN; i++) {
            swprintf_s(cardid_hex, 60, L"%ls%02x", cardid_hex, cardid[i]);
        }
        dprintf("RFID[%s]: Card ID to be written: %ls\n", rw(board), cardid_hex);

        rfid_file_set_id(cardid, CARD_ID_LEN);

        block_written_state = 1;
    }

    return rfid_frame_encode(&rfid_per_board_vars[board].boarduart.readable, &resp, sizeof(resp));
}

static HRESULT rfid_req_write_block(int board, const struct rfid_req_any *req)
{
    struct rfid_req_write_block* req_block = (struct rfid_req_write_block*)req;

    uint8_t data[2];
    memcpy(data, req_block->data.block_data, 2);

    dprintf("RFID[%s]: Write Block %02d \n", rw(board), req_block->data.block_num);
    dump(data, 2);

    rfid_file_write_block(data, 2);

    struct rfid_resp_write_block resp;

    memset(&resp, 0, sizeof(resp));
    resp.hdr.sync = RFID_FRAME_SYNC;
    resp.cmd = RFID_CMD_WRITE_DATA_BLOCK;
    resp.unknown = 0;
    resp.len = 32;

    block_written_state = 2;

    return rfid_frame_encode(&rfid_per_board_vars[board].boarduart.readable, &resp, sizeof(resp));
}
