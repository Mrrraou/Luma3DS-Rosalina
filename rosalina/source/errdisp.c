#include <3ds.h>
#include "errdisp.h"
#include "draw.h"
#include "menu.h"
#include "memory.h"

static inline void assertSuccess(Result res)
{
    if(R_FAILED(res))
        svcBreak(USERBREAK_PANIC);
}

static MyThread errDispThread;
static u8 ALIGN(8) errDispThreadStack[THREAD_STACK_SIZE];

static char msg[60];

MyThread *errDispCreateThread(void)
{
    MyThread_Create(&errDispThread, errDispThreadMain, errDispThreadStack, THREAD_STACK_SIZE, 0x18, CORE_SYSTEM);
    return &menuThread;
}

void errfHandleCommands(void)
{
    u32 *cmdbuf = getThreadCommandBuffer();

    switch(cmdbuf[0] >> 16)
    {
        case 0x10: // Throw
        {
            ERRF_FatalErrInfo *info = (ERRF_FatalErrInfo *)(cmdbuf + 1);

            draw_lock(); // if GSP hasn't been started yet we're fucked (screeninit)(?)
            draw_clearFramebuffer();
            draw_flushFramebuffer();

            draw_string("An error occurred (ErrDisp)", 10, 10, COLOR_TITLE);

            static const char *types[] = {
                "generic", "corrupted", "card removed", "exception", "result failure", "logged"
            };

            u32 posY = draw_string("Error type: ", 10, 30, COLOR_WHITE);
            draw_string((u32)info->type > (u32)ERRF_ERRTYPE_LOGGED ? types[0] : types[(u32)info->type], 10 + 12 * SPACING_X, 30, COLOR_WHITE);
            if(info->type != ERRF_ERRTYPE_CARD_REMOVED)
            {
                char processStr[] = "Process ID: 0x00000000"
                char tidAidStr[] = "TID 0000000000000000, AID 0000000000000000";
                hexItoa(info->procId, processStr + 14, 8, false);
                hexItoa(info->titleId, tidAidStr + 4, 16, false);
                hexItoa(info->appTitleId, tidAidStr + 26, 16, false);

                posY = draw_string(processStr, 10, posY + SPACING_Y, COLOR_WHITE);
                posY = draw_string(tidAidStr, 10, posY + SPACING_Y, COLOR_WHITE);
            }

            /*switch(info->type)
            {
                case ERRF_ERRTYPE_GENERIC:
                case ERRF_ERRTYPE_LOGGED: // we don't support logging, sorry
                case ERRF_ERRTYPE_MEM_CORRUPT:
            }*/

            if(info->type == ERRF_ERRTYPE_EXCEPTION)
            {
                posY += SPACING_Y;
                static const char *registerNames[] = {
                    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12",
                    "sp", "lr", "pc", "cpsr", "fsr", "far", "fpexc", "fpinst", "fpinst2"
                };

                char hexString[] = "00000000";

                u32 *regs = (u32 *)info->data.exception_data.regs;
                u32 posX = 0;
                for(u32 i = 0; i < 17; i += 2)
                {
                    posX = 10;
                    posY = draw_string(registerNames[i], posX, posY + SPACING_Y, COLOR_WHITE);
                    hexItoa(regs[i], hexString, 8, false);
                    draw_string(hexString, posX + 7 * SPACING_X, posY, COLOR_WHITE);

                    posX = 10 + 22 * SPACING_X;
                    if(i != 16)
                    {
                        draw_string(registerNames[i + 1], posX, posY, COLOR_WHITE);
                        hexItoa(i == 16 ? regs[20] : regs[i + 1], hexString, 8, false);
                        draw_string(hexString, posX + 7 * SPACING_X, posY, COLOR_WHITE);
                    }
                }

                if(info->data.exception_data.type == ERRF_EXCEPTION_PREFETCH_ABORT
                || info->data.exception_data.type == ERRF_EXCEPTION_DATA_ABORT)
                {
                    // on pabort this is pc instead of ifar...
                    posY = draw_string("far", posX, posY + SPACING_Y, COLOR_WHITE);
                    hexItoa(info->data.exception_data.regs.reg1, hexString, 8, false);
                    draw_string(hexString, posX + 7 * SPACING_X, posY, COLOR_WHITE);

                    posX = 10 + 22 * SPACING_X;
                    draw_string("fsr", posX, posY, COLOR_WHITE);
                    hexItoa(info->data.exception_data.regs.reg2, hexString, 8, false);
                    posY = draw_string(hexString, posX + 7 * SPACING_X, posY + SPACING_Y, COLOR_WHITE);
                }

                else if(info->data.exception_data.type == ERRF_EXCEPTION_VFP)
                {
                    posX = 10;
                    posY = draw_string("fpexc", posX, posY + SPACING_Y, COLOR_WHITE);
                    hexItoa(info->data.exception_data.regs.fpexc, hexString, 8, false);
                    draw_string(hexString, posX + 7 * SPACING_X, posY, COLOR_WHITE);

                    posX = 10;
                    posY = draw_string("fpinst", posX, posY + SPACING_Y, COLOR_WHITE);
                    hexItoa(info->data.exception_data.regs.fpinst, hexString, 8, false);
                    draw_string(hexString, posX + 7 * SPACING_X, posY, COLOR_WHITE);
                }
            }

            else if(info->type != ERRF_ERRTYPE_CARD_REMOVED)
            {
                if(info->type != ERRF_ERRTYPE_FAILURE)
                {
                    char pcString[] = "Address: 0x00000000";
                    hexItoa(info->pcAddr, pcString + 11, 8, false);
                    posY = draw_string(pcString, 10, posY + SPACING_Y, COLOR_WHITE);
                }

                char errCodeString[] = "Error code: 0x00000000";
                hexItoa(info->resCode, errCodeString + 14, 8, false);
                posY = draw_string(errCodeString, 10, posY + SPACING_Y, COLOR_WHITE);
            }
        }

        case 0x20: // SetUserString
        {
            u32 sz = cmdbuf[1];
            memcpy(msg, cmdbuf + 3, sz <= 0x60 ? sz : 0x60);
            if(sz < 0x60)
                msg[sz] = 0;
            break;
        }
    }
}

void errDispThreadMain(void)
{
    Handle serverHandle, clientHandle, sessionHandle = 0;
    Handle handles[2] = { serverHandle, sessionHandle };

    u32 replyTarget;
    s32 index;

    Result res;
    u32 *cmdbuf = getThreadCommandBuffer();

    assertSuccess(svcCreatePort(&serverHandle, &clientHandle, "err:f", 1));

    do
    {
        if(replyTarget == 0) // k11
            cmdbuf[0] = 0xFFFF0000;
        res = svcReplyAndReceive(&index, handles, 2, replyTarget);

        if(R_FAILED(res))
        {
            if(res == 0xC920181A) // session closed by remote
            {
                svcCloseHandle(sessionHandle);
                sessionHandle = 0;
                replyTarget = 0;
            }

            else
                svcBreak(USERBREAK_ASSERT);
        }

        else
        {
            if(index == 0)
            {
                Handle session;
                assertSuccess(svcAcceptSession(&session, serverHandle));

                if(sessionHandle == 0)
                    sessionHandle = session;
                else
                    svcCloseHandle(session);
            }
            else
            {
                errfHandleCommands();
                replyTarget = sessionHandle;
            }
        }
    }
    while(!terminationRequest);

    svcCloseHandle(sessionHandle);
    svcCloseHandle(clientHandle);
    svcCloseHandle(serverHandle);
}
