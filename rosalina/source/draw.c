#include <3ds.h>
#include "draw.h"
#include "font.h"
#include "memory.h"
#include "menu.h"
#include "utils.h"

u8 framebuffer_cache[FB_TOP_SIZE]; // 'top' because screenshots
static u32 gpu_cache_fb, gpu_cache_sel, gpu_cache_fmt, gpu_cache_stride;


void draw_copyFramebuffer(void *dst)
{
    memcpy(dst, FB_BOTTOM_VRAM, FB_BOTTOM_SIZE);
}

void draw_copyToFramebuffer(void *src)
{
    memcpy(FB_BOTTOM_VRAM, src, FB_BOTTOM_SIZE);
}

void draw_fillFramebuffer(u32 value)
{
    memset32(FB_BOTTOM_VRAM, value, FB_BOTTOM_SIZE);
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
    gpu_cache_fmt = GPU_FB_BOTTOM_1_FMT;
    gpu_cache_stride = GPU_FB_BOTTOM_1_STRIDE;

    u32 i;
    for(i = 0; i < 20; i++)
    {
        GPU_FB_BOTTOM_1_SEL &= ~1;
        GPU_FB_BOTTOM_1 = FB_BOTTOM_VRAM_PA;
        GPU_FB_BOTTOM_1_FMT = 0x80301;
        GPU_FB_BOTTOM_1_STRIDE = 240 * 3;
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
        GPU_FB_BOTTOM_1_FMT = gpu_cache_fmt;
        GPU_FB_BOTTOM_1_STRIDE = gpu_cache_stride;
        GPU_FB_BOTTOM_1_SEL = gpu_cache_sel;
    }

    draw_flushFramebuffer();
}

void draw_flushFramebuffer(void)
{
    svcFlushProcessDataCache((Handle)0xFFFF8001, FB_BOTTOM_VRAM, FB_BOTTOM_SIZE);
}


static void writeUnaligned(u8 *dst, u32 tmp, u32 size)
{
    memcpy(dst, &tmp, size);
}

void createBitmapHeader(u8 *dst, u32 width, u32 heigth)
{
    static const u8 bmpHeaderTemplate[54] = {
        0x42, 0x4D, 0xCC, 0xCC, 0xCC, 0xCC, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00,
        0x00, 0x00, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xCC, 0xCC, 0xCC, 0xCC, 0x12, 0x0B, 0x00, 0x00, 0x12, 0x0B, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    memcpy(dst, bmpHeaderTemplate, 54);
    writeUnaligned(dst + 2, 54 + 3*width*heigth, 4);
    writeUnaligned(dst + 0x12, width, 4);
    writeUnaligned(dst + 0x16, heigth, 4);
    writeUnaligned(dst + 0x22, 3*width*heigth, 4);
}

static void convertPixelToBGR8(u8 *dst, const u8 *src, GSPGPU_FramebufferFormats srcFormat)
{
    u8 red, green, blue;
    switch(srcFormat)
    {
        case GSP_RGBA8_OES:
            dst[2] = src[0];
            dst[1] = src[1];
            dst[0] = src[2];
            break;
        case GSP_BGR8_OES:
            dst[2] = src[2];
            dst[1] = src[1];
            dst[0] = src[0];
            break;
        case GSP_RGB565_OES:
            blue = src[0] & 0x1F; // thanks neobrain
            green = (src[1] & 7) | (src[0] >> 5);
            red = src[1] >> 3;

            dst[2] = (blue  << 3) | (blue  >> 5);
            dst[1] = (green << 2) | (green >> 6);
            dst[0] = (red   << 3) | (red   >> 5);

            break;
        case GSP_RGB5_A1_OES:
            blue = src[0] & 0x1F;
            green = (src[1] & 3) | (src[0] >> 5);
            red = (src[1] >> 2) & 0x1F;

            dst[2] = (blue  << 3) | (blue  >> 5);
            dst[1] = (green << 3) | (green >> 5);
            dst[0] = (red   << 3) | (red   >> 5);

            break;
        case GSP_RGBA4_OES:
            blue = src[0] & 0xF;
            green = src[0] >> 4;
            red = src[1] & 0xF;

            dst[2] = (blue  << 4) | (blue  >> 4);
            dst[1] = (green << 4) | (green >> 4);
            dst[0] = (red   << 4) | (red   >> 4);

            break;

        default: break;
    }
}

extern u8 *vramKMapping, *fcramKMapping;
void K_convertFrameBuffer(bool top)
{
    GSPGPU_FramebufferFormats fmt = top ? (GSPGPU_FramebufferFormats)(GPU_FB_TOP_1_FMT & 7) : (GSPGPU_FramebufferFormats)(GPU_FB_BOTTOM_1_FMT & 7);
    u32 width = top ? 400 : 320;
    u8 formatSizes[] = {4, 3, 2, 2, 2};
    u8 *pa = top ? (u8*)GPU_FB_TOP_1 : (u8*)GPU_FB_BOTTOM_1;
    u8 *kernelVA = (u8*)((pa >= (u8*)0x18000000 && pa < (u8*)0x18600000) ? (u32)pa - 0x18000000 + (u32)vramKMapping : (u32)pa - 0x20000000 + (u32)fcramKMapping);

    for(u32 p = (u32)kernelVA & ~0x1F; p < (u32)kernelVA + formatSizes[(u8)fmt]*width*240; p += 32)
        __asm__ volatile("mcr p15, 0, %0, c7, c10, 2" : "=r" (p) :: "memory"); // Clean Data Cache Line

    for(u32 x = 0; x < width; x++)
    {
        for (u32 y = 0; y < 240; y++)
            convertPixelToBGR8(framebuffer_cache + (y*width + x) * 3 , kernelVA + (x*240 + y) * formatSizes[(u8)fmt], fmt);
    }
}

u8 *convertFrameBuffer(bool top)
{
    svc_7b(K_convertFrameBuffer, top);
    return framebuffer_cache;
}
