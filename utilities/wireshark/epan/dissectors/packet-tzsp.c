/* packet-tzsp.c
 *
 * Copyright 2002, Tazmen Technologies Inc
 *
 * Tazmen Sniffer Protocol for encapsulating the packets across a network
 * from a remote packet sniffer. TZSP can encapsulate any other protocol.
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
#include <wiretap/wtap.h>

/*
 * See
 *
 *  http://web.archive.org/web/20050404125022/http://www.networkchemistry.com/support/appnotes/an001_tzsp.html
 *
 * for a description of the protocol.
 */

#define UDP_PORT_TZSP   0x9090

void proto_register_tzsp(void);
void proto_reg_handoff_tzsp(void);

static int proto_tzsp = -1;
static int hf_tzsp_version = -1;
static int hf_tzsp_type = -1;
static int hf_tzsp_encap = -1;

static dissector_handle_t tzsp_handle;

/*
 * Packet types.
 */
#define TZSP_RX_PACKET  0   /* Packet received from the sensor */
#define TZSP_TX_PACKET  1   /* Packet for the sensor to transmit */
#define TZSP_CONFIG     3   /* Configuration information for the sensor */
#define TZSP_NULL       4   /* Null frame, used as a keepalive */
#define TZSP_PORT       5   /* Port opener - opens a NAT tunnel */

static const value_string tzsp_type[] = {
    {TZSP_RX_PACKET,  "Received packet"},
    {TZSP_TX_PACKET,  "Packet for transmit"},
    {TZSP_CONFIG,     "Configuration"},
    {TZSP_NULL,       "Keepalive"},
    {TZSP_PORT,       "Port opener"},
    {0, NULL}
};

/* ************************************************************************* */
/*                        Encapsulation type values                          */
/*               Note that these are not all the same as DLT_ values         */
/* ************************************************************************* */

#define TZSP_ENCAP_ETHERNET              1
#define TZSP_ENCAP_TOKEN_RING            2
#define TZSP_ENCAP_SLIP                  3
#define TZSP_ENCAP_PPP                   4
#define TZSP_ENCAP_FDDI                  5
#define TZSP_ENCAP_RAW                   7   /* "Raw UO", presumably meaning "Raw IP" */
#define TZSP_ENCAP_IEEE_802_11          18
#define TZSP_ENCAP_IEEE_802_11_PRISM   119
#define TZSP_ENCAP_IEEE_802_11_AVS     127

/*
 * Packet encapsulations.
 */
static const value_string tzsp_encapsulation[] = {
    {TZSP_ENCAP_ETHERNET,          "Ethernet"},
    {TZSP_ENCAP_TOKEN_RING,        "Token Ring"},
    {TZSP_ENCAP_SLIP,              "SLIP"},
    {TZSP_ENCAP_PPP,               "PPP"},
    {TZSP_ENCAP_FDDI,              "FDDI"},
    {TZSP_ENCAP_RAW,               "Raw IP"},
    {TZSP_ENCAP_IEEE_802_11,       "IEEE 802.11"},
    {TZSP_ENCAP_IEEE_802_11_PRISM, "IEEE 802.11 with Prism headers"},
    {TZSP_ENCAP_IEEE_802_11_AVS,   "IEEE 802.11 with AVS headers"},
    {0, NULL}
};

static gint ett_tzsp = -1;
static gint ett_tag = -1;

static dissector_handle_t eth_maybefcs_handle;
static dissector_handle_t tr_handle;
static dissector_handle_t ppp_handle;
static dissector_handle_t fddi_handle;
static dissector_handle_t raw_ip_handle;
static dissector_handle_t ieee_802_11_handle;
static dissector_handle_t ieee_802_11_prism_handle;
static dissector_handle_t ieee_802_11_avs_handle;

/* ************************************************************************* */
/*                WLAN radio header fields                                    */
/* ************************************************************************* */

static int hf_option_tag = -1;
static int hf_option_length = -1;
/* static int hf_status_field = -1; */
static int hf_status_msg_type = -1;
static int hf_status_pcf = -1;
/* static int hf_status_mac_port = -1; */
static int hf_status_undecrypted = -1;
static int hf_status_fcs_error = -1;

static int hf_time = -1;
static int hf_silence = -1;
static int hf_signal = -1;
static int hf_rate = -1;
static int hf_channel = -1;
static int hf_unknown = -1;
static int hf_original_length = -1;
static int hf_sensormac = -1;

/* ************************************************************************* */
/*                          Generic header options                           */
/* ************************************************************************* */

#define TZSP_HDR_PAD               0  /* Pad. */
#define TZSP_HDR_END               1  /* End of the list. */
#define TZSP_WLAN_STA             30  /* Station statistics */
#define TZSP_WLAN_PKT             31  /* Packet statistics */
#define TZSP_PACKET_ID            40  /* Unique ID of the packet */
#define TZSP_HDR_ORIGINAL_LENGTH  41  /* Length of the packet before slicing. 2 bytes. */
#define TZSP_HDR_SENSOR           60  /* Sensor MAC address packet was received on, 6 byte ethernet address.*/

/* ************************************************************************* */
/*                          Options for 802.11 radios                        */
/* ************************************************************************* */

#define WLAN_RADIO_HDR_SIGNAL     10  /* Signal strength in dBm, signed byte. */
#define WLAN_RADIO_HDR_NOISE      11  /* Noise level in dBm, signed byte. */
#define WLAN_RADIO_HDR_RATE       12  /* Data rate, unsigned byte. */
#define WLAN_RADIO_HDR_TIMESTAMP  13  /* Timestamp in us, unsigned 32-bits network byte order. */
#define WLAN_RADIO_HDR_MSG_TYPE   14  /* Packet type, unsigned byte. */
#define WLAN_RADIO_HDR_CF         15  /* Whether packet arrived during CF period, unsigned byte. */
#define WLAN_RADIO_HDR_UN_DECR    16  /* Whether packet could not be decrypted by MAC, unsigned byte. */
#define WLAN_RADIO_HDR_FCS_ERR    17  /* Whether packet contains an FCS error, unsigned byte. */
#define WLAN_RADIO_HDR_CHANNEL    18  /* Channel number packet was received on, unsigned byte.*/

static const value_string option_tag_vals[] = {
    {TZSP_HDR_PAD,  "Pad"},
    {TZSP_HDR_END,  "End"},
    {TZSP_HDR_ORIGINAL_LENGTH,  "Original Length"},
    {WLAN_RADIO_HDR_SIGNAL,     "Signal"},
    {WLAN_RADIO_HDR_NOISE,      "Silence"},
    {WLAN_RADIO_HDR_RATE,       "Rate"},
    {WLAN_RADIO_HDR_TIMESTAMP,  "Time"},
    {WLAN_RADIO_HDR_MSG_TYPE,   "Message Type"},
    {WLAN_RADIO_HDR_CF,         "Point Coordination Function"},
    {WLAN_RADIO_HDR_UN_DECR,    "Undecrypted"},
    {WLAN_RADIO_HDR_FCS_ERR,    "Frame check sequence"},
    {WLAN_RADIO_HDR_CHANNEL,    "Channel"},
    {TZSP_HDR_SENSOR,           "Sensor MAC"},
    {0, NULL}
};


/* ************************************************************************* */
/*                Add option information to the display                      */
/* ************************************************************************* */

static int
add_option_info(tvbuff_t *tvb, int pos, proto_tree *tree, proto_item *ti)
{
    guint8      tag, length, fcs_err = 0, encr = 0, seen_fcs_err = 0;
    proto_tree *tag_tree;

    /*
     * Read all option tags in an endless loop. If the packet is malformed this
     * loop might be a problem.
     */
    while (TRUE) {
        tag = tvb_get_guint8(tvb, pos);
        if ((tag != TZSP_HDR_PAD) && (tag != TZSP_HDR_END)) {
            length = tvb_get_guint8(tvb, pos+1);
            tag_tree = proto_tree_add_subtree(tree, tvb, pos, 2+length, ett_tag, NULL, val_to_str_const(tag, option_tag_vals, "Unknown"));
        } else {
            tag_tree = proto_tree_add_subtree(tree, tvb, pos, 1, ett_tag, NULL, val_to_str_const(tag, option_tag_vals, "Unknown"));
            length = 0;
        }

        proto_tree_add_item(tag_tree, hf_option_tag, tvb, pos, 1, ENC_BIG_ENDIAN);
        pos++;
        if ((tag != TZSP_HDR_PAD) && (tag != TZSP_HDR_END)) {
            proto_tree_add_item(tag_tree, hf_option_length, tvb, pos, 1, ENC_BIG_ENDIAN);
            pos++;
        }

        switch (tag) {
        case TZSP_HDR_PAD:
            break;

        case TZSP_HDR_END:
            /* Fill in header with information from other tags. */
            if (seen_fcs_err) {
                proto_item_append_text(ti,"%s", fcs_err?"FCS Error":(encr?"Encrypted":"Good"));
            }
            return pos;

        case TZSP_HDR_ORIGINAL_LENGTH:
            proto_tree_add_item(tag_tree, hf_original_length, tvb, pos, 2, ENC_BIG_ENDIAN);
            break;

        case WLAN_RADIO_HDR_SIGNAL:
            proto_tree_add_item(tag_tree, hf_signal, tvb, pos, 1, ENC_BIG_ENDIAN);
            break;

        case WLAN_RADIO_HDR_NOISE:
            proto_tree_add_item(tag_tree, hf_silence, tvb, pos, 1, ENC_BIG_ENDIAN);
            break;

        case WLAN_RADIO_HDR_RATE:
            proto_tree_add_item(tag_tree, hf_rate, tvb, pos, 1, ENC_BIG_ENDIAN);
            break;

        case WLAN_RADIO_HDR_TIMESTAMP:
            proto_tree_add_item(tag_tree, hf_time, tvb, pos, 4, ENC_BIG_ENDIAN);
            break;

        case WLAN_RADIO_HDR_MSG_TYPE:
            proto_tree_add_item(tag_tree, hf_status_msg_type, tvb, pos, 1, ENC_BIG_ENDIAN);
            break;

        case WLAN_RADIO_HDR_CF:
            proto_tree_add_item(tag_tree, hf_status_pcf, tvb, pos, 1, ENC_NA);
            break;

        case WLAN_RADIO_HDR_UN_DECR:
            proto_tree_add_item(tag_tree, hf_status_undecrypted, tvb, pos, 1, ENC_NA);
            encr = tvb_get_guint8(tvb, pos);
            break;

        case WLAN_RADIO_HDR_FCS_ERR:
            seen_fcs_err = 1;
            proto_tree_add_item(tag_tree, hf_status_fcs_error, tvb, pos, 1, ENC_NA);
            fcs_err = tvb_get_guint8(tvb, pos);
            break;

        case WLAN_RADIO_HDR_CHANNEL:
            proto_tree_add_item(tag_tree, hf_channel, tvb, pos, 1, ENC_BIG_ENDIAN);
            break;

        case TZSP_HDR_SENSOR:
            proto_tree_add_item(tag_tree, hf_sensormac, tvb, pos, 6, ENC_NA);
            break;

        default:
            proto_tree_add_item(tag_tree, hf_unknown, tvb, pos, length, ENC_NA);
            break;
        }

        pos += length;
    }
}

/* ************************************************************************* */
/*                Dissect a TZSP packet                                      */
/* ************************************************************************* */

static int
dissect_tzsp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
    proto_tree         *tzsp_tree     = NULL;
    proto_item         *ti            = NULL;
    int                 pos           = 0;
    tvbuff_t           *next_tvb;
    guint16             encapsulation = 0;
    const char         *info;
    guint8              type;

    col_set_str(pinfo->cinfo, COL_PROTOCOL, "TZSP");
    col_clear(pinfo->cinfo, COL_INFO);

    type = tvb_get_guint8(tvb, 1);

    /* Find the encapsulation. */
    encapsulation = tvb_get_ntohs(tvb, 2);
    info = val_to_str(encapsulation, tzsp_encapsulation, "Unknown (%u)");

    col_add_str(pinfo->cinfo, COL_INFO, info);

    if (tree) {
        /* Adding TZSP item and subtree */
        ti = proto_tree_add_protocol_format(tree, proto_tzsp, tvb, 0,
            -1, "TZSP: %s ", info);
        tzsp_tree = proto_item_add_subtree(ti, ett_tzsp);

        proto_tree_add_item (tzsp_tree, hf_tzsp_version, tvb, 0, 1,
                    ENC_BIG_ENDIAN);
        proto_tree_add_uint (tzsp_tree, hf_tzsp_type, tvb, 1, 1,
                    type);
        proto_tree_add_uint (tzsp_tree, hf_tzsp_encap, tvb, 2, 2,
                    encapsulation);
    }

    /*
     * XXX - what about TZSP_CONFIG frames?
     *
     * The MIB at
     *
     *  http://web.archive.org/web/20021221195733/http://www.networkchemistry.com/support/appnotes/SENSOR-MIB
     *
     * seems to indicate that you can configure the probe using SNMP;
     * does TZSP_CONFIG also support that?  An old version of Kismet
     * included code to control a Network Chemistry WSP100 sensor:
     *
     *  https://www.kismetwireless.net/code-old/svn/tags/kismet-2004-02-R1/wsp100source.cc
     *
     * and it used SNMP to configure the probe.
     */
    if ((type != TZSP_NULL) && (type != TZSP_PORT)) {
        pos = add_option_info(tvb, 4, tzsp_tree, ti);

        if (tree)
            proto_item_set_end(ti, tvb, pos);
        next_tvb = tvb_new_subset_remaining(tvb, pos);
        switch (encapsulation) {

        case TZSP_ENCAP_ETHERNET:
            call_dissector(eth_maybefcs_handle, next_tvb, pinfo, tree);
            break;

        case TZSP_ENCAP_TOKEN_RING:
            call_dissector(tr_handle, next_tvb, pinfo, tree);
            break;

        case TZSP_ENCAP_PPP:
            call_dissector(ppp_handle, next_tvb, pinfo, tree);
            break;

        case TZSP_ENCAP_FDDI:
            call_dissector(fddi_handle, next_tvb, pinfo, tree);
            break;

        case TZSP_ENCAP_RAW:
            call_dissector(raw_ip_handle, next_tvb, pinfo, tree);
            break;

        case TZSP_ENCAP_IEEE_802_11:
            /*
             * XXX - get some of the information from the TLVs
             * and turn it into a radio metadata header to
             * hand to the radio dissector, and call it?
             */
            call_dissector(ieee_802_11_handle, next_tvb, pinfo, tree);
            break;

        case TZSP_ENCAP_IEEE_802_11_PRISM:
            call_dissector(ieee_802_11_prism_handle, next_tvb, pinfo, tree);
            break;

        case TZSP_ENCAP_IEEE_802_11_AVS:
            call_dissector(ieee_802_11_avs_handle, next_tvb, pinfo, tree);
            break;

        default:
            col_set_str(pinfo->cinfo, COL_PROTOCOL, "UNKNOWN");
            col_add_fstr(pinfo->cinfo, COL_INFO, "TZSP_ENCAP = %u",
                    encapsulation);
            call_data_dissector(next_tvb, pinfo, tree);
        }
    }

    return tvb_captured_length(tvb);
}

/* ************************************************************************* */
/*                Register the TZSP dissector                                */
/* ************************************************************************* */

void
proto_register_tzsp(void)
{
    static const value_string msg_type[] = {
        {0, "Normal"},
        {1, "RFC1042 encoded"},
        {2, "Bridge-tunnel encoded"},
        {4, "802.11 management frame"},
        {0, NULL}
    };

    static const true_false_string pcf_flag = {
        "CF: Frame received during CF period",
        "Not CF"
    };

    static const true_false_string undecr_flag = {
        "Encrypted frame could not be decrypted",
        "Unencrypted"
    };

    static const true_false_string fcs_err_flag = {
        "FCS error, frame is corrupted",
        "Frame is valid"
    };

    static const value_string channels[] = {
        /* 802.11b/g */
        {  1, "1 (2.412 GHz)"},
        {  2, "2 (2.417 GHz)"},
        {  3, "3 (2.422 GHz)"},
        {  4, "4 (2.427 GHz)"},
        {  5, "5 (2.432 GHz)"},
        {  6, "6 (2.437 GHz)"},
        {  7, "7 (2.442 GHz)"},
        {  8, "8 (2.447 GHz)"},
        {  9, "9 (2.452 GHz)"},
        { 10, "10 (2.457 GHz)"},
        { 11, "11 (2.462 GHz)"},
        { 12, "12 (2.467 GHz)"},
        { 13, "13 (2.472 GHz)"},
        { 14, "14 (2.484 GHz)"},
        /* 802.11a */
        { 36, "36 (5.180 GHz)"},
        { 40, "40 (5.200 GHz)"},
        { 44, "44 (5.220 GHz)"},
        { 48, "48 (5.240 GHz)"},
        { 52, "52 (5.260 GHz)"},
        { 56, "56 (5.280 GHz)"},
        { 60, "60 (5.300 GHz)"},
        { 64, "64 (5.320 GHz)"},
        {149, "149 (5.745 GHz)"},
        {153, "153 (5.765 GHz)"},
        {157, "157 (5.785 GHz)"},
        {161, "161 (5.805 GHz)"},
        {0, NULL}
    };

    static const value_string rates[] = {
        /* Old PRISM rates */
        {0x0A, "1 Mbit/s"},
        {0x14, "2 Mbit/s"},
        {0x37, "5.5 Mbit/s"},
        {0x6E, "11 Mbit/s"},
        /* MicroAP rates */
        {  2,  "1 Mbit/s"},
        {  4,  "2 Mbit/s"},
        { 11,  "5.5 Mbit/s"},
        { 12,  "6 Mbit/s"},
        { 18,  "9 Mbit/s"},
        { 22,  "11 Mbit/s"},
        { 24,  "12 Mbit/s"},
        { 36,  "18 Mbit/s"},
        { 48,  "24 Mbit/s"},
        { 72,  "36 Mbit/s"},
        { 96,  "48 Mbit/s"},
        {108,  "54 Mbit/s"},
        {0, NULL}
    };

    static hf_register_info hf[] = {
        { &hf_tzsp_version, {
            "Version", "tzsp.version", FT_UINT8, BASE_DEC,
            NULL, 0, NULL, HFILL }},
        { &hf_tzsp_type, {
            "Type", "tzsp.type", FT_UINT8, BASE_DEC,
            VALS(tzsp_type), 0, NULL, HFILL }},
        { &hf_tzsp_encap, {
            "Encapsulation", "tzsp.encap", FT_UINT16, BASE_DEC,
            VALS(tzsp_encapsulation), 0, NULL, HFILL }},

        { &hf_option_tag, {
            "Option Tag", "tzsp.option_tag", FT_UINT8, BASE_DEC,
            VALS(option_tag_vals), 0, NULL, HFILL }},
        { &hf_option_length, {
            "Option Length", "tzsp.option_length", FT_UINT8, BASE_DEC,
            NULL, 0, NULL, HFILL }},
#if 0
        { &hf_status_field, {
            "Status", "tzsp.wlan.status", FT_UINT16, BASE_HEX,
                NULL, 0, NULL, HFILL }},
#endif
        { &hf_status_msg_type, {
            "Type", "tzsp.wlan.status.msg_type", FT_UINT8, BASE_HEX,
            VALS(msg_type), 0, "Message type", HFILL }},
#if 0
        { &hf_status_mac_port, {
            "Port", "tzsp.wlan.status.mac_port", FT_UINT8, BASE_DEC,
            NULL, 0, "MAC port", HFILL }},
#endif
        { &hf_status_pcf, {
            "PCF", "tzsp.wlan.status.pcf", FT_BOOLEAN, BASE_NONE,
            TFS (&pcf_flag), 0x0, "Point Coordination Function", HFILL }},
        { &hf_status_undecrypted, {
            "Undecrypted", "tzsp.wlan.status.undecrypted", FT_BOOLEAN, BASE_NONE,
            TFS (&undecr_flag), 0x0, NULL, HFILL }},
        { &hf_status_fcs_error, {
            "FCS", "tzsp.wlan.status.fcs_err", FT_BOOLEAN, BASE_NONE,
            TFS (&fcs_err_flag), 0x0, "Frame check sequence", HFILL }},
        { &hf_time, {
            "Time", "tzsp.wlan.time", FT_UINT32, BASE_HEX,
            NULL, 0, NULL, HFILL }},
        { &hf_silence, {
            "Silence", "tzsp.wlan.silence", FT_INT8, BASE_DEC,
            NULL, 0, NULL, HFILL }},
        { &hf_original_length, {
            "Original Length", "tzsp.original_length", FT_INT16, BASE_DEC,
            NULL, 0, "OrigLength", HFILL }},
        { &hf_signal, {
            "Signal", "tzsp.wlan.signal", FT_INT8, BASE_DEC,
            NULL, 0, NULL, HFILL }},
        { &hf_rate, {
            "Rate", "tzsp.wlan.rate", FT_UINT8, BASE_DEC,
            VALS(rates), 0, NULL, HFILL }},
        { &hf_channel, {
            "Channel", "tzsp.wlan.channel", FT_UINT8, BASE_DEC,
            VALS(channels), 0, NULL, HFILL }},
        { &hf_unknown, {
            "Unknown tag", "tzsp.unknown", FT_BYTES, BASE_NONE,
            NULL, 0, "Unknown", HFILL }},
        { &hf_sensormac, {
            "Sensor Address", "tzsp.sensormac", FT_ETHER, BASE_NONE,
            NULL, 0, "Sensor MAC", HFILL }}
    };

    static gint *ett[] = {
        &ett_tzsp,
        &ett_tag
    };

    proto_tzsp = proto_register_protocol("Tazmen Sniffer Protocol", "TZSP",
        "tzsp");
    proto_register_field_array(proto_tzsp, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));

    tzsp_handle = register_dissector("tzsp", dissect_tzsp, proto_tzsp);
}

void
proto_reg_handoff_tzsp(void)
{
    dissector_add_uint("udp.port", UDP_PORT_TZSP, tzsp_handle);

    /* Get the data dissector for handling various encapsulation types. */
    eth_maybefcs_handle = find_dissector_add_dependency("eth_maybefcs", proto_tzsp);
    tr_handle = find_dissector_add_dependency("tr", proto_tzsp);
    ppp_handle = find_dissector_add_dependency("ppp_hdlc", proto_tzsp);
    fddi_handle = find_dissector_add_dependency("fddi", proto_tzsp);
    raw_ip_handle = find_dissector_add_dependency("raw_ip", proto_tzsp);
    ieee_802_11_handle = find_dissector_add_dependency("wlan", proto_tzsp);
    ieee_802_11_prism_handle = find_dissector_add_dependency("prism", proto_tzsp);
    ieee_802_11_avs_handle = find_dissector_add_dependency("wlancap", proto_tzsp);

    /* Register this protocol as an encapsulation type. */
    dissector_add_uint("wtap_encap", WTAP_ENCAP_TZSP, tzsp_handle);
}

/*
* Editor modelines - http://www.wireshark.org/tools/modelines.html
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
