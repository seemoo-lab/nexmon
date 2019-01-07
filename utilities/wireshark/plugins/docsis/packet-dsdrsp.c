/* packet-dsdrsp.c
 * Routines for Dynamic Service Delete Response dissection
 * Copyright 2002, Anand V. Narwani <anand[AT]narwani.org>
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

void proto_register_docsis_dsdrsp(void);
void proto_reg_handoff_docsis_dsdrsp(void);

/* Initialize the protocol and registered fields */
static int proto_docsis_dsdrsp = -1;
static int hf_docsis_dsdrsp_tranid = -1;
static int hf_docsis_dsdrsp_confcode = -1;
static int hf_docsis_dsdrsp_rsvd = -1;

extern value_string docsis_conf_code[];

/* Initialize the subtree pointers */
static gint ett_docsis_dsdrsp = -1;

/* Dissection */
static int
dissect_dsdrsp (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree, void* data _U_)
{
  proto_item *it;
  proto_tree *dsdrsp_tree;
  guint16 tranid;
  guint8 confcode;

  tranid = tvb_get_ntohs (tvb, 0);
  confcode = tvb_get_guint8 (tvb, 2);

  col_add_fstr (pinfo->cinfo, COL_INFO,
                "Dynamic Service Delete Response Tran id = %u (%s)",
                tranid, val_to_str (confcode, docsis_conf_code, "%d"));

  if (tree)
    {
      it =
        proto_tree_add_protocol_format (tree, proto_docsis_dsdrsp, tvb, 0, -1,
                                        "DSD Response");
      dsdrsp_tree = proto_item_add_subtree (it, ett_docsis_dsdrsp);
      proto_tree_add_item (dsdrsp_tree, hf_docsis_dsdrsp_tranid, tvb, 0, 2,
                           ENC_BIG_ENDIAN);
      proto_tree_add_item (dsdrsp_tree, hf_docsis_dsdrsp_confcode, tvb, 2, 1,
                           ENC_BIG_ENDIAN);
      proto_tree_add_item (dsdrsp_tree, hf_docsis_dsdrsp_rsvd, tvb, 3, 1,
                           ENC_BIG_ENDIAN);
    }

    return tvb_captured_length(tvb);
}

/* Register the protocol with Wireshark */
void
proto_register_docsis_dsdrsp (void)
{
  static hf_register_info hf[] = {
    {&hf_docsis_dsdrsp_tranid,
     {"Transaction Id", "docsis_dsdrsp.tranid",
      FT_UINT16, BASE_DEC, NULL, 0x0,
      NULL, HFILL}
    },
    {&hf_docsis_dsdrsp_confcode,
     {"Confirmation Code", "docsis_dsdrsp.confcode",
      FT_UINT8, BASE_DEC, VALS (docsis_conf_code), 0x0,
      NULL, HFILL}
    },
    {&hf_docsis_dsdrsp_rsvd,
     {"Reserved", "docsis_dsdrsp.rsvd",
      FT_UINT8, BASE_DEC, NULL, 0x0,
      NULL, HFILL}
    },
  };

  static gint *ett[] = {
    &ett_docsis_dsdrsp,
  };

  proto_docsis_dsdrsp =
    proto_register_protocol ("DOCSIS Dynamic Service Delete Response",
                             "DOCSIS DSD-RSP", "docsis_dsdrsp");

  proto_register_field_array (proto_docsis_dsdrsp, hf, array_length (hf));
  proto_register_subtree_array (ett, array_length (ett));

  register_dissector ("docsis_dsdrsp", dissect_dsdrsp, proto_docsis_dsdrsp);
}

void
proto_reg_handoff_docsis_dsdrsp (void)
{
  dissector_handle_t docsis_dsdrsp_handle;

  docsis_dsdrsp_handle = find_dissector ("docsis_dsdrsp");
  dissector_add_uint ("docsis_mgmt", 0x16, docsis_dsdrsp_handle);

}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local Variables:
 * c-basic-offset: 2
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * ex: set shiftwidth=2 tabstop=8 expandtab:
 * :indentSize=2:tabSize=8:noTabs=true:
 */
