#include "gdb/xfer.h"
#include "gdb/net.h"

static const char xferTargetXmlData[] = "<?xml version=\"1.0\"?>\n\
<!DOCTYPE target SYSTEM \"gdb-target.dtd\">\n\
<target version=\"1.0\">\n\
  <feature name=\"org.gnu.gdb.arm.core\">\n\
    <reg name=\"r0\" bitsize=\"32\"/>\n\
    <reg name=\"r1\" bitsize=\"32\"/>\n\
    <reg name=\"r2\" bitsize=\"32\"/>\n\
    <reg name=\"r3\" bitsize=\"32\"/>\n\
    <reg name=\"r4\" bitsize=\"32\"/>\n\
    <reg name=\"r5\" bitsize=\"32\"/>\n\
    <reg name=\"r6\" bitsize=\"32\"/>\n\
    <reg name=\"r7\" bitsize=\"32\"/>\n\
    <reg name=\"r8\" bitsize=\"32\"/>\n\
    <reg name=\"r9\" bitsize=\"32\"/>\n\
    <reg name=\"r10\" bitsize=\"32\"/>\n\
    <reg name=\"r11\" bitsize=\"32\"/>\n\
    <reg name=\"r12\" bitsize=\"32\"/>\n\
    <reg name=\"sp\" bitsize=\"32\" type=\"data_ptr\"/>\n\
    <reg name=\"lr\" bitsize=\"32\"/>\n\
    <reg name=\"pc\" bitsize=\"32\" type=\"code_ptr\"/>\n\
    <!-- The CPSR is register 25, rather than register 16, because\n\
         the FPA registers historically were placed between the PC\n\
         and the CPSR in the \"g\" packet.  -->\n\
    <reg name=\"cpsr\" bitsize=\"32\" regnum=\"25\"/>\n\
  </feature>\n\
  <feature name=\"org.gnu.gdb.arm.vfp\">\n\
    <reg name=\"d0\" bitsize=\"64\" type=\"float\"/>\n\
    <reg name=\"d1\" bitsize=\"64\" type=\"float\"/>\n\
    <reg name=\"d2\" bitsize=\"64\" type=\"float\"/>\n\
    <reg name=\"d3\" bitsize=\"64\" type=\"float\"/>\n\
    <reg name=\"d4\" bitsize=\"64\" type=\"float\"/>\n\
    <reg name=\"d5\" bitsize=\"64\" type=\"float\"/>\n\
    <reg name=\"d6\" bitsize=\"64\" type=\"float\"/>\n\
    <reg name=\"d7\" bitsize=\"64\" type=\"float\"/>\n\
    <reg name=\"d8\" bitsize=\"64\" type=\"float\"/>\n\
    <reg name=\"d9\" bitsize=\"64\" type=\"float\"/>\n\
    <reg name=\"d10\" bitsize=\"64\" type=\"float\"/>\n\
    <reg name=\"d11\" bitsize=\"64\" type=\"float\"/>\n\
    <reg name=\"d12\" bitsize=\"64\" type=\"float\"/>\n\
    <reg name=\"d13\" bitsize=\"64\" type=\"float\"/>\n\
    <reg name=\"d14\" bitsize=\"64\" type=\"float\"/>\n\
    <reg name=\"d15\" bitsize=\"64\" type=\"float\"/>\n\
    <reg name=\"fpscr\" bitsize=\"32\" type=\"int\" group=\"float\"/>\n\
    <reg name=\"fpexc\" bitsize=\"32\" type=\"int\" group=\"float\"/>\n\
  </feature>\n\
</target>";

static const u32 xferTargetXmlLen = sizeof(xferTargetXmlData) - 1;

GDB_DECLARE_QUERY_HANDLER(Xfer)
{
    const char *objectStart = ctx->commandData;
    char *objectEnd = (char*)strchr(objectStart, ':');
    if(objectEnd == NULL) return -1;
    *objectEnd = 0;

    char *opStart = objectEnd + 1;
    char *opEnd = (char*)strchr(opStart, ':');
    if(opEnd == NULL) return -1;
    *opEnd = 0;

    char *annexStart = opEnd + 1;
    char *annexEnd = (char*)strchr(annexStart, ':');
    if(annexEnd == NULL) return -1;
    *annexEnd = 0;

    const char *offStart = annexEnd + 1;
    char buf[GDB_BUF_LEN];

    if(strncmp(objectStart, "features", strlen("features")) == 0 && strncmp(opStart, "read", 4) == 0)
    {
        if(strncmp(annexStart, "target.xml", strlen("target.xml")) == 0)
        {
            u32 lst[2];
            if(GDB_ParseHexIntegerList(lst, offStart, 2, 0) == NULL)
                return GDB_ReplyErrno(ctx, EILSEQ);

            u32 off = lst[0];
            u32 len = lst[1];

            if(len > GDB_BUF_LEN - 5)
                len = GDB_BUF_LEN - 5;

            if(off + len > xferTargetXmlLen)
            {
                len = xferTargetXmlLen - off;
                buf[0] = 'l';
                memcpy(buf + 1, xferTargetXmlData + off, len);
                return GDB_SendPacket(ctx, buf, 1 + len);
            }
            else
            {
                buf[0] = 'm';
                memcpy(buf + 1, xferTargetXmlData + off, len);
                return GDB_SendPacket(ctx, buf, 1 + len);
            }

            return 0;
        }
    }

    return -1;
}
