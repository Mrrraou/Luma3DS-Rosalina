#pragma once

#include <3ds/types.h>


typedef struct DestructuredPatchItem {
	u32 opcode;
	u32 offset;
} DestructuredPatchItem;

typedef struct DestructuredPatch {
	u32 matchSize;
	u32 matchItems;
	DestructuredPatchItem match[0x10];
	u32 patchItems;
	DestructuredPatchItem patch[0x10];
} DestructuredPatch;


u32* DestructuredPatch_FindAndApply(DestructuredPatch *patch, u32 *code, u32 codeSize);
