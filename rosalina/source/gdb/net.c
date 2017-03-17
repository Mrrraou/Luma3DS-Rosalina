#include "gdb/net.h"
#include <stdarg.h>
#include "fmt.h"
#include "minisoc.h"

u8 GDB_ComputeChecksum(const char *packetData, u32 len)
{
    u8 cksum = 0;
    for(u32 i = 0; i < len; i++)
        cksum += (u8)packetData[i];

    return cksum;
}

void GDB_EncodeHex(char *dst, const void *src, u32 len)
{
    static const char *alphabet = "0123456789abcdef";
    const u8 *src8 = (u8 *)src;

    for(u32 i = 0; i < len; i++)
    {
        dst[2 * i] = alphabet[(src8[i] & 0xf0) >> 4];
        dst[2 * i + 1] = alphabet[src8[i] & 0x0f];
    }
}

static inline u32 GDB_DecodeHexDigit(char src, bool *ok)
{
    *ok = true;
    if(src >= '0' && src <= '9') return src - '0';
    else if(src >= 'a' && src <= 'f') return 0xA + (src - 'a');
    else if(src >= 'A' && src <= 'F') return 0xA + (src - 'A');
    else
    {
        *ok = false;
        return 0;
    }
}

int GDB_DecodeHex(void *dst, const char *src, u32 len)
{
    u32 i = 0;
    bool ok = true;
    u8 *dst8 = (u8 *)dst;
    for(i = 0; i < len && ok && src[2 * i] != 0 && src[2 * i + 1] != 0; i++)
        dst8[i] = (GDB_DecodeHexDigit(src[2 * i], &ok) << 4) | GDB_DecodeHexDigit(src[2 * i + 1], &ok);

    return (!ok) ? i - 1 : i;
}

int GDB_UnescapeBinaryData(void *dst, const void *src, u32 len)
{
    u8 *dst8 = (u8 *)dst;
    const u8 *src8 = (const u8 *)src;

    for(u32 i = 0; i < len; i++)
    {
        if(*src8 == '}')
        {
            src8++;
            *dst8++ = *src8++ ^ 0x20;
        }
        else
            *dst8++ = *src8++;
    }

    return len;
}

int GDB_SendPacket(GDBContext *ctx, const char *packetData, u32 len)
{
    ctx->buffer[0] = '$';

    memcpy(ctx->buffer + 1, packetData, len);

    char *checksumLoc = ctx->buffer + len + 1;
    *checksumLoc++ = '#';

    hexItoa(GDB_ComputeChecksum(packetData, len), checksumLoc, 2, false);
    return soc_send(ctx->super.sock, ctx->buffer, 4 + len, 0);
}

int GDB_SendFormattedPacket(GDBContext *ctx, const char *packetDataFmt, ...)
{
    // It goes without saying you shouldn't use that with user-controlled data...
    char buf[GDB_BUF_LEN + 1];
    va_list args;
    va_start(args, packetDataFmt);
    int n = vsprintf(buf, packetDataFmt, args);
    va_end(args);

    if(n < 0) return n;
    else return GDB_SendPacket(ctx, buf, (u32)n);
}

int GDB_SendHexPacket(GDBContext *ctx, const void *packetData, u32 len)
{
    if(4 + 2 * len > GDB_BUF_LEN)
        return -1;

    ctx->buffer[0] = '$';
    GDB_EncodeHex(ctx->buffer + 1, packetData, len);

    char *checksumLoc = ctx->buffer + 2 * len + 1;
    *checksumLoc++ = '#';

    hexItoa(GDB_ComputeChecksum(ctx->buffer + 1, 2 * len), checksumLoc, 2, false);
    return soc_send(ctx->super.sock, ctx->buffer, 4 + 2 * len, 0);
}

int GDB_SendDebugString(GDBContext *ctx, const char *fmt, ...) // unsecure
{
    if(ctx->state == GDB_STATE_CLOSING || !(ctx->flags & GDB_FLAG_PROCESS_CONTINUING))
        return 0;

    char formatted[(GDB_BUF_LEN - 1) / 2 + 1];
    ctx->buffer[0] = '$';
    ctx->buffer[1] = 'O';

    va_list args;
    va_start(args, fmt);
    int n = vsprintf(formatted, fmt, args);
    va_end(args);

    if(n <= 0) return n;
    GDB_EncodeHex(ctx->buffer + 2, formatted, 2 * n);

    char *checksumLoc = ctx->buffer + 2 * n + 2;
    *checksumLoc++ = '#';

    hexItoa(GDB_ComputeChecksum(ctx->buffer + 1, 2 * n + 1), checksumLoc, 2, false);
    return soc_send(ctx->super.sock, ctx->buffer, 5 + 2 * n, 0);
}

int GDB_ReplyEmpty(GDBContext *ctx)
{
    return soc_send(ctx->super.sock, "$#00", 4, 0);
}

int GDB_ReplyOk(GDBContext *ctx)
{
    return soc_send(ctx->super.sock, "$OK#9a", 6, 0);
}

int GDB_ReplyErrno(GDBContext *ctx, int no)
{
    char buf[] = "E01";
    hexItoa(no, buf + 1, 2, false);
    return GDB_SendPacket(ctx, buf, 3);
}
