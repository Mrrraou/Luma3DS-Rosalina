#pragma once

#include <3ds/types.h>

#define CONFIG_VERSIONMAJOR 1
#define CONFIG_VERSIONMINOR 0


typedef struct __attribute__((packed))
{
  char magic[4];

  u8 versionMajor;
  u8 versionMinor;
  u8 versionBuild;
  u8 flags; /* bit 0: dev branch; bit 1: is release */

  u32 commitHash;

  u32 config;
} CFWInfo;

typedef struct __attribute__((packed))
{
  char magic[4];
  u16 formatVersionMajor, formatVersionMinor;

  u32 config;
} ConfigData;


CFWInfo Luma_GetCFWInfo(void);
ConfigData Luma_GetCFWConfig(void);
void Luma_InitCFWInfo(void);
void Luma_ReadConfig(const char *configPath);
