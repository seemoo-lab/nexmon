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
    int txpwr = 127;

    set_mpc(wlc, 0);
    wlc_ioctl(wlc, WLC_SET_PM, &z, 4, 0);
    wlc_ioctl(wlc, WLC_SET_TXPWR, &txpwr, 4, 0);
    wlc_enable_mac(wlc);
}

int
wlc_ioctl_hook(struct wlc_info *wlc, int cmd, char *arg, int len, void *wlc_if)
{
    int ret = IOCTL_ERROR;
    argprintf_init(arg, len);

    switch(cmd) {
        case NEX_GET_CAPABILITIES:
        {
            if (len == 4) {
                memcpy(arg, &capabilities, 4);
                ret = IOCTL_SUCCESS;
            }
            break;
        }

        case 510:
        {
            argprintf("%s\n", __FUNCTION__);
            ret = IOCTL_SUCCESS;
            break;
        }

        case 511:
        {
            dumpcb_t *d;
            struct bcmstrbuf b;
            int err;

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
