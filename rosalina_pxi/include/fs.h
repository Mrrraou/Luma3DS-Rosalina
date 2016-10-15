#pragma once

#include "types.h"

typedef struct FILE_P9
{
    u32 v1;
    u32 v2;
    u32 v3;
    void* p4;
    s64 offset;
    u32 v5;
    u32 v6;
} FILE_P9;

enum FILE_P9_MODE
{
    MODE_READ      = 0b001,
    MODE_WRITE     = 0b010,
    MODE_CREATE    = 0b100
};


static inline void f9_create(FILE_P9 *file)
{
	memset(file, 0, sizeof(FILE_P9));
}


Result (*f9_open)(FILE_P9 *file, wchar_t *path, u32 flag);
Result (*f9_write)(FILE_P9 *file, u32 *written, void *src, u32 size, u32 flag);
Result (*f9_read)(FILE_P9 *file, u32 *read, void *dst, u32 size);
Result (*f9_close)(FILE_P9 *file);
Result (*f9_size)(FILE_P9 *file, s32* size);

//int p9RecvPxi(void);
