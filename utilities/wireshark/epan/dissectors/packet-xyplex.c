/* packet-xyplex.c
 * Routines for xyplex packet dissection
 *
 * Copyright 2002 Randy McEoin <rmceoin@pe.com>
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * Copied from packet-tftp.c
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
#include <epan/conversation.h>

void proto_register_xyplex(void);
void proto_reg_handoff_xyplex(void);

static int proto_xyplex = -1;
static int hf_xyplex_type = -1;
static int hf_xyplex_pad = -1;
static int hf_xyplex_server_port = -1;
static int hf_xyplex_return_port = -1;
static int hf_xyplex_reserved = -1;
static int hf_xyplex_reply = -1;
static int hf_xyplex_data = -1;

static gint ett_xyplex = -1;

static dissector_handle_t xyplex_handle;

#define UDP_PORT_XYPLEX    173

#define XYPLEX_REG_OK           0x00
#define XYPLEX_REG_QUEFULL      0x05

static const value_string xyplex_reg_vals[] = {
  { XYPLEX_REG_OK,      "OK" },
  { XYPLEX_REG_QUEFULL, "Queue Full" },
  { 0,          NULL }
};

static int
dissect_xyplex(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
  proto_tree     *xyplex_tree;
  proto_item     *ti;
  conversation_t *conversation;
  gint            offset = 0;

  guint8  prototype;
  guint8  padding;
  guint16 server_port;
  guint16 return_port;
  guint16 reserved;
  guint16 reply;

  col_set_str(pinfo->cinfo, COL_PROTOCOL, "XYPLEX");

  ti = proto_tree_add_item(tree, proto_xyplex, tvb, offset, -1, ENC_NA);
  xyplex_tree = proto_item_add_subtree(ti, ett_xyplex);

  if (pinfo->destport == UDP_PORT_XYPLEX) {
    /* This is a registration request from a Unix server
     * to the Xyplex server.  The server_port indicates
     * which Xyplex serial port is desired.  The
     * return_port tells the Xyplex server what TCP port
     * to open to the Unix server.
     */
    prototype = tvb_get_guint8(tvb, offset);
    padding = tvb_get_guint8(tvb, offset+1);
    server_port = tvb_get_ntohs(tvb, offset+2);
    return_port = tvb_get_ntohs(tvb, offset+4);
    reserved = tvb_get_ntohs(tvb, offset+6);
    col_add_fstr(pinfo->cinfo, COL_INFO,
                 "Registration Request: %d Return: %d",
                 server_port, return_port);

    if (tree) {
      proto_tree_add_uint(xyplex_tree, hf_xyplex_type, tvb,
                          offset, 1, prototype);
      proto_tree_add_uint(xyplex_tree, hf_xyplex_pad, tvb,
                          offset+1, 1, padding);
      proto_tree_add_uint(xyplex_tree, hf_xyplex_server_port, tvb,
                          offset+2, 2, server_port);
      proto_tree_add_uint(xyplex_tree, hf_xyplex_return_port, tvb,
                          offset+4, 2, return_port);
      proto_tree_add_uint(xyplex_tree, hf_xyplex_reserved, tvb,
                          offset+6, 2, reserved);
    }
    offset += 8;

    /* Look for all future TCP conversations between the
     * requesting server and the Xyplex host using the
     * return_port.
     */
    conversation = find_conversation(pinfo->num, &pinfo->src, &pinfo->dst,
                                     PT_TCP, return_port, 0, NO_PORT_B);
    if (conversation == NULL) {
      conversation = conversation_new(pinfo->num, &pinfo->src, &pinfo->dst,
                                      PT_TCP, return_port, 0, NO_PORT2);
      conversation_set_dissector(conversation, xyplex_handle);
    }
    return offset;
  }

  if (pinfo->srcport == UDP_PORT_XYPLEX) {
    prototype = tvb_get_guint8(tvb, offset);
    padding = tvb_get_guint8(tvb, offset+1);
    reply = tvb_get_ntohs(tvb, offset+2);
    col_add_fstr(pinfo->cinfo, COL_INFO, "Registration Reply: %s",
                 val_to_str(reply, xyplex_reg_vals, "Unknown (0x%02x)"));

    if (tree) {
      proto_tree_add_uint(xyplex_tree, hf_xyplex_type, tvb,
                          offset, 1, prototype);
      proto_tree_add_uint(xyplex_tree, hf_xyplex_pad, tvb,
                          offset+1, 1, padding);
      proto_tree_add_uint(xyplex_tree, hf_xyplex_reply, tvb,
                          offset+2, 2, reply);
    }
    offset += 4;
    return offset;
  }

  /*
   * This must be the TCP data stream.  This will just be
   * the raw data being transferred from the remote server
   * and the Xyplex serial port.
   */
  col_add_fstr(pinfo->cinfo, COL_INFO, "%d > %d Data",
               pinfo->srcport, pinfo->destport);

  proto_tree_add_item(xyplex_tree, hf_xyplex_data, tvb, offset, -1, ENC_NA);

  return tvb_reported_length_remaining(tvb, offset);
}


void
proto_register_xyplex(void)
{
  static hf_register_info hf[] = {
    { &hf_xyplex_type,
      { "Type",       "xyplex.type",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        "Protocol type", HFILL }},

    { &hf_xyplex_pad,
      { "Pad",        "xyplex.pad",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        "Padding", HFILL }},

    { &hf_xyplex_server_port,
      { "Server Port",        "xyplex.server_port",
        FT_UINT16, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_xyplex_return_port,
      { "Return Port",   "xyplex.return_port",
        FT_UINT16, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_xyplex_reserved,
      { "Reserved field",  "xyplex.reserved",
        FT_UINT16, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_xyplex_reply,
      { "Registration Reply",  "xyplex.reply",
        FT_UINT16, BASE_DEC, VALS(xyplex_reg_vals), 0x0,
        NULL, HFILL }},

    { &hf_xyplex_data,
      { "Data",  "xyplex.data",
        FT_BYTES, BASE_NONE, NULL, 0x0,
        NULL, HFILL }},

  };
  static gint *ett[] = {
    &ett_xyplex,
  };

  proto_xyplex = proto_register_protocol("Xyplex", "XYPLEX", "xyplex");
  proto_register_field_array(proto_xyplex, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));
}

void
proto_reg_handoff_xyplex(void)
{
  xyplex_handle = create_dissector_handle(dissect_xyplex, proto_xyplex);
  dissector_add_uint("udp.port", UDP_PORT_XYPLEX, xyplex_handle);
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
