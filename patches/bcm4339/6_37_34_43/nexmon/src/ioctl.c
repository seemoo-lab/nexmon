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
#include <objmem.h>             // Functions to access object memory

int 
wlc_ioctl_hook(struct wlc_info *wlc, int cmd, char *arg, int len, void *wlc_if)
{
    int ret = IOCTL_ERROR;

    switch (cmd) {
        case NEX_GET_CAPABILITIES:
            // sends back the chips capabilities
            if (len == 4) {
                memcpy(arg, &capabilities, 4);
                ret = IOCTL_SUCCESS;
            }
            break;

        case NEX_WRITE_TO_CONSOLE:
            // writes the string from arg to the console
            if (len > 0) {
                arg[len-1] = 0;
                printf("ioctl: %s\n", arg);
                ret = IOCTL_SUCCESS; 
            }
            break;

        case NEX_GET_PHYREG:
            // reads the value from arg[0] to arg[0]
            if(wlc->hw->up && len >= 4) {
                wlc_phyreg_enter(wlc->band->pi);
                *(int *) arg =  phy_reg_read(wlc->band->pi, ((int *) arg)[0]);
                wlc_phyreg_exit(wlc->band->pi);
                ret = IOCTL_SUCCESS;
            }
            break;

        case NEX_SET_PHYREG:
            // writes the value arg[1] to physical layer register arg[0]
            if(wlc->hw->up && len >= 8) {
                wlc_phyreg_enter(wlc->band->pi);
                phy_reg_write(wlc->band->pi, ((int *) arg)[1], ((int *) arg)[0]);
                wlc_phyreg_exit(wlc->band->pi);
                ret = IOCTL_SUCCESS;
            }
            break;

        case NEX_READ_OBJMEM:
            if(wlc->hw->up && len >= 4)
            {
                int addr = ((int *) arg)[0];
                int i = 0;
                
                for (i = 0; i < len/4; i++) {
                    wlc_bmac_read_objmem32_objaddr(wlc->hw, addr + i, &((unsigned int *) arg)[i]);
                }

                ret = IOCTL_SUCCESS;
            }
            break;

        case NEX_WRITE_OBJMEM:
            if(wlc->hw->up && len >= 5)
            {
                int addr = ((int *) arg)[0];
                int i = 0;
                
                for (i = 0; i < (len-4)/8; i+=2) {
                    wlc_bmac_write_objmem64_objaddr(wlc->hw, addr + i, ((unsigned int *) arg)[i + 1], ((unsigned int *) arg)[i + 2]);
                }

                switch((len-4) % 8) {
                    case 4:
                        wlc_bmac_write_objmem32_objaddr(wlc->hw, addr + i, ((unsigned int *) arg)[i + 1]);
                        break;
                }

                ret = IOCTL_SUCCESS;
            }
            break;

        default:
            ret = wlc_ioctl(wlc, cmd, arg, len, wlc_if);
    }

    return ret;
}

__attribute__((at(0x1F3488, "", CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704)))
GenericPatch4(wlc_ioctl_hook, wlc_ioctl_hook + 1);
