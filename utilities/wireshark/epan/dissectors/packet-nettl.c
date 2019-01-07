/* packet-nettl.c
 * Routines for nettl (HP-UX) record header dissection
 *
 * Original Author Mark C. Brown <mbrown@hp.com>
 * Copyright (C) 2005 Hewlett-Packard Development Company, L.P.
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * Copied from packet-pagp.c
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

#include "config.h"

#include <epan/packet.h>
#include <epan/ipproto.h>
#include <wiretap/nettl.h>

void proto_register_nettl(void);
void proto_reg_handoff_nettl(void);

/* Initialize the protocol and registered fields */

static int proto_nettl = -1;

static int hf_nettl_subsys = -1;
static int hf_nettl_devid = -1;
static int hf_nettl_kind = -1;
static int hf_nettl_pid = -1;
static int hf_nettl_uid = -1;

static dissector_handle_t eth_withoutfcs_handle;
static dissector_handle_t tr_handle;
static dissector_handle_t fddi_bitswapped_handle;
static dissector_handle_t lapb_handle;
static dissector_handle_t x25_handle;
static dissector_handle_t sctp_handle;
static dissector_handle_t raw_ip_handle;
static dissector_table_t ip_proto_dissector_table;
static dissector_table_t tcp_subdissector_table;

#define TCP_PORT_TELNET 23

/* Initialize the subtree pointers */

static gint ett_nettl = -1;

/* General declarations and macros */

static const value_string trace_kind[] = {
    { 0x80000000, "Incoming Header" },
    { 0x40000000, "Outgoing Header" },
    { 0x20000000, "Incoming PDU - PDUIN" },
    { 0x20000000, "PDUIN" },
    { 0x10000000, "Outgoing PDU - PDUOUT" },
    { 0x10000000, "PDUOUT" },
    { 0x08000000, "Procedure Trace" },
    { 0x04000000, "State Trace" },
    { 0x02000000, "Error Trace" },
    { 0x01000000, "Logging" },
    { 0x00800000, "Loopback" },
    { 0, NULL }
};

static const value_string subsystem[] = {
    {   0, "NS_LS_LOGGING" },
    {   1, "NS_LS_NFT" },
    {   2, "NS_LS_LOOPBACK" },
    {   3, "NS_LS_NI" },
    {   4, "NS_LS_IPC" },
    {   5, "NS_LS_SOCKREGD" },
    {   6, "NS_LS_TCP" },
    {   7, "NS_LS_PXP" },
    {   8, "NS_LS_UDP" },
    {   9, "NS_LS_IP" },
    {  10, "NS_LS_PROBE" },
    {  11, "NS_LS_DRIVER" },
    {  12, "NS_LS_RLBD" },
    {  13, "NS_LS_BUFS" },
    {  14, "NS_LS_CASE21" },
    {  15, "NS_LS_ROUTER21" },
    {  16, "NS_LS_NFS" },
    {  17, "NS_LS_NETISR" },
    {  18, "NS_LS_X25D" },
    {  19, "NS_LS_NSE" },
    {  20, "NS_LS_STRLOG" },
    {  21, "NS_LS_TIRDWR" },
    {  22, "NS_LS_TIMOD" },
    {  23, "NS_LS_ICMP" },
    {  24, "X25L2" },
    {  25, "X25L3" },
    {  26, "FILTER" },
    {  27, "NAME" },
    {  28, "ACC" },
    {  29, "NS_LS_IGMP" },
    {  31, "TOKEN" },
    {  32, "HIPPI" },
    {  33, "EISA_FC" },
    {  34, "SX25L2" },
    {  35, "SX25L3" },
    {  36, "NS_LS_SX25" },
    {  37, "100VG" },
    {  38, "EISA_ATM" },
    {  39, "SEAH_FDDI" },
    {  40, "TELECOM_HLR" },
    {  41, "TELECOM_SCE" },
    {  42, "TELECOM_SMS" },
    {  43, "TELECOM_NEM" },
    {  50, "FORE_ATM" },
    {  60, "TMOS_TOB" },
    {  62, "TELECOM_SCP" },
    {  63, "TELECOM_SS7" },
    {  64, "FTAM_INIT" },
    {  65, "FTAM_RESP" },
    {  70, "FTAM_VFS" },
    {  72, "FTAM_USER" },
    {  82, "OVS" },
    {  84, "OVEXTERNAL" },
    {  90, "OTS9000" },
    {  91, "OTS9000-NETWORK" },
    {  92, "OTS9000-TRANSPORT" },
    {  93, "OTS9000-SESSION" },
    {  94, "OTS9000-ACSE_PRES" },
    {  95, "FDDI" },
    { 116, "SHM" },
    { 119, "ACSE_US" },
    { 121, "HPS" },
    { 122, "CM" },
    { 123, "ULA_UTILS" },
    { 124, "EM" },
    { 129, "STREAMS" },
    { 164, "LAN100" },
    { 172, "EISA100BT" },
    { 173, "BASE100" },
    { 174, "EISA_FDDI" },
    { 176, "PCI_FDDI" },
    { 177, "HSC_FDDI" },
    { 178, "GSC100BT" },
    { 179, "PCI100BT" },
    { 180, "SPP100BT" },
    { 181, "GLE" },
    { 182, "FQE" },
    { 185, "GELAN" },
    { 187, "PCITR" },
    { 188, "HP_APA" },
    { 189, "HP_APAPORT" },
    { 190, "HP_APALACP" },
    { 210, "BTLAN" },
    { 233, "INTL100" },
    { 244, "NS_LS_IPV6" },
    { 245, "NS_LS_ICMPV6" },
    { 246, "DLPI" },
    { 247, "VLAN" },
    { 249, "NS_LS_LOOPBACK6" },
    { 250, "DHCPV6D" },
    { 252, "IGELAN" },
    { 253, "IETHER" },
    { 257, "WBEMProvider-LAN" },
    { 258, "SYSADMIN" },
    { 264, "LVMPROVIDER" },
    { 265, "IXGBE" },
    { 267, "NS_LS_TELNET" },
    { 268, "NS_LS_SCTP" },
    { 269, "HSSN" },
    { 270, "IGSSN" },
    { 271, "ICXGBE" },
    { 275, "IEXGBE" },
    { 277, "IOCXGBE" },
    { 278, "IQXGBE" },
    { 513, "KL_VM" },
    { 514, "KL_PKM" },
    { 515, "KL_DLKM" },
    { 516, "KL_PM" },
    { 517, "KL_VFS" },
    { 518, "KL_VXFS" },
    { 519, "KL_UFS" },
    { 520, "KL_NFS" },
    { 521, "KL_FSVM" },
    { 522, "KL_WSIO" },
    { 523, "KL_SIO" },
    { 524, "KL_NET" },
    { 525, "KL_MC" },
    { 526, "KL_DYNTUNE" },
    { 527, "KL_KEN" },
    { 0, NULL }
};
static value_string_ext subsystem_ext = VALUE_STRING_EXT_INIT(subsystem);

/* Code to actually dissect the nettl record headers */

static int
dissect_nettl(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
    if (tree) {
        proto_tree *nettl_tree;
        proto_item *nettl_item;

        nettl_item = proto_tree_add_protocol_format(tree, proto_nettl, tvb,
                           0, -1, "HP-UX Network Tracing and Logging (nettl) header");
        nettl_tree = proto_item_add_subtree(nettl_item, ett_nettl);
        proto_tree_add_uint_format_value(nettl_tree, hf_nettl_subsys, tvb,
                           0, 0, pinfo->pseudo_header->nettl.subsys, "%d (%s)",
                           pinfo->pseudo_header->nettl.subsys,
                           val_to_str_ext_const(pinfo->pseudo_header->nettl.subsys, &subsystem_ext, "Unknown"));
        proto_tree_add_int(nettl_tree, hf_nettl_devid, tvb,
                           0, 0, pinfo->pseudo_header->nettl.devid);
        proto_tree_add_uint_format_value(nettl_tree, hf_nettl_kind, tvb,
                           0, 0, pinfo->pseudo_header->nettl.kind,
                           "0x%08x (%s)", pinfo->pseudo_header->nettl.kind,
                           val_to_str_const(pinfo->pseudo_header->nettl.kind & ~NETTL_HDR_SUBSYSTEM_BITS_MASK, trace_kind, "Unknown"));
        proto_tree_add_int(nettl_tree, hf_nettl_pid, tvb,
                           0, 0, pinfo->pseudo_header->nettl.pid);
        proto_tree_add_uint(nettl_tree, hf_nettl_uid, tvb,
                           0, 0, pinfo->pseudo_header->nettl.uid);
    }

    switch (pinfo->pkt_encap) {
        case WTAP_ENCAP_NETTL_ETHERNET:
            call_dissector(eth_withoutfcs_handle, tvb, pinfo, tree);
            break;
        case WTAP_ENCAP_NETTL_TOKEN_RING:
            call_dissector(tr_handle, tvb, pinfo, tree);
            break;
        case WTAP_ENCAP_NETTL_FDDI:
            call_dissector(fddi_bitswapped_handle, tvb, pinfo, tree);
            break;
        case WTAP_ENCAP_NETTL_RAW_IP:
            if ( (pinfo->pseudo_header->nettl.kind & NETTL_HDR_PDU_MASK) == 0 )
                /* not actually a data packet (PDU) trace record */
                call_data_dissector(tvb, pinfo, tree);
            else if (pinfo->pseudo_header->nettl.subsys == NETTL_SUBSYS_NS_LS_SCTP )
                call_dissector(sctp_handle, tvb, pinfo, tree);
            else
                call_dissector(raw_ip_handle, tvb, pinfo, tree);
            break;
        case WTAP_ENCAP_NETTL_RAW_ICMP:
            if (!dissector_try_uint(ip_proto_dissector_table,
                                    IP_PROTO_ICMP, tvb, pinfo, tree))
                call_data_dissector(tvb, pinfo, tree);
            break;
        case WTAP_ENCAP_NETTL_RAW_ICMPV6:
            if (!dissector_try_uint(ip_proto_dissector_table,
                                    IP_PROTO_ICMPV6, tvb, pinfo, tree))
                call_data_dissector(tvb, pinfo, tree);
            break;
        case WTAP_ENCAP_NETTL_X25:
            if (pinfo->pseudo_header->nettl.kind == NETTL_HDR_PDUIN)
                pinfo->p2p_dir = P2P_DIR_RECV;
            else if (pinfo->pseudo_header->nettl.kind == NETTL_HDR_PDUOUT)
                pinfo->p2p_dir = P2P_DIR_SENT;
            if (pinfo->pseudo_header->nettl.subsys == NETTL_SUBSYS_SX25L2)
                call_dissector(lapb_handle, tvb, pinfo, tree);
            else
                call_dissector(x25_handle, tvb, pinfo, tree);
            break;
        case WTAP_ENCAP_NETTL_RAW_TELNET:
            if (!dissector_try_uint(tcp_subdissector_table,
                                    TCP_PORT_TELNET, tvb, pinfo, tree))
                call_data_dissector(tvb, pinfo, tree);
            break;
        default:
            col_set_str(pinfo->cinfo, COL_PROTOCOL, "UNKNOWN");
            col_add_fstr(pinfo->cinfo, COL_INFO, "Unsupported nettl subsytem: %d (%s)",
                         pinfo->pseudo_header->nettl.subsys,
                         val_to_str_ext_const(pinfo->pseudo_header->nettl.subsys, &subsystem_ext, "Unknown"));
            call_data_dissector(tvb, pinfo, tree);
    }
    return tvb_captured_length(tvb);
}


/* Register the protocol with Wireshark */

void
proto_register_nettl(void)
{
/* Setup list of header fields */

    static hf_register_info hf[] = {

        { &hf_nettl_subsys,
          { "Subsystem", "nettl.subsys", FT_UINT16, BASE_DEC | BASE_EXT_STRING, &subsystem_ext, 0x0,
            "HP-UX Subsystem/Driver", HFILL }},

        { &hf_nettl_devid,
          { "Device ID", "nettl.devid", FT_INT32, BASE_DEC, NULL, 0x0,
            "HP-UX Device ID", HFILL }},

        { &hf_nettl_kind,
          { "Trace Kind", "nettl.kind", FT_UINT32, BASE_HEX, VALS(trace_kind), 0x0,
            "HP-UX Trace record kind", HFILL}},

        { &hf_nettl_pid,
          { "Process ID (pid/ktid)", "nettl.pid", FT_INT32, BASE_DEC, NULL, 0x0,
            "HP-UX Process/thread id", HFILL}},

        { &hf_nettl_uid,
          { "User ID (uid)", "nettl.uid", FT_UINT16, BASE_DEC, NULL, 0x0,
            "HP-UX User ID", HFILL}}

    };

    /* Setup protocol subtree array */

    static gint *ett[] = {
        &ett_nettl
    };

    /* Register the protocol name and description */

    proto_nettl = proto_register_protocol("HP-UX Network Tracing and Logging", "nettl", "nettl");

    /* Required function calls to register the header fields and subtrees used */

    proto_register_field_array(proto_nettl, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));

}


void
proto_reg_handoff_nettl(void)
{
    dissector_handle_t nettl_handle;

    /*
     * Get handles for various dissectors and dissector tables.
     */
    eth_withoutfcs_handle    = find_dissector_add_dependency("eth_withoutfcs", proto_nettl);
    tr_handle                = find_dissector_add_dependency("tr", proto_nettl);
    fddi_bitswapped_handle   = find_dissector_add_dependency("fddi_bitswapped", proto_nettl);
    lapb_handle              = find_dissector_add_dependency("lapb", proto_nettl);
    x25_handle               = find_dissector_add_dependency("x.25", proto_nettl);
    sctp_handle              = find_dissector_add_dependency("sctp", proto_nettl);
    raw_ip_handle            = find_dissector_add_dependency("raw_ip", proto_nettl);
    ip_proto_dissector_table = find_dissector_table("ip.proto");
    tcp_subdissector_table   = find_dissector_table("tcp.port");

    nettl_handle = create_dissector_handle(dissect_nettl, proto_nettl);
    dissector_add_uint("wtap_encap", WTAP_ENCAP_NETTL_ETHERNET,   nettl_handle);
    dissector_add_uint("wtap_encap", WTAP_ENCAP_NETTL_TOKEN_RING, nettl_handle);
    dissector_add_uint("wtap_encap", WTAP_ENCAP_NETTL_FDDI,       nettl_handle);
    dissector_add_uint("wtap_encap", WTAP_ENCAP_NETTL_RAW_IP,     nettl_handle);
    dissector_add_uint("wtap_encap", WTAP_ENCAP_NETTL_RAW_ICMP,   nettl_handle);
    dissector_add_uint("wtap_encap", WTAP_ENCAP_NETTL_RAW_ICMPV6, nettl_handle);
    dissector_add_uint("wtap_encap", WTAP_ENCAP_NETTL_RAW_TELNET, nettl_handle);
    dissector_add_uint("wtap_encap", WTAP_ENCAP_NETTL_X25,        nettl_handle);
    dissector_add_uint("wtap_encap", WTAP_ENCAP_NETTL_UNKNOWN,    nettl_handle);

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
