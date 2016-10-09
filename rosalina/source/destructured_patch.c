#include <3ds.h>
#include "destructured_patch.h"
#include "memory.h"


u32* DestructuredPatch_FindAndApply(DestructuredPatch *patch, u32 *code, u32 codeSize)
{
	u8 *max = (u8*)code + codeSize;
	while((u8*)code + patch->matchSize < max)
	{
		bool foundMatch = true;
		for(u32 i = 0; i < patch->matchItems; i++)
		{
			if(*(u32*)((void*)code + patch->match[i].offset) != patch->match[i].opcode)
			{
				foundMatch = false;
				break;
			}
		}

		if(foundMatch)
		{
			for(u32 i = 0; i < patch->patchItems; i++)
				*(u32*)((void*)code + patch->patch[i].offset) = patch->patch[i].opcode;

			return code;
		}

		code++;
	}

	return NULL;
}
