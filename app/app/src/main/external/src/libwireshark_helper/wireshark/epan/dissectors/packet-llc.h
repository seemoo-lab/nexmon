/* packet-llc.h
 *
 * $Id: packet-llc.h 28273 2009-05-05 03:50:06Z guy $
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

#ifndef __PACKET_LLC_H__
#define __PACKET_LLC_H__

void capture_llc(const guchar *, int, int, packet_counts *);

extern const value_string sap_vals[];

void capture_snap(const guchar *, int, int, packet_counts *);

void dissect_snap(tvbuff_t *, int, packet_info *, proto_tree *,
    proto_tree *, int, int, int, int, int);

/*
 * Add an entry for a new OUI.
 */
void llc_add_oui(guint32, const char *, const char *, hf_register_info *);

/*
 * SNAP information about the PID for a particular OUI:
 *
 *	the dissector table to use with the PID's value;
 *	the field to use for the PID.
 */
typedef struct {
	dissector_table_t table;
	hf_register_info *field_info;
} oui_info_t;

/*
 * Return the oui_info_t for the PID for a particular OUI value, or NULL
 * if there isn't one.
 */
oui_info_t *get_snap_oui_info(guint32);

#endif
