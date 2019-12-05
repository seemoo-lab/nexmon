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

#ifndef VENDOR_RADIOTAP_H
#define VENDOR_RADIOTAP_H

extern const struct ieee80211_radiotap_vendor_namespaces rtap_vendor_namespaces;

/* Name                                 Data type    	Units
 * ----                                 ---------    	-----
 *
 * RADIOTAP_NEX_TXDELAY               	s32	    		milliseconds
 *
 *      Value in milliseconds to wait before transmitting this frame
 *		for the first time
 *
 * RADIOTAP_NEX_TXREPETITIONS        	2 x s32    		unitless, milliseconds
 *
 *      Amount of how often this frame should be transmitted and the
 *		periodicity in milliseconds of the retransmissions. Setting
 *		the number of retransmissions to -1 leads to infinite 
 *		retransmissions
 *
 * RADIOTAP_NEX_RATESPEC               	u32	    		unitless
 *
 *      Define the ratespec according to the definitions in rates.h
 *		This value overrides the rate settings in the regular 
 *		radiotap header
 */
enum radiotap_nex_vendor_subns_0_type {
    RADIOTAP_NEX_TXDELAY = 0,
    RADIOTAP_NEX_TXREPETITIONS = 1,
    RADIOTAP_NEX_RATESPEC = 2
};

#endif /* VENDOR_RADIOTAP_H */
