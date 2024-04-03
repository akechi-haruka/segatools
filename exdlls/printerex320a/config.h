#pragma once

#include <stddef.h>

#include "board/config.h"
#include "printer/printer.h"

#include "platform/config.h"

struct printer_hook_config {
    struct printer_config printer;
};

void printer_hook_config_load(
        struct printer_hook_config *cfg,
        const wchar_t *filename);
