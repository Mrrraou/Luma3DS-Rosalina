#include "gdb.h"
#include "gdb/net.h"
#include "gdb/watchpoints.h"

GDB_DECLARE_HANDLER(ToggleStopPoint)
{
    if(ctx->commandData[1] == 0 || ctx->commandData[0] < '0' || ctx->commandData[0] > '4')
        return GDB_ReplyErrno(ctx, EILSEQ);

    bool add = *(ctx->commandData - 1) == 'Z';

    u32 kind = ctx->commandData[0] - '0';
    char *addrStart = ctx->commandData + 2;
    char *addrEnd = (char *)strchr(addrStart, ',');
    if(addrEnd == NULL || addrEnd[1] == 0)
        return GDB_ReplyErrno(ctx, EILSEQ);
    *addrEnd = 0;

    char *sizeStart = addrEnd + 1;
    char *sizeEnd = (char *)strchr(addrStart, ';');
    if(sizeEnd != NULL)
        *sizeEnd = 0;

    u32 addr = (u32)atoi_(addrStart, 16);
    u32 size = (u32)atoi_(sizeStart, 16);

    int res;
    static const WatchpointKind kinds[3] = { WATCHPOINT_WRITE, WATCHPOINT_READ, WATCHPOINT_READWRITE };
    switch(kind)
    {
        case 0: // software breakpoint, not supported yet (TODO)
            return GDB_ReplyEmpty(ctx);

        // Watchpoints
        case 2:
        case 3:
        case 4:
            res = add ? GDB_AddWatchpoint(ctx, addr, size, kinds[kind - 2]) :
                        GDB_RemoveWatchpoint(ctx, addr, kinds[kind - 2]);
            return res == 0 ? GDB_ReplyOk(ctx) : GDB_ReplyErrno(ctx, -res);

        default:
            return GDB_ReplyEmpty(ctx);
    }
}
