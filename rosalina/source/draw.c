#include <3ds.h>
#include "draw.h"
#include "font.h"
#include "memory.h"
#include "menu.h"

static u8 framebuffer_cache[FB_BOTTOM_SIZE];
static u32 gpu_cache_fb, gpu_cache_sel;


void draw_copyFramebuffer(void *dst)
{
    memcpy(dst, FB_BOTTOM_VRAM, FB_BOTTOM_SIZE);
}

void draw_copyToFramebuffer(void *src)
{
    memcpy(FB_BOTTOM_VRAM, src, FB_BOTTOM_SIZE);
}

void draw_fillFramebuffer(u8 value)
{
    memset(FB_BOTTOM_VRAM, value, FB_BOTTOM_SIZE);
}

void draw_character(char character, u32 posX, u32 posY, u32 color)
{
    u8 *const fb = FB_BOTTOM_VRAM;

    s32 y;
    for(y = 0; y < 10; y++)
    {
        char charPos = font[character * 10 + y];

        s32 x;
        for(x = 6; x >= 1; x--)
        {
            u32 screenPos = (posX * SCREEN_BOT_HEIGHT * 3 + (SCREEN_BOT_HEIGHT - y - posY - 1) * 3) + (5 - x) * 3 * SCREEN_BOT_HEIGHT;
            if((charPos >> x) & 1)
            {
                fb[screenPos + 0] = color >> 16;
                fb[screenPos + 1] = color >> 8;
                fb[screenPos + 2] = color;
            }
            else
            {
                fb[screenPos + 0] = COLOR_BLACK >> 16;
                fb[screenPos + 1] = COLOR_BLACK >> 8;
                fb[screenPos + 2] = COLOR_BLACK;
            }
        }
    }
}

u32 draw_string(char *string, u32 posX, u32 posY, u32 color)
{
  for(u32 i = 0, line_i = 0; i < ((u32) strlen(string)); i++)
  {
    if(string[i] == '\n')
    {
      posY += SPACING_Y;
      line_i = 0;
      continue;
    }
    else if(line_i >= (SCREEN_BOT_WIDTH - posX) / SPACING_X)
    {
      // Make sure we never get out of the screen.
      posY += SPACING_Y;
      line_i = 0;
      if(string[i] == ' ')
                continue; // Spaces at the start look weird
    }

    draw_character(string[i], posX + line_i * SPACING_X, posY, color);
    line_i++;
  }

  return posY;
}

void draw_setupFramebuffer(void)
{
    memcpy(framebuffer_cache, FB_BOTTOM_VRAM, FB_BOTTOM_SIZE);
    gpu_cache_fb = GPU_FB_BOTTOM_1;
    gpu_cache_sel = GPU_FB_BOTTOM_1_SEL;

    u32 i;
    for(i = 0; i < 20; i++)
    {
        GPU_FB_BOTTOM_1_SEL &= ~1;
        GPU_FB_BOTTOM_1 = FB_BOTTOM_VRAM_PA;
    }

    draw_flushFramebuffer();
}

void draw_restoreFramebuffer(void)
{
    memcpy(FB_BOTTOM_VRAM, framebuffer_cache, FB_BOTTOM_SIZE);

    u32 i;
    for(i = 0; i < 20; i++)
    {
        GPU_FB_BOTTOM_1 = gpu_cache_fb;
        GPU_FB_BOTTOM_1_SEL = gpu_cache_sel;
    }

    draw_flushFramebuffer();
}

void draw_flushFramebuffer(void)
{
    svcFlushProcessDataCache((Handle)0xFFFF8001, FB_BOTTOM_VRAM, FB_BOTTOM_SIZE);
}
