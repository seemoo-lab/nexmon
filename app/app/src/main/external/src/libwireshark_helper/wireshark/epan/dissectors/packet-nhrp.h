/* packet-nhrp.h
 * Definitions for NHRP
 *
 * $Id: packet-nhrp.h 28273 2009-05-05 03:50:06Z guy $
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


#ifndef __PACKET_NHRP_H__
#define __PACKET_NHRP_H__

void capture_nhrp(const guchar *, int, int, packet_counts *);

/* Export the DSCP value-string table for other protocols */
/*extern const value_string dscp_vals[];*/

#endif
