#pragma once

#include <windows.h>

#include "jvs/jvs-bus.h"

#include "mkhook/config.h"

void mk_jvs_init(struct io_config* cfg);
HRESULT mk_jvs_node(struct jvs_node **root);
