#include <3ds.h>
#include "memory.h"
#include "services.h"
#include "fsreg.h"
#include "luma.h"
#include "port.h"
#include "menu.h"
#include "utils.h"
#include "MyThread.h"
#include "mmu.h"

#define USED_HANDLES 2
#define MAX_SESSIONS 8
#define MAX_HANDLES  (USED_HANDLES + MAX_SESSIONS)

static Handle handles[MAX_HANDLES];
static Handle client_handle;
static int active_handles;

bool isN3DS;
u8 *vramKMapping, *dspAndAxiWramMapping, *fcramKMapping;
static void K_InitMappingInfo(void)
{
    isN3DS = convertVAToPA((void*)0xE9000000) != 0; // check if there's the extra FCRAM
    if(convertVAToPA((void *)0xF0000000) != 0)
    {
        // Older mappings
        fcramKMapping = (u8*)0xF0000000;
        vramKMapping = (u8*)0xE8000000;
        dspAndAxiWramMapping = (u8*)0xEFF00000;
    }
    else
    {
        // Newer mappings
        fcramKMapping = (u8*)0xE0000000;
        vramKMapping = (u8*)0xD8000000;
        dspAndAxiWramMapping = (u8*)0xDFF00000;
    }
}

static void K_PatchDebugMode(void)
{
    /*
    vu8 *debugEnabled = (vu8*)0xFFF2E00A;
    *debugEnabled = true; // enables DebugActiveProcess and other debug SVCs
    */
    // Actually, nevermind... The mapping for 0xFFF2E000 differs between kernel versions,
    // and probably differs between consoles too. Causes a data abort if not mapped,
    // of course...
}

static void K_ConfigureAndSendSGI0ToAllCores(void)
{
    // see /patches/k11MainHook.s
    u32 *off;
    // 68 64 6C 72 = "hdlr"

    for(off = (u32 *)0xFFFF0000; off < (u32 *)0xFFFF1000 && *off != 0x726C6468; off++);

    // Caches? What are caches?
    *(volatile void **)PA_FROM_VA_PTR(off) = PA_FROM_VA_PTR(mapKernelExtensionAndSetupExceptionHandlers);
    *(volatile void **)PA_FROM_VA_PTR(off + 1) = PA_FROM_VA_PTR(L2MMUTableFor0x40000000);
    *(volatile void **)PA_FROM_VA_PTR(off + 2) = PA_FROM_VA_PTR(flushEntireCaches);

    *(vu32 *)PA_PTR(0x17E00000 + 0x1000 + 0xF00) = 0xF0000; // http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0360f/CACGDJJC.html
}

static void reconfigureMMUAndInstallHandlers(void)
{
    svc_7b(constructL2TranslationTableForRosalina);
    svc_7b_interrupts_enabled(K_ConfigureAndSendSGI0ToAllCores);
}

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
  svc_7b(K_InitMappingInfo);
  svc_7b(K_PatchDebugMode);
  reconfigureMMUAndInstallHandlers();

  __sync_init();
  __appInit();
}


int main(void)
{
  Result ret = 0;
  Handle *notificationHandle = &handles[0];
  Handle *serverHandle = &handles[1];
  s32 index = 1;
  bool terminationRequest = false;

  menuCreateThread();

  if(R_FAILED(svcCreatePort(serverHandle, &client_handle, "Rosalina", MAX_SESSIONS)))
    svcBreak(USERBREAK_ASSERT);

  if(R_FAILED(srvEnableNotification(notificationHandle)))
    svcBreak(USERBREAK_ASSERT);

  active_handles = USED_HANDLES;

  Handle handle = 0, reply_target = 0;
  u32* cmdbuf;
  do
  {
    if(reply_target == 0)
    // Don't send any command; wait for one
    {
      cmdbuf = getThreadCommandBuffer();
      cmdbuf[0] = 0xFFFF0000;
    }

    ret = svcReplyAndReceive(&index, handles, active_handles, reply_target);

    if(R_FAILED(ret))
    {
      switch(ret)
      {
        case 0xC920181A:
        // Closed session
        {
          if(index == -1)
          // If the index of the closed session handle is not given by the
          // kernel, get it ourselves
          {
            for(int i = USED_HANDLES; i < MAX_HANDLES; i++)
            {
              if(handles[i] == reply_target)
              {
                index = i;
                break;
              }
            }
          }

          svcCloseHandle(handles[index]);
          handles[index] = handles[--active_handles];
          reply_target = 0;
          break;
        }

        default:
        // Unhandled error
        {
          svcBreak(USERBREAK_ASSERT);
          break;
        }
      }
      continue;
    }

    reply_target = 0;
    switch(index)
    {
      case 0:
      // SM notification (Notification handle)
      {
        u32 notifId = 0;
        Result ret = 0;

        if(R_FAILED(ret = srvReceiveNotification(&notifId)))
          svcBreak(USERBREAK_ASSERT);

        if(notifId == 0x100)
        // Termination request
          terminationRequest = true;

        break;
      }

      case 1:
      // New session (Server handle)
      {
        if(R_FAILED(svcAcceptSession(&handle, *serverHandle)))
          svcBreak(USERBREAK_ASSERT);

        if(active_handles < MAX_HANDLES)
          handles[active_handles++] = handle;
        else
          svcCloseHandle(handle);
        break;
      }

      default:
      // Got command (Session handles)
      {
        handle_commands();
        reply_target = handles[index];
        break;
      }
    }
  }
  while(!terminationRequest || active_handles != USED_HANDLES);

  svcCloseHandle(*notificationHandle);
  svcCloseHandle(client_handle);
  svcCloseHandle(*serverHandle);
  return 0;
}
