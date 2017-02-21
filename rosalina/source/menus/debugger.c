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

static bool debugger_enabled;

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
        svcGetSystemInfo(&amt, 0, 3);

        char msg1[] = "00000000 free";
        hexItoa(0x01400000 - amt, msg1, 8, false);
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
            draw_string("SOC init success", 10, 10, COLOR_TITLE);

            Handle sock = 0;

            res = socSocket(&sock, AF_INET, SOCK_STREAM, 0);
            while(R_FAILED(res))
            {
                draw_string("Sleeping...", 10, 20, COLOR_TITLE);
                draw_flushFramebuffer();
                svcSleepThread(100000000LL);

                res = socSocket(&sock, AF_INET, SOCK_STREAM, 0);
            }

            draw_string("!!!!!", 10, 20, COLOR_TITLE);
            draw_flushFramebuffer();

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
                        draw_string("Accepting..", 10, 20, COLOR_TITLE);
                        Handle client_sock = 0;
                        res = socAccept(sock, &client_sock, NULL, 0);

                        if(R_SUCCEEDED(res))
                        {
                            draw_string("Got client!", 10, 20, COLOR_TITLE);

                            char buf[5];
                            buf[4] = 0;
                            res = soc_recvfrom(client_sock, buf, 4, 0, NULL, 0);
                            if(R_SUCCEEDED(res))
                            {
                                draw_string(buf, 10, 30, COLOR_TITLE);
                            }
                            else
                            {
                                draw_string("recv fail :(", 10, 30, COLOR_WHITE);
                            }

                            socClose(client_sock);
                            socClose(sock);
                        }
                        else
                        {
                            draw_string("accept fail :(", 10, 20, COLOR_WHITE);
                        }
                    }
                    else
                    {
                        draw_string("listen fail :(", 10, 20, COLOR_WHITE);
                    }
                }
                else
                {
                    draw_string("bind fail :(", 10, 20, COLOR_WHITE);
                }
            }
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

    miniSocExit();
    draw_string("SOC status: asdasdas", 10, 10, COLOR_TITLE);

    debugger_enabled = false;

    draw_flushFramebuffer();

    while(!(waitInput() & BUTTON_B));
}
