#pragma once

#include "utils.h"
#include "kernel.h"
#include "svc.h"

extern u32 rosalinaState;
bool shouldSignalSyscallDebugEvent(KProcess *process, u8 svcId);
Result KernelSetStateHook(u32 type, u32 varg1, u32 varg2, u32 varg3);
