#include "gdb/breakpoints.h"

#define _REENT_ONLY
#include <errno.h>

// We'll actually use SVC 0xFF for breakpoints :P
#define BREAKPOINT_INSTRUCTION_ARM      0xEF0000FF
#define BREAKPOINT_INSTRUCTION_THUMB    0xDFFF

u32 GDB_FindClosestBreakpointSlot(GDBContext *ctx, u32 address)
{
    if(address <= ctx->breakpoints[0].address)
        return 0;
    else if(address > ctx->breakpoints[ctx->nbBreakpoints - 1].address)
        return ctx->nbBreakpoints;

    u32 a = 0, b = ctx->nbBreakpoints - 1, m;

    do
    {
        m = (a + b) / 2;
        if(ctx->breakpoints[m].address < address)
            a = m;
        else if(ctx->breakpoints[m].address > address)
            b = m;
        else
            return m;
    }
    while(b - a > 1);

    return b;
}

int GDB_AddBreakpoint(GDBContext *ctx, u32 address, bool thumb, bool persist)
{
    u32 id = GDB_FindClosestBreakpointSlot(ctx, address);

    if(id != ctx->nbBreakpoints && ctx->breakpoints[id].instructionSize != 0 && ctx->breakpoints[id].address == address)
        return 0;
    else if(ctx->nbBreakpoints == MAX_BREAKPOINT)
        return -EBUSY;
    else if((thumb && (address & 1) != 0) || (!thumb && (address & 3) != 0))
        return -EINVAL;

    for(u32 i = ctx->nbBreakpoints; i > id && i != 0; i--)
        ctx->breakpoints[i] = ctx->breakpoints[i - 1];

    ctx->nbBreakpoints++;

    Breakpoint *bkpt = &ctx->breakpoints[id];
    u32 instr = thumb ? BREAKPOINT_INSTRUCTION_THUMB : BREAKPOINT_INSTRUCTION_ARM;
    if(R_FAILED(svcReadProcessMemory(&bkpt->savedInstruction, ctx->debug, address, thumb ? 2 : 4)) ||
       R_FAILED(svcWriteProcessMemory(ctx->debug, &instr, address, thumb ? 2 : 4)))
    {
        for(u32 i = id; i < ctx->nbBreakpoints - 1; i++)
            ctx->breakpoints[i] = ctx->breakpoints[i + 1];

        memset(&ctx->breakpoints[--ctx->nbBreakpoints], 0, sizeof(Breakpoint));
        return -EFAULT;
    }

    bkpt->instructionSize = thumb ? 2 : 4;
    bkpt->address = address;
    bkpt->persistent = persist;

    return 0;
}

int GDB_DisableBreakpointById(GDBContext *ctx, u32 id)
{
    Breakpoint *bkpt = &ctx->breakpoints[id];
    if(R_FAILED(svcWriteProcessMemory(ctx->debug, &bkpt->savedInstruction, bkpt->address, bkpt->instructionSize)))
        return -EFAULT;
    else return 0;
}

int GDB_RemoveBreakpoint(GDBContext *ctx, u32 address)
{
    u32 id = GDB_FindClosestBreakpointSlot(ctx, address);
    if(id == ctx->nbBreakpoints || ctx->breakpoints[id].address != address)
        return -EINVAL;

    int r = GDB_DisableBreakpointById(ctx, id);
    if(r != 0)
        return r;
    else
    {
        for(u32 i = id; i < ctx->nbBreakpoints - 1; i++)
            ctx->breakpoints[i] = ctx->breakpoints[i + 1];

        memset(&ctx->breakpoints[--ctx->nbBreakpoints], 0, sizeof(Breakpoint));

        // Delete pending events accordingly
        DebugEventInfo pendingDebugEvents[0x10];
        u32 nbPendingDebugEvents = 0;
        for(u32 i = 0; i < ctx->nbPendingDebugEvents && i < 0x10; i++)
        {
            DebugEventInfo *info = &ctx->pendingDebugEvents[i];
            if(!(info->type == DBGEVENT_EXCEPTION && info->exception.type == EXCEVENT_STOP_POINT && info->exception.stop_point.type == STOPPOINT_SVC_FF &&
                info->exception.address == address))
                pendingDebugEvents[nbPendingDebugEvents++] = *info;
        }
        memcpy(ctx->pendingDebugEvents, pendingDebugEvents, nbPendingDebugEvents);
        ctx->nbPendingDebugEvents = nbPendingDebugEvents;

        return 0;
    }
}
