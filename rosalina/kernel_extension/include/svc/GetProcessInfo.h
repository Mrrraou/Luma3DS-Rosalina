#pragma once

#include "utils.h"
#include "kernel.h"
#include "svc.h"

Result GetProcessInfoHook(u32 dummy, Handle processHandle, u32 type);
