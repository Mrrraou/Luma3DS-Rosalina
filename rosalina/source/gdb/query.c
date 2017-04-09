#include "gdb/query.h"
#include "gdb/xfer.h"
#include "gdb/thread.h"
#include "gdb/net.h"
#include "minisoc.h"

typedef enum GDBQueryDirection
{
    GDB_QUERY_DIRECTION_READ,
    GDB_QUERY_DIRECTION_WRITE
} GDBQueryDirection;

typedef struct GDBQueryHandler
{
    const char *name;
    GDBCommandHandler handler;
    GDBQueryDirection direction;
} GDBQueryHandler;

static GDBQueryHandler queryHandlers[] =
{
    {"Supported", GDB_HandleQuerySupported, GDB_QUERY_DIRECTION_READ},
    {"Xfer", GDB_HandleQueryXfer, GDB_QUERY_DIRECTION_READ},
    {"StartNoAckMode", GDB_HandleQueryStartNoAckMode, GDB_QUERY_DIRECTION_WRITE},
    {"Attached", GDB_HandleQueryAttached, GDB_QUERY_DIRECTION_READ},
    {"fThreadInfo", GDB_HandleQueryFThreadInfo, GDB_QUERY_DIRECTION_READ},
    {"sThreadInfo", GDB_HandleQuerySThreadInfo, GDB_QUERY_DIRECTION_READ},
    {"ThreadEvents", GDB_HandleQueryThreadEvents, GDB_QUERY_DIRECTION_WRITE},
    {"ThreadExtraInfo", GDB_HandleQueryThreadExtraInfo, GDB_QUERY_DIRECTION_READ},
    {"C", GDB_HandleQueryCurrentThreadId, GDB_QUERY_DIRECTION_READ},
    {"CatchSyscalls", GDB_HandleQueryCatchSyscalls, GDB_QUERY_DIRECTION_WRITE},
};

static int GDB_HandleQuery(GDBContext *ctx, GDBQueryDirection direction)
{
    char *nameBegin = ctx->commandData; // w/o leading 'q'/'Q'
    if(*nameBegin == 0)
        return GDB_ReplyErrno(ctx, EILSEQ);

    char *nameEnd;
    char *queryData = NULL;

    for(nameEnd = nameBegin; *nameEnd != 0 && *nameEnd != ':' && *nameEnd != ','; nameEnd++);
    if(*nameEnd != 0)
    {
        *nameEnd = 0;
        queryData = nameEnd + 1;
    }

    for(u32 i = 0; i < sizeof(queryHandlers) / sizeof(GDBQueryHandler); i++)
    {
        if(strncmp(queryHandlers[i].name, nameBegin, strlen(queryHandlers[i].name)) == 0 && queryHandlers[i].direction == direction)
        {
            ctx->commandData = queryData;
            return queryHandlers[i].handler(ctx);
        }
    }

    return GDB_HandleUnsupported(ctx); // No handler found!
}

int GDB_HandleReadQuery(GDBContext *ctx)
{
    return GDB_HandleQuery(ctx, GDB_QUERY_DIRECTION_READ);
}

int GDB_HandleWriteQuery(GDBContext *ctx)
{
    return GDB_HandleQuery(ctx, GDB_QUERY_DIRECTION_WRITE);
}

GDB_DECLARE_QUERY_HANDLER(Supported)
{
    return GDB_SendFormattedPacket(ctx, "PacketSize=%d;qXfer:features:read+;QStartNoAckMode+;QThreadEvents+;QCatchSyscalls+;vContSupported+;swbreak+", sizeof(ctx->buffer));
}

GDB_DECLARE_QUERY_HANDLER(StartNoAckMode)
{
    ctx->state = GDB_STATE_NOACK_SENT;
    return GDB_ReplyOk(ctx);
}

GDB_DECLARE_QUERY_HANDLER(Attached)
{
    return GDB_SendPacket(ctx, "1", 1);
}

GDB_DECLARE_QUERY_HANDLER(CatchSyscalls)
{
    if(ctx->commandData[0] == '0')
    {
        memset_(ctx->svcMask, 0, 32);
        return R_SUCCEEDED(svcKernelSetState(0x10002, ctx->pid, false)) ? GDB_ReplyOk(ctx) : GDB_ReplyErrno(ctx, EPERM);
    }
    else if(ctx->commandData[0] == '1')
    {
        if(ctx->commandData[1] == ';')
        {
            u32 id;
            const char *pos = ctx->commandData + 2;
            memset_(ctx->svcMask, 0, 32);

            do
            {
                pos = GDB_ParseHexIntegerList(&id, pos, 1, ',');
                if(pos == NULL)
                    return GDB_ReplyErrno(ctx, EILSEQ);

                ctx->svcMask[id / 32] |= 1 << (31 - (id % 32));
            }
            while(*pos != 0);
        }
        else
            memset_(ctx->svcMask, 0xFF, 32);

        return R_SUCCEEDED(svcKernelSetState(0x10002, ctx->pid, true, ctx->svcMask)) ? GDB_ReplyOk(ctx) : GDB_ReplyErrno(ctx, EPERM);
    }
    else
        return GDB_ReplyErrno(ctx, EILSEQ);
}