/* packet-pw-satop.c
 * Routines for CESoPSN PW dissection as per RFC5086.
 * Copyright 2009, Dmitry Trebich, Artem Tamazov <artem.tamazov@tellabs.com>
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
 *
 * History:
 * ---------------------------------
 * 16.03.2009 initial implementation for MPLS
 * 14.08.2009 added: support for IP/UDP demultiplexing
 * Not supported yet:
 * - All PW modes, except Basic NxDS0 mode.
 * - <Optional> RTP Headers (RFC3550)
 * - Decoding of PW payload
 */

#include "config.h"

#include <epan/packet.h>
#include <epan/expert.h>

#include "packet-mpls.h"
#include "packet-pw-common.h"

void proto_register_pw_cesopsn(void);
void proto_reg_handoff_pw_cesopsn(void);

static gint proto = -1;
static gint ett = -1;

static int hf_cw = -1;
static int hf_cw_bits03 = -1;
static int hf_cw_lm = -1;
static int hf_cw_r = -1;
static int hf_cw_frg = -1;
static int hf_cw_len = -1;
static int hf_cw_seq = -1;
static int hf_payload = -1;
static int hf_payload_l = -1;

static expert_field ei_payload_size_invalid_undecoded = EI_INIT;
static expert_field ei_cw_frg = EI_INIT;
static expert_field ei_payload_size_invalid_error = EI_INIT;
static expert_field ei_cw_bits03 = EI_INIT;
static expert_field ei_pref_cw_len = EI_INIT;
static expert_field ei_cw_lm = EI_INIT;
static expert_field ei_packet_size_too_small = EI_INIT;

static dissector_handle_t pw_padding_handle;

const char pwc_longname_pw_cesopsn[] = "CESoPSN basic NxDS0 mode (no RTP support)";
static const char shortname[] = "CESoPSN basic (no RTP)";

static const value_string vals_cw_lm[] = {
	/* note that bitmask in hs_register_info is 0xb == 1011B */
	/* this is why 0x8 comes just after 0x3 */
	{ 0x0,	"Normal situation - no AC faults" },
	/*{ 0x1,	"Reserved combination" },*/
	{ 0x2,	"AC Fault - RDI condition" },
	{ 0x3,	"Reserved for CESoPSN signaling" },
	{ 0x8,	"AC Fault - TDM data is invalid" },
	/*{ 0x9,	"Reserved combination" },*/
	/*{ 0xa,	"Reserved combination" },*/
	/*{ 0xb,	"Reserved combination" },*/
	{ 0,	NULL }
};


static
void dissect_pw_cesopsn( tvbuff_t * tvb_original
						,packet_info * pinfo
						,proto_tree * tree
						,pwc_demux_type_t demux)
{
	const int encaps_size = 4; /*RTP header in encapsulation is not supported yet*/
	gint      packet_size;
	gint      payload_size;
	gint      padding_size;
	int properties;

	packet_size = tvb_reported_length_remaining(tvb_original, 0);

	/*
	 * FIXME
	 * "4" below should be replaced by something like "min_packet_size_this_dissector"
	 * Also call to dissect_try_cw_first_nibble() should be moved before this block
	 */
	if (packet_size < 4) /* 4 is smallest size which may be sensible (for PWACH dissector) */
	{
		proto_item  *item;
		item = proto_tree_add_item(tree, proto, tvb_original, 0, -1, ENC_NA);
		expert_add_info_format(pinfo, item, &ei_packet_size_too_small,
				       "PW packet size (%d) is too small to carry sensible information"
				       ,(int)packet_size);
		col_set_str(pinfo->cinfo, COL_PROTOCOL, shortname);
		col_set_str(pinfo->cinfo, COL_INFO, "Malformed: PW packet is too small");
		return;
	}

	switch (demux)
	{
	case PWC_DEMUX_MPLS:
		if (dissect_try_cw_first_nibble(tvb_original, pinfo, tree))
		{
			return;
		}
		break;
	case PWC_DEMUX_UDP:
		break;
	default:
		DISSECTOR_ASSERT_NOT_REACHED();
		return;
	}

	/* check how "good" is this packet */
	/* also decide payload length from packet size and CW */
	properties = PWC_PACKET_PROPERTIES_T_INITIALIZER;
	if (0 != (tvb_get_guint8(tvb_original, 0) & 0xf0 /*bits03*/))
	{
		properties |= PWC_CW_BAD_BITS03;
	}
	if (0 != (tvb_get_guint8(tvb_original, 1) & 0xc0 /*frag*/))
	{
		properties |= PWC_CW_BAD_FRAG;
	}
	{
		/* RFC5086:
		 * [LEN (bits (10 to 15) MAY be used to carry the length of the CESoPSN
		 * packet (defined as the size of the CESoPSN header + the payload size)
		 * if it is less than 64 bytes, and MUST be set to zero otherwise.
		 * Note:  If fixed RTP header is used in the encapsulation, it is
		 * considered part of the CESoPSN header.]
		 *
		 * Note that this differs from RFC4385's definition of length:
		 * [ If the MPLS payload is less than 64 bytes, the length field
		 * MUST be set to the length of the PW payload...]
		 *
		 * We will use RFC5086's definition here.
		 */
		int  cw_len;
		gint payload_size_from_packet;

		cw_len = tvb_get_guint8(tvb_original, 1) & 0x3f;
		payload_size_from_packet = packet_size - encaps_size;
		if (cw_len != 0)
		{
			gint payload_size_from_cw;
			payload_size_from_cw = cw_len - encaps_size;
			/*
			 * Assumptions for error case,
			 * will be overwritten if no errors found:
			 */
			payload_size = payload_size_from_packet;
			padding_size = 0;

			if (payload_size_from_cw < 0)
			{
				properties |= PWC_CW_BAD_PAYLEN_LT_0;
			}
			else if (payload_size_from_cw > payload_size_from_packet)
			{
				properties |= PWC_CW_BAD_PAYLEN_GT_PACKET;
			}
			else if (payload_size_from_packet >= 64)
			{
				properties |= PWC_CW_BAD_LEN_MUST_BE_0;
			}
			else /* ok */
			{
				payload_size = payload_size_from_cw;
				padding_size = payload_size_from_packet - payload_size_from_cw; /* >=0 */
			}
		}
		else
		{
			payload_size = payload_size_from_packet;
			padding_size = 0;
		}
	}

	{
		guint8 cw_lm;
		cw_lm = tvb_get_guint8(tvb_original, 0) & 0x0b /*l+mod*/;
		if (NULL == try_val_to_str(cw_lm, vals_cw_lm))
		{
			properties |= PWC_CW_SUSPECT_LM;
		}

		{
			guint8 l_bit, m_bits;
			l_bit  = (cw_lm & 0x08) >> 3;
			m_bits = (cw_lm & 0x03) >> 0;
			if ((l_bit == 0 && m_bits == 0x0) /*CESoPSN data packet - normal situation*/
			    ||(l_bit == 0 && m_bits == 0x2) /*CESoPSN data packet - RDI on the AC*/ )
			{
				if ((payload_size == 0) || ((payload_size % 8) != 0))
				{
					properties |= PWC_PAY_SIZE_BAD;
				}
			}
			else if (l_bit == 1 && m_bits == 0x0) /*TDM data is invalid; payload MAY be omitted*/
			{
				/*allow any size of payload*/
			}
			else /*reserved combinations*/
			{
				/*allow any size of payload*/
			}
		}
	}

	/* fill up columns*/
	col_set_str(pinfo->cinfo, COL_PROTOCOL, shortname);
	col_clear(pinfo->cinfo, COL_INFO);
	if (properties & PWC_ANYOF_CW_BAD)
	{
		col_set_str(pinfo->cinfo, COL_INFO, "CW:Bad, ");
	}
	else if (properties & PWC_ANYOF_CW_SUSPECT)
	{
		col_append_str(pinfo->cinfo, COL_INFO, "CW:Suspect, ");
	}

	if (properties & PWC_PAY_SIZE_BAD)
	{
		col_append_str(pinfo->cinfo, COL_INFO, "Payload size:Bad, ");
	}

	col_append_fstr(pinfo->cinfo, COL_INFO, "TDM octets:%d", (int)payload_size);

	if (padding_size != 0)
	{
		col_append_fstr(pinfo->cinfo, COL_INFO, ", Padding:%d", (int)padding_size);
	}

	{
		proto_item* item;
		item = proto_tree_add_item(tree, proto, tvb_original, 0, -1, ENC_NA);
		pwc_item_append_cw(item,tvb_get_ntohl(tvb_original, 0),TRUE);
		pwc_item_append_text_n_items(item,(int)payload_size,"octet");
		{
			proto_tree* tree2;
			tree2 = proto_item_add_subtree(item, ett);
			{
				tvbuff_t* tvb;
				proto_item* item2;
				tvb = tvb_new_subset_length(tvb_original, 0, PWC_SIZEOF_CW);
				item2 = proto_tree_add_item(tree2, hf_cw, tvb, 0, -1, ENC_NA);
				pwc_item_append_cw(item2,tvb_get_ntohl(tvb, 0),FALSE);
				{
					proto_tree* tree3;
					tree3 = proto_item_add_subtree(item, ett);
					{
						proto_item* item3;
						if (properties & PWC_CW_BAD_BITS03) /*display only if value is wrong*/
						{
							item3 = proto_tree_add_item(tree3, hf_cw_bits03, tvb, 0, 1, ENC_BIG_ENDIAN);
							expert_add_info(pinfo, item3, &ei_cw_bits03);
						}

						item3 = proto_tree_add_item(tree3, hf_cw_lm,  tvb, 0, 1, ENC_BIG_ENDIAN);
						if (properties & PWC_CW_SUSPECT_LM)
						{
							expert_add_info(pinfo, item3, &ei_cw_lm);
						}

						proto_tree_add_item(tree3, hf_cw_r, tvb, 0, 1, ENC_BIG_ENDIAN);

						item3 = proto_tree_add_item(tree3, hf_cw_frg, tvb, 1, 1, ENC_BIG_ENDIAN);
						if (properties & PWC_CW_BAD_FRAG)
						{
							expert_add_info(pinfo, item3, &ei_cw_frg);
						}

						item3 = proto_tree_add_item(tree3, hf_cw_len, tvb, 1, 1, ENC_BIG_ENDIAN);
						if (properties & PWC_CW_BAD_PAYLEN_LT_0)
						{
							expert_add_info_format(pinfo, item3, &ei_pref_cw_len,
								"Bad Length: too small, must be > %d",
								(int)encaps_size);
						}
						if (properties & PWC_CW_BAD_PAYLEN_GT_PACKET)
						{
							expert_add_info_format(pinfo, item3, &ei_pref_cw_len,
								"Bad Length: must be <= than PSN packet size (%d)",
								(int)packet_size);
						}
						if (properties & PWC_CW_BAD_LEN_MUST_BE_0)
						{
							expert_add_info_format(pinfo, item3, &ei_pref_cw_len,
								"Bad Length: must be 0 if CESoPSN packet size (%d) is > 64",
								(int)packet_size);
						}

						proto_tree_add_item(tree3, hf_cw_seq, tvb, 2, 2, ENC_BIG_ENDIAN);

					}
				}
			}
		}

		/* payload */
		if (payload_size == 0)
		{
			if (properties & PWC_PAY_SIZE_BAD)
			{
				expert_add_info_format(pinfo, item, &ei_payload_size_invalid_error,
					"CESoPSN payload: none found. Size of payload must be <> 0");
			}
			else
			{
				expert_add_info_format(pinfo, item, &ei_payload_size_invalid_undecoded,
					"CESoPSN payload: omitted to conserve bandwidth");
			}
		}
		else
		{
			proto_tree* tree2;
			tree2 = proto_item_add_subtree(item, ett);
			{
				proto_item* item2;
				tvbuff_t* tvb;
				tvb = tvb_new_subset_length(tvb_original, PWC_SIZEOF_CW, payload_size);
				item2 = proto_tree_add_item(tree2, hf_payload, tvb, 0, -1, ENC_NA);
				pwc_item_append_text_n_items(item2,(int)payload_size,"octet");
				if (properties & PWC_PAY_SIZE_BAD)
				{
					expert_add_info_format(pinfo, item2, &ei_payload_size_invalid_error,
						"CESoPSN packet payload size must be multiple of 8");
				}
				tree2 = proto_item_add_subtree(item2, ett);
				call_data_dissector(tvb, pinfo, tree2);
				item2 = proto_tree_add_int(tree2, hf_payload_l, tvb, 0, 0
					,(int)payload_size); /* allow filtering */
				PROTO_ITEM_SET_HIDDEN(item2);
			}
		}

		/* padding */
		if (padding_size > 0)
		{
			proto_tree* tree2;
			tree2 = proto_item_add_subtree(item, ett);
			{
				tvbuff_t* tvb;
				tvb = tvb_new_subset(tvb_original, PWC_SIZEOF_CW + payload_size, padding_size, -1);
				call_dissector(pw_padding_handle, tvb, pinfo, tree2);
			}
		}
	}
	return;
}


static
int dissect_pw_cesopsn_mpls( tvbuff_t * tvb_original, packet_info * pinfo, proto_tree * tree, void* data _U_)
{
	dissect_pw_cesopsn(tvb_original,pinfo,tree,PWC_DEMUX_MPLS);
	return tvb_captured_length(tvb_original);
}


static
int dissect_pw_cesopsn_udp( tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree, void* data _U_)
{
	dissect_pw_cesopsn(tvb,pinfo,tree,PWC_DEMUX_UDP);
	return tvb_captured_length(tvb);
}


void proto_register_pw_cesopsn(void)
{
	static hf_register_info hf[] = {
		{ &hf_cw	,{"Control Word"		,"pwcesopsn.cw"
				  ,FT_NONE			,BASE_NONE		,NULL
				  ,0				,NULL			,HFILL }},

		{&hf_cw_bits03,{"Bits 0 to 3"			,"pwcesopsn.cw.bits03"
				,FT_UINT8			,BASE_DEC		,NULL
				,0xf0				,NULL			,HFILL }},

		{ &hf_cw_lm, 	{"L+M bits"			,"pwcesopsn.cw.lm"
				 ,FT_UINT8			,BASE_HEX		,VALS(vals_cw_lm)
				 ,0x0b	  			,NULL 			,HFILL }},

		{&hf_cw_r,	{"R bit: Local CE-bound IWF"	,"pwcesopsn.cw.rbit"
				 ,FT_UINT8			,BASE_DEC		,VALS(pwc_vals_cw_r_bit)
				 ,0x04				,NULL			,HFILL }},

		{&hf_cw_frg,	{"Fragmentation"		,"pwcesopsn.cw.frag"
				 ,FT_UINT8			,BASE_DEC		,VALS(pwc_vals_cw_frag)
				 ,0xc0				,NULL			,HFILL }},

		{&hf_cw_len,	{"Length"			,"pwcesopsn.cw.length"
				 ,FT_UINT8			,BASE_DEC		,NULL
				 ,0x3f				,NULL			,HFILL }},

		{&hf_cw_seq,	{"Sequence number"		,"pwcesopsn.cw.seqno"
				 ,FT_UINT16			,BASE_DEC		,NULL
				 ,0				,NULL			,HFILL }},

		{&hf_payload	,{"TDM payload"			,"pwcesopsn.payload"
				  ,FT_BYTES			,BASE_NONE		,NULL
				  ,0				,NULL			,HFILL }},

		{&hf_payload_l	,{"TDM payload length"		,"pwcesopsn.payload.len"
				  ,FT_INT32			,BASE_DEC		,NULL
				  ,0				,NULL			,HFILL }}
	};

	static gint *ett_array[] = {
		&ett
	};
	static ei_register_info ei[] = {
		{ &ei_packet_size_too_small, { "pwcesopsn.packet_size_too_small", PI_MALFORMED, PI_ERROR, "PW packet size is too small to carry sensible information", EXPFILL }},
		{ &ei_cw_bits03, { "pwcesopsn.cw.bits03.not_zero", PI_MALFORMED, PI_ERROR, "Bits 0..3 of Control Word must be 0", EXPFILL }},
		{ &ei_cw_lm, { "pwcesopsn.cw.lm.reserved", PI_UNDECODED, PI_WARN, "Reserved combination of L and Modifier bits", EXPFILL }},
		{ &ei_cw_frg, { "pwcesopsn.cw.frag.not_allowed", PI_MALFORMED, PI_ERROR, "Fragmentation of payload is not allowed for basic CESoPSN mode", EXPFILL }},
		{ &ei_pref_cw_len, { "pwcesopsn.cw.length.invalid", PI_MALFORMED, PI_ERROR, "Bad Length: too small", EXPFILL }},
		{ &ei_payload_size_invalid_error, { "pwcesopsn.payload.size_invalid", PI_MALFORMED, PI_ERROR, "CESoPSN payload size invalid", EXPFILL }},
		{ &ei_payload_size_invalid_undecoded, { "pwcesopsn.payload.undecoded", PI_UNDECODED, PI_NOTE, "CESoPSN payload: omitted to conserve bandwidth", EXPFILL }},
	};
	expert_module_t* expert_pwcesopsn;

	proto = proto_register_protocol(pwc_longname_pw_cesopsn, shortname, "pwcesopsn");
	proto_register_field_array(proto, hf, array_length(hf));
	proto_register_subtree_array(ett_array, array_length(ett_array));
	expert_pwcesopsn = expert_register_protocol(proto);
	expert_register_field_array(expert_pwcesopsn, ei, array_length(ei));
	register_dissector("pw_cesopsn_udp", dissect_pw_cesopsn_udp, proto);
}


void proto_reg_handoff_pw_cesopsn(void)
{
	dissector_handle_t pw_cesopsn_mpls_handle;

	pw_padding_handle = find_dissector_add_dependency("pw_padding", proto);

	/* For Decode As */
	pw_cesopsn_mpls_handle = create_dissector_handle( dissect_pw_cesopsn_mpls, proto );
	dissector_add_for_decode_as("mpls.label", pw_cesopsn_mpls_handle);

	dissector_add_for_decode_as("udp.port", find_dissector("pw_cesopsn_udp"));
	return;
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
