#include "gdb/debug.h"
#include "gdb/net.h"
#include "gdb/thread.h"
#include "gdb/mem.h"
#include "gdb/watchpoints.h"
#include "fmt.h"

GDB_DECLARE_HANDLER(Detach)
{
    ctx->state = GDB_STATE_CLOSING;
    return GDB_ReplyOk(ctx);
}

GDB_DECLARE_HANDLER(Kill)
{
    ctx->state = GDB_STATE_CLOSING;
    ctx->flags |= GDB_FLAG_TERMINATE_PROCESS;
    return 0;
}

GDB_DECLARE_HANDLER(Break)
{
    ctx->flags &= ~GDB_FLAG_PROCESS_CONTINUING;
    return 0;
}

GDB_DECLARE_HANDLER(Continue)
{
    char *addrStart;
    u32 addr = 0;

    if(ctx->commandData[-1] == 'C' && ctx->commandData[-2] == '$')
    {
        for(addrStart = ctx->commandData; *addrStart != ';' && *addrStart != 0; addrStart++);
        addrStart++;
    }
    else if(ctx->commandData[-2] == '$')
        addrStart = ctx->commandData;
    else
    {
        // Note: we can't wake up individual threads. This is uncompliant behavior
        addrStart = NULL;
    }

    if(addrStart != NULL && *addrStart != 0  && ctx->currentThreadId != 0)
    {
        ThreadContext regs;
        addr = (u32)atoi_(++addrStart, 16);
        Result r = svcGetDebugThreadContext(&regs, ctx->debug, ctx->currentThreadId, THREADCONTEXT_CONTROL_CPU_SPRS);
        if(R_SUCCEEDED(r))
        {
            regs.cpu_registers.pc = addr;
            r = svcSetDebugThreadContext(ctx->debug, ctx->currentThreadId, &regs, THREADCONTEXT_CONTROL_CPU_SPRS);
        }
    }

    int ret = 0;
    if(ctx->nbPendingDebugEvents > 0)
    {
        ret = GDB_SendStopReply(ctx, &ctx->pendingDebugEvents[0]);
        ctx->latestDebugEvent = ctx->pendingDebugEvents[0];
        for(u32 i = 0; i < ctx->nbPendingDebugEvents; i++)
            ctx->pendingDebugEvents[i] = ctx->pendingDebugEvents[i + 1];
        --ctx->nbPendingDebugEvents;
    }

    else
    {
        svcContinueDebugEvent(ctx->debug, ctx->continueFlags);
        ctx->flags |= GDB_FLAG_PROCESS_CONTINUING;
        ctx->currentThreadId = 0;
    }

    return ret;
}

GDB_DECLARE_HANDLER(GetStopReason)
{
    return GDB_SendStopReply(ctx, &ctx->latestDebugEvent);
}

static int GDB_ParseCommonThreadInfo(char *out, GDBContext *ctx, bool isUndefInstr)
{
    u32 threadId = ctx->currentThreadId;
    ThreadContext regs;
    s64 dummy;
    u32 core;
    Result r = svcGetDebugThreadContext(&regs, ctx->debug, threadId, THREADCONTEXT_CONTROL_CPU_SPRS);

    if(R_FAILED(r))
        return sprintf(out, "thread:%x", threadId);

    if(isUndefInstr && (regs.cpu_registers.cpsr & 0x20) != 0)
    {
        // kernel bugfix for thumb mode
        regs.cpu_registers.pc += 2;
        r = svcSetDebugThreadContext(ctx->debug, threadId, &regs, THREADCONTEXT_CONTROL_CPU_SPRS);
    }

    r = svcGetDebugThreadParam(&dummy, &core, ctx->debug, ctx->currentThreadId, DBGTHREAD_PARAMETER_CPU_IDEAL);

    if(R_FAILED(r))
        return sprintf(out, "thread:%x;d:%08x;e:%08x;f:%08x;19:%08x", threadId,
                       __builtin_bswap32(regs.cpu_registers.sp), __builtin_bswap32(regs.cpu_registers.lr), __builtin_bswap32(regs.cpu_registers.pc),
                       __builtin_bswap32(regs.cpu_registers.cpsr));
    else
        return sprintf(out, "thread:%x;core:%x;d:%08x;e:%08x;f:%08x;19:%08x", threadId, core,
        __builtin_bswap32(regs.cpu_registers.sp), __builtin_bswap32(regs.cpu_registers.lr), __builtin_bswap32(regs.cpu_registers.pc),
        __builtin_bswap32(regs.cpu_registers.cpsr));
}

int GDB_SendStopReply(GDBContext *ctx, const DebugEventInfo *info)
{
    char buffer[GDB_BUF_LEN + 1];

    switch(info->type)
    {
        case DBGEVENT_ATTACH_PROCESS:
            break; // Dismissed

        case DBGEVENT_ATTACH_THREAD:
        {
            if(info->attach_thread.creator_thread_id == 0 || !ctx->catchThreadEvents)
                break; // Dismissed

            return GDB_SendPacket(ctx, "T05create:;", 10);
        }

        case DBGEVENT_EXIT_THREAD:
        {
            if(info->exit_thread.reason >= EXITTHREAD_EVENT_EXIT_PROCESS)
            {
                ctx->processEnded = true;
                ctx->processExited = info->exit_thread.reason == EXITTHREAD_EVENT_EXIT_PROCESS;
            }

            if(!ctx->catchThreadEvents)
                break;

            // no signal, SIGTERM, SIGQUIT (process exited), SIGTERM (process terminated)
            static const char *threadExitRepliesPrefix[] = {"w00;", "w0f;", "w03;", "w0f;"};
            return GDB_SendFormattedPacket(ctx, "%s%x", threadExitRepliesPrefix[(u32)info->exit_thread.reason], info->thread_id);
        }

        case DBGEVENT_EXIT_PROCESS:
        {
            // This code is unreachable. DBGEVENT_EXIT_PROCESS is actually only sent on process __deletion__

            // exited (no error / unhandled exception), SIGTERM (process terminated) * 2
            static const char *processExitReplies[] = {"W00", "X0f", "X0f"};
            ctx->processEnded = true;
            ctx->processExited = info->exit_process.reason == EXITPROCESS_EVENT_EXIT;
            return GDB_SendPacket(ctx, processExitReplies[(u32)info->exit_process.reason], 3);
        }

        case DBGEVENT_EXCEPTION:
        {
            ExceptionEvent exc = info->exception;
            switch(exc.type)
            {
                case EXCEVENT_UNDEFINED_INSTRUCTION:
                case EXCEVENT_PREFETCH_ABORT: // doesn't include breakpoints
                case EXCEVENT_DATA_ABORT:     // doesn't include watchpoints
                case EXCEVENT_UNALIGNED_DATA_ACCESS:
                case EXCEVENT_UNDEFINED_SYSCALL:
                {
                    u32 signum = exc.type == EXCEVENT_UNDEFINED_INSTRUCTION ? 4 : // SIGILL
                                (exc.type == EXCEVENT_UNDEFINED_SYSCALL ? 12 : 11); // SIGSYS, SIGSEGV

                    ctx->currentThreadId = info->thread_id;
                    GDB_ParseCommonThreadInfo(buffer, ctx, exc.type == EXCEVENT_UNDEFINED_INSTRUCTION);
                    return GDB_SendFormattedPacket(ctx, "T%02x%s;", signum, buffer);
                }

                case EXCEVENT_ATTACH_BREAK:
                    return GDB_SendPacket(ctx, "S02", 3);

                case EXCEVENT_STOP_POINT:
                {
                    ctx->currentThreadId = info->thread_id;

                    switch(exc.stop_point.type)
                    {
                        case STOPPOINT_SVC_FF:
                        {
                            GDB_ParseCommonThreadInfo(buffer, ctx, false);
                            return GDB_SendFormattedPacket(ctx, "T05%s;swbreak:;", buffer);
                            break;
                        }

                        case STOPPOINT_BREAKPOINT:
                        {
                            // /!\ Not actually implemented (and will never be)
                            GDB_ParseCommonThreadInfo(buffer, ctx, false);
                            return GDB_SendFormattedPacket(ctx, "T05%s;hwbreak:;", buffer);
                            break;
                        }

                        case STOPPOINT_WATCHPOINT:
                        {
                            const char *kinds = "arwa";
                            WatchpointKind kind = GDB_GetWatchpointKind(ctx, exc.stop_point.fault_information);
                            if(kind == WATCHPOINT_DISABLED)
                                GDB_SendDebugString(ctx, "Warning: unknown watchpoint encountered!\n");

                            GDB_ParseCommonThreadInfo(buffer, ctx, false);
                            return GDB_SendFormattedPacket(ctx, "T05%cwatch:%08x;%s;", kinds[(u32)kind], exc.stop_point.fault_information, buffer);
                            break;
                        }

                        default:
                            break;
                    }
                }

                case EXCEVENT_USER_BREAK:
                {
                    ctx->currentThreadId = info->thread_id;
                    GDB_ParseCommonThreadInfo(buffer, ctx, false);
                    return GDB_SendFormattedPacket(ctx, "T02%s;", buffer); // SIGINT
                    //TODO
                }

                case EXCEVENT_DEBUGGER_BREAK:
                {
                    u32 threadIds[4];
                    u32 nbThreads = 0;

                    s32 idealProcessor;
                    Result r = svcGetProcessIdealProcessor(&idealProcessor, ctx->process);
                    if(R_SUCCEEDED(r) && exc.debugger_break.thread_ids[idealProcessor] > 0)
                        ctx->currentThreadId = (u32) exc.debugger_break.thread_ids[idealProcessor];
                    else
                    {
                        for(u32 i = 0; i < 4; i++)
                        {
                            if(exc.debugger_break.thread_ids[i] > 0)
                                threadIds[nbThreads++] = (u32)exc.debugger_break.thread_ids[i];
                        }
                        if(nbThreads > 0)
                            GDB_UpdateCurrentThreadFromList(ctx, threadIds, nbThreads);
                    }

                    if(ctx->currentThreadId == 0)
                        return GDB_SendPacket(ctx, "S02", 3);
                    else
                    {
                        GDB_ParseCommonThreadInfo(buffer, ctx, false);
                        return GDB_SendFormattedPacket(ctx, "T02%s;", buffer);
                    }
                }

                default:
                    break;
            }
        }

        case DBGEVENT_SYSCALL_IN:
        {
            ctx->currentThreadId = info->thread_id;
            GDB_ParseCommonThreadInfo(buffer, ctx, false);
            return GDB_SendFormattedPacket(ctx, "T05syscall_entry:%02x;%s;", info->syscall.syscall, buffer);
        }

        case DBGEVENT_SYSCALL_OUT:
        {
            ctx->currentThreadId = info->thread_id;
            GDB_ParseCommonThreadInfo(buffer, ctx, false);
            return GDB_SendFormattedPacket(ctx, "T05syscall_return:%02x;%s;", info->syscall.syscall, buffer);
        }

        case DBGEVENT_OUTPUT_STRING:
        {
            u32 addr = info->output_string.string_addr;
            u32 remaining = info->output_string.string_size;
            u32 sent = 0;
            int total = 0;
            while(remaining > 0)
            {
                u32 pending = (GDB_BUF_LEN - 1) / 2;
                pending = pending > remaining ? pending : remaining;

                int res = GDB_SendProcessMemory(ctx, "O", 1, addr + sent, pending);
                if(res < 0 || (u32) res != 5 + 2 * pending)
                    break;

                sent += pending;
                remaining -= pending;
                total += res;
            }

            return total;
        }
        default:
            break;
    }

    return 0;
}

int GDB_HandleDebugEvents(GDBContext *ctx)
{
    if(ctx->flags & GDB_FLAG_TERMINATE_PROCESS)
        return 0;

    DebugEventInfo info;
    Result rdbg = svcGetProcessDebugEvent(&info, ctx->debug);

    if(R_FAILED(rdbg))
        return -1;

    int ret = 0;

    bool dontBreak = info.type == DBGEVENT_OUTPUT_STRING || info.type == DBGEVENT_ATTACH_PROCESS ||
                    (info.type == DBGEVENT_ATTACH_THREAD && (info.attach_thread.creator_thread_id == 0 || !ctx->catchThreadEvents)) ||
                    (info.type == DBGEVENT_EXIT_THREAD && (info.exit_thread.reason >= EXITTHREAD_EVENT_EXIT_PROCESS || !ctx->catchThreadEvents)) ||
                     info.type == DBGEVENT_EXIT_PROCESS;


    if(dontBreak)
    {
        ret = GDB_SendStopReply(ctx, &info);
        if(info.type == DBGEVENT_EXIT_PROCESS || (info.flags & 1))
            svcContinueDebugEvent(ctx->debug, ctx->continueFlags);
        return -ret - 1;
    }

    else
    {
        if(ctx->processEnded)
            return 0;

        Result r = svcBreakDebugProcess(ctx->debug);
        do
        {
            if(R_FAILED(r) || info.type != DBGEVENT_EXCEPTION || info.exception.type != EXCEVENT_DEBUGGER_BREAK)
            {
                ctx->pendingDebugEvents[ctx->nbPendingDebugEvents++] = info;
                if(info.type != DBGEVENT_EXCEPTION || info.exception.type != EXCEVENT_DEBUGGER_BREAK || (info.type != DBGEVENT_EXIT_PROCESS && !(info.flags & 1)))
                    svcContinueDebugEvent(ctx->debug, ctx->continueFlags);
            }
        }
        while(R_SUCCEEDED(svcGetProcessDebugEvent(&info, ctx->debug)));

        if(ctx->nbPendingDebugEvents > 0)
        {
            ret = GDB_SendStopReply(ctx, &ctx->pendingDebugEvents[0]);
            ctx->flags &= ~GDB_FLAG_PROCESS_CONTINUING;
            ctx->latestDebugEvent = ctx->pendingDebugEvents[0];
            for(u32 i = 0; i < ctx->nbPendingDebugEvents; i++)
                ctx->pendingDebugEvents[i] = ctx->pendingDebugEvents[i + 1];
            --ctx->nbPendingDebugEvents;
        }

        return ret;
    }
}
