#pragma once

#include <3ds/types.h>

struct KLinkedListNode
{
  struct KLinkedListNode *next;
  struct KLinkedListNode *prev;
  void *key;
};

struct KLinkedList
{
	u32 size;
  struct KLinkedListNode *first;
  struct KLinkedListNode *last;
};

struct __attribute__((aligned(4))) KAutoObject
{
  void *vtable;
  u32 refCount;
};

struct KSynchronizationObject
{
  struct KAutoObject autoObject;
  struct KLinkedListNode syncedThreads;
};

struct __attribute__((packed)) __attribute__((aligned(4))) KThread
{
  struct KSynchronizationObject syncObject;
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
  struct KLinkedList mutexesUsed;
  u32 dynamicPriority;
  u32 createdByProcessor;
  u32 unknown_1;
  u8 gap78[4];
  u8 unknown_2;
  u8 unknown_3;
  u8 affinityMask;
  __attribute__((aligned(2))) struct KProcess *ownerProcess;
  u32 threadId;
  void *svcRegisterStorage;
  void *endOfThreadContext;
  u32 idealProcessor;
  void *threadLocalStorage;
  void *threadLocalStorageVmem;
  struct __attribute__((aligned(8))) KThread *prevThread;
  struct KThread *nextThread;
  void *linkedList;
  u8 gapAC[3];
  u8 unused;
};

struct KCodeSetMemDescriptor
{
  u32 vaStart;
  u32 totalPages;
  struct KLinkedList blocks;
};

struct __attribute__((packed)) __attribute__((aligned(4))) KCodeSet
{
  struct KAutoObject autoObject;
  struct KCodeSetMemDescriptor textSection;
  struct KCodeSetMemDescriptor rodataSection;
  struct KCodeSetMemDescriptor dataSection;
  u32 textPages;
  u32 rodataPages;
  u32 rwPages;
  char processName[8];
  u16 unknown_1;
  u16 unknown_2;
  u64 titleId;
};

struct HandleDescriptor
{
  int info;
  void *pointer;
};

struct KProcessHandleTable
{
  struct HandleDescriptor *handleTable;
  u16 maxHandleCount;
  u16 highestHandleCount;
  struct HandleDescriptor *nextOpenHandleDescriptor;
  u16 totalHandlesUsed;
  u16 handlesInUseCount;
  struct KThread *runningThread;
  u16 errorTracker;
  struct HandleDescriptor internalTable[40];
};

struct KProcess
{
  struct KSynchronizationObject syncObject;
  u32 unknown_1;
  u32 unknown_2;
  struct KThread *runningThread;
  u16 errorTracker;
  __attribute__((aligned(4))) u8 tlbEntriesCore0;
  __attribute__((aligned(4))) u8 tlbEntriesCore1;
  __attribute__((aligned(4))) u8 tlbEntriesCore2;
  __attribute__((aligned(4))) u8 tlbEntriesCore3;
  struct KLinkedList ownedKMemoryBlocks;
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
  __attribute__((aligned(4))) u16 threadCount;
  u16 maxThreadCount;
  u8 svcAccessControlMask[16];
  u32 interruptFlags[4];
  u32 kernelFlags;
  u16 handleTableSize;
  u16 kernelReleaseVersion;
  struct KCodeSet *codeSet;
  u32 processId;
  u64 creationTime;
  struct KThread *mainThread;
  u32 interruptEnabledFlags[4];
  struct KProcessHandleTable handleTable;
  u8 gap234[52];
  u64 unused;
};
