#include "types.h"
#include "svc.h"
#include "i2c.h"
#include "fs.h"
#include "memory.h"
#include "gamecart/protocol_gw.h"
#include "gamecart/command_gw.h"
#include "hid.h"


void findSubroutines(void)
{
    const u8 f9_closePattern[] = {0x10, 0xB5, 0x04, 0x00};

    struct Pattern {
        void **method;
        const u8 pattern[8];
        bool thumb;
    };

    const struct Pattern patterns[] = {
        {(void**)&f9_open, {0xF7, 0xB5, 0x84, 0xB0, 0x04, 0x98, 0x0D, 0x00}, true},
        {(void**)&f9_write, {0xFF, 0xB5, 0x85, 0xB0, 0x0E, 0x9F, 0x1C, 0x00}, true},
        {(void**)&f9_read, {0xFF, 0xB5, 0x07, 0x00, 0x1C, 0x00, 0x00, 0x26}, true},
        /*{&f9_size, {}}*/
    };
    for(u32 i = 0; i < sizeof(patterns) / sizeof(struct Pattern); i++)
        *(patterns[i].method) = (void*)((u32)memsearch((void*)0x08028000, patterns[i].pattern, 0x80000, 8) | patterns[i].thumb); // thumb mode

    u8 *f9_close_off = (u8*)0x08028000;
    do
    {
        f9_close_off = memsearch(f9_close_off, f9_closePattern, 0x80000, 4);
        if(*((u16*)f9_close_off + 3) != 0x2800) // cmp r0, #0
            break;
        f9_close_off++;
    }
    while(true);
    f9_close = (void(*))((u32)f9_close_off | 1);
}

void main(void)
{
    findSubroutines();

    /*FILE_P9 file;
    u32 bytes;

    f9_create(&file);
    f9_open(&file, L"sdmc:/luma/test.bin", MODE_CREATE | MODE_WRITE);
    f9_write(&file, &bytes, "Hello World", 12, 1);
    f9_close(&file);*/

    while(true)
    {
        switch(HID_PAD)
        {
            case BUTTON_X | BUTTON_Y:
                if(!gwcard_init())
                    svcBreak(0x1300);

                u8 mbr[0x200];
                gwcard_cmd_read(0, 0x200, 1, &mbr);
                svcBreak(mbr[0x1BE + 0x4]);
                return;
        }
        svcSleepThread(50 * 1000 * 1000);
    }
}
