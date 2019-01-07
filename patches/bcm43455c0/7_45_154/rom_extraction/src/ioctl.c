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
#include <capabilities.h>       // capabilities included in a nexmon patch
#include <sendframe.h>          // sendframe functionality
#include <version.h>            // version information
//#include <bcmpcie.h>
#include <argprintf.h>          // allows to execute argprintf to print into the arg buffer

extern unsigned int fp_orig_data[][2];
extern unsigned int fp_orig_data_len;

int
wlc_ioctl_hook(struct wlc_info *wlc, int cmd, char *arg, int len, void *wlc_if)
{
    int ret = IOCTL_ERROR;
    argprintf_init(arg, len);

    switch (cmd) {
        case 0x600:
            if (len >= 4)
                *(int *) arg = 0x11223344;
            ret = IOCTL_SUCCESS;
            break;

        // dump stored ROM values that were stored before flash patching
        case 0x601:
            memcpy(arg, fp_orig_data, len);
            ret = IOCTL_SUCCESS;
            break;

        // dump ROM contents starting from address stored in arg
        // automatically removes flash patches
        case 0x602:
        {
            unsigned int start_addr = *(unsigned int *) arg;
            memcpy(arg, *(char **) arg, len);
            int i;
            for (i = 0; i < fp_orig_data_len; i++) {
                if ((fp_orig_data[i][0] >= start_addr) && (fp_orig_data[i][0] < start_addr + len)) {
                    ((unsigned int *) arg)[(fp_orig_data[i][0] - start_addr) / 4] = fp_orig_data[i][1];
                }
            }
            ret = IOCTL_SUCCESS;
            break;
        }

        case 0x603: // read from memory
        {
            memcpy(arg, *(char **) arg, len);
            ret = IOCTL_SUCCESS;
            break;
        }

        case 0x604: // write to console
        {
            arg[len-1] = 0;
            printf("%s\n", arg);
            ret = IOCTL_SUCCESS;
            break;
        }

        case 0x605: // dump console
        {
            unsigned int *config = *(unsigned int **) 0x208e38;
            if (len >= config[3]) {
                memcpy(arg, (char *) (config[2] + config[4]), config[3] - config[4]);
                memcpy(arg + config[3] - config[4], (char *) config[2], config[4]);
                ret = IOCTL_SUCCESS;
            }
            break;
        }

        default:
            ret = wlc_ioctl(wlc, cmd, arg, len, wlc_if);
    }

    return ret;
}

__attribute__((at(0x1F1DE8, "", CHIP_VER_BCM4358, FW_VER_7_112_200_17)))
__attribute__((at(0x1F1EE8, "", CHIP_VER_BCM4358, FW_VER_7_112_201_3)))
__attribute__((at(0x210258, "", CHIP_VER_BCM43451b1, FW_VER_7_63_43_0)))
__attribute__((at(0x208F20, "", CHIP_VER_BCM43455c0, FW_VER_7_45_154)))
GenericPatch4(wlc_ioctl_hook, wlc_ioctl_hook + 1);
