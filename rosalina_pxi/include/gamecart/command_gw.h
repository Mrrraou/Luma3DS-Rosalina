/*
 * Copyright (c) 2015-2016, SALT.
 * This file is licensed under GPLv2 or later
 * See LICENSE.md for terms of use.
 */

#ifndef _COMMAND_GW_H
#define _COMMAND_GW_H

#include "types.h"

#include "gamecart/protocol.h"
#include "gamecart/protocol_ctr.h"

void gwcard_cmd_led(u32 color);
void gwcard_cmd_read(u32 sector, u32 length, u32 blocks, void* buffer);
void gwcard_cmd_write(u32 sector, u32 blocks, void* buffer);

#endif
