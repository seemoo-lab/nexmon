/* Do not modify this file.                                                   */
/* It is created automatically by the ASN.1 to Wireshark dissector compiler   */
/* packet-idmp.h                                                              */
/* ../../tools/asn2wrs.py -b -e -L -p idmp -c ./idmp.cnf -s ./packet-idmp-template -D . IDMProtocolSpecification.asn CommonProtocolSpecification.asn */

/* Input file: packet-idmp-template.h */

#line 1 "packet-idmp-template.h"
/* packet-idmp.h
 * Routines for X.519 Internet Directly Mapped Protocol (IDMP) packet dissection
 * Graeme Lunt 2010
 *
 * $Id: packet-idmp.h 33177 2010-06-09 14:17:51Z gal $
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

#ifndef PACKET_IDM_H
#define PACKET_IDM_H

void
register_idmp_protocol_info(const char *oid, const ros_info_t *rinfo, int proto _U_, const char *name);

#endif  /* PACKET_IDM_H */
