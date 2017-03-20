#include "gdb.h"
#include "gdb/net.h"

void GDB_InitializeContext(GDBContext *ctx)
{
	RecursiveLock_Init(&ctx->lock);

	RecursiveLock_Lock(&ctx->lock);

	svcCreateEvent(&ctx->continuedEvent, RESET_ONESHOT);
	svcCreateEvent(&ctx->clientAcceptedEvent, RESET_STICKY);

	ctx->eventToWaitFor = ctx->clientAcceptedEvent;
	ctx->continueFlags = (DebugFlags)(DBG_SIGNAL_FAULT_EXCEPTION_EVENTS | DBG_INHIBIT_USER_CPU_EXCEPTION_HANDLERS);

	RecursiveLock_Unlock(&ctx->lock);
}

void GDB_FinalizeContext(GDBContext *ctx)
{
	RecursiveLock_Lock(&ctx->lock);

	svcClearEvent(ctx->clientAcceptedEvent);

	svcCloseHandle(ctx->clientAcceptedEvent);
	svcCloseHandle(ctx->continuedEvent);

	RecursiveLock_Unlock(&ctx->lock);
}

GDB_DECLARE_HANDLER(Unsupported)
{
	return GDB_ReplyEmpty(ctx);
}
