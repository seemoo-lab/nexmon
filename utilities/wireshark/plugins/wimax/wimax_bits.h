/* wimax_bits.h
 * WiMax MAC Management UL-MAP Message decoder
 *
 * Copyright (c) 2007 by Intel Corporation.
 *
 * Author: Mike Harvey <michael.harvey@intel.com>
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1999 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __wimax_bits_h__
#define __wimax_bits_h__

#include <wsutil/pint.h>

/********************************************************************
 * Functions for working with nibbles and bits
 */

/* SWAR functions */
#define _BITS(n,hi,lo) (((n)>>(lo))&((1<<(((hi)-(lo))+1))-1))
#define _ADD_SWAP(x,y) { (x) = (x) + (y); (y) = (x) - (y); (x) = (x) - (y); }
#define _XOR_SWAP(x,y) { (x) ^= (y); (y) ^= (x); (x) ^= (y); }
#define _SWAP(x,y) do { int t = (x); (x) = (y); (y) = t; } while(0)


/********************************************************************
 * Functions for working with nibbles
 *
 */

#define NIBBLE_MASK 0x0F
#define BYTE_MASK 0xFF

/* extract the nibble at the given nibble address 'n' of tvbuff_t 't' */
#define TVB_NIB_NIBBLE(n,t) \
    (((n) & 1) \
    ?  tvb_get_guint8((t), (n)/2) & NIBBLE_MASK \
    : (tvb_get_guint8((t), (n)/2) >> 4) & NIBBLE_MASK)

/* extract the byte at the given nibble address 'n' of tvbuff_t 't' */
#define TVB_NIB_BYTE(n,t) \
    (n) & 1 \
    ? (tvb_get_ntohs((t), (n)/2) >> 4) & BYTE_MASK \
    : tvb_get_guint8((t), (n)/2)

/* extract 12 bits at the given nibble address */
#define TVB_NIB_BITS12(n,t) \
      (TVB_NIB_NIBBLE(n+2,t) | (TVB_NIB_BYTE(n,t) << 4))

/* extract the word at the given nibble address 'n' of tvbuff_t 't' */
#define TVB_NIB_WORD(n,t) \
    (n) & 1 \
    ? (gint)((tvb_get_ntohl((t), (n)/2) >> 12) & 0x0000FFFF) \
    : tvb_get_ntohs((t), (n)/2)

/* extract the word at the given nibble address 'n' of tvbuff_t 't' */
#define TVB_NIB_LONG(n,t) \
    (n) & 1 \
    ? (tvb_get_ntohl((t), (n)/2) << 4) | ((tvb_get_guint8((t), (n)/2 + 4) >> 4) & NIBBLE_MASK) \
    : tvb_get_ntohl((t), (n)/2)

/* Only currently used with nib == 1 or 2 */
#define TVB_NIB_NIBS(nib, t, num) \
    ((num) == 1 ? TVB_NIB_NIBBLE(nib,t) : \
    ((num) == 2 ? TVB_NIB_BYTE(nib,t) : \
    ((num) == 3 ? TVB_NIB_BITS12(nib,t) : \
    ((num) == 4 ? TVB_NIB_WORD(nib,t) : \
    0))))

/* to highlight nibfields correctly in wireshark
 * AddItem(..., WSADDR(buf,bit), WSLEN(bit), ...) */

/* determine starting byte to highlight a series of nibbles */
#define NIB_ADDR(nib) ((nib)/2)
/* determine number of bytes to highlight a series of nibbles */
#define NIB_LEN(nib,len)   ((1 +  ((nib)   &1) + (len))/2)

#define NIBHI(nib,len)  NIB_ADDR(nib),NIB_LEN(nib,len)

/********************************************************************
 * bitfield functions - for extracting bitfields from a buffer
 *
 * TODO: 64 bit functions use two 32-bit values;
 * would be better to use 32+8 bits to avoid overrunning buffers
 *
 */

/* find the byte-address for the bitfield */
#define ADDR(bit)   ((bit) / 8)
#define ADDR16(bit) ((bit) / 8)
#define ADDR32(bit) ((bit) / 8)

/* find the offset (from the MSB) to the start of the bitfield */
#define OFFSET(bit)    ((bit) % 8)
#define OFFSET16(bit)  ((bit) % 8)
#define OFFSET32(bit)  ((bit) % 8)

/* find the number of bits to shift right (SHIFT64 is upper dword) */
#define SHIFT(bit,num)    ( 8 - ((bit)%8) - (num))
#define SHIFT16(bit,num)  (16 - ((bit)%8) - (num))
#define SHIFT32(bit,num)  (32 - ((bit)%8) - (num))
#define SHIFT64a(bit,num) (num - (32 - OFFSET32(bit)))
#define SHIFT64b(bit,num) (32 - ((num) - (32 - OFFSET32(bit))))

/* create a mask to mask off the bitfield */
#define MASK8(num)       (0xFF >> (8 - (num)))
#define MASK16(num)      (0xFFFF >> (16 - (num)))
#define MASK32(num)      (0xFFFFFFFF >> (32 - (num)))
#define MASK64a(bit)     (MASK32(32 - OFFSET32(bit)))
#define MASK64b(bit,num) (MASK32(num - (32 - OFFSET32(bit))))

/* note that if you have a bitfield of length 2 or more, it may cross a
 * byte boundary so you should use TVB_BIT_BITS16 */

/* extract a single bit
 * bit ... bit address
 * tvb ... tvbuff_t
 */
#define TVB_BIT_BIT(bit, tvb) \
    (( tvb_get_guint8(tvb, ADDR(bit)) >> SHIFT(bit,1) ) & 0x1)

/* extract bitfield up to 9 bits
 * bit ... bit address
 * tvb ... tvbuff_t
 * num ... length of bitfield
 */
#define TVB_BIT_BITS16(bit, tvb, num) \
    (( tvb_get_ntohs(tvb, ADDR16(bit)) >> SHIFT16(bit,num) ) & MASK16(num))

/* extract bitfield up to 24 bits
 * bit ... bit address
 * tvb ... tvbuff_t
 * num ... length of bitfield
 */

#define TVB_BIT_BITS32(bit, tvb, num) \
      ((tvb_get_ntohl(tvb, ADDR32(bit)) >> SHIFT32(bit,num) ) & MASK32(num))

/* bitfield up to 32 bits */
#define TVB_BIT_BITS64a(bit, tvb, num) \
      ((tvb_get_ntohl(tvb, ADDR32(bit)) & MASK64a(bit)) << SHIFT64a(bit,num))

#define TVB_BIT_BITS64b(bit, tvb, num) \
      ((tvb_get_ntohl(tvb, ADDR32(bit)+4) >> SHIFT64b(bit,num) ) & MASK64b(bit,num))

#define TVB_BIT_BITS64(bit, tvb, num) \
      ( (OFFSET32(bit)+(num)) <= 32 \
      ? TVB_BIT_BITS32(bit,tvb,num) \
      : TVB_BIT_BITS64a(bit,tvb,num) \
      | TVB_BIT_BITS64b(bit,tvb,num) )

#define TVB_BIT_BITS(bit, tvb, num) \
    ((num) ==  1 ? (gint)TVB_BIT_BIT(bit,tvb) : \
    ((num) <=  9 ? (gint)TVB_BIT_BITS16(bit,tvb,num) : \
    ((num) <= 24 ? (gint)TVB_BIT_BITS32(bit,tvb,num) : \
    ((num) <= 32 ? (gint)TVB_BIT_BITS64(bit,tvb,num) : \
                   (gint)0 ))))

/* to highlight bitfields correctly in wireshark
 * AddItem(..., WSADDR(buf,bit), WSLEN(bit), ...) */

/* determine starting byte to highlight a series of nibbles */
#define BIT_ADDR(bit) (ADDR(bit))
/* determine number of bytes to highlight */
#define BIT_LEN(bit,len) (1 + ((OFFSET(bit) + len - 1) / 8))

#define BITHI(bit,len)  BIT_ADDR(bit),BIT_LEN(bit,len)

/********************************************************************
 * padding functions - return number of nibbles/bits needed to
 * pad to a byte boundary */

#define BIT_PADDING(bit, bits) ((bit) % (bits)) ? ((bits) - ((bit) % (bits))) : 0
#define NIB_PADDING(nib) ((nib) & 0x1)

/********************************************************************
 * conversion functions - between bytes, nibbles, and bits */

#define BYTE_TO_BIT(n)  ((n) * 8)
#define BYTE_TO_NIB(n)  ((n) * 2)

#define BIT_TO_BYTE(n)  ((n) / 8)
#define BIT_TO_NIB(n)   ((n) / 4)

#define NIB_TO_BYTE(n)  ((n) / 2)
#define NIB_TO_BIT(n)   ((n) * 4)

#endif

