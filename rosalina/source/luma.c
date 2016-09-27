#include <stdio.h>

#include <3ds.h>
#include "luma.h"
#include "memory.h"
#include "ifile.h"

static bool isCFWInfoInited;
static CFWInfo cfwinfo;
static ConfigData cfwconfig;


CFWInfo Luma_GetCFWInfo(void)
{
	if(!isCFWInfoInited)
		Luma_InitCFWInfo();
	return cfwinfo;
}

ConfigData Luma_GetCFWConfig(void)
{
	if(!isCFWInfoInited)
		Luma_InitCFWInfo();
	return cfwconfig;
}

void Luma_InitCFWInfo(void)
{
	Luma_ReadConfig("/luma/config.bin");

	memcpy(cfwinfo.magic, "LUMA", 4);

	const char *rev = REVISION;
	bool isRelease;

	cfwinfo.commitHash = COMMIT_HASH;
	cfwinfo.config = cfwconfig.config;
	cfwinfo.versionMajor = (u8)(rev[1] - '0');
	cfwinfo.versionMinor = (u8)(rev[3] - '0');
	if(rev[4] == '.')
	{
		cfwinfo.versionBuild = (u8)(rev[5] - '0');
		isRelease = rev[6] == 0;
	}
	else
		isRelease = rev[4] == 0;

	cfwinfo.flags = 1 /* dev branch */ | ((isRelease ? 1 : 0) << 1) /* is release */;

	isCFWInfoInited = true;
}

void Luma_ReadConfig(const char *configPath)
{
	u64 total;
	IFile configfile;
	FS_Path filePath 		= {PATH_ASCII, strnlen(configPath, PATH_MAX) + 1, configPath},
					archivePath = {PATH_EMPTY, 1, (u8*) ""};
	Result res;

	if(R_FAILED(res = IFile_Open(&configfile, ARCHIVE_SDMC, archivePath, filePath, FS_OPEN_READ)))
	{
		cfwconfig.config = res;
		return;
	}
	if(R_FAILED(res = IFile_Read(&configfile, &total, &cfwconfig, sizeof(ConfigData))))
	{
		cfwconfig.config = res;
		IFile_Close(&configfile);
		return;
	}
	IFile_Close(&configfile);

  if(memcmp(cfwconfig.magic, "CONF", 4) != 0
	|| cfwconfig.formatVersionMajor != CONFIG_VERSIONMAJOR
	|| cfwconfig.formatVersionMinor != CONFIG_VERSIONMINOR)
  {
    cfwconfig.config = 0;
  }
}
