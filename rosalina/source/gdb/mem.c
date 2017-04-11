#include "gdb/mem.h"
#include "gdb/net.h"
#include "utils.h"

extern u32 TTBCR;

static void *k_memcpy_no_interrupt(void *dst, const void *src, u32 len)
{
    __asm__ volatile("cpsid aif");
    return memcpy(dst, src, len);
}

Result GDB_ReadMemoryInPage(void *out, GDBContext *ctx, u32 addr, u32 len)
{
    if(addr < (1u << (32 - TTBCR)))
        return svcReadProcessMemory(out, ctx->debug, addr, len);
    else if(addr >= 0x80000000 && addr < 0xB0000000)
    {
        memcpy(out, (const void *)addr, len);
        return 0;
    }
    else
    {
        void *PA = svcConvertVAToPA(NULL, (const void *)addr, false);

        if(PA == NULL)
            return -1;
        else
        {
            svcCustomBackdoor(k_memcpy_no_interrupt, out, (const void *)addr, len);
            return 0;
        }
    }
}

Result GDB_WriteMemoryInPage(GDBContext *ctx, const void *in, u32 addr, u32 len)
{
    if(addr < (1u << (32 - TTBCR))) // not sure if it checks if it's IO or not. It probably does
        return svcWriteProcessMemory(ctx->debug, in, addr, len);
    else if(addr >= 0x80000000 && addr < 0xB0000000)
    {
        memcpy((void *)addr, in, len);
        return 0;
    }
    else
    {
        void *PA = svcConvertVAToPA(NULL, (const void *)addr, true);

        if(PA != NULL)
        {
            svcCustomBackdoor(k_memcpy_no_interrupt, (void *)addr, in, len);
            return 0;
        }
        else
        {
            // Unreliable, use at your own risk

            svcFlushEntireDataCache();
            svcInvalidateEntireInstructionCache();
            Result ret = GDB_WriteMemoryInPage(ctx, PA_FROM_VA_PTR(in), addr, len);
            svcFlushEntireDataCache();
            svcInvalidateEntireInstructionCache();
            return ret;
        }
    }
}

int GDB_SendMemory(GDBContext *ctx, const char *prefix, u32 prefixLen, u32 addr, u32 len)
{
    char buf[GDB_BUF_LEN];
    u8 membuf[GDB_BUF_LEN / 2];

    if(prefix != NULL)
        memcpy(buf, prefix, prefixLen);
    else
        prefixLen = 0;

    if(prefixLen + 2 * len > GDB_BUF_LEN) // gdb shouldn't send requests which responses don't fit in a packet
        return prefix == NULL ? GDB_ReplyErrno(ctx, ENOMEM) : -1;

    Result r = 0;
    u32 remaining = len, total = 0;
    do
    {
        u32 nb = (remaining > 0x1000 - (addr & 0xFFF)) ? 0x1000 - (addr & 0xFFF) : remaining;
        r = GDB_ReadMemoryInPage(membuf + total, ctx, addr, nb);
        if(R_SUCCEEDED(r))
        {
            addr += nb;
            total += nb;
            remaining -= nb;
        }
    }
    while(remaining > 0 && R_SUCCEEDED(r));

    if(total == 0)
        return prefix == NULL ? GDB_ReplyErrno(ctx, EFAULT) : -EFAULT;
    else
    {
        GDB_EncodeHex(buf + prefixLen, membuf, len);
        return GDB_SendPacket(ctx, buf, prefixLen + 2 * len);
    }
}

int GDB_WriteMemory(GDBContext *ctx, const void *buf, u32 addr, u32 len)
{
    Result r = 0;
    u32 remaining = len, total = 0;
    do
    {
        u32 nb = (remaining > 0x1000 - (addr & 0xFFF)) ? 0x1000 - (addr & 0xFFF) : remaining;
        r = GDB_WriteMemoryInPage(ctx, (u8 *)buf + total, addr, nb);
        if(R_SUCCEEDED(r))
        {
            addr += nb;
            total += nb;
            remaining -= nb;
        }
    }
    while(remaining > 0 && R_SUCCEEDED(r));

    if(R_FAILED(r))
        return GDB_ReplyErrno(ctx, EFAULT);
    else
        return GDB_ReplyOk(ctx);
}

GDB_DECLARE_HANDLER(ReadMemory)
{
    u32 lst[2];
    if(GDB_ParseHexIntegerList(lst, ctx->commandData, 2, 0) == NULL)
        return GDB_ReplyErrno(ctx, EILSEQ);

    u32 addr = lst[0];
    u32 len = lst[1];

    return GDB_SendMemory(ctx, NULL, 0, addr, len);
}

GDB_DECLARE_HANDLER(WriteMemory)
{
    u32 lst[2];
    const char *dataStart = GDB_ParseHexIntegerList(lst, ctx->commandData, 2, ':');
    if(dataStart == NULL || *dataStart != ':')
        return GDB_ReplyErrno(ctx, EILSEQ);

    dataStart++;
    u32 addr = lst[0];
    u32 len = lst[1];

    if(dataStart + 2 * len >= ctx->buffer + 4 + GDB_BUF_LEN)
        return GDB_ReplyErrno(ctx, ENOMEM);

    u8 data[GDB_BUF_LEN / 2];
    int n = GDB_DecodeHex(data, dataStart, len);

    if((u32)n != len)
        return GDB_ReplyErrno(ctx, EILSEQ);

    return GDB_WriteMemory(ctx, data, addr, len);
}

GDB_DECLARE_HANDLER(WriteMemoryRaw)
{
    u32 lst[2];
    const char *dataStart = GDB_ParseHexIntegerList(lst, ctx->commandData, 2, ':');
    if(dataStart == NULL || *dataStart != ':')
        return GDB_ReplyErrno(ctx, EILSEQ);

    dataStart++;
    u32 addr = lst[0];
    u32 len = lst[1];

    if(dataStart + len >= ctx->buffer + 4 + GDB_BUF_LEN)
        return GDB_ReplyErrno(ctx, ENOMEM);

    u8 data[GDB_BUF_LEN];
    int n = GDB_UnescapeBinaryData(data, dataStart, len);

    if((u32)n != len)
        return GDB_ReplyErrno(ctx, n);

    return GDB_WriteMemory(ctx, data, addr, len);
}
