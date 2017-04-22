#pragma once

#include "gdb.h"

u32 GDB_GetCurrentThreadFromList(GDBContext *ctx, u32 *threadIds, u32 nbThreads);
u32 GDB_GetCurrentThread(GDBContext *ctx);

GDB_DECLARE_HANDLER(SetThreadId);
GDB_DECLARE_HANDLER(IsThreadAlive);

GDB_DECLARE_QUERY_HANDLER(CurrentThreadId);
GDB_DECLARE_QUERY_HANDLER(fThreadInfo);
GDB_DECLARE_QUERY_HANDLER(sThreadInfo);
GDB_DECLARE_QUERY_HANDLER(ThreadEvents);
GDB_DECLARE_QUERY_HANDLER(ThreadExtraInfo);
