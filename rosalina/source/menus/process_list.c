#include <3ds.h>
#include <stdio.h>
#include "menus/process_list.h"
#include "memory.h"
#include "draw.h"
#include "menu.h"
#include "utils.h"
#include "kernel.h"


u32 rosalina_pid;
static struct ProcessInfo processes[0x40];

void K_CurrentKProcess_GetProcessInfoFromHandle(struct ProcessInfo *info, Handle handle)
{
    KProcess *process = KProcess_ConvertHandle(*(KProcess**)0xFFFF9004, handle);
    KCodeSet *codeSet = KPROCESS_GET_RVALUE(process, codeSet);
    info->process = process;
    info->pid = KPROCESS_GET_RVALUE(process, processId);
    memcpy(info->name, codeSet->processName, 8);
    info->tid = codeSet->titleId;
}

void CurrentKProcess_GetProcessInfoFromHandle(struct ProcessInfo *info, Handle handle)
{
    svc_7b(K_CurrentKProcess_GetProcessInfoFromHandle, info, handle);
}

void ProcessList_FetchLoadedProcesses(void)
{
    memset(processes, 0, sizeof(processes));

    s32 process_count;
    u32 process_ids[0x40];
    svcGetProcessList(&process_count, process_ids, sizeof(process_ids) / sizeof(u32));

    for(u32 i = 0; i < sizeof(process_ids) / sizeof(u32); i++)
    {
        if(i >= (u32) process_count)
            break;

        Handle processHandle;
        Result res;
        if(R_FAILED(res = svcOpenProcess(&processHandle, process_ids[i])))
            continue;

        CurrentKProcess_GetProcessInfoFromHandle(&processes[i], processHandle);
        svcCloseHandle(processHandle);
    }
}


void RosalinaMenu_ProcessList(void)
{
    vu32 sleep_i;
    while(HID_PAD);
    for(sleep_i = 0; sleep_i < 0x5000000; sleep_i++);

    svcGetProcessId(&rosalina_pid, 0xFFFF8001);
    ProcessList_FetchLoadedProcesses();

    u32 selected = 0, page = 0;
    while(true)
    {
        draw_copyToFramebuffer(splash);
        draw_string("Process list", 10, 10, COLOR_TITLE);

        for(u32 i = 0; i < PROCESSES_PER_MENU_PAGE; i++)
        {
            struct ProcessInfo *info = &processes[page * PROCESSES_PER_MENU_PAGE + i];

            if(!info->process)
                break;

            char str[18];
            hexItoa(info->pid, str, 8, false);
            str[8] = ' ';
            memcpy(str + 9, info->name, 8);
            str[17] = 0;

            draw_string(str, 30, 30 + i * SPACING_Y, COLOR_WHITE);
            if(page * PROCESSES_PER_MENU_PAGE + i == selected)
                draw_character('>', 10, 30 + i * SPACING_Y, COLOR_TITLE);
        }
        draw_flushFramebuffer();

        for(sleep_i = 0; sleep_i < 0x5000000; sleep_i++);
        while(!HID_PAD);

        if(HID_PAD & BUTTON_B)
            break;
        else if(HID_PAD & BUTTON_DOWN)
        {
            if(!processes[++selected].process)
                selected = 0;
        }
        else if(HID_PAD & BUTTON_UP)
        {
            if(selected-- <= 0)
            {
                u32 i = 0x40;
                while(!processes[--i].process);
                selected = i;
            }
        }

        page = selected / PROCESSES_PER_MENU_PAGE;
    }
}
