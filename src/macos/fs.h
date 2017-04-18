#pragma once

#include "platform/platform.h"

C_LINKAGE int32_t macos_fs_read(char *filename, void *memory, uint32_t size);
C_LINKAGE int32_t macos_fs_size(char *filename);
