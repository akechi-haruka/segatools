#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "board/io4.h"
#include "board/sg-reader.h"
#include "board/vfd.h"

void aime_config_load(struct aime_config *cfg, const wchar_t *filename);
void io4_config_load(struct io4_config *cfg, const wchar_t *filename);
void vfd_config_load(struct vfd_config *cfg, const wchar_t *filename);
