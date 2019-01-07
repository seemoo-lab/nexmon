/* packet-fmtp.c
 *
 * Routines for FMTP version 2 packet dissection.
 *
 * The specifications of this public protocol can be found on Eurocontrol web site:
 * http://www.eurocontrol.int/sites/default/files/publication/files/20070614-fmtp-spec-v2.0.pdf
 *
 * Copyright 2011, Christophe Paletou <c.paletou@free.fr>
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
#include "packet-tcp.h"

void proto_register_fmtp(void);
void proto_reg_handoff_fmtp(void);

static int proto_fmtp = -1;
static int hf_fmtp_pdu_version = -1;
static int hf_fmtp_pdu_reserved = -1;
static int hf_fmtp_pdu_type = -1;
static int hf_fmtp_pdu_length = -1;
static gint ett_fmtp = -1;

/* #define TCP_PORT_FMTP       8500 */
#define FMTP_HEADER_LEN     5
#define FMTP_MAX_DATA_LEN   10240
#define FMTP_MAX_LEN        FMTP_HEADER_LEN + FMTP_MAX_DATA_LEN

#define FMTP_TYP_OPERATIONAL    1
#define FMTP_TYP_OPERATOR       2
#define FMTP_TYP_IDENTIFICATION 3
#define FMTP_TYP_SYSTEM         4

#define INFO_STR_SIZE        1024

static const value_string packet_type_names[] = {
    { FMTP_TYP_OPERATIONAL,    "Operational message" },
    { FMTP_TYP_OPERATOR,       "Operator message" },
    { FMTP_TYP_IDENTIFICATION, "Identification message" },
    { FMTP_TYP_SYSTEM        , "System message" },
    { 0, NULL }
};

static const value_string system_message_names[] = {
    { 12337, "Startup" },   /* 0x3031 */
    { 12336, "Shutdown" },  /* 0x3030 */
    { 12339, "Heartbeat" }, /* 0x3033 */
    { 0, NULL }
};

static int
dissect_fmtp_message(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
    guint8      packet_type;
    guint16     packet_len;
    tvbuff_t   *next_tvb;
    proto_item *ti = NULL;
    proto_tree *fmtp_tree = NULL;

    packet_type = tvb_get_guint8(tvb, 4);
    packet_len  = tvb_get_ntohs(tvb, 2);

    col_set_str(pinfo->cinfo, COL_PROTOCOL, "FMTP");

    /* Clear out stuff in the info column */
    col_clear(pinfo->cinfo, COL_INFO);

    ti = proto_tree_add_item(tree, proto_fmtp, tvb, 0, -1, ENC_NA);
    proto_item_append_text(ti, ", %s",
        val_to_str(packet_type, packet_type_names, "Unknown (0x%02x)"));

    switch (packet_type) {

        case FMTP_TYP_IDENTIFICATION:
            proto_item_append_text(ti, " (%s)",
                tvb_get_string_enc(wmem_packet_scope(), tvb, FMTP_HEADER_LEN, packet_len-FMTP_HEADER_LEN, ENC_ASCII));
            col_add_fstr(pinfo->cinfo, COL_INFO, "%s (%s)",
                val_to_str(packet_type, packet_type_names, "Unknown (0x%02x)"),
                tvb_get_string_enc(wmem_packet_scope(), tvb, FMTP_HEADER_LEN, packet_len-FMTP_HEADER_LEN, ENC_ASCII));
            break;

        case FMTP_TYP_SYSTEM:
            proto_item_append_text(ti, " (%s)",
                tvb_get_string_enc(wmem_packet_scope(), tvb, FMTP_HEADER_LEN, packet_len-FMTP_HEADER_LEN, ENC_ASCII));
            col_add_fstr(pinfo->cinfo, COL_INFO, "%s (%s)",
                val_to_str(packet_type, packet_type_names, "Unknown (0x%02x)"),
                val_to_str(tvb_get_ntohs(tvb, FMTP_HEADER_LEN), system_message_names, "Unknown (0x%02x)"));
            break;

        default:
            col_add_fstr(pinfo->cinfo, COL_INFO, "%s",
                val_to_str(packet_type, packet_type_names, "Unknown (0x%02x)"));
            break;
    }
    if (tree) { /* we are being asked for details */
        fmtp_tree = proto_item_add_subtree(ti, ett_fmtp);
        proto_tree_add_item(fmtp_tree, hf_fmtp_pdu_version,  tvb, 0, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item(fmtp_tree, hf_fmtp_pdu_reserved, tvb, 1, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item(fmtp_tree, hf_fmtp_pdu_length,   tvb, 2, 2, ENC_BIG_ENDIAN);
        proto_tree_add_item(fmtp_tree, hf_fmtp_pdu_type,     tvb, 4, 1, ENC_BIG_ENDIAN);

        next_tvb = tvb_new_subset_remaining(tvb, FMTP_HEADER_LEN);
        call_data_dissector(next_tvb, pinfo, fmtp_tree);
    }

    return tvb_captured_length(tvb);
}

static guint
get_fmtp_message_len(packet_info *pinfo _U_, tvbuff_t *tvb, int offset, void *data _U_)
{
    return (guint)tvb_get_ntohs(tvb, offset+2);
}

static gboolean
dissect_fmtp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data)
{
    guint16 length;

    if (tvb_captured_length(tvb) < 5)
        return FALSE;
    /*
     * Check that packet looks like FMTP before going further
     */
    /* VERSION must currently be 0x02 */
    if (tvb_get_guint8(tvb, 0) != 0x02) return (FALSE);
    /* RESERVED must currently be 0x00 */
    if (tvb_get_guint8(tvb, 1) != 0x00) return (FALSE);
    length = tvb_get_ntohs(tvb, 2);
    /* LENGTH must currently not exceed 5 (header) + 10240 (data) */
    if ((length > FMTP_MAX_LEN) || (length < FMTP_HEADER_LEN)) return (FALSE);
    /* TYP must currently be in range 0x01-0x04 */
    if ((tvb_get_guint8(tvb, 4) < 0x01) || (tvb_get_guint8(tvb, 4) > 0x04))
        return (FALSE);

    tcp_dissect_pdus(tvb, pinfo, tree, TRUE, FMTP_HEADER_LEN,
                     get_fmtp_message_len, dissect_fmtp_message, data);
    return (TRUE);
}

void
proto_register_fmtp(void)
{
    static hf_register_info hf[] = {
        { &hf_fmtp_pdu_version,
            { "Version", "fmtp.version",
            FT_UINT8, BASE_DEC,
            NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_fmtp_pdu_reserved,
            { "Reserved", "fmtp.reserved",
            FT_UINT8, BASE_DEC,
            NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_fmtp_pdu_length,
            { "Length", "fmtp.length",
            FT_UINT16, BASE_DEC,
            NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_fmtp_pdu_type,
            { "Type", "fmtp.type",
            FT_UINT8, BASE_DEC,
            VALS(packet_type_names), 0x0,
            NULL, HFILL }
        }
    };

    /* Setup protocol subtree array */
    static gint *ett[] = {
        &ett_fmtp
    };

    proto_fmtp = proto_register_protocol(
        "Flight Message Transfer Protocol (FMTP)",
        "FMTP",
        "fmtp");

    proto_register_field_array(proto_fmtp, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
}

void
proto_reg_handoff_fmtp(void)
{
    /* Register as heuristic dissector for TCP */
    heur_dissector_add("tcp", dissect_fmtp, "FMTP over TCP", "fmtp_tcp", proto_fmtp, HEURISTIC_ENABLE);
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

