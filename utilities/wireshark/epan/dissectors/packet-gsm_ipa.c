/* packet-gsm_ipa.c
 * Routines for packet dissection of ip.access GSM A-bis over IP
 * Copyright 2009 by Harald Welte <laforge@gnumonks.org>
 * Copyright 2009, 2010 by Holger Hans Peter Freyther <zecke@selfish.org>
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
#include <epan/prefs.h>

void proto_register_ipa(void);
void proto_reg_handoff_gsm_ipa(void);

/*
 * Protocol used by ip.access's nanoBTS/nanoGSM GSM picocells:
 *
 *	http://www.ipaccess.com/en/nanoGSM-picocell
 *
 * to transport the GSM A-bis interface over TCP and UDP.
 *
 * See
 *
 *	http://openbsc.osmocom.org/trac/wiki/nanoBTS
 *
 * for some information about this protocol determined by reverse-
 * engineering.
 */

/*
 * These ports are also registered for other protocols, as per
 *
 * http://www.iana.org/assignments/service-names-port-numbers/service-names-port-numbers.xml
 *
 * exlm-agent     3002
 * cgms           3003
 * ii-admin       3006
 * vrml-multi-use 4200-4299
 * commplex-main  5000
 *
 * But, as that document says:
 *
 ************************************************************************
 * PLEASE NOTE THE FOLLOWING:                                           *
 *                                                                      *
 * ASSIGNMENT OF A PORT NUMBER DOES NOT IN ANY WAY IMPLY AN             *
 * ENDORSEMENT OF AN APPLICATION OR PRODUCT, AND THE FACT THAT NETWORK  *
 * TRAFFIC IS FLOWING TO OR FROM A REGISTERED PORT DOES NOT MEAN THAT   *
 * IT IS "GOOD" TRAFFIC, NOR THAT IT NECESSARILY CORRESPONDS TO THE     *
 * ASSIGNED SERVICE. FIREWALL AND SYSTEM ADMINISTRATORS SHOULD          *
 * CHOOSE HOW TO CONFIGURE THEIR SYSTEMS BASED ON THEIR KNOWLEDGE OF    *
 * THE TRAFFIC IN QUESTION, NOT WHETHER THERE IS A PORT NUMBER          *
 * REGISTERED OR NOT.                                                   *
 ************************************************************************
 */
#define IPA_TCP_PORTS "3002,3003,3006,4249,4250,5000"
#define IPA_UDP_PORTS "3006"
#define IPA_UDP_PORTS_DEFAULT "0"

static dissector_handle_t ipa_tcp_handle;
static dissector_handle_t ipa_udp_handle;
static range_t *global_ipa_tcp_ports = NULL;
static range_t *global_ipa_udp_ports = NULL;
static gboolean global_ipa_in_root = FALSE;
static gboolean global_ipa_in_info = FALSE;

/* Initialize the protocol and registered fields */
static int proto_ipa = -1;
static int proto_ipaccess = -1;

static int hf_ipa_data_len = -1;
static int hf_ipa_protocol = -1;
static int hf_ipa_hsl_debug = -1;
static int hf_ipa_osmo_proto = -1;
static int hf_ipa_osmo_ctrl_data = -1;

static int hf_ipaccess_msgtype = -1;
static int hf_ipaccess_attr_tag = -1;
static int hf_ipaccess_attr_string = -1;
static int hf_ipaccess_attribute_unk = -1;

/* Initialize the subtree pointers */
static gint ett_ipa = -1;
static gint ett_ipaccess = -1;

enum {
	SUB_OML,
	SUB_RSL,
	SUB_SCCP,
	SUB_MGCP,
/*	SUB_IPACCESS, */
	SUB_DATA,

	SUB_MAX
};

static dissector_handle_t sub_handles[SUB_MAX];
static dissector_table_t osmo_dissector_table;


#define ABISIP_RSL_MAX	0x20
#define HSL_DEBUG	0xdd
#define OSMO_EXT	0xee
#define IPA_MGCP	0xfc
#define AIP_SCCP	0xfd
#define ABISIP_IPACCESS	0xfe
#define ABISIP_OML	0xff
#define IPAC_PROTO_EXT_CTRL	0x00
#define IPAC_PROTO_EXT_MGCP	0x01

static const value_string ipa_protocol_vals[] = {
	{ 0x00,		"RSL" },
	{ 0xdd,		"HSL Debug" },
	{ 0xee,		"OSMO EXT" },
	{ 0xfc,		"MGCP (old)" },
	{ 0xfd,		"SCCP" },
	{ 0xfe,		"IPA" },
	{ 0xff,		"OML" },
	{ 0,		NULL }
};

static const value_string ipaccess_msgtype_vals[] = {
	{ 0x00,		"PING?" },
	{ 0x01, 	"PONG!" },
	{ 0x04, 	"IDENTITY REQUEST" },
	{ 0x05, 	"IDENTITY RESPONSE" },
	{ 0x06, 	"IDENTITY ACK" },
	{ 0x07, 	"IDENTITY NACK" },
	{ 0x08,		"PROXY REQUEST" },
	{ 0x09,		"PROXY ACK" },
	{ 0x0a,		"PROXY NACK" },
	{ 0,		NULL }
};

static const value_string ipaccess_idtag_vals[] = {
	{ 0x00,		"Serial Number" },
	{ 0x01,		"Unit Name" },
	{ 0x02,		"Location" },
	{ 0x03,		"Unit Type" },
	{ 0x04,		"Equipment Version" },
	{ 0x05,		"Software Version" },
	{ 0x06,		"IP Address" },
	{ 0x07,		"MAC Address" },
	{ 0x08,		"Unit ID" },
	{ 0,		NULL }
};

static const value_string ipa_osmo_proto_vals[] = {
	{ 0x00,		"CTRL" },
	{ 0x01,		"MGCP" },
	{ 0x02,		"LAC" },
	{ 0x03,		"SMSC" },
	{ 0,		NULL }
};


static gint
dissect_ipa_attr(tvbuff_t *tvb, int base_offs, proto_tree *tree)
{
	guint8 len, attr_type;

	int offset = base_offs;

	while (tvb_reported_length_remaining(tvb, offset) > 0) {
		attr_type = tvb_get_guint8(tvb, offset);

		switch (attr_type) {
		case 0x00:	/* a string prefixed by its length */
			len = tvb_get_guint8(tvb, offset+1);
			proto_tree_add_item(tree, hf_ipaccess_attr_tag,
					    tvb, offset+2, 1, ENC_BIG_ENDIAN);
			proto_tree_add_item(tree, hf_ipaccess_attr_string,
					    tvb, offset+3, len-1, ENC_ASCII|ENC_NA);
			break;
		case 0x01:	/* a single-byte request for a certain attr */
			len = 0;
			proto_tree_add_item(tree, hf_ipaccess_attr_tag,
					    tvb, offset+1, 1, ENC_BIG_ENDIAN);
			break;
		default:
			len = 0;
			proto_tree_add_uint(tree, hf_ipaccess_attribute_unk, tvb, offset+1, 1,
					    attr_type);
			break;
		};
		offset += len + 2;
	};
	return offset;
}

/* Dissect an ip.access specific message */
static gint
dissect_ipaccess(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	proto_item *ti;
	proto_tree *ipaccess_tree;
	guint8 msg_type;

	msg_type = tvb_get_guint8(tvb, 0);

	col_append_fstr(pinfo->cinfo, COL_INFO, "%s ",
	                val_to_str(msg_type, ipaccess_msgtype_vals,
	                           "unknown 0x%02x"));
	if (tree) {
		ti = proto_tree_add_item(tree, proto_ipaccess, tvb, 0, -1, ENC_NA);
		ipaccess_tree = proto_item_add_subtree(ti, ett_ipaccess);
		proto_tree_add_item(ipaccess_tree, hf_ipaccess_msgtype,
				    tvb, 0, 1, ENC_BIG_ENDIAN);
		switch (msg_type) {
		case 4:
		case 5:
			dissect_ipa_attr(tvb, 1, ipaccess_tree);
			break;
		}
	}

	return 1;
}

/* Dissect the osmocom extension header */
static gint
dissect_osmo(tvbuff_t *tvb, packet_info *pinfo, proto_tree *ipatree, proto_tree *tree)
{
	tvbuff_t *next_tvb;
	guint8 osmo_proto;

	osmo_proto = tvb_get_guint8(tvb, 0);

	col_append_fstr(pinfo->cinfo, COL_INFO, "%s ",
	                val_to_str(osmo_proto, ipa_osmo_proto_vals,
	                           "unknown 0x%02x"));
	if (ipatree) {
		proto_tree_add_item(ipatree, hf_ipa_osmo_proto,
				    tvb, 0, 1, ENC_BIG_ENDIAN);
	}

	next_tvb = tvb_new_subset_remaining(tvb, 1);

	/* Call any subdissectors that registered for this protocol */
	if (dissector_try_uint(osmo_dissector_table, osmo_proto, next_tvb, pinfo, tree))
		return 1;

	/* Fallback to the standard MGCP dissector */
	if (osmo_proto == IPAC_PROTO_EXT_MGCP) {
		call_dissector(sub_handles[SUB_MGCP], next_tvb, pinfo, tree);
		return 1;
	/* Simply display the CTRL data as text */
	} else if (osmo_proto == IPAC_PROTO_EXT_CTRL) {
		if (tree) {
			proto_tree_add_item(tree, hf_ipa_osmo_ctrl_data, next_tvb, 0, -1, ENC_ASCII|ENC_NA);
		}
		return 1;
	}

	call_dissector(sub_handles[SUB_DATA], next_tvb, pinfo, tree);

	return 1;
}



/* Code to actually dissect the packets */
static void
dissect_ipa(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, gboolean is_udp)
{
	gint remaining;
	gint header_length = 3;
	int offset = 0;

	col_set_str(pinfo->cinfo, COL_PROTOCOL, "IPA");
	col_clear(pinfo->cinfo, COL_INFO);

	while ((remaining = tvb_reported_length_remaining(tvb, offset)) > 0) {
		proto_item *ti;
		proto_tree *ipa_tree = NULL;
		guint16 len, msg_type;
		tvbuff_t *next_tvb;

		len = tvb_get_ntohs(tvb, offset);
		msg_type = tvb_get_guint8(tvb, offset+2);

		col_append_fstr(pinfo->cinfo, COL_INFO, "%s ",
		                val_to_str(msg_type, ipa_protocol_vals,
		                           "unknown 0x%02x"));

		/*
		 * The IPA header is different depending on the transport protocol.
		 * With UDP there seems to be a fourth byte for the IPA header.
		 * We attempt to detect this by checking if the length from the
		 * header + four bytes of the IPA header equals the remaining size.
		 */
		if (is_udp && (len + 4 == remaining)) {
			header_length++;
		}

		if (tree) {
			ti = proto_tree_add_protocol_format(tree, proto_ipa,
					tvb, offset, len+header_length,
					"IPA protocol ip.access, type: %s",
					val_to_str(msg_type, ipa_protocol_vals,
						   "unknown 0x%02x"));
			ipa_tree = proto_item_add_subtree(ti, ett_ipa);
			proto_tree_add_item(ipa_tree, hf_ipa_data_len,
					    tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(ipa_tree, hf_ipa_protocol,
					    tvb, offset+2, 1, ENC_BIG_ENDIAN);
		}

		next_tvb = tvb_new_subset_length(tvb, offset+header_length, len);

		switch (msg_type) {
		case ABISIP_OML:
			/* hand this off to the standard A-bis OML dissector */
			if (sub_handles[SUB_OML])
				call_dissector(sub_handles[SUB_OML], next_tvb,
						 pinfo, tree);
			break;
		case ABISIP_IPACCESS:
			dissect_ipaccess(next_tvb, pinfo, tree);
			break;
		case AIP_SCCP:
			/* hand this off to the standard SCCP dissector */
			call_dissector(sub_handles[SUB_SCCP], next_tvb, pinfo, tree);
			break;
		case IPA_MGCP:
			/* hand this off to the standard MGCP dissector */
			call_dissector(sub_handles[SUB_MGCP], next_tvb, pinfo, tree);
			break;
		case OSMO_EXT:
			dissect_osmo(next_tvb, pinfo, ipa_tree, tree);
			break;
		case HSL_DEBUG:
			if (tree) {
				proto_tree_add_item(ipa_tree, hf_ipa_hsl_debug,
						    next_tvb, 0, len, ENC_ASCII|ENC_NA);
				if (global_ipa_in_root == TRUE)
					proto_tree_add_item(tree, hf_ipa_hsl_debug,
							    next_tvb, 0, len, ENC_ASCII|ENC_NA);
			}
			if (global_ipa_in_info == TRUE)
				col_append_fstr(pinfo->cinfo, COL_INFO, "%s ",
						tvb_get_stringz_enc(wmem_packet_scope(), next_tvb, 0, NULL, ENC_ASCII));
			break;
		default:
			if (msg_type < ABISIP_RSL_MAX) {
				/* hand this off to the standard A-bis RSL dissector */
				call_dissector(sub_handles[SUB_RSL], next_tvb, pinfo, tree);
			}
			break;
		}
		offset += len + header_length;
	}
}

static int
dissect_ipa_tcp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
	dissect_ipa(tvb, pinfo, tree, FALSE);
	return tvb_captured_length(tvb);
}

static int
dissect_ipa_udp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
	dissect_ipa(tvb, pinfo, tree, TRUE);
	return tvb_captured_length(tvb);
}

void proto_register_ipa(void)
{
	module_t *ipa_module;

	static hf_register_info hf[] = {
		{&hf_ipa_data_len,
		 {"DataLen", "gsm_ipa.data_len",
		  FT_UINT16, BASE_DEC, NULL, 0x0,
		  "The length of the data (in bytes)", HFILL}
		 },
		{&hf_ipa_protocol,
		 {"Protocol", "gsm_ipa.protocol",
		  FT_UINT8, BASE_HEX, VALS(ipa_protocol_vals), 0x0,
		  "The IPA Sub-Protocol", HFILL}
		 },
		{&hf_ipa_hsl_debug,
		 {"Debug Message", "gsm_ipa.hsl_debug",
		  FT_STRING, BASE_NONE, NULL, 0,
		  "Hay Systems Limited debug message", HFILL}
		},
		{&hf_ipa_osmo_proto,
		 {"Osmo ext protocol", "gsm_ipa.osmo.protocol",
		  FT_UINT8, BASE_HEX, VALS(ipa_osmo_proto_vals), 0x0,
		  "The osmo extension protocol", HFILL}
		},

		{&hf_ipa_osmo_ctrl_data,
		 {"CTRL data", "gsm_ipa.ctrl.data",
		  FT_STRING, BASE_NONE, NULL, 0x0,
		  "Control interface data", HFILL}
		},

	};
	static hf_register_info hf_ipa[] = {
		{&hf_ipaccess_msgtype,
		 {"MessageType", "ipaccess.msg_type",
		  FT_UINT8, BASE_HEX, VALS(ipaccess_msgtype_vals), 0x0,
		  "Type of ip.access message", HFILL}
		 },
		{&hf_ipaccess_attr_tag,
		 {"Tag", "ipaccess.attr_tag",
		  FT_UINT8, BASE_HEX, VALS(ipaccess_idtag_vals), 0x0,
		  "Attribute Tag", HFILL}
		 },
		{&hf_ipaccess_attr_string,
		 {"String", "ipaccess.attr_string",
		  FT_STRING, BASE_NONE, NULL, 0x0,
		  "String attribute", HFILL}
		 },
		{&hf_ipaccess_attribute_unk,
		 {"Unknown attribute type", "ipaccess.attr_unk",
		  FT_UINT8, BASE_HEX, NULL, 0x0,
		  NULL, HFILL}
		 },
	};

	static gint *ett[] = {
		&ett_ipa,
		&ett_ipaccess,
	};

	proto_ipa = proto_register_protocol("GSM over IP protocol as used by ip.access", "GSM over IP", "gsm_ipa");
	proto_ipaccess = proto_register_protocol("GSM over IP ip.access CCM sub-protocol", "IPA", "ipaccess");

	proto_register_field_array(proto_ipa, hf, array_length(hf));
	proto_register_field_array(proto_ipaccess, hf_ipa, array_length(hf_ipa));
	proto_register_subtree_array(ett, array_length(ett));

	/* Register table for subdissectors */
	osmo_dissector_table = register_dissector_table("ipa.osmo.protocol",
					"GSM over IP ip.access Protocol", proto_ipa,
					FT_UINT8, BASE_DEC);


	range_convert_str(&global_ipa_tcp_ports, IPA_TCP_PORTS, MAX_TCP_PORT);
	range_convert_str(&global_ipa_udp_ports, IPA_UDP_PORTS_DEFAULT, MAX_UDP_PORT);
	ipa_module = prefs_register_protocol(proto_ipa,
					     proto_reg_handoff_gsm_ipa);

	prefs_register_range_preference(ipa_module, "tcp_ports",
					"GSM IPA TCP Port(s)",
					"Set the port(s) for ip.access IPA"
					" (default: " IPA_TCP_PORTS ")",
					&global_ipa_tcp_ports, MAX_TCP_PORT);
	prefs_register_range_preference(ipa_module, "udp_ports",
					"GSM IPA UDP Port(s)",
					"Set the port(s) for ip.access IPA"
					" (usually: " IPA_UDP_PORTS ")",
					&global_ipa_udp_ports, MAX_UDP_PORT);

	prefs_register_bool_preference(ipa_module, "hsl_debug_in_root_tree",
					"HSL Debug messages in root protocol tree",
					NULL, &global_ipa_in_root);
	prefs_register_bool_preference(ipa_module, "hsl_debug_in_info",
					"HSL Debug messages in INFO column",
					NULL, &global_ipa_in_info);
}

void proto_reg_handoff_gsm_ipa(void)
{
	static gboolean ipa_initialized = FALSE;
	static range_t *ipa_tcp_ports, *ipa_udp_ports;

	if (!ipa_initialized) {
		sub_handles[SUB_RSL] = find_dissector_add_dependency("gsm_abis_rsl", proto_ipa);
		sub_handles[SUB_OML] = find_dissector_add_dependency("gsm_abis_oml", proto_ipa);
		sub_handles[SUB_SCCP] = find_dissector_add_dependency("sccp", proto_ipa);
		sub_handles[SUB_MGCP] = find_dissector_add_dependency("mgcp", proto_ipa);
		sub_handles[SUB_DATA] = find_dissector("data");

		ipa_tcp_handle = create_dissector_handle(dissect_ipa_tcp, proto_ipa);
		ipa_udp_handle = create_dissector_handle(dissect_ipa_udp, proto_ipa);
		ipa_initialized = TRUE;
	} else {
		dissector_delete_uint_range("tcp.port", ipa_tcp_ports, ipa_tcp_handle);
		g_free(ipa_tcp_ports);
		dissector_delete_uint_range("udp.port", ipa_udp_ports, ipa_udp_handle);
		g_free(ipa_udp_ports);
	}

	ipa_tcp_ports = range_copy(global_ipa_tcp_ports);
	ipa_udp_ports = range_copy(global_ipa_udp_ports);

	dissector_add_uint_range("udp.port", ipa_udp_ports, ipa_udp_handle);
	dissector_add_uint_range("tcp.port", ipa_tcp_ports, ipa_tcp_handle);
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
