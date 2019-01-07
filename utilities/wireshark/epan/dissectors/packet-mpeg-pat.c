/* packet-mpeg-pat.c
 * Routines for MPEG2 (ISO/ISO 13818-1) Program Associate Table (PAT) dissection
 * Copyright 2012, Guy Martin <gmsoft@tuxicoman.be>
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <epan/packet.h>
#include "packet-mpeg-sect.h"

void proto_register_mpeg_pat(void);
void proto_reg_handoff_mpeg_pat(void);

static int proto_mpeg_pat = -1;
static int hf_mpeg_pat_transport_stream_id = -1;
static int hf_mpeg_pat_reserved = -1;
static int hf_mpeg_pat_version_number = -1;
static int hf_mpeg_pat_current_next_indicator = -1;
static int hf_mpeg_pat_section_number = -1;
static int hf_mpeg_pat_last_section_number = -1;

static int hf_mpeg_pat_program_number = -1;
static int hf_mpeg_pat_program_reserved = -1;
static int hf_mpeg_pat_program_map_pid = -1;


static gint ett_mpeg_pat = -1;
static gint ett_mpeg_pat_prog = -1;

#define MPEG_PAT_RESERVED_MASK                    0xC0
#define MPEG_PAT_VERSION_NUMBER_MASK              0x3E
#define MPEG_PAT_CURRENT_NEXT_INDICATOR_MASK      0x01

#define MPEG_PAT_PROGRAM_RESERVED_MASK          0xE000
#define MPEG_PAT_PROGRAM_MAP_PID_MASK           0x1FFF

static const true_false_string mpeg_pat_cur_next_vals = {

    "Currently applicable", "Not yet applicable"

};

static int
dissect_mpeg_pat(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
    guint offset = 0, length = 0;
    guint16 prog_num, prog_pid;

    proto_item *ti;
    proto_tree *mpeg_pat_tree;
    proto_tree *mpeg_pat_prog_tree;

    /* The TVB should start right after the section_length in the Section packet */

    col_set_str(pinfo->cinfo, COL_INFO, "Program Association Table (PAT)");

    ti = proto_tree_add_item(tree, proto_mpeg_pat, tvb, offset, -1, ENC_NA);
    mpeg_pat_tree = proto_item_add_subtree(ti, ett_mpeg_pat);

    offset += packet_mpeg_sect_header(tvb, offset, mpeg_pat_tree, &length, NULL);
    length -= 4;

    proto_tree_add_item(mpeg_pat_tree, hf_mpeg_pat_transport_stream_id, tvb, offset, 2, ENC_BIG_ENDIAN);
    offset += 2;

    proto_tree_add_item(mpeg_pat_tree, hf_mpeg_pat_reserved, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(mpeg_pat_tree, hf_mpeg_pat_version_number, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(mpeg_pat_tree, hf_mpeg_pat_current_next_indicator, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset += 1;

    proto_tree_add_item(mpeg_pat_tree, hf_mpeg_pat_section_number, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset += 1;

    proto_tree_add_item(mpeg_pat_tree, hf_mpeg_pat_last_section_number, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset += 1;

    if (offset >= length)
        return offset;


    /* Parse all the programs */
    while (offset < length) {

        prog_num = tvb_get_ntohs(tvb, offset);
        prog_pid = tvb_get_ntohs(tvb, offset + 2) & MPEG_PAT_PROGRAM_MAP_PID_MASK;

        mpeg_pat_prog_tree = proto_tree_add_subtree_format(mpeg_pat_tree, tvb, offset, 4,
                        ett_mpeg_pat_prog, NULL, "Program 0x%04hx -> PID 0x%04hx", prog_num, prog_pid);

        proto_tree_add_item(mpeg_pat_prog_tree, hf_mpeg_pat_program_number, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;

        proto_tree_add_item(mpeg_pat_prog_tree, hf_mpeg_pat_program_reserved, tvb, offset, 2, ENC_BIG_ENDIAN);
        proto_tree_add_item(mpeg_pat_prog_tree, hf_mpeg_pat_program_map_pid, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;

    }

    offset += packet_mpeg_sect_crc(tvb, pinfo, mpeg_pat_tree, 0, offset);
    proto_item_set_len(ti, offset);
    return tvb_captured_length(tvb);
}


void
proto_register_mpeg_pat(void)
{

    static hf_register_info hf[] = {

        { &hf_mpeg_pat_transport_stream_id, {
            "Transport Stream ID", "mpeg_pat.tsid",
            FT_UINT16, BASE_HEX, NULL, 0, NULL, HFILL
        } },

        { &hf_mpeg_pat_reserved, {
            "Reserved", "mpeg_pat.reserved",
            FT_UINT8, BASE_HEX, NULL, MPEG_PAT_RESERVED_MASK, NULL, HFILL
        } },

        { &hf_mpeg_pat_version_number, {
            "Version Number", "mpeg_pat.version",
            FT_UINT8, BASE_HEX, NULL, MPEG_PAT_VERSION_NUMBER_MASK, NULL, HFILL
        } },

        { &hf_mpeg_pat_current_next_indicator, {
            "Current/Next Indicator", "mpeg_pat.cur_next_ind",
            FT_BOOLEAN, 8, TFS(&mpeg_pat_cur_next_vals), MPEG_PAT_CURRENT_NEXT_INDICATOR_MASK, NULL, HFILL
        } },

        { &hf_mpeg_pat_section_number, {
            "Section Number", "mpeg_pat.sect_num",
            FT_UINT8, BASE_DEC, NULL, 0, NULL, HFILL
        } },

        { &hf_mpeg_pat_last_section_number, {
            "Last Section Number", "mpeg_pat.last_sect_num",
            FT_UINT8, BASE_DEC, NULL, 0, NULL, HFILL
        } },

        { &hf_mpeg_pat_program_number, {
            "Program Number", "mpeg_pat.prog_num",
            FT_UINT16, BASE_HEX, NULL, 0, NULL, HFILL
        } },

        { &hf_mpeg_pat_program_reserved, {
            "Reserved", "mpeg_pat.prog_reserved",
            FT_UINT16, BASE_HEX, NULL, MPEG_PAT_PROGRAM_RESERVED_MASK, NULL, HFILL
        } },

        { &hf_mpeg_pat_program_map_pid, {
            "Program Map PID", "mpeg_pat.prog_map_pid",
            FT_UINT16, BASE_HEX, NULL, MPEG_PAT_PROGRAM_MAP_PID_MASK, NULL, HFILL
        } },

    };

    static gint *ett[] = {
        &ett_mpeg_pat,
        &ett_mpeg_pat_prog
    };

    proto_mpeg_pat = proto_register_protocol("MPEG2 Program Association Table", "MPEG PAT", "mpeg_pat");

    proto_register_field_array(proto_mpeg_pat, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));

}


void proto_reg_handoff_mpeg_pat(void)
{
    dissector_handle_t mpeg_pat_handle;

    mpeg_pat_handle = create_dissector_handle(dissect_mpeg_pat, proto_mpeg_pat);
    dissector_add_uint("mpeg_sect.tid", MPEG_PAT_TID, mpeg_pat_handle);
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
