#include <assert.h>
#include <stddef.h>

#include "board/config.h"
#include "board/led1509306.h"
#include "board/vfd.h"

#include "hooklib/config.h"
#include "hooklib/dvd.h"
#include "hooklib/touch.h"
#include "hooklib/gfx.h"
#include "board/rfid-rw.h"

#include "fgohook/config.h"

#include "platform/config.h"

void fgo_dll_config_load(
        struct fgo_dll_config *cfg,
        const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    GetPrivateProfileStringW(
            L"fgoio",
            L"path",
            L"",
            cfg->path,
            _countof(cfg->path),
            filename);
}

void fgo_hook_config_load(
        struct fgo_hook_config *cfg,
        const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    platform_config_load(&cfg->platform, filename);
    aime_config_load(&cfg->aime, filename);
    dvd_config_load(&cfg->dvd, filename);
    io4_config_load(&cfg->io4, filename);
    vfd_config_load(&cfg->vfd, filename);
    touch_config_load(&cfg->touch, filename);
    fgo_dll_config_load(&cfg->dll, filename);
    rfid_config_load(&cfg->rfid, filename);
    //rfidwriter_config_load(&cfg->rfidwriter, filename);
    led1509306_config_load(&cfg->led1509306, filename);
    printer_config_load(&cfg->printer, filename);
    //printerspy_config_load(&cfg->printer, filename);
}
