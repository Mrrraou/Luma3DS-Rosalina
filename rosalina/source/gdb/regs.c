#include "gdb/regs.h"
#include "gdb/net.h"

GDB_DECLARE_HANDLER(ReadRegisters)
{
    if(ctx->selectedThreadId == 0)
        return GDB_ReplyErrno(ctx, EPERM);

    ThreadContext regs;
    Result r = svcGetDebugThreadContext(&regs, ctx->debug, ctx->selectedThreadId, THREADCONTEXT_CONTROL_ALL);
    if(R_FAILED(r))
        return GDB_ReplyErrno(ctx, EPERM);

    return GDB_SendHexPacket(ctx, &regs, sizeof(ThreadContext));
}

GDB_DECLARE_HANDLER(WriteRegisters)
{
    if(ctx->selectedThreadId == 0)
        return GDB_ReplyErrno(ctx, EPERM);

    ThreadContext regs;

    if(GDB_DecodeHex(&regs, ctx->commandData, sizeof(ThreadContext)) != sizeof(ThreadContext))
        return GDB_ReplyErrno(ctx, EPERM);

    Result r = svcSetDebugThreadContext(ctx->debug, ctx->selectedThreadId, &regs, THREADCONTEXT_CONTROL_ALL);
    if(R_FAILED(r))
        return GDB_ReplyErrno(ctx, EPERM);
    else
        return GDB_ReplyOk(ctx);
}

static u32 GDB_ConvertRegisterNumber(ThreadContextControlFlags *flags, u32 gdbNum)
{
    if(gdbNum <= 15)
    {
        *flags = (gdbNum >= 13) ? THREADCONTEXT_CONTROL_CPU_SPRS : THREADCONTEXT_CONTROL_CPU_GPRS;
        return gdbNum;
    }
    else if(gdbNum == 25)
    {
        *flags = THREADCONTEXT_CONTROL_CPU_SPRS;
        return 16;
    }
    else if(gdbNum >= 26 && gdbNum <= 57)
    {
        *flags = THREADCONTEXT_CONTROL_FPU_GPRS;
        return gdbNum - 26;
    }
    else if(gdbNum == 58 || gdbNum == 59)
    {
        *flags = THREADCONTEXT_CONTROL_FPU_SPRS;
        return gdbNum - 58;
    }
    else
    {
        *flags = (ThreadContextControlFlags)0;
        return 0;
    }
}

GDB_DECLARE_HANDLER(ReadRegister)
{
    if(ctx->selectedThreadId == 0)
        return GDB_ReplyErrno(ctx, EPERM);

    ThreadContext regs;
    ThreadContextControlFlags flags;

    u32 n = GDB_ConvertRegisterNumber(&flags, (u32)atoi_(ctx->commandData, 16));
    if(!flags)
        return GDB_ReplyErrno(ctx, EPERM);

    Result r = svcGetDebugThreadContext(&regs, ctx->debug, ctx->selectedThreadId, flags);

    if(R_FAILED(r))
        return GDB_ReplyErrno(ctx, EPERM);

    if(flags & THREADCONTEXT_CONTROL_CPU_GPRS)
        return GDB_SendHexPacket(ctx, &regs.cpu_registers.r[n], 4);
    else if(flags & THREADCONTEXT_CONTROL_CPU_SPRS)
        return GDB_SendHexPacket(ctx, &regs.cpu_registers.sp + (n - 13), 4); // hacky
    else if(flags & THREADCONTEXT_CONTROL_CPU_GPRS)
        return GDB_SendHexPacket(ctx, &regs.fpu_registers.s[n], 4);
    else
        return GDB_SendHexPacket(ctx, &regs.fpu_registers.fpscr + n, 4); // hacky
}

GDB_DECLARE_HANDLER(WriteRegister)
{
    if(ctx->selectedThreadId == 0)
        return GDB_ReplyErrno(ctx, EPERM);

    ThreadContext regs;
    ThreadContextControlFlags flags;
    char *valueStart;

    for(valueStart = ctx->commandData; *valueStart != '='; valueStart++)
    {
        if(*valueStart == 0)
            return GDB_ReplyErrno(ctx, EPERM);
    }
    *valueStart++ = 0;

    u32 n = GDB_ConvertRegisterNumber(&flags, (u32)atoi_(ctx->commandData, 16));
    u32 value = (u32)atoi_(valueStart, 16);

    if(!flags)
        return GDB_ReplyErrno(ctx, EPERM);

    Result r = svcGetDebugThreadContext(&regs, ctx->debug, ctx->selectedThreadId, flags);

    if(R_FAILED(r))
        return GDB_ReplyErrno(ctx, EPERM);

    if(flags & THREADCONTEXT_CONTROL_CPU_GPRS)
        regs.cpu_registers.r[n] = value;
    else if(flags & THREADCONTEXT_CONTROL_CPU_SPRS)
        *(&regs.cpu_registers.sp + (n - 13)) = value; // hacky
    else if(flags & THREADCONTEXT_CONTROL_FPU_GPRS)
        memcpy(&regs.fpu_registers.s[n], &value, 4);
    else
        *(&regs.fpu_registers.fpscr + n) = value; // hacky

    r = svcSetDebugThreadContext(ctx->debug, ctx->selectedThreadId, &regs, flags);
    if(R_FAILED(r))
        return GDB_ReplyErrno(ctx, EPERM);
    else
        return GDB_ReplyOk(ctx);
}
