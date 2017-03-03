#include <3ds/result.h>
#include "gdb_ctx.h"
#include "minisoc.h"
#include "memory.h"
#include "macros.h"

int gdb_get_thread_id(Handle sock, struct gdb_client_ctx *c, char *buffer UNUSED)
{
    struct gdb_server_ctx *serv = c->proc;
    s32 n_threads = serv->n_threads;
    if(n_threads == 0)
    {
        return gdb_send_packet(sock, "E01", 3);
    }

    if(c->curr_thread_id == 0)
    {
        c->curr_thread_id = serv->thread_ids[0];
    }

    char buf[3];
    hexItoa(c->curr_thread_id, buf, 3, false);
    return gdb_send_packet(sock, buf, 3);
}

int gdb_handle_set_thread_id(Handle sock, struct gdb_client_ctx *c, char *buffer)
{
    if(buffer[1] != 'g')
    {
        return -1;
    }

    u32 id = atoi_(buffer+2, 16);
    c->curr_thread_id = id;
    return gdb_send_packet(sock, "OK", 2);    
}

// TODO: stub

int gdb_f_thread_info(Handle sock, struct gdb_client_ctx *c, char *buffer UNUSED)
{
    struct gdb_server_ctx *serv = c->proc;
    s32 n_threads = serv->n_threads;
    if(n_threads == 0)
    {
        Result r = svcGetThreadList(&n_threads, serv->thread_ids, MAX_THREAD, serv->proc);
        if(R_FAILED(r))
        {
            return -1;
        }
        serv->n_threads = n_threads;
    }

    char str_buff[MAX_THREAD * 4 - 1];
    for(s32 i = 0; i < n_threads; i++)
    {
        hexItoa(serv->thread_ids[i], str_buff + i * 4, 3, false);
        str_buff[i*4 + 3] = ',';
    }
    
    //str_buff[n_threads * 9 - 1] = 0;
    return gdb_send_packet_prefix(sock, "m", 1, str_buff, n_threads * 4 - 1);
}

int gdb_s_thread_info(Handle sock, struct gdb_client_ctx *c UNUSED, char *buffer UNUSED)
{
    return gdb_send_packet(sock, "l", 1);
}