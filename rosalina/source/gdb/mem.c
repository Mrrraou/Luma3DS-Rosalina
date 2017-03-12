#include "gdb/mem.h"
#include "gdb/net.h"
#include "minisoc.h"

int GDB_SendProcessMemory(GDBContext *ctx, const char *prefix, u32 prefixLen, u32 addr, u32 len)
{
    char buff[256];
    Result r = 0;
    u8 cksum = 0;

    // Send packet header.
    soc_send(ctx->socketCtx.sock, "$", 1, 0);
    soc_send(ctx->socketCtx.sock, prefix, prefixLen, 0);
    u32 total = len;

    while(len != 0 && R_SUCCEEDED(r))
    {
        u32 readLen = len;
        if(readLen > 256)
            readLen = 256;

        r = svcReadProcessMemory(buff, ctx->debug, addr, len);
        if(R_FAILED(r))
        {
            if(len == total)
            {
                memcpy(buff, "E01", 3);
                readLen = 3;
            }
            else
                readLen = 0;
        }

        len -= readLen;
        cksum += GDB_ComputeChecksum(buff, readLen);
        GDB_EncodeHex(ctx->buffer, buff, readLen);
        soc_send(ctx->socketCtx.sock, ctx->buffer, 2 * readLen, 0);
    }

    // Send packet trailer.
    char s[] = "#00";
    hexItoa(cksum, s + 1, 2, false);
    soc_send(ctx->socketCtx.sock, s, 3, 0);

    return 4 + (len == total) ? 3 : total - len;
}

GDB_DECLARE_HANDLER(ReadMemory)
{
    const char *addr_start = ctx->commandData;
    char *addr_end = (char*)strchr(addr_start, ',');
    if(addr_end == NULL) return -1;

    *addr_end = 0;
    const char *len_start = addr_end + 1;
    u32 addr = atoi_(addr_start, 16);
    u32 len = atoi_(len_start, 16);

    if(addr < 0x1000)
        return GDB_ReplyErrno(ctx, EPERM);

    return GDB_SendProcessMemory(ctx, NULL, 0, addr, len);
}
