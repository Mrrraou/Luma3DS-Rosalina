#pragma once

#include <3ds/types.h>
#include "utils.h"

#define GPU_FB_TOP_1             REG32(0x10400468)
#define GPU_FB_TOP_1_FMT         REG32(0x10400470)
#define GPU_FB_TOP_1_SEL         REG32(0x10400478)
#define GPU_FB_TOP_1_STRIDE      REG32(0x10400490)

#define GPU_FB_BOTTOM_1             REG32(0x10400568)
#define GPU_FB_BOTTOM_1_FMT         REG32(0x10400570)
#define GPU_FB_BOTTOM_1_SEL         REG32(0x10400578)
#define GPU_FB_BOTTOM_1_STRIDE      REG32(0x10400590)

#define FB_BOTTOM_VRAM              ((void *)0x1F48F000) // cached
#define FB_BOTTOM_VRAM_PA           0x1848F000
#define FB_BOTTOM_SIZE              (320 * 240 * 2)

#define SCREEN_BOT_WIDTH  320
#define SCREEN_BOT_HEIGHT 240

#define SPACING_Y 11
#define SPACING_X 6

#define MAKE_COLOR_RGB565(R, G, B) (((R) << 11)| ((G) << 5) | (B))
#define COLOR_TITLE MAKE_COLOR_RGB565(0x00, 0x26, 0x1F) //0x04DF //0xFF9900
#define COLOR_WHITE MAKE_COLOR_RGB565(0x1F, 0x3F, 0x1F)
#define COLOR_RED   MAKE_COLOR_RGB565(0x1F, 0x00, 0x00)
#define COLOR_BLACK MAKE_COLOR_RGB565(0x00, 0x00, 0x00)

void draw_lock(void);
void draw_unlock(void);

void draw_copyFramebuffer(void *dst);
void draw_copyToFramebuffer(const void *src);
void draw_fillFramebuffer(u32 value);
void draw_character(char character, u32 posX, u32 posY, u32 color);
u32 draw_string(const char *string, u32 posX, u32 posY, u32 color);

static inline void draw_clearFramebuffer(void)
{
    draw_fillFramebuffer(0);
}

void draw_setupFramebuffer(void);
void draw_restoreFramebuffer(void);
void draw_flushFramebuffer(void);

void createBitmapHeader(u8 *dst, u32 width, u32 heigth);
u8 *convertFrameBufferLine(bool top, u32 y);
