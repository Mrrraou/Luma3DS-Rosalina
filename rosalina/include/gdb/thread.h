#pragma once

#include "gdb.h"

void GDB_UpdateCurrentThreadFromList(GDBContext *ctx, u32 *threadIds, u32 nbThreads);

GDB_DECLARE_HANDLER(SetThreadId);
GDB_DECLARE_HANDLER(IsThreadAlive);

GDB_DECLARE_QUERY_HANDLER(CurrentThreadId);
GDB_DECLARE_QUERY_HANDLER(FThreadInfo);
GDB_DECLARE_QUERY_HANDLER(SThreadInfo);
