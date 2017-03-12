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
        svcOpenThread(&thread, scheduledThreadIds[i], ctx->process);
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
        u32 id = atoi_(ctx->commandData + 1, 16);
        ctx->selectedThreadId = id;
        return GDB_ReplyOk(ctx);
    }
    else if(ctx->commandData[0] == 'c')
        return GDB_ReplyOk(ctx); // ignore (because we only support all-stop mode)
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
    }

    return GDB_SendFormattedPacket(ctx, "%x", ctx->currentThreadId);
}

GDB_DECLARE_QUERY_HANDLER(FThreadInfo)
{
    u32 threadIds[0x7F];
    s32 nbThreads;

    char buf[GDB_BUF_LEN + 1] = {'m'};

    Result r = svcGetThreadList(&nbThreads, threadIds, 0x7F, ctx->process);
    if(R_FAILED(r))
        return GDB_SendPacket(ctx, "l", 1);

    if(nbThreads == 0x7F) __asm__ volatile("bkpt 1");
    char *bufptr = buf + 1;

    for(s32 i = 0; i < nbThreads && bufptr < buf + GDB_BUF_LEN - 9; i++)
    {
        *bufptr++ = ',';
        bufptr += sprintf(bufptr, "%x", threadIds[i]);
    }

    return GDB_SendPacket(ctx, buf, bufptr - buf);
}

GDB_DECLARE_QUERY_HANDLER(SThreadInfo)
{
    return GDB_SendPacket(ctx, "l", 1);
}
