#include "gdb/query.h"
#include "gdb/xfer.h"
#include "gdb/thread.h"
#include "gdb/net.h"

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
    {"TStatus", GDB_HandleQueryTStatus, GDB_QUERY_DIRECTION_READ},
    {"fThreadInfo", GDB_HandleQueryFThreadInfo, GDB_QUERY_DIRECTION_READ},
    {"sThreadInfo", GDB_HandleQuerySThreadInfo, GDB_QUERY_DIRECTION_READ},
    {"C", GDB_HandleQueryCurrentThreadId, GDB_QUERY_DIRECTION_READ},
};

static int GDB_HandleQuery(GDBContext *ctx, GDBQueryDirection direction)
{
    char *nameBegin = ctx->commandData; // w/o leading 'q'/'Q'

    char *nameEnd = (char*)strchr(ctx->commandData, ':');
    char *queryData = NULL;

    if(nameEnd != NULL)
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
    const char *supp = "PacketSize=400;qXfer:features:read+;QStartNoAckMode+";
    return GDB_SendPacket(ctx, supp, strlen(supp));
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

GDB_DECLARE_QUERY_HANDLER(TStatus) // tracepoint status
{
    return GDB_SendPacket(ctx, "T0", 2);
}
