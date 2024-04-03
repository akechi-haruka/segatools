// C310 Emulator, squashed into an ugly C Source File and Header.
// Originally by Raki. Converted to this form by TenkoParrot devs
// (allegedly?). Modified by Coburn.
// Fixed and made to work by Haruka.

#include <windows.h>
#include <shellapi.h>

#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h>

#include "hook/table.h"

#include "hooklib/dll.h"
#include "printer/printer.h"
#include "board/rfid-cmd.h"
#include "board/rfid-file.h"

#include "util/bitmap.h"
#include "util/dump.h"
#include "util/dprintf.h"

#define IMAGE_SIZE 0x24FC00
#define HOLO_SIZE 0xC5400
#define CARD_ID_LEN 12
#define SUPER_VERBOSE 0

#define kPINFTAG_PAPER 0
#define kPINFTAG_USBINQ 2
#define kPINFTAG_ENGID 3
#define kPINFTAG_PRINTCNT 4
#define kPINFTAG_PRINTCNT2 5
#define kPINFTAG_SVCINFO 7
#define kPINFTAG_PRINTSTANDBY 8
#define kPINFTAG_MEMORY 16
#define kPINFTAG_PRINTMODE 20
#define kPINFTAG_SERIALINFO 26
#define kPINFTAG_TEMPERATURE 40
#define kPINFTAG_ERRHISTORY 50
#define kPINFTAG_TONETABLE 60


#if SUPER_VERBOSE
#define SUPER_VERBOSE_RESULT_PRINT(var) dprintf("Printer: Call Result: %d\n", var);
static void dprintf_svf(const char *fmt, va_list ap){
    dprintfv(fmt, ap);
}

static void dprintf_sv(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    dprintf_svf(fmt, ap);
    va_end(ap);
}
#else
#define SUPER_VERBOSE_RESULT_PRINT(var)
#define dprintf_sv(...)
#endif

static const int8_t idNumber = 0x01;
static const uint64_t serialNo = 0x534E4952504C4F4C;

static uint8_t mainFirmware[0x40] = {0};
static uint8_t paramFirmware[0x40] = {0};

static int32_t STATUS = 0;
static int32_t PRINTER_MODEL;
static uint16_t WIDTH = 0;
static uint16_t HEIGHT = 0;

static uint8_t currentRfidId[CARD_ID_LEN] = {0};
static bool rfid_write_has_started = false;
static int current_page = 0;

static int32_t PAPERINFO[10];
static int32_t CURVE[3][3];
static uint8_t POLISH[2];
static int32_t MTF[9];

#pragma region hooktable
static const struct hook_symbol C3XXFWDLusb_hooks[] = {
    {
        .name   = "fwdlusb_open",
        .patch  = fwdlusb_open,
        .link   = NULL
    }, {
        .name   = "fwdlusb_close",
        .patch  = fwdlusb_close,
        .link   = NULL
    },  {
        .name   = "fwdlusb_listupPrinter",
        .patch  = fwdlusb_listupPrinter,
        .link   = NULL
    },  {
        .name   = "fwdlusb_listupPrinterSN",
        .patch  = fwdlusb_listupPrinterSN,
        .link   = NULL
    },  {
        .name   = "fwdlusb_selectPrinter",
        .patch  = fwdlusb_selectPrinter,
        .link   = NULL
    },  {
        .name   = "fwdlusb_selectPrinterSN",
        .patch  = fwdlusb_selectPrinterSN,
        .link   = NULL
    },  {
        .name   = "fwdlusb_getPrinterInfo",
        .patch  = fwdlusb_getPrinterInfo,
        .link   = NULL
    },  {
        .name   = "fwdlusb_status",
        .patch  = fwdlusb_status,
        .link   = NULL
    },  {
        .name   = "fwdlusb_statusAll",
        .patch  = fwdlusb_statusAll,
        .link   = NULL
    },  {
        .name   = "fwdlusb_resetPrinter",
        .patch  = fwdlusb_resetPrinter,
        .link   = NULL
    },  {
        .name   = "fwdlusb_updateFirmware",
        .patch  = fwdlusb_updateFirmware,
        .link   = NULL
    },  {
        .name   = "fwdlusb_getFirmwareInfo",
        .patch  = fwdlusb_getFirmwareInfo,
        .link   = NULL
    },  {
        .name   = "fwdlusb_MakeThread",
        .patch  = fwdlusb_MakeThread,
        .link   = NULL
    },  {
        .name   = "fwdlusb_ReleaseThread",
        .patch  = fwdlusb_ReleaseThread,
        .link   = NULL
    },  {
        .name   = "fwdlusb_AttachThreadCount",
        .patch  = fwdlusb_AttachThreadCount,
        .link   = NULL
    },  {
        .name   = "fwdlusb_getErrorLog",
        .patch  = fwdlusb_getErrorLog,
        .link   = NULL
    },
};

static const struct hook_symbol C3XXusb_hooks[] = {
    {
        .name   = "chcusb_MakeThread",
        .patch  = chcusb_MakeThread,
        .link   = NULL
    }, {
        .name   = "chcusb_open",
        .patch  = chcusb_open,
        .link   = NULL
    }, {
        .name   = "chcusb_close",
        .patch  = chcusb_close,
        .link   = NULL
    }, {
        .name   = "chcusb_ReleaseThread",
        .patch  = chcusb_ReleaseThread,
        .link   = NULL
    }, {
        .name   = "chcusb_listupPrinter",
        .patch  = chcusb_listupPrinter,
        .link   = NULL
    }, {
        .name   = "chcusb_listupPrinterSN",
        .patch  = chcusb_listupPrinterSN,
        .link   = NULL
    }, {
        .name   = "chcusb_selectPrinter",
        .patch  = chcusb_selectPrinter,
        .link   = NULL
    }, {
        .name   = "chcusb_selectPrinterSN",
        .patch  = chcusb_selectPrinterSN,
        .link   = NULL
    }, {
        .name   = "chcusb_getPrinterInfo",
        .patch  = chcusb_getPrinterInfo,
        .link   = NULL
    }, {
        .name   = "chcusb_setmtf",
        .patch  = chcusb_setmtf,
        .link   = NULL
    }, {
        .name   = "chcusb_makeGamma",
        .patch  = chcusb_makeGamma,
        .link   = NULL
    }, {
        .name   = "chcusb_setIcctable",
        .patch  = chcusb_setIcctable,
        .link   = NULL
    }, {
        .name   = "chcusb_copies",
        .patch  = chcusb_copies,
        .link   = NULL
    }, {
        .name   = "chcusb_status",
        .patch  = chcusb_status,
        .link   = NULL
    }, {
        .name   = "chcusb_statusAll",
        .patch  = chcusb_statusAll,
        .link   = NULL
    }, {
        .name   = "chcusb_startpage",
        .patch  = chcusb_startpage,
        .link   = NULL
    }, {
        .name   = "chcusb_endpage",
        .patch  = chcusb_endpage,
        .link   = NULL
    }, {
        .name   = "chcusb_write",
        .patch  = chcusb_write,
        .link   = NULL
    }, {
        .name   = "chcusb_writeLaminate",
        .patch  = chcusb_writeLaminate,
        .link   = NULL
    }, {
        .name   = "chcusb_writeHolo",
        .patch  = chcusb_writeHolo,
        .link   = NULL
    }, {
        .name   = "chcusb_setPrinterInfo",
        .patch  = chcusb_setPrinterInfo,
        .link   = NULL
    }, {
        .name   = "chcusb_getGamma",
        .patch  = chcusb_getGamma,
        .link   = NULL
    }, {
        .name   = "chcusb_getMtf",
        .patch  = chcusb_getMtf,
        .link   = NULL
    }, {
        .name   = "chcusb_cancelCopies",
        .patch  = chcusb_cancelCopies,
        .link   = NULL
    }, {
        .name   = "chcusb_setPrinterToneCurve",
        .patch  = chcusb_setPrinterToneCurve,
        .link   = NULL
    }, {
        .name   = "chcusb_getPrinterToneCurve",
        .patch  = chcusb_getPrinterToneCurve,
        .link   = NULL
    }, {
        .name   = "chcusb_blinkLED",
        .patch  = chcusb_blinkLED,
        .link   = NULL
    }, {
        .name   = "chcusb_resetPrinter",
        .patch  = chcusb_resetPrinter,
        .link   = NULL
    }, {
        .name   = "chcusb_AttachThreadCount",
        .patch  = chcusb_AttachThreadCount,
        .link   = NULL
    }, {
        .name   = "chcusb_getPrintIDStatus",
        .patch  = chcusb_getPrintIDStatus,
        .link   = NULL
    }, {
        .name   = "chcusb_setPrintStandby",
        .patch  = chcusb_setPrintStandby,
        .link   = NULL
    }, {
        .name   = "chcusb_testCardFeed",
        .patch  = chcusb_testCardFeed,
        .link   = NULL
    }, {
        .name   = "chcusb_exitCard",
        .patch  = chcusb_exitCard,
        .link   = NULL
    }, {
        .name   = "chcusb_getCardRfidTID",
        .patch  = chcusb_getCardRfidTID,
        .link   = NULL
    }, {
        .name   = "chcusb_commCardRfidReader",
        .patch  = chcusb_commCardRfidReader,
        .link   = NULL
    }, {
        .name   = "chcusb_updateCardRfidReader",
        .patch  = chcusb_updateCardRfidReader,
        .link   = NULL
    }, {
        .name   = "chcusb_getErrorLog",
        .patch  = chcusb_getErrorLog,
        .link   = NULL
    }, {
        .name   = "chcusb_getErrorStatus",
        .patch  = chcusb_getErrorStatus,
        .link   = NULL
    }, {
        .name   = "chcusb_setCutList",
        .patch  = chcusb_setCutList,
        .link   = NULL
    }, {
        .name   = "chcusb_setLaminatePattern",
        .patch  = chcusb_setLaminatePattern,
        .link   = NULL
    }, {
        .name   = "chcusb_color_adjustment",
        .patch  = chcusb_color_adjustment,
        .link   = NULL
    }, {
        .name   = "chcusb_color_adjustmentEx",
        .patch  = chcusb_color_adjustmentEx,
        .link   = NULL
    }, {
        .name   = "chcusb_getEEPROM",
        .patch  = chcusb_getEEPROM,
        .link   = NULL
    }, {
        .name   = "chcusb_setParameter",
        .patch  = chcusb_setParameter,
        .link   = NULL
    }, {
        .name   = "chcusb_getParameter",
        .patch  = chcusb_getParameter,
        .link   = NULL
    }, {
        .name   = "chcusb_universal_command",
        .patch  = chcusb_universal_command,
        .link   = NULL
    }, {
        .name   = "chcusb_writeIred",
        .patch  = chcusb_writeIred,
        .link   = NULL
    },
};

static const struct hook_symbol C310usb_hooks[] = {
    {
        .name   = "chcusb_imageformat",
        .patch  = chcusb_imageformat_310,
        .link   = NULL
    },
};

static const struct hook_symbol C330usb_hooks[] = {
    {
        .name   = "chcusb_imageformat",
        .patch  = chcusb_imageformat_330,
        .link   = NULL
    },
};
#pragma endregion


static struct printer_config printer_config;
static struct printer_save_data printer_save_data;

DWORD printer_load_data(struct printer_save_data *svd, const wchar_t *filename){
    DWORD bytesRead = 0;
    HANDLE hSave = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hSave != INVALID_HANDLE_VALUE) {
        if (!ReadFile(hSave, (void*)svd, sizeof(*svd), &bytesRead, NULL)){
            CloseHandle(hSave);
            return GetLastError();
        }
        CloseHandle(hSave);
        if (bytesRead != sizeof(*svd)){
            return 1;
        }
        return 0;
    } else {
        return GetLastError();
    }
}

DWORD printer_write_data(struct printer_save_data *svd, const wchar_t *filename){
    DWORD bytesWritten = 0;
    HANDLE hSave = CreateFileW(filename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hSave != NULL) {
        if (!WriteFile(hSave, (void*)svd, sizeof(*svd), &bytesWritten, NULL)){
            CloseHandle(hSave);
            dprintf("Printer: Failed writing save data: %lx\n", GetLastError());
            return GetLastError();
        }
        CloseHandle(hSave);
        return 0;
    } else {
        dprintf("Printer: Failed opening save data file for writing: %lx\n", GetLastError());
        return GetLastError();
    }
}

void generate_new_rfid_id(){
    rfid_get_random_id(currentRfidId, CARD_ID_LEN);
}

void printer_config_load(struct printer_config *cfg, const wchar_t *filename){
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"printer", L"enable", 1, filename);
    cfg->clean_on_start = GetPrivateProfileIntW(L"printer", L"clean_on_start", 1, filename);
    cfg->auto_refill = GetPrivateProfileIntW(L"printer", L"auto_refill_paper", 1, filename);
    cfg->write_rfid_to_image = GetPrivateProfileIntW(L"printer", L"write_rfid_to_image", 1, filename);
    cfg->write_holo = GetPrivateProfileIntW(L"printer", L"write_holo", 0, filename);
    GetPrivateProfileStringA("printer", "serialno", "LOLPRINT", cfg->printer_serialno, 9, ".\\segatools.ini");

    cfg->rfid_boot_ver = GetPrivateProfileIntW(L"rfid", L"boot_fw_ver_reader", 0, filename);
    cfg->rfid_app_ver = GetPrivateProfileIntW(L"rfid", L"app_fw_ver_reader", 0, filename);
    cfg->write_rfid_bin = GetPrivateProfileIntW(L"rfid", L"enable_bin_writing", 0, filename);

    wchar_t board_tmp[18];
    GetPrivateProfileStringW(L"rfid", L"board_number_reader", L"837-15345", board_tmp, 18, filename);
    wcstombs(cfg->rfid_board_no, board_tmp, 9);


    GetPrivateProfileStringW(
                L"printer",
                L"main_fw_path",
                L".\\DEVICE\\printer_main_fw.bin",
                cfg->main_fw_path,
                _countof(cfg->main_fw_path),
                filename);
    GetPrivateProfileStringW(
            L"printer",
            L"param_fw_path",
            L".\\DEVICE\\printer_param_fw.bin",
            cfg->param_fw_path,
            _countof(cfg->param_fw_path),
            filename);
    GetPrivateProfileStringW(
            L"printer",
            L"print_path",
            L".\\DEVICE\\printer",
            cfg->print_path,
            _countof(cfg->print_path),
            filename);
    GetPrivateProfileStringW(
            L"printer",
            L"save_path",
            L".\\DEVICE\\printer_save_data.dat",
            cfg->save_path,
            _countof(cfg->save_path),
            filename);
    GetPrivateProfileStringW(
            L"rfid",
            L"path",
            L".\\DEVICE\\card",
            cfg->rfid_path,
            _countof(cfg->rfid_path),
            filename);

}

void check_printer_init(){
    if (printer_config.rfid_boot_ver <= 0){
        dprintf("WARNING: printer dll initialized indirectly, have to use default params\n");
        printer_config_load(&printer_config, L".\\bin\\segatools.ini");
        printer_hook_init(&printer_config, NULL, 0, false, false);
    }
}

void printer_hook_init(const struct printer_config *cfg, HINSTANCE self, int model, bool apply_dll_hook, bool apply_hook_table)
{
    HANDLE fwFile = NULL;
    DWORD bytesRead = 0;

    assert(cfg != NULL);

    if (!cfg->enable) {
        return;
    }

    memcpy(&printer_config, cfg, sizeof(*cfg));
    // generic hooks
    if (apply_hook_table){
        hook_table_apply(NULL, "C310Ausb.dll", C3XXusb_hooks, _countof(C3XXusb_hooks));
        hook_table_apply(NULL, "C310FWDLusb.dll", C3XXFWDLusb_hooks, _countof(C3XXFWDLusb_hooks));
        hook_table_apply(NULL, "C310Busb.dll", C3XXusb_hooks, _countof(C3XXusb_hooks));
        hook_table_apply(NULL, "C310BFWDLusb.dll", C3XXFWDLusb_hooks, _countof(C3XXFWDLusb_hooks));
        hook_table_apply(NULL, "C320Ausb.dll", C3XXusb_hooks, _countof(C3XXusb_hooks));
        hook_table_apply(NULL, "C320AFWDLusb.dll", C3XXFWDLusb_hooks, _countof(C3XXFWDLusb_hooks));
        hook_table_apply(NULL, "C330Ausb.dll", C3XXusb_hooks, _countof(C3XXusb_hooks));
        hook_table_apply(NULL, "C330AFWDLusb.dll", C3XXFWDLusb_hooks, _countof(C3XXFWDLusb_hooks));

        // specific hooks
        hook_table_apply(NULL, "C310Ausb.dll", C310usb_hooks, _countof(C310usb_hooks));
        hook_table_apply(NULL, "C330Ausb.dll", C330usb_hooks, _countof(C330usb_hooks));
    }

    if (self != NULL && apply_dll_hook) {
        dll_hook_push(self, L"C310Ausb.dll");
        dll_hook_push(self, L"C310FWDLusb.dll");
        dll_hook_push(self, L"C330Ausb.dll");
        dll_hook_push(self, L"C330AFWDLusb.dll");
        dll_hook_push(self, L"C310Busb.dll");
        dll_hook_push(self, L"C310BFWDLusb.dll");
        dll_hook_push(self, L"C320Ausb.dll");
        dll_hook_push(self, L"C320FWDLusb.dll");
    }

    // Load firmware if has previously been written to
    fwFile = CreateFileW(cfg->main_fw_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fwFile != NULL) {
        ReadFile(fwFile, mainFirmware, 0x40, &bytesRead, NULL);
        CloseHandle(fwFile);
    }

    fwFile = CreateFileW(cfg->param_fw_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fwFile != NULL) {
        ReadFile(fwFile, paramFirmware, 0x40, &bytesRead, NULL);
        CloseHandle(fwFile);
    }

    DWORD loadRet = printer_load_data(&printer_save_data, cfg->save_path);
    if (loadRet && loadRet != ERROR_FILE_NOT_FOUND){
        dprintf("Printer: failed loading save data from %S: %lx\n", cfg->save_path, loadRet);
    }
    if (printer_save_data.version != PRINT_SAVE_DATA_VERSION){
        memset(&printer_save_data, 0, sizeof(printer_save_data));
        printer_save_data.version = PRINT_SAVE_DATA_VERSION;
        printer_save_data.remain.ribbon = 50;
        printer_save_data.remain.holo = 50;
        printer_write_data(&printer_save_data, cfg->save_path);
        dprintf("Printer: save data created\n");
    }

    if (cfg->clean_on_start){
        SHFILEOPSTRUCTW fos = {0};
        fos.wFunc = FO_DELETE;
        fos.pFrom = cfg->print_path;
        fos.fFlags = FOF_NO_UI;
        CoInitialize(NULL);
        int ret = SHFileOperationW(&fos);
        if (ret && ret != ERROR_FILE_NOT_FOUND){
            dprintf("Printer: failed clearing printer directory: %x\n", ret);
        } else if (!ret) {
            dprintf("Printer: printer directory cleaned\n");
        }
        if (!CreateDirectoryW(cfg->print_path, NULL)){
            dprintf("Printer: failed creating printer directory: %lx\n", GetLastError());
        }
    }

    PRINTER_MODEL = model;

    generate_new_rfid_id();

    dprintf("Printer: hook enabled.\n");
}

int fwdlusb_open(uint16_t* rResult)
{
    dprintf("Printer: fwdlusb_open(%p)\n", rResult);

    check_printer_init();

    *rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return 1;
}

void fwdlusb_close()
{
    dprintf("Printer: fwdlusb_close()\n");
}

int fwdlusb_listupPrinter(uint8_t* rIdArray)
{
    dprintf("Printer: fwdlusb_listupPrinter(%p)\n", rIdArray);
    memset(rIdArray, 0xFF, 0x80);
    rIdArray[0] = idNumber;
    return 1;
}

int fwdlusb_listupPrinterSN(uint64_t* rSerialArray)
{
    dprintf("Printer: fwdlusb_listupPrinterSN(%p)\n", rSerialArray);
    memset(rSerialArray, 0xFF, 0x400);
    rSerialArray[0] = serialNo;
    return 1;
}

int fwdlusb_selectPrinter(uint8_t printerId, uint16_t* rResult)
{
    dprintf("Printer: fwdlusb_selectPrinter(%d, %p)\n", printerId, rResult);
    *rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return 1;
}

int fwdlusb_selectPrinterSN(uint64_t printerSN, uint16_t* rResult)
{
    dprintf("Printer: fwdlusb_selectPrinterSN(%I64d, %p)\n", printerSN, rResult);
    *rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return 1;
}

int fwdlusb_getPrinterInfo(uint16_t tagNumber, uint8_t* rBuffer, uint32_t* rLen)
{
    dprintf("Printer: fwdlusb_getPrinterInfo(%d, %p, %d)\n", tagNumber, rBuffer, *rLen);

    switch (tagNumber)
    {
        case kPINFTAG_PAPER: //getPaperInfo
            if (*rLen != 0x67) *rLen = 0x67;
            if (rBuffer) memset(rBuffer, 0, *rLen);
            return 1;

        case kPINFTAG_ENGID: // getFirmwareVersion
            if (*rLen != 0x99) *rLen = 0x99;
            if (rBuffer)
            {
                memset(rBuffer, 0, *rLen);
                // bootFirmware
                int i = 1;
                memcpy(rBuffer + i, mainFirmware, sizeof(mainFirmware));
                // mainFirmware
                i += 0x26;
                memcpy(rBuffer + i, mainFirmware, sizeof(mainFirmware));
                // printParameterTable
                i += 0x26;
                memcpy(rBuffer + i, paramFirmware, sizeof(paramFirmware));
            }
            return 1;

        case kPINFTAG_PRINTCNT2: // getPrintCountInfo
            if (!rBuffer)
            {
                *rLen = 0x28;
                return 1;
            }
            int32_t bInfo[10] = { 0 };
            bInfo[0] = printer_save_data.stats.print_count_1;      // printCounter0
            bInfo[1] = printer_save_data.stats.print_count_2;      // printCounter1
            bInfo[2] = printer_save_data.stats.feed_roller;      // feedRollerCount
            bInfo[3] = printer_save_data.stats.cutter;      // cutterCount
            bInfo[4] = printer_save_data.stats.head;      // headCount
            bInfo[5] = printer_save_data.remain.ribbon;     // ribbonRemain
            bInfo[6] = printer_save_data.remain.holo;     // holoHeadCount
            if (*rLen <= 0x1Cu)
            {
                memcpy(rBuffer, bInfo, *rLen);
            }
            else
            {
                bInfo[7] = printer_save_data.stats.paper;  // paperCount
                bInfo[8] = printer_save_data.stats.print_count_3;  // printCounter2
                bInfo[9] = printer_save_data.stats.holo_count;  // holoPrintCounter
                if (*rLen > 0x28u) *rLen = 0x28;
                memcpy(rBuffer, bInfo, *rLen);
            }
            break;
        case kPINFTAG_PRINTSTANDBY:
            *rLen = 1;
            if (rBuffer) {
                memset(rBuffer, 0, 1);
            }
            break;
        case kPINFTAG_SERIALINFO: // getPrinterSerial
            if (*rLen != 8) *rLen = 8;
            if (rBuffer) memcpy(rBuffer, &printer_config.printer_serialno, 8);
            return 1;

            // Fall through
        default:
            dprintf("Unknown parameter 'tagNumber' value: %d", tagNumber);
            break;
    }
    return 1;
}

int fwdlusb_status(uint16_t* rResult)
{
    dprintf("Printer: fwdlusb_status(%p)\n", rResult);
    *rResult = 0;
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return 1;
}

int fwdlusb_statusAll(uint8_t* idArray, uint16_t* rResultArray)
{
    dprintf("Printer: fwdlusb_statusAll(%p, %p)\n", idArray, rResultArray);

    for (int i = 0; *(uint8_t*)(idArray + i) != 255 && i < 128; ++i)
    {
        *(uint16_t*)(rResultArray + 2 * i) = 0;
    }

    return 1;
}

int fwdlusb_resetPrinter(uint16_t* rResult)
{
    dprintf("Printer: fwdlusb_resetPrinter(%p)\n", rResult);
    *rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return 1;
}

int fwdlusb_getFirmwareVersion(uint8_t* buffer, int size)
{
    int8_t a;
    uint32_t b = 0;
    for (int32_t i = 0; i < size; ++i)
    {
        if (*(int8_t*)(buffer + i) < 0x30 || *(int8_t*)(buffer + i) > 0x39)
        {
            if (*(int8_t*)(buffer + i) < 0x41 || *(int8_t*)(buffer + i) > 0x46)
            {
                if (*(int8_t*)(buffer + i) < 0x61 || *(int8_t*)(buffer + i) > 0x66)
                {
                    return 0;
                }
                a = *(int8_t*)(buffer + i) - 0x57;
            }
            else
            {
                a = *(int8_t*)(buffer + i) - 0x37;
            }
        }
        else
        {
            a = *(int8_t*)(buffer + i) - 0x30;
        }
        b = a + 0x10 * b;
    }
    return b;
}

int fwdlusb_updateFirmware_main(uint8_t update, LPCSTR filename, uint16_t* rResult)
{
    DWORD result;
    HANDLE fwFile = NULL;
    DWORD bytesWritten = 0;

    if (filename)
    {
        HANDLE hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) return 0;
        {
            dprintf("Printer: Firmware update failed (1005, open:%s): %lx\n", filename, GetLastError());
            if (rResult) *rResult = 1005; SUPER_VERBOSE_RESULT_PRINT(*rResult);
            result = 0;
        }

        DWORD read;
        uint8_t buffer[0x40];
        result = ReadFile(hFile, buffer, 0x40, &read, NULL);
        CloseHandle(hFile);
        if (result && read > 0x24)
        {
            uint8_t rBuffer[0x40] = { 0 };

            memcpy(rBuffer, buffer + 0x2, 0x8);
            memcpy(rBuffer + 0x8, buffer + 0xA, 0x10);
            memcpy(rBuffer + 0x18, buffer + 0x20, 0xA);
            *(rBuffer + 0x22) = (uint8_t)fwdlusb_getFirmwareVersion(buffer + 0x1A, 0x2);
            *(rBuffer + 0x23) = (uint8_t)fwdlusb_getFirmwareVersion(buffer + 0x1C, 0x2);
            memcpy(rBuffer + 0x24, buffer + 0x2A, 0x2);

            memcpy(mainFirmware, rBuffer, 0x40);

            fwFile = CreateFileW(printer_config.main_fw_path, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (fwFile != NULL) {
                WriteFile(fwFile, rBuffer, 0x40, &bytesWritten, NULL);
                CloseHandle(fwFile);
            }

            if (rResult) *rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
            result = 1;
        }
        else
        {
            dprintf("Printer: Firmware update failed (1005, read): %lx\n", GetLastError());
            if (rResult) *rResult = 1005; SUPER_VERBOSE_RESULT_PRINT(*rResult);
            result = 0;
        }
    }
    else
    {
        dprintf("Printer: Firmware update failed (1006): %lx\n", GetLastError());
        if (rResult) *rResult = 1006; SUPER_VERBOSE_RESULT_PRINT(*rResult);
        result = 0;
    }

    return result;
}

int fwdlusb_updateFirmware_param(uint8_t update, LPCSTR filename, uint16_t* rResult)
{
    DWORD result;
    HANDLE fwFile = NULL;
    DWORD bytesWritten = 0;

    if (filename)
    {
        HANDLE hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) return 0;
        {
            dprintf("Printer: Firmware update failed (1005, open:%s): %lx\n", filename, GetLastError());
            if (rResult) *rResult = 1005; SUPER_VERBOSE_RESULT_PRINT(*rResult);
            result = 0;
        }

        DWORD read;
        uint8_t buffer[0x40];
        result = ReadFile(hFile, buffer, 0x40, &read, NULL);
        CloseHandle(hFile);
        if (result && read > 0x24)
        {
            uint8_t rBuffer[0x40] = { 0 };

            memcpy(rBuffer, buffer + 0x2, 8);
            memcpy(rBuffer + 0x8, buffer + 0xA, 0x10);
            memcpy(rBuffer + 0x18, buffer + 0x20, 0xA);
            memcpy(rBuffer + 0x22, buffer + 0x1A, 0x1);
            memcpy(rBuffer + 0x23, buffer + 0x1C, 0x1);
            memcpy(rBuffer + 0x24, buffer + 0x2A, 0x2);

            memcpy(paramFirmware, rBuffer, 0x40);

            fwFile = CreateFileW(printer_config.param_fw_path, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (fwFile != NULL) {
                WriteFile(fwFile, rBuffer, 0x40, &bytesWritten, NULL);
                CloseHandle(fwFile);
            }

            if (rResult) *rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
            result = 1;
        }
        else
        {
            dprintf("Printer: Firmware update failed (1005, read): %lx\n", GetLastError());
            if (rResult) *rResult = 1005; SUPER_VERBOSE_RESULT_PRINT(*rResult);
            result = 0;
        }
    }
    else
    {
        dprintf("Printer: Firmware update failed (1006): %lx\n", GetLastError());
        if (rResult) *rResult = 1006; SUPER_VERBOSE_RESULT_PRINT(*rResult);
        result = 0;
    }

    return result;
}

int fwdlusb_updateFirmware(uint8_t update, LPCSTR filename, uint16_t* rResult)
{
    dprintf("Printer: fwdlusb_updateFirmware(%d, '%s', %p)\n", update, filename, rResult);

    if (update == 1)
    {
        return fwdlusb_updateFirmware_main(update, filename, rResult);
    }
    else if (update == 3)
    {
        return fwdlusb_updateFirmware_param(update, filename, rResult);
    }
    else
    {
        *rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
        return 1;
    }
}

int fwdlusb_getFirmwareInfo_main(LPCSTR filename, uint8_t* rBuffer, uint32_t* rLen, uint16_t* rResult)
{
    DWORD result;

    if (filename)
    {
        HANDLE hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) return 0;
        {
            if (rResult) *rResult = 1005; SUPER_VERBOSE_RESULT_PRINT(*rResult);
            result = 0;
        }

        DWORD read;
        uint8_t buffer[0x40];
        result = ReadFile(hFile, buffer, 0x40, &read, NULL);
        if (result && read > 0x24)
        {
            memcpy(rBuffer, buffer + 0x2, 0x8);
            memcpy(rBuffer + 0x8, buffer + 0xA, 0x10);
            memcpy(rBuffer + 0x18, buffer + 0x20, 0xA);
            *(rBuffer + 0x22) = (uint8_t)fwdlusb_getFirmwareVersion(buffer + 0x1A, 0x2);
            *(rBuffer + 0x23) = (uint8_t)fwdlusb_getFirmwareVersion(buffer + 0x1C, 0x2);
            memcpy(rBuffer + 0x24, buffer + 0x2A, 0x2);

            if (rResult) *rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
            result = 1;
        }
        else
        {
            if (rResult) *rResult = 1005; SUPER_VERBOSE_RESULT_PRINT(*rResult);
            result = 0;
        }
    }
    else
    {
        if (rResult) *rResult = 1006; SUPER_VERBOSE_RESULT_PRINT(*rResult);
        result = 0;
    }

    return result;
}

int fwdlusb_getFirmwareInfo_param(LPCSTR filename, uint8_t* rBuffer, uint32_t* rLen, uint16_t* rResult)
{
    DWORD result;

    if (filename)
    {
        HANDLE hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) return 0;
        {
            if (rResult) *rResult = 1005; SUPER_VERBOSE_RESULT_PRINT(*rResult);
            result = 0;
        }

        DWORD read;
        uint8_t buffer[0x40];
        result = ReadFile(hFile, buffer, 0x40, &read, NULL);
        if (result && read > 0x24)
        {
            memcpy(rBuffer, buffer + 0x2, 8);
            memcpy(rBuffer + 0x8, buffer + 0xA, 0x10);
            memcpy(rBuffer + 0x18, buffer + 0x20, 0xA);
            memcpy(rBuffer + 0x22, buffer + 0x1A, 0x1);
            memcpy(rBuffer + 0x23, buffer + 0x1C, 0x1);
            memcpy(rBuffer + 0x24, buffer + 0x2A, 0x2);

            if (rResult) *rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
            result = 1;
        }
        else
        {
            if (rResult) *rResult = 1005; SUPER_VERBOSE_RESULT_PRINT(*rResult);
            result = 0;
        }
    }
    else
    {
        if (rResult) *rResult = 1006; SUPER_VERBOSE_RESULT_PRINT(*rResult);
        result = 0;
    }

    return result;
}

int fwdlusb_getFirmwareInfo(uint8_t update, LPCSTR filename, uint8_t* rBuffer, uint32_t* rLen, uint16_t* rResult)
{
    dprintf("Printer: fwdlusb_getFirmwareInfo(%d, '%s', %p, %p, %p)\n", update, filename, rBuffer, rLen, rResult);

    if (!rBuffer)
    {
        *rLen = 38;
        return 1;
    }
    if (*rLen > 38) *rLen = 38;
    if (update == 1)
    {
        return fwdlusb_getFirmwareInfo_main(filename, rBuffer, rLen, rResult);
    }
    else if (update == 3)
    {
        return fwdlusb_getFirmwareInfo_param(filename, rBuffer, rLen, rResult);
    }
    else
    {
        if (rResult) *rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
        return 1;
    }
}

int fwdlusb_MakeThread(uint16_t maxCount)
{
    dprintf("Printer: fwdlusb_MakeThread(%d)\n", maxCount);
    return 1;
}

int fwdlusb_ReleaseThread(uint16_t* rResult)
{
    dprintf("Printer: fwdlusb_ReleaseThread(%p)\n", rResult);
    *rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return 1;
}

int fwdlusb_AttachThreadCount(uint16_t* rCount, uint16_t* rMaxCount)
{
    dprintf("Printer: fwdlusb_AttachThreadCount(%p, %p)\n", rCount, rMaxCount);
    *rCount = 0;
    *rMaxCount = 1;
    return 1;
}

int fwdlusb_getErrorLog(uint16_t index, uint8_t* rData, uint16_t* rResult)
{
    dprintf("Printer: fwdlusb_getErrorLog(%d, %p, %p)\n", index, rData, rResult);
    *rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return 1;
}

int chcusb_MakeThread(uint16_t maxCount)
{
	dprintf_sv("Printer: chcusb_MakeThread(%d)\n", maxCount);
	return 1;
}

int chcusb_open(uint16_t* rResult)
{
	dprintf_sv("Printer: chcusb_open(%p)\n", rResult);
	dprintf("Printer: Open\n");

	check_printer_init();

	*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
	STATUS = 0;
	return 1;
}

void chcusb_close()
{
	dprintf_sv("Printer: chcusb_close()\n");
	dprintf("Printer: Close\n");
}

int chcusb_ReleaseThread(uint16_t* rResult)
{
	dprintf_sv("Printer: chcusb_ReleaseThread(%p)\n", rResult);
	return 1;
}

int chcusb_listupPrinter(uint8_t* rIdArray)
{
	dprintf_sv("Printer: chcusb_listupPrinter(%p)\n", rIdArray);
	memset(rIdArray, 0xFF, 0x80);
	rIdArray[0] = idNumber;
	return 1;
}

int chcusb_listupPrinterSN(uint64_t* rSerialArray)
{
	dprintf_sv("Printer: chcusb_listupPrinterSN(%p)\n", rSerialArray);
	memset(rSerialArray, 0xFF, 0x400);
	rSerialArray[0] = serialNo;
	return 1;
}

int chcusb_selectPrinter(uint8_t printerId, uint16_t* rResult)
{
	dprintf_sv("Printer: chcusb_selectPrinter(%d, %p)\n", printerId, rResult);
	*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
	return 1;
}

int chcusb_selectPrinterSN(uint64_t printerSN, uint16_t* rResult)
{
	dprintf_sv("Printer: chcusb_selectPrinterSN(%I64d, %p)\n", printerSN, rResult);
	*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
	//DebugBreak();
	return 1;
}

int chcusb_getPrinterInfo(uint16_t tagNumber, uint8_t* rBuffer, uint32_t* rLen)
{
    #if SUPER_VERBOSE
	dprintf_sv("Printer: chcusb_getPrinterInfo(%d, %p, %d)\n", tagNumber, rBuffer, *rLen);
	#endif

	switch (tagNumber)
	{
		// getPaperInfo
	case kPINFTAG_PAPER:
		if (*rLen != 0x67) *rLen = 0x67;
		if (rBuffer) memset(rBuffer, 0, *rLen);
		break;

		// getFirmwareVersion
	case kPINFTAG_ENGID:
		if (*rLen != 0x99) *rLen = 0x99;
		if (rBuffer)
		{
			memset(rBuffer, 0, *rLen);
			// bootFirmware
			int i = 1;
			memcpy(rBuffer + i, mainFirmware, sizeof(mainFirmware));
			// mainFirmware
			i += 0x26;
			memcpy(rBuffer + i, mainFirmware, sizeof(mainFirmware));
			// printParameterTable
			i += 0x26;
			memcpy(rBuffer + i, paramFirmware, sizeof(paramFirmware));
		}
		break;

		// getPrintCountInfo
	case kPINFTAG_PRINTCNT2:
		if (!rBuffer)
		{
			*rLen = 0x28;
			return 1;
		}
        int32_t bInfo[10] = { 0 };
        bInfo[0] = printer_save_data.stats.print_count_1;      // printCounter0
        bInfo[1] = printer_save_data.stats.print_count_2;      // printCounter1
        bInfo[2] = printer_save_data.stats.feed_roller;      // feedRollerCount
        bInfo[3] = printer_save_data.stats.cutter;      // cutterCount
        bInfo[4] = printer_save_data.stats.head;      // headCount
        bInfo[5] = printer_save_data.remain.ribbon;     // ribbonRemain
        bInfo[6] = printer_save_data.remain.holo;     // holoHeadCount
        if (*rLen <= 0x1Cu)
        {
            memcpy(rBuffer, bInfo, *rLen);
        }
        else
        {
            bInfo[7] = printer_save_data.stats.paper;  // paperCount
            bInfo[8] = printer_save_data.stats.print_count_3;  // printCounter2
            bInfo[9] = printer_save_data.stats.holo_count;  // holoPrintCounter
            if (*rLen > 0x28u) *rLen = 0x28;
            memcpy(rBuffer, bInfo, *rLen);
        }
		break;

		// Unknown.
	case 6:
		*rLen = 32;
		if (rBuffer)
		{
			memset(rBuffer, 0, 32);
			// ???
			rBuffer[0] = 44;
		}
		break;

		// Unknown.
	case kPINFTAG_PRINTSTANDBY:
		*rLen = 1;
		if (rBuffer) {
			if (STATUS == 4) {
                STATUS = 5;
				memset(rBuffer, 240, 1);
			}
			else {
				if (STATUS >= 2 && STATUS < 4) {
					STATUS++;
				}
				//int x = GetPrivateProfileIntA("print", "status", 0, ".\\print.txt");
				//dprintf("PRINTSTANDBY: %d\n", x);
				memset(rBuffer, 0, 1);
			}
		}
		break;

		// getPrinterSerial
	case kPINFTAG_SERIALINFO:
		if (*rLen != 8) *rLen = 8;
		if (rBuffer) memcpy(rBuffer, &printer_config.printer_serialno, 8);
		break;

		// Unknown.
	case kPINFTAG_TEMPERATURE:
		*rLen = 10;
		if (rBuffer)
		{
			memset(rBuffer, 0, 10);
			rBuffer[0] = 1;
			rBuffer[1] = 2;
			rBuffer[2] = 3;
			/*rBuffer[0] = 0x7e;
			rBuffer[1] = 0x87;
			rBuffer[2] = 0x88;*/
		}
		break;

		// Unknown.
	case kPINFTAG_ERRHISTORY:
		*rLen = 61;
		if (rBuffer) memset(rBuffer, 0, 61);

		rBuffer[0] = 0x0A;
		rBuffer[1] = 0x06;
		rBuffer[2] = 0x0B;
		rBuffer[3] = 0x57;
		rBuffer[4] = 0x90;
		rBuffer[5] = 0x0;
		rBuffer[6] = 0x0;
        for (int i = 0; i < 9; i++){
            rBuffer[7+(i*6)] = 0x06;
            rBuffer[7+(i*6)+1] = 0x0D;
            rBuffer[7+(i*6)+2] = 0x57;
            rBuffer[7+(i*6)+3] = 0x90;
            rBuffer[7+(i*6)+4] = 0x0;
            rBuffer[7+(i*6)+5] = 0x0;
		}

		break;

		// Fall through
	default:
		dprintf("Printer: Unknown kPINFTAG: %d\n", tagNumber);
		break;
	}
	return 1;
}

int chcusb_imageformat_310(uint16_t format, uint16_t ncomp, uint16_t depth, uint16_t width, uint16_t height, uint8_t * image, uint16_t* rResult) {
	return chcusb_imageformat_330(format, ncomp, depth, width, height, rResult);
}

int chcusb_imageformat_330(uint16_t format, uint16_t ncomp, uint16_t depth, uint16_t width, uint16_t height, uint16_t* rResult) {
	dprintf_sv("Printer: chcusb_imageformat[330](%d, %d, %d, %d, %d, %d, %p)\n", format, ncomp, depth, width, height, 0, rResult);

	WIDTH = width;
	HEIGHT = height;

	*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
	return 1;
}

int chcusb_setmtf(int32_t* mtf)
{
	dprintf_sv("Printer: chcusb_setmtf(%p)\n", mtf);
	memcpy(MTF, mtf, sizeof(MTF));
	return 1;
}

int chcusb_makeGamma(uint16_t k, uint8_t* intoneR, uint8_t* intoneG, uint8_t* intoneB)
{
	dprintf_sv("Printer: chcusb_makeGamma(%d, %p, %p, %p)\n", k, intoneR, intoneG, intoneB);

	uint8_t tone;
	int32_t value;
	double power;

	double factor = (double)k / 100.0;

	for (int i = 0; i < 256; ++i)
	{
		power = pow((double)i, factor);
		value = (int)(power / pow(255.0, factor) * 255.0);

		if (value > 255)
			tone = 255;
		if (value >= 0)
			tone = value;
		else
			tone = 0;

		if (intoneR)
			*(uint8_t*)(intoneR + i) = tone;
		if (intoneG)
			*(uint8_t*)(intoneG + i) = tone;
		if (intoneB)
			*(uint8_t*)(intoneB + i) = tone;
	}

	return 1;
}

int chcusb_setIcctable(LPCSTR icc1, LPCSTR icc2, uint16_t intents, uint8_t* intoneR, uint8_t* intoneG, uint8_t* intoneB, uint8_t* outtoneR, uint8_t* outtoneG, uint8_t* outtoneB, uint16_t* rResult) {
	dprintf_sv("Printer: chcusb_setIcctable(%p, %p, %d, %p, %p, %p, %p, %p, %p, %p)\n", icc1, icc2, intents, intoneR, intoneG, intoneB, outtoneR, outtoneG, outtoneB, rResult);

    if (intoneR != NULL && intoneG != NULL && intoneB != NULL && outtoneR != NULL && outtoneG != NULL && outtoneB != NULL){
        for (int i = 0; i < 256; i++){
            intoneR[i] = i;
            intoneG[i] = i;
            intoneB[i] = i;
            outtoneR[i] = i;
            outtoneG[i] = i;
            outtoneB[i] = i;
        }
	}

	*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
	return 1;
}

int chcusb_copies(uint16_t copies, uint16_t* rResult)
{
	dprintf_sv("Printer: chcusb_copies(%d, %p)\n", copies, rResult);
	*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
	return 1;
}



int chcusb_statusAll(uint8_t* idArray, uint16_t* rResultArray)
{
	dprintf_sv("Printer: chcusb_statusAll(%p, %p)\n", idArray, rResultArray);

	for (int i = 0; *(uint8_t*)(idArray + i) != 255 && i < 128; ++i)
	{
		*(uint16_t*)(rResultArray + 2 * i) = 0;
	}

	return 1;
}

int chcusb_startpage(uint16_t postCardState, uint16_t* pageId, uint16_t* rResult)
{
	dprintf_sv("Printer: chcusb_startpage(%d, %p, %p)\n", postCardState, pageId, rResult);
	current_page = 0;
    *pageId = 1;
	*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
	return 1;
}

int chcusb_endpage(uint16_t* rResult)
{
	dprintf_sv("Printer: chcusb_endpage(%p)\n", rResult);

    dprintf("Printer: End Printing\n");

	STATUS = 2;

	*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
	return 1;
}

int chcusb_write(uint8_t* data, uint32_t* writeSize, uint16_t* rResult)
{
	SYSTEMTIME t;
	GetLocalTime(&t);

	wchar_t dumpPath[MAX_PATH];
    swprintf_s
	(
		dumpPath, MAX_PATH,
		L"%s\\%04d%02d%02d%02d%02d%02d_p%d.bmp",
		printer_config.print_path, t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, current_page++
	);
    dprintf("Printer: writing to %ls\n", dumpPath);

    WriteDataToBitmapFile(dumpPath, 24, WIDTH, HEIGHT, data, IMAGE_SIZE, NULL, 0);
    if (printer_config.write_rfid_to_image){
        if (rfid_file_is_open()){
            rfid_file_commit(dumpPath, true, NULL);
        } else {
            dprintf("Printer: No RFID data found for writing!\n");
        }
    }

    printer_save_data.stats.print_count_1++;
    printer_save_data.stats.feed_roller++;
    printer_save_data.stats.head++;
    printer_save_data.stats.paper++;
    if (printer_save_data.remain.ribbon-- < 3 && printer_config.auto_refill){
        printer_save_data.remain.ribbon += 25;
    }
    printer_write_data(&printer_save_data, printer_config.save_path);

	*writeSize = IMAGE_SIZE;
	*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);

	return 1;
}

int chcusb_writeLaminate(uint8_t* data, uint32_t* writeSize, uint16_t* rResult)
{
	SYSTEMTIME t; GetLocalTime(&t);

	wchar_t dumpPath[MAX_PATH];
    swprintf_s
    (
            dumpPath, MAX_PATH,
            L"%s\\%04d%02d%02d%02d%02d%02dlaminate.bin",
            printer_config.print_path, t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond
    );
    dprintf("Printer: writing to %ls\n", dumpPath);
	WriteArrayToFile(dumpPath, data, *writeSize, FALSE);
	dprintf_sv("Printer: chcusb_writeLaminate(%p, %p, %p)\n", data, writeSize, rResult);

	// *writeSize = written;
	*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);

	return 1;
}

int chcusb_writeHolo(uint8_t* data, uint32_t* writeSize, uint16_t* rResult)
{
	SYSTEMTIME t; GetLocalTime(&t);

	wchar_t dumpPath[MAX_PATH];
    swprintf_s
	(
		dumpPath, MAX_PATH,
		L"%s\\%04d%02d%02d%02d%02d%02dholo.bmp",
		printer_config.print_path, t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond
	);
    dprintf("Printer: writing to %ls\n", dumpPath);
    printer_save_data.stats.holo_count++;
    if (printer_save_data.remain.holo-- < 3 && printer_config.auto_refill){
        printer_save_data.remain.holo += 25;
    }
    printer_write_data(&printer_save_data, printer_config.save_path);

    if (printer_config.write_holo){
	    WriteDataToBitmapFile(dumpPath, 8, WIDTH, HEIGHT, data, HOLO_SIZE, NULL, 0);
	}

	*writeSize = HOLO_SIZE;
	*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);

	return 1;
}

int chcusb_setPrinterInfo(uint16_t tagNumber, uint8_t* rBuffer, uint32_t* rLen, uint16_t* rResult)
{
	dprintf_sv("Printer: chcusb_setPrinterInfo(%d, %p, %d, %p)\n", tagNumber, rBuffer, *rLen, rResult);

	switch (tagNumber)
	{
		//setPaperInfo
		case 0:
			memcpy(PAPERINFO, rBuffer, sizeof(PAPERINFO));
			break;

		// setPolishInfo
		case 20:
			memcpy(POLISH, rBuffer, sizeof(POLISH));
			break;

		default:
			break;
	}

	*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
	return 1;
}

int chcusb_getGamma(LPCSTR filename, uint8_t* r, uint8_t* g, uint8_t* b, uint16_t* rResult)
{
	dprintf_sv("Printer: chcusb_getGamma('%s', %p, %p, %p, %p)\n", filename, r, g, b, rResult);

	for (int i = 0; i < 256; ++i)
	{
		r[i] = i;
		g[i] = i;
		b[i] = i;
	}

	*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
	return 1;
}

int chcusb_getMtf(LPCSTR filename, int32_t* mtf, uint16_t* rResult)
{
	dprintf_sv("Printer: chcusb_getMtf('%s', %p, %p)\n", filename, mtf, rResult);

	HANDLE hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) return 0;

	DWORD read;
	uint8_t buffer[0x40];
	BOOL result = ReadFile(hFile, buffer, 0x40, &read, NULL);
	if (!result) return 0;

	int a, c;
	int b = 1;
	int d = c = 0;

	memset(mtf, 0, sizeof(MTF));

	for (DWORD i = 0; i < read; i++)
	{
		a = buffer[i] - 0x30;
		if (a == -3 && c == 0)
		{
			b = -1;
		}
		else if (a >= 0 && a <= 9)
		{
			mtf[d] = mtf[d] * 10 + a;
			c++;
		}
		else if (c > 0)
		{
			mtf[d] *= b;
			b = 1;
			c = 0;
			d++;
		}
		if (d > 9) break;
	}

	*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
	return 1;
}

int chcusb_cancelCopies(uint16_t pageId, uint16_t* rResult)
{
	dprintf_sv("Printer: chcusb_cancelCopies(%d, %p)\n", pageId, rResult);
	*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
	return 1;
}

int chcusb_setPrinterToneCurve(uint16_t type, uint16_t number, uint16_t* data, uint16_t* rResult)
{
	dprintf_sv("Printer: chcusb_setPrinterToneCurve(%d, %d, %p, %p)\n", type, number, data, rResult);
	if (0 < type && type < 3 && 0 < number && number < 3) CURVE[type][number] = *data;
	*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
	return 1;
}

int chcusb_getPrinterToneCurve(uint16_t type, uint16_t number, uint16_t* data, uint16_t* rResult)
{
	dprintf_sv("Printer: chcusb_getPrinterToneCurve(%d, %d, %p, %p)\n", type, number, data, rResult);
	if (0 < type && type < 3 && 0 < number && number < 3) *data = CURVE[type][number];
	*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
	return 1;
}

int chcusb_blinkLED(uint16_t* rResult)
{
	dprintf_sv("Printer: chcusb_blinkLED(%p)\n", rResult);
	*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
	return 1;
}

int chcusb_resetPrinter(uint16_t* rResult)
{
	dprintf_sv("Printer: chcusb_resetPrinter(%p)\n", rResult);
	*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
	return 1;
}

int chcusb_AttachThreadCount(uint16_t* rCount, uint16_t* rMaxCount)
{
	dprintf_sv("Printer: chcusb_AttachThreadCount(%p, %p)\n", rCount, rMaxCount);
	*rCount = 0;
	*rMaxCount = 1;
	return 1;
}

int chcusb_getPrintIDStatus(uint16_t pageId, uint8_t* rBuffer, uint16_t* rResult)
{
    #if SUPER_VERBOSE
	dprintf_sv("Printer: chcusb_getPrintIDStatus(%d, %p, %p)\n", pageId, rBuffer, rResult);
	#endif

	memset(rBuffer, 0, 8);
    // *rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);

    int c = 0;

    if (STATUS > 1) {
        c = 2212; // print complete
    } else {
        c = 2300; // no print
    }

    //c = GetPrivateProfileIntA("print", "printid", c, ".\\print.txt");
    //dprintf("PRINTID: %d\n", c);
    for (int i = 0; i < 7; i++){
        *((uint16_t*)(rBuffer + i)) = 0;
    }
    *((uint16_t*)(rBuffer + 6)) = c;
    if (GetAsyncKeyState(0x35) & 0x8000){
    DebugBreak();
    }

    SUPER_VERBOSE_RESULT_PRINT(*((uint16_t*)(rBuffer + 6)));
	*rResult = 0;
	return 1;
}

int chcusb_status(uint16_t* rResult)
{
    #if SUPER_VERBOSE
	dprintf_sv("Printer: chcusb_status(%p)\n", rResult);
	#endif
	*rResult = 0;
    if ((GetAsyncKeyState(0x11) & 0x8000) && (GetAsyncKeyState(0x42) & 0x8000)) {
        *rResult = 0xEE;
        dprintf("Printer: Ctrl+B is active!\n");
    }
	SUPER_VERBOSE_RESULT_PRINT(*rResult);
	return 1;
}

int chcusb_setPrintStandby(uint16_t position, uint16_t* rResult)
{
	dprintf_sv("Printer: chcusb_setPrintStandby(%d, %p)\n", position, rResult);

    dprintf("Printer: Standby\n");

	*rResult = 0;

	if (STATUS == 0)
	{
	    rfid_write_has_started = false;
	    rfid_file_close();
		STATUS = 1;
		*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
	}
	else
	{
		*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
	}

	//DebugBreak();

	return 1;
}

int chcusb_testCardFeed(uint16_t mode, uint16_t times, uint16_t* rResult)
{
	dprintf_sv("Printer: chcusb_testCardFeed(%d, %d, %p)\n", mode, times, rResult);
	*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
	return 1;
}

int chcusb_exitCard(uint16_t* rResult)
{
	dprintf_sv("Printer: chcusb_exitCard(%p)\n", rResult);
	dprintf("Printer: Eject card\n");
	*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
    printer_save_data.stats.cutter++;
    printer_write_data(&printer_save_data, printer_config.save_path);
    rfid_file_close();
    generate_new_rfid_id();
	STATUS = 0;
	return 1;
}

int chcusb_getCardRfidTID(uint8_t* rCardTID, uint16_t* rResult)
{
	dprintf_sv("Printer: chcusb_getCardRfidTID(%p, %p)\n", rCardTID, rResult);

    memcpy(rCardTID, currentRfidId, CARD_ID_LEN);
    *rResult = 2405; SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return 1;
}

int chcusb_commCardRfidReader(uint8_t* sendData, uint8_t* rRecvData, uint32_t sendSize, uint32_t* rRecvSize, uint16_t* rResult)
{
	DWORD dwRecvSize = 0;
	PUINT16 pbSendData = (PUINT16)sendData;

    dprintf_sv("Printer: chcusb_commCardRfidReader(%d, %p, %d, %p, %p)\n", pbSendData[0], rRecvData, sendSize, rRecvSize, rResult);

	switch (pbSendData[0]) {
        case RFID_CMD_RESET:
            dwRecvSize = 0x8;
            memset(rRecvData, 0, dwRecvSize);
            rRecvData[0] = RFID_CMD_RESET;
            rRecvData[1] = 0x03;
            break;
        case RFID_CMD_GET_APP_VERSION:
            dwRecvSize = 0x20;
            memset(rRecvData, 0xBE, dwRecvSize);
            memset(rRecvData, 0, 8);
            rRecvData[0] = RFID_CMD_GET_APP_VERSION;
            rRecvData[1] = 0x00;
            rRecvData[2] = 0x01;
            rRecvData[3] = printer_config.rfid_app_ver;
            break;
        case RFID_CMD_UNK_INIT_2:
            dwRecvSize = 0x8;
            memset(rRecvData, 0, dwRecvSize);
            rRecvData[0] = RFID_CMD_UNK_INIT_2;
            rRecvData[1] = 0x00;
            rRecvData[2] = 0x00;
            break;
        case RFID_CMD_GET_BOOT_VERSION:
            dwRecvSize = 0x20;
            memset(rRecvData, 0xFD, dwRecvSize);
            memset(rRecvData, 0, 8);
            rRecvData[0] = RFID_CMD_GET_BOOT_VERSION;
            rRecvData[1] = 0x00;
            rRecvData[2] = 0x01;
            rRecvData[3] = printer_config.rfid_boot_ver;
            break;
        case RFID_CMD_BOARD_INFO:
            dwRecvSize = 0x12;
            memset(rRecvData, 0, dwRecvSize);
            rRecvData[0] = RFID_CMD_BOARD_INFO;
            rRecvData[1] = 0x00;
            rRecvData[2] = 0x09;
            memcpy(&rRecvData[3], printer_config.rfid_board_no, 9);
            break;
        case 4098:
            ; // lol
            wchar_t cardid_hex[29] = {0};

            for (size_t i = 0; i < CARD_ID_LEN; i++) {
                swprintf_s(cardid_hex, 29, L"%ls%02x", cardid_hex, *(sendData + 6 + i));
            }

            if (!rfid_write_has_started){

                dprintf("Printer: Start RFID write (card id: %ls)\n", cardid_hex);
                rfid_file_open();
                rfid_file_set_id(currentRfidId, CARD_ID_LEN);

                rfid_write_has_started = true;

            } else {
                dprintf("Printer: End RFID write\n");

                if (printer_config.write_rfid_bin){

                    wchar_t filename[MAX_PATH];
                    swprintf_s(filename, MAX_PATH, L"%ls\\%ls.bin", printer_config.rfid_path, cardid_hex);

                    rfid_file_commit(filename, false, NULL);
                }

            }

            dwRecvSize = 35;
            memset(rRecvData, 0, dwRecvSize);
            rRecvData[0] = 0x02;
            rRecvData[1] = 0x00;
            rRecvData[2] = 0x20;
            if (rfid_get_written_bytes() >= dwRecvSize - 3){
                int written;
                if (rfid_file_get_buffer(rRecvData + 3, dwRecvSize - 3, &written, 0)){
                    dwRecvSize = min(dwRecvSize, written + 3);
                } else {
                    dwRecvSize = 0;
                    dprintf("Printer: RFID get bytes failed\n");
                }
            }
            dprintf("Printer: RFID get bytes: %ld\n", dwRecvSize);

            break;
        case 4355:

            dprintf("Printer: RFID write block %d\n", *(sendData + 4));

            uint8_t data[2];
            memcpy(data, sendData + 5, 2);

            rfid_file_write_block(data, 2);

            dwRecvSize = 3;
            memset(rRecvData, 0, dwRecvSize);
            rRecvData[0] = 3;
            break;
        default:
            dprintf("Printer: Unknown RFID command: %d\n", pbSendData[0]);
            break;
	}

	*rRecvSize = dwRecvSize;
	*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
	return 1;

}

int chcusb_updateCardRfidReader(uint8_t* data, uint32_t size, uint16_t* rResult)
{
	dprintf_sv("Printer: chcusb_updateCardRfidReader(%p, %d, %p)\n", data, size, rResult);
	*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
	return 1;
}

int chcusb_getErrorLog(uint16_t index, uint8_t* rData, uint16_t* rResult)
{
    #if SUPER_VERBOSE
	dprintf_sv("Printer: chcusb_getErrorLog(%d, %p, %p)\n", index, rData, rResult);
	#endif
	*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
	return 1;
}

int chcusb_getErrorStatus(uint16_t* rBuffer)
{
    #if SUPER_VERBOSE
	dprintf_sv("Printer: chcusb_getErrorStatus(%p)\n", rBuffer);
	#endif
	memset(rBuffer, 0, 0x80);
	return 1;
}

int chcusb_setCutList(uint8_t* rData, uint16_t* rResult)
{
	dprintf_sv("Printer: chcusb_setCutList(%p, %p)\n", rData, rResult);
	*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
	return 1;
}

int chcusb_setLaminatePattern(uint16_t index, uint8_t* rData, uint16_t* rResult)
{
	dprintf_sv("Printer: chcusb_setLaminatePattern(%d, %p, %p)\n", index, rData, rResult);
	*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
	return 1;
}

int chcusb_color_adjustment(LPCSTR filename, int32_t a2, int32_t a3, int16_t a4, int16_t a5, int64_t a6, int64_t a7, uint16_t* rResult)
{
	dprintf_sv("Printer: chcusb_color_adjustment('%s', %d, %d, %d, %d, %I64d, %I64d, %p)\n", filename, a2, a3, a4, a5, a6, a7, rResult);
	*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
	return 1;
}

int chcusb_color_adjustmentEx(int32_t a1, int32_t a2, int32_t a3, int16_t a4, int16_t a5, int64_t a6, int64_t a7, uint16_t* rResult)
{
	dprintf_sv("Printer: chcusb_color_adjustmentEx(%d, %d, %d, %d, %d, %I64d, %I64d, %p)\n", a1, a2, a3, a4, a5, a6, a7, rResult);
	*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
	return 1;
}

int chcusb_writeIred(uint8_t* a1, uint8_t* a2, uint16_t* rResult)
{
	dprintf_sv("Printer: chcusb_writeIred(%p, %p, %p)\n", a1, a2, rResult);
	*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
	return 1;
}

int chcusb_getEEPROM(uint8_t index, uint8_t* rData, uint16_t* rResult)
{
	dprintf_sv("Printer: chcusb_getEEPROM(%d, %p, %p)\n", index, rData, rResult);
	*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
	return 1;
}

int chcusb_setParameter(uint8_t a1, uint32_t a2, uint16_t* rResult)
{
	dprintf_sv("Printer: chcusb_setParameter(%d, %d, %p)\n", a1, a2, rResult);
	*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
	return 1;
}

int chcusb_getParameter(uint8_t a1, uint8_t* a2, uint16_t* rResult)
{
	dprintf_sv("Printer: chcusb_getParameter(%d, %p, %p)\n", a1, a2, rResult);
	*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
	return 1;
}

int chcusb_universal_command(int32_t a1, uint8_t a2, int32_t a3, uint8_t* a4, uint16_t* rResult)
{
	dprintf_sv("Printer: chcusb_universal_command(%d, %d, %d, %p, %p)\n", a1, a2, a3, a4, rResult);
	*rResult = 0; SUPER_VERBOSE_RESULT_PRINT(*rResult);
	return 1;
}
