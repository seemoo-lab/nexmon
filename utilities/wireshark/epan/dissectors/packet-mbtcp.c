/* packet-mbtcp.c
 * Routines for Modbus/TCP and Modbus/UDP dissection
 * By Riaan Swart <rswart@cs.sun.ac.za>
 * Copyright 2001, Institute for Applied Computer Science
 *                   University of Stellenbosch
 *
 * See http://www.modbus.org/ for information on Modbus/TCP.
 *
 * Updated to v1.1b of the Modbus Application Protocol specification
 *   Michael Mann * Copyright 2011
 *
 *****************************************************************************************************
 * A brief explanation of the distinction between Modbus/TCP and Modbus RTU over TCP:
 *
 * Consider a Modbus poll message: Unit 01, Scan Holding Register Address 0 for 30 Registers
 *
 * The Modbus/TCP message structure will follow the pattern below:
 * 00 00 00 00 00 06 01 03 00 00 00 1E
 * AA AA BB BB CC CC DD EE FF FF GG GG
 *
 * A = 16-bit Transaction Identifier (typically increments, or is locked at zero)
 * B = 16-bit Protocol Identifier (typically zero)
 * C = 16-bit Length of data payload following (and inclusive of) the length byte
 * D = 8-bit Unit / Slave ID
 * E = 8-bit Modbus Function Code
 * F = 16-bit Reference Number / Register Base Address
 * G = 16-bit Word Count / Number of Registers to scan
 *
 * A identical Modbus RTU (or Modbus RTU over TCP) message will overlay partially with the msg above
 * and contain 16-bit CRC at the end:
 * 00 00 00 00 00 06 01 03 00 00 00 1E -- -- (Modbus/TCP message, repeated from above)
 * -- -- -- -- -- -- 01 03 00 00 00 1E C5 C2 (Modbus RTU over TCP message, includes 16-bit CRC footer)
 * AA AA BB BB CC CC DD EE FF FF GG GG HH HH
 *
 * A = Not present in Modbus RTU message
 * B = Not present in Modbus RTU message
 * C = Not present in Modbus RTU message
 * D = 8-bit Unit / Slave ID
 * E = 8-bit Modbus Function Code
 * F = 16-bit Reference Number / Register Base Address
 * G = 16-bit Word Count / Number of Registers to scan
 * H = 16-bit CRC
 *
 *****************************************************************************************************
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
#include "packet-tcp.h"
#include "packet-mbtcp.h"
#include <epan/prefs.h>
#include <epan/expert.h>
#include <epan/crc16-tvb.h> /* For CRC verification */
#include <epan/proto_data.h>

void proto_register_modbus(void);
void proto_reg_handoff_mbtcp(void);
void proto_reg_handoff_mbrtu(void);

/* Initialize the protocol and registered fields */
static int proto_mbtcp = -1;
static int proto_mbrtu = -1;
static int proto_modbus = -1;
static int hf_mbtcp_transid = -1;
static int hf_mbtcp_protid = -1;
static int hf_mbtcp_len = -1;
static int hf_mbtcp_unitid = -1;
static int hf_modbus_request_frame = -1;
static int hf_modbus_functioncode = -1;
static int hf_modbus_reference = -1;
static int hf_modbus_padding = -1;
static int hf_modbus_lreference = -1;
static int hf_modbus_reftype = -1;
static int hf_modbus_readref = -1;
static int hf_modbus_writeref = -1;
static int hf_modbus_wordcnt = -1;
static int hf_modbus_readwordcnt = -1;
static int hf_modbus_writewordcnt = -1;
static int hf_modbus_bytecnt = -1;
static int hf_modbus_lbytecnt = -1;
static int hf_modbus_bitcnt = -1;
static int hf_modbus_exceptioncode = -1;
static int hf_modbus_diag_sf = -1;
static int hf_modbus_diag_return_query_data_request = -1;
static int hf_modbus_diag_return_query_data_echo = -1;
static int hf_modbus_diag_restart_communication_option = -1;
static int hf_modbus_diag_return_diag_register = -1;
static int hf_modbus_diag_ascii_input_delimiter = -1;
static int hf_modbus_diag_clear_ctr_diag_reg = -1;
static int hf_modbus_diag_return_bus_message_count = -1;
static int hf_modbus_diag_return_bus_comm_error_count = -1;
static int hf_modbus_diag_return_bus_exception_error_count = -1;
static int hf_modbus_diag_return_slave_message_count = -1;
static int hf_modbus_diag_return_no_slave_response_count = -1;
static int hf_modbus_diag_return_slave_nak_count = -1;
static int hf_modbus_diag_return_slave_busy_count = -1;
static int hf_modbus_diag_return_bus_char_overrun_count = -1;
static int hf_modbus_status = -1;
static int hf_modbus_event = -1;
static int hf_modbus_event_count = -1;
static int hf_modbus_message_count = -1;
static int hf_modbus_event_recv_comm_err = -1;
static int hf_modbus_event_recv_char_over = -1;
static int hf_modbus_event_recv_lo_mode = -1;
static int hf_modbus_event_recv_broadcast = -1;
static int hf_modbus_event_send_read_ex = -1;
static int hf_modbus_event_send_slave_abort_ex = -1;
static int hf_modbus_event_send_slave_busy_ex = -1;
static int hf_modbus_event_send_slave_nak_ex = -1;
static int hf_modbus_event_send_write_timeout = -1;
static int hf_modbus_event_send_lo_mode = -1;
static int hf_modbus_andmask = -1;
static int hf_modbus_ormask = -1;
static int hf_modbus_data = -1;
static int hf_modbus_mei = -1;
static int hf_modbus_read_device_id = -1;
static int hf_modbus_object_id = -1;
static int hf_modbus_num_objects = -1;
static int hf_modbus_list_object_len = -1;
static int hf_modbus_conformity_level = -1;
static int hf_modbus_more_follows = -1;
static int hf_modbus_next_object_id = -1;
static int hf_modbus_object_str_value = -1;
static int hf_modbus_object_value = -1;
static int hf_modbus_reg16 = -1;
static int hf_modbus_reg32 = -1;
static int hf_mbrtu_unitid = -1;
static int hf_mbrtu_crc16 = -1;

/* Initialize the subtree pointers */
static gint ett_mbtcp = -1;
static gint ett_mbrtu = -1;
static gint ett_modbus_hdr = -1;
static gint ett_group_hdr = -1;
static gint ett_events = -1;
static gint ett_events_recv = -1;
static gint ett_events_send = -1;
static gint ett_device_id_objects = -1;
static gint ett_device_id_object_items = -1;

static expert_field ei_mbrtu_crc16_incorrect = EI_INIT;
static expert_field ei_modbus_data_decode = EI_INIT;
static expert_field ei_mbtcp_cannot_classify = EI_INIT;

static dissector_handle_t modbus_handle;
static dissector_handle_t mbtcp_handle;
static dissector_handle_t mbrtu_handle;

static dissector_table_t   modbus_data_dissector_table;
static dissector_table_t   modbus_dissector_table;


/* Globals for Modbus/TCP Preferences */
static gboolean mbtcp_desegment = TRUE;
static guint global_mbus_tcp_port = PORT_MBTCP; /* Port 502, by default */

/* Globals for Modbus RTU over TCP Preferences */
static gboolean mbrtu_desegment = TRUE;
static guint global_mbus_rtu_port = PORT_MBRTU; /* 0, by default        */
static gboolean mbrtu_crc = FALSE;

/* Globals for Modbus Preferences */
static gint global_mbus_register_format = MODBUS_PREF_REGISTER_FORMAT_UINT16;

static int
classify_mbtcp_packet(packet_info *pinfo)
{
    /* see if nature of packets can be derived from src/dst ports */
    /* if so, return as found */
    /*                        */
    /* XXX Update Oct 2012 - It can be difficult to determine if a packet is a query or response; some way to track  */
    /* the Modbus/TCP transaction ID for each pair of messages would allow for detection based on a new seq. number. */
    /* Otherwise, we can stick with this method; a configurable port option has been added to allow for usage of     */
    /* user ports either than the default of 502.                                                                    */
    if (( pinfo->srcport == global_mbus_tcp_port ) && ( pinfo->destport != global_mbus_tcp_port ))
        return RESPONSE_PACKET;
    if (( pinfo->srcport != global_mbus_tcp_port ) && ( pinfo->destport == global_mbus_tcp_port ))
        return QUERY_PACKET;

    /* else, cannot classify */
    return CANNOT_CLASSIFY;
}

static int
classify_mbrtu_packet(packet_info *pinfo, tvbuff_t *tvb)
{
    guint8 func, len;

    func = tvb_get_guint8(tvb, 1);
    len = tvb_reported_length(tvb);

    /* see if nature of packets can be derived from src/dst ports */
    /* if so, return as found */
    if (( pinfo->srcport == global_mbus_rtu_port ) && ( pinfo->destport != global_mbus_rtu_port ))
        return RESPONSE_PACKET;
    if (( pinfo->srcport != global_mbus_rtu_port ) && ( pinfo->destport == global_mbus_rtu_port ))
        return QUERY_PACKET;


    /* We may not have an Ethernet header or unique ports. */
    /* Dig into these a little deeper to try to guess the message type */

    /* The 'exception' bit is set, so this is a response */
    if (func & 0x80) {
        return RESPONSE_PACKET;
    }
    switch (func) {
        case READ_COILS:
        case READ_DISCRETE_INPUTS:
            /* Only possible to get a response message of 8 bytes with Discrete or Coils */
            if (len == 8) {
                /* If this is, in fact, a response then the data byte count will be 3 */
                /* This will correctly identify all messages except for those that are discrete or coil polls */
                /* where the base address range happens to have 0x03 in the upper 16-bit address register     */
                if (tvb_get_guint8(tvb, 2) == 3) {
                    return RESPONSE_PACKET;
                }
                else {
                    return QUERY_PACKET;
                }
            }
            else {
                return RESPONSE_PACKET;
            }
            break;

        case READ_HOLDING_REGS:
        case READ_INPUT_REGS:
        case WRITE_SINGLE_COIL:
        case WRITE_SINGLE_REG:
            if (len == 8) {
                return QUERY_PACKET;
            }
            else {
                return RESPONSE_PACKET;
            }
            break;

        case WRITE_MULT_REGS:
        case WRITE_MULT_COILS:
            if (len == 8) {
                return RESPONSE_PACKET;
            }
            else {
                return QUERY_PACKET;
            }
            break;
    }


    /* else, cannot classify */
    return CANNOT_CLASSIFY;
}

/* Translate function to string, as given on p6 of
 * "Open Modbus/TCP Specification", release 1 by Andy Swales.
 */
static const value_string function_code_vals[] = {
    { READ_COILS,             "Read Coils" },
    { READ_DISCRETE_INPUTS,   "Read Discrete Inputs" },
    { READ_HOLDING_REGS,      "Read Holding Registers" },
    { READ_INPUT_REGS,        "Read Input Registers" },
    { WRITE_SINGLE_COIL,      "Write Single Coil" },
    { WRITE_SINGLE_REG,       "Write Single Register" },
    { READ_EXCEPT_STAT,       "Read Exception Status" },
    { DIAGNOSTICS,            "Diagnostics" },
    { GET_COMM_EVENT_CTRS,    "Get Comm. Event Counters" },
    { GET_COMM_EVENT_LOG,     "Get Comm. Event Log" },
    { WRITE_MULT_COILS,       "Write Multiple Coils" },
    { WRITE_MULT_REGS,        "Write Multiple Registers" },
    { REPORT_SLAVE_ID,        "Report Slave ID" },
    { READ_FILE_RECORD,       "Read File Record" },
    { WRITE_FILE_RECORD,      "Write File Record" },
    { MASK_WRITE_REG,         "Mask Write Register" },
    { READ_WRITE_REG,         "Read Write Register" },
    { READ_FIFO_QUEUE,        "Read FIFO Queue" },
    { ENCAP_INTERFACE_TRANSP, "Encapsulated Interface Transport" },
    { UNITY_SCHNEIDER,        "Unity (Schneider)" },
    { 0,                      NULL }
};

/* Translate exception code to string */
static const value_string exception_code_vals[] = {
    { ILLEGAL_FUNCTION,    "Illegal function" },
    { ILLEGAL_ADDRESS,     "Illegal data address" },
    { ILLEGAL_VALUE,       "Illegal data value" },
    { SLAVE_FAILURE,       "Slave device failure" },
    { ACKNOWLEDGE,         "Acknowledge" },
    { SLAVE_BUSY,          "Slave device busy" },
    { MEMORY_ERR,          "Memory parity error" },
    { GATEWAY_UNAVAILABLE, "Gateway path unavailable" },
    { GATEWAY_TRGT_FAIL,   "Gateway target device failed to respond" },
    { 0,                    NULL }
};

/* Translate Modbus Encapsulation Interface (MEI) code to string */
static const value_string encap_interface_code_vals[] = {
    { CANOPEN_REQ_RESP, "CANopen Request/Response " },
    { READ_DEVICE_ID,   "Read Device Identification" },
    { 0,                NULL }
};

/* Translate Modbus Diagnostic subfunction code to string */
static const value_string diagnostic_code_vals[] = {
    { RETURN_QUERY_DATA,                "Return Query Data" },
    { RESTART_COMMUNICATION_OPTION,     "Restart Communications Option" },
    { RETURN_DIAGNOSTIC_REGISTER,       "Return Diagnostic Register" },
    { CHANGE_ASCII_INPUT_DELIMITER,     "Change ASCII Input Delimiter" },
    { FORCE_LISTEN_ONLY_MODE,           "Force Listen Only Mode" },
    { CLEAR_COUNTERS_AND_DIAG_REG,      "Clear Counters and Diagnostic Register" },
    { RETURN_BUS_MESSAGE_COUNT,         "Return Bus Message Count" },
    { RETURN_BUS_COMM_ERROR_COUNT,      "Return Bus Communication Error Count" },
    { RETURN_BUS_EXCEPTION_ERROR_COUNT, "Return Bus Exception Error Count" },
    { RETURN_SLAVE_MESSAGE_COUNT,       "Return Slave Message Count" },
    { RETURN_SLAVE_NO_RESPONSE_COUNT,   "Return Slave No Response Count" },
    { RETURN_SLAVE_NAK_COUNT,           "Return Slave NAK Count" },
    { RETURN_SLAVE_BUSY_COUNT,          "Return Slave Busy Count" },
    { RETURN_BUS_CHAR_OVERRUN_COUNT,    "Return Bus Character Overrun Count" },
    { CLEAR_OVERRUN_COUNTER_AND_FLAG,   "Clear Overrun Counter and Flag" },
    { 0,                                NULL }
};

static const value_string diagnostic_restart_communication_option_vals[] = {
    { 0,        "Leave Log" },
    { 0xFF,     "Clear Log" },
    { 0,        NULL }
};

/* Translate read device code to string */
static const value_string read_device_id_vals[] = {
    { 1,        "Basic Device Identification" },
    { 2,        "Regular Device Identification"  },
    { 3,        "Extended Device Identification"  },
    { 4,        "Specific Identification Object"  },

    { 0,        NULL             }
};

/* Translate read device code to string */
static const value_string object_id_vals[] = {
    { 0,        "VendorName" },
    { 1,        "ProductCode" },
    { 2,        "MajorMinorRevision"  },
    { 3,        "VendorURL"  },
    { 4,        "ProductName"  },
    { 5,        "ModelName"  },
    { 6,        "UserApplicationName"  },

    { 0,        NULL             }
};

static const value_string conformity_level_vals[] = {
    { 0x01,     "Basic Device Identification (stream)" },
    { 0x02,     "Regular Device Identification (stream)"  },
    { 0x03,     "Extended Device Identification (stream)"  },
    { 0x81,     "Basic Device Identification (stream and individual)" },
    { 0x82,     "Regular Device Identification (stream and individual)"  },
    { 0x83,     "Extended Device Identification (stream and individual)"  },

    { 0,        NULL             }
};

static const enum_val_t mbus_register_format[] = {
  { "UINT16     ", "UINT16     ",  MODBUS_PREF_REGISTER_FORMAT_UINT16  },
  { "INT16      ", "INT16      ",  MODBUS_PREF_REGISTER_FORMAT_INT16   },
  { "UINT32     ", "UINT32     ",  MODBUS_PREF_REGISTER_FORMAT_UINT32  },
  { "INT32      ", "INT32      ",  MODBUS_PREF_REGISTER_FORMAT_INT32  },
  { "IEEE FLT   ", "IEEE FLT   ",  MODBUS_PREF_REGISTER_FORMAT_IEEE_FLOAT  },
  { "MODICON FLT", "MODICON FLT",  MODBUS_PREF_REGISTER_FORMAT_MODICON_FLOAT  },
  { NULL, NULL, 0 }
};

/* Code to dissect Modbus/TCP packets */
static int
dissect_mbtcp_pdu(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
/* Set up structures needed to add the protocol subtree and manage it */
    proto_item    *mi;
    proto_tree    *mbtcp_tree;
    int           offset, packet_type;
    tvbuff_t      *next_tvb;
    const char    *func_string = "";
    const char    *pkt_type_str = "";
    const char    *err_str = "";
    guint16       transaction_id, protocol_id, len;
    guint8        unit_id, function_code, exception_code, subfunction_code;

    /* Make entries in Protocol column on summary display */
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "Modbus/TCP");
    col_clear(pinfo->cinfo, COL_INFO);

    transaction_id = tvb_get_ntohs(tvb, 0);
    protocol_id = tvb_get_ntohs(tvb, 2);
    len = tvb_get_ntohs(tvb, 4);

    unit_id = tvb_get_guint8(tvb, 6);
    function_code = tvb_get_guint8(tvb, 7) & 0x7F;

    offset = 0;

    /* "Request" or "Response" */
    packet_type = classify_mbtcp_packet(pinfo);

    switch ( packet_type ) {
        case QUERY_PACKET :
            pkt_type_str="Query";
            break;
        case RESPONSE_PACKET :
            pkt_type_str="Response";
            break;
        case CANNOT_CLASSIFY :
            err_str="Unable to classify as query or response.";
            pkt_type_str="unknown";
            break;
        default :
            break;
    }

    /* Find exception - last bit set in function code */
    if (tvb_get_guint8(tvb, 7) & 0x80) {
        exception_code = tvb_get_guint8(tvb, offset + 8);
    }
    else {
        exception_code = 0;
    }

    if ((function_code == ENCAP_INTERFACE_TRANSP) && (exception_code == 0))  {
        func_string = val_to_str_const(tvb_get_guint8(tvb, offset + 8), encap_interface_code_vals, "Encapsulated Interface Transport");
        subfunction_code = 1;
    }
    else if ((function_code == DIAGNOSTICS) && (exception_code == 0))  {
        func_string = val_to_str_const(tvb_get_ntohs(tvb, offset + 8), diagnostic_code_vals, "Diagnostics");
        subfunction_code = 1;
    }
    else {
        func_string = val_to_str(function_code, function_code_vals, "Unknown function (%d)");
        subfunction_code = 0;
    }

    if ( exception_code != 0 )
        err_str="Exception returned ";

    /* Make entries in Info column on summary display */
    if (subfunction_code == 0) {
        if (strlen(err_str) > 0) {
            col_add_fstr(pinfo->cinfo, COL_INFO,
                    "%8s: Trans: %5u; Unit: %3u, Func: %3u: %s. %s",
                    pkt_type_str, transaction_id, unit_id,
                    function_code, func_string, err_str);
        }
        else {
            col_add_fstr(pinfo->cinfo, COL_INFO,
                    "%8s: Trans: %5u; Unit: %3u, Func: %3u: %s",
                    pkt_type_str, transaction_id, unit_id,
                    function_code, func_string);
        }
    }
    else {
        if (strlen(err_str) > 0) {
            col_add_fstr(pinfo->cinfo, COL_INFO,
                    "%8s: Trans: %5u; Unit: %3u, Func: %3u/%3u: %s. %s",
                    pkt_type_str, transaction_id, unit_id,
                    function_code, subfunction_code, func_string, err_str);
        }
        else {
            col_add_fstr(pinfo->cinfo, COL_INFO,
                    "%8s: Trans: %5u; Unit: %3u, Func: %3u/%3u: %s",
                    pkt_type_str, transaction_id, unit_id,
                    function_code, subfunction_code, func_string);
        }
    }

    /* Create protocol tree */
    mi = proto_tree_add_protocol_format(tree, proto_mbtcp, tvb, offset,
            len+6, "Modbus/TCP");
    mbtcp_tree = proto_item_add_subtree(mi, ett_mbtcp);

    if (packet_type == CANNOT_CLASSIFY)
        expert_add_info(pinfo, mi, &ei_mbtcp_cannot_classify);

    /* Add items to protocol tree specific to Modbus/TCP */
    proto_tree_add_uint(mbtcp_tree, hf_mbtcp_transid, tvb, offset, 2, transaction_id);
    proto_tree_add_uint(mbtcp_tree, hf_mbtcp_protid, tvb, offset + 2, 2, protocol_id);
    proto_tree_add_uint(mbtcp_tree, hf_mbtcp_len, tvb, offset + 4, 2, len);
    proto_tree_add_uint(mbtcp_tree, hf_mbtcp_unitid, tvb, offset + 6, 1, unit_id);

    /* dissect the Modbus PDU */
    next_tvb = tvb_new_subset_length( tvb, offset+7, len-1);

    /* Continue with dissection of Modbus data payload following Modbus/TCP frame */
    if( tvb_reported_length_remaining(tvb, offset) > 0 )
        call_dissector_with_data(modbus_handle, next_tvb, pinfo, tree, &packet_type);

    return tvb_captured_length(tvb);
}

/* Code to dissect Modbus RTU over TCP packets */
static int
dissect_mbrtu_pdu(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
/* Set up structures needed to add the protocol subtree and manage it */
    proto_item    *mi;
    proto_tree    *mbrtu_tree;
    int           offset, packet_type;
    tvbuff_t      *next_tvb;
    const char    *func_string = "";
    const char    *pkt_type_str = "";
    const char    *err_str = "";
    guint16       len, calc_crc16;
    guint8        unit_id, function_code, exception_code, subfunction_code;

    /* Make entries in Protocol column on summary display */
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "Modbus RTU");
    col_clear(pinfo->cinfo, COL_INFO);

    len = tvb_reported_length(tvb);

    unit_id = tvb_get_guint8(tvb, 0);
    function_code = tvb_get_guint8(tvb, 1) & 0x7F;

    offset = 0;

    /* "Request" or "Response" */
    packet_type = classify_mbrtu_packet(pinfo, tvb);

    switch ( packet_type ) {
        case QUERY_PACKET :
            pkt_type_str="Query";
            break;
        case RESPONSE_PACKET :
            pkt_type_str="Response";
            break;
        case CANNOT_CLASSIFY :
            err_str="Unable to classify as query or response.";
            pkt_type_str="unknown";
            break;
        default :
            break;
    }

    /* Find exception - last bit set in function code */
    if (tvb_get_guint8(tvb, 1) & 0x80) {
        exception_code = tvb_get_guint8(tvb, offset + 2);
    }
    else {
        exception_code = 0;
    }

    if ((function_code == ENCAP_INTERFACE_TRANSP) && (exception_code == 0))  {
        func_string = val_to_str_const(tvb_get_guint8(tvb, offset + 2), encap_interface_code_vals, "Encapsulated Interface Transport");
        subfunction_code = 1;
    }
    else if ((function_code == DIAGNOSTICS) && (exception_code == 0))  {
        func_string = val_to_str_const(tvb_get_ntohs(tvb, offset + 2), diagnostic_code_vals, "Diagnostics");
        subfunction_code = 1;
    }
    else {
        func_string = val_to_str(function_code, function_code_vals, "Unknown function (%d)");
        subfunction_code = 0;
    }

    if ( exception_code != 0 )
        err_str="Exception returned ";

    /* Make entries in Info column on summary display */
    if (subfunction_code == 0) {
        if (strlen(err_str) > 0) {
            col_add_fstr(pinfo->cinfo, COL_INFO,
                    "%8s: Unit: %3u, Func: %3u: %s. %s",
                    pkt_type_str, unit_id,
                    function_code, func_string, err_str);
        }
        else {
            col_add_fstr(pinfo->cinfo, COL_INFO,
                    "%8s: Unit: %3u, Func: %3u: %s",
                    pkt_type_str, unit_id,
                    function_code, func_string);
        }
    }
    else {
        if (strlen(err_str) > 0) {
            col_add_fstr(pinfo->cinfo, COL_INFO,
                    "%8s: Unit: %3u, Func: %3u/%3u: %s. %s",
                    pkt_type_str, unit_id,
                    function_code, subfunction_code, func_string, err_str);
        }
        else {
            col_add_fstr(pinfo->cinfo, COL_INFO,
                    "%8s: Unit: %3u, Func: %3u/%3u: %s",
                    pkt_type_str, unit_id,
                    function_code, subfunction_code, func_string);
        }
    }

    /* Create protocol tree */
    mi = proto_tree_add_protocol_format(tree, proto_mbrtu, tvb, offset,
            len, "Modbus RTU");
    mbrtu_tree = proto_item_add_subtree(mi, ett_mbrtu);

    /* Add items to protocol tree specific to Modbus RTU */
    proto_tree_add_uint(mbrtu_tree, hf_mbrtu_unitid, tvb, offset, 1, unit_id);

    /* CRC validation */
    if (mbrtu_crc)
    {
        calc_crc16 = crc16_plain_tvb_offset_seed(tvb, offset, len-2, 0xFFFF);
        proto_tree_add_checksum(mbrtu_tree, tvb, len-2, hf_mbrtu_crc16, -1, &ei_mbrtu_crc16_incorrect, pinfo, g_htons(calc_crc16), ENC_BIG_ENDIAN, PROTO_CHECKSUM_VERIFY);
    }
    else
    {
        proto_tree_add_checksum(mbrtu_tree, tvb, len-2, hf_mbrtu_crc16, -1, &ei_mbrtu_crc16_incorrect, pinfo, 0, ENC_BIG_ENDIAN, PROTO_CHECKSUM_NO_FLAGS);
    }

    /* when determining payload length, make sure to ignore the unit ID header & CRC-16 footer bytes */
    len = len - 3;

    /* dissect the Modbus PDU                      */
    next_tvb = tvb_new_subset_length( tvb, offset+1, len);

    /* Continue with dissection of Modbus data payload following Modbus RTU frame */
    if( tvb_reported_length_remaining(tvb, offset) > 0 )
        call_dissector_with_data(modbus_handle, next_tvb, pinfo, tree, &packet_type);

    return tvb_captured_length(tvb);
}


/* Return length of Modbus/TCP message */
static guint
get_mbtcp_pdu_len(packet_info *pinfo _U_, tvbuff_t *tvb, int offset, void *data _U_)
{
    guint16 plen;

    /*
     * Get the length of the data from the encapsulation header.
     */
    plen = tvb_get_ntohs(tvb, offset + 4);

    /*
     * That length doesn't include the encapsulation header itself;
     * add that in.
     */
    return plen + 6;
}

/* Return length of Modbus RTU over TCP message */
static guint
get_mbrtu_pdu_len(packet_info *pinfo _U_, tvbuff_t *tvb,
                  int offset _U_, void *data _U_)
{
    int packet_type;
    guint8 function_code;

    function_code = tvb_get_guint8(tvb, 1);

    /* Modbus RTU requests do not contain a length field but they are typically a consistent size.
       Responses do contain a usable 'length' byte at offset 2
       XXX - Note that only some function codes are supported by this lookup function;
             the rest can be added as pcap examples are made available */

    /* Determine "Query" or "Response" */
    packet_type = classify_mbrtu_packet(pinfo, tvb);

    switch ( packet_type ) {
        case QUERY_PACKET :
            switch (function_code) {
                case READ_COILS:   /* Query messages of these types are always 8 bytes */
                case READ_DISCRETE_INPUTS:
                case READ_HOLDING_REGS:
                case READ_INPUT_REGS:
                case WRITE_SINGLE_COIL:
                case WRITE_SINGLE_REG:
                    return 8;
                    break;
                case WRITE_MULT_REGS:
                case WRITE_MULT_COILS:
                    return tvb_get_guint8(tvb, 6) + 9; /* Reported size does not include 2 header, 4 FC15/16-specific, 1 size byte or 2 CRC16 bytes */
                    break;
                default :
                    return tvb_captured_length(tvb);  /* Fall back on tvb length */
                    break;
            }
        case RESPONSE_PACKET :
            /* The 'exception' bit is set, so this is a 5-byte response */
            if (function_code & 0x80) {
                return 5;
            }

            switch (function_code) {
                case READ_COILS:
                case READ_DISCRETE_INPUTS:
                case READ_HOLDING_REGS:
                case READ_INPUT_REGS:
                case WRITE_SINGLE_COIL:
                case WRITE_SINGLE_REG:
                    return tvb_get_guint8(tvb, 2) + 5;  /* Reported size does not include 2 header, 1 size byte, 2 CRC16 bytes */
                    break;
                case WRITE_MULT_REGS:  /* Response messages of FC15/16 are always 8 bytes */
                case WRITE_MULT_COILS:
                    return 8;
                    break;
                default :
                    return tvb_captured_length(tvb);  /* Fall back on tvb length */
                    break;
            }
        case CANNOT_CLASSIFY :
        default :
            return tvb_captured_length(tvb);  /* Fall back on tvb length */
            break;
    }

}


/* Code to dissect Modbus/TCP messages */
static int
dissect_mbtcp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data)
{

    /* Make sure there's at least enough data to determine it's a Modbus TCP packet */
    if (!tvb_bytes_exist(tvb, 0, 8))
        return 0;

    /* check that it actually looks like Modbus/TCP */
    /* protocol id == 0 */
    if(tvb_get_ntohs(tvb, 2) != 0 ){
        return 0;
    }
    /* length is at least 2 (unit_id + function_code) */
    if(tvb_get_ntohs(tvb, 4) < 2 ){
        return 0;
    }

    /* build up protocol tree and iterate over multiple packets */
    tcp_dissect_pdus(tvb, pinfo, tree, mbtcp_desegment, 6,
                     get_mbtcp_pdu_len, dissect_mbtcp_pdu, data);

    return tvb_captured_length(tvb);
}

/* Code to dissect Modbus RTU over TCP messages */
static int
dissect_mbrtu(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data)
{

    /* Make sure there's at least enough data to determine it's a Modbus packet */
    /* 5 bytes is the smallest possible valid message (exception response) */
    if (!tvb_bytes_exist(tvb, 0, 5))
        return 0;

    /* For Modbus RTU mode, confirm that the first byte is a valid address (non-zero), */
    /* so we can eliminate false-posititves on Modbus TCP messages loaded as RTU       */
    if(tvb_get_guint8(tvb, 0) == 0 )
        return 0;

    /* build up protocol tree and iterate over multiple packets */
    tcp_dissect_pdus(tvb, pinfo, tree, mbrtu_desegment, 5,
                     get_mbrtu_pdu_len, dissect_mbrtu_pdu, data);

    return tvb_captured_length(tvb);
}

static int
dissect_mbrtu_udp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data)
{

    /* Make sure there's at least enough data to determine it's a Modbus packet */
    /* 5 bytes is the smallest possible valid message (exception response) */
    if (tvb_reported_length(tvb) < 5)
        return 0;

    return dissect_mbrtu_pdu(tvb, pinfo, tree, data);
}


/* Code to allow further dissection of Modbus data payload */
/* Common to both Modbus/TCP and Modbus RTU dissectors     */
static void
dissect_modbus_data(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, guint8 function_code,
                    gint payload_start, gint payload_len, gint register_format, guint16 reg_base)
{
    gint reported_len, data_offset;
    gint16  data16s;
    gint32  data32s;
    guint16 data16, modflt_lo, modflt_hi, reg_num=reg_base;
    guint32 data32, modflt_comb;
    gfloat data_float, modfloat;
    proto_item    *register_item = NULL;
    tvbuff_t *next_tvb;

    reported_len = tvb_reported_length_remaining(tvb, payload_start);
    data_offset = 0;

    if ( payload_start < 0 || ( payload_len + payload_start ) == 0 )
        return;

    /* If calculated length from remaining tvb data != bytes in packet, do not attempt to decode      */
    if ( payload_len != reported_len ) {
        proto_tree_add_item(tree, hf_modbus_data, tvb, payload_start, reported_len, ENC_NA);
        return;
    }

    /* If data type of payload is Holding or Input registers */
    /* AND */
    /* if payload length is not a multiple of 4, don't attempt to decode anything in 32-bit format */
    if ((function_code == READ_HOLDING_REGS) || (function_code == READ_INPUT_REGS) || (function_code == WRITE_MULT_REGS)) {
        if ((payload_len % 4 != 0) && ( (register_format == MODBUS_PREF_REGISTER_FORMAT_UINT32) ||
            (register_format == MODBUS_PREF_REGISTER_FORMAT_IEEE_FLOAT) ||
            (register_format == MODBUS_PREF_REGISTER_FORMAT_MODICON_FLOAT) ) ) {
            register_item = proto_tree_add_item(tree, hf_modbus_data, tvb, payload_start, payload_len, ENC_NA);
            expert_add_info(pinfo, register_item, &ei_modbus_data_decode);
            return;
        }
    }

    /* Build a new tvb containing just the data payload   */
    next_tvb = tvb_new_subset(tvb, payload_start, payload_len, reported_len);

    switch ( function_code ) {

        case READ_HOLDING_REGS:
        case READ_INPUT_REGS:
        case WRITE_MULT_REGS:
            while (data_offset < payload_len) {
                /* Use "Preferences" options to determine decoding format of register data, as no format is implied by the protocol itself. */
                /* Based on a standard register size of 16-bits, use decoding format preference to step through each register and display  */
                /* it in an appropriate fashion. */
                switch (register_format) {
                    case MODBUS_PREF_REGISTER_FORMAT_UINT16: /* Standard-size unsigned integer 16-bit register */
                        data16 = tvb_get_ntohs(next_tvb, data_offset);
                        proto_tree_add_uint_format(tree, hf_modbus_reg16, next_tvb, data_offset, 2, reg_num,
                                                    "Register %u (UINT16): %u", reg_num, data16);
                        data_offset += 2;
                        reg_num += 1;
                        break;
                    case MODBUS_PREF_REGISTER_FORMAT_INT16: /* Standard-size signed integer 16-bit register */
                        data16s = tvb_get_ntohs(next_tvb, data_offset);
                        proto_tree_add_uint_format(tree, hf_modbus_reg16, next_tvb, data_offset, 2, reg_num,
                                                    "Register %u (INT16): %d", reg_num, data16s);
                        data_offset += 2;
                        reg_num += 1;
                        break;
                    case MODBUS_PREF_REGISTER_FORMAT_UINT32: /* Double-size unsigned integer 2 x 16-bit registers */
                        data32 = tvb_get_ntohl(next_tvb, data_offset);
                        proto_tree_add_uint_format(tree, hf_modbus_reg32, next_tvb, data_offset, 4, reg_num,
                                                    "Register %u (UINT32): %u", reg_num, data32);
                        data_offset += 4;
                        reg_num += 2;
                        break;
                    case MODBUS_PREF_REGISTER_FORMAT_INT32: /* Double-size signed integer 2 x 16-bit registers */
                        data32s = tvb_get_ntohl(next_tvb, data_offset);
                        proto_tree_add_uint_format(tree, hf_modbus_reg32, next_tvb, data_offset, 4, reg_num,
                                                    "Register %u (INT32): %d", reg_num, data32s);
                        data_offset += 4;
                        reg_num += 2;
                        break;
                    case MODBUS_PREF_REGISTER_FORMAT_IEEE_FLOAT: /* IEEE Floating Point, 2 x 16-bit registers */
                        data_float = tvb_get_ntohieee_float(next_tvb, data_offset);

                        proto_tree_add_uint_format(tree, hf_modbus_reg32, next_tvb, data_offset, 4, reg_num,
                                                    "Register %u (IEEE Float): %f", reg_num, data_float);
                        data_offset += 4;
                        reg_num += 2;
                        break;
                    case MODBUS_PREF_REGISTER_FORMAT_MODICON_FLOAT: /* Modicon Floating Point (word-swap), 2 x 16-bit registers */
                        /* Modicon-style Floating Point values are stored in reverse-word order.                     */
                        /* ie: a standard IEEE float value 59.991459 is equal to 0x426ff741                          */
                        /*     while the Modicon equivalent to this value is 0xf741426f                              */
                        /* To re-assemble a proper IEEE float, we must retrieve the 2 x 16-bit words, bit-shift the  */
                        /* "hi" component by 16-bits and then OR them together into a combined 32-bit int.           */
                        /* Following that operation, use some memcpy magic to copy the 4 raw data bytes from the     */
                        /* 32-bit integer into a standard float.  Not sure if there is a cleaner way possible using  */
                        /* the Wireshark libraries, but this seems to work OK.                                        */

                        modflt_lo = tvb_get_ntohs(next_tvb, data_offset);
                        modflt_hi = tvb_get_ntohs(next_tvb, data_offset+2);
                        modflt_comb = (guint32)(modflt_hi<<16) | modflt_lo;
                        memcpy(&modfloat, &modflt_comb, 4);

                        proto_tree_add_uint_format(tree, hf_modbus_reg32, next_tvb, data_offset, 4, reg_num,
                                                    "Register %u (Modicon Float): %f", reg_num, modfloat);
                        data_offset += 4;
                        reg_num += 2;
                        break;
                    default:
                        /* Avoid any chance of an infinite loop */
                        data_offset = payload_len;
                        break;
                    } /* register format switch */

                } /* while loop */

                break;

        default:
            if ( ! dissector_try_string(modbus_data_dissector_table, "data", next_tvb, pinfo, tree, NULL) )
                proto_tree_add_item(tree, hf_modbus_data, tvb, payload_start, payload_len, ENC_NA);
            break;
        }
}

/* Code to dissect Modbus request message */
static int
dissect_modbus_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *modbus_tree, guint8 function_code, gint payload_start, gint payload_len)
{
    proto_tree    *group_tree;
    gint          byte_cnt, group_offset, ii;
    gint          register_format = MODBUS_PREF_REGISTER_FORMAT_UINT16;  /* Default value for register formatting.. */
    guint8        mei_code;
    guint16       reg_base=0, diagnostic_code;
    guint32       group_byte_cnt, group_word_cnt;

    modbus_conversation   *conv;

    /* See if we have any context */
    conv = (modbus_conversation *)p_get_proto_data(wmem_file_scope(), pinfo, proto_modbus, 0);

    if (conv) {
        register_format = conv->register_format;
    }

    switch (function_code) {

        case READ_COILS:
        case READ_DISCRETE_INPUTS:
            proto_tree_add_item(modbus_tree, hf_modbus_reference, tvb, payload_start, 2, ENC_BIG_ENDIAN);
            proto_tree_add_item(modbus_tree, hf_modbus_bitcnt, tvb, payload_start + 2, 2, ENC_BIG_ENDIAN);
            break;

        case READ_HOLDING_REGS:
        case READ_INPUT_REGS:
            proto_tree_add_item(modbus_tree, hf_modbus_reference, tvb, payload_start, 2, ENC_BIG_ENDIAN);
            proto_tree_add_item(modbus_tree, hf_modbus_wordcnt, tvb, payload_start + 2, 2, ENC_BIG_ENDIAN);
            break;

        case WRITE_SINGLE_COIL:
            proto_tree_add_item(modbus_tree, hf_modbus_reference, tvb, payload_start, 2, ENC_BIG_ENDIAN);
            dissect_modbus_data(tvb, pinfo, modbus_tree, function_code, payload_start + 2, 1, register_format, reg_base);
            proto_tree_add_item(modbus_tree, hf_modbus_padding, tvb, payload_start + 3, 1, ENC_NA);
            break;

        case WRITE_SINGLE_REG:
            proto_tree_add_item(modbus_tree, hf_modbus_reference, tvb, payload_start, 2, ENC_BIG_ENDIAN);
            dissect_modbus_data(tvb, pinfo, modbus_tree, function_code, payload_start + 2, 2, register_format, reg_base);
            break;

        case READ_EXCEPT_STAT:
            /* Do Nothing  */
            break;

        case DIAGNOSTICS:
            diagnostic_code = tvb_get_ntohs(tvb, payload_start);
            proto_tree_add_uint(modbus_tree, hf_modbus_diag_sf, tvb, payload_start, 2, diagnostic_code);
            switch(diagnostic_code)
            {
                case RETURN_QUERY_DATA:
                    if (payload_len > 2)
                        proto_tree_add_item(modbus_tree, hf_modbus_diag_return_query_data_request, tvb, payload_start+2, payload_len-2, ENC_NA);
                    break;
                case RESTART_COMMUNICATION_OPTION:
                    proto_tree_add_item(modbus_tree, hf_modbus_diag_restart_communication_option, tvb, payload_start+2, 2, ENC_BIG_ENDIAN);
                    break;
                case CHANGE_ASCII_INPUT_DELIMITER:
                    proto_tree_add_item(modbus_tree, hf_modbus_diag_ascii_input_delimiter, tvb, payload_start+2, 1, ENC_BIG_ENDIAN);
                    break;
                case RETURN_DIAGNOSTIC_REGISTER:           /* 00 00 Data Field */
                case FORCE_LISTEN_ONLY_MODE:               /* 00 00 Data Field */
                case CLEAR_COUNTERS_AND_DIAG_REG:          /* 00 00 Data Field */
                case RETURN_BUS_MESSAGE_COUNT:             /* 00 00 Data Field */
                case RETURN_BUS_COMM_ERROR_COUNT:          /* 00 00 Data Field */
                case RETURN_BUS_EXCEPTION_ERROR_COUNT:     /* 00 00 Data Field */
                case RETURN_SLAVE_MESSAGE_COUNT:           /* 00 00 Data Field */
                case RETURN_SLAVE_NO_RESPONSE_COUNT:       /* 00 00 Data Field */
                case RETURN_SLAVE_NAK_COUNT:               /* 00 00 Data Field */
                case RETURN_SLAVE_BUSY_COUNT:              /* 00 00 Data Field */
                case RETURN_BUS_CHAR_OVERRUN_COUNT:        /* 00 00 Data Field */
                case CLEAR_OVERRUN_COUNTER_AND_FLAG:
                default:
                    if (payload_len > 2)
                        dissect_modbus_data(tvb, pinfo, modbus_tree, function_code, payload_start+2, payload_len-2, register_format, reg_base);
                    break;
            }
            break;
        case WRITE_MULT_COILS:
            proto_tree_add_item(modbus_tree, hf_modbus_reference, tvb, payload_start, 2, ENC_BIG_ENDIAN);
            proto_tree_add_item(modbus_tree, hf_modbus_bitcnt, tvb, payload_start + 2, 2, ENC_BIG_ENDIAN);
            byte_cnt = (guint32)tvb_get_guint8(tvb, payload_start + 4);
            proto_tree_add_uint(modbus_tree, hf_modbus_bytecnt, tvb, payload_start + 4, 1, byte_cnt);
            dissect_modbus_data(tvb, pinfo, modbus_tree, function_code, payload_start + 5, byte_cnt, register_format, reg_base);
            break;

        case WRITE_MULT_REGS:
            reg_base = tvb_get_ntohs(tvb, payload_start);
            proto_tree_add_item(modbus_tree, hf_modbus_reference, tvb, payload_start, 2, ENC_BIG_ENDIAN);
            proto_tree_add_item(modbus_tree, hf_modbus_wordcnt, tvb, payload_start + 2, 2, ENC_BIG_ENDIAN);
            byte_cnt = (guint32)tvb_get_guint8(tvb, payload_start + 4);
            proto_tree_add_uint(modbus_tree, hf_modbus_bytecnt, tvb, payload_start + 4, 1, byte_cnt);
            dissect_modbus_data(tvb, pinfo, modbus_tree, function_code, payload_start + 5, byte_cnt, register_format, reg_base);
            break;

        case READ_FILE_RECORD:
            byte_cnt = (guint32)tvb_get_guint8(tvb, payload_start);
            proto_tree_add_uint(modbus_tree, hf_modbus_bytecnt, tvb, payload_start, 1,
                    byte_cnt);

            /* add subtrees to describe each group of packet */
            group_offset = payload_start + 1;
            for (ii = 0; ii < byte_cnt / 7; ii++) {
                group_tree = proto_tree_add_subtree_format( modbus_tree, tvb, group_offset, 7,
                        ett_group_hdr, NULL, "Group %u", ii);
                proto_tree_add_item(group_tree, hf_modbus_reftype, tvb, group_offset, 1, ENC_BIG_ENDIAN);
                proto_tree_add_item(group_tree, hf_modbus_lreference, tvb, group_offset + 1, 4, ENC_BIG_ENDIAN);
                proto_tree_add_item(group_tree, hf_modbus_wordcnt, tvb, group_offset + 5, 2, ENC_BIG_ENDIAN);
                group_offset += 7;
            }
            break;

        case WRITE_FILE_RECORD:
            byte_cnt = (guint32)tvb_get_guint8(tvb, payload_start);
            proto_tree_add_uint(modbus_tree, hf_modbus_bytecnt, tvb, payload_start, 1, byte_cnt);

            /* add subtrees to describe each group of packet */
            group_offset = payload_start + 1;
            ii = 0;
            while (byte_cnt > 0) {
                group_word_cnt = tvb_get_ntohs(tvb, group_offset + 5);
                group_byte_cnt = (2 * group_word_cnt) + 7;
                group_tree = proto_tree_add_subtree_format( modbus_tree, tvb, group_offset,
                        group_byte_cnt, ett_group_hdr, NULL, "Group %u", ii);
                proto_tree_add_item(group_tree, hf_modbus_reftype, tvb, group_offset, 1, ENC_BIG_ENDIAN);
                proto_tree_add_item(group_tree, hf_modbus_lreference, tvb, group_offset + 1, 4, ENC_BIG_ENDIAN);
                proto_tree_add_uint(group_tree, hf_modbus_wordcnt, tvb, group_offset + 5, 2, group_word_cnt);
                dissect_modbus_data(tvb, pinfo, group_tree, function_code, group_offset + 7, group_byte_cnt - 7, register_format, reg_base);
                group_offset += group_byte_cnt;
                byte_cnt -= group_byte_cnt;
                ii++;
            }
            break;

        case MASK_WRITE_REG:
            proto_tree_add_item(modbus_tree, hf_modbus_reference, tvb, payload_start, 2, ENC_BIG_ENDIAN);
            proto_tree_add_item(modbus_tree, hf_modbus_andmask, tvb, payload_start + 2, 2, ENC_BIG_ENDIAN);
            proto_tree_add_item(modbus_tree, hf_modbus_ormask, tvb, payload_start + 4, 2, ENC_BIG_ENDIAN);
            break;

        case READ_WRITE_REG:
            proto_tree_add_item(modbus_tree, hf_modbus_readref, tvb, payload_start, 2, ENC_BIG_ENDIAN);
            proto_tree_add_item(modbus_tree, hf_modbus_readwordcnt, tvb, payload_start + 2, 2, ENC_BIG_ENDIAN);
            proto_tree_add_item(modbus_tree, hf_modbus_writeref, tvb, payload_start + 4, 2, ENC_BIG_ENDIAN);
            proto_tree_add_item(modbus_tree, hf_modbus_writewordcnt, tvb, payload_start + 6, 2, ENC_BIG_ENDIAN);
            byte_cnt = (guint32)tvb_get_guint8(tvb, payload_start + 8);
            proto_tree_add_uint(modbus_tree, hf_modbus_bytecnt, tvb, payload_start + 8, 1, byte_cnt);
            dissect_modbus_data(tvb, pinfo, modbus_tree, function_code, payload_start + 9, byte_cnt, register_format, reg_base);
            break;

        case READ_FIFO_QUEUE:
            proto_tree_add_item(modbus_tree, hf_modbus_reference, tvb, payload_start, 2, ENC_BIG_ENDIAN);
            break;

        case ENCAP_INTERFACE_TRANSP:
            proto_tree_add_item(modbus_tree, hf_modbus_mei, tvb, payload_start, 1, ENC_BIG_ENDIAN);
            mei_code = tvb_get_guint8(tvb, payload_start);
            switch (mei_code)
            {
                case READ_DEVICE_ID:
                    proto_tree_add_item(modbus_tree, hf_modbus_read_device_id, tvb, payload_start+1, 1, ENC_BIG_ENDIAN);
                    proto_tree_add_item(modbus_tree, hf_modbus_object_id, tvb, payload_start+2, 1, ENC_BIG_ENDIAN);
                    break;

                case CANOPEN_REQ_RESP:
                    /* CANopen protocol not part of the Modbus/TCP specification */
                default:
                    if (payload_len > 1)
                        dissect_modbus_data(tvb, pinfo, modbus_tree, function_code, payload_start, payload_len-1, register_format, reg_base);
                    break;
            }

            break;

        case REPORT_SLAVE_ID:
        default:
            if (payload_len > 0)
                dissect_modbus_data(tvb, pinfo, modbus_tree, function_code, payload_start, payload_len, register_format, reg_base);
            break;

    } /* Function Code */

    return tvb_captured_length(tvb);
}

/* Code to dissect Modbus Response message */
static int
dissect_modbus_response(tvbuff_t *tvb, packet_info *pinfo, proto_tree *modbus_tree, guint8 function_code, gint payload_start, gint payload_len)
{

    proto_tree    *group_tree, *event_tree, *event_item_tree, *device_objects_tree, *device_objects_item_tree;
    proto_item    *mei;
    gint          byte_cnt, group_offset, event_index, object_index, object_len, num_objects, ii;
    gint          register_format = MODBUS_PREF_REGISTER_FORMAT_UINT16;  /* Default value for register formatting.. */
    guint8        object_type, mei_code, event_code;
    guint16       reg_base=0, diagnostic_code;
    guint32       group_byte_cnt, group_word_cnt;

    /* Conversation tracking */
    proto_item            *request_frame_item;
    modbus_conversation   *conv;
    guint8                req_function_code;
    guint32               req_frame_num;
    gboolean              request_found = FALSE;
    modbus_request_info_t *request_data;

    /* See if we have any context */
    conv = (modbus_conversation *)p_get_proto_data(wmem_file_scope(), pinfo, proto_modbus, 0);

    if (conv) {

        wmem_list_frame_t *frame = wmem_list_head(conv->modbus_request_frame_data);
        /* Step backward through all logged instances of request frames, looking for a request frame number that
           occurred immediately prior to current frame number that has a matching function code */
        while (frame && !request_found) {
            request_data = (modbus_request_info_t *)wmem_list_frame_data(frame);
            req_frame_num = request_data->fnum;
            req_function_code = request_data->function_code;
            if ((pinfo->num > req_frame_num) && (req_function_code == function_code)) {
                request_frame_item = proto_tree_add_uint(modbus_tree, hf_modbus_request_frame, tvb, 0, 0, req_frame_num);
                reg_base = request_data->base_address;
                PROTO_ITEM_SET_GENERATED(request_frame_item);
                request_found = TRUE;
            }
            frame = wmem_list_frame_next(frame);
        }

        register_format = conv->register_format;
    } /* conv */

    switch (function_code) {

        case READ_COILS:
        case READ_DISCRETE_INPUTS:
            byte_cnt = (guint32)tvb_get_guint8(tvb, payload_start);
            proto_tree_add_uint(modbus_tree, hf_modbus_bytecnt, tvb, payload_start, 1, byte_cnt);
            dissect_modbus_data(tvb, pinfo, modbus_tree, function_code, payload_start + 1, byte_cnt, register_format, reg_base);
            break;

        case READ_HOLDING_REGS:
        case READ_INPUT_REGS:
            byte_cnt = (guint32)tvb_get_guint8(tvb, payload_start);
            proto_tree_add_uint(modbus_tree, hf_modbus_bytecnt, tvb, payload_start, 1, byte_cnt);
            dissect_modbus_data(tvb, pinfo, modbus_tree, function_code, payload_start + 1, byte_cnt, register_format, reg_base);
            break;

        case WRITE_SINGLE_COIL:
            proto_tree_add_item(modbus_tree, hf_modbus_reference, tvb, payload_start, 2, ENC_BIG_ENDIAN);
            dissect_modbus_data(tvb, pinfo, modbus_tree, function_code, payload_start + 2, 1, register_format, reg_base);
            proto_tree_add_item(modbus_tree, hf_modbus_padding, tvb, payload_start + 3, 1, ENC_NA);
            break;

        case WRITE_SINGLE_REG:
            proto_tree_add_item(modbus_tree, hf_modbus_reference, tvb, payload_start, 2, ENC_BIG_ENDIAN);
            dissect_modbus_data(tvb, pinfo, modbus_tree, function_code, payload_start + 2, 2, register_format, reg_base);
            break;

        case READ_EXCEPT_STAT:
            dissect_modbus_data(tvb, pinfo, modbus_tree, function_code, payload_start, 1, register_format, reg_base);
            break;

        case DIAGNOSTICS:
            diagnostic_code = tvb_get_ntohs(tvb, payload_start);
            proto_tree_add_uint(modbus_tree, hf_modbus_diag_sf, tvb, payload_start, 2, diagnostic_code);
            switch(diagnostic_code)
            {
                case RETURN_QUERY_DATA: /* Echo of Request */
                    if (payload_len > 2)
                        proto_tree_add_item(modbus_tree, hf_modbus_diag_return_query_data_echo, tvb, payload_start+2, payload_len-2, ENC_NA);
                    break;
                case RESTART_COMMUNICATION_OPTION:  /* Echo of Request */
                    proto_tree_add_item(modbus_tree, hf_modbus_diag_restart_communication_option, tvb, payload_start+2, 2, ENC_BIG_ENDIAN);
                    break;
                case RETURN_DIAGNOSTIC_REGISTER:
                    proto_tree_add_item(modbus_tree, hf_modbus_diag_return_diag_register, tvb, payload_start+2, 2, ENC_BIG_ENDIAN);
                    break;
                case CHANGE_ASCII_INPUT_DELIMITER:   /* XXX - Do we expect this to ever be a response? */
                    proto_tree_add_item(modbus_tree, hf_modbus_diag_ascii_input_delimiter, tvb, payload_start+2, 1, ENC_BIG_ENDIAN);
                    break;
                case CLEAR_COUNTERS_AND_DIAG_REG:   /* Echo of Request */
                    proto_tree_add_item(modbus_tree, hf_modbus_diag_clear_ctr_diag_reg, tvb, payload_start+2, 2, ENC_BIG_ENDIAN);
                    break;
                case RETURN_BUS_MESSAGE_COUNT:
                    proto_tree_add_item(modbus_tree, hf_modbus_diag_return_bus_message_count, tvb, payload_start+2, 2, ENC_BIG_ENDIAN);
                    break;
                case RETURN_BUS_COMM_ERROR_COUNT:
                    proto_tree_add_item(modbus_tree, hf_modbus_diag_return_bus_comm_error_count, tvb, payload_start+2, 2, ENC_BIG_ENDIAN);
                    break;
                case RETURN_BUS_EXCEPTION_ERROR_COUNT:
                    proto_tree_add_item(modbus_tree, hf_modbus_diag_return_bus_exception_error_count, tvb, payload_start+2, 2, ENC_BIG_ENDIAN);
                    break;
                case RETURN_SLAVE_MESSAGE_COUNT:
                    proto_tree_add_item(modbus_tree, hf_modbus_diag_return_slave_message_count, tvb, payload_start+2, 2, ENC_BIG_ENDIAN);
                    break;
                case RETURN_SLAVE_NO_RESPONSE_COUNT:
                    proto_tree_add_item(modbus_tree, hf_modbus_diag_return_no_slave_response_count, tvb, payload_start+2, 2, ENC_BIG_ENDIAN);
                    break;
                case RETURN_SLAVE_NAK_COUNT:
                    proto_tree_add_item(modbus_tree, hf_modbus_diag_return_slave_nak_count, tvb, payload_start+2, 2, ENC_BIG_ENDIAN);
                    break;
                case RETURN_SLAVE_BUSY_COUNT:
                    proto_tree_add_item(modbus_tree, hf_modbus_diag_return_slave_busy_count, tvb, payload_start+2, 2, ENC_BIG_ENDIAN);
                    break;
                case RETURN_BUS_CHAR_OVERRUN_COUNT:
                    proto_tree_add_item(modbus_tree, hf_modbus_diag_return_bus_char_overrun_count, tvb, payload_start+2, 2, ENC_BIG_ENDIAN);
                    break;
                case CLEAR_OVERRUN_COUNTER_AND_FLAG:        /* Echo of Request */
                case FORCE_LISTEN_ONLY_MODE:                /* No response anticipated */
                default:
                    if (payload_len > 2)
                        dissect_modbus_data(tvb, pinfo, modbus_tree, function_code, payload_start+2, payload_len-2, register_format, reg_base);
                    break;
            } /* diagnostic_code */
            break;

        case GET_COMM_EVENT_CTRS:
            proto_tree_add_item(modbus_tree, hf_modbus_status, tvb, payload_start, 2, ENC_BIG_ENDIAN);
            proto_tree_add_item(modbus_tree, hf_modbus_event_count, tvb, payload_start+2, 2, ENC_BIG_ENDIAN);
            break;

        case GET_COMM_EVENT_LOG:
            byte_cnt = (guint32)tvb_get_guint8(tvb, payload_start);
            proto_tree_add_uint(modbus_tree, hf_modbus_bytecnt, tvb, payload_start, 1, byte_cnt);
            proto_tree_add_item(modbus_tree, hf_modbus_status, tvb, payload_start+1, 2, ENC_BIG_ENDIAN);
            proto_tree_add_item(modbus_tree, hf_modbus_event_count, tvb, payload_start+3, 2, ENC_BIG_ENDIAN);
            proto_tree_add_item(modbus_tree, hf_modbus_message_count, tvb, payload_start+5, 2, ENC_BIG_ENDIAN);
            if (byte_cnt-6 > 0) {
                byte_cnt -= 6;
                event_index = 0;
                event_tree = proto_tree_add_subtree(modbus_tree, tvb, payload_start+7, byte_cnt, ett_events, NULL, "Events");
                while (byte_cnt > 0) {
                    event_code = tvb_get_guint8(tvb, payload_start+7+event_index);
                    if (event_code == 0) {
                        proto_tree_add_uint_format(event_tree, hf_modbus_event, tvb, payload_start+7+event_index, 1, event_code, "Initiated Communication Restart");
                    }
                    else if (event_code == 4) {
                        proto_tree_add_uint_format(event_tree, hf_modbus_event, tvb, payload_start+7+event_index, 1, event_code, "Entered Listen Only Mode");
                    }
                    else if (event_code & REMOTE_DEVICE_RECV_EVENT_MASK) {
                        mei = proto_tree_add_uint_format(event_tree, hf_modbus_event, tvb, payload_start+7+event_index, 1,
                                    event_code, "Receive Event: 0x%02X", event_code);
                        event_item_tree = proto_item_add_subtree(mei, ett_events_recv);

                        /* add subtrees to describe each event bit */
                        proto_tree_add_item(event_item_tree, hf_modbus_event_recv_comm_err,
                          tvb, payload_start+7+event_index, 1, ENC_LITTLE_ENDIAN );
                        proto_tree_add_item(event_item_tree, hf_modbus_event_recv_char_over,
                          tvb, payload_start+7+event_index, 1, ENC_LITTLE_ENDIAN );
                        proto_tree_add_item(event_item_tree, hf_modbus_event_recv_lo_mode,
                          tvb, payload_start+7+event_index, 1, ENC_LITTLE_ENDIAN );
                        proto_tree_add_item(event_item_tree, hf_modbus_event_recv_broadcast,
                          tvb, payload_start+7+event_index, 1, ENC_LITTLE_ENDIAN );
                    }
                    else if ((event_code & REMOTE_DEVICE_SEND_EVENT_MASK) == REMOTE_DEVICE_SEND_EVENT_VALUE) {
                        mei = proto_tree_add_uint_format(event_tree, hf_modbus_event, tvb, payload_start+7+event_index, 1,
                                    event_code, "Send Event: 0x%02X", event_code);
                        event_item_tree = proto_item_add_subtree(mei, ett_events_send);

                        /* add subtrees to describe each event bit */
                        proto_tree_add_item(event_item_tree, hf_modbus_event_send_read_ex,
                          tvb, payload_start+7+event_index, 1, ENC_LITTLE_ENDIAN );
                        proto_tree_add_item(event_item_tree, hf_modbus_event_send_slave_abort_ex,
                          tvb, payload_start+7+event_index, 1, ENC_LITTLE_ENDIAN );
                        proto_tree_add_item(event_item_tree, hf_modbus_event_send_slave_busy_ex,
                          tvb, payload_start+7+event_index, 1, ENC_LITTLE_ENDIAN );
                        proto_tree_add_item(event_item_tree, hf_modbus_event_send_slave_nak_ex,
                          tvb, payload_start+7+event_index, 1, ENC_LITTLE_ENDIAN );
                        proto_tree_add_item(event_item_tree, hf_modbus_event_send_write_timeout,
                          tvb, payload_start+7+event_index, 1, ENC_LITTLE_ENDIAN );
                        proto_tree_add_item(event_item_tree, hf_modbus_event_send_lo_mode,
                          tvb, payload_start+7+event_index, 1, ENC_LITTLE_ENDIAN );
                    }
                    else {
                        proto_tree_add_uint_format(event_tree, hf_modbus_event, tvb, payload_start+7+event_index, 1, event_code, "Unknown Event");
                    }

                    byte_cnt--;
                    event_index++;
                }
            }
            break;

        case WRITE_MULT_COILS:
            proto_tree_add_item(modbus_tree, hf_modbus_reference, tvb, payload_start, 2, ENC_BIG_ENDIAN);
            proto_tree_add_item(modbus_tree, hf_modbus_bitcnt, tvb, payload_start + 2, 2, ENC_BIG_ENDIAN);
            break;

        case WRITE_MULT_REGS:
            proto_tree_add_item(modbus_tree, hf_modbus_reference, tvb, payload_start, 2, ENC_BIG_ENDIAN);
            proto_tree_add_item(modbus_tree, hf_modbus_wordcnt, tvb, payload_start + 2, 2, ENC_BIG_ENDIAN);
            break;

        case READ_FILE_RECORD:
            byte_cnt = (guint32)tvb_get_guint8(tvb, payload_start);
            proto_tree_add_uint(modbus_tree, hf_modbus_bytecnt, tvb, payload_start, 1,
                    byte_cnt);

            /* add subtrees to describe each group of packet */
            group_offset = payload_start + 1;
            ii = 0;
            while (byte_cnt > 0) {
                group_byte_cnt = (guint32)tvb_get_guint8(tvb, group_offset);
                group_tree = proto_tree_add_subtree_format( modbus_tree, tvb, group_offset, group_byte_cnt + 1,
                        ett_group_hdr, NULL, "Group %u", ii);
                proto_tree_add_uint(group_tree, hf_modbus_bytecnt, tvb, group_offset, 1,
                        group_byte_cnt);
                proto_tree_add_item(group_tree, hf_modbus_reftype, tvb, group_offset + 1, 1, ENC_BIG_ENDIAN);
                dissect_modbus_data(tvb, pinfo, group_tree, function_code, group_offset + 2, group_byte_cnt - 1, register_format, reg_base);
                group_offset += (group_byte_cnt + 1);
                byte_cnt -= (group_byte_cnt + 1);
                ii++;
            }
            break;

        case WRITE_FILE_RECORD:   /* Normal response is echo of request */
            byte_cnt = (guint32)tvb_get_guint8(tvb, payload_start);
            proto_tree_add_uint(modbus_tree, hf_modbus_bytecnt, tvb, payload_start, 1, byte_cnt);

            /* add subtrees to describe each group of packet */
            group_offset = payload_start + 1;
            ii = 0;
            while (byte_cnt > 0) {
                group_word_cnt = tvb_get_ntohs(tvb, group_offset + 5);
                group_byte_cnt = (2 * group_word_cnt) + 7;
                group_tree = proto_tree_add_subtree_format( modbus_tree, tvb, group_offset,
                        group_byte_cnt, ett_group_hdr, NULL, "Group %u", ii);
                proto_tree_add_item(group_tree, hf_modbus_reftype, tvb, group_offset, 1, ENC_BIG_ENDIAN);
                proto_tree_add_item(group_tree, hf_modbus_lreference, tvb, group_offset + 1, 4, ENC_BIG_ENDIAN);
                proto_tree_add_uint(group_tree, hf_modbus_wordcnt, tvb, group_offset + 5, 2, group_word_cnt);
                dissect_modbus_data(tvb, pinfo, group_tree, function_code, group_offset + 7, group_byte_cnt - 7, register_format, reg_base);
                group_offset += group_byte_cnt;
                byte_cnt -= group_byte_cnt;
                ii++;
            }
            break;

        case MASK_WRITE_REG:      /* Normal response is echo of request */
            proto_tree_add_item(modbus_tree, hf_modbus_reference, tvb, payload_start, 2, ENC_BIG_ENDIAN);
            proto_tree_add_item(modbus_tree, hf_modbus_andmask, tvb, payload_start + 2, 2, ENC_BIG_ENDIAN);
            proto_tree_add_item(modbus_tree, hf_modbus_ormask, tvb, payload_start + 4, 2, ENC_BIG_ENDIAN);
            break;

        case READ_WRITE_REG:
            byte_cnt = (guint32)tvb_get_guint8(tvb, payload_start);
            proto_tree_add_uint(modbus_tree, hf_modbus_bytecnt, tvb, payload_start, 1, byte_cnt);
            dissect_modbus_data(tvb, pinfo, modbus_tree, function_code, payload_start + 1, byte_cnt, register_format, reg_base);
            break;

        case READ_FIFO_QUEUE:
            byte_cnt = (guint32)tvb_get_ntohs(tvb, payload_start);
            proto_tree_add_uint(modbus_tree, hf_modbus_lbytecnt, tvb, payload_start, 2, byte_cnt);
            proto_tree_add_item(modbus_tree, hf_modbus_wordcnt, tvb, payload_start + 2, 2, ENC_BIG_ENDIAN);
            dissect_modbus_data(tvb, pinfo, modbus_tree, function_code, payload_start + 4, byte_cnt - 2, register_format, reg_base);
            break;

        case ENCAP_INTERFACE_TRANSP:
            proto_tree_add_item(modbus_tree, hf_modbus_mei, tvb, payload_start, 1, ENC_BIG_ENDIAN);
            mei_code = tvb_get_guint8(tvb, payload_start);
            switch (mei_code)
            {
                case READ_DEVICE_ID:
                    proto_tree_add_item(modbus_tree, hf_modbus_read_device_id, tvb, payload_start+1, 1, ENC_BIG_ENDIAN);
                    proto_tree_add_item(modbus_tree, hf_modbus_conformity_level, tvb, payload_start+2, 1, ENC_BIG_ENDIAN);
                    proto_tree_add_item(modbus_tree, hf_modbus_more_follows, tvb, payload_start+3, 1, ENC_BIG_ENDIAN);
                    proto_tree_add_item(modbus_tree, hf_modbus_next_object_id, tvb, payload_start+4, 1, ENC_BIG_ENDIAN);
                    num_objects = tvb_get_guint8(tvb, payload_start+5);
                    proto_tree_add_uint(modbus_tree, hf_modbus_num_objects, tvb, payload_start+5, 1, num_objects);
                    device_objects_tree = proto_tree_add_subtree(modbus_tree, tvb, payload_start+6, payload_len-6,
                                                                    ett_device_id_objects, NULL, "Objects");

                    object_index = 0;
                    for (ii = 0; ii < num_objects; ii++)
                    {
                        /* add each "object item" as its own subtree */

                        /* compute length of object */
                        object_type = tvb_get_guint8(tvb, payload_start+6+object_index);
                        object_len = tvb_get_guint8(tvb, payload_start+6+object_index+1);

                        device_objects_item_tree = proto_tree_add_subtree_format(device_objects_tree, tvb, payload_start+6+object_index, 2+object_len,
                                                    ett_device_id_object_items, NULL, "Object #%d", ii+1);

                        proto_tree_add_item(device_objects_item_tree, hf_modbus_object_id, tvb, payload_start+6+object_index, 1, ENC_BIG_ENDIAN);
                        object_index++;

                        proto_tree_add_uint(device_objects_item_tree, hf_modbus_list_object_len, tvb, payload_start+6+object_index, 1, object_len);
                        object_index++;

                        if (object_type < 7)
                        {
                            proto_tree_add_item(device_objects_item_tree, hf_modbus_object_str_value, tvb, payload_start+6+object_index, object_len, ENC_ASCII|ENC_NA);
                        }
                        else
                        {
                            if (object_len > 0)
                                proto_tree_add_item(device_objects_item_tree, hf_modbus_object_value, tvb, payload_start+6+object_index, object_len, ENC_NA);
                        }
                        object_index += object_len;
                    } /* for ii */
                    break;

                case CANOPEN_REQ_RESP:
                    /* CANopen protocol not part of the Modbus/TCP specification */
                default:
                    if (payload_len > 1)
                        dissect_modbus_data(tvb, pinfo, modbus_tree, function_code, payload_start, payload_len-1, register_format, reg_base);
                    break;
            } /* mei_code */
            break;

        case REPORT_SLAVE_ID:
        default:
            if (payload_len > 0)
                dissect_modbus_data(tvb, pinfo, modbus_tree, function_code, payload_start, payload_len, register_format, reg_base);
            break;

    } /* function code */

    return tvb_captured_length(tvb);
}


/* Dissect the Modbus Payload.  Called from either Modbus/TCP or Modbus RTU Dissector */
static int
dissect_modbus(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data)
{
    proto_tree    *modbus_tree;
    proto_item    *mi;
    int           offset = 0;
    int*          packet_type = (int*)data;
    gint          payload_start, payload_len, len;
    guint8        function_code, exception_code;

    /* Reject the packet if data passed from the mbrtu or mbtcp dissector is NULL */
    if (packet_type == NULL)
        return 0;

    len = tvb_captured_length(tvb);

    /* If the packet is zero-length, we should not attempt to dissect any further */
    if (len == 0)
        return 0;

    /* Add items to protocol tree specific to Modbus */
    mi = proto_tree_add_protocol_format(tree, proto_modbus, tvb, offset, len, "Modbus");
    modbus_tree = proto_item_add_subtree(mi, ett_modbus_hdr);

    function_code = tvb_get_guint8(tvb, offset) & 0x7F;
    proto_tree_add_item(modbus_tree, hf_modbus_functioncode, tvb, offset, 1, ENC_BIG_ENDIAN);

    /* Conversation support */
    if (!pinfo->fd->flags.visited) {
        conversation_t       *conversation = NULL;
        modbus_conversation  *modbus_conv_data = NULL;

        /* Find a conversation, create a new if no one exists */
        conversation = find_or_create_conversation(pinfo);
        modbus_conv_data = (modbus_conversation *)conversation_get_proto_data(conversation, proto_modbus);

        if (modbus_conv_data == NULL){
           modbus_conv_data = wmem_new(wmem_file_scope(), modbus_conversation);
           modbus_conv_data->modbus_request_frame_data = wmem_list_new(wmem_file_scope());
           modbus_conv_data->register_format = global_mbus_register_format;
           conversation_add_proto_data(conversation, proto_modbus, (void *)modbus_conv_data);
        }

        p_add_proto_data(wmem_file_scope(), pinfo, proto_modbus, 0, modbus_conv_data);

        if (*packet_type == QUERY_PACKET) {
            /*create the modbus_request frame. It holds the request information.*/
            modbus_request_info_t    *frame_ptr = wmem_new(wmem_file_scope(), modbus_request_info_t);

            /* load information into the modbus request frame */
            frame_ptr->fnum = pinfo->num;
            frame_ptr->function_code = function_code;
            frame_ptr->base_address = tvb_get_ntohs(tvb, 1);
            frame_ptr->num_reg = tvb_get_ntohs(tvb, 3);

            wmem_list_prepend(modbus_conv_data->modbus_request_frame_data, frame_ptr);
        }

    } /* !visited */

    /* Find exception - last bit set in function code */
    if (tvb_get_guint8(tvb, offset) & 0x80 ) {
        exception_code = tvb_get_guint8(tvb, offset+1);
    }
    else {
        exception_code = 0;
    }

    payload_start = offset + 1;
    payload_len = len - 1;

    if (exception_code != 0) {
        proto_item_set_text(mi, "Function %u:  %s.  Exception: %s",
                            function_code,
                            val_to_str_const(function_code, function_code_vals, "Unknown Function"),
                            val_to_str(exception_code,
                                       exception_code_vals,
                                       "Unknown Exception Code (%u)"));
        proto_tree_add_uint(modbus_tree, hf_modbus_exceptioncode, tvb, payload_start, 1,
                            exception_code);
    }
    else {

        /* Follow different dissection path depending on whether packet is query or response */
        if (*packet_type == QUERY_PACKET) {
            dissect_modbus_request(tvb, pinfo, modbus_tree, function_code, payload_start, payload_len);
        }
        else if (*packet_type == RESPONSE_PACKET) {
            dissect_modbus_response(tvb, pinfo, modbus_tree, function_code, payload_start, payload_len);
        }

    }

    return tvb_captured_length(tvb);
}

/* Register the protocol with Wireshark */
void
proto_register_modbus(void)
{
    /* Modbus/TCP header fields */
    static hf_register_info mbtcp_hf[] = {
        { &hf_mbtcp_transid,
            { "Transaction Identifier", "mbtcp.trans_id",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_mbtcp_protid,
            { "Protocol Identifier", "mbtcp.prot_id",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_mbtcp_len,
            { "Length", "mbtcp.len",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_mbtcp_unitid,
            { "Unit Identifier", "mbtcp.unit_id",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
    };

    static ei_register_info mbtcp_ei[] = {
        { &ei_mbtcp_cannot_classify,
          { "mbtcp.cannot_classify", PI_PROTOCOL, PI_WARN,
            "Cannot classify packet type. Try setting Modbus/TCP Port preference to this destination or source port", EXPFILL }
        },
    };

    /* Modbus RTU header fields */
    static hf_register_info mbrtu_hf[] = {
        { &hf_mbrtu_unitid,
            { "Unit ID", "mbrtu.unit_id",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_mbrtu_crc16,
            { "CRC-16", "mbrtu.crc16",
            FT_UINT16, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
    };

    static ei_register_info mbrtu_ei[] = {
        { &ei_mbrtu_crc16_incorrect,
          { "mbrtu.crc16.incorrect", PI_CHECKSUM, PI_WARN,
            "Incorrect CRC", EXPFILL }
        },
    };

    /* Modbus header fields */
    static hf_register_info hf[] = {
        { &hf_modbus_request_frame,
            { "Request Frame", "modbus.request_frame",
            FT_FRAMENUM, BASE_NONE,
            NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_functioncode,
            { "Function Code", "modbus.func_code",
            FT_UINT8, BASE_DEC, VALS(function_code_vals), 0x7F,
            NULL, HFILL }
        },
        { &hf_modbus_reference,
            { "Reference Number", "modbus.reference_num",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_padding,
            { "Padding", "modbus.padding",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_lreference,
            { "Reference Number (32 bit)", "modbus.reference_num_32",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_reftype,
            { "Reference Type", "modbus.reference_type",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_readref,
            { "Read Reference Number", "modbus.read_reference_num",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_writeref,
            { "Write Reference Number", "modbus.write_reference_num",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_wordcnt,
            { "Word Count", "modbus.word_cnt",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_readwordcnt,
            { "Read Word Count", "modbus.read_word_cnt",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_writewordcnt,
            { "Write Word Count", "modbus.write_word_cnt",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_bitcnt,
            { "Bit Count", "modbus.bit_cnt",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_bytecnt,
            { "Byte Count", "modbus.byte_cnt",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_lbytecnt,
            { "Byte Count (16-bit)", "modbus.byte_cnt_16",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_exceptioncode,
            { "Exception Code", "modbus.exception_code",
            FT_UINT8, BASE_DEC, VALS(exception_code_vals), 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_diag_sf,
            { "Diagnostic Code", "modbus.diagnostic_code",
            FT_UINT16, BASE_DEC, VALS(diagnostic_code_vals), 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_diag_return_query_data_request,
            { "Request Data", "modbus.diagnostic.return_query_data.request",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_diag_return_query_data_echo,
            { "Echo Data", "modbus.diagnostic.return_query_data.echo",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_diag_restart_communication_option,
            { "Restart Communication Option", "modbus.diagnostic.restart_communication_option",
            FT_UINT16, BASE_HEX, VALS(diagnostic_restart_communication_option_vals), 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_diag_return_diag_register,
            { "Diagnostic Register Contents", "modbus.diagnostic.return_diag_register",
            FT_UINT16, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_diag_ascii_input_delimiter,
            { "CHAR", "modbus.diagnostic.ascii_input_delimiter",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_diag_clear_ctr_diag_reg,
            { "Clear Counters & Diag Register Echo", "modbus.diagnostic.clear_ctr_diag_reg",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_diag_return_bus_message_count,
            { "Total Message Count", "modbus.diagnostic.bus_message_count",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_diag_return_bus_comm_error_count,
            { "CRC Error Count", "modbus.diagnostic.bus_comm_error_count",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_diag_return_bus_exception_error_count,
            { "Exception Error Count", "modbus.diagnostic.bus_exception_error_count",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_diag_return_slave_message_count,
            { "Slave Message Count", "modbus.diagnostic.slave_message_count",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_diag_return_no_slave_response_count,
            { "Slave No Response Count", "modbus.diagnostic.no_slave_response_count",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_diag_return_slave_nak_count,
            { "Slave NAK Count", "modbus.diagnostic.slave_nak_count",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_diag_return_slave_busy_count,
            { "Slave Device Busy Count", "modbus.diagnostic.slave_busy_count",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_diag_return_bus_char_overrun_count,
            { "Slave Character Overrun Count", "modbus.diagnostic.bus_char_overrun_count",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_status,
            { "Status", "modbus.ev_status",
            FT_UINT16, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_event,
            { "Event", "modbus.event",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_event_count,
            { "Event Count", "modbus.ev_count",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_message_count,
            { "Message Count", "modbus.ev_msg_count",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_event_recv_comm_err,
            { "Communication Error", "modbus.ev_recv_comm_err",
            FT_UINT8, BASE_DEC, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_modbus_event_recv_char_over,
            { "Character Overrun", "modbus.ev_recv_char_over",
            FT_UINT8, BASE_DEC, NULL, 0x10,
            NULL, HFILL }
        },
        { &hf_modbus_event_recv_lo_mode,
            { "Currently in Listen Only Mode", "modbus.ev_recv_lo_mode",
            FT_UINT8, BASE_DEC, NULL, 0x20,
            NULL, HFILL }
        },
        { &hf_modbus_event_recv_broadcast,
            { "Broadcast Received", "modbus.ev_recv_broadcast",
            FT_UINT8, BASE_DEC, NULL, 0x40,
            NULL, HFILL }
        },
        { &hf_modbus_event_send_read_ex,
            { "Read Exception Sent", "modbus.ev_send_read_ex",
            FT_UINT8, BASE_DEC, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_modbus_event_send_slave_abort_ex,
            { "Slave Abort Exception Sent", "modbus.ev_send_slave_abort_ex",
            FT_UINT8, BASE_DEC, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_modbus_event_send_slave_busy_ex,
            { "Slave Busy Exception Sent", "modbus.ev_send_slave_busy_ex",
            FT_UINT8, BASE_DEC, NULL, 0x04,
            NULL, HFILL }
        },
        { &hf_modbus_event_send_slave_nak_ex,
            { "Slave Program NAK Exception Sent", "modbus.ev_send_slave_nak_ex",
            FT_UINT8, BASE_DEC, NULL, 0x08,
            NULL, HFILL }
        },
        { &hf_modbus_event_send_write_timeout,
            { "Write Timeout Error Occurred", "modbus.ev_send_write_timeout",
            FT_UINT8, BASE_DEC, NULL, 0x10,
            NULL, HFILL }
        },
        { &hf_modbus_event_send_lo_mode,
            { "Currently in Listen Only Mode", "modbus.ev_send_lo_mode",
            FT_UINT8, BASE_DEC, NULL, 0x20,
            NULL, HFILL }
        },
        { &hf_modbus_andmask,
            { "AND mask", "modbus.and_mask",
            FT_UINT16, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_ormask,
            { "OR mask", "modbus.or_mask",
            FT_UINT16, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_data,
            { "Data",  "modbus.data",
            FT_BYTES,  BASE_NONE, NULL,    0x0, NULL, HFILL }
        },
        { &hf_modbus_mei,
            { "MEI type", "modbus.mei",
            FT_UINT8, BASE_DEC, VALS(encap_interface_code_vals), 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_read_device_id,
            { "Read Device ID", "modbus.read_device_id",
            FT_UINT8, BASE_DEC, VALS(read_device_id_vals), 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_object_id,
            { "Object ID", "modbus.object_id",
            FT_UINT8, BASE_DEC, VALS(object_id_vals), 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_num_objects,
            { "Number of Objects", "modbus.num_objects",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_list_object_len,
            { "Object length", "modbus.objects_len",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_conformity_level,
            { "Conformity Level", "modbus.conformity_level",
            FT_UINT8, BASE_HEX, VALS(conformity_level_vals), 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_more_follows,
            { "More Follows", "modbus.more_follows",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_next_object_id,
            { "Next Object ID", "modbus.next_object_id",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_object_str_value,
            { "Object String Value", "modbus.object_str_value",
            FT_STRING, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_object_value,
            { "Object Value", "modbus.object_value",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_reg16,
            { "Register Value (16-bit)", "modbus.reg16",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_modbus_reg32,
            { "Register Value (32-bit)", "modbus.reg32",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
    };

    /* Setup protocol subtree array */
    static gint *ett[] = {
        &ett_mbtcp,
        &ett_mbrtu,
        &ett_modbus_hdr,
        &ett_group_hdr,
        &ett_events,
        &ett_events_recv,
        &ett_events_send,
        &ett_device_id_objects,
        &ett_device_id_object_items
    };

    static ei_register_info ei[] = {
        { &ei_modbus_data_decode,
          { "modbus.data.decode", PI_PROTOCOL, PI_WARN,
            "Invalid decoding options, register data not a multiple of 4!", EXPFILL }
        },
    };
    module_t *mbtcp_module;
    module_t *mbrtu_module;
    module_t *modbus_module;
    expert_module_t* expert_mbtcp;
    expert_module_t* expert_mbrtu;
    expert_module_t* expert_modbus;

    /* Register the protocol name and description */
    proto_mbtcp = proto_register_protocol("Modbus/TCP", "Modbus/TCP", "mbtcp");
    proto_mbrtu = proto_register_protocol("Modbus RTU", "Modbus RTU", "mbrtu");
    proto_modbus = proto_register_protocol("Modbus", "Modbus", "modbus");

    /* Registering protocol to be called by another dissector */
    modbus_handle = register_dissector("modbus", dissect_modbus, proto_modbus);
    mbtcp_handle = register_dissector("mbtcp", dissect_mbtcp, proto_mbtcp);
    mbrtu_handle = register_dissector("mbrtu", dissect_mbrtu, proto_mbrtu);

    /* Registering subdissectors table */
    modbus_data_dissector_table = register_dissector_table("modbus.data", "Modbus Data", proto_modbus, FT_STRING, BASE_NONE);
    modbus_dissector_table = register_dissector_table("mbtcp.prot_id", "Modbus/TCP protocol identifier", proto_mbtcp, FT_UINT16, BASE_DEC);

    /* Required function calls to register the header fields and subtrees used */
    proto_register_field_array(proto_mbtcp, mbtcp_hf, array_length(mbtcp_hf));
    proto_register_field_array(proto_mbrtu, mbrtu_hf, array_length(mbrtu_hf));
    proto_register_field_array(proto_modbus, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
    expert_mbtcp = expert_register_protocol(proto_mbtcp);
    expert_register_field_array(expert_mbtcp, mbtcp_ei, array_length(mbtcp_ei));
    expert_mbrtu = expert_register_protocol(proto_mbrtu);
    expert_register_field_array(expert_mbrtu, mbrtu_ei, array_length(mbrtu_ei));
    expert_modbus = expert_register_protocol(proto_modbus);
    expert_register_field_array(expert_modbus, ei, array_length(ei));


    /* Register required preferences for Modbus Protocol variants */
    mbtcp_module = prefs_register_protocol(proto_mbtcp, proto_reg_handoff_mbtcp);
    mbrtu_module = prefs_register_protocol(proto_mbrtu, proto_reg_handoff_mbrtu);
    modbus_module = prefs_register_protocol(proto_modbus, NULL);

    /* Modbus RTU Preference - Desegment, defaults to TRUE for TCP desegmentation */
    prefs_register_bool_preference(mbtcp_module, "desegment",
                                  "Desegment all Modbus RTU packets spanning multiple TCP segments",
                                  "Whether the Modbus RTU dissector should desegment all messages spanning multiple TCP segments",
                                  &mbtcp_desegment);

    /* Modbus/TCP Preference - Default TCP Port, allows for "user" port either than 502. */
    prefs_register_uint_preference(mbtcp_module, "tcp.port", "Modbus/TCP Port",
                       "Set the TCP port for Modbus/TCP packets (if other"
                       " than the default of 502)",
                       10, &global_mbus_tcp_port);

    /* Modbus RTU Preference - Desegment, defaults to TRUE for TCP desegmentation */
    prefs_register_bool_preference(mbrtu_module, "desegment",
                                  "Desegment all Modbus RTU packets spanning multiple TCP segments",
                                  "Whether the Modbus RTU dissector should desegment all messages spanning multiple TCP segments",
                                  &mbrtu_desegment);

    /* Modbus RTU Preference - CRC verification, defaults to FALSE (no verification)*/
    prefs_register_bool_preference(mbrtu_module, "crc_verification",
                                  "Validate CRC",
                                  "Whether to validate the CRC",
                                  &mbrtu_crc);

    /* Modbus RTU Preference - Default TCP Port, defaults to zero, allows custom user port. */
    prefs_register_uint_preference(mbrtu_module, "tcp.port", "Modbus RTU Port",
                       "Set the TCP/UDP port for encapsulated Modbus RTU packets",
                       10, &global_mbus_rtu_port);

    /* Modbus Preference - Holding/Input Register format, this allows for deeper dissection of response data */
    prefs_register_enum_preference(modbus_module, "mbus_register_format",
                                    "Holding/Input Register Format",
                                    "Register Format",
                                    &global_mbus_register_format,
                                    mbus_register_format,
                                    TRUE);

    /* Obsolete Preferences */
    prefs_register_obsolete_preference(mbtcp_module, "mbus_register_addr_type");
    prefs_register_obsolete_preference(mbtcp_module, "mbus_register_format");
    prefs_register_obsolete_preference(mbrtu_module, "mbus_register_addr_type");
    prefs_register_obsolete_preference(mbrtu_module, "mbus_register_format");

}


/* If this dissector uses sub-dissector registration add a registration routine.
   This format is required because a script is used to find these routines and
   create the code that calls these routines.
 */
void
proto_reg_handoff_mbtcp(void)
{
    static unsigned int mbtcp_port;

    /* Make sure to use Modbus/TCP Preferences field to determine default TCP port */

    if(mbtcp_port != 0 && mbtcp_port != global_mbus_tcp_port){
        dissector_delete_uint("tcp.port", mbtcp_port, mbtcp_handle);
    }

    if(global_mbus_tcp_port != 0 && mbtcp_port != global_mbus_tcp_port) {
        dissector_add_uint("tcp.port", global_mbus_tcp_port, mbtcp_handle);
    }

    mbtcp_port = global_mbus_tcp_port;

    dissector_add_uint("mbtcp.prot_id", MODBUS_PROTOCOL_ID, modbus_handle);

}

void
proto_reg_handoff_mbrtu(void)
{
    static unsigned int mbrtu_port = 0;
    dissector_handle_t mbrtu_udp_handle = create_dissector_handle(dissect_mbrtu_udp, proto_mbrtu);

    /* Make sure to use Modbus RTU Preferences field to determine default TCP port */

    if(mbrtu_port != 0 && mbrtu_port != global_mbus_rtu_port){
        dissector_delete_uint("tcp.port", mbrtu_port, mbrtu_handle);
        dissector_delete_uint("udp.port", mbrtu_port, mbrtu_udp_handle);
    }

    if(global_mbus_rtu_port != 0 && mbrtu_port != global_mbus_rtu_port) {
        dissector_add_uint("tcp.port", global_mbus_rtu_port, mbrtu_handle);
        dissector_add_uint("udp.port", global_mbus_rtu_port, mbrtu_udp_handle);
    }

    mbrtu_port = global_mbus_rtu_port;

    dissector_add_uint("mbtcp.prot_id", MODBUS_PROTOCOL_ID, modbus_handle);
    dissector_add_for_decode_as("rtacser.data", mbrtu_handle);

}

/*
 * Editor modelines
 *
 * Local Variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * ex: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
