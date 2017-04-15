#pragma once

#include <3ds/types.h>
#include <3ds/svc.h>
#include <3ds/synchronization.h>
#include <3ds/result.h>
#include "sock_util.h"
#include "memory.h"

#define MAX_DEBUG           3
#define MAX_DEBUG_THREAD    127
#define MAX_BREAKPOINT      256
// 512+24 is the ideal size as IDA will try to read exactly 0x100 bytes at a time. Add 4 to this, for $#<checksum>, see below.
// IDA seems to want additional bytes as well.
// 1024 is fine enough to put all regs in the 'T' stop reply packets
#define GDB_BUF_LEN 1024

#define GDB_HANDLER(name)           GDB_Handle##name
#define GDB_QUERY_HANDLER(name)     GDB_HANDLER(Query##name)
#define GDB_VERBOSE_HANDLER(name)   GDB_HANDLER(Verbose##name)

#define GDB_DECLARE_HANDLER(name)           int GDB_HANDLER(name)(GDBContext *ctx)
#define GDB_DECLARE_QUERY_HANDLER(name)     GDB_DECLARE_HANDLER(Query##name)
#define GDB_DECLARE_VERBOSE_HANDLER(name)   GDB_DECLARE_HANDLER(Verbose##name)

typedef struct Breakpoint
{
    u32 address;
    u32 savedInstruction;
    u8 instructionSize;
    bool persistent;
} Breakpoint;

typedef enum GDBFlags
{
    GDB_FLAG_SELECTED = 1,
    GDB_FLAG_USED  = 2,
    GDB_FLAG_PROCESS_CONTINUING = 4,
    GDB_FLAG_TERMINATE_PROCESS = 8,
} GDBFlags;

typedef enum GDBState
{
    GDB_STATE_DISCONNECTED,
    GDB_STATE_CONNECTED,
    GDB_STATE_NOACK_SENT,
    GDB_STATE_NOACK,
    GDB_STATE_CLOSING
} GDBState;

typedef struct ThreadInfo
{
    u32 id;
    u32 tls;
} ThreadInfo;

typedef struct GDBContext
{
    sock_ctx super;

    RecursiveLock lock;
    GDBFlags flags;
    GDBState state;

    u32 pid;
    Handle debug;
    ThreadInfo threadInfos[MAX_DEBUG_THREAD];
    u32 nbThreads;

    u32 previousThreadId, currentThreadId, selectedThreadId, selectedThreadIdForContinuing;

    Handle clientAcceptedEvent, continuedEvent;
    Handle eventToWaitFor;

    bool catchThreadEvents;
    bool processEnded, processExited;

    DebugEventInfo latestDebugEvent;
    DebugFlags continueFlags;
    u32 svcMask[8];

    u32 nbBreakpoints;
    Breakpoint breakpoints[MAX_BREAKPOINT];

    u32 nbWatchpoints;
    u32 watchpoints[2];

    bool isGDB;
    char *commandData;
    int latestSentPacketSize;
    char buffer[GDB_BUF_LEN + 4];
} GDBContext;

typedef int (*GDBCommandHandler)(GDBContext *ctx);

void GDB_InitializeContext(GDBContext *ctx);
void GDB_FinalizeContext(GDBContext *ctx);
GDB_DECLARE_HANDLER(Unsupported);
