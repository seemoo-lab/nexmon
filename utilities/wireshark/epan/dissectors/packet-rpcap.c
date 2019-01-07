/* packet-rpcap.c
 *
 * Routines for RPCAP message formats.
 *
 * Copyright 2008, Stig Bjorlykke <stig@bjorlykke.org>, Thales Norway AS
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
#include <epan/aftypes.h>
#include <epan/prefs.h>
#include <epan/to_str.h>
#include <epan/expert.h>
#include <epan/exceptions.h>
#include <epan/show_exception.h>
#include <wsutil/str_util.h>

#include "packet-frame.h"
#include "packet-pcap_pktdata.h"
#include "packet-tcp.h"

#define PNAME  "Remote Packet Capture"
#define PSNAME "RPCAP"
#define PFNAME "rpcap"

#define RPCAP_MSG_ERROR               1
#define RPCAP_MSG_FINDALLIF_REQ       2
#define RPCAP_MSG_OPEN_REQ            3
#define RPCAP_MSG_STARTCAP_REQ        4
#define RPCAP_MSG_UPDATEFILTER_REQ    5
#define RPCAP_MSG_CLOSE               6
#define RPCAP_MSG_PACKET              7
#define RPCAP_MSG_AUTH_REQ            8
#define RPCAP_MSG_STATS_REQ           9
#define RPCAP_MSG_ENDCAP_REQ          10
#define RPCAP_MSG_SETSAMPLING_REQ     11

#define RPCAP_MSG_FINDALLIF_REPLY     (128+RPCAP_MSG_FINDALLIF_REQ)
#define RPCAP_MSG_OPEN_REPLY          (128+RPCAP_MSG_OPEN_REQ)
#define RPCAP_MSG_STARTCAP_REPLY      (128+RPCAP_MSG_STARTCAP_REQ)
#define RPCAP_MSG_UPDATEFILTER_REPLY  (128+RPCAP_MSG_UPDATEFILTER_REQ)
#define RPCAP_MSG_AUTH_REPLY          (128+RPCAP_MSG_AUTH_REQ)
#define RPCAP_MSG_STATS_REPLY         (128+RPCAP_MSG_STATS_REQ)
#define RPCAP_MSG_ENDCAP_REPLY        (128+RPCAP_MSG_ENDCAP_REQ)
#define RPCAP_MSG_SETSAMPLING_REPLY   (128+RPCAP_MSG_SETSAMPLING_REQ)

#define RPCAP_ERR_NETW            1
#define RPCAP_ERR_INITTIMEOUT     2
#define RPCAP_ERR_AUTH            3
#define RPCAP_ERR_FINDALLIF       4
#define RPCAP_ERR_NOREMOTEIF      5
#define RPCAP_ERR_OPEN            6
#define RPCAP_ERR_UPDATEFILTER    7
#define RPCAP_ERR_GETSTATS        8
#define RPCAP_ERR_READEX          9
#define RPCAP_ERR_HOSTNOAUTH      10
#define RPCAP_ERR_REMOTEACCEPT    11
#define RPCAP_ERR_STARTCAPTURE    12
#define RPCAP_ERR_ENDCAPTURE      13
#define RPCAP_ERR_RUNTIMETIMEOUT  14
#define RPCAP_ERR_SETSAMPLING     15
#define RPCAP_ERR_WRONGMSG        16
#define RPCAP_ERR_WRONGVER        17

#define RPCAP_SAMP_NOSAMP            0
#define RPCAP_SAMP_1_EVERY_N         1
#define RPCAP_SAMP_FIRST_AFTER_N_MS  2

#define RPCAP_RMTAUTH_NULL  0
#define RPCAP_RMTAUTH_PWD   1

#define FLAG_PROMISC     0x0001
#define FLAG_DGRAM       0x0002
#define FLAG_SERVEROPEN  0x0004
#define FLAG_INBOUND     0x0008
#define FLAG_OUTBOUND    0x0010

void proto_register_rpcap (void);
void proto_reg_handoff_rpcap (void);

static int proto_rpcap = -1;

static int hf_version = -1;
static int hf_type = -1;
static int hf_value = -1;
static int hf_plen = -1;

static int hf_error = -1;
static int hf_error_value = -1;

static int hf_packet = -1;
static int hf_timestamp = -1;
static int hf_caplen = -1;
static int hf_len = -1;
static int hf_npkt = -1;

static int hf_auth_request = -1;
static int hf_auth_type = -1;
static int hf_auth_slen1 = -1;
static int hf_auth_slen2 = -1;
static int hf_auth_username = -1;
static int hf_auth_password = -1;

static int hf_open_request = -1;

static int hf_open_reply = -1;
static int hf_linktype = -1;
static int hf_tzoff = -1;

static int hf_startcap_request = -1;
static int hf_snaplen = -1;
static int hf_read_timeout = -1;
static int hf_flags = -1;
static int hf_flags_promisc = -1;
static int hf_flags_dgram = -1;
static int hf_flags_serveropen = -1;
static int hf_flags_inbound = -1;
static int hf_flags_outbound = -1;
static int hf_client_port = -1;
static int hf_startcap_reply = -1;
static int hf_bufsize = -1;
static int hf_server_port = -1;
static int hf_dummy = -1;

static int hf_filter = -1;
static int hf_filtertype = -1;
static int hf_nitems = -1;

static int hf_filterbpf_insn = -1;
static int hf_code = -1;
static int hf_code_class = -1;
static int hf_code_fields = -1;
static int hf_code_ld_size = -1;
static int hf_code_ld_mode = -1;
static int hf_code_alu_op = -1;
static int hf_code_jmp_op = -1;
static int hf_code_src = -1;
static int hf_code_rval = -1;
static int hf_code_misc_op = -1;
static int hf_jt = -1;
static int hf_jf = -1;
static int hf_instr_value = -1;

static int hf_stats_reply = -1;
static int hf_ifrecv = -1;
static int hf_ifdrop = -1;
static int hf_krnldrop = -1;
static int hf_srvcapt = -1;

static int hf_findalldevs_reply = -1;
static int hf_findalldevs_if = -1;
static int hf_namelen = -1;
static int hf_desclen = -1;
static int hf_if_flags = -1;
static int hf_naddr = -1;
static int hf_if_name = -1;
static int hf_if_desc = -1;

static int hf_findalldevs_ifaddr = -1;
static int hf_if_addr = -1;
static int hf_if_netmask = -1;
static int hf_if_broadaddr = -1;
static int hf_if_dstaddr = -1;
static int hf_if_af = -1;
static int hf_if_port = -1;
static int hf_if_ip = -1;
static int hf_if_padding = -1;
static int hf_if_unknown = -1;

static int hf_sampling_request = -1;
static int hf_sampling_method = -1;
static int hf_sampling_dummy1 = -1;
static int hf_sampling_dummy2 = -1;
static int hf_sampling_value = -1;

static gint ett_rpcap = -1;
static gint ett_error = -1;
static gint ett_packet = -1;
static gint ett_auth_request = -1;
static gint ett_open_reply = -1;
static gint ett_startcap_request = -1;
static gint ett_startcap_reply = -1;
static gint ett_startcap_flags = -1;
static gint ett_filter = -1;
static gint ett_filterbpf_insn = -1;
static gint ett_filterbpf_insn_code = -1;
static gint ett_stats_reply = -1;
static gint ett_findalldevs_reply = -1;
static gint ett_findalldevs_if = -1;
static gint ett_findalldevs_ifaddr = -1;
static gint ett_ifaddr = -1;
static gint ett_sampling_request = -1;

static expert_field ei_error = EI_INIT;
static expert_field ei_if_unknown = EI_INIT;
static expert_field ei_no_more_data = EI_INIT;
static expert_field ei_caplen_too_big = EI_INIT;

static dissector_handle_t pcap_pktdata_handle;

/* User definable values */
static gboolean rpcap_desegment = TRUE;
static gboolean decode_content = TRUE;
static int global_linktype = -1;

/* Global variables */
static int linktype = -1;
static gboolean info_added = FALSE;

static const true_false_string open_closed = {
  "Open", "Closed"
};

static const value_string message_type[] = {
  { RPCAP_MSG_ERROR,              "Error"                       },
  { RPCAP_MSG_FINDALLIF_REQ,      "Find all interfaces request" },
  { RPCAP_MSG_OPEN_REQ,           "Open request"                },
  { RPCAP_MSG_STARTCAP_REQ,       "Start capture request"       },
  { RPCAP_MSG_UPDATEFILTER_REQ,   "Update filter request"       },
  { RPCAP_MSG_CLOSE,              "Close"                       },
  { RPCAP_MSG_PACKET,             "Packet"                      },
  { RPCAP_MSG_AUTH_REQ,           "Authentication request"      },
  { RPCAP_MSG_STATS_REQ,          "Statistics request"          },
  { RPCAP_MSG_ENDCAP_REQ,         "End capture request"         },
  { RPCAP_MSG_SETSAMPLING_REQ,    "Set sampling request"        },
  { RPCAP_MSG_FINDALLIF_REPLY,    "Find all interfaces reply"   },
  { RPCAP_MSG_OPEN_REPLY,         "Open reply"                  },
  { RPCAP_MSG_STARTCAP_REPLY,     "Start capture reply"         },
  { RPCAP_MSG_UPDATEFILTER_REPLY, "Update filter reply"         },
  { RPCAP_MSG_AUTH_REPLY,         "Authentication reply"        },
  { RPCAP_MSG_STATS_REPLY,        "Statistics reply"            },
  { RPCAP_MSG_ENDCAP_REPLY,       "End capture reply"           },
  { RPCAP_MSG_SETSAMPLING_REPLY,  "Set sampling reply"          },
  { 0,   NULL }
};

static const value_string error_codes[] = {
  { RPCAP_ERR_NETW,            "Network error"                        },
  { RPCAP_ERR_INITTIMEOUT,     "Initial timeout has expired"          },
  { RPCAP_ERR_AUTH,            "Authentication error"                 },
  { RPCAP_ERR_FINDALLIF,       "Generic findalldevs error"            },
  { RPCAP_ERR_NOREMOTEIF,      "No remote interfaces"                 },
  { RPCAP_ERR_OPEN,            "Generic pcap_open error"              },
  { RPCAP_ERR_UPDATEFILTER,    "Generic updatefilter error"           },
  { RPCAP_ERR_GETSTATS,        "Generic pcap_stats error"             },
  { RPCAP_ERR_READEX,          "Generic pcap_next_ex error"           },
  { RPCAP_ERR_HOSTNOAUTH,      "The host is not authorized"           },
  { RPCAP_ERR_REMOTEACCEPT,    "Generic pcap_remoteaccept error"      },
  { RPCAP_ERR_STARTCAPTURE,    "Generic pcap_startcapture error"      },
  { RPCAP_ERR_ENDCAPTURE,      "Generic pcap_endcapture error"        },
  { RPCAP_ERR_RUNTIMETIMEOUT,  "Runtime timeout has expired"          },
  { RPCAP_ERR_SETSAMPLING,     "Error in setting sampling parameters" },
  { RPCAP_ERR_WRONGMSG,        "Unrecognized message"                 },
  { RPCAP_ERR_WRONGVER,        "Incompatible version"                 },
  { 0,   NULL }
};

static const value_string sampling_method[] = {
  { RPCAP_SAMP_NOSAMP,            "No sampling"      },
  { RPCAP_SAMP_1_EVERY_N,         "1 every N"        },
  { RPCAP_SAMP_FIRST_AFTER_N_MS,  "First after N ms" },
  { 0,   NULL }
};

static const value_string auth_type[] = {
  { RPCAP_RMTAUTH_NULL, "None"     },
  { RPCAP_RMTAUTH_PWD,  "Password" },
  { 0,   NULL }
};

static const value_string address_family[] = {
  { COMMON_AF_UNSPEC,   "AF_UNSPEC" },
  { COMMON_AF_INET,     "AF_INET"   },
  { 0,   NULL }
};

static const value_string bpf_class[] = {
  { 0x00, "ld"   },
  { 0x01, "ldx"  },
  { 0x02, "st"   },
  { 0x03, "stx"  },
  { 0x04, "alu"  },
  { 0x05, "jmp"  },
  { 0x06, "ret"  },
  { 0x07, "misc" },
  { 0, NULL }
};

static const value_string bpf_size[] = {
  { 0x00, "w" },
  { 0x01, "h" },
  { 0x02, "b" },
  { 0, NULL }
};

static const value_string bpf_mode[] = {
  { 0x00, "imm" },
  { 0x01, "abs" },
  { 0x02, "ind" },
  { 0x03, "mem" },
  { 0x04, "len" },
  { 0x05, "msh" },
  { 0, NULL }
};

static const value_string bpf_alu_op[] = {
  { 0x00, "add" },
  { 0x01, "sub" },
  { 0x02, "mul" },
  { 0x03, "div" },
  { 0x04, "or"  },
  { 0x05, "and" },
  { 0x06, "lsh" },
  { 0x07, "rsh" },
  { 0x08, "neg" },
  { 0, NULL }
};

static const value_string bpf_jmp_op[] = {
  { 0x00, "ja"   },
  { 0x01, "jeq"  },
  { 0x02, "jgt"  },
  { 0x03, "jge"  },
  { 0x04, "jset" },
  { 0, NULL }
};

static const value_string bpf_src[] = {
  { 0x00, "k" },
  { 0x01, "x" },
  { 0, NULL }
};

static const value_string bpf_rval[] = {
  { 0x00, "k" },
  { 0x01, "x" },
  { 0x02, "a" },
  { 0, NULL }
};

static const value_string bpf_misc_op[] = {
  { 0x00, "tax" },
  { 0x10, "txa" },
  { 0, NULL }
};


static void rpcap_frame_end (void)
{
  info_added = FALSE;
}


static void
dissect_rpcap_error (tvbuff_t *tvb, packet_info *pinfo,
                     proto_tree *parent_tree, gint offset)
{
  proto_item *ti;
  gint len;

  len = tvb_captured_length_remaining (tvb, offset);
  if (len <= 0)
    return;

  col_append_fstr (pinfo->cinfo, COL_INFO, ": %s",
                   tvb_format_text_wsp (tvb, offset, len));

  ti = proto_tree_add_item (parent_tree, hf_error, tvb, offset, len, ENC_ASCII|ENC_NA);
  expert_add_info_format(pinfo, ti, &ei_error,
                         "Error: %s", tvb_format_text_wsp (tvb, offset, len));
}


static gint
dissect_rpcap_ifaddr (tvbuff_t *tvb, packet_info *pinfo,
                      proto_tree *parent_tree, gint offset, int hf_id,
                      proto_item *parent_item)
{
  proto_tree *tree;
  proto_item *ti;
  gchar ipaddr[MAX_ADDR_STR_LEN];
  guint32 ipv4;
  guint16 af;

  ti = proto_tree_add_item (parent_tree, hf_id, tvb, offset, 128, ENC_BIG_ENDIAN);
  tree = proto_item_add_subtree (ti, ett_ifaddr);

  af = tvb_get_ntohs (tvb, offset);
  proto_tree_add_item (tree, hf_if_af, tvb, offset, 2, ENC_BIG_ENDIAN);
  offset += 2;

  if (af == COMMON_AF_INET) {
    proto_tree_add_item (tree, hf_if_port, tvb, offset, 2, ENC_BIG_ENDIAN);
    offset += 2;

    ipv4 = tvb_get_ipv4 (tvb, offset);
    ip_to_str_buf((guint8 *)&ipv4, ipaddr, MAX_ADDR_STR_LEN);
    proto_item_append_text (ti, ": %s", ipaddr);
    if (parent_item) {
      proto_item_append_text (parent_item, ": %s", ipaddr);
    }
    proto_tree_add_item (tree, hf_if_ip, tvb, offset, 4, ENC_BIG_ENDIAN);
    offset += 4;

    proto_tree_add_item (tree, hf_if_padding, tvb, offset, 120, ENC_NA);
    offset += 120;
  } else {
    ti = proto_tree_add_item (tree, hf_if_unknown, tvb, offset, 126, ENC_NA);
    if (af != COMMON_AF_UNSPEC) {
      expert_add_info_format(pinfo, ti, &ei_if_unknown,
                             "Unknown address family: %d", af);
    }
    offset += 126;
  }

  return offset;
}


static gint
dissect_rpcap_findalldevs_ifaddr (tvbuff_t *tvb, packet_info *pinfo _U_,
                                  proto_tree *parent_tree, gint offset)
{
  proto_tree *tree;
  proto_item *ti;
  gint boffset = offset;

  ti = proto_tree_add_item (parent_tree, hf_findalldevs_ifaddr, tvb, offset, -1, ENC_NA);
  tree = proto_item_add_subtree (ti, ett_findalldevs_ifaddr);

  offset = dissect_rpcap_ifaddr (tvb, pinfo, tree, offset, hf_if_addr, ti);
  offset = dissect_rpcap_ifaddr (tvb, pinfo, tree, offset, hf_if_netmask, NULL);
  offset = dissect_rpcap_ifaddr (tvb, pinfo, tree, offset, hf_if_broadaddr, NULL);
  offset = dissect_rpcap_ifaddr (tvb, pinfo, tree, offset, hf_if_dstaddr, NULL);

  proto_item_set_len (ti, offset - boffset);

  return offset;
}


static gint
dissect_rpcap_findalldevs_if (tvbuff_t *tvb, packet_info *pinfo _U_,
                              proto_tree *parent_tree, gint offset)
{
  proto_tree *tree;
  proto_item *ti;
  guint16 namelen, desclen, naddr, i;
  gint boffset = offset;

  ti = proto_tree_add_item (parent_tree, hf_findalldevs_if, tvb, offset, -1, ENC_NA);
  tree = proto_item_add_subtree (ti, ett_findalldevs_if);

  namelen = tvb_get_ntohs (tvb, offset);
  proto_tree_add_item (tree, hf_namelen, tvb, offset, 2, ENC_BIG_ENDIAN);
  offset += 2;

  desclen = tvb_get_ntohs (tvb, offset);
  proto_tree_add_item (tree, hf_desclen, tvb, offset, 2, ENC_BIG_ENDIAN);
  offset += 2;

  proto_tree_add_item (tree, hf_if_flags, tvb, offset, 4, ENC_BIG_ENDIAN);
  offset += 4;

  naddr = tvb_get_ntohs (tvb, offset);
  proto_tree_add_item (tree, hf_naddr, tvb, offset, 2, ENC_BIG_ENDIAN);
  offset += 2;

  proto_tree_add_item (tree, hf_dummy, tvb, offset, 2, ENC_BIG_ENDIAN);
  offset += 2;

  if (namelen) {
    const guint8* name;
    proto_tree_add_item_ret_string(tree, hf_if_name, tvb, offset, namelen, ENC_ASCII|ENC_NA, wmem_packet_scope(), &name);
    proto_item_append_text (ti, ": %s", name);
    offset += namelen;
  }

  if (desclen) {
    proto_tree_add_item (tree, hf_if_desc, tvb, offset, desclen, ENC_ASCII|ENC_NA);
    offset += desclen;
  }

  for (i = 0; i < naddr; i++) {
    offset = dissect_rpcap_findalldevs_ifaddr (tvb, pinfo, tree, offset);
    if (tvb_reported_length_remaining (tvb, offset) < 0) {
      /* No more data in packet */
      expert_add_info(pinfo, ti, &ei_no_more_data);
      break;
    }
  }

  proto_item_set_len (ti, offset - boffset);

  return offset;
}


static void
dissect_rpcap_findalldevs_reply (tvbuff_t *tvb, packet_info *pinfo _U_,
                                 proto_tree *parent_tree, gint offset, guint16 no_devs)
{
  proto_tree *tree;
  proto_item *ti;
  guint16 i;

  ti = proto_tree_add_item (parent_tree, hf_findalldevs_reply, tvb, offset, -1, ENC_NA);
  tree = proto_item_add_subtree (ti, ett_findalldevs_reply);

  for (i = 0; i < no_devs; i++) {
    offset = dissect_rpcap_findalldevs_if (tvb, pinfo, tree, offset);
    if (tvb_reported_length_remaining (tvb, offset) < 0) {
      /* No more data in packet */
      expert_add_info(pinfo, ti, &ei_no_more_data);
      break;
    }
  }

  proto_item_append_text (ti, ", %d item%s", no_devs, plurality (no_devs, "", "s"));
}


static gint
dissect_rpcap_filterbpf_insn (tvbuff_t *tvb, packet_info *pinfo _U_,
                              proto_tree *parent_tree, gint offset)
{
  proto_tree *tree, *code_tree;
  proto_item *ti, *code_ti;
  guint8 inst_class;

  ti = proto_tree_add_item (parent_tree, hf_filterbpf_insn, tvb, offset, 8, ENC_NA);
  tree = proto_item_add_subtree (ti, ett_filterbpf_insn);

  code_ti = proto_tree_add_item (tree, hf_code, tvb, offset, 2, ENC_BIG_ENDIAN);
  code_tree = proto_item_add_subtree (code_ti, ett_filterbpf_insn_code);
  proto_tree_add_item (code_tree, hf_code_class, tvb, offset, 2, ENC_BIG_ENDIAN);
  inst_class = tvb_get_guint8 (tvb, offset + 1) & 0x07;
  proto_item_append_text (ti, ": %s", val_to_str_const (inst_class, bpf_class, ""));
  switch (inst_class) {
  case 0x00: /* ld */
  case 0x01: /* ldx */
    proto_tree_add_item (code_tree, hf_code_ld_size, tvb, offset, 2, ENC_BIG_ENDIAN);
    proto_tree_add_item (code_tree, hf_code_ld_mode, tvb, offset, 2, ENC_BIG_ENDIAN);
    break;
  case 0x04: /* alu */
    proto_tree_add_item (code_tree, hf_code_src, tvb, offset, 2, ENC_BIG_ENDIAN);
    proto_tree_add_item (code_tree, hf_code_alu_op, tvb, offset, 2, ENC_BIG_ENDIAN);
    break;
  case 0x05: /* jmp */
    proto_tree_add_item (code_tree, hf_code_src, tvb, offset, 2, ENC_BIG_ENDIAN);
    proto_tree_add_item (code_tree, hf_code_jmp_op, tvb, offset, 2, ENC_BIG_ENDIAN);
    break;
  case 0x06: /* ret */
    proto_tree_add_item (code_tree, hf_code_rval, tvb, offset, 2, ENC_BIG_ENDIAN);
    break;
  case 0x07: /* misc */
    proto_tree_add_item (code_tree, hf_code_misc_op, tvb, offset, 2, ENC_BIG_ENDIAN);
    break;
  default:
    proto_tree_add_item (code_tree, hf_code_fields, tvb, offset, 2, ENC_BIG_ENDIAN);
    break;
  }
  offset += 2;

  proto_tree_add_item (tree, hf_jt, tvb, offset, 1, ENC_BIG_ENDIAN);
  offset += 1;

  proto_tree_add_item (tree, hf_jf, tvb, offset, 1, ENC_BIG_ENDIAN);
  offset += 1;

  proto_tree_add_item (tree, hf_instr_value, tvb, offset, 4, ENC_BIG_ENDIAN);
  offset += 4;

  return offset;
}


static void
dissect_rpcap_filter (tvbuff_t *tvb, packet_info *pinfo,
                      proto_tree *parent_tree, gint offset)
{
  proto_tree *tree;
  proto_item *ti;
  guint32 nitems, i;

  ti = proto_tree_add_item (parent_tree, hf_filter, tvb, offset, -1, ENC_NA);
  tree = proto_item_add_subtree (ti, ett_filter);

  proto_tree_add_item (tree, hf_filtertype, tvb, offset, 2, ENC_BIG_ENDIAN);
  offset += 2;

  proto_tree_add_item (tree, hf_dummy, tvb, offset, 2, ENC_BIG_ENDIAN);
  offset += 2;

  nitems = tvb_get_ntohl (tvb, offset);
  proto_tree_add_item (tree, hf_nitems, tvb, offset, 4, ENC_BIG_ENDIAN);
  offset += 4;

  for (i = 0; i < nitems; i++) {
    offset = dissect_rpcap_filterbpf_insn (tvb, pinfo, tree, offset);
    if (tvb_reported_length_remaining (tvb, offset) < 0) {
      /* No more data in packet */
      expert_add_info(pinfo, ti, &ei_no_more_data);
      break;
    }
  }
}


static int
dissect_rpcap_auth_request (tvbuff_t *tvb, packet_info *pinfo _U_,
                            proto_tree *parent_tree, gint offset)
{
  proto_tree *tree;
  proto_item *ti;
  guint16 type, slen1, slen2;

  ti = proto_tree_add_item (parent_tree, hf_auth_request, tvb, offset, -1, ENC_NA);
  tree = proto_item_add_subtree (ti, ett_auth_request);

  type = tvb_get_ntohs (tvb, offset);
  proto_tree_add_item (tree, hf_auth_type, tvb, offset, 2, ENC_BIG_ENDIAN);
  offset += 2;

  proto_tree_add_item (tree, hf_dummy, tvb, offset, 2, ENC_BIG_ENDIAN);
  offset += 2;

  slen1 = tvb_get_ntohs (tvb, offset);
  proto_tree_add_item (tree, hf_auth_slen1, tvb, offset, 2, ENC_BIG_ENDIAN);
  offset += 2;

  slen2 = tvb_get_ntohs (tvb, offset);
  proto_tree_add_item (tree, hf_auth_slen2, tvb, offset, 2, ENC_BIG_ENDIAN);
  offset += 2;

  if (type == RPCAP_RMTAUTH_NULL) {
    proto_item_append_text (ti, " (none)");
  } else if (type == RPCAP_RMTAUTH_PWD) {
    const guint8 *username, *password;

    proto_tree_add_item_ret_string(tree, hf_auth_username, tvb, offset, slen1, ENC_ASCII|ENC_NA, wmem_packet_scope(), &username);
    offset += slen1;

    proto_tree_add_item_ret_string(tree, hf_auth_password, tvb, offset, slen2, ENC_ASCII|ENC_NA, wmem_packet_scope(), &password);
    offset += slen2;

    proto_item_append_text (ti, " (%s/%s)", username, password);
  }
  return offset;
}


static void
dissect_rpcap_open_request (tvbuff_t *tvb, packet_info *pinfo _U_,
                            proto_tree *parent_tree, gint offset)
{
  gint len;

  len = tvb_captured_length_remaining (tvb, offset);
  proto_tree_add_item (parent_tree, hf_open_request, tvb, offset, len, ENC_ASCII|ENC_NA);
}


static void
dissect_rpcap_open_reply (tvbuff_t *tvb, packet_info *pinfo _U_,
                          proto_tree *parent_tree, gint offset)
{
  proto_tree *tree;
  proto_item *ti;

  ti = proto_tree_add_item (parent_tree, hf_open_reply, tvb, offset, -1, ENC_NA);
  tree = proto_item_add_subtree (ti, ett_open_reply);

  linktype = tvb_get_ntohl (tvb, offset);
  proto_tree_add_item (tree, hf_linktype, tvb, offset, 4, ENC_BIG_ENDIAN);
  offset += 4;

  proto_tree_add_item (tree, hf_tzoff, tvb, offset, 4, ENC_BIG_ENDIAN);
}


static void
dissect_rpcap_startcap_request (tvbuff_t *tvb, packet_info *pinfo,
                                proto_tree *parent_tree, gint offset)
{
  proto_tree *tree, *field_tree;
  proto_item *ti, *field_ti;
  guint16 flags;

  ti = proto_tree_add_item (parent_tree, hf_startcap_request, tvb, offset, -1, ENC_NA);
  tree = proto_item_add_subtree (ti, ett_startcap_request);

  proto_tree_add_item (tree, hf_snaplen, tvb, offset, 4, ENC_BIG_ENDIAN);
  offset += 4;

  proto_tree_add_item (tree, hf_read_timeout, tvb, offset, 4, ENC_BIG_ENDIAN);
  offset += 4;

  flags = tvb_get_ntohs (tvb, offset);
  field_ti = proto_tree_add_uint_format (tree, hf_flags, tvb, offset, 2, flags, "Flags");
  field_tree = proto_item_add_subtree (field_ti, ett_startcap_flags);
  proto_tree_add_item (field_tree, hf_flags_promisc, tvb, offset, 2, ENC_BIG_ENDIAN);
  proto_tree_add_item (field_tree, hf_flags_dgram, tvb, offset, 2, ENC_BIG_ENDIAN);
  proto_tree_add_item (field_tree, hf_flags_serveropen, tvb, offset, 2, ENC_BIG_ENDIAN);
  proto_tree_add_item (field_tree, hf_flags_inbound, tvb, offset, 2, ENC_BIG_ENDIAN);
  proto_tree_add_item (field_tree, hf_flags_outbound, tvb, offset, 2, ENC_BIG_ENDIAN);

  if (flags & 0x1F) {
    gchar *flagstr = wmem_strdup_printf (wmem_packet_scope(), "%s%s%s%s%s",
          (flags & FLAG_PROMISC)    ? ", Promiscuous" : "",
          (flags & FLAG_DGRAM)      ? ", Datagram"    : "",
          (flags & FLAG_SERVEROPEN) ? ", ServerOpen"  : "",
          (flags & FLAG_INBOUND)    ? ", Inbound"     : "",
          (flags & FLAG_OUTBOUND)   ? ", Outbound"    : "");
    proto_item_append_text (field_ti, ":%s", &flagstr[1]);
  } else {
    proto_item_append_text (field_ti, " (none)");
  }
  offset += 2;

  proto_tree_add_item (tree, hf_client_port, tvb, offset, 2, ENC_BIG_ENDIAN);
  offset += 2;

  dissect_rpcap_filter (tvb, pinfo, tree, offset);
}


static void
dissect_rpcap_startcap_reply (tvbuff_t *tvb, packet_info *pinfo _U_,
                              proto_tree *parent_tree, gint offset)
{
  proto_tree *tree;
  proto_item *ti;

  ti = proto_tree_add_item (parent_tree, hf_startcap_reply, tvb, offset, -1, ENC_NA);
  tree = proto_item_add_subtree (ti, ett_startcap_reply);

  proto_tree_add_item (tree, hf_bufsize, tvb, offset, 4, ENC_BIG_ENDIAN);
  offset += 4;

  proto_tree_add_item (tree, hf_server_port, tvb, offset, 2, ENC_BIG_ENDIAN);
  offset += 2;

  proto_tree_add_item (tree, hf_dummy, tvb, offset, 2, ENC_BIG_ENDIAN);
}


static void
dissect_rpcap_stats_reply (tvbuff_t *tvb, packet_info *pinfo _U_,
                           proto_tree *parent_tree, gint offset)
{
  proto_tree *tree;
  proto_item *ti;

  ti = proto_tree_add_item (parent_tree, hf_stats_reply, tvb, offset, 16, ENC_NA);
  tree = proto_item_add_subtree (ti, ett_stats_reply);

  proto_tree_add_item (tree, hf_ifrecv, tvb, offset, 4, ENC_BIG_ENDIAN);
  offset += 4;

  proto_tree_add_item (tree, hf_ifdrop, tvb, offset, 4, ENC_BIG_ENDIAN);
  offset += 4;

  proto_tree_add_item (tree, hf_krnldrop, tvb, offset, 4, ENC_BIG_ENDIAN);
  offset += 4;

  proto_tree_add_item (tree, hf_srvcapt, tvb, offset, 4, ENC_BIG_ENDIAN);
}


static int
dissect_rpcap_sampling_request (tvbuff_t *tvb, packet_info *pinfo _U_,
                                proto_tree *parent_tree, gint offset)
{
  proto_tree *tree;
  proto_item *ti;
  guint32 value;
  guint8 method;

  ti = proto_tree_add_item (parent_tree, hf_sampling_request, tvb, offset, -1, ENC_NA);
  tree = proto_item_add_subtree (ti, ett_sampling_request);

  method = tvb_get_guint8 (tvb, offset);
  proto_tree_add_item (tree, hf_sampling_method, tvb, offset, 1, ENC_BIG_ENDIAN);
  offset += 1;

  proto_tree_add_item (tree, hf_sampling_dummy1, tvb, offset, 1, ENC_BIG_ENDIAN);
  offset += 1;

  proto_tree_add_item (tree, hf_sampling_dummy2, tvb, offset, 2, ENC_BIG_ENDIAN);
  offset += 2;

  value = tvb_get_ntohl (tvb, offset);
  proto_tree_add_item (tree, hf_sampling_value, tvb, offset, 4, ENC_BIG_ENDIAN);
  offset += 4;

  switch (method) {
  case RPCAP_SAMP_NOSAMP:
    proto_item_append_text (ti, ": None");
    break;
  case RPCAP_SAMP_1_EVERY_N:
    proto_item_append_text (ti, ": 1 every %d", value);
    break;
  case RPCAP_SAMP_FIRST_AFTER_N_MS:
    proto_item_append_text (ti, ": First after %d ms", value);
    break;
  default:
    break;
  }
  return offset;
}


static void
dissect_rpcap_packet (tvbuff_t *tvb, packet_info *pinfo, proto_tree *top_tree,
                      proto_tree *parent_tree, gint offset, proto_item *top_item)
{
  proto_tree *tree;
  proto_item *ti;
  nstime_t ts;
  tvbuff_t *new_tvb;
  guint caplen, len, frame_no;
  gint reported_length_remaining;

  ti = proto_tree_add_item (parent_tree, hf_packet, tvb, offset, 20, ENC_NA);
  tree = proto_item_add_subtree (ti, ett_packet);

  ts.secs = tvb_get_ntohl (tvb, offset);
  ts.nsecs = tvb_get_ntohl (tvb, offset + 4) * 1000;
  proto_tree_add_time(tree, hf_timestamp, tvb, offset, 8, &ts);
  offset += 8;

  caplen = tvb_get_ntohl (tvb, offset);
  ti = proto_tree_add_item (tree, hf_caplen, tvb, offset, 4, ENC_BIG_ENDIAN);
  offset += 4;

  len = tvb_get_ntohl (tvb, offset);
  proto_tree_add_item (tree, hf_len, tvb, offset, 4, ENC_BIG_ENDIAN);
  offset += 4;

  frame_no = tvb_get_ntohl (tvb, offset);
  proto_tree_add_item (tree, hf_npkt, tvb, offset, 4, ENC_BIG_ENDIAN);
  offset += 4;

  proto_item_append_text (ti, ", Frame %u", frame_no);
  proto_item_append_text (top_item, " Frame %u", frame_no);

  /*
   * reported_length_remaining should not be -1, as offset is at
   * most right past the end of the available data in the packet.
   */
  reported_length_remaining = tvb_reported_length_remaining (tvb, offset);
  if (caplen > (guint)reported_length_remaining) {
    expert_add_info(pinfo, ti, &ei_caplen_too_big);
    return;
  }

  new_tvb = tvb_new_subset (tvb, offset, caplen, len);
  if (decode_content && linktype != -1) {
    TRY {
      call_dissector_with_data(pcap_pktdata_handle, new_tvb, pinfo, top_tree, &linktype);
    }
    CATCH_BOUNDS_ERRORS {
      show_exception(tvb, pinfo, top_tree, EXCEPT_CODE, GET_MESSAGE);
    }
    ENDTRY;

    if (!info_added) {
      /* Only indicate when not added before */
      /* Indicate RPCAP in the protocol column */
      col_prepend_fence_fstr(pinfo->cinfo, COL_PROTOCOL, "R|");

      /* Indicate RPCAP in the info column */
      col_prepend_fence_fstr (pinfo->cinfo, COL_INFO, "Remote | ");
      info_added = TRUE;
      register_frame_end_routine(pinfo, rpcap_frame_end);
    }
  } else {
    if (linktype == -1) {
      proto_item_append_text (ti, ", Unknown link-layer type");
    }
    call_data_dissector(new_tvb, pinfo, top_tree);
  }
}


static int
dissect_rpcap (tvbuff_t *tvb, packet_info *pinfo, proto_tree *top_tree, void* data _U_)
{
  proto_tree *tree;
  proto_item *ti;
  tvbuff_t *new_tvb;
  gint len, offset = 0;
  guint8 msg_type;
  guint16 msg_value;

  col_set_str (pinfo->cinfo, COL_PROTOCOL, PSNAME);

  col_clear(pinfo->cinfo, COL_INFO);

  ti = proto_tree_add_item (top_tree, proto_rpcap, tvb, offset, -1, ENC_NA);
  tree = proto_item_add_subtree (ti, ett_rpcap);

  proto_tree_add_item (tree, hf_version, tvb, offset, 1, ENC_BIG_ENDIAN);
  offset++;

  msg_type = tvb_get_guint8 (tvb, offset);
  proto_tree_add_item (tree, hf_type, tvb, offset, 1, ENC_BIG_ENDIAN);
  offset++;

  col_append_fstr (pinfo->cinfo, COL_INFO, "%s",
                     val_to_str (msg_type, message_type, "Unknown: %d"));

  proto_item_append_text (ti, ", %s", val_to_str (msg_type, message_type, "Unknown: %d"));

  msg_value = tvb_get_ntohs (tvb, offset);
  if (msg_type == RPCAP_MSG_ERROR) {
    proto_tree_add_item (tree, hf_error_value, tvb, offset, 2, ENC_BIG_ENDIAN);
  } else {
    proto_tree_add_item (tree, hf_value, tvb, offset, 2, ENC_BIG_ENDIAN);
  }
  offset += 2;

  proto_tree_add_item (tree, hf_plen, tvb, offset, 4, ENC_BIG_ENDIAN);
  offset += 4;


  switch (msg_type) {
  case RPCAP_MSG_ERROR:
    dissect_rpcap_error (tvb, pinfo, tree, offset);
    break;
  case RPCAP_MSG_OPEN_REQ:
    dissect_rpcap_open_request (tvb, pinfo, tree, offset);
    break;
  case RPCAP_MSG_STARTCAP_REQ:
    dissect_rpcap_startcap_request (tvb, pinfo, tree, offset);
    break;
  case RPCAP_MSG_UPDATEFILTER_REQ:
    dissect_rpcap_filter (tvb, pinfo, tree, offset);
    break;
  case RPCAP_MSG_PACKET:
    proto_item_set_len (ti, 28);
    dissect_rpcap_packet (tvb, pinfo, top_tree, tree, offset, ti);
    break;
  case RPCAP_MSG_AUTH_REQ:
    dissect_rpcap_auth_request (tvb, pinfo, tree, offset);
    break;
  case RPCAP_MSG_SETSAMPLING_REQ:
    dissect_rpcap_sampling_request (tvb, pinfo, tree, offset);
    break;
  case RPCAP_MSG_FINDALLIF_REPLY:
    dissect_rpcap_findalldevs_reply (tvb, pinfo, tree, offset, msg_value);
    break;
  case RPCAP_MSG_OPEN_REPLY:
    dissect_rpcap_open_reply (tvb, pinfo, tree, offset);
    break;
  case RPCAP_MSG_STARTCAP_REPLY:
    dissect_rpcap_startcap_reply (tvb, pinfo, tree, offset);
    break;
  case RPCAP_MSG_STATS_REPLY:
    dissect_rpcap_stats_reply (tvb, pinfo, tree, offset);
    break;
  default:
    len = tvb_reported_length_remaining (tvb, offset);
    if (len) {
      /* Yet unknown, dump as data */
      proto_item_set_len (ti, 8);
      new_tvb = tvb_new_subset_remaining (tvb, offset);
      call_data_dissector(new_tvb, pinfo, top_tree);
    }
    break;
  }

  return tvb_captured_length(tvb);
}


static gboolean
check_rpcap_heur (tvbuff_t *tvb, gboolean tcp)
{
  gint offset = 0;
  guint8 version, msg_type;
  guint16 msg_value;
  guint32 plen, len, caplen;

  if (tvb_captured_length (tvb) < 8)
    /* Too short */
    return FALSE;

  version = tvb_get_guint8 (tvb, offset);
  if (version != 0)
    /* Incorrect version */
    return FALSE;
  offset++;

  msg_type = tvb_get_guint8 (tvb, offset);
  if (!tcp && msg_type != 7) {
    /* UDP is only used for packets */
    return FALSE;
  }
  if (try_val_to_str(msg_type, message_type) == NULL)
    /* Unknown message type */
    return FALSE;
  offset++;

  msg_value = tvb_get_ntohs (tvb, offset);
  if (msg_value > 0) {
    if (msg_type == RPCAP_MSG_ERROR) {
      /* Must have a valid error code */
      if (try_val_to_str(msg_value, error_codes) == NULL)
        return FALSE;
    } else if (msg_type != RPCAP_MSG_FINDALLIF_REPLY) {
      return FALSE;
    }
  }
  offset += 2;

  plen = tvb_get_ntohl (tvb, offset);
  offset += 4;
  len = (guint32) tvb_reported_length_remaining (tvb, offset);

  switch (msg_type) {

  case RPCAP_MSG_FINDALLIF_REQ:
  case RPCAP_MSG_UPDATEFILTER_REPLY:
  case RPCAP_MSG_AUTH_REPLY:
  case RPCAP_MSG_STATS_REQ:
  case RPCAP_MSG_CLOSE:
  case RPCAP_MSG_SETSAMPLING_REPLY:
  case RPCAP_MSG_ENDCAP_REQ:
  case RPCAP_MSG_ENDCAP_REPLY:
    /* Empty payload */
    if (plen != 0 || len != 0)
      return FALSE;
    break;

  case RPCAP_MSG_OPEN_REPLY:
  case RPCAP_MSG_STARTCAP_REPLY:
  case RPCAP_MSG_SETSAMPLING_REQ:
    /* Always 8 bytes */
    if (plen != 8 || len != 8)
      return FALSE;
    break;

  case RPCAP_MSG_STATS_REPLY:
    /* Always 16 bytes */
    if (plen != 16 || len != 16)
      return FALSE;
    break;

  case RPCAP_MSG_PACKET:
    /* Must have the frame header */
    if (plen < 20)
      return FALSE;

    /* Check if capture length is valid */
    caplen = tvb_get_ntohl (tvb, offset+8);
    /* Always 20 bytes less than packet length */
    if (caplen != (plen - 20) || caplen > 65535)
      return FALSE;
    break;

  case RPCAP_MSG_FINDALLIF_REPLY:
  case RPCAP_MSG_ERROR:
  case RPCAP_MSG_OPEN_REQ:
  case RPCAP_MSG_STARTCAP_REQ:
  case RPCAP_MSG_UPDATEFILTER_REQ:
  case RPCAP_MSG_AUTH_REQ:
    /* Variable length */
    if (plen != len)
      return FALSE;
    break;
  default:
    /* Unknown message type */
    return FALSE;
  }

  return TRUE;
}


static guint
get_rpcap_pdu_len (packet_info *pinfo _U_, tvbuff_t *tvb, int offset, void *data _U_)
{
  return tvb_get_ntohl (tvb, offset + 4) + 8;
}


static gboolean
dissect_rpcap_heur_tcp (tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data)
{
  if (check_rpcap_heur (tvb, TRUE)) {
    /* This is probably a rpcap tcp package */
    tcp_dissect_pdus (tvb, pinfo, tree, rpcap_desegment, 8,
                      get_rpcap_pdu_len, dissect_rpcap, data);

    return TRUE;
  }

  return FALSE;
}


static gboolean
dissect_rpcap_heur_udp (tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data)
{
  if (check_rpcap_heur (tvb, FALSE)) {
    /* This is probably a rpcap udp package */
    dissect_rpcap (tvb, pinfo, tree, data);

    return TRUE;
  }

  return FALSE;
}


void
proto_register_rpcap (void)
{
  static hf_register_info hf[] = {
    /* Common header for all messages */
    { &hf_version,
      { "Version", "rpcap.version", FT_UINT8, BASE_DEC,
        NULL, 0x0, NULL, HFILL } },
    { &hf_type,
      { "Message type", "rpcap.type", FT_UINT8, BASE_DEC,
        VALS(message_type), 0x0, NULL, HFILL } },
    { &hf_value,
      { "Message value", "rpcap.value", FT_UINT16, BASE_DEC,
        NULL, 0x0, NULL, HFILL } },
    { &hf_plen,
      { "Payload length", "rpcap.len", FT_UINT32, BASE_DEC,
        NULL, 0x0, NULL, HFILL } },

    /* Error */
    { &hf_error,
      { "Error", "rpcap.error", FT_STRING, BASE_NONE,
        NULL, 0x0, "Error text", HFILL } },
    { &hf_error_value,
      { "Error value", "rpcap.error_value", FT_UINT16, BASE_DEC,
        VALS(error_codes), 0x0, NULL, HFILL } },

    /* Packet header */
    { &hf_packet,
      { "Packet", "rpcap.packet", FT_NONE, BASE_NONE,
        NULL, 0x0, "Packet data", HFILL } },
    { &hf_timestamp,
      { "Arrival time", "rpcap.time", FT_ABSOLUTE_TIME, ABSOLUTE_TIME_LOCAL,
        NULL, 0x0, NULL, HFILL } },
    { &hf_caplen,
      { "Capture length", "rpcap.cap_len", FT_UINT32, BASE_DEC,
        NULL, 0x0, NULL, HFILL } },
    { &hf_len,
      { "Frame length", "rpcap.len", FT_UINT32, BASE_DEC,
        NULL, 0x0, "Frame length (off wire)", HFILL } },
    { &hf_npkt,
      { "Frame number", "rpcap.number", FT_UINT32, BASE_DEC,
        NULL, 0x0, NULL, HFILL } },

    /* Authentication request */
    { &hf_auth_request,
      { "Authentication", "rpcap.auth", FT_NONE, BASE_NONE,
        NULL, 0x0, NULL, HFILL } },
    { &hf_auth_type,
      { "Authentication type", "rpcap.auth_type", FT_UINT16, BASE_DEC,
        VALS(auth_type), 0x0, NULL, HFILL } },
    { &hf_auth_slen1,
      { "Authentication item length 1", "rpcap.auth_len1", FT_UINT16, BASE_DEC,
        NULL, 0x0, NULL, HFILL } },
    { &hf_auth_slen2,
      { "Authentication item length 2", "rpcap.auth_len2", FT_UINT16, BASE_DEC,
        NULL, 0x0, NULL, HFILL } },
    { &hf_auth_username,
      { "Username", "rpcap.username", FT_STRING, BASE_NONE,
        NULL, 0x0, NULL, HFILL } },
    { &hf_auth_password,
      { "Password", "rpcap.password", FT_STRING, BASE_NONE,
        NULL, 0x0, NULL, HFILL } },

    /* Open request */
    { &hf_open_request,
      { "Open request", "rpcap.open_request", FT_STRING, BASE_NONE,
        NULL, 0x0, NULL, HFILL } },

    /* Open reply */
    { &hf_open_reply,
      { "Open reply", "rpcap.open_reply", FT_NONE, BASE_NONE,
        NULL, 0x0, NULL, HFILL } },
    /*
     * XXX - the code probably sends a DLT_ value over the wire, but
     * it should really send a LINKTYPE_ value, so that if the client
     * and server are running OSes that disagree on the numerical value
     * of that DLT_, they won't get confused (LINKTYPE_ values aren't
     * platform-dependent).  The vast majority of LINKTYPE_ values and
     * DLT_ values are the same for the same link-layer type.
     */
    { &hf_linktype,
      { "Link type", "rpcap.linktype", FT_UINT32, BASE_DEC,
        VALS(link_type_vals), 0x0, NULL, HFILL } },
    { &hf_tzoff,
      { "Timezone offset", "rpcap.tzoff", FT_UINT32, BASE_DEC,
        NULL, 0x0, NULL, HFILL } },

    /* Start capture request */
    { &hf_startcap_request,
      { "Start capture request", "rpcap.startcap_request", FT_NONE, BASE_NONE,
        NULL, 0x0, NULL, HFILL } },
    { &hf_snaplen,
      { "Snap length", "rpcap.snaplen", FT_UINT32, BASE_DEC,
        NULL, 0x0, NULL, HFILL } },
    { &hf_read_timeout,
      { "Read timeout", "rpcap.read_timeout", FT_UINT32, BASE_DEC,
        NULL, 0x0, NULL, HFILL } },
    { &hf_flags,
      { "Flags", "rpcap.flags", FT_UINT16, BASE_DEC,
        NULL, 0x0, "Capture flags", HFILL } },
    { &hf_flags_promisc,
      { "Promiscuous mode", "rpcap.flags.promisc", FT_BOOLEAN, 16,
        TFS(&tfs_enabled_disabled), FLAG_PROMISC, NULL, HFILL } },
    { &hf_flags_dgram,
      { "Use Datagram", "rpcap.flags.dgram", FT_BOOLEAN, 16,
        TFS(&tfs_yes_no), FLAG_DGRAM, NULL, HFILL } },
    { &hf_flags_serveropen,
      { "Server open", "rpcap.flags.serveropen", FT_BOOLEAN, 16,
        TFS(&open_closed), FLAG_SERVEROPEN, NULL, HFILL } },
    { &hf_flags_inbound,
      { "Inbound", "rpcap.flags.inbound", FT_BOOLEAN, 16,
        TFS(&tfs_yes_no), FLAG_INBOUND, NULL, HFILL } },
    { &hf_flags_outbound,
      { "Outbound", "rpcap.flags.outbound", FT_BOOLEAN, 16,
        TFS(&tfs_yes_no), FLAG_OUTBOUND, NULL, HFILL } },
    { &hf_client_port,
      { "Client Port", "rpcap.client_port", FT_UINT16, BASE_DEC,
        NULL, 0x0, NULL, HFILL } },

    /* Start capture reply */
    { &hf_startcap_reply,
      { "Start capture reply", "rpcap.startcap_reply", FT_NONE, BASE_NONE,
        NULL, 0x0, NULL, HFILL } },
    { &hf_bufsize,
      { "Buffer size", "rpcap.bufsize", FT_UINT32, BASE_DEC,
        NULL, 0x0, NULL, HFILL } },
    { &hf_server_port,
      { "Server port", "rpcap.server_port", FT_UINT16, BASE_DEC,
        NULL, 0x0, NULL, HFILL } },
    { &hf_dummy,
      { "Dummy", "rpcap.dummy", FT_UINT16, BASE_DEC,
        NULL, 0x0, NULL, HFILL } },

    /* Filter */
    { &hf_filter,
      { "Filter", "rpcap.filter", FT_NONE, BASE_NONE,
        NULL, 0x0, NULL, HFILL } },
    { &hf_filtertype,
      { "Filter type", "rpcap.filtertype", FT_UINT16, BASE_DEC,
        NULL, 0x0, "Filter type (BPF)", HFILL } },
    { &hf_nitems,
      { "Number of items", "rpcap.nitems", FT_UINT32, BASE_DEC,
        NULL, 0x0, NULL, HFILL } },

    /* Filter BPF instruction */
    { &hf_filterbpf_insn,
      { "Filter BPF instruction", "rpcap.filterbpf_insn", FT_NONE, BASE_NONE,
        NULL, 0x0, NULL, HFILL } },
    { &hf_code,
      { "Op code", "rpcap.opcode", FT_UINT16, BASE_HEX,
        NULL, 0x0, "Operation code", HFILL } },
    { &hf_code_class,
      { "Class", "rpcap.opcode.class", FT_UINT16, BASE_HEX,
        VALS(bpf_class), 0x07, "Instruction Class", HFILL } },
    { &hf_code_fields,
      { "Fields", "rpcap.opcode.fields", FT_UINT16, BASE_HEX,
        NULL, 0xF8, "Class Fields", HFILL } },
    { &hf_code_ld_size,
      { "Size", "rpcap.opcode.size", FT_UINT16, BASE_HEX,
        VALS(bpf_size), 0x18, NULL, HFILL } },
    { &hf_code_ld_mode,
      { "Mode", "rpcap.opcode.mode", FT_UINT16, BASE_HEX,
        VALS(bpf_mode), 0xE0, NULL, HFILL } },
    { &hf_code_alu_op,
      { "Op", "rpcap.opcode.aluop", FT_UINT16, BASE_HEX,
        VALS(bpf_alu_op), 0xF0, NULL, HFILL } },
    { &hf_code_jmp_op,
      { "Op", "rpcap.opcode.jmpop", FT_UINT16, BASE_HEX,
        VALS(bpf_jmp_op), 0xF0, NULL, HFILL } },
    { &hf_code_src,
      { "Src", "rpcap.opcode.src", FT_UINT16, BASE_HEX,
        VALS(bpf_src), 0x08, NULL, HFILL } },
    { &hf_code_rval,
      { "Rval", "rpcap.opcode.rval", FT_UINT16, BASE_HEX,
        VALS(bpf_rval), 0x18, NULL, HFILL } },
    { &hf_code_misc_op,
      { "Op", "rpcap.opcode.miscop", FT_UINT16, BASE_HEX,
        VALS(bpf_misc_op), 0xF8, NULL, HFILL } },
    { &hf_jt,
      { "JT", "rpcap.jt", FT_UINT8, BASE_DEC,
        NULL, 0x0, NULL, HFILL } },
    { &hf_jf,
      { "JF", "rpcap.jf", FT_UINT8, BASE_DEC,
        NULL, 0x0, NULL, HFILL } },
    { &hf_instr_value,
      { "Instruction value", "rpcap.instr_value", FT_UINT32, BASE_DEC,
        NULL, 0x0, "Instruction-Dependent value", HFILL } },

    /* Statistics reply */
    { &hf_stats_reply,
      { "Statistics", "rpcap.stats_reply", FT_NONE, BASE_NONE,
        NULL, 0x0, "Statistics reply data", HFILL } },
    { &hf_ifrecv,
      { "Received by kernel filter", "rpcap.ifrecv", FT_UINT32, BASE_DEC,
        NULL, 0x0, "Received by kernel", HFILL } },
    { &hf_ifdrop,
      { "Dropped by network interface", "rpcap.ifdrop", FT_UINT32, BASE_DEC,
        NULL, 0x0, NULL, HFILL } },
    { &hf_krnldrop,
      { "Dropped by kernel filter", "rpcap.krnldrop", FT_UINT32, BASE_DEC,
        NULL, 0x0, NULL, HFILL } },
    { &hf_srvcapt,
      { "Captured by rpcapd", "rpcap.srvcapt", FT_UINT32, BASE_DEC,
        NULL, 0x0, "Captured by RPCAP daemon", HFILL } },

    /* Find all devices reply */
    { &hf_findalldevs_reply,
      { "Find all devices", "rpcap.findalldevs_reply", FT_NONE, BASE_NONE,
        NULL, 0x0, NULL, HFILL } },
    { &hf_findalldevs_if,
      { "Interface", "rpcap.if", FT_NONE, BASE_NONE,
        NULL, 0x0, NULL, HFILL } },
    { &hf_namelen,
      { "Name length", "rpcap.namelen", FT_UINT16, BASE_DEC,
        NULL, 0x0, NULL, HFILL } },
    { &hf_desclen,
      { "Description length", "rpcap.desclen", FT_UINT32, BASE_DEC,
        NULL, 0x0, NULL, HFILL } },
    { &hf_if_flags,
      { "Interface flags", "rpcap.if.flags", FT_UINT32, BASE_DEC,
        NULL, 0x0, NULL, HFILL } },
    { &hf_naddr,
      { "Number of addresses", "rpcap.naddr", FT_UINT32, BASE_DEC,
        NULL, 0x0, NULL, HFILL } },
    { &hf_if_name,
      { "Name", "rpcap.ifname", FT_STRING, BASE_NONE,
        NULL, 0x0, "Interface name", HFILL } },
    { &hf_if_desc,
      { "Description", "rpcap.ifdesc", FT_STRING, BASE_NONE,
        NULL, 0x0, "Interface description", HFILL } },

    /* Find all devices / Interface addresses */
    { &hf_findalldevs_ifaddr,
      { "Interface address", "rpcap.ifaddr", FT_NONE, BASE_NONE,
        NULL, 0x0, NULL, HFILL } },
    { &hf_if_addr,
      { "Address", "rpcap.addr", FT_NONE, BASE_NONE,
        NULL, 0x0, "Network address", HFILL } },
    { &hf_if_netmask,
      { "Netmask", "rpcap.netmask", FT_NONE, BASE_NONE,
        NULL, 0x0, NULL, HFILL } },
    { &hf_if_broadaddr,
      { "Broadcast", "rpcap.broadaddr", FT_NONE, BASE_NONE,
        NULL, 0x0, NULL, HFILL } },
    { &hf_if_dstaddr,
      { "P2P destination address", "rpcap.dstaddr", FT_NONE, BASE_NONE,
        NULL, 0x0, NULL, HFILL } },
    { &hf_if_af,
      { "Address family", "rpcap.if.af", FT_UINT16, BASE_HEX,
        VALS(address_family), 0x0, NULL, HFILL } },
    { &hf_if_port,
      { "Port", "rpcap.if.port", FT_UINT16, BASE_DEC,
        NULL, 0x0, "Port number", HFILL } },
    { &hf_if_ip,
      { "IP address", "rpcap.if.ip", FT_IPv4, BASE_NONE,
        NULL, 0x0, NULL, HFILL } },
    { &hf_if_padding,
      { "Padding", "rpcap.if.padding", FT_BYTES, BASE_NONE,
        NULL, 0x0, NULL, HFILL } },
    { &hf_if_unknown,
      { "Unknown address", "rpcap.if.unknown", FT_BYTES, BASE_NONE,
        NULL, 0x0, NULL, HFILL } },

    /* Sampling request */
    { &hf_sampling_request,
      { "Sampling", "rpcap.sampling_request", FT_NONE, BASE_NONE,
        NULL, 0x0, NULL, HFILL } },
    { &hf_sampling_method,
      { "Method", "rpcap.sampling_method", FT_UINT8, BASE_DEC,
        VALS(sampling_method), 0x0, "Sampling method", HFILL } },
    { &hf_sampling_dummy1,
      { "Dummy1", "rpcap.dummy", FT_UINT8, BASE_DEC,
        NULL, 0x0, NULL, HFILL } },
    { &hf_sampling_dummy2,
      { "Dummy2", "rpcap.dummy", FT_UINT16, BASE_DEC,
        NULL, 0x0, NULL, HFILL } },
    { &hf_sampling_value,
      { "Value", "rpcap.sampling_value", FT_UINT32, BASE_DEC,
        NULL, 0x0, NULL, HFILL } },
  };

  static gint *ett[] = {
    &ett_rpcap,
    &ett_error,
    &ett_packet,
    &ett_auth_request,
    &ett_open_reply,
    &ett_startcap_request,
    &ett_startcap_reply,
    &ett_startcap_flags,
    &ett_filter,
    &ett_filterbpf_insn,
    &ett_filterbpf_insn_code,
    &ett_stats_reply,
    &ett_findalldevs_reply,
    &ett_findalldevs_if,
    &ett_findalldevs_ifaddr,
    &ett_ifaddr,
    &ett_sampling_request
  };

  static ei_register_info ei[] = {
     { &ei_error, { "rpcap.error.expert", PI_SEQUENCE, PI_NOTE, "Error", EXPFILL }},
     { &ei_if_unknown, { "rpcap.if_unknown", PI_SEQUENCE, PI_NOTE, "Unknown address family", EXPFILL }},
     { &ei_no_more_data, { "rpcap.no_more_data", PI_MALFORMED, PI_ERROR, "No more data in packet", EXPFILL }},
     { &ei_caplen_too_big, { "rpcap.caplen_too_big", PI_MALFORMED, PI_ERROR, "Caplen is bigger than the remaining message length", EXPFILL }},
  };

  module_t *rpcap_module;
  expert_module_t* expert_rpcap;

  proto_rpcap = proto_register_protocol (PNAME, PSNAME, PFNAME);
  register_dissector (PFNAME, dissect_rpcap, proto_rpcap);
  expert_rpcap = expert_register_protocol(proto_rpcap);
  expert_register_field_array(expert_rpcap, ei, array_length(ei));

  proto_register_field_array (proto_rpcap, hf, array_length (hf));
  proto_register_subtree_array (ett, array_length (ett));

  /* Register our configuration options */
  rpcap_module = prefs_register_protocol (proto_rpcap, proto_reg_handoff_rpcap);

  prefs_register_bool_preference (rpcap_module, "desegment_pdus",
                                  "Reassemble PDUs spanning multiple TCP segments",
                                  "Whether the RPCAP dissector should reassemble PDUs"
                                  " spanning multiple TCP segments."
                                  " To use this option, you must also enable \"Allow subdissectors"
                                  " to reassemble TCP streams\" in the TCP protocol settings.",
                                  &rpcap_desegment);
  prefs_register_bool_preference (rpcap_module, "decode_content",
                                  "Decode content according to link-layer type",
                                  "Whether the packets should be decoded according to"
                                  " the link-layer type.",
                                  &decode_content);
  prefs_register_uint_preference (rpcap_module, "linktype",
                                  "Default link-layer type",
                                  "Default link-layer type to use if an Open Reply packet"
                                  " has not been received.",
                                  10, &global_linktype);
}

void
proto_reg_handoff_rpcap (void)
{
  static gboolean rpcap_prefs_initialized = FALSE;

  if (!rpcap_prefs_initialized) {
    pcap_pktdata_handle = find_dissector_add_dependency("pcap_pktdata", proto_rpcap);
    rpcap_prefs_initialized = TRUE;

    heur_dissector_add ("tcp", dissect_rpcap_heur_tcp, "RPCAP over TCP", "rpcap_tcp", proto_rpcap, HEURISTIC_ENABLE);
    heur_dissector_add ("udp", dissect_rpcap_heur_udp, "RPCAP over UDP", "rpcap_udp", proto_rpcap, HEURISTIC_ENABLE);
  }

  info_added = FALSE;
  linktype = global_linktype;
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
