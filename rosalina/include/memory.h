#pragma once

#include <3ds/types.h>

void memcpy(void *dest, const void *src, u32 size);
int memcmp(const void *buf1, const void *buf2, u32 size);
void *memset_(void *dest, u32 value, u32 size); // thanks binutils for the nice bug involving memset.
void *memset32(void *dest, u32 value, u32 size);
u8 *memsearch(u8 *startPos, const void *pattern, u32 size, u32 patternSize);
char *strncpy(char *dest, const char *src, u32 size);
s32 strnlen(const char *string, s32 maxlen);
s32 strlen(const char *string);
s32 strncmp(const char *str1, const char *str2, u32 size);
const char *strchr(const char *string, int c);
void hexItoa(u64 number, char *out, u32 digits, bool uppercase);
int atoi_(const char *str, int base);