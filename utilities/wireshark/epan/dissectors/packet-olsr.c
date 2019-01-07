/* packet-olsr.c
 * Routines for OLSR (IPv4 & IPv6 compatible) RFC parsing
 * Compatible with RFC-compliant OLSR implementations such as
 * NRLOLSRD (http://pf.itd.nrl.navy.mil/projects/olsr/).
 * Parser created by Aaron Woo <woo@itd.nrl.navy.mil> of
 * the Naval Research Laboratory
 * Currently maintained by Jeff Weston <weston@itd.nrl.navy.mil>.
 *
 * Updated to Olsr.org and NRLOLSR packages by Henning Rogge <rogge@fgan.de>
 * http://www.ietf.org/rfc/rfc3626.txt
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
#include <epan/prefs.h>
#include <epan/expert.h>
#include <epan/to_str.h>

void proto_register_olsr(void);
void proto_reg_handoff_olsr(void);

#define UDP_PORT_OLSR   698
#define HELLO   1
#define TC      2
#define MID     3
#define HNA     4

#define OLSR_ORG_NAMESERVICE 130
#define OLSR_ORG_LQ_HELLO    201
#define OLSR_ORG_LQ_TC       202
#define NRLOLSR_TC_EXTRA     241

/* Initialize the protocol and registered fields */
static int proto_olsr = -1;

static int hf_olsr_packet_len = -1;
static int hf_olsr_packet_seq_num = -1;

static int hf_olsr_message = -1;
static int hf_olsr_message_type = -1;
static int hf_olsr_vtime = -1;
static int hf_olsr_message_size = -1;
static int hf_olsr_ttl = -1;
static int hf_olsr_hop_count = -1;
static int hf_olsr_message_seq_num = -1;

static int hf_olsr_htime = -1;
static int hf_olsr_willingness = -1;

static int hf_olsr_link_type = -1;
static int hf_olsr_link_message_size = -1;
static int hf_olsr_ansn = -1;

static int hf_olsr_neighbor = -1;
static int hf_olsr_origin_addr = -1;
static int hf_olsr_neighbor_addr = -1;
static int hf_olsr_interface_addr = -1;
static int hf_olsr_netmask = -1;
static int hf_olsr_network_addr = -1;

/* static int hf_olsr_neighbor6 = -1; */
static int hf_olsr_origin6_addr = -1;
static int hf_olsr_neighbor6_addr = -1;
static int hf_olsr_interface6_addr = -1;
static int hf_olsr_netmask6 = -1;
static int hf_olsr_network6_addr = -1;

static int hf_olsrorg_lq = -1;
static int hf_olsrorg_nlq = -1;
static int hf_nrlolsr_f1 = -1;
static int hf_nrlolsr_f2 = -1;

static int hf_olsrorg_ns_version = -1;
static int hf_olsrorg_ns_count = -1;

static int hf_olsrorg_ns = -1;
static int hf_olsrorg_ns_type = -1;
static int hf_olsrorg_ns_length = -1;
static int hf_olsrorg_ns_ip = -1;
static int hf_olsrorg_ns_ip6 = -1;
static int hf_olsrorg_ns_content = -1;

static int hf_olsr_data = -1;

/* Initialize the subtree pointers*/
static gint ett_olsr = -1;
static gint ett_olsr_message[G_MAXUINT8 + 1];
static gint ett_olsr_message_linktype = -1;
static gint ett_olsr_message_neigh = -1;
static gint ett_olsr_message_neigh6 = -1;
static gint ett_olsr_message_ns = -1;

static expert_field ei_olsr_not_enough_bytes = EI_INIT;
static expert_field ei_olsrorg_ns_version = EI_INIT;
static expert_field ei_olsr_data_misaligned = EI_INIT;

static const value_string message_type_vals[] = {
    { HELLO,                "HELLO" },
    { TC,                   "TC" },
    { MID,                  "MID" },
    { HNA,                  "HNA" },
    { OLSR_ORG_LQ_HELLO,    "HELLO (LQ, olsr.org)" },
    { OLSR_ORG_LQ_TC,       "TC (LQ, olsr.org)" },
    { OLSR_ORG_NAMESERVICE, "Nameservice (olsr.org)" },
    { NRLOLSR_TC_EXTRA,     "TC (LQ, nrlolsr)" },
    { 0, NULL }
};

static const value_string link_type_vals[] = {
    {  0, "Unspecified Link" },
    {  1, "Asymmetric Link" },
    {  3, "Lost Link" },
    {  5, "Pending Link" },
    {  6, "Symmetric Link" },
    { 10, "MPR Link" },
    { 0, NULL }
};

static const value_string willingness_type_vals[] = {
    { 0, "never" },
    { 7, "always" },
    { 0, NULL }
};

static const value_string nameservice_type_vals[] = {
    { 0, "HOST" },
    { 1, "FORWARDER" },
    { 2, "SERVICE" },
    { 3, "LATLON" },
    { 0, NULL }
};

static gboolean global_olsr_olsrorg = TRUE;
static gboolean global_olsr_nrlolsr = TRUE;

static double getOlsrTime(guint8 timeval) {
  int high_bits, low_bits;

  high_bits = ((timeval & 0xF0) >> 4);
  low_bits  = (timeval & 0x0F);
  return ((G_GUINT64_CONSTANT(1) << low_bits) / 16.0) * (1 + (high_bits / 16.0));
}

/*------------------------- TC Dissecting Code-------------------------*/
static int dissect_olsr_tc(tvbuff_t *tvb, packet_info *pinfo, proto_tree *olsr_tree, int offset, int message_end) {
  if (message_end - offset < 4) {
    proto_tree_add_expert_format(olsr_tree, pinfo, &ei_olsr_not_enough_bytes, tvb, offset, message_end - offset,
        "Not enough bytes for TC");
    return message_end;
  }

  proto_tree_add_item(olsr_tree, hf_olsr_ansn, tvb, offset, 2, ENC_BIG_ENDIAN);
  offset += 4;

  while (offset < message_end) {
    if (message_end - offset < pinfo->src.len) {
      proto_tree_add_expert_format(olsr_tree, pinfo, &ei_olsr_not_enough_bytes, tvb, offset, message_end - offset,
          "Not enough bytes for last neighbor");
      return message_end;
    }
    if (pinfo->src.type == AT_IPv4) {
      proto_tree_add_item(olsr_tree, hf_olsr_neighbor_addr, tvb, offset, 4, ENC_BIG_ENDIAN);
      offset += 4;
    } else if (pinfo->src.type == AT_IPv6) {
      proto_tree_add_item(olsr_tree, hf_olsr_neighbor6_addr, tvb, offset, 16, ENC_NA);
      offset += 16;
    } else {
      break; /* unknown address type should be handled in dissect_olsr, just be sure */
    }
  }
  return message_end;
}

static int dissect_olsrorg_lq_tc(tvbuff_t *tvb, packet_info *pinfo, proto_tree *olsr_tree, int offset, int message_end) {

  if (message_end - offset < 4) {
    proto_tree_add_expert_format(olsr_tree, pinfo, &ei_olsr_not_enough_bytes, tvb, offset, message_end - offset,
        "Not enough bytes for Olsr.org LQ-TC");
    return message_end;
  }
  proto_tree_add_item(olsr_tree, hf_olsr_ansn, tvb, offset, 2, ENC_BIG_ENDIAN);
  offset += 4;

  while (offset < message_end) {
    proto_item *address_group;
    proto_tree *address_tree;
    guint8 lq, nlq;

    if (pinfo->src.type == AT_IPv4) {
      if (message_end - offset < 8) {
        proto_tree_add_expert_format(olsr_tree, pinfo, &ei_olsr_not_enough_bytes, tvb, offset, message_end - offset,
            "Not enough bytes for last entry (need 8 bytes)");
        return message_end;
      }
      lq = tvb_get_guint8(tvb, offset + 4);
      nlq = tvb_get_guint8(tvb, offset + 5);

      address_group = proto_tree_add_bytes_format_value(olsr_tree, hf_olsr_neighbor, tvb, offset, 8,
          NULL, "%s (%d/%d)", tvb_ip_to_str(tvb, offset), lq, nlq);

      address_tree = proto_item_add_subtree(address_group, ett_olsr_message_neigh);

      proto_tree_add_item(address_tree, hf_olsr_neighbor_addr, tvb, offset, 4, ENC_BIG_ENDIAN);
      offset += 4;
    } else if (pinfo->src.type == AT_IPv6) {
      if (message_end - offset < 20) {
        proto_tree_add_expert_format(olsr_tree, pinfo, &ei_olsr_not_enough_bytes, tvb, offset, message_end - offset,
            "Not enough bytes for last entry (need 20 bytes)");
        return message_end;
      }
      lq = tvb_get_guint8(tvb, offset + 16);
      nlq = tvb_get_guint8(tvb, offset + 17);

      address_group = proto_tree_add_bytes_format_value(olsr_tree, hf_olsr_neighbor, tvb, offset, 20,
          NULL, "%s (%d/%d)", tvb_ip6_to_str(tvb, offset), lq, nlq);

      address_tree = proto_item_add_subtree(address_group, ett_olsr_message_neigh);

      proto_tree_add_item(address_tree, hf_olsr_neighbor6_addr, tvb, offset, 16, ENC_NA);
      offset += 16;
    } else {
      break; /* unknown address type should be handled in dissect_olsr, just be sure */
    }

    proto_tree_add_item(address_tree, hf_olsrorg_lq, tvb, offset++, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(address_tree, hf_olsrorg_nlq, tvb, offset++, 1, ENC_BIG_ENDIAN);
    offset += 2;
  }
  return message_end;
}

static int dissect_nrlolsr_tc(tvbuff_t *tvb, packet_info *pinfo, proto_tree *olsr_tree, int offset, int message_end) {
  int field1Ptr, field2Ptr, saneEnd;
  if (message_end - offset < 4) {
    proto_tree_add_expert_format(olsr_tree, pinfo, &ei_olsr_not_enough_bytes, tvb, offset, message_end - offset,
        "Not enough bytes for NRLOLSR LQ-TC");
    return message_end;
  }

  proto_tree_add_item(olsr_tree, hf_olsr_ansn, tvb, offset, 2, ENC_BIG_ENDIAN);
  offset += 4;

  field1Ptr = offset + (message_end - offset) / (pinfo->src.len + 2) * (pinfo->src.len);
  field2Ptr = offset + (message_end - offset) / (pinfo->src.len + 2) * (pinfo->src.len + 1);

  saneEnd = message_end - ((message_end - offset) % (pinfo->src.len + 2));
  while (field2Ptr < saneEnd) {
    if (pinfo->src.type == AT_IPv4) {
      proto_tree_add_item(olsr_tree, hf_olsr_neighbor_addr, tvb, offset, 4, ENC_BIG_ENDIAN);
      offset += 4;
    } else if (pinfo->src.type == AT_IPv6) {
      proto_tree_add_item(olsr_tree, hf_olsr_neighbor6_addr, tvb, offset, 16, ENC_NA);
      offset += 16;
    } else {
      break; /* unknown address type should be handled in dissect_olsr, just be sure */
    }

    proto_tree_add_item(olsr_tree, hf_nrlolsr_f1, tvb, field1Ptr++, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(olsr_tree, hf_nrlolsr_f2, tvb, field2Ptr++, 1, ENC_BIG_ENDIAN);
  }
  return message_end;
}

/*------------------------- Hello Dissecting Code-------------------------*/
static int dissect_olsr_hello(tvbuff_t *tvb, packet_info *pinfo, proto_tree *olsr_tree, int offset, int message_end,
    int(*handleNeighbors)(tvbuff_t *, packet_info *, proto_tree *, int, int)) {
  double      hTime;
  proto_item *ti;
  proto_tree *link_type_tree;

  guint16     message_size = 0;

  if (message_end - offset < 4) {
    proto_tree_add_expert_format(olsr_tree, pinfo, &ei_olsr_not_enough_bytes, tvb, offset, message_end - offset,
        "Not enough bytes for Hello");
    return message_end;
  }

  offset += 2;

  /*---------------------Dissect Hello Emission Invertal-------------------*/
  hTime = getOlsrTime(tvb_get_guint8(tvb, offset));
  proto_tree_add_double_format_value(olsr_tree, hf_olsr_htime, tvb, offset, 1, hTime,
      "%.3f (in seconds)", hTime);
  offset += 1;

  /*-------------------------Dissect Willingness---------------------------*/
  proto_tree_add_item(olsr_tree, hf_olsr_willingness, tvb, offset++, 1, ENC_BIG_ENDIAN);

  while (offset < message_end) {
    if (message_end - offset < 4) {
      proto_tree_add_expert_format(olsr_tree, pinfo, &ei_olsr_not_enough_bytes, tvb, offset, message_end - offset,
          "Not enough bytes for last Hello entry");
      return message_end;
    }

    /*------------------------------Dissect Link Type---------------------------------- */
    ti = proto_tree_add_item(olsr_tree, hf_olsr_link_type, tvb, offset++, 1, ENC_BIG_ENDIAN);
    link_type_tree = proto_item_add_subtree(ti, ett_olsr_message_linktype);

    /* reserved byte */
    offset += 1;

    /*----------------------Dissect Link Message Size--------------------------*/
    message_size = tvb_get_ntohs(tvb, offset);
    ti = proto_tree_add_item(link_type_tree, hf_olsr_link_message_size, tvb, offset, 2, ENC_BIG_ENDIAN);
    offset += 2;

    if (message_size < 4) {
      expert_add_info_format(pinfo, ti, &ei_olsr_not_enough_bytes, "(too short, must be >= 4)");
      return message_end;
    }
    offset = handleNeighbors(tvb, pinfo, link_type_tree, offset, offset + message_size - 4);
  }
  return message_end;
}

static int handle_olsr_hello_rfc(tvbuff_t *tvb, packet_info *pinfo, proto_tree *olsr_tree, int offset,
    int link_message_end) {
  /*-------------------Dissect Neighbor Addresses--------------------*/
  while (offset < link_message_end) {
    if (link_message_end - offset < pinfo->src.len) {
      proto_tree_add_expert_format(olsr_tree, pinfo, &ei_olsr_not_enough_bytes, tvb, offset, link_message_end - offset,
          "Not enough bytes for last Hello entry");
      return link_message_end;
    }
    if (pinfo->src.type == AT_IPv4) {
      proto_tree_add_item(olsr_tree, hf_olsr_neighbor_addr, tvb, offset, 4, ENC_BIG_ENDIAN);
      offset += 4;
    } else if (pinfo->src.type == AT_IPv6) {
      proto_tree_add_item(olsr_tree, hf_olsr_neighbor6_addr, tvb, offset, 16, ENC_NA);
      offset += 16;
    } else {
      break; /* unknown address type should be handled in dissect_olsr, just be sure */
    }
  } /* end while */
  return link_message_end;
}

static int handle_olsr_hello_olsrorg(tvbuff_t *tvb, packet_info *pinfo, proto_tree *olsr_tree, int offset,
    int link_message_end) {
  /*-------------------Dissect Neighbor Addresses--------------------*/

  while (offset < link_message_end) {
    proto_item *address_group;
    proto_tree *address_tree;
    guint8      lq, nlq;

    if (link_message_end - offset < pinfo->src.len + 4) {
      proto_tree_add_expert_format(olsr_tree, pinfo, &ei_olsr_not_enough_bytes, tvb, offset, link_message_end - offset,
          "Not enough bytes for last Olsr.org LQ-Hello entry");
      return link_message_end;
    }

    if (pinfo->src.type == AT_IPv4) {
      lq = tvb_get_guint8(tvb, offset + 4);
      nlq = tvb_get_guint8(tvb, offset + 5);

      address_group = proto_tree_add_bytes_format_value(olsr_tree, hf_olsr_neighbor, tvb, offset, 8,
           NULL, "%s (%d/%d)", tvb_ip_to_str(tvb, offset), lq, nlq);

      address_tree = proto_item_add_subtree(address_group, ett_olsr_message_neigh);

      proto_tree_add_item(address_tree, hf_olsr_neighbor_addr, tvb, offset, 4, ENC_BIG_ENDIAN);
      offset += 4;
    } else if (pinfo->src.type == AT_IPv6) {
      lq = tvb_get_guint8(tvb, offset + 16);
      nlq = tvb_get_guint8(tvb, offset + 17);

      address_group = proto_tree_add_bytes_format_value(olsr_tree, hf_olsr_neighbor, tvb, offset, 20,
          NULL, "%s (%d/%d)", tvb_ip6_to_str(tvb, offset), lq, nlq);

      address_tree = proto_item_add_subtree(address_group, ett_olsr_message_neigh);

      proto_tree_add_item(address_tree, hf_olsr_neighbor6_addr, tvb, offset, 16, ENC_NA);
      offset += 16;
    } else {
      break; /* unknown address type should be handled in dissect_olsr, just be sure */
    }

    proto_tree_add_item(address_tree, hf_olsrorg_lq, tvb, offset++, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(address_tree, hf_olsrorg_nlq, tvb, offset++, 1, ENC_BIG_ENDIAN);
    offset += 2;
  } /* end while */
  return link_message_end;
}

/*------------------------- MID Dissecting Code-------------------------*/
static int dissect_olsr_mid(tvbuff_t *tvb, packet_info *pinfo, proto_tree *olsr_tree, int offset, int message_end) {
  while (offset < message_end) {
    if (message_end - offset < pinfo->src.len) {
      proto_tree_add_expert_format(olsr_tree, pinfo, &ei_olsr_not_enough_bytes, tvb, offset, message_end - offset,
          "Not enough bytes for last MID entry");
      return message_end;
    }
    if (pinfo->src.type == AT_IPv4) {
      proto_tree_add_item(olsr_tree, hf_olsr_interface_addr, tvb, offset, 4, ENC_BIG_ENDIAN);
      offset += 4;
    } else if (pinfo->src.type == AT_IPv6) {
      proto_tree_add_item(olsr_tree, hf_olsr_interface6_addr, tvb, offset, 16, ENC_NA);
      offset += 16;
    } else {
      break; /* unknown address type should be handled in dissect_olsr, just be sure */
    }
  } /* end while for MID */
  return message_end;
}

/*------------------------- HNA Dissecting Code-------------------------*/
static int dissect_olsr_hna(tvbuff_t *tvb, packet_info *pinfo, proto_tree *olsr_tree, int offset, int message_end) {
  while (offset < message_end) {
    if (message_end - offset < pinfo->src.len * 2) {
      proto_tree_add_expert_format(olsr_tree, pinfo, &ei_olsr_not_enough_bytes, tvb, offset, message_end - offset,
          "Not enough bytes for last HNA entry");
      return message_end;
    }

    if (pinfo->src.type == AT_IPv4) {
      proto_tree_add_item(olsr_tree, hf_olsr_network_addr, tvb, offset, 4, ENC_BIG_ENDIAN);
      offset += 4;
      proto_tree_add_item(olsr_tree, hf_olsr_netmask, tvb, offset, 4, ENC_BIG_ENDIAN);
      offset += 4;
    } else if (pinfo->src.type == AT_IPv6) {
      proto_tree_add_item(olsr_tree, hf_olsr_network6_addr, tvb, offset, 4, ENC_NA);
      offset += 16;
      proto_tree_add_item(olsr_tree, hf_olsr_netmask6, tvb, offset, 4, ENC_NA);
      offset += 16;
    } else {
      break; /* unknown address type should be handled in dissect_olsr, just be sure */
    }
  } /* end while for HNA */
  return message_end;
}

/*------------------------- MID Dissecting Code-------------------------*/
static int dissect_olsrorg_nameservice(tvbuff_t *tvb, packet_info *pinfo, proto_tree *olsr_tree, int offset,
    int message_end) {
  guint16     version, count;

  proto_item *olsr_ns_item, *ti;
  proto_tree *olsr_ns_tree;

  if (message_end - offset < 4) {
    proto_tree_add_expert_format(olsr_tree, pinfo, &ei_olsr_not_enough_bytes, tvb, offset, message_end - offset,
        "Not enough bytes for Olsr.org Nameservice message");
    return message_end;
  }

  version = tvb_get_ntohs(tvb, offset);
  ti = proto_tree_add_item(olsr_tree, hf_olsrorg_ns_version, tvb, offset, 2, ENC_BIG_ENDIAN);

  count = tvb_get_ntohs(tvb, offset + 2);
  proto_tree_add_item(olsr_tree, hf_olsrorg_ns_count, tvb, offset + 2, 2, ENC_BIG_ENDIAN);

  offset += 4;

  if (version != 1) {

    expert_add_info(pinfo, ti, &ei_olsrorg_ns_version);
    proto_tree_add_item(olsr_tree, hf_olsr_data, tvb, offset, message_end - offset, ENC_NA);
    return message_end;
  }

  while (offset < message_end && count-- > 0) {
    guint16 type, length;
    int total_length;

    if (message_end - offset < 20) {
      proto_tree_add_expert_format(olsr_tree, pinfo, &ei_olsr_not_enough_bytes, tvb, offset, message_end - offset,
          "Not enough bytes for last nameservice entry");
      return message_end;
    }

    type = tvb_get_ntohs(tvb, offset);
    length = tvb_get_ntohs(tvb, offset + 2);

    total_length = 4 + 16 + ((length - 1) | 3) + 1;

    olsr_ns_item = proto_tree_add_bytes_format_value(olsr_tree, hf_olsrorg_ns, tvb, offset, total_length,
        NULL, "%s (%d)", val_to_str_const(type, nameservice_type_vals, "UNKNOWN"), type);

    olsr_ns_tree = proto_item_add_subtree(olsr_ns_item, ett_olsr_message_ns);

    proto_tree_add_item(olsr_ns_tree, hf_olsrorg_ns_type, tvb, offset, 2, ENC_BIG_ENDIAN);
    proto_tree_add_uint(olsr_ns_tree, hf_olsrorg_ns_length, tvb, offset + 2, 2, length);

    if (pinfo->src.type == AT_IPv4) {
      proto_tree_add_item(olsr_ns_tree, hf_olsrorg_ns_ip, tvb, offset + 4, 4, ENC_BIG_ENDIAN);
    } else if (pinfo->src.type == AT_IPv6) {
      proto_tree_add_item(olsr_ns_tree, hf_olsrorg_ns_ip6, tvb, offset + 4, 16, ENC_NA);
    } else {
      break; /* unknown address type should be handled in dissect_olsr, just be sure */
    }

    if (message_end - offset < total_length) {
      proto_tree_add_expert_format(olsr_tree, pinfo, &ei_olsr_not_enough_bytes, tvb, offset, message_end - offset,
          "Not enough bytes for content of last nameservice entry");
      return message_end;
    }
    proto_tree_add_item(olsr_ns_tree, hf_olsrorg_ns_content, tvb, offset + 20, length, ENC_ASCII|ENC_NA);
    offset += 4 + 16 + ((length - 1) | 3) + 1;
  }
  return message_end;
}

/*------------------------- Packet Dissecting Code-------------------------*/
static int dissect_olsr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_) {
  proto_item *ti;
  proto_tree *olsr_tree;

  int    offset, message_len, message_end;
  guint  message_type;
  double vTime;

  guint16 packet_len;

  /* Does this packet have a valid message type at the beginning? */
  if (tvb_captured_length(tvb) < 4) {
    col_add_fstr(pinfo->cinfo, COL_INFO, "OLSR Packet,  Length: %u Bytes (not enough data in packet)",
          tvb_captured_length(tvb));
    return 0; /* not enough bytes for the packet length */
  }
  packet_len = tvb_get_ntohs(tvb, 0);
  if (packet_len > tvb_reported_length(tvb)) {
    col_add_fstr(pinfo->cinfo, COL_INFO, "OLSR Packet,  Length: %u Bytes (not enough data in packet)", packet_len);
    return 0;
  }
  /*-------------Setting the Protocol and Info Columns in the Wireshark Display----------*/
  col_set_str(pinfo->cinfo, COL_PROTOCOL, "OLSR v1");
  col_clear(pinfo->cinfo, COL_INFO);

  if ((pinfo->src.type != AT_IPv4) && (pinfo->src.type != AT_IPv6)) {
    col_add_fstr(pinfo->cinfo, COL_INFO, "OLSR (unknown address type) Packet,  Length: %u Bytes", packet_len);
    return 0;
  }
  if (pinfo->src.type == AT_IPv4) {
    col_add_fstr(pinfo->cinfo, COL_INFO, "OLSR (IPv4) Packet,  Length: %u Bytes", packet_len);
  } else if (pinfo->src.type == AT_IPv6) {
    col_add_fstr(pinfo->cinfo, COL_INFO, "OLSR (IPv6) Packet,  Length: %u Bytes", packet_len);
  }

  /*-----------------Fetching Info from IP Packet and Adding to Tree------------------------*/
    ti = proto_tree_add_item(tree, proto_olsr, tvb, 0, -1, ENC_NA);
    olsr_tree = proto_item_add_subtree(ti, ett_olsr);

    proto_tree_add_item(olsr_tree, hf_olsr_packet_len, tvb, 0, 2, ENC_BIG_ENDIAN);
    proto_tree_add_item(olsr_tree, hf_olsr_packet_seq_num, tvb, 2, 2, ENC_BIG_ENDIAN);

    offset = 4;

    while (offset < packet_len) {
      proto_item *message_item;
      proto_tree *message_tree;

      if (packet_len - offset < 4) {
        proto_tree_add_expert_format(olsr_tree, pinfo, &ei_olsr_not_enough_bytes, tvb, offset, packet_len - offset,
            "Message too short !");
        break;
      }

      message_type = tvb_get_guint8(tvb, offset);
      vTime = getOlsrTime(tvb_get_guint8(tvb, offset + 1));
      message_len = tvb_get_ntohs(tvb, offset + 2);

      message_item = proto_tree_add_bytes_format_value(olsr_tree, hf_olsr_message, tvb, offset, message_len,
          NULL, "%s (%d)", val_to_str_const(message_type, message_type_vals, "UNKNOWN"),
          message_type);
      message_tree = proto_item_add_subtree(message_item, ett_olsr_message[message_type]);

      proto_tree_add_uint(message_tree, hf_olsr_message_type, tvb, offset, 1, message_type);
      offset += 1;

      /*-------------Dissect Validity Time-------------------------*/
      proto_tree_add_double_format_value(message_tree, hf_olsr_vtime, tvb, offset, 1, vTime,
          "%.3f (in seconds)", vTime);
      offset += 1;

      /*-------------Dissect Message Size---------------------------*/
      ti = proto_tree_add_item(message_tree, hf_olsr_message_size, tvb, offset, 2, ENC_BIG_ENDIAN);
      offset += 2;

      if (message_len < 8 + pinfo->src.len) {
        expert_add_info_format(pinfo, ti, &ei_olsr_not_enough_bytes, "(too short, must be >= %d)", 8 + pinfo->src.len);
        break;
      }

      message_end = offset + message_len - 4;
      if (message_end > packet_len) {
        expert_add_info_format(pinfo, ti, &ei_olsr_not_enough_bytes, "(not enough data for message)");
        break;
      }

      /*-----------------Dissecting: Origin Addr, TTL, Hop Count, and Message Seq Number*/
      if (pinfo->src.type == AT_IPv4) {
        proto_tree_add_item(message_tree, hf_olsr_origin_addr, tvb, offset, 4, ENC_BIG_ENDIAN);
        offset += 4;
      } else if (pinfo->src.type == AT_IPv6) {
        proto_tree_add_item(message_tree, hf_olsr_origin6_addr, tvb, offset, 16, ENC_NA);
        offset += 16;
      } else {
        break; /* unknown address type should be handled before this loop, just be sure */
      }

      proto_tree_add_item(message_tree, hf_olsr_ttl, tvb, offset, 1, ENC_BIG_ENDIAN);
      proto_tree_add_item(message_tree, hf_olsr_hop_count, tvb, offset + 1, 1, ENC_BIG_ENDIAN);
      proto_tree_add_item(message_tree, hf_olsr_message_seq_num, tvb, offset + 2, 2, ENC_BIG_ENDIAN);
      offset += 4;

      if (offset < message_end) {
        /* --------------Dissecting TC message--------------------- */
        if (message_type == TC) {
          dissect_olsr_tc(tvb, pinfo, message_tree, offset, message_end);
        }
        /* -------------Dissect HELLO message----------------------- */
        else if (message_type == HELLO) {
          dissect_olsr_hello(tvb, pinfo, message_tree, offset, message_end, &handle_olsr_hello_rfc);
        }
        /*---------------------------------Dissect MID Message----------------------------------*/
        else if (message_type == MID) {
          dissect_olsr_mid(tvb, pinfo, message_tree, offset, message_end);
        }
        /*-----------------------------Dissect HNA Message--------------------------------*/
        else if (message_type == HNA) {
          dissect_olsr_hna(tvb, pinfo, message_tree, offset, message_end);
        }
        /*-----------------------------Dissect Olsr.org Hello Message--------------------------------*/
        else if (global_olsr_olsrorg && message_type == OLSR_ORG_LQ_HELLO) {
          dissect_olsr_hello(tvb, pinfo, message_tree, offset, message_end, &handle_olsr_hello_olsrorg);
        }
        /*-----------------------------Dissect Olsr.org TC Message--------------------------------*/
        else if (global_olsr_olsrorg && message_type == OLSR_ORG_LQ_TC) {
          dissect_olsrorg_lq_tc(tvb, pinfo, message_tree, offset, message_end);
        }

        /*-----------------------------Dissect Olsr.org NS Message--------------------------------*/
        else if (global_olsr_olsrorg && message_type == OLSR_ORG_NAMESERVICE) {
          dissect_olsrorg_nameservice(tvb, pinfo, message_tree, offset, message_end);
        }
        /*-----------------------Dissect NRL OLSR TC Message-------------------------------------*/
        else if (global_olsr_nrlolsr && message_type == NRLOLSR_TC_EXTRA) {
          dissect_nrlolsr_tc(tvb, pinfo, message_tree, offset, message_end);
        }

        /*-----------------------------Undefined message types-----------------------------*/
        else {
          ti = proto_tree_add_bytes_format(message_tree, hf_olsr_data, tvb, offset, message_len - 12,
              NULL, "Data (%u bytes)", message_len - 12);
          if ((message_len - 12) % 4) {
              expert_add_info(pinfo, ti, &ei_olsr_data_misaligned);
            break;
          }
          /*offset += message_len - 12;*/
        } /* end if for undefined message types */
      }
      offset = message_end;
    } /* end while for message alive */

  return tvb_captured_length(tvb);
} /* end Dissecting */

/*-----------Register the Dissector for OLSR--------------*/
void proto_register_olsr(void) {
  static hf_register_info hf[] = {
    { &hf_olsr_packet_len,
      { "Packet Length", "olsr.packet_len",
        FT_UINT16, BASE_DEC, NULL, 0,
        "Packet Length in Bytes", HFILL
      }
    },

    { &hf_olsr_packet_seq_num,
      { "Packet Sequence Number", "olsr.packet_seq_num",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL
      }
    },

    { &hf_olsr_message,
      { "Message", "olsr.message",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL
      }
    },

    { &hf_olsr_message_type,
      { "Message Type", "olsr.message_type",
        FT_UINT8, BASE_DEC, VALS(message_type_vals), 0,
        NULL, HFILL
      }
    },

    { &hf_olsr_message_size,
      { "Message", "olsr.message_size",
        FT_UINT16, BASE_DEC, NULL, 0,
        "Message Size in Bytes", HFILL
      }
    },

    { &hf_olsr_message_seq_num,
      { "Message Sequence Number", "olsr.message_seq_num",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL
      }
    },

    { &hf_olsr_vtime,
      { "Validity Time", "olsr.vtime",
        FT_DOUBLE, BASE_NONE, NULL, 0,
        "Validity Time in seconds", HFILL
      }
    },

    { &hf_olsr_ansn,
      { "Advertised Neighbor Sequence Number (ANSN)", "olsr.ansn",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL
      }
    },

    { &hf_olsr_htime,
      { "Hello Emission Interval", "olsr.htime",
        FT_DOUBLE, BASE_NONE, NULL, 0,
        "Hello emission interval in seconds", HFILL
      }
    },

    { &hf_olsr_willingness,
      { "Willingness to forward messages", "olsr.willingness",
        FT_UINT8, BASE_DEC, VALS(willingness_type_vals), 0,
        NULL, HFILL
      }
    },

    { &hf_olsr_ttl,
      { "TTL", "olsr.ttl",
        FT_UINT8, BASE_DEC, NULL, 0,
        "Time to Live in hops", HFILL
      }
    },

    { &hf_olsr_link_type,
      { "Link Type", "olsr.link_type",
        FT_UINT8, BASE_DEC, VALS(link_type_vals), 0,
        NULL, HFILL
      }
    },

    { &hf_olsr_link_message_size,
      { "Link Message Size", "olsr.link_message_size",
        FT_UINT16, BASE_DEC, NULL, 0,
        "Link Message Size in bytes", HFILL
      }
    },

    { &hf_olsr_hop_count,
      { "Hop Count", "olsr.hop_count",
        FT_UINT8, BASE_DEC, NULL, 0,
        NULL, HFILL
      }
    },

    { &hf_olsr_neighbor,
      { "Neighbor Address", "olsr.neighbor",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL
      }
    },

    { &hf_olsr_origin_addr,
      { "Originator Address", "olsr.origin_addr",
        FT_IPv4, BASE_NONE, NULL, 0,
        NULL, HFILL
      }
    },

    { &hf_olsr_neighbor_addr,
      { "Neighbor Address", "olsr.neighbor_addr",
        FT_IPv4, BASE_NONE, NULL, 0,
        NULL, HFILL
      }
    },

    { &hf_olsr_network_addr,
      { "Network Address", "olsr.network_addr",
        FT_IPv4, BASE_NONE, NULL, 0,
        NULL, HFILL
      }
    },

    { &hf_olsr_interface_addr,
      { "Interface Address", "olsr.interface_addr",
        FT_IPv4, BASE_NONE, NULL, 0,
        NULL, HFILL
      }
    },

    { &hf_olsr_netmask,
      { "Netmask", "olsr.netmask",
        FT_IPv4, BASE_NETMASK, NULL, 0,
        NULL, HFILL
      }
    },

#if 0
    { &hf_olsr_neighbor6,
      { "Neighbor Address", "olsr.neighbor6",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL
      }
    },
#endif

    { &hf_olsr_origin6_addr,
      { "Originator Address", "olsr.origin6_addr",
        FT_IPv6, BASE_NONE, NULL, 0,
        NULL, HFILL
      }
    },

    { &hf_olsr_neighbor6_addr,
      { "Neighbor Address", "olsr.neighbor6_addr",
        FT_IPv6, BASE_NONE, NULL, 0,
        NULL, HFILL
      }
    },

    { &hf_olsr_network6_addr,
      { "Network Address", "olsr.network6_addr",
        FT_IPv6, BASE_NONE, NULL, 0,
        NULL, HFILL
      }
    },

    { &hf_olsr_interface6_addr,
      { "Interface Address", "olsr.interface6_addr",
        FT_IPv6, BASE_NONE, NULL, 0,
        NULL, HFILL
      }
    },

    { &hf_olsr_netmask6,
      { "Netmask", "olsr.netmask6",
        FT_IPv6, BASE_NONE, NULL, 0,
        NULL, HFILL
      }
    },

    { &hf_olsr_data,
      { "Data", "olsr.data",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL
      }
    },

    { &hf_olsrorg_lq,
      { "LQ", "olsr.lq",
        FT_UINT8, BASE_DEC, NULL, 0,
        "Link quality", HFILL
      }
    },
    { &hf_olsrorg_nlq,
      { "NLQ", "olsr.nlq",
        FT_UINT8, BASE_DEC, NULL, 0,
        "Neighbor link quality", HFILL
      }
    },

    { &hf_olsrorg_ns,
      { "Nameservice", "olsr.ns",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL
      }
    },
    { &hf_olsrorg_ns_version,
      { "Version", "olsr.ns.version",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL
      }
    },
    { &hf_olsrorg_ns_count,
      { "Count", "olsr.ns.count",
        FT_UINT16, BASE_DEC, NULL, 0,
        "Number of nameservice messages", HFILL
      }
    },
    { &hf_olsrorg_ns_type,
      { "Message Type", "olsr.ns.type",
        FT_UINT16, BASE_DEC, VALS(nameservice_type_vals), 0,
        NULL, HFILL
      }
    },
    { &hf_olsrorg_ns_length,
      { "Length", "olsr.ns.length",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL
      }
    },
    { &hf_olsrorg_ns_ip,
      { "Address", "olsr.ns.ip",
        FT_IPv4, BASE_NONE, NULL, 0,
        NULL, HFILL
      }
    },
    { &hf_olsrorg_ns_ip6,
      { "Address", "olsr.ns.ip6",
        FT_IPv6, BASE_NONE, NULL, 0,
        NULL, HFILL
      }
    },
    { &hf_olsrorg_ns_content,
      { "Content", "olsr.ns.content",
        FT_STRING, BASE_NONE, NULL, 0,
        NULL, HFILL
      }
    },

    { &hf_nrlolsr_f1,
      { "NRL MINMAX", "olsr.nrl.minmax",
        FT_UINT8, BASE_DEC, NULL, 0,
        NULL, HFILL
      }
    },
    { &hf_nrlolsr_f2,
      { "NRL SPF", "olsr.nrl.spf",
        FT_UINT8, BASE_DEC, NULL, 0,
        NULL, HFILL
      }
    }
  };

  static gint *ett_base[] = {
    &ett_olsr,
    &ett_olsr_message_linktype,
    &ett_olsr_message_neigh,
    &ett_olsr_message_neigh6,
    &ett_olsr_message_ns
  };

  static ei_register_info ei[] = {
    { &ei_olsr_not_enough_bytes, { "olsr.not_enough_bytes", PI_MALFORMED, PI_ERROR, "Not enough bytes for field", EXPFILL }},
    { &ei_olsrorg_ns_version,    { "olsr.ns.version.unknown", PI_PROTOCOL, PI_WARN, "Unknown nameservice protocol version", EXPFILL }},
    { &ei_olsr_data_misaligned,  { "olsr.data.misaligned", PI_PROTOCOL, PI_WARN, "Must be aligned on 32 bits", EXPFILL }},
  };

  gint *ett[array_length(ett_base) + (G_MAXUINT8+1)];

  module_t        *olsr_module;
  expert_module_t *expert_olsr;
  int              i,j;

  memcpy(ett, ett_base, sizeof(ett_base));
  j = array_length(ett_base);
  for (i=0; i<G_MAXUINT8+1; i++) {
    ett_olsr_message[i] = -1;
    ett[j++] = &ett_olsr_message[i];
  }

  proto_olsr = proto_register_protocol("Optimized Link State Routing Protocol", "OLSR", "olsr");

  proto_register_field_array(proto_olsr, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));
  expert_olsr = expert_register_protocol(proto_olsr);
  expert_register_field_array(expert_olsr, ei, array_length(ei));

  olsr_module = prefs_register_protocol(proto_olsr, NULL);
  prefs_register_bool_preference(olsr_module, "ff_olsrorg",
                                 "Dissect olsr.org messages","Dissect custom olsr.org message types (compatible with rfc routing agents)",
                                 &global_olsr_olsrorg);
  prefs_register_bool_preference(olsr_module, "nrlolsr",
                                 "Dissect NRL-Olsr TC messages", "Dissect custom nrlolsr tc message (incompatible with rfc routing agents)",
                                 &global_olsr_nrlolsr);
}

void proto_reg_handoff_olsr(void) {
  dissector_handle_t olsr_handle;

  olsr_handle = create_dissector_handle(dissect_olsr, proto_olsr);
  dissector_add_uint("udp.port", UDP_PORT_OLSR, olsr_handle);
}

/*
 * Editor modelines
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
