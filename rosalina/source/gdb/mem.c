#include "gdb/mem.h"
#include "gdb/net.h"
#include "minisoc.h"

int GDB_SendProcessMemory(GDBContext *ctx, const char *prefix, u32 prefixLen, u32 addr, u32 len)
{
    char buf[GDB_BUF_LEN];
    char membuf[GDB_BUF_LEN / 2];

    if(prefix != NULL)
        memcpy(buf, prefix, prefixLen);
    else
        prefixLen = 0;

    if(prefixLen + 2 * len > GDB_BUF_LEN) // gdb shouldn't send requests which responses don't fit in a packet
        return prefix == NULL ? GDB_ReplyErrno(ctx, ENOMEM) : -1;

    Result r = svcReadProcessMemory(membuf, ctx->debug, addr, len);
    if(R_FAILED(r))
    {
        // if the requested memory is split inside two pages, with the second one being not accessible...
        u32 newlen = 0x1000 - (addr & 0xFFF);
        if(newlen >= len || R_FAILED(svcReadProcessMemory(membuf, ctx->debug, addr, newlen)))
            return prefix == NULL ? GDB_ReplyErrno(ctx, EFAULT) : -EFAULT;
        else
        {
            GDB_EncodeHex(buf + prefixLen, membuf, newlen);
            return GDB_SendPacket(ctx, buf, prefixLen + 2 * newlen);
        }
    }
    else
    {
        GDB_EncodeHex(buf + prefixLen, membuf, len);
        return GDB_SendPacket(ctx, buf, prefixLen + 2 * len);
    }
}

int GDB_WriteProcessMemory(GDBContext *ctx, const void *buf, u32 addr, u32 len)
{
    Result r = svcWriteProcessMemory(ctx->debug, buf, addr, len);
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

    return GDB_SendProcessMemory(ctx, NULL, 0, addr, len);
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

    return GDB_WriteProcessMemory(ctx, data, addr, len);
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

    return GDB_WriteProcessMemory(ctx, data, addr, len);
}
