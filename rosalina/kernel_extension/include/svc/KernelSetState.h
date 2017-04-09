#pragma once

#include "utils.h"
#include "kernel.h"
#include "svc.h"

extern s32 rosalinaState;
bool shouldSignalSyscallDebugEvent(const KProcess *process, u8 svcId);
Result KernelSetStateHook(u32 type, u32 varg1, u32 varg2, u32 varg3);
