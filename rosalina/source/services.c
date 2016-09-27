#include <3ds.h>
#include <string.h>
#include "fsreg.h"
#include "services.h"

static Handle *srvHandlePtr;
static int srvRefCount;
static RecursiveLock initLock;
static int initLockinit = 0;


Result srvSysInit(void)
{
  Result rc = 0;

  if (!initLockinit)
    RecursiveLock_Init(&initLock);

  RecursiveLock_Lock(&initLock);

  if (srvRefCount > 0)
  {
    RecursiveLock_Unlock(&initLock);
    return MAKERESULT(RL_INFO, RS_NOP, 25, RD_ALREADY_INITIALIZED);
  }

  srvHandlePtr = srvGetSessionHandle();

  while(1)
  {
    rc = svcConnectToPort(srvHandlePtr, "srv:");
    if (R_LEVEL(rc) != RL_PERMANENT ||
        R_SUMMARY(rc) != RS_NOTFOUND ||
        R_DESCRIPTION(rc) != RD_NOT_FOUND
       ) break;
    svcSleepThread(500000);
  }
  
  if(R_SUCCEEDED(rc))
  {
    rc = srvRegisterClient();
    srvRefCount++;
  }

  RecursiveLock_Unlock(&initLock);
  return rc;
}

Result srvSysExit(void)
{
  Result rc = 0;
  RecursiveLock_Lock(&initLock);

  if(srvRefCount > 1)
  {
    srvRefCount--;
    RecursiveLock_Unlock(&initLock);
    return MAKERESULT(RL_INFO, RS_NOP, 25, RD_BUSY);
  }

  if(srvHandlePtr != 0)
    svcCloseHandle(*srvHandlePtr);
  else
    svcBreak(USERBREAK_ASSERT);

  srvHandlePtr = 0;
  srvRefCount--;

  RecursiveLock_Unlock(&initLock);
  return rc;
}

void fsSysInit(void)
{
	if(R_FAILED(fsregSetupPermissions()))
		svcBreak(USERBREAK_ASSERT);

	Handle *fsHandlePtr = fsGetSessionHandle();
	srvGetServiceHandle(fsHandlePtr, "fs:USER");

	FSUSER_InitializeWithSdkVersion(*fsHandlePtr, SDK_VERSION);
	FSUSER_SetPriority(0);
}
