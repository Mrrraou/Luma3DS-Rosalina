#include <3ds/result.h>
#include "gdb_ctx.h"
#include "memory.h"
#include "macros.h"
#include "draw.h"


int gdb_handle_read_regs(Handle sock, struct gdb_client_ctx *c, char *buffer UNUSED)
{
    if(c->curr_thread_id == 0)
    {
        return gdb_send_packet(sock, "E01", 3);
    }

    struct gdb_server_ctx *serv = c->proc;

	ThreadContext regs;
    //memset_(&regs, 0, sizeof(regs));

	Result r = svcGetDebugThreadContext(&regs, serv->debug, c->curr_thread_id, THREADCONTEXT_CONTROL_CPU_REGS);
    if(R_FAILED(r))
    {
        return gdb_send_packet(sock, "E01", 3);
    }

	return gdb_send_packet_hex(sock, (const char *)&regs, sizeof(CpuRegisters));
}