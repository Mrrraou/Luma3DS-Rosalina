#pragma once

#include "utils.h"
#include "kernel.h"
#include "svc.h"

/// Operations for svcControlService
typedef enum ServiceOp
{
    SERVICEOP_STEAL_CLIENT_SESSION = 0, ///< Steal a client session given a service or global port name
    SERVICEOP_GET_NAME,                 ///< Get the name of a service or global port given a client or session handle
} ServiceOp;

Result ControlService(ServiceOp op, u32 varg1, u32 varg2);
