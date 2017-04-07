#include "gdb/thread.h"
#include "gdb/net.h"
#include "fmt.h"
#include <stdlib.h>

// There's a grand maximum of 0x7F threads debugged for all debugged processes
// but more than 14 debugged threads per process involve undefined behavior

// This allows us to always respond 'l' (end of list) in 'qsThreadInfo' queries, though

struct ThreadIdWithDbg
{
    Handle debug;
    u32 id;
};

static int compare_func(const void *a, const void *b)
{
    u32 maskA = 2, maskB = 2;
    u32 prioA = 64, prioB = 64;
    s64 dummy;
    const struct ThreadIdWithDbg *a_ = (const struct ThreadIdWithDbg *)a;
    const struct ThreadIdWithDbg *b_ = (const struct ThreadIdWithDbg *)b;

    svcGetDebugThreadParam(&dummy, &maskA, a_->debug, a_->id, DBGTHREAD_PARAMETER_SCHEDULING_MASK_LOW);
    svcGetDebugThreadParam(&dummy, &maskB, b_->debug, b_->id, DBGTHREAD_PARAMETER_SCHEDULING_MASK_LOW);
    svcGetDebugThreadParam(&dummy, &prioA, a_->debug, a_->id, DBGTHREAD_PARAMETER_PRIORITY);
    svcGetDebugThreadParam(&dummy, &prioB, b_->debug, b_->id, DBGTHREAD_PARAMETER_PRIORITY);

    if(maskA == 1 && maskB != 1)
        return 1;
    else if(maskA != 1 && maskB == 1)
        return -1;
    else
        return (int)(prioB - prioA);
}

void GDB_UpdateCurrentThreadFromList(GDBContext *ctx, u32 *threadIds, u32 nbThreads)
{
    struct ThreadIdWithDbg lst[MAX_DEBUG_THREAD];
    for(u32 i = 0; i < nbThreads; i++)
    {
        lst[i].debug = ctx->debug;
        lst[i].id = threadIds[i];
    }
    qsort(lst, nbThreads, sizeof(struct ThreadIdWithDbg), compare_func);
    ctx->currentThreadId = lst[0].id;
}

GDB_DECLARE_HANDLER(SetThreadId)
{
    if(ctx->commandData[0] == 'g')
    {
        if(strncmp(ctx->commandData + 1, "-1", 2) == 0)
            return GDB_ReplyErrno(ctx, EILSEQ); // a thread must be specified

        u32 id;
        if(GDB_ParseHexIntegerList(&id, ctx->commandData + 1, 1, 0) == NULL)
            return GDB_ReplyErrno(ctx, EILSEQ);
        ctx->selectedThreadId = id;
        return GDB_ReplyOk(ctx);
    }
    else if(ctx->commandData[0] == 'c')
    {
        // We can't stop/continue particular threads (uncompliant behavior)
        if(strncmp(ctx->commandData + 1, "-1", 2) == 0)
            ctx->selectedThreadIdForContinuing = 0;
        else
        {
            u32 id;
            if(GDB_ParseHexIntegerList(&id, ctx->commandData + 1, 1, 0) == NULL)
                return GDB_ReplyErrno(ctx, EILSEQ);
            ctx->selectedThreadIdForContinuing = id;
        }

        return GDB_ReplyOk(ctx);
    }
    else
        return GDB_ReplyErrno(ctx, EPERM);
}

GDB_DECLARE_HANDLER(IsThreadAlive)
{
    u32 threadId;
    s64 dummy;
    u32 mask;

    if(GDB_ParseHexIntegerList(&threadId, ctx->commandData, 1, 0) == NULL)
        return GDB_ReplyErrno(ctx, EILSEQ);

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
        u32 threadIds[MAX_DEBUG_THREAD];

        if(ctx->nbThreads == 0)
            return GDB_ReplyErrno(ctx, EPERM);

        for(u32 i = 0; i < ctx->nbThreads; i++)
            threadIds[i] = ctx->threadInfos[i].id;

        GDB_UpdateCurrentThreadFromList(ctx, threadIds, ctx->nbThreads);
        if(ctx->currentThreadId == 0)
            ctx->currentThreadId = threadIds[0];
    }

    return GDB_SendFormattedPacket(ctx, "QC%x", ctx->currentThreadId);
}

GDB_DECLARE_QUERY_HANDLER(FThreadInfo)
{
    u32 aliveThreadIds[MAX_DEBUG_THREAD];
    u32 nbAliveThreads = 0; // just in case. This is probably redundant
    char buf[GDB_BUF_LEN + 1];

    for(u32 i = 0; i < ctx->nbThreads; i++)
    {
        s64 dummy;
        u32 mask;

        Result r = svcGetDebugThreadParam(&dummy, &mask, ctx->debug, ctx->threadInfos[i].id, DBGTHREAD_PARAMETER_SCHEDULING_MASK_LOW);
        if(R_SUCCEEDED(r) && mask != 2)
            aliveThreadIds[nbAliveThreads++] = ctx->threadInfos[i].id;
    }

    if(nbAliveThreads == 0)
        return GDB_SendPacket(ctx, "l", 1);

    char *bufptr = buf;

    for(u32 i = 0; i < nbAliveThreads && bufptr < buf + GDB_BUF_LEN - 9; i++)
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
