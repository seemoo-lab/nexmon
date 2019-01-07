/* packet-ipdr.c
 *
 * Routines for IP Detail Record (IPDR) dissection.
 *
 * Original dissection based off of a Lua script found at
 * https://bitbucket.org/abn/ipdr-dissector/overview
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
#include "packet-tcp.h"

void proto_register_ipdr(void);
void proto_reg_handoff_ipdr(void);

static int proto_ipdr = -1;

static int hf_ipdr_version = -1;
static int hf_ipdr_message_id = -1;
static int hf_ipdr_session_id = -1;
static int hf_ipdr_message_flags = -1;
static int hf_ipdr_message_len = -1;
static int hf_ipdr_initiator_id = -1;
static int hf_ipdr_initiator_port = -1;
static int hf_ipdr_capabilities = -1;
static int hf_ipdr_keepalive_interval = -1;
static int hf_ipdr_vendor_id = -1;
static int hf_ipdr_timestamp = -1;
static int hf_ipdr_error_code = -1;
static int hf_ipdr_description = -1;
static int hf_ipdr_exporter_boot_time = -1;
static int hf_ipdr_first_record_sequence_number = -1;
static int hf_ipdr_dropped_record_count = -1;
static int hf_ipdr_reason_code = -1;
static int hf_ipdr_reason_info = -1;
static int hf_ipdr_request_id = -1;
static int hf_ipdr_config_id = -1;
static int hf_ipdr_flags = -1;
static int hf_ipdr_primary = -1;
static int hf_ipdr_ack_time_interval = -1;
static int hf_ipdr_ack_sequence_interval = -1;
static int hf_ipdr_template_id = -1;
static int hf_ipdr_document_id = -1;
static int hf_ipdr_sequence_num = -1;
static int hf_ipdr_request_number = -1;
static int hf_ipdr_data_record = -1;

static gint ett_ipdr = -1;

static expert_field ei_ipdr_message_id = EI_INIT;

#define IPDR_PORT 4737
#define IPDR_HEADER_LEN     8

enum
{
    IPDR_FLOW_START = 0x01,
    IPDR_FLOW_STOP = 0x03,
    IPDR_CONNECT = 0x05,
    IPDR_CONNECT_RESPONSE = 0x06,
    IPDR_DISCONNECT = 0x07,
    IPDR_SESSION_START = 0x08,
    IPDR_SESSION_STOP = 0x09,
    IPDR_TEMPLATE_DATA = 0x10,
    IPDR_FINAL_TEMPLATE_DATA_ACK = 0x13,
    IPDR_GET_SESSIONS = 0x14,
    IPDR_GET_SESSIONS_RESPONSE = 0x15,
    IPDR_GET_TEMPLATES = 0x16,
    IPDR_GET_TEMPLATES_RESPONSE = 0x17,
    IPDR_MODIFY_TEMPLATE = 0x1A,
    IPDR_MODIFY_TEMPLATE_RESPONSE = 0x1B,
    IPDR_START_NEGOTIATION = 0x1D,
    IPDR_START_NEGOTIATION_REJECT = 0x1E,
    IPDR_DATA = 0x20,
    IPDR_DATA_ACK = 0x21,
    IPDR_ERROR = 0x23,
    IPDR_REQUEST = 0x30,
    IPDR_RESPONSE = 0x31,
    IPDR_KEEP_ALIVE = 0x40
};

static const value_string ipdr_message_type_vals[] = {
    { IPDR_FLOW_START,              "FLOW_START" },
    { IPDR_FLOW_STOP,               "FLOW_STOP" },
    { IPDR_CONNECT,                 "CONNECT" },
    { IPDR_CONNECT_RESPONSE,        "CONNECT_RESPONSE" },
    { IPDR_DISCONNECT,              "DISCONNECT" },
    { IPDR_SESSION_START,           "SESSION_START" },
    { IPDR_SESSION_STOP,            "SESSION_STOP" },
    { IPDR_TEMPLATE_DATA,           "TEMPLATE_DATA" },
    { IPDR_FINAL_TEMPLATE_DATA_ACK, "FINAL_TEMPLATE_DATA_ACK" },
    { IPDR_GET_SESSIONS,            "GET_SESSIONS" },
    { IPDR_GET_SESSIONS_RESPONSE,   "GET_SESSIONS_RESPONSE" },
    { IPDR_GET_TEMPLATES,           "GET_TEMPLATES" },
    { IPDR_GET_TEMPLATES_RESPONSE,  "GET_TEMPLATES_RESPONSE" },
    { IPDR_MODIFY_TEMPLATE,         "MODIFY_TEMPLATE" },
    { IPDR_MODIFY_TEMPLATE_RESPONSE,"MODIFY_TEMPLATE_RESPONSE" },
    { IPDR_START_NEGOTIATION,       "START_NEGOTIATION" },
    { IPDR_START_NEGOTIATION_REJECT,"START_NEGOTIATION_REJECT" },
    { IPDR_DATA,                    "DATA" },
    { IPDR_DATA_ACK,                "DATA_ACK" },
    { IPDR_ERROR,                   "ERROR" },
    { IPDR_REQUEST,                 "REQUEST" },
    { IPDR_RESPONSE,                "RESPONSE" },
    { IPDR_KEEP_ALIVE,              "KEEP_ALIVE" },
    { 0, NULL }
};

static int
dissect_ipdr_message(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
    proto_item *ti, *type_item;
    proto_tree *ipdr_tree;
    int offset = 0;
    guint32 message_len, message_type;

    ti = proto_tree_add_item(tree, proto_ipdr, tvb, 0, -1, ENC_NA);
    ipdr_tree = proto_item_add_subtree(ti, ett_ipdr);

    proto_tree_add_item(ipdr_tree, hf_ipdr_version, tvb, offset, 1, ENC_NA);
    offset++;

    type_item = proto_tree_add_item_ret_uint(ipdr_tree, hf_ipdr_message_id, tvb, offset, 1, ENC_NA, &message_type);
    col_append_sep_str(pinfo->cinfo, COL_INFO, ", ", val_to_str(message_type, ipdr_message_type_vals, "Unknown (0x%02x)"));
    offset++;

    proto_tree_add_item(ipdr_tree, hf_ipdr_session_id, tvb, offset, 1, ENC_NA);
    offset++;

    proto_tree_add_item(ipdr_tree, hf_ipdr_message_flags, tvb, offset, 1, ENC_NA);
    offset++;

    proto_tree_add_item_ret_uint(ipdr_tree, hf_ipdr_message_len, tvb, offset, 4, ENC_BIG_ENDIAN, &message_len);
    offset += 4;

    switch(message_type)
    {
    case IPDR_FLOW_START:
    case IPDR_DISCONNECT:
    case IPDR_FINAL_TEMPLATE_DATA_ACK:
    case IPDR_START_NEGOTIATION:
    case IPDR_START_NEGOTIATION_REJECT:
    case IPDR_KEEP_ALIVE:
        /* No additional fields */
        break;
    case IPDR_FLOW_STOP:
        proto_tree_add_item(ipdr_tree, hf_ipdr_reason_code, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;
        proto_tree_add_item(ipdr_tree, hf_ipdr_reason_info, tvb, offset, -1, ENC_ASCII|ENC_NA);
        break;
    case IPDR_CONNECT:
        proto_tree_add_item(ipdr_tree, hf_ipdr_initiator_id, tvb, offset, 4, ENC_BIG_ENDIAN);
        offset += 4;
        proto_tree_add_item(ipdr_tree, hf_ipdr_initiator_port, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;
        proto_tree_add_item(ipdr_tree, hf_ipdr_capabilities, tvb, offset, 4, ENC_BIG_ENDIAN);
        offset += 4;
        proto_tree_add_item(ipdr_tree, hf_ipdr_keepalive_interval, tvb, offset, 4, ENC_BIG_ENDIAN);
        offset += 4;
        proto_tree_add_item(ipdr_tree, hf_ipdr_vendor_id, tvb, offset, 4, ENC_ASCII|ENC_BIG_ENDIAN);
        break;
    case IPDR_CONNECT_RESPONSE:
        proto_tree_add_item(ipdr_tree, hf_ipdr_capabilities, tvb, offset, 4, ENC_BIG_ENDIAN);
        offset += 4;
        proto_tree_add_item(ipdr_tree, hf_ipdr_keepalive_interval, tvb, offset, 4, ENC_BIG_ENDIAN);
        offset += 4;
        proto_tree_add_item(ipdr_tree, hf_ipdr_vendor_id, tvb, offset, 4, ENC_ASCII|ENC_BIG_ENDIAN);
        break;
    case IPDR_SESSION_START:
        proto_tree_add_item(ipdr_tree, hf_ipdr_exporter_boot_time, tvb, offset, 4, ENC_BIG_ENDIAN);
        offset += 4;
        proto_tree_add_item(ipdr_tree, hf_ipdr_first_record_sequence_number, tvb, offset, 8, ENC_BIG_ENDIAN);
        offset += 8;
        proto_tree_add_item(ipdr_tree, hf_ipdr_dropped_record_count, tvb, offset, 8, ENC_BIG_ENDIAN);
        offset += 8;
        proto_tree_add_item(ipdr_tree, hf_ipdr_primary, tvb, offset, 1, ENC_NA);
        offset++;
        proto_tree_add_item(ipdr_tree, hf_ipdr_ack_time_interval, tvb, offset, 4, ENC_BIG_ENDIAN);
        offset += 4;
        proto_tree_add_item(ipdr_tree, hf_ipdr_ack_sequence_interval, tvb, offset, 4, ENC_BIG_ENDIAN);
        offset += 4;
        proto_tree_add_item(ipdr_tree, hf_ipdr_document_id, tvb, offset, 16, ENC_NA);
        break;
    case IPDR_SESSION_STOP:
        proto_tree_add_item(ipdr_tree, hf_ipdr_reason_code, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;
        proto_tree_add_item(ipdr_tree, hf_ipdr_reason_info, tvb, offset, -1, ENC_ASCII|ENC_NA);
        break;
    case IPDR_TEMPLATE_DATA:
        proto_tree_add_item(ipdr_tree, hf_ipdr_config_id, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;
        proto_tree_add_item(ipdr_tree, hf_ipdr_flags, tvb, offset, 1, ENC_NA);
        break;
    case IPDR_GET_SESSIONS:
        proto_tree_add_item(ipdr_tree, hf_ipdr_request_id, tvb, offset, 2, ENC_BIG_ENDIAN);
        break;
    case IPDR_GET_SESSIONS_RESPONSE:
        proto_tree_add_item(ipdr_tree, hf_ipdr_request_id, tvb, offset, 2, ENC_BIG_ENDIAN);
        break;
    case IPDR_GET_TEMPLATES:
        proto_tree_add_item(ipdr_tree, hf_ipdr_request_id, tvb, offset, 2, ENC_BIG_ENDIAN);
        break;
    case IPDR_GET_TEMPLATES_RESPONSE:
        proto_tree_add_item(ipdr_tree, hf_ipdr_request_id, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;
        proto_tree_add_item(ipdr_tree, hf_ipdr_config_id, tvb, offset, 2, ENC_BIG_ENDIAN);
        break;
    case IPDR_MODIFY_TEMPLATE:
        proto_tree_add_item(ipdr_tree, hf_ipdr_config_id, tvb, offset, 2, ENC_BIG_ENDIAN);
        break;
    case IPDR_MODIFY_TEMPLATE_RESPONSE:
        proto_tree_add_item(ipdr_tree, hf_ipdr_config_id, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;
        proto_tree_add_item(ipdr_tree, hf_ipdr_flags, tvb, offset, 1, ENC_NA);
        offset++;
        break;
    case IPDR_DATA:
        proto_tree_add_item(ipdr_tree, hf_ipdr_template_id, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;
        proto_tree_add_item(ipdr_tree, hf_ipdr_config_id, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;
        proto_tree_add_item(ipdr_tree, hf_ipdr_flags, tvb, offset, 1, ENC_NA);
        offset++;
        proto_tree_add_item(ipdr_tree, hf_ipdr_sequence_num, tvb, offset, 8, ENC_BIG_ENDIAN);
        offset += 8;
        proto_tree_add_item(ipdr_tree, hf_ipdr_data_record, tvb, offset, -1, ENC_NA);
        break;
    case IPDR_DATA_ACK:
        proto_tree_add_item(ipdr_tree, hf_ipdr_config_id, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;
        proto_tree_add_item(ipdr_tree, hf_ipdr_sequence_num, tvb, offset, 8, ENC_BIG_ENDIAN);
        break;
    case IPDR_ERROR:
        proto_tree_add_item(ipdr_tree, hf_ipdr_timestamp, tvb, offset, 4, ENC_BIG_ENDIAN);
        offset += 4;
        proto_tree_add_item(ipdr_tree, hf_ipdr_error_code, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;
        proto_tree_add_item(ipdr_tree, hf_ipdr_description, tvb, offset, -1, ENC_ASCII|ENC_NA);
        break;
    case IPDR_REQUEST:
        proto_tree_add_item(ipdr_tree, hf_ipdr_template_id, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;
        proto_tree_add_item(ipdr_tree, hf_ipdr_config_id, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;
        proto_tree_add_item(ipdr_tree, hf_ipdr_flags, tvb, offset, 1, ENC_NA);
        offset++;
        proto_tree_add_item(ipdr_tree, hf_ipdr_request_number, tvb, offset, 8, ENC_BIG_ENDIAN);
        offset += 8;
        proto_tree_add_item(ipdr_tree, hf_ipdr_data_record, tvb, offset, -1, ENC_NA);
        break;
    case IPDR_RESPONSE:
        proto_tree_add_item(ipdr_tree, hf_ipdr_template_id, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;
        proto_tree_add_item(ipdr_tree, hf_ipdr_config_id, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;
        proto_tree_add_item(ipdr_tree, hf_ipdr_flags, tvb, offset, 1, ENC_NA);
        offset++;
        proto_tree_add_item(ipdr_tree, hf_ipdr_request_number, tvb, offset, 8, ENC_BIG_ENDIAN);
        offset += 8;
        proto_tree_add_item(ipdr_tree, hf_ipdr_data_record, tvb, offset, -1, ENC_NA);
        break;
    default:
        expert_add_info(pinfo, type_item, &ei_ipdr_message_id);
        break;
    }

    return tvb_captured_length(tvb);
}

static guint
get_ipdr_message_len(packet_info *pinfo _U_, tvbuff_t *tvb, int offset, void *data _U_)
{
    return (guint)tvb_get_ntohl(tvb, offset+4);
}

static int
dissect_ipdr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data)
{
    if (tvb_reported_length(tvb) < 1)
        return 0;

    if (tvb_get_guint8(tvb, 0) != 2) /* Only version 2 supported */
        return 0;

    col_set_str(pinfo->cinfo, COL_PROTOCOL, "IPDR/SP");
    col_clear(pinfo->cinfo, COL_INFO);

    tcp_dissect_pdus(tvb, pinfo, tree, TRUE, IPDR_HEADER_LEN,
                     get_ipdr_message_len, dissect_ipdr_message, data);
    return tvb_captured_length(tvb);
}

void
proto_register_ipdr(void)
{
    static hf_register_info hf[] = {
        { &hf_ipdr_version, { "Version", "ipdr.version", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL } },
        { &hf_ipdr_message_id, { "Message id", "ipdr.message_id", FT_UINT8, BASE_DEC, VALS(ipdr_message_type_vals), 0x0, NULL, HFILL } },
        { &hf_ipdr_session_id, { "Session id", "ipdr.session_id", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL } },
        { &hf_ipdr_message_flags, { "Message flags", "ipdr.message_flags", FT_UINT8, BASE_HEX, NULL, 0x0, NULL, HFILL } },
        { &hf_ipdr_message_len, { "Message length", "ipdr.message_len", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL } },
        { &hf_ipdr_initiator_id, { "Initiator id", "ipdr.initiator_id", FT_IPv4, BASE_NONE, NULL, 0x0, NULL, HFILL } },
        { &hf_ipdr_initiator_port, { "Initiator port", "ipdr.initiator_port", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL } },
        { &hf_ipdr_capabilities, { "Capabilities", "ipdr.capabilities", FT_UINT32, BASE_HEX, NULL, 0x0, NULL, HFILL } },
        { &hf_ipdr_keepalive_interval, { "Keep-alive interval", "ipdr.keepalive_interval", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL } },
        { &hf_ipdr_vendor_id, { "Vendor id", "ipdr.vendor_id", FT_UINT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL } },
        { &hf_ipdr_timestamp, { "Timestamp", "ipdr.timestamp", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL } },
        { &hf_ipdr_error_code, { "Error code", "ipdr.error_code", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL } },
        { &hf_ipdr_description, { "Description", "ipdr.description", FT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL } },
        { &hf_ipdr_exporter_boot_time, { "Exporter boot time", "ipdr.exporter_boot_time", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL } },
        { &hf_ipdr_first_record_sequence_number, { "First record sequence number", "ipdr.first_record_sequence_number", FT_UINT64, BASE_DEC, NULL, 0x0, NULL, HFILL } },
        { &hf_ipdr_dropped_record_count, { "Dropped record count", "ipdr.dropped_record_count", FT_UINT64, BASE_DEC, NULL, 0x0, NULL, HFILL } },
        { &hf_ipdr_reason_code, { "Reason code", "ipdr.reason_code", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL } },
        { &hf_ipdr_reason_info, { "Reason info", "ipdr.reason_info", FT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL } },
        { &hf_ipdr_request_id, { "Request id", "ipdr.request_id", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL } },
        { &hf_ipdr_config_id, { "Config id", "ipdr.config_id", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL } },
        { &hf_ipdr_flags, { "Flags", "ipdr.flags", FT_UINT8, BASE_HEX, NULL, 0x0, NULL, HFILL } },
        { &hf_ipdr_primary, { "Primary", "ipdr.primary", FT_BOOLEAN, 8, NULL, 0x0, NULL, HFILL } },
        { &hf_ipdr_ack_time_interval, { "ACK time interval", "ipdr.ack_time_interval", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL } },
        { &hf_ipdr_ack_sequence_interval, { "ACK sequence interval", "ipdr.ack_sequence_interval", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL } },
        { &hf_ipdr_template_id, { "Template id", "ipdr.template_id", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL } },
        { &hf_ipdr_document_id, { "Document id", "ipdr.document_id", FT_GUID, BASE_NONE, NULL, 0x0, NULL, HFILL } },
        { &hf_ipdr_sequence_num, { "Sequence number", "ipdr.sequence_num", FT_UINT64, BASE_DEC, NULL, 0x0, NULL, HFILL } },
        { &hf_ipdr_request_number, { "Request number", "ipdr.request_number", FT_UINT64, BASE_DEC, NULL, 0x0, NULL, HFILL } },
        { &hf_ipdr_data_record, { "Data record", "ipdr.data_record", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL } },
    };

    static gint *ett[] = {
        &ett_ipdr
    };

    static ei_register_info ei[] = {
        { &ei_ipdr_message_id, { "ipdr.message_id.unknown", PI_PROTOCOL, PI_WARN, "Unknown message ID", EXPFILL }},
    };

    expert_module_t* expert_ipdr;

    proto_ipdr = proto_register_protocol("IPDR", "IPDR/SP", "ipdr");

    proto_register_field_array(proto_ipdr, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
    expert_ipdr = expert_register_protocol(proto_ipdr);
    expert_register_field_array(expert_ipdr, ei, array_length(ei));
}

void
proto_reg_handoff_ipdr(void)
{
    dissector_handle_t ipdr_handle;

    ipdr_handle = create_dissector_handle(dissect_ipdr, proto_ipdr);
    dissector_add_uint("tcp.port", IPDR_PORT, ipdr_handle);
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
