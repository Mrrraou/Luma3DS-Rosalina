#pragma once

#include <3ds/types.h>
#include "exheader.h"

// CFW info
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


/// Initializes Rosalina.
Result rosalinaInit(void);

/// Exits Rosalina.
Result rosalinaExit(void);


/**
 * @brief Gets the CFWInfo struct.
 * @param Output for CFWInfo struct.
 */
void Rosalina_GetCFWInfo(CFWInfo *out);

Handle Rosalina_GiveFSREGHandleCopy(void);

Result Rosalina_Get3DSXInfo(exheader_header *header);
