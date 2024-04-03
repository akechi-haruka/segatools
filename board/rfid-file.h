#pragma once

HRESULT rfid_file_open();
bool rfid_file_get_id(uint8_t* id, int buf_len, int* len);
bool rfid_file_get_buffer(uint8_t* data, int buf_len, int* len, int offset);
int rfid_get_written_bytes();
void rfid_get_random_id(uint8_t* id, int len);
void rfid_file_set_id(uint8_t* id, int len);
void rfid_file_write_block(uint8_t* data, int len);
HRESULT rfid_file_commit(wchar_t* filename, bool append, long* bytesWritten);
bool rfid_file_is_open();
void rfid_file_close();
