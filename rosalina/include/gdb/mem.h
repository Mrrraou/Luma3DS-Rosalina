#pragma once

#include "gdb.h"

int GDB_SendProcessMemory(GDBContext *ctx, const char *prefix, u32 prefixLen, u32 addr, u32 len);

GDB_DECLARE_HANDLER(ReadMemory);
