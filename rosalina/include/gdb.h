#pragma once

#include <3ds/types.h>
#include <3ds/svc.h>
#include <3ds/synchronization.h>
#include <3ds/result.h>
#include "sock_util.h"
#include "macros.h"
#include "memory.h"

#define MAX_DEBUG 3
#define MAX_THREAD 32
#define GDB_BUF_LEN 512

#define GDB_DECLARE_HANDLER(name) int GDB_Handle##name(GDBContext *ctx)
#define GDB_DECLARE_QUERY_HANDLER(name) GDB_DECLARE_HANDLER(Query##name)
#define GDB_DECLARE_LONG_HANDLER(name) GDB_DECLARE_HANDLER(Long##name)

typedef enum GDBFlags
{
    GDB_FLAG_USED  = 1,
    GDB_FLAG_PROCESS_CONTINUING = 2
} GDBFlags;

typedef enum GDBState
{
    GDB_STATE_CONNECTED,
    GDB_STATE_NOACK_SENT,
    GDB_STATE_NOACK
} GDBState;

typedef struct GDBContext
{
    sock_ctx socketCtx;

    RecursiveLock lock;
    GDBFlags flags;
    GDBState state;

    u32 pid;
    Handle process;
    Handle debug;

    u32 currentThreadId, selectedThreadId;

    Handle clientAcceptedEvent, continuedEvent;
    Handle eventToWaitFor;

    bool catchThreadEvents;
    bool processExited;

    DebugEventInfo pendingDebugEvents[0x10], latestDebugEvent;
    u32 numPendingDebugEvents;

    char buffer[GDB_BUF_LEN + 4];
    char *commandData;
} GDBContext;

typedef int (*GDBCommandHandler)(GDBContext *ctx);

void GDB_InitializeContext(GDBContext *ctx);
void GDB_FinalizeContext(GDBContext *ctx);
GDB_DECLARE_HANDLER(Unsupported);
