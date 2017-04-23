#pragma once

#include "utils.h"
#include "kernel.h"
#include "svc.h"

Result TranslateHandleWrapper(u32 *outKAddr, char *outClassName, Handle handle);
Result TranslateHandle(u32 *outKAddr, char *outClassName, Handle handle);
