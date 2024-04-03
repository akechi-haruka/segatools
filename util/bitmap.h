#pragma once

#include <math.h>
#include <stdio.h>
#include <windows.h>

DWORD ConvertDataToBitmap
(
    DWORD dwBitCount,
    DWORD dwWidth, DWORD dwHeight,
    PBYTE pbInput, DWORD cbInput,
    PBYTE pbOutput, DWORD cbOutput,
    PDWORD pcbResult
);

DWORD WriteDataToBitmapFile
(
    LPCWSTR lpFilePath, DWORD dwBitCount,
    DWORD dwWidth, DWORD dwHeight,
    PBYTE pbInput, DWORD cbInput, char* extra_data, int extra_data_len
);
DWORD WriteArrayToFile(LPCWSTR lpOutputFilePath, LPVOID lpDataTemp, DWORD nDataSize, BOOL isAppend);
