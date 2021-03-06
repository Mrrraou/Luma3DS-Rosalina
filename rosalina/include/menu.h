#pragma once

#include <3ds/types.h>
#include "MyThread.h"
#include "utils.h"

#define HID_PAD                 (REG32(0x10146000) ^ 0xFFF)

#define BUTTON_A          (1 << 0)
#define BUTTON_B          (1 << 1)
#define BUTTON_SELECT     (1 << 2)
#define BUTTON_START      (1 << 3)
#define BUTTON_RIGHT      (1 << 4)
#define BUTTON_LEFT       (1 << 5)
#define BUTTON_UP         (1 << 6)
#define BUTTON_DOWN       (1 << 7)
#define BUTTON_R1         (1 << 8)
#define BUTTON_L1         (1 << 9)
#define BUTTON_X          (1 << 10)
#define BUTTON_Y          (1 << 11)

#define CORE_APPLICATION  0
#define CORE_SYSTEM       1


typedef enum MenuItemAction {
    METHOD,
    MENU
} MenuItemAction;
typedef struct MenuItem {
    const char *title;

    MenuItemAction action_type;
    union {
        struct Menu *menu;
        void (*method)(void);
    };
} MenuItem;
typedef struct Menu {
    const char *title;

    u32 items;
    MenuItem item[0x40];
} Menu;

extern bool terminationRequest;
extern Handle terminationRequestEvent;

u32 waitInputWithTimeout(u32 msec);
u32 waitInput(void);

MyThread *menuCreateThread(void);
void menuEnter(void);
void menuLeave(void);
void menuThreadMain(void);
void menuShow(void);
