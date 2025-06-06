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

#include <ieee80211_radiotap.h>
#include <vendor_radiotap.h>

static const struct radiotap_align_size radiotap_nex_vendor_subns_0_sizes[] = {
    [RADIOTAP_NEX_TXDELAY] = { .align = 4, .size = 4, },
    [RADIOTAP_NEX_TXREPETITIONS] = { .align = 8, .size = 8, },
    [RADIOTAP_NEX_RATESPEC] = { .align = 4, .size = 4, },
};

static const struct ieee80211_radiotap_namespace radiotap_nex_vendor_ns[] = {
    [0] = {
        .n_bits = ARRAY_SIZE(radiotap_nex_vendor_subns_0_sizes),
        .align_size = radiotap_nex_vendor_subns_0_sizes,
        .oui = 0x004e4558, // NEX
        .subns = 0
    }
};

const struct ieee80211_radiotap_vendor_namespaces rtap_vendor_namespaces = {
    .ns = radiotap_nex_vendor_ns,
    .n_ns = ARRAY_SIZE(radiotap_nex_vendor_ns),
};
 