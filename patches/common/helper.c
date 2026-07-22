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
#include <nexioctls.h>          // ioctls added in the nexmon patch
#include <argprintf.h>

/**
 *  Setup a timer to schedule a function call
 */
struct hndrte_timer *
schedule_work(void *context, void *data, void *mainfn, int ms, int periodic)
{
    struct hndrte_timer *task;
    task = hndrte_init_timer(context, data, mainfn, 0);
    if (task) {
        if(!hndrte_add_timer(task, ms, periodic))
            hndrte_free_timer(task);
    }
    return task;
}

struct delayed_task {
    void *context;
    void *data;
    void *mainfn;
    int ms;
    int periodic;
};

static void
perform_delayed_task(struct hndrte_timer *t)
{
    struct delayed_task *delayed_task = (struct delayed_task *) t->data;

    schedule_work(delayed_task->context, delayed_task->data, delayed_task->mainfn, delayed_task->ms, delayed_task->periodic);
    
    free(t->data);
}

struct hndrte_timer *
schedule_delayed_work(void *context, void *data, void *mainfn, int ms, int periodic, int delay_ms)
{
    struct hndrte_timer *task;
    struct delayed_task *delayed_task = malloc(sizeof(struct delayed_task), 0);
    
    delayed_task->context = context;
    delayed_task->data = data;
    delayed_task->mainfn = mainfn;
    delayed_task->ms = ms;
    delayed_task->periodic = periodic;

    task = hndrte_init_timer(context, delayed_task, perform_delayed_task, 0);
    if (task) {
        if(!hndrte_add_timer(task, delay_ms, 0))
            hndrte_free_timer(task);
    }
    return task;
}

/**
 *  add data to the start of a buffer
 */
void *
skb_push(sk_buff *p, unsigned int len)
{
    p->data -= len;
    p->len += len;

    //if (p->data < p->head)
    //    printf("%s: failed", __FUNCTION__);

    return p->data;
}

/**
 *  remove data from the start of a buffer
 */
void *
skb_pull(sk_buff *p, unsigned int len)
{
    p->data += len;
    p->len -= len;

    return p->data;
}

void
_hexdump(char *desc, void *addr, int len, int (*_printf)(const char *, ...))
{
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

    // Output description if given.
    if (desc != 0)
        _printf("%s:\n", desc);

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                _printf("  %s\n", buff);

            // Output the offset.
            _printf("  %04x ", i);
        }

        // Now the hex code for the specific character.
        _printf(" %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        _printf("   ");
        i++;
    }

    // And print the final ASCII bit.
    _printf("  %s\n", buff);
}

void
hexdump(char *desc, void *addr, int len)
{
    _hexdump(desc, addr, len, printf);
}


/* Quarter dBm units to mW
 * Table starts at QDBM_OFFSET, so the first entry is mW for qdBm=153
 * Table is offset so the last entry is largest mW value that fits in
 * a uint16.
 */

#define QDBM_OFFSET 153     /* Offset for first entry */
#define QDBM_TABLE_LEN 40   /* Table size */

/* Smallest mW value that will round up to the first table entry, QDBM_OFFSET.
 * Value is ( mW(QDBM_OFFSET - 1) + mW(QDBM_OFFSET) ) / 2
 */
#define QDBM_TABLE_LOW_BOUND 6493 /* Low bound */

/* Largest mW value that will round down to the last table entry,
 * QDBM_OFFSET + QDBM_TABLE_LEN-1.
 * Value is ( mW(QDBM_OFFSET + QDBM_TABLE_LEN - 1) + mW(QDBM_OFFSET + QDBM_TABLE_LEN) ) / 2.
 */
#define QDBM_TABLE_HIGH_BOUND 64938 /* High bound */

static const unsigned short nqdBm_to_mW_map[QDBM_TABLE_LEN] = {
/* qdBm:    +0  +1  +2  +3  +4  +5  +6  +7 */
/* 153: */      6683,   7079,   7499,   7943,   8414,   8913,   9441,   10000,
/* 161: */      10593,  11220,  11885,  12589,  13335,  14125,  14962,  15849,
/* 169: */      16788,  17783,  18836,  19953,  21135,  22387,  23714,  25119,
/* 177: */      26607,  28184,  29854,  31623,  33497,  35481,  37584,  39811,
/* 185: */      42170,  44668,  47315,  50119,  53088,  56234,  59566,  63096
};

unsigned short
bcm_qdbm_to_mw(unsigned char qdbm)
{
    unsigned int factor = 1;
    int idx = qdbm - QDBM_OFFSET;

    if (idx >= QDBM_TABLE_LEN) {
        /* clamp to max uint16 mW value */
        return 0xFFFF;
    }

    /* scale the qdBm index up to the range of the table 0-40
     * where an offset of 40 qdBm equals a factor of 10 mW.
     */
    while (idx < 0) {
        idx += 40;
        factor *= 10;
    }

    /* return the mW value scaled down to the correct factor of 10,
     * adding in factor/2 to get proper rounding.
     */
    return ((nqdBm_to_mW_map[idx] + factor/2) / factor);
}

unsigned char
bcm_mw_to_qdbm(unsigned short mw)
{
    unsigned char qdbm;
    int offset;
    unsigned int mw_uint = mw;
    unsigned int boundary;

    /* handle boundary case */
    if (mw_uint <= 1)
        return 0;

    offset = QDBM_OFFSET;

    /* move mw into the range of the table */
    while (mw_uint < QDBM_TABLE_LOW_BOUND) {
        mw_uint *= 10;
        offset -= 40;
    }

    for (qdbm = 0; qdbm < QDBM_TABLE_LEN-1; qdbm++) {
        boundary = nqdBm_to_mW_map[qdbm] + (nqdBm_to_mW_map[qdbm+1] -
                                            nqdBm_to_mW_map[qdbm])/2;
        if (mw_uint < boundary) break;
    }

    qdbm += (uint8)offset;

    return (qdbm);
}

void
set_chanspec(struct wlc_info *wlc, unsigned short chanspec)
{
    unsigned int local_chanspec = chanspec;
    wlc_iovar_op(wlc, "chanspec", 0, 0, &local_chanspec, 4, 1, 0);
}

unsigned int
get_chanspec(struct wlc_info *wlc)
{
    unsigned int chanspec = 0;
    wlc_iovar_op(wlc, "chanspec", 0, 0, &chanspec, 4, 0, 0);
    return chanspec;
}

void
set_mpc(struct wlc_info *wlc, uint32 mpc)
{
    wlc_iovar_op(wlc, "mpc", 0, 0, &mpc, 4, 1, 0);
}

uint32
get_mpc(struct wlc_info *wlc)
{
    uint32 mpc = 0;

    wlc_iovar_op(wlc, "mpc", 0, 0, &mpc, 4, 0, 0);
    
    return mpc;
}

void
set_monitormode(struct wlc_info *wlc, uint32 monitor)
{
    wlc_ioctl(wlc, WLC_SET_MONITOR, &monitor, 4, 0);
}

void
set_intioctl(struct wlc_info *wlc, uint32 cmd, uint32 arg)
{
    wlc_ioctl(wlc, cmd, &arg, 4, 0);
}

uint32
get_intioctl(struct wlc_info *wlc, uint32 cmd)
{
    uint32 arg;
    wlc_ioctl(wlc, cmd, &arg, 4, 0);
    return arg;
}

#if (NEXMON_CHIP == CHIP_VER_BCM4339 || NEXMON_CHIP == CHIP_VER_BCM4358 || NEXMON_CHIP == CHIP_VER_BCM43455c0 || NEXMON_CHIP == CHIP_VER_BCM4366c0)
void
set_scansuppress(struct wlc_info *wlc, uint32 scansuppress)
{
    wlc_scan_ioctl(wlc->scan, WLC_SET_SCANSUPPRESS, &scansuppress, 4, 0);
}

uint32
get_scansuppress(struct wlc_info *wlc)
{
    uint32 scansuppress = 0;

    wlc_scan_ioctl(wlc->scan, WLC_GET_SCANSUPPRESS, &scansuppress, 4, 0);
    
    return scansuppress;
}
#endif
