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
    const char *addrStart = ctx->commandData;
    char *addrEnd = (char*)strchr(addrStart, ',');
    if(addrEnd == NULL) return -1;

    *addrEnd = 0;
    const char *lenStart = addrEnd + 1;
    u32 addr = atoi_(addrStart, 16);
    u32 len = atoi_(lenStart, 16);

    return GDB_SendProcessMemory(ctx, NULL, 0, addr, len);
}

GDB_DECLARE_HANDLER(WriteMemory)
{
    char *addrStart = ctx->commandData;
    char *addrEnd = (char*)strchr(addrStart, ',');
    if(addrEnd == NULL) return -1;

    char *lenStart = addrEnd + 1;
    char *lenEnd = (char*)strchr(lenStart, ':');
    char *dataStart = lenEnd + 1;

    *addrEnd = 0;
    *lenEnd = 0;
    u32 addr = atoi_(addrStart, 16);
    u32 len = atoi_(lenStart, 16);

    if(dataStart + 2 * len >= ctx->buffer + 4 + GDB_BUF_LEN)
        return GDB_ReplyErrno(ctx, ENOMEM);

    u8 data[GDB_BUF_LEN / 2];
    len = GDB_DecodeHex(data, dataStart, len);

    return GDB_WriteProcessMemory(ctx, data, addr, len);
}

GDB_DECLARE_HANDLER(WriteMemoryRaw)
{
    char *addrStart = ctx->commandData;
    char *addrEnd = (char*)strchr(addrStart, ',');
    if(addrEnd == NULL) return -1;

    char *lenStart = addrEnd + 1;
    char *lenEnd = (char*)strchr(lenStart, ':');
    char *dataStart = lenEnd + 1;

    *addrEnd = 0;
    *lenEnd = 0;
    u32 addr = atoi_(addrStart, 16);
    u32 len = atoi_(lenStart, 16);

    if(dataStart + 2 * len >= ctx->buffer + 4 + GDB_BUF_LEN)
        return GDB_ReplyErrno(ctx, ENOMEM);

    u8 data[GDB_BUF_LEN / 2];
    GDB_UnescapeBinaryData(data, dataStart, len);

    return GDB_WriteProcessMemory(ctx, data, addr, len);
}
