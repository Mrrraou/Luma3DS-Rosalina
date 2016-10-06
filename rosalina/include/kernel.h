#pragma once

#include <3ds/types.h>

typedef struct KLinkedListNode
{
    struct KLinkedListNode *next;
    struct KLinkedListNode *prev;
    void *key;
} KLinkedListNode;

typedef struct KLinkedList
{
    u32 size;
    KLinkedListNode *first;
    KLinkedListNode *last;
} KLinkedList;

typedef struct KAutoObject
{
    void *vtable;
    u32 refCount;
} ALIGN(4) KAutoObject;

typedef struct KSynchronizationObject
{
    KAutoObject autoObject;
    KLinkedListNode syncedThreads;
} KSynchronizationObject;

typedef struct KThread
{
    KSynchronizationObject syncObject;
    void **interruptEventVtable;
    void *waitingOn_1;
    s64 timerSuspended;
    u8 unnamed_1[8];
    u8 unnamed_2[8];
    u8 signalScheduling;
    u8 syncing_1;
    u8 syncing_2;
    u8 syncing_3;
    void *debugThread;
    u32 basePriority;
    u32 waitingOn_2;
    u32 resultObjectWaited_2;
    struct KThread **doubleKThreadPtr;
    u32 arbitrationAddr;
    u32 field_50;
    u32 field_54;
    u32 field_58;
    void *mutex_1;
    KLinkedList mutexesUsed;
    u32 dynamicPriority;
    u32 createdByProcessor;
    u32 unknown_1;
    u8 gap78[4];
    u8 unknown_2;
    u8 unknown_3;
    u8 affinityMask;
    struct ALIGN(2) KProcess *ownerProcess;
    u32 threadId;
    void *svcRegisterStorage;
    void *endOfThreadContext;
    u32 idealProcessor;
    void *threadLocalStorage;
    void *threadLocalStorageVmem;
    struct ALIGN(8) KThread *prevThread;
    struct KThread *nextThread;
    void *linkedList;
    u8 gapAC[3];
    u8 unused;
} PACKED ALIGN(4) KThread;

typedef struct KCodeSetMemDescriptor
{
    u32 vaStart;
    u32 totalPages;
    KLinkedList blocks;
} KCodeSetMemDescriptor;

typedef struct KCodeSet
{
    KAutoObject autoObject;
    KCodeSetMemDescriptor textSection;
    KCodeSetMemDescriptor rodataSection;
    KCodeSetMemDescriptor dataSection;
    u32 textPages;
    u32 rodataPages;
    u32 rwPages;
    char processName[8];
    u16 unknown_1;
    u16 unknown_2;
    u64 titleId;
} PACKED ALIGN(4) KCodeSet;

typedef struct HandleDescriptor
{
    u32 info;
    void *pointer;
} HandleDescriptor;

typedef struct KProcessHandleTable
{
    HandleDescriptor *handleTable;
    u16 maxHandleCount;
    u16 highestHandleCount;
    HandleDescriptor *nextOpenHandleDescriptor;
    u16 totalHandlesUsed;
    u16 handlesInUseCount;
    KThread *runningThread;
    u16 errorTracker;
    HandleDescriptor internalTable[40];
} KProcessHandleTable;

typedef struct KProcess
{
    KSynchronizationObject syncObject;
    u32 unknown_1;
    u32 unknown_2;
    KThread *runningThread;
    u16 errorTracker;
    u8 ALIGN(4) tlbEntriesCore0;
    u8 ALIGN(4) tlbEntriesCore1;
    u8 ALIGN(4) tlbEntriesCore2;
    u8 ALIGN(4) tlbEntriesCore3;
    KLinkedList ownedKMemoryBlocks;
    u32 unknown_3;
    u32 unknown_4;
    void *translationTableBase;
    u8 contextId;
    u8 memAllocRelated;
    bool currentlyLoadedApp;
    u32 unknown_5;
    void *endOfUserlandVmem;
    void *linearVAUserlandBase;
    u32 unknown_6;
    u32 mmuTableSize;
    void *mmuTableVA;
    u32 totalThreadContextSize;
    struct KLinkedList threadLocalPages;
    u32 unknown_7;
    u32 idealProcessor;
    void *debug;
    void *resourceLimits;
    u8 status;
    u8 affinityMask;
    u16 ALIGN(4) threadCount;
    u16 maxThreadCount;
    u8 svcAccessControlMask[16];
    u32 interruptFlags[4];
    u32 kernelFlags;
    u16 handleTableSize;
    u16 kernelReleaseVersion;
    KCodeSet *codeSet;
    u32 processId;
    u64 creationTime;
    KThread *mainThread;
    u32 interruptEnabledFlags[4];
    KProcessHandleTable handleTable;
    u8 gap234[52];
    u64 unused;
} KProcess;

static inline void *KProcess_ConvertHandle(KProcess *process, Handle handle)
{
    switch(handle)
    {
        case 0xFFFF8000: return (void *)*(KThread**)0xFFFF9000;
        case 0xFFFF8001: return (void *)*(KProcess**)0xFFFF9004;
        default:
            return process->handleTable.handleTable[handle & 0x7fff].pointer;
    }
}
