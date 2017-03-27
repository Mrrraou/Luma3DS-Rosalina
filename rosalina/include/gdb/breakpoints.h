#pragma once

#include "gdb.h"

u32 GDB_FindClosestBreakpointSlot(GDBContext *ctx, u32 address);
int GDB_AddBreakpoint(GDBContext *ctx, u32 address, bool thumb, bool persist);
int GDB_DisableBreakpointById(GDBContext *ctx, u32 id);
int GDB_RemoveBreakpoint(GDBContext *ctx, u32 address);
