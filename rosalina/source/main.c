#include <3ds.h>
#include "memory.h"
#include "services.h"
#include "fsreg.h"
#include "menu.h"
#include "utils.h"
#include "MyThread.h"
#include "kernel_extension.h"

// this is called before main
void __appInit()
{
  srvSysInit();
  fsregInit();
  fsSysInit();
}

// this is called after main exits
void __appExit()
{
  fsExit();
  fsregExit();
  srvSysExit();
}


Result __sync_init(void);
Result __sync_fini(void);

void __ctru_exit()
{
  __appExit();
  __sync_fini();
  svcExitProcess();
}

void initSystem()
{
    installKernelExtension();

  __sync_init();
  __appInit();
}


int main(void)
{
    Result ret = 0;
    Handle notificationHandle;
    bool terminationRequest = false;

    MyThread *t = menuCreateThread();

    if(R_FAILED(srvEnableNotification(&notificationHandle)))
        svcBreak(USERBREAK_ASSERT);

    do
    {
        svcWaitSynchronization(notificationHandle, -1LL);
        u32 notifId = 0;

        if(R_FAILED(ret = srvReceiveNotification(&notifId)))
          svcBreak(USERBREAK_ASSERT);

        if(notifId == 0x100)
        // Termination request
          terminationRequest = true;
    }
    while(!terminationRequest);

    MyThread_Join(t, -1LL);
    svcCloseHandle(notificationHandle);
    return 0;
}
