/* packet-spp.c
 * Routines for XNS SPP
 * Based on the Netware SPX dissector by Gilbert Ramirez <gram@alumni.rice.edu>
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
#include "packet-idp.h"

void proto_register_spp(void);
void proto_reg_handoff_spp(void);

static int proto_spp = -1;
static int hf_spp_connection_control = -1;
static int hf_spp_connection_control_sys = -1;
static int hf_spp_connection_control_send_ack = -1;
static int hf_spp_connection_control_attn = -1;
static int hf_spp_connection_control_eom = -1;
static int hf_spp_datastream_type = -1;
static int hf_spp_src_id = -1;
static int hf_spp_dst_id = -1;
static int hf_spp_seq_nr = -1;
static int hf_spp_ack_nr = -1;
static int hf_spp_all_nr = -1;
/* static int hf_spp_rexmt_frame = -1; */

static gint ett_spp = -1;
static gint ett_spp_connctrl = -1;

static dissector_table_t spp_socket_dissector_table;

/*
 * See
 *
 *	"Internet Transport Protocols", XSIS 028112, December 1981
 *
 * if you can find it; this is based on the headers in the BSD XNS
 * implementation.
 */

#define SPP_SYS_PACKET	0x80
#define SPP_SEND_ACK	0x40
#define SPP_ATTN	0x20
#define SPP_EOM		0x10

static const char*
spp_conn_ctrl(guint8 ctrl)
{
	static const value_string conn_vals[] = {
		{ 0x00,                        "Data, No Ack Required" },
		{ SPP_EOM,                     "End-of-Message" },
		{ SPP_ATTN,                    "Attention" },
		{ SPP_SEND_ACK,                "Acknowledgment Required"},
		{ SPP_SEND_ACK|SPP_EOM,        "Send Ack: End Message"},
		{ SPP_SYS_PACKET,              "System Packet"},
		{ SPP_SYS_PACKET|SPP_SEND_ACK, "System Packet: Send Ack"},
		{ 0x00,                        NULL }
	};

	return val_to_str_const((ctrl & 0xf0), conn_vals, "Unknown");
}

static const char*
spp_datastream(guint8 type)
{
	switch (type) {
		case 0xfe:
			return "End-of-Connection";
		case 0xff:
			return "End-of-Connection Acknowledgment";
		default:
			return NULL;
	}
}

#define SPP_HEADER_LEN	12

/*
 * XXX - do reassembly, using the EOM flag.  (Then do that in the Netware
 * SPX implementation, too.)
 *
 * XXX - hand off to subdissectors based on the socket number.
 */
static int
dissect_spp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
	proto_tree *spp_tree;
	proto_item *ti;
	tvbuff_t   *next_tvb;
	guint8	    conn_ctrl;
	guint8	    datastream_type;
	const char *datastream_type_string;
	guint16     spp_seq;
	const char *spp_msg_string;
	guint16	    low_socket, high_socket;
	static const int * ctrl[] = {
		&hf_spp_connection_control_sys,
		&hf_spp_connection_control_send_ack,
		&hf_spp_connection_control_attn,
		&hf_spp_connection_control_eom,
		NULL
	};

	col_set_str(pinfo->cinfo, COL_PROTOCOL, "SPP");
	col_set_str(pinfo->cinfo, COL_INFO, "SPP");

	ti = proto_tree_add_item(tree, proto_spp, tvb, 0, SPP_HEADER_LEN, ENC_NA);
	spp_tree = proto_item_add_subtree(ti, ett_spp);

	conn_ctrl = tvb_get_guint8(tvb, 0);
	spp_msg_string = spp_conn_ctrl(conn_ctrl);
	col_append_fstr(pinfo->cinfo, COL_INFO, " %s", spp_msg_string);

	proto_tree_add_bitmask_with_flags(spp_tree, tvb, 0, hf_spp_connection_control, ett_spp_connctrl,
								ctrl, ENC_NA, BMT_NO_FALSE);

	datastream_type = tvb_get_guint8(tvb, 1);
	datastream_type_string = spp_datastream(datastream_type);
	if (datastream_type_string != NULL) {
		col_append_fstr(pinfo->cinfo, COL_INFO, " (%s)", datastream_type_string);
	}
	if (tree) {
		if (datastream_type_string != NULL) {
			proto_tree_add_uint_format_value(spp_tree, hf_spp_datastream_type, tvb,
						   1, 1, datastream_type,
						   "%s (0x%02X)",
						   datastream_type_string,
						   datastream_type);
		} else {
			proto_tree_add_uint(spp_tree, hf_spp_datastream_type, tvb,
						   1, 1, datastream_type);
		}
		proto_tree_add_item(spp_tree, hf_spp_src_id, tvb,  2, 2, ENC_BIG_ENDIAN);
		proto_tree_add_item(spp_tree, hf_spp_dst_id, tvb,  4, 2, ENC_BIG_ENDIAN);
	}
	spp_seq = tvb_get_ntohs(tvb, 6);
	if (tree) {
		proto_tree_add_uint(spp_tree, hf_spp_seq_nr, tvb,  6, 2, spp_seq);
		proto_tree_add_item(spp_tree, hf_spp_ack_nr, tvb,  8, 2, ENC_BIG_ENDIAN);
		proto_tree_add_item(spp_tree, hf_spp_all_nr, tvb, 10, 2, ENC_BIG_ENDIAN);
	}

	if (tvb_reported_length_remaining(tvb, SPP_HEADER_LEN) > 0) {
		if (pinfo->srcport > pinfo->destport) {
			low_socket = pinfo->destport;
			high_socket = pinfo->srcport;
		} else {
			low_socket = pinfo->srcport;
			high_socket = pinfo->destport;
		}

		next_tvb = tvb_new_subset_remaining(tvb, SPP_HEADER_LEN);
		if (dissector_try_uint(spp_socket_dissector_table, low_socket,
		    next_tvb, pinfo, tree))
			return tvb_captured_length(tvb);

		if (dissector_try_uint(spp_socket_dissector_table, high_socket,
		    next_tvb, pinfo, tree))
			return tvb_captured_length(tvb);

		call_data_dissector(next_tvb, pinfo, tree);
	}
	return tvb_captured_length(tvb);
}


void
proto_register_spp(void)
{
	static hf_register_info hf_spp[] = {
		{ &hf_spp_connection_control,
		{ "Connection Control",		"spp.ctl",
		  FT_UINT8,	BASE_HEX,	NULL,	0x0,
		  NULL, HFILL }},

		{ &hf_spp_connection_control_sys,
		{ "System Packet",		"spp.ctl.sys",
		  FT_BOOLEAN,	8,	NULL,	SPP_SYS_PACKET,
		  NULL, HFILL }},

		{ &hf_spp_connection_control_send_ack,
		{ "Send Ack",		"spp.ctl.send_ack",
		  FT_BOOLEAN,	8,	NULL,	SPP_SEND_ACK,
		  NULL, HFILL }},

		{ &hf_spp_connection_control_attn,
		{ "Attention",		"spp.ctl.attn",
		  FT_BOOLEAN,	8,	NULL,	SPP_ATTN,
		  NULL, HFILL }},

		{ &hf_spp_connection_control_eom,
		{ "End of Message",	"spp.ctl.eom",
		  FT_BOOLEAN,	8,	NULL,	SPP_EOM,
		  NULL, HFILL }},

		{ &hf_spp_datastream_type,
		{ "Datastream Type",	       	"spp.type",
		  FT_UINT8,	BASE_HEX,	NULL,	0x0,
		  NULL, HFILL }},

		{ &hf_spp_src_id,
		{ "Source Connection ID",	"spp.src",
		  FT_UINT16,	BASE_DEC,	NULL,	0x0,
		  NULL, HFILL }},

		{ &hf_spp_dst_id,
		{ "Destination Connection ID",	"spp.dst",
		  FT_UINT16,	BASE_DEC,	NULL,	0x0,
		  NULL, HFILL }},

		{ &hf_spp_seq_nr,
		{ "Sequence Number",		"spp.seq",
		  FT_UINT16,	BASE_DEC,	NULL,	0x0,
		  NULL, HFILL }},

		{ &hf_spp_ack_nr,
		{ "Acknowledgment Number",	"spp.ack",
		  FT_UINT16,	BASE_DEC,	NULL,	0x0,
		  NULL, HFILL }},

		{ &hf_spp_all_nr,
		{ "Allocation Number",		"spp.alloc",
		  FT_UINT16,	BASE_DEC,	NULL,	0x0,
		  NULL, HFILL }},

#if 0
		{ &hf_spp_rexmt_frame,
		{ "Retransmitted Frame Number",	"spp.rexmt_frame",
		  FT_FRAMENUM,	BASE_NONE,	NULL,	0x0,
		  NULL, HFILL }},
#endif
	};

	static gint *ett[] = {
		&ett_spp,
		&ett_spp_connctrl,
	};

	proto_spp = proto_register_protocol("Sequenced Packet Protocol",
	    "SPP", "spp");
	proto_register_field_array(proto_spp, hf_spp, array_length(hf_spp));
	proto_register_subtree_array(ett, array_length(ett));

	spp_socket_dissector_table = register_dissector_table("spp.socket",
	    "SPP socket", proto_spp, FT_UINT16, BASE_HEX);
}

void
proto_reg_handoff_spp(void)
{
	dissector_handle_t spp_handle;

	spp_handle = create_dissector_handle(dissect_spp, proto_spp);
	dissector_add_uint("idp.packet_type", IDP_PACKET_TYPE_SPP, spp_handle);
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
