#include <3ds.h>
#include "destructured_patch.h"
#include "memory.h"


u8 *DestructuredPatch_FindAndApply(DestructuredPatch *patch, u8 *code, u32 codeSize, bool isThumb)
{
    u8 *max = code + codeSize;
    while(code + patch->matchSize < max)
    {
        bool foundMatch = true;

        for(u32 i = 0; i < patch->matchItems; i++)
        {
            u8 *offset = code + patch->match[i].offset;
            u32 opcode = isThumb ? (u32)*(u16 *)offset : *(u32 *)offset;
            if(opcode != patch->match[i].opcode)
            {
                foundMatch = false;
                break;
            }
        }

        if(foundMatch)
        {
            for(u32 i = 0; i < patch->patchItems; i++)
            {
                if(isThumb)
                    *(u16 *)(code + patch->patch[i].offset) = patch->patch[i].opcode;
                else
                    *(u32 *)(code + patch->patch[i].offset) = patch->patch[i].opcode;
            }
            patch->applied = true;
            return code;
        }

        code += isThumb ? 2 : 4;
    }

    return NULL;
}
