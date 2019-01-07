/* crc16.h
 * Declaration of CRC-16 routines and table
 *
 * 2004 Richard van der Hoff <richardv@mxtelecom.com>
 *
 * $Id: crc16.h 27790 2009-03-18 21:59:30Z gerald $
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@xxxxxxxxxxxx>
 * Copyright 1998 Gerald Combs
 *
 * Copied from README.developer
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef __CRC16_H_
#define __CRC16_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Calculate the CCITT/ITU/CRC-16 16-bit CRC

   (parameters for this CRC are:
       Polynomial: x^16 + x^12 + x^5 + 1  (0x1021);
       Start value 0xFFFF;
       XOR result with 0xFFFF;
       First bit is LSB)
*/

/** Compute CRC16 CCITT checksum of a buffer of data.
 @param buf The buffer containing the data.
 @param len The number of bytes to include in the computation.
 @return The CRC16 CCITT checksum. */
extern guint16 crc16_ccitt(const guint8 *buf, guint len);

/** Compute CRC16 X.25 CCITT checksum of a buffer of data.
 @param buf The buffer containing the data.
 @param len The number of bytes to include in the computation.
 @return The CRC16 X.25 CCITT checksum. */
extern guint16 crc16_x25_ccitt(const guint8 *buf, guint len);

/** Compute CRC16 CCITT checksum of a buffer of data.  If computing the
 *  checksum over multiple buffers and you want to feed the partial CRC16
 *  back in, remember to take the 1's complement of the partial CRC16 first.
 @param buf The buffer containing the data.
 @param len The number of bytes to include in the computation.
 @param seed The seed to use.
 @return The CRC16 CCITT checksum (using the given seed). */
extern guint16 crc16_ccitt_seed(const guint8 *buf, guint len, guint16 seed);

/** Compute CRC16 CCITT checksum of a tv buffer.
 @param tvb The tv buffer containing the data.
 @param len The number of bytes to include in the computation.
 @return The CRC16 CCITT checksum. */
extern guint16 crc16_ccitt_tvb(tvbuff_t *tvb, guint len);

/** Compute CRC16 X.25 CCITT checksum of a tv buffer.
 @param tvb The tv buffer containing the data.
 @param len The number of bytes to include in the computation.
 @return The CRC16 X.25 CCITT checksum. */
extern guint16 crc16_x25_ccitt_tvb(tvbuff_t *tvb, guint len);

/** Compute CRC16 CCITT checksum of a tv buffer.
 @param tvb The tv buffer containing the data.
 @param offset The offset into the tv buffer.
 @param len The number of bytes to include in the computation.
 @return The CRC16 CCITT checksum. */
extern guint16 crc16_ccitt_tvb_offset(tvbuff_t *tvb, guint offset, guint len);

/** Compute CRC16 CCITT checksum of a tv buffer.  If computing the
 *  checksum over multiple tv buffers and you want to feed the partial CRC16
 *  back in, remember to take the 1's complement of the partial CRC16 first.
 @param tvb The tv buffer containing the data.
 @param len The number of bytes to include in the computation.
 @param seed The seed to use.
 @return The CRC16 CCITT checksum (using the given seed). */
extern guint16 crc16_ccitt_tvb_seed(tvbuff_t *tvb, guint len, guint16 seed);

/** Compute CRC16 CCITT checksum of a tv buffer.  If computing the
 *  checksum over multiple tv buffers and you want to feed the partial CRC16
 *  back in, remember to take the 1's complement of the partial CRC16 first.
 @param tvb The tv buffer containing the data.
 @param offset The offset into the tv buffer.
 @param len The number of bytes to include in the computation.
 @param seed The seed to use.
 @return The CRC16 CCITT checksum (using the given seed). */
extern guint16 crc16_ccitt_tvb_offset_seed(tvbuff_t *tvb, guint offset,
                                           guint len, guint16 seed);

/** Compute the "plain" CRC16 checksum of a tv buffer using the following
 * parameters:
 *    Width        = 16
 *    Poly         = 0x8005
 *    XorIn        = 0x0000
 *    ReflectIn    = True
 *    XorOut       = 0x0000
 *    ReflectOut   = True
 *    Algorithm    = table-driven
 *    Direct       = True
 @param tvb The tv buffer containing the data.
 @param offset The offset into the tv buffer.
 @param len The number of bytes to include in the computation.
 @return The CRC16 checksum. */
extern guint16 crc16_plain_tvb_offset(tvbuff_t *tvb, guint offset, guint len);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* crc16.h */
