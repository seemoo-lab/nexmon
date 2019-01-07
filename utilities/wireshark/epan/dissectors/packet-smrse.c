/* Do not modify this file. Changes will be overwritten.                      */
/* Generated automatically by the ASN.1 to Wireshark dissector compiler       */
/* packet-smrse.c                                                             */
/* asn2wrs.py -b -p smrse -c ./smrse.cnf -s ./packet-smrse-template -D . -O ../.. SMRSE.asn */

/* Input file: packet-smrse-template.c */

#line 1 "./asn1/smrse/packet-smrse-template.c"
/* packet-smrse.c
 * Routines for SMRSE Short Message Relay Service packet dissection
 *   Ronnie Sahlberg 2004
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

#include "config.h"

#include <epan/packet.h>
#include <epan/asn1.h>

#include "packet-ber.h"
#include "packet-smrse.h"

#define PNAME  "Short Message Relaying Service"
#define PSNAME "SMRSE"
#define PFNAME "smrse"

#define TCP_PORT_SMRSE 4321

void proto_register_smrse(void);
void proto_reg_handoff_smrse(void);

/* Initialize the protocol and registered fields */
static int proto_smrse = -1;
static int hf_smrse_reserved = -1;
static int hf_smrse_tag = -1;
static int hf_smrse_length = -1;
static int hf_smrse_Octet_Format = -1;

/*--- Included file: packet-smrse-hf.c ---*/
#line 1 "./asn1/smrse/packet-smrse-hf.c"
static int hf_smrse_sc_address = -1;              /* SMS_Address */
static int hf_smrse_password = -1;                /* Password */
static int hf_smrse_address_type = -1;            /* T_address_type */
static int hf_smrse_numbering_plan = -1;          /* T_numbering_plan */
static int hf_smrse_address_value = -1;           /* T_address_value */
static int hf_smrse_octet_format = -1;            /* T_octet_format */
static int hf_smrse_connect_fail_reason = -1;     /* Connect_fail */
static int hf_smrse_mt_priority_request = -1;     /* BOOLEAN */
static int hf_smrse_mt_mms = -1;                  /* BOOLEAN */
static int hf_smrse_mt_message_reference = -1;    /* RP_MR */
static int hf_smrse_mt_originating_address = -1;  /* SMS_Address */
static int hf_smrse_mt_destination_address = -1;  /* SMS_Address */
static int hf_smrse_mt_user_data = -1;            /* RP_UD */
static int hf_smrse_mt_origVMSCAddr = -1;         /* SMS_Address */
static int hf_smrse_mt_tariffClass = -1;          /* SM_TC */
static int hf_smrse_mo_message_reference = -1;    /* RP_MR */
static int hf_smrse_mo_originating_address = -1;  /* SMS_Address */
static int hf_smrse_mo_user_data = -1;            /* RP_UD */
static int hf_smrse_origVMSCAddr = -1;            /* SMS_Address */
static int hf_smrse_moimsi = -1;                  /* IMSI_Address */
static int hf_smrse_message_reference = -1;       /* RP_MR */
static int hf_smrse_error_reason = -1;            /* Error_reason */
static int hf_smrse_msg_waiting_set = -1;         /* BOOLEAN */
static int hf_smrse_alerting_MS_ISDN = -1;        /* SMS_Address */
static int hf_smrse_sm_diag_info = -1;            /* RP_UD */
static int hf_smrse_ms_address = -1;              /* SMS_Address */

/*--- End of included file: packet-smrse-hf.c ---*/
#line 48 "./asn1/smrse/packet-smrse-template.c"

/* Initialize the subtree pointers */
static gint ett_smrse = -1;

/*--- Included file: packet-smrse-ett.c ---*/
#line 1 "./asn1/smrse/packet-smrse-ett.c"
static gint ett_smrse_SMR_Bind = -1;
static gint ett_smrse_SMS_Address = -1;
static gint ett_smrse_T_address_value = -1;
static gint ett_smrse_SMR_Bind_Confirm = -1;
static gint ett_smrse_SMR_Bind_Failure = -1;
static gint ett_smrse_SMR_Unbind = -1;
static gint ett_smrse_RPDataMT = -1;
static gint ett_smrse_RPDataMO = -1;
static gint ett_smrse_RPAck = -1;
static gint ett_smrse_RPError = -1;
static gint ett_smrse_RPAlertSC = -1;

/*--- End of included file: packet-smrse-ett.c ---*/
#line 52 "./asn1/smrse/packet-smrse-template.c"



/*--- Included file: packet-smrse-fn.c ---*/
#line 1 "./asn1/smrse/packet-smrse-fn.c"

static const value_string smrse_T_address_type_vals[] = {
  {   0, "unknown-type" },
  {   1, "internat-number" },
  {   2, "national-number" },
  {   3, "net-spec-number" },
  {   4, "short-number" },
  { 0, NULL }
};


static int
dissect_smrse_T_address_type(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                                NULL);

  return offset;
}


static const value_string smrse_T_numbering_plan_vals[] = {
  {   0, "unknown-numbering" },
  {   1, "iSDN-numbering" },
  {   3, "data-network-numbering" },
  {   4, "telex-numbering" },
  {   8, "national-numbering" },
  {   9, "private-numbering" },
  { 0, NULL }
};


static int
dissect_smrse_T_numbering_plan(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                                NULL);

  return offset;
}




static int
dissect_smrse_T_octet_format(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 19 "./asn1/smrse/smrse.cnf"
	char *strp,tmpstr[21];
	guint32 i, start_offset;
	gint8 ber_class;
	gboolean pc, ind;
	gint32 tag;
	guint32 len;
	static char n2a[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

	start_offset=offset;

	/* skip the tag and length */
	offset=dissect_ber_identifier(actx->pinfo, tree, tvb, offset, &ber_class, &pc, &tag);
	offset=dissect_ber_length(actx->pinfo, tree, tvb, offset, &len, &ind);
	if(len>10){
		len=10;
	}
	strp=tmpstr;
	for(i=0;i<len;i++){
		*strp++=n2a[tvb_get_guint8(tvb, offset)&0x0f];
		*strp++=n2a[(tvb_get_guint8(tvb, offset)>>4)&0x0f];
		offset++;
	}
	*strp=0;

	proto_tree_add_string(tree, hf_smrse_Octet_Format, tvb, start_offset, offset-start_offset, tmpstr);

	return offset;



  return offset;
}


static const value_string smrse_T_address_value_vals[] = {
  {   0, "octet-format" },
  { 0, NULL }
};

static const ber_choice_t T_address_value_choice[] = {
  {   0, &hf_smrse_octet_format  , BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_smrse_T_octet_format },
  { 0, NULL, 0, 0, 0, NULL }
};

static int
dissect_smrse_T_address_value(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_choice(actx, tree, tvb, offset,
                                 T_address_value_choice, hf_index, ett_smrse_T_address_value,
                                 NULL);

  return offset;
}


static const ber_sequence_t SMS_Address_sequence[] = {
  { &hf_smrse_address_type  , BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_smrse_T_address_type },
  { &hf_smrse_numbering_plan, BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_smrse_T_numbering_plan },
  { &hf_smrse_address_value , BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_smrse_T_address_value },
  { NULL, 0, 0, 0, NULL }
};

static int
dissect_smrse_SMS_Address(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   SMS_Address_sequence, hf_index, ett_smrse_SMS_Address);

  return offset;
}



static int
dissect_smrse_Password(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_restricted_string(implicit_tag, BER_UNI_TAG_PrintableString,
                                            actx, tree, tvb, offset, hf_index,
                                            NULL);

  return offset;
}


static const ber_sequence_t SMR_Bind_sequence[] = {
  { &hf_smrse_sc_address    , BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_smrse_SMS_Address },
  { &hf_smrse_password      , BER_CLASS_UNI, BER_UNI_TAG_PrintableString, BER_FLAGS_NOOWNTAG, dissect_smrse_Password },
  { NULL, 0, 0, 0, NULL }
};

static int
dissect_smrse_SMR_Bind(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   SMR_Bind_sequence, hf_index, ett_smrse_SMR_Bind);

  return offset;
}



static int
dissect_smrse_IMSI_Address(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, actx, tree, tvb, offset, hf_index,
                                       NULL);

  return offset;
}


static const ber_sequence_t SMR_Bind_Confirm_sequence[] = {
  { NULL, 0, 0, 0, NULL }
};

static int
dissect_smrse_SMR_Bind_Confirm(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   SMR_Bind_Confirm_sequence, hf_index, ett_smrse_SMR_Bind_Confirm);

  return offset;
}


static const value_string smrse_Connect_fail_vals[] = {
  {   0, "not-entitled" },
  {   1, "tmp-overload" },
  {   2, "tmp-failure" },
  {   3, "id-or-passwd" },
  {   4, "not-supported" },
  {   5, "inv-SC-addr" },
  { 0, NULL }
};


static int
dissect_smrse_Connect_fail(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                                NULL);

  return offset;
}


static const ber_sequence_t SMR_Bind_Failure_sequence[] = {
  { &hf_smrse_connect_fail_reason, BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_smrse_Connect_fail },
  { NULL, 0, 0, 0, NULL }
};

static int
dissect_smrse_SMR_Bind_Failure(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   SMR_Bind_Failure_sequence, hf_index, ett_smrse_SMR_Bind_Failure);

  return offset;
}


static const ber_sequence_t SMR_Unbind_sequence[] = {
  { NULL, 0, 0, 0, NULL }
};

static int
dissect_smrse_SMR_Unbind(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   SMR_Unbind_sequence, hf_index, ett_smrse_SMR_Unbind);

  return offset;
}



static int
dissect_smrse_BOOLEAN(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_boolean(implicit_tag, actx, tree, tvb, offset, hf_index, NULL);

  return offset;
}



static int
dissect_smrse_RP_MR(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                                NULL);

  return offset;
}



static int
dissect_smrse_RP_UD(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, actx, tree, tvb, offset, hf_index,
                                       NULL);

  return offset;
}



static int
dissect_smrse_SM_TC(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                                NULL);

  return offset;
}


static const ber_sequence_t RPDataMT_sequence[] = {
  { &hf_smrse_mt_priority_request, BER_CLASS_UNI, BER_UNI_TAG_BOOLEAN, BER_FLAGS_NOOWNTAG, dissect_smrse_BOOLEAN },
  { &hf_smrse_mt_mms        , BER_CLASS_UNI, BER_UNI_TAG_BOOLEAN, BER_FLAGS_NOOWNTAG, dissect_smrse_BOOLEAN },
  { &hf_smrse_mt_message_reference, BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_smrse_RP_MR },
  { &hf_smrse_mt_originating_address, BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_smrse_SMS_Address },
  { &hf_smrse_mt_destination_address, BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_smrse_SMS_Address },
  { &hf_smrse_mt_user_data  , BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_smrse_RP_UD },
  { &hf_smrse_mt_origVMSCAddr, BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_smrse_SMS_Address },
  { &hf_smrse_mt_tariffClass, BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_smrse_SM_TC },
  { NULL, 0, 0, 0, NULL }
};

static int
dissect_smrse_RPDataMT(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   RPDataMT_sequence, hf_index, ett_smrse_RPDataMT);

  return offset;
}


static const ber_sequence_t RPDataMO_sequence[] = {
  { &hf_smrse_mo_message_reference, BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_smrse_RP_MR },
  { &hf_smrse_mo_originating_address, BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_smrse_SMS_Address },
  { &hf_smrse_mo_user_data  , BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_smrse_RP_UD },
  { &hf_smrse_origVMSCAddr  , BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_smrse_SMS_Address },
  { &hf_smrse_moimsi        , BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_smrse_IMSI_Address },
  { NULL, 0, 0, 0, NULL }
};

static int
dissect_smrse_RPDataMO(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   RPDataMO_sequence, hf_index, ett_smrse_RPDataMO);

  return offset;
}


static const ber_sequence_t RPAck_sequence[] = {
  { &hf_smrse_message_reference, BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_smrse_RP_MR },
  { NULL, 0, 0, 0, NULL }
};

static int
dissect_smrse_RPAck(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   RPAck_sequence, hf_index, ett_smrse_RPAck);

  return offset;
}


static const value_string smrse_Error_reason_vals[] = {
  {   1, "unknown-subscriber" },
  {   9, "illegal-subscriber" },
  {  11, "teleservice-not-provisioned" },
  {  13, "call-barred" },
  {  15, "cug-reject" },
  {  19, "sMS-ll-capabilities-not-prov" },
  {  20, "error-in-MS" },
  {  21, "facility-not-supported" },
  {  22, "memory-capacity-exceeded" },
  {  29, "absent-subscriber" },
  {  30, "ms-busy-for-MT-sms" },
  {  36, "system-failure" },
  {  44, "illegal-equipment" },
  {  60, "no-resp-to-paging" },
  {  61, "gMSC-congestion" },
  {  70, "dublicate-sm" },
  { 101, "sC-congestion" },
  { 103, "mS-not-SC-Subscriber" },
  { 104, "invalid-sme-address" },
  { 0, NULL }
};


static int
dissect_smrse_Error_reason(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                                NULL);

  return offset;
}


static const ber_sequence_t RPError_sequence[] = {
  { &hf_smrse_error_reason  , BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_smrse_Error_reason },
  { &hf_smrse_msg_waiting_set, BER_CLASS_UNI, BER_UNI_TAG_BOOLEAN, BER_FLAGS_NOOWNTAG, dissect_smrse_BOOLEAN },
  { &hf_smrse_message_reference, BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_smrse_RP_MR },
  { &hf_smrse_alerting_MS_ISDN, BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_smrse_SMS_Address },
  { &hf_smrse_sm_diag_info  , BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_smrse_RP_UD },
  { NULL, 0, 0, 0, NULL }
};

static int
dissect_smrse_RPError(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   RPError_sequence, hf_index, ett_smrse_RPError);

  return offset;
}


static const ber_sequence_t RPAlertSC_sequence[] = {
  { &hf_smrse_ms_address    , BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_smrse_SMS_Address },
  { &hf_smrse_message_reference, BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_smrse_RP_MR },
  { NULL, 0, 0, 0, NULL }
};

static int
dissect_smrse_RPAlertSC(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   RPAlertSC_sequence, hf_index, ett_smrse_RPAlertSC);

  return offset;
}


/*--- End of included file: packet-smrse-fn.c ---*/
#line 55 "./asn1/smrse/packet-smrse-template.c"

static const value_string tag_vals[] = {
	{  1,	"AliveTest" },
	{  2,	"AliveTestRsp" },
	{  3,	"Bind" },
	{  4,	"BindRsp" },
	{  5,	"BindFail" },
	{  6,	"Unbind" },
	{  7,	"MT" },
	{  8,	"MO" },
	{  9,	"Ack" },
	{ 10,	"Error" },
	{ 11,	"Alert" },
	{ 0, NULL }
};

static int
dissect_smrse(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree, void *data _U_)
{
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	guint8 reserved, tag;
	int offset=0;
	asn1_ctx_t asn1_ctx;
	asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);

	reserved=tvb_get_guint8(tvb, 0);
	tag=tvb_get_guint8(tvb, 3);

	if( reserved!= 126 )
		return 0;
	if( (tag<1)||(tag>11) )
		return 0;

	if(parent_tree){
		item = proto_tree_add_item(parent_tree, proto_smrse, tvb, 0, -1, ENC_NA);
		tree = proto_item_add_subtree(item, ett_smrse);
	}

	col_set_str(pinfo->cinfo, COL_PROTOCOL, "SMRSE");
	col_add_str(pinfo->cinfo, COL_INFO, val_to_str(tag, tag_vals,"Unknown Tag:0x%02x"));

	proto_tree_add_item(tree, hf_smrse_reserved, tvb, 0, 1, ENC_BIG_ENDIAN);
	proto_tree_add_item(tree, hf_smrse_length, tvb, 1, 2, ENC_BIG_ENDIAN);
	proto_tree_add_item(tree, hf_smrse_tag, tvb, 3, 1, ENC_BIG_ENDIAN);

	switch(tag){
	case 1:
	case 2:
		offset=4;
		break;
	case 3:
		offset=dissect_smrse_SMR_Bind(FALSE, tvb, 4, &asn1_ctx, tree, -1);
		break;
	case 4:
		offset=dissect_smrse_SMR_Bind_Confirm(FALSE, tvb, 4, &asn1_ctx, tree, -1);
		break;
	case 5:
		offset=dissect_smrse_SMR_Bind_Failure(FALSE, tvb, 4, &asn1_ctx, tree, -1);
		break;
	case 6:
		offset=dissect_smrse_SMR_Unbind(FALSE, tvb, 4, &asn1_ctx, tree, -1);
		break;
	case 7:
		offset=dissect_smrse_RPDataMT(FALSE, tvb, 4, &asn1_ctx, tree, -1);
		break;
	case 8:
		offset=dissect_smrse_RPDataMO(FALSE, tvb, 4, &asn1_ctx, tree, -1);
		break;
	case 9:
		offset=dissect_smrse_RPAck(FALSE, tvb, 4, &asn1_ctx, tree, -1);
		break;
	case 10:
		offset=dissect_smrse_RPError(FALSE, tvb, 4, &asn1_ctx, tree, -1);
		break;
	case 11:
		offset=dissect_smrse_RPAlertSC(FALSE, tvb, 4, &asn1_ctx, tree, -1);
		break;
	}

	return offset;
}

/*--- proto_register_smrse ----------------------------------------------*/
void proto_register_smrse(void) {

  /* List of fields */
  static hf_register_info hf[] = {
	{ &hf_smrse_reserved, {
		"Reserved", "smrse.reserved", FT_UINT8, BASE_DEC,
		NULL, 0, "Reserved byte, must be 126", HFILL }},
	{ &hf_smrse_tag, {
		"Tag", "smrse.tag", FT_UINT8, BASE_DEC,
		VALS(tag_vals), 0, NULL, HFILL }},
	{ &hf_smrse_length, {
		"Length", "smrse.length", FT_UINT16, BASE_DEC,
		NULL, 0, "Length of SMRSE PDU", HFILL }},
    { &hf_smrse_Octet_Format,
      { "octet-Format", "smrse.octet_Format",
        FT_STRING, BASE_NONE, NULL, 0,
        "SMS-Address/address-value/octet-format", HFILL }},


/*--- Included file: packet-smrse-hfarr.c ---*/
#line 1 "./asn1/smrse/packet-smrse-hfarr.c"
    { &hf_smrse_sc_address,
      { "sc-address", "smrse.sc_address_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "SMS_Address", HFILL }},
    { &hf_smrse_password,
      { "password", "smrse.password",
        FT_STRING, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_smrse_address_type,
      { "address-type", "smrse.address_type",
        FT_INT32, BASE_DEC, VALS(smrse_T_address_type_vals), 0,
        NULL, HFILL }},
    { &hf_smrse_numbering_plan,
      { "numbering-plan", "smrse.numbering_plan",
        FT_INT32, BASE_DEC, VALS(smrse_T_numbering_plan_vals), 0,
        NULL, HFILL }},
    { &hf_smrse_address_value,
      { "address-value", "smrse.address_value",
        FT_UINT32, BASE_DEC, VALS(smrse_T_address_value_vals), 0,
        NULL, HFILL }},
    { &hf_smrse_octet_format,
      { "octet-format", "smrse.octet_format",
        FT_BYTES, BASE_NONE, NULL, 0,
        "T_octet_format", HFILL }},
    { &hf_smrse_connect_fail_reason,
      { "connect-fail-reason", "smrse.connect_fail_reason",
        FT_INT32, BASE_DEC, VALS(smrse_Connect_fail_vals), 0,
        "Connect_fail", HFILL }},
    { &hf_smrse_mt_priority_request,
      { "mt-priority-request", "smrse.mt_priority_request",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_smrse_mt_mms,
      { "mt-mms", "smrse.mt_mms",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_smrse_mt_message_reference,
      { "mt-message-reference", "smrse.mt_message_reference",
        FT_UINT32, BASE_DEC, NULL, 0,
        "RP_MR", HFILL }},
    { &hf_smrse_mt_originating_address,
      { "mt-originating-address", "smrse.mt_originating_address_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "SMS_Address", HFILL }},
    { &hf_smrse_mt_destination_address,
      { "mt-destination-address", "smrse.mt_destination_address_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "SMS_Address", HFILL }},
    { &hf_smrse_mt_user_data,
      { "mt-user-data", "smrse.mt_user_data",
        FT_BYTES, BASE_NONE, NULL, 0,
        "RP_UD", HFILL }},
    { &hf_smrse_mt_origVMSCAddr,
      { "mt-origVMSCAddr", "smrse.mt_origVMSCAddr_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "SMS_Address", HFILL }},
    { &hf_smrse_mt_tariffClass,
      { "mt-tariffClass", "smrse.mt_tariffClass",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SM_TC", HFILL }},
    { &hf_smrse_mo_message_reference,
      { "mo-message-reference", "smrse.mo_message_reference",
        FT_UINT32, BASE_DEC, NULL, 0,
        "RP_MR", HFILL }},
    { &hf_smrse_mo_originating_address,
      { "mo-originating-address", "smrse.mo_originating_address_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "SMS_Address", HFILL }},
    { &hf_smrse_mo_user_data,
      { "mo-user-data", "smrse.mo_user_data",
        FT_BYTES, BASE_NONE, NULL, 0,
        "RP_UD", HFILL }},
    { &hf_smrse_origVMSCAddr,
      { "origVMSCAddr", "smrse.origVMSCAddr_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "SMS_Address", HFILL }},
    { &hf_smrse_moimsi,
      { "moimsi", "smrse.moimsi",
        FT_BYTES, BASE_NONE, NULL, 0,
        "IMSI_Address", HFILL }},
    { &hf_smrse_message_reference,
      { "message-reference", "smrse.message_reference",
        FT_UINT32, BASE_DEC, NULL, 0,
        "RP_MR", HFILL }},
    { &hf_smrse_error_reason,
      { "error-reason", "smrse.error_reason",
        FT_INT32, BASE_DEC, VALS(smrse_Error_reason_vals), 0,
        NULL, HFILL }},
    { &hf_smrse_msg_waiting_set,
      { "msg-waiting-set", "smrse.msg_waiting_set",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_smrse_alerting_MS_ISDN,
      { "alerting-MS-ISDN", "smrse.alerting_MS_ISDN_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "SMS_Address", HFILL }},
    { &hf_smrse_sm_diag_info,
      { "sm-diag-info", "smrse.sm_diag_info",
        FT_BYTES, BASE_NONE, NULL, 0,
        "RP_UD", HFILL }},
    { &hf_smrse_ms_address,
      { "ms-address", "smrse.ms_address_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "SMS_Address", HFILL }},

/*--- End of included file: packet-smrse-hfarr.c ---*/
#line 158 "./asn1/smrse/packet-smrse-template.c"
  };

  /* List of subtrees */
  static gint *ett[] = {
    &ett_smrse,

/*--- Included file: packet-smrse-ettarr.c ---*/
#line 1 "./asn1/smrse/packet-smrse-ettarr.c"
    &ett_smrse_SMR_Bind,
    &ett_smrse_SMS_Address,
    &ett_smrse_T_address_value,
    &ett_smrse_SMR_Bind_Confirm,
    &ett_smrse_SMR_Bind_Failure,
    &ett_smrse_SMR_Unbind,
    &ett_smrse_RPDataMT,
    &ett_smrse_RPDataMO,
    &ett_smrse_RPAck,
    &ett_smrse_RPError,
    &ett_smrse_RPAlertSC,

/*--- End of included file: packet-smrse-ettarr.c ---*/
#line 164 "./asn1/smrse/packet-smrse-template.c"
  };

  /* Register protocol */
  proto_smrse = proto_register_protocol(PNAME, PSNAME, PFNAME);

  /* Register fields and subtrees */
  proto_register_field_array(proto_smrse, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));

}


/*--- proto_reg_handoff_smrse -------------------------------------------*/
void proto_reg_handoff_smrse(void) {
  dissector_handle_t smrse_handle;

  smrse_handle = create_dissector_handle(dissect_smrse, proto_smrse);
  dissector_add_uint("tcp.port",TCP_PORT_SMRSE, smrse_handle);
}

