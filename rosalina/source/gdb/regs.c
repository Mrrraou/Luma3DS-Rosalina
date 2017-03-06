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
        return gdb_send_packet(sock, "E01", 3);
    return gdb_send_packet_hex(sock, (const char *)&regs, sizeof(ThreadContext));
}

int gdb_handle_write_regs(Handle sock, struct gdb_client_ctx *c, char *buffer)
{
    if(c->curr_thread_id == 0)
    {
        return gdb_send_packet(sock, "E01", 3);
    }
    struct gdb_server_ctx *serv = c->proc;

	ThreadContext regs;
	if(gdb_hex_decode((char *)&regs, buffer + 1, sizeof(ThreadContext)) != sizeof(ThreadContext))
		return gdb_send_packet(sock, "E01", 3);

	Result r = svcSetDebugThreadContext(serv->debug, c->curr_thread_id, &regs, THREADCONTEXT_CONTROL_ALL);
	if(R_FAILED(r))
		return gdb_send_packet(sock, "E01", 3);
	else
    	return gdb_send_packet(sock, "OK", 2);
}

static u32 gdb_convert_reg_num(ThreadContextControlFlags *flags, u32 gdbNum)
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

int gdb_handle_read_reg(Handle sock, struct gdb_client_ctx *c, char *buffer)
{
    if(c->curr_thread_id == 0)
    {
        return gdb_send_packet(sock, "E01", 3);
    }
    struct gdb_server_ctx *serv = c->proc;

	ThreadContext regs;
	ThreadContextControlFlags flags;
	u32 n = gdb_convert_reg_num(&flags, (u32)atoi_(buffer + 1, 16));
	if(!flags)
		return gdb_send_packet(sock, "E01", 3);

	Result r = svcGetDebugThreadContext(&regs, serv->debug, c->curr_thread_id, flags);

	if(R_FAILED(r))
		return gdb_send_packet(sock, "E01", 3);

	if(flags & THREADCONTEXT_CONTROL_CPU_GPRS)
		return gdb_send_packet_hex(sock, (const char *)&regs.cpu_registers.r[n], 4);
	else if(flags & THREADCONTEXT_CONTROL_CPU_SPRS)
		return gdb_send_packet_hex(sock, (const char *)(&regs.cpu_registers.sp + (n - 13)), 4); // hacky
	else if(flags & THREADCONTEXT_CONTROL_FPU_GPRS)
		return gdb_send_packet_hex(sock, (const char *)&regs.fpu_registers.f[n], 4);
	else
		return gdb_send_packet_hex(sock, (const char *)(&regs.fpu_registers.fpscr + n), 4); // hacky
}

int gdb_handle_write_reg(Handle sock, struct gdb_client_ctx *c, char *buffer)
{
    if(c->curr_thread_id == 0)
    {
        return gdb_send_packet(sock, "E01", 3);
    }
    struct gdb_server_ctx *serv = c->proc;

	ThreadContext regs;
	ThreadContextControlFlags flags;
	char *valueStart;
	for(valueStart = buffer + 1; *valueStart != '='; valueStart++)
	{
		if(*valueStart == 0)
			return gdb_send_packet(sock, "E01", 3);
	}
	*valueStart++ = 0;

	u32 n = gdb_convert_reg_num(&flags, (u32)atoi_(buffer + 1, 16)), value = (u32)atoi_(valueStart, 16);
	if(!flags)
		return gdb_send_packet(sock, "E01", 3);

	Result r = svcGetDebugThreadContext(&regs, serv->debug, c->curr_thread_id, flags);

	if(R_FAILED(r))
		return gdb_send_packet(sock, "E01", 3);

	if(flags & THREADCONTEXT_CONTROL_CPU_GPRS)
		regs.cpu_registers.r[n] = value;
	else if(flags & THREADCONTEXT_CONTROL_CPU_SPRS)
		*(&regs.cpu_registers.sp + (n - 13)) = value; // hacky
	else if(flags & THREADCONTEXT_CONTROL_FPU_GPRS)
		memcpy(&regs.fpu_registers.f[n], &value, 4);
	else
		*(&regs.fpu_registers.fpscr + n) = value; // hacky

	r = svcSetDebugThreadContext(serv->debug, c->curr_thread_id, &regs, flags);

	if(R_FAILED(r))
		return gdb_send_packet(sock, "E01", 3);
	else
		return gdb_send_packet(sock, "OK", 2);
}
