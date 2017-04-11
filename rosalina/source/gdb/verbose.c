#include "gdb/verbose.h"
#include "gdb/net.h"
#include "gdb/debug.h"

static const struct
{
    const char *name;
    GDBCommandHandler handler;
} gdbVerboseCommandHandlers[] =
{
    { "Cont?", GDB_VERBOSE_HANDLER(ContinueSupported) },
    { "Cont",  GDB_VERBOSE_HANDLER(Continue) },
    { "MustReplyEmpty", GDB_HANDLER(Unsupported) },
};

GDB_DECLARE_HANDLER(VerboseCommand)
{
    char *nameBegin = ctx->commandData; // w/o leading 'v'
    if(*nameBegin == 0)
        return GDB_ReplyErrno(ctx, EILSEQ);

    char *nameEnd;
    char *vData = NULL;

    for(nameEnd = nameBegin; *nameEnd != 0 && *nameEnd != ';' && *nameEnd != ':'; nameEnd++);
    if(*nameEnd != 0)
    {
        *nameEnd = 0;
        vData = nameEnd + 1;
    }

    for(u32 i = 0; i < sizeof(gdbVerboseCommandHandlers) / sizeof(gdbVerboseCommandHandlers[0]); i++)
    {
        if(strcmp(gdbVerboseCommandHandlers[i].name, nameBegin) == 0)
        {
            ctx->commandData = vData;
            return gdbVerboseCommandHandlers[i].handler(ctx);
        }
    }

    return GDB_HandleUnsupported(ctx); // No handler found!
}

GDB_DECLARE_VERBOSE_HANDLER(ContinueSupported)
{
    return GDB_SendPacket(ctx, "vCont;c;C", 9);
}
