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
#include <wrapper.h>            // wrapper definitions for functions that already exist in the firmware
#include <structs.h>            // structures that are used by the code in the firmware
#include <helper.h>             // useful helper functions
#include <patcher.h>            // macros used to craete patches such as BLPatch, BPatch, ...
#include <nexioctls.h>          // ioctls added in the nexmon patch
#include <capabilities.h>       // capabilities included in a nexmon patch
#include <sendframe.h>          // sendframe functionality
#include <objmem.h>             // Functions to access object memory
#include <ieee80211_radiotap.h> // Radiotap header related
#include <securitycookie.h>     // Security cookie related
#include <version.h>            // version information
#include <argprintf.h>          // allows to execute argprintf to print into the arg buffer

extern void *inject_frame(struct wlc_info *wlc, struct sk_buff *p);

struct inject_frame {
    unsigned short len;
    unsigned char pad;
    unsigned char type;
    char data[1];
};

int
wlc_ioctl_4xx(struct wlc_info *wlc, int cmd, char *arg, int len, void *wlc_if)
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

        case NEX_INJECT_FRAME:
            {
                sk_buff *p;
                int bytes_used = 0;
                struct inject_frame *frm = (struct inject_frame *) arg;

                while ((frm->len > 0) && (bytes_used + frm->len <= len)) {
                    // add a dummy radiotap header if frame does not contain one
                    if (frm->type == 0) {
                        p = pkt_buf_get_skb(wlc->osh, frm->len + 202 + 8 - 4);
                        skb_pull(p, 202);
                        struct ieee80211_radiotap_header *radiotap = 
                            (struct ieee80211_radiotap_header *) p->data;
                        
                        memset(radiotap, 0, sizeof(struct ieee80211_radiotap_header));
                        
                        radiotap->it_len = 8;
                        
                        skb_pull(p, 8);
                        memcpy(p->data, frm->data, frm->len - 4);
                        skb_push(p, 8);
                    } else {
                        p = pkt_buf_get_skb(wlc->osh, frm->len + 202 - 4);
                        skb_pull(p, 202);

                        memcpy(p->data, frm->data, frm->len - 4);
                    }

                    inject_frame(wlc, p);

                    bytes_used += frm->len;

                    frm = (struct inject_frame *) (arg + bytes_used);
                }

                ret = IOCTL_SUCCESS;
            }
            break;

        case NEX_PRINT_TIMERS:
            {
                struct _ctimeout *timers = (struct _ctimeout *) 0x180E6C;
                struct _ctimeout *this;
                int bytes_written = 0;

                if ((this = timers->next) == 0) {
                    printf("No timers\n");
                    ret = IOCTL_ERROR;
                } else {
                    while (this != 0 && len >= bytes_written + 64) {
                        bytes_written += sprintf(arg + bytes_written, 
                            "timer %p, fun %p, arg %p, %d ms\n", 
                            this, this->fun, this->arg, this->ms);
                        this = this->next;
                    }

                    ret = IOCTL_SUCCESS;
                }
            }
            break;

        case NEX_GET_SECURITYCOOKIE:
            if (len == 4) {
                *(int *) arg = get_securitycookie();
                ret = IOCTL_SUCCESS;
            }
            break;

        case NEX_SET_SECURITYCOOKIE:
            if (len == 4) {
                set_securitycookie(*(int *) arg);
                ret = IOCTL_SUCCESS;
            }
            break;

        case NEX_GET_WL_CNT:
            if (len >= sizeof(wl_cnt_t)) {
                wlc_statsupd(wlc);
                memcpy(arg, wlc->pub->_cnt, sizeof(wl_cnt_t));
                ret = IOCTL_SUCCESS;
            }
            break;

        case NEX_GET_VERSION_STRING:
            {
                if (len >= strlen(version)) {
                    memcpy(arg, version, strlen(version));
                    ret = IOCTL_SUCCESS;
                }
            }
            break;
    }

    return ret;
}