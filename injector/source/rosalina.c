#include <string.h>
#include <3ds.h>
#include "srvsys.h"
#include "fsreg.h"
#include "rosalina.h"

static Handle rosalinaHandle;
static int rosalinaRefCount;
static RecursiveLock initLock;
static int initLockinit = 0;

Result rosalinaInit(void)
{
	Result res;
	if(!initLockinit)
    RecursiveLock_Init(&initLock);
  RecursiveLock_Lock(&initLock);

  if(rosalinaRefCount > 0)
  {
    RecursiveLock_Unlock(&initLock);
    return MAKERESULT(RL_INFO, RS_NOP, 25, RD_ALREADY_INITIALIZED);
  }

	while(1)
  {
    res = svcConnectToPort(&rosalinaHandle, "Rosalina");
    if(R_LEVEL(res) != RL_PERMANENT
		|| R_SUMMARY(res) != RS_NOTFOUND
		|| R_DESCRIPTION(res) != RD_NOT_FOUND)
			break;
    svcSleepThread(500000);
  }

	if(R_SUCCEEDED(res))
		rosalinaRefCount++;

	RecursiveLock_Unlock(&initLock);
	return res;
}

Result rosalinaExit(void)
{
	Result rc;
  RecursiveLock_Lock(&initLock);

  if(rosalinaRefCount > 1)
  {
    rosalinaRefCount--;
    RecursiveLock_Unlock(&initLock);
    return MAKERESULT(RL_INFO, RS_NOP, 25, RD_BUSY);
  }

  if(rosalinaHandle != 0)
		svcCloseHandle(rosalinaHandle);
  else
		svcBreak(USERBREAK_ASSERT);

  rosalinaHandle = 0;
  rosalinaRefCount--;
  RecursiveLock_Unlock(&initLock);

	return rc;
}

void Rosalina_GetCFWInfo(CFWInfo *out)
{
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = IPC_MakeHeader(1, 4, 0);
	svcSendSyncRequest(rosalinaHandle);

	memcpy(out, &cmdbuf[1], sizeof(CFWInfo));
}

Handle Rosalina_GiveFSREGHandleCopy(void)
{
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = IPC_MakeHeader(1300, 4, 0);
	svcSendSyncRequest(rosalinaHandle);

	return cmdbuf[2];
}

Result Rosalina_Get3DSXInfo(exheader_header *header)
{
	return 0;
}
