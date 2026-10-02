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
 * Copyright (c) 2023 NexMon Team                                          *
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
#include <wrapper.h>            // wrapper definitions for functions that already exist in the firmware
#include <structs.h>            // structures that are used by the code in the firmware
#include <helper.h>             // useful helper functions
#include <patcher.h>            // macros used to create patches such as BLPatch, BPatch, ...
#include <nexioctls.h>          // ioctls added in the nexmon patch
#include <capabilities.h>       // capabilities included in a nexmon patch
#include <sendframe.h>          // sendframe functionality
#include <argprintf.h>

// Defined in injection.c.  Parses the radiotap header off an skb and hands the
// bare 802.11 frame to sendframe(); consumes (tx) or frees the skb on error.
int inject_frame(struct wlc_info *wlc, struct sk_buff *p);

// Headroom reserved in front of the injected frame so wlc_d11hdrs() can prepend
// the d11 TX header (PLCP + d11txh) without reallocating.  Borrowed from the
// bcm43455 ioctl-driven injection path.  The 43439 d11 tx header tops out near
// 0xb0 bytes (per wlc_get_txh_info), so 202 over-reserves it.
#define INJECT_HEADROOM 202

int
wlc_ioctl_hook(struct wlc_info *wlc, int cmd, char *arg, int len, void *wlc_if)
{
    argprintf_init(arg, len);
    int ret = IOCTL_ERROR;

    switch (cmd) {
        case NEX_GET_CAPABILITIES:
            if (len == 4) {
                memcpy(arg, &capabilities, 4);
                ret = IOCTL_SUCCESS;
            }
            break;

        case NEX_WRITE_TO_CONSOLE:
            if (len > 0) {
                arg[len-1] = 0;
                printf("ioctl: %s\n", arg);
                ret = IOCTL_SUCCESS;
            }
            break;

        case NEX_INJECT_FRAME:
            // arg is a single radiotap-prefixed 802.11 frame of length len,
            // fired by the host with cyw43_ioctl(IOCTL_CMD_SET(408), len, frame, ...)
            if (len > 0) {
                struct sk_buff *p = pkt_buf_get_skb(wlc->osh, len + INJECT_HEADROOM);

                if (p != 0) {
                    skb_pull(p, INJECT_HEADROOM);
                    memcpy(p->data, arg, len);
                    // Do not report success for a frame we never queued: a
                    // malformed radiotap header makes inject_frame return -1
                    // (and free the skb). The host must not read ret==0 as
                    // "transmitted". Note the wlc-down / no-scb drops still
                    // return 0 via the void sendframe; the over-the-air witness,
                    // not this ret, is the proof of transmission.
                    ret = inject_frame(wlc, p) == 0 ? IOCTL_SUCCESS : IOCTL_ERROR;
                } else {
                    ret = IOCTL_ERROR;
                }
            }
            break;

        default:
            ret = wlc_ioctl(wlc, cmd, arg, len, wlc_if);
    }

    return ret;
}

__attribute__((at(0x1EFC4, "", CHIP_VER_BCM43439a0, FW_VER_7_95_49_2271bb6)))
GenericPatch4(wlc_ioctl_hook, wlc_ioctl_hook + 1);
