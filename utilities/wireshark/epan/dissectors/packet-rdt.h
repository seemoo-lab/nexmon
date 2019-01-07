/* packet-rdt.h
 *
 * Routines for RDT dissection
 * RDT = Real Data Transport
 *
 * Written by Martin Mathieson
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

/* Info to save in RDT conversation / packet-info */
#define MAX_RDT_SETUP_METHOD_SIZE 7
struct _rdt_conversation_info
{
    gchar   method[MAX_RDT_SETUP_METHOD_SIZE + 1];
    guint32 frame_number;
    gint    feature_level;
};

/* Add an RDT conversation with the given details */
void rdt_add_address(packet_info *pinfo,
                     address *addr, int port,
                     int other_port,
                     const gchar *setup_method,
                     gint  rdt_feature_level);

