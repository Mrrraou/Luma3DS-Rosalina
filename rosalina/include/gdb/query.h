#pragma once

#include "gdb.h"

int GDB_HandleReadQuery(GDBContext *ctx);
int GDB_HandleWriteQuery(GDBContext *ctx);

GDB_DECLARE_QUERY_HANDLER(Supported);
GDB_DECLARE_QUERY_HANDLER(StartNoAckMode);
GDB_DECLARE_QUERY_HANDLER(Attached);
