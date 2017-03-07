#include <3ds.h>
#include "minisoc.h"
#include "gdb_ctx.h"
#include "memory.h"
#include "macros.h"

// TODO: stub
// Right now, just sink all debug events.

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

	DebugEventInfo info, dummy;
	Result r = svcGetProcessDebugEvent(&info, debug);

	if(R_FAILED(r))
		return -1;

	u32 n = 1;
	while(R_SUCCEEDED(svcGetProcessDebugEvent(&dummy, debug))) n++;
	r = svcBreakDebugProcess(debug);
	if(R_SUCCEEDED(r)) n++;
	for(u32 i = 0; i < n - 1; i++)
		svcContinueDebugEvent(debug, DBG_NO_ERRF_CPU_EXCEPTION_DUMPS);
	return 0;
}
