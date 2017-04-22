#pragma once

#include "gdb.h"

#define GDB_REMOTE_COMMAND_HANDLER(name)         GDB_HANDLER(RemoteCommand##name)
#define GDB_DECLARE_REMOTE_COMMAND_HANDLER(name) GDB_DECLARE_HANDLER(RemoteCommand##name)

GDB_DECLARE_REMOTE_COMMAND_HANDLER(SyncRequestInfo);

GDB_DECLARE_QUERY_HANDLER(Rcmd);
