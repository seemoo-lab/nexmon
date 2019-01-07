/* packet-netanalyzer.c
 * Dissector for Hilscher netANALYZER frames.
 * Copyright 2008-2011, Hilscher GmbH, Holger Pfrommer hpfrommer[AT]hilscher.com
 *
 * Packet structure:
 * +---------------------------+
 * |           Header          |
 * |         (4 Octets)        |
 * +---------------------------+
 * |           Payload         |
 * .                           .
 * .                           .
 * .                           .
 *
 * Description:
 * The header field contains a 32-bit value in little-endian byte order.
 * The low-order 8 bits are a set of error flags for the packet:
 *     0x00000001 - MII RX_ER
 *     0x00000002 - alignment error
 *     0x00000004 - FCS error
 *     0x00000008 - frame too long
 *     0x00000010 - SFD error
 *     0x00000020 - frame shorter than 64 bytes
 *     0x00000040 - preamble shorter than 7 bytes
 *     0x00000080 - preamble longer than 7 bytes/li>
 * The next bit, 0x00000100, is set if the packet arrived on the GPIO port rather tha the Ethernet port.
 * The next bit, 0x00000200, is set if the packet was received in transparent capture mode.
 *   That should never be set for LINKTYPE_NETANALYZER and should always be set for LINKTYPE_NETANALYZER_TRANSPARENT.
 * The next 4 bits, 0x00003C00, are a bitfield giving the version of the header field; the current version is 1.
 * The next 2 bits, 0x0000C000, are the capture port/GPIO number, from 0 to 3.
 * The next 12 bits, 0x0FFF0000, are the frame length, in bytes.
 * The topmost 4 bits, 0xF0000000, are reserved.
 * The payload is an Ethernet frame, beginning with the MAC header and ending with the FCS, for LINKTYPE_NETANALYZER,
 *   and an Ethernet frame, beginning with the preamble and ending with the FCS, for LINKTYPE_NETANALYZER_TRANSPARENT.
 *
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald[AT]wireshark.org>
 * Copyright 1999 Gerald Combs
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
#include <wiretap/wtap.h>

void proto_register_netanalyzer(void);
void proto_reg_handoff_netanalyzer(void);

#define HEADER_SIZE  4
#define INFO_TYPE_OFFSET    18

#define MSK_RX_ERR             0x01
#define MSK_ALIGN_ERR          0x02
#define MSK_FCS_ERROR          0x04
#define MSK_TOO_LONG           0x08
#define MSK_SFD_ERROR          0x10
#define MSK_SHORT_FRAME        0x20
#define MSK_SHORT_PREAMBLE     0x40
#define MSK_LONG_PREAMBLE      0x80

static const char *msk_strings[] = {
  "MII RX_ER error",                /* 0x01 */
  "Alignment error",                /* 0x02 */
  "FCS error",                      /* 0x04 */
  "Frame too long",                 /* 0x08 */
  "No valid SFD found",             /* 0x10 */
  "Frame smaller 64 bytes",         /* 0x20 */
  "Preamble shorter than 7 bytes",  /* 0x40 */
  "Preamble longer than 7 bytes"    /* 0x80 */
};

#define SRT_PORT_NUM           6
#define SRT_VERSION            2
#define SRT_GPIO_FLAG          0
#define MSK_PACKET_STATUS      0xff
#define MSK_LENGTH             0x0fff
#define MSK_TRANSPARENT_MODE   0x02


static const value_string gpio_number[] = {
  { 0x0, "GPIO 0" },
  { 0x1, "GPIO 1" },
  { 0x2, "GPIO 2" },
  { 0x3, "GPIO 3" },
  { 0,   NULL }
};

static const value_string gpio_edge_vals[] = {
  { 0x0, "rising edge" },
  { 0x1, "falling edge" },
  { 0,   NULL }
};


static dissector_handle_t  eth_dissector_handle;

static gint  proto_netanalyzer           = -1;

static gint  hf_netanalyzer_gpio_number              = -1;
static gint  hf_netanalyzer_gpio_edge                = -1;
static gint  hf_netanalyzer_port                     = -1;
static gint  hf_netanalyzer_length                   = -1;
static gint  hf_netanalyzer_status                   = -1;
static gint  hf_netanalyzer_status_rx_err            = -1;
static gint  hf_netanalyzer_status_align_err         = -1;
static gint  hf_netanalyzer_status_fcs               = -1;
static gint  hf_netanalyzer_status_too_long          = -1;
static gint  hf_netanalyzer_status_sfd_error         = -1;
static gint  hf_netanalyzer_status_short_frame       = -1;
static gint  hf_netanalyzer_status_short_preamble    = -1;
static gint  hf_netanalyzer_status_long_preamble     = -1;

static gint  ett_netanalyzer                         = -1;
static gint  ett_netanalyzer_status                  = -1;
static gint  ett_netanalyzer_transparent             = -1;

static expert_field ei_netanalyzer_header_version_wrong = EI_INIT;
static expert_field ei_netanalyzer_gpio_def_none = EI_INIT;
static expert_field ei_netanalyzer_header_version_none = EI_INIT;
static expert_field ei_netanalyzer_transparent_frame = EI_INIT;
static expert_field ei_netanalyzer_alignment_error = EI_INIT;

/* common routine for Ethernet and transparent mode */
static int
dissect_netanalyzer_common(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  proto_item              *ti = NULL;
  proto_tree              *netanalyzer_header_tree = NULL;
  proto_item              *ti_status = NULL;
  proto_tree              *netanalyzer_status_tree = NULL;
  guint32                 packet_status;
  guint32                 port_num;
  guint32                 frame_length;
  guint                   is_gpio;
  guint32                 offset;
  guint                   gpio_num;
  guint                   gpio_edge;
  guint                   version;
  guint                   idx;

  if (tree)
  {
    /* generate netANALYZER tree */
    ti = proto_tree_add_item(tree, proto_netanalyzer, tvb, 0, HEADER_SIZE, ENC_NA);
    netanalyzer_header_tree = proto_item_add_subtree(ti, ett_netanalyzer);

    is_gpio = (tvb_get_guint8(tvb, 1) >> SRT_GPIO_FLAG) & 0x1;

    if (!is_gpio)
    {
      /* normal packet, no GPIO */

      /* decode version */
      version = (tvb_get_guint8(tvb, 1) >> SRT_VERSION) & 0xf;
      if (version != 1)
      {
        /* something is wrong */
        expert_add_info(pinfo, ti, &ei_netanalyzer_header_version_wrong);
        return FALSE;
      }

      /* decode port */
      port_num = (tvb_get_guint8(tvb, 1) >> SRT_PORT_NUM) & 0x3;
      proto_tree_add_uint(netanalyzer_header_tree, hf_netanalyzer_port, tvb, 0, 4, port_num);
      proto_item_append_text(ti, " (Port: %u, ", port_num);

      /* decode length */
      frame_length = tvb_get_letohs(tvb, 2) & MSK_LENGTH;
      proto_tree_add_uint(netanalyzer_header_tree, hf_netanalyzer_length, tvb, 0, 4, frame_length);
      proto_item_append_text(ti, "Length: %u byte%s, ", frame_length, (frame_length == 1) ? "" : "s");

      /* decode status */
      proto_item_append_text(ti, "Status: ");
      packet_status = tvb_get_guint8(tvb, 0);
      if (packet_status == 0)
      {
        ti_status = proto_tree_add_uint_format_value(netanalyzer_header_tree, hf_netanalyzer_status, tvb, 0, 1,
                                               packet_status, "No Error");
        proto_item_append_text(ti, "No Error)");
      }
      else
      {
        wmem_strbuf_t      *strbuf;
        gboolean            first = TRUE;

        ti_status = proto_tree_add_uint_format_value(netanalyzer_header_tree, hf_netanalyzer_status, tvb, 0, 1,
                                               packet_status, "Error present (expand tree for details)");
        strbuf = wmem_strbuf_new_label(wmem_epan_scope());
        for (idx = 0; idx < 8; idx++)
        {
          if (packet_status & (1 << idx))
          {
            if (first)
            {
              first = FALSE;
            }
            else
            {
              wmem_strbuf_append(strbuf, ", ");
            }
            wmem_strbuf_append(strbuf, msk_strings[idx]);
          }
        }
        proto_item_append_text(ti, "%s)", wmem_strbuf_get_str(strbuf));
      }

      netanalyzer_status_tree = proto_item_add_subtree(ti_status, ett_netanalyzer_status);
      proto_tree_add_item(netanalyzer_status_tree, hf_netanalyzer_status_rx_err, tvb, 0, 1, ENC_LITTLE_ENDIAN);
      proto_tree_add_item(netanalyzer_status_tree, hf_netanalyzer_status_align_err, tvb, 0, 1, ENC_LITTLE_ENDIAN);
      proto_tree_add_item(netanalyzer_status_tree, hf_netanalyzer_status_fcs, tvb, 0, 1, ENC_LITTLE_ENDIAN);
      proto_tree_add_item(netanalyzer_status_tree, hf_netanalyzer_status_too_long, tvb, 0, 1, ENC_LITTLE_ENDIAN);
      proto_tree_add_item(netanalyzer_status_tree, hf_netanalyzer_status_sfd_error, tvb, 0, 1, ENC_LITTLE_ENDIAN);
      proto_tree_add_item(netanalyzer_status_tree, hf_netanalyzer_status_short_frame, tvb, 0, 1, ENC_LITTLE_ENDIAN);
      proto_tree_add_item(netanalyzer_status_tree, hf_netanalyzer_status_short_preamble, tvb, 0, 1, ENC_LITTLE_ENDIAN);
      proto_tree_add_item(netanalyzer_status_tree, hf_netanalyzer_status_long_preamble, tvb, 0, 1, ENC_LITTLE_ENDIAN);

      /* decode transparent mode */
      if (tvb_get_guint8(tvb, 1) & MSK_TRANSPARENT_MODE)
      {
        proto_tree_add_expert(netanalyzer_header_tree, pinfo, &ei_netanalyzer_transparent_frame, tvb, 0, 4);
        proto_item_append_text(ti, ", Transparent Mode");

        if (packet_status & MSK_ALIGN_ERR)
        {
          proto_tree_add_expert(netanalyzer_header_tree, pinfo, &ei_netanalyzer_alignment_error, tvb, tvb_captured_length(tvb)-1, 1);
        }
      }
    }
    else
    {
      guchar *szTemp;

      /* GPIO pseudo packet */
      /* check consistency */
      if ( (tvb_get_guint8(tvb, 10) == 0x00) &&
           (tvb_get_guint8(tvb, 11) == 0x02) &&
           (tvb_get_guint8(tvb, 12) == 0xa2) &&
           (tvb_get_guint8(tvb, 13) == 0xff) &&
           (tvb_get_guint8(tvb, 14) == 0xff) &&
           (tvb_get_guint8(tvb, 15) == 0xff) &&
           (tvb_get_guint8(tvb, 16) == 0x88) &&
           (tvb_get_guint8(tvb, 17) == 0xff) &&
           (tvb_get_guint8(tvb, INFO_TYPE_OFFSET) == 0x00) )
      {
#define MAX_BUFFER 255
        szTemp=(guchar *)wmem_alloc(wmem_epan_scope(), MAX_BUFFER);

        /* everything ok */
        col_set_str(pinfo->cinfo, COL_PROTOCOL, "netANALYZER");

        offset = INFO_TYPE_OFFSET;

        /* GPIO number */
        offset++;
        proto_tree_add_item (netanalyzer_header_tree, hf_netanalyzer_gpio_number, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        gpio_num = (tvb_get_guint8(tvb, offset) & 0x03);

        /* GPIO edge */
        offset++;
        ti = proto_tree_add_item (netanalyzer_header_tree, hf_netanalyzer_gpio_edge, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        gpio_edge = (tvb_get_guint8(tvb, offset) & 0x01);

        g_snprintf(szTemp, MAX_BUFFER,
                   "GPIO event on GPIO %d (%sing edge)", gpio_num, (gpio_edge == 0x00) ? "ris" : "fall");

        col_add_fstr(pinfo->cinfo, COL_INFO, "%s", szTemp);
        proto_item_append_text(ti, " %s", szTemp);
      }
      else
      {
        /* something is wrong */
        expert_add_info(pinfo, ti, &ei_netanalyzer_gpio_def_none);
      }
      return FALSE;
    }
  }
  return TRUE;
}


/* Ethernet capture mode */
static int
dissect_netanalyzer(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
  tvbuff_t                *next_tvb;

  if (tvb_reported_length(tvb) >= 4)
  {
    /* generate tvb subset for Ethernet frame */
    if (dissect_netanalyzer_common(tvb, pinfo, tree))
    {
      /* hand off to eth dissector with the new tvb subset */
      next_tvb = tvb_new_subset_remaining(tvb, 4);
      call_dissector(eth_dissector_handle, next_tvb, pinfo, tree);
    }
  }
  else
  {
    /* something is wrong */
    proto_tree_add_expert_format(tree, pinfo, &ei_netanalyzer_header_version_none, tvb, 4, -1,
        "netANALYZER - No netANALYZER header found");
  }
  return tvb_captured_length(tvb);
}


/* Transparent capture mode */
static int
dissect_netanalyzer_transparent(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
  proto_tree              *transparent_payload_tree = NULL;
  tvbuff_t                *next_tvb;

  if (tvb_reported_length(tvb) >= 4)
  {
    /* generate tvb subset for Ethernet frame */
    if (dissect_netanalyzer_common(tvb, pinfo, tree))
    {
      /* do not hand off transparent packet for further Ethernet dissectors
       * as normally the transparent mode is used for low level analysis
       * where dissecting the frame's content wouldn't make much sense
       * use data dissector instead */
      transparent_payload_tree = proto_tree_add_subtree(tree, tvb, 4, tvb_captured_length(tvb)-4,
                                    ett_netanalyzer_transparent, NULL, "Raw packet data");
      next_tvb = tvb_new_subset_remaining(tvb, 4);
      call_data_dissector(next_tvb, pinfo, transparent_payload_tree);

      col_set_str(pinfo->cinfo, COL_PROTOCOL, "netANALYZER");
      col_set_str(pinfo->cinfo, COL_INFO, "Frame captured in transparent mode");
    }
  }
  else
  {
    /* something is wrong */
    proto_tree_add_expert_format(tree, pinfo, &ei_netanalyzer_header_version_none, tvb, 4, -1,
        "netANALYZER transparent mode - No netANALYZER header found");
  }
  return tvb_captured_length(tvb);
}


void proto_register_netanalyzer(void)
{
  static hf_register_info hf[] = {
    { &hf_netanalyzer_gpio_number,
      { "Event on", "netanalyzer.gpio_event.gpio_number",
        FT_UINT8, BASE_HEX, VALS(gpio_number), 0x0,
        "Event on GPIO number", HFILL }
    },
    { &hf_netanalyzer_gpio_edge,
      { "Event type", "netanalyzer.gpio_event.gpio_edge",
        FT_UINT8, BASE_HEX, VALS(gpio_edge_vals), 0x0,
        "Edge of GPIO event", HFILL }
    },
    { &hf_netanalyzer_port,
      { "Reception Port", "netanalyzer.port",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        "netANALYZER reception port", HFILL }
    },
    { &hf_netanalyzer_length,
      { "Ethernet frame length", "netanalyzer.framelen",
        FT_UINT16, BASE_DEC, NULL, 0x0,
        "Actual Ethernet frame length", HFILL }
    },
    { &hf_netanalyzer_status,
      { "Status", "netanalyzer.packetstatus",
        FT_UINT8, BASE_HEX, NULL, MSK_PACKET_STATUS,
        "Status of Ethernet frame", HFILL }
    },
    { &hf_netanalyzer_status_rx_err,
      { "MII RX_ER error", "netanalyzer.packetstatus.rx_er",
        FT_BOOLEAN, 8, NULL, MSK_RX_ERR,
        "RX_ER detected in frame", HFILL }
    },
    { &hf_netanalyzer_status_align_err,
      { "Alignment error", "netanalyzer.packetstatus.alignment_error",
        FT_BOOLEAN, 8, NULL, MSK_ALIGN_ERR,
        NULL, HFILL }
    },
    { &hf_netanalyzer_status_fcs,
      { "FCS error", "netanalyzer.packetstatus.fcs_error",
        FT_BOOLEAN, 8, NULL, MSK_FCS_ERROR,
        NULL, HFILL }
    },
    { &hf_netanalyzer_status_too_long,
      { "Frame too long", "netanalyzer.packetstatus.too_long",
        FT_BOOLEAN, 8, NULL, MSK_TOO_LONG,
        "Frame too long (capture truncated)", HFILL }
    },
    { &hf_netanalyzer_status_sfd_error,
      { "No valid SFD found", "netanalyzer.packetstatus.sfd_error",
        FT_BOOLEAN, 8, NULL, MSK_SFD_ERROR,
        "SDF error detected in frame", HFILL }
    },
    { &hf_netanalyzer_status_short_frame,
      { "Frame smaller 64 bytes", "netanalyzer.packetstatus.short_frame",
        FT_BOOLEAN, 8, NULL, MSK_SHORT_FRAME,
        NULL, HFILL }
    },
    { &hf_netanalyzer_status_short_preamble,
      { "Preamble shorter than 7 bytes", "netanalyzer.packetstatus.short_preamble",
        FT_BOOLEAN, 8, NULL, MSK_SHORT_PREAMBLE,
        NULL, HFILL }
    },
    { &hf_netanalyzer_status_long_preamble,
      { "Preamble longer than 7 bytes", "netanalyzer.packetstatus.long_preamble",
        FT_BOOLEAN, 8, NULL, MSK_LONG_PREAMBLE,
        NULL, HFILL }
    },
  };

  static gint *ett[] = {
    &ett_netanalyzer,
    &ett_netanalyzer_status,
    &ett_netanalyzer_transparent,
  };

  static ei_register_info ei[] = {
    { &ei_netanalyzer_header_version_wrong, { "netanalyzer.header_version.wrong", PI_PROTOCOL, PI_ERROR, "Wrong netANALYZER header version", EXPFILL }},
    { &ei_netanalyzer_gpio_def_none, { "netanalyzer.gpio_def_none", PI_MALFORMED, PI_ERROR, "No valid netANALYZER GPIO definition found", EXPFILL }},
    { &ei_netanalyzer_header_version_none, { "netanalyzer.header_version.none", PI_MALFORMED, PI_ERROR, "No netANALYZER header found", EXPFILL }},
    { &ei_netanalyzer_transparent_frame, { "netanalyzer.transparent_frame", PI_PROTOCOL, PI_NOTE, "This frame was captured in transparent mode", EXPFILL }},
    { &ei_netanalyzer_alignment_error, { "netanalyzer.alignment_error", PI_PROTOCOL, PI_WARN, "Displayed frame data contains additional nibble due to alignment error (upper nibble is not valid)", EXPFILL }},
  };

  expert_module_t* expert_netanalyzer;

  proto_netanalyzer = proto_register_protocol (
    "netANALYZER",            /* name */
    "netANALYZER",            /* short name */
    "netanalyzer" );          /* abbrev */

  proto_register_field_array(proto_netanalyzer, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));
  expert_netanalyzer = expert_register_protocol(proto_netanalyzer);
  expert_register_field_array(expert_netanalyzer, ei, array_length(ei));

}


void proto_reg_handoff_netanalyzer(void)
{
  dissector_handle_t netana_handle;
  dissector_handle_t netana_handle_transparent;

  eth_dissector_handle  = find_dissector_add_dependency("eth_withfcs", proto_netanalyzer);

  netana_handle             = create_dissector_handle(dissect_netanalyzer,             proto_netanalyzer);
  netana_handle_transparent = create_dissector_handle(dissect_netanalyzer_transparent, proto_netanalyzer);
  dissector_add_uint("wtap_encap", WTAP_ENCAP_NETANALYZER,             netana_handle);
  dissector_add_uint("wtap_encap", WTAP_ENCAP_NETANALYZER_TRANSPARENT, netana_handle_transparent);
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
