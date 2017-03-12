#include "gdb/long_command.h"
#include "gdb/net.h"

typedef struct GDBLongCommandHandler
{
    const char *name;
    GDBCommandHandler handler;
} GDBLongCommandHandler;

static GDBLongCommandHandler longCommandHandlers[] =
{
    {"MustReplyEmpty", GDB_HandleUnsupported}
};

GDB_DECLARE_HANDLER(LongCommand)
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

    for(u32 i = 0; i < sizeof(longCommandHandlers) / sizeof(GDBLongCommandHandler); i++)
    {
        if(strncmp(longCommandHandlers[i].name, nameBegin, strlen(longCommandHandlers[i].name)) == 0)
        {
            ctx->commandData = vData;
            return longCommandHandlers[i].handler(ctx);
        }
    }

    return GDB_HandleUnsupported(ctx); // No handler found!
}
