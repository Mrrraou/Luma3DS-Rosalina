#include "memory.h"
#include "utils.h"

void *memcpy(void *dest, const void *src, u32 size)
{
    u8 *destc = (u8 *)dest;
    const u8 *srcc = (const u8 *)src;

    for(u32 i = 0; i < size; i++)
        destc[i] = srcc[i];

    return dest;
}

int memcmp(const void *buf1, const void *buf2, u32 size)
{
    const u8 *buf1c = (const u8 *)buf1;
    const u8 *buf2c = (const u8 *)buf2;

    for(u32 i = 0; i < size; i++)
    {
        int cmp = buf1c[i] - buf2c[i];
        if(cmp)
          return cmp;
    }

    return 0;
}

void *memset(void *dest, u32 value, u32 size)
{
    u8 *destc = (u8 *)dest;

    for(u32 i = 0; i < size; i++) destc[i] = (u8)value;

    return dest;
}

void *memset32(void *dest, u32 value, u32 size)
{
    u32 *dest32 = (u32 *)dest;

    for(u32 i = 0; i < size/4; i++) dest32[i] = value;

    return dest;
}

//Quick Search algorithm, adapted from http://igm.univ-mlv.fr/~lecroq/string/node19.html#SECTION00190
u8 *memsearch(u8 *startPos, const void *pattern, u32 size, u32 patternSize)
{
    const u8 *patternc = (const u8 *)pattern;

    //Preprocessing
    u32 table[256];

    for(u32 i = 0; i < 256; ++i)
        table[i] = patternSize + 1;
    for(u32 i = 0; i < patternSize; ++i)
        table[patternc[i]] = patternSize - i;

    //Searching
    u32 j = 0;

    while(j <= size - patternSize)
    {
        if(memcmp(patternc, startPos + j, patternSize) == 0)
            return startPos + j;
        j += table[startPos[j + patternSize]];
    }

    return NULL;
}

char *strncpy(char *dest, const char *src, u32 size)
{
    for(u32 i = 0; i < size && src[i] != 0; i++)
        dest[i] = src[i];

    return dest;
}

s32 strnlen(const char *string, s32 maxlen)
{
    s32 size;
    for(size = 0; *string && size < maxlen; string++, size++);

    return size;
}

s32 strlen(const char *string)
{
    char *stringEnd = (char *)string;
    while(*stringEnd) stringEnd++;

    return stringEnd - string;
}

s32 strcmp(const char *str1, const char *str2)
{
    while(*str1 && (*str1 == *str2))
    {
        str1++;
        str2++;
    }

    return *str1 - *str2;
}

s32 strncmp(const char *str1, const char *str2, u32 size)
{
    while(size && *str1 && (*str1 == *str2))
    {
        str1++;
        str2++;
        size--;
    }
    if (!size)
        return 0;
    else
        return *str1 - *str2;
}

void hexItoa(u64 number, char *out, u32 digits, bool uppercase)
{
    const char hexDigits[] = "0123456789ABCDEF";
    const char hexDigitsLowercase[] = "0123456789abcdef";
    u32 i = 0;

    while(number > 0)
    {
        out[digits - 1 - i++] = uppercase ? hexDigits[number & 0xF] : hexDigitsLowercase[number & 0xF];
        number >>= 4;
    }

    while(i < digits) out[digits - 1 - i++] = '0';
}
