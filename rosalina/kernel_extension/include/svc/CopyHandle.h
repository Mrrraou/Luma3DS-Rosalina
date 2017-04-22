#pragma once

#include "utils.h"
#include "kernel.h"
#include "svc.h"

Result CopyHandleWrapper(Handle *outHandle, Handle outProcessHandle, Handle inHandle, Handle inProcessHandle);
Result CopyHandle(Handle *outHandle, Handle outProcessHandle, Handle inHandle, Handle inProcessHandle);
