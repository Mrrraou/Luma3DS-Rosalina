#pragma once

#include "gdb.h"

int GDB_SendProcessMemory(GDBContext *ctx, const char *prefix, u32 prefixLen, u32 addr, u32 len);
int GDB_WriteProcessMemory(GDBContext *ctx, const void *buf, u32 addr, u32 len);

GDB_DECLARE_HANDLER(ReadMemory);
GDB_DECLARE_HANDLER(WriteMemory);
GDB_DECLARE_HANDLER(WriteMemoryRaw);
