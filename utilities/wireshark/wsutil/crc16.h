/* crc16.h
 * Declaration of CRC-16 routines and table
 *
 * 2004 Richard van der Hoff <richardv@mxtelecom.com>
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
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

#ifndef __CRC16_H__
#define __CRC16_H__

#include "ws_symbol_export.h"

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
WS_DLL_PUBLIC guint16 crc16_ccitt(const guint8 *buf, guint len);

/** Compute CRC16 X.25 CCITT checksum of a buffer of data.
 @param buf The buffer containing the data.
 @param len The number of bytes to include in the computation.
 @return The CRC16 X.25 CCITT checksum. */
WS_DLL_PUBLIC guint16 crc16_x25_ccitt_seed(const guint8 *buf, guint len, guint16 seed);

/** Compute CRC16 CCITT checksum of a buffer of data.  If computing the
 *  checksum over multiple buffers and you want to feed the partial CRC16
 *  back in, remember to take the 1's complement of the partial CRC16 first.
 @param buf The buffer containing the data.
 @param len The number of bytes to include in the computation.
 @param seed The seed to use.
 @return The CRC16 CCITT checksum (using the given seed). */
WS_DLL_PUBLIC guint16 crc16_ccitt_seed(const guint8 *buf, guint len, guint16 seed);

/** Compute the 16bit CRC_A value of a buffer as defined in ISO14443-3.
 @param buf The buffer containing the data.
 @param len The number of bytes to include in the computation.
 @return the CRC16 checksum for the buffer */
WS_DLL_PUBLIC guint16 crc16_iso14443a(const guint8 *buf, guint len);

/** Calculates a CRC16 checksum for the given buffer with the polynom
 *  0x5935 using a precompiled CRC table
 * @param buf a pointer to a buffer of the given length
 * @param len the length of the given buffer
 * @param seed The seed to use.
 * @return the CRC16 checksum for the buffer
 */
WS_DLL_PUBLIC guint16 crc16_0x5935(const guint8 *buf, guint32 len, guint16 seed);

/** Calculates a CRC16 checksum for the given buffer with the polynom
 *  0x755B using a precompiled CRC table
 * @param buf a pointer to a buffer of the given length
 * @param len the length of the given buffer
 * @param seed The seed to use.
 * @return the CRC16 checksum for the buffer
 */
WS_DLL_PUBLIC guint16 crc16_0x755B(const guint8 *buf, guint32 len, guint16 seed);

/** Computes CRC16 checksum for the given data with the polynom 0x9949 using
 *  precompiled CRC table
 * @param buf a pointer to a buffer of the given length
 * @param len the length of the given buffer
 * @param seed The seed to use.
 * @return the CRC16 checksum for the buffer
 */
WS_DLL_PUBLIC guint16 crc16_0x9949_seed(const guint8 *buf, guint len, guint16 seed);

/** Computes CRC16 checksum for the given data with the polynom 0x3D65 using
 *  precompiled CRC table
 * @param buf a pointer to a buffer of the given length
 * @param len the length of the given buffer
 * @param seed The seed to use.
 * @return the CRC16 checksum for the buffer
 */
WS_DLL_PUBLIC guint16 crc16_0x3D65_seed(const guint8 *buf, guint len, guint16 seed);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* crc16.h */
