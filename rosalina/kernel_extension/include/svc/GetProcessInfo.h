#pragma once

#include "utils.h"
#include "kernel.h"
#include "svc.h"

Result GetProcessInfoHookWrapper(u32 dummy, Handle processHandle, u32 type);
Result GetProcessInfoHook(s64 *out, Handle processHandle, u32 type);
