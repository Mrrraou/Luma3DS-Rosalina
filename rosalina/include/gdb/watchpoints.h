#pragma once

#include "gdb.h"

typedef enum WatchpointKind
{
    WATCHPOINT_DISABLED = 0,
    WATCHPOINT_READ,
    WATCHPOINT_WRITE,
    WATCHPOINT_READWRITE
} WatchpointKind;

void GDB_ResetWatchpoints(void); // needed for software breakpoints to be detected as debug events as well

int GDB_AddWatchpoint(GDBContext *ctx, u32 address, u32 size, WatchpointKind kind);
int GDB_RemoveWatchpoint(GDBContext *ctx, u32 address, WatchpointKind kind);

WatchpointKind GDB_GetWatchpointKind(GDBContext *ctx, u32 address);
