#pragma once

#include "gdb.h"

GDB_DECLARE_HANDLER(Detach);
GDB_DECLARE_HANDLER(Kill);
GDB_DECLARE_HANDLER(Break);
GDB_DECLARE_HANDLER(Continue);
GDB_DECLARE_HANDLER(GetStopReason);

int GDB_SendStopReply(GDBContext *ctx, const DebugEventInfo *info);
int GDB_HandleDebugEvents(GDBContext *ctx);
bool GDB_BreakProcessAndSinkDebugEvents(GDBContext *ctx, const DebugEventInfo *info, DebugFlags flags);
