#include <3ds/result.h>
#include "gdb_ctx.h"
#include "memory.h"
#include "macros.h"
#include "minisoc.h"

int gdb_handle_break(Handle sock UNUSED, struct gdb_client_ctx *c, char *buffer UNUSED)
{
    struct gdb_server_ctx *serv = c->proc;
    svcBreakDebugProcess(serv->debug);
    return 0;
}

int gdb_handle_continue(Handle sock , struct gdb_client_ctx *c, char *buffer)
{
    struct gdb_server_ctx *serv = c->proc;

    char *addrStart;
    u32 addr = 0;
    if(buffer[0] == 'C')
    {
	    for(addrStart = buffer + 1; *addrStart != ';' && *addrStart != 0; addrStart++);
        addrStart++;
    }
    else
        addrStart = buffer + 1;

    if(*addrStart != 0  && c->curr_thread_id != 0)
    {
        ThreadContext regs;
        addr = (u32)atoi_(++addrStart, 16);
        Result r = svcGetDebugThreadContext(&regs, serv->debug, c->curr_thread_id, THREADCONTEXT_CONTROL_CPU_SPRS);
        if(R_SUCCEEDED(r))
        {
            regs.cpu_registers.pc = addr;
            r = svcSetDebugThreadContext(serv->debug, c->curr_thread_id, &regs, THREADCONTEXT_CONTROL_CPU_SPRS);
        }
    }

    if(serv->numPendingDebugEvents > 0)
    {
        gdb_send_stop_reply(serv->debug, &serv->pendingDebugEvents[0], serv->client);
        serv->latestDebugEvent = serv->pendingDebugEvents[0];
        for(u32 i = 0; i < serv->numPendingDebugEvents; i++)
            serv->pendingDebugEvents[i] = serv->pendingDebugEvents[i + 1];
        --serv->numPendingDebugEvents;
    }

    else
    {
        DebugEventInfo dummy;
        while(R_SUCCEEDED(svcGetProcessDebugEvent(&dummy, serv->debug)));
        while(R_SUCCEEDED(svcContinueDebugEvent(serv->debug, DBG_NO_ERRF_CPU_EXCEPTION_DUMPS)));
        svcSignalEvent(serv->continuedEvent);
    }

    return 0; //Hmmm...
}
