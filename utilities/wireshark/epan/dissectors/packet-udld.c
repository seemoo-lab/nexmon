/* packet-udld.c
 * Routines for the disassembly of the "UniDirectional Link Detection"
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



/*
 * See
 *
 *  http://www.ietf.org/internet-drafts/draft-foschiano-udld-02.txt
 *
 * for some information on UDLD.
 */

/* Offsets in TLV structure. */
#define TLV_TYPE    0
#define TLV_LENGTH  2

void proto_register_udld(void);
void proto_reg_handoff_udld(void);

static int proto_udld = -1;
static int hf_udld_version = -1;
static int hf_udld_opcode = -1;
static int hf_udld_flags = -1;
static int hf_udld_flags_rt = -1;
static int hf_udld_flags_rsy = -1;
static int hf_udld_checksum = -1;
static int hf_udld_tlvtype = -1;
static int hf_udld_tlvlength = -1;
static int hf_udld_device_id = -1;
static int hf_udld_sent_through_interface = -1;
static int hf_udld_data = -1;


static expert_field ei_udld_tlvlength = EI_INIT;

static gint ett_udld = -1;
static gint ett_udld_flags = -1;
static gint ett_udld_tlv = -1;

#define TYPE_DEVICE_ID        0x0001
#define TYPE_PORT_ID          0x0002
#define TYPE_ECHO             0x0003
#define TYPE_MESSAGE_INTERVAL 0x0004
#define TYPE_TIMEOUT_INTERVAL 0x0005
#define TYPE_DEVICE_NAME      0x0006
#define TYPE_SEQUENCE_NUMBER  0x0007


static const value_string type_vals[] = {
    { TYPE_DEVICE_ID,        "Device ID" },
    { TYPE_PORT_ID,          "Port ID" },
    { TYPE_ECHO,             "Echo" },
    { TYPE_MESSAGE_INTERVAL, "Message interval" },
    { TYPE_TIMEOUT_INTERVAL, "Timeout interval" },
    { TYPE_DEVICE_NAME,      "Device name" },
    { TYPE_SEQUENCE_NUMBER,  "Sequence number" },
    { 0, NULL }
};

#define OPCODE_RESERVED 0x00
#define OPCODE_PROBE    0x01
#define OPCODE_ECHO     0x02
#define OPCODE_FLUSH    0x03

static const value_string opcode_vals[] = {
    { OPCODE_RESERVED, "Reserved" },
    { OPCODE_PROBE,    "Probe" },
    { OPCODE_ECHO,     "Echo" },
    { OPCODE_FLUSH,    "Flush" },
    { 0, NULL }
};

static int
dissect_udld(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
    proto_item *ti;
    proto_tree *udld_tree = NULL;
    int         offset    = 0;
    guint16     type;
    guint16     length;
    proto_tree *tlv_tree;
    int         real_length;

    col_set_str(pinfo->cinfo, COL_PROTOCOL, "UDLD");
    col_clear(pinfo->cinfo, COL_INFO);

    if (tree) {
    proto_item *flags_ti;
    proto_tree *flags_tree;

    ti = proto_tree_add_item(tree, proto_udld, tvb, offset, -1, ENC_NA);
    udld_tree = proto_item_add_subtree(ti, ett_udld);

    /* UDLD header */
    proto_tree_add_item(udld_tree, hf_udld_version, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(udld_tree, hf_udld_opcode, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset += 1;
    flags_ti = proto_tree_add_item(udld_tree, hf_udld_flags, tvb, offset, 1, ENC_BIG_ENDIAN);
    flags_tree = proto_item_add_subtree(flags_ti, ett_udld_flags);
    proto_tree_add_item(flags_tree, hf_udld_flags_rt, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(flags_tree, hf_udld_flags_rsy, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset += 1;
    proto_tree_add_checksum(udld_tree, tvb, offset, hf_udld_checksum, -1, NULL, pinfo, 0, ENC_BIG_ENDIAN, PROTO_CHECKSUM_NO_FLAGS);
    offset += 2;
    } else {
    offset += 4; /* The version/opcode/flags/checksum fields from above */
    }

    while (tvb_reported_length_remaining(tvb, offset) != 0) {
        type = tvb_get_ntohs(tvb, offset + TLV_TYPE);
        length = tvb_get_ntohs(tvb, offset + TLV_LENGTH);
        if (length < 4) {
            tlv_tree = proto_tree_add_subtree_format(udld_tree, tvb, offset, 4,
                ett_udld_tlv, NULL, "TLV with invalid length %u (< 4)", length);
            proto_tree_add_uint(tlv_tree, hf_udld_tlvtype, tvb,
                offset + TLV_TYPE, 2, type);
            ti = proto_tree_add_uint(tlv_tree, hf_udld_tlvlength, tvb,
                offset + TLV_LENGTH, 2, length);
            expert_add_info(pinfo, ti, &ei_udld_tlvlength);
            offset += 4;
            break;
        }

        switch (type) {

        case TYPE_DEVICE_ID:
        /* Device ID */

        col_append_fstr(pinfo->cinfo, COL_INFO,
                    "Device ID: %s  ",
                    tvb_format_stringzpad(tvb, offset + 4,
                              length - 4));

        if (tree) {
            tlv_tree = proto_tree_add_subtree_format(udld_tree, tvb, offset,
                length, ett_udld_tlv, NULL, "Device ID: %s",
                tvb_format_stringzpad(tvb, offset + 4, length - 4));
            proto_tree_add_uint(tlv_tree, hf_udld_tlvtype, tvb,
                offset + TLV_TYPE, 2, type);
            proto_tree_add_uint(tlv_tree, hf_udld_tlvlength, tvb,
                offset + TLV_LENGTH, 2, length);
            proto_tree_add_item(tlv_tree, hf_udld_device_id, tvb, offset + 4,
                length - 4, ENC_ASCII|ENC_NA);
        }
        offset += length;
        break;

        case TYPE_PORT_ID:
        real_length = length;
        if (tvb_get_guint8(tvb, offset + real_length) != 0x00) {
            /* The length in the TLV doesn't appear to be the
               length of the TLV, as the byte just past it
               isn't the first byte of a 2-byte big-endian
               small integer; make the length of the TLV the length
               in the TLV, plus 4 bytes for the TLV type and length,
               minus 1 because that's what makes one capture work. */
            real_length = length + 3;
        }

        col_append_fstr(pinfo->cinfo, COL_INFO,
                    "Port ID: %s  ",
                    tvb_format_stringzpad(tvb, offset + 4, length - 4));

        if (tree) {
            tlv_tree = proto_tree_add_subtree_format(udld_tree, tvb, offset,
                real_length, ett_udld_tlv, NULL, "Port ID: %s",
                tvb_format_text(tvb, offset + 4, real_length - 4));
            proto_tree_add_uint(tlv_tree, hf_udld_tlvtype, tvb,
                offset + TLV_TYPE, 2, type);
            proto_tree_add_uint(tlv_tree, hf_udld_tlvlength, tvb,
                offset + TLV_LENGTH, 2, length);
            proto_tree_add_item(tlv_tree, hf_udld_sent_through_interface, tvb, offset + 4,
                real_length - 4, ENC_ASCII|ENC_NA);
        }
        offset += real_length;
        break;

        case TYPE_ECHO:
        case TYPE_MESSAGE_INTERVAL:
        case TYPE_TIMEOUT_INTERVAL:
        case TYPE_DEVICE_NAME:
        case TYPE_SEQUENCE_NUMBER:
        default:
        tlv_tree = proto_tree_add_subtree_format(udld_tree, tvb, offset,
            length, ett_udld_tlv, NULL, "Type: %s, length: %u",
            val_to_str(type, type_vals, "Unknown (0x%04x)"),
            length);
        proto_tree_add_uint(tlv_tree, hf_udld_tlvtype, tvb,
            offset + TLV_TYPE, 2, type);
        proto_tree_add_uint(tlv_tree, hf_udld_tlvlength, tvb,
            offset + TLV_LENGTH, 2, length);
        if (length > 4) {
            proto_tree_add_item(tlv_tree, hf_udld_data, tvb, offset + 4,
                length - 4, ENC_NA);
        } else {
            return offset;
        }
        offset += length;
        }
    }

    call_data_dissector(tvb_new_subset_remaining(tvb, offset), pinfo, udld_tree);
    return tvb_captured_length(tvb);
}

void
proto_register_udld(void)
{
    static hf_register_info hf[] = {
        { &hf_udld_version,
          { "Version",        "udld.version",  FT_UINT8, BASE_DEC, NULL, 0xE0,
            NULL, HFILL }},

        { &hf_udld_opcode,
          { "Opcode",     "udld.opcode", FT_UINT8, BASE_DEC, VALS(opcode_vals), 0x1F,
            NULL, HFILL }},

        { &hf_udld_flags,
          { "Flags",      "udld.flags", FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_udld_flags_rt,
          { "Recommended timeout",        "udld.flags.rt", FT_UINT8, BASE_HEX, NULL, 0x80,
            NULL, HFILL }},

        { &hf_udld_flags_rsy,
          { "ReSynch",        "udld.flags.rsy", FT_UINT8, BASE_HEX, NULL, 0x40,
            NULL, HFILL }},

        { &hf_udld_checksum,
          { "Checksum",       "udld.checksum", FT_UINT16, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},

        { &hf_udld_tlvtype,
          { "Type",       "udld.tlv.type", FT_UINT16, BASE_HEX, VALS(type_vals), 0x0,
            NULL, HFILL }},

        { &hf_udld_tlvlength,
          { "Length",     "udld.tlv.len", FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_udld_device_id,
          { "Device ID",      "udld.device_id", FT_STRINGZ, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},

        { &hf_udld_sent_through_interface,
          { "Sent through Interface",     "udld.sent_through_interface", FT_STRING, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},

        { &hf_udld_data,
          { "Data",       "udld.data", FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},
    };

    static gint *ett[] = {
        &ett_udld,
        &ett_udld_flags,
        &ett_udld_tlv
    };

    static ei_register_info ei[] = {
        { &ei_udld_tlvlength, { "udld.tlv.len.invalid", PI_PROTOCOL, PI_WARN, "TLV with invalid length (< 4)", EXPFILL }},
    };

    expert_module_t* expert_udld;

    proto_udld = proto_register_protocol("Unidirectional Link Detection",
                    "UDLD", "udld");
    proto_register_field_array(proto_udld, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
    expert_udld = expert_register_protocol(proto_udld);
    expert_register_field_array(expert_udld, ei, array_length(ei));
}

void
proto_reg_handoff_udld(void)
{
    dissector_handle_t udld_handle;

    udld_handle = create_dissector_handle(dissect_udld, proto_udld);
    dissector_add_uint("llc.cisco_pid", 0x0111, udld_handle);
    dissector_add_uint("chdlc.protocol", 0x0111, udld_handle);
}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
