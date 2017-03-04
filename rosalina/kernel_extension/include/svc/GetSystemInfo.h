#pragma once

#include "utils.h"
#include "kernel.h"
#include "svc.h"

Result GetSystemInfoHookWrapper(u32 dummy, s32 type, s32 param);
Result GetSystemInfoHook(s64 *out, s32 type, s32 param);
