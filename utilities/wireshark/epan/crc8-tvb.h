/* crc8.h
 *
 * Checks the checksum (FCS) of the 3G TS 27.010 Multiplexing protocol.
 * The algorithm to check the FCS is described in "3G TS 27.010 V2.0.0 (1999-06)"
 * See: www.3gpp.org/ftp/tsg_t/TSG_T/TSGT_04/docs/PDFs/TP-99119.pdf
 * or: http://www.3gpp.org/ftp/Specs/html-info/27010.htm
 *
 * Polynom: (x^8 + x^2 + x^1 + 1)
 *
 * 2011 Hans-Christoph Schemmel <hans-christoph.schemmel[AT]cinterion.com>
 * 2014 Philip Rosenberg-Watt <p.rosenberg-watt[at]cablelabs.com>
 *  + Added CRC-8 for IEEE 802.3 EPON, with shift register initialized to 0x00
 *    See IEEE Std 802.3-2012 Section 5, Clause 65.1.3.2.3.
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


/**
 * Check the final crc value(Receiver code)
 *
 * \param p The tv buffer containing the data.
 * \param len Number of bytes in the message.
 * \param offset Offset in the message.
 * \param received_fcs The received FCS.
 * \return     Returns TRUE if the checksum is correct, FALSE if it is not correct
 *****************************************************************************/

#ifndef __CRC8_TVB_H__
#define __CRC8_TVB_H__

extern gboolean check_fcs(tvbuff_t *p, guint8 len, guint8 offset, guint8 received_fcs);
extern guint8 get_crc8_ieee8023_epon(tvbuff_t *p, guint8 len, guint8 offset);

#endif /* __CRC8_TVB_H__ */
