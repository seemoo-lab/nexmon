/* Do not modify this file.                                                   */
/* It is created automatically by the ASN.1 to Wireshark dissector compiler   */
/* packet-pkcs1.h                                                             */
/* ../../tools/asn2wrs.py -b -p pkcs1 -c ./pkcs1.cnf -s ./packet-pkcs1-template -D . PKCS1.asn */

/* Input file: packet-pkcs1-template.h */

#line 1 "packet-pkcs1-template.h"
/* packet-pkcs1.h
 * Routines for PKCS#1/RFC2313 packet dissection
 *  Ronnie Sahlberg 2004
 *
 * $Id: packet-pkcs1.h 28379 2009-05-16 22:07:03Z stig $
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

#ifndef PACKET_PKCS1_H
#define PACKET_PKCS1_H


/*--- Included file: packet-pkcs1-exp.h ---*/
#line 1 "packet-pkcs1-exp.h"
int dissect_pkcs1_RSAPublicKey(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_);
int dissect_pkcs1_RSAPrivateKey(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_);
int dissect_pkcs1_DigestInfo(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_);

/*--- End of included file: packet-pkcs1-exp.h ---*/
#line 30 "packet-pkcs1-template.h"

#endif  /* PACKET_PKCS1_H */

