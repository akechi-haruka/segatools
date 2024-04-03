#include <math.h>
#include <stdio.h>
#include <windows.h>
#include "util/dprintf.h"

#define BITMAPHEADERSIZE    0x36

DWORD ConvertDataToBitmap
(
    DWORD dwBitCount,
    DWORD dwWidth, DWORD dwHeight,
    PBYTE pbInput, DWORD cbInput,
    PBYTE pbOutput, DWORD cbOutput,
    PDWORD pcbResult
)
{
    if (!pbInput || !pbOutput || dwBitCount < 8) return -3;

    if (cbInput < (dwWidth * dwHeight * dwBitCount / 8)) return -3;

    PBYTE pBuffer = (PBYTE)malloc(cbInput);
    if (!pBuffer) return -2;

    BYTE dwColors = (BYTE)(dwBitCount / 8);
    if (!dwColors) return -1;

    UINT16 cbColors;
    RGBQUAD pbColors[256];

    switch (dwBitCount)
    {
        case 1:
            cbColors = 1;
            break;
        case 2:
            cbColors = 4;
            break;
        case 4:
            cbColors = 16;
            break;
        case 8:
            cbColors = 256;
            break;
        default:
            cbColors = 0;
            break;
    }

    if (cbColors)
    {
        BYTE dwStep = (BYTE)(256 / cbColors);

        for (UINT16 i = 0; i < cbColors; ++i)
        {
            pbColors[i].rgbRed = dwStep * i;
            pbColors[i].rgbGreen = dwStep * i;
            pbColors[i].rgbBlue = dwStep * i;
            pbColors[i].rgbReserved = 0;
        }
    }

    DWORD dwTable = cbColors * sizeof(RGBQUAD);
    DWORD dwOffset = BITMAPHEADERSIZE + dwTable;

    BITMAPFILEHEADER bFile = { 0 };
    BITMAPINFOHEADER bInfo = { 0 };

    bFile.bfType = 0x4D42; // MAGIC
    bFile.bfSize = dwOffset + cbInput;
    bFile.bfOffBits = dwOffset;

    bInfo.biSize = sizeof(BITMAPINFOHEADER);
    bInfo.biWidth = dwWidth;
    bInfo.biHeight = dwHeight;
    bInfo.biPlanes = 1;
    bInfo.biBitCount = (WORD)dwBitCount;
    bInfo.biCompression = BI_RGB;
    bInfo.biSizeImage = cbInput;

    if (cbOutput < bFile.bfSize) return -1;

    for (size_t i = 0; i < dwHeight; i++)
    {
        for (size_t j = 0; j < dwWidth; j++)
        {
            for (size_t k = 0; k < dwColors; k++)
            {
                size_t x = i * dwWidth * dwColors + j * dwColors + (dwColors - k - 1);
                size_t y = (dwHeight - i - 1) * dwWidth * dwColors + j * dwColors + k;
                *(pBuffer + x) = *(pbInput + y);
            }
        }
    }

    memcpy(pbOutput, &bFile, sizeof(BITMAPFILEHEADER));
    memcpy(pbOutput + sizeof(BITMAPFILEHEADER), &bInfo, sizeof(BITMAPINFOHEADER));
    if (cbColors) memcpy(pbOutput + BITMAPHEADERSIZE, pbColors, dwTable);
    memcpy(pbOutput + dwOffset, pBuffer, cbInput);

    *pcbResult = bFile.bfSize;

    free(pBuffer);
    return 0;
}

DWORD WriteDataToBitmapFile
(
    LPCWSTR lpFilePath, DWORD dwBitCount,
    DWORD dwWidth, DWORD dwHeight,
    PBYTE pbInput, DWORD cbInput, char* extra_data, int extra_data_len
)
{
    if (!lpFilePath || !pbInput) return -3;

    HANDLE hFile;
    DWORD dwBytesWritten;

    hFile = CreateFileW
    (
        lpFilePath,
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
        NULL
    );
    if (hFile == INVALID_HANDLE_VALUE) return -1;

    DWORD cbResult;
    DWORD cbBuffer = cbInput + 0x500;
    PBYTE pbBuffer = (PBYTE)calloc(cbBuffer, 1);
    if (!pbBuffer) return -2;

    if (ConvertDataToBitmap(dwBitCount, dwWidth, dwHeight, pbInput, cbInput, pbBuffer, cbBuffer, &cbResult) < 0)
    {
        cbResult = -1;
        goto WriteDataToBitmapFile_End;
    }

    WriteFile(hFile, pbBuffer, cbResult, &dwBytesWritten, NULL);
    cbResult = dwBytesWritten;
    if (extra_data_len > 0){
        dprintf("Bitmap: writing %d extra byte(s)\n", extra_data_len);
        WriteFile(hFile, extra_data, extra_data_len, &dwBytesWritten, NULL);
        cbResult += dwBytesWritten;
    }

    CloseHandle(hFile);


WriteDataToBitmapFile_End:
    free(pbBuffer);
    return cbResult;
}

DWORD WriteArrayToFile(LPCWSTR lpOutputFilePath, LPVOID lpDataTemp, DWORD nDataSize, BOOL isAppend)
{
    HANDLE hFile;
    DWORD dwBytesWritten;
    DWORD dwDesiredAccess;
    DWORD dwCreationDisposition;

    if (isAppend)
    {
        dwDesiredAccess = FILE_APPEND_DATA;
        dwCreationDisposition = OPEN_ALWAYS;
    }
    else
    {
        dwDesiredAccess = GENERIC_WRITE;
        dwCreationDisposition = CREATE_ALWAYS;
    }

    hFile = CreateFileW
    (
        lpOutputFilePath,
        dwDesiredAccess,
        FILE_SHARE_READ,
        NULL,
        dwCreationDisposition,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
        NULL
    );
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    WriteFile(hFile, lpDataTemp, nDataSize, &dwBytesWritten, NULL);
    CloseHandle(hFile);

    return dwBytesWritten;
}
