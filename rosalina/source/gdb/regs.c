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

	Result r = svcGetDebugThreadContext(&regs, serv->debug, c->curr_thread_id, THREADCONTEXT_CONTROL_ALL);
    if(R_FAILED(r))
    {
        return gdb_send_packet(sock, "E01", 3);
    }

    char buf[sizeof(ThreadContext) + 4];
    memcpy(buf, &regs->cpu_registers, sizeof(CpuRegisters));
    memcpy(buf + sizeof(CpuRegisters) + 4, &reg->fpu_registers, sizeof(FpuRegisters));
	return gdb_send_packet_hex(sock, buf, sizeof(buf));
}