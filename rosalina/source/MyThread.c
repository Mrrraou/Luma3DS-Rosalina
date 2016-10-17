#include "MyThread.h"
#include "memory.h"

static void _thread_begin(void* arg)
{
    MyThread *t = (MyThread *)arg;
    t->ep();
    MyThread_Exit();
}

Result MyThread_Create(MyThread *t, void (*entrypoint)(void), void *stack, u32 stackSize, int prio, int affinity)
{
    t->ep       = entrypoint;
    t->stacktop = (u8 *)stack + stackSize;

    return svcCreateThread(&t->handle, _thread_begin, (u32)t, (u32*)t->stacktop, prio, affinity);
}

Result MyThread_Join(MyThread *thread, s64 timeout_ns)
{
    if (thread == NULL) return 0;
    Result res = svcWaitSynchronization(thread->handle, timeout_ns);
    if(R_FAILED(res)) return res;

    svcCloseHandle(thread->handle);
    thread->handle = (Handle)0;

    return res;
}

void MyThread_Exit(void)
{
    svcExitThread();
}

extern bool isN3DS;
Result runOnAllCores(void (*entrypoint)(void), void *stack, u32 stackSize, int prio)
{
    u32 nbCores = isN3DS ? 4 : 2;
    MyThread threads[nbCores];
    Result res = 0;

    for(u32 i = 0; i < nbCores; i++)
    {
        if(i == 1) continue;
        res = MyThread_Create(&threads[i], entrypoint, stack + stackSize * (i + 1), stackSize, prio, i);
        if(R_FAILED(res)) return res;
    }

    entrypoint();

    for(u32 i = 0; i < nbCores; i++)
    {
        if(i == 1) continue;
        res = MyThread_Join(&threads[i], -1LL);
        if(R_FAILED(res)) return res;
    }

    return res;
}
