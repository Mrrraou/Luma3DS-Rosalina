#pragma once

#include <3ds/types.h>
#include "MyThread.h"

MyThread *hbldrCreateThread(void);

void hbldrThreadMain(void);
