#include <3ds.h>
#include "minisoc.h"
#include "gdb_ctx.h"
#include "memory.h"
#include "macros.h"

// TODO: stub
// Right now, just sink all debug events.

u32 gdb_parse_common_thread_info(char *out, Handle debug, DebugEventInfo *info)
{
	char buf[] = "thread:00000000;d:00000000;f:00000000;";
	hexItoa(info->thread_id, buf + 7, 8, false);
	ThreadContext regs;
	Result r = svcGetDebugThreadContext(&regs, debug, info->thread_id, THREADCONTEXT_CONTROL_CPU_SPRS);
	
	if(R_FAILED(r))
	{
		memcpy(out, buf, 16);
		return 16;
	}
	hexItoa(regs.cpu_registers.sp, buf + 18, 8, false);
	hexItoa(regs.cpu_registers.pc, buf + 29, 8, false);

	memcpy(out, buf, 38);
	return 38;
}
int gdb_send_debug_events(Handle debug, DebugEventInfo *info)
{
	char buffer[256];
	switch(info->type)
	{
		case DBGEVENT_EXIT_THREAD:
		{
			// no signal, SIGTERM, SIGQUIT (process exited), SIGTERM (process terminated)
			static const char *threadExitRepliesPrefix[] = {"w00;", "w0f;", "w03;", "w0f;"};
			memcpy(buffer, threadExitRepliesPrefix[(u32)info->exit_thread.reason], 3);
			hexItoa(info->thread_id, buffer + 4, 8, false);
			break;
		}
		case DBGEVENT_EXIT_PROCESS:
		{
			// exited (no error), SIGTERM (terminated), SIGKILL (killed due to unhandled CPU exception), SIGTERM (process terminated)
			static const char *processExitReplies[] = {"W00", "X0f", "X09"};
			memcpy(buffer, processExitReplies[(u32)info->exit_process.reason], 3);
			break;
		}
		case DBGEVENT_SYSCALL_IN:
		{
			char *bufptr = buffer;
			memcpy(bufptr, "T05", 3);
			bufptr += 3;
			bufptr += gdb_parse_common_thread_info(bufptr, debug, info);
			
			char syscall_str[] = "syscall_entry:00";
			hexItoa(info->syscall.syscall, syscall_str + 14, 2, false);
			memcpy(bufptr, syscall_str, 16);
			bufptr += 16;
			*bufptr = 0;

			break;
		}

		case DBGEVENT_SYSCALL_OUT:
		{
			char *bufptr = buffer;
			memcpy(bufptr, "T05", 3);
			bufptr += 3;
			bufptr += gdb_parse_common_thread_info(bufptr, debug, info);
			
			char syscall_str[] = "syscall_return:00";
			hexItoa(info->syscall.syscall, syscall_str + 15, 2, false);
			memcpy(bufptr, syscall_str, 17);
			bufptr += 17;
			*bufptr = 0;

			break;
		}

		case DBGEVENT_EXCEPTION:
		{
			//TODO
			break;
		}

		default:
			break;
	}

	return 0;
}

int gdb_handle_debug_events(Handle debug, struct sock_ctx *ctx UNUSED)
{
	/*
	DebugEventInfo info;
	Result r;

	while(true)
	{
		if((r = svcGetProcessDebugEvent(&info, debug)) != 0)
		{
			if(r == (Result)0xd8402009)
			{
				// Would block! we are done.
				break;
			}
		}
		else
		{
			// Debug event.
		}
		svcContinueDebugEvent(debug, DBG_NO_ERRF_CPU_EXCEPTION_DUMPS);
	}
	return 0;
*/

	DebugEventInfo infos[0x10];
	Result r = svcGetProcessDebugEvent(infos, debug);

	if(R_FAILED(r))
		return -1;

	u32 n = 1;
	while(R_SUCCEEDED(svcGetProcessDebugEvent(infos + n, debug))) n++;
	r = svcBreakDebugProcess(debug);
	if(R_SUCCEEDED(r)) n++;
	for(u32 i = 0; i < n - 1; i++)
		svcContinueDebugEvent(debug, DBG_NO_ERRF_CPU_EXCEPTION_DUMPS);
	return 0;
}
