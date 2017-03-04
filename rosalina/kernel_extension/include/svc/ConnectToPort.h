#pragma once

#include "utils.h"
#include "kernel.h"
#include "svc.h"

Result ConnectToPortHookWrapper(u32 dummy, const char *name);
Result ConnectToPortHook(Handle *out, const char *name);
