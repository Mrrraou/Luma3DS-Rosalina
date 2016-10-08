#pragma once

#include <3ds/types.h>

#define GPU_FB_BOTTOM_1             (*((vu32*)0x1EF00568))
#define GPU_FB_BOTTOM_1_FMT         (*((vu32*)0x1EF00570))
#define GPU_FB_BOTTOM_1_SEL         (*((vu32*)0x1EF00578))
#define GPU_FB_BOTTOM_1_STRIDE      (*((vu32*)0x1EF00590))

#define FB_BOTTOM_VRAM              ((void*)0x1F48F000)
#define FB_BOTTOM_VRAM_PA           0x1848F000
#define FB_BOTTOM_SIZE              (320 * 240 * 3)

#define SCREEN_BOT_WIDTH  320
#define SCREEN_BOT_HEIGHT 240

#define SPACING_Y 11
#define SPACING_X 6

#define COLOR_TITLE 0xFF9900
#define COLOR_WHITE 0xFFFFFF
#define COLOR_RED   0x0000FF
#define COLOR_BLACK 0x000000

void draw_copyFramebuffer(void *dst);
void draw_copyToFramebuffer(void *src);
void draw_fillFramebuffer(u32 value);
void draw_character(char character, u32 posX, u32 posY, u32 color);
u32 draw_string(char *string, u32 posX, u32 posY, u32 color);

void draw_setupFramebuffer(void);
void draw_restoreFramebuffer(void);
void draw_flushFramebuffer(void);
