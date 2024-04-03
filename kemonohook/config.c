#include <assert.h>
#include <stddef.h>

#include "amex/amex.h"
#include "amex/config.h"

#include "board/config.h"
#include "board/led1509306.h"
#include "board/vfd.h"

#include "kemonohook/config.h"

#include "hooklib/config.h"
#include "hooklib/dvd.h"
#include "hooklib/touch.h"
#include "hooklib/gfx.h"
#include "printer/printer.h"
#include "board/rfid-rw.h"

#include "platform/config.h"

void kemono_hook_config_load(
        struct kemono_hook_config *cfg,
        const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    printer_config_load(&cfg->printer, filename);
}
