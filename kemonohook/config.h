#pragma once

#include <stddef.h>

#include "amex/amex.h"

#include "board/sg-reader.h"
#include "board/config.h"
#include "board/vfd.h"

#include "hooklib/touch.h"
#include "hooklib/dvd.h"
#include "printer/printer.h"

#include "board/rfid-rw.h"

#include "platform/config.h"

struct kemono_hook_config {
    struct platform_config platform;
    struct aime_config aime;
    struct printer_config printer;
};

void kemono_hook_config_load(
        struct kemono_hook_config *cfg,
        const wchar_t *filename);
