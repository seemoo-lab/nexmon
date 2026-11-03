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
 * Copyright (c) 2020 NexMon Team                                          *
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
#include <version.h>            // version information
#include <argprintf.h>          // allows to execute argprintf to print into the arg buffer
#include <capabilities.h>

#define NULL 0

/* Keep the D11 core awake and the TX path un-muted for monitor-mode TX.
 * With mpc left at 1, the core powers down once the STA goes idle and an
 * injected frame is accounted as PACKET_OUTGOING in the host pcap while the
 * radio never actually keys. */
static void
inject_radio_up(struct wlc_info *wlc)
{
    int z = 0;
    int txpwr = 255;

    /*
     * Match the proven AC injection-enable sequence (bcm4358 case 703):
     * suppress background/roam scans so the MAC stays on-channel and the
     * ucode does not park the TX engine off-channel; drop mpc so the core
     * never powers down; clear PM; un-mute TX power; enable the MAC.
     */
    set_scansuppress(wlc, 1);
    set_mpc(wlc, 0);
    wlc_ioctl(wlc, WLC_SET_PM, &z, 4, 0);
    wlc_ioctl(wlc, WLC_SET_TXPWR, &txpwr, 4, 0);

    if (wlc && wlc->hw) {
        *(int *)((char *)wlc->hw + 0x144) = 0;
        *(int *)((char *)wlc->hw + 0xf0) = 1;
    }
    wlc_enable_mac(wlc);
}

static char saved_dbg_buf[1024] = {0};
static int saved_dbg_len = 0;

int
wlc_ioctl_hook(struct wlc_info *wlc, int cmd, char *arg, int len, void *wlc_if)
{
    int ret = IOCTL_ERROR;
    argprintf_init(arg, len);

    switch(cmd) {
        case NEX_GET_CAPABILITIES:
        {
            if (len >= 4 && arg) {
                memcpy(arg, &capabilities, 4);
                ret = IOCTL_SUCCESS;
            }
            break;
        }

        case 514:
        {
            if (len > 0 && arg) {
                int copy_len = saved_dbg_len < len ? saved_dbg_len : len;
                if (copy_len > 0) {
                    memcpy(arg, saved_dbg_buf, copy_len);
                }
                ret = IOCTL_SUCCESS;
            }
            break;
        }

        case 510:
        {
            inject_radio_up(wlc);
            argprintf("wlc_ioctl_hook: inject radio up done\n");
            ret = IOCTL_SUCCESS;
            break;
        }

        case 511:
        {
            dumpcb_t *d;
            struct bcmstrbuf b;
            int err;

            if (!wlc || !arg || len <= 0)
                break;

            bcm_binit(&b, arg, len);

            for (d = wlc->dumpcb_head; d != NULL; d = d->next) {
                bcm_bprintf(&b, "%s\n", d->name);
                err = d->dump_fn(d->dump_fn_arg, &b);
                if (err) bcm_bprintf(&b, "ERR: %d\n", err);
                bcm_bprintf(&b, "\n");
            }
            ret = IOCTL_SUCCESS;
            break;
        }

        case 512:
        {
            extern int dbg_hook_calls, dbg_hook_m_zero, dbg_hook_p_null, dbg_hook_len_small;
            extern int dbg_hook_short_nz, dbg_hook_rtap_bad, dbg_inject_calls, dbg_sf_rate;
            extern int dbg_sf_calls, dbg_sf_txoff_null, dbg_sf_no_scb, dbg_sf_wlc_down;
            extern int dbg_sf_txfifo_calls, dbg_sf_band, dbg_sf_hw_up, dbg_sf_wlc_clk;
            extern unsigned int dbg_sf_scb_ptr, dbg_wlc_ptr, dbg_wlc_10, dbg_wlc_14;
            extern unsigned int dbg_di_ptr, dbg_fn_ptr, dbg_p_len, dbg_txh_p28;
            extern unsigned int dbg_txh_dump[16];
            extern int dbg_sendctl_ret;
            extern int dbg_scb_check_null_count, dbg_scb_check_ok_count;
            extern unsigned int dbg_sf_p_scb, dbg_mctl_before, dbg_mctl_after;

            if (!wlc)
                break;

            struct dma_info *di = 0;
            if (wlc->hw)
                di = wlc->hw->di[1];
            else if (dbg_di_ptr)
                di = (struct dma_info *) dbg_di_ptr;

            unsigned int *txregs = (di) ? (unsigned int *) di->txregs : 0;
            unsigned int txavail = (di) ? di->hnddma.txavail : 0;
            unsigned short txin = (di) ? di->txin : 0;
            unsigned short txout = (di) ? di->txout : 0;
            unsigned int macctl = 0, macstatus = 0, maccmd = 0, chnst = 0;
            unsigned int psmdbg = 0, phydbg = 0, frmtx = 0, frmtx2 = 0, clkctl = 0;
            unsigned short phytxerr = 0;
            char *regs = *(char **)((char *)wlc + 0x0c);
            if (wlc->hw && wlc->hw->up && regs) {
                macctl = *(unsigned int *)(regs + 0x120);
                maccmd = *(unsigned int *)(regs + 0x124);
                macstatus = *(unsigned int *)(regs + 0x128);
                chnst = *(unsigned int *)(regs + 0x150);
                psmdbg = *(unsigned int *)(regs + 0x154);
                phydbg = *(unsigned int *)(regs + 0x158);
                frmtx = *(unsigned int *)(regs + 0x170);
                frmtx2 = *(unsigned int *)(regs + 0x174);
                clkctl = *(unsigned int *)(regs + 0x1e0);
                phytxerr = *(unsigned short *)(regs + 0x3ee);
            }
            void *scb = wlc->band ? wlc->band->hwrs_scb : 0;
            void *scb_bss = (scb) ? *(void **)((char *)scb + 16) : 0;
            unsigned int bss_flags = (scb_bss) ? *(unsigned int *)((char *)scb_bss + 232) : 0;
            void *active_q = *(void **)((char *)wlc + 0x5d0);
            int susp_depth = (wlc->hw) ? *(int *)((char *)wlc->hw + 0xf0) : -1;
            int hw_144 = (wlc->hw) ? *(int *)((char *)wlc->hw + 0x144) : -1;

            argprintf("sf=%d scb_ok=%d scb_null=%d ret=%d inj=%d rate=%d\n"
                      "scb=%p p_scb=%08x bss=%p bss_f=%08x\n"
                      "susp=%d hw144=%d mctl_b=%08x mctl_a=%08x macctl=%08x macst=%08x\n"
                      "q=%p di=%p txavail=%u in=%u out=%u\n"
                      "txregs=%p ctl=%08x ptr=%08x s0=%08x s1=%08x\n"
                      "cmd=%08x chn=%08x psm=%08x phy=%08x frmtx=%08x,%08x clk=%08x terr=%04x\n"
                      "d0: %08x %08x %08x %08x\n",
                      dbg_sf_calls, dbg_scb_check_ok_count, dbg_scb_check_null_count,
                      dbg_sendctl_ret, dbg_inject_calls, dbg_sf_rate,
                      scb, dbg_sf_p_scb, scb_bss, bss_flags,
                      susp_depth, hw_144, dbg_mctl_before, dbg_mctl_after, macctl, macstatus,
                      active_q, di, txavail, (unsigned int) txin, (unsigned int) txout,
                      txregs,
                      (txregs) ? txregs[0] : 0, (txregs) ? txregs[1] : 0,
                      (txregs) ? txregs[4] : 0, (txregs) ? txregs[5] : 0,
                      maccmd, chnst, psmdbg, phydbg, frmtx, frmtx2, clkctl, (unsigned int) phytxerr,
                      dbg_txh_dump[0], dbg_txh_dump[1], dbg_txh_dump[2], dbg_txh_dump[3]);

            if (arg && len > 0) {
                int copy_len = len < (int) sizeof(saved_dbg_buf) ? len : (int) sizeof(saved_dbg_buf) - 1;
                memcpy(saved_dbg_buf, arg, copy_len);
                saved_dbg_buf[copy_len] = '\0';
                saved_dbg_len = copy_len;
            }

            ret = IOCTL_SUCCESS;
            break;
        }

        /*
         * cmd 513 (SHM macstat dump) removed: wlc_read_shm has no mapped
         * address for 43455c0 (dummy in wrapper.c), so it both returned
         * garbage and left the linker with "no memory region specified for
         * loadable section .text.wlc_read_shm". Restore only with a real
         * 7.45.265 wlc_read_shm/wlc_bmac_read_shm address.
         */

        default:
            ret = wlc_ioctl(wlc, cmd, arg, len, wlc_if);
    }

    /* Once monitor mode is actually on, force the radio into a state that
     * can transmit. Done after the ioctl so it is not undone by it. */
    if (cmd == WLC_SET_MONITOR && arg && len >= 4 && *(int *) arg)
        inject_radio_up(wlc);

    return ret;
}

__attribute__((at(0x20B988, "", CHIP_VER_BCM43455c0, FW_VER_7_45_265_28bca26_CY)))
GenericPatch4(wlc_ioctl_hook, wlc_ioctl_hook + 1);
