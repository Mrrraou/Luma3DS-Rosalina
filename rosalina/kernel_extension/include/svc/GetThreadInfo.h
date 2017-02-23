#pragma once

#include "utils.h"
#include "kernel.h"
#include "svc.h"

Result GetThreadInfoHook(u32 dummy, Handle threadHandle, u32 type);
