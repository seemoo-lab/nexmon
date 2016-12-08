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
#include <ieee80211_radiotap.h>

inline uint16_t
get_unaligned_le16(uint8 *p) {
    return p[0] | p[1] << 8;
}

inline uint32_t
get_unaligned_le32(uint8 *p) {
    return p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24;
}

/* see: https://github.com/spotify/linux/blob/master/net/wireless/radiotap.c */

/**
 * ieee80211_radiotap_iterator_init - radiotap parser iterator initialization
 * @iterator: radiotap_iterator to initialize
 * @radiotap_header: radiotap header to parse
 * @max_length: total length we can parse into (eg, whole packet length)
 *
 * Returns: 0 or a negative error code if there is a problem.
 *
 * This function initializes an opaque iterator struct which can then
 * be passed to ieee80211_radiotap_iterator_next() to visit every radiotap
 * argument which is present in the header.  It knows about extended
 * present headers and handles them.
 *
 * How to use:
 * call __ieee80211_radiotap_iterator_init() to init a semi-opaque iterator
 * struct ieee80211_radiotap_iterator (no need to init the struct beforehand)
 * checking for a good 0 return code.  Then loop calling
 * __ieee80211_radiotap_iterator_next()... it returns either 0,
 * -ENOENT if there are no more args to parse, or -1 if there is a problem.
 * The iterator's @this_arg member points to the start of the argument
 * associated with the current argument index that is present, which can be
 * found in the iterator's @this_arg_index member.  This arg index corresponds
 * to the IEEE80211_RADIOTAP_... defines.
 *
 * Radiotap header length:
 * You can find the CPU-endian total radiotap header length in
 * iterator->max_length after executing ieee80211_radiotap_iterator_init()
 * successfully.
 *
 * Alignment Gotcha:
 * You must take care when dereferencing iterator.this_arg
 * for multibyte types... the pointer is not aligned.  Use
 * get_unaligned((type *)iterator.this_arg) to dereference
 * iterator.this_arg for type "type" safely on all arches.
 *
 * Example code:
 * See Documentation/networking/radiotap-headers.txt
 */

int ieee80211_radiotap_iterator_init(
    struct ieee80211_radiotap_iterator *iterator,
    struct ieee80211_radiotap_header *radiotap_header,
    int max_length)
{
    /* Linux only supports version 0 radiotap format */
    if (radiotap_header->it_version)
        return -1;

    /* sanity check for allowed length and radiotap length field */
    if (max_length < get_unaligned_le16((uint8 *) &radiotap_header->it_len))
        return -1;

    iterator->rtheader = radiotap_header;
    iterator->max_length = get_unaligned_le16((uint8 *) &radiotap_header->it_len);
    iterator->arg_index = 0;
    iterator->bitmap_shifter = get_unaligned_le32((uint8 *) &radiotap_header->it_present);
    //The original radiotap header (without sub-header) consists of 8 byte
    iterator->arg = (uint8 *)radiotap_header + sizeof(uint8) * 8;
    iterator->this_arg = 0;

    /* find payload start allowing for extended bitmap(s) */

    if (iterator->bitmap_shifter & (1<<IEEE80211_RADIOTAP_EXT)) {
        while (get_unaligned_le32(iterator->arg) & (1<<IEEE80211_RADIOTAP_EXT)) {
            iterator->arg += sizeof(uint32);

            /*
             * check for insanity where the present bitmaps
             * keep claiming to extend up to or even beyond the
             * stated radiotap header length
             */

            if (((unsigned long)iterator->arg - (unsigned long)iterator->rtheader) > iterator->max_length)
                return -1;
        }

        iterator->arg += sizeof(uint32);

        /*
         * no need to check again for blowing past stated radiotap
         * header length, because ieee80211_radiotap_iterator_next
         * checks it before it is dereferenced
         */
    }

    /* we are all initialized happily */

    return 0;
}

/* see: https://github.com/spotify/linux/blob/master/net/wireless/radiotap.c */

/**
 * ieee80211_radiotap_iterator_next - return next radiotap parser iterator arg
 * @iterator: radiotap_iterator to move to next arg (if any)
 *
 * Returns: 0 if there is an argument to handle,
 * -ENOENT if there are no more args or -1
 * if there is something else wrong.
 *
 * This function provides the next radiotap arg index (IEEE80211_RADIOTAP_*)
 * in @this_arg_index and sets @this_arg to point to the
 * payload for the field.  It takes care of alignment handling and extended
 * present fields.  @this_arg can be changed by the caller (eg,
 * incremented to move inside a compound argument like
 * IEEE80211_RADIOTAP_CHANNEL).  The args pointed to are in
 * little-endian format whatever the endianess of your CPU.
 *
 * Alignment Gotcha:
 * You must take care when dereferencing iterator.this_arg
 * for multibyte types... the pointer is not aligned.  Use
 * get_unaligned((type *)iterator.this_arg) to dereference
 * iterator.this_arg for type "type" safely on all arches.
 */

int ieee80211_radiotap_iterator_next(
    struct ieee80211_radiotap_iterator *iterator)
{

    /*
     * small length lookup table for all radiotap types we heard of
     * starting from b0 in the bitmap, so we can walk the payload
     * area of the radiotap header
     *
     * There is a requirement to pad args, so that args
     * of a given length must begin at a boundary of that length
     * -- but note that compound args are allowed (eg, 2 x u16
     * for IEEE80211_RADIOTAP_CHANNEL) so total arg length is not
     * a reliable indicator of alignment requirement.
     *
     * upper nybble: content alignment for arg
     * lower nybble: content length for arg
     */

    uint8 rt_sizes[] = {
        [IEEE80211_RADIOTAP_TSFT] = 0x88,
        [IEEE80211_RADIOTAP_FLAGS] = 0x11,
        [IEEE80211_RADIOTAP_RATE] = 0x11,
        [IEEE80211_RADIOTAP_CHANNEL] = 0x24,
        [IEEE80211_RADIOTAP_FHSS] = 0x22,
        [IEEE80211_RADIOTAP_DBM_ANTSIGNAL] = 0x11,
        [IEEE80211_RADIOTAP_DBM_ANTNOISE] = 0x11,
        [IEEE80211_RADIOTAP_LOCK_QUALITY] = 0x22,
        [IEEE80211_RADIOTAP_TX_ATTENUATION] = 0x22,
        [IEEE80211_RADIOTAP_DB_TX_ATTENUATION] = 0x22,
        [IEEE80211_RADIOTAP_DBM_TX_POWER] = 0x11,
        [IEEE80211_RADIOTAP_ANTENNA] = 0x11,
        [IEEE80211_RADIOTAP_DB_ANTSIGNAL] = 0x11,
        [IEEE80211_RADIOTAP_DB_ANTNOISE] = 0x11,
        [IEEE80211_RADIOTAP_RX_FLAGS] = 0x22,
        [IEEE80211_RADIOTAP_TX_FLAGS] = 0x22,
        [IEEE80211_RADIOTAP_RTS_RETRIES] = 0x11,
        [IEEE80211_RADIOTAP_DATA_RETRIES] = 0x11,
        /*
         * add more here as they are defined in
         * include/net/ieee80211_radiotap.h
         */
    };

    /*
     * for every radiotap entry we can at
     * least skip (by knowing the length)...
     */

    while (iterator->arg_index < sizeof(rt_sizes)) {
        int hit = 0;
        int pad;

        if (!(iterator->bitmap_shifter & 1))
            goto next_entry; /* arg not present */

        /*
         * arg is present, account for alignment padding
         *  8-bit args can be at any alignment
         * 16-bit args must start on 16-bit boundary
         * 32-bit args must start on 32-bit boundary
         * 64-bit args must start on 64-bit boundary
         *
         * note that total arg size can differ from alignment of
         * elements inside arg, so we use upper nybble of length
         * table to base alignment on
         *
         * also note: these alignments are ** relative to the
         * start of the radiotap header **.  There is no guarantee
         * that the radiotap header itself is aligned on any
         * kind of boundary.
         *
         * the above is why get_unaligned() is used to dereference
         * multibyte elements from the radiotap area
         */

        pad = (((unsigned long)iterator->arg) -
            ((unsigned long)iterator->rtheader)) &
            ((rt_sizes[iterator->arg_index] >> 4) - 1);

        if (pad)
            iterator->arg +=
                (rt_sizes[iterator->arg_index] >> 4) - pad;

        /*
         * this is what we will return to user, but we need to
         * move on first so next call has something fresh to test
         */
        iterator->this_arg_index = iterator->arg_index;
        iterator->this_arg = iterator->arg;
        hit = 1;

        /* internally move on the size of this arg */
        iterator->arg += rt_sizes[iterator->arg_index] & 0x0f;

        /*
         * check for insanity where we are given a bitmap that
         * claims to have more arg content than the length of the
         * radiotap section.  We will normally end up equalling this
         * max_length on the last arg, never exceeding it.
         */
        if (((unsigned long)iterator->arg - (unsigned long)iterator->rtheader) > iterator->max_length)
            return -1;

    next_entry:
        iterator->arg_index++;
        if ((iterator->arg_index & 31) == 0) {
            /* completed current u32 bitmap */
            if (iterator->bitmap_shifter & 1) {
                /* b31 was set, there is more */
                /* move to next u32 bitmap */
                iterator->bitmap_shifter =
                    get_unaligned_le32((uint8 *) iterator->next_bitmap);
                iterator->next_bitmap++;
            } else
                /* no more bitmaps: end */
                iterator->arg_index = sizeof(rt_sizes);
        } else /* just try the next bit */
            iterator->bitmap_shifter >>= 1;

        /* if we found a valid arg earlier, return it now */
        if (hit)
            return 0;
    }

    /* we don't know how to handle any more args, we're done */
    return -2;
}
