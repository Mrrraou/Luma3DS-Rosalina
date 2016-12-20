#include "types.h"
#include "svc.h"
#include "i2c.h"
#include "fs.h"
#include "memory.h"
#include "hid.h"
#include "ipc.h"
#include "utils.h"


void findSubroutines(void)
{
    const u8 f9_closePattern[] = {0x10, 0xB5, 0x04, 0x00};

    struct Pattern {
        void **method;
        const u8 pattern[8];
        bool thumb;
    };

    const struct Pattern patterns[] = {
        {(void**)&f9_open,  {0xF7, 0xB5, 0x84, 0xB0, 0x04, 0x98, 0x0D, 0x00}, true},
        {(void**)&f9_write, {0xFF, 0xB5, 0x85, 0xB0, 0x0E, 0x9F, 0x1C, 0x00}, true},
        {(void**)&f9_read,  {0xFF, 0xB5, 0x07, 0x00, 0x1C, 0x00, 0x00, 0x26}, true},
        /*{&f9_size, {}}*/
    };
    for(u32 i = 0; i < sizeof(patterns) / sizeof(struct Pattern); i++)
        *(patterns[i].method) = (void*)((u32)memsearch((void*)0x08028000, patterns[i].pattern, 0x80000, 8) | patterns[i].thumb); // thumb mode

    u8 *f9_close_off = (u8*)0x08028000;
    do
    {
        f9_close_off = memsearch(f9_close_off, f9_closePattern, 0x80000, 4);
        if(*((u16*)f9_close_off + 3) != 0x2800) // cmp r0, #0
            break;
        f9_close_off++;
    }
    while(true);
    f9_close = (void(*))((u32)f9_close_off | 1);
}

Result starbitHandler(u32 *buffer)
{
    /*FILE_P9 file;
    u32 bytes;

    f9_create(&file);
    f9_open(&file, L"sdmc:/luma/test.bin", MODE_CREATE | MODE_WRITE);
    f9_write(&file, &bytes, "Hello World", 12, 1);
    f9_close(&file);*/

    switch(buffer[0] >> 16)
	{
		// Starbit UserlandBackdoor
		case 0x1000:
			buffer[0] = IPC_MakeHeader(0x1000, 0x1, 0x0);
			return ((u32 (*)(u32*))buffer[1])(buffer);

		// Starbit KernelBackdoor
		case 0x1001:
			buffer[0] = IPC_MakeHeader(0x1001, 0x1, 0x0);
			return svcBackdoor((void*)buffer[1], buffer);

		// Starbit DumpMemory
		case 0x1002:
			buffer[0] = IPC_MakeHeader(0x1002, 0x1, 0x0);
			return 0xdeaddead;

		// Starbit memcpy
		case 0x1003:
		{
			void *dst = (void*)buffer[1];
			void *src = (void*)buffer[2];
			u32  size = buffer[3];
			memcpy(dst, src, size);
			FlushDCacheRange(dst, size);
			InvalidateICacheRange(dst, size);
			buffer[0] = IPC_MakeHeader(0x1003, 0x1, 0x0);
			return 0;
		}

		// Starbit memset
		case 0x1004:
		{
			void *dst  = (void*)buffer[1];
			u8   value = buffer[2] & 0xFF;
			u32  size  = buffer[3];
			memset(dst, value, size);
			FlushDCacheRange(dst, size);
			InvalidateICacheRange(dst, size);
			buffer[0] = IPC_MakeHeader(0x1004, 0x1, 0x0);
			return 0;
		}

		// Starbit WriteBufferToMemory
		case 0x1005:
		{
			void *src = (void*)buffer[2];
			void *dst = (void*)buffer[3];
			u32  size = buffer[4];
			memcpy(dst, src, size);
			FlushDCacheRange(dst, size);
			InvalidateICacheRange(dst, size);
			buffer[0] = IPC_MakeHeader(0x1005, 0x1, 0x0);
			return 0;
		}

		// Starbit ReadMemoryToBuffer
		case 0x1006:
		{
			void *dst = (void*)buffer[2];
			void *src = (void*)buffer[3];
			u32  size = buffer[4];
			memcpy(dst, src, size);
			FlushDCacheRange(dst, size);
			InvalidateICacheRange(dst, size);
			buffer[0] = IPC_MakeHeader(0x1006, 0x1, 0x0);
			return 0;
		}


		// Starbit sha
		case 0x1100:
		{
			void *dst = (void*)buffer[2];
			void *src = (void*)buffer[4];
			u32  size = buffer[5];
			u32  mode = buffer[6];

			sha(dst, src, size, mode);
			buffer[0] = IPC_MakeHeader(0x1100, 0x1, 0x0);
			return 0;
		}

		// Starbit aes
		case 0x1101:
		{
			void *dst = (void*)buffer[2];
			void *src = (void*)buffer[4];
			void *iv  = (void*)buffer[6];
			u32 blockCount = buffer[7];
			u32 mode = buffer[8];
			u32 ivMode = buffer[9];

			aes(dst, src, blockCount, iv, mode, ivMode);
			buffer[0] = IPC_MakeHeader(0x1101, 0x1, 0x0);
			return 0;
		}

		// Starbit aes_advctr
		case 0x1102:
		{
			void *ctr = (void*)buffer[2];
			u32 val = buffer[3];
			u32 mode = buffer[4];

			aes_advctr(ctr, val, mode);
			buffer[0] = IPC_MakeHeader(0x1102, 0x1, 0x0);
			return 0;
		}

		// Unknown command
		default:
			buffer[0] = IPC_MakeHeader(0x0, 0x1, 0x0);
			return 0xD900182F;
	}
}
