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
