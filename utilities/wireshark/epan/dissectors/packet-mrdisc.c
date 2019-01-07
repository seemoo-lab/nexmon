/* packet-mrdisc.c   2001 Ronnie Sahlberg <See AUTHORS for email>
 * Routines for IGMP/MRDISC packet disassembly
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
/*


			MRDISC
	code

	0x24		x
	0x25		x
	0x26		x

	MRDISC : IGMP Multicast Router DISCovery
	Defined in draft-ietf-idmr-igmp-mrdisc-06.txt
	TTL==1 and IP.DST==224.0.0.2 for all packets.
*/

#include "config.h"

#include <epan/packet.h>
#include <epan/exceptions.h>

#include "packet-igmp.h"

void proto_register_mrdisc(void);
void proto_reg_handoff_mrdisc(void);

static int proto_mrdisc = -1;
static int hf_checksum = -1;
static int hf_checksum_status = -1;
static int hf_type = -1;
static int hf_advint = -1;
static int hf_numopts = -1;
static int hf_options = -1;
static int hf_option = -1;
static int hf_option_len = -1;
static int hf_qi = -1;
static int hf_rv = -1;
static int hf_option_bytes = -1;

static int ett_mrdisc = -1;
static int ett_options = -1;

#define MC_ALL_ROUTERS		0xe0000002

#define MRDISC_MRA	0x24
#define MRDISC_MRS	0x25
#define MRDISC_MRT	0x26
static const value_string mrdisc_types[] = {
	{MRDISC_MRA,	"Multicast Router Advertisement"},
	{MRDISC_MRS,	"Multicast Router Solicitation"},
	{MRDISC_MRT,	"Multicast Router Termination"},
	{0,					NULL}
};

#define MRDISC_QI	0x01
#define MRDISC_RV	0x02
static const value_string mrdisc_options[] = {
	{MRDISC_QI,	"Query Interval"},
	{MRDISC_RV,	"Robustness Variable"},
	{0,					NULL}
};


static int
dissect_mrdisc_mra(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree, int offset)
{
	guint16 num;

	/* Advertising Interval */
	proto_tree_add_item(parent_tree, hf_advint, tvb, offset, 1, ENC_BIG_ENDIAN);
	offset += 1;

	/* checksum */
	igmp_checksum(parent_tree, tvb, hf_checksum, hf_checksum_status, pinfo, 0);
	offset += 2;

	/* skip unused bytes */
	offset += 2;

	/* number of options */
	num = tvb_get_ntohs(tvb, offset);
	proto_tree_add_uint(parent_tree, hf_numopts, tvb,
		offset, 2, num);
	offset += 2;

	/* process any options */
	while (num--) {
		proto_tree *tree;
		proto_item *item;
		guint8 type,len;
		int old_offset = offset;

		item = proto_tree_add_item(parent_tree, hf_options,
			tvb, offset, -1, ENC_NA);
		tree = proto_item_add_subtree(item, ett_options);

		type = tvb_get_guint8(tvb, offset);
		proto_tree_add_uint(tree, hf_option, tvb, offset, 1, type);
		offset += 1;

		len = tvb_get_guint8(tvb, offset);
		proto_tree_add_uint(tree, hf_option_len, tvb, offset, 1, len);
		offset += 1;

		switch (type) {
		case MRDISC_QI:
			proto_item_set_text(item,"Option: %s == %d",
					val_to_str(type, mrdisc_options, "unknown %x"),
					tvb_get_ntohs(tvb, offset));
			proto_tree_add_item(tree, hf_qi, tvb, offset, len,
				ENC_BIG_ENDIAN);
			offset += len;
			break;
		case MRDISC_RV:
			proto_item_set_text(item,"Option: %s == %d",
					val_to_str(type, mrdisc_options, "unknown %x"),
					tvb_get_ntohs(tvb, offset));
			proto_tree_add_item(tree, hf_rv, tvb, offset, len,
				ENC_BIG_ENDIAN);
			offset += len;
			break;
		default:
			proto_item_set_text(item,"Option: unknown");

			proto_tree_add_item(tree, hf_option_bytes,
				tvb, offset, len, ENC_NA);
			offset += len;
		}
		proto_item_set_len(item, offset-old_offset);
	}

	return offset;
}


static int
dissect_mrdisc_mrst(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree, int offset)
{
	/* skip reserved byte */
	offset += 1;

	/* checksum */
	igmp_checksum(parent_tree, tvb, hf_checksum, hf_checksum_status, pinfo, 0);
	offset += 2;

	return offset;
}


/* This function is only called from the IGMP dissector */
static int
dissect_mrdisc(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree, void* data _U_)
{
	proto_tree *tree;
	proto_item *item;
	guint8 type;
	int offset = 0;
	guint32 dst = g_htonl(MC_ALL_ROUTERS);

	/* Shouldn't be destined for us */
	if (memcmp(pinfo->dst.data, &dst, 4))
	return 0;

	col_set_str(pinfo->cinfo, COL_PROTOCOL, "MRDISC");
	col_clear(pinfo->cinfo, COL_INFO);

	item = proto_tree_add_item(parent_tree, proto_mrdisc, tvb, offset, 0, ENC_NA);
	tree = proto_item_add_subtree(item, ett_mrdisc);

	type = tvb_get_guint8(tvb, offset);
	col_add_str(pinfo->cinfo, COL_INFO,
			val_to_str(type, mrdisc_types,
				"Unknown Type:0x%02x"));

	/* type of command */
	proto_tree_add_uint(tree, hf_type, tvb, offset, 1, type);
	offset += 1;

	switch (type) {
	case MRDISC_MRA:
		offset = dissect_mrdisc_mra(tvb, pinfo, tree, offset);
		break;
	case MRDISC_MRS:
	case MRDISC_MRT:
		/* MRS and MRT packets looks the same */
		offset = dissect_mrdisc_mrst(tvb, pinfo, tree, offset);
		break;
	}
	return offset;
}


void
proto_register_mrdisc(void)
{
	static hf_register_info hf[] = {
		{ &hf_type,
			{ "Type", "mrdisc.type", FT_UINT8, BASE_HEX,
			  VALS(mrdisc_types), 0, "MRDISC Packet Type", HFILL }},

		{ &hf_checksum,
			{ "Checksum", "mrdisc.checksum", FT_UINT16, BASE_HEX,
			  NULL, 0, "MRDISC Checksum", HFILL }},

		{ &hf_checksum_status,
			{ "Checksum Status", "mrdisc.checksum.status", FT_UINT8, BASE_NONE,
			  VALS(proto_checksum_vals), 0x0, NULL, HFILL }},

		{ &hf_advint,
			{ "Advertising Interval", "mrdisc.adv_int", FT_UINT8, BASE_DEC,
			  NULL, 0, "MRDISC Advertising Interval in seconds", HFILL }},

		{ &hf_numopts,
			{ "Number Of Options", "mrdisc.num_opts", FT_UINT16, BASE_DEC,
			  NULL, 0, "MRDISC Number Of Options", HFILL }},

		{ &hf_options,
			{ "Options", "mrdisc.options", FT_NONE, BASE_NONE,
			  NULL, 0, "MRDISC Options", HFILL }},

		{ &hf_option,
			{ "Option", "mrdisc.option", FT_UINT8, BASE_DEC,
			  VALS(mrdisc_options), 0, "MRDISC Option Type", HFILL }},

		{ &hf_option_len,
			{ "Length", "mrdisc.opt_len", FT_UINT8, BASE_DEC,
			  NULL, 0, "MRDISC Option Length", HFILL }},

		{ &hf_qi,
			{ "Query Interval", "mrdisc.query_int", FT_UINT16, BASE_DEC,
			  NULL, 0, "MRDISC Query Interval", HFILL }},

		{ &hf_rv,
			{ "Robustness Variable", "mrdisc.rob_var", FT_UINT16, BASE_DEC,
			  NULL, 0, "MRDISC Robustness Variable", HFILL }},

		{ &hf_option_bytes,
			{ "Data", "mrdisc.option_data", FT_BYTES, BASE_NONE,
			  NULL, 0, "MRDISC Unknown Option Data", HFILL }},

	};
	static gint *ett[] = {
		&ett_mrdisc,
		&ett_options,
	};

	proto_mrdisc = proto_register_protocol("Multicast Router DISCovery protocol", "MRDISC", "mrdisc");
	proto_register_field_array(proto_mrdisc, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
}

void
proto_reg_handoff_mrdisc(void)
{
	dissector_handle_t mrdisc_handle;

	mrdisc_handle = create_dissector_handle(dissect_mrdisc, proto_mrdisc);
	dissector_add_uint("igmp.type", IGMP_TYPE_0x24, mrdisc_handle);
	dissector_add_uint("igmp.type", IGMP_TYPE_0x25, mrdisc_handle);
	dissector_add_uint("igmp.type", IGMP_TYPE_0x26, mrdisc_handle);
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
