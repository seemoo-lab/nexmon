/* packet-turbocell.c
 * Routines for Turbocell Header dissection
 * Copyright 2004, Colin Slater <kiltedtaco@xxxxxxxxx>
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

/* This dissector was written entirely from reverse engineering captured
 * packets. No documentation was used or supplied by Karlnet. Hence, this
 * dissector is very incomplete. If you have any insight into decoding
 * these packets, or if you can supply packet captures from turbocell
 * networks, contact kiltedtaco@xxxxxxxxx */

/* 2008-08-05 : Added support for aggregate frames.
 * AP mode, NWID and sat mode fiels identification were
 * taken from http://aphopper.sourceforge.net/turbocell.html
 * everything else is based on (educated) guesses.
*/

#include "config.h"

#include <epan/packet.h>
#include <epan/strutil.h>

#define TURBOCELL_TYPE_BEACON_NON_POLLING  0x00
#define TURBOCELL_TYPE_BEACON_NORMAL       0x40
#define TURBOCELL_TYPE_BEACON_POLLING      0x80
#define TURBOCELL_TYPE_BEACON_ISP          0xA0

#define TURBOCELL_TYPE_DATA             0x01
#define TURBOCELL_TYPE_MANAGEMENT       0x11

#define TURBOCELL_SATTELITE_MODE_DENY  0x1
#define TURBOCELL_SATTELITE_MODE_ALLOW 0x2

void proto_register_turbocell(void);
void proto_reg_handoff_turbocell(void);

static int proto_turbocell = -1;
static int proto_aggregate = -1;

static int hf_turbocell_type = -1;
static int hf_turbocell_dst = -1;
static int hf_turbocell_counter = -1;
static int hf_turbocell_name = -1;
static int hf_turbocell_nwid = -1;
static int hf_turbocell_satmode = -1;
static int hf_turbocell_unknown = -1;
static int hf_turbocell_timestamp = -1;
static int hf_turbocell_station = -1;
static int hf_turbocell_ip = -1;

static int hf_turbocell_aggregate_msdu_header_text = -1;
static int hf_turbocell_aggregate_msdu_len = -1;
static int hf_turbocell_aggregate_unknown1 = -1;
static int hf_turbocell_aggregate_unknown2 = -1;
static int hf_turbocell_aggregate_len = -1;

static gint ett_turbocell = -1;
static gint ett_network = -1;
static gint ett_msdu_aggregation_parent_tree = -1;
static gint ett_msdu_aggregation_subframe_tree = -1;

/* The ethernet dissector we hand off to */
static dissector_handle_t eth_handle;

static const value_string turbocell_type_values[] = {
    { TURBOCELL_TYPE_BEACON_NON_POLLING, "Beacon (Non-Polling Base Station)" },
    { TURBOCELL_TYPE_BEACON_NORMAL,      "Beacon (Normal Base Station)" },
    { TURBOCELL_TYPE_BEACON_POLLING,     "Beacon (Polling Base Station)" },
    { TURBOCELL_TYPE_BEACON_ISP,         "Beacon (ISP Base Station)" },
    { TURBOCELL_TYPE_DATA,               "Data Packet" },
    { TURBOCELL_TYPE_MANAGEMENT,         "Management Packet" },
    { 0, NULL }
};

static const value_string turbocell_satmode_values[] = {
    { TURBOCELL_SATTELITE_MODE_DENY,     "Allowed to connect" },
    { TURBOCELL_SATTELITE_MODE_ALLOW,    "NOT allowed to connect" },
    { 0, NULL }
};


static int
dissect_turbocell(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{

    proto_item *ti, *name_item;
    proto_tree *turbocell_tree = NULL, *network_tree;
    tvbuff_t   *next_tvb;
    int i=0;
    guint8 packet_type;
    guint8 * str_name;
    guint str_len;
    gint remaining_length;

    packet_type = tvb_get_guint8(tvb, 0);

    if (!(packet_type & 0x0F)){
        col_set_str(pinfo->cinfo, COL_INFO, "Turbocell Packet (Beacon)");
        col_set_str(pinfo->cinfo, COL_PROTOCOL, "Turbocell");
    }  else if ( packet_type == TURBOCELL_TYPE_MANAGEMENT ) {
        col_set_str(pinfo->cinfo, COL_INFO, "Turbocell Packet (Management)");
        col_set_str(pinfo->cinfo, COL_PROTOCOL, "Turbocell");
    } else if ( packet_type == TURBOCELL_TYPE_DATA ) {
        col_set_str(pinfo->cinfo, COL_INFO, "Turbocell Packet (Data)");
        col_set_str(pinfo->cinfo, COL_PROTOCOL, "Turbocell");
    } else {
        col_set_str(pinfo->cinfo, COL_INFO, "Turbocell Packet (Unknown)");
        col_set_str(pinfo->cinfo, COL_PROTOCOL, "Turbocell");
    }

    if (tree) {
        ti = proto_tree_add_item(tree, proto_turbocell, tvb, 0, 20, ENC_NA);

        turbocell_tree = proto_item_add_subtree(ti, ett_turbocell);

        proto_tree_add_item(turbocell_tree, hf_turbocell_type, tvb, 0, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item(turbocell_tree, hf_turbocell_satmode, tvb, 1, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item(turbocell_tree, hf_turbocell_nwid, tvb, 1, 1, ENC_BIG_ENDIAN);

        /* it seem when we have this magic number,that means an alternate header version */

        if (tvb_get_bits64(tvb, 64,48,ENC_BIG_ENDIAN) != G_GINT64_CONSTANT(0x000001fe23dc45ba)){
        proto_tree_add_item(turbocell_tree, hf_turbocell_counter, tvb, 0x02, 2, ENC_BIG_ENDIAN);
        proto_tree_add_item(turbocell_tree, hf_turbocell_dst, tvb, 0x04, 6, ENC_NA);
        proto_tree_add_item(turbocell_tree, hf_turbocell_timestamp, tvb, 0x0A, 3, ENC_BIG_ENDIAN);

        } else {
        proto_tree_add_item(turbocell_tree, hf_turbocell_timestamp, tvb, 0x02, 3, ENC_BIG_ENDIAN);
        proto_tree_add_item(turbocell_tree, hf_turbocell_counter, tvb, 0x05, 3, ENC_BIG_ENDIAN);
        proto_tree_add_item(turbocell_tree, hf_turbocell_dst, tvb, 0x08, 6, ENC_NA);
        }

        proto_tree_add_item(turbocell_tree, hf_turbocell_unknown, tvb, 0x0E, 2, ENC_BIG_ENDIAN);
        proto_tree_add_item(turbocell_tree, hf_turbocell_ip, tvb, 0x10, 4, ENC_BIG_ENDIAN);

    }

    remaining_length=tvb_reported_length_remaining(tvb, 0x14);

    if (remaining_length > 6) {

        /* If the first character is a printable character that means we have a payload with network info */
        /* I couldn't find anything in the header that would definitively indicate if payload is either data or network info */
        /* Since the frame size is limited this should work ok */

        if (tvb_get_guint8(tvb, 0x14)>=0x20){
            name_item = proto_tree_add_item(turbocell_tree, hf_turbocell_name, tvb, 0x14, 30, ENC_ASCII|ENC_NA);
            network_tree = proto_item_add_subtree(name_item, ett_network);

            str_name=tvb_get_stringz_enc(wmem_packet_scope(), tvb, 0x14, &str_len, ENC_ASCII);
            col_append_fstr(pinfo->cinfo, COL_INFO, ", Network=\"%s\"",format_text(str_name, str_len-1));

            while(tvb_get_guint8(tvb, 0x34 + 8*i)==0x00 && (tvb_reported_length_remaining(tvb,0x34 + 8*i) > 6) && (i<32)) {
                proto_tree_add_item(network_tree, hf_turbocell_station, tvb, 0x34+8*i, 6, ENC_NA);
                i++;
            }

            /*Couldn't make sense of the apparently random data in the end*/

            next_tvb = tvb_new_subset_remaining(tvb, 0x34 + 8*i);
            call_data_dissector(next_tvb, pinfo, tree);

        } else {

            tvbuff_t *msdu_tvb = NULL;
            guint32 msdu_offset = 0x04;
            guint16 j = 1;
            guint16 msdu_length;

            proto_item *parent_item;
            proto_tree *mpdu_tree;
            proto_tree *subframe_tree;

            next_tvb = tvb_new_subset(tvb, 0x14, -1, tvb_get_ntohs(tvb, 0x14));
            parent_item = proto_tree_add_protocol_format(tree, proto_aggregate, next_tvb, 0,
                                                         tvb_reported_length_remaining(next_tvb, 0), "Turbocell Aggregate Frames");
            mpdu_tree = proto_item_add_subtree(parent_item, ett_msdu_aggregation_parent_tree);
            proto_tree_add_item(mpdu_tree, hf_turbocell_aggregate_len, next_tvb, 0x00, 2, ENC_BIG_ENDIAN);
            proto_tree_add_item(mpdu_tree, hf_turbocell_aggregate_unknown1, next_tvb, 0x02, 2, ENC_BIG_ENDIAN);

            remaining_length=tvb_reported_length_remaining(next_tvb, msdu_offset);

            do {
                msdu_length = (tvb_get_letohs(next_tvb, msdu_offset) & 0x0FFF);
                if (msdu_length==0) break;
                parent_item = proto_tree_add_uint_format(mpdu_tree, hf_turbocell_aggregate_msdu_header_text,
                                                         next_tvb,msdu_offset, msdu_length + 0x02,j, "A-MSDU Subframe #%u", j);

                subframe_tree = proto_item_add_subtree(parent_item, ett_msdu_aggregation_subframe_tree);
                j++;

                proto_tree_add_item(subframe_tree, hf_turbocell_aggregate_msdu_len, next_tvb, msdu_offset, 2, ENC_LITTLE_ENDIAN);
                proto_tree_add_item(subframe_tree, hf_turbocell_aggregate_unknown2, next_tvb, msdu_offset+1, 1, ENC_BIG_ENDIAN);

                msdu_offset += 0x02;
                remaining_length -= 0x02;
                msdu_tvb = tvb_new_subset(next_tvb, msdu_offset, (msdu_length>remaining_length)?remaining_length:msdu_length, msdu_length);
                call_dissector(eth_handle, msdu_tvb, pinfo, subframe_tree);
                msdu_offset += msdu_length;
                remaining_length -= msdu_length;
            } while (remaining_length > 6);

            if (remaining_length > 2) {
                next_tvb = tvb_new_subset_remaining(next_tvb, msdu_offset);
                call_data_dissector(next_tvb, pinfo, tree);
            }
        }
    }
    return tvb_captured_length(tvb);
}

/* Register the protocol with Wireshark */

void proto_register_turbocell(void)
{

    static hf_register_info hf[] = {
        { &hf_turbocell_type,
          { "Packet Type", "turbocell.type",
            FT_UINT8, BASE_HEX, VALS(turbocell_type_values), 0,
            NULL, HFILL }
        },
        { &hf_turbocell_satmode,
          { "Satellite Mode", "turbocell.satmode",
            FT_UINT8, BASE_HEX, VALS(turbocell_satmode_values), 0xF0,
            NULL, HFILL }
        },
        { &hf_turbocell_nwid,
          { "Network ID", "turbocell.nwid",
            FT_UINT8, BASE_DEC, NULL, 0x0F,
            NULL, HFILL }
        },
        { &hf_turbocell_counter,
          { "Counter", "turbocell.counter",
            FT_UINT24, BASE_DEC_HEX, NULL, 0,
            "Increments every frame (per station)", HFILL }
        },
        { &hf_turbocell_dst,
          { "Destination", "turbocell.dst",
            FT_ETHER, BASE_NONE, NULL, 0,
            "Seems to be the destination", HFILL }
        },

        { &hf_turbocell_ip,
          { "IP", "turbocell.ip",
            FT_IPv4, BASE_NONE, NULL, 0,
            "IP address of base station ?", HFILL }
        },

        { &hf_turbocell_unknown,
          { "Unknown", "turbocell.unknown",
            FT_UINT16, BASE_HEX, NULL, 0,
            "Always 0000", HFILL }
        },

        { &hf_turbocell_timestamp,
          { "Timestamp (in 10 ms)", "turbocell.timestamp",
            FT_UINT24, BASE_DEC_HEX, NULL, 0,
            "Timestamp per station (since connection?)", HFILL }
        },
        { &hf_turbocell_name,
          { "Network Name", "turbocell.name",
            FT_STRINGZ, BASE_NONE, NULL, 0,
            NULL, HFILL }
        },
        { &hf_turbocell_station,
            { "Station", "turbocell.station",
            FT_ETHER, BASE_NONE, NULL, 0,
            "connected stations / satellites ?", HFILL },
        }
    };

    static hf_register_info aggregate_fields[] = {
        { &hf_turbocell_aggregate_msdu_header_text,
          {"MAC Service Data Unit (MSDU)", "turbocell_aggregate.msduheader",
           FT_UINT16, BASE_DEC, 0, 0x0000, NULL, HFILL }
        },
        { &hf_turbocell_aggregate_msdu_len,
          {"MSDU length", "turbocell_aggregate.msdulen",
           FT_UINT16, BASE_DEC_HEX, 0, 0x0FFF, NULL, HFILL }
        },
        { &hf_turbocell_aggregate_len,
          { "Total Length", "turbocell_aggregate.len",
            FT_UINT16, BASE_DEC_HEX, NULL, 0,
            "Total reported length", HFILL }
        },
        { &hf_turbocell_aggregate_unknown1,
          { "Unknown", "turbocell_aggregate.unknown1",
            FT_UINT16, BASE_HEX, NULL, 0,
            "Always 0x7856", HFILL }
        },
        { &hf_turbocell_aggregate_unknown2,
          { "Unknown", "turbocell_aggregate.unknown2",
            FT_UINT8, BASE_HEX, NULL, 0xF0,
            "have the values 0x4,0xC or 0x8", HFILL }
        },
    };

    static gint *ett[] = {
        &ett_turbocell,
        &ett_network,
        &ett_msdu_aggregation_parent_tree,
        &ett_msdu_aggregation_subframe_tree
    };

    proto_turbocell = proto_register_protocol("Turbocell Header", "Turbocell", "turbocell");

    proto_aggregate = proto_register_protocol("Turbocell Aggregate Data",
                                              "Turbocell Aggregate Data", "turbocell_aggregate");
    proto_register_field_array(proto_aggregate, aggregate_fields, array_length(aggregate_fields));

    register_dissector("turbocell", dissect_turbocell, proto_turbocell);

    proto_register_field_array(proto_turbocell, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));

}


void proto_reg_handoff_turbocell(void)
{
    eth_handle = find_dissector_add_dependency("eth_withoutfcs", proto_turbocell);
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
