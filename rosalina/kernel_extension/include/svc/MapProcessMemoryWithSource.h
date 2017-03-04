#pragma once

#include "utils.h"
#include "kernel.h"
#include "svc.h"

Result MapProcessMemoryWithSource(Handle processHandle, void *dst, void *src, u32 size);
