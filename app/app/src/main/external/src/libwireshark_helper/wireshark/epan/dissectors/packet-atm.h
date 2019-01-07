/* packet-atm.h
 *
 * $Id: packet-atm.h 28881 2009-06-29 19:24:14Z etxrab $
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

#ifndef __PACKET_ATM_H__
#define __PACKET_ATM_H__

void capture_atm(const union wtap_pseudo_header *, const guchar *, int,
    packet_counts *);

gboolean atm_is_oam_cell(const guint16 vci, const guint8 pt); /*For pw-atm dissector*/

extern const value_string atm_pt_vals[]; /*For pw-atm dissector*/

#endif
