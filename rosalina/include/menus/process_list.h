#pragma once

#include <3ds/types.h>
#include "kernel.h"

#define PROCESSES_PER_MENU_PAGE 18

struct ProcessInfo {
    KProcess *process;
    u32 pid;
    char name[8];
    u64 tid;
};


void RosalinaMenu_ProcessList(void);
