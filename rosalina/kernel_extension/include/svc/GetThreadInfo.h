#pragma once

#include "utils.h"
#include "kernel.h"
#include "svc.h"

Result GetThreadInfoHookWrapper(u32 dummy, Handle threadHandle, u32 type);
Result GetThreadInfoHook(s64 *out, Handle threadHandle, u32 type);
