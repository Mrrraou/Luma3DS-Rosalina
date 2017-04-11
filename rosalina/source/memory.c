#include "memory.h"

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
    for(size = 0; size < maxlen && *string; string++, size++);

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
        return *(u8*)str1 - *(u8*)str2;
}

const char *strchr(const char *string, int c)
{
    char *stringEnd = (char *)string;
    while(*stringEnd != 0)
    {
        if(*stringEnd == c) return stringEnd;
        stringEnd++;
    }

    return NULL;
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


// Copied from newlib, without the reent stuff + some other stuff
unsigned long int xstrtoul(const char *nptr, char **endptr, int base, bool allowPrefix, bool *ok)
{
    register const unsigned char *s = (const unsigned char *)nptr;
    register unsigned long acc;
    register int c;
    register unsigned long cutoff;
    register int neg = 0, any, cutlim;

    if(ok != NULL)
        *ok = true;
    /*
     * See strtol for comments as to the logic used.
     */
    do {
        c = *s++;
    } while ((c >= 9 && c <= 13) || c == ' ');
    if (c == '-') {
        if(!allowPrefix) {
            if(ok != NULL)
                *ok = false;
            return 0;
        }
        neg = 1;
        c = *s++;
    } else if (c == '+'){
        if(!allowPrefix) {
            if(ok != NULL)
                *ok = false;
            return 0;
        }

        c = *s++;
    }
    if ((base == 0 || base == 16) &&
        c == '0' && (*s == 'x' || *s == 'X')) {

        if(!allowPrefix) {
            if(ok != NULL)
                *ok = false;
            return 0;
        }
        c = s[1];
        s += 2;
        base = 16;
    }
    if (base == 0) {
        if(!allowPrefix) {
            if(ok != NULL)
                *ok = false;
            return 0;
        }

        base = c == '0' ? 8 : 10;
    }
    cutoff = (unsigned long)(-1) / (unsigned long)base;
    cutlim = (unsigned long)(-1) % (unsigned long)base;
    for (acc = 0, any = 0;; c = *s++) {
        if (c >= '0' && c <= '9')
            c -= '0';
        else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
            c -= c >= 'A' && c <= 'Z' ? 'A' - 10 : 'a' - 10;
        else
            break;
        if (c >= base)
            break;
               if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
            any = -1;
        else {
            any = 1;
            acc *= base;
            acc += c;
        }
    }
    if (any < 0) {
        acc = (unsigned long)-1;
        if(ok != NULL)
            *ok = false;
//        rptr->_errno = ERANGE;
    } else if (neg)
        acc = -acc;
    if (endptr != 0)
        *endptr = (char *) (any ? (char *)s - 1 : nptr);
    return (acc);
}
