/* packet-x29.c
 * Routines for X.29 packet dissection
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
#include <epan/nlpid.h>

void proto_register_x29(void);
void proto_reg_handoff_x29(void);

static int proto_x29 = -1;
static int hf_msg_code = -1;
static int hf_error_type = -1;
static int hf_inv_msg_code = -1;

/* Generated from convert_proto_tree_add_text.pl */
static int hf_x29_pad_message_data = -1;
static int hf_x29_type_reference_value = -1;
static int hf_x29_type_reference = -1;
static int hf_x29_data = -1;
static int hf_x29_type_of_aspect = -1;
static int hf_x29_reselection_message_data = -1;
static int hf_x29_break_value = -1;
static int hf_x29_parameter = -1;
static int hf_x29_value = -1;

static gint ett_x29 = -1;

/*
 * PAD messages.
 */
#define SET_MSG			0x02
#define READ_MSG		0x04
#define SET_AND_READ_MSG	0x06
#define PARAMETER_IND_MSG	0x00
#define INV_TO_CLEAR_MSG	0x01
#define BREAK_IND_MSG		0x03
#define RESELECTION_MSG		0x07
#define ERROR_MSG		0x05
#define RESEL_WITH_TOA_NPI_MSG	0x08

static const value_string message_code_vals[] = {
	{ SET_MSG,			"Set" },
	{ READ_MSG,			"Read" },
	{ SET_AND_READ_MSG,		"Set and read" },
	{ PARAMETER_IND_MSG,		"Parameter indication" },
	{ INV_TO_CLEAR_MSG,		"Invitation to clear" },
	{ BREAK_IND_MSG,		"Indication of break" },
	{ RESELECTION_MSG,		"Reselection" },
	{ ERROR_MSG,			"Error" },
	{ RESEL_WITH_TOA_NPI_MSG,	"Reselection with TOA/NPI" },
	{ 0,				NULL }
};

static const value_string error_type_vals[] = {
	{ 0x00, "Received PAD message contained less than eight bits" },
	{ 0x02, "Unrecognized message code in received PAD message" },
	{ 0x04, "Parameter field format was incorrect or incompatible with message code" },
	{ 0x06, "Received PAD message did not contain an integral number of octets" },
	{ 0x08, "Received Parameter Indication PAD message was unsolicited" },
	{ 0x0A, "Received PAD message was too long" },
	{ 0x0C, "Unauthorized reselection PAD message" },
	{ 0,    NULL },
};

static const value_string reference_type_vals[] = {
	{ 0x01, "Change in PAD Aspect" },
	{ 0x08, "Break" },
	{ 0,    NULL },
};

static int
dissect_x29(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data)
{
	int         offset = 0;
	proto_tree *x29_tree;
	proto_item *ti;
	gboolean   *q_bit_set;
	guint8      msg_code;
	guint8      error_type;
	guint8      type_ref;
	gint        next_offset;
	int         linelen;

	/* Reject the packet if data is NULL */
	if (data == NULL)
		return 0;
	q_bit_set = (gboolean *)data;

	col_set_str(pinfo->cinfo, COL_PROTOCOL, "X.29");
	col_clear(pinfo->cinfo, COL_INFO);

	ti = proto_tree_add_item(tree, proto_x29, tvb, offset, -1, ENC_NA);
	x29_tree = proto_item_add_subtree(ti, ett_x29);

	if (*q_bit_set) {
		/*
		 * Q bit set - this is a PAD message.
		 */
		msg_code = tvb_get_guint8(tvb, offset);
		col_add_fstr(pinfo->cinfo, COL_INFO, "%s PAD message",
			    val_to_str(msg_code, message_code_vals,
			        "Unknown (0x%02x)"));

		proto_tree_add_uint(x29_tree, hf_msg_code, tvb,
		    offset, 1, msg_code);
		offset++;

		switch (msg_code) {

		case SET_MSG:
		case READ_MSG:
		case SET_AND_READ_MSG:
		case PARAMETER_IND_MSG:
			/*
			 * XXX - dissect the references as per X.3.
			 */
			while (tvb_reported_length_remaining(tvb, offset) > 0) {
				proto_tree_add_item(x29_tree, hf_x29_parameter, tvb, offset, 1, ENC_BIG_ENDIAN);
				offset++;
				proto_tree_add_item(x29_tree, hf_x29_value, tvb, offset, 1, ENC_BIG_ENDIAN);
				offset++;
			}
			break;

		case INV_TO_CLEAR_MSG:
			/*
			 * No data for this message.
			 */
			break;

		case ERROR_MSG:
			error_type = tvb_get_guint8(tvb, offset);
			proto_tree_add_uint(x29_tree, hf_error_type, tvb,
			    offset, 1, error_type);
			offset++;
			if (error_type != 0) {
				proto_tree_add_item(x29_tree, hf_inv_msg_code,
				    tvb, offset, 1, ENC_BIG_ENDIAN);
			}
			break;

		case BREAK_IND_MSG:
			if (tvb_reported_length_remaining(tvb, offset) > 0) {
				type_ref = tvb_get_guint8(tvb, offset);
				proto_tree_add_item(x29_tree, hf_x29_type_reference, tvb, offset, 1, ENC_BIG_ENDIAN);
				offset++;
				switch (type_ref) {

				case 0x01:	/* change in PAD Aspect */
					/*
					 * XXX - dissect as per X.28.
					 */
					proto_tree_add_item(x29_tree, hf_x29_type_of_aspect, tvb, offset, 1, ENC_BIG_ENDIAN);
					offset++;
					break;

				case 0x08:	/* break */
					proto_tree_add_item(x29_tree, hf_x29_break_value, tvb, offset, 1, ENC_BIG_ENDIAN);
					offset++;
					break;

				default:
					proto_tree_add_item(x29_tree, hf_x29_type_reference_value, tvb, offset, 1, ENC_BIG_ENDIAN);
					offset++;
					break;
				}
			}
			break;

		case RESELECTION_MSG:
			/*
			 * XXX - dissect me.
			 */
			proto_tree_add_item(x29_tree, hf_x29_reselection_message_data, tvb, offset, -1, ENC_NA);
			break;

		case RESEL_WITH_TOA_NPI_MSG:
			/*
			 * XXX - dissect me.
			 */
			proto_tree_add_item(x29_tree, hf_x29_reselection_message_data, tvb, offset, -1, ENC_NA);
			break;

		default:
			proto_tree_add_item(x29_tree, hf_x29_pad_message_data, tvb, offset, -1, ENC_NA);
			break;
		}
	} else {
		/*
		 * Q bit not set - this is data.
		 */
		col_set_str(pinfo->cinfo, COL_INFO, "Data ...");

		if (tree) {
			while (tvb_offset_exists(tvb, offset)) {
				/*
				 * Find the end of the line.
				 */
				tvb_find_line_end(tvb, offset, -1,
				    &next_offset, FALSE);

				/*
				 * Now compute the length of the line
				 * *including* the end-of-line indication,
				 * if any; we display it all.
				 */
				linelen = next_offset - offset;

				proto_tree_add_item(x29_tree, hf_x29_data, tvb, offset, linelen, ENC_NA|ENC_ASCII);
				offset = next_offset;
			}
		}
	}

	return tvb_captured_length(tvb);
}

void
proto_register_x29(void)
{
	static hf_register_info hf[] = {
	    { &hf_msg_code,
		{ "Message code", "x29.msg_code", FT_UINT8, BASE_HEX,
		  VALS(message_code_vals), 0x0, "X.29 PAD message code",
		  HFILL }},
	    { &hf_error_type,
		{ "Error type", "x29.error_type", FT_UINT8, BASE_HEX,
		  VALS(error_type_vals), 0x0, "X.29 error PAD message error type",
		  HFILL }},
	    { &hf_inv_msg_code,
		{ "Invalid message code", "x29.inv_msg_code", FT_UINT8, BASE_HEX,
		  VALS(message_code_vals), 0x0, "X.29 Error PAD message invalid message code",
		  HFILL }},

		/* Generated from convert_proto_tree_add_text.pl */
		{ &hf_x29_type_reference, { "Type reference", "x29.type_reference", FT_UINT8, BASE_DEC, VALS(reference_type_vals), 0x0, NULL, HFILL }},
		{ &hf_x29_type_of_aspect, { "Type of aspect", "x29.type_of_aspect", FT_UINT8, BASE_HEX, NULL, 0x0, NULL, HFILL }},
		{ &hf_x29_break_value, { "Break value", "x29.break_value", FT_UINT8, BASE_HEX, NULL, 0x0, NULL, HFILL }},
		{ &hf_x29_type_reference_value, { "Type value", "x29.type_reference.value", FT_UINT8, BASE_HEX, NULL, 0x0, NULL, HFILL }},
		{ &hf_x29_reselection_message_data, { "Reselection message data", "x29.reselection_message_data", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }},
		{ &hf_x29_pad_message_data, { "PAD message data", "x29.pad_message_data", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }},
		{ &hf_x29_data, { "Data", "x29.data", FT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL }},
		{ &hf_x29_parameter, { "Parameter", "x29.parameter", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL }},
		{ &hf_x29_value, { "Value", "x29.value", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL }},
	};
	static gint *ett[] = {
		&ett_x29,
	};

	proto_x29 = proto_register_protocol("X.29", "X.29", "x29");
	proto_register_field_array(proto_x29, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
}

void
proto_reg_handoff_x29(void)
{
	dissector_handle_t x29_handle;

	x29_handle = create_dissector_handle(dissect_x29, proto_x29);
	dissector_add_uint("x.25.spi", NLPID_SPI_X_29, x29_handle);
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
