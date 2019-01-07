/* packet-e212.h
 * E212 tables
 * Copyright 2006, Anders Broman <anders.broman@ericsson.com>
 *
 * $Id: packet-e212.h 35360 2011-01-04 16:58:07Z etxrab $
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

#ifndef __PACKET_E212_H__
#define __PACKET_E212_H__

#include <epan/value_string.h>

extern value_string_ext E212_codes_ext;

gchar* dissect_e212_mcc_mnc_ep_str(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, gboolean little_endian);
int dissect_e212_mcc_mnc(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, gboolean little_endian);
int dissect_e212_mcc_mnc_in_address(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset);

#endif /* __PACKET_E212_H__ */

