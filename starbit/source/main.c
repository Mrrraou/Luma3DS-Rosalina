#include "types.h"
#include "svc.h"
#include "memory.h"
#include "ipc.h"

#define CUR_PROCESS_HANDLE ((Handle)0xFFFF8001)

Result starbitHandler(u32 *buffer)
{
    switch(buffer[0] >> 16)
	{
		// Starbit Backdoor
		case 0x1000:
			buffer[0] = IPC_MakeHeader(0x1000, 0x1, 0x0);
            invalidateInstructionCacheRange(buffer[1], 0x4000); // invalidate entire instruction cache
			return ((Result (*)(u32 *))buffer[1])(buffer);

		// Starbit memcpy
		case 0x1001:
		{
			void *dst = (void *)buffer[1];
			void *src = (void *)buffer[2];
			u32  size = buffer[3];
            svcInvalidateProcessDataCache(CUR_PROCESS_HANDLE, src, size);
			memcpy(dst, src, size);
			svcFlushProcessDataCache(CUR_PROCESS_HANDLE, dst, size);
			buffer[0] = IPC_MakeHeader(0x1001, 0x1, 0x0);
			return 0;
		}

		// Starbit memset
		case 0x1002:
		{
			void *dst  = (void *)buffer[1];
			u8   value = buffer[2] & 0xFF;
			u32  size  = buffer[3];
			memset(dst, value, size);
			svcFlushProcessDataCache(CUR_PROCESS_HANDLE, dst, size);
			buffer[0] = IPC_MakeHeader(0x1002, 0x1, 0x0);
			return 0;
		}

		case 0x1101:
		{
            u8 keyslot = (u8)buffer[1];
            if(buffer[1] >= 0x40)
            {
                buffer[0] = IPC_MakeHeader(0x1101, 0x1, 0x0);
                return 0xD8E043ED;
            }

			void *dst = (void *)buffer[2];
			void *src = (void *)buffer[3];
			void *iv  = (void *)buffer[4]; // <- input and output IV (for further chaining)
			u32 blockCount = buffer[5];
			u32 mode = buffer[6];
			u32 ivMode = buffer[7];
            void *key = (void *)buffer[8];
            void *keyX = (void *)buffer[9];
            void *keyY = (void *)buffer[10];

            AES__Acquire();

            svcInvalidateProcessDataCache(CUR_PROCESS_HANDLE, src, blockCount << 4);
            svcInvalidateProcessDataCache(CUR_PROCESS_HANDLE, iv, 16);

            if(key != NULL)
            {
                svcInvalidateProcessDataCache(CUR_PROCESS_HANDLE, key, 16);
                aes_setkey(keyslot, key, AES_KEYNORMAL, AES_INPUT_BE | AES_INPUT_NORMAL);
            }

            else if(keyX != NULL)
            {
                svcInvalidateProcessDataCache(CUR_PROCESS_HANDLE, keyX, 16);
                aes_setkey(keyslot, keyX, AES_KEYX, AES_INPUT_BE | AES_INPUT_NORMAL);
            }

            else if(keyY != NULL)
            {
                svcInvalidateProcessDataCache(CUR_PROCESS_HANDLE, keyY, 16);
                aes_setkey(keyslot, keyY, AES_KEYX, AES_INPUT_BE | AES_INPUT_NORMAL);
            }

            aes_use_keyslot(keyslot);
			aes(dst, src, blockCount, iv, mode, ivMode);

            svcFlushProcessDataCache(CUR_PROCESS_HANDLE, dst, blockCount << 4);
            svcFlushProcessDataCache(CUR_PROCESS_HANDLE, iv, 16);

			buffer[0] = IPC_MakeHeader(0x1101, 0x1, 0x0);
			return 0;

            AES__Release();
		}

		// Unknown command
		default:
			buffer[0] = IPC_MakeHeader(0x0, 0x1, 0x0);
			return 0xD900182F;
	}
}
