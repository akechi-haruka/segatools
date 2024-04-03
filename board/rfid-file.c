#include <sys/time.h>

#include "util/dump.h"
#include "util/dprintf.h"

static uint8_t* currentRfidId = NULL;
static int rfid_cardid_len = 0;
static char* rfid_write_buffer = NULL;
static int rfid_write_length = 0;

HRESULT rfid_file_open(){
    if (rfid_write_length > 0 || rfid_write_buffer != NULL){
        return E_FAIL;
    }

    dprintf("RFIDOut: Opened\n");

    rfid_write_buffer = (char*)malloc(255);

    return S_OK;
}

void rfid_get_random_id(uint8_t* id, int len){
    struct timeval time;
    gettimeofday(&time,NULL);
    srand((time.tv_sec * 1000) + (time.tv_usec / 1000));

    for (int i = 0; i < len; i++){
        id[i] = (uint8_t)((rand() % 254) + 1);
    }

    dprintf("RFIDOut: Generated a card ID\n");
}

bool rfid_file_get_id(uint8_t* id, int buf_len, int* len){
    if (currentRfidId == NULL){
        return false;
    }
    if (buf_len < rfid_cardid_len){
        dprintf("rfid_file_get_id: buffer too small %d/%d\n", buf_len, rfid_cardid_len);
        return false;
    }
    memcpy(id, currentRfidId, rfid_cardid_len);
    *len = rfid_cardid_len;
    return true;
}

bool rfid_file_get_buffer(uint8_t* data, int buf_len, int* len, int offset){
    if (rfid_write_length <= 0 || rfid_write_buffer == NULL){
        dprintf("rfid_file_get_buffer: no buffer\n");
        return false;
    }
    if (rfid_write_length < offset){
        dprintf("rfid_file_get_buffer: offset larger than written bytes (%d>%d)\n", rfid_write_length, offset);
        return false;
    }
    if (buf_len < rfid_write_length){
        dprintf("rfid_file_get_buffer: buffer too small %d/%d\n", buf_len, rfid_cardid_len);
        return false;
    }

    memcpy(data, rfid_write_buffer + offset, rfid_write_length - offset);
    *len = rfid_write_length - offset;
    return true;
}

int rfid_get_written_bytes(){
    return rfid_write_length;
}

void rfid_file_set_id(uint8_t* id, int len){
    currentRfidId = (uint8_t*)malloc(len);
    memcpy(currentRfidId, id, len);
    rfid_cardid_len = len;
    dprintf("RFIDOut: Card ID set\n");
}

void rfid_file_write_block(uint8_t* data, int len){
    memcpy(rfid_write_buffer + rfid_write_length, data, len);
    rfid_write_length += len;
    dprintf("RFIDOut: Wrote payload (length=%d)\n", len);
}

void printer_error_to_log(wchar_t* what, wchar_t* file, DWORD error){
    LPWSTR psz = NULL;
    FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        psz,
        0,
        NULL
    );
    dprintf("!!!!! PRINTER ERROR !!!!!\nCouldn't write to %ls.\nAre you out of disk space? Read-only directory?\nJanky hacks mate? Well, that card is now history.\nError: %lx %ls\n!!!!! PRINTER ERROR !!!!!\n", file, error, psz);
    if (psz != NULL){
        LocalFree(psz);
    }
}

HRESULT rfid_file_commit(wchar_t* filename, bool append, long* bytesWritten){
    int write_mode;
    int open_mode;
    if (append){
        write_mode = FILE_APPEND_DATA;
        open_mode = OPEN_ALWAYS;
    } else {
        write_mode = GENERIC_WRITE;
        open_mode = CREATE_NEW;
    }

    dprintf("RFIDOut: Writing to %ls (append=%d)\n", filename, append);

    HANDLE rfid_writer = CreateFileW(filename, write_mode, FILE_SHARE_WRITE, NULL, open_mode, FILE_ATTRIBUTE_NORMAL, NULL);
    if (rfid_writer == INVALID_HANDLE_VALUE){
        dprintf("RFIDOut: Failed to open RFID file: %lx\n", GetLastError());
        printer_error_to_log(L"RFID file create", filename, GetLastError());
        return E_FAIL;
    }

    int bytesTotal = 0;
    unsigned long bytesSingleWrite = 0;

    if (!WriteFile(rfid_writer, currentRfidId, rfid_cardid_len, &bytesSingleWrite, NULL)){
        dprintf("RFIDOut: Failed to write file: %lx\n", GetLastError());
        printer_error_to_log(L"RFID file write (ID)", filename, GetLastError());
        CloseHandle(rfid_writer);
        return E_FAIL;
    }
    bytesTotal += bytesSingleWrite;

    if (!WriteFile(rfid_writer, rfid_write_buffer, rfid_write_length, &bytesSingleWrite, NULL)){
        dprintf("RFIDOut: Failed to write file: %lx\n", GetLastError());
        printer_error_to_log(L"RFID file write (Data)", filename, GetLastError());
        CloseHandle(rfid_writer);
        return E_FAIL;
    }
    bytesTotal += bytesSingleWrite;

    dprintf("RFIDOut: Wrote %d byte(s)\n", bytesTotal);

    if (bytesWritten != NULL){
        *bytesWritten = bytesTotal;
    }

    CloseHandle(rfid_writer);

    return S_OK;
}

bool rfid_file_is_open(){
    return rfid_write_buffer != NULL;
}

void rfid_file_close(){
    if (rfid_write_buffer != NULL){
        free(rfid_write_buffer);
    }
    rfid_write_buffer = NULL;
    if (currentRfidId != NULL){
        free(currentRfidId);
    }
    currentRfidId = NULL;
    rfid_write_length = 0;

    dprintf("RFIDOut: Reset\n");
}
