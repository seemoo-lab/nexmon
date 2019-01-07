/* Do not modify this file.                                                   */
/* It is created automatically by the ASN.1 to Wireshark dissector compiler   */
/* packet-p7.h                                                                */
/* ../../tools/asn2wrs.py -b -e -L -C -p p7 -c ./p7.cnf -s ./packet-p7-template -D . MSAbstractService.asn MSGeneralAttributeTypes.asn MSAccessProtocol.asn MSUpperBounds.asn */

/* Input file: packet-p7-template.h */

#line 1 "packet-p7-template.h"
/* packet-p7.h
 * Routines for X.413 (P7) packet dissection
 * Graeme Lunt 2007
 *
 * $Id: packet-p7.h 31321 2009-12-19 14:26:26Z stig $
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

#ifndef PACKET_P7_H
#define PACKET_P7_H


/*--- Included file: packet-p7-exp.h ---*/
#line 1 "packet-p7-exp.h"
extern const value_string p7_SignatureStatus_vals[];
int dissect_p7_SequenceNumber(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_);
int dissect_p7_SignatureStatus(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_);

/*--- End of included file: packet-p7-exp.h ---*/
#line 30 "packet-p7-template.h"

#endif  /* PACKET_P7_H */
