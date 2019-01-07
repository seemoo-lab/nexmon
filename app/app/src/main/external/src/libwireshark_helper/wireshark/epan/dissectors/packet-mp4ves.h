/* packet-mp4ves.h
 *
 * $Id: packet-mp4ves.h 26757 2008-11-11 21:44:29Z martinm $
 *
 * Copyright 2008, Anders Broman <anders.broman[at]ericsson.com>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef PACKET_MP4VES_H
#define PACKET_MP4VES_H

extern const value_string mp4ves_level_indication_vals[]; 

void dissect_mp4ves_config(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);
#endif  /* PACKET_MP4VES_H */
