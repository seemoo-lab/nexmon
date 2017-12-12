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
#include "karma.h"

ssid_list_t* ssids_to_beacon = NULL;
extern uint32 mame82_opts; //decalred in ioctl.c

void
autostart(void)
{
	printf("autostart\n");
	
	/*** startup settings for KARMA mode ***/
	MAME82_ENABLE_OPTION(mame82_opts, MAME82_KARMA_PROBE_RESP); //Enable responding foreign probe requests
	MAME82_ENABLE_OPTION(mame82_opts, MAME82_KARMA_ASSOC_RESP); //Enable responding foreign ASSOC REQUESTS
	MAME82_ENABLE_OPTION(mame82_opts, MAME82_KARMA_BEACONING); //enable beaconing for already responded foreign probe requests (KARMA loud)
	
	ssids_to_beacon = (ssid_list_t*) malloc(sizeof(ssid_list_t), 4); //allignment needed ?
	printf("Pointer to SSID list %p\n", ssids_to_beacon);
	
}

__attribute__((at(0x2a94, "", CHIP_VER_BCM43430a1, FW_VER_7_45_41_46)))
HookPatch4(hndrte_idle, autostart, "push {r4, lr}\nmov r4, r0");
