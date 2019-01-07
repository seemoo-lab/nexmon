/* packet-miop.c
 * Routines for CORBA MIOP packet disassembly
 * Significantly based on packet-giop.c
 * Copyright 2009 Alvaro Vega Garcia <avega at tid dot es>
 *
 * According with Unreliable Multicast Draft Adopted Specification
 * 2001 October (OMG)
 * Chapter 29: Unreliable Multicast Inter-ORB Protocol (MIOP)
 * http://www.omg.org/technology/documents/specialized_corba.htm
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

#include "packet-giop.h"

void proto_register_miop(void);
void proto_reg_handoff_miop(void);

/*
 * Useful visible data/structs
 */

#define MIOP_MAX_UNIQUE_ID_LENGTH   252

#define MIOP_HEADER_SIZE 16

/*
 * Set to 1 for DEBUG output - TODO make this a runtime option
 */

#define DEBUG   0

/*
 * ------------------------------------------------------------------------------------------+
 *                                 Data/Variables/Structs
 * ------------------------------------------------------------------------------------------+
 */


static int proto_miop = -1;

/*
 * (sub)Tree declares
 */


static gint hf_miop_magic = -1;
static gint hf_miop_hdr_version = -1;
static gint hf_miop_flags = -1;
static gint hf_miop_packet_length = -1;
static gint hf_miop_packet_number = -1;
static gint hf_miop_number_of_packets = -1;
static gint hf_miop_unique_id_len = -1;
static gint hf_miop_unique_id = -1;

static gint ett_miop = -1;

static expert_field ei_miop_version_not_supported = EI_INIT;
static expert_field ei_miop_unique_id_len_exceed_max_value = EI_INIT;

#define MIOP_MAGIC   0x4d494f50 /* "MIOP" */

static gboolean
dissect_miop_heur_check (tvbuff_t * tvb, packet_info * pinfo _U_, proto_tree * tree _U_, void * data _U_) {

  guint tot_len;
  guint32 magic;

  /* check magic number and version */


  tot_len = tvb_captured_length(tvb);

  if (tot_len < MIOP_HEADER_SIZE) /* tot_len < 16 */
    {
      /* Not enough data captured to hold the GIOP header; don't try
         to interpret it as GIOP. */
      return FALSE;
    }

    magic = tvb_get_ntohl(tvb,0);
    if(magic != MIOP_MAGIC){
        /* Not a MIOP packet. */
        return FALSE;
    }

  return TRUE;
}

/* Main entry point */
static int dissect_miop (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree, void* data _U_) {
  guint offset = 0;

  proto_tree *miop_tree = NULL;
  proto_item *ti;

  guint8 hdr_version;
  guint version_major;
  guint version_minor;

  guint8 flags;

  guint16 packet_length;
  guint packet_number;
  guint number_of_packets;
  guint byte_order;

  guint32 unique_id_len;

  wmem_strbuf_t *flags_strbuf = wmem_strbuf_new_label(wmem_packet_scope());
  wmem_strbuf_append(flags_strbuf, "none");

  if (!dissect_miop_heur_check(tvb, pinfo, tree, data))
      return 0;

  col_set_str (pinfo->cinfo, COL_PROTOCOL, "MIOP");
  /* Clear out stuff in the info column */
  col_clear(pinfo->cinfo, COL_INFO);

  /* Extract major and minor version numbers */
  hdr_version = tvb_get_guint8(tvb, 4);
  version_major = ((hdr_version & 0xf0) >> 4);
  version_minor =  (hdr_version & 0x0f);

  if (hdr_version != 16)
  {
      col_add_fstr (pinfo->cinfo, COL_INFO, "Version %u.%u",
                    version_major, version_minor);

      ti = proto_tree_add_item (tree, proto_miop, tvb, 0, -1, ENC_NA);
      miop_tree = proto_item_add_subtree (ti, ett_miop);
      proto_tree_add_expert_format(miop_tree, pinfo, &ei_miop_version_not_supported,
                               tvb, 0, -1,
                               "MIOP version %u.%u not supported",
                               version_major, version_minor);
      return 5;
  }

  flags = tvb_get_guint8(tvb, 5);
  byte_order = (flags & 0x01) ? ENC_LITTLE_ENDIAN : ENC_BIG_ENDIAN;

  if (byte_order == ENC_BIG_ENDIAN) {
    packet_length = tvb_get_ntohs(tvb, 6);
    packet_number = tvb_get_ntohl(tvb, 8);
    number_of_packets = tvb_get_ntohl(tvb, 12);
    unique_id_len = tvb_get_ntohl(tvb, 16);
  }
  else {
    packet_length = tvb_get_letohs(tvb, 6);
    packet_number = tvb_get_letohl(tvb, 8);
    number_of_packets = tvb_get_letohl(tvb, 12);
    unique_id_len = tvb_get_letohl(tvb, 16);
  }

  col_add_fstr (pinfo->cinfo, COL_INFO, "MIOP %u.%u Packet s=%d (%u of %u)",
                version_major, version_minor, packet_length,
                packet_number + 1,
                number_of_packets);

  if (tree)
    {

      ti = proto_tree_add_item (tree, proto_miop, tvb, 0, -1, ENC_NA);
      miop_tree = proto_item_add_subtree (ti, ett_miop);

      /* XXX - Should we bail out if we don't have the right magic number? */
      proto_tree_add_item(miop_tree, hf_miop_magic, tvb, offset, 4, ENC_ASCII|ENC_NA);
      offset += 4;
      proto_tree_add_uint_format_value(miop_tree, hf_miop_hdr_version, tvb, offset, 1, hdr_version,
                                 "%u.%u", version_major, version_minor);
      offset++;
      if (flags & 0x01) {
        wmem_strbuf_truncate(flags_strbuf, 0);
        wmem_strbuf_append(flags_strbuf, "little-endian");
      }
      if (flags & 0x02) {
        wmem_strbuf_append_printf(flags_strbuf, "%s%s",
                                  wmem_strbuf_get_len(flags_strbuf) ? ", " : "", "last message");
      }
      proto_tree_add_uint_format_value(miop_tree, hf_miop_flags, tvb, offset, 1,
                                       flags, "0x%02x (%s)", flags, wmem_strbuf_get_str(flags_strbuf));
      offset++;
      proto_tree_add_item(miop_tree, hf_miop_packet_length, tvb, offset, 2, byte_order);
      offset += 2;
      proto_tree_add_item(miop_tree, hf_miop_packet_number, tvb, offset, 4, byte_order);
      offset += 4;
      proto_tree_add_item(miop_tree, hf_miop_number_of_packets, tvb, offset, 4, byte_order);

      offset += 4;
      ti = proto_tree_add_item(miop_tree, hf_miop_unique_id_len, tvb, offset, 4, byte_order);

      if (unique_id_len >= MIOP_MAX_UNIQUE_ID_LENGTH) {
        expert_add_info_format(pinfo, ti, &ei_miop_unique_id_len_exceed_max_value,
                       "Unique Id length (%u) exceeds max value (%u)",
                       unique_id_len, MIOP_MAX_UNIQUE_ID_LENGTH);
        return offset;
      }

      offset += 4;
      proto_tree_add_item(miop_tree, hf_miop_unique_id, tvb, offset, unique_id_len,
                          byte_order);

      if (packet_number == 0) {
        /*  It is the first packet of the collection
            We can call to GIOP dissector to show more about this first
            uncompleted GIOP message
        */
        tvbuff_t *payload_tvb;

        offset += unique_id_len;
        payload_tvb = tvb_new_subset_remaining (tvb, offset);
        dissect_giop(payload_tvb, pinfo, tree);
      }
    }

   return tvb_captured_length(tvb);
}

static gboolean
dissect_miop_heur (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree, void * data _U_) {

  if (!dissect_miop_heur_check(tvb, pinfo, tree, data))
      return FALSE;

  dissect_miop (tvb, pinfo, tree, data);

  /* TODO: make reasembly */
  return TRUE;

}

void proto_register_miop (void) {


  /* A header field is something you can search/filter on.
   *
   * We create a structure to register our fields. It consists of an
   * array of hf_register_info structures, each of which are of the format
   * {&(field id), {name, abbrev, type, display, strings, bitmask, blurb, HFILL}}.
   */
  static hf_register_info hf[] = {
    { &hf_miop_magic,
      { "Magic", "miop.magic", FT_STRING, BASE_NONE, NULL, 0x0,
        "PacketHeader magic", HFILL }},
    { &hf_miop_hdr_version,
      { "Version", "miop.hdr_version", FT_UINT8, BASE_HEX, NULL, 0x0,
        "PacketHeader hdr_version", HFILL }},
    { &hf_miop_flags,
      { "Flags", "miop.flags", FT_UINT8, BASE_OCT, NULL, 0x0,
        "PacketHeader flags", HFILL }},
    { &hf_miop_packet_length,
      { "Length", "miop.packet_length", FT_UINT16, BASE_DEC, NULL, 0x0,
        "PacketHeader packet_length", HFILL }},
    { &hf_miop_packet_number,
      { "PacketNumber", "miop.packet_number",  FT_UINT32, BASE_DEC, NULL, 0x0,
        "PacketHeader packet_number", HFILL }},
    { &hf_miop_number_of_packets,
      { "NumberOfPackets", "miop.number_of_packets", FT_UINT32, BASE_DEC, NULL, 0x0,
        "PacketHeader number_of_packets", HFILL }},
    { &hf_miop_unique_id_len,
      { "UniqueIdLength", "miop.unique_id_len", FT_UINT32, BASE_DEC, NULL, 0x0,
        "UniqueId length", HFILL }},
    { &hf_miop_unique_id,
      { "UniqueId", "miop.unique_id", FT_BYTES, BASE_NONE, NULL, 0x0,
        "UniqueId id", HFILL }},
  };


  static gint *ett[] = {
    &ett_miop
  };

  static ei_register_info ei[] = {
     { &ei_miop_version_not_supported, { "miop.version.not_supported", PI_UNDECODED, PI_WARN, "MIOP version not supported", EXPFILL }},
     { &ei_miop_unique_id_len_exceed_max_value, { "miop.unique_id_len.exceed_max_value", PI_MALFORMED, PI_WARN, "Unique Id length exceeds max value", EXPFILL }},
  };

  expert_module_t* expert_miop;

  proto_miop = proto_register_protocol("Unreliable Multicast Inter-ORB Protocol", "MIOP", "miop");
  proto_register_field_array (proto_miop, hf, array_length (hf));
  proto_register_subtree_array (ett, array_length (ett));
  expert_miop = expert_register_protocol(proto_miop);
  expert_register_field_array(expert_miop, ei, array_length(ei));

  register_dissector("miop", dissect_miop, proto_miop);

}


void proto_reg_handoff_miop (void) {

  dissector_handle_t miop_handle;

  miop_handle = find_dissector("miop");
  dissector_add_for_decode_as("udp.port", miop_handle);

  heur_dissector_add("udp", dissect_miop_heur, "MIOP over UDP", "miop_udp", proto_miop, HEURISTIC_ENABLE);

}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 2
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=2 tabstop=8 expandtab:
 * :indentSize=2:tabSize=8:noTabs=true:
 */
