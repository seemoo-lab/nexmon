/* packet-btbnep.c
 * Routines for Bluetooth BNEP dissection
 *
 * Copyright 2012, Michal Labedzki for Tieto Corporation
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
#include <epan/etypes.h>
#include <epan/expert.h>

#include "packet-bluetooth.h"
#include "packet-btl2cap.h"
#include "packet-btsdp.h"

#define BNEP_TYPE_GENERAL_ETHERNET                                          0x00
#define BNEP_TYPE_CONTROL                                                   0x01
#define BNEP_TYPE_COMPRESSED_ETHERNET                                       0x02
#define BNEP_TYPE_COMPRESSED_ETHERNET_SOURCE_ONLY                           0x03
#define BNEP_TYPE_COMPRESSED_ETHERNET_DESTINATION_ONLY                      0x04
#define RESERVED_802                                                        0x7F

static int proto_btbnep                                                    = -1;
static int hf_btbnep_bnep_type                                             = -1;
static int hf_btbnep_extension_flag                                        = -1;
static int hf_btbnep_extension_type                                        = -1;
static int hf_btbnep_extension_length                                      = -1;
static int hf_btbnep_dst                                                   = -1;
static int hf_btbnep_src                                                   = -1;
static int hf_btbnep_len                                                   = -1;
static int hf_btbnep_invalid_lentype                                       = -1;
static int hf_btbnep_type                                                  = -1;
static int hf_btbnep_addr                                                  = -1;
static int hf_btbnep_lg                                                    = -1;
static int hf_btbnep_ig                                                    = -1;
static int hf_btbnep_control_type                                          = -1;
static int hf_btbnep_unknown_control_type                                  = -1;
static int hf_btbnep_uuid_size                                             = -1;
static int hf_btbnep_destination_service_uuid                              = -1;
static int hf_btbnep_source_service_uuid                                   = -1;
static int hf_btbnep_setup_connection_response_message                     = -1;
static int hf_btbnep_filter_net_type_response_message                      = -1;
static int hf_btbnep_filter_multi_addr_response_message                    = -1;
static int hf_btbnep_list_length                                           = -1;
static int hf_btbnep_network_type_start                                    = -1;
static int hf_btbnep_network_type_end                                      = -1;
static int hf_btbnep_multicast_address_start                               = -1;
static int hf_btbnep_multicast_address_end                                 = -1;

static gint ett_btbnep                                                     = -1;
static gint ett_addr                                                       = -1;

static expert_field ei_btbnep_src_not_group_address = EI_INIT;
static expert_field ei_btbnep_invalid_lentype       = EI_INIT;
static expert_field ei_btbnep_len_past_end          = EI_INIT;

static dissector_handle_t btbnep_handle;

static gboolean top_dissect                                              = TRUE;

static dissector_handle_t llc_handle;
static dissector_handle_t ipx_handle;
static dissector_handle_t ethertype_handle;

static const true_false_string ig_tfs = {
    "Group address (multicast/broadcast)",
    "Individual address (unicast)"
};

static const true_false_string lg_tfs = {
    "Locally administered address (this is NOT the factory default)",
    "Globally unique address (factory default)"
};

static const value_string bnep_type_vals[] = {
    { 0x00,   "General Ethernet" },
    { 0x01,   "Control" },
    { 0x02,   "Compressed Ethernet" },
    { 0x03,   "Compressed Ethernet Source Only" },
    { 0x04,   "Compressed Ethernet Destination Only" },
    { 0x7F,   "Reserved for 802.2 LLC Packets for IEEE 802.15.1 WG" },
    { 0, NULL }
};

static const value_string control_type_vals[] = {
    { 0x00,   "Command Not Understood" },
    { 0x01,   "Setup Connection Request" },
    { 0x02,   "Setup Connection Response" },
    { 0x03,   "Filter Net Type Set" },
    { 0x04,   "Filter Net Type Response" },
    { 0x05,   "Filter Multi Addr Set" },
    { 0x06,   "Filter Multi Addr Response" },
    { 0, NULL }
};

static const value_string extension_type_vals[] = {
    { 0x00,   "Extension Control" },
    { 0, NULL }
};

static const value_string setup_connection_response_message_vals[] = {
    { 0x0000,   "Operation Successful" },
    { 0x0001,   "Operation FAIL: Invalid Destination Service UUID" },
    { 0x0002,   "Operation FAIL: Invalid Source Service UUID" },
    { 0x0003,   "Operation FAIL: Invalid Service UUID Size" },
    { 0x0004,   "Operation FAIL: Connection Not Allowed" },
    { 0, NULL }
};

static const value_string filter_net_type_response_message_vals[] = {
    { 0x0000,   "Operation Successful" },
    { 0x0001,   "Unsupported Request" },
    { 0x0002,   "Operation FAIL: Invalid Networking Protocol Type Range" },
    { 0x0003,   "Operation FAIL: Too many filters" },
    { 0x0004,   "Operation FAIL: Unable to fulfill request due to security reasons" },
    { 0, NULL }
};

static const value_string filter_multi_addr_response_message_vals[] = {
    { 0x0000,   "Operation Successful" },
    { 0x0001,   "Unsupported Request" },
    { 0x0002,   "Operation FAIL: Invalid Multicast Address" },
    { 0x0003,   "Operation FAIL: Too many filters" },
    { 0x0004,   "Operation FAIL: Unable to fulfill request due to security reasons" },
    { 0, NULL }
};

void proto_register_btbnep(void);
void proto_reg_handoff_btbnep(void);

static int
dissect_control(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset)
{
    proto_item  *pitem = NULL;
    guint        control_type;
    guint8       unknown_control_type;
    guint8       uuid_size;
    guint16      uuid_dst;
    guint16      uuid_src;
    guint16      response_message;
    guint16      list_length;
    guint        i_item;

    proto_tree_add_item(tree, hf_btbnep_control_type, tvb, offset, 1, ENC_BIG_ENDIAN);
    control_type = tvb_get_guint8(tvb, offset);
    offset += 1;

    col_append_fstr(pinfo->cinfo, COL_INFO, " - %s", val_to_str_const(control_type, control_type_vals,  "Unknown type"));

    switch(control_type) {
        case 0x00: /* Command Not Understood */
            proto_tree_add_item(tree, hf_btbnep_unknown_control_type, tvb, offset, 1, ENC_BIG_ENDIAN);
            unknown_control_type = tvb_get_guint8(tvb, offset);
            offset += 1;

            col_append_fstr(pinfo->cinfo, COL_INFO, " - Unknown(%s)", val_to_str_const(unknown_control_type, control_type_vals,  "Unknown type"));

            break;
        case 0x01: /* Setup Connection Request */
            proto_tree_add_item(tree, hf_btbnep_uuid_size, tvb, offset, 1, ENC_BIG_ENDIAN);
            uuid_size = tvb_get_guint8(tvb, offset);
            offset += 1;

            pitem = proto_tree_add_item(tree, hf_btbnep_destination_service_uuid, tvb, offset, uuid_size, ENC_NA);
            uuid_dst = tvb_get_ntohs(tvb, offset);
            proto_item_append_text(pitem, " (%s)", val_to_str_ext(uuid_dst, &bluetooth_uuid_vals_ext,  "Unknown uuid"));
            offset += uuid_size;

            pitem = proto_tree_add_item(tree, hf_btbnep_source_service_uuid, tvb, offset, uuid_size, ENC_NA);
            uuid_src = tvb_get_ntohs(tvb, offset);
            proto_item_append_text(pitem, " (%s)", val_to_str_ext(uuid_src, &bluetooth_uuid_vals_ext,  "Unknown uuid"));
            offset += uuid_size;

            col_append_fstr(pinfo->cinfo, COL_INFO, " - dst: <%s>, src: <%s>",
                    val_to_str_ext(uuid_dst, &bluetooth_uuid_vals_ext,  "Unknown uuid"),
                    val_to_str_ext(uuid_src, &bluetooth_uuid_vals_ext,  "Unknown uuid"));
            break;
        case 0x02: /* Setup Connection Response */
            proto_tree_add_item(tree, hf_btbnep_setup_connection_response_message, tvb, offset, 2, ENC_BIG_ENDIAN);
            response_message = tvb_get_ntohs(tvb, offset);
            offset += 2;
            col_append_fstr(pinfo->cinfo, COL_INFO, " - %s",
                    val_to_str_const(response_message, setup_connection_response_message_vals,  "Unknown response message"));
            break;
        case 0x03: /* Filter Net Type Set */
            proto_tree_add_item(tree, hf_btbnep_list_length, tvb, offset, 2, ENC_BIG_ENDIAN);
            list_length = tvb_get_ntohs(tvb, offset);
            offset += 2;

            for (i_item = 0; i_item + 4 > i_item && i_item < list_length; i_item += 4) {
                proto_tree_add_item(tree, hf_btbnep_network_type_start, tvb, offset, 2, ENC_BIG_ENDIAN);
                offset += 2;

                proto_tree_add_item(tree, hf_btbnep_network_type_end, tvb, offset, 2, ENC_BIG_ENDIAN);
                offset += 2;
            }
            break;
        case 0x04: /* Filter Net Type Response */
            proto_tree_add_item(tree, hf_btbnep_filter_net_type_response_message, tvb, offset, 2, ENC_BIG_ENDIAN);
            response_message = tvb_get_ntohs(tvb, offset);
            offset += 2;
            col_append_fstr(pinfo->cinfo, COL_INFO, " - %s",
                    val_to_str_const(response_message, filter_net_type_response_message_vals,  "Unknown response message"));
            break;
        case 0x05: /*Filter Multi Addr Set*/
            proto_tree_add_item(tree, hf_btbnep_list_length, tvb, offset, 2, ENC_BIG_ENDIAN);
            list_length = tvb_get_ntohs(tvb, offset);
            offset += 2;

            for (i_item = 0; i_item + 12 > i_item && i_item < list_length; i_item += 12) {
                proto_tree_add_item(tree, hf_btbnep_multicast_address_start, tvb, offset, 6, ENC_NA);
                offset += 6;

                proto_tree_add_item(tree, hf_btbnep_multicast_address_end, tvb, offset, 6, ENC_NA);
                offset += 6;
            }
            break;
        case 0x06: /* Filter Multi Addr Response */
            proto_tree_add_item(tree, hf_btbnep_filter_multi_addr_response_message, tvb, offset, 2, ENC_BIG_ENDIAN);
            response_message = tvb_get_ntohs(tvb, offset);
            offset += 2;
            col_append_fstr(pinfo->cinfo, COL_INFO, " - %s",
                    val_to_str_const(response_message, filter_multi_addr_response_message_vals,  "Unknown response message"));
            break;

    };

    return offset;
}

static int
dissect_extension(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset)
{
    guint8  extension_flag;
    guint8  extension_type;
    guint16 extension_length;
    guint8  type;

    proto_tree_add_item(tree, hf_btbnep_extension_type, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_btbnep_extension_flag, tvb, offset, 1, ENC_BIG_ENDIAN);
    type = tvb_get_guint8(tvb, offset);
    extension_flag = type & 0x01;
    extension_type = type >> 1;
    offset += 1;

    proto_tree_add_item(tree, hf_btbnep_extension_length, tvb, offset, 1, ENC_BIG_ENDIAN);
    extension_length = tvb_get_ntohs(tvb, offset);
    offset += 2;

    if (extension_type == 0x00) {
        /* Extension Control */
        offset = dissect_control(tvb, pinfo, tree, offset);
    } else {
        offset += extension_length;
    }

    if (extension_flag) offset = dissect_extension(tvb, pinfo, tree, offset);

    return offset;
}

static gint
dissect_btbnep(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    proto_item   *pi;
    proto_tree   *btbnep_tree;
    gint          offset = 0;
    guint         bnep_type;
    guint         extension_flag;
    guint         len_type = 0;
    proto_item   *addr_item;
    proto_tree   *addr_tree = NULL;
    proto_item   *length_ti = NULL;

    pi = proto_tree_add_item(tree, proto_btbnep, tvb, offset, -1, ENC_NA);
    btbnep_tree = proto_item_add_subtree(pi, ett_btbnep);

    col_set_str(pinfo->cinfo, COL_PROTOCOL, "BNEP");
    col_clear(pinfo->cinfo, COL_INFO);

    switch (pinfo->p2p_dir) {
        case P2P_DIR_SENT:
            col_set_str(pinfo->cinfo, COL_INFO, "Sent ");
            break;
        case P2P_DIR_RECV:
            col_set_str(pinfo->cinfo, COL_INFO, "Rcvd ");
            break;
        default:
            col_set_str(pinfo->cinfo, COL_INFO, "UnknownDirection ");
            break;
    }

    proto_tree_add_item(btbnep_tree, hf_btbnep_extension_flag, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(btbnep_tree, hf_btbnep_bnep_type, tvb, offset, 1, ENC_BIG_ENDIAN);
    bnep_type = tvb_get_guint8(tvb, offset);
    extension_flag = bnep_type & 0x80;
    bnep_type = bnep_type & 0x7F;
    offset += 1;

    col_append_str(pinfo->cinfo, COL_INFO, val_to_str_const(bnep_type, bnep_type_vals,  "Unknown type"));
    if (extension_flag) col_append_str(pinfo->cinfo, COL_INFO, "+E");

    if (bnep_type == BNEP_TYPE_GENERAL_ETHERNET || bnep_type == BNEP_TYPE_COMPRESSED_ETHERNET_DESTINATION_ONLY) {
        set_address_tvb(&pinfo->dl_dst, AT_ETHER, 6, tvb, offset);
        copy_address_shallow(&pinfo->dst, &pinfo->dl_dst);

        addr_item = proto_tree_add_item(btbnep_tree, hf_btbnep_dst, tvb, offset, 6, ENC_NA);
        addr_tree = proto_item_add_subtree(addr_item, ett_addr);
        proto_tree_add_item(addr_tree, hf_btbnep_addr, tvb, offset, 6, ENC_NA);
        proto_tree_add_item(addr_tree, hf_btbnep_lg, tvb, offset, 3, ENC_BIG_ENDIAN);
        proto_tree_add_item(addr_tree, hf_btbnep_ig, tvb, offset, 3, ENC_BIG_ENDIAN);
        offset += 6;
    }

    if (bnep_type == BNEP_TYPE_GENERAL_ETHERNET || bnep_type == BNEP_TYPE_COMPRESSED_ETHERNET_SOURCE_ONLY) {
        set_address_tvb(&pinfo->dl_src, AT_ETHER, 6, tvb, offset);
        copy_address_shallow(&pinfo->src, &pinfo->dl_src);

        addr_item = proto_tree_add_item(btbnep_tree, hf_btbnep_src, tvb, offset, 6, ENC_NA);
        addr_tree = proto_item_add_subtree(addr_item, ett_addr);
        if (tvb_get_guint8(tvb, offset) & 0x01) {
            expert_add_info(pinfo, addr_item, &ei_btbnep_src_not_group_address);
        }

        proto_tree_add_item(addr_tree, hf_btbnep_addr, tvb, offset, 6, ENC_NA);
        proto_tree_add_item(addr_tree, hf_btbnep_lg, tvb, offset, 3, ENC_BIG_ENDIAN);
        proto_tree_add_item(addr_tree, hf_btbnep_ig, tvb, offset, 3, ENC_BIG_ENDIAN);
        offset += 6;
    }

    if (bnep_type != BNEP_TYPE_CONTROL) {
        len_type = tvb_get_ntohs(tvb, offset);
        if (len_type <= IEEE_802_3_MAX_LEN) {
            /*
             * The BNEP Version 1.0 spec says, for BNEP_GENERAL_ETHERNET
             * packets, "Note: Networking Protocol Types as used in this
             * specification SHALL be taken to include values in the range
             * 0x0000-0x05dc, used to represent the IEEE802.3 length
             * interpretation of the IEEE802.3 length/type field.",
             * although it says that it's not mandatory to process
             * those packets.
             */
            length_ti = proto_tree_add_item(btbnep_tree, hf_btbnep_len, tvb, offset, 2, ENC_BIG_ENDIAN);
        } else if (len_type < ETHERNET_II_MIN_LEN) {
            /*
             * Not a valid Ethernet length, not a valid Ethernet type.
             */
            proto_item *ti;

            ti = proto_tree_add_item(btbnep_tree, hf_btbnep_invalid_lentype, tvb, offset, 2, ENC_BIG_ENDIAN);
            expert_add_info_format(pinfo, ti, &ei_btbnep_invalid_lentype,
                                   "Invalid length/type: 0x%04x (%u)",
                                   len_type, len_type);
        } else {
            /*
             * Ethernet type.
             */
            if (!top_dissect)
                proto_tree_add_item(btbnep_tree, hf_btbnep_type, tvb, offset, 2, ENC_BIG_ENDIAN);
            col_append_fstr(pinfo->cinfo, COL_INFO, " - Type: %s", val_to_str_const(len_type, etype_vals, "unknown"));
        }
        offset += 2;
    } else {
        offset = dissect_control(tvb, pinfo, btbnep_tree, offset);
    }

    if (extension_flag) {
        offset = dissect_extension(tvb, pinfo, btbnep_tree, offset);
    }

    if (bnep_type != BNEP_TYPE_CONTROL) {
        /* dissect normal network */
        if (top_dissect) {
            if (len_type <= IEEE_802_3_MAX_LEN) {
                gboolean is_802_2;
                gint reported_length;
                tvbuff_t  *next_tvb;

                /*
                 * The BNEP Version 1.0 spec says, for BNEP_GENERAL_ETHERNET
                 * packets, "Note: Networking Protocol Types as used in this
                 * specification SHALL be taken to include values in the range
                 * 0x0000-0x05dc, used to represent the IEEE802.3 length
                 * interpretation of the IEEE802.3 length/type field.",
                 * although it says that it's not mandatory to process
                 * those packets.
                 */

                /*
                 * Is there an 802.2 layer? I can tell by looking at the
                 * first 2 bytes of the payload. If they are 0xffff, then
                 * the payload is IPX.
                 *
                 * (Probably won't happen, but we might as well do this
                 * anyway.)
                 */
                is_802_2 = TRUE;

                /* Don't throw an exception for this check (even a BoundsError) */
                if (tvb_bytes_exist(tvb, offset, 2)) {
                    if (tvb_get_ntohs(tvb, offset) == 0xffff) {
                        is_802_2 = FALSE;
                    }
                }

                reported_length = tvb_reported_length_remaining(tvb, offset);

                /*
                 * Make sure the length doesn't go past the end of the
                 * payload.
                 */
                if (reported_length >= 0 && len_type > (guint)reported_length) {
                    len_type = reported_length;
                    expert_add_info(pinfo, length_ti, &ei_btbnep_len_past_end);
                }

                /* Give the next dissector only 'len_type' number of bytes. */
                next_tvb = tvb_new_subset_length(tvb, offset, len_type);
                if (is_802_2) {
                    call_dissector(llc_handle, next_tvb, pinfo, tree);
                } else {
                    call_dissector(ipx_handle, next_tvb, pinfo, tree);
                }
            } else if (len_type < ETHERNET_II_MIN_LEN) {
                /*
                 * Not a valid packet.
                 */
                tvbuff_t  *next_tvb;

                next_tvb = tvb_new_subset_remaining(tvb, offset);
                call_data_dissector(next_tvb, pinfo, tree);
            } else {
                /*
                 * Valid Ethertype.
                 */
                ethertype_data_t ethertype_data;

                ethertype_data.etype = len_type;
                ethertype_data.offset_after_ethertype = offset;
                ethertype_data.fh_tree = btbnep_tree;
                ethertype_data.etype_id = hf_btbnep_type;
                ethertype_data.trailer_id = 0;
                ethertype_data.fcs_len = 0;

                call_dissector_with_data(ethertype_handle, tvb, pinfo, tree, &ethertype_data);
            }
        } else {
            tvbuff_t  *next_tvb;

            next_tvb = tvb_new_subset_remaining(tvb, offset);
            call_data_dissector(next_tvb, pinfo, tree);
        }
    }

    return offset;
}

void
proto_register_btbnep(void)
{
    module_t *module;
    expert_module_t* expert_btbnep;

    static hf_register_info hf[] = {
        { &hf_btbnep_bnep_type,
            { "BNEP Type",                         "btbnep.bnep_type",
            FT_UINT8, BASE_HEX, VALS(bnep_type_vals), 0x7F,
            NULL, HFILL }
        },
        { &hf_btbnep_extension_flag,
            { "Extension Flag",                    "btbnep.extension_flag",
            FT_BOOLEAN, 8, NULL, 0x80,
            NULL, HFILL }
        },
        { &hf_btbnep_control_type,
            { "Control Type",                      "btbnep.control_type",
            FT_UINT8, BASE_HEX, VALS(control_type_vals), 0x00,
            NULL, HFILL }
        },
        { &hf_btbnep_extension_type,
            { "Extension Type",                    "btbnep.extension_type",
            FT_UINT8, BASE_HEX, VALS(extension_type_vals), 0x00,
            NULL, HFILL }
        },
        { &hf_btbnep_extension_length,
            { "Extension Length",                  "btbnep.extension_length",
            FT_UINT16, BASE_DEC, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btbnep_unknown_control_type,
            { "Unknown Control Type",              "btbnep.uknown_control_type",
            FT_UINT8, BASE_HEX, VALS(control_type_vals), 0x00,
            NULL, HFILL }
        },
        { &hf_btbnep_uuid_size,
            { "UIDD Size",                         "btbnep.uuid_size",
            FT_UINT8, BASE_DEC, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btbnep_destination_service_uuid,
            { "Destination Service UUID",          "btbnep.destination_service_uuid",
            FT_NONE, BASE_NONE, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btbnep_source_service_uuid,
            { "Source Service UUID",               "btbnep.source_service_uuid",
            FT_NONE, BASE_NONE, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btbnep_setup_connection_response_message,
            { "Response Message",                  "btbnep.setup_connection_response_message",
            FT_UINT16, BASE_HEX, VALS(setup_connection_response_message_vals), 0x00,
            NULL, HFILL }
        },
        { &hf_btbnep_filter_net_type_response_message,
            { "Response Message",                  "btbnep.filter_net_type_response_message",
            FT_UINT16, BASE_HEX, VALS(filter_net_type_response_message_vals), 0x00,
            NULL, HFILL }
        },
        { &hf_btbnep_filter_multi_addr_response_message,
            { "Response Message",                  "btbnep.filter_multi_addr_response_message",
            FT_UINT16, BASE_HEX, VALS(filter_multi_addr_response_message_vals), 0x00,
            NULL, HFILL }
        },
        { &hf_btbnep_list_length,
            { "List Length",                       "btbnep.list_length",
            FT_UINT16, BASE_DEC, NULL, 0x00,
            NULL, HFILL }
        },
        /* http://www.iana.org/assignments/ethernet-numbers */
        { &hf_btbnep_network_type_start,
            { "Network Protocol Type Range Start", "btbnep.network_type_start",
            FT_UINT16, BASE_HEX, VALS(etype_vals), 0x00,
            NULL, HFILL }
        },
        { &hf_btbnep_network_type_end,
            { "Network Protocol Type Range End",   "btbnep.network_type_end",
            FT_UINT16, BASE_HEX, VALS(etype_vals), 0x00,
            NULL, HFILL }
        },
        { &hf_btbnep_multicast_address_start,
            { "Multicast Address Start",           "btbnep.multicast_address_start",
            FT_ETHER, BASE_NONE, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btbnep_multicast_address_end,
            { "Multicast Address End",             "btbnep.multicast_address_end",
            FT_ETHER, BASE_NONE, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btbnep_dst,
            { "Destination",                       "btbnep.dst",
            FT_ETHER, BASE_NONE, NULL, 0x0,
            "Destination Hardware Address", HFILL }
        },
        { &hf_btbnep_src,
            { "Source",                            "btbnep.src",
            FT_ETHER, BASE_NONE, NULL, 0x0,
            "Source Hardware Address", HFILL }
        },
        { &hf_btbnep_len,
            { "Length",                            "btbnep.len",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_btbnep_invalid_lentype,
            { "Invalid length/type",               "btbnep.invalid_lentype",
            FT_UINT16, BASE_HEX_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_btbnep_type,
            { "Type",                              "btbnep.type",
            FT_UINT16, BASE_HEX, VALS(etype_vals), 0x0,
            NULL, HFILL }
        },
        { &hf_btbnep_addr,
            { "Address",                           "btbnep.addr",
            FT_ETHER, BASE_NONE, NULL, 0x0,
            "Source or Destination Hardware Address", HFILL }
        },
        { &hf_btbnep_lg,
            { "LG bit",                            "btbnep.lg",
            FT_BOOLEAN, 24, TFS(&lg_tfs), 0x020000,
            "Specifies if this is a locally administered or globally unique (IEEE assigned) address", HFILL }
        },
        { &hf_btbnep_ig,
            { "IG bit",                            "btbnep.ig",
            FT_BOOLEAN, 24, TFS(&ig_tfs), 0x010000,
            "Specifies if this is an individual (unicast) or group (broadcast/multicast) address", HFILL }
        }
    };

    static gint *ett[] = {
        &ett_btbnep,
        &ett_addr
    };

    static ei_register_info ei[] = {
        { &ei_btbnep_src_not_group_address, { "btbnep.src.not_group_address", PI_PROTOCOL, PI_WARN, "Source MAC must not be a group address: IEEE 802.3-2002, Section 3.2.3(b)", EXPFILL }},
        { &ei_btbnep_invalid_lentype, { "btbnep.invalid_lentype.expert", PI_PROTOCOL, PI_WARN, "Invalid length/type", EXPFILL }},
        { &ei_btbnep_len_past_end, { "btbnep.len.past_end", PI_MALFORMED, PI_ERROR, "Length field value goes past the end of the payload", EXPFILL }},
    };

    proto_btbnep = proto_register_protocol("Bluetooth BNEP Protocol", "BT BNEP", "btbnep");
    btbnep_handle = register_dissector("btbnep", dissect_btbnep, proto_btbnep);

    proto_register_field_array(proto_btbnep, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
    expert_btbnep = expert_register_protocol(proto_btbnep);
    expert_register_field_array(expert_btbnep, ei, array_length(ei));

    module = prefs_register_protocol(proto_btbnep, NULL);
    prefs_register_static_text_preference(module, "bnep.version",
            "Bluetooth Protocol BNEP version: 1.0",
            "Version of protocol supported by this dissector.");

    prefs_register_bool_preference(module, "bnep.top_dissect",
            "Dissecting the top protocols", "Dissecting the top protocols",
            &top_dissect);
}

void
proto_reg_handoff_btbnep(void)
{
    ipx_handle    = find_dissector_add_dependency("ipx", proto_btbnep);
    llc_handle    = find_dissector_add_dependency("llc", proto_btbnep);
    ethertype_handle = find_dissector_add_dependency("ethertype", proto_btbnep);

    dissector_add_string("bluetooth.uuid", "1115", btbnep_handle);
    dissector_add_string("bluetooth.uuid", "1116", btbnep_handle);
    dissector_add_string("bluetooth.uuid", "1117", btbnep_handle);

    dissector_add_uint("btl2cap.psm", BTL2CAP_PSM_BNEP, btbnep_handle);
    dissector_add_for_decode_as("btl2cap.cid", btbnep_handle);
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
