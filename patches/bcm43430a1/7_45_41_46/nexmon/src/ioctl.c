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
#include <argprintf.h>
#include "karma.h"

uint32 mame82_opts = 0;

extern mame82_config_t *g_mame82_conf;


int 
wlc_ioctl_hook(struct wlc_info *wlc, int cmd, char *arg, int len, void *wlc_if)
{
	mame82_ioctl_arg_t *mame82_arg = NULL;
	void *dump_addr = NULL;
	
    argprintf_init(arg, len);
    int ret = IOCTL_ERROR;

	//printf("received ioctl cmd %d with arg %s\n", cmd, arg);

    switch (cmd) 
    {
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

        case 500: // dump wlif list
            {
                struct wlc_if *wlcif = wlc->wlcif_list;

                for (wlcif = wlc->wlcif_list;  wlcif != 0; wlcif = wlcif->next) 
                {
                    char ifname[32];

                    strncpy(ifname, wlcif->wlif == 0 ? wlc->wl->dev->name : wlcif->wlif->dev->name, sizeof(ifname));
                    ifname[sizeof(ifname) - 1] = '\0';

                    argprintf(" \"%s\" 0x%p type=%02x index=%02x flags=%02x\n", ifname, wlcif, wlcif->type, wlcif->index, wlcif->flags);
				}

                ret = IOCTL_SUCCESS;
            }
            break;
		case 666:
			printf("666 (MaMe82) called, arg %x\n", *arg);
		
			mame82_arg = (mame82_ioctl_arg_t *) arg;
			uint32 tmp = 0;
			
			switch(mame82_arg->type)
			{
				case MAME82_IOCTL_ARG_TYPE_SET_ENABLE_KARMA_PROBE:
					printf("Enable KARMA probe: %d\n", *mame82_arg->val);
					g_mame82_conf->karma_probes = *mame82_arg->val;
					break;
				case MAME82_IOCTL_ARG_TYPE_SET_ENABLE_KARMA_ASSOC:
					printf("Enable KARMA assoc: %d\n", *mame82_arg->val);
					g_mame82_conf->karma_assocs = *mame82_arg->val;
					break;
				case MAME82_IOCTL_ARG_TYPE_SET_ENABLE_KARMA_BEACON:
					printf("Enable KARMA beaconing: %d\n", *mame82_arg->val);
					g_mame82_conf->karma_beacons = *mame82_arg->val;
					
					//clear SSID list (filled from probes) in case KARMA beaconing gets disabled
					if (!(*mame82_arg->val)) clear_ssids(g_mame82_conf->ssids_karma);
					break;
				case MAME82_IOCTL_ARG_TYPE_SET_ENABLE_CUSTOM_BEACONS:
					printf("Enable custom beaconing: %d\n", *mame82_arg->val);
					g_mame82_conf->custom_beacons = *mame82_arg->val;
					break;	
				case MAME82_IOCTL_ARG_TYPE_SET_ENABLE_KARMA:
					printf("Enable KARMA (probe and assoc responses): %d\n", *mame82_arg->val);
					g_mame82_conf->karma_probes = *mame82_arg->val;
					g_mame82_conf->karma_assocs = *mame82_arg->val;
					
					//If Karma gets disabled, we clear the list of SSIDs which have been spotted from "old" probe requests
					if (!(*mame82_arg->val)) clear_ssids(g_mame82_conf->ssids_karma);
					break;
				case MAME82_IOCTL_ARG_TYPE_ADD_CUSTOM_SSID:
					printf("Add custom SSID len %d: %s\n", mame82_arg->len, mame82_arg->val);
					append_ssid(g_mame82_conf->ssids_custom, (char*) mame82_arg->val, MIN(mame82_arg->len, 32), g_mame82_conf->max_custom_beacon_ssids);
					break;
				case MAME82_IOCTL_ARG_TYPE_CLEAR_CUSTOM_SSIDS:
					printf("Clear custom SSIDs\n");
					clear_ssids(g_mame82_conf->ssids_custom);
					break;
				case MAME82_IOCTL_ARG_TYPE_CLEAR_KARMA_SSIDS:
					printf("Clear karma SSIDs\n");
					clear_ssids(g_mame82_conf->ssids_karma);
					break;
				case MAME82_IOCTL_ARG_TYPE_SET_CUSTOM_BEACON_AUTO_REMOVE_COUNT:
					memcpy(&tmp, mame82_arg->val, mame82_arg->len);
					g_mame82_conf->custom_beacon_autoremove = tmp;
					printf("Send a maximum of %d custom beacons without association request per SSID\n", g_mame82_conf->custom_beacon_autoremove);
					break;
				case MAME82_IOCTL_ARG_TYPE_SET_KARMA_BEACON_AUTO_REMOVE_COUNT:
					memcpy(&tmp, mame82_arg->val, mame82_arg->len);
					g_mame82_conf->karma_beacon_autoremove = tmp;
					printf("Send a maximum of %d karma beacons without association request per SSID\n", g_mame82_conf->karma_beacon_autoremove);
					break;
				case MAME82_IOCTL_ARG_TYPE_GET_CONFIG:
					//assure buffer is large enough
					if (len < sizeof(mame82_config_t))
					{
						printf("Tried to read mame82_config in buf of size %d, but %d needed\n", len, sizeof(mame82_config_t));
						return IOCTL_ERROR;
					}
					else printf("Dumping %d bytes of mame82_config to buf of size %d\n", sizeof(mame82_config_t), len);
					//cp back config struct (leaking list pointers ;-))
					memcpy(arg, g_mame82_conf, sizeof(mame82_config_t));
					break;
				case MAME82_IOCTL_ARG_TYPE_GET_MEM:
					//dump as many bytes as the buffer can hold of the address given as argument
					memcpy(&tmp, mame82_arg->val, mame82_arg->len);
					
					
					dump_addr = (void*) tmp;
					printf("Dumping %d bytes from %x\n", len, dump_addr);
					//Assure we're not dumping from regions causing a crash
					//valid 0x80 - 0x07ffff
					//valid 0x800000 - 0x89ffff
					if (!(
						((0x80 <= tmp) && (tmp < 0x080000) && (0x80 <= (tmp+len)) && ((tmp+len) < 0x080000)) ||
						((0x800000 <= tmp) && (tmp < 0x8A0000) && (0x800000 <= (tmp+len)) && ((tmp+len) < 0x8A0000))
					))
					{
						printf("Couldn't dump from this region, valid are [0x80 to 0x080000] and [0x800000 to 0x8A0000]\n");
						return IOCTL_ERROR;
					}
					
					memmove(arg, dump_addr, len);
					break;
				default:
					printf("Unknown command type %d, len %d, val %d\n", mame82_arg->type, mame82_arg->len, mame82_arg->val);
			}
		
			
			ret = IOCTL_SUCCESS;
			break;

        default:
            ret = wlc_ioctl(wlc, cmd, arg, len, wlc_if);
    }

    return ret;
}

__attribute__((at(0x4305c, "", CHIP_VER_BCM43430a1, FW_VER_7_45_41_46)))
GenericPatch4(wlc_ioctl_hook, wlc_ioctl_hook + 1);

