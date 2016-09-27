#pragma once

#include <3ds/types.h>

typedef struct
{
	char magic[4];

	u16 header_size, reloc_header_size;
	u32 version, flags;

	u32 code_size, rodata_size, databss_size, bss_size;
} PACKED Header_3DSX;

typedef struct
{
	u32 SMDH_offset, SMDH_size;
	u32 RomFS_offset;
} PACKED Exheader_3DSX;

typedef struct
{
	u32 absolute_relocs, relative_relocs;
} PACKED RelocHeader_3DSX;

typedef struct
{
	u16 skip_words, patch_words;
} PACKED Relocation_3DSX;
