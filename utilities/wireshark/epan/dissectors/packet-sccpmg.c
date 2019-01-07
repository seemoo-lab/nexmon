/* packet-sccpmg.c
 * Routines for Signalling Connection Control Part (SCCP) Management dissection
 *
 * It is hopefully compliant to:
 *   ANSI T1.112.3-1996
 *   ITU-T Q.713 7/1996
 *   YDN 038-1997 (Chinese ITU variant)
 *   JT-Q714 and NTT-Q714 (Japan)
 *
 *   Note that NTT Annex E (SCCP Management Procedure (Global Title Status
 *   Management)) is not implemented (yet)
 *
 * Copyright 2002, Jeff Morriss <jeff.morriss.ws [AT] gmail.com>
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
#include <epan/expert.h>
#include "packet-mtp3.h"

#define SCCPMG_SSN 1

#define SCCPMG_MESSAGE_TYPE_SSA 0x01
#define SCCPMG_MESSAGE_TYPE_SSP 0x02
#define SCCPMG_MESSAGE_TYPE_SST 0x03
#define SCCPMG_MESSAGE_TYPE_SOR 0x04
#define SCCPMG_MESSAGE_TYPE_SOG 0x05
/* SSC is ITU only */
#define SCCPMG_MESSAGE_TYPE_SSC 0x06
/* Below are ANSI only */
#define SCCPMG_MESSAGE_TYPE_SBR 0xfd
#define SCCPMG_MESSAGE_TYPE_SNR 0xfe
#define SCCPMG_MESSAGE_TYPE_SRT 0xff

void proto_register_sccpmg(void);
void proto_reg_handoff_sccpmg(void);

/* Same as below but with names typed out */
static const value_string sccpmg_message_type_values[] = {
	{ SCCPMG_MESSAGE_TYPE_SSA,   "SubSystem Allowed" },
	{ SCCPMG_MESSAGE_TYPE_SSP,   "SubSystem Prohibited" },
	{ SCCPMG_MESSAGE_TYPE_SST,   "Subsystem Status Test" },
	{ SCCPMG_MESSAGE_TYPE_SOR,   "Subsystem Out of service Request" },
	{ SCCPMG_MESSAGE_TYPE_SOG,   "Subsystem Out of service Grant" },
	{ SCCPMG_MESSAGE_TYPE_SSC,   "SubSystem Congested (ITU)" },
	{ SCCPMG_MESSAGE_TYPE_SBR,   "Subsystem Backup Routing (ANSI)" },
	{ SCCPMG_MESSAGE_TYPE_SNR,   "Subsystem Normal Routing (ANSI)" },
	{ SCCPMG_MESSAGE_TYPE_SRT,   "Subsystem Routing status Test (ANSI)" },
	{ 0,                       NULL } };

/* Same as above but in acronym for (for the Info column) */
static const value_string sccpmg_message_type_acro_values[] = {
	{ SCCPMG_MESSAGE_TYPE_SSA,   "SSA" },
	{ SCCPMG_MESSAGE_TYPE_SSP,   "SSP" },
	{ SCCPMG_MESSAGE_TYPE_SST,   "SST" },
	{ SCCPMG_MESSAGE_TYPE_SOR,   "SOR" },
	{ SCCPMG_MESSAGE_TYPE_SOG,   "SOG" },
	{ SCCPMG_MESSAGE_TYPE_SSC,   "SSC" },
	{ SCCPMG_MESSAGE_TYPE_SBR,   "SBR" },
	{ SCCPMG_MESSAGE_TYPE_SNR,   "SNR" },
	{ SCCPMG_MESSAGE_TYPE_SRT,   "SRT" },
	{ 0,                       NULL } };


#define SCCPMG_MESSAGE_TYPE_OFFSET 0
#define SCCPMG_MESSAGE_TYPE_LENGTH 1

#define SCCPMG_AFFECTED_SSN_OFFSET SCCPMG_MESSAGE_TYPE_LENGTH
#define SCCPMG_AFFECTED_SSN_LENGTH 1

#define SCCPMG_AFFECTED_PC_OFFSET (SCCPMG_AFFECTED_SSN_OFFSET + SCCPMG_AFFECTED_SSN_LENGTH)
#define ITU_SCCPMG_AFFECTED_PC_LENGTH 2
#define ANSI_SCCPMG_AFFECTED_PC_LENGTH 3

#define ITU_SCCPMG_SMI_OFFSET (SCCPMG_AFFECTED_PC_OFFSET + ITU_SCCPMG_AFFECTED_PC_LENGTH)
#define ANSI_SCCPMG_SMI_OFFSET (SCCPMG_AFFECTED_PC_OFFSET + ANSI_SCCPMG_AFFECTED_PC_LENGTH)
#define SCCPMG_SMI_LENGTH 1
#define SCCPMG_SMI_MASK 0x3

#define ITU_SCCPMG_CONGESTION_OFFSET (ITU_SCCPMG_SMI_OFFSET + SCCPMG_SMI_LENGTH)
#define ITU_SCCPMG_CONGESTION_LENGTH 1
#define ITU_SCCPMG_CONGESTION_MASK 0x0f
#define CHINESE_ITU_SCCPMG_CONGESTION_OFFSET (ANSI_SCCPMG_SMI_OFFSET + SCCPMG_SMI_LENGTH)

#define SCCPMG_SSN_LENGTH    1

/* Initialize the protocol and registered fields */
static int proto_sccpmg = -1;
static int hf_sccpmg_message_type = -1;
static int hf_sccpmg_affected_ssn = -1;
static int hf_sccpmg_affected_itu_pc = -1;
static int hf_sccpmg_affected_japan_pc = -1;
static int hf_sccpmg_affected_ansi_pc = -1;
static int hf_sccpmg_affected_chinese_pc = -1;
static int hf_sccpmg_affected_pc_member = -1;
static int hf_sccpmg_affected_pc_cluster = -1;
static int hf_sccpmg_affected_pc_network = -1;
static int hf_sccpmg_smi = -1;
static int hf_sccpmg_congestion_level = -1;

/* Initialize the subtree pointers */
static gint ett_sccpmg = -1;
static gint ett_sccpmg_affected_pc = -1;

static expert_field ei_sccpmg_unknown_msg = EI_INIT;

static void
dissect_sccpmg_affected_ssn(tvbuff_t *tvb, proto_tree *sccpmg_tree)
{
	proto_tree_add_item(sccpmg_tree, hf_sccpmg_affected_ssn, tvb,
			    SCCPMG_AFFECTED_SSN_OFFSET, SCCPMG_SSN_LENGTH,
			    ENC_BIG_ENDIAN);
}

static void
dissect_sccpmg_affected_pc(tvbuff_t *tvb, proto_tree *sccpmg_tree)
{
	guint8 offset = SCCPMG_AFFECTED_PC_OFFSET;

	if (mtp3_standard == ITU_STANDARD) {
		proto_tree_add_item(sccpmg_tree, hf_sccpmg_affected_itu_pc, tvb,
				    offset, ITU_PC_LENGTH, ENC_LITTLE_ENDIAN);
	} else if (mtp3_standard == JAPAN_STANDARD) {
		proto_tree_add_item(sccpmg_tree, hf_sccpmg_affected_japan_pc,
				    tvb, offset, JAPAN_PC_LENGTH, ENC_LITTLE_ENDIAN);
	} else /* ANSI_STANDARD and CHINESE_ITU_STANDARD */ {
		int *hf_affected_pc;

		if (mtp3_standard == ANSI_STANDARD)
		{
			hf_affected_pc = &hf_sccpmg_affected_ansi_pc;
		} else /* CHINESE_ITU_STANDARD */ {
			hf_affected_pc = &hf_sccpmg_affected_chinese_pc;
		}

		/* create and fill the PC tree */
		dissect_mtp3_3byte_pc(tvb, offset, sccpmg_tree,
				      ett_sccpmg_affected_pc, *hf_affected_pc,
				      hf_sccpmg_affected_pc_network,
				      hf_sccpmg_affected_pc_cluster,
				      hf_sccpmg_affected_pc_member, 0, 0);

	}
}

static void
dissect_sccpmg_smi(tvbuff_t *tvb, proto_tree *sccpmg_tree)
{
	guint8 offset = 0;

	if (mtp3_standard == ITU_STANDARD || mtp3_standard == JAPAN_STANDARD)
		offset = ITU_SCCPMG_SMI_OFFSET;
	else /* ANSI_STANDARD and CHINESE_ITU_STANDARD */
		offset = ANSI_SCCPMG_SMI_OFFSET;

	proto_tree_add_item(sccpmg_tree, hf_sccpmg_smi, tvb, offset,
			    SCCPMG_SMI_LENGTH, ENC_BIG_ENDIAN);
}

static void
dissect_sccpmg_congestion_level(tvbuff_t *tvb, proto_tree *sccpmg_tree)
{
	guint8 offset = 0;

	if (mtp3_standard == CHINESE_ITU_STANDARD)
		offset = CHINESE_ITU_SCCPMG_CONGESTION_OFFSET;
	else /* ITU_STANDARD or JAPAN_STANDARD */
		offset = ITU_SCCPMG_CONGESTION_OFFSET;

	proto_tree_add_item(sccpmg_tree, hf_sccpmg_congestion_level, tvb,
			    offset, ITU_SCCPMG_CONGESTION_LENGTH, ENC_BIG_ENDIAN);
}

static void
dissect_sccpmg_message(tvbuff_t *tvb, packet_info *pinfo, proto_tree *sccpmg_tree)
{
	guint8 message_type;

	/* Extract the message type;  all other processing is based on this */
	message_type   = tvb_get_guint8(tvb, SCCPMG_MESSAGE_TYPE_OFFSET);

	col_add_fstr(pinfo->cinfo, COL_INFO, "%s ", val_to_str_const(message_type, sccpmg_message_type_acro_values, "Unknown"));

	if (sccpmg_tree) {
		/* add the message type to the protocol tree */
		proto_tree_add_uint(sccpmg_tree, hf_sccpmg_message_type, tvb,
				    SCCPMG_MESSAGE_TYPE_OFFSET,
				    SCCPMG_MESSAGE_TYPE_LENGTH, message_type);
	}

	switch(message_type) {
	case SCCPMG_MESSAGE_TYPE_SBR:
	case SCCPMG_MESSAGE_TYPE_SNR:
	case SCCPMG_MESSAGE_TYPE_SRT:
		if (mtp3_standard != ANSI_STANDARD)
		{
			proto_tree_add_expert(sccpmg_tree, pinfo, &ei_sccpmg_unknown_msg, tvb, 0, -1);
			break;
		}
		/* else fallthrough */
	case SCCPMG_MESSAGE_TYPE_SSA:
	case SCCPMG_MESSAGE_TYPE_SSP:
	case SCCPMG_MESSAGE_TYPE_SST:
	case SCCPMG_MESSAGE_TYPE_SOR:
	case SCCPMG_MESSAGE_TYPE_SOG:
		dissect_sccpmg_affected_ssn(tvb, sccpmg_tree);
		dissect_sccpmg_affected_pc(tvb, sccpmg_tree);
		dissect_sccpmg_smi(tvb, sccpmg_tree);

		break;
	case SCCPMG_MESSAGE_TYPE_SSC:
		if (mtp3_standard != ANSI_STANDARD)
		{
			dissect_sccpmg_affected_ssn(tvb, sccpmg_tree);
			dissect_sccpmg_affected_pc(tvb, sccpmg_tree);
			dissect_sccpmg_smi(tvb, sccpmg_tree);
			dissect_sccpmg_congestion_level(tvb, sccpmg_tree);
		}
		/* else fallthrough */

	default:
		proto_tree_add_expert(sccpmg_tree, pinfo, &ei_sccpmg_unknown_msg, tvb, 0, -1);
	}
}

static int
dissect_sccpmg(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
	proto_item *sccpmg_item;
	proto_tree *sccpmg_tree = NULL;

	/* Make entry in the Protocol column on summary display */
	switch(mtp3_standard) {
	case ITU_STANDARD:
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "SCCPMG (Int. ITU)");
		break;
	case ANSI_STANDARD:
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "SCCPMG (ANSI)");
		break;
	case CHINESE_ITU_STANDARD:
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "SCCPMG (Chin. ITU)");
		break;
	};

	/* In the interest of speed, if "tree" is NULL, don't do any work not
	   necessary to generate protocol tree items. */
	if (tree) {
		/* create the sccpmg protocol tree */
		sccpmg_item = proto_tree_add_item(tree, proto_sccpmg, tvb, 0,
						  -1, ENC_NA);
		sccpmg_tree = proto_item_add_subtree(sccpmg_item, ett_sccpmg);
	}

	/* dissect the message */
	dissect_sccpmg_message(tvb, pinfo, sccpmg_tree);
	return tvb_captured_length(tvb);
}

/* Register the protocol with Wireshark */
void
proto_register_sccpmg(void)
{
	/* Setup list of header fields */
	static hf_register_info hf[] = {
	  { &hf_sccpmg_message_type,
	    { "Message Type", "sccpmg.message_type",
	      FT_UINT8, BASE_HEX, VALS(sccpmg_message_type_values), 0x0,
	      NULL, HFILL}},
	  { &hf_sccpmg_affected_ssn,
	    { "Affected SubSystem Number", "sccpmg.ssn",
	      FT_UINT8, BASE_DEC, NULL, 0x0,
	      NULL, HFILL}},
	  { &hf_sccpmg_affected_itu_pc,
	    { "Affected Point Code", "sccpmg.pc",
	      FT_UINT16, BASE_DEC, NULL, ITU_PC_MASK,
	      NULL, HFILL}},
	  { &hf_sccpmg_affected_japan_pc,
	    { "Affected Point Code", "sccpmg.pc",
	      FT_UINT16, BASE_DEC, NULL, 0x0,
	      NULL, HFILL}},
	  { &hf_sccpmg_affected_ansi_pc,
	    { "Affected Point Code", "sccpmg.ansi_pc",
	      FT_STRING, BASE_NONE, NULL, 0x0,
	      NULL, HFILL}},
	  { &hf_sccpmg_affected_chinese_pc,
	    { "Affected Point Code", "sccpmg.chinese_pc",
	      FT_STRING, BASE_NONE, NULL, 0x0,
	      NULL, HFILL}},
	  { &hf_sccpmg_affected_pc_network,
	    { "Affected PC Network", "sccpmg.network",
	      FT_UINT24, BASE_DEC, NULL, ANSI_NETWORK_MASK,
	      NULL, HFILL}},
	  { &hf_sccpmg_affected_pc_cluster,
	    { "Affected PC Cluster", "sccpmg.cluster",
	      FT_UINT24, BASE_DEC, NULL, ANSI_CLUSTER_MASK,
	      NULL, HFILL}},
	  { &hf_sccpmg_affected_pc_member,
	    { "Affected PC Member", "sccpmg.member",
	      FT_UINT24, BASE_DEC, NULL, ANSI_MEMBER_MASK,
	      NULL, HFILL}},
	  { &hf_sccpmg_smi,
	    { "Subsystem Multiplicity Indicator", "sccpmg.smi",
	      FT_UINT8, BASE_DEC, NULL, SCCPMG_SMI_MASK,
	      NULL, HFILL}},
	  { &hf_sccpmg_congestion_level,
	    { "SCCP Congestion Level (ITU)", "sccpmg.congestion",
	      FT_UINT8, BASE_DEC, NULL, ITU_SCCPMG_CONGESTION_MASK,
	      NULL, HFILL}}
	};

	/* Setup protocol subtree array */
	static gint *ett[] = {
		&ett_sccpmg,
		&ett_sccpmg_affected_pc
	};

	static ei_register_info ei[] = {
		{ &ei_sccpmg_unknown_msg, { "sccpmg.unknown_msg", PI_UNDECODED, PI_WARN, "Unknown message", EXPFILL }},
	};

	expert_module_t* expert_sccpmg;

	/* Register the protocol name and description */
	proto_sccpmg = proto_register_protocol("Signalling Connection Control Part Management",
					       "SCCPMG", "sccpmg");

	/* Required function calls to register the header fields and subtrees
	   used */
	proto_register_field_array(proto_sccpmg, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
	expert_sccpmg = expert_register_protocol(proto_sccpmg);
	expert_register_field_array(expert_sccpmg, ei, array_length(ei));
}

void
proto_reg_handoff_sccpmg(void)
{
	dissector_handle_t sccpmg_handle;

	sccpmg_handle = create_dissector_handle(dissect_sccpmg, proto_sccpmg);

	/* Register for SCCP SSN=1 messages */
	dissector_add_uint("sccp.ssn", SCCPMG_SSN, sccpmg_handle);
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
