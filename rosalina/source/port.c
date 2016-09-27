#include <3ds.h>
#include "port.h"
#include "memory.h"
#include "luma.h"
#include "fsreg.h"
#include "ifile.h"
#include "3dsx.h"
#include "exheader.h"
#include "menu.h"

static char path_3dsx[PATH_MAX] = "/3ds/prboom.3dsx";
static IFile ifile_3dsx;


void handle_commands(void)
{
  u32* cmdbuf;
  u16 cmdid;

  cmdbuf = getThreadCommandBuffer();
  cmdid = cmdbuf[0] >> 16;
  switch(cmdid)
  {
		// Rosalina GetCFWInfo
    case 1:
    {
      cmdbuf[0] = IPC_MakeHeader(1, 1 + (sizeof(CFWInfo) / sizeof(u32)), 0);
      *((CFWInfo*) &cmdbuf[1]) = Luma_GetCFWInfo();
      break;
    }

		// Rosalina GiveFSREGHandleCopy (dirty)
    case 1300:
    {
      Handle fsreg;
      svcDuplicateHandle(&fsreg, fsregGetHandle());

      cmdbuf[0] = IPC_MakeHeader(1301, 2, 0);
      cmdbuf[1] = IPC_Desc_MoveHandles(1);
      cmdbuf[2] = fsreg;
      break;
    }

		// Rosalina Load3DSX (dirty)
		// Used for 3dsx loading on a TID, called by loader
    case 1301:
    {
      cmdbuf[0] = IPC_MakeHeader(1302, 1, 1);

			load_3dsx_as_process();
      break;
    }

		// Rosalina Get3DSXInfo (dirty)
		// Gets info from the 3DSX exheader.
		case 1304:
		{
			exheader_header exheader;
			Header_3DSX header;

			load_3dsx_header(&header);

			memcpy(&exheader.codesetinfo.name, "3DSX", 4);
			exheader.codesetinfo.flags.flag = 1 << 1; // SD Application

			exheader.codesetinfo.text.address = 0x100000;
			//exheader.codesetinfo.text.nummaxpages = *ROUND_UPPER(header.code_size, MEMORY_PAGESIZE);
			exheader.codesetinfo.text.codesize = header.code_size;
			exheader.codesetinfo.ro.address = exheader.codesetinfo.text.address + header.code_size;
			exheader.codesetinfo.ro.codesize = header.rodata_size;
			exheader.codesetinfo.data.address = exheader.codesetinfo.ro.address + header.rodata_size;
			exheader.codesetinfo.data.codesize = header.databss_size - header.bss_size;
			exheader.codesetinfo.bsssize = header.bss_size;

			exheader.arm11systemlocalcaps.programid = 0x000400000F006900;
			exheader.arm11systemlocalcaps.firm = 0x00000002;
		}

		// Rosalina GetDebugInfo (for debugging purposes)
    case 1337:
    {
      u32 reply_length = 2;

      Handle fsreg;
      svcDuplicateHandle(&fsreg, fsregGetHandle());

      cmdbuf[reply_length++] = IPC_Desc_MoveHandles(1);
      cmdbuf[reply_length++] = fsreg;

      cmdbuf[reply_length++] = HID_PAD;

      cmdbuf[0] = IPC_MakeHeader(1337, reply_length - 2, 4);
      cmdbuf[1] = reply_length - 2;
      break;
    }

		// Error
    default:
    {
      cmdbuf[0] = 0x40;
      cmdbuf[1] = 0xD900182F;
      break;
    }
  }
}


void load_3dsx_header(Header_3DSX *header)
{
	u64 total;
	open_3dsx_file();

	if(R_FAILED(IFile_Read(&ifile_3dsx, &total, header, sizeof(Header_3DSX))))
		svcBreak(USERBREAK_ASSERT);
}

void load_3dsx_as_process(void)
{
	Header_3DSX header;

	open_3dsx_file();
	load_3dsx_header(&header);
}

void open_3dsx_file(void)
{
	if(ifile_3dsx.handle)
		return;

	FS_Path filePath 		= {PATH_ASCII, strnlen(path_3dsx, PATH_MAX) + 1, path_3dsx},
					archivePath = {PATH_EMPTY, 1, (u8*) ""};

	if(R_FAILED(IFile_Open(&ifile_3dsx, ARCHIVE_SDMC, archivePath, filePath, FS_OPEN_READ)))
		svcBreak(USERBREAK_ASSERT);
}

void close_3dsx_file(void)
{
	IFile_Close(&ifile_3dsx);
	ifile_3dsx.handle = 0;
}
