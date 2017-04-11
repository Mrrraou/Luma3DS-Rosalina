#pragma once

#include "gdb.h"

Result GDB_ReadMemoryInPage(void *out, GDBContext *ctx, u32 addr, u32 len);
Result GDB_WriteMemoryInPage(GDBContext *ctx, const void *in, u32 addr, u32 len);
int GDB_SendMemory(GDBContext *ctx, const char *prefix, u32 prefixLen, u32 addr, u32 len);
int GDB_WriteMemory(GDBContext *ctx, const void *buf, u32 addr, u32 len);

GDB_DECLARE_HANDLER(ReadMemory);
GDB_DECLARE_HANDLER(WriteMemory);
GDB_DECLARE_HANDLER(WriteMemoryRaw);
