#pragma once

#include <windows.h>
#include "printer/printer.h"

void printer_std_hook_init(const struct printer_config *cfg, HINSTANCE a, HINSTANCE fwdl, int model, bool apply_dll_hook);
