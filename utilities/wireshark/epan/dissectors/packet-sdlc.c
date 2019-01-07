/* packet-sdlc.c
 * Routines for SDLC frame disassembly
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998
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
#include <wiretap/wtap.h>
#include <epan/xdlc.h>

/*
 * See
 *
 *	http://www.protocols.com/pbook/sna.htm
 */
void proto_register_sdlc(void);
void proto_reg_handoff_sdlc(void);

static int proto_sdlc = -1;
static int hf_sdlc_address = -1;
static int hf_sdlc_control = -1;
static int hf_sdlc_n_r = -1;
static int hf_sdlc_n_s = -1;
static int hf_sdlc_p = -1;
static int hf_sdlc_f = -1;
static int hf_sdlc_s_ftype = -1;
static int hf_sdlc_u_modifier_cmd = -1;
static int hf_sdlc_u_modifier_resp = -1;
static int hf_sdlc_ftype_i = -1;
static int hf_sdlc_ftype_s_u = -1;

static gint ett_sdlc = -1;
static gint ett_sdlc_control = -1;

static dissector_handle_t sna_handle;

static const xdlc_cf_items sdlc_cf_items = {
	&hf_sdlc_n_r,
	&hf_sdlc_n_s,
	&hf_sdlc_p,
	&hf_sdlc_f,
	&hf_sdlc_s_ftype,
	&hf_sdlc_u_modifier_cmd,
	&hf_sdlc_u_modifier_resp,
	&hf_sdlc_ftype_i,
	&hf_sdlc_ftype_s_u
};

static int
dissect_sdlc(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
	proto_tree	*sdlc_tree;
	proto_item	*sdlc_ti;
	guint8		addr;
	guint16		control;
	int		sdlc_header_len;
	gboolean	is_response;
	tvbuff_t	*next_tvb;

	col_set_str(pinfo->cinfo, COL_PROTOCOL, "SDLC");
	col_clear(pinfo->cinfo, COL_INFO);

	addr = tvb_get_guint8(tvb, 0);
	sdlc_header_len = 1;	/* address */

	/*
	 * XXX - is there something in the SDLC header that indicates
	 * how to interpret "command vs. response" based on the
	 * direction?
	 */
	if (pinfo->p2p_dir == P2P_DIR_SENT) {
		is_response = FALSE;
		col_set_str(pinfo->cinfo, COL_RES_DL_DST, "DCE");
		col_set_str(pinfo->cinfo, COL_RES_DL_SRC, "DTE");
	}
	else {
		/* XXX - what if the direction is unknown? */
		is_response = TRUE;
		col_set_str(pinfo->cinfo, COL_RES_DL_DST, "DTE");
		col_set_str(pinfo->cinfo, COL_RES_DL_SRC, "DCE");
	}

	sdlc_ti = proto_tree_add_item(tree, proto_sdlc, tvb, 0, -1,
		ENC_NA);
	sdlc_tree = proto_item_add_subtree(sdlc_ti, ett_sdlc);

	proto_tree_add_uint(sdlc_tree, hf_sdlc_address, tvb, 0, 1,
		addr);

	/*
	 * XXX - SDLC has a mod-128 mode as well as a mod-7 mode.
	 * We can infer the mode from an SNRM/SRME frame, but if
	 * we don't see one of them, we may have to have a preference
	 * to control what to use.
	 */
	control = dissect_xdlc_control(tvb, 1, pinfo, sdlc_tree, hf_sdlc_control,
	    ett_sdlc_control, &sdlc_cf_items, NULL, NULL, NULL,
	    is_response, FALSE, FALSE);
	sdlc_header_len += XDLC_CONTROL_LEN(control, FALSE);

	proto_item_set_len(sdlc_ti, sdlc_header_len);

	/*
	 * XXX - is there an FCS at the end, at least in Sniffer
	 * captures?  (There doesn't appear to be.)
	 */
	next_tvb = tvb_new_subset_remaining(tvb, sdlc_header_len);
	if (XDLC_IS_INFORMATION(control)) {
		/* call the SNA dissector */
		call_dissector(sna_handle, next_tvb, pinfo, tree);
	} else
		call_data_dissector(next_tvb, pinfo, tree);

	return tvb_captured_length(tvb);
}

void
proto_register_sdlc(void)
{
	static hf_register_info hf[] = {
		{ &hf_sdlc_address,
		  { "Address Field", "sdlc.address", FT_UINT8, BASE_HEX,
		    NULL, 0x0, "Address", HFILL }},

		{ &hf_sdlc_control,
		  { "Control Field", "sdlc.control", FT_UINT16, BASE_HEX,
		    NULL, 0x0, NULL, HFILL }},

		{ &hf_sdlc_n_r,
		  { "N(R)", "sdlc.control.n_r", FT_UINT8, BASE_DEC,
		    NULL, XDLC_N_R_MASK, NULL, HFILL }},

		{ &hf_sdlc_n_s,
		  { "N(S)", "sdlc.control.n_s", FT_UINT8, BASE_DEC,
		    NULL, XDLC_N_S_MASK, NULL, HFILL }},

		{ &hf_sdlc_p,
		  { "Poll", "sdlc.control.p", FT_BOOLEAN, 8,
		    TFS(&tfs_set_notset), XDLC_P_F, NULL, HFILL }},

		{ &hf_sdlc_f,
		  { "Final", "sdlc.control.f", FT_BOOLEAN, 8,
		    TFS(&tfs_set_notset), XDLC_P_F, NULL, HFILL }},

		{ &hf_sdlc_s_ftype,
		  { "Supervisory frame type", "sdlc.control.s_ftype", FT_UINT8, BASE_HEX,
		    VALS(stype_vals), XDLC_S_FTYPE_MASK, NULL, HFILL }},

		{ &hf_sdlc_u_modifier_cmd,
		  { "Command", "sdlc.control.u_modifier_cmd", FT_UINT8, BASE_HEX,
		    VALS(modifier_vals_cmd), XDLC_U_MODIFIER_MASK, NULL, HFILL }},

		{ &hf_sdlc_u_modifier_resp,
		  { "Response", "sdlc.control.u_modifier_resp", FT_UINT8, BASE_HEX,
		    VALS(modifier_vals_resp), XDLC_U_MODIFIER_MASK, NULL, HFILL }},

		{ &hf_sdlc_ftype_i,
		  { "Frame type", "sdlc.control.ftype", FT_UINT8, BASE_HEX,
		    VALS(ftype_vals), XDLC_I_MASK, NULL, HFILL }},

		{ &hf_sdlc_ftype_s_u,
		  { "Frame type", "sdlc.control.ftype", FT_UINT8, BASE_HEX,
		    VALS(ftype_vals), XDLC_S_U_MASK, NULL, HFILL }},
	};
	static gint *ett[] = {
		&ett_sdlc,
		&ett_sdlc_control,
	};

	proto_sdlc = proto_register_protocol(
		"Synchronous Data Link Control (SDLC)", "SDLC", "sdlc");
	proto_register_field_array(proto_sdlc, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
}

void
proto_reg_handoff_sdlc(void)
{
	dissector_handle_t sdlc_handle;

	/*
	 * Get handle for the SNA dissector.
	 */
	sna_handle = find_dissector_add_dependency("sna", proto_sdlc);

	sdlc_handle = create_dissector_handle(dissect_sdlc, proto_sdlc);
	dissector_add_uint("wtap_encap", WTAP_ENCAP_SDLC, sdlc_handle);
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
