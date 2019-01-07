/* packet-snaeth.c
 * Routines for SNA-over-Ethernet (Ethernet type 80d5)
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
#include <epan/etypes.h>

/*
 * See
 *
 * http://www.cisco.com/univercd/cc/td/doc/product/software/ssr90/rpc_r/18059.pdf
 */
void proto_register_snaeth(void);
void proto_reg_handoff_snaeth(void);

static int proto_snaeth = -1;
static int hf_snaeth_len = -1;
static int hf_snaeth_padding = -1;

static gint ett_snaeth = -1;

static dissector_handle_t llc_handle;

static int
dissect_snaeth(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
	proto_tree	*snaeth_tree;
	proto_item	*snaeth_ti;
	guint16		len;
	tvbuff_t	*next_tvb;

	col_set_str(pinfo->cinfo, COL_PROTOCOL, "SNAETH");
	col_set_str(pinfo->cinfo, COL_INFO, "SNA over Ethernet");

	/* length */
	len = tvb_get_ntohs(tvb, 0);

	if (tree) {
		snaeth_ti = proto_tree_add_item(tree, proto_snaeth, tvb, 0, 3,
		    ENC_NA);
		snaeth_tree = proto_item_add_subtree(snaeth_ti, ett_snaeth);
		proto_tree_add_uint(snaeth_tree, hf_snaeth_len, tvb, 0, 2, len);
		proto_tree_add_item(snaeth_tree, hf_snaeth_padding, tvb, 2, 1, ENC_BIG_ENDIAN);
	}

	/*
	 * Adjust the length of this tvbuff to include only the SNA-over-
	 * Ethernet header and data.
	 */
	set_actual_length(tvb, 3 + len);

	/*
	 * Rest of packet starts with an 802.2 LLC header.
	 */
	next_tvb = tvb_new_subset_remaining(tvb, 3);
	call_dissector(llc_handle, next_tvb, pinfo, tree);
	return tvb_captured_length(tvb);
}

void
proto_register_snaeth(void)
{
	static hf_register_info hf[] = {
		{ &hf_snaeth_len,
		{ "Length",	"snaeth.len", FT_UINT16, BASE_DEC, NULL, 0x0,
			"Length of LLC payload", HFILL }},
		{ &hf_snaeth_padding,
		{ "Padding",	"snaeth.padding", FT_UINT8, BASE_HEX, NULL, 0x0,
			NULL, HFILL }},
	};
	static gint *ett[] = {
		&ett_snaeth,
	};

	proto_snaeth = proto_register_protocol("SNA-over-Ethernet",
	    "SNAETH", "snaeth");
	proto_register_field_array(proto_snaeth, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
}

void
proto_reg_handoff_snaeth(void)
{
	dissector_handle_t snaeth_handle;

	/*
	 * Get handle for the LLC dissector.
	 */
	llc_handle = find_dissector_add_dependency("llc", proto_snaeth);

	snaeth_handle = create_dissector_handle(dissect_snaeth, proto_snaeth);
	dissector_add_uint("ethertype", ETHERTYPE_SNA, snaeth_handle);
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
