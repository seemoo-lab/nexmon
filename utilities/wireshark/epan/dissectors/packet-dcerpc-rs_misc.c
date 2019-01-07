/* packet-dcerpc-rs_misc.c
 *
 * Routines for dcerpc RS-MISC
 * Copyright 2002, Jaime Fournier <Jaime.Fournier@hush.com>
 * This information is based off the released idl files from opengroup.
 * ftp://ftp.opengroup.org/pub/dce122/dce/src/security.tar.gz security/idl/rs_misc.idl
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

void proto_register_rs_misc (void);
void proto_reg_handoff_rs_misc (void);

static int proto_rs_misc = -1;
static int hf_rs_misc_opnum = -1;
static int hf_rs_misc_login_get_info_rqst_var = -1;
static int hf_rs_misc_login_get_info_rqst_key_size = -1;
static int hf_rs_misc_login_get_info_rqst_key_t = -1;


static gint ett_rs_misc = -1;


static e_guid_t uuid_rs_misc = { 0x4c878280, 0x5000, 0x0000, { 0x0d, 0x00, 0x02, 0x87, 0x14, 0x00, 0x00, 0x00 } };
static guint16  ver_rs_misc = 1;


static int
rs_misc_dissect_login_get_info_rqst (tvbuff_t *tvb, int offset,
	packet_info *pinfo, proto_tree *tree, dcerpc_info *di, guint8 *drep)
{

	guint32 key_size;
	const guint8 *key_t1 = NULL;

	offset = dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
			hf_rs_misc_login_get_info_rqst_var, NULL);
	offset = dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
			hf_rs_misc_login_get_info_rqst_key_size, &key_size);

	if (key_size){ /* Not able to yet decipher the OTHER versions of this call just yet. */

		proto_tree_add_item_ret_string(tree, hf_rs_misc_login_get_info_rqst_key_t, tvb, offset, key_size, ENC_ASCII|ENC_NA, wmem_packet_scope(), &key_t1);
		offset += key_size;

		col_append_fstr(pinfo->cinfo, COL_INFO,
				"rs_login_get_info Request for: %s ", key_t1);
	} else {
		col_append_str(pinfo->cinfo, COL_INFO,
				"rs_login_get_info Request (other)");
	}

	return offset;
}


static dcerpc_sub_dissector rs_misc_dissectors[] = {
	{ 0, "login_get_info", rs_misc_dissect_login_get_info_rqst, NULL},
	{ 1, "wait_until_consistent", NULL, NULL},
	{ 2, "check_consistency", NULL, NULL},
	{ 0, NULL, NULL, NULL }
};

void
proto_register_rs_misc (void)
{
	static hf_register_info hf[] = {
	{ &hf_rs_misc_opnum,
		{ "Operation", "rs_misc.opnum", FT_UINT16, BASE_DEC,
		NULL, 0x0, NULL, HFILL }},
	{ &hf_rs_misc_login_get_info_rqst_var,
		{ "Var", "rs_misc.login_get_info_rqst_var", FT_UINT32, BASE_DEC,
		NULL, 0x0, NULL, HFILL }},
	{ &hf_rs_misc_login_get_info_rqst_key_size,
		{ "Key Size", "rs_misc.login_get_info_rqst_key_size", FT_UINT32, BASE_DEC,
		NULL, 0x0, NULL, HFILL }},
	{ &hf_rs_misc_login_get_info_rqst_key_t,
		{ "Key", "rs_misc.login_get_info_rqst_key_t", FT_STRING, BASE_NONE,
		NULL, 0x0, NULL, HFILL }}
	};

	static gint *ett[] = {
		&ett_rs_misc,
	};
	proto_rs_misc = proto_register_protocol ("DCE/RPC RS_MISC", "rs_misc", "rs_misc");
	proto_register_field_array (proto_rs_misc, hf, array_length (hf));
	proto_register_subtree_array (ett, array_length (ett));
}

void
proto_reg_handoff_rs_misc (void)
{
	/* Register the protocol as dcerpc */
	dcerpc_init_uuid (proto_rs_misc, ett_rs_misc, &uuid_rs_misc, ver_rs_misc, rs_misc_dissectors, hf_rs_misc_opnum);
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
