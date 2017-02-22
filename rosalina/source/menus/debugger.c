#include "menus/debugger.h"
#include "memory.h"
#include "draw.h"
#include "minisoc.h"
#include "gdb/client_ctx.h"
#include <sys/socket.h>

Menu menu_debugger = {
    "Debugger options menu",
    .items = 2,
    {
        {"Enable debugger", METHOD, .method = &Debugger_Enable},
        {"Disable debugger", METHOD, .method = &Debugger_Disable}
    }
};

#define MAX_CLIENTS 8

static bool debugger_enabled;
static MyThread debuggerSocketThread;
static MyThread debuggerDebugThread;
static u8 ALIGN(8) debuggerSocketThreadStack[THREAD_STACK_SIZE];
static u8 ALIGN(8) debuggerDebugThreadStack[THREAD_STACK_SIZE];


struct gdb_client_ctx real_client_ctxs[MAX_CLIENTS];
struct gdb_client_ctx *client_ctxs[MAX_CLIENTS];

void debuggerSocketThreadMain(void);
MyThread *debuggerCreateSocketThread(void)
{
    MyThread_Create(&debuggerSocketThread, debuggerSocketThreadMain, debuggerSocketThreadStack, THREAD_STACK_SIZE, 0x20, CORE_SYSTEM);
    return &debuggerSocketThread;
}

void debuggerDebugThreadMain(void);
MyThread *debuggerCreateDebugThread(void)
{
    MyThread_Create(&debuggerDebugThread, debuggerDebugThreadMain, debuggerDebugThreadStack, THREAD_STACK_SIZE, 0x20, CORE_SYSTEM);
    return &debuggerDebugThread;
}

void Debugger_Enable(void)
{
    draw_clearFramebuffer();
    draw_flushFramebuffer();

    if(debugger_enabled)
    {
        draw_string("Already enabled!", 10, 10, COLOR_TITLE);
    }
    else
    {  
        draw_string("Initialising SOC...", 10, 10, COLOR_WHITE);

        s64 amt = 0;
        svcGetSystemInfo(&amt, 0, 2);

        char msg1[] = "00000000 free in SYSTEM";
        hexItoa(0x02C00000 - amt, msg1, 8, false);
        draw_string(msg1, 10, 20, COLOR_WHITE);

        Result res = miniSocInit(0x20000);
        if(R_FAILED(res))
        {
            char msg2[] = "00000000 err";
            hexItoa(res, msg2, 8, false);
            draw_string(msg2, 10, 30, COLOR_WHITE);
        }
        else
        {
            for(int i = 0; i < MAX_CLIENTS; i++)
            {
                real_client_ctxs[i].flags = 0;
                client_ctxs[i] = NULL;
            }

            debuggerCreateSocketThread();
            debuggerCreateDebugThread();
            draw_string("Debugger thread started successfully.", 10, 10, COLOR_TITLE);
        }
    }

    debugger_enabled = true;

    draw_flushFramebuffer();

    while(!(waitInput() & BUTTON_B));
}

void Debugger_Disable(void)
{
    draw_clearFramebuffer();
    draw_flushFramebuffer();

    debugger_enabled = false;
    MyThread_Join(&debuggerSocketThread, -1);
    MyThread_Join(&debuggerDebugThread, -1);

    miniSocExit();
    draw_string("Debugger disabled.", 10, 10, COLOR_TITLE);
    draw_flushFramebuffer();

    while(!(waitInput() & BUTTON_B));
}

void compact(struct pollfd *fds, nfds_t *nfds);
void close_then_compact(struct pollfd *fds, nfds_t *nfds, int i)
{
    socClose(fds[i].fd);
    fds[i].fd = -1;
    fds[i].events = 0;
    fds[i].revents = 0;

    if(client_ctxs[i] != NULL)
    {
        gdb_destroy_client(client_ctxs[i]);
        client_ctxs[i] = NULL;
    }

    compact(fds, nfds);
}

void debuggerSocketThreadMain(void)
{
    Result res = 0;
    Handle sock = 0;

    res = socSocket(&sock, AF_INET, SOCK_STREAM, 0);
    while(R_FAILED(res))
    {
        svcSleepThread(100000000LL);

        res = socSocket(&sock, AF_INET, SOCK_STREAM, 0);
    }

    if(R_SUCCEEDED(res))
    {
        struct sockaddr_in saddr;
        saddr.sin_family = AF_INET;
        saddr.sin_port = 0xa00f; // 4000, byteswapped
        saddr.sin_addr.s_addr = gethostid();

        res = socBind(sock, (struct sockaddr*)&saddr, sizeof(struct sockaddr_in));

        if(R_SUCCEEDED(res))
        {
            res = socListen(sock, 2);
            if(R_SUCCEEDED(res))
            {
                struct pollfd fds[MAX_CLIENTS];
                nfds_t nfds = 1;
                fds[0].fd = sock;
                fds[0].events = POLLIN | POLLHUP;

                while(debugger_enabled)
                {
                    int res = socPoll(fds, nfds, 50); // 50ms
                    if(res == 0) continue; // timeout reached, no activity.

                    for(unsigned int i = 0; i < nfds; i++)
                    {
                        if((fds[i].revents & POLLIN) == POLLIN)
                        {
                            if(i == 0)
                            {
                                Handle client_sock = 0;
                                res = socAccept(sock, &client_sock, NULL, 0);
                                if(nfds == MAX_CLIENTS)
                                {
                                    socClose(client_sock);
                                }
                                else
                                {
                                    fds[nfds].fd = client_sock;
                                    fds[nfds].events = POLLIN | POLLHUP;
                                    nfds++;

                                    bool found = false;
                                    for(int j = 0; j < MAX_CLIENTS; j++) // Find an empty GDB context.
                                    {
                                        if(!(real_client_ctxs[j].flags & GDB_FLAG_USED))
                                        {
                                            client_ctxs[nfds-1] = &real_client_ctxs[j];
                                            gdb_setup_client(client_ctxs[nfds-1]);
                                            found = true;
                                            break;
                                        }
                                    }

                                    if(!found) // just in case
                                    {
                                        close_then_compact(fds, &nfds, nfds-1);
                                    }
                                }
                            }
                            else
                            {
                                if(client_ctxs[i] != NULL)
                                {
                                    if(gdb_do_packet(fds[i].fd, client_ctxs[i]) == -1)
                                    {
                                        close_then_compact(fds, &nfds, i);
                                    }
                                }
                            }
                        }
                        else if(fds[i].revents & POLLHUP || fds[i].revents & POLLERR)
                        {
                            close_then_compact(fds, &nfds, i);
                        }
                    }
                }

                // Clean up.
                for(unsigned int i = 0; i < nfds; i++)
                {
                    socClose(fds[i].fd);
                }
            }
        }
    }
}

// soc's poll function is odd, and doesn't like -1 as fd.
// so this compacts everything together

void compact(struct pollfd *fds, nfds_t *nfds)
{
    int new_fds[MAX_CLIENTS];
    struct gdb_client_ctx *new_gdb_ctxs[MAX_CLIENTS];
    nfds_t n = 0;

    for(nfds_t i = 0; i < *nfds; i++)
    {
        if(fds[i].fd != -1)
        {
            new_fds[n] = fds[i].fd;
            new_gdb_ctxs[n] = client_ctxs[i];
            n++;
        }
    }

    for(nfds_t i = 0; i < n; i++)
    {
        fds[i].fd = new_fds[i];
        client_ctxs[i] = new_gdb_ctxs[i];
    }
    *nfds = n;
}

void debuggerDebugThreadMain(void)
{
}