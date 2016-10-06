#include <3ds.h>
#include "memory.h"
#include "services.h"
#include "fsreg.h"
#include "luma.h"
#include "port.h"
#include "menu.h"

#define USED_HANDLES 2
#define MAX_SESSIONS 8
#define MAX_HANDLES  (USED_HANDLES + MAX_SESSIONS)

static Handle handles[MAX_HANDLES];
static Handle client_handle;
static int active_handles;


int main()
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

// stubs for non-needed pre-main functions
void __sync_init(void);
void __sync_fini(void);
void __system_initSyscalls(void);

void __ctru_exit(void)
{
  __appExit();
  __sync_fini();
  svcExitProcess();
}

extern u8* fake_heap_start;
extern u8* fake_heap_end;
u32 __ctru_heap;
u32 __ctru_heap_size;
u32 __ctru_linear_heap;
u32 __ctru_linear_heap_size;

void __system_allocateHeaps(void)
{
    u32 tmp=0;

    // Distribute available memory, aligning to page size.
    u32 size = (0x50000/*osGetMemRegionFree(MEMREGION_BASE) / 32*/) & 0xFFFFF000;
    __ctru_heap_size = size;
    __ctru_linear_heap_size = size;

    // Allocate the application heap
    __ctru_heap = 0x08000000;
    svcControlMemory(&tmp, __ctru_heap, 0x0, __ctru_heap_size, MEMOP_ALLOC, MEMPERM_READ | MEMPERM_WRITE);

    // Allocate the linear heap
    svcControlMemory(&__ctru_linear_heap, 0x0, 0x0, __ctru_linear_heap_size, MEMOP_ALLOC_LINEAR, MEMPERM_READ | MEMPERM_WRITE);

    // Set up newlib heap
    fake_heap_start = (u8*) __ctru_heap;
    fake_heap_end = fake_heap_start + __ctru_heap_size;
}

void initSystem(void)
{
    __sync_init();
    __system_initSyscalls();
    __system_allocateHeaps();
    __appInit();
}
