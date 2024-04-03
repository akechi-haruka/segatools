// Forwarder for that weird ass printer dll variant that uses __stdcall instead of __fastcall

#include <windows.h>
#include <shellapi.h>

#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h>

#include "printer/printer.h"
#include "printer/printer-stdcall.h"

#include "hook/table.h"
#include "hooklib/dll.h"

#include "util/dprintf.h"

#pragma region defs
int __stdcall fwdlusb_std_open(uint16_t * rResult);
void __stdcall fwdlusb_std_close();
int __stdcall fwdlusb_std_listupPrinter(uint8_t * rIdArray);
int __stdcall fwdlusb_std_listupPrinterSN(uint64_t * rSerialArray);
int __stdcall fwdlusb_std_selectPrinter(uint8_t printerId, uint16_t * rResult);
int __stdcall fwdlusb_std_selectPrinterSN(uint64_t printerSN, uint16_t * rResult);
int __stdcall fwdlusb_std_getPrinterInfo(uint16_t tagNumber, uint8_t * rBuffer, uint32_t * rLen);
int __stdcall fwdlusb_std_status(uint16_t * rResult);
int __stdcall fwdlusb_std_statusAll(uint8_t * idArray, uint16_t * rResultArray);
int __stdcall fwdlusb_std_resetPrinter(uint16_t * rResult);
int __stdcall fwdlusb_std_updateFirmware(uint8_t update, LPCSTR filename, uint16_t * rResult);
int __stdcall fwdlusb_std_getFirmwareInfo(uint8_t update, LPCSTR filename, uint8_t * rBuffer, uint32_t * rLen, uint16_t * rResult);
int __stdcall fwdlusb_std_MakeThread(uint16_t maxCount);
int __stdcall fwdlusb_std_ReleaseThread(uint16_t * rResult);
int __stdcall fwdlusb_std_AttachThreadCount(uint16_t * rCount, uint16_t * rMaxCount);
int __stdcall fwdlusb_std_getErrorLog(uint16_t index, uint8_t * rData, uint16_t * rResult);
int __stdcall chcusb_std_MakeThread(uint16_t maxCount);
int __stdcall chcusb_std_open(uint16_t * rResult);
void __stdcall chcusb_std_close();
int __stdcall chcusb_std_ReleaseThread(uint16_t * rResult);
int __stdcall chcusb_std_listupPrinter(uint8_t * rIdArray);
int __stdcall chcusb_std_listupPrinterSN(uint64_t * rSerialArray);
int __stdcall chcusb_std_selectPrinter(uint8_t printerId, uint16_t * rResult);
int __stdcall chcusb_std_selectPrinterSN(uint64_t printerSN, uint16_t * rResult);
int __stdcall chcusb_std_getPrinterInfo(uint16_t tagNumber, uint8_t * rBuffer, uint32_t * rLen);
int __stdcall chcusb_std_imageformat(
        uint16_t format,
        uint16_t ncomp,
        uint16_t depth,
        uint16_t width,
        uint16_t height,
        uint16_t * rResult
);
int __stdcall chcusb_std_setmtf(int32_t * mtf);
int __stdcall chcusb_std_makeGamma(uint16_t k, uint8_t * intoneR, uint8_t * intoneG, uint8_t * intoneB);
int __stdcall chcusb_std_setIcctable(
        LPCSTR icc1,
        LPCSTR icc2,
        uint16_t intents,
        uint8_t * intoneR,
        uint8_t * intoneG,
        uint8_t * intoneB,
        uint8_t * outtoneR,
        uint8_t * outtoneG,
        uint8_t * outtoneB,
        uint16_t * rResult
);
int __stdcall chcusb_std_copies(uint16_t copies, uint16_t * rResult);
int __stdcall chcusb_std_status(uint16_t * rResult);
int __stdcall chcusb_std_statusAll(uint8_t * idArray, uint16_t * rResultArray);
int __stdcall chcusb_std_startpage(uint16_t postCardState, uint16_t * pageId, uint16_t * rResult);
int __stdcall chcusb_std_endpage(uint16_t * rResult);
int __stdcall chcusb_std_write(uint8_t * data, uint32_t * writeSize, uint16_t * rResult);
int __stdcall chcusb_std_writeLaminate(uint8_t * data, uint32_t * writeSize, uint16_t * rResult);
int __stdcall chcusb_std_writeHolo(uint8_t * data, uint32_t * writeSize, uint16_t * rResult);
int __stdcall chcusb_std_setPrinterInfo(uint16_t tagNumber, uint8_t * rBuffer, uint32_t * rLen, uint16_t * rResult);
int __stdcall chcusb_std_getGamma(LPCSTR filename, uint8_t * r, uint8_t * g, uint8_t * b, uint16_t * rResult);
int __stdcall chcusb_std_getMtf(LPCSTR filename, int32_t * mtf, uint16_t * rResult);
int __stdcall chcusb_std_cancelCopies(uint16_t pageId, uint16_t * rResult);
int __stdcall chcusb_std_setPrinterToneCurve(uint16_t type, uint16_t number, uint16_t * data, uint16_t * rResult);
int __stdcall chcusb_std_getPrinterToneCurve(uint16_t type, uint16_t number, uint16_t * data, uint16_t * rResult);
int __stdcall chcusb_std_blinkLED(uint16_t * rResult);
int __stdcall chcusb_std_resetPrinter(uint16_t * rResult);
int __stdcall chcusb_std_AttachThreadCount(uint16_t * rCount, uint16_t * rMaxCount);
int __stdcall chcusb_std_getPrintIDStatus(uint16_t pageId, uint8_t * rBuffer, uint16_t * rResult);
int __stdcall chcusb_std_setPrintStandby(uint16_t position, uint16_t * rResult);
int __stdcall chcusb_std_testCardFeed(uint16_t mode, uint16_t times, uint16_t * rResult);
int __stdcall chcusb_std_exitCard(uint16_t * rResult);
int __stdcall chcusb_std_getCardRfidTID(uint8_t * rCardTID, uint16_t * rResult);
int __stdcall chcusb_std_commCardRfidReader(uint8_t * sendData, uint8_t * rRecvData, uint32_t sendSize, uint32_t * rRecvSize, uint16_t * rResult);
int __stdcall chcusb_std_updateCardRfidReader(uint8_t * data, uint32_t size, uint16_t * rResult);
int __stdcall chcusb_std_getErrorLog(uint16_t index, uint8_t * rData, uint16_t * rResult);
int __stdcall chcusb_std_getErrorStatus(uint16_t * rBuffer);
int __stdcall chcusb_std_setCutList(uint8_t * rData, uint16_t * rResult);
int __stdcall chcusb_std_setLaminatePattern(uint16_t index, uint8_t * rData, uint16_t * rResult);
int __stdcall chcusb_std_color_adjustment(LPCSTR filename, int32_t a2, int32_t a3, int16_t a4, int16_t a5, int64_t a6, int64_t a7, uint16_t * rResult);
int __stdcall chcusb_std_color_adjustmentEx(int32_t a1, int32_t a2, int32_t a3, int16_t a4, int16_t a5, int64_t a6, int64_t a7, uint16_t * rResult);
int __stdcall chcusb_std_getEEPROM(uint8_t index, uint8_t * rData, uint16_t * rResult);
int __stdcall chcusb_std_setParameter(uint8_t a1, uint32_t a2, uint16_t * rResult);
int __stdcall chcusb_std_getParameter(uint8_t a1, uint8_t * a2, uint16_t * rResult);
int __stdcall chcusb_std_universal_command(int32_t a1, uint8_t a2, int32_t a3, uint8_t * a4, uint16_t * rResult);
int __stdcall chcusb_std_writeIred(uint8_t* a1, uint8_t* a2, uint16_t* rResult);
#pragma endregion

#pragma region hooktable
static const struct hook_symbol C3XXfwdlusb_hooks[] = {
    {
        .name   = "fwdlusb_open",
        .ordinal= 1,
        .patch  = fwdlusb_std_open,
        .link   = NULL
    }, {
        .name   = "__imp_fwdlusb_close",
        .ordinal= 2,
        .patch  = fwdlusb_std_close,
        .link   = NULL
    },  {
        .name   = "fwdlusb_listupPrinter",
        .ordinal= 3,
        .patch  = fwdlusb_std_listupPrinter,
        .link   = NULL
    },  {
        .name   = "fwdlusb_listupPrinterSN",
        .ordinal= 4,
        .patch  = fwdlusb_std_listupPrinterSN,
        .link   = NULL
    },  {
        .name   = "fwdlusb_selectPrinter",
        .ordinal= 5,
        .patch  = fwdlusb_std_selectPrinter,
        .link   = NULL
    },  {
        .name   = "fwdlusb_selectPrinterSN",
        .ordinal= 6,
        .patch  = fwdlusb_std_selectPrinterSN,
        .link   = NULL
    },  {
        .name   = "fwdlusb_getPrinterInfo",
        .ordinal= 7,
        .patch  = fwdlusb_std_getPrinterInfo,
        .link   = NULL
    },  {
        .name   = "fwdlusb_status",
        .ordinal= 8,
        .patch  = fwdlusb_std_status,
        .link   = NULL
    },  {
        .name   = "fwdlusb_statusAll",
        .ordinal= 9,
        .patch  = fwdlusb_std_statusAll,
        .link   = NULL
    },  {
        .name   = "fwdlusb_resetPrinter",
        .ordinal= 10,
        .patch  = fwdlusb_std_resetPrinter,
        .link   = NULL
    },  {
        .name   = "fwdlusb_updateFirmware",
        .ordinal= 11,
        .patch  = fwdlusb_std_updateFirmware,
        .link   = NULL
    },  {
        .name   = "fwdlusb_getFirmwareInfo",
        .ordinal= 12,
        .patch  = fwdlusb_std_getFirmwareInfo,
        .link   = NULL
    },  {
        .name   = "fwdlusb_MakeThread",
        .ordinal= 13,
        .patch  = fwdlusb_std_MakeThread,
        .link   = NULL
    },  {
        .name   = "fwdlusb_ReleaseThread",
        .ordinal= 14,
        .patch  = fwdlusb_std_ReleaseThread,
        .link   = NULL
    },  {
        .name   = "fwdlusb_AttachThreadCount",
        .ordinal= 15,
        .patch  = fwdlusb_std_AttachThreadCount,
        .link   = NULL
    },  {
        .name   = "fwdlusb_getErrorLog",
        .ordinal= 16,
        .patch  = fwdlusb_std_getErrorLog,
        .link   = NULL
    },
};

static const struct hook_symbol C3XXusb_hooks[] = {
    {
        .name   = "__imp_chcusb_MakeThread",
        .ordinal= 1,
        .patch  = chcusb_std_MakeThread,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_open",
        .ordinal= 2,
        .patch  = chcusb_std_open,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_close",
        .ordinal= 3,
        .patch  = chcusb_std_close,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_ReleaseThread",
        .ordinal= 4,
        .patch  = chcusb_std_ReleaseThread,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_listupPrinter",
        .ordinal= 5,
        .patch  = chcusb_std_listupPrinter,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_listupPrinterSN",
        .ordinal= 6,
        .patch  = chcusb_std_listupPrinterSN,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_selectPrinter",
        .ordinal= 7,
        .patch  = chcusb_std_selectPrinter,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_selectPrinterSN",
        .ordinal= 8,
        .patch  = chcusb_std_selectPrinterSN,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_getPrinterInfo",
        .ordinal= 9,
        .patch  = chcusb_std_getPrinterInfo,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_imageformat",
        .ordinal= 10,
        .patch  = chcusb_std_imageformat,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_setmtf",
        .ordinal= 11,
        .patch  = chcusb_std_setmtf,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_makeGamma",
        .ordinal= 12,
        .patch  = chcusb_std_makeGamma,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_setIcctable",
        .ordinal= 13,
        .patch  = chcusb_std_setIcctable,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_copies",
        .ordinal= 14,
        .patch  = chcusb_std_copies,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_status",
        .ordinal= 15,
        .patch  = chcusb_std_status,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_statusAll",
        .ordinal= 16,
        .patch  = chcusb_std_statusAll,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_startpage",
        .ordinal= 17,
        .patch  = chcusb_std_startpage,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_endpage",
        .ordinal= 18,
        .patch  = chcusb_std_endpage,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_write",
        .ordinal= 19,
        .patch  = chcusb_std_write,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_writeLaminate",
        .ordinal= 20,
        .patch  = chcusb_std_writeLaminate,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_writeHolo",
        .ordinal= 21,
        .patch  = chcusb_std_writeHolo,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_setPrinterInfo",
        .ordinal= 22,
        .patch  = chcusb_std_setPrinterInfo,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_getGamma",
        .ordinal= 23,
        .patch  = chcusb_std_getGamma,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_getMtf",
        .ordinal= 24,
        .patch  = chcusb_std_getMtf,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_cancelCopies",
        .ordinal= 25,
        .patch  = chcusb_std_cancelCopies,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_setPrinterToneCurve",
        .ordinal= 26,
        .patch  = chcusb_std_setPrinterToneCurve,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_getPrinterToneCurve",
        .ordinal= 27,
        .patch  = chcusb_std_getPrinterToneCurve,
        .link   = NULL
    }, {
        .name   = "chcusb_blinkLED",
        .ordinal= 28,
        .patch  = chcusb_std_blinkLED,
        .link   = NULL
    }, {
        .name   = "chcusb_resetPrinter",
        .ordinal= 29,
        .patch  = chcusb_std_resetPrinter,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_AttachThreadCount",
        .ordinal= 30,
        .patch  = chcusb_std_AttachThreadCount,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_getPrintIDStatus",
        .ordinal= 31,
        .patch  = chcusb_std_getPrintIDStatus,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_setPrintStandby",
        .ordinal= 32,
        .patch  = chcusb_std_setPrintStandby,
        .link   = NULL
    }, {
        .name   = "chcusb_testCardFeed",
        .ordinal= 33,
        .patch  = chcusb_std_testCardFeed,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_exitCard",
        .ordinal= 34,
        .patch  = chcusb_std_exitCard,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_getCardRfidTID",
        .ordinal= 35,
        .patch  = chcusb_std_getCardRfidTID,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_commCardRfidReader",
        .ordinal= 36,
        .patch  = chcusb_std_commCardRfidReader,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_updateCardRfidReader",
        .ordinal= 37,
        .patch  = chcusb_std_updateCardRfidReader,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_getErrorLog",
        .ordinal= 38,
        .patch  = chcusb_std_getErrorLog,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_getErrorStatus",
        .ordinal= 39,
        .patch  = chcusb_std_getErrorStatus,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_setCutList",
        .ordinal= 40,
        .patch  = chcusb_std_setCutList,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_setLaminatePattern",
        .ordinal= 41,
        .patch  = chcusb_std_setLaminatePattern,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_color_adjustment",
        .ordinal= 42,
        .patch  = chcusb_std_color_adjustment,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_color_adjustmentEx",
        .ordinal= 43,
        .patch  = chcusb_std_color_adjustmentEx,
        .link   = NULL
    },  {
        .name   = "__imp_chcusb_writeIred",
        .ordinal= 50,
        .patch  = chcusb_std_writeIred,
        .link   = NULL
    },  {
        .name   = "__imp_chcusb_getEEPROM",
        .ordinal= 58,
        .patch  = chcusb_std_getEEPROM,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_setParameter",
        .ordinal= 64,
        .patch  = chcusb_std_setParameter,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_getParameter",
        .ordinal= 65,
        .patch  = chcusb_std_getParameter,
        .link   = NULL
    }, {
        .name   = "__imp_chcusb_universal_command",
        .ordinal= 73,
        .patch  = chcusb_std_universal_command,
        .link   = NULL
    },
};

void printer_std_hook_init(const struct printer_config *cfg, HINSTANCE a, HINSTANCE fwdl, int model, bool apply_dll_hook)
{
    assert(cfg != NULL);

    if (!cfg->enable) {
        return;
    }

    printer_hook_init(cfg, NULL, model, false, false);

    hook_table_apply(NULL, "C320Ausb.dll", C3XXusb_hooks, _countof(C3XXusb_hooks));
    hook_table_apply(NULL, "C320AFWDLusb.dll", C3XXfwdlusb_hooks, _countof(C3XXfwdlusb_hooks));
    hook_table_apply(NULL, "C300usb.dll", C3XXusb_hooks, _countof(C3XXusb_hooks));

    if (a != NULL && apply_dll_hook) {
        dll_hook_push(a, L"C320Ausb.dll");
        dll_hook_push(a, L"C300usb.dll");
    }
    if (fwdl != NULL && apply_dll_hook) {
        dll_hook_push(fwdl, L"C320FWDLusb.dll");
    }

    dprintf("Printer-stdcall: hook enabled.\n");
}

int __stdcall fwdlusb_std_open(uint16_t* rResult){
    return fwdlusb_open(rResult);
}

void __stdcall fwdlusb_std_close()
{
    fwdlusb_close();
}

int __stdcall fwdlusb_std_listupPrinter(uint8_t* rIdArray){
    return fwdlusb_listupPrinter(rIdArray);
}

int __stdcall fwdlusb_std_listupPrinterSN(uint64_t* rSerialArray)
{
    return fwdlusb_listupPrinterSN(rSerialArray);
}

int __stdcall fwdlusb_std_selectPrinter(uint8_t printerId, uint16_t* rResult){
    return fwdlusb_selectPrinter(printerId, rResult);
}

int __stdcall fwdlusb_std_selectPrinterSN(uint64_t printerSN, uint16_t* rResult)
{
    return fwdlusb_selectPrinterSN(printerSN, rResult);
}

int __stdcall fwdlusb_std_getPrinterInfo(uint16_t tagNumber, uint8_t* rBuffer, uint32_t* rLen){
    return fwdlusb_getPrinterInfo(tagNumber, rBuffer, rLen);
}

int __stdcall fwdlusb_std_status(uint16_t* rResult)
{
    return fwdlusb_status(rResult);
}

int __stdcall fwdlusb_std_statusAll(uint8_t* idArray, uint16_t* rResultArray){
    return fwdlusb_statusAll(idArray, rResultArray);
}

int __stdcall fwdlusb_std_resetPrinter(uint16_t* rResult)
{
    return fwdlusb_resetPrinter(rResult);
}

int __stdcall fwdlusb_std_updateFirmware(uint8_t update, LPCSTR filename, uint16_t* rResult)
{
    return fwdlusb_updateFirmware(update, filename, rResult);
}

int __stdcall fwdlusb_std_getFirmwareInfo(uint8_t update, LPCSTR filename, uint8_t* rBuffer, uint32_t* rLen, uint16_t* rResult){
    return fwdlusb_getFirmwareInfo(update, filename, rBuffer, rLen, rResult);
}

int __stdcall fwdlusb_std_MakeThread(uint16_t maxCount)
{
    return fwdlusb_MakeThread(maxCount);
}

int __stdcall fwdlusb_std_ReleaseThread(uint16_t* rResult){
    return fwdlusb_ReleaseThread(rResult);
}

int __stdcall fwdlusb_std_AttachThreadCount(uint16_t* rCount, uint16_t* rMaxCount)
{
    return fwdlusb_AttachThreadCount(rCount, rMaxCount);
}

int __stdcall fwdlusb_std_getErrorLog(uint16_t index, uint8_t* rData, uint16_t* rResult){
    return fwdlusb_getErrorLog(index, rData, rResult);
}

int __stdcall chcusb_std_MakeThread(uint16_t maxCount)
{
    return chcusb_MakeThread(maxCount);
}

int __stdcall chcusb_std_open(uint16_t* rResult){
    return chcusb_open(rResult);
}

void __stdcall chcusb_std_close()
{
    return chcusb_close();
}

int __stdcall chcusb_std_ReleaseThread(uint16_t* rResult){
    return chcusb_ReleaseThread(rResult);
}

int __stdcall chcusb_std_listupPrinter(uint8_t* rIdArray)
{
    return chcusb_listupPrinter(rIdArray);
}

int __stdcall chcusb_std_listupPrinterSN(uint64_t* rSerialArray){
    return chcusb_listupPrinterSN(rSerialArray);
}

int __stdcall chcusb_std_selectPrinter(uint8_t printerId, uint16_t* rResult)
{
    return chcusb_selectPrinter(printerId, rResult);
}

int __stdcall chcusb_std_selectPrinterSN(uint64_t printerSN, uint16_t* rResult){
    return chcusb_selectPrinterSN(printerSN, rResult);
}

int __stdcall chcusb_std_getPrinterInfo(uint16_t tagNumber, uint8_t* rBuffer, uint32_t* rLen)
{
    return chcusb_getPrinterInfo(tagNumber, rBuffer, rLen);
}

int __stdcall chcusb_std_imageformat(uint16_t format, uint16_t ncomp, uint16_t depth, uint16_t width, uint16_t height, uint16_t* rResult) {
    return chcusb_imageformat_330(format, ncomp, depth, width, height, rResult);
}

int __stdcall chcusb_std_setmtf(int32_t* mtf){
    return chcusb_setmtf(mtf);
}

int __stdcall chcusb_std_makeGamma(uint16_t k, uint8_t* intoneR, uint8_t* intoneG, uint8_t* intoneB)
{
    return chcusb_makeGamma(k, intoneR, intoneG, intoneB);
}

int __stdcall chcusb_std_setIcctable(LPCSTR icc1, LPCSTR icc2, uint16_t intents, uint8_t* intoneR, uint8_t* intoneG, uint8_t* intoneB, uint8_t* outtoneR, uint8_t* outtoneG, uint8_t* outtoneB, uint16_t* rResult) {
    return chcusb_setIcctable(icc1, icc2, intents, intoneR, intoneG, intoneB, outtoneR, outtoneG, outtoneB, rResult);
}

int __stdcall chcusb_std_copies(uint16_t copies, uint16_t* rResult){
    return chcusb_copies(copies, rResult);
}

int __stdcall chcusb_std_statusAll(uint8_t* idArray, uint16_t* rResultArray)
{
    return chcusb_statusAll(idArray, rResultArray);
}

int __stdcall chcusb_std_startpage(uint16_t postCardState, uint16_t* pageId, uint16_t* rResult){
    return chcusb_startpage(postCardState, pageId, rResult);
}

int __stdcall chcusb_std_endpage(uint16_t* rResult)
{
    return chcusb_endpage(rResult);
}

int __stdcall chcusb_std_write(uint8_t* data, uint32_t* writeSize, uint16_t* rResult){
    return chcusb_write(data, writeSize, rResult);
}

int __stdcall chcusb_std_writeLaminate(uint8_t* data, uint32_t* writeSize, uint16_t* rResult)
{
    return chcusb_writeLaminate(data, writeSize, rResult);
}

int __stdcall chcusb_std_writeHolo(uint8_t* data, uint32_t* writeSize, uint16_t* rResult){
    return chcusb_writeHolo(data, writeSize, rResult);
}

int __stdcall chcusb_std_setPrinterInfo(uint16_t tagNumber, uint8_t* rBuffer, uint32_t* rLen, uint16_t* rResult)
{
    return chcusb_setPrinterInfo(tagNumber, rBuffer, rLen, rResult);
}

int __stdcall chcusb_std_getGamma(LPCSTR filename, uint8_t* r, uint8_t* g, uint8_t* b, uint16_t* rResult){
    return chcusb_getGamma(filename, r, g, b, rResult);
}

int __stdcall chcusb_std_getMtf(LPCSTR filename, int32_t* mtf, uint16_t* rResult)
{
    return chcusb_getMtf(filename, mtf, rResult);
}

int __stdcall chcusb_std_cancelCopies(uint16_t pageId, uint16_t* rResult){
    return chcusb_cancelCopies(pageId, rResult);
}

int __stdcall chcusb_std_setPrinterToneCurve(uint16_t type, uint16_t number, uint16_t* data, uint16_t* rResult)
{
    return chcusb_setPrinterToneCurve(type, number, data, rResult);
}

int __stdcall chcusb_std_getPrinterToneCurve(uint16_t type, uint16_t number, uint16_t* data, uint16_t* rResult){
    return chcusb_getPrinterToneCurve(type, number, data, rResult);
}

int __stdcall chcusb_std_blinkLED(uint16_t* rResult)
{
    return chcusb_blinkLED(rResult);
}

int __stdcall chcusb_std_resetPrinter(uint16_t* rResult){
    return chcusb_resetPrinter(rResult);
}

int __stdcall chcusb_std_AttachThreadCount(uint16_t* rCount, uint16_t* rMaxCount)
{
    return chcusb_AttachThreadCount(rCount, rMaxCount);
}

int __stdcall chcusb_std_getPrintIDStatus(uint16_t pageId, uint8_t* rBuffer, uint16_t* rResult){
    return chcusb_getPrintIDStatus(pageId, rBuffer, rResult);
}

int __stdcall chcusb_std_status(uint16_t* rResult)
{
    return chcusb_status(rResult);
}

int __stdcall chcusb_std_setPrintStandby(uint16_t position, uint16_t* rResult){
    return chcusb_setPrintStandby(position, rResult);
}

int __stdcall chcusb_std_testCardFeed(uint16_t mode, uint16_t times, uint16_t* rResult)
{
    return chcusb_testCardFeed(mode, times, rResult);
}

int __stdcall chcusb_std_exitCard(uint16_t* rResult){
    return chcusb_exitCard(rResult);
}

int __stdcall chcusb_std_getCardRfidTID(uint8_t* rCardTID, uint16_t* rResult)
{
    return chcusb_getCardRfidTID(rCardTID, rResult);
}

int __stdcall chcusb_std_commCardRfidReader(uint8_t* sendData, uint8_t* rRecvData, uint32_t sendSize, uint32_t* rRecvSize, uint16_t* rResult){
    return chcusb_commCardRfidReader(sendData, rRecvData, sendSize, rRecvSize, rResult);
}

int __stdcall chcusb_std_updateCardRfidReader(uint8_t* data, uint32_t size, uint16_t* rResult)
{
    return chcusb_updateCardRfidReader(data, size, rResult);
}

int __stdcall chcusb_std_getErrorLog(uint16_t index, uint8_t* rData, uint16_t* rResult){
    return chcusb_getErrorLog(index, rData, rResult);
}

int __stdcall chcusb_std_getErrorStatus(uint16_t* rBuffer)
{
    return chcusb_getErrorStatus(rBuffer);
}

int __stdcall chcusb_std_setCutList(uint8_t* rData, uint16_t* rResult){
    return chcusb_setCutList(rData, rResult);
}

int __stdcall chcusb_std_setLaminatePattern(uint16_t index, uint8_t* rData, uint16_t* rResult)
{
    return chcusb_setLaminatePattern(index, rData, rResult);
}

int __stdcall chcusb_std_color_adjustment(LPCSTR filename, int32_t a2, int32_t a3, int16_t a4, int16_t a5, int64_t a6, int64_t a7, uint16_t* rResult){
    return chcusb_color_adjustment(filename, a2, a3, a4, a5, a6, a7, rResult);
}

int __stdcall chcusb_std_color_adjustmentEx(int32_t a1, int32_t a2, int32_t a3, int16_t a4, int16_t a5, int64_t a6, int64_t a7, uint16_t* rResult)
{
    return chcusb_color_adjustmentEx(a1, a2, a3, a4, a5, a6, a7, rResult);
}

int __stdcall chcusb_std_writeIred(uint8_t* a1, uint8_t* a2, uint16_t* rResult){
    return chcusb_writeIred(a1, a2, rResult);
}

int __stdcall chcusb_std_getEEPROM(uint8_t index, uint8_t* rData, uint16_t* rResult)
{
    return chcusb_getEEPROM(index, rData, rResult);
}

int __stdcall chcusb_std_setParameter(uint8_t a1, uint32_t a2, uint16_t* rResult){
    return chcusb_setParameter(a1, a2, rResult);
}

int __stdcall chcusb_std_getParameter(uint8_t a1, uint8_t* a2, uint16_t* rResult){
    return chcusb_getParameter(a1, a2, rResult);
}

int __stdcall chcusb_std_universal_command(int32_t a1, uint8_t a2, int32_t a3, uint8_t* a4, uint16_t* rResult){
    return chcusb_universal_command(a1, a2, a3, a4, rResult);
}
