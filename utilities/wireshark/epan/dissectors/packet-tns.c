/* packet-tns.c
 * Routines for Oracle TNS packet dissection
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * Copied from packet-tftp.c
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
#include "packet-tcp.h"
#include "packet-tns.h"

#include <epan/prefs.h>

void proto_register_tns(void);

/* desegmentation of TNS over TCP */
static gboolean tns_desegment = TRUE;

static int proto_tns = -1;
static int hf_tns_request = -1;
static int hf_tns_response = -1;
static int hf_tns_length = -1;
static int hf_tns_packet_checksum = -1;
static int hf_tns_header_checksum = -1;
static int hf_tns_packet_type = -1;
static int hf_tns_reserved_byte = -1;
static int hf_tns_connect = -1;
static int hf_tns_version = -1;
static int hf_tns_compat_version = -1;

static int hf_tns_service_options = -1;
static int hf_tns_sopt_flag_bconn = -1;
static int hf_tns_sopt_flag_pc = -1;
static int hf_tns_sopt_flag_hc = -1;
static int hf_tns_sopt_flag_fd = -1;
static int hf_tns_sopt_flag_hd = -1;
static int hf_tns_sopt_flag_dc1 = -1;
static int hf_tns_sopt_flag_dc2 = -1;
static int hf_tns_sopt_flag_dio = -1;
static int hf_tns_sopt_flag_ap = -1;
static int hf_tns_sopt_flag_ra = -1;
static int hf_tns_sopt_flag_sa = -1;

static int hf_tns_sdu_size = -1;
static int hf_tns_max_tdu_size = -1;

static int hf_tns_nt_proto_characteristics = -1;
static int hf_tns_ntp_flag_hangon = -1;
static int hf_tns_ntp_flag_crel = -1;
static int hf_tns_ntp_flag_tduio = -1;
static int hf_tns_ntp_flag_srun = -1;
static int hf_tns_ntp_flag_dtest = -1;
static int hf_tns_ntp_flag_cbio = -1;
static int hf_tns_ntp_flag_asio = -1;
static int hf_tns_ntp_flag_pio = -1;
static int hf_tns_ntp_flag_grant = -1;
static int hf_tns_ntp_flag_handoff = -1;
static int hf_tns_ntp_flag_sigio = -1;
static int hf_tns_ntp_flag_sigpipe = -1;
static int hf_tns_ntp_flag_sigurg = -1;
static int hf_tns_ntp_flag_urgentio = -1;
static int hf_tns_ntp_flag_fdio = -1;
static int hf_tns_ntp_flag_testop = -1;

static int hf_tns_line_turnaround = -1;
static int hf_tns_value_of_one = -1;
static int hf_tns_connect_data_length = -1;
static int hf_tns_connect_data_offset = -1;
static int hf_tns_connect_data_max = -1;

static int hf_tns_connect_flags0 = -1;
static int hf_tns_connect_flags1 = -1;
static int hf_tns_conn_flag_nareq = -1;
static int hf_tns_conn_flag_nalink = -1;
static int hf_tns_conn_flag_enablena = -1;
static int hf_tns_conn_flag_ichg = -1;
static int hf_tns_conn_flag_wantna = -1;

static int hf_tns_connect_data = -1;
static int hf_tns_trace_cf1 = -1;
static int hf_tns_trace_cf2 = -1;
static int hf_tns_trace_cid = -1;

static int hf_tns_accept = -1;
static int hf_tns_accept_data_length = -1;
static int hf_tns_accept_data_offset = -1;
static int hf_tns_accept_data = -1;

static int hf_tns_refuse = -1;
static int hf_tns_refuse_reason_user = -1;
static int hf_tns_refuse_reason_system = -1;
static int hf_tns_refuse_data_length = -1;
static int hf_tns_refuse_data = -1;

static int hf_tns_abort = -1;
static int hf_tns_abort_reason_user = -1;
static int hf_tns_abort_reason_system = -1;
static int hf_tns_abort_data = -1;

static int hf_tns_marker = -1;
static int hf_tns_marker_type = -1;
static int hf_tns_marker_data_byte = -1;
/* static int hf_tns_marker_data = -1; */

static int hf_tns_redirect = -1;
static int hf_tns_redirect_data_length = -1;
static int hf_tns_redirect_data = -1;

static int hf_tns_control = -1;
static int hf_tns_control_cmd = -1;
static int hf_tns_control_data = -1;

static int hf_tns_data_flag = -1;
static int hf_tns_data_flag_send = -1;
static int hf_tns_data_flag_rc = -1;
static int hf_tns_data_flag_c = -1;
static int hf_tns_data_flag_reserved = -1;
static int hf_tns_data_flag_more = -1;
static int hf_tns_data_flag_eof = -1;
static int hf_tns_data_flag_dic = -1;
static int hf_tns_data_flag_rts = -1;
static int hf_tns_data_flag_sntt = -1;
static int hf_tns_data = -1;

static gint ett_tns = -1;
static gint ett_tns_connect = -1;
static gint ett_tns_accept = -1;
static gint ett_tns_refuse = -1;
static gint ett_tns_abort = -1;
static gint ett_tns_redirect = -1;
static gint ett_tns_marker = -1;
static gint ett_tns_attention = -1;
static gint ett_tns_control = -1;
static gint ett_tns_data = -1;
static gint ett_tns_data_flag = -1;
static gint ett_tns_sopt_flag = -1;
static gint ett_tns_ntp_flag = -1;
static gint ett_tns_conn_flag = -1;
static gint ett_sql = -1;

#define TCP_PORT_TNS			1521

static const value_string tns_type_vals[] = {
	{TNS_TYPE_CONNECT,   "Connect" },
	{TNS_TYPE_ACCEPT,    "Accept" },
	{TNS_TYPE_ACK,       "Acknowledge" },
	{TNS_TYPE_REFUSE,    "Refuse" },
	{TNS_TYPE_REDIRECT,  "Redirect" },
	{TNS_TYPE_DATA,      "Data" },
	{TNS_TYPE_NULL,      "Null" },
	{TNS_TYPE_ABORT,     "Abort" },
	{TNS_TYPE_RESEND,    "Resend"},
	{TNS_TYPE_MARKER,    "Marker"},
	{TNS_TYPE_ATTENTION, "Attention"},
	{TNS_TYPE_CONTROL,   "Control"},
	{0, NULL}
};

static const value_string tns_marker_types[] = {
	{0, "Data Marker - 0 Data Bytes"},
	{1, "Data Marker - 1 Data Bytes"},
	{2, "Attention Marker"},
	{0, NULL}
};

static const value_string tns_control_cmds[] = {
	{1, "Oracle Trace Command"},
	{0, NULL}
};

void proto_reg_handoff_tns(void);
static guint get_tns_pdu_len(packet_info *pinfo, tvbuff_t *tvb, int offset, void *data);
static int dissect_tns_pdu(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_);

static void dissect_tns_service_options(tvbuff_t *tvb, int offset,
	proto_tree *sopt_tree)
{

	proto_tree_add_item(sopt_tree, hf_tns_sopt_flag_bconn, tvb,
			offset, 2, ENC_BIG_ENDIAN);
	proto_tree_add_item(sopt_tree, hf_tns_sopt_flag_pc, tvb,
			offset, 2, ENC_BIG_ENDIAN);
	proto_tree_add_item(sopt_tree, hf_tns_sopt_flag_hc, tvb,
			offset, 2, ENC_BIG_ENDIAN);
	proto_tree_add_item(sopt_tree, hf_tns_sopt_flag_fd, tvb,
			offset, 2, ENC_BIG_ENDIAN);
	proto_tree_add_item(sopt_tree, hf_tns_sopt_flag_hd, tvb,
			offset, 2, ENC_BIG_ENDIAN);
	proto_tree_add_item(sopt_tree, hf_tns_sopt_flag_dc1, tvb,
			offset, 2, ENC_BIG_ENDIAN);
	proto_tree_add_item(sopt_tree, hf_tns_sopt_flag_dc2, tvb,
			offset, 2, ENC_BIG_ENDIAN);
	proto_tree_add_item(sopt_tree, hf_tns_sopt_flag_dio, tvb,
			offset, 2, ENC_BIG_ENDIAN);
	proto_tree_add_item(sopt_tree, hf_tns_sopt_flag_ap, tvb,
			offset, 2, ENC_BIG_ENDIAN);
	proto_tree_add_item(sopt_tree, hf_tns_sopt_flag_ra, tvb,
			offset, 2, ENC_BIG_ENDIAN);
	proto_tree_add_item(sopt_tree, hf_tns_sopt_flag_sa, tvb,
			offset, 2, ENC_BIG_ENDIAN);

}

static void dissect_tns_connect_flag(tvbuff_t *tvb, int offset,
	proto_tree *cflag_tree)
{

	proto_tree_add_item(cflag_tree, hf_tns_conn_flag_nareq, tvb, offset, 1, ENC_BIG_ENDIAN);
	proto_tree_add_item(cflag_tree, hf_tns_conn_flag_nalink, tvb, offset, 1, ENC_BIG_ENDIAN);
	proto_tree_add_item(cflag_tree, hf_tns_conn_flag_enablena, tvb, offset, 1, ENC_BIG_ENDIAN);
	proto_tree_add_item(cflag_tree, hf_tns_conn_flag_ichg, tvb, offset, 1, ENC_BIG_ENDIAN);
	proto_tree_add_item(cflag_tree, hf_tns_conn_flag_wantna, tvb, offset, 1, ENC_BIG_ENDIAN);
}

static void dissect_tns_data(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, proto_tree *tns_tree)
{
	proto_tree *data_tree = NULL, *ti;
	proto_item *hidden_item;
	int is_sns = 0;

	if ( tvb_bytes_exist(tvb, offset+2, 4) )
	{
		if ( tvb_get_guint8(tvb, offset+2) == 0xDE &&
		     tvb_get_guint8(tvb, offset+3) == 0xAD &&
		     tvb_get_guint8(tvb, offset+4) == 0xBE &&
		     tvb_get_guint8(tvb, offset+5) == 0xEF )
		{
			is_sns = 1;
		}
	}

	if ( tree )
	{
		if ( is_sns )
		{
			data_tree = proto_tree_add_subtree(tns_tree, tvb, offset, -1,
			    ett_tns_data, NULL, "Secure Network Services");
		}
		else
		{
			data_tree = proto_tree_add_subtree(tns_tree, tvb, offset, -1,
			    ett_tns_data, NULL, "Data");
		}

		hidden_item = proto_tree_add_boolean(tns_tree, hf_tns_data, tvb, 0, 0,
					TRUE);
		PROTO_ITEM_SET_HIDDEN(hidden_item);
	}

	if ( tree )
	{
		proto_tree *df_tree = NULL;

		ti = proto_tree_add_item(data_tree, hf_tns_data_flag, tvb, offset, 2, ENC_BIG_ENDIAN);

		df_tree = proto_item_add_subtree(ti, ett_tns_data_flag);
		proto_tree_add_item(df_tree, hf_tns_data_flag_send, tvb, offset, 2, ENC_BIG_ENDIAN);
		proto_tree_add_item(df_tree, hf_tns_data_flag_rc, tvb, offset, 2, ENC_BIG_ENDIAN);
		proto_tree_add_item(df_tree, hf_tns_data_flag_c, tvb, offset, 2, ENC_BIG_ENDIAN);
		proto_tree_add_item(df_tree, hf_tns_data_flag_reserved, tvb, offset, 2, ENC_BIG_ENDIAN);
		proto_tree_add_item(df_tree, hf_tns_data_flag_more, tvb, offset, 2, ENC_BIG_ENDIAN);
		proto_tree_add_item(df_tree, hf_tns_data_flag_eof, tvb, offset, 2, ENC_BIG_ENDIAN);
		proto_tree_add_item(df_tree, hf_tns_data_flag_dic, tvb, offset, 2, ENC_BIG_ENDIAN);
		proto_tree_add_item(df_tree, hf_tns_data_flag_rts, tvb, offset, 2, ENC_BIG_ENDIAN);
		proto_tree_add_item(df_tree, hf_tns_data_flag_sntt, tvb, offset, 2, ENC_BIG_ENDIAN);
	}
	offset += 2;

	if ( is_sns )
	{
		col_append_str(pinfo->cinfo, COL_INFO, ", SNS");
	}
	else
	{
		col_append_str(pinfo->cinfo, COL_INFO, ", Data");
	}

	call_data_dissector(tvb_new_subset_remaining(tvb, offset), pinfo, data_tree);

	return;
}

static void dissect_tns_connect(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, proto_tree *tns_tree)
{
	proto_tree *connect_tree = NULL, *ti;
	proto_item *hidden_item;
	int cd_offset;
	int cd_len;
	int tns_offset = offset-8;

	if ( tree )
	{
		connect_tree = proto_tree_add_subtree(tns_tree, tvb, offset, -1,
		    ett_tns_connect, NULL, "Connect");

		hidden_item = proto_tree_add_boolean(tns_tree, hf_tns_connect, tvb,
				    0, 0, TRUE);
		PROTO_ITEM_SET_HIDDEN(hidden_item);
	}

	col_append_str(pinfo->cinfo, COL_INFO, ", Connect");

	if ( connect_tree )
	{
		proto_tree_add_item(connect_tree, hf_tns_version, tvb,
			offset, 2, ENC_BIG_ENDIAN);
	}
	offset += 2;

	if ( connect_tree )
	{
		proto_tree_add_item(connect_tree, hf_tns_compat_version, tvb,
			offset, 2, ENC_BIG_ENDIAN);
	}
	offset += 2;

	if ( connect_tree )
	{
		proto_tree *sopt_tree = NULL;

		ti = proto_tree_add_item(connect_tree, hf_tns_service_options, tvb,
			offset, 2, ENC_BIG_ENDIAN);

		sopt_tree = proto_item_add_subtree(ti, ett_tns_sopt_flag);

		dissect_tns_service_options(tvb, offset, sopt_tree);


	}
	offset += 2;

	if ( connect_tree )
	{
		proto_tree_add_item(connect_tree, hf_tns_sdu_size, tvb,
			offset, 2, ENC_BIG_ENDIAN);
	}
	offset += 2;

	if ( connect_tree )
	{
		proto_tree_add_item(connect_tree, hf_tns_max_tdu_size, tvb,
			offset, 2, ENC_BIG_ENDIAN);
	}
	offset += 2;

	if ( connect_tree )
	{
		proto_tree *ntp_tree = NULL;

		ti = proto_tree_add_item(connect_tree, hf_tns_nt_proto_characteristics, tvb,
			offset, 2, ENC_BIG_ENDIAN);

		ntp_tree = proto_item_add_subtree(ti, ett_tns_ntp_flag);

		proto_tree_add_item(ntp_tree, hf_tns_ntp_flag_hangon, tvb, offset, 2, ENC_BIG_ENDIAN);
		proto_tree_add_item(ntp_tree, hf_tns_ntp_flag_crel, tvb, offset, 2, ENC_BIG_ENDIAN);
		proto_tree_add_item(ntp_tree, hf_tns_ntp_flag_tduio, tvb, offset, 2, ENC_BIG_ENDIAN);
		proto_tree_add_item(ntp_tree, hf_tns_ntp_flag_srun, tvb, offset, 2, ENC_BIG_ENDIAN);
		proto_tree_add_item(ntp_tree, hf_tns_ntp_flag_dtest, tvb, offset, 2, ENC_BIG_ENDIAN);
		proto_tree_add_item(ntp_tree, hf_tns_ntp_flag_cbio, tvb, offset, 2, ENC_BIG_ENDIAN);
		proto_tree_add_item(ntp_tree, hf_tns_ntp_flag_asio, tvb, offset, 2, ENC_BIG_ENDIAN);
		proto_tree_add_item(ntp_tree, hf_tns_ntp_flag_pio, tvb, offset, 2, ENC_BIG_ENDIAN);
		proto_tree_add_item(ntp_tree, hf_tns_ntp_flag_grant, tvb, offset, 2, ENC_BIG_ENDIAN);
		proto_tree_add_item(ntp_tree, hf_tns_ntp_flag_handoff, tvb, offset, 2, ENC_BIG_ENDIAN);
		proto_tree_add_item(ntp_tree, hf_tns_ntp_flag_sigio, tvb, offset, 2, ENC_BIG_ENDIAN);
		proto_tree_add_item(ntp_tree, hf_tns_ntp_flag_sigpipe, tvb, offset, 2, ENC_BIG_ENDIAN);
		proto_tree_add_item(ntp_tree, hf_tns_ntp_flag_sigurg, tvb, offset, 2, ENC_BIG_ENDIAN);
		proto_tree_add_item(ntp_tree, hf_tns_ntp_flag_urgentio, tvb, offset, 2, ENC_BIG_ENDIAN);
		proto_tree_add_item(ntp_tree, hf_tns_ntp_flag_fdio, tvb, offset, 2, ENC_BIG_ENDIAN);
		proto_tree_add_item(ntp_tree, hf_tns_ntp_flag_testop, tvb, offset, 2, ENC_BIG_ENDIAN);
	}
	offset += 2;

	if ( connect_tree )
	{
		proto_tree_add_item(connect_tree, hf_tns_line_turnaround, tvb,
			offset, 2, ENC_BIG_ENDIAN);
	}
	offset += 2;

	if ( connect_tree )
	{
		proto_tree_add_item(connect_tree, hf_tns_value_of_one, tvb,
			offset, 2, ENC_NA);
	}
	offset += 2;

	cd_len = tvb_get_ntohs(tvb, offset);
	if ( connect_tree )
	{
		proto_tree_add_uint(connect_tree, hf_tns_connect_data_length, tvb,
			offset, 2, cd_len);
	}
	offset += 2;

	cd_offset = tvb_get_ntohs(tvb, offset);
	if ( connect_tree )
	{
		proto_tree_add_uint(connect_tree, hf_tns_connect_data_offset, tvb,
			offset, 2, cd_offset);
	}
	offset += 2;

	if ( connect_tree )
	{
		proto_tree_add_item(connect_tree, hf_tns_connect_data_max, tvb,
			offset, 4, ENC_BIG_ENDIAN);
	}
	offset += 4;

	if ( connect_tree )
	{
		proto_tree *cflag_tree = NULL;

		ti = proto_tree_add_item(connect_tree, hf_tns_connect_flags0, tvb,
			offset, 1, ENC_BIG_ENDIAN);

		cflag_tree = proto_item_add_subtree(ti, ett_tns_conn_flag);

		dissect_tns_connect_flag(tvb, offset, cflag_tree);
	}
	offset += 1;

	if ( connect_tree )
	{
		proto_tree *cflag_tree = NULL;

		ti = proto_tree_add_item(connect_tree, hf_tns_connect_flags1, tvb,
			offset, 1, ENC_BIG_ENDIAN);

		cflag_tree = proto_item_add_subtree(ti, ett_tns_conn_flag);

		dissect_tns_connect_flag(tvb, offset, cflag_tree);
	}
	offset += 1;

	/*
	 * XXX - sometimes it appears that this stuff isn't present
	 * in the packet.
	 */
	if (offset + 16 <= tns_offset+cd_offset)
	{
		if ( connect_tree )
		{
			proto_tree_add_item(connect_tree, hf_tns_trace_cf1, tvb,
				offset, 4, ENC_BIG_ENDIAN);
		}
		offset += 4;

		if ( connect_tree )
		{
			proto_tree_add_item(connect_tree, hf_tns_trace_cf2, tvb,
				offset, 4, ENC_BIG_ENDIAN);
		}
		offset += 4;

		if ( connect_tree )
		{
			proto_tree_add_item(connect_tree, hf_tns_trace_cid, tvb,
				offset, 8, ENC_BIG_ENDIAN);
		}
		/* offset += 8;*/
	}

	if ( connect_tree && cd_len > 0)
	{
		proto_tree_add_item(connect_tree, hf_tns_connect_data, tvb,
			tns_offset+cd_offset, -1, ENC_ASCII|ENC_NA);
	}
	return;
}

static void dissect_tns_accept(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree _U_, proto_tree *tns_tree)
{
	proto_tree *accept_tree, *ti;
	proto_item *hidden_item;
	int accept_offset;
	int accept_len;
	int tns_offset = offset-8;

	accept_tree = proto_tree_add_subtree(tns_tree, tvb, offset, -1,
		    ett_tns_accept, NULL, "Accept");

	hidden_item = proto_tree_add_boolean(tns_tree, hf_tns_accept, tvb,
				    0, 0, TRUE);
	PROTO_ITEM_SET_HIDDEN(hidden_item);

	col_append_str(pinfo->cinfo, COL_INFO, ", Accept");

	proto_tree_add_item(accept_tree, hf_tns_version, tvb,
			offset, 2, ENC_BIG_ENDIAN);
	offset += 2;

	if ( accept_tree )
	{
		proto_tree *sopt_tree = NULL;

		ti = proto_tree_add_item(accept_tree, hf_tns_service_options,
			tvb, offset, 2, ENC_BIG_ENDIAN);

		sopt_tree = proto_item_add_subtree(ti, ett_tns_sopt_flag);

		dissect_tns_service_options(tvb, offset, sopt_tree);

	}
	offset += 2;

	if ( accept_tree )
	{
		proto_tree_add_item(accept_tree, hf_tns_sdu_size, tvb,
			offset, 2, ENC_BIG_ENDIAN);
	}
	offset += 2;

	if ( accept_tree )
	{
		proto_tree_add_item(accept_tree, hf_tns_max_tdu_size, tvb,
			offset, 2, ENC_BIG_ENDIAN);
	}
	offset += 2;

	if ( accept_tree )
	{
		proto_tree_add_item(accept_tree, hf_tns_value_of_one, tvb,
			offset, 2, ENC_NA);
	}
	offset += 2;

	accept_len = tvb_get_ntohs(tvb, offset);
	if ( accept_tree )
	{
		proto_tree_add_uint(accept_tree, hf_tns_accept_data_length, tvb,
			offset, 2, accept_len);
	}
	offset += 2;

	accept_offset = tvb_get_ntohs(tvb, offset);
	if ( accept_tree )
	{
		proto_tree_add_uint(accept_tree, hf_tns_accept_data_offset, tvb,
			offset, 2, accept_offset);
	}
	offset += 2;

	if ( accept_tree )
	{
		proto_tree *cflag_tree = NULL;

		ti = proto_tree_add_item(accept_tree, hf_tns_connect_flags0, tvb,
			offset, 1, ENC_BIG_ENDIAN);

		cflag_tree = proto_item_add_subtree(ti, ett_tns_conn_flag);

		dissect_tns_connect_flag(tvb, offset, cflag_tree);

	}
	offset += 1;

	if ( accept_tree )
	{
		proto_tree *cflag_tree = NULL;

		ti = proto_tree_add_item(accept_tree, hf_tns_connect_flags1, tvb,
			offset, 1, ENC_BIG_ENDIAN);

		cflag_tree = proto_item_add_subtree(ti, ett_tns_conn_flag);

		dissect_tns_connect_flag(tvb, offset, cflag_tree);

	}
	/* offset += 1; */

	if ( accept_tree && accept_len > 0)
	{
		proto_tree_add_item(accept_tree, hf_tns_accept_data, tvb,
			tns_offset+accept_offset, -1, ENC_ASCII|ENC_NA);
	}
	return;
}


static void dissect_tns_refuse(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree _U_, proto_tree *tns_tree)
{
	proto_tree *refuse_tree;
	proto_item *hidden_item;

	refuse_tree = proto_tree_add_subtree(tns_tree, tvb, offset, -1,
		    ett_tns_refuse, NULL, "Refuse");

	hidden_item = proto_tree_add_boolean(tns_tree, hf_tns_refuse, tvb,
				    0, 0, TRUE);
	PROTO_ITEM_SET_HIDDEN(hidden_item);

	col_append_str(pinfo->cinfo, COL_INFO, ", Refuse");

	proto_tree_add_item(refuse_tree, hf_tns_refuse_reason_user, tvb,
			offset, 1, ENC_BIG_ENDIAN);
	offset += 1;

	proto_tree_add_item(refuse_tree, hf_tns_refuse_reason_system, tvb,
			offset, 1, ENC_BIG_ENDIAN);
	offset += 1;

	proto_tree_add_item(refuse_tree, hf_tns_refuse_data_length, tvb,
			offset, 2, ENC_BIG_ENDIAN);
	offset += 2;

	proto_tree_add_item(refuse_tree, hf_tns_refuse_data, tvb,
			offset, -1, ENC_ASCII|ENC_NA);
}


static void dissect_tns_abort(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree _U_, proto_tree *tns_tree)
{
	proto_tree *abort_tree;
	proto_item *hidden_item;

	abort_tree = proto_tree_add_subtree(tns_tree, tvb, offset, -1,
		    ett_tns_abort, NULL, "Abort");

	hidden_item = proto_tree_add_boolean(tns_tree, hf_tns_abort, tvb,
				    0, 0, TRUE);
	PROTO_ITEM_SET_HIDDEN(hidden_item);

	col_append_str(pinfo->cinfo, COL_INFO, ", Abort");

	proto_tree_add_item(abort_tree, hf_tns_abort_reason_user, tvb,
			offset, 1, ENC_BIG_ENDIAN);
	offset += 1;

	proto_tree_add_item(abort_tree, hf_tns_abort_reason_system, tvb,
			offset, 1, ENC_BIG_ENDIAN);
	offset += 1;

	proto_tree_add_item(abort_tree, hf_tns_abort_data, tvb,
			offset, -1, ENC_ASCII|ENC_NA);
}


static void dissect_tns_marker(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree _U_, proto_tree *tns_tree, int is_attention)
{
	proto_tree *marker_tree;
	proto_item *hidden_item;

	if ( is_attention )
	{
		col_append_str(pinfo->cinfo, COL_INFO, ", Marker");
		marker_tree = proto_tree_add_subtree(tns_tree, tvb, offset, -1,
			    ett_tns_marker, NULL, "Marker");
	}
	else
	{
		col_append_str(pinfo->cinfo, COL_INFO, ", Attention");
		marker_tree = proto_tree_add_subtree(tns_tree, tvb, offset, -1,
			    ett_tns_marker, NULL, "Attention");
	}

	hidden_item = proto_tree_add_boolean(tns_tree, hf_tns_marker, tvb,
				    0, 0, TRUE);
	PROTO_ITEM_SET_HIDDEN(hidden_item);

	proto_tree_add_item(marker_tree, hf_tns_marker_type, tvb,
			offset, 1, ENC_BIG_ENDIAN);
	offset += 1;

	proto_tree_add_item(marker_tree, hf_tns_marker_data_byte, tvb,
			offset, 1, ENC_BIG_ENDIAN);
	offset += 1;

	proto_tree_add_item(marker_tree, hf_tns_marker_data_byte, tvb,
			offset, 1, ENC_BIG_ENDIAN);
	/*offset += 1;*/
}

static void dissect_tns_redirect(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree _U_, proto_tree *tns_tree)
{
	proto_tree *redirect_tree;
	proto_item *hidden_item;

	redirect_tree = proto_tree_add_subtree(tns_tree, tvb, offset, -1,
		    ett_tns_redirect, NULL, "Redirect");

	hidden_item = proto_tree_add_boolean(tns_tree, hf_tns_redirect, tvb,
				    0, 0, TRUE);
	PROTO_ITEM_SET_HIDDEN(hidden_item);

	col_append_str(pinfo->cinfo, COL_INFO, ", Redirect");

	proto_tree_add_item(redirect_tree, hf_tns_redirect_data_length, tvb,
			offset, 2, ENC_BIG_ENDIAN);
	offset += 2;

	proto_tree_add_item(redirect_tree, hf_tns_redirect_data, tvb,
			offset, -1, ENC_ASCII|ENC_NA);
}

static void dissect_tns_control(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree _U_, proto_tree *tns_tree)
{
	proto_tree *control_tree;
	proto_item *hidden_item;

	control_tree = proto_tree_add_subtree(tns_tree, tvb, offset, -1,
		    ett_tns_control, NULL, "Control");

	hidden_item = proto_tree_add_boolean(tns_tree, hf_tns_control, tvb,
				    0, 0, TRUE);
	PROTO_ITEM_SET_HIDDEN(hidden_item);

	col_append_str(pinfo->cinfo, COL_INFO, ", Control");

	proto_tree_add_item(control_tree, hf_tns_control_cmd, tvb,
			offset, 2, ENC_BIG_ENDIAN);
	offset += 2;

	proto_tree_add_item(control_tree, hf_tns_control_data, tvb,
			offset, -1, ENC_NA);
}

static guint
get_tns_pdu_len(packet_info *pinfo _U_, tvbuff_t *tvb, int offset, void *data _U_)
{
	/*
	 * Get the length of the TNS message, including header
	 */
	return tvb_get_ntohs(tvb, offset);
}

static int
dissect_tns(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data)
{
	guint8 type;

	/*
	 * First, do a sanity check to make sure what we have
	 * starts with a TNS PDU.
	 */
	if (tvb_bytes_exist(tvb, 4, 1)) {
		/*
		 * Well, we have the packet type; let's make sure
		 * it's a known type.
		 */
		type = tvb_get_guint8(tvb, 4);
		if (type < TNS_TYPE_CONNECT || type > TNS_TYPE_MAX)
			return 0;	/* it's not a known type */
	}

	tcp_dissect_pdus(tvb, pinfo, tree, tns_desegment, 2,
	    get_tns_pdu_len, dissect_tns_pdu, data);
	return tvb_captured_length(tvb);
}

static int
dissect_tns_pdu(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
	proto_tree      *tns_tree = NULL, *ti;
	proto_item *hidden_item;
	int offset = 0;
	guint16 length;
	guint16 type;

	col_set_str(pinfo->cinfo, COL_PROTOCOL, "TNS");

	col_set_str(pinfo->cinfo, COL_INFO,
			(pinfo->match_uint == pinfo->destport) ? "Request" : "Response");

	if (tree)
	{
		ti = proto_tree_add_item(tree, proto_tns, tvb, 0, -1, ENC_NA);
		tns_tree = proto_item_add_subtree(ti, ett_tns);

		if (pinfo->match_uint == pinfo->destport)
		{
			hidden_item = proto_tree_add_boolean(tns_tree, hf_tns_request,
					   tvb, offset, 0, TRUE);
		}
		else
		{
			hidden_item = proto_tree_add_boolean(tns_tree, hf_tns_response,
					    tvb, offset, 0, TRUE);
		}
		PROTO_ITEM_SET_HIDDEN(hidden_item);
	}

	length = tvb_get_ntohs(tvb, offset);
	if (tree)
	{
		proto_tree_add_uint(tns_tree, hf_tns_length, tvb,
			offset, 2, length);
	}
	offset += 2;

	proto_tree_add_checksum(tns_tree, tvb, offset, hf_tns_packet_checksum, -1, NULL, pinfo, 0, ENC_BIG_ENDIAN, PROTO_CHECKSUM_NO_FLAGS);
	offset += 2;

	type = tvb_get_guint8(tvb, offset);
	if ( tree )
	{
		proto_tree_add_uint(tns_tree, hf_tns_packet_type, tvb,
			offset, 1, type);
	}
	offset += 1;

	col_append_fstr(pinfo->cinfo, COL_INFO, ", %s (%u)",
			val_to_str_const(type, tns_type_vals, "Unknown"), type);

	if ( tree )
	{
		proto_tree_add_item(tns_tree, hf_tns_reserved_byte, tvb,
			offset, 1, ENC_NA);
	}
	offset += 1;

	proto_tree_add_checksum(tns_tree, tvb, offset, hf_tns_header_checksum, -1, NULL, pinfo, 0, ENC_BIG_ENDIAN, PROTO_CHECKSUM_NO_FLAGS);
	offset += 2;

	switch (type)
	{
		case TNS_TYPE_CONNECT:
			dissect_tns_connect(tvb,offset,pinfo,tree,tns_tree);
			break;
		case TNS_TYPE_ACCEPT:
			dissect_tns_accept(tvb,offset,pinfo,tree,tns_tree);
			break;
		case TNS_TYPE_REFUSE:
			dissect_tns_refuse(tvb,offset,pinfo,tree,tns_tree);
			break;
		case TNS_TYPE_REDIRECT:
			dissect_tns_redirect(tvb,offset,pinfo,tree,tns_tree);
			break;
		case TNS_TYPE_ABORT:
			dissect_tns_abort(tvb,offset,pinfo,tree,tns_tree);
			break;
		case TNS_TYPE_MARKER:
			dissect_tns_marker(tvb,offset,pinfo,tree,tns_tree, 0);
			break;
		case TNS_TYPE_ATTENTION:
			dissect_tns_marker(tvb,offset,pinfo,tree,tns_tree, 1);
			break;
		case TNS_TYPE_CONTROL:
			dissect_tns_control(tvb,offset,pinfo,tree,tns_tree);
			break;
		case TNS_TYPE_DATA:
			dissect_tns_data(tvb,offset,pinfo,tree,tns_tree);
			break;
		default:
			call_data_dissector(tvb_new_subset_remaining(tvb, offset), pinfo,
			    tns_tree);
			break;
	}

	return tvb_captured_length(tvb);
}

void proto_register_tns(void)
{
	static hf_register_info hf[] = {
		{ &hf_tns_response, {
			"Response", "tns.response", FT_BOOLEAN, BASE_NONE,
			NULL, 0x0, "TRUE if TNS response", HFILL }},
		{ &hf_tns_request, {
			"Request", "tns.request", FT_BOOLEAN, BASE_NONE,
			NULL, 0x0, "TRUE if TNS request", HFILL }},
		{ &hf_tns_length, {
			"Packet Length", "tns.length", FT_UINT16, BASE_DEC,
			NULL, 0x0, "Length of TNS packet", HFILL }},
		{ &hf_tns_packet_checksum, {
			"Packet Checksum", "tns.packet_checksum", FT_UINT16, BASE_HEX,
			NULL, 0x0, "Checksum of Packet Data", HFILL }},
		{ &hf_tns_header_checksum, {
			"Header Checksum", "tns.header_checksum", FT_UINT16, BASE_HEX,
			NULL, 0x0, "Checksum of Header Data", HFILL }},

		{ &hf_tns_version, {
			"Version", "tns.version", FT_UINT16, BASE_DEC,
			NULL, 0x0, NULL, HFILL }},
		{ &hf_tns_compat_version, {
			"Version (Compatible)", "tns.compat_version", FT_UINT16, BASE_DEC,
			NULL, 0x0, NULL, HFILL }},

		{ &hf_tns_service_options, {
			"Service Options", "tns.service_options", FT_UINT16, BASE_HEX,
			NULL, 0x0, NULL, HFILL }},

		{ &hf_tns_sopt_flag_bconn, {
			"Broken Connect Notify", "tns.so_flag.bconn", FT_BOOLEAN, 16,
			NULL, 0x2000, NULL, HFILL }},
		{ &hf_tns_sopt_flag_pc, {
			"Packet Checksum", "tns.so_flag.pc", FT_BOOLEAN, 16,
			NULL, 0x1000, NULL, HFILL }},
		{ &hf_tns_sopt_flag_hc, {
			"Header Checksum", "tns.so_flag.hc", FT_BOOLEAN, 16,
			NULL, 0x0800, NULL, HFILL }},
		{ &hf_tns_sopt_flag_fd, {
			"Full Duplex", "tns.so_flag.fd", FT_BOOLEAN, 16,
			NULL, 0x0400, NULL, HFILL }},
		{ &hf_tns_sopt_flag_hd, {
			"Half Duplex", "tns.so_flag.hd", FT_BOOLEAN, 16,
			NULL, 0x0200, NULL, HFILL }},
		{ &hf_tns_sopt_flag_dc1, {
			"Don't Care", "tns.so_flag.dc1", FT_BOOLEAN, 16,
			NULL, 0x0100, NULL, HFILL }},
		{ &hf_tns_sopt_flag_dc2, {
			"Don't Care", "tns.so_flag.dc2", FT_BOOLEAN, 16,
			NULL, 0x0080, NULL, HFILL }},
		{ &hf_tns_sopt_flag_dio, {
			"Direct IO to Transport", "tns.so_flag.dio", FT_BOOLEAN, 16,
			NULL, 0x0010, NULL, HFILL }},
		{ &hf_tns_sopt_flag_ap, {
			"Attention Processing", "tns.so_flag.ap", FT_BOOLEAN, 16,
			NULL, 0x0008, NULL, HFILL }},
		{ &hf_tns_sopt_flag_ra, {
			"Can Receive Attention", "tns.so_flag.ra", FT_BOOLEAN, 16,
			NULL, 0x0004, NULL, HFILL }},
		{ &hf_tns_sopt_flag_sa, {
			"Can Send Attention", "tns.so_flag.sa", FT_BOOLEAN, 16,
			NULL, 0x0002, NULL, HFILL }},


		{ &hf_tns_sdu_size, {
			"Session Data Unit Size", "tns.sdu_size", FT_UINT16, BASE_DEC,
			NULL, 0x0, NULL, HFILL }},
		{ &hf_tns_max_tdu_size, {
			"Maximum Transmission Data Unit Size", "tns.max_tdu_size", FT_UINT16, BASE_DEC,
			NULL, 0x0, NULL, HFILL }},

		{ &hf_tns_nt_proto_characteristics, {
			"NT Protocol Characteristics", "tns.nt_proto_characteristics", FT_UINT16, BASE_HEX,
			NULL, 0x0, NULL, HFILL }},
		{ &hf_tns_ntp_flag_hangon, {
			"Hangon to listener connect", "tns.ntp_flag.hangon", FT_BOOLEAN, 16,
			NULL, 0x8000, NULL, HFILL }},
		{ &hf_tns_ntp_flag_crel, {
			"Confirmed release", "tns.ntp_flag.crel", FT_BOOLEAN, 16,
			NULL, 0x4000, NULL, HFILL }},
		{ &hf_tns_ntp_flag_tduio, {
			"TDU based IO", "tns.ntp_flag.tduio", FT_BOOLEAN, 16,
			NULL, 0x2000, NULL, HFILL }},
		{ &hf_tns_ntp_flag_srun, {
			"Spawner running", "tns.ntp_flag.srun", FT_BOOLEAN, 16,
			NULL, 0x1000, NULL, HFILL }},
		{ &hf_tns_ntp_flag_dtest, {
			"Data test", "tns.ntp_flag.dtest", FT_BOOLEAN, 16,
			NULL, 0x0800, NULL, HFILL }},
		{ &hf_tns_ntp_flag_cbio, {
			"Callback IO supported", "tns.ntp_flag.cbio", FT_BOOLEAN, 16,
			NULL, 0x0400, NULL, HFILL }},
		{ &hf_tns_ntp_flag_asio, {
			"ASync IO Supported", "tns.ntp_flag.asio", FT_BOOLEAN, 16,
			NULL, 0x0200, NULL, HFILL }},
		{ &hf_tns_ntp_flag_pio, {
			"Packet oriented IO", "tns.ntp_flag.pio", FT_BOOLEAN, 16,
			NULL, 0x0100, NULL, HFILL }},
		{ &hf_tns_ntp_flag_grant, {
			"Can grant connection to another", "tns.ntp_flag.grant", FT_BOOLEAN, 16,
			NULL, 0x0080, NULL, HFILL }},
		{ &hf_tns_ntp_flag_handoff, {
			"Can handoff connection to another", "tns.ntp_flag.handoff", FT_BOOLEAN, 16,
			NULL, 0x0040, NULL, HFILL }},
		{ &hf_tns_ntp_flag_sigio, {
			"Generate SIGIO signal", "tns.ntp_flag.sigio", FT_BOOLEAN, 16,
			NULL, 0x0020, NULL, HFILL }},
		{ &hf_tns_ntp_flag_sigpipe, {
			"Generate SIGPIPE signal", "tns.ntp_flag.sigpipe", FT_BOOLEAN, 16,
			NULL, 0x0010, NULL, HFILL }},
		{ &hf_tns_ntp_flag_sigurg, {
			"Generate SIGURG signal", "tns.ntp_flag.sigurg", FT_BOOLEAN, 16,
			NULL, 0x0008, NULL, HFILL }},
		{ &hf_tns_ntp_flag_urgentio, {
			"Urgent IO supported", "tns.ntp_flag.urgentio", FT_BOOLEAN, 16,
			NULL, 0x0004, NULL, HFILL }},
		{ &hf_tns_ntp_flag_fdio, {
			"Full duplex IO supported", "tns.ntp_flag.dfio", FT_BOOLEAN, 16,
			NULL, 0x0002, NULL, HFILL }},
		{ &hf_tns_ntp_flag_testop, {
			"Test operation", "tns.ntp_flag.testop", FT_BOOLEAN, 16,
			NULL, 0x0001, NULL, HFILL }},




		{ &hf_tns_line_turnaround, {
			"Line Turnaround Value", "tns.line_turnaround", FT_UINT16, BASE_DEC,
			NULL, 0x0, NULL, HFILL }},
		{ &hf_tns_value_of_one, {
			"Value of 1 in Hardware", "tns.value_of_one", FT_BYTES, BASE_NONE,
			NULL, 0x0, NULL, HFILL }},

		{ &hf_tns_connect, {
			"Connect", "tns.connect", FT_BOOLEAN, BASE_NONE,
			NULL, 0x0, NULL, HFILL }},
		{ &hf_tns_connect_data_length, {
			"Length of Connect Data", "tns.connect_data_length", FT_UINT16, BASE_DEC,
			NULL, 0x0, NULL, HFILL }},
		{ &hf_tns_connect_data_offset, {
			"Offset to Connect Data", "tns.connect_data_offset", FT_UINT16, BASE_DEC,
			NULL, 0x0, NULL, HFILL }},
		{ &hf_tns_connect_data_max, {
			"Maximum Receivable Connect Data", "tns.connect_data_max", FT_UINT32, BASE_DEC,
			NULL, 0x0, NULL, HFILL }},

		{ &hf_tns_connect_flags0, {
			"Connect Flags 0", "tns.connect_flags0", FT_UINT8, BASE_HEX,
			NULL, 0x0, NULL, HFILL }},
		{ &hf_tns_connect_flags1, {
			"Connect Flags 1", "tns.connect_flags1", FT_UINT8, BASE_HEX,
			NULL, 0x0, NULL, HFILL }},

		{ &hf_tns_conn_flag_nareq, {
			"NA services required", "tns.connect_flags.nareq", FT_BOOLEAN, 8,
			NULL, 0x10, NULL, HFILL }},
		{ &hf_tns_conn_flag_nalink, {
			"NA services linked in", "tns.connect_flags.nalink", FT_BOOLEAN, 8,
			NULL, 0x08, NULL, HFILL }},
		{ &hf_tns_conn_flag_enablena, {
			"NA services enabled", "tns.connect_flags.enablena", FT_BOOLEAN, 8,
			NULL, 0x04, NULL, HFILL }},
		{ &hf_tns_conn_flag_ichg, {
			"Interchange is involved", "tns.connect_flags.ichg", FT_BOOLEAN, 8,
			NULL, 0x02, NULL, HFILL }},
		{ &hf_tns_conn_flag_wantna, {
			"NA services wanted", "tns.connect_flags.wantna", FT_BOOLEAN, 8,
			NULL, 0x01, NULL, HFILL }},


		{ &hf_tns_trace_cf1, {
			"Trace Cross Facility Item 1", "tns.trace_cf1", FT_UINT32, BASE_HEX,
			NULL, 0x0, NULL, HFILL }},
		{ &hf_tns_trace_cf2, {
			"Trace Cross Facility Item 2", "tns.trace_cf2", FT_UINT32, BASE_HEX,
			NULL, 0x0, NULL, HFILL }},
		{ &hf_tns_trace_cid, {
			"Trace Unique Connection ID", "tns.trace_cid", FT_UINT64, BASE_HEX,
			NULL, 0x0, NULL, HFILL }},
		{ &hf_tns_connect_data, {
			"Connect Data", "tns.connect_data", FT_STRING, BASE_NONE,
			NULL, 0x0, NULL, HFILL }},

		{ &hf_tns_accept, {
			"Accept", "tns.accept", FT_BOOLEAN, BASE_NONE,
			NULL, 0x0, NULL, HFILL }},
		{ &hf_tns_accept_data_length, {
			"Accept Data Length", "tns.accept_data_length", FT_UINT16, BASE_DEC,
			NULL, 0x0, "Length of Accept Data", HFILL }},
		{ &hf_tns_accept_data, {
			"Accept Data", "tns.accept_data", FT_STRING, BASE_NONE,
			NULL, 0x0, NULL, HFILL }},
		{ &hf_tns_accept_data_offset, {
			"Offset to Accept Data", "tns.accept_data_offset", FT_UINT16, BASE_DEC,
			NULL, 0x0, NULL, HFILL }},


		{ &hf_tns_refuse, {
			"Refuse", "tns.refuse", FT_BOOLEAN, BASE_NONE,
			NULL, 0x0, NULL, HFILL }},
		{ &hf_tns_refuse_reason_user, {
			"Refuse Reason (User)", "tns.refuse_reason_user", FT_UINT8, BASE_HEX,
			NULL, 0x0, "Refuse Reason from Application", HFILL }},
		{ &hf_tns_refuse_reason_system, {
			"Refuse Reason (System)", "tns.refuse_reason_system", FT_UINT8, BASE_HEX,
			NULL, 0x0, "Refuse Reason from System", HFILL }},
		{ &hf_tns_refuse_data_length, {
			"Refuse Data Length", "tns.refuse_data_length", FT_UINT16, BASE_DEC,
			NULL, 0x0, "Length of Refuse Data", HFILL }},
		{ &hf_tns_refuse_data, {
			"Refuse Data", "tns.refuse_data", FT_STRING, BASE_NONE,
			NULL, 0x0, NULL, HFILL }},

		{ &hf_tns_abort, {
			"Abort", "tns.abort", FT_BOOLEAN, BASE_NONE,
			NULL, 0x0, NULL, HFILL }},
		{ &hf_tns_abort_reason_user, {
			"Abort Reason (User)", "tns.abort_reason_user", FT_UINT8, BASE_HEX,
			NULL, 0x0, "Abort Reason from Application", HFILL }},
		{ &hf_tns_abort_reason_system, {
			"Abort Reason (User)", "tns.abort_reason_system", FT_UINT8, BASE_HEX,
			NULL, 0x0, "Abort Reason from System", HFILL }},
		{ &hf_tns_abort_data, {
			"Abort Data", "tns.abort_data", FT_STRING, BASE_NONE,
			NULL, 0x0, NULL, HFILL }},

		{ &hf_tns_marker, {
			"Marker", "tns.marker", FT_BOOLEAN, BASE_NONE,
			NULL, 0x0, NULL, HFILL }},
		{ &hf_tns_marker_type, {
			"Marker Type", "tns.marker.type", FT_UINT8, BASE_HEX,
			VALS(tns_marker_types), 0x0, NULL, HFILL }},
		{ &hf_tns_marker_data_byte, {
			"Marker Data Byte", "tns.marker.databyte", FT_UINT8, BASE_HEX,
			NULL, 0x0, NULL, HFILL }},
#if 0
		{ &hf_tns_marker_data, {
			"Marker Data", "tns.marker.data", FT_UINT16, BASE_HEX,
			NULL, 0x0, NULL, HFILL }},
#endif

		{ &hf_tns_control, {
			"Control", "tns.control", FT_BOOLEAN, BASE_NONE,
			NULL, 0x0, NULL, HFILL }},
		{ &hf_tns_control_cmd, {
			"Control Command", "tns.control.cmd", FT_UINT16, BASE_HEX,
			VALS(tns_control_cmds), 0x0, NULL, HFILL }},
		{ &hf_tns_control_data, {
			"Control Data", "tns.control.data", FT_BYTES, BASE_NONE,
			NULL, 0x0, NULL, HFILL }},

		{ &hf_tns_redirect, {
			"Redirect", "tns.redirect", FT_BOOLEAN, BASE_NONE,
			NULL, 0x0, NULL, HFILL }},
		{ &hf_tns_redirect_data_length, {
			"Redirect Data Length", "tns.redirect_data_length", FT_UINT16, BASE_DEC,
			NULL, 0x0, "Length of Redirect Data", HFILL }},
		{ &hf_tns_redirect_data, {
			"Redirect Data", "tns.redirect_data", FT_STRING, BASE_NONE,
			NULL, 0x0, NULL, HFILL }},

		{ &hf_tns_data, {
			"Data", "tns.data", FT_BOOLEAN, BASE_NONE,
			NULL, 0x0, NULL, HFILL }},

		{ &hf_tns_data_flag, {
			"Data Flag", "tns.data_flag", FT_UINT16, BASE_HEX,
			NULL, 0x0, NULL, HFILL }},
		{ &hf_tns_data_flag_send, {
			"Send Token", "tns.data_flag.send", FT_BOOLEAN, 16,
			NULL, 0x1, NULL, HFILL }},
		{ &hf_tns_data_flag_rc, {
			"Request Confirmation", "tns.data_flag.rc", FT_BOOLEAN, 16,
			NULL, 0x2, NULL, HFILL }},
		{ &hf_tns_data_flag_c, {
			"Confirmation", "tns.data_flag.c", FT_BOOLEAN, 16,
			NULL, 0x4, NULL, HFILL }},
		{ &hf_tns_data_flag_reserved, {
			"Reserved", "tns.data_flag.reserved", FT_BOOLEAN, 16,
			NULL, 0x8, NULL, HFILL }},
		{ &hf_tns_data_flag_more, {
			"More Data to Come", "tns.data_flag.more", FT_BOOLEAN, 16,
			NULL, 0x20, NULL, HFILL }},
		{ &hf_tns_data_flag_eof, {
			"End of File", "tns.data_flag.eof", FT_BOOLEAN, 16,
			NULL, 0x40, NULL, HFILL }},
		{ &hf_tns_data_flag_dic, {
			"Do Immediate Confirmation", "tns.data_flag.dic", FT_BOOLEAN, 16,
			NULL, 0x80, NULL, HFILL }},
		{ &hf_tns_data_flag_rts, {
			"Request To Send", "tns.data_flag.rts", FT_BOOLEAN, 16,
			NULL, 0x100, NULL, HFILL }},
		{ &hf_tns_data_flag_sntt, {
			"Send NT Trailer", "tns.data_flag.sntt", FT_BOOLEAN, 16,
			NULL, 0x200, NULL, HFILL }},


		{ &hf_tns_reserved_byte, {
			"Reserved Byte", "tns.reserved_byte", FT_BYTES, BASE_NONE,
			NULL, 0x0, NULL, HFILL }},
		{ &hf_tns_packet_type, {
			"Packet Type", "tns.type", FT_UINT8, BASE_DEC,
			VALS(tns_type_vals), 0x0, "Type of TNS packet", HFILL }}

	};

	static gint *ett[] = {
		&ett_tns,
		&ett_tns_connect,
		&ett_tns_accept,
		&ett_tns_refuse,
		&ett_tns_abort,
		&ett_tns_redirect,
		&ett_tns_marker,
		&ett_tns_attention,
		&ett_tns_control,
		&ett_tns_data,
		&ett_tns_data_flag,
		&ett_tns_sopt_flag,
		&ett_tns_ntp_flag,
		&ett_tns_conn_flag,
		&ett_sql
	};
	module_t *tns_module;

	proto_tns = proto_register_protocol(
		"Transparent Network Substrate Protocol", "TNS", "tns");
	proto_register_field_array(proto_tns, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));

	tns_module = prefs_register_protocol(proto_tns, NULL);
	prefs_register_bool_preference(tns_module, "desegment_tns_messages",
	  "Reassemble TNS messages spanning multiple TCP segments",
	  "Whether the TNS dissector should reassemble messages spanning multiple TCP segments. "
	  "To use this option, you must also enable \"Allow subdissectors to reassemble TCP streams\" in the TCP protocol settings.",
	  &tns_desegment);
}

void
proto_reg_handoff_tns(void)
{
	dissector_handle_t tns_handle;

	tns_handle = create_dissector_handle(dissect_tns, proto_tns);
	dissector_add_uint("tcp.port", TCP_PORT_TNS, tns_handle);
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
