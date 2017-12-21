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


mame82_config_t *g_mame82_conf = NULL;

void
autostart(void)
{
	printf("autostart\n");
	
	
	g_mame82_conf = (mame82_config_t *) malloc(sizeof(mame82_config_t), 4);
	memset(g_mame82_conf, 0, sizeof(mame82_config_t));
	g_mame82_conf->ssids_custom = (ssid_list_t*) malloc(sizeof(ssid_list_t), 4);
	g_mame82_conf->ssids_karma = (ssid_list_t*) malloc(sizeof(ssid_list_t), 4);
	
	/* Disable all KARMA related stuff by default, to avoid changing legacy nexmon default behavior */
	
	//startup (default) config
	g_mame82_conf->karma_probes = 0;
	g_mame82_conf->karma_assocs = 0;			
	g_mame82_conf->debug_out = 0;
	g_mame82_conf->karma_beacons = 0;
	g_mame82_conf->custom_beacons = 0;
	g_mame82_conf->custom_beacon_autoremove = 0; //never remove custom SSIDs after if they don't receive an assoc req
	g_mame82_conf->karma_beacon_autoremove = 600; //remove SSIDs extracted from received probe requests after sending 600 beacons (1 minute) without receiving an assoc req
	
	/*
	 * There're two lists of SSIDs for which beacons are sent:
	 * 1) 	The one for spotted SSIDs from probe requests, for which in addition to the according probe responses (karma_probes = 1)
	 * 		beacons could be send (karma_beacons = 1). This would allow to spawn potetial known APs to stations, which have been probed
	 * 		for by other stations, which are more chatty. This list could be fast growing, depending on the probing devices around
	 * 2) 	The list for custom SSIDs which have been added manually by the user (they are send if custom_beacons = 1 is set).
	 * 
	 * The current implementations is able to send one beacon on every 2s, which makes a rough maximum of 50 per beacon interval 
	 * (which is about 100ms per default). The more traffic is on the channel, the more retransmissions are needed for management frames 
	 * (including probe responses and beacons), which puts another limit on the maximum SSIDs which could be beaconed.
	 * For this reasons, the overall count of the SSIDs has been limited to 40, which splits into 20 for custom_beacon_ssids and
	 * 20 for karma_beacon_ssids. So it is not adviced to change these limits.
	 * 
	 * When the limits are reached, it isn't possible to add more SSID. The SSID lists could be cleared manually (via ioctl from
	 * the additional python utility) or auto-removeed. Auto-remove deletes SSIDs for which no association has taken place in a
	 * predefined period. This period is measured in beacons which have been sent, without an association. With the beacon interval
	 * of 100ms, about 10 beacons are sent per second. 
	 * If for example 'karma_beacon_autoremove' is set to 600, every SSID which hasn't seen an association after 60 seconds is removed -
	 * where SSIDs are added to this list in case a probe request was spotted (at least, if karma_beacons = 1).
	 * custom_beacon_autoremove has the same meaning, but targets SSIDs which have been added manually by the user.
	 * If custom_beacon_autoremove or karma_beacon_autoremove are set to 0, the beacons are never removed from the list.
	 * 
	 */
	g_mame82_conf->max_custom_beacon_ssids = 20;
	g_mame82_conf->max_karma_beacon_ssids = 20;
	
/*	
	cur_ssid_ptr = custom_ssids;
	while (*cur_ssid_ptr)
	{
		append_ssid(g_mame82_conf->ssids_custom, *cur_ssid_ptr, strlen(*cur_ssid_ptr));
		cur_ssid_ptr++;
	}
	
	access_test = get_ssid_entry(g_mame82_conf->ssids_custom, "tmobile", 7);
	if (access_test) printf("SSID found '%s'\n", access_test->ssid);
*/	
}

__attribute__((at(0x2a94, "", CHIP_VER_BCM43430a1, FW_VER_7_45_41_46)))
HookPatch4(hndrte_idle, autostart, "push {r4, lr}\nmov r4, r0");
