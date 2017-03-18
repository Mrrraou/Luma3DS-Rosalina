#include "gdb/verbose.h"
#include "gdb/net.h"
#include "gdb/debug.h"

typedef struct GDBVerboseCommandHandler
{
    const char *name;
    GDBCommandHandler handler;
} GDBVerboseCommandHandler;

static GDBVerboseCommandHandler verboseCommandHandlers[] =
{
    {"Cont?", GDB_HandleVerboseContinueSupported},
    {"Cont", GDB_HandleVerboseContinue},
    {"MustReplyEmpty", GDB_HandleUnsupported},
};

GDB_DECLARE_HANDLER(VerboseCommand)
{
    char *nameBegin = ctx->commandData; // w/o leading 'v'
    if(*nameBegin == 0)
        return -1;

    char *nameEnd;
    char *vData = NULL;

    for(nameEnd = nameBegin; *nameEnd != 0 && *nameEnd != ';' && *nameEnd != ':'; nameEnd++);
    if(*nameEnd != 0)
    {
        *nameEnd = 0;
        vData = nameEnd + 1;
    }

    for(u32 i = 0; i < sizeof(verboseCommandHandlers) / sizeof(GDBVerboseCommandHandler); i++)
    {
        if(strncmp(verboseCommandHandlers[i].name, nameBegin, strlen(verboseCommandHandlers[i].name)) == 0)
        {
            ctx->commandData = vData;
            return verboseCommandHandlers[i].handler(ctx);
        }
    }

    return GDB_HandleUnsupported(ctx); // No handler found!
}

GDB_DECLARE_VERBOSE_HANDLER(ContinueSupported)
{
    return GDB_SendPacket(ctx, "c", 1);
}

GDB_DECLARE_VERBOSE_HANDLER(Continue)
{
    if(ctx->commandData[0] == 'c' || ctx->commandData[0] == 'C')
        return GDB_HandleContinue(ctx);
    else
        return GDB_ReplyErrno(ctx, EPERM);
}
