#include <3ds/types.h>
#include "gdb_ctx.h"
#include "memory.h"
#include "macros.h"

#include "xfer_data.h"

int gdb_handle_xfer(Handle sock, struct gdb_client_ctx *c UNUSED, char *buffer)
{
	const char *object_start = buffer;
	char *object_end = (char*)strchr(object_start, ':');
	if(object_end == NULL) return -1;
	*object_end = 0;

	const char *op_start = object_end + 1;
	char *op_end = (char*)strchr(op_start, ':');
	if(op_end == NULL) return -1;
	*op_end = 0;

	const char *annex_start = op_end + 1;
	char *annex_end = (char*)strchr(annex_start, ':');
	if(annex_end == NULL) return -1;
	*annex_end = 0;

	const char *off_start = annex_end + 1;

	if(strncmp(object_start, "features", strlen("features")) == 0 && strncmp(op_start, "read", 4) == 0)
	{
		if(strncmp(annex_start, "target.xml", strlen("target.xml")) == 0)
		{
			char *off_end = (char*)strchr(off_start, ',');
			if(off_end == NULL) return -1;
			*off_end = 0;

			const char *len_start = off_end + 1;

			size_t off = atoi_(off_start, 16);
			size_t len = atoi_(len_start, 16);

			if(len > GDB_BUF_LEN - 5)
			{
				len = GDB_BUF_LEN - 5;
			}

			if(off+len > xfer_target_xml_len)
			{
				len = xfer_target_xml_len - off;
				return gdb_send_packet_prefix(sock, "l", 1, xfer_target_xml_data + off, len);
			}
			else
			{
				return gdb_send_packet_prefix(sock, "m", 1, xfer_target_xml_data + off, len);
			}

			return 0;
		}
	}

	return -1;
}