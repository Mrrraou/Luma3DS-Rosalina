#include <3ds/result.h>
#include "gdb_ctx.h"
#include "memory.h"
#include "macros.h"
#include "minisoc.h"

int gdb_send_mem(Handle sock, Handle debug, const char *prefix, u32 prefixLen, u32 addr, u32 len)
{
    char buff[256];
    Result r = 0;
    u8 cksum = 0;

    // Send packet header.
    soc_send(sock, "$", 1, 0);
    soc_send(sock, (void *)prefix, prefixLen, 0);
    u32 len_total = len;
    while(len != 0 && R_SUCCEEDED(r))
    {
        u32 read_len = len;
        if(read_len > 256)
        {
            read_len = 256;
        }

        r = svcReadProcessMemory(buff, debug, addr, len);
        if(R_FAILED(r))
        {
            if(len == len_total)
            {
                memcpy(buff, "E01", 3);
                read_len = 3;
            }
            else
                read_len = 0;
        }

        len -= read_len;
        cksum += gdb_cksum(buff, read_len);
        gdb_hex_encode(gdb_buffer, buff, read_len);
        soc_send(sock, gdb_buffer, read_len*2, 0);
    }

    // Send packet trailer.
    char s[] = "#00";
    hexItoa(cksum, s+1, 2, false);
    soc_send(sock, s, 3, 0);

    return 0;
}

int gdb_handle_read_mem(Handle sock, struct gdb_client_ctx *c, char *buffer UNUSED)
{
    struct gdb_server_ctx *serv = c->proc;

    const char *addr_start = buffer + 1;
    char *addr_end = (char*)strchr(addr_start, ',');
    if(addr_end == NULL) return -1;

    *addr_end = 0;
    const char *len_start = addr_end + 1;
    u32 addr = atoi_(addr_start, 16);
    u32 len = atoi_(len_start, 16);

    if(addr < 0x1000)
    {
        return gdb_send_packet(sock, "E01", 3);
    }

    return gdb_send_mem(sock, serv->debug, NULL, 0, addr, len);
}
