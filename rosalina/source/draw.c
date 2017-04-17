#include <3ds.h>
#include "draw.h"
#include "font.h"
#include "memory.h"
#include "menu.h"

u8 framebuffer_cache[FB_BOTTOM_SIZE];

static u32 gpu_cache_fb, gpu_cache_sel, gpu_cache_fmt, gpu_cache_stride;

static RecursiveLock lock;

void draw_lock(void)
{
    static bool lockInitialized = false;
    if(!lockInitialized)
    {
        RecursiveLock_Init(&lock);
        lockInitialized = true;
    }

    RecursiveLock_Lock(&lock);
}

void draw_unlock(void)
{
    RecursiveLock_Unlock(&lock);
}

void draw_copyFramebuffer(void *dst)
{
    memcpy(dst, FB_BOTTOM_VRAM, FB_BOTTOM_SIZE);
}

void draw_copyToFramebuffer(const void *src)
{
    memcpy(FB_BOTTOM_VRAM, src, FB_BOTTOM_SIZE);
}

void draw_fillFramebuffer(u32 value)
{
    memset32(FB_BOTTOM_VRAM, value, FB_BOTTOM_SIZE);
}

void draw_character(char character, u32 posX, u32 posY, u32 color)
{
    volatile u16 *const fb = (volatile u16 *const)FB_BOTTOM_VRAM;

    s32 y;
    for(y = 0; y < 10; y++)
    {
        char charPos = font[character * 10 + y];

        s32 x;
        for(x = 6; x >= 1; x--)
        {
            u32 screenPos = (posX * SCREEN_BOT_HEIGHT * 2 + (SCREEN_BOT_HEIGHT - y - posY - 1) * 2) + (5 - x) * 2 * SCREEN_BOT_HEIGHT;
            u32 pixelColor = ((charPos >> x) & 1) ? color : COLOR_BLACK;
            fb[screenPos / 2] = pixelColor;
        }
    }
}

u32 draw_string(const char *string, u32 posX, u32 posY, u32 color)
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
        GPU_FB_BOTTOM_1_FMT = 0x80302;
        GPU_FB_BOTTOM_1_STRIDE = 240 * 2;
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
    svcFlushProcessDataCache(CUR_PROCESS_HANDLE, FB_BOTTOM_VRAM, FB_BOTTOM_SIZE);
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

static inline void convertPixelToBGR8(u8 *dst, const u8 *src, GSPGPU_FramebufferFormats srcFormat)
{
    u8 red, green, blue;
    switch(srcFormat)
    {
        case GSP_RGBA8_OES:
        {
            u32 px = *(u32 *)src;
            dst[2] = (px >>  8) & 0xFF;
            dst[1] = (px >> 16) & 0xFF;
            dst[0] = (px >> 24) & 0xFF;
            break;
        }
        case GSP_BGR8_OES:
        {
            dst[2] = src[2];
            dst[1] = src[1];
            dst[0] = src[0];
            break;
        }
        case GSP_RGB565_OES:
        {
            // thanks neobrain
            u16 px = *(u16 *)src;
            blue = px & 0x1F;
            green = (px >> 5) & 0x3F;
            red = (px >> 11) & 0x1F;

            dst[0] = (blue  << 3) | (blue  >> 2);
            dst[1] = (green << 2) | (green >> 4);
            dst[2] = (red   << 3) | (red   >> 2);

            break;
        }
        case GSP_RGB5_A1_OES:
        {
            u16 px = *(u16 *)src;
            blue = px & 0x1F;
            green = (px >> 5) & 0x1F;
            red = (px >> 10) & 0x1F;

            dst[0] = (blue  << 3) | (blue  >> 2);
            dst[1] = (green << 3) | (green >> 2);
            dst[2] = (red   << 3) | (red   >> 2);

            break;
        }
        case GSP_RGBA4_OES:
        {
            u16 px = *(u32 *)src;
            blue = px & 0xF;
            green = (px >> 4) & 0xF;
            red = (px >> 8) & 0xF;

            dst[0] = (blue  << 4) | (blue  >> 4);
            dst[1] = (green << 4) | (green >> 4);
            dst[2] = (red   << 4) | (red   >> 4);

            break;
        }
        default: break;
    }
}

static u8 line[3*400];

u8 *convertFrameBufferLine(bool top, u32 y)
{
    GSPGPU_FramebufferFormats fmt = top ? (GSPGPU_FramebufferFormats)(GPU_FB_TOP_1_FMT & 7) : (GSPGPU_FramebufferFormats)(GPU_FB_BOTTOM_1_FMT & 7);
    u32 width = top ? 400 : 320;
    u8 formatSizes[] = {4, 3, 2, 2, 2};
    u32 pa = top ? GPU_FB_TOP_1 : GPU_FB_BOTTOM_1;

    u8 *addr = (u8 *)(pa | (1u << 31));

    for(u32 x = 0; x < width; x++)
        convertPixelToBGR8(line + x * 3 , addr + (x*240 + y) * formatSizes[(u8)fmt], fmt);
    return line;
}
