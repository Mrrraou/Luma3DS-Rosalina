#include <3ds.h>
#include "kernel.h"
#include "memory.h"
#include "utils.h"


ProcessInfo processes_info[0x40];

static void K_CurrentKProcess_GetProcessInfoFromHandle(ProcessInfo *info, Handle handle)
{
    KProcess *process = KProcess_ConvertHandle(*(KProcess**)0xFFFF9004, handle);
    KCodeSet *codeSet = KPROCESS_GET_RVALUE(process, codeSet);
    info->process = process;
    info->pid = KPROCESS_GET_RVALUE(process, processId);
    memcpy(info->name, codeSet->processName, 8);
    info->tid = codeSet->titleId;
}

void Kernel_CurrentKProcess_GetProcessInfoFromHandle(ProcessInfo *info, Handle handle)
{
    svc_7b(K_CurrentKProcess_GetProcessInfoFromHandle, info, handle);
}

void Kernel_FetchLoadedProcesses(void)
{
    memset_(processes_info, 0, sizeof(processes_info));

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

        Kernel_CurrentKProcess_GetProcessInfoFromHandle(&processes_info[i], processHandle);
        svcCloseHandle(processHandle);
    }
}
