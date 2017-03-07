#include <3ds/result.h>
#include "gdb_ctx.h"
#include "memory.h"
#include "macros.h"
#include "minisoc.h"

int gdb_handle_break(Handle sock, struct gdb_client_ctx *c, char *buffer UNUSED)
{
    struct gdb_server_ctx *serv = c->proc;
    
    Result r = svcBreakDebugProcess(serv->debug);

    if(r == (Result)0xD8202007 /* already interrupted */ || R_SUCCEEDED(r))
        return gdb_send_packet(sock, "OK", 2);
    else if(r == (Result)0xD8A02008) // parent process ended
        return gdb_send_packet(sock, "E03", 3);
    else
        return gdb_send_packet(sock, "E01", 3);
}

int gdb_handle_continue(Handle sock UNUSED, struct gdb_client_ctx *c, char *buffer)
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

    while(svcContinueDebugEvent(serv->debug, DBG_NO_ERRF_CPU_EXCEPTION_DUMPS) != (Result)0xD8402009); // continue all

    return 0; //Hmmm...
}