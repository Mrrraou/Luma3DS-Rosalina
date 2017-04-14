#include <3ds.h>
#include "memory.h"
#include "services.h"
#include "fsreg.h"
#include "menu.h"
#include "errdisp.h"
#include "hbloader.h"
#include "utils.h"
#include "MyThread.h"
#include "kernel_extension_setup.h"
#include "menus/process_patches.h"

// this is called before main
bool isN3DS;
void __appInit()
{
    srvSysInit();
    fsregInit();
    fsSysInit();

    s64 config = 0;
    svcGetSystemInfo(&config, 0x10000, 2);
    if(((config >> 24) & 1) != 0) // "patch games" option
        ProcessPatches_PatchFS_NoDisplay();

    s64 dummy;
    isN3DS = svcGetSystemInfo(&dummy, 0x10001, 0) == 0;
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
void __libc_init_array(void);
void __libc_fini_array(void);

void __ctru_exit()
{
    __appExit();
    __sync_fini();
    __libc_fini_array();
    svcSleepThread(-1LL); // kernel-loaded sysmodules except PXI are not supposed to terminate anyways
    svcExitProcess();
}


void initSystem()
{
    __libc_init_array();
    installKernelExtension();
    __sync_init();
    __appInit();
}

bool terminationRequest = false;
Handle terminationRequestEvent;

int main(void)
{
    Result res = 0;
    Handle notificationHandle;

    MyThread *menuThread = menuCreateThread(), *errDispThread = errDispCreateThread(), *hbldrThread = hbldrCreateThread();

    if(R_FAILED(srvEnableNotification(&notificationHandle)))
        svcBreak(USERBREAK_ASSERT);

    if(R_FAILED(svcCreateEvent(&terminationRequestEvent, RESET_STICKY)))
        svcBreak(USERBREAK_ASSERT);

    do
    {
        res = svcWaitSynchronization(notificationHandle, -1LL);

        if(R_FAILED(res))
            continue;

        u32 notifId = 0;

        if(R_FAILED(srvReceiveNotification(&notifId)))
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
    MyThread_Join(hbldrThread, -1LL);

    svcCloseHandle(notificationHandle);
    return 0;
}
