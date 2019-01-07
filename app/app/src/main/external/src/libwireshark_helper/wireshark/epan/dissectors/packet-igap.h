/* packet-igap.h
 * Declarations of routines for IGMP/IGAP packet disassembly
 * 2003, Endoh Akira <See AUTHORS for email>
 *
 * $Id: packet-igap.h 18196 2006-05-21 04:49:01Z sahlberg $
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

#ifndef __PACKET_IGAP_H__
#define __PACKET_IGAP_H__

#define IGMP_IGAP_JOIN  0x40
#define IGMP_IGAP_QUERY 0x41
#define IGMP_IGAP_LEAVE 0x42

int dissect_igap(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset);

#endif
