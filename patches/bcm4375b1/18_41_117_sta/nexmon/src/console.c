/***************************************************************************
 *                                                                         *
 *          ###########   ###########   ##########    ##########           *
 *         ############  ############  ############  ############          *
 *         ##            ##            ##   ##   ##  ##        ##          *
 *         ##            ##            ##   ##   ##  ##        ##          *
 *         ###########   ####  ######  ##   ##   ##  ##    ######          *
 *          ###########  ####  #       ##   ##   ##  ##    #    #          *
 *                   ##  ##    ######  ##   ##   ##  ##    #    #          *
 *                   ##  ##    #       ##   ##   ##  ##    #    #          *
 *         ############  ##### ######  ##   ##   ##  ##### ######          *
 *         ###########    ###########  ##   ##   ##   ##########           *
 *                                                                         *
 *            S E C U R E   M O B I L E   N E T W O R K I N G              *
 *                                                                         *
 * This file is part of NexMon.                                            *
 *                                                                         *
 * Copyright (c) 2016 NexMon Team                                          *
 *                                                                         *
 * NexMon is free software: you can redistribute it and/or modify          *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation, either version 3 of the License, or       *
 * (at your option) any later version.                                     *
 *                                                                         *
 * NexMon is distributed in the hope that it will be useful,               *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with NexMon. If not, see <http://www.gnu.org/licenses/>.          *
 *                                                                         *
 **************************************************************************/

#pragma NEXMON targetregion "patch"

#include <firmware_version.h>   // definition of firmware version macros
#include <debug.h>              // contains macros to access the debug hardware
#include <wrapper.h>            // wrapper definitions for functions that already exist in the firmware
#include <structs.h>            // structures that are used by the code in the firmware
#include <helper.h>             // useful helper functions
#include <patcher.h>            // macros used to craete patches such as BLPatch, BPatch, ...
#include <rates.h>              // rates used to build the ratespec for frame injection
#include <local_wrapper.h>

#define LOG_BUF_LEN     (4*1024)

static char console_buf[LOG_BUF_LEN] = {0};

typedef struct {
    char        *buf;
    uint32_t    buf_size;
    uint32_t    idx;
} hndrte_log_t;

hndrte_log_t active_log = {
    .buf = console_buf,
    .buf_size = LOG_BUF_LEN,
    .idx = 0
};

void
clearconsole(void)
{
    int i = 0;
    for (i = 0; i < LOG_BUF_LEN; i++) {
        console_buf[i] = 0;
    }

    active_log.idx = 0;
}

void
putc(int c)
{
    hndrte_log_t *log = &active_log;

    /* CR before LF */
    if (c == '\n')
        putc('\r');

    if (log->buf != 0) {
        int idx = log->idx;

        /* Store in log buffer */
        log->buf[idx] = (char)c;
        log->idx = (idx + 1) % LOG_BUF_LEN;
    }
}

int
vprintf(void *lr, void *sp, const char *fmt, va_list va)
{
    char line_buf[256] = { 0 };

    if (active_log.buf[(active_log.idx - 1) % active_log.buf_size] == '\n') {
        uint32_t time = hndrte_time_ms();
        int written = 0;

        switch ((int) lr) {
            case 0x1855e7:
                // at offset 17 uint32_t values of the sp, we find the pused link register
                written = snprintf(line_buf, sizeof(line_buf) - 3 - written, "%03u.%03u %06x %06x ", time / 1000, time % 1000, lr, ((uint32_t *) sp)[17]);
                break;
            case 0xd4e9:
                // at offset 23 uint32_t values of the sp, we find the pused link register
                written = snprintf(line_buf, sizeof(line_buf) - 3 - written, "%03u.%03u %06x %06x ", time / 1000, time % 1000, lr, ((uint32_t *) sp)[23]);
                //hexdump("printf", sp, 50*4);
                break;
            case 0x1854ad:
                // at offset 23 uint32_t values of the sp, we find the pused link register
                written = snprintf(line_buf, sizeof(line_buf) - 3 - written, "%03u.%03u %06x %06x ", time / 1000, time % 1000, lr, ((uint32_t *) sp)[15]);
                //for (int i = 0; i < 50; i++)
                //    printf("%02d %08x\n", i, ((uint32_t *) sp)[i]);
                break;
            default:
                written = snprintf(line_buf, sizeof(line_buf) - 3 - written, "%03u.%03u %06x ", time / 1000, time % 1000, lr);
        }
        if (written >= 0 && written < sizeof(line_buf) - 3 - written)
            vsnprintf(line_buf + written, sizeof(line_buf) -3 - written, fmt, va);
    } else {
        vsnprintf(line_buf, sizeof(line_buf)-3, fmt, va);
    }

    for (char *c = line_buf; *c != 0; c++) {
        putc(*c);
    }

    return 0;
}

#if 1
int
printf(const char *fmt, ...)
{
    void *lr = 0x0;
    void *sp = 0x0;
    int ret = 0;
    __asm("mov %0, lr" : "=r" (lr));
    __asm("mov %0, sp" : "=r" (sp));

    va_list va;
    va_start(va,fmt);
    ret = vprintf(lr, sp, fmt, va);
    va_end(va);

    return ret;
}

__attribute__((at(0x89D0, "flashpatch", CHIP_VER_BCM4375b1, FW_VER_18_38_18_sta)))
__attribute__((naked))
void
printf_hook(void)
{
// HINT: the flashpatches on the BCM4375 are always 8 byte long and also aligned
// on an 8 byte boundary, hence, we need to add the bytes that will be overwritten
// by the flashpatch
    asm(
        "b printf\n"
        ".byte 0x00, 0x00, 0x00, 0x00"
    );
}
#endif
