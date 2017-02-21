#include "menus/debugger.h"
#include "memory.h"
#include "draw.h"
#include "minisoc.h"
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
static MyThread debuggerThread;
static u8 ALIGN(8) debuggerThreadStack[THREAD_STACK_SIZE];

void debuggerThreadMain(void);
MyThread debuggerCreateThread(void)
{
    Result res = MyThread_Create(&debuggerThread, debuggerThreadMain, debuggerThreadStack, THREAD_STACK_SIZE, 0x20, CORE_SYSTEM);
    char msg2[] = "00000000 threadCreate";
    hexItoa(res, msg2, 8, false);
    draw_string(msg2, 10, 30, COLOR_WHITE);

    return debuggerThread;
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
            draw_string("Debugger thread started successfully.", 10, 10, COLOR_TITLE);
            //debuggerThreadMain();
            debuggerCreateThread();
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
    MyThread_Join(&debuggerThread, 0);

    miniSocExit();
    draw_string("Debugger disabled.", 10, 10, COLOR_TITLE);
    draw_flushFramebuffer();

    while(!(waitInput() & BUTTON_B));
}

void debuggerThreadMain(void)
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
                struct pollfd fds[8];
                nfds_t nfds = 1;
                fds[0].fd = sock;
                fds[0].events = POLLIN;

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
                                fds[nfds].fd = client_sock;
                                fds[nfds].events = POLLIN;
                                nfds++;
                            }
                            else
                            {
                                char buf[6];
                                buf[5] = 0;
                                res = soc_recvfrom(fds[i].fd, buf, 5, 0, NULL, 0);
                                if(R_SUCCEEDED(res))
                                {
                                    soc_sendto(fds[i].fd, buf, 5, 0, NULL, 0);
                                    if(memcmp(buf, "stop", 4) == 0)
                                    {
                                        debugger_enabled = false;
                                    }
                                }
                            }
                        }
                        else if((fds[i].revents & POLLHUP) == POLLHUP)
                        {
                            fds[i].fd = -1;
                        }
                    }
                }

                for(unsigned int i = 0; i < nfds; i++)
                {
                    socClose(fds[i].fd);
                }
            }
        }
    }
}