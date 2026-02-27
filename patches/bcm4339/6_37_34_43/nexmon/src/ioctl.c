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
#include <version.h>            // version information
#include <argprintf.h>          // allows to execute argprintf to print into the arg buffer

extern int wlc_ioctl_4xx(struct wlc_info *wlc, int cmd, char *arg, int len, void *wlc_if);
extern int wlc_ioctl_5xx(struct wlc_info *wlc, int cmd, char *arg, int len, void *wlc_if);
extern int wlc_ioctl_6xx(struct wlc_info *wlc, int cmd, char *arg, int len, void *wlc_if);
extern int wlc_ioctl_7xx(struct wlc_info *wlc, int cmd, char *arg, int len, void *wlc_if);
extern int wlc_ioctl_8xx(struct wlc_info *wlc, int cmd, char *arg, int len, void *wlc_if);

int 
wlc_ioctl_hook(struct wlc_info *wlc, int cmd, char *arg, int len, void *wlc_if)
{
    int ret = IOCTL_ERROR;
    argprintf_init(arg, len);

    if (cmd >= 400 && cmd < 500)
        ret = wlc_ioctl_4xx(wlc, cmd, arg, len, wlc_if);
    else if (cmd >= 500 && cmd < 600)
        ret = wlc_ioctl_5xx(wlc, cmd, arg, len, wlc_if);
    else if (cmd >= 600 && cmd < 700)
        ret = wlc_ioctl_6xx(wlc, cmd, arg, len, wlc_if);
    else if (cmd >= 700 && cmd < 800)
        ret = wlc_ioctl_7xx(wlc, cmd, arg, len, wlc_if);
    else if (cmd >= 800 && cmd < 900)
        ret = wlc_ioctl_8xx(wlc, cmd, arg, len, wlc_if);
    else
        ret = wlc_ioctl(wlc, cmd, arg, len, wlc_if);

    return ret;
}

__attribute__((at(0x1F3488, "", CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704)))
GenericPatch4(wlc_ioctl_hook, wlc_ioctl_hook + 1);
