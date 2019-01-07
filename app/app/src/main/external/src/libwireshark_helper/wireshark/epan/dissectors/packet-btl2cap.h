/* packet-btl2cap.h
 *
 * $Id: packet-btl2cap.h 35323 2011-01-02 08:13:40Z krj $
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __PACKET_BTL2CAP_H__
#define __PACKET_BTL2CAP_H__

#define BTL2CAP_PSM_SDP			0x0001
#define BTL2CAP_PSM_RFCOMM		0x0003
#define BTL2CAP_PSM_BNEP		0x000f
#define BTL2CAP_PSM_HID_CTRL	0x0011
#define BTL2CAP_PSM_HID_INTR	0x0013
#define BTL2CAP_PSM_AVCTP_CTRL	0x0017
#define BTL2CAP_PSM_AVDTP		0x0019
#define BTL2CAP_PSM_AVCTP_BRWS	0x001b

#define BTL2CAP_DYNAMIC_PSM_START   0x1000

#define BTL2CAP_FIXED_CID_NULL		0x0000
#define BTL2CAP_FIXED_CID_SIGNAL	0x0001
#define BTL2CAP_FIXED_CID_CONNLESS	0x0002
#define BTL2CAP_FIXED_CID_AMP_MAN	0x0003
#define BTL2CAP_FIXED_CID_AMP_TEST	0x003F
#define BTL2CAP_FIXED_CID_MAX		0x0040

/* This structure is passed to higher layer protocols through
 * pinfo->private_data so that they can track "conversations" based on
 * chandle, cid and direction
 */
typedef struct _btl2cap_data_t {
	guint16 chandle;  /* only low 12 bits used */
	guint16 cid;
} btl2cap_data_t;

#endif
