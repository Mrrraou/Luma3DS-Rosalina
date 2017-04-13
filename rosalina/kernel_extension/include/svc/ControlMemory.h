#pragma once

#include "utils.h"
#include "kernel.h"
#include "svc.h"

Result ControlMemoryHookWrapper(u32 *addrOut, u32 addr0, u32 addr1, u32 size, MemOp op, MemPerm perm);
Result ControlMemoryHook(u32 *addrOut, u32 addr0, u32 addr1, u32 size, MemOp op, MemPerm perm);
Result ControlMemoryEx(u32 *addrOut, u32 addr0, u32 addr1, u32 size, MemOp op, MemPerm perm, bool isLoader);
