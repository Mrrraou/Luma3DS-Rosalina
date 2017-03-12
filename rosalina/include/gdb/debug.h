#pragma once

#include "gdb.h"

GDB_DECLARE_HANDLER(Break);
GDB_DECLARE_HANDLER(Continue);
GDB_DECLARE_HANDLER(GetStopReason);

int GDB_SendStopReply(GDBContext *ctx, DebugEventInfo *info);
int GDB_HandleDebugEvents(GDBContext *ctx);
