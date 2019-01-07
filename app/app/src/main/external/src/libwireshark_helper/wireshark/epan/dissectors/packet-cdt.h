/* Do not modify this file.                                                   */
/* It is created automatically by the ASN.1 to Wireshark dissector compiler   */
/* packet-cdt.h                                                               */
/* ../../tools/asn2wrs.py -b -p cdt -c ./cdt.cnf -s ./packet-cdt-template -D . cdt.asn */

/* Input file: packet-cdt-template.h */

#line 1 "packet-cdt-template.h"
/* packet-cdt.h
 *
 * Routines for Compressed Data Type packet dissection.
 *
 * Copyright 2005, Stig Bjorlykke <stig@bjorlykke.org>, Thales Norway AS 
 *
 * $Id: packet-cdt.h 27733 2009-03-15 21:13:27Z stig $
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

#ifndef PACKET_CDT_H
#define PACKET_CDT_H

void dissect_cdt (tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree);

/*--- Included file: packet-cdt-exp.h ---*/
#line 1 "packet-cdt-exp.h"
int dissect_cdt_CompressedData(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_);

/*--- End of included file: packet-cdt-exp.h ---*/
#line 33 "packet-cdt-template.h"

#endif  /* PACKET_CDT_H */

