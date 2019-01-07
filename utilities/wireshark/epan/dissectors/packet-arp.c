/* packet-arp.c
 * Routines for ARP packet disassembly (RFC 826)
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * By Deepti Ragha <dlragha@ncsu.edu>
 * Copyright 2012 Deepti Ragha
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

#include "config.h"

#include <epan/packet.h>
#include <epan/capture_dissectors.h>
#include <epan/arptypes.h>
#include <epan/addr_resolv.h>
#include "packet-arp.h"
#include <epan/etypes.h>
#include <epan/arcnet_pids.h>
#include <epan/ax25_pids.h>
#include <epan/prefs.h>
#include <epan/expert.h>
#include <epan/proto_data.h>

void proto_register_arp(void);
void proto_reg_handoff_arp(void);

static int proto_arp = -1;
static int hf_arp_hard_type = -1;
static int hf_arp_proto_type = -1;
static int hf_arp_hard_size = -1;
static int hf_atmarp_sht = -1;
static int hf_atmarp_shl = -1;
static int hf_atmarp_sst = -1;
static int hf_atmarp_ssl = -1;
static int hf_arp_proto_size = -1;
static int hf_arp_opcode = -1;
static int hf_arp_isgratuitous = -1;
static int hf_atmarp_spln = -1;
static int hf_atmarp_tht = -1;
static int hf_atmarp_thl = -1;
static int hf_atmarp_tst = -1;
static int hf_atmarp_tsl = -1;
static int hf_atmarp_tpln = -1;
static int hf_arp_src_hw = -1;
static int hf_arp_src_hw_mac = -1;
static int hf_arp_src_proto = -1;
static int hf_arp_src_proto_ipv4 = -1;
static int hf_arp_dst_hw = -1;
static int hf_arp_dst_hw_mac = -1;
static int hf_arp_dst_proto = -1;
static int hf_arp_dst_proto_ipv4 = -1;
static int hf_drarp_error_status = -1;
static int hf_arp_duplicate_ip_address_earlier_frame = -1;
static int hf_arp_duplicate_ip_address_seconds_since_earlier_frame = -1;

static int hf_atmarp_src_atm_num_e164 = -1;
static int hf_atmarp_src_atm_num_nsap = -1;
static int hf_atmarp_src_atm_subaddr = -1;
static int hf_atmarp_dst_atm_num_e164 = -1;
static int hf_atmarp_dst_atm_num_nsap = -1;
static int hf_atmarp_dst_atm_subaddr = -1;
/* Generated from convert_proto_tree_add_text.pl */
static int hf_atmarp_src_atm_data_country_code = -1;
static int hf_atmarp_src_atm_data_country_code_group = -1;
static int hf_atmarp_src_atm_e_164_isdn = -1;
static int hf_atmarp_src_atm_e_164_isdn_group = -1;
static int hf_atmarp_src_atm_rest_of_address = -1;
static int hf_atmarp_src_atm_end_system_identifier = -1;
static int hf_atmarp_src_atm_high_order_dsp = -1;
static int hf_atmarp_src_atm_selector = -1;
static int hf_atmarp_src_atm_international_code_designator = -1;
static int hf_atmarp_src_atm_international_code_designator_group = -1;
static int hf_atmarp_src_atm_afi = -1;

static int hf_arp_dst_hw_ax25 = -1;
static int hf_arp_src_hw_ax25 = -1;

static gint ett_arp = -1;
static gint ett_atmarp_nsap = -1;
static gint ett_atmarp_tl = -1;
static gint ett_arp_duplicate_address = -1;

static expert_field ei_seq_arp_dup_ip = EI_INIT;
static expert_field ei_seq_arp_storm = EI_INIT;
static expert_field ei_atmarp_src_atm_unknown_afi = EI_INIT;

static dissector_handle_t arp_handle;

static dissector_handle_t atmarp_handle;
static dissector_handle_t ax25arp_handle;

/* Used for determining if frequency of ARP requests constitute a storm */
#define STORM    1
#define NO_STORM 2

/* Preference settings */
static gboolean global_arp_detect_request_storm = FALSE;
static guint32  global_arp_detect_request_storm_packets = 30;
static guint32  global_arp_detect_request_storm_period = 100;

static gboolean global_arp_detect_duplicate_ip_addresses = TRUE;
static gboolean global_arp_register_network_address_binding = TRUE;

static guint32  arp_request_count = 0;
static nstime_t time_at_start_of_count;


/* Map of (IP address -> MAC address) to detect duplicate IP addresses
   Key is unsigned32 */
static GHashTable *address_hash_table = NULL;

typedef struct address_hash_value {
  guint8    mac[6];
  guint     frame_num;
  time_t    time_of_entry;
} address_hash_value;

/* Map of ((frame Num, IP address) -> MAC address) */
static GHashTable *duplicate_result_hash_table = NULL;

typedef struct duplicate_result_key {
  guint32 frame_number;
  guint32 ip_address;
} duplicate_result_key;


/* Definitions taken from Linux "linux/if_arp.h" header file, and from

   http://www.iana.org/assignments/arp-parameters

*/


/* ARP / RARP structs and definitions */
#ifndef ARPOP_REQUEST
#define ARPOP_REQUEST  1       /* ARP request.  */
#endif
#ifndef ARPOP_REPLY
#define ARPOP_REPLY    2       /* ARP reply.  */
#endif
/* Some OSes have different names, or don't define these at all */
#ifndef ARPOP_RREQUEST
#define ARPOP_RREQUEST 3       /* RARP request.  */
#endif
#ifndef ARPOP_RREPLY
#define ARPOP_RREPLY   4       /* RARP reply.  */
#endif

/*Additional parameters as per http://www.iana.org/assignments/arp-parameters*/
#ifndef ARPOP_DRARPREQUEST
#define ARPOP_DRARPREQUEST 5   /* DRARP request.  */
#endif

#ifndef ARPOP_DRARPREPLY
#define ARPOP_DRARPREPLY 6     /* DRARP reply.  */
#endif

#ifndef ARPOP_DRARPERROR
#define ARPOP_DRARPERROR 7     /* DRARP error.  */
#endif

#ifndef ARPOP_IREQUEST
#define ARPOP_IREQUEST 8       /* Inverse ARP (RFC 1293) request.  */
#endif
#ifndef ARPOP_IREPLY
#define ARPOP_IREPLY   9       /* Inverse ARP reply.  */
#endif
#ifndef ATMARPOP_NAK
#define ATMARPOP_NAK   10      /* ATMARP NAK.  */
#endif

/*Additional parameters as per http://www.iana.org/assignments/arp-parameters*/
#ifndef ARPOP_MARS_REQUEST
#define ARPOP_MARS_REQUEST   11       /*MARS request message. */
#endif

#ifndef ARPOP_MARS_MULTI
#define ARPOP_MARS_MULTI   12       /*MARS-Multi message. */
#endif

#ifndef ARPOP_MARS_MSERV
#define ARPOP_MARS_MSERV   13       /*MARS-Mserv message. */
#endif

#ifndef ARPOP_MARS_JOIN
#define ARPOP_MARS_JOIN  14       /*MARS-Join request. */
#endif

#ifndef ARPOP_MARS_LEAVE
#define ARPOP_MARS_LEAVE   15       /*MARS Leave request. */
#endif

#ifndef ARPOP_MARS_NAK
#define ARPOP_MARS_NAK   16       /*MARS nak message.*/
#endif

#ifndef ARPOP_MARS_UNSERV
#define ARPOP_MARS_UNSERV   17       /*MARS Unserv message. */
#endif

#ifndef ARPOP_MARS_SJOIN
#define ARPOP_MARS_SJOIN   18       /*MARS Sjoin message. */
#endif

#ifndef ARPOP_MARS_SLEAVE
#define ARPOP_MARS_SLEAVE   19       /*MARS Sleave message. */
#endif

#ifndef ARPOP_MARS_GROUPLIST_REQUEST
#define ARPOP_MARS_GROUPLIST_REQUEST   20       /*MARS Grouplist request message. */
#endif

#ifndef ARPOP_MARS_GROUPLIST_REPLY
#define ARPOP_MARS_GROUPLIST_REPLY   21       /*MARS Grouplist reply message. */
#endif

#ifndef ARPOP_MARS_REDIRECT_MAP
#define ARPOP_MARS_REDIRECT_MAP   22       /*MARS Grouplist request message. */
#endif

#ifndef ARPOP_MAPOS_UNARP
#define ARPOP_MAPOS_UNARP   23 /*MAPOS UNARP*/
#endif

#ifndef ARPOP_EXP1
#define ARPOP_EXP1     24      /* Experimental 1 */
#endif
#ifndef ARPOP_EXP2
#define ARPOP_EXP2     25      /* Experimental 2 */
#endif

#ifndef ARPOP_RESERVED1
#define ARPOP_RESERVED1         0  /*Reserved opcode 1*/
#endif

#ifndef ARPOP_RESERVED2
#define ARPOP_RESERVED2         65535 /*Reserved opcode 2*/
#endif

#ifndef DRARPERR_RESTRICTED
#define DRARPERR_RESTRICTED      1
#endif

#ifndef DRARPERR_NOADDRESSES
#define DRARPERR_NOADDRESSES     2
#endif

#ifndef DRARPERR_SERVERDOWN
#define DRARPERR_SERVERDOWN     3
#endif

#ifndef DRARPERR_MOVED
#define DRARPERR_MOVED     4
#endif

#ifndef DRARPERR_FAILURE
#define DRARPERR_FAILURE     5
#endif



static const value_string op_vals[] = {
  {ARPOP_REQUEST,                "request"                },
  {ARPOP_REPLY,                  "reply"                  },
  {ARPOP_RREQUEST,               "reverse request"        },
  {ARPOP_RREPLY,                 "reverse reply"          },
  {ARPOP_DRARPREQUEST,           "drarp request"          },
  {ARPOP_DRARPREPLY,             "drarp reply"            },
  {ARPOP_DRARPERROR,             "drarp error"            },
  {ARPOP_IREQUEST,               "inverse request"        },
  {ARPOP_IREPLY,                 "inverse reply"          },
  {ATMARPOP_NAK,                 "arp nak"                },
  {ARPOP_MARS_REQUEST,           "mars request"           },
  {ARPOP_MARS_MULTI,             "mars multi"             },
  {ARPOP_MARS_MSERV,             "mars mserv"             },
  {ARPOP_MARS_JOIN,              "mars join"              },
  {ARPOP_MARS_LEAVE,             "mars leave"             },
  {ARPOP_MARS_NAK,               "mars nak"               },
  {ARPOP_MARS_UNSERV,            "mars unserv"            },
  {ARPOP_MARS_SJOIN,             "mars sjoin"             },
  {ARPOP_MARS_SLEAVE,            "mars sleave"            },
  {ARPOP_MARS_GROUPLIST_REQUEST, "mars grouplist request" },
  {ARPOP_MARS_GROUPLIST_REPLY,   "mars gruoplist reply"   },
  {ARPOP_MARS_REDIRECT_MAP,      "mars redirect map"      },
  {ARPOP_MAPOS_UNARP,            "mapos unarp"            },
  {ARPOP_EXP1,                   "experimental 1"         },
  {ARPOP_EXP2,                   "experimental 2"         },
  {ARPOP_RESERVED1,              "reserved"               },
  {ARPOP_RESERVED2,              "reserved"               },
  {0, NULL}};

static const value_string drarp_status[]={
{DRARPERR_RESTRICTED,  "restricted" },
{DRARPERR_NOADDRESSES, "no address" },
{DRARPERR_SERVERDOWN,  "serverdown" },
{DRARPERR_MOVED,       "moved"      },
{DRARPERR_FAILURE,     "failure"    },
{0, NULL}};

static const value_string atmop_vals[] = {
  {ARPOP_REQUEST,                "request"                },
  {ARPOP_REPLY,                  "reply"                  },
  {ARPOP_IREQUEST,               "inverse request"        },
  {ARPOP_IREPLY,                 "inverse reply"          },
  {ATMARPOP_NAK,                 "nak"                    },
  {ARPOP_MARS_REQUEST,           "mars request"           },
  {ARPOP_MARS_MULTI,             "mars multi"             },
  {ARPOP_MARS_MSERV,             "mars mserv"             },
  {ARPOP_MARS_JOIN,              "mars join"              },
  {ARPOP_MARS_LEAVE,             "mars leave"             },
  {ARPOP_MARS_NAK,               "mars nak"               },
  {ARPOP_MARS_UNSERV,            "mars unserv"            },
  {ARPOP_MARS_SJOIN,             "mars sjoin"             },
  {ARPOP_MARS_SLEAVE,            "mars sleave"            },
  {ARPOP_MARS_GROUPLIST_REQUEST, "mars grouplist request" },
  {ARPOP_MARS_GROUPLIST_REPLY,   "mars gruoplist reply"   },
  {ARPOP_MARS_REDIRECT_MAP,      "mars redirect map"      },
  {ARPOP_MAPOS_UNARP,            "mapos unarp"            },
  {ARPOP_EXP1,                   "experimental 1"         },
  {ARPOP_EXP2,                   "experimental 2"         },
  {ARPOP_RESERVED1,              "reserved"               },
  {ARPOP_RESERVED2,              "reserved"               },
  {0, NULL} };

#define ATMARP_IS_E164  0x40    /* bit in type/length for E.164 format */
#define ATMARP_LEN_MASK 0x3F    /* length of {sub}address in type/length */

/*
 * Given the hardware address type and length, check whether an address
 * is an Ethernet address - the address must be of type "Ethernet" or
 * "IEEE 802.x", and the length must be 6 bytes.
 */
#define ARP_HW_IS_ETHER(ar_hrd, ar_hln)                         \
  (((ar_hrd) == ARPHRD_ETHER || (ar_hrd) == ARPHRD_IEEE802)     \
   && (ar_hln) == 6)

/*
* Given the hardware address type and length, check whether an address
* is an AX.25 address - the address must be of type "AX.25" and the
* length must be 7 bytes.
*/
#define ARP_HW_IS_AX25(ar_hrd, ar_hln) \
  ((ar_hrd) == ARPHRD_AX25 && (ar_hln) == 7)

/*
 * Given the protocol address type and length, check whether an address
 * is an IPv4 address - the address must be of type "IP", and the length
 * must be 4 bytes.
 */
#define ARP_PRO_IS_IPv4(ar_pro, ar_pln)         \
  (((ar_pro) == ETHERTYPE_IP || (ar_pro) == AX25_P_IP) && (ar_pln) == 4)

const gchar *
tvb_arphrdaddr_to_str(tvbuff_t *tvb, gint offset, int ad_len, guint16 type)
{
  if (ad_len == 0)
    return "<No address>";
  if (ARP_HW_IS_ETHER(type, ad_len)) {
    /* Ethernet address (or IEEE 802.x address, which is the same type of
       address). */
    return tvb_ether_to_str(tvb, offset);
  }
  return tvb_bytes_to_str(wmem_packet_scope(), tvb, offset, ad_len);
}

static const gchar *
arpproaddr_to_str(const guint8 *ad, int ad_len, guint16 type)
{
  address addr;

  if (ad_len == 0)
    return "<No address>";
  if (ARP_PRO_IS_IPv4(type, ad_len)) {
    /* IPv4 address.  */
    set_address(&addr, AT_IPv4, 4, ad);

    return address_to_str(wmem_packet_scope(), &addr);
  }
  if (ARP_HW_IS_AX25(type, ad_len)) {
    {
    /* AX.25 address */
    set_address(&addr, AT_AX25, AX25_ADDR_LEN, ad);

    return address_to_str(wmem_packet_scope(), &addr);
    }
  }
  return bytes_to_str(wmem_packet_scope(), ad, ad_len);
}

static const gchar *
tvb_arpproaddr_to_str(tvbuff_t *tvb, gint offset, int ad_len, guint16 type)
{
    return arpproaddr_to_str(tvb_get_ptr(tvb, offset, ad_len), ad_len, type);
}

#define MAX_E164_STR_LEN                20

static const gchar *
atmarpnum_to_str(tvbuff_t *tvb, int offset, int ad_tl)
{
  int    ad_len = ad_tl & ATMARP_LEN_MASK;
  gchar *cur;

  if (ad_len == 0)
    return "<No address>";

  if (ad_tl & ATMARP_IS_E164) {
    /*
     * I'm assuming this means it's an ASCII (IA5) string.
     */
    cur = (gchar *)wmem_alloc(wmem_packet_scope(), MAX_E164_STR_LEN+3+1);
    if (ad_len > MAX_E164_STR_LEN) {
      /* Can't show it all. */
      tvb_memcpy(tvb, cur, offset, MAX_E164_STR_LEN);
      g_snprintf(&cur[MAX_E164_STR_LEN], 3+1, "...");
    } else {
      tvb_memcpy(tvb, cur, offset, ad_len);
      cur[ad_len + 1] = '\0';
    }
    return cur;
  } else {
    /*
     * NSAP.
     *
     * XXX - break down into subcomponents.
     */
    return tvb_bytes_to_str(wmem_packet_scope(), tvb, offset, ad_len);
  }
}

static const gchar *
atmarpsubaddr_to_str(tvbuff_t *tvb, int offset, int ad_tl)
{
  int ad_len = ad_tl & ATMARP_LEN_MASK;

  if (ad_len == 0)
    return "<No address>";

  /*
   * E.164 isn't considered legal in subaddresses (RFC 2225 says that
   * a null or unknown ATM address is indicated by setting the length
   * to 0, in which case the type must be ignored; we've seen some
   * captures in which the length of a subaddress is 0 and the type
   * is E.164).
   *
   * XXX - break down into subcomponents?
   */
  return tvb_bytes_to_str(wmem_packet_scope(), tvb, offset, ad_len);
}

const value_string arp_hrd_vals[] = {
  {ARPHRD_NETROM,             "NET/ROM pseudo"             },
  {ARPHRD_ETHER,              "Ethernet"                   },
  {ARPHRD_EETHER,             "Experimental Ethernet"      },
  {ARPHRD_AX25,               "AX.25"                      },
  {ARPHRD_PRONET,             "ProNET"                     },
  {ARPHRD_CHAOS,              "Chaos"                      },
  {ARPHRD_IEEE802,            "IEEE 802"                   },
  {ARPHRD_ARCNET,             "ARCNET"                     },
  {ARPHRD_HYPERCH,            "Hyperchannel"               },
  {ARPHRD_LANSTAR,            "Lanstar"                    },
  {ARPHRD_AUTONET,            "Autonet Short Address"      },
  {ARPHRD_LOCALTLK,           "Localtalk"                  },
  {ARPHRD_LOCALNET,           "LocalNet"                   },
  {ARPHRD_ULTRALNK,           "Ultra link"                 },
  {ARPHRD_SMDS,               "SMDS"                       },
  {ARPHRD_DLCI,               "Frame Relay DLCI"           },
  {ARPHRD_ATM,                "ATM"                        },
  {ARPHRD_HDLC,               "HDLC"                       },
  {ARPHRD_FIBREC,             "Fibre Channel"              },
  {ARPHRD_ATM2225,            "ATM (RFC 2225)"             },
  {ARPHRD_SERIAL,             "Serial Line"                },
  {ARPHRD_ATM2,               "ATM"                        },
  {ARPHRD_MS188220,           "MIL-STD-188-220"            },
  {ARPHRD_METRICOM,           "Metricom STRIP"             },
  {ARPHRD_IEEE1394,           "IEEE 1394.1995"             },
  {ARPHRD_MAPOS,              "MAPOS"                      },
  {ARPHRD_TWINAX,             "Twinaxial"                  },
  {ARPHRD_EUI_64,             "EUI-64"                     },
  {ARPHRD_HIPARP,             "HIPARP"                     },
  {ARPHRD_IP_ARP_ISO_7816_3,  "IP and ARP over ISO 7816-3" },
  {ARPHRD_ARPSEC,             "ARPSec"                     },
  {ARPHRD_IPSEC_TUNNEL,       "IPsec tunnel"               },
  {ARPHRD_INFINIBAND,         "InfiniBand"                 },
  {ARPHRD_TIA_102_PRJ_25_CAI, "TIA-102 Project 25 CAI"     },
  {ARPHRD_WIEGAND_INTERFACE,  "Wiegand Interface"          },
  {ARPHRD_PURE_IP,            "Pure IP"                    },
  {ARPHDR_HW_EXP1,            "Experimental 1"             },
  {ARPHDR_HFI,                "HFI"                        },
  {ARPHDR_HW_EXP2,            "Experimental 2"             },
  {0, NULL                  } };

/* Offsets of fields within an ARP packet. */
#define AR_HRD          0
#define AR_PRO          2
#define AR_HLN          4
#define AR_PLN          5
#define AR_OP           6
#define MIN_ARP_HEADER_SIZE     8

/* Offsets of fields within an ATMARP packet. */
#define ATM_AR_HRD       0
#define ATM_AR_PRO       2
#define ATM_AR_SHTL      4
#define ATM_AR_SSTL      5
#define ATM_AR_OP        6
#define ATM_AR_SPLN      8
#define ATM_AR_THTL      9
#define ATM_AR_TSTL     10
#define ATM_AR_TPLN     11
#define MIN_ATMARP_HEADER_SIZE  12

static void
dissect_atm_number(tvbuff_t *tvb, packet_info* pinfo, int offset, int tl, int hf_e164,
                   int hf_nsap, proto_tree *tree)
{
  int         len = tl & ATMARP_LEN_MASK;
  proto_item *ti;
  proto_tree *nsap_tree;

  if (tl & ATMARP_IS_E164)
    proto_tree_add_item(tree, hf_e164, tvb, offset, len, ENC_BIG_ENDIAN);
  else {
    ti = proto_tree_add_item(tree, hf_nsap, tvb, offset, len, ENC_BIG_ENDIAN);
    if (len >= 20) {
      nsap_tree = proto_item_add_subtree(ti, ett_atmarp_nsap);
      dissect_atm_nsap(tvb, pinfo, offset, len, nsap_tree);
    }
  }
}

static const value_string atm_nsap_afi_vals[] = {
    { 0x39,    "DCC ATM format"},
    { 0xBD,    "DCC ATM group format"},
    { 0x47,    "ICD ATM format"},
    { 0xC5,    "ICD ATM group format"},
    { 0x45,    "E.164 ATM format"},
    { 0xC3,    "E.164 ATM group format"},
    { 0,            NULL}
};

/*
 * XXX - shouldn't there be a centralized routine for dissecting NSAPs?
 * See also "dissect_nsap()" in epan/dissectors/packet-isup.c and
 * "print_nsap_net_buf()" and "print_nsap_net()" in epan/osi=utils.c.
 */
void
dissect_atm_nsap(tvbuff_t *tvb, packet_info* pinfo, int offset, int len, proto_tree *tree)
{
  guint8 afi;
  proto_item* ti;

  afi = tvb_get_guint8(tvb, offset);
  ti = proto_tree_add_item(tree, hf_atmarp_src_atm_afi, tvb, offset, 1, ENC_BIG_ENDIAN);
  switch (afi) {

    case 0x39:  /* DCC ATM format */
    case 0xBD:  /* DCC ATM group format */
      proto_tree_add_item(tree, (afi == 0xBD) ? hf_atmarp_src_atm_data_country_code_group : hf_atmarp_src_atm_data_country_code,
                          tvb, offset + 1, 2, ENC_BIG_ENDIAN);
      proto_tree_add_item(tree, hf_atmarp_src_atm_high_order_dsp, tvb, offset + 3, 10, ENC_NA);
      proto_tree_add_item(tree, hf_atmarp_src_atm_end_system_identifier, tvb, offset + 13, 6, ENC_NA);
      proto_tree_add_item(tree, hf_atmarp_src_atm_selector, tvb, offset + 19, 1, ENC_BIG_ENDIAN);
      break;

    case 0x47:  /* ICD ATM format */
    case 0xC5:  /* ICD ATM group format */
      proto_tree_add_item(tree, (afi == 0xC5) ? hf_atmarp_src_atm_international_code_designator_group : hf_atmarp_src_atm_international_code_designator,
                          tvb, offset + 1, 2, ENC_BIG_ENDIAN);
      proto_tree_add_item(tree, hf_atmarp_src_atm_high_order_dsp, tvb, offset + 3, 10, ENC_NA);
      proto_tree_add_item(tree, hf_atmarp_src_atm_end_system_identifier, tvb, offset + 13, 6, ENC_NA);
      proto_tree_add_item(tree, hf_atmarp_src_atm_selector, tvb, offset + 19, 1, ENC_BIG_ENDIAN);
      break;

    case 0x45:  /* E.164 ATM format */
    case 0xC3:  /* E.164 ATM group format */
      proto_tree_add_item(tree, (afi == 0xC3) ? hf_atmarp_src_atm_e_164_isdn_group : hf_atmarp_src_atm_e_164_isdn,
                          tvb, offset + 1, 8, ENC_NA);
      proto_tree_add_item(tree, hf_atmarp_src_atm_high_order_dsp, tvb, offset + 9, 4, ENC_NA);
      proto_tree_add_item(tree, hf_atmarp_src_atm_end_system_identifier, tvb, offset + 13, 6, ENC_NA);
      proto_tree_add_item(tree, hf_atmarp_src_atm_selector, tvb, offset + 19, 1, ENC_BIG_ENDIAN);
      break;

    default:
      expert_add_info(pinfo, ti, &ei_atmarp_src_atm_unknown_afi);
      proto_tree_add_item(tree, hf_atmarp_src_atm_rest_of_address, tvb, offset + 1, len - 1, ENC_NA);
      break;
  }
}

/* l.s. 32 bits are ipv4 address */
static guint
address_hash_func(gconstpointer v)
{
  return GPOINTER_TO_UINT(v);
}

/* Compare 2 ipv4 addresses */
static gint
address_equal_func(gconstpointer v, gconstpointer v2)
{
  return v == v2;
}

static guint
duplicate_result_hash_func(gconstpointer v)
{
  const duplicate_result_key *key = (const duplicate_result_key*)v;
  return (key->frame_number + key->ip_address);
}

static gint
duplicate_result_equal_func(gconstpointer v, gconstpointer v2)
{
  const duplicate_result_key *key1 = (const duplicate_result_key*)v;
  const duplicate_result_key *key2 = (const duplicate_result_key*)v2;

  return (memcmp(key1, key2, sizeof(duplicate_result_key)) == 0);
}




/* Check to see if this mac & ip pair represent 2 devices trying to share
   the same IP address - report if found (+ return TRUE and set out param) */
static gboolean
check_for_duplicate_addresses(packet_info *pinfo, proto_tree *tree,
                                              tvbuff_t *tvb,
                                              const guint8 *mac, guint32 ip,
                                              guint32 *duplicate_ip)
{
  address_hash_value   *value;
  address_hash_value   *result     = NULL;
  duplicate_result_key  result_key = {pinfo->num, ip};

  /* Look up existing result */
  if (pinfo->fd->flags.visited) {
      result = (address_hash_value *)g_hash_table_lookup(duplicate_result_hash_table,
                                   &result_key);
  }
  else {
      /* First time around, need to work out if represents duplicate and
         store result */

      /* Look up current assignment of IP address */
      value = (address_hash_value *)g_hash_table_lookup(address_hash_table, GUINT_TO_POINTER(ip));

      /* If MAC matches table, just update details */
      if (value != NULL)
      {
        if (pinfo->num > value->frame_num)
        {
          if ((memcmp(value->mac, mac, 6) == 0))
          {
            /* Same MAC as before - update existing entry */
            value->frame_num = pinfo->num;
            value->time_of_entry = pinfo->abs_ts.secs;
          }
          else
          {
            /* Create result and store in result table */
            duplicate_result_key *persistent_key = wmem_new(wmem_file_scope(), duplicate_result_key);
            memcpy(persistent_key, &result_key, sizeof(duplicate_result_key));

            result = wmem_new(wmem_file_scope(), address_hash_value);
            memcpy(result, value, sizeof(address_hash_value));

            g_hash_table_insert(duplicate_result_hash_table, persistent_key, result);
          }
        }
      }
      else
      {
        /* No existing entry. Prepare one */
        value = wmem_new(wmem_file_scope(), struct address_hash_value);
        memcpy(value->mac, mac, 6);
        value->frame_num = pinfo->num;
        value->time_of_entry = pinfo->abs_ts.secs;

        /* Add it */
        g_hash_table_insert(address_hash_table, GUINT_TO_POINTER(ip), value);
      }
  }

  /* Add report to tree if we found a duplicate */
  if (result != NULL) {
    proto_tree *duplicate_tree;
    proto_item *ti;
    address mac_addr, result_mac_addr;

    set_address(&mac_addr, AT_ETHER, 6, mac);
    set_address(&result_mac_addr, AT_ETHER, 6, result->mac);

    /* Create subtree */
    duplicate_tree = proto_tree_add_subtree_format(tree, tvb, 0, 0, ett_arp_duplicate_address, &ti,
                                                "Duplicate IP address detected for %s (%s) - also in use by %s (frame %u)",
                                                arpproaddr_to_str((guint8*)&ip, 4, ETHERTYPE_IP),
                                                address_to_str(wmem_packet_scope(), &mac_addr),
                                                address_to_str(wmem_packet_scope(), &result_mac_addr),
                                                result->frame_num);
    PROTO_ITEM_SET_GENERATED(ti);

    /* Add item for navigating to earlier frame */
    ti = proto_tree_add_uint(duplicate_tree, hf_arp_duplicate_ip_address_earlier_frame,
                             tvb, 0, 0, result->frame_num);
    PROTO_ITEM_SET_GENERATED(ti);
    expert_add_info_format(pinfo, ti,
                           &ei_seq_arp_dup_ip,
                           "Duplicate IP address configured (%s)",
                           arpproaddr_to_str((guint8*)&ip, 4, ETHERTYPE_IP));

    /* Time since that frame was seen */
    ti = proto_tree_add_uint(duplicate_tree,
                             hf_arp_duplicate_ip_address_seconds_since_earlier_frame,
                             tvb, 0, 0,
                             (guint32)(pinfo->abs_ts.secs - result->time_of_entry));
    PROTO_ITEM_SET_GENERATED(ti);

    /* Set out parameter */
    *duplicate_ip = ip;
  }


  return (result != NULL);
}



/* Initializes the hash table each time a new
 * file is loaded or re-loaded in wireshark */
static void
arp_init_protocol(void)
{
  address_hash_table = g_hash_table_new(address_hash_func, address_equal_func);
  duplicate_result_hash_table = g_hash_table_new(duplicate_result_hash_func,
                                                 duplicate_result_equal_func);
}

static void
arp_cleanup_protocol(void)
{
  g_hash_table_destroy(address_hash_table);
  g_hash_table_destroy(duplicate_result_hash_table);
}




/* Take note that a request has been seen */
static void
request_seen(packet_info *pinfo)
{
  /* Don't count frame again after already recording first time around. */
  if (p_get_proto_data(wmem_file_scope(), pinfo, proto_arp, 0) == 0)
  {
    arp_request_count++;
  }
}

/* Has storm request rate been exceeded with this request? */
static void
check_for_storm_count(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  gboolean report_storm = FALSE;

  if (p_get_proto_data(wmem_file_scope(), pinfo, proto_arp, 0) != 0)
  {
    /* Read any previous stored packet setting */
    report_storm = (p_get_proto_data(wmem_file_scope(), pinfo, proto_arp, 0) == (void*)STORM);
  }
  else
  {
    /* Seeing packet for first time - check against preference settings */
    gint seconds_delta  = (gint) (pinfo->abs_ts.secs - time_at_start_of_count.secs);
    gint nseconds_delta = pinfo->abs_ts.nsecs - time_at_start_of_count.nsecs;
    gint gap = (seconds_delta*1000) + (nseconds_delta / 1000000);

    /* Reset if gap exceeds period or -ve gap (indicates we're rescanning from start) */
    if ((gap > (gint)global_arp_detect_request_storm_period) ||
        (gap < 0))
    {
      /* Time period elapsed without threshold being exceeded */
      arp_request_count = 1;
      time_at_start_of_count = pinfo->abs_ts;
      p_add_proto_data(wmem_file_scope(), pinfo, proto_arp, 0, (void*)NO_STORM);
      return;
    }
    else
      if (arp_request_count > global_arp_detect_request_storm_packets)
      {
        /* Storm detected, record and reset start time. */
        report_storm = TRUE;
        p_add_proto_data(wmem_file_scope(), pinfo, proto_arp, 0, (void*)STORM);
        time_at_start_of_count = pinfo->abs_ts;
      }
      else
      {
        /* Threshold not exceeded yet - no storm */
        p_add_proto_data(wmem_file_scope(), pinfo, proto_arp, 0, (void*)NO_STORM);
      }
  }

  if (report_storm)
  {
    /* Report storm and reset counter */
    proto_tree_add_expert_format(tree, pinfo, &ei_seq_arp_storm, tvb, 0, 0,
                           "ARP packet storm detected (%u packets in < %u ms)",
                           global_arp_detect_request_storm_packets,
                           global_arp_detect_request_storm_period);
    arp_request_count = 0;
  }
}


/*
 * RFC 2225 ATMARP - it's just like ARP, except where it isn't.
 */
static int
dissect_atmarp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
  guint16       ar_hrd;
  guint16       ar_pro;
  guint8        ar_shtl;
  guint8        ar_shl;
  guint8        ar_sstl;
  guint8        ar_ssl;
  guint16       ar_op;
  guint8        ar_spln;
  guint8        ar_thtl;
  guint8        ar_thl;
  guint8        ar_tstl;
  guint8        ar_tsl;
  guint8        ar_tpln;
  int           tot_len;
  proto_tree   *arp_tree;
  proto_item   *ti;
  const gchar  *op_str;
  int           sha_offset, ssa_offset, spa_offset;
  int           tha_offset, tsa_offset, tpa_offset;
  const gchar  *sha_str, *ssa_str, *spa_str;
  const gchar  *tha_str, *tsa_str, *tpa_str;
  proto_tree   *tl_tree;

  ar_hrd = tvb_get_ntohs(tvb, ATM_AR_HRD);
  ar_pro = tvb_get_ntohs(tvb, ATM_AR_PRO);
  ar_shtl = tvb_get_guint8(tvb, ATM_AR_SHTL);
  ar_shl = ar_shtl & ATMARP_LEN_MASK;
  ar_sstl = tvb_get_guint8(tvb, ATM_AR_SSTL);
  ar_ssl = ar_sstl & ATMARP_LEN_MASK;
  ar_op  = tvb_get_ntohs(tvb, AR_OP);
  ar_spln = tvb_get_guint8(tvb, ATM_AR_SPLN);
  ar_thtl = tvb_get_guint8(tvb, ATM_AR_THTL);
  ar_thl = ar_thtl & ATMARP_LEN_MASK;
  ar_tstl = tvb_get_guint8(tvb, ATM_AR_TSTL);
  ar_tsl = ar_tstl & ATMARP_LEN_MASK;
  ar_tpln = tvb_get_guint8(tvb, ATM_AR_TPLN);

  tot_len = MIN_ATMARP_HEADER_SIZE + ar_shl + ar_ssl + ar_spln +
    ar_thl + ar_tsl + ar_tpln;

  /* Adjust the length of this tvbuff to include only the ARP datagram.
     Our caller may use that to determine how much of its packet
     was padding. */
  tvb_set_reported_length(tvb, tot_len);

  /* Extract the addresses.  */
  sha_offset = MIN_ATMARP_HEADER_SIZE;
  sha_str = atmarpnum_to_str(tvb, sha_offset, ar_shtl);

  ssa_offset = sha_offset + ar_shl;
  if (ar_ssl != 0) {
    ssa_str = atmarpsubaddr_to_str(tvb, ssa_offset, ar_sstl);
  } else {
    ssa_str = NULL;
  }

  spa_offset = ssa_offset + ar_ssl;
  spa_str = tvb_arpproaddr_to_str(tvb, spa_offset, ar_spln, ar_pro);

  tha_offset = spa_offset + ar_spln;
  tha_str = atmarpnum_to_str(tvb, tha_offset, ar_thtl);

  tsa_offset = tha_offset + ar_thl;
  if (ar_tsl != 0) {
    tsa_str = atmarpsubaddr_to_str(tvb, tsa_offset, ar_tstl);
  } else {
    tsa_str = NULL;
  }

  tpa_offset = tsa_offset + ar_tsl;
  tpa_str = tvb_arpproaddr_to_str(tvb, tpa_offset, ar_tpln, ar_pro);

  switch (ar_op) {

  case ARPOP_REQUEST:
  case ARPOP_REPLY:
  case ATMARPOP_NAK:
  default:
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "ATMARP");
    break;

  case ARPOP_RREQUEST:
  case ARPOP_RREPLY:
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "ATMRARP");
    break;

  case ARPOP_IREQUEST:
  case ARPOP_IREPLY:
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "Inverse ATMARP");
    break;

  case ARPOP_MARS_REQUEST:
  case ARPOP_MARS_MULTI:
  case ARPOP_MARS_MSERV:
  case ARPOP_MARS_JOIN:
  case ARPOP_MARS_LEAVE:
  case ARPOP_MARS_NAK:
  case ARPOP_MARS_UNSERV:
  case ARPOP_MARS_SJOIN:
  case ARPOP_MARS_SLEAVE:
  case ARPOP_MARS_GROUPLIST_REQUEST:
  case ARPOP_MARS_GROUPLIST_REPLY:
  case ARPOP_MARS_REDIRECT_MAP:
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "MARS");
    break;

  case ARPOP_MAPOS_UNARP:
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "MAPOS");
    break;

  }

  switch (ar_op) {
  case ARPOP_REQUEST:
    col_add_fstr(pinfo->cinfo, COL_INFO, "Who has %s? Tell %s",
                 tpa_str, spa_str);
    break;
  case ARPOP_REPLY:
    col_add_fstr(pinfo->cinfo, COL_INFO, "%s is at %s%s%s", spa_str, sha_str,
                 ((ssa_str != NULL) ? "," : ""),
                 ((ssa_str != NULL) ? ssa_str : ""));
    break;
  case ARPOP_IREQUEST:
    col_add_fstr(pinfo->cinfo, COL_INFO, "Who is %s%s%s? Tell %s%s%s",
                 tha_str,
                 ((tsa_str != NULL) ? "," : ""),
                 ((tsa_str != NULL) ? tsa_str : ""),
                 sha_str,
                 ((ssa_str != NULL) ? "," : ""),
                 ((ssa_str != NULL) ? ssa_str : ""));
    break;
  case ARPOP_IREPLY:
    col_add_fstr(pinfo->cinfo, COL_INFO, "%s%s%s is at %s",
                 sha_str,
                 ((ssa_str != NULL) ? "," : ""),
                 ((ssa_str != NULL) ? ssa_str : ""),
                 spa_str);
    break;
  case ATMARPOP_NAK:
    col_add_fstr(pinfo->cinfo, COL_INFO, "I don't know where %s is", spa_str);
    break;
  case ARPOP_MARS_REQUEST:
    col_add_fstr(pinfo->cinfo, COL_INFO, "MARS request from %s%s%s at %s",
                 sha_str,
                 ((ssa_str != NULL) ? "," : ""),
                 ((ssa_str != NULL) ? ssa_str : ""),
                 spa_str);
    break;

  case ARPOP_MARS_MULTI:
    col_add_fstr(pinfo->cinfo, COL_INFO, "MARS MULTI request from %s%s%s at %s",
                 sha_str,
                 ((ssa_str != NULL) ? "," : ""),
                 ((ssa_str != NULL) ? ssa_str : ""),
                 spa_str);
    break;

  case ARPOP_MARS_MSERV:
    col_add_fstr(pinfo->cinfo, COL_INFO, "MARS MSERV request from %s%s%s at %s",
                 sha_str,
                 ((ssa_str != NULL) ? "," : ""),
                 ((ssa_str != NULL) ? ssa_str : ""),
                 spa_str);
    break;

  case ARPOP_MARS_JOIN:
    col_add_fstr(pinfo->cinfo, COL_INFO, "MARS JOIN request from %s%s%s at %s",
                 sha_str,
                 ((ssa_str != NULL) ? "," : ""),
                 ((ssa_str != NULL) ? ssa_str : ""),
                 spa_str);
    break;

  case ARPOP_MARS_LEAVE:
    col_add_fstr(pinfo->cinfo, COL_INFO, "MARS LEAVE from %s%s%s at %s",
                 sha_str,
                 ((ssa_str != NULL) ? "," : ""),
                 ((ssa_str != NULL) ? ssa_str : ""),
                 spa_str);
    break;

  case ARPOP_MARS_NAK:
    col_add_fstr(pinfo->cinfo, COL_INFO, "MARS NAK from %s%s%s at %s",
                 sha_str,
                 ((ssa_str != NULL) ? "," : ""),
                 ((ssa_str != NULL) ? ssa_str : ""),
                 spa_str);
    break;

  case ARPOP_MARS_UNSERV:
    col_add_fstr(pinfo->cinfo, COL_INFO, "MARS UNSERV request from %s%s%s at %s",
                 sha_str,
                 ((ssa_str != NULL) ? "," : ""),
                 ((ssa_str != NULL) ? ssa_str : ""),
                 spa_str);
    break;

  case ARPOP_MARS_SJOIN:
    col_add_fstr(pinfo->cinfo, COL_INFO, "MARS SJOIN request from %s%s%s at %s",
                 sha_str,
                 ((ssa_str != NULL) ? "," : ""),
                 ((ssa_str != NULL) ? ssa_str : ""),
                 spa_str);
    break;

  case ARPOP_MARS_SLEAVE:
    col_add_fstr(pinfo->cinfo, COL_INFO, "MARS SLEAVE from %s%s%s at %s",
                 sha_str,
                 ((ssa_str != NULL) ? "," : ""),
                 ((ssa_str != NULL) ? ssa_str : ""),
                 spa_str);
    break;

  case ARPOP_MARS_GROUPLIST_REQUEST:
    col_add_fstr(pinfo->cinfo, COL_INFO, "MARS grouplist request from %s%s%s at %s",
                 sha_str,
                 ((ssa_str != NULL) ? "," : ""),
                 ((ssa_str != NULL) ? ssa_str : ""),
                 spa_str);
    break;

  case ARPOP_MARS_GROUPLIST_REPLY:
    col_add_fstr(pinfo->cinfo, COL_INFO, "MARS grouplist reply from %s%s%s at %s",
                 sha_str,
                 ((ssa_str != NULL) ? "," : ""),
                 ((ssa_str != NULL) ? ssa_str : ""),
                 spa_str);
    break;

  case ARPOP_MARS_REDIRECT_MAP:
    col_add_fstr(pinfo->cinfo, COL_INFO, "MARS redirect map from %s%s%s at %s",
                 sha_str,
                 ((ssa_str != NULL) ? "," : ""),
                 ((ssa_str != NULL) ? ssa_str : ""),
                 spa_str);
    break;

  case ARPOP_MAPOS_UNARP:
    col_add_fstr(pinfo->cinfo, COL_INFO, "MAPOS UNARP request from %s%s%s at %s",
                 sha_str,
                 ((ssa_str != NULL) ? "," : ""),
                 ((ssa_str != NULL) ? ssa_str : ""),
                 spa_str);
    break;

  case ARPOP_EXP1:
    col_add_fstr(pinfo->cinfo, COL_INFO, "Experimental 1 ( opcode %d )", ar_op);
    break;

  case ARPOP_EXP2:
    col_add_fstr(pinfo->cinfo, COL_INFO, "Experimental 2 ( opcode %d )", ar_op);
    break;

  case 0:
  case 65535:
    col_add_fstr(pinfo->cinfo, COL_INFO, "Reserved opcode %d", ar_op);
    break;

  default:
    col_add_fstr(pinfo->cinfo, COL_INFO, "Unknown ATMARP opcode 0x%04x", ar_op);
    break;
  }

  if (tree) {
    if ((op_str = try_val_to_str(ar_op, atmop_vals)))
      ti = proto_tree_add_protocol_format(tree, proto_arp, tvb, 0, tot_len,
                                          "ATM Address Resolution Protocol (%s)",
                                          op_str);
    else
      ti = proto_tree_add_protocol_format(tree, proto_arp, tvb, 0, tot_len,
                                          "ATM Address Resolution Protocol (opcode 0x%04x)", ar_op);
    arp_tree = proto_item_add_subtree(ti, ett_arp);

    proto_tree_add_uint(arp_tree, hf_arp_hard_type, tvb, ATM_AR_HRD, 2, ar_hrd);

    proto_tree_add_uint(arp_tree, hf_arp_proto_type, tvb, ATM_AR_PRO, 2,ar_pro);

    tl_tree = proto_tree_add_subtree_format(arp_tree, tvb, ATM_AR_SHTL, 1,
                             ett_atmarp_tl, NULL,
                             "Sender ATM number type/length: %s/%u",
                             (ar_shtl & ATMARP_IS_E164 ?
                              "E.164" :
                              "ATM Forum NSAPA"),
                             ar_shl);
    proto_tree_add_boolean(tl_tree, hf_atmarp_sht, tvb, ATM_AR_SHTL, 1, ar_shtl);
    proto_tree_add_uint(tl_tree, hf_atmarp_shl, tvb, ATM_AR_SHTL, 1, ar_shtl);

    tl_tree = proto_tree_add_subtree_format(arp_tree, tvb, ATM_AR_SSTL, 1,
                             ett_atmarp_tl, NULL,
                             "Sender ATM subaddress type/length: %s/%u",
                             (ar_sstl & ATMARP_IS_E164 ?
                              "E.164" :
                              "ATM Forum NSAPA"),
                             ar_ssl);
    proto_tree_add_boolean(tl_tree, hf_atmarp_sst, tvb, ATM_AR_SSTL, 1, ar_sstl);
    proto_tree_add_uint(tl_tree, hf_atmarp_ssl, tvb, ATM_AR_SSTL, 1, ar_sstl);

    proto_tree_add_uint(arp_tree, hf_arp_opcode, tvb, AR_OP,  2, ar_op);


    proto_tree_add_uint(arp_tree, hf_atmarp_spln, tvb, ATM_AR_SPLN, 1, ar_spln);

    tl_tree = proto_tree_add_subtree_format(arp_tree, tvb, ATM_AR_THTL, 1,
                             ett_atmarp_tl, NULL,
                             "Target ATM number type/length: %s/%u",
                             (ar_thtl & ATMARP_IS_E164 ?
                              "E.164" :
                              "ATM Forum NSAPA"),
                             ar_thl);
    proto_tree_add_boolean(tl_tree, hf_atmarp_tht, tvb, ATM_AR_THTL, 1, ar_thtl);
    proto_tree_add_uint(tl_tree, hf_atmarp_thl, tvb, ATM_AR_THTL, 1, ar_thtl);

    tl_tree = proto_tree_add_subtree_format(arp_tree, tvb, ATM_AR_TSTL, 1,
                             ett_atmarp_tl, NULL,
                             "Target ATM subaddress type/length: %s/%u",
                             (ar_tstl & ATMARP_IS_E164 ?
                              "E.164" :
                              "ATM Forum NSAPA"),
                             ar_tsl);
    proto_tree_add_boolean(tl_tree, hf_atmarp_tst, tvb, ATM_AR_TSTL, 1, ar_tstl);
    proto_tree_add_uint(tl_tree, hf_atmarp_tsl, tvb, ATM_AR_TSTL, 1, ar_tstl);

    proto_tree_add_uint(arp_tree, hf_atmarp_tpln, tvb, ATM_AR_TPLN, 1, ar_tpln);

    if (ar_shl != 0)
      dissect_atm_number(tvb, pinfo, sha_offset, ar_shtl, hf_atmarp_src_atm_num_e164,
                         hf_atmarp_src_atm_num_nsap, arp_tree);

    if (ar_ssl != 0)
      proto_tree_add_bytes_format_value(arp_tree, hf_atmarp_src_atm_subaddr, tvb, ssa_offset,
                                  ar_ssl, NULL, "%s", ssa_str);

    if (ar_spln != 0) {
      proto_tree_add_item(arp_tree,
                          ARP_PRO_IS_IPv4(ar_pro, ar_spln) ? hf_arp_src_proto_ipv4
                          : hf_arp_src_proto,
                          tvb, spa_offset, ar_spln, ENC_BIG_ENDIAN);
    }

    if (ar_thl != 0)
      dissect_atm_number(tvb, pinfo, tha_offset, ar_thtl, hf_atmarp_dst_atm_num_e164,
                         hf_atmarp_dst_atm_num_nsap, arp_tree);

    if (ar_tsl != 0)
      proto_tree_add_bytes_format_value(arp_tree, hf_atmarp_dst_atm_subaddr, tvb, tsa_offset,
                                  ar_tsl, NULL, "%s", tsa_str);

    if (ar_tpln != 0) {
      proto_tree_add_item(arp_tree,
                          ARP_PRO_IS_IPv4(ar_pro, ar_tpln) ? hf_arp_dst_proto_ipv4
                          : hf_arp_dst_proto,
                          tvb, tpa_offset, ar_tpln, ENC_BIG_ENDIAN);
    }
  }
  return tvb_captured_length(tvb);
}

/*
 * AX.25 ARP - it's just like ARP, except where it isn't.
 */
static int
dissect_ax25arp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
#define ARP_AX25 204

  guint16      ar_hrd;
  guint16      ar_pro;
  guint8       ar_hln;
  guint8       ar_pln;
  guint16      ar_op;
  int          tot_len;
  proto_tree  *arp_tree = NULL;
  proto_item  *ti;
  const gchar *op_str;
  int          sha_offset, spa_offset, tha_offset, tpa_offset;
  const gchar *spa_str, *tpa_str;
  gboolean     is_gratuitous;

  /* Hardware Address Type */
  ar_hrd = tvb_get_ntohs(tvb, AR_HRD);
  /* Protocol Address Type */
  ar_pro = tvb_get_ntohs(tvb, AR_PRO);
  /* Hardware Address Size */
  ar_hln = tvb_get_guint8(tvb, AR_HLN);
  /* Protocol Address Size */
  ar_pln = tvb_get_guint8(tvb, AR_PLN);
  /* Operation */
  ar_op  = tvb_get_ntohs(tvb, AR_OP);

  tot_len = MIN_ARP_HEADER_SIZE + ar_hln*2 + ar_pln*2;

  /* Adjust the length of this tvbuff to include only the ARP datagram.
     Our caller may use that to determine how much of its packet
     was padding. */
  tvb_set_reported_length(tvb, tot_len);

  switch (ar_op) {

  case ARPOP_REQUEST:
    if (global_arp_detect_request_storm)
      request_seen(pinfo);
      /* fall-through */
  case ARPOP_REPLY:
  default:
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "ARP");
    break;

  case ARPOP_RREQUEST:
  case ARPOP_RREPLY:
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "RARP");
    break;

  case ARPOP_IREQUEST:
  case ARPOP_IREPLY:
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "Inverse ARP");
    break;
  }

  /* Get the offsets of the addresses. */
  /* Source Hardware Address */
  sha_offset = MIN_ARP_HEADER_SIZE;
  /* Source Protocol Address */
  spa_offset = sha_offset + ar_hln;
  /* Target Hardware Address */
  tha_offset = spa_offset + ar_pln;
  /* Target Protocol Address */
  tpa_offset = tha_offset + ar_hln;

  spa_str = tvb_arpproaddr_to_str(tvb, spa_offset, ar_pln, ar_pro);
  tpa_str = tvb_arpproaddr_to_str(tvb, tpa_offset, ar_pln, ar_pro);

  /* ARP requests/replies with the same sender and target protocol
     address are flagged as "gratuitous ARPs", i.e. ARPs sent out as,
     in effect, an announcement that the machine has MAC address
     XX:XX:XX:XX:XX:XX and IPv4 address YY.YY.YY.YY. Requests are to
     provoke complaints if some other machine has the same IPv4 address,
     replies are used to announce relocation of network address, like
     in failover solutions. */
  if (((ar_op == ARPOP_REQUEST) || (ar_op == ARPOP_REPLY)) && (strcmp(spa_str, tpa_str) == 0))
    is_gratuitous = TRUE;
  else
    is_gratuitous = FALSE;

  switch (ar_op) {
    case ARPOP_REQUEST:
      if (is_gratuitous)
        col_add_fstr(pinfo->cinfo, COL_INFO, "Gratuitous ARP for %s (Request)", tpa_str);
      else
        col_add_fstr(pinfo->cinfo, COL_INFO, "Who has %s? Tell %s", tpa_str, spa_str);
      break;
    case ARPOP_REPLY:
      if (is_gratuitous)
        col_add_fstr(pinfo->cinfo, COL_INFO, "Gratuitous ARP for %s (Reply)", spa_str);
      else
        col_add_fstr(pinfo->cinfo, COL_INFO, "%s is at %s",
                     spa_str,
                     tvb_arphrdaddr_to_str(tvb, sha_offset, ar_hln, ar_hrd));
      break;
    case ARPOP_RREQUEST:
    case ARPOP_IREQUEST:
      col_add_fstr(pinfo->cinfo, COL_INFO, "Who is %s? Tell %s",
                   tvb_arphrdaddr_to_str(tvb, tha_offset, ar_hln, ar_hrd),
                   tvb_arphrdaddr_to_str(tvb, sha_offset, ar_hln, ar_hrd));
      break;
    case ARPOP_RREPLY:
      col_add_fstr(pinfo->cinfo, COL_INFO, "%s is at %s",
                   tvb_arphrdaddr_to_str(tvb, tha_offset, ar_hln, ar_hrd),
                   tpa_str);
      break;
    case ARPOP_IREPLY:
      col_add_fstr(pinfo->cinfo, COL_INFO, "%s is at %s",
                   tvb_arphrdaddr_to_str(tvb, sha_offset, ar_hln, ar_hrd),
                   spa_str);
      break;
    default:
      col_add_fstr(pinfo->cinfo, COL_INFO, "Unknown ARP opcode 0x%04x", ar_op);
      break;
  }

  if (tree) {
    if ((op_str = try_val_to_str(ar_op, op_vals))) {
      if (is_gratuitous && (ar_op == ARPOP_REQUEST))
        op_str = "request/gratuitous ARP";
      if (is_gratuitous && (ar_op == ARPOP_REPLY))
        op_str = "reply/gratuitous ARP";
      ti = proto_tree_add_protocol_format(tree, proto_arp, tvb, 0, tot_len,
                                        "Address Resolution Protocol (%s)", op_str);
    } else
      ti = proto_tree_add_protocol_format(tree, proto_arp, tvb, 0, tot_len,
                                      "Address Resolution Protocol (opcode 0x%04x)", ar_op);
    arp_tree = proto_item_add_subtree(ti, ett_arp);
    proto_tree_add_uint(arp_tree, hf_arp_hard_type, tvb, AR_HRD, 2, ar_hrd);
    proto_tree_add_uint(arp_tree, hf_arp_proto_type, tvb, AR_PRO, 2, ar_pro);
    proto_tree_add_uint(arp_tree, hf_arp_hard_size, tvb, AR_HLN, 1, ar_hln);
    proto_tree_add_uint(arp_tree, hf_arp_proto_size, tvb, AR_PLN, 1, ar_pln);
    proto_tree_add_uint(arp_tree, hf_arp_opcode, tvb, AR_OP,  2, ar_op);
    if (ar_hln != 0) {
      proto_tree_add_item(arp_tree,
        ARP_HW_IS_AX25(ar_hrd, ar_hln) ? hf_arp_src_hw_ax25 : hf_arp_src_hw,
        tvb, sha_offset, ar_hln, FALSE);
    }
    if (ar_pln != 0) {
      proto_tree_add_item(arp_tree,
        ARP_PRO_IS_IPv4(ar_pro, ar_pln) ? hf_arp_src_proto_ipv4
                                        : hf_arp_src_proto,
        tvb, spa_offset, ar_pln, FALSE);
    }
    if (ar_hln != 0) {
      proto_tree_add_item(arp_tree,
        ARP_HW_IS_AX25(ar_hrd, ar_hln) ? hf_arp_dst_hw_ax25 : hf_arp_dst_hw,
        tvb, tha_offset, ar_hln, FALSE);
    }
    if (ar_pln != 0) {
      proto_tree_add_item(arp_tree,
        ARP_PRO_IS_IPv4(ar_pro, ar_pln) ? hf_arp_dst_proto_ipv4
                                        : hf_arp_dst_proto,
        tvb, tpa_offset, ar_pln, FALSE);
    }
  }

  if (global_arp_detect_request_storm)
  {
    check_for_storm_count(tvb, pinfo, arp_tree);
  }
  return tvb_captured_length(tvb);
}

gboolean
capture_arp(const guchar *pd _U_, int offset _U_, int len _U_, capture_packet_info_t *cpinfo, const union wtap_pseudo_header *pseudo_header _U_)
{
  capture_dissector_increment_count(cpinfo, proto_arp);
  return TRUE;
}

static const guint8 mac_allzero[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

static int
dissect_arp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
  guint16       ar_hrd;
  guint16       ar_pro;
  guint8        ar_hln;
  guint8        ar_pln;
  guint16       ar_op;
  int           tot_len;
  proto_tree   *arp_tree           = NULL;
  proto_item   *ti, *item;
  const gchar  *op_str;
  int           sha_offset, spa_offset, tha_offset, tpa_offset;
  gboolean      is_gratuitous;
  gboolean      duplicate_detected = FALSE;
  guint32       duplicate_ip       = 0;

  /* Call it ARP, for now, so that if we throw an exception before
     we decide whether it's ARP or RARP or IARP or ATMARP, it shows
     up in the packet list as ARP.

     Clear the Info column so that, if we throw an exception, it
     shows up as a short or malformed ARP frame. */
  col_set_str(pinfo->cinfo, COL_PROTOCOL, "ARP");
  col_clear(pinfo->cinfo, COL_INFO);

  /* Hardware Address Type */
  ar_hrd = tvb_get_ntohs(tvb, AR_HRD);
  if (ar_hrd == ARPHRD_ATM2225) {
    call_dissector(atmarp_handle, tvb, pinfo, tree);
    return tvb_captured_length(tvb);
  }
  if (ar_hrd == ARPHRD_AX25) {
    call_dissector(ax25arp_handle, tvb, pinfo, tree);
    return tvb_captured_length(tvb);
  }
  /* Protocol Address Type */
  ar_pro = tvb_get_ntohs(tvb, AR_PRO);
  /* Hardware Address Size */
  ar_hln = tvb_get_guint8(tvb, AR_HLN);
  /* Protocol Address Size */
  ar_pln = tvb_get_guint8(tvb, AR_PLN);
  /* Operation */
  ar_op  = tvb_get_ntohs(tvb, AR_OP);

  tot_len = MIN_ARP_HEADER_SIZE + ar_hln*2 + ar_pln*2;

  /* Adjust the length of this tvbuff to include only the ARP datagram.
     Our caller may use that to determine how much of its packet
     was padding. */
  tvb_set_reported_length(tvb, tot_len);

  switch (ar_op) {

    case ARPOP_REQUEST:
      if (global_arp_detect_request_storm)
      {
        request_seen(pinfo);
      }
      /* FALLTHRU */
    case ARPOP_REPLY:
    default:
      col_set_str(pinfo->cinfo, COL_PROTOCOL, "ARP");
      break;

    case ARPOP_RREQUEST:
    case ARPOP_RREPLY:
      col_set_str(pinfo->cinfo, COL_PROTOCOL, "RARP");
      break;

    case ARPOP_DRARPREQUEST:
    case ARPOP_DRARPREPLY:
    case ARPOP_DRARPERROR:
      col_set_str(pinfo->cinfo, COL_PROTOCOL, "DRARP");
      break;

    case ARPOP_IREQUEST:
    case ARPOP_IREPLY:
      col_set_str(pinfo->cinfo, COL_PROTOCOL, "Inverse ARP");
      break;

   case ARPOP_MARS_REQUEST:
   case ARPOP_MARS_MULTI:
   case ARPOP_MARS_MSERV:
   case ARPOP_MARS_JOIN:
   case ARPOP_MARS_LEAVE:
   case ARPOP_MARS_NAK:
   case ARPOP_MARS_UNSERV:
   case ARPOP_MARS_SJOIN:
   case ARPOP_MARS_SLEAVE:
   case ARPOP_MARS_GROUPLIST_REQUEST:
   case ARPOP_MARS_GROUPLIST_REPLY:
   case ARPOP_MARS_REDIRECT_MAP:
     col_set_str(pinfo->cinfo, COL_PROTOCOL, "MARS");
     break;

   case ARPOP_MAPOS_UNARP:
     col_set_str(pinfo->cinfo, COL_PROTOCOL, "MAPOS");
     break;
  }

  /* Get the offsets of the addresses. */
  /* Source Hardware Address */
  sha_offset = MIN_ARP_HEADER_SIZE;
  /* Source Protocol Address */
  spa_offset = sha_offset + ar_hln;
  /* Target Hardware Address */
  tha_offset = spa_offset + ar_pln;
  /* Target Protocol Address */
  tpa_offset = tha_offset + ar_hln;

  if ((ar_op == ARPOP_REPLY || ar_op == ARPOP_REQUEST) &&
      ARP_HW_IS_ETHER(ar_hrd, ar_hln) &&
      ARP_PRO_IS_IPv4(ar_pro, ar_pln)) {

    /* inform resolv.c module of the new discovered addresses */

    guint32 ip;
    const guint8 *mac;

    /* Add sender address if sender MAC address is neither a broadcast/
       multicast address nor an all-zero address and if sender IP address
       isn't all zeroes. */
    ip = tvb_get_ipv4(tvb, spa_offset);
    mac = (const guint8*)tvb_memdup(wmem_packet_scope(), tvb, sha_offset, 6);
    if ((mac[0] & 0x01) == 0 && memcmp(mac, mac_allzero, 6) != 0 && ip != 0)
    {
      if (global_arp_register_network_address_binding)
      {
        add_ether_byip(ip, mac);
      }
      if (global_arp_detect_duplicate_ip_addresses)
      {
        duplicate_detected =
          check_for_duplicate_addresses(pinfo, tree, tvb, mac, ip,
                                        &duplicate_ip);
      }
    }

    /* Add target address if target MAC address is neither a broadcast/
       multicast address nor an all-zero address and if target IP address
       isn't all zeroes. */

    /* Do not add target address if the packet is a Request. According to the RFC,
       target addresses in requests have no meaning */


    ip = tvb_get_ipv4(tvb, tpa_offset);
    mac = (const guint8*)tvb_memdup(wmem_packet_scope(), tvb, tha_offset, 6);
    if ((mac[0] & 0x01) == 0 && memcmp(mac, mac_allzero, 6) != 0 && ip != 0
        && ar_op != ARPOP_REQUEST)
    {
      if (global_arp_register_network_address_binding)
      {
        add_ether_byip(ip, mac);
      }
      /* If Gratuitous, don't report duplicate for same IP address twice */
      if (global_arp_detect_duplicate_ip_addresses && (duplicate_ip!=ip))
      {
        duplicate_detected =
          check_for_duplicate_addresses(pinfo, tree, tvb, mac, ip,
                                        &duplicate_ip);
      }
    }


  }

  /* ARP requests/replies with the same sender and target protocol
     address are flagged as "gratuitous ARPs", i.e. ARPs sent out as,
     in effect, an announcement that the machine has MAC address
     XX:XX:XX:XX:XX:XX and IPv4 address YY.YY.YY.YY. Requests are to
     provoke complaints if some other machine has the same IPv4 address,
     replies are used to announce relocation of network address, like
     in failover solutions. */
  if (((ar_op == ARPOP_REQUEST) || (ar_op == ARPOP_REPLY)) &&
      (tvb_memeql(tvb, spa_offset, tvb_get_ptr(tvb, tpa_offset, ar_pln), ar_pln) == 0))
    is_gratuitous = TRUE;
  else
    is_gratuitous = FALSE;

  switch (ar_op) {
    case ARPOP_REQUEST:
      if (is_gratuitous)
        col_add_fstr(pinfo->cinfo, COL_INFO, "Gratuitous ARP for %s (Request)",
                     tvb_arpproaddr_to_str(tvb, tpa_offset, ar_pln, ar_pro));
      else
        col_add_fstr(pinfo->cinfo, COL_INFO, "Who has %s? Tell %s",
                     tvb_arpproaddr_to_str(tvb, tpa_offset, ar_pln, ar_pro),
                     tvb_arpproaddr_to_str(tvb, spa_offset, ar_pln, ar_pro));
      break;
    case ARPOP_REPLY:
      if (is_gratuitous)
        col_add_fstr(pinfo->cinfo, COL_INFO, "Gratuitous ARP for %s (Reply)",
                     tvb_arpproaddr_to_str(tvb, spa_offset, ar_pln, ar_pro));
      else
        col_add_fstr(pinfo->cinfo, COL_INFO, "%s is at %s",
                     tvb_arpproaddr_to_str(tvb, spa_offset, ar_pln, ar_pro),
                     tvb_arphrdaddr_to_str(tvb, sha_offset, ar_hln, ar_hrd));
      break;
    case ARPOP_RREQUEST:
    case ARPOP_IREQUEST:
    case ARPOP_DRARPREQUEST:
      col_add_fstr(pinfo->cinfo, COL_INFO, "Who is %s? Tell %s",
                   tvb_arphrdaddr_to_str(tvb, tha_offset, ar_hln, ar_hrd),
                   tvb_arphrdaddr_to_str(tvb, sha_offset, ar_hln, ar_hrd));
      break;
    case ARPOP_RREPLY:
    case ARPOP_DRARPREPLY:
      col_add_fstr(pinfo->cinfo, COL_INFO, "%s is at %s",
                   tvb_arphrdaddr_to_str(tvb, tha_offset, ar_hln, ar_hrd),
                   tvb_arpproaddr_to_str(tvb, tpa_offset, ar_pln, ar_pro));
      break;

    case ARPOP_DRARPERROR:
      col_add_fstr(pinfo->cinfo, COL_INFO, "DRARP Error");
      break;

    case ARPOP_IREPLY:
      col_add_fstr(pinfo->cinfo, COL_INFO, "%s is at %s",
                   tvb_arphrdaddr_to_str(tvb, sha_offset, ar_hln, ar_hrd),
                   tvb_arpproaddr_to_str(tvb, spa_offset, ar_pln, ar_pro));
      break;

    case ATMARPOP_NAK:
      col_add_fstr(pinfo->cinfo, COL_INFO, "ARP NAK");
      break;

    case ARPOP_MARS_REQUEST:
      col_add_fstr(pinfo->cinfo, COL_INFO, "MARS request from %s at %s",
                   tvb_arphrdaddr_to_str(tvb, sha_offset, ar_hln, ar_hrd),
                   tvb_arpproaddr_to_str(tvb, spa_offset, ar_pln, ar_pro));
      break;

    case ARPOP_MARS_MULTI:
      col_add_fstr(pinfo->cinfo, COL_INFO, "MARS MULTI request from %s at %s",
                   tvb_arphrdaddr_to_str(tvb, sha_offset, ar_hln, ar_hrd),
                   tvb_arpproaddr_to_str(tvb, spa_offset, ar_pln, ar_pro));
      break;

    case ARPOP_MARS_MSERV:
      col_add_fstr(pinfo->cinfo, COL_INFO, "MARS MSERV request from %s at %s",
                   tvb_arphrdaddr_to_str(tvb, sha_offset, ar_hln, ar_hrd),
                   tvb_arpproaddr_to_str(tvb, spa_offset, ar_pln, ar_pro));
      break;

    case ARPOP_MARS_JOIN:
      col_add_fstr(pinfo->cinfo, COL_INFO, "MARS JOIN request from %s at %s",
                   tvb_arphrdaddr_to_str(tvb, sha_offset, ar_hln, ar_hrd),
                   tvb_arpproaddr_to_str(tvb, spa_offset, ar_pln, ar_pro));
      break;

    case ARPOP_MARS_LEAVE:
      col_add_fstr(pinfo->cinfo, COL_INFO, "MARS LEAVE from %s at %s",
                   tvb_arphrdaddr_to_str(tvb, sha_offset, ar_hln, ar_hrd),
                   tvb_arpproaddr_to_str(tvb, spa_offset, ar_pln, ar_pro));
      break;

    case ARPOP_MARS_NAK:
      col_add_fstr(pinfo->cinfo, COL_INFO, "MARS NAK from %s at %s",
                   tvb_arphrdaddr_to_str(tvb, sha_offset, ar_hln, ar_hrd),
                   tvb_arpproaddr_to_str(tvb, spa_offset, ar_pln, ar_pro));
      break;

    case ARPOP_MARS_UNSERV:
      col_add_fstr(pinfo->cinfo, COL_INFO, "MARS UNSERV request from %s at %s",
                   tvb_arphrdaddr_to_str(tvb, sha_offset, ar_hln, ar_hrd),
                   tvb_arpproaddr_to_str(tvb, spa_offset, ar_pln, ar_pro));
      break;

    case ARPOP_MARS_SJOIN:
      col_add_fstr(pinfo->cinfo, COL_INFO, "MARS SJOIN request from %s at %s",
                   tvb_arphrdaddr_to_str(tvb, sha_offset, ar_hln, ar_hrd),
                   tvb_arpproaddr_to_str(tvb, spa_offset, ar_pln, ar_pro));
      break;

    case ARPOP_MARS_SLEAVE:
      col_add_fstr(pinfo->cinfo, COL_INFO, "MARS SLEAVE from %s at %s",
                   tvb_arphrdaddr_to_str(tvb, sha_offset, ar_hln, ar_hrd),
                   tvb_arpproaddr_to_str(tvb, spa_offset, ar_pln, ar_pro));
      break;

    case ARPOP_MARS_GROUPLIST_REQUEST:
      col_add_fstr(pinfo->cinfo, COL_INFO, "MARS grouplist request from %s at %s",
                   tvb_arphrdaddr_to_str(tvb, sha_offset, ar_hln, ar_hrd),
                   tvb_arpproaddr_to_str(tvb, spa_offset, ar_pln, ar_pro));
      break;

    case ARPOP_MARS_GROUPLIST_REPLY:
      col_add_fstr(pinfo->cinfo, COL_INFO, "MARS grouplist reply from %s at %s",
                   tvb_arphrdaddr_to_str(tvb, sha_offset, ar_hln, ar_hrd),
                   tvb_arpproaddr_to_str(tvb, spa_offset, ar_pln, ar_pro));
      break;

    case ARPOP_MARS_REDIRECT_MAP:
      col_add_fstr(pinfo->cinfo, COL_INFO, "MARS redirect map from %s at %s",
                   tvb_arphrdaddr_to_str(tvb, sha_offset, ar_hln, ar_hrd),
                   tvb_arpproaddr_to_str(tvb, spa_offset, ar_pln, ar_pro));
      break;

    case ARPOP_MAPOS_UNARP:
      col_add_fstr(pinfo->cinfo, COL_INFO, "MAPOS UNARP request from %s at %s",
                   tvb_arphrdaddr_to_str(tvb, sha_offset, ar_hln, ar_hrd),
                   tvb_arpproaddr_to_str(tvb, spa_offset, ar_pln, ar_pro));
      break;

    case ARPOP_EXP1:
      col_add_fstr(pinfo->cinfo, COL_INFO, "Experimental 1 ( opcode %d )", ar_op);
      break;

    case ARPOP_EXP2:
      col_add_fstr(pinfo->cinfo, COL_INFO, "Experimental 2 ( opcode %d )", ar_op);
      break;

    case 0:
    case 65535:
      col_add_fstr(pinfo->cinfo, COL_INFO, "Reserved opcode %d", ar_op);
      break;

    default:
      col_add_fstr(pinfo->cinfo, COL_INFO, "Unknown ARP opcode 0x%04x", ar_op);
      break;
  }

  if (tree) {
    if ((op_str = try_val_to_str(ar_op, op_vals)))  {
      if (is_gratuitous && (ar_op == ARPOP_REQUEST))
        op_str = "request/gratuitous ARP";
      if (is_gratuitous && (ar_op == ARPOP_REPLY))
        op_str = "reply/gratuitous ARP";
      ti = proto_tree_add_protocol_format(tree, proto_arp, tvb, 0, tot_len,
                                          "Address Resolution Protocol (%s)", op_str);
    } else
      ti = proto_tree_add_protocol_format(tree, proto_arp, tvb, 0, tot_len,
                                          "Address Resolution Protocol (opcode 0x%04x)", ar_op);
    arp_tree = proto_item_add_subtree(ti, ett_arp);
    proto_tree_add_uint(arp_tree, hf_arp_hard_type, tvb, AR_HRD, 2, ar_hrd);
    proto_tree_add_uint(arp_tree, hf_arp_proto_type, tvb, AR_PRO, 2, ar_pro);
    proto_tree_add_uint(arp_tree, hf_arp_hard_size, tvb, AR_HLN, 1, ar_hln);
    proto_tree_add_uint(arp_tree, hf_arp_proto_size, tvb, AR_PLN, 1, ar_pln);
    proto_tree_add_uint(arp_tree, hf_arp_opcode, tvb, AR_OP,  2, ar_op);
    if (is_gratuitous)
   {
    item = proto_tree_add_boolean(arp_tree, hf_arp_isgratuitous, tvb, 0, 0, is_gratuitous);
    PROTO_ITEM_SET_GENERATED(item);
   }
    if (ar_hln != 0) {
      proto_tree_add_item(arp_tree,
                          ARP_HW_IS_ETHER(ar_hrd, ar_hln) ?
                          hf_arp_src_hw_mac :
                          hf_arp_src_hw,
                          tvb, sha_offset, ar_hln, ENC_BIG_ENDIAN);
    }
    if (ar_pln != 0) {
      proto_tree_add_item(arp_tree,
                          ARP_PRO_IS_IPv4(ar_pro, ar_pln) ?
                          hf_arp_src_proto_ipv4 :
                          hf_arp_src_proto,
                          tvb, spa_offset, ar_pln, ENC_BIG_ENDIAN);
    }
    if (ar_hln != 0) {
      proto_tree_add_item(arp_tree,
                          ARP_HW_IS_ETHER(ar_hrd, ar_hln) ?
                          hf_arp_dst_hw_mac :
                          hf_arp_dst_hw,
                          tvb, tha_offset, ar_hln, ENC_BIG_ENDIAN);
    }
    if (ar_pln != 0 && ar_op != ARPOP_DRARPERROR) {     /*DISPLAYING ERROR NUMBER FOR DRARPERROR*/
      proto_tree_add_item(arp_tree,
                          ARP_PRO_IS_IPv4(ar_pro, ar_pln) ?
                          hf_arp_dst_proto_ipv4 :
                          hf_arp_dst_proto,
                          tvb, tpa_offset, ar_pln, ENC_BIG_ENDIAN);
    }
    else if (ar_pln != 0 && ar_op == ARPOP_DRARPERROR) {
       proto_tree_add_item(arp_tree, hf_drarp_error_status, tvb, tpa_offset, 1, ENC_BIG_ENDIAN); /*Adding the first byte of tpa field as drarp_error_status*/
    }
  }

  if (global_arp_detect_request_storm)
  {
    check_for_storm_count(tvb, pinfo, arp_tree);
  }

  if (duplicate_detected)
  {
    /* Also indicate in info column */
    col_append_fstr(pinfo->cinfo, COL_INFO, " (duplicate use of %s detected!)",
                    arpproaddr_to_str((guint8*)&duplicate_ip, 4, ETHERTYPE_IP));
  }
  return tvb_captured_length(tvb);
}

void
proto_register_arp(void)
{
  static struct true_false_string tfs_type_bit = { "E.164", "ATM Forum NSAPA" };

  static hf_register_info hf[] = {
    { &hf_arp_hard_type,
      { "Hardware type",                "arp.hw.type",
        FT_UINT16,      BASE_DEC,       VALS(arp_hrd_vals), 0x0,
        NULL, HFILL }},

    { &hf_arp_proto_type,
      { "Protocol type",                "arp.proto.type",
        FT_UINT16,      BASE_HEX,       VALS(etype_vals),       0x0,
        NULL, HFILL }},

    { &hf_arp_hard_size,
      { "Hardware size",                "arp.hw.size",
        FT_UINT8,       BASE_DEC,       NULL,   0x0,
        NULL, HFILL }},

    { &hf_atmarp_sht,
      { "Sender ATM number type",       "arp.src.htype",
        FT_BOOLEAN,     8,              TFS(&tfs_type_bit),     ATMARP_IS_E164,
        NULL, HFILL }},

    { &hf_atmarp_shl,
      { "Sender ATM number length",     "arp.src.hlen",
        FT_UINT8,       BASE_DEC,       NULL,           ATMARP_LEN_MASK,
        NULL, HFILL }},

    { &hf_atmarp_sst,
      { "Sender ATM subaddress type",   "arp.src.stype",
        FT_BOOLEAN,     8,              TFS(&tfs_type_bit),     ATMARP_IS_E164,
        NULL, HFILL }},

    { &hf_atmarp_ssl,
      { "Sender ATM subaddress length", "arp.src.slen",
        FT_UINT8,       BASE_DEC,       NULL,           ATMARP_LEN_MASK,
        NULL, HFILL }},

    { &hf_arp_proto_size,
      { "Protocol size",                "arp.proto.size",
        FT_UINT8,       BASE_DEC,       NULL,   0x0,
        NULL, HFILL }},

    { &hf_arp_opcode,
      { "Opcode",                       "arp.opcode",
        FT_UINT16,      BASE_DEC,       VALS(op_vals),  0x0,
        NULL, HFILL }},

    { &hf_arp_isgratuitous,
      { "Is gratuitous",                "arp.isgratuitous",
        FT_BOOLEAN,     BASE_NONE,      TFS(&tfs_true_false),   0x0,
        NULL, HFILL }},

    { &hf_atmarp_spln,
      { "Sender protocol size",         "arp.src.pln",
        FT_UINT8,       BASE_DEC,       NULL,   0x0,
        NULL, HFILL }},

    { &hf_atmarp_tht,
      { "Target ATM number type",       "arp.dst.htype",
        FT_BOOLEAN,     8,              TFS(&tfs_type_bit),     ATMARP_IS_E164,
        NULL, HFILL }},

    { &hf_atmarp_thl,
      { "Target ATM number length",     "arp.dst.hlen",
        FT_UINT8,       BASE_DEC,       NULL,           ATMARP_LEN_MASK,
        NULL, HFILL }},

    { &hf_atmarp_tst,
      { "Target ATM subaddress type",   "arp.dst.stype",
        FT_BOOLEAN,     8,              TFS(&tfs_type_bit),     ATMARP_IS_E164,
        NULL, HFILL }},

    { &hf_atmarp_tsl,
      { "Target ATM subaddress length", "arp.dst.slen",
        FT_UINT8,       BASE_DEC,       NULL,           ATMARP_LEN_MASK,
        NULL, HFILL }},

    { &hf_atmarp_tpln,
      { "Target protocol size",         "arp.dst.pln",
        FT_UINT8,       BASE_DEC,       NULL,   0x0,
        NULL, HFILL }},

    { &hf_arp_src_hw,
      { "Sender hardware address",      "arp.src.hw",
        FT_BYTES,       BASE_NONE,      NULL,   0x0,
        NULL, HFILL }},

    { &hf_arp_src_hw_mac,
      { "Sender MAC address",           "arp.src.hw_mac",
        FT_ETHER,       BASE_NONE,      NULL,   0x0,
        NULL, HFILL }},

    { &hf_arp_src_hw_ax25,
      { "Sender AX.25 address",         "arp.src.hw_ax25",
        FT_AX25,        BASE_NONE,      NULL,   0x0,
        NULL, HFILL }},

    { &hf_atmarp_src_atm_num_e164,
      { "Sender ATM number (E.164)",    "arp.src.atm_num_e164",
        FT_STRING,      BASE_NONE,      NULL,   0x0,
        NULL, HFILL }},

    { &hf_atmarp_src_atm_num_nsap,
      { "Sender ATM number (NSAP)",     "arp.src.atm_num_nsap",
        FT_BYTES,       BASE_NONE,      NULL,   0x0,
        NULL, HFILL }},

    { &hf_atmarp_src_atm_subaddr,
      { "Sender ATM subaddress",        "arp.src.atm_subaddr",
        FT_BYTES,       BASE_NONE,      NULL,   0x0,
        NULL, HFILL }},

    { &hf_arp_src_proto,
      { "Sender protocol address",      "arp.src.proto",
        FT_BYTES,       BASE_NONE,      NULL,   0x0,
        NULL, HFILL }},

    { &hf_arp_src_proto_ipv4,
      { "Sender IP address",            "arp.src.proto_ipv4",
        FT_IPv4,        BASE_NONE,      NULL,   0x0,
        NULL, HFILL }},

    { &hf_arp_dst_hw,
      { "Target hardware address",      "arp.dst.hw",
        FT_BYTES,       BASE_NONE,      NULL,   0x0,
        NULL, HFILL }},

    { &hf_arp_dst_hw_mac,
      { "Target MAC address",           "arp.dst.hw_mac",
        FT_ETHER,       BASE_NONE,      NULL,   0x0,
        NULL, HFILL }},

    { &hf_arp_dst_hw_ax25,
      { "Target AX.25 address",         "arp.dst.hw_ax25",
        FT_AX25,        BASE_NONE,      NULL,   0x0,
        NULL, HFILL }},

    { &hf_atmarp_dst_atm_num_e164,
      { "Target ATM number (E.164)",    "arp.dst.atm_num_e164",
        FT_STRING,      BASE_NONE,      NULL,   0x0,
        NULL, HFILL }},

    { &hf_atmarp_dst_atm_num_nsap,
      { "Target ATM number (NSAP)",     "arp.dst.atm_num_nsap",
        FT_BYTES,       BASE_NONE,      NULL,   0x0,
        NULL, HFILL }},

    { &hf_atmarp_dst_atm_subaddr,
      { "Target ATM subaddress",        "arp.dst.atm_subaddr",
        FT_BYTES,       BASE_NONE,      NULL,   0x0,
        NULL, HFILL }},

    { &hf_arp_dst_proto,
      { "Target protocol address",      "arp.dst.proto",
        FT_BYTES,       BASE_NONE,      NULL,   0x0,
        NULL, HFILL }},

    { &hf_arp_dst_proto_ipv4,
      { "Target IP address",            "arp.dst.proto_ipv4",
        FT_IPv4,        BASE_NONE,      NULL,   0x0,
        NULL, HFILL }},

    { &hf_drarp_error_status,
      { "DRARP error status",    "arp.dst.drarp_error_status",
        FT_UINT16,      BASE_DEC,      VALS(drarp_status),   0x0,
        NULL, HFILL }},

    { &hf_arp_duplicate_ip_address_earlier_frame,
      { "Frame showing earlier use of IP address",      "arp.duplicate-address-frame",
        FT_FRAMENUM,    BASE_NONE,      NULL,   0x0,
        NULL, HFILL }},

    { &hf_arp_duplicate_ip_address_seconds_since_earlier_frame,
      { "Seconds since earlier frame seen",     "arp.seconds-since-duplicate-address-frame",
        FT_UINT32,      BASE_DEC,       NULL,   0x0,
        NULL, HFILL }},

      /* Generated from convert_proto_tree_add_text.pl */
      { &hf_atmarp_src_atm_data_country_code, { "Data Country Code", "arp.src.atm_data_country_code", FT_UINT16, BASE_HEX, NULL, 0x0, NULL, HFILL }},
      { &hf_atmarp_src_atm_data_country_code_group, { "Data Country Code (group)", "arp.src.atm_data_country_code_group", FT_UINT16, BASE_HEX, NULL, 0x0, NULL, HFILL }},
      { &hf_atmarp_src_atm_high_order_dsp, { "High Order DSP", "arp.src.atm_high_order_dsp", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }},
      { &hf_atmarp_src_atm_end_system_identifier, { "End System Identifier", "arp.src.atm_end_system_identifier", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }},
      { &hf_atmarp_src_atm_selector, { "Selector", "arp.src.atm_selector", FT_UINT8, BASE_HEX, NULL, 0x0, NULL, HFILL }},
      { &hf_atmarp_src_atm_international_code_designator, { "International Code Designator", "arp.src.atm_international_code_designator", FT_UINT16, BASE_HEX, NULL, 0x0, NULL, HFILL }},
      { &hf_atmarp_src_atm_international_code_designator_group, { "International Code Designator (group)", "arp.src.atm_international_code_designator_group", FT_UINT16, BASE_HEX, NULL, 0x0, NULL, HFILL }},
      { &hf_atmarp_src_atm_e_164_isdn, { "E.164 ISDN", "arp.src.atm_e.164_isdn", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }},
      { &hf_atmarp_src_atm_e_164_isdn_group, { "E.164 ISDN", "arp.src.atm_e.164_isdn_group", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }},
      { &hf_atmarp_src_atm_rest_of_address, { "Rest of address", "arp.src.atm_rest_of_address", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }},
      { &hf_atmarp_src_atm_afi, { "AFI", "arp.src.atm_afi", FT_UINT8, BASE_HEX, VALS(atm_nsap_afi_vals), 0x0, NULL, HFILL }},
  };

  static gint *ett[] = {
    &ett_arp,
    &ett_atmarp_nsap,
    &ett_atmarp_tl,
    &ett_arp_duplicate_address
  };

  static ei_register_info ei[] = {
     { &ei_seq_arp_dup_ip, { "arp.duplicate-address-detected", PI_SEQUENCE, PI_WARN, "Duplicate IP address configured", EXPFILL }},
     { &ei_seq_arp_storm, { "arp.packet-storm-detected", PI_SEQUENCE, PI_NOTE, "ARP packet storm detected", EXPFILL }},
     { &ei_atmarp_src_atm_unknown_afi, { "arp.src.atm_afi.unknown", PI_PROTOCOL, PI_WARN, "Unknown AFI", EXPFILL }},
  };

  module_t *arp_module;
  expert_module_t* expert_arp;
  int proto_atmarp;

  proto_arp = proto_register_protocol("Address Resolution Protocol",
                                      "ARP/RARP", "arp");
  proto_atmarp = proto_register_protocol("ATM Address Resolution Protocol",
                                      "ATMARP", "atmarp");

  proto_register_field_array(proto_arp, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));

  expert_arp = expert_register_protocol(proto_arp);
  expert_register_field_array(expert_arp, ei, array_length(ei));

  atmarp_handle = create_dissector_handle(dissect_atmarp, proto_atmarp);
  ax25arp_handle = create_dissector_handle(dissect_ax25arp, proto_arp);

  arp_handle = register_dissector( "arp" , dissect_arp, proto_arp );

  /* Preferences */
  arp_module = prefs_register_protocol(proto_arp, NULL);

  prefs_register_bool_preference(arp_module, "detect_request_storms",
                                 "Detect ARP request storms",
                                 "Attempt to detect excessive rate of ARP requests",
                                 &global_arp_detect_request_storm);

  prefs_register_uint_preference(arp_module, "detect_storm_number_of_packets",
                                 "Number of requests to detect during period",
                                 "Number of requests needed within period to indicate a storm",
                                 10, &global_arp_detect_request_storm_packets);

  prefs_register_uint_preference(arp_module, "detect_storm_period",
                                 "Detection period (in ms)",
                                 "Period in milliseconds during which a packet storm may be detected",
                                 10, &global_arp_detect_request_storm_period);

  prefs_register_bool_preference(arp_module, "detect_duplicate_ips",
                                 "Detect duplicate IP address configuration",
                                 "Attempt to detect duplicate use of IP addresses",
                                 &global_arp_detect_duplicate_ip_addresses);

  prefs_register_bool_preference(arp_module, "register_network_address_binding",
                                 "Register network address mappings",
                                 "Try to resolve physical addresses to host names from ARP requests/responses",
                                 &global_arp_register_network_address_binding);

  /* TODO: define a minimum time between sightings that is worth reporting? */

  register_init_routine(&arp_init_protocol);
  register_cleanup_routine(&arp_cleanup_protocol);
}

void
proto_reg_handoff_arp(void)
{
  dissector_add_uint("ethertype", ETHERTYPE_ARP, arp_handle);
  dissector_add_uint("ethertype", ETHERTYPE_REVARP, arp_handle);
  dissector_add_uint("arcnet.protocol_id", ARCNET_PROTO_ARP_1051, arp_handle);
  dissector_add_uint("arcnet.protocol_id", ARCNET_PROTO_ARP_1201, arp_handle);
  dissector_add_uint("arcnet.protocol_id", ARCNET_PROTO_RARP_1201, arp_handle);
  dissector_add_uint("ax25.pid", AX25_P_ARP, arp_handle);
  dissector_add_uint("gre.proto", ETHERTYPE_ARP, arp_handle);
  register_capture_dissector("ethertype", ETHERTYPE_ARP, capture_arp, proto_arp);
  register_capture_dissector("ax25.pid", AX25_P_ARP, capture_arp, proto_arp);
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
