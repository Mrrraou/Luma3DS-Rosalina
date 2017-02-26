#include <3ds.h>
#include "minisoc.h"
#include "gdb_ctx.h"
#include "draw.h"
#include "memory.h"

uint8_t gdb_cksum(const char *pkt_data, size_t len)
{
	uint8_t cksum = 0;
	for(size_t i = 0; i < len; i++)
	{
		cksum += (uint8_t)pkt_data[i];
	}
	return cksum;
}

void gdb_hex_encode(char *dst, const char *src, size_t len) // Len is in bytes.
{
	const char *alphabet = "0123456789abcdef";
	for(size_t i = 0; i < len; i++)
	{
		dst[i*2] = alphabet[(src[i] & 0xf0) >> 4];
		dst[i*2+1] = alphabet[src[i] & 0x0f];
	}
}

int gdb_send_packet(Handle socket, const char *pkt_data, size_t len)
{
	gdb_buffer[0] = '$';
	memcpy(gdb_buffer + 1, pkt_data, len);

	char *cksum_loc = gdb_buffer + len + 1;
	*cksum_loc++ = '#';

	hexItoa(gdb_cksum(pkt_data, len), cksum_loc, 2, false);

	return soc_send(socket, gdb_buffer, len+4, 0);
}

int gdb_send_packet_prefix(Handle socket, const char *prefix, size_t prefix_len, const char *pkt_data, size_t len)
{
	if(prefix_len + len + 4 > GDB_BUF_LEN) return -1; // Too long!

	gdb_buffer[0] = '$';

	memcpy(gdb_buffer + 1, prefix, prefix_len);
	memcpy(gdb_buffer + prefix_len + 1, pkt_data, len);

	uint8_t total_cksum = gdb_cksum(prefix, prefix_len);
	total_cksum += gdb_cksum(pkt_data, len);

	char *cksum_loc = gdb_buffer + len + prefix_len + 1;
	*cksum_loc++ = '#';

	hexItoa(total_cksum, cksum_loc, 2, false);

	return soc_send(socket, gdb_buffer, prefix_len+len+4, 0);
}

int gdb_send_packet_hex(Handle socket, const char *pkt_data, size_t len)
{
	if(len*2 + 4 > GDB_BUF_LEN) return -1; // Too long!

	gdb_buffer[0] = '$';

	gdb_hex_encode(gdb_buffer + 1, pkt_data, len);

	char *cksum_loc = gdb_buffer + len*2 + 1;
	*cksum_loc++ = '#';

	hexItoa(gdb_cksum(pkt_data, len), cksum_loc, 2, false);

	return soc_send(socket, gdb_buffer, len*2+4, 0);
}

