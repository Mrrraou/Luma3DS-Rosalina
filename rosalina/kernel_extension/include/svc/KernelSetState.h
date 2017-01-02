#pragma once

#include "utils.h"
#include "kernel.h"
#include "svc.h"

// Result svcKernelSetState(u32 type, ...) <= official SVC
extern s32 rosalinaState;
Result KernelSetStateHook(u32 type, u32 varg1, u32 varg2, u32 varg3, u32 varg4);
