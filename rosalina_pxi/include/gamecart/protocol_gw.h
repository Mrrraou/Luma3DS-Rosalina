/*
 * Copyright (c) 2015-2016, SALT.
 * This file is licensed under GPLv2 or later
 * See LICENSE.md for terms of use.
 */

#pragma once

#include "types.h"

#include "gamecart/protocol.h"
void ResetCartSlot();
void SwitchToNTRCARD();
void SwitchToCTRCARD();

#include "gamecart/protocol_ntr.h"
#include "gamecart/command_ntr.h"

bool gwcard_init();
void gwcard_deinit();
bool gwcard_init_status();

//Gateway SPI magic data
const u32 gw_magic_size;
const u8 gw_magic[0x1001];

//Supercard DSTWO+ variant
const u32 gw_magic_sc_size;
const u8 gw_magic_sc[0x1001];
