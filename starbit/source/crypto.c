/*
*   This file is part of Luma3DS
*   Copyright (C) 2016 Aurora Wright, TuxSH
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*   Additional Terms 7.b of GPLv3 applies to this file: Requiring preservation of specified
*   reasonable legal notices or author attributions in that material or in the Appropriate Legal
*   Notices displayed by works containing it.
*/

/*
*   Crypto libs from http://github.com/b1l1s/ctr
*   kernel9Loader code originally adapted from https://github.com/Reisyukaku/ReiNand/blob/228c378255ba693133dec6f3368e14d386f2cde7/source/crypto.c#L233
*   decryptNusFirm code adapted from https://github.com/mid-kid/CakesForeveryWan/blob/master/source/firm.c
*   ctrNandWrite logic adapted from https://github.com/d0k3/GodMode9/blob/master/source/nand/nand.c
*/

#include "crypto.h"
#include "memory.h"

/****************************************************************
*                  Crypto libs
****************************************************************/

/* original version by megazig */

#ifndef __thumb__
#define BSWAP32(x) {\
    __asm__\
    (\
        "eor r1, %1, %1, ror #16\n\t"\
        "bic r1, r1, #0xFF0000\n\t"\
        "mov %0, %1, ror #8\n\t"\
        "eor %0, %0, r1, lsr #8\n\t"\
        :"=r"(x)\
        :"0"(x)\
        :"r1"\
    );\
};

#define ADD_u128_u32(u128_0, u128_1, u128_2, u128_3, u32_0) {\
__asm__\
    (\
        "adds %0, %4\n\t"\
        "addcss %1, %1, #1\n\t"\
        "addcss %2, %2, #1\n\t"\
        "addcs %3, %3, #1\n\t"\
        : "+r"(u128_0), "+r"(u128_1), "+r"(u128_2), "+r"(u128_3)\
        : "r"(u32_0)\
        : "cc"\
    );\
}
#else
#define BSWAP32(x) {x = __builtin_bswap32(x);}

#define ADD_u128_u32(u128_0, u128_1, u128_2, u128_3, u32_0) {\
__asm__\
    (\
        "mov r4, #0\n\t"\
        "add %0, %0, %4\n\t"\
        "adc %1, %1, r4\n\t"\
        "adc %2, %2, r4\n\t"\
        "adc %3, %3, r4\n\t"\
        : "+r"(u128_0), "+r"(u128_1), "+r"(u128_2), "+r"(u128_3)\
        : "r"(u32_0)\
        : "cc", "r4"\
    );\
}
#endif /*__thumb__*/

void aes_setkey(u8 keyslot, const void *key, u32 keyType, u32 mode)
{
    u32 *key32 = (u32 *)key;
    *REG_AESCNT = (*REG_AESCNT & ~(AES_CNT_INPUT_ENDIAN | AES_CNT_INPUT_ORDER)) | mode;

    if(keyslot <= 3)
    {
        if((mode & AES_CNT_INPUT_ORDER) == AES_INPUT_REVERSED)
        {
            REGs_AESTWLKEYS[keyslot][keyType][0] = key32[3];
            REGs_AESTWLKEYS[keyslot][keyType][1] = key32[2];
            REGs_AESTWLKEYS[keyslot][keyType][2] = key32[1];
            REGs_AESTWLKEYS[keyslot][keyType][3] = key32[0];
        }
        else
        {
            REGs_AESTWLKEYS[keyslot][keyType][0] = key32[0];
            REGs_AESTWLKEYS[keyslot][keyType][1] = key32[1];
            REGs_AESTWLKEYS[keyslot][keyType][2] = key32[2];
            REGs_AESTWLKEYS[keyslot][keyType][3] = key32[3];
        }
    }

    else if(keyslot < 0x40)
    {
        *REG_AESKEYCNT = (*REG_AESKEYCNT >> 6 << 6) | keyslot | AES_KEYCNT_WRITE;

        REG_AESKEYFIFO[keyType] = key32[0];
        REG_AESKEYFIFO[keyType] = key32[1];
        REG_AESKEYFIFO[keyType] = key32[2];
        REG_AESKEYFIFO[keyType] = key32[3];
    }
}

void aes_use_keyslot(u8 keyslot)
{
    if(keyslot > 0x3F)
        return;

    *REG_AESKEYSEL = keyslot;
    *REG_AESCNT = *REG_AESCNT | 0x04000000; /* mystery bit */
}

static void aes_setiv(const void *iv, u32 mode)
{
    const u32 *iv32 = (const u32 *)iv;
    *REG_AESCNT = (*REG_AESCNT & ~(AES_CNT_INPUT_ENDIAN | AES_CNT_INPUT_ORDER)) | mode;

    //Word order for IV can't be changed in REG_AESCNT and always default to reversed
    if(mode & AES_INPUT_NORMAL)
    {
        REG_AESCTR[0] = iv32[3];
        REG_AESCTR[1] = iv32[2];
        REG_AESCTR[2] = iv32[1];
        REG_AESCTR[3] = iv32[0];
    }
    else
    {
        REG_AESCTR[0] = iv32[0];
        REG_AESCTR[1] = iv32[1];
        REG_AESCTR[2] = iv32[2];
        REG_AESCTR[3] = iv32[3];
    }
}

static void aes_advctr(void *ctr, u32 val, u32 mode)
{
    u32 *ctr32 = (u32 *)ctr;

    int i;
    if(mode & AES_INPUT_BE)
    {
        for(i = 0; i < 4; ++i) //Endian swap
            BSWAP32(ctr32[i]);
    }

    if(mode & AES_INPUT_NORMAL)
    {
        ADD_u128_u32(ctr32[3], ctr32[2], ctr32[1], ctr32[0], val);
    }
    else
    {
        ADD_u128_u32(ctr32[0], ctr32[1], ctr32[2], ctr32[3], val);
    }

    if(mode & AES_INPUT_BE)
    {
        for(i = 0; i < 4; ++i) //Endian swap
            BSWAP32(ctr32[i]);
    }
}

static void aes_change_ctrmode(void *ctr, u32 fromMode, u32 toMode)
{
    u32 *ctr32 = (u32 *)ctr;
    int i;
    if((fromMode ^ toMode) & AES_CNT_INPUT_ENDIAN)
    {
        for(i = 0; i < 4; ++i)
            BSWAP32(ctr32[i]);
    }

    if((fromMode ^ toMode) & AES_CNT_INPUT_ORDER)
    {
        u32 temp = ctr32[0];
        ctr32[0] = ctr32[3];
        ctr32[3] = temp;

        temp = ctr32[1];
        ctr32[1] = ctr32[2];
        ctr32[2] = temp;
    }
}

static void aes_batch(void *dst, const void *src, u32 blockCount)
{
    *REG_AESBLKCNT = blockCount << 16;
    *REG_AESCNT |=  AES_CNT_START;

    const u32 *src32    = (const u32 *)src;
    u32 *dst32          = (u32 *)dst;

    u32 wbc = blockCount;
    u32 rbc = blockCount;

    while(rbc)
    {
        if(wbc && ((*REG_AESCNT & 0x1F) <= 0xC)) //There's space for at least 4 ints
        {
            *REG_AESWRFIFO = *src32++;
            *REG_AESWRFIFO = *src32++;
            *REG_AESWRFIFO = *src32++;
            *REG_AESWRFIFO = *src32++;
            wbc--;
        }

        if(rbc && ((*REG_AESCNT & (0x1F << 0x5)) >= (0x4 << 0x5))) //At least 4 ints available for read
        {
            *dst32++ = *REG_AESRDFIFO;
            *dst32++ = *REG_AESRDFIFO;
            *dst32++ = *REG_AESRDFIFO;
            *dst32++ = *REG_AESRDFIFO;
            rbc--;
        }
    }
}

void aes(void *dst, const void *src, u32 blockCount, void *iv, u32 mode, u32 ivMode)
{
    *REG_AESCNT =   mode |
                    AES_CNT_INPUT_ORDER | AES_CNT_OUTPUT_ORDER |
                    AES_CNT_INPUT_ENDIAN | AES_CNT_OUTPUT_ENDIAN |
                    AES_CNT_FLUSH_READ | AES_CNT_FLUSH_WRITE;

    u32 blocks;
    while(blockCount != 0)
    {
        if((mode & AES_ALL_MODES) != AES_ECB_ENCRYPT_MODE
        && (mode & AES_ALL_MODES) != AES_ECB_DECRYPT_MODE)
            aes_setiv(iv, ivMode);

        blocks = (blockCount >= 0xFFFF) ? 0xFFFF : blockCount;

        //Save the last block for the next decryption CBC batch's iv
        if((mode & AES_ALL_MODES) == AES_CBC_DECRYPT_MODE)
        {
            memcpy(iv, src + (blocks - 1) * AES_BLOCK_SIZE, AES_BLOCK_SIZE);
            aes_change_ctrmode(iv, AES_INPUT_BE | AES_INPUT_NORMAL, ivMode);
        }

        //Process the current batch
        aes_batch(dst, src, blocks);

        //Save the last block for the next encryption CBC batch's iv
        if((mode & AES_ALL_MODES) == AES_CBC_ENCRYPT_MODE)
        {
            memcpy(iv, dst + (blocks - 1) * AES_BLOCK_SIZE, AES_BLOCK_SIZE);
            aes_change_ctrmode(iv, AES_INPUT_BE | AES_INPUT_NORMAL, ivMode);
        }

        //Advance counter for CTR mode
        else if((mode & AES_ALL_MODES) == AES_CTR_MODE)
            aes_advctr(iv, blocks, ivMode);

        src += blocks * AES_BLOCK_SIZE;
        dst += blocks * AES_BLOCK_SIZE;
        blockCount -= blocks;
    }
}
