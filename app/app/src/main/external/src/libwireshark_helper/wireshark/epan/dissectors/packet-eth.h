/* packet-eth.h
 *
 * $Id: packet-eth.h 33255 2010-06-19 01:16:29Z stig $
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

#ifndef __PACKET_ETH_H__
#define __PACKET_ETH_H__

typedef struct _eth_hdr {
	address dst;
	address src;
	guint16 type;
} eth_hdr;

void capture_eth(const guchar *, int, int, packet_counts *);

void add_ethernet_trailer(packet_info *pinfo, proto_tree *tree, proto_tree *fh_tree,
			  int trailer_id, tvbuff_t *tvb, tvbuff_t *trailer_tvb,
			  int fcs_len);

#endif
