/*
 * coWPAtty - Brute-force dictionary attack against WPA-PSK.
 *
 * Copyright (c) 2004-2005, Joshua Wright <jwright@hasborg.com>
 *
 * $Id: utils.h,v 4.1 2007-11-03 20:28:39 jwright Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. See COPYING for more
 * details.
 *
 * coWPAtty is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*
 * Significant code is graciously taken from the following:
 * wpa_supplicant by Jouni Malinen.  This tool would have been MUCH more
 * difficult for me if not for this code.  Thanks Jouni.
 */

/* Prototypes */
void lamont_hdump(unsigned char *bp, unsigned int length);
char *printmac(unsigned char *mac);
int IsBlank(char *s);
int radiotap_offset(pcap_t *p, struct pcap_pkthdr *h);


#define __swab16(x) \
({ \
        uint16_t __x = (x); \
        ((uint16_t)( \
                (((uint16_t)(__x) & (uint16_t)0x00ffU) << 8) | \
                (((uint16_t)(__x) & (uint16_t)0xff00U) >> 8) )); \
})

#ifdef WORDS_BIGENDIAN
#warning "Compiling for big-endian"
#define le16_to_cpu(x) __swab16(x)
#else
#define le16_to_cpu(x) (x)
#endif
