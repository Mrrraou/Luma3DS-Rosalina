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
	struct KProcess *process;

	switch(handle)
	{
		case 0xFFFF8001: process = *(struct KProcess**)0xFFFF9004; break;
		default:
		{
			struct KProcess *currentKProcess = *(struct KProcess**)0xFFFF9004;
			process = (struct KProcess*) currentKProcess->handleTable.handleTable[handle & 0x7fff].pointer;
			break;
		}
	}

	info->process = process;
	info->pid = process->processId;
	memcpy(info->name, process->codeSet->processName, 8);
	info->tid = process->codeSet->titleId;
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

	u32 i;
	for(i = 0; i < sizeof(process_ids) / sizeof(u32); i++)
	{
		if(i >= (u32) process_count)
			break;

		Handle processHandle;
		Result res;
		if(R_FAILED(res = svcOpenProcess(&processHandle, process_ids[i])))
			continue;

		CurrentKProcess_GetProcessInfoFromHandle(&processes[i], processHandle);
		svcCloseHandle(processHandle);

/*
		Result res;
		DebugEventInfo debugInfo;

		if(process_ids[i] != rosalina_pid)
		{
			Handle debug_handle;
			svcDebugActiveProcess(&debug_handle, process_ids[i]);

			svcGetProcessDebugEvent(&debugInfo, debug_handle);
			res = svcContinueDebugEvent(debug_handle, 0b11);
			memcpy(process_name, debugInfo.process.process_name, 8);

			svcContinueDebugEvent(debug_handle, 0);
			svcCloseHandle(debug_handle);
		}
		else
			memcpy(process_name, "rosalina", 8);
*/
	}
}


void RosalinaMenu_ProcessList(void)
{
	u32 sleep_i;
	while(HID_PAD);
	for(sleep_i = 0; sleep_i < 0x5000000; sleep_i++) __asm__("mov r0, r0");

	svcGetProcessId(&rosalina_pid, 0xFFFF8001);
	ProcessList_FetchLoadedProcesses();

	u32 selected = 0, page = 0;
	while(true)
	{
		draw_copyToFramebuffer(splash);
		draw_string("Process list", 10, 10, COLOR_TITLE);

		u32 i = 0;
		while(i < PROCESSES_PER_MENU_PAGE)
		{
			struct ProcessInfo *info = &processes[page * PROCESSES_PER_MENU_PAGE + i];

			if(!info->process)
				break;

			char processName[9] = {0};
			strncpy(processName, info->name, 8);

			char str[80];
			sprintf(str, "%08lx %s", info->pid, processName);

			draw_string(str, 30, 30 + i * SPACING_Y, COLOR_WHITE);
			if(page * PROCESSES_PER_MENU_PAGE + i == selected)
				draw_character('>', 10, 30 + i * SPACING_Y, COLOR_TITLE);

			i++;
		}
		draw_flushFramebuffer();

		for(sleep_i = 0; sleep_i < 0x5000000; sleep_i++) __asm__("mov r0, r0");
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
