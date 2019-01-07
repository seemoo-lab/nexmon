/* packet-omapi.c
 * ISC OMAPI (Object Management API) dissector
 * Copyright 2006, Jaap Keuter <jaap.keuter@xs4all.nl>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/*
 * From the description api+protocol.
 * All fields are 32 bit unless stated otherwise.
 *
 * On startup, each side sends a status message indicating what version
 * of the protocol they are speaking. The status message looks like this:
 * +---------+---------+
 * | version | hlength |
 * +---------+---------+
 *
 * The fixed-length header consists of:
 * +--------+----+--------+----+-----+---------+------------+------------+-----+
 * | authid | op | handle | id | rid | authlen | msg values | obj values | sig |
 * +--------+----+--------+----+-----+---------+------v-----+-----v------+--v--+
 * NOTE: real life capture shows order to be: authid, authlen, opcode, handle...
 *
 * The message and object values consists of:
 * +---------+------+----------+-------+
 * | namelen | name | valuelen | value |
 * +---16b---+--v---+----------+---v---+
 */

#include "config.h"

#include <epan/packet.h>
#include <epan/ptvcursor.h>

void proto_register_omapi(void);
void proto_reg_handoff_omapi(void);

static int proto_omapi = -1;
static int hf_omapi_version = -1;
static int hf_omapi_hlength = -1;
static int hf_omapi_auth_id = -1;
static int hf_omapi_auth_len = -1;
static int hf_omapi_opcode = -1;
static int hf_omapi_handle = -1;
static int hf_omapi_id = -1;
static int hf_omapi_rid = -1;
static int hf_omapi_msg_name_len = -1; /* 16bit */
static int hf_omapi_msg_name = -1;
static int hf_omapi_msg_value_len = -1;
static int hf_omapi_msg_value = -1;
static int hf_omapi_obj_name_len = -1; /* 16bit */
static int hf_omapi_obj_name = -1;
static int hf_omapi_obj_value_len = -1;
static int hf_omapi_obj_value = -1;
static int hf_omapi_signature = -1;

/* Generated from convert_proto_tree_add_text.pl */
static int hf_omapi_empty_string = -1;
static int hf_omapi_object_end_tag = -1;
static int hf_omapi_message_end_tag = -1;
static int hf_omapi_no_value = -1;

static gint ett_omapi = -1;

#define OMAPI_PORT 7911

#define OP_OPEN             1
#define OP_REFRESH          2
#define OP_UPDATE           3
#define OP_NOTIFY           4
#define OP_ERROR            5
#define OP_DELETE           6
#define OP_NOTIFY_CANCEL    7
#define OP_NOTIFY_CANCELLED 8

static const value_string omapi_opcode_vals[] = {
  { OP_OPEN,             "Open" },
  { OP_REFRESH,          "Refresh" },
  { OP_UPDATE,           "Update" },
  { OP_NOTIFY,           "Notify" },
  { OP_ERROR,            "Error" },
  { OP_DELETE,           "Delete" },
  { OP_NOTIFY_CANCEL,    "Notify cancel" },
  { OP_NOTIFY_CANCELLED, "Notify cancelled" },
  { 0, NULL }
};

static int
dissect_omapi(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
  proto_item  *ti;
  proto_tree  *omapi_tree;
  ptvcursor_t *cursor;

  guint32 authlength;
  guint32 msglength;
  guint32 objlength;

    /* Payload too small for OMAPI */
  if (tvb_reported_length_remaining(tvb, 0) < 8)
    return 0;

  col_set_str(pinfo->cinfo, COL_PROTOCOL, "OMAPI");

  col_clear(pinfo->cinfo, COL_INFO);

  ti = proto_tree_add_item(tree, proto_omapi, tvb, 0, -1, ENC_NA);
  omapi_tree = proto_item_add_subtree(ti, ett_omapi);
  cursor = ptvcursor_new(omapi_tree, tvb, 0);

  if (tvb_reported_length_remaining(tvb, 0) < 24)
  {
    /* This is a startup message */
    ptvcursor_add(cursor, hf_omapi_version, 4, ENC_BIG_ENDIAN);
    ptvcursor_add(cursor, hf_omapi_hlength, 4, ENC_BIG_ENDIAN);

    col_set_str(pinfo->cinfo, COL_INFO, "Status message");
    proto_item_append_text(ti, ", Status message");

    ptvcursor_free(cursor);
    return 8;
  }
  else if ( !(tvb_get_ntohl(tvb, 8) || tvb_get_ntohl(tvb, 12)) )
  {
    /* This is a startup message, and more */
    ptvcursor_add(cursor, hf_omapi_version, 4, ENC_BIG_ENDIAN);
    ptvcursor_add(cursor, hf_omapi_hlength, 4, ENC_BIG_ENDIAN);

    col_append_str(pinfo->cinfo, COL_INFO, "Status message");

    proto_item_append_text(ti, ", Status message");
  }

  ptvcursor_add(cursor, hf_omapi_auth_id, 4, ENC_BIG_ENDIAN);
  authlength = tvb_get_ntohl(tvb, ptvcursor_current_offset(cursor));
  ptvcursor_add(cursor, hf_omapi_auth_len, 4, ENC_BIG_ENDIAN);

  col_append_sep_str(pinfo->cinfo, COL_INFO, NULL,
      val_to_str(tvb_get_ntohl(tvb, ptvcursor_current_offset(cursor)), omapi_opcode_vals, "Unknown opcode (0x%04x)"));

  proto_item_append_text(ti, ", Opcode: %s",
    val_to_str(tvb_get_ntohl(tvb, ptvcursor_current_offset(cursor)), omapi_opcode_vals, "Unknown opcode (0x%04x)"));

  ptvcursor_add(cursor, hf_omapi_opcode, 4, ENC_BIG_ENDIAN);
  ptvcursor_add(cursor, hf_omapi_handle, 4, ENC_BIG_ENDIAN);
  ptvcursor_add(cursor, hf_omapi_id, 4, ENC_BIG_ENDIAN);
  ptvcursor_add(cursor, hf_omapi_rid, 4, ENC_BIG_ENDIAN);

  msglength = tvb_get_ntohs(tvb, ptvcursor_current_offset(cursor));
  while (msglength)
  {
    ptvcursor_add(cursor, hf_omapi_msg_name_len, 2, ENC_BIG_ENDIAN);
    ptvcursor_add(cursor, hf_omapi_msg_name, msglength, ENC_ASCII|ENC_NA);
    msglength = tvb_get_ntohl(tvb, ptvcursor_current_offset(cursor));
    ptvcursor_add(cursor, hf_omapi_msg_value_len, 4, ENC_BIG_ENDIAN);

    if (msglength == 0)
    {
      proto_tree_add_item(omapi_tree, hf_omapi_empty_string, tvb, 0, 0, ENC_NA);
    }
    else if (msglength == (guint32)~0)
    {
      proto_tree_add_item(omapi_tree, hf_omapi_no_value, tvb, 0, 0, ENC_NA);
    }
    else
    {
      ptvcursor_add(cursor, hf_omapi_msg_value, msglength, ENC_ASCII|ENC_NA);
    }

    msglength = tvb_get_ntohs(tvb, ptvcursor_current_offset(cursor));
  }

  ptvcursor_add(cursor, hf_omapi_message_end_tag, 2, ENC_NA);

  objlength = tvb_get_ntohs(tvb, ptvcursor_current_offset(cursor));
  while (objlength)
  {
    ptvcursor_add(cursor, hf_omapi_obj_name_len, 2, ENC_BIG_ENDIAN);
    ptvcursor_add(cursor, hf_omapi_obj_name, objlength, ENC_ASCII|ENC_NA);
    objlength = tvb_get_ntohl(tvb, ptvcursor_current_offset(cursor));
    ptvcursor_add(cursor, hf_omapi_obj_value_len, 4, ENC_BIG_ENDIAN);

    if (objlength == 0)
    {
      proto_tree_add_item(omapi_tree, hf_omapi_empty_string, tvb, 0, 0, ENC_NA);
    }
    else if (objlength == (guint32)~0)
    {
      proto_tree_add_item(omapi_tree, hf_omapi_no_value, tvb, 0, 0, ENC_NA);
    }
    else
    {
      ptvcursor_add(cursor, hf_omapi_obj_value, objlength, ENC_NA);
    }

    objlength = tvb_get_ntohs(tvb, ptvcursor_current_offset(cursor));
  }

  ptvcursor_add(cursor, hf_omapi_object_end_tag, 2, ENC_NA);

  if (authlength > 0) {
    ptvcursor_add(cursor, hf_omapi_signature, authlength, ENC_NA);
  }

  ptvcursor_free(cursor);
  return tvb_captured_length(tvb);
}

void
proto_register_omapi(void)
{
  static hf_register_info hf[] = {
    { &hf_omapi_version,
      { "Version", "omapi.version",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},
    { &hf_omapi_hlength,
      { "Header length", "omapi.hlength",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},
    { &hf_omapi_auth_id,
      { "Authentication ID", "omapi.authid",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},
    { &hf_omapi_auth_len,
      { "Authentication length", "omapi.authlength",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},
    { &hf_omapi_opcode,
      { "Opcode", "omapi.opcode",
        FT_UINT32, BASE_DEC, VALS(omapi_opcode_vals), 0x0,
        NULL, HFILL }},
    { &hf_omapi_handle,
      { "Handle", "omapi.handle",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},
    { &hf_omapi_id,
      { "ID", "omapi.id",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},
    { &hf_omapi_rid,
      { "Response ID", "omapi.rid",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},
    { &hf_omapi_msg_name_len,
      { "Message name length", "omapi.msg_name_length",
        FT_UINT16, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},
    { &hf_omapi_msg_name,
      { "Message name", "omapi.msg_name",
        FT_STRING, BASE_NONE, NULL, 0x0,
        NULL, HFILL }},
    { &hf_omapi_msg_value_len,
      { "Message value length", "omapi.msg_value_length",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},
    { &hf_omapi_msg_value,
      { "Message value", "omapi.msg_value",
        FT_STRING, BASE_NONE, NULL, 0x0,
        NULL, HFILL }},
    { &hf_omapi_obj_name_len,
      { "Object name length", "omapi.obj_name_length",
        FT_UINT16, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},
    { &hf_omapi_obj_name,
      { "Object name", "omapi.obj_name",
        FT_STRING, BASE_NONE, NULL, 0x0,
        NULL, HFILL }},
    { &hf_omapi_obj_value_len,
      { "Object value length", "omapi.object_value_length",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},
    { &hf_omapi_obj_value,
      { "Object value", "omapi.obj_value",
        FT_BYTES, BASE_NONE, NULL, 0x0,
        NULL, HFILL }},
    { &hf_omapi_signature,
      { "Signature", "omapi.signature",
        FT_BYTES, BASE_NONE, NULL, 0x0,
        NULL, HFILL }},

      /* Generated from convert_proto_tree_add_text.pl */
      { &hf_omapi_empty_string, { "Empty string", "omapi.empty_string", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }},
      { &hf_omapi_no_value, { "No value", "omapi.no_value", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }},
      { &hf_omapi_message_end_tag, { "Message end tag", "omapi.message_end_tag", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }},
      { &hf_omapi_object_end_tag, { "Object end tag", "omapi.object_end_tag", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }},

  };

  static gint *ett[] = {
    &ett_omapi
  };

  proto_omapi = proto_register_protocol("ISC Object Management API", "OMAPI", "omapi");
  proto_register_field_array(proto_omapi, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));
}

void
proto_reg_handoff_omapi(void)
{
  dissector_handle_t omapi_handle;

  omapi_handle = create_dissector_handle(dissect_omapi, proto_omapi);
  dissector_add_uint("tcp.port", OMAPI_PORT, omapi_handle);
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
