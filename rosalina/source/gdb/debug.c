#include <3ds.h>
#include "minisoc.h"
#include "gdb_ctx.h"
#include "memory.h"
#include "macros.h"

// TODO: stub
// Right now, just sink all debug events.

u32 gdb_parse_common_thread_info(char *out, Handle debug, u32 threadId, bool isUndefInstr)
{
	char buf[] = "thread:00000000;d:00000000;e:00000000;f:00000000;19:00000000";
	hexItoa(threadId, buf + 7, 8, false);
	ThreadContext regs;
	Result r = svcGetDebugThreadContext(&regs, debug, threadId, THREADCONTEXT_CONTROL_CPU_SPRS);

	if(R_FAILED(r))
	{
		memcpy(out, buf, 16);
		return 16;
	}

	if(isUndefInstr && (regs.cpu_registers.cpsr & 0x20) != 0)
	{
		// kernel bugfix for thumb mode
		regs.cpu_registers.pc += 2;
		r = svcSetDebugThreadContext(debug, threadId, &regs, THREADCONTEXT_CONTROL_CPU_SPRS);
	}

	hexItoa(regs.cpu_registers.sp, buf + 18, 8, false);
	hexItoa(regs.cpu_registers.lr, buf + 29, 8, false);
	hexItoa(regs.cpu_registers.pc, buf + 40, 8, false);
	hexItoa(regs.cpu_registers.cpsr, buf + 52, 8, false);

	memcpy(out, buf, 60);
	return 60;
}

int gdb_send_stop_reply(Handle debug, DebugEventInfo *info, struct sock_ctx *ctx)
{
	struct gdb_client_ctx *c_ctx = (struct gdb_client_ctx *)ctx->data;
	struct gdb_server_ctx *s_ctx = (struct gdb_server_ctx *)ctx->serv->data;
	u32 len = 0;
	char buffer[256];

	switch(info->type)
	{
		case DBGEVENT_ATTACH_PROCESS:
			break; // Dismissed

		case DBGEVENT_ATTACH_THREAD:
		{
			if(info->attach_thread.creator_thread_id == 0)
				break; // Dismissed
			char str[] = "T05create:00000000";
			hexItoa(info->thread_id, str + 10, 8, false);
			memcpy(buffer, str, 18);

			len = 18;
			break;
		}

		case DBGEVENT_EXIT_THREAD:
		{
			// no signal, SIGTERM, SIGQUIT (process exited), SIGTERM (process terminated)
			static const char *threadExitRepliesPrefix[] = {"w00;", "w0f;", "w03;", "w0f;"};
			memcpy(buffer, threadExitRepliesPrefix[(u32)info->exit_thread.reason], 4);
			hexItoa(info->thread_id, buffer + 4, 8, false);
			len = 12;
			break;
		}

		case DBGEVENT_EXIT_PROCESS:
		{
			// exited (no error), SIGTERM (terminated), SIGKILL (killed due to unhandled CPU exception), SIGTERM (process terminated)
			static const char *processExitReplies[] = {"W00", "X0f", "X09"};
			memcpy(buffer, processExitReplies[(u32)info->exit_process.reason], 3);
			len = 3;
			s_ctx->processExited = true;
			break;
		}

		case DBGEVENT_EXCEPTION:
		{
			ExceptionEvent exc = info->exception;
			switch(exc.type)
			{
				case EXCEVENT_UNDEFINED_INSTRUCTION:
				{
					memcpy(buffer, "T04", 3); // SIGILL
					len = 3 + gdb_parse_common_thread_info(buffer + 3, debug, info->thread_id, true);
					break;
				}

				case EXCEVENT_PREFETCH_ABORT: // doesn't include breakpoints
				case EXCEVENT_DATA_ABORT:     // doesn't include watchpoints
				case EXCEVENT_UNALIGNED_DATA_ACCESS:
				{
					memcpy(buffer, "T0b", 3); // SIGSEGV
					len = 3 + gdb_parse_common_thread_info(buffer + 3, debug, info->thread_id, false);
					break;
				}

				case EXCEVENT_ATTACH_BREAK:
				{
					memcpy(buffer, "S05", 3);
					len = 3;
					break;
				}

				case EXCEVENT_STOP_POINT:
				{
					switch(exc.stop_point.type)
					{
						case STOPPOINT_SVC_FF:
						case STOPPOINT_BREAKPOINT:
						{
							//TODO: implement breakpoints

							/*
							memcpy(buffer, "T05", 3);
							len = 3 + gdb_parse_common_thread_info(bufptr, debug, info->thread_id, false);

							char str[] = ";swbreak:";
							memcpy(buffer + len, str, 9);
							len += 9;
							*/
							break;
						}


						case STOPPOINT_WATCHPOINT:
						{
							//TODO: implement watchpoints

							/*
							memcpy(buffer, "T05", 3);
							len = 3 + gdb_parse_common_thread_info(bufptr, debug, info->thread_id, false);

							char str[] = ";awatch:00000000";
							str[1] = ...;
							hexItoa(exc.stop_point.fault_information, str + 8, 8, false);
							memcpy(buffer + len, str, 16);
							len += 16;
							*/
							break;
						}

						default:
							break;
					}
				}

				case EXCEVENT_USER_BREAK:
				{
					memcpy(buffer, "T05", 3);
					len = 3 + gdb_parse_common_thread_info(buffer + 3, debug, info->thread_id, false);
					break;
				}

				case EXCEVENT_DEBUGGER_BREAK:
				{
					memcpy(buffer, "T05", 3);
					len = 3;

					for(u32 i = 0; i < 4; i++)
					{
						if(exc.debugger_break.threadIds[i] > 0)
							len += gdb_parse_common_thread_info(buffer + len, debug, exc.debugger_break.threadIds[i], false);
					}

					break;
				}

				case EXCEVENT_UNDEFINED_SYSCALL:
				{
					memcpy(buffer, "T0c", 3); // SIGSYS
					len = 3 + gdb_parse_common_thread_info(buffer + 3, debug, info->thread_id, false);
					break;
				}

				default:
					break;
			}
		}

		case DBGEVENT_SYSCALL_IN:
		{
			memcpy(buffer, "T05", 3);
			len = 3 + gdb_parse_common_thread_info(buffer + 3, debug, info->thread_id, false);

			char syscall_str[] = ";syscall_entry:00";
			hexItoa(info->syscall.syscall, syscall_str + 15, 2, false);
			memcpy(buffer + len, syscall_str, 17);
			len += 17;

			break;
		}

		case DBGEVENT_SYSCALL_OUT:
		{
			memcpy(buffer, "T05", 3);
			len = 3 + gdb_parse_common_thread_info(buffer + 3, debug, info->thread_id, false);

			char syscall_str[] = ";syscall_return:00";
			hexItoa(info->syscall.syscall, syscall_str + 16, 2, false);
			memcpy(buffer + len, syscall_str, 18);
			len += 18;

			break;
		}

		case DBGEVENT_OUTPUT_STRING:
		{
			gdb_send_mem(ctx->sock, debug, "O", 1, info->output_string.string_addr, info->output_string.string_size);
			break;
		}

		default:
			break;
	}

	if(len != 0)
	{
		c_ctx->curr_thread_id = info->thread_id;
		gdb_send_packet(ctx->sock, buffer, len);
	}

	return len;
}

int gdb_handle_debug_events(Handle debug, struct sock_ctx *ctx)
{
	struct gdb_server_ctx *serv = (struct gdb_server_ctx *)ctx->serv->data;

	u32 n = 0, m = 0;
	DebugEventInfo info;
	while(R_SUCCEEDED(svcGetProcessDebugEvent(&info, debug)))
	{
		if(info.type != DBGEVENT_EXIT_PROCESS && (info.flags & 1) != 0)
			m++;
		serv->pendingDebugEvents[n++, serv->numPendingDebugEvents++] = info;
	}
	if(n == 0)
		return -1;

	if(R_SUCCEEDED(svcBreakDebugProcess(debug))) m++;
	for(u32 i = 0; i < m - 1; i++)
		svcContinueDebugEvent(debug, DBG_NO_ERRF_CPU_EXCEPTION_DUMPS);

	int ret = 0;
	if(serv->numPendingDebugEvents > 0)
    {
        ret = gdb_send_stop_reply(serv->debug, &serv->pendingDebugEvents[0], serv->client);
        serv->latestDebugEvent = serv->pendingDebugEvents[0];
        for(u32 i = 0; i < serv->numPendingDebugEvents; i++)
            serv->pendingDebugEvents[i] = serv->pendingDebugEvents[i + 1];
        --serv->numPendingDebugEvents;
    }

	return ret;
}
