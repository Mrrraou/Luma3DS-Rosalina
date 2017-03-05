#include <3ds.h>
#include "memory.h"
#include "services.h"
#include "fsreg.h"
#include "menu.h"
#include "errdisp.h"
#include "utils.h"
#include "MyThread.h"
#include "kernel_extension.h"
#include "menus/process_patches.h"

// this is called before main
void __appInit()
{
    srvSysInit();
    fsregInit();
    fsSysInit();

    s64 config = 0;
    svcGetSystemInfo(&config, 0x10000, 2);
    if(((config >> 24) & 1) != 0) // "patch games" option
        ProcessPatches_PatchFS_NoDisplay();
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

bool terminationRequest = false;
Handle terminationRequestEvent;

int main(void)
{
    Result ret = 0;
    Handle notificationHandle;

    MyThread *menuThread = menuCreateThread(), *errDispThread = errDispCreateThread();

    if(R_FAILED(srvEnableNotification(&notificationHandle)))
        svcBreak(USERBREAK_ASSERT);

    if(R_FAILED(svcCreateEvent(&terminationRequestEvent, RESET_STICKY)))
        svcBreak(USERBREAK_ASSERT);

    do
    {
        svcWaitSynchronization(notificationHandle, -1LL);
        u32 notifId = 0;

        if(R_FAILED(ret = srvReceiveNotification(&notifId)))
          svcBreak(USERBREAK_ASSERT);

        if(notifId == 0x100)
        {
            // Termination request
            terminationRequest = true;
            svcSignalEvent(terminationRequestEvent);
        }
    }
    while(!terminationRequest);

    MyThread_Join(menuThread, -1LL);
    MyThread_Join(errDispThread, -1LL);
    svcCloseHandle(notificationHandle);
    return 0;
}
