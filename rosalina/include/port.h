#pragma once

#include <3ds/types.h>
#include "3dsx.h"

#define ROUND_UPPER(x,y) ((x / y) + (x % y ? 1 : 0))


void handle_commands(void);

void load_3dsx_header(Header_3DSX *header);
void load_3dsx_as_process(void);
void open_3dsx_file(void);
void close_3dsx_file(void);
