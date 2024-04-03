#include <assert.h>
#include <stddef.h>

#include "amex/amex.h"
#include "amex/config.h"

#include "board/config.h"
#include "board/led1509306.h"

#include "hooklib/config.h"
#include "hooklib/dvd.h"
#include "hooklib/touch.h"
#include "hooklib/gfx.h"
#include "printer/printer.h"
#include "board/rfid-rw.h"

#include "platform/config.h"

#include "exdlls/printerex320a/config.h"

void printer_hook_config_load(
        struct printer_hook_config *cfg,
        const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    printer_config_load(&cfg->printer, filename);
}
