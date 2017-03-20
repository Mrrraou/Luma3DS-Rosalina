#pragma once

#include "gdb.h"
#include <errno.h>

u8 GDB_ComputeChecksum(const char *packetData, u32 len);
void GDB_EncodeHex(char *dst, const void *src, u32 len);
int GDB_DecodeHex(void *dst, const char *src, u32 len);
int GDB_UnescapeBinaryData(void *dst, const void *src, u32 len);
int GDB_ReceivePacket(GDBContext *ctx);
int GDB_SendPacket(GDBContext *ctx, const char *packetData, u32 len);
int GDB_SendFormattedPacket(GDBContext *ctx, const char *packetDataFmt, ...);
int GDB_SendHexPacket(GDBContext *ctx, const void *packetData, u32 len);
int GDB_SendDebugString(GDBContext *ctx, const char *fmt, ...); // unsecure
int GDB_ReplyEmpty(GDBContext *ctx);
int GDB_ReplyOk(GDBContext *ctx);
int GDB_ReplyErrno(GDBContext *ctx, int no);
