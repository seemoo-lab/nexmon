/* packet-t124.h
 * Routines for t124 packet dissection
 * Copyright 2010, Graeme Lunt
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

#ifndef PACKET_T124_H
#define PACKET_T124_H

#include <epan/packet_info.h>
#include <epan/dissectors/packet-per.h>

extern int dissect_DomainMCSPDU_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_);
extern guint32 t124_get_last_channelId(void);
extern void t124_set_top_tree(proto_tree *tree);

extern void register_t124_ns_dissector(const char *nsKey, dissector_t dissector, int proto);
extern void register_t124_sd_dissector(packet_info *pinfo, guint32 channelId, dissector_t dissector, int proto);

#include "packet-t124-exp.h"

#endif  /* PACKET_T124_H */


