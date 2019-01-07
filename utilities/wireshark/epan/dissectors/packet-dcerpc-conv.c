/* packet-dcerpc-conv.c
 * Routines for dcerpc conv dissection
 * Copyright 2001, Todd Sabin <tas@webspan.net>
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
#include "packet-dcerpc.h"
#include "packet-dcerpc-dce122.h"

void proto_register_conv (void);
void proto_reg_handoff_conv (void);

static int proto_conv = -1;
static int hf_conv_opnum = -1;
static int hf_conv_rc = -1;
static int hf_conv_who_are_you_rqst_actuid = -1;
static int hf_conv_who_are_you_rqst_boot_time = -1;
static int hf_conv_who_are_you2_rqst_actuid = -1;
static int hf_conv_who_are_you2_rqst_boot_time = -1;
static int hf_conv_who_are_you_resp_seq = -1;
static int hf_conv_who_are_you2_resp_seq = -1;
static int hf_conv_who_are_you2_resp_casuuid = -1;

static gint ett_conv = -1;


static e_guid_t uuid_conv = { 0x333a2276, 0x0000, 0x0000, { 0x0d, 0x00, 0x00, 0x80, 0x9c, 0x00, 0x00, 0x00 } };
static guint16  ver_conv = 3;


static int
conv_dissect_who_are_you_rqst (tvbuff_t *tvb, int offset,
			       packet_info *pinfo, proto_tree *tree,
			       dcerpc_info *di, guint8 *drep)
{
	/*
	 *         [in]    uuid_t          *actuid,
	 *         [in]    unsigned32      boot_time,
	 */
	e_guid_t actuid;

	offset = dissect_ndr_uuid_t(tvb, offset, pinfo, tree, di, drep, hf_conv_who_are_you_rqst_actuid, &actuid);
	offset = dissect_ndr_time_t(tvb, offset, pinfo, tree, di, drep, hf_conv_who_are_you_rqst_boot_time, NULL);

	col_add_fstr(pinfo->cinfo, COL_INFO,
			     "conv_who_are_you request actuid: %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			     actuid.data1, actuid.data2, actuid.data3,
			     actuid.data4[0], actuid.data4[1], actuid.data4[2], actuid.data4[3],
			     actuid.data4[4], actuid.data4[5], actuid.data4[6], actuid.data4[7]);

	return offset;
}

static int
conv_dissect_who_are_you_resp (tvbuff_t *tvb, int offset,
			       packet_info *pinfo, proto_tree *tree,
			       dcerpc_info *di, guint8 *drep)
{
	/*
	 *         [out]   unsigned32      *seq,
	 *         [out]   unsigned32      *st
	 */
	guint32 seq, st;

	offset = dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep, hf_conv_who_are_you_resp_seq, &seq);
	offset = dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep, hf_conv_rc, &st);


	col_add_fstr(pinfo->cinfo, COL_INFO, "conv_who_are_you response seq:%u st:%s",
			     seq, val_to_str_ext(st, &dce_error_vals_ext, "%u"));

	return offset;
}



static int
conv_dissect_who_are_you2_rqst (tvbuff_t *tvb, int offset,
				packet_info *pinfo, proto_tree *tree,
				dcerpc_info *di, guint8 *drep)
{
	/*
	 *         [in]    uuid_t          *actuid,
	 *         [in]    unsigned32      boot_time,
	 */
	e_guid_t actuid;

	offset = dissect_ndr_uuid_t(tvb, offset, pinfo, tree, di, drep, hf_conv_who_are_you2_rqst_actuid, &actuid);
	offset = dissect_ndr_time_t(tvb, offset, pinfo, tree, di, drep, hf_conv_who_are_you2_rqst_boot_time, NULL);

		col_add_fstr(pinfo->cinfo, COL_INFO,
			     "conv_who_are_you2 request actuid: %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			     actuid.data1, actuid.data2, actuid.data3,
			     actuid.data4[0], actuid.data4[1], actuid.data4[2], actuid.data4[3],
			     actuid.data4[4], actuid.data4[5], actuid.data4[6], actuid.data4[7]);

	return offset;
}
static int
conv_dissect_who_are_you2_resp (tvbuff_t *tvb, int offset,
				packet_info *pinfo, proto_tree *tree,
				dcerpc_info *di, guint8 *drep)
{
	/*
	 *         [out]   unsigned32      *seq,
	 *         [out]   uuid_t          *cas_uuid,
	 *
	 *         [out]   unsigned32      *st
	 */
	guint32 seq, st;
	e_guid_t cas_uuid;

	offset = dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep, hf_conv_who_are_you2_resp_seq, &seq);
	offset = dissect_ndr_uuid_t (tvb, offset, pinfo, tree, di, drep, hf_conv_who_are_you2_resp_casuuid, &cas_uuid);
	offset = dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep, hf_conv_rc, &st);

	col_add_fstr(pinfo->cinfo, COL_INFO,
			     "conv_who_are_you2 response seq:%u st:%s cas:%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			     seq, val_to_str_ext(st, &dce_error_vals_ext, "%u"),
			     cas_uuid.data1, cas_uuid.data2, cas_uuid.data3,
			     cas_uuid.data4[0], cas_uuid.data4[1], cas_uuid.data4[2], cas_uuid.data4[3],
			     cas_uuid.data4[4], cas_uuid.data4[5], cas_uuid.data4[6], cas_uuid.data4[7]);

	return offset;
}


static dcerpc_sub_dissector conv_dissectors[] = {
	{ 0, "who_are_you",
	  conv_dissect_who_are_you_rqst, conv_dissect_who_are_you_resp },
	{ 1, "who_are_you2",
	  conv_dissect_who_are_you2_rqst, conv_dissect_who_are_you2_resp },
	{ 2, "are_you_there",
	  NULL, NULL },
	{ 3, "who_are_you_auth",
	  NULL, NULL },
	{ 4, "who_are_you_auth_more",
	  NULL, NULL },
	{ 0, NULL, NULL, NULL }
};

void
proto_register_conv (void)
{
	static hf_register_info hf[] = {
	{ &hf_conv_opnum,
	    { "Operation", "conv.opnum", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }},
	{ &hf_conv_rc,
	    {"Status", "conv.status", FT_UINT32, BASE_DEC|BASE_EXT_STRING, &dce_error_vals_ext, 0x0, NULL, HFILL }},

	{ &hf_conv_who_are_you_rqst_actuid,
	    {"Activity UID", "conv.who_are_you_rqst_actuid", FT_GUID, BASE_NONE, NULL, 0x0, "UUID", HFILL }},
	{ &hf_conv_who_are_you_rqst_boot_time,
	    {"Boot time", "conv.who_are_you_rqst_boot_time", FT_ABSOLUTE_TIME, ABSOLUTE_TIME_LOCAL, NULL, 0x0, NULL, HFILL }},
	{ &hf_conv_who_are_you2_rqst_actuid,
	    {"Activity UID", "conv.who_are_you2_rqst_actuid", FT_GUID, BASE_NONE, NULL, 0x0, "UUID", HFILL }},
	{ &hf_conv_who_are_you2_rqst_boot_time,
	    {"Boot time", "conv.who_are_you2_rqst_boot_time", FT_ABSOLUTE_TIME, ABSOLUTE_TIME_LOCAL, NULL, 0x0, NULL, HFILL }},

	{ &hf_conv_who_are_you_resp_seq,
	    {"Sequence Number", "conv.who_are_you_resp_seq", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
	{ &hf_conv_who_are_you2_resp_seq,
	    {"Sequence Number", "conv.who_are_you2_resp_seq", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
	{ &hf_conv_who_are_you2_resp_casuuid,
	    {"Client's address space UUID", "conv.who_are_you2_resp_casuuid", FT_GUID, BASE_NONE, NULL, 0x0, "UUID", HFILL }}
	};

	static gint *ett[] = {
		&ett_conv
	};
	proto_conv = proto_register_protocol ("DCE/RPC Conversation Manager", "CONV", "conv");
	proto_register_field_array (proto_conv, hf, array_length (hf));
	proto_register_subtree_array (ett, array_length (ett));
}

void
proto_reg_handoff_conv (void)
{
	/* Register the protocol as dcerpc */
	dcerpc_init_uuid (proto_conv, ett_conv, &uuid_conv, ver_conv, conv_dissectors, hf_conv_opnum);
}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: t
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 noexpandtab:
 * :indentSize=8:tabSize=8:noTabs=false:
 */
