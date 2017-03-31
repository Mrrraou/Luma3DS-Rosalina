#include "gdb/thread.h"
#include "gdb/net.h"
#include "fmt.h"

// There's a grand maximum of 0x7F threads debugged for all debugged processes
// but more than 14 debugged threads per process involve undefined behavior

// This allows us to always respond 'l' (end of list) in 'qsThreadInfo' queries, though

void GDB_UpdateCurrentThreadFromList(GDBContext *ctx, u32 *threadIds, u32 nbThreads)
{
    u32 scheduledThreadIds[0x7F];
    s32 nbScheduledThreads = 0;
    s64 dummy;
    for(u32 i = 0; i < nbThreads; i++)
    {
        u32 mask = 0;
        svcGetDebugThreadParam(&dummy, &mask, ctx->debug, threadIds[i], DBGTHREAD_PARAMETER_SCHEDULING_MASK_LOW);
        if(mask == 1)
            scheduledThreadIds[nbScheduledThreads++] = threadIds[i];
    }

    s32 maxDynaPrio = 64;
    for(s32 i = 0; i < nbScheduledThreads; i++)
    {
        Handle thread;
        s32 dynaPrio = 64;
        svcOpenThread(&thread, ctx->process, scheduledThreadIds[i]);
        svcGetThreadPriority(&dynaPrio, thread);
        svcCloseHandle(thread);
        if(dynaPrio < maxDynaPrio)
        {
            maxDynaPrio = dynaPrio;
            ctx->currentThreadId = scheduledThreadIds[i];
        }
    }
}

GDB_DECLARE_HANDLER(SetThreadId)
{
    if(ctx->commandData[0] == 'g')
    {
        if(strncmp(ctx->commandData + 1, "-1", 2) == 0)
            return GDB_ReplyErrno(ctx, EPERM); // a thread must be specified

        u32 id = atoi_(ctx->commandData + 1, 16);
        ctx->selectedThreadId = id;
        return GDB_ReplyOk(ctx);
    }
    else if(ctx->commandData[0] == 'c')
    {
        // We can't stop/continue particular threads (uncompliant behavior)
        if(strncmp(ctx->commandData + 1, "-1", 2) == 0)
            ctx->selectedThreadIdForContinuing = 0;

        u32 id = atoi_(ctx->commandData + 1, 16);
        ctx->selectedThreadIdForContinuing = id;

        return GDB_ReplyOk(ctx);
    }
    else
        return GDB_ReplyErrno(ctx, EPERM);
}

GDB_DECLARE_HANDLER(IsThreadAlive)
{
    u32 threadId = (u32)atoi_(ctx->commandData, 16);
    s64 dummy;
    u32 mask;

    Result r = svcGetDebugThreadParam(&dummy, &mask, ctx->debug, threadId, DBGTHREAD_PARAMETER_SCHEDULING_MASK_LOW);
    if(R_SUCCEEDED(r) && mask != 2)
        return GDB_ReplyOk(ctx);
    else
        return GDB_ReplyErrno(ctx, EPERM);
}

GDB_DECLARE_QUERY_HANDLER(CurrentThreadId)
{
    if(ctx->currentThreadId == 0)
    {
        u32 threadIds[0x7F];
        s32 nbThreads;
        Result r = svcGetThreadList(&nbThreads, threadIds, 0x7F, ctx->process);

        if(R_FAILED(r))
            return GDB_ReplyErrno(ctx, EPERM);

        GDB_UpdateCurrentThreadFromList(ctx, threadIds, nbThreads);
        if(ctx->currentThreadId == 0)
            ctx->currentThreadId = threadIds[0];
    }

    return GDB_SendFormattedPacket(ctx, "QC%x", ctx->currentThreadId);
}

GDB_DECLARE_QUERY_HANDLER(FThreadInfo)
{
    u32 threadIds[0x7F], aliveThreadIds[0x7F];
    s32 nbThreads, nbAliveThreads = 0;

    char buf[GDB_BUF_LEN + 1];

    Result r = svcGetThreadList(&nbThreads, threadIds, 0x7F, ctx->process);
    if(R_FAILED(r))
        return GDB_SendPacket(ctx, "l", 1);

    for(s32 i = 0; i < nbThreads; i++)
    {
        s64 dummy;
        u32 mask;

        Result r2 = svcGetDebugThreadParam(&dummy, &mask, ctx->debug, threadIds[i], DBGTHREAD_PARAMETER_SCHEDULING_MASK_LOW);
        if(R_SUCCEEDED(r2) && mask != 2)
            aliveThreadIds[nbAliveThreads++] = threadIds[i];
    }

    if(nbAliveThreads == 0)
        return GDB_SendPacket(ctx, "l", 1);

    char *bufptr = buf;

    for(s32 i = 0; i < nbAliveThreads && bufptr < buf + GDB_BUF_LEN - 9; i++)
    {
        *bufptr++ = ',';
        bufptr += sprintf(bufptr, "%x", aliveThreadIds[i]);
    }
    buf[0] = 'm';

    return GDB_SendPacket(ctx, buf, bufptr - buf);
}

GDB_DECLARE_QUERY_HANDLER(SThreadInfo)
{
    return GDB_SendPacket(ctx, "l", 1);
}

GDB_DECLARE_QUERY_HANDLER(ThreadEvents)
{
    switch(ctx->commandData[0])
    {
        case '0':
            ctx->catchThreadEvents = false;
            return GDB_ReplyOk(ctx);
        case '1':
            ctx->catchThreadEvents = true;
            return GDB_ReplyOk(ctx);
        default:
            return GDB_ReplyErrno(ctx, EPERM);
    }
}
