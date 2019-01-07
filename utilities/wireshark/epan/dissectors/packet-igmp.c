/* packet-igmp.c
 * Routines for IGMP packet disassembly
 * 2001 Ronnie Sahlberg
 * 2007 Thomas Morin
 * <See AUTHORS for emails>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
/*
	IGMP is defined in the following RFCs
	RFC988	Version 0	Obsolete
	RFC1054	Version 1
	RFC1112	Version 1	(same as RFC1054 as far as we are concerned)
	RFC2236	Version 2
	RFC3376	Version 3

	Size in bytes for each packet
	type	RFC988	RFC1054	RFC2236 RFC3376  DVMRP  MRDISC  MSNIP  IGAP  RGMP
	        v0      v1      v2      v3       v1/v3
	0x01      20
	0x02      20
	0x03      20
	0x04      20
	0x05      20
	0x06      20
	0x07      20
	0x08      20
	0x11               8*     8*     >=12
	0x12               8*     8*
	0x13                                     x
	0x16                      8
	0x17                      8
	0x22                            >=8
	0x23                                                    >=8b
	0x24                                            >=8a    8b
	0x25                                            4a      >=8b
	0x26                                            4a
	0x40                                                           ??c
	0x41                                                           ??c
	0x42                                                           ??c
	0xfc                                                                  8
	0xfd                                                                  8
	0xfe                                                                  8
	0xff                                                                  8

   * Differs in second byte of protocol. Always 0 in V1


	Multicast traceroute was taken from
	draft-ietf-idmr-traceroute-ipm-07.txt

	Size in bytes for each packet
	type    draft-ietf-idmr-traceroute-ipm-07.ps
	0x1e      24 + n*32
	0x1f      24 + n*32 (n == 0 for Query)

   x DVMRP Protocol  see packet-dvmrp.c

	DVMRP is defined in the following RFCs
	RFC1075 Version 1
	draft-ietf-idmr-dvmrp-v3-10.txt Version 3

	V1 and V3 can be distinguished by looking at bytes 6 and 7 in the
	IGMP header.
	If header[6]==0xff and header[7]==0x03 we have version 3.

   a MRDISC Protocol  see packet-mrdisc.c

	MRDISC : IGMP Multicast Router DISCovery
	draft-ietf-idmr-igmp-mrdisc-06.txt
	TTL == 1 and IP.DST==224.0.0.2 for all packets

   b MSNIP Protocol  see packet-msnip.c

	MSNIP : Multicast Source Notification of Interest Protocol
	draft-ietf-idmr-msnip-00.txt
	0x23, 0x24 are sent with ip.dst==224.0.0.22
	0x25 is sent as unicast.

   c IGAP Protocol  see packet-igap.c

        IGAP : Internet Group membership Authentication Protocol
	draft-hayashi-igap-03.txt

   d RGMP Protocol  see packet-rgmp.c

	RGMP : Router-port Group Management Protocol
	RFC3488
	TTL == 1 and IP.DST==224.0.0.25 for all packets

*/

#include "config.h"

#include <epan/packet.h>
#include <epan/range.h>
#include <epan/to_str.h>
#include <epan/ipproto.h>
#include <epan/in_cksum.h>
#include <wsutil/str_util.h>
#include "packet-igmp.h"

void proto_register_igmp(void);
void proto_reg_handoff_igmp(void);

static int proto_igmp = -1;
static int hf_type = -1;
static int hf_reserved = -1;
static int hf_version = -1;
static int hf_group_type = -1;
static int hf_reply_code = -1;
static int hf_reply_pending = -1;
static int hf_checksum = -1;
static int hf_checksum_status = -1;
static int hf_identifier = -1;
static int hf_access_key = -1;
static int hf_max_resp = -1;
static int hf_max_resp_exp = -1;
static int hf_max_resp_mant = -1;
static int hf_suppress = -1;
static int hf_qrv = -1;
static int hf_qqic = -1;
static int hf_num_src = -1;
static int hf_saddr = -1;
static int hf_num_grp_recs = -1;
static int hf_record_type = -1;
static int hf_aux_data_len = -1;
static int hf_maddr = -1;
static int hf_aux_data = -1;
static int hf_data = -1;
static int hf_mtrace_max_hops = -1;
static int hf_mtrace_saddr = -1;
static int hf_mtrace_raddr = -1;
static int hf_mtrace_rspaddr = -1;
static int hf_mtrace_resp_ttl = -1;
static int hf_mtrace_q_id = -1;
static int hf_mtrace_q_arrival = -1;
static int hf_mtrace_q_inaddr = -1;
static int hf_mtrace_q_outaddr = -1;
static int hf_mtrace_q_prevrtr = -1;
static int hf_mtrace_q_inpkt = -1;
static int hf_mtrace_q_outpkt = -1;
static int hf_mtrace_q_total = -1;
static int hf_mtrace_q_rtg_proto = -1;
static int hf_mtrace_q_fwd_ttl = -1;
static int hf_mtrace_q_mbz = -1;
static int hf_mtrace_q_s = -1;
static int hf_mtrace_q_src_mask = -1;
static int hf_mtrace_q_fwd_code = -1;

static int ett_igmp = -1;
static int ett_group_record = -1;
static int ett_max_resp = -1;
static int ett_mtrace_block = -1;

static dissector_table_t   subdissector_table;

#define IGMP_TRACEROUTE_HDR_LEN           24
#define IGMP_TRACEROUTE_RSP_LEN           32

static const value_string commands[] = {
	{IGMP_V0_CREATE_GROUP_REQUEST,	"Create Group Request"		},
	{IGMP_V0_CREATE_GROUP_REPLY,	"Create Group Reply"		},
	{IGMP_V0_JOIN_GROUP_REQUEST,	"Join Group Request"		},
	{IGMP_V0_JOIN_GROUP_REPLY,	"Join Group Reply"		},
	{IGMP_V0_LEAVE_GROUP_REQUEST,	"Leave Group Request"		},
	{IGMP_V0_LEAVE_GROUP_REPLY,	"Leave Group Reply"		},
	{IGMP_V0_CONFIRM_GROUP_REQUEST,	"Confirm Group Request"		},
	{IGMP_V0_CONFIRM_GROUP_REPLY,	"Confirm Group Reply"		},
	{IGMP_V1_HOST_MEMBERSHIP_QUERY,	"Membership Query"		},
	{IGMP_V1_HOST_MEMBERSHIP_REPORT,"Membership Report"		},
	{IGMP_DVMRP,			"DVMRP Protocol"		},
	{IGMP_V1_PIM_ROUTING_MESSAGE,	"PIM Routing Message"		},
	{IGMP_V2_MEMBERSHIP_REPORT,	"Membership Report"		},
	{IGMP_V2_LEAVE_GROUP,		"Leave Group"			},
	{IGMP_TRACEROUTE_RESPONSE,	"Traceroute Response"		},
	{IGMP_TRACEROUTE_QUERY_REQ,	"Traceroute Query or Request"	},
	{IGMP_V3_MEMBERSHIP_REPORT,	"Membership Report"		},
	{0,		NULL}
};

#define IGMP_V3_S		0x08
#define IGMP_V3_QRV_MASK	0x07

#define IGMP_MAX_RESP_EXP	0x70
#define IGMP_MAX_RESP_MANT	0x0f

#define IGMP_V0_GROUP_PUBLIC	0x00
#define IGMP_V0_GROUP_PRIVATE	0x01

static const value_string vs_group_type[] = {
	{IGMP_V0_GROUP_PUBLIC,		"Public Group"			},
	{IGMP_V0_GROUP_PRIVATE,		"Private Group"			},
	{0,		NULL}
};

#define IGMP_V0_REPLY_GRANTED	0x00
#define IGMP_V0_REPLY_NO_RESOURCES	0x01
#define IGMP_V0_REPLY_INVALID_CODE	0x02
#define IGMP_V0_REPLY_INVALID_GROUP	0x03
#define IGMP_V0_REPLY_INVALID_KEY	0x04

static const value_string vs_reply_code[] = {
	{IGMP_V0_REPLY_GRANTED,	"Request Granted"	},
	{IGMP_V0_REPLY_NO_RESOURCES,	"Request Denied, No Resources"	},
	{IGMP_V0_REPLY_INVALID_CODE,	"Request Denied, Invalid Code"	},
	{IGMP_V0_REPLY_INVALID_GROUP,	"Request Denied, Invalid Group"	},
	{IGMP_V0_REPLY_INVALID_KEY,	"Request Denied, Invalid Key"	},
	{0,		NULL}
};

static const true_false_string tfs_s = {
	"SUPPRESS router side processing",
	"Do not suppress router side processing"
};

#define IGMP_V3_MODE_IS_INCLUDE		1
#define IGMP_V3_MODE_IS_EXCLUDE		2
#define IGMP_V3_CHANGE_TO_INCLUDE_MODE	3
#define IGMP_V3_CHANGE_TO_EXCLUDE_MODE	4
#define IGMP_V3_ALLOW_NEW_SOURCES	5
#define IGMP_V3_BLOCK_OLD_SOURCES	6

static const value_string vs_record_type[] = {
	{IGMP_V3_MODE_IS_INCLUDE,	"Mode Is Include"		},
	{IGMP_V3_MODE_IS_EXCLUDE,	"Mode Is Exclude"		},
	{IGMP_V3_CHANGE_TO_INCLUDE_MODE,"Change To Include Mode"	},
	{IGMP_V3_CHANGE_TO_EXCLUDE_MODE,"Change To Exclude Mode"	},
	{IGMP_V3_ALLOW_NEW_SOURCES,	"Allow New Sources"		},
	{IGMP_V3_BLOCK_OLD_SOURCES,	"Block Old Sources"		},
	{ 0,	NULL}
};

static const value_string mtrace_rtg_vals[] = {
	{1,  "DVMRP"                                        },
	{2,  "MOSPF"                                        },
	{3,  "PIM"                                          },
	{4,  "CBT"                                          },
	{5,  "PIM using special routing table"              },
	{6,  "PIM using a static route"                     },
	{7,  "DVMRP using a static route"                   },
	{8,  "PIM using MBGP (aka BGP4+) route"             },
	{9,  "CBT using special routing table"              },
	{10, "CBT using a static route"                     },
	{11, "PIM using state created by Assert processing" },
	{0,  NULL}
};

static const value_string mtrace_fwd_code_vals[] = {
	{0x00, "NO_ERROR"       },
	{0x01, "WRONG_IF"       },
	{0x02, "PRUNE_SENT"     },
	{0x03, "PRUNE_RCVD"     },
	{0x04, "SCOPED"         },
	{0x05, "NO_ROUTE"       },
	{0x06, "WRONG_LAST_HOP" },
	{0x07, "NOT_FORWARDING" },
	{0x08, "REACHED_RP"     },
	{0x09, "RPF_IF"         },
	{0x0A, "NO_MULTICAST"   },
	{0x0B, "INFO_HIDDEN"    },
	{0x81, "NO_SPACE"       },
	{0x82, "OLD_ROUTER"     },
	{0x83, "ADMIN_PROHIB"   },
	{0, NULL}
};

void igmp_checksum(proto_tree *tree, tvbuff_t *tvb, int hf_index,
	int hf_index_status, packet_info *pinfo, guint len)
{
	vec_t cksum_vec[1];

	if (len == 0) {
		/*
		 * Checksum the entire IGMP packet.
		 */
		len = tvb_reported_length(tvb);
	}

	if (!pinfo->fragmented && tvb_captured_length(tvb) >= len) {
		/*
		 * The packet isn't part of a fragmented datagram and isn't
		 * truncated, so we can checksum it.
		 */
		SET_CKSUM_VEC_TVB(cksum_vec[0], tvb, 0, len);
		proto_tree_add_checksum(tree, tvb, 2, hf_index, hf_index_status, NULL, pinfo, in_cksum(&cksum_vec[0], 1),
                                ENC_BIG_ENDIAN, PROTO_CHECKSUM_VERIFY|PROTO_CHECKSUM_IN_CKSUM);
	} else
		proto_tree_add_checksum(tree, tvb, 2, hf_index, -1, NULL, pinfo, 0, ENC_BIG_ENDIAN, PROTO_CHECKSUM_NO_FLAGS);

	return;
}

static proto_tree*
dissect_igmp_common(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int* offset, unsigned char* type, int version)
{
	proto_item* ti;
	proto_tree* igmp_tree;

	col_add_fstr(pinfo->cinfo, COL_PROTOCOL, "IGMPv%d", version);
	col_clear(pinfo->cinfo, COL_INFO);

	ti = proto_tree_add_item(tree, proto_igmp, tvb, 0, -1, ENC_NA);
	igmp_tree = proto_item_add_subtree(ti, ett_igmp);

	*type = tvb_get_guint8(tvb, 0);
	col_add_fstr(pinfo->cinfo, COL_INFO, "%s", val_to_str(*type, commands, "Unknown Type:0x%02x"));

	/* version of IGMP protocol */
	ti = proto_tree_add_uint(igmp_tree, hf_version, tvb, 0, 0, version);
	PROTO_ITEM_SET_GENERATED(ti);

	/* type of command */
	proto_tree_add_item(igmp_tree, hf_type, tvb, 0, 1, ENC_BIG_ENDIAN);
	*offset = 1;

	return igmp_tree;
}


/* Unknown IGMP message type */
static int
dissect_igmp_unknown(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree)
{
	proto_item* ti;
	proto_tree* tree;
	int len;
	int offset = 0;
	unsigned char type;

	col_set_str(pinfo->cinfo, COL_PROTOCOL, "IGMP");
	col_clear(pinfo->cinfo, COL_INFO);

	ti = proto_tree_add_item(parent_tree, proto_igmp, tvb, offset, -1, ENC_NA);
	tree = proto_item_add_subtree(ti, ett_igmp);

	type = tvb_get_guint8(tvb, offset);
	col_add_str(pinfo->cinfo, COL_INFO,
		val_to_str(type, commands, "Unknown Type:0x%02x"));

	/* type of command */
	proto_tree_add_uint(tree, hf_type, tvb, offset, 1, type);
	offset += 1;

	/* Just call the rest of it "data" */
	len = tvb_captured_length_remaining(tvb, offset);
	proto_tree_add_item(tree, hf_data, tvb, offset, -1, ENC_NA);
	offset += len;

	return offset;
}



/*************************************************************
 * IGMP Protocol dissectors
 *************************************************************/
static int
dissect_v3_max_resp(tvbuff_t *tvb, proto_tree *parent_tree, int offset)
{
	proto_tree *tree;
	proto_item *item;
	guint8 bits;
	guint32 tsecs;

	bits = tvb_get_guint8(tvb, offset);
	if (bits&0x80) {
		tsecs = ((bits&IGMP_MAX_RESP_MANT)|0x10);
		tsecs = tsecs << ( ((bits&IGMP_MAX_RESP_EXP)>>4) + 3);
	} else {
		tsecs = bits;
	}

	item = proto_tree_add_uint_format_value(parent_tree, hf_max_resp, tvb,
			offset, 1, tsecs, "%.1f sec (0x%02x)",tsecs*0.1,bits);

	if (bits&0x80) {
		tree = proto_item_add_subtree(item, ett_max_resp);

		proto_tree_add_uint(tree, hf_max_resp_exp, tvb, offset, 1,
			bits);
		proto_tree_add_uint(tree, hf_max_resp_mant, tvb, offset, 1,
			bits);
	}

	offset += 1;

	return offset;
}

static int
dissect_v3_sqrv_bits(tvbuff_t *tvb, proto_tree *parent_tree, int offset)
{
    static const int * bits[] = {
        &hf_suppress,
        &hf_qrv,
        NULL
    };

    proto_tree_add_bitmask_list(parent_tree, tvb, offset, 1, bits, ENC_NA);
	offset += 1;

	return offset;
}

static int
dissect_v3_group_record(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree, int offset)
{
	proto_tree *tree;
	proto_item *item;
	int old_offset = offset;
	guint8	adl;
	guint16 num;
	const gchar *maddr_str;
	guint8 record_type;

	tree = proto_tree_add_subtree_format(parent_tree, tvb, offset, -1,
			ett_group_record, &item, "Group Record : %s  %s",
			tvb_ip_to_str(tvb, offset+4),
			val_to_str_const(tvb_get_guint8(tvb, offset), vs_record_type,"")
		);

	/* record type */
	record_type = tvb_get_guint8(tvb, offset);
	proto_tree_add_item(tree, hf_record_type, tvb, offset, 1, ENC_BIG_ENDIAN);
	offset += 1;

	/* aux data len */
	adl = tvb_get_guint8(tvb, offset);
	proto_tree_add_uint(tree, hf_aux_data_len, tvb, offset, 1, adl);
	offset += 1;

	/*number of sources*/
	num = tvb_get_ntohs(tvb, offset);
	proto_tree_add_uint(tree, hf_num_src, tvb, offset, 2, num);
	offset += 2;

	/* multicast address */
	proto_tree_add_item(tree, hf_maddr, tvb, offset, 4, ENC_BIG_ENDIAN);
	maddr_str = tvb_ip_to_str(tvb, offset);
	offset += 4;

	if (num == 0) {
		switch(record_type) {
		case IGMP_V3_MODE_IS_INCLUDE:
		case IGMP_V3_CHANGE_TO_INCLUDE_MODE:
			col_append_fstr(pinfo->cinfo, COL_INFO, " / Leave group %s", maddr_str);
			break;
		case IGMP_V3_MODE_IS_EXCLUDE:
		case IGMP_V3_CHANGE_TO_EXCLUDE_MODE:
			col_append_fstr(pinfo->cinfo, COL_INFO,
				" / Join group %s for any sources", maddr_str);
			break;
		case IGMP_V3_ALLOW_NEW_SOURCES:
			col_append_fstr(pinfo->cinfo, COL_INFO,
				" / Group %s, ALLOW_NEW_SOURCES but no source specified (?)",
				maddr_str);
			break;
		case IGMP_V3_BLOCK_OLD_SOURCES:
			col_append_fstr(pinfo->cinfo, COL_INFO,
				" / Group %s, BLOCK_OLD_SOURCES but no source specified (?)",
				maddr_str);
			break;
		default:
			col_append_fstr(pinfo->cinfo, COL_INFO,
				" / Group %s, unknown record type (?)", maddr_str);
				break;
		}
	} else {
		switch(record_type) {
		case IGMP_V3_MODE_IS_INCLUDE:
		case IGMP_V3_CHANGE_TO_INCLUDE_MODE:
			col_append_fstr(pinfo->cinfo, COL_INFO,
				" / Join group %s for source%s {",
				maddr_str, (num>1) ? "s in" : "");
			break;
		case IGMP_V3_MODE_IS_EXCLUDE:
		case IGMP_V3_CHANGE_TO_EXCLUDE_MODE:
			col_append_fstr(pinfo->cinfo, COL_INFO,
				" / Join group %s, for source%s {",
				maddr_str, (num>1) ? "s not in" : " not");
			break;
		case IGMP_V3_ALLOW_NEW_SOURCES:
			col_append_fstr(pinfo->cinfo, COL_INFO,
				" / Group %s, new source%s {",
				maddr_str, (num>1) ? "s" : "");
			break;
		case IGMP_V3_BLOCK_OLD_SOURCES:
			col_append_fstr(pinfo->cinfo, COL_INFO,
				" / Group %s, block source%s {",
				maddr_str, (num>1) ? "s" : "");
			break;
		default:
			col_append_fstr(pinfo->cinfo, COL_INFO,
				" / Group %s, unknown record type (?), sources {",
				maddr_str);
			break;
		}
	}

	/* source addresses */
	while(num--){
		col_append_fstr(pinfo->cinfo, COL_INFO, "%s%s",
				tvb_ip_to_str(tvb, offset), (num?", ":"}"));

		proto_tree_add_item(tree, hf_saddr, tvb, offset, 4, ENC_BIG_ENDIAN);
		offset += 4;
	}

	/* aux data */
	if(adl){
		proto_tree_add_item(tree, hf_aux_data, tvb, offset, adl*4, ENC_NA);
		offset += adl*4;
	}

	proto_item_set_len(item, offset-old_offset);
	return offset;
}

/* dissectors for version 3, rfc3376 */
static int
dissect_igmp_v3_report(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree, void* data _U_)
{
	proto_tree* tree;
	guint16 num;
	int offset;
	unsigned char type;

	tree = dissect_igmp_common(tvb, pinfo, parent_tree, &offset, &type, 3);

        proto_tree_add_item(tree, hf_reserved, tvb, offset, 1, ENC_NA);
	offset += 1;

	/* checksum */
	igmp_checksum(tree, tvb, hf_checksum, hf_checksum_status, pinfo, 0);
	offset += 2;

        proto_tree_add_item(tree, hf_reserved, tvb, offset, 2, ENC_NA);
	offset += 2;

	/* number of group records */
	num = tvb_get_ntohs(tvb, offset);
	if (!num)
		col_append_str(pinfo->cinfo, COL_INFO, " - General query");

	proto_tree_add_uint(tree, hf_num_grp_recs, tvb, offset, 2, num);
	offset += 2;

	while (num--)
		offset = dissect_v3_group_record(tvb, pinfo, tree, offset);

	return offset;
}

static int
dissect_igmp_v3_query(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree, void* data _U_)
{
	proto_tree* tree;
	guint16 num;
	int offset;
	unsigned char type;

	tree = dissect_igmp_common(tvb, pinfo, parent_tree, &offset, &type, 3);

	num = tvb_get_ntohs(tvb, offset+9);
	/* max resp code */
	offset = dissect_v3_max_resp(tvb, tree, offset);

	/* checksum */
	igmp_checksum(tree, tvb, hf_checksum, hf_checksum_status, pinfo, 0);
	offset += 2;

	/* group address */
	proto_tree_add_item(tree, hf_maddr, tvb, offset, 4, ENC_BIG_ENDIAN);

	if (!tvb_get_ipv4(tvb, offset)) {
		col_append_str(pinfo->cinfo, COL_INFO, ", general");
	} else {
		col_append_fstr(pinfo->cinfo, COL_INFO, ", specific for group %s",
			tvb_ip_to_str(tvb, offset));
	}
	offset +=4;

	/* bitmask for S and QRV */
	offset = dissect_v3_sqrv_bits(tvb, tree, offset);

	/* qqic */
	proto_tree_add_item(tree, hf_qqic, tvb, offset, 1, ENC_BIG_ENDIAN);
	offset += 1;

	/*number of sources*/
	proto_tree_add_uint(tree, hf_num_src, tvb, offset, 2, num);
	if (num) {
		col_append_fstr(pinfo->cinfo, COL_INFO, ", source%s {", (num>1)?"s":"");
	}
	offset += 2;

	while(num--){
		col_append_fstr(pinfo->cinfo, COL_INFO, "%s%s", tvb_ip_to_str(tvb, offset), (num?", ":"}"));
		proto_tree_add_item(tree, hf_saddr, tvb, offset, 4, ENC_BIG_ENDIAN);
		offset += 4;
	}

	return offset;
}

/* dissector for version 2 query and report, rfc2236 */
static int
dissect_igmp_v2(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree, void* data _U_)
{
	proto_tree* tree;
	guint8 tsecs;
	int offset;
	unsigned char type;

	tree = dissect_igmp_common(tvb, pinfo, parent_tree, &offset, &type, 2);

	/* max resp time */
	tsecs = tvb_get_guint8(tvb, offset);
	proto_tree_add_uint_format_value(tree, hf_max_resp, tvb,
		offset, 1, tsecs, "%.1f sec (0x%02x)", tsecs*0.1,tsecs);
	offset += 1;

	/* checksum */
	igmp_checksum(tree, tvb, hf_checksum, hf_checksum_status, pinfo, 8);
	offset += 2;

	/* group address */
	proto_tree_add_item(tree, hf_maddr, tvb, offset, 4, ENC_BIG_ENDIAN);

	if (! tvb_get_ipv4(tvb, offset)) {
		col_append_str(pinfo->cinfo, COL_INFO, ", general");
	} else {
		switch(type)
		{
		case IGMP_V2_LEAVE_GROUP:
			col_append_fstr(pinfo->cinfo, COL_INFO, " %s", tvb_ip_to_str(tvb, offset));
			break;
		case IGMP_V1_HOST_MEMBERSHIP_QUERY:
			col_append_fstr(pinfo->cinfo, COL_INFO, ", specific for group %s", tvb_ip_to_str(tvb, offset));
			break;
		default: /* IGMP_V2_MEMBERSHIP_REPORT is the only case left */
			col_append_fstr(pinfo->cinfo, COL_INFO, " group %s", tvb_ip_to_str(tvb, offset));
			break;
		}
	}
	offset +=4;

	return offset;
}

/* dissector for version 1 query and report, rfc1054 */
static int
dissect_igmp_v1(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree, void* data _U_)
{
	proto_tree* tree;
	int offset;
	unsigned char type;

	tree = dissect_igmp_common(tvb, pinfo, parent_tree, &offset, &type, 1);

	/* skip unused byte */
	proto_tree_add_item(tree, hf_reserved, tvb, offset, 1, ENC_NA);
	offset += 1;

	/* checksum */
	igmp_checksum(tree, tvb, hf_checksum, hf_checksum_status, pinfo, 8);
	offset += 2;

	/* group address */
	proto_tree_add_item(tree, hf_maddr, tvb, offset, 4, ENC_BIG_ENDIAN);
	offset +=4;

	return offset;
}

/* dissector for version 0, rfc988 */
static int
dissect_igmp_v0(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree, void* data _U_)
{
	proto_tree* tree;
	unsigned char code;
	int offset;
	unsigned char type;

	tree = dissect_igmp_common(tvb, pinfo, parent_tree, &offset, &type, 0);

	/* Code */
	code = tvb_get_guint8(tvb, offset);
	if (type==IGMP_V0_CREATE_GROUP_REQUEST) {
		proto_tree_add_uint(tree, hf_group_type, tvb, offset, 1, code);
	} else if (!(type&0x01)) {
		if (code <5) {
			proto_tree_add_uint(tree, hf_reply_code, tvb, offset, 1, code);
		} else {
			proto_tree_add_uint(tree, hf_reply_pending, tvb, offset, 1, code);
		}
	}
	offset += 1;

	/* checksum */
	igmp_checksum(tree, tvb, hf_checksum, hf_checksum_status, pinfo, 20);
	offset += 2;

	/* identifier */
	proto_tree_add_item(tree, hf_identifier, tvb, offset, 4, ENC_BIG_ENDIAN);
	offset += 4;

	/* group address */
	proto_tree_add_item(tree, hf_maddr, tvb, offset, 4, ENC_BIG_ENDIAN);
	offset += 4;

	/* access key */
	proto_tree_add_item(tree, hf_access_key, tvb, offset, 8, ENC_NA);
	offset += 8;

	return offset;
}

static int
dissect_igmp_mquery(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree, void* data)
{
	if ( tvb_reported_length(tvb)>=12 ) {
			/* version 3 */
		return dissect_igmp_v3_query(tvb, pinfo, parent_tree, data);
	}

	/* v1 and v2 differs in second byte of header */
	if (tvb_get_guint8(tvb, 1)) {
		return dissect_igmp_v2(tvb, pinfo, parent_tree, data);
	}

	return dissect_igmp_v1(tvb, pinfo, parent_tree, data);
}

/* dissector for multicast traceroute, rfc???? */
static int
dissect_igmp_mtrace(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree, void* data _U_)
{
	proto_tree* tree;
	proto_item* ti;
	int offset = 0;
	unsigned char type;
	const char *typestr, *blocks = NULL;
	char buf[20];

	ti = proto_tree_add_item(parent_tree, proto_igmp, tvb, offset, -1, ENC_NA);
	tree = proto_item_add_subtree(ti, ett_igmp);

	col_set_str(pinfo->cinfo, COL_PROTOCOL, "IGMP");
	col_clear(pinfo->cinfo, COL_INFO);

	/* All multicast traceroute packets (Query, Request and
	 * Response) have the same fixed header. Request and Response
	 * have one or more response data blocks following this fixed
	 * header. Since Query and Request share the same IGMP type,
	 * the method to differentiate between them is to check the
	 * IGMP packet length. Queries are only
	 * IGMP_TRACEROUTE_HDR_LEN bytes long.
	 */
	type = tvb_get_guint8(tvb, offset);
	if (type == IGMP_TRACEROUTE_RESPONSE) {
		int i = (tvb_reported_length_remaining(tvb, offset) - IGMP_TRACEROUTE_HDR_LEN) / IGMP_TRACEROUTE_RSP_LEN;
		g_snprintf(buf, sizeof buf, ", %d block%s", i, plurality(i, "", "s"));
		typestr = "Traceroute Response";
		blocks = buf;
	} else if (tvb_reported_length_remaining(tvb, offset) == IGMP_TRACEROUTE_HDR_LEN)
		typestr = "Traceroute Query";
	else
		typestr = "Traceroute Request";

	col_set_str(pinfo->cinfo, COL_INFO, typestr);
	if (blocks)
		col_append_str(pinfo->cinfo, COL_INFO, blocks);

	proto_tree_add_uint_format_value(tree, hf_type, tvb, offset, 1, type,
		"%s (0x%02x)", typestr, type);
	offset += 1;

	/* maximum number of hops that the requester wants to trace */
	proto_tree_add_item(tree, hf_mtrace_max_hops, tvb, offset, 1, ENC_BIG_ENDIAN);
	offset += 1;

	/* checksum */
	igmp_checksum(tree, tvb, hf_checksum, hf_checksum_status, pinfo, 0);
	offset += 2;

	/* group address to be traced */
	proto_tree_add_item(tree, hf_maddr, tvb, offset, 4, ENC_BIG_ENDIAN);
	offset += 4;

	/* address of multicast source for the path being traced */
	proto_tree_add_item(tree, hf_mtrace_saddr, tvb, offset, 4, ENC_BIG_ENDIAN);
	offset += 4;

	/* address of multicast receiver for the path being traced */
	proto_tree_add_item(tree, hf_mtrace_raddr, tvb, offset, 4, ENC_BIG_ENDIAN);
	offset += 4;

	/* address where the completed traceroute response packet gets sent */
	proto_tree_add_item(tree, hf_mtrace_rspaddr, tvb, offset, 4, ENC_BIG_ENDIAN);
	offset += 4;

	/* for multicasted responses, TTL at which to multicast the response */
	proto_tree_add_item(tree, hf_mtrace_resp_ttl, tvb, offset, 1, ENC_BIG_ENDIAN);
	offset += 1;

	/* unique identifier for this traceroute request (for e.g. duplicate/delay detection) */
	proto_tree_add_item(tree, hf_mtrace_q_id, tvb, offset, 3, ENC_BIG_ENDIAN);
	offset += 3;

	/* If this was Query, we only had the fixed header */
	if (tvb_reported_length_remaining(tvb, offset) == 0)
		return offset;

	/* Loop through the response data blocks */
	while (tvb_reported_length_remaining(tvb, offset) >= IGMP_TRACEROUTE_RSP_LEN) {
		proto_tree *block_tree;

		block_tree = proto_tree_add_subtree_format(tree, tvb, offset, IGMP_TRACEROUTE_RSP_LEN,
			ett_mtrace_block, NULL, "Response data block: %s -> %s,  Proto: %s,  Forwarding Code: %s",
			tvb_ip_to_str(tvb, offset + 4),
			tvb_ip_to_str(tvb, offset + 8),
			val_to_str_const(tvb_get_guint8(tvb, offset + 28), mtrace_rtg_vals, "Unknown"),
			val_to_str_const(tvb_get_guint8(tvb, offset + 31), mtrace_fwd_code_vals, "Unknown"));

		/* Query Arrival Time */
		proto_tree_add_item(block_tree, hf_mtrace_q_arrival, tvb, offset, 4, ENC_BIG_ENDIAN);
		offset += 4;

		/* Incoming Interface Address */
		proto_tree_add_item(block_tree, hf_mtrace_q_inaddr, tvb, offset, 4, ENC_BIG_ENDIAN);
		offset += 4;

		/* Outgoing Interface Address */
		proto_tree_add_item(block_tree, hf_mtrace_q_outaddr, tvb, offset, 4, ENC_BIG_ENDIAN);
		offset += 4;

		/* Previous-Hop Router Address */
		proto_tree_add_item(block_tree, hf_mtrace_q_prevrtr, tvb, offset, 4, ENC_BIG_ENDIAN);
		offset += 4;

		/* Input packet count on incoming interface */
		proto_tree_add_item(block_tree, hf_mtrace_q_inpkt, tvb, offset, 4, ENC_BIG_ENDIAN);
		offset += 4;

		/* Output packet count on outgoing interface */
		proto_tree_add_item(block_tree, hf_mtrace_q_outpkt, tvb, offset, 4, ENC_BIG_ENDIAN);
		offset += 4;

		/* Total number of packets for this source-group pair */
		proto_tree_add_item(block_tree, hf_mtrace_q_total, tvb, offset, 4, ENC_BIG_ENDIAN);
		offset += 4;

		/* Routing protocol in use between this and previous-hop router */
		proto_tree_add_item(block_tree, hf_mtrace_q_rtg_proto, tvb, offset, 1, ENC_BIG_ENDIAN);
		offset += 1;

		/* TTL that a packet is required to be forwarded */
		proto_tree_add_item(block_tree, hf_mtrace_q_fwd_ttl, tvb, offset, 1, ENC_BIG_ENDIAN);
		offset += 1;

		/* Must be zeroed and ignored bit, S bit and src network mask length */
		proto_tree_add_item(block_tree, hf_mtrace_q_mbz, tvb, offset, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(block_tree, hf_mtrace_q_s, tvb, offset, 1, ENC_BIG_ENDIAN);
		proto_tree_add_item(block_tree, hf_mtrace_q_src_mask, tvb, offset, 1, ENC_BIG_ENDIAN);
		offset += 1;

		/* Forwarding information/error code */
		proto_tree_add_item(block_tree, hf_mtrace_q_fwd_code, tvb, offset, 1, ENC_BIG_ENDIAN);
		offset += 1;
	}

	return offset;
}

static int
dissect_igmp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree, void* data _U_)
{
	int offset = 0;
	unsigned char type;

	type = tvb_get_guint8(tvb, offset);

	if (!dissector_try_uint(subdissector_table, type, tvb, pinfo, parent_tree))
	{
		dissect_igmp_unknown(tvb, pinfo, parent_tree);
	}
	return tvb_captured_length(tvb);
}

void
proto_register_igmp(void)
{
	static hf_register_info hf[] = {
		{ &hf_type,
			{ "Type", "igmp.type", FT_UINT8, BASE_HEX,
			  VALS(commands), 0, "IGMP Packet Type", HFILL }},

		{ &hf_reserved,
			{ "Reserved", "igmp.reserved", FT_BYTES, BASE_NONE,
			  NULL, 0, "IGMP Reserved", HFILL }},

		{ &hf_version,
			{ "IGMP Version", "igmp.version", FT_UINT8, BASE_DEC,
			  NULL, 0, NULL, HFILL }},

		{ &hf_group_type,
			{ "Type Of Group", "igmp.group_type", FT_UINT8, BASE_DEC,
			  VALS(vs_group_type), 0, "IGMP V0 Type Of Group", HFILL }},

		{ &hf_reply_code,
			{ "Reply", "igmp.reply", FT_UINT8, BASE_DEC,
			  VALS(vs_reply_code), 0, "IGMP V0 Reply", HFILL }},

		{ &hf_reply_pending,
			{ "Reply Pending", "igmp.reply.pending", FT_UINT8, BASE_DEC,
			  NULL, 0, "IGMP V0 Reply Pending, Retry in this many seconds", HFILL }},

		{ &hf_checksum,
			{ "Checksum", "igmp.checksum", FT_UINT16, BASE_HEX,
			  NULL, 0, "IGMP Checksum", HFILL }},

		{ &hf_checksum_status,
			{ "Checksum Status", "igmp.checksum.status", FT_UINT8, BASE_NONE,
			  VALS(proto_checksum_vals), 0x0, NULL, HFILL }},

		{ &hf_identifier,
			{ "Identifier", "igmp.identifier", FT_UINT32, BASE_DEC,
			  NULL, 0, "IGMP V0 Identifier", HFILL }},

		{ &hf_access_key,
			{ "Access Key", "igmp.access_key", FT_BYTES, BASE_NONE,
			  NULL, 0, "IGMP V0 Access Key", HFILL }},

		{ &hf_max_resp,
			{ "Max Resp Time", "igmp.max_resp", FT_UINT8, BASE_DEC,
			  NULL, 0, "Max Response Time", HFILL }},

		{ &hf_suppress,
			{ "S", "igmp.s", FT_BOOLEAN, 8,
			  TFS(&tfs_s), IGMP_V3_S, "Suppress Router Side Processing", HFILL }},

		{ &hf_qrv,
			{ "QRV", "igmp.qrv", FT_UINT8, BASE_DEC,
			NULL, IGMP_V3_QRV_MASK, "Querier's Robustness Value", HFILL }},

		{ &hf_qqic,
			{ "QQIC", "igmp.qqic", FT_UINT8, BASE_DEC,
			  NULL, 0, "Querier's Query Interval Code", HFILL }},

		{ &hf_num_src,
			{ "Num Src", "igmp.num_src", FT_UINT16, BASE_DEC,
			  NULL, 0, "Number Of Sources", HFILL }},

		{ &hf_saddr,
			{ "Source Address", "igmp.saddr", FT_IPv4, BASE_NONE,
			  NULL, 0, NULL, HFILL }},

		{ &hf_num_grp_recs,
			{ "Num Group Records", "igmp.num_grp_recs", FT_UINT16, BASE_DEC,
			  NULL, 0, "Number Of Group Records", HFILL }},

		{ &hf_record_type,
			{ "Record Type", "igmp.record_type", FT_UINT8, BASE_DEC,
			VALS(vs_record_type), 0, NULL, HFILL }},

		{ &hf_aux_data_len,
			{ "Aux Data Len", "igmp.aux_data_len", FT_UINT8, BASE_DEC,
			NULL, 0, "Aux Data Len, In units of 32bit words", HFILL }},

		{ &hf_maddr,
			{ "Multicast Address", "igmp.maddr", FT_IPv4, BASE_NONE,
			  NULL, 0, NULL, HFILL }},

		{ &hf_aux_data,
			{ "Aux Data", "igmp.aux_data", FT_BYTES, BASE_NONE,
			  NULL, 0, "IGMP V3 Auxiliary Data", HFILL }},

		{ &hf_data,
			{ "Data", "igmp.data", FT_BYTES, BASE_NONE,
			  NULL, 0, NULL, HFILL }},

		{ &hf_max_resp_exp,
			{ "Exponent", "igmp.max_resp.exp", FT_UINT8, BASE_HEX,
			NULL, IGMP_MAX_RESP_EXP, "Maximum Response Time, Exponent", HFILL }},

		{ &hf_max_resp_mant,
			{ "Mantissa", "igmp.max_resp.mant", FT_UINT8, BASE_HEX,
			NULL, IGMP_MAX_RESP_MANT, "Maximum Response Time, Mantissa", HFILL }},

		{ &hf_mtrace_max_hops,
			{ "# hops", "igmp.mtrace.max_hops", FT_UINT8, BASE_DEC,
			NULL, 0, "Maximum Number of Hops to Trace", HFILL }},

		{ &hf_mtrace_saddr,
			{ "Source Address", "igmp.mtrace.saddr", FT_IPv4, BASE_NONE,
			  NULL, 0, "Multicast Source for the Path Being Traced", HFILL }},

		{ &hf_mtrace_raddr,
			{ "Receiver Address", "igmp.mtrace.raddr", FT_IPv4, BASE_NONE,
			  NULL, 0, "Multicast Receiver for the Path Being Traced", HFILL }},

		{ &hf_mtrace_rspaddr,
			{ "Response Address", "igmp.mtrace.rspaddr", FT_IPv4, BASE_NONE,
			  NULL, 0, "Destination of Completed Traceroute Response", HFILL }},

		{ &hf_mtrace_resp_ttl,
			{ "Response TTL", "igmp.mtrace.resp_ttl", FT_UINT8, BASE_DEC,
			NULL, 0, "TTL for Multicasted Responses", HFILL }},

		{ &hf_mtrace_q_id,
			{ "Query ID", "igmp.mtrace.q_id", FT_UINT24, BASE_DEC,
			NULL, 0, "Identifier for this Traceroute Request", HFILL }},

		{ &hf_mtrace_q_arrival,
			{ "Query Arrival", "igmp.mtrace.q_arrival", FT_UINT32, BASE_DEC,
			NULL, 0, "Query Arrival Time", HFILL }},

		{ &hf_mtrace_q_inaddr,
			{ "In itf addr", "igmp.mtrace.q_inaddr", FT_IPv4, BASE_NONE,
			NULL, 0, "Incoming Interface Address", HFILL }},

		{ &hf_mtrace_q_outaddr,
			{ "Out itf addr", "igmp.mtrace.q_outaddr", FT_IPv4, BASE_NONE,
			NULL, 0, "Outgoing Interface Address", HFILL }},

		{ &hf_mtrace_q_prevrtr,
			{ "Previous rtr addr", "igmp.mtrace.q_prevrtr", FT_IPv4, BASE_NONE,
			NULL, 0, "Previous-Hop Router Address", HFILL }},

		{ &hf_mtrace_q_inpkt,
			{ "In pkts", "igmp.mtrace.q_inpkt", FT_UINT32, BASE_DEC,
			NULL, 0, "Input packet count on incoming interface", HFILL }},

		{ &hf_mtrace_q_outpkt,
			{ "Out pkts", "igmp.mtrace.q_outpkt", FT_UINT32, BASE_DEC,
			NULL, 0, "Output packet count on outgoing interface", HFILL }},

		{ &hf_mtrace_q_total,
			{ "S,G pkt count", "igmp.mtrace.q_total", FT_UINT32, BASE_DEC,
			NULL, 0, "Total number of packets for this source-group pair", HFILL }},

		{ &hf_mtrace_q_rtg_proto,
			{ "Rtg Protocol", "igmp.mtrace.q_rtg_proto", FT_UINT8, BASE_DEC,
			VALS(mtrace_rtg_vals), 0, "Routing protocol between this and previous hop rtr", HFILL }},

		{ &hf_mtrace_q_fwd_ttl,
			{ "FwdTTL", "igmp.mtrace.q_fwd_ttl", FT_UINT8, BASE_DEC,
			NULL, 0, "TTL required for forwarding", HFILL }},

		{ &hf_mtrace_q_mbz,
			{ "MBZ", "igmp.mtrace.q_mbz", FT_UINT8, BASE_HEX,
			NULL, 0x80, "Must be zeroed on transmission and ignored on reception", HFILL }},

		{ &hf_mtrace_q_s,
			{ "S", "igmp.mtrace.q_s", FT_UINT8, BASE_HEX,
			NULL, 0x40, "Set if S,G packet count is for source network", HFILL }},

		{ &hf_mtrace_q_src_mask,
			{ "Src Mask", "igmp.mtrace.q_src_mask", FT_UINT8, BASE_HEX,
			NULL, 0x3F, "Source mask length. 63 when forwarding on group state", HFILL }},

		{ &hf_mtrace_q_fwd_code,
			{ "Forwarding Code", "igmp.mtrace.q_fwd_code", FT_UINT8, BASE_HEX,
			VALS(mtrace_fwd_code_vals), 0, "Forwarding information/error code", HFILL }},

	};
	static gint *ett[] = {
		&ett_igmp,
		&ett_group_record,
		&ett_max_resp,
		&ett_mtrace_block,
	};

	proto_igmp = proto_register_protocol("Internet Group Management Protocol",
		"IGMP", "igmp");
	proto_register_field_array(proto_igmp, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));

	subdissector_table = register_dissector_table("igmp.type", "IGMP commands", proto_igmp, FT_UINT32, BASE_HEX);

}

void
proto_reg_handoff_igmp(void)
{
	dissector_handle_t igmp_handle, igmpv0_handle, igmpv1_handle, igmpv2_handle,
						igmp_mquery_handle, igmp_mtrace_handle, igmp_report_handle;
	range_t *igmpv0_range = NULL;

	igmp_handle = create_dissector_handle(dissect_igmp, proto_igmp);
	dissector_add_uint("ip.proto", IP_PROTO_IGMP, igmp_handle);

	/* IGMP v0 */
	range_convert_str(&igmpv0_range, "0-15", 15);
	igmpv0_handle = create_dissector_handle(dissect_igmp_v0, proto_igmp);
	dissector_add_uint_range("igmp.type", igmpv0_range, igmpv0_handle);
	g_free(igmpv0_range);

	/* IGMP v1 */
	igmpv1_handle = create_dissector_handle(dissect_igmp_v1, proto_igmp);
	dissector_add_uint("igmp.type", IGMP_V1_HOST_MEMBERSHIP_REPORT, igmpv1_handle);

	/* IGMP v2 */
	igmpv2_handle = create_dissector_handle(dissect_igmp_v2, proto_igmp);
	dissector_add_uint("igmp.type", IGMP_V2_MEMBERSHIP_REPORT, igmpv2_handle);
	dissector_add_uint("igmp.type", IGMP_V2_LEAVE_GROUP, igmpv2_handle);

	/* IGMP_V1_HOST_MEMBERSHIP_QUERY, all versions */
	igmp_mquery_handle = create_dissector_handle(dissect_igmp_mquery, proto_igmp);
	dissector_add_uint("igmp.type", IGMP_V1_HOST_MEMBERSHIP_QUERY, igmp_mquery_handle);

	igmp_report_handle = create_dissector_handle(dissect_igmp_v3_report, proto_igmp);
	dissector_add_uint("igmp.type", IGMP_V3_MEMBERSHIP_REPORT, igmp_report_handle);

	igmp_mtrace_handle = create_dissector_handle(dissect_igmp_mtrace, proto_igmp);
	dissector_add_uint("igmp.type", IGMP_TRACEROUTE_RESPONSE, igmp_mtrace_handle);
	dissector_add_uint("igmp.type", IGMP_TRACEROUTE_QUERY_REQ, igmp_mtrace_handle);
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

