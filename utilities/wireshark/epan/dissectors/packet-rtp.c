/* packet-rtp.c
 *
 * Routines for RTP dissection
 * RTP = Real time Transport Protocol
 *
 * Copyright 2000, Philips Electronics N.V.
 * Written by Andreas Sikkema <h323@ramdyne.nl>
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

/*
 * This dissector tries to dissect the RTP protocol according to Annex A
 * of ITU-T Recommendation H.225.0 (02/98) or RFC 1889
 *
 * RTP traffic is handled by an even UDP portnumber. This can be any
 * port number, but there is a registered port available, port 5004
 * See Annex B of ITU-T Recommendation H.225.0, section B.7
 *
 * This doesn't dissect older versions of RTP, such as:
 *
 *    the vat protocol ("version 0") - see
 *
 *  ftp://ftp.ee.lbl.gov/conferencing/vat/alpha-test/vatsrc-4.0b2.tar.gz
 *
 *    and look in "session-vat.cc" if you want to write a dissector
 *    (have fun - there aren't any nice header files showing the packet
 *    format);
 *
 *    version 1, as documented in
 *
 *  ftp://gaia.cs.umass.edu/pub/hgschulz/rtp/draft-ietf-avt-rtp-04.txt
 *
 * It also dissects PacketCable CCC-encapsulated RTP data, as described in
 * chapter 5 of the PacketCable Electronic Surveillance Specification:
 *
 *   http://www.packetcable.com/downloads/specs/PKT-SP-ESP1.5-I01-050128.pdf
 */


#include "config.h"

#include <epan/packet.h>
#include <epan/exceptions.h>
#include <epan/expert.h>
#include <epan/proto_data.h>

#include "packet-rtp.h"

#include <epan/rtp_pt.h>
#include <epan/conversation.h>
#include <epan/reassemble.h>
#include <epan/tap.h>
#include <epan/prefs.h>

/* un-comment the following as well as this line in conversation.c, to enable debug printing */
/* #define DEBUG_CONVERSATION */
#include "conversation_debug.h"

/* uncomment this to enable debugging of fragment reassembly */
/* #define DEBUG   1 */
/* #define DEBUG_FRAGMENTS   1 */

typedef struct _rfc2198_hdr {
    unsigned int pt;
    int offset;
    int len;
    struct _rfc2198_hdr *next;
} rfc2198_hdr;

/* we have one of these for each pdu which spans more than one segment
 */
typedef struct _rtp_multisegment_pdu {
    /* the seqno of the segment where the pdu starts */
    guint32 startseq;

    /* the seqno of the segment where the pdu ends */
    guint32 endseq;
} rtp_multisegment_pdu;

typedef struct  _rtp_private_conv_info {
    /* This tree is indexed by sequence number and keeps track of all
     * all pdus spanning multiple segments for this flow.
     */
    wmem_tree_t *multisegment_pdus;
} rtp_private_conv_info;

typedef struct {
    char *encoding_name;
    int   sample_rate;
} encoding_name_and_rate_t;

struct _rtp_dyn_payload_t
{
    GHashTable *table;
    size_t ref_count;
};

static reassembly_table rtp_reassembly_table;

static int hf_rtp_fragments = -1;
static int hf_rtp_fragment = -1;
static int hf_rtp_fragment_overlap = -1;
static int hf_rtp_fragment_overlap_conflict = -1;
static int hf_rtp_fragment_multiple_tails = -1;
static int hf_rtp_fragment_too_long_fragment = -1;
static int hf_rtp_fragment_error = -1;
static int hf_rtp_fragment_count = -1;
static int hf_rtp_reassembled_in = -1;
static int hf_rtp_reassembled_length = -1;

static gint ett_rtp_fragment = -1;
static gint ett_rtp_fragments = -1;

static const fragment_items rtp_fragment_items = {
    &ett_rtp_fragment,
    &ett_rtp_fragments,
    &hf_rtp_fragments,
    &hf_rtp_fragment,
    &hf_rtp_fragment_overlap,
    &hf_rtp_fragment_overlap_conflict,
    &hf_rtp_fragment_multiple_tails,
    &hf_rtp_fragment_too_long_fragment,
    &hf_rtp_fragment_error,
    &hf_rtp_fragment_count,
    &hf_rtp_reassembled_in,
    &hf_rtp_reassembled_length,
    /* Reassembled data field */
    NULL,
    "RTP fragments"
};

static dissector_handle_t rtp_handle;
static dissector_handle_t rtcp_handle;
static dissector_handle_t classicstun_handle;
static dissector_handle_t stun_handle;
static dissector_handle_t classicstun_heur_handle;
static dissector_handle_t stun_heur_handle;
static dissector_handle_t t38_handle;
static dissector_handle_t zrtp_handle;

static dissector_handle_t sprt_handle;
static dissector_handle_t v150fw_handle;

static dissector_handle_t bta2dp_content_protection_header_scms_t;
static dissector_handle_t btvdp_content_protection_header_scms_t;
static dissector_handle_t bta2dp_handle;
static dissector_handle_t btvdp_handle;
static dissector_handle_t sbc_handle;

static int rtp_tap = -1;

static dissector_table_t rtp_pt_dissector_table;
static dissector_table_t rtp_dyn_pt_dissector_table;

static dissector_table_t rtp_hdr_ext_dissector_table;
static dissector_table_t rtp_hdr_ext_rfc5285_dissector_table;

/* RTP header fields             */
static int proto_rtp           = -1;
static int hf_rtp_version      = -1;
static int hf_rtp_padding      = -1;
static int hf_rtp_extension    = -1;
static int hf_rtp_csrc_count   = -1;
static int hf_rtp_marker       = -1;
static int hf_rtp_payload_type = -1;
static int hf_rtp_seq_nr       = -1;
static int hf_rtp_ext_seq_nr   = -1;
static int hf_rtp_timestamp    = -1;
static int hf_rtp_ssrc         = -1;
static int hf_rtp_csrc_items   = -1;
static int hf_rtp_csrc_item    = -1;
static int hf_rtp_data         = -1;
static int hf_rtp_padding_data = -1;
static int hf_rtp_padding_count= -1;
static int hf_rtp_rfc2198_follow= -1;
static int hf_rtp_rfc2198_tm_off= -1;
static int hf_rtp_rfc2198_bl_len= -1;

/* RTP header extension fields   */
static int hf_rtp_prof_define  = -1;
static int hf_rtp_length       = -1;
static int hf_rtp_hdr_exts     = -1;
static int hf_rtp_hdr_ext      = -1;

/* RTP header ED137A extension fields   */
static int hf_rtp_hdr_ed137s    = -1;
static int hf_rtp_hdr_ed137     = -1;
static int hf_rtp_hdr_ed137a     = -1;
static int hf_rtp_hdr_ed137_ptt_type  = -1;
static int hf_rtp_hdr_ed137_squ       = -1;
static int hf_rtp_hdr_ed137_ptt_id    = -1;
static int hf_rtp_hdr_ed137_sct       = -1;
static int hf_rtp_hdr_ed137_x         = -1;
static int hf_rtp_hdr_ed137_x_nu      = -1;
static int hf_rtp_hdr_ed137_ft_type   = -1;
static int hf_rtp_hdr_ed137_ft_len    = -1;
static int hf_rtp_hdr_ed137_ft_value  = -1;
static int hf_rtp_hdr_ed137_ft_bss_qidx  = -1;
static int hf_rtp_hdr_ed137_ft_bss_rssi_qidx  = -1;
static int hf_rtp_hdr_ed137_ft_bss_qidx_ml  = -1;
static int hf_rtp_hdr_ed137_ft_bss_nu  = -1;
static int hf_rtp_hdr_ed137_vf  = -1;
static int hf_rtp_hdr_ed137a_ptt_type  = -1;
static int hf_rtp_hdr_ed137a_squ       = -1;
static int hf_rtp_hdr_ed137a_ptt_id    = -1;
static int hf_rtp_hdr_ed137a_pm        = -1;
static int hf_rtp_hdr_ed137a_ptts      = -1;
static int hf_rtp_hdr_ed137a_sct       = -1;
static int hf_rtp_hdr_ed137a_reserved  = -1;
static int hf_rtp_hdr_ed137a_x         = -1;
static int hf_rtp_hdr_ed137a_x_nu      = -1;
static int hf_rtp_hdr_ed137a_ft_type   = -1;
static int hf_rtp_hdr_ed137a_ft_len    = -1;
static int hf_rtp_hdr_ed137a_ft_value  = -1;
static int hf_rtp_hdr_ed137a_ft_sqi_qidx  = -1;
static int hf_rtp_hdr_ed137a_ft_sqi_rssi_qidx  = -1;
static int hf_rtp_hdr_ed137a_ft_sqi_qidx_ml  = -1;
static gint ett_hdr_ext_ed137s  = -1;
static gint ett_hdr_ext_ed137   = -1;
static gint ett_hdr_ext_ed137a   = -1;

/* RTP setup fields */
static int hf_rtp_setup        = -1;
static int hf_rtp_setup_frame  = -1;
static int hf_rtp_setup_method = -1;

/* RTP fields defining a sub tree */
static gint ett_rtp       = -1;
static gint ett_csrc_list = -1;
static gint ett_hdr_ext   = -1;
static gint ett_hdr_ext_rfc5285 = -1;
static gint ett_rtp_setup = -1;
static gint ett_rtp_rfc2198 = -1;
static gint ett_rtp_rfc2198_hdr = -1;

/* SRTP fields */
static int hf_srtp_encrypted_payload = -1;
/* static int hf_srtp_null_encrypted_payload = -1; */
static int hf_srtp_mki = -1;
static int hf_srtp_auth_tag = -1;

/* PacketCable CCC header fields */
static int proto_pkt_ccc       = -1;
static int hf_pkt_ccc_id       = -1;
static int hf_pkt_ccc_ts       = -1;

/* PacketCable CCC field defining a sub tree */
static gint ett_pkt_ccc = -1;

static expert_field ei_rtp_fragment_unfinished = EI_INIT;
static expert_field ei_rtp_padding_missing = EI_INIT;

/* PacketCable CCC port preference */
static guint global_pkt_ccc_udp_port = 0;

/* RFC 5285 Header extensions */
static int hf_rtp_ext_rfc5285_id = -1;
static int hf_rtp_ext_rfc5285_length = -1;
static int hf_rtp_ext_rfc5285_appbits = -1;
static int hf_rtp_ext_rfc5285_data = -1;

#define RTP0_INVALID 0
#define RTP0_STUN    1
#define RTP0_CLASSICSTUN    2
#define RTP0_T38     3
#define RTP0_SPRT    4

static const enum_val_t rtp_version0_types[] = {
    { "invalid", "Invalid or ZRTP packets", RTP0_INVALID },
    { "stun", "STUN packets", RTP0_STUN },
    { "classicstun", "CLASSIC-STUN packets", RTP0_CLASSICSTUN },
    { "t38", "T.38 packets", RTP0_T38 },
    { "sprt", "SPRT packets", RTP0_SPRT },
    { NULL, NULL, 0 }
};
static gint global_rtp_version0_type = 0;

/* Forward declaration we need below */
void proto_register_rtp(void);
void proto_reg_handoff_rtp(void);
void proto_register_pkt_ccc(void);
void proto_reg_handoff_pkt_ccc(void);

static gint dissect_rtp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data);
static void show_setup_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);
static void get_conv_info(packet_info *pinfo, struct _rtp_info *rtp_info);

/* Preferences bool to control whether or not setup info should be shown */
static gboolean global_rtp_show_setup_info = TRUE;

/* desegment RTP streams */
static gboolean desegment_rtp = TRUE;

/* RFC2198 Redundant Audio Data */
static guint rtp_rfc2198_pt = 99;

/*
 * Fields in the first octet of the RTP header.
 */

/* Version is the first 2 bits of the first octet*/
#define RTP_VERSION(octet)  ((octet) >> 6)

/* Padding is the third bit; No need to shift, because true is any value
   other than 0! */
#define RTP_PADDING(octet)  ((octet) & 0x20)

/* Extension bit is the fourth bit */
#define RTP_EXTENSION(octet)    ((octet) & 0x10)

/* ED137 signature */
#define RTP_ED137_SIG    0x0067

/* ED137A signature */
#define RTP_ED137A_SIG   0x0167

/* ED137 PTT */
#define RTP_ED137_ptt_mask(octet)   ((octet) & 0xE0000000)
#define RTP_ED137A_ptt_mask(octet)   ((octet) & 0xE0000000)
#define RTP_ED137_squ_mask(octet)   ((octet) & 0x10000000)
#define RTP_ED137A_squ_mask(octet)   ((octet) & 0x10000000)

/* ED137 extended information */
#define RTP_ED137_extended_information(octet)   ((octet) & 0x00400000)
#define RTP_ED137A_extended_information(octet)  ((octet) & 0x00010000)

/* ED137 feature type */
#define RTP_ED137_feature_type(octet)  (((octet) & 0x003C0000) >> 18)
#define RTP_ED137A_feature_type(octet) (((octet) & 0x0000F000) >> 12)

/* ED137 feature length */
#define RTP_ED137_feature_length(octet)  (((octet) & 0x0003C000) >> 14)
#define RTP_ED137A_feature_length(octet) (((octet) & 0x00000F00) >> 8)

/* ED137 feature value */
#define RTP_ED137_feature_value(octet)  (((octet) & 0x00003FFE) >> 1)
#define RTP_ED137A_feature_value(octet) (((octet) & 0x000000FF) >> 0)

/* ED137 BSS constants */
#define RTP_ED137_feature_bss_type    0x1
#define RTP_ED137_feature_bss_len     11
#define RTP_ED137_feature_bss_qidx(octet)   (((octet) & 0x00003FC0) >> 6)
#define RTP_ED137_feature_bss_qidx_ml(octet)   (((octet) & 0x00000038) >> 2)
#define RTP_ED137_feature_bss_qidx_ml_rssi      0
#define RTP_ED137_feature_bss_qidx_rssi_max     15

/* ED137 SQI constants */
#define RTP_ED137A_feature_sqi_type   0x1
#define RTP_ED137A_feature_sqi_len    11
#define RTP_ED137A_feature_sqi_qidx(octet)  (((octet) & 0x000000F8) >> 3)
#define RTP_ED137A_feature_sqi_qidx_ml(octet)  (((octet) & 0x00000007) >> 0)
#define RTP_ED137A_feature_sqi_qidx_ml_rssi     0
#define RTP_ED137A_feature_sqi_qidx_rssi_max    15

/* RFC 5285 one byte header signature */
#define RTP_RFC5285_ONE_BYTE_SIG        0xBEDE

/* RFC 5285 two byte header mask and signature */
#define RTP_RFC5285_TWO_BYTE_MASK       0xFFF0
#define RTP_RFC5285_TWO_BYTE_SIG        0x1000

/* CSRC count is the last four bits */
#define RTP_CSRC_COUNT(octet)   ((octet) & 0xF)

static const value_string rtp_version_vals[] =
{
    { 2, "RFC 1889 Version" }, /* First for speed */
    { 0, "Old VAT Version" },
    { 1, "First Draft Version" },
    { 0, NULL },
};

static const value_string rtp_ext_profile_vals[] =
{
    { RTP_ED137_SIG, "ED137" },
    { RTP_ED137A_SIG, "ED137A" },
    { 0, NULL },
};

static const value_string rtp_ext_ed137_ptt_type[] =
{
    { 0x00, "PTT OFF" },
    { 0x01, "Normal PTT ON" },
    { 0x02, "Coupling PTT ON" },
    { 0x03, "Priority PTT ON" },
    { 0x04, "Emergency PTT ON" },
    { 0x05, "Reserved" },
    { 0x06, "Reserved" },
    { 0x07, "Reserved" },
    { 0, NULL },
};

static const value_string rtp_ext_ed137_squ[] =
{
    { 0x00, "SQ OFF" },
    { 0x01, "SQ ON" },
    { 0, NULL },
};

static const value_string rtp_ext_ed137_ft_type[] =
{
    { 0x0, "No features" },
    { 0x1, "Best signal selection" },
    { 0x2, "CLIMAX time delay" },
    { 0x3, "Reserved" },
    { 0x4, "Reserved" },
    { 0x5, "Reserved" },
    { 0x6, "Reserved" },
    { 0x7, "Reserved" },
    { 0x8, "Reserved" },
    { 0x9, "Reserved" },
    { 0xA, "Reserved" },
    { 0xB, "Vendor reserved" },
    { 0xC, "Vendor reserved" },
    { 0xD, "Vendor reserved" },
    { 0xE, "Vendor reserved" },
    { 0xF, "Vendor reserved" },
    { 0, NULL },
};

static const value_string rtp_ext_ed137_vf[] =
{
    { 0x00, "VF OFF" },
    { 0x01, "VF ON" },
    { 0, NULL },
};

static const value_string rtp_ext_ed137_ft_bss_rssi_qidx[] =
{
    { 0x00, "lower than -100.00 dBm" },
    { 0x01, "lower than or equal to -97.86 dBm" },
    { 0x02, "lower than or equal to -95.71 dBm" },
    { 0x03, "lower than or equal to -93.57 dBm" },
    { 0x04, "lower than or equal to -91.43 dBm" },
    { 0x05, "lower than or equal to -89.29 dBm" },
    { 0x06, "lower than or equal to -87.14 dBm" },
    { 0x07, "lower than or equal to -85.00 dBm" },
    { 0x08, "lower than or equal to -82.86 dBm" },
    { 0x09, "lower than or equal to -80.71 dBm" },
    { 0x0a, "lower than or equal to -78.57 dBm" },
    { 0x0b, "lower than or equal to -76.43 dBm" },
    { 0x0c, "lower than or equal to -74.29 dBm" },
    { 0x0d, "lower than or equal to -72.14 dBm" },
    { 0x0e, "lower than or equal to -70.00 dBm" },
    { 0x0f, "higher than -70.00 dBm" },
    { 0, NULL },
};

static const value_string rtp_ext_ed137_ft_bss_qidx_ml[] =
{
    { 0x00, "RSSI" },
    { 0x01, "AGC Level" },
    { 0x02, "C/N" },
    { 0x03, "Standardized PSD" },
    { 0x04, "Vendor specific method" },
    { 0x05, "Vendor specific method" },
    { 0x06, "Vendor specific method" },
    { 0x07, "Vendor specific method" },
    { 0, NULL },
};

static const value_string rtp_ext_ed137a_ptt_type[] =
{
    { 0x00, "PTT OFF" },
    { 0x01, "Normal PTT ON" },
    { 0x02, "Coupling PTT ON" },
    { 0x03, "Priority PTT ON" },
    { 0x04, "Emergency PTT ON" },
    { 0x05, "Reserved" },
    { 0x06, "Reserved" },
    { 0x07, "Reserved" },
    { 0, NULL },
};

static const value_string rtp_ext_ed137a_squ[] =
{
    { 0x00, "SQ OFF" },
    { 0x01, "SQ ON" },
    { 0, NULL },
};

static const value_string rtp_ext_ed137a_ft_type[] =
{
    { 0x0, "No features" },
    { 0x1, "Signal Quality Information" },
    { 0x2, "CLIMAX time delay" },
    { 0x3, "Radio remote control" },
    { 0x4, "CLIMAX dynamic delay compensation" },
    { 0x5, "Reserved" },
    { 0x6, "Reserved" },
    { 0x7, "Reserved" },
    { 0x8, "Reserved" },
    { 0x9, "Reserved" },
    { 0xA, "Reserved" },
    { 0xB, "Vendor reserved" },
    { 0xC, "Vendor reserved" },
    { 0xD, "Vendor reserved" },
    { 0xE, "Vendor reserved" },
    { 0xF, "Vendor reserved" },
    { 0, NULL },
};

static const value_string rtp_ext_ed137a_ft_sqi_rssi_qidx[] =
{
    { 0x00, "lower than -100.00 dBm" },
    { 0x01, "lower than or equal to -97.86 dBm" },
    { 0x02, "lower than or equal to -95.71 dBm" },
    { 0x03, "lower than or equal to -93.57 dBm" },
    { 0x04, "lower than or equal to -91.43 dBm" },
    { 0x05, "lower than or equal to -89.29 dBm" },
    { 0x06, "lower than or equal to -87.14 dBm" },
    { 0x07, "lower than or equal to -85.00 dBm" },
    { 0x08, "lower than or equal to -82.86 dBm" },
    { 0x09, "lower than or equal to -80.71 dBm" },
    { 0x0a, "lower than or equal to -78.57 dBm" },
    { 0x0b, "lower than or equal to -76.43 dBm" },
    { 0x0c, "lower than or equal to -74.29 dBm" },
    { 0x0d, "lower than or equal to -72.14 dBm" },
    { 0x0e, "lower than or equal to -70.00 dBm" },
    { 0x0f, "higher than -70.00 dBm" },
    { 0, NULL },
};

static const value_string rtp_ext_ed137a_ft_sqi_qidx_ml[] =
{
    { 0x00, "RSSI" },
    { 0x01, "AGC Level" },
    { 0x02, "C/N" },
    { 0x03, "Standardized PSD" },
    { 0x04, "Vendor specific method" },
    { 0x05, "Vendor specific method" },
    { 0x06, "Vendor specific method" },
    { 0x07, "Vendor specific method" },
    { 0, NULL },
};

/*
 * Fields in the second octet of the RTP header.
 */

/* Marker is the first bit of the second octet */
#define RTP_MARKER(octet)   ((octet) & 0x80)

/* Payload type is the last 7 bits */
#define RTP_PAYLOAD_TYPE(octet) ((octet) & 0x7F)
/* http://www.iana.org/assignments/rtp-parameters */

#define FIRST_RTCP_CONFLICT_PAYLOAD_TYPE 64
#define LAST_RTCP_CONFLICT_PAYLOAD_TYPE  95

static const value_string rtp_payload_type_vals[] =
{
/*  0 */    { PT_PCMU,          "ITU-T G.711 PCMU" },
/*  1 */    { PT_1016,          "USA Federal Standard FS-1016" },
/*  2 */    { PT_G721,          "ITU-T G.721" },
/*  3 */    { PT_GSM,           "GSM 06.10" },
/*  4 */    { PT_G723,          "ITU-T G.723" },
/*  5 */    { PT_DVI4_8000,     "DVI4 8000 samples/s" },
/*  6 */    { PT_DVI4_16000,    "DVI4 16000 samples/s" },
/*  7 */    { PT_LPC,           "Experimental linear predictive encoding from Xerox PARC" },
/*  8 */    { PT_PCMA,          "ITU-T G.711 PCMA" },
/*  9 */    { PT_G722,          "ITU-T G.722" },
/* 10 */    { PT_L16_STEREO,    "16-bit uncompressed audio, stereo" },
/* 11 */    { PT_L16_MONO,      "16-bit uncompressed audio, monaural" },
/* 12 */    { PT_QCELP,         "Qualcomm Code Excited Linear Predictive coding" },
/* 13 */    { PT_CN,            "Comfort noise" },
/* 14 */    { PT_MPA,           "MPEG-I/II Audio"},
/* 15 */    { PT_G728,          "ITU-T G.728" },
/* 16 */    { PT_DVI4_11025,    "DVI4 11025 samples/s" },
/* 17 */    { PT_DVI4_22050,    "DVI4 22050 samples/s" },
/* 18 */    { PT_G729,          "ITU-T G.729" },
/* 19 */    { PT_CN_OLD,        "Comfort noise (old)" },
/* 20 */    { 20,               "Unassigned" },
/* 21 */    { 21,               "Unassigned" },
/* 22 */    { 22,               "Unassigned" },
/* 23 */    { 23,               "Unassigned" },
/* 24 */    { 24,               "Unassigned" },
/* 25 */    { PT_CELB,          "Sun CellB video encoding" },
/* 26 */    { PT_JPEG,          "JPEG-compressed video" },
/* 27 */    { 27,               "Unassigned" },
/* 28 */    { PT_NV,            "'nv' program" },
/* 29 */    { 29,               "Unassigned" },
/* 30 */    { 30,               "Unassigned" },
/* 31 */    { PT_H261,          "ITU-T H.261" },
/* 32 */    { PT_MPV,           "MPEG-I/II Video"},
/* 33 */    { PT_MP2T,          "MPEG-II transport streams"},
/* 34 */    { PT_H263,          "ITU-T H.263" },
/* 35-71     Unassigned  */
/* 35 */    { 35,               "Unassigned" },
/* 36 */    { 36,               "Unassigned" },
/* 37 */    { 37,               "Unassigned" },
/* 38 */    { 38,               "Unassigned" },
/* 39 */    { 39,               "Unassigned" },
/* 40 */    { 40,               "Unassigned" },
/* 41 */    { 41,               "Unassigned" },
/* 42 */    { 42,               "Unassigned" },
/* 43 */    { 43,               "Unassigned" },
/* 44 */    { 44,               "Unassigned" },
/* 45 */    { 45,               "Unassigned" },
/* 46 */    { 46,               "Unassigned" },
/* 47 */    { 47,               "Unassigned" },
/* 48 */    { 48,               "Unassigned" },
/* 49 */    { 49,               "Unassigned" },
/* 50 */    { 50,               "Unassigned" },
/* 51 */    { 51,               "Unassigned" },
/* 52 */    { 52,               "Unassigned" },
/* 53 */    { 53,               "Unassigned" },
/* 54 */    { 54,               "Unassigned" },
/* 55 */    { 55,               "Unassigned" },
/* 56 */    { 56,               "Unassigned" },
/* 57 */    { 57,               "Unassigned" },
/* 58 */    { 58,               "Unassigned" },
/* 59 */    { 59,               "Unassigned" },
/* 60 */    { 60,               "Unassigned" },
/* 61 */    { 61,               "Unassigned" },
/* 62 */    { 62,               "Unassigned" },
/* 63 */    { 63,               "Unassigned" },
/* 64 */    { 64,               "Unassigned" },
/* 65 */    { 65,               "Unassigned" },
/* 66 */    { 66,               "Unassigned" },
/* 67 */    { 67,               "Unassigned" },
/* 68 */    { 68,               "Unassigned" },
/* 69 */    { 69,               "Unassigned" },
/* 70 */    { 70,               "Unassigned" },
/* 71 */    { 71,               "Unassigned" },
/* 72-76     Reserved for RTCP conflict avoidance                                  [RFC3551] */
/* 72 */    { 72,               "Reserved for RTCP conflict avoidance" },
/* 73 */    { 73,               "Reserved for RTCP conflict avoidance" },
/* 74 */    { 74,               "Reserved for RTCP conflict avoidance" },
/* 75 */    { 75,               "Reserved for RTCP conflict avoidance" },
/* 76 */    { 76,               "Reserved for RTCP conflict avoidance" },
/* 77-95     Unassigned      ? */
/* 77 */    { 77,               "Unassigned" },
/* 78 */    { 78,               "Unassigned" },
/* 79 */    { 79,               "Unassigned" },
/* 80 */    { 80,               "Unassigned" },
/* 81 */    { 81,               "Unassigned" },
/* 82 */    { 82,               "Unassigned" },
/* 83 */    { 83,               "Unassigned" },
/* 84 */    { 84,               "Unassigned" },
/* 85 */    { 85,               "Unassigned" },
/* 86 */    { 86,               "Unassigned" },
/* 87 */    { 87,               "Unassigned" },
/* 88 */    { 88,               "Unassigned" },
/* 89 */    { 89,               "Unassigned" },
/* 90 */    { 90,               "Unassigned" },
/* 91 */    { 91,               "Unassigned" },
/* 92 */    { 92,               "Unassigned" },
/* 93 */    { 93,               "Unassigned" },
/* 94 */    { 94,               "Unassigned" },
/* 95 */    { 95,               "Unassigned" },
        /* Added to support addtional RTP payload types
         * See epan/rtp_pt.h */
        { PT_UNDF_96,   "DynamicRTP-Type-96" },
        { PT_UNDF_97,   "DynamicRTP-Type-97" },
        { PT_UNDF_98,   "DynamicRTP-Type-98" },
        { PT_UNDF_99,   "DynamicRTP-Type-99" },
        { PT_UNDF_100,  "DynamicRTP-Type-100" },
        { PT_UNDF_101,  "DynamicRTP-Type-101" },
        { PT_UNDF_102,  "DynamicRTP-Type-102" },
        { PT_UNDF_103,  "DynamicRTP-Type-103" },
        { PT_UNDF_104,  "DynamicRTP-Type-104" },
        { PT_UNDF_105,  "DynamicRTP-Type-105" },
        { PT_UNDF_106,  "DynamicRTP-Type-106" },
        { PT_UNDF_107,  "DynamicRTP-Type-107" },
        { PT_UNDF_108,  "DynamicRTP-Type-108" },
        { PT_UNDF_109,  "DynamicRTP-Type-109" },
        { PT_UNDF_110,  "DynamicRTP-Type-110" },
        { PT_UNDF_111,  "DynamicRTP-Type-111" },
        { PT_UNDF_112,  "DynamicRTP-Type-112" },
        { PT_UNDF_113,  "DynamicRTP-Type-113" },
        { PT_UNDF_114,  "DynamicRTP-Type-114" },
        { PT_UNDF_115,  "DynamicRTP-Type-115" },
        { PT_UNDF_116,  "DynamicRTP-Type-116" },
        { PT_UNDF_117,  "DynamicRTP-Type-117" },
        { PT_UNDF_118,  "DynamicRTP-Type-118" },
        { PT_UNDF_119,  "DynamicRTP-Type-119" },
        { PT_UNDF_120,  "DynamicRTP-Type-120" },
        { PT_UNDF_121,  "DynamicRTP-Type-121" },
        { PT_UNDF_122,  "DynamicRTP-Type-122" },
        { PT_UNDF_123,  "DynamicRTP-Type-123" },
        { PT_UNDF_124,  "DynamicRTP-Type-124" },
        { PT_UNDF_125,  "DynamicRTP-Type-125" },
        { PT_UNDF_126,  "DynamicRTP-Type-126" },
        { PT_UNDF_127,  "DynamicRTP-Type-127" },

        { 0,        NULL },
};

value_string_ext rtp_payload_type_vals_ext = VALUE_STRING_EXT_INIT(rtp_payload_type_vals);

static const value_string rtp_payload_type_short_vals[] =
{
    { PT_PCMU,      "g711U" },
    { PT_1016,      "fs-1016" },
    { PT_G721,      "g721" },
    { PT_GSM,       "GSM" },
    { PT_G723,      "g723" },
    { PT_DVI4_8000, "DVI4 8k" },
    { PT_DVI4_16000, "DVI4 16k" },
    { PT_LPC,       "Exp. from Xerox PARC" },
    { PT_PCMA,      "g711A" },
    { PT_G722,      "g722" },
    { PT_L16_STEREO, "16-bit audio, stereo" },
    { PT_L16_MONO,  "16-bit audio, monaural" },
    { PT_QCELP,     "Qualcomm" },
    { PT_CN,        "CN" },
    { PT_MPA,       "MPEG-I/II Audio"},
    { PT_G728,      "g728" },
    { PT_DVI4_11025, "DVI4 11k" },
    { PT_DVI4_22050, "DVI4 22k" },
    { PT_G729,      "g729" },
    { PT_CN_OLD,    "CN(old)" },
    { 20,               "Unassigned" },
    { 21,               "Unassigned" },
    { 22,               "Unassigned" },
    { 23,               "Unassigned" },
    { 24,               "Unassigned" },
    { PT_CELB,      "CellB" },
    { PT_JPEG,      "JPEG" },
    { 27,               "Unassigned" },
    { PT_NV,        "NV" },
    { 29,               "Unassigned" },
    { 30,               "Unassigned" },
    { PT_H261,      "h261" },
    { PT_MPV,       "MPEG-I/II Video"},
    { PT_MP2T,      "MPEG-II streams"},
    { PT_H263,      "h263" },
/* 35-71     Unassigned  */
    { 35,               "Unassigned" },
    { 36,               "Unassigned" },
    { 37,               "Unassigned" },
    { 38,               "Unassigned" },
    { 39,               "Unassigned" },
    { 40,               "Unassigned" },
    { 41,               "Unassigned" },
    { 42,               "Unassigned" },
    { 43,               "Unassigned" },
    { 44,               "Unassigned" },
    { 45,               "Unassigned" },
    { 46,               "Unassigned" },
    { 47,               "Unassigned" },
    { 48,               "Unassigned" },
    { 49,               "Unassigned" },
    { 50,               "Unassigned" },
    { 51,               "Unassigned" },
    { 52,               "Unassigned" },
    { 53,               "Unassigned" },
    { 54,               "Unassigned" },
    { 55,               "Unassigned" },
    { 56,               "Unassigned" },
    { 57,               "Unassigned" },
    { 58,               "Unassigned" },
    { 59,               "Unassigned" },
    { 60,               "Unassigned" },
    { 61,               "Unassigned" },
    { 62,               "Unassigned" },
    { 63,               "Unassigned" },
    { 64,               "Unassigned" },
    { 65,               "Unassigned" },
    { 66,               "Unassigned" },
    { 67,               "Unassigned" },
    { 68,               "Unassigned" },
    { 69,               "Unassigned" },
    { 70,               "Unassigned" },
    { 71,               "Unassigned" },
/* 72-76     Reserved for RTCP conflict avoidance  - [RFC3551] */
    { 72,               "Reserved for RTCP conflict avoidance" },
    { 73,               "Reserved for RTCP conflict avoidance" },
    { 74,               "Reserved for RTCP conflict avoidance" },
    { 75,               "Reserved for RTCP conflict avoidance" },
    { 76,               "Reserved for RTCP conflict avoidance" },
/* 77-95     Unassigned      ? */
    { 77,               "Unassigned" },
    { 78,               "Unassigned" },
    { 79,               "Unassigned" },
    { 80,               "Unassigned" },
    { 81,               "Unassigned" },
    { 82,               "Unassigned" },
    { 83,               "Unassigned" },
    { 84,               "Unassigned" },
    { 85,               "Unassigned" },
    { 86,               "Unassigned" },
    { 87,               "Unassigned" },
    { 88,               "Unassigned" },
    { 89,               "Unassigned" },
    { 90,               "Unassigned" },
    { 91,               "Unassigned" },
    { 92,               "Unassigned" },
    { 93,               "Unassigned" },
    { 94,               "Unassigned" },
    { 95,               "Unassigned" },
    /* Short RTP types */
    { PT_UNDF_96,   "RTPType-96" },
    { PT_UNDF_97,   "RTPType-97" },
    { PT_UNDF_98,   "RTPType-98" },
    { PT_UNDF_99,   "RTPType-99" },
    { PT_UNDF_100,  "RTPType-100" },
    { PT_UNDF_101,  "RTPType-101" },
    { PT_UNDF_102,  "RTPType-102" },
    { PT_UNDF_103,  "RTPType-103" },
    { PT_UNDF_104,  "RTPType-104" },
    { PT_UNDF_105,  "RTPType-105" },
    { PT_UNDF_106,  "RTPType-106" },
    { PT_UNDF_107,  "RTPType-107" },
    { PT_UNDF_108,  "RTPType-108" },
    { PT_UNDF_109,  "RTPType-109" },
    { PT_UNDF_110,  "RTPType-110" },
    { PT_UNDF_111,  "RTPType-111" },
    { PT_UNDF_112,  "RTPType-112" },
    { PT_UNDF_113,  "RTPType-113" },
    { PT_UNDF_114,  "RTPType-114" },
    { PT_UNDF_115,  "RTPType-115" },
    { PT_UNDF_116,  "RTPType-116" },
    { PT_UNDF_117,  "RTPType-117" },
    { PT_UNDF_118,  "RTPType-118" },
    { PT_UNDF_119,  "RTPType-119" },
    { PT_UNDF_120,  "RTPType-120" },
    { PT_UNDF_121,  "RTPType-121" },
    { PT_UNDF_122,  "RTPType-122" },
    { PT_UNDF_123,  "RTPType-123" },
    { PT_UNDF_124,  "RTPType-124" },
    { PT_UNDF_125,  "RTPType-125" },
    { PT_UNDF_126,  "RTPType-126" },
    { PT_UNDF_127,  "RTPType-127" },

    { 0,            NULL },
};
value_string_ext rtp_payload_type_short_vals_ext = VALUE_STRING_EXT_INIT(rtp_payload_type_short_vals);

#if 0
static const value_string srtp_encryption_alg_vals[] =
{
    { SRTP_ENC_ALG_NULL,    "Null Encryption" },
    { SRTP_ENC_ALG_AES_CM,  "AES-128 Counter Mode" },
    { SRTP_ENC_ALG_AES_F8,  "AES-128 F8 Mode" },
    { 0, NULL },
};

static const value_string srtp_auth_alg_vals[] =
{
    { SRTP_AUTH_ALG_NONE,       "No Authentication" },
    { SRTP_AUTH_ALG_HMAC_SHA1,  "HMAC-SHA1" },
    { 0, NULL },
};
#endif

#ifdef DEBUG_CONVERSATION
/* Called for each entry in the rtp_dyn_payload hash table. */
static void
rtp_dyn_payload_table_foreach_func(gpointer key, gpointer value, gpointer user_data _U_) {
    guint pt = GPOINTER_TO_UINT(key);
    encoding_name_and_rate_t *encoding = (encoding_name_and_rate_t*) value;

    DPRINT2(("pt=%d", pt));
    if (encoding) {
        DPRINT2(("encoding_name=%s",
                encoding->encoding_name ? encoding->encoding_name : "NULL"));
        DPRINT2(("sample_rate=%d", encoding->sample_rate));
    } else {
        DPRINT2(("encoding=NULL"));
    }
}

void
rtp_dump_dyn_payload(rtp_dyn_payload_t *rtp_dyn_payload) {
    DPRINT2(("rtp_dyn_payload hash table contents:"));
    DINDENT();
        if (!rtp_dyn_payload) {
            DPRINT2(("null pointer to rtp_dyn_payload"));
            DENDENT();
            return;
        }
        DPRINT2(("ref_count=%" G_GSIZE_FORMAT, rtp_dyn_payload->ref_count));
        if (!rtp_dyn_payload->table) {
            DPRINT2(("null rtp_dyn_payload table"));
            DENDENT();
            return;
        }
        if (g_hash_table_size(rtp_dyn_payload->table) == 0) {
            DPRINT2(("rtp_dyn_payload has no entries"));
        } else {
            g_hash_table_foreach(rtp_dyn_payload->table, rtp_dyn_payload_table_foreach_func, NULL);
        }
    DENDENT();
}
#endif /* DEBUG_CONVERSATION */

/* initialisation routine */
static void
rtp_fragment_init(void)
{
    reassembly_table_init(&rtp_reassembly_table,
                  &addresses_reassembly_table_functions);
}

static void
rtp_fragment_cleanup(void)
{
    reassembly_table_destroy(&rtp_reassembly_table);
}

/* A single hash table to hold pointers to all the rtp_dyn_payload_t's we create/destroy.
   This is necessary because we need to g_hash_table_destroy() them, either individually or
   all at once at the end of the wmem file scope. Since rtp_dyn_payload_free() removes them
   individually, we need to remove those then; and when the file scope is over, we have a
   single registered callback walk this GHashTable and destroy each member as well as this
   GHashTable.
 */
static GHashTable *rtp_dyn_payloads = NULL;

/* the following is the GDestroyNotify function used when the individual rtp_dyn_payload_t
   GHashTables are destroyed */
static void
rtp_dyn_payload_value_destroy(gpointer data)
{
    encoding_name_and_rate_t *encoding_name_and_rate_pt = (encoding_name_and_rate_t*) data;
    wmem_free(wmem_file_scope(), encoding_name_and_rate_pt->encoding_name);
    wmem_free(wmem_file_scope(), encoding_name_and_rate_pt);
}

/* this gets called by wmem_rtp_dyn_payload_destroy_cb */
static gboolean
rtp_dyn_payloads_table_steal_func(gpointer key _U_, gpointer value, gpointer user_data _U_)
{
    rtp_dyn_payload_t *rtp_dyn_payload = (rtp_dyn_payload_t *)value;

#ifdef DEBUG_CONVERSATION
    DPRINT(("about to steal_all and destroy the following:"));
    DINDENT();
    rtp_dump_dyn_payload(rtp_dyn_payload);
    DENDENT();
#endif

    if (rtp_dyn_payload->ref_count == 0) {
        /* this shouldn't happen */
        DPRINT(("rtp_dyn_payload cannot be free'd because it should already have been!\n"));
    }
    else if (rtp_dyn_payload->table) {
        /* each member was created with a wmem file scope, so there's no point in calling the
           destroy functions for the GHashTable entries, so we steal them instead */
        g_hash_table_steal_all(rtp_dyn_payload->table);
        g_hash_table_destroy(rtp_dyn_payload->table);
    }

    return TRUE;
}

/* the following is used as the wmem callback to destroy *all* alive rtp_dyn_payload_t's,
   which are pointed to by the single rtp_dyn_payloads GHashTable above.
 */
static gboolean
wmem_rtp_dyn_payload_destroy_cb(wmem_allocator_t *allocator _U_, wmem_cb_event_t event _U_,
        void *user_data _U_)
{
    DPRINT(("destroying %u remaining rtp_dyn_payload_t's", g_hash_table_size(rtp_dyn_payloads)));

    /* each member was created with a wmem file scope, so there's no point in calling the
       destroy functions for the GHashTable entries, so we steal them instead */
    g_hash_table_foreach_steal(rtp_dyn_payloads, rtp_dyn_payloads_table_steal_func, NULL);
    g_hash_table_destroy(rtp_dyn_payloads);
    rtp_dyn_payloads = NULL;

    /* remove this callback? */
    return FALSE;
}

/* the following initializes the single GHashTable - this is invoked as an init_routine,
   but those are called both at init and cleanup times, and the cleanup time is before
   wmem scope is exited, so we ignore this if rtp_dyn_payloads is not NULL.
 */
static void
rtp_dyn_payloads_init(void)
{
    if (rtp_dyn_payloads == NULL) {
        rtp_dyn_payloads = g_hash_table_new(NULL, NULL);
        wmem_register_callback(wmem_file_scope(), wmem_rtp_dyn_payload_destroy_cb, NULL);
    }
}

/* creates a new hashtable and sets ref_count to 1, returning the newly created object */
rtp_dyn_payload_t* rtp_dyn_payload_new(void)
{
    /* create the new entry */
    rtp_dyn_payload_t * rtp_dyn_payload = wmem_new(wmem_file_scope(), rtp_dyn_payload_t);
    rtp_dyn_payload->table = g_hash_table_new_full(NULL, NULL, NULL, rtp_dyn_payload_value_destroy);
    rtp_dyn_payload->ref_count = 1;

    /* now put it in our single rtp_dyn_payloads GHashTable */
    g_hash_table_insert(rtp_dyn_payloads, rtp_dyn_payload, rtp_dyn_payload);

    return rtp_dyn_payload;
}

static rtp_dyn_payload_t*
rtp_dyn_payload_ref(rtp_dyn_payload_t *rtp_dyn_payload)
{
    if (rtp_dyn_payload) {
        rtp_dyn_payload->ref_count++;
    }
    return rtp_dyn_payload;
}

/* Inserts the given payload type key, for the encoding name and sample rate, into the hash table.
   This makes copies of the encoding name, scoped to the life of the capture file or sooner if
   rtp_dyn_payload_free is called. */
void
rtp_dyn_payload_insert(rtp_dyn_payload_t *rtp_dyn_payload,
                       const guint pt,
                       const gchar* encoding_name,
                       const int sample_rate)
{
    if (rtp_dyn_payload && rtp_dyn_payload->table) {
        encoding_name_and_rate_t *encoding_name_and_rate_pt =
                    wmem_new(wmem_file_scope(), encoding_name_and_rate_t);
        encoding_name_and_rate_pt->encoding_name = wmem_strdup(wmem_file_scope(), encoding_name);
        encoding_name_and_rate_pt->sample_rate = sample_rate;
        g_hash_table_insert(rtp_dyn_payload->table, GUINT_TO_POINTER(pt), encoding_name_and_rate_pt);
    }
}

/* Replaces the given payload type key in the hash table, with the encoding name and sample rate.
   This makes copies of the encoding name, scoped to the life of the capture file or sooner if
   rtp_dyn_payload_free is called. */
void
rtp_dyn_payload_replace(rtp_dyn_payload_t *rtp_dyn_payload,
                        const guint pt,
                        const gchar* encoding_name,
                        const int sample_rate)
{
    if (rtp_dyn_payload && rtp_dyn_payload->table) {
        encoding_name_and_rate_t *encoding_name_and_rate_pt =
                    wmem_new(wmem_file_scope(), encoding_name_and_rate_t);
        encoding_name_and_rate_pt->encoding_name = wmem_strdup(wmem_file_scope(), encoding_name);
        encoding_name_and_rate_pt->sample_rate = sample_rate;
        g_hash_table_replace(rtp_dyn_payload->table, GUINT_TO_POINTER(pt), encoding_name_and_rate_pt);
    }
}

/* removes the given payload type */
gboolean
rtp_dyn_payload_remove(rtp_dyn_payload_t *rtp_dyn_payload, const guint pt)
{
    return (rtp_dyn_payload && rtp_dyn_payload->table &&
            g_hash_table_remove(rtp_dyn_payload->table, GUINT_TO_POINTER(pt)));
}

/* retrieves the encoding name for the given payload type */
const gchar*
rtp_dyn_payload_get_name(rtp_dyn_payload_t *rtp_dyn_payload, const guint pt)
{
    encoding_name_and_rate_t *encoding_name_and_rate_pt;

    if (!rtp_dyn_payload || !rtp_dyn_payload->table) return NULL;

    encoding_name_and_rate_pt = (encoding_name_and_rate_t*)g_hash_table_lookup(rtp_dyn_payload->table,
                                                                               GUINT_TO_POINTER(pt));

    return (encoding_name_and_rate_pt ? encoding_name_and_rate_pt->encoding_name : NULL);
}

/* retrieves the encoding name and sample rate for the given payload type, returning TRUE if
   successful, else FALSE. The encoding string pointed to is only valid until the entry is
   replaced, removed, or the hash table is destroyed, so duplicate it if you need it long. */
gboolean
rtp_dyn_payload_get_full(rtp_dyn_payload_t *rtp_dyn_payload, const guint pt,
                         const gchar **encoding_name, int *sample_rate)
{
    encoding_name_and_rate_t *encoding_name_and_rate_pt;
    *encoding_name = NULL;
    *sample_rate = 0;

    if (!rtp_dyn_payload || !rtp_dyn_payload->table) return FALSE;

    encoding_name_and_rate_pt = (encoding_name_and_rate_t*)g_hash_table_lookup(rtp_dyn_payload->table,
                                                                               GUINT_TO_POINTER(pt));

    if (encoding_name_and_rate_pt) {
        *encoding_name = encoding_name_and_rate_pt->encoding_name;
        *sample_rate = encoding_name_and_rate_pt->sample_rate;
    }

    return (encoding_name_and_rate_pt != NULL);
}

/* Free's and destroys the dyn_payload hash table; internally this decrements the ref_count
   and only free's it if the ref_count == 0. */
void
rtp_dyn_payload_free(rtp_dyn_payload_t *rtp_dyn_payload)
{
    if (!rtp_dyn_payload) return;

    if (rtp_dyn_payload->ref_count > 0)
        --(rtp_dyn_payload->ref_count);

    if (rtp_dyn_payload->ref_count == 0) {

#ifdef DEBUG_CONVERSATION
        DPRINT(("free'ing the following rtp_dyn_payload:"));
        DINDENT();
        rtp_dump_dyn_payload(rtp_dyn_payload);
        DENDENT();
#endif

        /* remove it from the single rtp_dyn_payloads GHashTable */
        if (!g_hash_table_remove(rtp_dyn_payloads, rtp_dyn_payload)) {
            DPRINT(("rtp_dyn_payload not found in rtp_dyn_payloads table to remove!"));
        }

        /* destroy the table GHashTable in it - this automatically deletes the
           members too, because we used destroy function callbacks */
        if (rtp_dyn_payload->table)
            g_hash_table_destroy(rtp_dyn_payload->table);

        /* free the object itself */
        wmem_free(wmem_file_scope(), rtp_dyn_payload);
    }
}

void
bluetooth_add_address(packet_info *pinfo, address *addr, guint32 stream_number,
         const gchar *setup_method, guint32 setup_frame_number,
         gboolean is_video, void *data)
{
    address null_addr;
    conversation_t* p_conv;
    struct _rtp_conversation_info *p_conv_data = NULL;
    /*
     * If this isn't the first time this packet has been processed,
     * we've already done this work, so we don't need to do it
     * again.
     */
    if ((pinfo->fd->flags.visited) || (rtp_handle == NULL))
    {
        return;
    }

    clear_address(&null_addr);

    /*
     * Check if the ip address and port combination is not
     * already registered as a conversation.
     */
    p_conv = find_conversation(setup_frame_number, addr, &null_addr, PT_BLUETOOTH, stream_number, stream_number,
                   NO_ADDR_B | NO_PORT_B);

    /*
     * If not, create a new conversation.
     */
    if (!p_conv || p_conv->setup_frame != setup_frame_number) {
        p_conv = conversation_new(setup_frame_number, addr, &null_addr, PT_BLUETOOTH, stream_number, stream_number,
                   NO_ADDR2 | NO_PORT2);
    }

    /* Set dissector */
    conversation_set_dissector(p_conv, rtp_handle);

    /*
     * Check if the conversation has data associated with it.
     */
    p_conv_data = (struct _rtp_conversation_info *)conversation_get_proto_data(p_conv, proto_rtp);

    /*
     * If not, add a new data item.
     */
    if (! p_conv_data) {
        /* Create conversation data */
        p_conv_data = wmem_new(wmem_file_scope(), struct _rtp_conversation_info);
        p_conv_data->rtp_dyn_payload = NULL;

        /* start this at 0x10000 so that we cope gracefully with the
         * first few packets being out of order (hence 0,65535,1,2,...)
         */
        p_conv_data->extended_seqno = 0x10000;
        p_conv_data->rtp_conv_info = wmem_new(wmem_file_scope(), rtp_private_conv_info);
        p_conv_data->rtp_conv_info->multisegment_pdus = wmem_tree_new(wmem_file_scope());
        conversation_add_proto_data(p_conv, proto_rtp, p_conv_data);

        if (is_video) {
            p_conv_data->bta2dp_info = NULL;
            p_conv_data->btvdp_info = (btvdp_codec_info_t *) wmem_memdup(wmem_file_scope(), data, sizeof(btvdp_codec_info_t));
        } else {
            p_conv_data->bta2dp_info = (bta2dp_codec_info_t *) wmem_memdup(wmem_file_scope(), data, sizeof(bta2dp_codec_info_t));
            p_conv_data->btvdp_info = NULL;
        }
    }

    /*
     * Update the conversation data.
     */
    /* Free the hash if already exists */
    rtp_dyn_payload_free(p_conv_data->rtp_dyn_payload);

    g_strlcpy(p_conv_data->method, setup_method, MAX_RTP_SETUP_METHOD_SIZE+1);
    p_conv_data->frame_number = setup_frame_number;
    p_conv_data->is_video = is_video;
    p_conv_data->rtp_dyn_payload = NULL;
    p_conv_data->srtp_info = NULL;
}

/* Set up an SRTP conversation */
void
srtp_add_address(packet_info *pinfo, address *addr, int port, int other_port,
         const gchar *setup_method, guint32 setup_frame_number,
         gboolean is_video _U_, rtp_dyn_payload_t *rtp_dyn_payload,
         struct srtp_info *srtp_info)
{
    address null_addr;
    conversation_t* p_conv;
    struct _rtp_conversation_info *p_conv_data;

    /*
     * If this isn't the first time this packet has been processed,
     * we've already done this work, so we don't need to do it
     * again.
     */
    if ((pinfo->fd->flags.visited) || (rtp_handle == NULL))
    {
        return;
    }

    DPRINT(("#%u: %srtp_add_address(%s, %u, %u, %s, %u)",
            pinfo->num, (srtp_info)?"s":"", address_to_str(wmem_packet_scope(), addr), port,
            other_port, setup_method, setup_frame_number));
    DINDENT();

    clear_address(&null_addr);

    /*
     * Check if the ip address and port combination is not
     * already registered as a conversation.
     */
    p_conv = find_conversation(setup_frame_number, addr, &null_addr, PT_UDP, port, other_port,
                   NO_ADDR_B | (!other_port ? NO_PORT_B : 0));

    DENDENT();
    DPRINT(("did %sfind conversation", p_conv?"":"NOT "));

    /*
     * If not, create a new conversation.
     */
    if (!p_conv || p_conv->setup_frame != setup_frame_number) {
        p_conv = conversation_new(setup_frame_number, addr, &null_addr, PT_UDP,
                                  (guint32)port, (guint32)other_port,
                      NO_ADDR2 | (!other_port ? NO_PORT2 : 0));
    }

    /* Set dissector */
    conversation_set_dissector(p_conv, rtp_handle);

    /*
     * Check if the conversation has data associated with it.
     */
    p_conv_data = (struct _rtp_conversation_info *)conversation_get_proto_data(p_conv, proto_rtp);

    /*
     * If not, add a new data item.
     */
    if (! p_conv_data) {
        DPRINT(("creating new conversation data"));

        /* Create conversation data */
        p_conv_data = wmem_new(wmem_file_scope(), struct _rtp_conversation_info);
        p_conv_data->rtp_dyn_payload = NULL;

        /* start this at 0x10000 so that we cope gracefully with the
         * first few packets being out of order (hence 0,65535,1,2,...)
         */
        p_conv_data->extended_seqno = 0x10000;
        p_conv_data->rtp_conv_info = wmem_new(wmem_file_scope(), rtp_private_conv_info);
        p_conv_data->rtp_conv_info->multisegment_pdus = wmem_tree_new(wmem_file_scope());
        DINDENT();
        conversation_add_proto_data(p_conv, proto_rtp, p_conv_data);
        DENDENT();
    }
#ifdef DEBUG_CONVERSATION
    else {
        DPRINT(("conversation already exists"));
    }
#endif

    /*
     * Update the conversation data.
     */
    /* Free the hash if a different one already exists */
    if (p_conv_data->rtp_dyn_payload != rtp_dyn_payload) {
        rtp_dyn_payload_free(p_conv_data->rtp_dyn_payload);
        p_conv_data->rtp_dyn_payload = rtp_dyn_payload_ref(rtp_dyn_payload);
    } else {
        DPRINT(("passed-in rtp_dyn_payload is the same as in the conversation"));
    }

    g_strlcpy(p_conv_data->method, setup_method, MAX_RTP_SETUP_METHOD_SIZE+1);
    p_conv_data->frame_number = setup_frame_number;
    p_conv_data->is_video = is_video;
    p_conv_data->srtp_info = srtp_info;
    p_conv_data->bta2dp_info = NULL;
    p_conv_data->btvdp_info = NULL;
}

/* Set up an RTP conversation */
void
rtp_add_address(packet_info *pinfo, address *addr, int port, int other_port,
        const gchar *setup_method, guint32 setup_frame_number,
        gboolean is_video , rtp_dyn_payload_t *rtp_dyn_payload)
{
    srtp_add_address(pinfo, addr, port, other_port, setup_method, setup_frame_number, is_video, rtp_dyn_payload, NULL);
}

static gboolean
dissect_rtp_heur_common(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data, gboolean check_destport)
{
    guint8       octet1;
    unsigned int version;
    unsigned int offset = 0;

    /* Get the fields in the first octet */
    octet1 = tvb_get_guint8( tvb, offset );
    version = RTP_VERSION( octet1 );

    if (version == 0) {
        if (!(tvb_memeql(tvb, 4, "ZRTP", 4)))
        {
            call_dissector_only(zrtp_handle, tvb, pinfo, tree, NULL);
            return TRUE;
        } else {
            switch (global_rtp_version0_type) {
            case RTP0_STUN:
                return call_dissector_only(stun_heur_handle, tvb, pinfo, tree, NULL);
            case RTP0_CLASSICSTUN:
                return call_dissector_only(classicstun_heur_handle, tvb, pinfo, tree, NULL);

            case RTP0_T38:
                /* XXX: Should really be calling a heuristic dissector for T38 ??? */
                call_dissector_only(t38_handle, tvb, pinfo, tree, NULL);
                return TRUE;

            case RTP0_SPRT:
                call_dissector_only(sprt_handle, tvb, pinfo, tree, NULL);
                return TRUE;

            case RTP0_INVALID:

            default:
                return FALSE; /* Unknown or unsupported version */
            }
        }
    } else if (version != 2) {
        /* Unknown or unsupported version */
        return FALSE;
    }

    /* Was it sent to an even-numbered port? */
    if (check_destport && ((pinfo->destport % 2) != 0)) {
        return FALSE;
    }

    /* Create a conversation in case none exists so as to allow reassembly code to work */
    if (!find_conversation(pinfo->num, &pinfo->net_dst, &pinfo->net_src, pinfo->ptype,
                           pinfo->destport, pinfo->srcport, NO_ADDR2)) {
        conversation_t *p_conv;
        struct _rtp_conversation_info *p_conv_data;
        p_conv = conversation_new(pinfo->num, &pinfo->net_dst, &pinfo->net_src, pinfo->ptype,
                                  pinfo->destport, pinfo->srcport, NO_ADDR2);
        p_conv_data = (struct _rtp_conversation_info *)conversation_get_proto_data(p_conv, proto_rtp);
        if (! p_conv_data) {
            /* Create conversation data */
            p_conv_data = wmem_new(wmem_file_scope(), struct _rtp_conversation_info);
            p_conv_data->rtp_dyn_payload = NULL;
            p_conv_data->extended_seqno = 0x10000;
            p_conv_data->rtp_conv_info = wmem_new(wmem_file_scope(), rtp_private_conv_info);
            p_conv_data->rtp_conv_info->multisegment_pdus = wmem_tree_new(wmem_file_scope());
            conversation_add_proto_data(p_conv, proto_rtp, p_conv_data);
        }
        g_strlcpy(p_conv_data->method, "HEUR RTP", MAX_RTP_SETUP_METHOD_SIZE+1);
        p_conv_data->frame_number = pinfo->num;
        p_conv_data->is_video = FALSE;
        p_conv_data->srtp_info = NULL;
        p_conv_data->bta2dp_info = NULL;
        p_conv_data->btvdp_info = NULL;
    }
    dissect_rtp( tvb, pinfo, tree, data );
    return TRUE;
}

static gboolean
dissect_rtp_heur_udp( tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data )
{
    return dissect_rtp_heur_common(tvb, pinfo, tree, data, TRUE);
}

static gboolean
dissect_rtp_heur_app( tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data )
{
    return dissect_rtp_heur_common(tvb, pinfo, tree, data, FALSE);
}

/*
 * Process the payload of the RTP packet, hand it to the subdissector
 */
static void
process_rtp_payload(tvbuff_t *newtvb, packet_info *pinfo, proto_tree *tree,
            proto_tree *rtp_tree, unsigned int payload_type)
{
    struct _rtp_conversation_info *p_conv_data;
    gboolean found_match = FALSE;
    int payload_len;
    struct srtp_info *srtp_info;
    int offset = 0;

    payload_len = tvb_captured_length_remaining(newtvb, offset);

    /* first check if this is added as an SRTP stream - if so, don't try to dissector the payload data for now */
    p_conv_data = (struct _rtp_conversation_info *)p_get_proto_data(wmem_file_scope(), pinfo, proto_rtp, 0);
    if (p_conv_data && p_conv_data->srtp_info) {
        srtp_info = p_conv_data->srtp_info;
        payload_len -= srtp_info->mki_len + srtp_info->auth_tag_len;
#if 0
#error Currently the srtp_info structure contains no cipher data, see packet-sdp.c adding dummy_srtp_info structure
        if (p_conv_data->srtp_info->encryption_algorithm == SRTP_ENC_ALG_NULL) {
            if (rtp_tree)
                proto_tree_add_item(rtp_tree, hf_srtp_null_encrypted_payload, newtvb, offset, payload_len, ENC_NA);
        }
        else
#endif
        {
            if (rtp_tree)
                proto_tree_add_item(rtp_tree, hf_srtp_encrypted_payload, newtvb, offset, payload_len, ENC_NA);
            found_match = TRUE; /* use this flag to prevent dissection below */
        }
        offset += payload_len;

        if (srtp_info->mki_len) {
            proto_tree_add_item(rtp_tree, hf_srtp_mki, newtvb, offset, srtp_info->mki_len, ENC_NA);
            offset += srtp_info->mki_len;
        }

        if (srtp_info->auth_tag_len) {
            proto_tree_add_item(rtp_tree, hf_srtp_auth_tag, newtvb, offset, srtp_info->auth_tag_len, ENC_NA);
            /*offset += srtp_info->auth_tag_len;*/
        }
    } else if (p_conv_data && !p_conv_data->bta2dp_info && !p_conv_data->btvdp_info &&
            payload_type >= PT_UNDF_96 && payload_type <= PT_UNDF_127) {
        /* if the payload type is dynamic, we check if the conv is set and we look for the pt definition */
        if (p_conv_data->rtp_dyn_payload) {
            const gchar *payload_type_str = rtp_dyn_payload_get_name(p_conv_data->rtp_dyn_payload, payload_type);
            if (payload_type_str) {
                int len;
                len = dissector_try_string(rtp_dyn_pt_dissector_table,
                                   payload_type_str, newtvb, pinfo, tree, NULL);
                /* If payload type string set from conversation and
                 * no matching dissector found it's probably because no subdissector
                 * exists. Don't call the dissectors based on payload number
                 * as that'd probably be the wrong dissector in this case.
                 * Just add it as data.
                 */
                if(len == 0)
                    proto_tree_add_item( rtp_tree, hf_rtp_data, newtvb, 0, -1, ENC_NA );
                return;
            }

        }
    } else if (p_conv_data && p_conv_data->bta2dp_info) {
        tvbuff_t  *nexttvb;
        gint       suboffset = 0;

        found_match = TRUE;

        if (p_conv_data->bta2dp_info->content_protection_type == BTAVDTP_CONTENT_PROTECTION_TYPE_SCMS_T) {
            nexttvb = tvb_new_subset_length(newtvb, 0, 1);
             call_dissector(bta2dp_content_protection_header_scms_t, nexttvb, pinfo, tree);
            suboffset = 1;
        }

        nexttvb = tvb_new_subset_remaining(newtvb, suboffset);
        if (p_conv_data->bta2dp_info->codec_dissector)
            call_dissector_with_data(p_conv_data->bta2dp_info->codec_dissector, nexttvb, pinfo, tree, p_conv_data->bta2dp_info);
        else
            call_data_dissector(nexttvb, pinfo, tree);
    } else if (p_conv_data && p_conv_data->btvdp_info) {
        tvbuff_t  *nexttvb;
        gint       suboffset = 0;

        found_match = TRUE;

        if (p_conv_data->btvdp_info->content_protection_type == BTAVDTP_CONTENT_PROTECTION_TYPE_SCMS_T) {
            nexttvb = tvb_new_subset_length(newtvb, 0, 1);
            call_dissector(btvdp_content_protection_header_scms_t, nexttvb, pinfo, tree);
            suboffset = 1;
        }

        nexttvb = tvb_new_subset_remaining(newtvb, suboffset);
        if (p_conv_data->btvdp_info->codec_dissector)
            call_dissector_with_data(p_conv_data->btvdp_info->codec_dissector, nexttvb, pinfo, tree, p_conv_data->btvdp_info);
        else
            call_data_dissector(nexttvb, pinfo, tree);
    }

    /* if we don't found, it is static OR could be set static from the preferences */
    if (!found_match && !dissector_try_uint(rtp_pt_dissector_table, payload_type, newtvb, pinfo, tree))
        proto_tree_add_item( rtp_tree, hf_rtp_data, newtvb, 0, -1, ENC_NA );

}

/* Rtp payload reassembly
 *
 * This handles the reassembly of PDUs for higher-level protocols.
 *
 * We're a bit limited on how we can cope with out-of-order packets, because
 * we don't have any idea of where the datagram boundaries are. So if we see
 * packets A, C, B (all of which comprise a single datagram), we cannot know
 * that C should be added to the same datagram as A, until we come to B (which
 * may or may not actually be present...).
 *
 * What we end up doing in this case is passing A+B to the subdissector as one
 * datagram, and make out that a new one starts on C.
 */
static void
dissect_rtp_data(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
         proto_tree *rtp_tree, int offset, unsigned int data_len,
         unsigned int data_reported_len,
         unsigned int payload_type)
{
    tvbuff_t *newtvb;
    struct _rtp_conversation_info *p_conv_data;
    gboolean must_desegment = FALSE;
    rtp_private_conv_info *finfo = NULL;
    rtp_multisegment_pdu *msp;
    guint32 seqno;

    /* Retrieve RTPs idea of a converation */
    p_conv_data = (struct _rtp_conversation_info *)p_get_proto_data(wmem_file_scope(), pinfo, proto_rtp, 0);

    if(p_conv_data != NULL)
        finfo = p_conv_data->rtp_conv_info;

    if(finfo == NULL || !desegment_rtp) {
        /* Hand the whole lot off to the subdissector */
        newtvb = tvb_new_subset(tvb, offset, data_len, data_reported_len);
        process_rtp_payload(newtvb, pinfo, tree, rtp_tree, payload_type);
        return;
    }

    seqno = p_conv_data->extended_seqno;

    pinfo->can_desegment = 2;
    pinfo->desegment_offset = 0;
    pinfo->desegment_len = 0;

#ifdef DEBUG_FRAGMENTS
    g_debug("%d: RTP Part of convo %d(%p); seqno %d",
        pinfo->num,
        p_conv_data->frame_number, p_conv_data,
        seqno
        );
#endif

    /* look for a pdu which we might be extending */
    msp = (rtp_multisegment_pdu *)wmem_tree_lookup32_le(finfo->multisegment_pdus, seqno-1);

    if(msp && msp->startseq < seqno && msp->endseq >= seqno) {
        guint32 fid = msp->startseq;
        fragment_head *fd_head;

#ifdef DEBUG_FRAGMENTS
        g_debug("\tContinues fragment %d", fid);
#endif

        /* we always assume the datagram is complete; if this is the
         * first pass, that's our best guess, and if it's not, what we
         * say gets ignored anyway.
         */
        fd_head = fragment_add_seq(&rtp_reassembly_table,
                       tvb, offset, pinfo, fid, NULL,
                       seqno-msp->startseq, data_len,
                       FALSE, 0);

        newtvb = process_reassembled_data(tvb, offset, pinfo, "Reassembled RTP", fd_head,
                          &rtp_fragment_items, NULL, tree);

#ifdef DEBUG_FRAGMENTS
        g_debug("\tFragment Coalesced; fd_head=%p, newtvb=%p (len %d)", fd_head, newtvb,
            newtvb?tvb_reported_length(newtvb):0);
#endif

        if(newtvb != NULL) {
            /* Hand off to the subdissector */
            process_rtp_payload(newtvb, pinfo, tree, rtp_tree, payload_type);

            /*
             * Check to see if there were any complete fragments within the chunk
             */
            if( pinfo->desegment_len )
            {
                if (pinfo->desegment_offset == 0) {
#ifdef DEBUG_FRAGMENTS
                    g_debug("\tNo complete pdus in payload" );
#endif
                    /* Mark the fragments as not complete yet */
                    fragment_set_partial_reassembly(&rtp_reassembly_table,
                                    pinfo, fid, NULL);

                    /* we must need another segment */
                    msp->endseq = MIN(msp->endseq, seqno) + 1;

                }

                /* the higher-level dissector has asked for some more data - ie,
                   the end of this segment does not coincide with the end of a
                   higher-level PDU. */
                must_desegment = TRUE;
            }

        }

    }
    else
    {
        /*
         * The segment is not the continuation of a fragmented segment
         * so process it as normal
         */
#ifdef DEBUG_FRAGMENTS
        g_debug("\tRTP non-fragment payload");
#endif
        newtvb = tvb_new_subset( tvb, offset, data_len, data_reported_len );

        /* Hand off to the subdissector */
        process_rtp_payload(newtvb, pinfo, tree, rtp_tree, payload_type);

        if(pinfo->desegment_len) {
            /* the higher-level dissector has asked for some more data - ie,
               the end of this segment does not coincide with the end of a
               higher-level PDU. */
            must_desegment = TRUE;
        }
    }

    /*
     * There were bytes left over that the higher protocol couldn't dissect so save them
     */
    if(must_desegment)
    {
        guint32 deseg_offset = pinfo->desegment_offset;
        guint32 frag_len = tvb_reported_length_remaining(newtvb, deseg_offset);
        fragment_head *fd_head;

#ifdef DEBUG_FRAGMENTS
        g_debug("\tRTP Must Desegment: tvb_len=%d ds_len=%d %d frag_len=%d ds_off=%d",
            tvb_reported_length(newtvb),
            pinfo->desegment_len,
            pinfo->fd->flags.visited,
            frag_len,
            deseg_offset);
#endif
        /* allocate a new msp for this pdu */
        if (!PINFO_FD_VISITED(pinfo)) {
            msp = wmem_new(wmem_file_scope(), rtp_multisegment_pdu);
            msp->startseq = seqno;
            msp->endseq = seqno+1;
            wmem_tree_insert32(finfo->multisegment_pdus, seqno, msp);
        }

        /*
         * Add the fragment to the fragment table
         */
        fd_head = fragment_add_seq(&rtp_reassembly_table,
                       newtvb, deseg_offset, pinfo, seqno, NULL, 0, frag_len,
                       TRUE, 0);

        if(fd_head != NULL)
        {
            if( fd_head->reassembled_in != 0 && !(fd_head->flags & FD_PARTIAL_REASSEMBLY) )
            {
                proto_item *rtp_tree_item;
                rtp_tree_item = proto_tree_add_uint( tree, hf_rtp_reassembled_in,
                                     newtvb, deseg_offset, tvb_reported_length_remaining(newtvb, deseg_offset),
                                     fd_head->reassembled_in);
                PROTO_ITEM_SET_GENERATED(rtp_tree_item);
#ifdef DEBUG_FRAGMENTS
                g_debug("\tReassembled in %d", fd_head->reassembled_in);
#endif
            }
            else if (fd_head->reassembled_in == 0)
            {
#ifdef DEBUG_FRAGMENTS
                g_debug("\tUnfinished fragment");
#endif
                /* this fragment is never reassembled */
                proto_tree_add_expert(tree, pinfo, &ei_rtp_fragment_unfinished, tvb, deseg_offset, -1);
            }
        }
        else
        {
            /*
             * This fragment was the first fragment in a new entry in the
             * frag_table; we don't yet know where it is reassembled
             */
#ifdef DEBUG_FRAGMENTS
            g_debug("\tnew pdu");
#endif
        }

        if( pinfo->desegment_offset == 0 )
        {
            col_set_str(pinfo->cinfo, COL_PROTOCOL, "RTP");
            col_set_str(pinfo->cinfo, COL_INFO, "[RTP segment of a reassembled PDU]");
        }
    }

    pinfo->can_desegment = 0;
    pinfo->desegment_offset = 0;
    pinfo->desegment_len = 0;
}



static int
dissect_rtp_rfc2198(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
    gint offset = 0;
    int cnt;
    gboolean hdr_follow = TRUE;
    proto_tree *rfc2198_tree;
    rfc2198_hdr *hdr_last;
    rfc2198_hdr *hdr_chain = NULL;
    struct _rtp_conversation_info *p_conv_data;

    /* Retrieve RTPs idea of a converation */
    p_conv_data = (struct _rtp_conversation_info *)p_get_proto_data(wmem_file_scope(), pinfo, proto_rtp, 0);

    /* Add try to RFC 2198 data */
    rfc2198_tree = proto_tree_add_subtree(tree, tvb, offset, -1, ett_rtp_rfc2198, NULL, "RFC 2198: Redundant Audio Data");

    hdr_last = NULL;
    cnt = 0;
    while (hdr_follow) {
        proto_item *ti;
        proto_tree *rfc2198_hdr_tree;
        const gchar *payload_type_str;
        rfc2198_hdr *hdr_new;
        guint8 octet1;

        cnt++;
        payload_type_str = NULL;

        /* Allocate and fill in header */
        hdr_new = wmem_new(wmem_packet_scope(), rfc2198_hdr);
        hdr_new->next = NULL;
        octet1 = tvb_get_guint8(tvb, offset);
        hdr_new->pt = RTP_PAYLOAD_TYPE(octet1);
        hdr_follow = (octet1 & 0x80);

        /* if it is dynamic payload, let use the conv data to see if it is defined */
        if ((hdr_new->pt > 95) && (hdr_new->pt < 128)) {
            if (p_conv_data && p_conv_data->rtp_dyn_payload){
                payload_type_str = rtp_dyn_payload_get_name(p_conv_data->rtp_dyn_payload, hdr_new->pt);
            }
        }
        /* Add a subtree for this header and add items */
        rfc2198_hdr_tree = proto_tree_add_subtree_format(rfc2198_tree, tvb, offset, (hdr_follow)?4:1,
                                    ett_rtp_rfc2198_hdr, &ti, "Header %u", cnt);
        proto_tree_add_item(rfc2198_hdr_tree, hf_rtp_rfc2198_follow, tvb, offset, 1, ENC_BIG_ENDIAN );
        proto_tree_add_uint_format_value(rfc2198_hdr_tree, hf_rtp_payload_type, tvb,
            offset, 1, octet1, "%s (%u)",
            payload_type_str ? payload_type_str : val_to_str_ext_const(hdr_new->pt, &rtp_payload_type_vals_ext, "Unknown"),
            hdr_new->pt);
        proto_item_append_text(ti, ": PT=%s",
                       payload_type_str ? payload_type_str :
                                          val_to_str_ext(hdr_new->pt, &rtp_payload_type_vals_ext, "Unknown (%u)"));
        offset += 1;

        /* Timestamp offset and block length don't apply to last header */
        if (hdr_follow) {
            proto_tree_add_item(rfc2198_hdr_tree, hf_rtp_rfc2198_tm_off, tvb, offset, 2, ENC_BIG_ENDIAN );
            proto_tree_add_item(rfc2198_hdr_tree, hf_rtp_rfc2198_bl_len, tvb, offset + 1, 2, ENC_BIG_ENDIAN );
            hdr_new->len = tvb_get_ntohs(tvb, offset + 1) & 0x03FF;
            proto_item_append_text(ti, ", len=%u", hdr_new->len);
            offset += 3;
        } else {
            hdr_new->len = -1;
            hdr_follow = FALSE;
        }

        if (hdr_last) {
            hdr_last->next = hdr_new;
        } else {
            hdr_chain = hdr_new;
        }
        hdr_last = hdr_new;
    }

    /* Dissect each data block according to the header info */
    hdr_last = hdr_chain;
    while (hdr_last) {
        hdr_last->offset = offset;
        if (!hdr_last->next) {
            hdr_last->len = tvb_reported_length_remaining(tvb, offset);
        }
        dissect_rtp_data(tvb, pinfo, tree, rfc2198_tree, hdr_last->offset, hdr_last->len, hdr_last->len, hdr_last->pt);
        offset += hdr_last->len;
        hdr_last = hdr_last->next;
    }
    return tvb_captured_length(tvb);
}

static void
dissect_rtp_hext_rfc5285_onebyte( tvbuff_t *tvb, packet_info *pinfo,
        proto_tree *rtp_hext_tree )
{
    proto_tree *rtp_hext_rfc5285_tree = NULL;
    guint ext_offset = 0;

    while (ext_offset < tvb_captured_length (tvb)) {
        guint8    ext_hdr_hdr;
        guint8    ext_id;
        guint8    ext_length;
        guint     start_ext_offset;
        tvbuff_t *subtvb;

        /* Skip bytes with the value 0, they are padding */
        start_ext_offset = ext_offset;
        while (tvb_get_guint8 (tvb, ext_offset) == 0) {
            ext_offset ++;
            if (ext_offset >= tvb_captured_length (tvb))
                return;
        }

        /* Add padding */
        if (ext_offset > start_ext_offset)
            proto_tree_add_item(rtp_hext_tree, hf_rtp_padding_data, tvb, start_ext_offset, ext_offset-start_ext_offset, ENC_NA );

        ext_hdr_hdr = tvb_get_guint8 (tvb, ext_offset);
        ext_id = ext_hdr_hdr >> 4;

        /* 15 is for future extensibility, ignore length, etc and stop processing packet if it shows up */
        if (ext_id == 15)
            return;

        ext_length = (ext_hdr_hdr & 0x0F) + 1;

        /* Exit on malformed extension headers */
        if (ext_offset + ext_length + 1 > tvb_captured_length (tvb)) {
            return;
        }

        if (rtp_hext_tree) {
            rtp_hext_rfc5285_tree = proto_tree_add_subtree(rtp_hext_tree, tvb, ext_offset, ext_length + 1,
                                            ett_hdr_ext_rfc5285, NULL, "RFC 5285 Header Extension (One-Byte Header)");

            proto_tree_add_uint( rtp_hext_rfc5285_tree, hf_rtp_ext_rfc5285_id, tvb, ext_offset, 1, ext_id);
            proto_tree_add_uint( rtp_hext_rfc5285_tree, hf_rtp_ext_rfc5285_length, tvb, ext_offset, 1, ext_length);
        }
        ext_offset ++;

        subtvb = tvb_new_subset_length(tvb, ext_offset, ext_length);
        if (!dissector_try_uint (rtp_hdr_ext_rfc5285_dissector_table, ext_id, subtvb, pinfo, rtp_hext_rfc5285_tree)) {
            if (rtp_hext_tree)
                proto_tree_add_item(rtp_hext_rfc5285_tree, hf_rtp_ext_rfc5285_data, subtvb, 0, ext_length, ENC_NA );
        }

        ext_offset += ext_length;
    }
}


static void
dissect_rtp_hext_rfc5285_twobytes(tvbuff_t *parent_tvb, guint id_offset,
        guint8 id, tvbuff_t *tvb, packet_info *pinfo, proto_tree *rtp_hext_tree)
{
    proto_tree *rtp_hext_rfc5285_tree = NULL;
    guint ext_offset = 0, start_ext_offset;

    while (ext_offset + 2 < tvb_captured_length (tvb)) {
        guint8    ext_id;
        guint8    ext_length;
        tvbuff_t *subtvb;

        /* Skip bytes with the value 0, they are padding */
        start_ext_offset = ext_offset;
        while (tvb_get_guint8 (tvb, ext_offset) == 0) {
            if (ext_offset + 2 >= tvb_captured_length (tvb))
                return;
            ext_offset ++;
        }
        /* Add padding */
        if (ext_offset > start_ext_offset)
            proto_tree_add_item(rtp_hext_tree, hf_rtp_padding_data, tvb, start_ext_offset, ext_offset-start_ext_offset, ENC_NA );

        ext_id = tvb_get_guint8 (tvb, ext_offset);
        ext_length = tvb_get_guint8 (tvb, ext_offset + 1);

        if (rtp_hext_tree) {
            rtp_hext_rfc5285_tree = proto_tree_add_subtree(rtp_hext_tree, tvb, ext_offset, ext_length + 2,
                                    ett_hdr_ext_rfc5285, NULL, "RFC 5285 Header Extension (Two-Byte Header)");

            proto_tree_add_uint( rtp_hext_rfc5285_tree, hf_rtp_ext_rfc5285_appbits, parent_tvb, id_offset + 1, 1, id & 0x000F);
            proto_tree_add_uint( rtp_hext_rfc5285_tree, hf_rtp_ext_rfc5285_id, tvb, ext_offset, 1, ext_id);
            proto_tree_add_uint( rtp_hext_rfc5285_tree, hf_rtp_ext_rfc5285_length, tvb, ext_offset + 1, 1, ext_length);
        }

        ext_offset += 2;

        subtvb = tvb_new_subset_length(tvb, ext_offset, ext_length);
        if (ext_length && !dissector_try_uint (rtp_hdr_ext_rfc5285_dissector_table, ext_id, subtvb, pinfo, rtp_hext_rfc5285_tree)) {
            proto_tree_add_item(rtp_hext_rfc5285_tree, hf_rtp_ext_rfc5285_data, subtvb, 0, ext_length, ENC_NA );
        }

        ext_offset += ext_length;
    }
}

static gint
dissect_rtp( tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    proto_item *ti            = NULL;
    proto_tree *volatile rtp_tree = NULL;
    guint8      octet1, octet2;
    unsigned int version;
    gboolean    padding_set;
    gboolean    extension_set;
    unsigned int csrc_count;
    gboolean    marker_set;
    unsigned int payload_type;
    const gchar *payload_type_str = NULL;
    gboolean    is_srtp = FALSE;
    unsigned int i;
    gint        length, reported_length;
    int         data_len;
    volatile unsigned int offset = 0;
    guint16     seq_num;
    guint32     timestamp;
    guint32     sync_src;
    struct _rtp_conversation_info *p_conv_data;
    /*struct srtp_info *srtp_info = NULL;*/
    /*unsigned int srtp_offset;*/
    const char   *pt = NULL;
    /* Can tap up to 4 RTP packets within same packet */
    static struct _rtp_info rtp_info_arr[4];
    static int rtp_info_current = 0;
    struct _rtp_info *rtp_info;
    static const int * octet1_fields[] = {
        &hf_rtp_version,
        &hf_rtp_padding,
        &hf_rtp_extension,
        &hf_rtp_csrc_count,
        NULL
    };

    rtp_info_current++;
    if (rtp_info_current == 4) {
        rtp_info_current = 0;
    }
    rtp_info = &rtp_info_arr[rtp_info_current];

    /* Get the fields in the first octet */
    octet1 = tvb_get_guint8( tvb, offset );
    version = RTP_VERSION( octet1 );

    if (version == 0) {
        switch (global_rtp_version0_type) {
        case RTP0_STUN:
            call_dissector(stun_handle, tvb, pinfo, tree);
            return tvb_captured_length(tvb);
        case RTP0_CLASSICSTUN:
            call_dissector(classicstun_handle, tvb, pinfo, tree);
            return tvb_captured_length(tvb);

        case RTP0_T38:
            call_dissector(t38_handle, tvb, pinfo, tree);
            return tvb_captured_length(tvb);

        case RTP0_SPRT:
            call_dissector(sprt_handle, tvb, pinfo, tree);
            return tvb_captured_length(tvb);

        case RTP0_INVALID:
            if (!(tvb_memeql(tvb, 4, "ZRTP", 4)))
            {
                call_dissector(zrtp_handle, tvb,pinfo, tree);
                return tvb_captured_length(tvb);
            }
        default:
            ; /* Unknown or unsupported version (let it fall through) */
        }
    }

    /* fill in the rtp_info structure */
    rtp_info->info_version = version;
    if (version != 2) {
        /*
         * Unknown or unsupported version.
         */
        col_set_str(pinfo->cinfo, COL_PROTOCOL, "RTP");

        col_add_fstr( pinfo->cinfo, COL_INFO,
            "Unknown RTP version %u", version);

        if ( tree ) {
            ti = proto_tree_add_item( tree, proto_rtp, tvb, offset, -1, ENC_NA );
            rtp_tree = proto_item_add_subtree( ti, ett_rtp );

            proto_tree_add_uint( rtp_tree, hf_rtp_version, tvb,
                offset, 1, octet1);
        }
        return offset;
    }

    padding_set = RTP_PADDING( octet1 );
    extension_set = RTP_EXTENSION( octet1 );
    csrc_count = RTP_CSRC_COUNT( octet1 );

    /* Get the fields in the second octet */
    octet2 = tvb_get_guint8( tvb, offset + 1 );
    marker_set = RTP_MARKER( octet2 );
    payload_type = RTP_PAYLOAD_TYPE( octet2 );

    if (marker_set && payload_type >= FIRST_RTCP_CONFLICT_PAYLOAD_TYPE && payload_type <=  LAST_RTCP_CONFLICT_PAYLOAD_TYPE) {
        call_dissector(rtcp_handle, tvb, pinfo, tree);
        return tvb_captured_length(tvb);
    }

    /* Get the subsequent fields */
    seq_num = tvb_get_ntohs( tvb, offset + 2 );
    timestamp = tvb_get_ntohl( tvb, offset + 4 );
    sync_src = tvb_get_ntohl( tvb, offset + 8 );

    /* fill in the rtp_info structure */
    rtp_info->info_padding_set = padding_set;
    rtp_info->info_padding_count = 0;
    rtp_info->info_marker_set = marker_set;
    rtp_info->info_is_video = FALSE;
    rtp_info->info_payload_type = payload_type;
    rtp_info->info_seq_num = seq_num;
    rtp_info->info_timestamp = timestamp;
    rtp_info->info_sync_src = sync_src;
    rtp_info->info_is_srtp = FALSE;
    rtp_info->info_setup_frame_num = 0;
    rtp_info->info_payload_type_str = NULL;
    rtp_info->info_payload_rate = 0;

    /*
     * Do we have all the data?
     */
    length = tvb_captured_length_remaining(tvb, offset);
    reported_length = tvb_reported_length_remaining(tvb, offset);
    if (reported_length >= 0 && length >= reported_length) {
        /*
         * Yes.
         */
        rtp_info->info_all_data_present = TRUE;
        rtp_info->info_data_len = reported_length;

        /*
         * Save the pointer to raw rtp data (header + payload incl.
         * padding).
         * That should be safe because the "epan_dissect_t"
         * constructed for the packet has not yet been freed when
         * the taps are called.
         * (Destroying the "epan_dissect_t" will end up freeing
         * all the tvbuffs and hence invalidating pointers to
         * their data.)
         * See "add_packet_to_packet_list()" for details.
         */
        rtp_info->info_data = tvb_get_ptr(tvb, 0, -1);
    } else {
        /*
         * No - packet was cut short at capture time.
         */
        rtp_info->info_all_data_present = FALSE;
        rtp_info->info_data_len = 0;
        rtp_info->info_data = NULL;
    }

    /* Look for conv and add to the frame if found */
    get_conv_info(pinfo, rtp_info);
    p_conv_data = (struct _rtp_conversation_info *)p_get_proto_data(wmem_file_scope(), pinfo, proto_rtp, 0);

    if (p_conv_data)
        rtp_info->info_is_video = p_conv_data->is_video;

    if (p_conv_data && p_conv_data->srtp_info) is_srtp = TRUE;
    rtp_info->info_is_srtp = is_srtp;

    col_set_str( pinfo->cinfo, COL_PROTOCOL, (is_srtp) ? "SRTP" : "RTP" );

    /* check if this is added as an SRTP stream - if so, don't try to dissect the payload data for now */
    p_conv_data = (struct _rtp_conversation_info *)p_get_proto_data(wmem_file_scope(), pinfo, proto_rtp, 0);

#if 0 /* XXX: srtp_offset never actually used ?? */
    if (p_conv_data && p_conv_data->srtp_info) {
        srtp_info = p_conv_data->srtp_info;
        if (rtp_info->info_all_data_present) {
            srtp_offset = rtp_info->info_data_len - srtp_info->mki_len - srtp_info->auth_tag_len;
        }
    }
#endif

    if (p_conv_data && p_conv_data->bta2dp_info && p_conv_data->bta2dp_info->codec_dissector) {
        rtp_info->info_payload_type_str = (const char *) dissector_handle_get_short_name(p_conv_data->bta2dp_info->codec_dissector);
    } else if (p_conv_data && p_conv_data->btvdp_info && p_conv_data->btvdp_info->codec_dissector) {
        rtp_info->info_payload_type_str = (const char *) dissector_handle_get_short_name(p_conv_data->btvdp_info->codec_dissector);
    }

    /* if it is dynamic payload, let use the conv data to see if it is defined */
    if ( (payload_type>95) && (payload_type<128) ) {
        if (p_conv_data && p_conv_data->rtp_dyn_payload) {
            int sample_rate = 0;

#ifdef DEBUG_CONVERSATION
            rtp_dump_dyn_payload(p_conv_data->rtp_dyn_payload);
#endif
            DPRINT(("looking up conversation data for dyn_pt=%d", payload_type));

            if (rtp_dyn_payload_get_full(p_conv_data->rtp_dyn_payload, payload_type,
                                        &payload_type_str, &sample_rate)) {
                DPRINT(("found conversation data for dyn_pt=%d, enc_name=%s",
                        payload_type, payload_type_str));
                rtp_info->info_payload_type_str = payload_type_str;
                rtp_info->info_payload_rate     = sample_rate;
            }
        }
    }

    if (p_conv_data && p_conv_data->bta2dp_info) {
        pt = (p_conv_data->bta2dp_info->codec_dissector) ? dissector_handle_get_short_name(p_conv_data->bta2dp_info->codec_dissector) : "Unknown";
    } else if (p_conv_data && p_conv_data->btvdp_info) {
        pt = (p_conv_data->btvdp_info->codec_dissector) ? dissector_handle_get_short_name(p_conv_data->btvdp_info->codec_dissector) : "Unknown";
    } else {
        pt = (payload_type_str ? payload_type_str : val_to_str_ext(payload_type, &rtp_payload_type_vals_ext, "Unknown (%u)"));
    }

    col_add_fstr( pinfo->cinfo, COL_INFO,
        "PT=%s, SSRC=0x%X, Seq=%u, Time=%u%s",
        pt,
        sync_src,
        seq_num,
        timestamp,
        marker_set ? ", Mark" : "");

    if ( tree ) {
        proto_tree *item;
        /* Create RTP protocol tree */
        ti = proto_tree_add_item(tree, proto_rtp, tvb, offset, -1, ENC_NA );
        rtp_tree = proto_item_add_subtree(ti, ett_rtp );

        /* Conversation setup info */
        if (global_rtp_show_setup_info)
        {
            show_setup_info(tvb, pinfo, rtp_tree);
        }

        proto_tree_add_bitmask_list(rtp_tree, tvb, offset, 1, octet1_fields, ENC_NA);
        offset++;

        proto_tree_add_boolean( rtp_tree, hf_rtp_marker, tvb, offset,
            1, octet2 );

        proto_tree_add_uint_format( rtp_tree, hf_rtp_payload_type, tvb,
            offset, 1, octet2, "Payload type: %s (%u)", pt, payload_type);

        offset++;

        /* Sequence number 16 bits (2 octets) */
        proto_tree_add_uint( rtp_tree, hf_rtp_seq_nr, tvb, offset, 2, seq_num );
        if(p_conv_data != NULL) {
            item = proto_tree_add_uint( rtp_tree, hf_rtp_ext_seq_nr, tvb, offset, 2, p_conv_data->extended_seqno );
            PROTO_ITEM_SET_GENERATED(item);
        }
        offset += 2;

        /* Timestamp 32 bits (4 octets) */
        proto_tree_add_uint( rtp_tree, hf_rtp_timestamp, tvb, offset, 4, timestamp );
        offset += 4;

        /* Synchronization source identifier 32 bits (4 octets) */
        proto_tree_add_uint( rtp_tree, hf_rtp_ssrc, tvb, offset, 4, sync_src );
        offset += 4;
    } else {
        offset += 12;
    }
    /* CSRC list*/
    if ( csrc_count > 0 ) {
        proto_tree *rtp_csrc_tree;
        guint32 csrc_item;
        ti = proto_tree_add_item(rtp_tree, hf_rtp_csrc_items, tvb, offset,
                                     csrc_count * 4, ENC_NA);
        proto_item_append_text(ti, " (%u items)", csrc_count);
        rtp_csrc_tree = proto_item_add_subtree( ti, ett_csrc_list );

        for (i = 0; i < csrc_count; i++ ) {
            csrc_item = tvb_get_ntohl( tvb, offset );
            proto_tree_add_uint_format( rtp_csrc_tree,
                hf_rtp_csrc_item, tvb, offset, 4,
                csrc_item,
                "CSRC item %d: 0x%X",
                i, csrc_item );
            offset += 4;
        }
    }

    /* Optional RTP header extension */
    if ( extension_set ) {
        unsigned int hdr_extension_len;
        unsigned int hdr_extension_id;

        /* Defined by profile field is 16 bits (2 octets) */
        hdr_extension_id = tvb_get_ntohs( tvb, offset );
        if ( tree ) proto_tree_add_uint( rtp_tree, hf_rtp_prof_define, tvb, offset, 2, hdr_extension_id );
        offset += 2;

        hdr_extension_len = tvb_get_ntohs( tvb, offset );
        if ( tree ) proto_tree_add_uint( rtp_tree, hf_rtp_length, tvb, offset, 2, hdr_extension_len);
        offset += 2;
        if ( hdr_extension_len > 0 ) {
            proto_tree *rtp_hext_tree = NULL;
            tvbuff_t   *newtvb;

            if ( tree ) {
                ti = proto_tree_add_item(rtp_tree, hf_rtp_hdr_exts, tvb, offset, hdr_extension_len * 4, ENC_NA);
                rtp_hext_tree = proto_item_add_subtree( ti, ett_hdr_ext );
            }

            /* pass interpretation of header extension to a registered subdissector */
            newtvb = tvb_new_subset_length(tvb, offset, hdr_extension_len * 4);

            if (hdr_extension_id == RTP_RFC5285_ONE_BYTE_SIG) {
                dissect_rtp_hext_rfc5285_onebyte (newtvb, pinfo, rtp_hext_tree);
            }
            else if ((hdr_extension_id & RTP_RFC5285_TWO_BYTE_MASK) == RTP_RFC5285_TWO_BYTE_SIG) {
                dissect_rtp_hext_rfc5285_twobytes(tvb,
                    offset - 4, hdr_extension_id, newtvb,
                    pinfo, rtp_hext_tree);
            }
            else {
                if ( !(dissector_try_uint(rtp_hdr_ext_dissector_table, hdr_extension_id, newtvb, pinfo, rtp_hext_tree)) ) {
                    if ( tree ) {
                        unsigned int hdrext_offset;

                        hdrext_offset = offset;
                        for ( i = 0; i < hdr_extension_len; i++ ) {
                            proto_tree_add_uint( rtp_hext_tree, hf_rtp_hdr_ext, tvb, hdrext_offset, 4, tvb_get_ntohl( tvb, hdrext_offset ) );
                            hdrext_offset += 4;
                        }
                    }
                }
            }
        }
        offset += hdr_extension_len * 4;
    }

    if ( padding_set ) {
        /*
         * This RTP frame has padding - find it.
         *
         * The padding count is found in the LAST octet of
         * the packet; it contains the number of octets
         * that can be ignored at the end of the packet.
         */
        volatile unsigned int padding_count;
        if (tvb_captured_length(tvb) < tvb_reported_length(tvb)) {
            /*
             * We don't *have* the last octet of the
             * packet, so we can't get the padding
             * count.
             *
             * Put an indication of that into the
             * tree, and just put in a raw data
             * item.
             */
            proto_tree_add_expert(rtp_tree, pinfo, &ei_rtp_padding_missing, tvb, 0, 0);
            call_data_dissector(tvb_new_subset_remaining(tvb, offset),
                pinfo, rtp_tree);
            return tvb_captured_length(tvb);
        }

        padding_count = tvb_get_guint8( tvb,
            tvb_reported_length( tvb ) - 1 );
        data_len =
            tvb_reported_length_remaining( tvb, offset ) - padding_count;

        rtp_info->info_payload_offset = offset;
        rtp_info->info_payload_len = tvb_captured_length_remaining(tvb, offset);
        rtp_info->info_padding_count = padding_count;

        if (p_conv_data && p_conv_data->bta2dp_info) {
            if (p_conv_data->bta2dp_info->codec_dissector == sbc_handle) {
                rtp_info->info_payload_offset += 1;
                rtp_info->info_payload_len -= 1;
            }

            if (p_conv_data->bta2dp_info->content_protection_type == BTAVDTP_CONTENT_PROTECTION_TYPE_SCMS_T) {
                rtp_info->info_payload_offset += 1;
                rtp_info->info_payload_len -= 1;
            }
        }

        if (p_conv_data && p_conv_data->btvdp_info &&
                p_conv_data->btvdp_info->content_protection_type == BTAVDTP_CONTENT_PROTECTION_TYPE_SCMS_T) {
            rtp_info->info_payload_offset += 1;
            rtp_info->info_payload_len -= 1;
        }

        if (data_len > 0) {
            /*
             * There's data left over when you take out
             * the padding; dissect it.
             */
            /* Ensure that tap is called after packet dissection, even in case of exception */
            TRY {
                dissect_rtp_data( tvb, pinfo, tree, rtp_tree,
                    offset,
                    data_len,
                    data_len,
                    payload_type);
            } CATCH_ALL {
                if (!pinfo->flags.in_error_pkt)
                    tap_queue_packet(rtp_tap, pinfo, rtp_info);
                RETHROW;
            }
            ENDTRY;
            offset += data_len;
        } else if (data_len < 0) {
            /*
             * The padding count is bigger than the
             * amount of RTP payload in the packet!
             * Clip the padding count.
             *
             * XXX - put an item in the tree to indicate
             * that the padding count is bogus?
             */
            padding_count =
                tvb_reported_length_remaining(tvb, offset);
        }
        if (padding_count > 1) {
            /*
             * There's more than one byte of padding;
             * show all but the last byte as padding
             * data.
             */
            if ( tree ) proto_tree_add_item( rtp_tree, hf_rtp_padding_data,
                tvb, offset, padding_count - 1, ENC_NA );
            offset += padding_count - 1;
        }
        /*
         * Show the last byte in the PDU as the padding
         * count.
         */
        if ( tree ) proto_tree_add_item( rtp_tree, hf_rtp_padding_count,
            tvb, offset, 1, ENC_BIG_ENDIAN );
    }
    else {
        /*
         * No padding.
         */
        rtp_info->info_payload_offset = offset;
        rtp_info->info_payload_len = tvb_captured_length_remaining(tvb, offset);

        if (p_conv_data && p_conv_data->bta2dp_info) {
            if (p_conv_data->bta2dp_info->codec_dissector == sbc_handle) {
                rtp_info->info_payload_offset += 1;
                rtp_info->info_payload_len -= 1;
            }

            if (p_conv_data->bta2dp_info->content_protection_type == BTAVDTP_CONTENT_PROTECTION_TYPE_SCMS_T) {
                rtp_info->info_payload_offset += 1;
                rtp_info->info_payload_len -= 1;
            }
        }

        if (p_conv_data && p_conv_data->btvdp_info &&
                p_conv_data->btvdp_info->content_protection_type == BTAVDTP_CONTENT_PROTECTION_TYPE_SCMS_T) {
            rtp_info->info_payload_offset += 1;
            rtp_info->info_payload_len -= 1;
        }

        if (tvb_reported_length_remaining(tvb, offset) > 0) {
            /* Ensure that tap is called after packet dissection, even in case of exception */
            TRY {
                dissect_rtp_data( tvb, pinfo, tree, rtp_tree, offset,
                          tvb_captured_length_remaining( tvb, offset ),
                          tvb_reported_length_remaining( tvb, offset ),
                          payload_type);
            } CATCH_ALL {
                if (!pinfo->flags.in_error_pkt)
                    tap_queue_packet(rtp_tap, pinfo, rtp_info);
                RETHROW;
            }
            ENDTRY;
        }
    }
    if (!pinfo->flags.in_error_pkt)
        tap_queue_packet(rtp_tap, pinfo, rtp_info);

    return offset;
}

static int
dissect_rtp_hdr_ext_ed137(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_ )
{
    unsigned int hdr_extension_len;

    hdr_extension_len = tvb_reported_length(tvb)/4;

    if ( hdr_extension_len > 0 ) {
        proto_tree   *rtp_hext_tree = NULL;
        unsigned int  offset = 0;
        unsigned int  hdrext_offset = 0;
        unsigned int  i;

        if ( tree ) {
            proto_item *ti;
            ti = proto_tree_add_item(tree, hf_rtp_hdr_ed137s, tvb, offset, hdr_extension_len * 4, ENC_NA);
            rtp_hext_tree = proto_item_add_subtree( ti, ett_hdr_ext_ed137s );
        }
        for(i=0; i<hdr_extension_len; i++) {
            guint32 ext_value = tvb_get_ntohl( tvb, hdrext_offset );

            if (RTP_ED137_ptt_mask(ext_value)) {
                col_append_str(pinfo->cinfo, COL_INFO, ", PTT");
            }
            if (RTP_ED137_squ_mask(ext_value)) {
                col_append_str(pinfo->cinfo, COL_INFO, ", SQU");
            }
            if ( tree ) {
                proto_item *ti2;
                proto_tree *rtp_hext_tree2;

                ti2 = proto_tree_add_item(rtp_hext_tree, hf_rtp_hdr_ed137, tvb, hdrext_offset, 4, ENC_NA);
                rtp_hext_tree2 = proto_item_add_subtree( ti2, ett_hdr_ext_ed137 );

                /* Following bits are used from ED137 RTPRx/RTPTx Information field */
                proto_tree_add_item( rtp_hext_tree2, hf_rtp_hdr_ed137_ptt_type, tvb, hdrext_offset, 4, ENC_BIG_ENDIAN);
                proto_tree_add_item( rtp_hext_tree2, hf_rtp_hdr_ed137_squ, tvb, hdrext_offset, 4, ENC_BIG_ENDIAN);
                proto_tree_add_item( rtp_hext_tree2, hf_rtp_hdr_ed137_ptt_id, tvb, hdrext_offset, 4, ENC_BIG_ENDIAN);
                proto_tree_add_item( rtp_hext_tree2, hf_rtp_hdr_ed137_sct, tvb, hdrext_offset, 4, ENC_BIG_ENDIAN);
                proto_tree_add_item( rtp_hext_tree2, hf_rtp_hdr_ed137_x, tvb, hdrext_offset, 4, ENC_BIG_ENDIAN);

                if (RTP_ED137_extended_information(ext_value)) {
                /* Extended information is used */
                    unsigned int ft_type;
                    proto_tree_add_item( rtp_hext_tree2, hf_rtp_hdr_ed137_ft_type, tvb, hdrext_offset, 4, ENC_BIG_ENDIAN);
                    proto_tree_add_item( rtp_hext_tree2, hf_rtp_hdr_ed137_ft_len, tvb, hdrext_offset, 4, ENC_BIG_ENDIAN);

                    ft_type = RTP_ED137_feature_type(ext_value);
                    switch (ft_type) {
                        case RTP_ED137_feature_bss_type:
                        {
                            unsigned int bss_qidx    = RTP_ED137_feature_bss_qidx(ext_value);
                            unsigned int bss_qidx_ml = RTP_ED137_feature_bss_qidx_ml(ext_value);
                            if (RTP_ED137_feature_bss_qidx_ml_rssi == bss_qidx_ml) {
                                /* Special handling for RSSI method */
                                if (bss_qidx <= RTP_ED137_feature_bss_qidx_rssi_max) {
                                    /* Correct range */
                                    proto_tree_add_item( rtp_hext_tree2, hf_rtp_hdr_ed137_ft_bss_rssi_qidx, tvb, hdrext_offset, 4, ENC_BIG_ENDIAN);
                                }
                                else {
                                    /* Handle as other method */
                                    proto_tree_add_item( rtp_hext_tree2, hf_rtp_hdr_ed137_ft_bss_qidx, tvb, hdrext_offset, 4, ENC_BIG_ENDIAN);
                                }
                            }
                            else {
                                /* Other BSS method handling */
                                proto_tree_add_item( rtp_hext_tree2, hf_rtp_hdr_ed137_ft_bss_qidx, tvb, hdrext_offset, 4, ENC_BIG_ENDIAN);
                            }
                            proto_tree_add_item( rtp_hext_tree2, hf_rtp_hdr_ed137_ft_bss_qidx_ml, tvb, hdrext_offset, 4, ENC_BIG_ENDIAN);
                            proto_tree_add_item( rtp_hext_tree2, hf_rtp_hdr_ed137_ft_bss_nu, tvb, hdrext_offset, 4, ENC_BIG_ENDIAN);
                            break;
                        }
                        default:
                            proto_tree_add_item( rtp_hext_tree2, hf_rtp_hdr_ed137_ft_value, tvb, hdrext_offset, 4, ENC_BIG_ENDIAN);
                            break;
                    }
                }
                else {
                    /* Extended information is not used */
                    proto_tree_add_item( rtp_hext_tree2, hf_rtp_hdr_ed137_x_nu, tvb, hdrext_offset, 4, ENC_BIG_ENDIAN);
                }

                proto_tree_add_item( rtp_hext_tree2, hf_rtp_hdr_ed137_vf, tvb, hdrext_offset, 4, ENC_BIG_ENDIAN);
            }
            hdrext_offset += 4;
        }
    }
    return tvb_captured_length(tvb);
}

static int
dissect_rtp_hdr_ext_ed137a(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_ )
{
    unsigned int hdr_extension_len;

    hdr_extension_len = tvb_reported_length(tvb)/4;

    if ( hdr_extension_len > 0 ) {
        proto_tree *rtp_hext_tree = NULL;
        unsigned int offset = 0;
        unsigned int hdrext_offset = 0;
        unsigned int i;

        if ( tree ) {
            proto_item *ti;
            ti = proto_tree_add_item(tree, hf_rtp_hdr_ed137s, tvb, offset, hdr_extension_len * 4, ENC_NA);
            rtp_hext_tree = proto_item_add_subtree( ti, ett_hdr_ext_ed137s );
        }
        for(i=0; i<hdr_extension_len; i++) {
            guint32 ext_value = tvb_get_ntohl( tvb, hdrext_offset );

            if (RTP_ED137A_ptt_mask(ext_value)) {
                col_append_str(pinfo->cinfo, COL_INFO, ", PTT");
            }
            if (RTP_ED137A_squ_mask(ext_value)) {
                col_append_str(pinfo->cinfo, COL_INFO, ", SQU");
            }
            if ( tree ) {
                proto_item *ti2;
                proto_tree *rtp_hext_tree2;

                ti2 = proto_tree_add_item(rtp_hext_tree, hf_rtp_hdr_ed137a, tvb, hdrext_offset, 4, ENC_NA);
                rtp_hext_tree2 = proto_item_add_subtree( ti2, ett_hdr_ext_ed137a );

                /* Following bits are used from ED137A/B RTPRx Information field */
                proto_tree_add_item( rtp_hext_tree2, hf_rtp_hdr_ed137a_ptt_type, tvb, hdrext_offset, 4, ENC_BIG_ENDIAN);
                proto_tree_add_item( rtp_hext_tree2, hf_rtp_hdr_ed137a_squ, tvb, hdrext_offset, 4, ENC_BIG_ENDIAN);
                proto_tree_add_item( rtp_hext_tree2, hf_rtp_hdr_ed137a_ptt_id, tvb, hdrext_offset, 4, ENC_BIG_ENDIAN);
                proto_tree_add_item( rtp_hext_tree2, hf_rtp_hdr_ed137a_pm, tvb, hdrext_offset, 4, ENC_BIG_ENDIAN);
                proto_tree_add_item( rtp_hext_tree2, hf_rtp_hdr_ed137a_ptts, tvb, hdrext_offset, 4, ENC_BIG_ENDIAN);
                proto_tree_add_item( rtp_hext_tree2, hf_rtp_hdr_ed137a_sct, tvb, hdrext_offset, 4, ENC_BIG_ENDIAN);
                proto_tree_add_item( rtp_hext_tree2, hf_rtp_hdr_ed137a_reserved, tvb, hdrext_offset, 4, ENC_BIG_ENDIAN);
                proto_tree_add_item( rtp_hext_tree2, hf_rtp_hdr_ed137a_x, tvb, hdrext_offset, 4, ENC_BIG_ENDIAN);

                if (RTP_ED137A_extended_information(ext_value)) {
                    /* Extended information is used */
                    unsigned int ft_type;
                    proto_tree_add_item( rtp_hext_tree2, hf_rtp_hdr_ed137a_ft_type, tvb, hdrext_offset, 4, ENC_BIG_ENDIAN);
                    proto_tree_add_item( rtp_hext_tree2, hf_rtp_hdr_ed137a_ft_len, tvb, hdrext_offset, 4, ENC_BIG_ENDIAN);

                    ft_type = RTP_ED137A_feature_type(ext_value);
                    switch (ft_type) {
                        case RTP_ED137A_feature_sqi_type:
                        {
                            unsigned int sqi_qidx;
                            unsigned int sqi_qidx_ml;

                            sqi_qidx    = RTP_ED137A_feature_sqi_qidx(ext_value);
                            sqi_qidx_ml = RTP_ED137A_feature_sqi_qidx_ml(ext_value);
                            if (RTP_ED137A_feature_sqi_qidx_ml_rssi == sqi_qidx_ml) {
                                /* Special handling for RSSI method */
                                if (sqi_qidx <= RTP_ED137A_feature_sqi_qidx_rssi_max) {
                                    /* Correct range */
                                    proto_tree_add_item( rtp_hext_tree2, hf_rtp_hdr_ed137a_ft_sqi_rssi_qidx, tvb, hdrext_offset, 4, ENC_BIG_ENDIAN);
                                }
                                else {
                                    /* Handle as other method */
                                    proto_tree_add_item( rtp_hext_tree2, hf_rtp_hdr_ed137a_ft_sqi_qidx, tvb, hdrext_offset, 4, ENC_BIG_ENDIAN);
                                }
                            }
                            else {
                                /* Other SQI method handling */
                                proto_tree_add_item( rtp_hext_tree2, hf_rtp_hdr_ed137a_ft_sqi_qidx, tvb, hdrext_offset, 4, ENC_BIG_ENDIAN);
                            }
                            proto_tree_add_item( rtp_hext_tree2, hf_rtp_hdr_ed137a_ft_sqi_qidx_ml, tvb, hdrext_offset, 4, ENC_BIG_ENDIAN);
                            break;
                        }
                        default:
                            proto_tree_add_item( rtp_hext_tree2, hf_rtp_hdr_ed137a_ft_value, tvb, hdrext_offset, 4, ENC_BIG_ENDIAN);
                            break;
                    }
                }
                else {
                    /* Extended information is not used */
                    proto_tree_add_item( rtp_hext_tree2, hf_rtp_hdr_ed137a_x_nu, tvb, hdrext_offset, 4, ENC_BIG_ENDIAN);
                }
            }
            hdrext_offset += 4;
        }
    }
    return tvb_captured_length(tvb);
}

/* calculate the extended sequence number - top 16 bits of the previous sequence number,
 * plus our own; then correct for wrapping */
static guint32
calculate_extended_seqno(guint32 previous_seqno, guint16 raw_seqno)
{
    guint32 seqno = (previous_seqno & 0xffff0000) | raw_seqno;
    if(seqno + 0x8000 < previous_seqno) {
        seqno += 0x10000;
    } else if(previous_seqno + 0x8000 < seqno) {
        /* we got an out-of-order packet which happened to go backwards over the
         * wrap boundary */
        seqno -= 0x10000;
    }
    return seqno;
}

/* Look for conversation info */
static void
get_conv_info(packet_info *pinfo, struct _rtp_info *rtp_info)
{
    /* Conversation and current data */
    struct _rtp_conversation_info *p_conv_data;

    /* Use existing packet info if available */
    p_conv_data = (struct _rtp_conversation_info *)p_get_proto_data(wmem_file_scope(), pinfo, proto_rtp, 0);

    if (!p_conv_data)
    {
        conversation_t *p_conv;

        /* First time, get info from conversation */
        p_conv = find_conversation(pinfo->num, &pinfo->net_dst, &pinfo->net_src,
                                   pinfo->ptype,
                                   pinfo->destport, pinfo->srcport, NO_ADDR_B);
        if (p_conv)
        {
            /* Create space for packet info */
            struct _rtp_conversation_info *p_conv_packet_data;
            p_conv_data = (struct _rtp_conversation_info *)conversation_get_proto_data(p_conv, proto_rtp);

            if (p_conv_data) {
                guint32 seqno;

                /* Save this conversation info into packet info */
                /* XXX: why is this file pool not pinfo->pool? */
                p_conv_packet_data = wmem_new(wmem_file_scope(), struct _rtp_conversation_info);
                g_strlcpy(p_conv_packet_data->method, p_conv_data->method, MAX_RTP_SETUP_METHOD_SIZE+1);
                p_conv_packet_data->frame_number = p_conv_data->frame_number;
                p_conv_packet_data->is_video = p_conv_data->is_video;
                /* do not increment ref count for the rtp_dyn_payload */
                p_conv_packet_data->rtp_dyn_payload = p_conv_data->rtp_dyn_payload;
                p_conv_packet_data->rtp_conv_info = p_conv_data->rtp_conv_info;
                p_conv_packet_data->srtp_info = p_conv_data->srtp_info;
                p_conv_packet_data->bta2dp_info = p_conv_data->bta2dp_info;
                p_conv_packet_data->btvdp_info = p_conv_data->btvdp_info;
                /* XXX: why is this file pool not pinfo->pool? */
                p_add_proto_data(wmem_file_scope(), pinfo, proto_rtp, 0, p_conv_packet_data);

                /* calculate extended sequence number */
                seqno = calculate_extended_seqno(p_conv_data->extended_seqno,
                                 rtp_info->info_seq_num);

                p_conv_packet_data->extended_seqno = seqno;
                p_conv_data->extended_seqno = seqno;
            }
        }
    }
    if (p_conv_data) rtp_info->info_setup_frame_num = p_conv_data->frame_number;
}


/* Display setup info */
static void
show_setup_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    /* Conversation and current data */
    struct _rtp_conversation_info *p_conv_data;
    proto_tree *rtp_setup_tree;
    proto_item *ti;

    /* Use existing packet info if available */
    p_conv_data = (struct _rtp_conversation_info *)p_get_proto_data(wmem_file_scope(), pinfo, proto_rtp, 0);

    if (!p_conv_data) return;

    /* Create setup info subtree with summary info. */
    ti =  proto_tree_add_string_format(tree, hf_rtp_setup, tvb, 0, 0,
                       "", "Stream setup by %s (frame %u)",
                       p_conv_data->method,
                       p_conv_data->frame_number);
        PROTO_ITEM_SET_GENERATED(ti);
        rtp_setup_tree = proto_item_add_subtree(ti, ett_rtp_setup);
        if (rtp_setup_tree)
        {
            /* Add details into subtree */
            proto_item* item = proto_tree_add_uint(rtp_setup_tree, hf_rtp_setup_frame,
                                                   tvb, 0, 0, p_conv_data->frame_number);
            PROTO_ITEM_SET_GENERATED(item);
            item = proto_tree_add_string(rtp_setup_tree, hf_rtp_setup_method,
                                         tvb, 0, 0, p_conv_data->method);
            PROTO_ITEM_SET_GENERATED(item);
        }
}

/* Dissect PacketCable CCC header */

static int
dissect_pkt_ccc(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data)
{

    if ( tree ) {
        proto_item *ti;
        proto_tree *pkt_ccc_tree;

        ti = proto_tree_add_item(tree, proto_pkt_ccc, tvb, 0, 12, ENC_NA);
        pkt_ccc_tree = proto_item_add_subtree(ti, ett_pkt_ccc);

        proto_tree_add_item(pkt_ccc_tree, hf_pkt_ccc_id, tvb, 0, 4, ENC_BIG_ENDIAN);
        proto_tree_add_item(pkt_ccc_tree, hf_pkt_ccc_ts, tvb, 4, 8,
                    ENC_TIME_NTP|ENC_BIG_ENDIAN);
    }

    return dissect_rtp(tvb, pinfo, tree, data);
}


/* Register PacketCable CCC */

void
proto_register_pkt_ccc(void)
{
    static hf_register_info hf[] =
    {
        {
            &hf_pkt_ccc_id,
            {
                "PacketCable CCC Identifier",
                "pkt_ccc.ccc_id",
                FT_UINT32,
                BASE_DEC,
                NULL,
                0x0,
                NULL, HFILL
            }
        },
        {
            &hf_pkt_ccc_ts,
            {
                "PacketCable CCC Timestamp",
                "pkt_ccc.ts",
                FT_ABSOLUTE_TIME,
                ABSOLUTE_TIME_UTC,
                NULL,
                0x0,
                NULL, HFILL
            }
        },

    };

    static gint *ett[] =
    {
        &ett_pkt_ccc,
    };

    module_t *pkt_ccc_module;

    proto_pkt_ccc = proto_register_protocol("PacketCable Call Content Connection",
        "PKT CCC", "pkt_ccc");
    proto_register_field_array(proto_pkt_ccc, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));

    register_dissector("pkt_ccc", dissect_pkt_ccc, proto_pkt_ccc);

    pkt_ccc_module = prefs_register_protocol(proto_pkt_ccc, proto_reg_handoff_pkt_ccc);

    prefs_register_uint_preference(pkt_ccc_module, "udp_port",
                                   "UDP port",
                                   "Decode packets on this UDP port as PacketCable CCC",
                                   10, &global_pkt_ccc_udp_port);
}

void
proto_reg_handoff_pkt_ccc(void)
{
    /*
     * Register this dissector as one that can be selected by a
     * UDP port number.
     */
    static gboolean initialized = FALSE;
    static dissector_handle_t pkt_ccc_handle;
    static guint saved_pkt_ccc_udp_port;

    if (!initialized) {
        pkt_ccc_handle = find_dissector("pkt_ccc");
        dissector_add_for_decode_as("udp.port", pkt_ccc_handle);
        initialized = TRUE;
    } else {
        if (saved_pkt_ccc_udp_port != 0) {
            dissector_delete_uint("udp.port", saved_pkt_ccc_udp_port, pkt_ccc_handle);
        }
    }

    if (global_pkt_ccc_udp_port != 0) {
        dissector_add_uint("udp.port", global_pkt_ccc_udp_port, pkt_ccc_handle);
    }
    saved_pkt_ccc_udp_port = global_pkt_ccc_udp_port;
}

/* Register RTP */

void
proto_register_rtp(void)
{
    static hf_register_info hf[] =
    {
        {
            &hf_rtp_version,
            {
                "Version",
                "rtp.version",
                FT_UINT8,
                BASE_DEC,
                VALS(rtp_version_vals),
                0xC0,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_padding,
            {
                "Padding",
                "rtp.padding",
                FT_BOOLEAN,
                8,
                NULL,
                0x20,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_extension,
            {
                "Extension",
                "rtp.ext",
                FT_BOOLEAN,
                8,
                NULL,
                0x10,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_csrc_count,
            {
                "Contributing source identifiers count",
                "rtp.cc",
                FT_UINT8,
                BASE_DEC,
                NULL,
                0x0F,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_marker,
            {
                "Marker",
                "rtp.marker",
                FT_BOOLEAN,
                8,
                NULL,
                0x80,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_payload_type,
            {
                "Payload type",
                "rtp.p_type",
                FT_UINT8,
                BASE_DEC,
                NULL,
                0x7F,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_seq_nr,
            {
                "Sequence number",
                "rtp.seq",
                FT_UINT16,
                BASE_DEC,
                NULL,
                0x0,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_ext_seq_nr,
            {
                "Extended sequence number",
                "rtp.extseq",
                FT_UINT32,
                BASE_DEC,
                NULL,
                0x0,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_timestamp,
            {
                "Timestamp",
                "rtp.timestamp",
                FT_UINT32,
                BASE_DEC,
                NULL,
                0x0,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_ssrc,
            {
                "Synchronization Source identifier",
                "rtp.ssrc",
                FT_UINT32,
                BASE_HEX_DEC,
                NULL,
                0x0,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_prof_define,
            {
                "Defined by profile",
                "rtp.ext.profile",
                FT_UINT16,
                BASE_HEX_DEC,
                VALS(rtp_ext_profile_vals),
                0x0,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_length,
            {
                "Extension length",
                "rtp.ext.len",
                FT_UINT16,
                BASE_DEC,
                NULL,
                0x0,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_csrc_items,
            {
                "Contributing Source identifiers",
                "rtp.csrc.items",
                FT_NONE,
                BASE_NONE,
                NULL,
                0x0,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_csrc_item,
            {
                "CSRC item",
                "rtp.csrc.item",
                FT_UINT32,
                BASE_HEX_DEC,
                NULL,
                0x0,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_hdr_exts,
            {
                "Header extensions",
                "rtp.hdr_exts",
                FT_NONE,
                BASE_NONE,
                NULL,
                0x0,
                NULL, HFILL
            }
        },
/* ED137 and ED137A common structures */
        {
            &hf_rtp_hdr_ed137s,
            {
                "ED137 extensions",
                "rtp.ext.ed137s",
                FT_NONE,
                BASE_NONE,
                NULL,
                0x0,
                NULL, HFILL
            }
        },
/* ED137 only structures */
        {
            &hf_rtp_hdr_ed137,
            {
                "ED137 extension",
                "rtp.ext.ed137",
                FT_NONE,
                BASE_NONE,
                NULL,
                0x0,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_hdr_ed137_ptt_type,
            {
                "PTT Type",
                "rtp.ext.ed137.ptt_type",
                FT_UINT32,
                BASE_DEC,
                VALS(rtp_ext_ed137_ptt_type),
                0xE0000000,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_hdr_ed137_squ,
            {
                "SQU",
                "rtp.ext.ed137.squ",
                FT_UINT32,
                BASE_DEC,
                VALS(rtp_ext_ed137_squ),
                0x10000000,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_hdr_ed137_ptt_id,
            {
                "PTT-id",
                "rtp.ext.ed137.ptt_id",
                FT_UINT32,
                BASE_DEC,
                NULL,
                0x0F000000,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_hdr_ed137_sct,
            {
                "Simultaneous Call Transmissions",
                "rtp.ext.ed137.sct",
                FT_UINT32,
                BASE_DEC,
                NULL,
                0x00800000,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_hdr_ed137_x,
            {
                "X",
                "rtp.ext.ed137.x",
                FT_UINT32,
                BASE_DEC,
                NULL,
                0x00400000,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_hdr_ed137_x_nu,
            {
                "Not used",
                "rtp.ext.ed137.x-nu",
                FT_UINT32,
                BASE_DEC,
                NULL,
                0x003FFFFE,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_hdr_ed137_ft_type,
            {
                "Feature type",
                "rtp.ext.ed137.ft.type",
                FT_UINT32,
                BASE_HEX_DEC,
                VALS(rtp_ext_ed137_ft_type),
                0x003C0000,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_hdr_ed137_ft_len,
            {
                "Feature length",
                "rtp.ext.ed137.ft.len",
                FT_UINT32,
                BASE_DEC,
                NULL,
                0x0003C000,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_hdr_ed137_ft_value,
            {
                "Feature value",
                "rtp.ext.ed137.ft.value",
                FT_UINT32,
                BASE_HEX_DEC,
                NULL,
                0x00003FFE,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_hdr_ed137_vf,
            {
                "VF",
                "rtp.ext.ed137.vf",
                FT_UINT32,
                BASE_DEC,
                VALS(rtp_ext_ed137_vf),
                0x00000001,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_hdr_ed137_ft_bss_qidx,
            {
                "BSS Quality Index",
                "rtp.ext.ed137.ft.bss.qidx",
                FT_UINT32,
                BASE_DEC,
                NULL,
                0x00003FC0,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_hdr_ed137_ft_bss_rssi_qidx,
            {
                "BSS Quality Index",
                "rtp.ext.ed137.ft.bss.qidx",
                FT_UINT32,
                BASE_DEC,
                VALS(rtp_ext_ed137_ft_bss_rssi_qidx),
                0x00003FC0,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_hdr_ed137_ft_bss_qidx_ml,
            {
                "BSS Quality Index Method",
                "rtp.ext.ed137.ft.bss.qidx-ml",
                FT_UINT32,
                BASE_DEC,
                VALS(rtp_ext_ed137_ft_bss_qidx_ml),
                0x00000038,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_hdr_ed137_ft_bss_nu,
            {
                "Not used",
                "rtp.ext.ed137.ft.bss-nu",
                FT_UINT32,
                BASE_DEC,
                NULL,
                0x00000006,
                NULL, HFILL
            }
        },
/* ED137A only structures */
        {
            &hf_rtp_hdr_ed137a,
            {
                "ED137A extension",
                "rtp.ext.ed137A",
                FT_NONE,
                BASE_NONE,
                NULL,
                0x0,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_hdr_ed137a_ptt_type,
            {
                "PTT Type",
                "rtp.ext.ed137A.ptt_type",
                FT_UINT32,
                BASE_DEC,
                VALS(rtp_ext_ed137a_ptt_type),
                0xE0000000,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_hdr_ed137a_squ,
            {
                "SQU",
                "rtp.ext.ed137A.squ",
                FT_UINT32,
                BASE_DEC,
                VALS(rtp_ext_ed137a_squ),
                0x10000000,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_hdr_ed137a_ptt_id,
            {
                "PTT-id",
                "rtp.ext.ed137A.ptt_id",
                FT_UINT32,
                BASE_DEC,
                NULL,
                0x0FC00000,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_hdr_ed137a_pm,
            {
                "PTT Mute",
                "rtp.ext.ed137A.pm",
                FT_UINT32,
                BASE_DEC,
                NULL,
                0x00200000,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_hdr_ed137a_ptts,
            {
                "PTT Summation",
                "rtp.ext.ed137A.ptts",
                FT_UINT32,
                BASE_DEC,
                NULL,
                0x00100000,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_hdr_ed137a_sct,
            {
                "Simultaneous Call Transmissions",
                "rtp.ext.ed137a.sct",
                FT_UINT32,
                BASE_DEC,
                NULL,
                0x00080000,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_hdr_ed137a_reserved,
            {
                "Reserved",
                "rtp.ext.ed137A.reserved",
                FT_UINT32,
                BASE_HEX_DEC,
                NULL,
                0x00060000,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_hdr_ed137a_x,
            {
                "X",
                "rtp.ext.ed137A.x",
                FT_UINT32,
                BASE_DEC,
                NULL,
                0x00010000,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_hdr_ed137a_x_nu,
            {
                "Not used",
                "rtp.ext.ed137A.x-nu",
                FT_UINT32,
                BASE_DEC,
                NULL,
                0x0000FFFF,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_hdr_ed137a_ft_type,
            {
                "Feature type",
                "rtp.ext.ed137A.ft.type",
                FT_UINT32,
                BASE_HEX_DEC,
                VALS(rtp_ext_ed137a_ft_type),
                0x0000F000,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_hdr_ed137a_ft_len,
            {
                "Feature length",
                "rtp.ext.ed137A.ft.len",
                FT_UINT32,
                BASE_DEC,
                NULL,
                0x00000F00,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_hdr_ed137a_ft_value,
            {
                "Feature value",
                "rtp.ext.ed137A.ft.value",
                FT_UINT32,
                BASE_HEX_DEC,
                NULL,
                0x000000FF,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_hdr_ed137a_ft_sqi_qidx,
            {
                "SQI Quality Index",
                "rtp.ext.ed137A.ft.sqi.qidx",
                FT_UINT32,
                BASE_DEC,
                NULL,
                0x000000F8,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_hdr_ed137a_ft_sqi_rssi_qidx,
            {
                "SQI Quality Index",
                "rtp.ext.ed137A.ft.sqi.qidx",
                FT_UINT32,
                BASE_DEC,
                VALS(rtp_ext_ed137a_ft_sqi_rssi_qidx),
                0x000000F8,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_hdr_ed137a_ft_sqi_qidx_ml,
            {
                "SQI Quality Index Method",
                "rtp.ext.ed137A.ft.sqi.qidx-ml",
                FT_UINT32,
                BASE_DEC,
                VALS(rtp_ext_ed137a_ft_sqi_qidx_ml),
                0x00000007,
                NULL, HFILL
            }
        },
/* Other RTP structures */
        {
            &hf_rtp_hdr_ext,
            {
                "Header extension",
                "rtp.hdr_ext",
                FT_UINT32,
                BASE_HEX_DEC,
                NULL,
                0x0,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_data,
            {
                "Payload",
                "rtp.payload",
                FT_BYTES,
                BASE_NONE,
                NULL,
                0x0,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_padding_data,
            {
                "Padding data",
                "rtp.padding.data",
                FT_BYTES,
                BASE_NONE,
                NULL,
                0x0,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_padding_count,
            {
                "Padding count",
                "rtp.padding.count",
                FT_UINT8,
                BASE_DEC,
                NULL,
                0x0,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_setup,
            {
                "Stream setup",
                "rtp.setup",
                FT_STRING,
                BASE_NONE,
                NULL,
                0x0,
                "Stream setup, method and frame number", HFILL
            }
        },
        {
            &hf_rtp_setup_frame,
            {
                "Setup frame",
                "rtp.setup-frame",
                FT_FRAMENUM,
                BASE_NONE,
                NULL,
                0x0,
                "Frame that set up this stream", HFILL
            }
        },
        {
            &hf_rtp_setup_method,
            {
                "Setup Method",
                "rtp.setup-method",
                FT_STRING,
                BASE_NONE,
                NULL,
                0x0,
                "Method used to set up this stream", HFILL
            }
        },
        {
            &hf_rtp_rfc2198_follow,
            {
                "Follow",
                "rtp.follow",
                FT_BOOLEAN,
                8,
                TFS(&tfs_set_notset),
                0x80,
                "Next header follows", HFILL
            }
        },
        {
            &hf_rtp_rfc2198_tm_off,
            {
                "Timestamp offset",
                "rtp.timestamp-offset",
                FT_UINT16,
                BASE_DEC,
                NULL,
                0xFFFC,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_rfc2198_bl_len,
            {
                "Block length",
                "rtp.block-length",
                FT_UINT16,
                BASE_DEC,
                NULL,
                0x03FF,
                NULL, HFILL
            }
        },
        {
            &hf_rtp_ext_rfc5285_id,
            {
                "Identifier",
                "rtp.ext.rfc5285.id",
                FT_UINT8,
                BASE_DEC,
                NULL,
                0x0,
                "RFC 5285 Header Extension Identifier",
                HFILL
            }
        },
        {
            &hf_rtp_ext_rfc5285_length,
            {
                "Length",
                "rtp.ext.rfc5285.len",
                FT_UINT8,
                BASE_DEC,
                NULL,
                0x0,
                "RFC 5285 Header Extension length",
                HFILL
            }
        },
        {
            &hf_rtp_ext_rfc5285_appbits,
            {
                "Application Bits",
                "rtp.ext.rfc5285.appbits",
                FT_UINT8,
                BASE_DEC,
                NULL,
                0x0,
                "RFC 5285 2-bytes header application bits",
                HFILL
            }
        },
        {
            &hf_rtp_ext_rfc5285_data,
            {
                "Extension Data",
                "rtp.ext.rfc5285.data",
                FT_BYTES,
                BASE_NONE,
                NULL,
                0x0,
                "RFC 5285 Extension Data",
                HFILL
            }
        },

        /* reassembly stuff */
        {&hf_rtp_fragments,
         {"RTP Fragments", "rtp.fragments", FT_NONE, BASE_NONE, NULL, 0x0,
          NULL, HFILL }
        },

        {&hf_rtp_fragment,
         {"RTP Fragment data", "rtp.fragment", FT_FRAMENUM, BASE_NONE, NULL, 0x0,
          NULL, HFILL }
        },

        {&hf_rtp_fragment_overlap,
         {"Fragment overlap", "rtp.fragment.overlap", FT_BOOLEAN, BASE_NONE,
          NULL, 0x0, "Fragment overlaps with other fragments", HFILL }
        },

        {&hf_rtp_fragment_overlap_conflict,
         {"Conflicting data in fragment overlap", "rtp.fragment.overlap.conflict",
          FT_BOOLEAN, BASE_NONE, NULL, 0x0,
          "Overlapping fragments contained conflicting data", HFILL }
        },

        {&hf_rtp_fragment_multiple_tails,
         {"Multiple tail fragments found", "rtp.fragment.multipletails",
          FT_BOOLEAN, BASE_NONE, NULL, 0x0,
          "Several tails were found when defragmenting the packet", HFILL }
        },

        {&hf_rtp_fragment_too_long_fragment,
         {"Fragment too long", "rtp.fragment.toolongfragment",
          FT_BOOLEAN, BASE_NONE, NULL, 0x0,
          "Fragment contained data past end of packet", HFILL }
        },

        {&hf_rtp_fragment_error,
         {"Defragmentation error", "rtp.fragment.error",
          FT_FRAMENUM, BASE_NONE, NULL, 0x0,
          "Defragmentation error due to illegal fragments", HFILL }
        },

        {&hf_rtp_fragment_count,
         {"Fragment count", "rtp.fragment.count",
          FT_UINT32, BASE_DEC, NULL, 0x0,
          NULL, HFILL }
        },

        {&hf_rtp_reassembled_in,
         {"RTP fragment, reassembled in frame", "rtp.reassembled_in",
          FT_FRAMENUM, BASE_NONE, NULL, 0x0,
          "This RTP packet is reassembled in this frame", HFILL }
        },
        {&hf_rtp_reassembled_length,
         {"Reassembled RTP length", "rtp.reassembled.length",
          FT_UINT32, BASE_DEC, NULL, 0x0,
          "The total length of the reassembled payload", HFILL }
        },
        {&hf_srtp_encrypted_payload,
         {"SRTP Encrypted Payload", "srtp.enc_payload",
          FT_BYTES, BASE_NONE, NULL, 0x0,
          NULL, HFILL }
        },
#if 0
        {&hf_srtp_null_encrypted_payload,
         {"SRTP Payload with NULL encryption", "srtp.null_enc_payload",
          FT_BYTES, BASE_NONE, NULL, 0x0,
          NULL, HFILL }
        },
#endif
        {&hf_srtp_mki,
         {"SRTP MKI", "srtp.mki",
          FT_BYTES, BASE_NONE, NULL, 0x0,
          "SRTP Master Key Index", HFILL }
        },
        {&hf_srtp_auth_tag,
         {"SRTP Auth Tag", "srtp.auth_tag",
          FT_BYTES, BASE_NONE, NULL, 0x0,
          "SRTP Authentication Tag", HFILL }
        }

    };

    static gint *ett[] =
    {
        &ett_rtp,
        &ett_csrc_list,
        &ett_hdr_ext,
        &ett_hdr_ext_rfc5285,
        &ett_hdr_ext_ed137s,
        &ett_hdr_ext_ed137,
        &ett_hdr_ext_ed137a,
        &ett_rtp_setup,
        &ett_rtp_rfc2198,
        &ett_rtp_rfc2198_hdr,
        &ett_rtp_fragment,
        &ett_rtp_fragments
    };

    static ei_register_info ei[] = {
        { &ei_rtp_fragment_unfinished, { "rtp.fragment_unfinished", PI_REASSEMBLE, PI_CHAT, "RTP fragment, unfinished", EXPFILL }},
        { &ei_rtp_padding_missing, { "rtp.padding_missing", PI_MALFORMED, PI_ERROR, "Frame has padding, but not all the frame data was captured", EXPFILL }},
    };

    module_t *rtp_module;
    expert_module_t *expert_rtp;

    proto_rtp = proto_register_protocol("Real-Time Transport Protocol",
                        "RTP", "rtp");
    proto_register_field_array(proto_rtp, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
    expert_rtp = expert_register_protocol(proto_rtp);
    expert_register_field_array(expert_rtp, ei, array_length(ei));

    register_dissector("rtp", dissect_rtp, proto_rtp);
    register_dissector("rtp.rfc2198", dissect_rtp_rfc2198, proto_rtp);

    rtp_tap = register_tap("rtp");

    rtp_pt_dissector_table = register_dissector_table("rtp.pt",
                                    "RTP payload type", proto_rtp, FT_UINT8, BASE_DEC);
    rtp_dyn_pt_dissector_table = register_dissector_table("rtp_dyn_payload_type",
                                    "Dynamic RTP payload type", proto_rtp, FT_STRING, TRUE);


    rtp_hdr_ext_dissector_table = register_dissector_table("rtp.hdr_ext",
                                    "RTP header extension", proto_rtp, FT_UINT32, BASE_HEX);
    rtp_hdr_ext_rfc5285_dissector_table = register_dissector_table("rtp.ext.rfc5285.id",
                                    "RTP Generic header extension (RFC 5285)", proto_rtp, FT_UINT8, BASE_DEC);

    register_dissector("rtp.ext.ed137", dissect_rtp_hdr_ext_ed137, proto_rtp);
    register_dissector("rtp.ext.ed137a", dissect_rtp_hdr_ext_ed137a, proto_rtp);

    rtp_module = prefs_register_protocol(proto_rtp, proto_reg_handoff_rtp);

    prefs_register_bool_preference(rtp_module, "show_setup_info",
                                    "Show stream setup information",
                                    "Where available, show which protocol and frame caused "
                                    "this RTP stream to be created",
                                    &global_rtp_show_setup_info);

    prefs_register_obsolete_preference(rtp_module, "heuristic_rtp");

    prefs_register_bool_preference(rtp_module, "desegment_rtp_streams",
                                    "Allow subdissector to reassemble RTP streams",
                                    "Whether subdissector can request RTP streams to be reassembled",
                                    &desegment_rtp);

    prefs_register_enum_preference(rtp_module, "version0_type",
                                    "Treat RTP version 0 packets as",
                                    "If an RTP version 0 packet is encountered, it can be treated as "
                                    "an invalid or ZRTP packet, a CLASSIC-STUN packet, or a T.38 packet",
                                    &global_rtp_version0_type,
                                    rtp_version0_types, FALSE);
    prefs_register_uint_preference(rtp_module,
                                    "rfc2198_payload_type", "Payload Type for RFC2198",
                                    "Payload Type for RFC2198 Redundant Audio Data",
                                    10,
                                    &rtp_rfc2198_pt);

    register_init_routine(rtp_fragment_init);
    register_cleanup_routine(rtp_fragment_cleanup);
    register_init_routine(rtp_dyn_payloads_init);
}

void
proto_reg_handoff_rtp(void)
{
    static gboolean rtp_prefs_initialized = FALSE;
    static dissector_handle_t rtp_rfc2198_handle;
    static guint rtp_saved_rfc2198_pt;

    if (!rtp_prefs_initialized) {
        dissector_handle_t rtp_hdr_ext_ed137_handle;
        dissector_handle_t rtp_hdr_ext_ed137a_handle;

        rtp_handle = find_dissector("rtp");
        rtp_rfc2198_handle = find_dissector("rtp.rfc2198");

        dissector_add_for_decode_as("udp.port", rtp_handle);
        dissector_add_string("rtp_dyn_payload_type", "red", rtp_rfc2198_handle);
        heur_dissector_add( "udp", dissect_rtp_heur_udp,  "RTP over UDP", "rtp_udp", proto_rtp, HEURISTIC_DISABLE);
        heur_dissector_add("stun", dissect_rtp_heur_app, "RTP over TURN", "rtp_stun", proto_rtp, HEURISTIC_DISABLE);
        heur_dissector_add("rtsp", dissect_rtp_heur_app, "RTP over RTSP", "rtp_rtsp", proto_rtp, HEURISTIC_DISABLE);

        rtp_hdr_ext_ed137_handle = find_dissector("rtp.ext.ed137");
        rtp_hdr_ext_ed137a_handle = find_dissector("rtp.ext.ed137a");
        dissector_add_uint("rtp.hdr_ext", RTP_ED137_SIG, rtp_hdr_ext_ed137_handle);
        dissector_add_uint("rtp.hdr_ext", RTP_ED137A_SIG, rtp_hdr_ext_ed137a_handle);
        dissector_add_for_decode_as("flip.payload", rtp_handle );


        rtcp_handle = find_dissector_add_dependency("rtcp", proto_rtp);
        stun_handle = find_dissector_add_dependency("stun-udp", proto_rtp);
        classicstun_handle = find_dissector_add_dependency("classicstun", proto_rtp);
        classicstun_heur_handle = find_dissector_add_dependency("classicstun-heur", proto_rtp);
        stun_heur_handle = find_dissector_add_dependency("stun-heur", proto_rtp);
        t38_handle = find_dissector_add_dependency("t38_udp", proto_rtp);
        zrtp_handle = find_dissector_add_dependency("zrtp", proto_rtp);

        sprt_handle = find_dissector_add_dependency("sprt", proto_rtp);
        v150fw_handle = find_dissector("v150fw");

        bta2dp_content_protection_header_scms_t = find_dissector_add_dependency("bta2dp_content_protection_header_scms_t", proto_rtp);
        btvdp_content_protection_header_scms_t = find_dissector_add_dependency("btvdp_content_protection_header_scms_t", proto_rtp);
        bta2dp_handle = find_dissector_add_dependency("bta2dp", proto_rtp);
        btvdp_handle = find_dissector_add_dependency("btvdp", proto_rtp);
        sbc_handle = find_dissector_add_dependency("sbc", proto_rtp);

        dissector_add_string("rtp_dyn_payload_type", "v150fw", v150fw_handle);

        dissector_add_for_decode_as("btl2cap.cid", rtp_handle);

        rtp_prefs_initialized = TRUE;
    } else {
        dissector_delete_uint("rtp.pt", rtp_saved_rfc2198_pt, rtp_rfc2198_handle);
    }
    dissector_add_uint("rtp.pt", rtp_rfc2198_pt, rtp_rfc2198_handle);
    rtp_saved_rfc2198_pt = rtp_rfc2198_pt;
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
