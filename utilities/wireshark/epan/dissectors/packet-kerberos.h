/* Do not modify this file. Changes will be overwritten.                      */
/* Generated automatically by the ASN.1 to Wireshark dissector compiler       */
/* packet-kerberos.h                                                          */
/* asn2wrs.py -b -p kerberos -c ./kerberos.cnf -s ./packet-kerberos-template -D . -O ../.. KerberosV5Spec2.asn k5.asn RFC3244.asn */

/* Input file: packet-kerberos-template.h */

#line 1 "./asn1/kerberos/packet-kerberos-template.h"
/* packet-kerberos.h
 * Routines for kerberos packet dissection
 * Copyright 2007, Anders Broman <anders.broman@ericsson.com>
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

#ifndef __PACKET_KERBEROS_H
#define __PACKET_KERBEROS_H

#include "ws_symbol_export.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* This is a list of callback functions a caller can use to specify that
   octet strings in kerberos to be passed back to application specific
   dissectors, outside of kerberos.
   This is used for dissection of application specific data for PacketCable
   KRB_SAFE user data and eventually to pass kerberos session keys
   to future DCERPC decryption and other uses.
   The list is terminated by {0, NULL }
*/
#define KRB_CBTAG_SAFE_USER_DATA	        1
#define KRB_CBTAG_PRIV_USER_DATA	        2
typedef struct _kerberos_callbacks {
	int tag;
	int (*callback)(packet_info *pinfo, tvbuff_t *tvb, proto_tree *tree);
} kerberos_callbacks;

/* Function prototypes */

gint
dissect_kerberos_main(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, gboolean do_col_info, kerberos_callbacks *cb);

int
dissect_krb5_Checksum(proto_tree *tree, tvbuff_t *tvb, int offset, asn1_ctx_t *actx _U_);

int
dissect_krb5_ctime(proto_tree *tree, tvbuff_t *tvb, int offset, asn1_ctx_t *actx _U_);

int dissect_krb5_cname(proto_tree *tree, tvbuff_t *tvb, int offset, asn1_ctx_t *actx _U_);
int dissect_krb5_realm(proto_tree *tree, tvbuff_t *tvb, int offset, asn1_ctx_t *actx _U_);
guint32 kerberos_output_keytype(void);

guint get_krb_pdu_len(packet_info *, tvbuff_t *tvb, int offset, void *data _U_);

gint kerberos_rm_to_reclen(guint krb_rm);

void
show_krb_recordmark(proto_tree *tree, tvbuff_t *tvb, gint start, guint32 krb_rm);

#ifdef HAVE_KERBEROS
#define KRB_MAX_ORIG_LEN	256

#if defined(HAVE_HEIMDAL_KERBEROS) || defined(HAVE_MIT_KERBEROS)
typedef struct _enc_key_t {
	struct _enc_key_t	*next;
	int keytype;
	int keylength;
	char *keyvalue;
	char 			key_origin[KRB_MAX_ORIG_LEN+1];
	int fd_num; /* remember where we learned a key */
} enc_key_t;
extern enc_key_t *enc_key_list;

guint8 *
decrypt_krb5_data(proto_tree *tree, packet_info *pinfo,
			int usage,
			tvbuff_t *crypototvb,
			int keytype,
			int *datalen);

#endif /* HAVE_HEIMDAL_KERBEROS || HAVE_MIT_KERBEROS */

extern gboolean krb_decrypt;

WS_DLL_PUBLIC
void read_keytab_file(const char *);

WS_DLL_PUBLIC
void read_keytab_file_from_preferences(void);

#endif /* HAVE_KERBEROS */


/*--- Included file: packet-kerberos-exp.h ---*/
#line 1 "./asn1/kerberos/packet-kerberos-exp.h"

/* enumerated values for Applications */
#define KERBEROS_APPLICATIONS_TICKET   1
#define KERBEROS_APPLICATIONS_AUTHENTICATOR   2
#define KERBEROS_APPLICATIONS_ENCTICKETPART   3
#define KERBEROS_APPLICATIONS_AS_REQ  10
#define KERBEROS_APPLICATIONS_AS_REP  11
#define KERBEROS_APPLICATIONS_TGS_REQ  12
#define KERBEROS_APPLICATIONS_TGS_REP  13
#define KERBEROS_APPLICATIONS_AP_REQ  14
#define KERBEROS_APPLICATIONS_AP_REP  15
#define KERBEROS_APPLICATIONS_KRB_SAFE  20
#define KERBEROS_APPLICATIONS_KRB_PRIV  21
#define KERBEROS_APPLICATIONS_KRB_CRED  22
#define KERBEROS_APPLICATIONS_ENCASREPPART  25
#define KERBEROS_APPLICATIONS_ENCTGSREPPART  26
#define KERBEROS_APPLICATIONS_ENCAPREPPART  27
#define KERBEROS_APPLICATIONS_ENCKRBPRIVPART  28
#define KERBEROS_APPLICATIONS_ENCKRBCREDPART  29
#define KERBEROS_APPLICATIONS_KRB_ERROR  30
int dissect_kerberos_ChangePasswdData(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_);

/*--- End of included file: packet-kerberos-exp.h ---*/
#line 104 "./asn1/kerberos/packet-kerberos-template.h"

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __PACKET_KERBEROS_H */
