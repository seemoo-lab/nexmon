/* packet-pw-eth.c
 * Routines for ethernet PW dissection: it should conform to RFC 4448.
 *
 * Copyright 2008 _FF_
 *
 * Francesco Fondelli <francesco dot fondelli, gmail dot com>
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
#include <epan/addr_resolv.h>

#include "packet-mpls.h"

void proto_register_pw_eth(void);
void proto_reg_handoff_pw_eth(void);

static gint proto_pw_eth_cw = -1;
static gint proto_pw_eth_nocw = -1;
static gint proto_pw_eth_heuristic = -1;

static gint ett_pw_eth = -1;

static int hf_pw_eth = -1;
static int hf_pw_eth_cw = -1;
static int hf_pw_eth_cw_sequence_number = -1;

static dissector_handle_t eth_withoutfcs_handle;
static dissector_handle_t pw_eth_handle_cw;
static dissector_handle_t pw_eth_handle_nocw;

static int
dissect_pw_eth_cw(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
    tvbuff_t *next_tvb;
    guint16   sequence_number;

    if (tvb_reported_length_remaining(tvb, 0) < 4) {
        return 0;
    }

    if (dissect_try_cw_first_nibble(tvb, pinfo, tree))
        return tvb_captured_length(tvb);

    sequence_number = tvb_get_ntohs(tvb, 2);

    if (tree) {
        proto_tree *pw_eth_tree;
        proto_item *ti;

        ti = proto_tree_add_boolean(tree, hf_pw_eth_cw,
                                    tvb, 0, 0, TRUE);
        PROTO_ITEM_SET_HIDDEN(ti);
        ti = proto_tree_add_item(tree, proto_pw_eth_cw,
                                 tvb, 0, 4, ENC_NA);
        pw_eth_tree = proto_item_add_subtree(ti, ett_pw_eth);

        proto_tree_add_uint_format(pw_eth_tree,
                                   hf_pw_eth_cw_sequence_number,
                                   tvb, 2, 2, sequence_number,
                                   "Sequence Number: %d",
                                   sequence_number);
    }

    next_tvb = tvb_new_subset_remaining(tvb, 4);
    {
        call_dissector(eth_withoutfcs_handle, next_tvb, pinfo, tree);
    }

    return tvb_captured_length(tvb);
}

static int
dissect_pw_eth_nocw(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
    tvbuff_t *next_tvb;

    if (tree) {
        proto_item *ti;
        ti = proto_tree_add_boolean(tree, hf_pw_eth, tvb, 0, 0, TRUE);
        PROTO_ITEM_SET_HIDDEN(ti);
    }

    next_tvb = tvb_new_subset_remaining(tvb, 0);
    call_dissector(eth_withoutfcs_handle, next_tvb, pinfo, tree);

    return tvb_captured_length(tvb);
}

/*
 * FF: this function returns TRUE if the first 12 bytes in tvb looks like
 *     two valid ethernet addresses.  FALSE otherwise.
 */
static gboolean
looks_like_plain_eth(tvbuff_t *tvb _U_)
{
    const gchar *manuf_name_da;
    const gchar *manuf_name_sa;

    if (tvb_reported_length_remaining(tvb, 0) < 14) {
        return FALSE;
    }

    manuf_name_da = tvb_get_manuf_name_if_known(tvb, 0);
    manuf_name_sa = tvb_get_manuf_name_if_known(tvb, 6);

    if (manuf_name_da && manuf_name_sa) {
        return TRUE;
    }

    return FALSE;
}

static int
dissect_pw_eth_heuristic(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
    guint8 first_nibble = (tvb_get_guint8(tvb, 0) >> 4) & 0x0F;

    if (looks_like_plain_eth(tvb))
        call_dissector(pw_eth_handle_nocw, tvb, pinfo, tree);
    else if (first_nibble == 0)
        call_dissector(pw_eth_handle_cw, tvb, pinfo, tree);
    else
        call_dissector(pw_eth_handle_nocw, tvb, pinfo, tree);
    return tvb_captured_length(tvb);
}

void
proto_register_pw_eth(void)
{
    static hf_register_info hf[] = {
        {
            &hf_pw_eth,
            {
                "PW (ethernet)",
                "pweth", FT_BOOLEAN,
                BASE_NONE, NULL, 0x0, NULL, HFILL
            }
        },
        {
            &hf_pw_eth_cw,
            {
                "PW Control Word (ethernet)",
                "pweth.cw", FT_BOOLEAN,
                BASE_NONE, NULL, 0x0, NULL, HFILL
            }
        },
        {
            &hf_pw_eth_cw_sequence_number,
            {
                "PW sequence number (ethernet)",
                "pweth.cw.sequence_number", FT_UINT16,
                BASE_DEC, NULL, 0x0, NULL, HFILL
            }
        }
    };

    static gint *ett[] = {
        &ett_pw_eth
    };

    proto_pw_eth_cw =
        proto_register_protocol("PW Ethernet Control Word",
                                "Ethernet PW (with CW)",
                                "pwethcw");
    proto_pw_eth_nocw =
        proto_register_protocol("Ethernet PW (no CW)", /* not displayed */
                                "Ethernet PW (no CW)",
                                "pwethnocw");
    proto_pw_eth_heuristic =
        proto_register_protocol("Ethernet PW (CW heuristic)", /* not disp. */
                                "Ethernet PW (CW heuristic)",
                                "pwethheuristic");
    proto_register_field_array(proto_pw_eth_cw, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
    register_dissector("pw_eth_heuristic", dissect_pw_eth_heuristic,
                       proto_pw_eth_heuristic);
}

void
proto_reg_handoff_pw_eth(void)
{
    dissector_handle_t pw_eth_handle_heuristic;

    eth_withoutfcs_handle = find_dissector_add_dependency("eth_withoutfcs", proto_pw_eth_cw);

    pw_eth_handle_cw = create_dissector_handle( dissect_pw_eth_cw, proto_pw_eth_cw );
    dissector_add_for_decode_as("mpls.label", pw_eth_handle_cw);

    pw_eth_handle_nocw = create_dissector_handle( dissect_pw_eth_nocw, proto_pw_eth_nocw );
    dissector_add_for_decode_as("mpls.label", pw_eth_handle_nocw);

    pw_eth_handle_heuristic = find_dissector("pw_eth_heuristic");
    dissector_add_for_decode_as("mpls.label", pw_eth_handle_heuristic);
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
