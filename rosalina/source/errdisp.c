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

static char userString[0x100+1] = {0};

MyThread *errDispCreateThread(void)
{
    MyThread_Create(&errDispThread, errDispThreadMain, errDispThreadStack, THREAD_STACK_SIZE, 0x18, CORE_SYSTEM);
    return &errDispThread;
}

void errfDisplayError(ERRF_FatalErrInfo *info)
{
    draw_lock();

    u32 posY = draw_string(userString[0] == 0 ? "An error occurred (ErrDisp)" : userString, 10, 10, COLOR_RED);

    static const char *types[] = {
        "generic", "corrupted", "card removed", "exception", "result failure", "logged"
    };

    posY = posY < 30 ? 30 : posY;
    draw_string("Error type: ", 10, posY, COLOR_WHITE);
    draw_string((u32)info->type > (u32)ERRF_ERRTYPE_LOGGED ? types[0] : types[(u32)info->type], 10 + 18 * SPACING_X, posY, COLOR_WHITE);
    if(info->type != ERRF_ERRTYPE_CARD_REMOVED)
    {
        posY += SPACING_Y;

        char pidStr[]   = "Process ID:       0x00000000";
        char pnameStr[] = "Process name:             ";
        char tidStr[]   = "Process title ID: 0x0000000000000000";

        Handle processHandle;
        Result res;

        hexItoa(info->procId, pidStr + 20, 8, false);
        posY = draw_string(pidStr, 10, posY + SPACING_Y, COLOR_WHITE);

        res = svcOpenProcess(&processHandle, info->procId);
        if(R_SUCCEEDED(res))
        {
            u64 titleId;
            svcGetProcessInfo((s64 *)(pnameStr + 18), processHandle, 0x10000);
            svcGetProcessInfo((s64 *)&titleId, processHandle, 0x10001);
            svcCloseHandle(processHandle);
            hexItoa(titleId, tidStr + 20, 16, false);

            posY = draw_string(pnameStr, 10, posY + SPACING_Y, COLOR_WHITE);
            posY = draw_string(tidStr, 10, posY + SPACING_Y, COLOR_WHITE);
        }

        posY += SPACING_Y;
    }

    if(info->type == ERRF_ERRTYPE_EXCEPTION)
    {
        static const char *registerNames[] = {
            "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12",
            "sp", "lr", "pc", "cpsr"
        };

        char hexString[] = "00000000";

        u32 *regs = (u32 *)(&info->data.exception_data.regs);
        for(u32 i = 0; i < 17; i += 2)
        {
            posY = draw_string(registerNames[i], 10, posY + SPACING_Y, COLOR_WHITE);
            hexItoa(regs[i], hexString, 8, false);
            draw_string(hexString, 10 + 10 * SPACING_X, posY, COLOR_WHITE);

            if(i != 16)
            {
                draw_string(registerNames[i + 1], 10 + 28 * SPACING_X, posY, COLOR_WHITE);
                hexItoa(i == 16 ? regs[20] : regs[i + 1], hexString, 8, false);
                draw_string(hexString, 10 + 38 * SPACING_X, posY, COLOR_WHITE);
            }
        }

        if(info->data.exception_data.excep.type == ERRF_EXCEPTION_PREFETCH_ABORT
        || info->data.exception_data.excep.type == ERRF_EXCEPTION_DATA_ABORT)
        {
            draw_string("far", 10 + 28 * SPACING_X, posY, COLOR_WHITE);
            hexItoa(info->data.exception_data.excep.far, hexString, 8, false);
            draw_string(hexString, 10 + 38 * SPACING_X, posY, COLOR_WHITE);

            posY = draw_string("fsr", 10, posY + SPACING_Y, COLOR_WHITE);
            hexItoa(info->data.exception_data.excep.fsr, hexString, 8, false);
            draw_string(hexString, 10 + 10 * SPACING_X, posY, COLOR_WHITE);
        }

        else if(info->data.exception_data.excep.type == ERRF_EXCEPTION_VFP)
        {
            draw_string("fpexc", 10 + 28 * SPACING_X, posY, COLOR_WHITE);
            hexItoa(info->data.exception_data.excep.fpexc, hexString, 8, false);
            draw_string(hexString, 10 + 38 * SPACING_X, posY, COLOR_WHITE);

            posY = draw_string("fpinst", 10, posY + SPACING_Y, COLOR_WHITE);
            hexItoa(info->data.exception_data.excep.fpinst, hexString, 8, false);
            draw_string(hexString, 10 + 10 * SPACING_X, posY, COLOR_WHITE);

            draw_string("fpinst2", 10 + 28 * SPACING_X, posY, COLOR_WHITE);
            hexItoa(info->data.exception_data.excep.fpinst2, hexString, 8, false);
            draw_string(hexString, 10 + 38 * SPACING_X, posY, COLOR_WHITE);
        }
    }

    else if(info->type != ERRF_ERRTYPE_CARD_REMOVED)
    {
        if(info->type != ERRF_ERRTYPE_FAILURE)
        {
            char pcString[] = "Address:          0x00000000";
            hexItoa(info->pcAddr, pcString + 20, 8, false);
            posY = draw_string(pcString, 10, posY + SPACING_Y, COLOR_WHITE);
        }

        char errCodeString[] = "Error code:       0x00000000";
        hexItoa(info->resCode, errCodeString + 20, 8, false);
        posY = draw_string(errCodeString, 10, posY + SPACING_Y, COLOR_WHITE);
    }

    const char *desc;
    switch(info->type)
    {
        case ERRF_ERRTYPE_CARD_REMOVED:
            desc = "The Game Card was removed.";
            break;
        case ERRF_ERRTYPE_MEM_CORRUPT:
            desc = "The System Memory has been damaged.";
            break;
        case ERRF_ERRTYPE_FAILURE:
            info->data.failure_mesg[0x60] = 0; // make sure the last byte in the IPC buffer is NULL
            desc = info->data.failure_mesg;
            break;
        default:
            desc = "";
            break;
    }

    posY += SPACING_Y;
    if(desc[0] != 0)
        posY = draw_string(desc, 10, posY + SPACING_Y, COLOR_WHITE) + SPACING_Y;
    posY = draw_string("Press any button to reboot", 10, posY + SPACING_Y, COLOR_WHITE);

    draw_flushFramebuffer();
    draw_unlock();
}

void errfHandleCommands(void)
{
    u32 *cmdbuf = getThreadCommandBuffer();

    switch(cmdbuf[0] >> 16)
    {
        case 1: // Throw
        {
            ERRF_FatalErrInfo *info = (ERRF_FatalErrInfo *)(cmdbuf + 1);
            menuEnter();

            draw_lock();
            draw_clearFramebuffer();
            draw_flushFramebuffer();

            errfDisplayError(info);

            /*
            If we ever wanted to return:
            draw_unlock();
            menuLeave();

            cmdbuf[0] = 0x10040;
            cmdbuf[1] = 0;

            but we don't
            */
            while(!HID_PAD);
            svcKernelSetState(7);
            __builtin_unreachable();
            break;
        }

        case 2: // SetUserString
        {
            if(cmdbuf[0] != 0x20042 || (cmdbuf[2] & 0x3C0F) != 2)
            {
                cmdbuf[0] = 0x40;
                cmdbuf[1] = 0xD9001830;
            }
            else
            {
                cmdbuf[0] = 0x20040;
                u32 sz = cmdbuf[1] <= 0x100 ? sz : 0x100;
                memcpy(userString, cmdbuf + 3, sz);
                userString[sz] = 0;
            }
            break;
        }
    }
}

void errDispThreadMain(void)
{
    Handle handles[2];
    Handle serverHandle, clientHandle, sessionHandle = 0;

    u32 replyTarget;
    s32 index;

    Result res;
    u32 *cmdbuf = getThreadCommandBuffer();

    assertSuccess(svcCreatePort(&serverHandle, &clientHandle, "err:f", 1));

    do
    {
        handles[0] = serverHandle;
        handles[1] = sessionHandle;

        if(replyTarget == 0) // k11
            cmdbuf[0] = 0xFFFF0000;
        res = svcReplyAndReceive(&index, handles, sessionHandle == 0 ? 1 : 2, replyTarget);

        if(R_FAILED(res))
        {
            if((u32)res == 0xC920181A) // session closed by remote
            {
                svcCloseHandle(sessionHandle);
                sessionHandle = 0;
                replyTarget = 0;
            }

            else
                svcBreak(USERBREAK_PANIC);
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
