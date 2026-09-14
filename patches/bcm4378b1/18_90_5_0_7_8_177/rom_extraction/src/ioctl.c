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
#include <local_wrapper.h>

extern unsigned int fp_orig_data[][3];
extern unsigned int fp_orig_data_len;

int
wlc_doioctl_hook(void *wlc, int cmd, char *arg, int len, void *wlc_if)
{
    int ret = IOCTL_ERROR;

    switch (cmd) {
        // dump memory contents starting from address stored in arg
        // automatically removes flash patches
        case 0x602:
        {
            unsigned int start_addr = *(unsigned int *) arg;
            memcpy(arg, *(char **) arg, len);
            int i;
            for (i = 0; i < fp_orig_data_len; i++) {
                if ((fp_orig_data[i][0] >= start_addr) && (fp_orig_data[i][0] + 8 <= start_addr + len)) {
                    ((unsigned int *) arg)[(fp_orig_data[i][0] - start_addr) / 4] = fp_orig_data[i][1];
                    ((unsigned int *) arg)[(fp_orig_data[i][0] - start_addr) / 4 + 1] = fp_orig_data[i][2];
                }
            }
            ret = IOCTL_SUCCESS;
            break;
        }

        default:
            ret = wlc_doioctl(wlc, cmd, arg, len, wlc_if);
    }

    return ret;
}

__attribute__((at(0x466744, "", CHIP_VER_BCM4378b1, FW_VER_18_90_5_0_7_8_177)))
GenericPatch4(wlc_doioctl_hook, wlc_doioctl_hook + 1);
