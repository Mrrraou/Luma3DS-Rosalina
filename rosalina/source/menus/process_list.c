#include <3ds.h>
#include "menus/process_list.h"
#include "memory.h"
#include "draw.h"
#include "menu.h"
#include "utils.h"
#include "kernel.h"


u32 rosalina_pid;

void RosalinaMenu_ProcessList(void)
{
    svcGetProcessId(&rosalina_pid, 0xFFFF8001);
    Kernel_FetchLoadedProcesses();

    u32 selected = 0, page = 0, pagePrev = 0;

    while(true)
    {
        if(page != pagePrev)
            draw_clearFramebuffer();
        draw_string("Process list", 10, 10, COLOR_TITLE);

        for(u32 i = 0; i < PROCESSES_PER_MENU_PAGE; i++)
        {
            ProcessInfo *info = &processes_info[page * PROCESSES_PER_MENU_PAGE + i];

            if(!info->process)
                break;

            char str[18];
            hexItoa(info->pid, str, 8, false);
            str[8] = ' ';
            memcpy(str + 9, info->name, 8);
            str[17] = 0;

            draw_string(str, 30, 30 + i * SPACING_Y, COLOR_WHITE);
            draw_character(page * PROCESSES_PER_MENU_PAGE + i == selected ? '>' : ' ', 10, 30 + i * SPACING_Y, COLOR_TITLE);
        }
        draw_flushFramebuffer();

        u32 pressed = waitInput();

        if(pressed & BUTTON_B)
            break;
        else if(pressed & BUTTON_DOWN)
        {
            if(!processes_info[++selected].process)
                selected = 0;
        }
        else if(pressed & BUTTON_UP)
        {
            if(selected-- <= 0)
            {
                u32 i = 0x40;
                while(!processes_info[--i].process);
                selected = i;
            }
        }

        pagePrev = page;
        page = selected / PROCESSES_PER_MENU_PAGE;
    }
}
