/* packet-sctp.c
 * Routines for Stream Control Transmission Protocol dissection
 * Copyright 2000-2012 Michael Tuexen <tuexen [AT] fh-muenster.de>
 * Copyright 2011 Thomas Dreibholz <dreibh [AT] iem.uni-due.de>
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
 * It should be compliant to
 * - RFC 2960
 * - RFC 3309
 * - RFC 3758
 * - RFC 4460
 * - RFC 4895
 * - RFC 4960
 * - RFC 5061
 * - RFC 6525
 * - RFC 7053
 * - http://tools.ietf.org/html/draft-stewart-sctp-pktdrprep-02
 * - http://tools.ietf.org/html/draft-ladha-sctp-nonce-02
 *
 * Still to do (so stay tuned)
 * - error checking mode
 *   * padding errors
 *   * length errors
 *   * bundling errors
 *   * value errors
 *
 * Reassembly added 2006 by Robin Seggelmann
 * TSN Tracking by Luis E. G. Ontanon (Feb 2007)
 * Copyright 2009, Varun Notibala <nbvarun [AT] gmail.com>
 *
 * PPID types updated by Thomas Dreibholz (Feb 2011)
 */

#include "config.h"

#include "ws_symbol_export.h"

#include <epan/packet.h>
#include <epan/capture_dissectors.h>
#include <epan/prefs.h>
#include <epan/exceptions.h>
#include <epan/exported_pdu.h>
#include <epan/ipproto.h>
#include <epan/addr_resolv.h>
#include <epan/sctpppids.h>
#include <epan/uat.h>
#include <epan/expert.h>
#include <epan/conversation_table.h>
#include <epan/show_exception.h>
#include <epan/decode_as.h>
#include <epan/proto_data.h>

#include <wsutil/crc32.h>
#include <wsutil/adler32.h>
#include <wsutil/utf8_entities.h>
#include <wsutil/str_util.h>

#include "packet-sctp.h"

#define LT(x, y) ((gint32)((x) - (y)) < 0)

#define ADD_PADDING(x) ((((x) + 3) >> 2) << 2)
#define UDP_TUNNELING_PORT 9899

#define MAX_NUMBER_OF_PPIDS     2
/** This is a valid PPID, but we use it to mark the end of the list */
#define LAST_PPID 0xffffffff

void proto_register_sctp(void);
void proto_reg_handoff_sctp(void);

/* Initialize the protocol and registered fields */
static int proto_sctp = -1;
static int hf_port = -1;
static int hf_source_port      = -1;
static int hf_destination_port = -1;
static int hf_verification_tag = -1;
static int hf_checksum         = -1;
static int hf_checksum_adler   = -1;
static int hf_checksum_crc32c  = -1;
static int hf_checksum_status  = -1;

static int hf_chunk_type       = -1;
static int hf_chunk_flags      = -1;
static int hf_chunk_bit_1      = -1;
static int hf_chunk_bit_2      = -1;
static int hf_chunk_length     = -1;
static int hf_chunk_padding    = -1;
static int hf_chunk_value    = -1;

static int hf_initiate_tag   = -1;
static int hf_init_chunk_initiate_tag   = -1;
static int hf_init_chunk_adv_rec_window_credit = -1;
static int hf_init_chunk_number_of_outbound_streams = -1;
static int hf_init_chunk_number_of_inbound_streams  = -1;
static int hf_init_chunk_initial_tsn    = -1;

static int hf_initack_chunk_initiate_tag   = -1;
static int hf_initack_chunk_adv_rec_window_credit = -1;
static int hf_initack_chunk_number_of_outbound_streams = -1;
static int hf_initack_chunk_number_of_inbound_streams  = -1;
static int hf_initack_chunk_initial_tsn    = -1;

/* static int hf_cumulative_tsn_ack = -1; */

static int hf_data_chunk_tsn = -1;
static int hf_data_chunk_stream_id = -1;
static int hf_data_chunk_stream_seq_number = -1;
static int hf_data_chunk_payload_proto_id = -1;
static int hf_idata_chunk_reserved = -1;
static int hf_idata_chunk_mid = -1;
static int hf_idata_chunk_fsn = -1;

static int hf_data_chunk_e_bit = -1;
static int hf_data_chunk_b_bit = -1;
static int hf_data_chunk_u_bit = -1;
static int hf_data_chunk_i_bit = -1;

static int hf_sack_chunk_ns = -1;
static int hf_sack_chunk_cumulative_tsn_ack = -1;
static int hf_sack_chunk_adv_rec_window_credit = -1;
static int hf_sack_chunk_number_of_gap_blocks = -1;
static int hf_sack_chunk_number_of_dup_tsns = -1;
static int hf_sack_chunk_gap_block_start = -1;
static int hf_sack_chunk_gap_block_end = -1;
static int hf_sack_chunk_gap_block_start_tsn = -1;
static int hf_sack_chunk_gap_block_end_tsn = -1;
static int hf_sack_chunk_number_tsns_gap_acked = -1;
static int hf_sack_chunk_duplicate_tsn = -1;

static int hf_nr_sack_chunk_ns = -1;
static int hf_nr_sack_chunk_cumulative_tsn_ack = -1;
static int hf_nr_sack_chunk_adv_rec_window_credit = -1;
static int hf_nr_sack_chunk_number_of_gap_blocks = -1;
static int hf_nr_sack_chunk_number_of_nr_gap_blocks = -1;
static int hf_nr_sack_chunk_number_of_dup_tsns = -1;
static int hf_nr_sack_chunk_reserved = -1;
static int hf_nr_sack_chunk_gap_block_start = -1;
static int hf_nr_sack_chunk_gap_block_end = -1;
static int hf_nr_sack_chunk_gap_block_start_tsn = -1;
static int hf_nr_sack_chunk_gap_block_end_tsn = -1;
static int hf_nr_sack_chunk_number_tsns_gap_acked = -1;
static int hf_nr_sack_chunk_nr_gap_block_start = -1;
static int hf_nr_sack_chunk_nr_gap_block_end = -1;
static int hf_nr_sack_chunk_nr_gap_block_start_tsn = -1;
static int hf_nr_sack_chunk_nr_gap_block_end_tsn = -1;
static int hf_nr_sack_chunk_number_tsns_nr_gap_acked = -1;
/* static int hf_nr_sack_chunk_duplicate_tsn = -1; */

static int hf_shutdown_chunk_cumulative_tsn_ack = -1;
static int hf_cookie = -1;
static int hf_cwr_chunk_lowest_tsn = -1;

static int hf_ecne_chunk_lowest_tsn = -1;
static int hf_abort_chunk_t_bit = -1;
static int hf_shutdown_complete_chunk_t_bit = -1;

static int hf_parameter_type = -1;
static int hf_parameter_length = -1;
static int hf_parameter_value = -1;
static int hf_parameter_padding = -1;
static int hf_parameter_bit_1      = -1;
static int hf_parameter_bit_2      = -1;
static int hf_ipv4_address = -1;
static int hf_ipv6_address = -1;
static int hf_heartbeat_info = -1;
static int hf_state_cookie = -1;
static int hf_cookie_preservative_increment = -1;
static int hf_hostname = -1;
static int hf_supported_address_type = -1;
static int hf_stream_reset_req_seq_nr = -1;
static int hf_stream_reset_rsp_seq_nr = -1;
static int hf_senders_last_assigned_tsn = -1;
static int hf_senders_next_tsn = -1;
static int hf_receivers_next_tsn = -1;
static int hf_stream_reset_rsp_result = -1;
static int hf_stream_reset_sid = -1;
static int hf_add_outgoing_streams_number_streams = -1;
static int hf_add_outgoing_streams_reserved = -1;
static int hf_add_incoming_streams_number_streams = -1;
static int hf_add_incoming_streams_reserved = -1;

static int hf_random_number = -1;
static int hf_chunks_to_auth = -1;
static int hf_hmac_id = -1;
static int hf_hmac = -1;
static int hf_shared_key_id = -1;
static int hf_supported_chunk_type = -1;

static int hf_cause_code = -1;
static int hf_cause_length = -1;
static int hf_cause_padding = -1;
static int hf_cause_info = -1;

static int hf_cause_stream_identifier = -1;
static int hf_cause_reserved = -1;

static int hf_cause_number_of_missing_parameters = -1;
static int hf_cause_missing_parameter_type = -1;

static int hf_cause_measure_of_staleness = -1;

static int hf_cause_tsn = -1;

static int hf_forward_tsn_chunk_tsn = -1;
static int hf_forward_tsn_chunk_sid = -1;
static int hf_forward_tsn_chunk_ssn = -1;

static int hf_i_forward_tsn_chunk_tsn = -1;
static int hf_i_forward_tsn_chunk_sid = -1;
static int hf_i_forward_tsn_chunk_flags = -1;
static int hf_i_forward_tsn_chunk_res = -1;
static int hf_i_forward_tsn_chunk_u_bit = -1;
static int hf_i_forward_tsn_chunk_mid = -1;

static int hf_asconf_ack_seq_nr = -1;
static int hf_asconf_seq_nr = -1;
static int hf_correlation_id = -1;

static int hf_adap_indication = -1;

static int hf_pktdrop_chunk_m_bit = -1;
static int hf_pktdrop_chunk_b_bit = -1;
static int hf_pktdrop_chunk_t_bit = -1;
static int hf_pktdrop_chunk_bandwidth = -1;
static int hf_pktdrop_chunk_queuesize = -1;
static int hf_pktdrop_chunk_truncated_length = -1;
static int hf_pktdrop_chunk_reserved = -1;
static int hf_pktdrop_chunk_data_field = -1;

static int hf_pad_chunk_padding_data = -1;

static int hf_sctp_reassembled_in = -1;
static int hf_sctp_duplicate = -1;
static int hf_sctp_fragments = -1;
static int hf_sctp_fragment = -1;

static int hf_sctp_retransmission = -1;
static int hf_sctp_retransmitted = -1;
static int hf_sctp_retransmitted_count = -1;
static int hf_sctp_data_rtt = -1;
static int hf_sctp_sack_rtt = -1;
static int hf_sctp_rto = -1;
static int hf_sctp_ack_tsn = -1;
static int hf_sctp_ack_frame = -1;
static int hf_sctp_acked = -1;
static int hf_sctp_retransmitted_after_ack = -1;

static int hf_sctp_assoc_index = -1;

static dissector_table_t sctp_port_dissector_table;
static dissector_table_t sctp_ppi_dissector_table;
static heur_dissector_list_t sctp_heur_subdissector_list;
static int sctp_tap = -1;
static int exported_pdu_tap = -1;

/* Initialize the subtree pointers */
static gint ett_sctp = -1;
static gint ett_sctp_chunk = -1;
static gint ett_sctp_chunk_parameter = -1;
static gint ett_sctp_chunk_cause = -1;
static gint ett_sctp_chunk_type = -1;
static gint ett_sctp_data_chunk_flags = -1;
static gint ett_sctp_sack_chunk_flags = -1;
static gint ett_sctp_nr_sack_chunk_flags = -1;
static gint ett_sctp_abort_chunk_flags = -1;
static gint ett_sctp_shutdown_complete_chunk_flags = -1;
static gint ett_sctp_pktdrop_chunk_flags = -1;
static gint ett_sctp_parameter_type= -1;
static gint ett_sctp_sack_chunk_gap_block = -1;
static gint ett_sctp_sack_chunk_gap_block_start = -1;
static gint ett_sctp_sack_chunk_gap_block_end = -1;
static gint ett_sctp_nr_sack_chunk_gap_block = -1;
static gint ett_sctp_nr_sack_chunk_gap_block_start = -1;
static gint ett_sctp_nr_sack_chunk_gap_block_end = -1;
static gint ett_sctp_nr_sack_chunk_nr_gap_block = -1;
static gint ett_sctp_nr_sack_chunk_nr_gap_block_start = -1;
static gint ett_sctp_nr_sack_chunk_nr_gap_block_end = -1;
static gint ett_sctp_unrecognized_parameter_parameter = -1;
static gint ett_sctp_i_forward_tsn_chunk_flags = -1;

static gint ett_sctp_fragments = -1;
static gint ett_sctp_fragment  = -1;

static gint ett_sctp_tsn = -1;
static gint ett_sctp_ack = -1;
static gint ett_sctp_acked = -1;
static gint ett_sctp_tsn_retransmission = -1;
static gint ett_sctp_tsn_retransmitted_count = -1;
static gint ett_sctp_tsn_retransmitted = -1;

static expert_field ei_sctp_sack_chunk_adv_rec_window_credit = EI_INIT;
static expert_field ei_sctp_nr_sack_chunk_number_tsns_gap_acked_100 = EI_INIT;
static expert_field ei_sctp_parameter_length = EI_INIT;
static expert_field ei_sctp_bad_sctp_checksum = EI_INIT;
static expert_field ei_sctp_tsn_retransmitted_more_than_twice = EI_INIT;
static expert_field ei_sctp_parameter_padding = EI_INIT;
static expert_field ei_sctp_retransmitted_after_ack = EI_INIT;
static expert_field ei_sctp_nr_sack_chunk_number_tsns_nr_gap_acked_100 = EI_INIT;
static expert_field ei_sctp_sack_chunk_gap_block_out_of_order = EI_INIT;
static expert_field ei_sctp_chunk_length_bad = EI_INIT;
static expert_field ei_sctp_tsn_retransmitted = EI_INIT;
static expert_field ei_sctp_sack_chunk_gap_block_malformed = EI_INIT;
static expert_field ei_sctp_sack_chunk_number_tsns_gap_acked_100 = EI_INIT;

WS_DLL_PUBLIC_DEF const value_string chunk_type_values[] = {
  { SCTP_DATA_CHUNK_ID,              "DATA" },
  { SCTP_INIT_CHUNK_ID,              "INIT" },
  { SCTP_INIT_ACK_CHUNK_ID,          "INIT_ACK" },
  { SCTP_SACK_CHUNK_ID,              "SACK" },
  { SCTP_HEARTBEAT_CHUNK_ID,         "HEARTBEAT" },
  { SCTP_HEARTBEAT_ACK_CHUNK_ID,     "HEARTBEAT_ACK" },
  { SCTP_ABORT_CHUNK_ID,             "ABORT" },
  { SCTP_SHUTDOWN_CHUNK_ID,          "SHUTDOWN" },
  { SCTP_SHUTDOWN_ACK_CHUNK_ID,      "SHUTDOWN_ACK" },
  { SCTP_ERROR_CHUNK_ID,             "ERROR" },
  { SCTP_COOKIE_ECHO_CHUNK_ID,       "COOKIE_ECHO" },
  { SCTP_COOKIE_ACK_CHUNK_ID,        "COOKIE_ACK" },
  { SCTP_ECNE_CHUNK_ID,              "ECNE" },
  { SCTP_CWR_CHUNK_ID,               "CWR" },
  { SCTP_SHUTDOWN_COMPLETE_CHUNK_ID, "SHUTDOWN_COMPLETE" },
  { SCTP_AUTH_CHUNK_ID,              "AUTH" },
  { SCTP_NR_SACK_CHUNK_ID,           "NR_SACK" },
  { SCTP_I_DATA_CHUNK_ID,            "I_DATA" },
  { SCTP_ASCONF_ACK_CHUNK_ID,        "ASCONF_ACK" },
  { SCTP_PKTDROP_CHUNK_ID,           "PKTDROP" },
  { SCTP_RE_CONFIG_CHUNK_ID,         "RE_CONFIG" },
  { SCTP_PAD_CHUNK_ID,               "PAD" },
  { SCTP_FORWARD_TSN_CHUNK_ID,       "FORWARD_TSN" },
  { SCTP_ASCONF_CHUNK_ID,            "ASCONF" },
  { SCTP_I_FORWARD_TSN_CHUNK_ID,     "I_FORWARD_TSN" },
  { SCTP_IETF_EXT,                   "IETF_EXTENSION" },
  { 0,                               NULL } };

/*
 * Based on http://www.iana.org/assignments/sctp-parameters
 * as of March 15th, 2012
 */
static const value_string sctp_payload_proto_id_values[] = {
  { NOT_SPECIFIED_PROTOCOL_ID,                      "not specified" },
  { IUA_PAYLOAD_PROTOCOL_ID,                        "IUA" },
  { M2UA_PAYLOAD_PROTOCOL_ID,                       "M2UA" },
  { M3UA_PAYLOAD_PROTOCOL_ID,                       "M3UA" },
  { SUA_PAYLOAD_PROTOCOL_ID,                        "SUA" },
  { M2PA_PAYLOAD_PROTOCOL_ID,                       "M2PA" },
  { V5UA_PAYLOAD_PROTOCOL_ID,                       "V5UA" },
  { H248_PAYLOAD_PROTOCOL_ID,                       "H.248/MEGACO" },
  { BICC_PAYLOAD_PROTOCOL_ID,                       "BICC/Q.2150.3" },
  { TALI_PAYLOAD_PROTOCOL_ID,                       "TALI" },
  { DUA_PAYLOAD_PROTOCOL_ID,                        "DUA" },
  { ASAP_PAYLOAD_PROTOCOL_ID,                       "ASAP" },
  { ENRP_PAYLOAD_PROTOCOL_ID,                       "ENRP" },
  { H323_PAYLOAD_PROTOCOL_ID,                       "H.323" },
  { QIPC_PAYLOAD_PROTOCOL_ID,                       "Q.IPC/Q.2150.3" },
  { SIMCO_PAYLOAD_PROTOCOL_ID,                      "SIMCO" },
  { DDP_SEG_CHUNK_PROTOCOL_ID,                      "DDP Segment Chunk" },
  { DDP_STREAM_SES_CTRL_PROTOCOL_ID,                "DDP Stream Session Control" },
  { S1AP_PAYLOAD_PROTOCOL_ID,                       "S1 Application Protocol (S1AP)" },
  { RUA_PAYLOAD_PROTOCOL_ID,                        "RUA" },
  { HNBAP_PAYLOAD_PROTOCOL_ID,                      "HNBAP" },
  { FORCES_HP_PAYLOAD_PROTOCOL_ID,                  "ForCES-HP" },
  { FORCES_MP_PAYLOAD_PROTOCOL_ID,                  "ForCES-MP" },
  { FORCES_LP_PAYLOAD_PROTOCOL_ID,                  "ForCES-LP" },
  { SBC_AP_PAYLOAD_PROTOCOL_ID,                     "SBc-AP" },
  { NBAP_PAYLOAD_PROTOCOL_ID,                       "NBAP" },
  /* Unassigned 26 */
  { X2AP_PAYLOAD_PROTOCOL_ID,                       "X2AP" },
  { IRCP_PAYLOAD_PROTOCOL_ID,                       "IRCP" },
  { LCS_AP_PAYLOAD_PROTOCOL_ID,                     "LCS-AP" },
  { MPICH2_PAYLOAD_PROTOCOL_ID,                     "MPICH2" },
  { SABP_PAYLOAD_PROTOCOL_ID,                       "SABP" },
  { FGP_PAYLOAD_PROTOCOL_ID,                        "Fractal Generator Protocol" },
  { PPP_PAYLOAD_PROTOCOL_ID,                        "Ping Pong Protocol" },
  { CALCAPP_PAYLOAD_PROTOCOL_ID,                    "CalcApp Protocol" },
  { SSP_PAYLOAD_PROTOCOL_ID,                        "Scripting Service Protocol" },
  { NPMP_CTRL_PAYLOAD_PROTOCOL_ID,                  "NetPerfMeter Control" },
  { NPMP_DATA_PAYLOAD_PROTOCOL_ID,                  "NetPerfMeter Data" },
  { ECHO_PAYLOAD_PROTOCOL_ID,                       "Echo" },
  { DISCARD_PAYLOAD_PROTOCOL_ID,                    "Discard" },
  { DAYTIME_PAYLOAD_PROTOCOL_ID,                    "Daytime" },
  { CHARGEN_PAYLOAD_PROTOCOL_ID,                    "Character Generator" },
  { PROTO_3GPP_RNA_PROTOCOL_ID,                     "3GPP RNA" },
  { PROTO_3GPP_M2AP_PROTOCOL_ID,                    "3GPP M2AP" },
  { PROTO_3GPP_M3AP_PROTOCOL_ID,                    "3GPP M3AP" },
  { SSH_PAYLOAD_PROTOCOL_ID,                        "SSH" },
  { DIAMETER_PROTOCOL_ID,                           "DIAMETER" },
  { DIAMETER_DTLS_PROTOCOL_ID,                      "DIAMETER OVER DTLS" },
  { R14P_BER_PROTOCOL_ID,                           "R14P" },
  { WEBRTC_DCEP_PROTOCOL_ID,                        "WebRTC Control" },
  { WEBRTC_STRING_PAYLOAD_PROTOCOL_ID,              "WebRTC String" },
  { WEBRTC_BINARY_PARTIAL_PAYLOAD_PROTOCOL_ID,      "WebRTC Binary Partial (Deprecated)" },
  { WEBRTC_BINARY_PAYLOAD_PROTOCOL_ID,              "WebRTC Binary" },
  { WEBRTC_STRING_PARTIAL_PAYLOAD_PROTOCOL_ID,      "WebRTC String Partial (Deprecated)" },
  { PROTO_3GPP_PUA_PAYLOAD_PROTOCOL_ID,             "3GPP PUA" },
  { WEBRTC_STRING_EMPTY_PAYLOAD_PROTOCOL_ID,        "WebRTC String Empty" },
  { WEBRTC_BINARY_EMPTY_PAYLOAD_PROTOCOL_ID,        "WebRTC Binary Empty" },
  { 0,                                              NULL } };


#define CHUNK_TYPE_LENGTH             1
#define CHUNK_FLAGS_LENGTH            1
#define CHUNK_LENGTH_LENGTH           2
#define CHUNK_HEADER_LENGTH           (CHUNK_TYPE_LENGTH + \
                                       CHUNK_FLAGS_LENGTH + \
                                       CHUNK_LENGTH_LENGTH)
#define CHUNK_HEADER_OFFSET           0
#define CHUNK_TYPE_OFFSET             CHUNK_HEADER_OFFSET
#define CHUNK_FLAGS_OFFSET            (CHUNK_TYPE_OFFSET + CHUNK_TYPE_LENGTH)
#define CHUNK_LENGTH_OFFSET           (CHUNK_FLAGS_OFFSET + CHUNK_FLAGS_LENGTH)
#define CHUNK_VALUE_OFFSET            (CHUNK_LENGTH_OFFSET + CHUNK_LENGTH_LENGTH)

#define PARAMETER_TYPE_LENGTH            2
#define PARAMETER_LENGTH_LENGTH          2
#define PARAMETER_HEADER_LENGTH          (PARAMETER_TYPE_LENGTH + PARAMETER_LENGTH_LENGTH)

#define PARAMETER_HEADER_OFFSET          0
#define PARAMETER_TYPE_OFFSET            PARAMETER_HEADER_OFFSET
#define PARAMETER_LENGTH_OFFSET          (PARAMETER_TYPE_OFFSET + PARAMETER_TYPE_LENGTH)
#define PARAMETER_VALUE_OFFSET           (PARAMETER_LENGTH_OFFSET + PARAMETER_LENGTH_LENGTH)

#define SOURCE_PORT_LENGTH      2
#define DESTINATION_PORT_LENGTH 2
#define VERIFICATION_TAG_LENGTH 4
#define CHECKSUM_LENGTH         4
#define COMMON_HEADER_LENGTH    (SOURCE_PORT_LENGTH + \
                                 DESTINATION_PORT_LENGTH + \
                                 VERIFICATION_TAG_LENGTH + \
                                 CHECKSUM_LENGTH)
#define SOURCE_PORT_OFFSET      0
#define DESTINATION_PORT_OFFSET (SOURCE_PORT_OFFSET + SOURCE_PORT_LENGTH)
#define VERIFICATION_TAG_OFFSET (DESTINATION_PORT_OFFSET + DESTINATION_PORT_LENGTH)
#define CHECKSUM_OFFSET         (VERIFICATION_TAG_OFFSET + VERIFICATION_TAG_LENGTH)

#define SCTP_CHECKSUM_NONE      0
#define SCTP_CHECKSUM_ADLER32   1
#define SCTP_CHECKSUM_CRC32C    2
#define SCTP_CHECKSUM_AUTOMATIC 3

#define FORWARD_STREAM                     0
#define BACKWARD_STREAM                    1
#define FORWARD_ADD_FORWARD_VTAG           2
#define BACKWARD_ADD_FORWARD_VTAG          3
#define BACKWARD_ADD_BACKWARD_VTAG         4
#define ASSOC_NOT_FOUND                    5

/* Default values for preferences */
static gboolean show_port_numbers          = TRUE;
static gint sctp_checksum                  = SCTP_CHECKSUM_NONE;
static gboolean enable_tsn_analysis        = TRUE;
static gboolean enable_ulp_dissection      = TRUE;
static gboolean use_reassembly             = TRUE;
/* FIXME
static gboolean show_chunk_types           = TRUE;
*/
static gboolean show_always_control_chunks = TRUE;

/* Data types and functions for generation/handling of chunk types for chunk statistics */

static const value_string chunk_enabled[] = {
  {0,"Show"},
  {1,"Hide"},
  {0, NULL}
};
typedef struct _type_field_t {
  guint type_id;
  gchar* type_name;
  guint type_enable;
} type_field_t;

static type_field_t* type_fields = NULL;
static guint num_type_fields = 0;

typedef struct _assoc_info_t {
  guint16 assoc_index;
  guint16 direction;
  gboolean vtag_reflected;
  guint16 sport;
  guint16 dport;
  guint32 verification_tag1;
  guint32 verification_tag2;
  guint32 initiate_tag;
} assoc_info_t;

typedef struct _infodata_t {
  guint16 assoc_index;
  guint16 direction;
} infodata_t;

static wmem_list_t *assoc_info_list = NULL;
static guint num_assocs = 0;

UAT_CSTRING_CB_DEF(type_fields, type_name, type_field_t)
UAT_VS_DEF(type_fields, type_enable, type_field_t, guint, 0, "Show")
UAT_DEC_CB_DEF(type_fields, type_id, type_field_t)

static void *sctp_chunk_type_copy_cb(void* n, const void* o, size_t siz _U_)
{
  type_field_t* new_rec = (type_field_t*)n;
  const type_field_t* old_rec = (const type_field_t*)o;
  if (old_rec->type_name) {
    new_rec->type_name = g_strdup(old_rec->type_name);
  } else {
    new_rec->type_name = NULL;
  }

  return new_rec;
}

static void
sctp_chunk_type_free_cb(void* r)
{
  type_field_t* rec = (type_field_t*)r;
  if (rec->type_name) g_free(rec->type_name);
}

static gboolean
sctp_chunk_type_update_cb(void *r, char **err)
{
  type_field_t *rec = (type_field_t *)r;
  char c;
  if (rec->type_name == NULL) {
    *err = g_strdup("Header name can't be empty");
    return FALSE;
  }

  g_strstrip(rec->type_name);
  if (rec->type_name[0] == 0) {
    *err = g_strdup("Header name can't be empty");
    return FALSE;
  }

  /* Check for invalid characters (to avoid asserting out when
  * registering the field).
  */
  c = proto_check_field_name(rec->type_name);
  if (c) {
    *err = g_strdup_printf("Header name can't contain '%c'", c);
    return FALSE;
  }

  *err = NULL;
  return TRUE;
}

static struct _sctp_info sctp_info;

#define RETURN_DIRECTION(direction) \
  do { \
    /*g_warning("Returning %d at %d: a-itag=0x%x, a-vtag1=0x%x, a-vtag2=0x%x, b-itag=0x%x, b-vtag1=0x%x, b-vtag2=0x%x", \
              direction, __LINE__, a->initiate_tag, a->verification_tag1, a->verification_tag2, b->initiate_tag, b->verification_tag1, b->verification_tag2);*/ \
    return direction; \
  } while (0)
static gint sctp_assoc_vtag_cmp(const assoc_info_t *a, const assoc_info_t *b)
{
  if (a == NULL || b == NULL)
    RETURN_DIRECTION(ASSOC_NOT_FOUND);

  if ((a->sport == b->sport) &&
      (a->dport == b->dport) &&
      (b->verification_tag2 != 0) &&
      (a->initiate_tag == b->verification_tag2) &&
      (a->initiate_tag == b->initiate_tag))
    RETURN_DIRECTION(FORWARD_STREAM);

  if ((a->sport == b->sport) &&
      (a->dport == b->dport) &&
      (a->verification_tag1 == b->verification_tag1) &&
      (a->initiate_tag ==  b->initiate_tag))
    RETURN_DIRECTION(FORWARD_STREAM);

  if ((a->sport == b->sport) &&
      (a->dport == b->dport) &&
      (a->verification_tag1 == b->verification_tag1) &&
      (a->verification_tag1 == 0 && a->initiate_tag != 0) &&
      (a->initiate_tag != b->initiate_tag ))
    RETURN_DIRECTION(ASSOC_NOT_FOUND);   /* two INITs that belong to different assocs */

  /* assoc known*/
  if ((a->sport == b->sport) &&
      (a->dport == b->dport) &&
      (a->verification_tag1 == b->verification_tag1) &&
      ((a->verification_tag1 != 0 ||
       (b->verification_tag2 != 0))))
    RETURN_DIRECTION(FORWARD_STREAM);

  /* ABORT, vtag reflected */
  if ((a->sport == b->sport) &&
      (a->dport == b->dport) &&
      (a->verification_tag2 == b->verification_tag2) &&
      (a->verification_tag1 == 0 && b->verification_tag1 != 0))
    RETURN_DIRECTION(FORWARD_STREAM);

  if ((a->sport == b->dport) &&
      (a->dport == b->sport) &&
      (a->verification_tag1 == b->verification_tag2) &&
      (a->verification_tag1 != 0))
    RETURN_DIRECTION(BACKWARD_STREAM);

  if ((a->sport == b->dport) &&
      (a->dport == b->sport) &&
      (a->verification_tag2 == b->verification_tag1) &&
      (a->verification_tag2 != 0))
    RETURN_DIRECTION(BACKWARD_STREAM);

  if ((a->sport == b->dport) &&
      (a->dport == b->sport) &&
      (a->verification_tag1 == b->initiate_tag) &&
      (a->verification_tag2 == 0))
    RETURN_DIRECTION(BACKWARD_ADD_BACKWARD_VTAG);

  /* ABORT, vtag reflected */
  if ((a->sport == b->dport) &&
      (a->dport == b->sport) &&
      (a->verification_tag2 == b->verification_tag1) &&
      (a->verification_tag1 == 0 && b->verification_tag2 != 0))
    RETURN_DIRECTION(BACKWARD_STREAM);

  /*forward stream verification tag can be added*/
  if ((a->sport == b->sport) &&
      (a->dport == b->dport) &&
      (a->verification_tag1 != 0) &&
      (b->verification_tag1 == 0) &&
      (b->verification_tag2 !=0))
    RETURN_DIRECTION(FORWARD_ADD_FORWARD_VTAG);

  if ((a->sport == b->dport) &&
      (a->dport == b->sport) &&
      (a->verification_tag1 == b->verification_tag2) &&
      (b->verification_tag1 == 0))
    RETURN_DIRECTION(BACKWARD_ADD_FORWARD_VTAG);

  /*backward stream verification tag can be added */
  if ((a->sport == b->dport) &&
      (a->dport == b->sport) &&
      (a->verification_tag1 != 0) &&
      (b->verification_tag1 != 0) &&
      (b->verification_tag2 == 0))
    RETURN_DIRECTION(BACKWARD_ADD_BACKWARD_VTAG);

  RETURN_DIRECTION(ASSOC_NOT_FOUND);
}
#undef RETURN_DIRECTION

static infodata_t
find_assoc_index(assoc_info_t* tmpinfo, gboolean visited)
{
  assoc_info_t *info = NULL;
  wmem_list_frame_t *elem;
  gboolean cmp = FALSE;
  infodata_t inf;
  inf.assoc_index = -1;
  inf.direction = 1;

  if (assoc_info_list == NULL) {
    assoc_info_list = wmem_list_new(wmem_file_scope());
  }

  for (elem = wmem_list_head(assoc_info_list); elem; elem = wmem_list_frame_next(elem))
  {
    info = (assoc_info_t*) wmem_list_frame_data(elem);

    if (!visited) {
      cmp = sctp_assoc_vtag_cmp(tmpinfo, info);
      if (cmp < ASSOC_NOT_FOUND) {
        switch (cmp)
        {
          case FORWARD_ADD_FORWARD_VTAG:
          case BACKWARD_ADD_FORWARD_VTAG:
            info->verification_tag1 = tmpinfo->verification_tag1;
            break;
          case BACKWARD_ADD_BACKWARD_VTAG:
            info->verification_tag2 = tmpinfo->verification_tag1;
            break;
        }
        if (cmp == FORWARD_STREAM || cmp == FORWARD_ADD_FORWARD_VTAG) {
          info->direction = 1;
        } else {
          info->direction = 2;
        }
        inf.assoc_index = info->assoc_index;
        inf.direction = info->direction;
        return inf;
      }
    } else {
      if ((tmpinfo->initiate_tag != 0 && tmpinfo->initiate_tag == info->initiate_tag) ||
          (tmpinfo->verification_tag1 != 0 && tmpinfo->verification_tag1 == info->verification_tag1) ||
          (tmpinfo->verification_tag2 != 0 && tmpinfo->verification_tag2 == info->verification_tag2)) {
        inf.assoc_index = info->assoc_index;
        inf.direction = info->direction;
        return inf;
      } else if ((tmpinfo->verification_tag1 != 0 && tmpinfo->verification_tag1 == info->verification_tag2) ||
                 (tmpinfo->verification_tag2 != 0 && tmpinfo->verification_tag2 == info->verification_tag1)) {
        inf.assoc_index = info->assoc_index;
        if (info->direction == 1)
          inf.direction = 2;
        else
          inf.direction = 1;
        return inf;
      }
    }
  }

  if (!elem && !visited) {
    info = wmem_new0(wmem_file_scope(), assoc_info_t);
    info->assoc_index = num_assocs;
    info->sport = tmpinfo->sport;
    info->dport = tmpinfo->dport;
    info->verification_tag1 = tmpinfo->verification_tag1;
    info->verification_tag2 = tmpinfo->verification_tag2;
    info->initiate_tag = tmpinfo->initiate_tag;
    num_assocs++;
    wmem_list_prepend(assoc_info_list, info);
    inf.assoc_index = info->assoc_index;
    inf.direction = 1;
  }

  return inf;
}

static void
sctp_src_prompt(packet_info *pinfo, gchar *result)
{
    g_snprintf(result, MAX_DECODE_AS_PROMPT_LEN, "source (%s%u)", UTF8_RIGHTWARDS_ARROW, pinfo->srcport);
}

static gpointer
sctp_src_value(packet_info *pinfo)
{
    return GUINT_TO_POINTER(pinfo->srcport);
}

static void
sctp_dst_prompt(packet_info *pinfo, gchar *result)
{
    g_snprintf(result, MAX_DECODE_AS_PROMPT_LEN, "destination (%s%u)", UTF8_RIGHTWARDS_ARROW, pinfo->destport);
}

static gpointer
sctp_dst_value(packet_info *pinfo)
{
    return GUINT_TO_POINTER(pinfo->destport);
}

static void
sctp_both_prompt(packet_info *pinfo, gchar *result)
{
    g_snprintf(result, MAX_DECODE_AS_PROMPT_LEN, "both (%u%s%u)", pinfo->srcport, UTF8_LEFT_RIGHT_ARROW, pinfo->destport);
}

static void
sctp_ppi_prompt1(packet_info *pinfo _U_, gchar* result)
{
    guint32 ppid;
    void *tmp = p_get_proto_data(pinfo->pool, pinfo, proto_sctp, 0);

    ppid = GPOINTER_TO_UINT(tmp);

    if (ppid == LAST_PPID) {
        g_snprintf(result, MAX_DECODE_AS_PROMPT_LEN, "PPID (none)");
    } else {
        g_snprintf(result, MAX_DECODE_AS_PROMPT_LEN, "PPID (%d)", ppid);
    }
}

static void
sctp_ppi_prompt2(packet_info *pinfo _U_, gchar* result)
{
    guint32 ppid;
    void *tmp = p_get_proto_data(pinfo->pool, pinfo, proto_sctp, 1);

    ppid = GPOINTER_TO_UINT(tmp);

    if (ppid == LAST_PPID) {
        g_snprintf(result, MAX_DECODE_AS_PROMPT_LEN, "PPID (none)");
    } else {
        g_snprintf(result, MAX_DECODE_AS_PROMPT_LEN, "PPID (%d)", ppid);
    }
}

static gpointer
sctp_ppi_value1(packet_info *pinfo)
{
    return p_get_proto_data(pinfo->pool, pinfo, proto_sctp, 0);
}

static gpointer
sctp_ppi_value2(packet_info *pinfo)
{
    return p_get_proto_data(pinfo->pool, pinfo, proto_sctp, 1);
}

static const char* sctp_conv_get_filter_type(conv_item_t* conv, conv_filter_type_e filter)
{
    if (filter == CONV_FT_SRC_PORT)
        return "sctp.srcport";

    if (filter == CONV_FT_DST_PORT)
        return "sctp.dstport";

    if (filter == CONV_FT_ANY_PORT)
        return "sctp.port";

    if(!conv) {
        return CONV_FILTER_INVALID;
    }

    if (filter == CONV_FT_SRC_ADDRESS) {
        if (conv->src_address.type == AT_IPv4)
            return "ip.src";
        if (conv->src_address.type == AT_IPv6)
            return "ipv6.src";
    }

    if (filter == CONV_FT_DST_ADDRESS) {
        if (conv->dst_address.type == AT_IPv4)
            return "ip.dst";
        if (conv->dst_address.type == AT_IPv6)
            return "ipv6.dst";
    }

    if (filter == CONV_FT_ANY_ADDRESS) {
        if (conv->src_address.type == AT_IPv4)
            return "ip.addr";
        if (conv->src_address.type == AT_IPv6)
            return "ipv6.addr";
    }

    return CONV_FILTER_INVALID;
}

static ct_dissector_info_t sctp_ct_dissector_info = {&sctp_conv_get_filter_type};

static int
sctp_conversation_packet(void *pct, packet_info *pinfo, epan_dissect_t *edt _U_, const void *vip)
{
  conv_hash_t *hash = (conv_hash_t*) pct;
  const struct _sctp_info *sctphdr=(const struct _sctp_info *)vip;

  add_conversation_table_data(hash, &sctphdr->ip_src, &sctphdr->ip_dst,
        sctphdr->sport, sctphdr->dport, 1, pinfo->fd->pkt_len, &pinfo->rel_ts, &pinfo->abs_ts, &sctp_ct_dissector_info, PT_SCTP);


  return 1;
}

static const char* sctp_host_get_filter_type(hostlist_talker_t* host, conv_filter_type_e filter)
{
    if (filter == CONV_FT_SRC_PORT)
        return "sctp.srcport";

    if (filter == CONV_FT_DST_PORT)
        return "sctp.dstport";

    if (filter == CONV_FT_ANY_PORT)
        return "sctp.port";

    if(!host) {
        return CONV_FILTER_INVALID;
    }

    if (filter == CONV_FT_SRC_ADDRESS || filter == CONV_FT_DST_ADDRESS || filter == CONV_FT_ANY_ADDRESS) {
        if (host->myaddress.type == AT_IPv4)
            return "ip.addr";
        if (host->myaddress.type == AT_IPv6)
            return "ipv6.addr";
    }


    return CONV_FILTER_INVALID;
}

static hostlist_dissector_info_t sctp_host_dissector_info = {&sctp_host_get_filter_type};

static int
sctp_hostlist_packet(void *pit, packet_info *pinfo, epan_dissect_t *edt _U_, const void *vip)
{
  conv_hash_t *hash = (conv_hash_t*) pit;
  const struct _sctp_info *sctphdr=(const struct _sctp_info *)vip;

  /* Take two "add" passes per packet, adding for each direction, ensures that all
  packets are counted properly (even if address is sending to itself)
  XXX - this could probably be done more efficiently inside hostlist_table */
  add_hostlist_table_data(hash, &sctphdr->ip_src, sctphdr->sport, TRUE, 1, pinfo->fd->pkt_len, &sctp_host_dissector_info, PT_SCTP);
  add_hostlist_table_data(hash, &sctphdr->ip_dst, sctphdr->dport, FALSE, 1, pinfo->fd->pkt_len, &sctp_host_dissector_info, PT_SCTP);

  return 1;
}

static unsigned int
sctp_adler32(tvbuff_t *tvb, unsigned int len)
{
  const guint8 *buf = tvb_get_ptr(tvb, 0, len);
  guint32 result = 1;

  result = update_adler32(result, buf, SOURCE_PORT_LENGTH + DESTINATION_PORT_LENGTH + VERIFICATION_TAG_LENGTH);
  /* handle four 0 bytes as checksum */
  result = update_adler32(result, "\0\0\0\0", 4);
  result = update_adler32(result, buf+COMMON_HEADER_LENGTH, len-COMMON_HEADER_LENGTH);

  return result;
}

static guint32
sctp_crc32c(tvbuff_t *tvb, unsigned int len)
{
  const guint8 *buf = tvb_get_ptr(tvb, 0, len);
  guint32 crc32,
          zero = 0;
  guint32 result;

  /* CRC for header */
  crc32 = crc32c_calculate_no_swap(buf, SOURCE_PORT_LENGTH + DESTINATION_PORT_LENGTH + VERIFICATION_TAG_LENGTH, CRC32C_PRELOAD);

  /* handle four 0 bytes as checksum */
  crc32 = crc32c_calculate_no_swap(&zero, 4, crc32);

  /* CRC for the rest of the packet */
  crc32 = crc32c_calculate_no_swap(&buf[COMMON_HEADER_LENGTH], len-COMMON_HEADER_LENGTH, crc32);

  result = CRC32C_SWAP(crc32);

  return ( ~result );
}

/*
 * Routines for dissecting parameters
 */

typedef struct _sctp_half_assoc_t sctp_half_assoc_t;

static void dissect_parameter(tvbuff_t *, packet_info *, proto_tree *, proto_item *, gboolean, gboolean);

static void dissect_parameters(tvbuff_t *, packet_info *, proto_tree *, proto_item *, gboolean);

static void dissect_error_cause(tvbuff_t *, packet_info *, proto_tree *);

static void dissect_error_causes(tvbuff_t *, packet_info *, proto_tree *);

static gboolean dissect_data_chunk(tvbuff_t*, guint16, packet_info*, proto_tree*, proto_tree*, proto_item*, proto_item*, sctp_half_assoc_t*, gboolean);

static void dissect_sctp_packet(tvbuff_t *, packet_info *, proto_tree *, gboolean);


/* TSN ANALYSIS CODE */

struct _sctp_half_assoc_t {
  guint32 spt;
  guint32 dpt;
  guint32 vtag;

  gboolean started;

  guint32 first_tsn; /* start */
  guint32 cumm_ack; /* rel */
  wmem_tree_t *tsns; /* sctp_tsn_t* by rel_tsn */
  wmem_tree_t *tsn_acks; /* sctp_tsn_t* by ctsn_frame */

  struct _sctp_half_assoc_t *peer;
};


typedef struct _retransmit_t {
  guint32 framenum;
  nstime_t ts;
  struct _retransmit_t *next;
} retransmit_t;

typedef struct _sctp_tsn_t {
  guint32 tsn;
  struct {
    guint32 framenum;
    nstime_t ts;
  } first_transmit;
  struct {
    guint32 framenum;
    nstime_t ts;
  } ack;
  retransmit_t *retransmit;
  guint32 retransmit_count;
  struct _sctp_tsn_t *next;
} sctp_tsn_t;


static wmem_tree_key_t*
make_address_key(guint32 spt, guint32 dpt, address *addr)
{
  wmem_tree_key_t *k = (wmem_tree_key_t *)wmem_alloc(wmem_packet_scope(), sizeof(wmem_tree_key_t)*6);

  k[0].length = 1;    k[0].key = (guint32*)wmem_memdup(wmem_packet_scope(), &spt,sizeof(spt));
  k[1].length = 1;    k[1].key = (guint32*)wmem_memdup(wmem_packet_scope(), &dpt,sizeof(dpt));
  k[2].length = 1;    k[2].key = (guint32*)(void *)&(addr->type);
  k[3].length = 1;    k[3].key = (guint32*)(void *)&(addr->len);

  k[4].length = ((addr->len/4)+1);
  k[4].key = (guint32*)wmem_alloc0(wmem_packet_scope(), ((addr->len/4)+1)*4);
  if (addr->len) memcpy(k[4].key, addr->data, addr->len);

  k[5].length = 0;    k[5].key = NULL;

  return k;
}

static wmem_tree_key_t *
make_dir_key(guint32 spt, guint32 dpt, guint32 vtag)
{
  wmem_tree_key_t *k =  (wmem_tree_key_t *)wmem_alloc(wmem_packet_scope(), sizeof(wmem_tree_key_t)*4);

  k[0].length = 1;    k[0].key = (guint32*)wmem_memdup(wmem_packet_scope(), &spt,sizeof(spt));
  k[1].length = 1;    k[1].key = (guint32*)wmem_memdup(wmem_packet_scope(), &dpt,sizeof(dpt));
  k[2].length = 1;    k[2].key = (guint32*)wmem_memdup(wmem_packet_scope(), &vtag,sizeof(vtag));
  k[3].length = 0;    k[3].key = NULL;

  return k;
}



static wmem_tree_t *dirs_by_ptvtag; /* sctp_half_assoc_t*  */
static wmem_tree_t *dirs_by_ptaddr; /* sctp_half_assoc_t**, it may contain a null pointer */

static sctp_half_assoc_t *
get_half_assoc(packet_info *pinfo, guint32 spt, guint32 dpt, guint32 vtag)
{
  sctp_half_assoc_t *ha;
  sctp_half_assoc_t **hb;
  wmem_tree_key_t *k;

  if (!enable_tsn_analysis || !vtag || pinfo->flags.in_error_pkt)
    return NULL;

  /* look for the current half_assoc by spt, dpt and vtag */

  k = make_dir_key(spt, dpt, vtag);
  if (( ha = (sctp_half_assoc_t *)wmem_tree_lookup32_array(dirs_by_ptvtag, k)  )) {
    /* found, if it has been already matched we're done */
    if (ha->peer) return ha;
  } else {
    /* not found, make a new one and add it to the table */
    ha = wmem_new0(wmem_file_scope(), sctp_half_assoc_t);
    ha->spt = spt;
    ha->dpt = dpt;
    ha->vtag = vtag;
    ha->tsns = wmem_tree_new(wmem_file_scope());
    ha->tsn_acks = wmem_tree_new(wmem_file_scope());
    ha->started = FALSE;
    ha->first_tsn= 0;
    ha->cumm_ack= 0;

    /* add this half to the table indexed by ports and vtag */
    wmem_tree_insert32_array(dirs_by_ptvtag, k, ha);
  }

  /* at this point we have an unmatched half, look for its other half using the ports and IP address */
  k = make_address_key(dpt, spt, &(pinfo->dst));

  if (( hb = (sctp_half_assoc_t **)wmem_tree_lookup32_array(dirs_by_ptaddr, k) )) {
    /*the table contains a pointer to a pointer to a half */
    if (! *hb) {
      /* if there is no half pointed by this, add the current half to the table */
      *hb = ha;
    } else {
      /* there's a half pointed by this, assume it's our peer and clear the table's pointer */
      ha->peer = *hb;
      (*hb)->peer = ha;
      *hb = NULL;
    }
  } else {
    /* we found no entry in the table: add one (using reversed ports and src addresss) so that it can be matched later */
    *(hb = (sctp_half_assoc_t **)wmem_alloc(wmem_file_scope(), sizeof(void*))) = ha;
    k = make_address_key(spt, dpt, &(pinfo->src));
    wmem_tree_insert32_array(dirs_by_ptaddr, k, hb);
  }

  return ha;
}

/*  Limit the number of retransmissions we track (to limit memory usage--and
 *  tree size--in pathological cases, for example zero window probing forever).
 */
#define MAX_RETRANS_TRACKED_PER_TSN 100

static void
tsn_tree(sctp_tsn_t *t, proto_item *tsn_item, packet_info *pinfo,
         tvbuff_t *tvb, guint32 framenum)
{
  proto_item *pi;
  proto_tree *pt;
  proto_tree *tsn_tree_pt = proto_item_add_subtree(tsn_item, ett_sctp_tsn);

  if (t->first_transmit.framenum != framenum) {
    nstime_t rto;

    pi = proto_tree_add_uint(tsn_tree_pt, hf_sctp_retransmission, tvb, 0, 0, t->first_transmit.framenum);
    pt = proto_item_add_subtree(pi, ett_sctp_tsn_retransmission);
    PROTO_ITEM_SET_GENERATED(pi);
    expert_add_info(pinfo, pi, &ei_sctp_tsn_retransmitted);

    nstime_delta( &rto, &pinfo->abs_ts, &(t->first_transmit.ts) );
    pi = proto_tree_add_time(pt, hf_sctp_rto, tvb, 0, 0, &rto);
    PROTO_ITEM_SET_GENERATED(pi);

    /* Detect reneged acks */
    /* XXX what if the frames aren't sorted by time? */
    if (t->ack.framenum && t->ack.framenum < framenum)
    {
      pi = proto_tree_add_uint_format(pt, hf_sctp_retransmitted_after_ack, tvb, 0, 0, t->ack.framenum,
                                      "This TSN was acked (in frame %u) prior to this retransmission (reneged ack?)",
                                      t->ack.framenum);
      PROTO_ITEM_SET_GENERATED(pi);
      expert_add_info(pinfo, pi, &ei_sctp_retransmitted_after_ack);
    }
  } else if (t->retransmit) {
    retransmit_t **r;
    nstime_t rto;
    char ds[64];

    if (t->retransmit_count > MAX_RETRANS_TRACKED_PER_TSN)
      g_snprintf(ds, sizeof(ds), " (only %d displayed)", MAX_RETRANS_TRACKED_PER_TSN);
    else
      ds[0] = 0;

    pi = proto_tree_add_uint_format(tsn_tree_pt,
                                    hf_sctp_retransmitted_count,
                                    tvb, 0, 0, t->retransmit_count,
                                    "This TSN was retransmitted %u time%s%s",
                                    t->retransmit_count,
                                    plurality(t->retransmit_count, "", "s"),
                                    ds);
    PROTO_ITEM_SET_GENERATED(pi);

    if (t->retransmit_count > 2)
      expert_add_info(pinfo, pi, &ei_sctp_tsn_retransmitted_more_than_twice);

    pt = proto_item_add_subtree(pi, ett_sctp_tsn_retransmitted_count);

    r = &t->retransmit;
    while (*r) {
      nstime_delta(&rto, &((*r)->ts), &pinfo->abs_ts);
      pi = proto_tree_add_uint_format(pt,
                                      hf_sctp_retransmitted,
                                      tvb, 0, 0,
                                      (*r)->framenum,
                                      "This TSN was retransmitted in frame %u (%s seconds after this frame)",
                                      (*r)->framenum,
                                      rel_time_to_secs_str(wmem_packet_scope(), &rto));
      PROTO_ITEM_SET_GENERATED(pi);
      r = &(*r)->next;
    }
  }

  if (t->ack.framenum) {
    nstime_t rtt;

    pi = proto_tree_add_uint(tsn_tree_pt, hf_sctp_acked, tvb, 0 , 0, t->ack.framenum);
    PROTO_ITEM_SET_GENERATED(pi);
    pt = proto_item_add_subtree(pi, ett_sctp_ack);

    nstime_delta( &rtt, &(t->ack.ts), &(t->first_transmit.ts) );
    pi = proto_tree_add_time(pt, hf_sctp_data_rtt, tvb, 0, 0, &rtt);
    PROTO_ITEM_SET_GENERATED(pi);
  }
}

#define RELTSN(tsn) (((tsn) < h->first_tsn) ? (tsn + (0xffffffff - (h->first_tsn)) + 1) : (tsn - h->first_tsn))

/* Returns TRUE if the tsn is a retransmission (we've seen it before), FALSE
 * otherwise.
 */
static gboolean
sctp_tsn(packet_info *pinfo,  tvbuff_t *tvb, proto_item *tsn_item,
         sctp_half_assoc_t *h, guint32 tsn)
{
  sctp_tsn_t *t;
  guint32 framenum;
  guint32 reltsn;
  gboolean is_retransmission = FALSE;

  /* no half assoc? nothing to do!*/
  if (!h)
    return(is_retransmission);


  framenum = pinfo->num;

  /*  If we're dissecting for a read filter in the GUI [tshark assigns
   *  frame numbers before running the read filter], don't do the TSN
   *  analysis.  (We can't anyway because we don't have a valid frame
   *  number...)
   *
   *  Without this check if you load a capture file in the
   *  GUI while using a read filter, every SCTP TSN is marked as a
   *  retransmission of that in frame 0.
   */
  if (framenum == 0)
    return(is_retransmission);

  /* we have not seen any tsn yet in this half assoc set the ground */
  if (! h->started) {
    h->first_tsn = tsn;
    h->started = TRUE;
  }


  reltsn = RELTSN(tsn);

  /* printf("%.3d REL TSN: %p->%p [%u] %u \n",framenum,h,h->peer,tsn,reltsn); */

  /* look for this tsn in this half's tsn table */
  if (! (t = (sctp_tsn_t *)wmem_tree_lookup32(h->tsns,reltsn) )) {
    /* no tsn found, create a new one */
    t = wmem_new0(wmem_file_scope(), sctp_tsn_t);
    t->tsn = tsn;

    t->first_transmit.framenum = framenum;
    t->first_transmit.ts = pinfo->abs_ts;

    wmem_tree_insert32(h->tsns,reltsn,t);
  }

  is_retransmission = (t->first_transmit.framenum != framenum);

  if ( (! pinfo->fd->flags.visited ) && is_retransmission ) {
    retransmit_t **r;
    int i;

    t->retransmit_count++;
    r = &t->retransmit;
    i = 0;
    while (*r && i < MAX_RETRANS_TRACKED_PER_TSN) {
      r = &(*r)->next;
      i++;
    }

    if (i <= MAX_RETRANS_TRACKED_PER_TSN) {
      *r = wmem_new0(wmem_file_scope(), retransmit_t);
      (*r)->framenum = framenum;
      (*r)->ts = pinfo->abs_ts;
    }
  }

  tsn_tree(t, tsn_item, pinfo, tvb, framenum);

  return(is_retransmission);
}

static void
ack_tree(sctp_tsn_t *t, proto_tree *acks_tree,
         tvbuff_t *tvb, packet_info *pinfo)
{
  proto_item *pi;
  proto_tree *pt;
  nstime_t rtt;
  guint framenum =  pinfo->num;

  if ( t->ack.framenum == framenum ) {
    nstime_delta( &rtt, &(t->ack.ts), &(t->first_transmit.ts) );

    pi = proto_tree_add_uint(acks_tree, hf_sctp_ack_tsn, tvb, 0 , 0, t->tsn);
    PROTO_ITEM_SET_GENERATED(pi);

    pt = proto_item_add_subtree(pi, ett_sctp_acked);

    pi = proto_tree_add_uint(pt, hf_sctp_ack_frame, tvb, 0 , 0, t->first_transmit.framenum);
    PROTO_ITEM_SET_GENERATED(pi);

    pi = proto_tree_add_time(pt, hf_sctp_sack_rtt, tvb, 0, 0, &rtt);
    PROTO_ITEM_SET_GENERATED(pi);
  }
}

static void
sctp_ack(packet_info *pinfo, tvbuff_t *tvb,  proto_tree *acks_tree,
         sctp_half_assoc_t *h, guint32 reltsn)
{
  sctp_tsn_t *t;
  guint32 framenum;


  if (!h || !h->peer)
    return;

  framenum = pinfo->num;

  /* printf("%.6d ACK: %p->%p [%u] \n",framenum,h,h->peer,reltsn); */

  t = (sctp_tsn_t *)wmem_tree_lookup32(h->peer->tsns,reltsn);

  if (t) {
    if (! t->ack.framenum) {
      sctp_tsn_t *t2;

      t->ack.framenum = framenum;
      t->ack.ts = pinfo->abs_ts;

      if (( t2 = (sctp_tsn_t *)wmem_tree_lookup32(h->peer->tsn_acks, framenum) )) {
        for(;t2->next;t2 = t2->next)
          ;

        t2->next = t;
      } else {
        wmem_tree_insert32(h->peer->tsn_acks, framenum,t);
      }
    }

    if ( t->ack.framenum == framenum)
      ack_tree(t, acks_tree, tvb, pinfo);

  } /* else {
       proto_tree_add_debug_text(acks_tree, tvb, 0 , 0, "Assoc: %p vs %p ?? %ld",h,h->peer,tsn);
       } */
}

#define RELTSNACK(tsn) (((tsn) < h->peer->first_tsn) ? ((tsn) + (0xffffffff - (h->peer->first_tsn)) + 1) : ((tsn) - h->peer->first_tsn))
static void
sctp_ack_block(packet_info *pinfo, sctp_half_assoc_t *h, tvbuff_t *tvb,
               proto_item *acks_tree, const guint32 *tsn_start_ptr,
               guint32 tsn_end)
{
  sctp_tsn_t *t;
  guint32 framenum;
  guint32 rel_start;
  guint32 rel_end;


  if ( !h || !h->peer || ! h->peer->started )
    return;

  framenum =  pinfo->num;
  rel_end = RELTSNACK(tsn_end);

  if (tsn_start_ptr) {
    rel_start = RELTSNACK(*tsn_start_ptr);
    /* printf("%.3d BACK: %p->%p [%u-%u]\n",framenum,h,h->peer,rel_start,rel_end); */
  }  else {
    rel_start = h->peer->cumm_ack;
    /* printf("%.3d CACK: %p->%p  [%u-%u]\n",framenum,h,h->peer,rel_start,rel_end); */
  }


  if ((t = (sctp_tsn_t *)wmem_tree_lookup32(h->peer->tsn_acks, framenum))) {
    for(;t;t = t->next) {
      guint32 tsn = t->tsn;

      if ( tsn  < h->peer->first_tsn ) {
        tsn += (0xffffffff - (h->peer->first_tsn)) + 1;
      } else {
        tsn -= h->peer->first_tsn;
      }

      if (t->ack.framenum == framenum && ( (!tsn_start_ptr) || rel_start <= tsn) && tsn <= rel_end)
        ack_tree(t, acks_tree, tvb, pinfo);
    }

    return;
  }

  if (PINFO_FD_VISITED(pinfo) || rel_end < rel_start || rel_end - rel_start > 0xffff0000 ) return;

  if (! tsn_start_ptr )
    h->peer->cumm_ack = rel_end + 1;

  if (rel_start <= rel_end && rel_end - rel_start < 5000 ) {
    guint32 rel_tsn, i;
    for (i=0; i <= rel_end-rel_start; i++) {
      rel_tsn = (guint32) (i+rel_start);
      sctp_ack(pinfo, tvb,  acks_tree, h, rel_tsn);
    }
  }
}

/* END TSN ANALYSIS CODE */




#define HEARTBEAT_INFO_PARAMETER_INFO_OFFSET PARAMETER_VALUE_OFFSET

static void
dissect_heartbeat_info_parameter(tvbuff_t *parameter_tvb, proto_tree *parameter_tree, proto_item *parameter_item)
{
  guint16 heartbeat_info_length;

  heartbeat_info_length = tvb_get_ntohs(parameter_tvb, PARAMETER_LENGTH_OFFSET) - PARAMETER_HEADER_LENGTH;
  if (heartbeat_info_length > 0)
    proto_tree_add_item(parameter_tree, hf_heartbeat_info, parameter_tvb, HEARTBEAT_INFO_PARAMETER_INFO_OFFSET, heartbeat_info_length, ENC_NA);
  proto_item_append_text(parameter_item, " (Information: %u byte%s)", heartbeat_info_length, plurality(heartbeat_info_length, "", "s"));
}

#define IPV4_ADDRESS_LENGTH 4
#define IPV4_ADDRESS_OFFSET PARAMETER_VALUE_OFFSET

static void
dissect_ipv4_parameter(tvbuff_t *parameter_tvb, proto_tree *parameter_tree, proto_item *parameter_item, proto_item *additional_item, gboolean dissecting_init_init_ack_chunk)
{
  if (parameter_tree) {
    proto_tree_add_item(parameter_tree, hf_ipv4_address, parameter_tvb, IPV4_ADDRESS_OFFSET, IPV4_ADDRESS_LENGTH, ENC_BIG_ENDIAN);
    proto_item_append_text(parameter_item, " (Address: %s)", tvb_ip_to_str(parameter_tvb, IPV4_ADDRESS_OFFSET));
    if (additional_item)
        proto_item_append_text(additional_item, "%s", tvb_ip_to_str(parameter_tvb, IPV4_ADDRESS_OFFSET));
  }
  if (dissecting_init_init_ack_chunk) {
    if (sctp_info.number_of_tvbs < MAXIMUM_NUMBER_OF_TVBS)
      sctp_info.tvb[sctp_info.number_of_tvbs++] = parameter_tvb;
    else
      sctp_info.incomplete = TRUE;
  }
}

#define IPV6_ADDRESS_LENGTH 16
#define IPV6_ADDRESS_OFFSET PARAMETER_VALUE_OFFSET

static void
dissect_ipv6_parameter(tvbuff_t *parameter_tvb, proto_tree *parameter_tree, proto_item *parameter_item, proto_item *additional_item, gboolean dissecting_init_init_ack_chunk)
{
  if (parameter_tree) {
    proto_tree_add_item(parameter_tree, hf_ipv6_address, parameter_tvb, IPV6_ADDRESS_OFFSET, IPV6_ADDRESS_LENGTH, ENC_NA);
    proto_item_append_text(parameter_item, " (Address: %s)", tvb_ip6_to_str(parameter_tvb, IPV6_ADDRESS_OFFSET));
    if (additional_item)
      proto_item_append_text(additional_item, "%s", tvb_ip6_to_str(parameter_tvb, IPV6_ADDRESS_OFFSET));
  }
  if (dissecting_init_init_ack_chunk) {
    if (sctp_info.number_of_tvbs < MAXIMUM_NUMBER_OF_TVBS)
      sctp_info.tvb[sctp_info.number_of_tvbs++] = parameter_tvb;
    else
      sctp_info.incomplete = TRUE;
  }
}

#define STATE_COOKIE_PARAMETER_COOKIE_OFFSET   PARAMETER_VALUE_OFFSET

static void
dissect_state_cookie_parameter(tvbuff_t *parameter_tvb, proto_tree *parameter_tree, proto_item *parameter_item)
{
  guint16 state_cookie_length;

  state_cookie_length = tvb_get_ntohs(parameter_tvb, PARAMETER_LENGTH_OFFSET) - PARAMETER_HEADER_LENGTH;
  if (state_cookie_length > 0)
    proto_tree_add_item(parameter_tree, hf_state_cookie, parameter_tvb, STATE_COOKIE_PARAMETER_COOKIE_OFFSET, state_cookie_length, ENC_NA);
  proto_item_append_text(parameter_item, " (Cookie length: %u byte%s)", state_cookie_length, plurality(state_cookie_length, "", "s"));
}

static void
dissect_unrecognized_parameters_parameter(tvbuff_t *parameter_tvb, packet_info *pinfo, proto_tree *parameter_tree)
{
  /* FIXME: Does it contain one or more parameters? */
  dissect_parameter(tvb_new_subset_remaining(parameter_tvb, PARAMETER_VALUE_OFFSET), pinfo, parameter_tree, NULL, FALSE, FALSE);
}

#define COOKIE_PRESERVATIVE_PARAMETER_INCR_LENGTH 4
#define COOKIE_PRESERVATIVE_PARAMETER_INCR_OFFSET PARAMETER_VALUE_OFFSET

static void
dissect_cookie_preservative_parameter(tvbuff_t *parameter_tvb, proto_tree *parameter_tree, proto_item *parameter_item)
{
  proto_tree_add_item(parameter_tree, hf_cookie_preservative_increment, parameter_tvb, COOKIE_PRESERVATIVE_PARAMETER_INCR_OFFSET, COOKIE_PRESERVATIVE_PARAMETER_INCR_LENGTH, ENC_BIG_ENDIAN);
  proto_item_append_text(parameter_item, " (Increment :%u msec)", tvb_get_ntohl(parameter_tvb, COOKIE_PRESERVATIVE_PARAMETER_INCR_OFFSET));
}

#define HOSTNAME_OFFSET PARAMETER_VALUE_OFFSET

static void
dissect_hostname_parameter(tvbuff_t *parameter_tvb, proto_tree *parameter_tree, proto_item *parameter_item)
{
  guint16 hostname_length;

  hostname_length = tvb_get_ntohs(parameter_tvb, PARAMETER_LENGTH_OFFSET) - PARAMETER_HEADER_LENGTH;
  proto_tree_add_item(parameter_tree, hf_hostname, parameter_tvb, HOSTNAME_OFFSET, hostname_length, ENC_ASCII|ENC_NA);
  proto_item_append_text(parameter_item, " (Hostname: %.*s)", hostname_length, tvb_format_text(parameter_tvb, HOSTNAME_OFFSET, hostname_length));

}

#define IPv4_ADDRESS_TYPE      5
#define IPv6_ADDRESS_TYPE      6
#define HOSTNAME_ADDRESS_TYPE 11

static const value_string address_types_values[] = {
  {  IPv4_ADDRESS_TYPE,    "IPv4 address"     },
  {  IPv6_ADDRESS_TYPE,    "IPv6 address"     },
  { HOSTNAME_ADDRESS_TYPE, "Hostname address" },
  {  0, NULL               }
};

#define SUPPORTED_ADDRESS_TYPE_PARAMETER_ADDRESS_TYPE_LENGTH 2

static void
dissect_supported_address_types_parameter(tvbuff_t *parameter_tvb, proto_tree *parameter_tree, proto_item *parameter_item)
{
  guint16 addr_type, number_of_addr_types, addr_type_number;
  guint offset;

  number_of_addr_types = (tvb_get_ntohs(parameter_tvb, PARAMETER_LENGTH_OFFSET) - PARAMETER_HEADER_LENGTH)
                            / SUPPORTED_ADDRESS_TYPE_PARAMETER_ADDRESS_TYPE_LENGTH;

  offset = PARAMETER_VALUE_OFFSET;
  proto_item_append_text(parameter_item, " (Supported types: ");
  for(addr_type_number = 0; addr_type_number < number_of_addr_types; addr_type_number++) {
    proto_tree_add_item(parameter_tree, hf_supported_address_type, parameter_tvb, offset, SUPPORTED_ADDRESS_TYPE_PARAMETER_ADDRESS_TYPE_LENGTH, ENC_BIG_ENDIAN);
    addr_type = tvb_get_ntohs(parameter_tvb, offset);
    switch (addr_type) {
    case IPv4_ADDRESS_TYPE:
      proto_item_append_text(parameter_item, "IPv4");
      break;
    case IPv6_ADDRESS_TYPE:
      proto_item_append_text(parameter_item, "IPv6");
      break;
    case HOSTNAME_ADDRESS_TYPE:
      proto_item_append_text(parameter_item, "hostname");
      break;
    default:
      proto_item_append_text(parameter_item, "%u", addr_type);
    }
    if (addr_type_number < (number_of_addr_types-1))
      proto_item_append_text(parameter_item, ", ");
    offset += SUPPORTED_ADDRESS_TYPE_PARAMETER_ADDRESS_TYPE_LENGTH;
  }
  proto_item_append_text(parameter_item, ")");
}

#define STREAM_RESET_SEQ_NR_LENGTH       4
#define SENDERS_LAST_ASSIGNED_TSN_LENGTH 4
#define SID_LENGTH                       2

#define STREAM_RESET_REQ_SEQ_NR_OFFSET     PARAMETER_VALUE_OFFSET
#define STREAM_RESET_REQ_RSP_SEQ_NR_OFFSET (PARAMETER_VALUE_OFFSET + STREAM_RESET_SEQ_NR_LENGTH)
#define SENDERS_LAST_ASSIGNED_TSN_OFFSET   (STREAM_RESET_REQ_RSP_SEQ_NR_OFFSET + STREAM_RESET_SEQ_NR_LENGTH)

static void
dissect_outgoing_ssn_reset_request_parameter(tvbuff_t *parameter_tvb, proto_tree *parameter_tree, proto_item *parameter_item _U_)
{
  guint length, number_of_sids, sid_number, sid_offset;

  proto_tree_add_item(parameter_tree, hf_stream_reset_req_seq_nr,   parameter_tvb, STREAM_RESET_REQ_SEQ_NR_OFFSET,     STREAM_RESET_SEQ_NR_LENGTH,       ENC_BIG_ENDIAN);
  proto_tree_add_item(parameter_tree, hf_stream_reset_rsp_seq_nr,   parameter_tvb, STREAM_RESET_REQ_RSP_SEQ_NR_OFFSET, STREAM_RESET_SEQ_NR_LENGTH,       ENC_BIG_ENDIAN);
  proto_tree_add_item(parameter_tree, hf_senders_last_assigned_tsn, parameter_tvb, SENDERS_LAST_ASSIGNED_TSN_OFFSET,   SENDERS_LAST_ASSIGNED_TSN_LENGTH, ENC_BIG_ENDIAN);

  length = tvb_get_ntohs(parameter_tvb, PARAMETER_LENGTH_OFFSET);
  sid_offset = SENDERS_LAST_ASSIGNED_TSN_OFFSET + SENDERS_LAST_ASSIGNED_TSN_LENGTH;
  if (length > PARAMETER_HEADER_LENGTH + STREAM_RESET_SEQ_NR_LENGTH + STREAM_RESET_SEQ_NR_LENGTH + SENDERS_LAST_ASSIGNED_TSN_LENGTH) {
    number_of_sids = (length - (PARAMETER_HEADER_LENGTH + STREAM_RESET_SEQ_NR_LENGTH + STREAM_RESET_SEQ_NR_LENGTH + SENDERS_LAST_ASSIGNED_TSN_LENGTH)) / SID_LENGTH;
    for(sid_number = 0; sid_number < number_of_sids; sid_number++) {
      proto_tree_add_item(parameter_tree, hf_stream_reset_sid, parameter_tvb, sid_offset, SID_LENGTH, ENC_BIG_ENDIAN);
      sid_offset += SID_LENGTH;
    }
  }
}

static void
dissect_incoming_ssn_reset_request_parameter(tvbuff_t *parameter_tvb, proto_tree *parameter_tree, proto_item *parameter_item _U_)
{
  guint length, number_of_sids, sid_number, sid_offset;

  proto_tree_add_item(parameter_tree, hf_stream_reset_req_seq_nr, parameter_tvb, STREAM_RESET_REQ_SEQ_NR_OFFSET, STREAM_RESET_SEQ_NR_LENGTH, ENC_BIG_ENDIAN);

  length = tvb_get_ntohs(parameter_tvb, PARAMETER_LENGTH_OFFSET);
  sid_offset = STREAM_RESET_REQ_SEQ_NR_OFFSET + STREAM_RESET_SEQ_NR_LENGTH;
  if (length > PARAMETER_HEADER_LENGTH + STREAM_RESET_SEQ_NR_LENGTH) {
    number_of_sids = (length - (PARAMETER_HEADER_LENGTH + STREAM_RESET_SEQ_NR_LENGTH)) / SID_LENGTH;
    for(sid_number = 0; sid_number < number_of_sids; sid_number++) {
      proto_tree_add_item(parameter_tree, hf_stream_reset_sid, parameter_tvb, sid_offset, SID_LENGTH, ENC_BIG_ENDIAN);
      sid_offset += SID_LENGTH;
    }
  }
}

#define STREAM_RESET_REQ_SEQ_NR_OFFSET PARAMETER_VALUE_OFFSET

static void
dissect_ssn_tsn_reset_request_parameter(tvbuff_t *parameter_tvb, proto_tree *parameter_tree, proto_item *parameter_item _U_)
{
  proto_tree_add_item(parameter_tree, hf_stream_reset_req_seq_nr, parameter_tvb, STREAM_RESET_REQ_SEQ_NR_OFFSET, STREAM_RESET_SEQ_NR_LENGTH, ENC_BIG_ENDIAN);
}

#define STREAM_RESET_RSP_RESULT_LENGTH 4
#define SENDERS_NEXT_TSN_LENGTH        4
#define RECEIVERS_NEXT_TSN_LENGTH      4

#define STREAM_RESET_RSP_RSP_SEQ_NR_OFFSET PARAMETER_VALUE_OFFSET
#define STREAM_RESET_RSP_RESULT_OFFSET     (STREAM_RESET_RSP_RSP_SEQ_NR_OFFSET + STREAM_RESET_SEQ_NR_LENGTH)
#define SENDERS_NEXT_TSN_OFFSET            (STREAM_RESET_RSP_RESULT_OFFSET + STREAM_RESET_RSP_RESULT_LENGTH)
#define RECEIVERS_NEXT_TSN_OFFSET          (SENDERS_NEXT_TSN_OFFSET + SENDERS_NEXT_TSN_LENGTH)

static const value_string stream_reset_result_values[] = {
  { 0, "Nothing to do"                       },
  { 1, "Performed"                           },
  { 2, "Denied"                              },
  { 3, "Error - Wrong SSN"                   },
  { 4, "Error - Request already in progress" },
  { 5, "Error - Bad sequence number"         },
  { 6, "In progress"                         },
  { 0, NULL                                  }
};


static void
dissect_re_configuration_response_parameter(tvbuff_t *parameter_tvb, proto_tree *parameter_tree, proto_item *parameter_item _U_)
{
  guint length;

  length = tvb_get_ntohs(parameter_tvb, PARAMETER_LENGTH_OFFSET);

  proto_tree_add_item(parameter_tree, hf_stream_reset_rsp_seq_nr, parameter_tvb, STREAM_RESET_RSP_RSP_SEQ_NR_OFFSET, STREAM_RESET_SEQ_NR_LENGTH,     ENC_BIG_ENDIAN);
  proto_tree_add_item(parameter_tree, hf_stream_reset_rsp_result, parameter_tvb, STREAM_RESET_RSP_RESULT_OFFSET,     STREAM_RESET_RSP_RESULT_LENGTH, ENC_BIG_ENDIAN);
  if (length >= PARAMETER_HEADER_LENGTH + STREAM_RESET_SEQ_NR_LENGTH + STREAM_RESET_RSP_RESULT_LENGTH + SENDERS_NEXT_TSN_LENGTH)
    proto_tree_add_item(parameter_tree, hf_senders_next_tsn,   parameter_tvb, SENDERS_NEXT_TSN_OFFSET,   SENDERS_NEXT_TSN_LENGTH,   ENC_BIG_ENDIAN);
  if (length >= PARAMETER_HEADER_LENGTH + STREAM_RESET_SEQ_NR_LENGTH + STREAM_RESET_RSP_RESULT_LENGTH + SENDERS_NEXT_TSN_LENGTH + RECEIVERS_NEXT_TSN_LENGTH)
    proto_tree_add_item(parameter_tree, hf_receivers_next_tsn, parameter_tvb, RECEIVERS_NEXT_TSN_OFFSET, RECEIVERS_NEXT_TSN_LENGTH, ENC_BIG_ENDIAN);
}

#define ADD_OUTGOING_STREAM_REQ_SEQ_NR_LENGTH      4
#define ADD_OUTGOING_STREAM_REQ_NUM_STREAMS_LENGTH 2
#define ADD_OUTGOING_STREAM_REQ_RESERVED_LENGTH    2

#define ADD_OUTGOING_STREAM_REQ_SEQ_NR_OFFSET      PARAMETER_VALUE_OFFSET
#define ADD_OUTGOING_STREAM_REQ_NUM_STREAMS_OFFSET (ADD_OUTGOING_STREAM_REQ_SEQ_NR_OFFSET + ADD_OUTGOING_STREAM_REQ_SEQ_NR_LENGTH)
#define ADD_OUTGOING_STREAM_REQ_RESERVED_OFFSET    (ADD_OUTGOING_STREAM_REQ_NUM_STREAMS_OFFSET + ADD_OUTGOING_STREAM_REQ_NUM_STREAMS_LENGTH)

static void
dissect_add_outgoing_streams_parameter(tvbuff_t *parameter_tvb, proto_tree *parameter_tree, proto_item *parameter_item _U_)
{
  /* guint length; */

  /* length = tvb_get_ntohs(parameter_tvb, PARAMETER_LENGTH_OFFSET); */

  proto_tree_add_item(parameter_tree, hf_stream_reset_req_seq_nr,             parameter_tvb, ADD_OUTGOING_STREAM_REQ_SEQ_NR_OFFSET,      ADD_OUTGOING_STREAM_REQ_SEQ_NR_LENGTH,      ENC_BIG_ENDIAN);
  proto_tree_add_item(parameter_tree, hf_add_outgoing_streams_number_streams, parameter_tvb, ADD_OUTGOING_STREAM_REQ_NUM_STREAMS_OFFSET, ADD_OUTGOING_STREAM_REQ_NUM_STREAMS_LENGTH, ENC_BIG_ENDIAN);
  proto_tree_add_item(parameter_tree, hf_add_outgoing_streams_reserved,       parameter_tvb, ADD_OUTGOING_STREAM_REQ_RESERVED_OFFSET,    ADD_OUTGOING_STREAM_REQ_RESERVED_LENGTH,    ENC_BIG_ENDIAN);
}

#define ADD_INCOMING_STREAM_REQ_SEQ_NR_LENGTH      4
#define ADD_INCOMING_STREAM_REQ_NUM_STREAMS_LENGTH 2
#define ADD_INCOMING_STREAM_REQ_RESERVED_LENGTH    2

#define ADD_INCOMING_STREAM_REQ_SEQ_NR_OFFSET      PARAMETER_VALUE_OFFSET
#define ADD_INCOMING_STREAM_REQ_NUM_STREAMS_OFFSET (ADD_INCOMING_STREAM_REQ_SEQ_NR_OFFSET + ADD_OUTGOING_STREAM_REQ_SEQ_NR_LENGTH)
#define ADD_INCOMING_STREAM_REQ_RESERVED_OFFSET    (ADD_INCOMING_STREAM_REQ_NUM_STREAMS_OFFSET + ADD_OUTGOING_STREAM_REQ_NUM_STREAMS_LENGTH)

static void
dissect_add_incoming_streams_parameter(tvbuff_t *parameter_tvb, proto_tree *parameter_tree, proto_item *parameter_item _U_)
{
  /* guint length; */

  /* length = tvb_get_ntohs(parameter_tvb, PARAMETER_LENGTH_OFFSET); */

  proto_tree_add_item(parameter_tree, hf_stream_reset_req_seq_nr,             parameter_tvb, ADD_INCOMING_STREAM_REQ_SEQ_NR_OFFSET,      ADD_OUTGOING_STREAM_REQ_SEQ_NR_LENGTH,    ENC_BIG_ENDIAN);
  proto_tree_add_item(parameter_tree, hf_add_incoming_streams_number_streams, parameter_tvb, ADD_INCOMING_STREAM_REQ_NUM_STREAMS_OFFSET, ADD_INCOMING_STREAM_REQ_NUM_STREAMS_LENGTH, ENC_BIG_ENDIAN);
  proto_tree_add_item(parameter_tree, hf_add_incoming_streams_reserved,       parameter_tvb, ADD_INCOMING_STREAM_REQ_RESERVED_OFFSET,    ADD_INCOMING_STREAM_REQ_RESERVED_LENGTH,    ENC_BIG_ENDIAN);
}

static void
dissect_ecn_parameter(tvbuff_t *parameter_tvb _U_)
{
}

static void
dissect_nonce_supported_parameter(tvbuff_t *parameter_tvb _U_)
{
}

#define RANDOM_NUMBER_OFFSET PARAMETER_VALUE_OFFSET

static void
dissect_random_parameter(tvbuff_t *parameter_tvb, proto_tree *parameter_tree)
{
  gint32 number_length;

  number_length = tvb_get_ntohs(parameter_tvb, PARAMETER_LENGTH_OFFSET) - PARAMETER_HEADER_LENGTH;
  if (number_length > 0)
    proto_tree_add_item(parameter_tree, hf_random_number, parameter_tvb, RANDOM_NUMBER_OFFSET, number_length, ENC_NA);
}

static void
dissect_chunks_parameter(tvbuff_t *parameter_tvb, proto_tree *parameter_tree, proto_item *parameter_item)
{
  guint16 number_of_chunks;
  guint16 chunk_number, offset;

  proto_item_append_text(parameter_item, " (Chunk types to be authenticated: ");
  number_of_chunks = tvb_get_ntohs(parameter_tvb, PARAMETER_LENGTH_OFFSET) - PARAMETER_HEADER_LENGTH;
  for(chunk_number = 0, offset = PARAMETER_VALUE_OFFSET; chunk_number < number_of_chunks; chunk_number++, offset +=  CHUNK_TYPE_LENGTH) {
    proto_tree_add_item(parameter_tree, hf_chunks_to_auth, parameter_tvb, offset, CHUNK_TYPE_LENGTH, ENC_BIG_ENDIAN);
    proto_item_append_text(parameter_item, "%s", val_to_str_const(tvb_get_guint8(parameter_tvb, offset), chunk_type_values, "Unknown"));
    if (chunk_number < (number_of_chunks - 1))
      proto_item_append_text(parameter_item, ", ");
  }
  proto_item_append_text(parameter_item, ")");
}

static const value_string hmac_id_values[] = {
  { 0x0000,         "Reserved" },
  { 0x0001,         "SHA-1"    },
  { 0x0002,         "Reserved" },
  { 0x0003,         "SHA-256"  },
  { 0,              NULL       } };

#define HMAC_ID_LENGTH 2

static void
dissect_hmac_algo_parameter(tvbuff_t *parameter_tvb, proto_tree *parameter_tree, proto_item *parameter_item)
{
  guint16 number_of_ids;
  guint16 id_number, offset;

  proto_item_append_text(parameter_item, " (Supported HMACs: ");
  number_of_ids = (tvb_get_ntohs(parameter_tvb, PARAMETER_LENGTH_OFFSET) - PARAMETER_HEADER_LENGTH) / HMAC_ID_LENGTH;
  for(id_number = 0, offset = PARAMETER_VALUE_OFFSET; id_number < number_of_ids; id_number++, offset +=  HMAC_ID_LENGTH) {
    proto_tree_add_item(parameter_tree, hf_hmac_id, parameter_tvb, offset, HMAC_ID_LENGTH, ENC_BIG_ENDIAN);
    proto_item_append_text(parameter_item, "%s", val_to_str_const(tvb_get_ntohs(parameter_tvb, offset), hmac_id_values, "Unknown"));
    if (id_number < (number_of_ids - 1))
      proto_item_append_text(parameter_item, ", ");
  }
  proto_item_append_text(parameter_item, ")");
}

static void
dissect_supported_extensions_parameter(tvbuff_t *parameter_tvb, proto_tree *parameter_tree, proto_item *parameter_item)
{
  guint16 number_of_types;
  guint16 type_number, offset;

  proto_item_append_text(parameter_item, " (Supported types: ");
  number_of_types = (tvb_get_ntohs(parameter_tvb, PARAMETER_LENGTH_OFFSET) - PARAMETER_HEADER_LENGTH) / CHUNK_TYPE_LENGTH;
  for(type_number = 0, offset = PARAMETER_VALUE_OFFSET; type_number < number_of_types; type_number++, offset +=  CHUNK_TYPE_LENGTH) {
    proto_tree_add_item(parameter_tree, hf_supported_chunk_type, parameter_tvb, offset, CHUNK_TYPE_LENGTH, ENC_BIG_ENDIAN);
    proto_item_append_text(parameter_item, "%s", val_to_str_const(tvb_get_guint8(parameter_tvb, offset), chunk_type_values, "Unknown"));
    if (type_number < (number_of_types - 1))
      proto_item_append_text(parameter_item, ", ");
  }
  proto_item_append_text(parameter_item, ")");
}

static void
dissect_forward_tsn_supported_parameter(tvbuff_t *parameter_tvb _U_)
{
}

#define CORRELATION_ID_LENGTH    4
#define CORRELATION_ID_OFFSET    PARAMETER_VALUE_OFFSET
#define ADDRESS_PARAMETER_OFFSET (CORRELATION_ID_OFFSET + CORRELATION_ID_LENGTH)

static void
dissect_add_ip_address_parameter(tvbuff_t *parameter_tvb, packet_info *pinfo, proto_tree *parameter_tree, proto_item *parameter_item)
{
  guint16 address_length;
  tvbuff_t *address_tvb;

  address_length = tvb_get_ntohs(parameter_tvb, PARAMETER_LENGTH_OFFSET) - PARAMETER_HEADER_LENGTH - CORRELATION_ID_LENGTH;

  proto_tree_add_item(parameter_tree, hf_correlation_id, parameter_tvb, CORRELATION_ID_OFFSET, CORRELATION_ID_LENGTH, ENC_BIG_ENDIAN);
  address_tvb =  tvb_new_subset(parameter_tvb, ADDRESS_PARAMETER_OFFSET,
                                MIN(address_length, tvb_captured_length_remaining(parameter_tvb, ADDRESS_PARAMETER_OFFSET)),
                                MIN(address_length, tvb_reported_length_remaining(parameter_tvb, ADDRESS_PARAMETER_OFFSET)));
  proto_item_append_text(parameter_item, " (Address: ");
  dissect_parameter(address_tvb, pinfo, parameter_tree, parameter_item, FALSE, FALSE);
  proto_item_append_text(parameter_item, ", correlation ID: %u)", tvb_get_ntohl(parameter_tvb, CORRELATION_ID_OFFSET));
}

static void
dissect_del_ip_address_parameter(tvbuff_t *parameter_tvb, packet_info *pinfo, proto_tree *parameter_tree, proto_item *parameter_item)
{
  guint16 address_length;
  tvbuff_t *address_tvb;

  address_length = tvb_get_ntohs(parameter_tvb, PARAMETER_LENGTH_OFFSET) - PARAMETER_HEADER_LENGTH - CORRELATION_ID_LENGTH;

  proto_tree_add_item(parameter_tree, hf_correlation_id, parameter_tvb, CORRELATION_ID_OFFSET, CORRELATION_ID_LENGTH, ENC_BIG_ENDIAN);
  address_tvb =  tvb_new_subset(parameter_tvb, ADDRESS_PARAMETER_OFFSET,
                                MIN(address_length, tvb_captured_length_remaining(parameter_tvb, ADDRESS_PARAMETER_OFFSET)),
                                MIN(address_length, tvb_reported_length_remaining(parameter_tvb, ADDRESS_PARAMETER_OFFSET)));
  proto_item_append_text(parameter_item, " (Address: ");
  dissect_parameter(address_tvb, pinfo, parameter_tree, parameter_item, FALSE, FALSE);
  proto_item_append_text(parameter_item, ", correlation ID: %u)", tvb_get_ntohl(parameter_tvb, CORRELATION_ID_OFFSET));
}

#define ERROR_CAUSE_IND_CASUES_OFFSET (CORRELATION_ID_OFFSET + CORRELATION_ID_LENGTH)

static void
dissect_error_cause_indication_parameter(tvbuff_t *parameter_tvb, packet_info *pinfo, proto_tree *parameter_tree)
{
  guint16 causes_length;
  tvbuff_t *causes_tvb;

  proto_tree_add_item(parameter_tree, hf_correlation_id, parameter_tvb, CORRELATION_ID_OFFSET, CORRELATION_ID_LENGTH, ENC_BIG_ENDIAN);
  causes_length = tvb_get_ntohs(parameter_tvb, PARAMETER_LENGTH_OFFSET) - PARAMETER_HEADER_LENGTH - CORRELATION_ID_LENGTH;
  causes_tvb    = tvb_new_subset(parameter_tvb, ERROR_CAUSE_IND_CASUES_OFFSET,
                                 MIN(causes_length, tvb_captured_length_remaining(parameter_tvb, ERROR_CAUSE_IND_CASUES_OFFSET)),
                                 MIN(causes_length, tvb_reported_length_remaining(parameter_tvb, ERROR_CAUSE_IND_CASUES_OFFSET)));
  dissect_error_causes(causes_tvb, pinfo,  parameter_tree);
}

static void
dissect_set_primary_address_parameter(tvbuff_t *parameter_tvb, packet_info *pinfo, proto_tree *parameter_tree, proto_item *parameter_item)
{
  guint16 address_length;
  tvbuff_t *address_tvb;

  address_length = tvb_get_ntohs(parameter_tvb, PARAMETER_LENGTH_OFFSET) - PARAMETER_HEADER_LENGTH - CORRELATION_ID_LENGTH;

  proto_tree_add_item(parameter_tree, hf_correlation_id, parameter_tvb, CORRELATION_ID_OFFSET, CORRELATION_ID_LENGTH, ENC_BIG_ENDIAN);
  address_tvb    =  tvb_new_subset(parameter_tvb, ADDRESS_PARAMETER_OFFSET,
                                   MIN(address_length, tvb_captured_length_remaining(parameter_tvb, ADDRESS_PARAMETER_OFFSET)),
                                   MIN(address_length, tvb_reported_length_remaining(parameter_tvb, ADDRESS_PARAMETER_OFFSET)));
  proto_item_append_text(parameter_item, " (Address: ");
  dissect_parameter(address_tvb, pinfo, parameter_tree, parameter_item, FALSE, FALSE);
  proto_item_append_text(parameter_item, ", correlation ID: %u)", tvb_get_ntohl(parameter_tvb, CORRELATION_ID_OFFSET));
}

static void
dissect_success_report_parameter(tvbuff_t *parameter_tvb, proto_tree *parameter_tree, proto_item *parameter_item)
{
  proto_tree_add_item(parameter_tree, hf_correlation_id, parameter_tvb, CORRELATION_ID_OFFSET, CORRELATION_ID_LENGTH, ENC_BIG_ENDIAN);
  proto_item_append_text(parameter_item, " (Correlation ID: %u)", tvb_get_ntohl(parameter_tvb, CORRELATION_ID_OFFSET));
}

#define ADAP_INDICATION_LENGTH 4
#define ADAP_INDICATION_OFFSET PARAMETER_VALUE_OFFSET

static void
dissect_adap_indication_parameter(tvbuff_t *parameter_tvb, proto_tree *parameter_tree, proto_item *parameter_item)
{
  proto_tree_add_item(parameter_tree, hf_adap_indication, parameter_tvb, ADAP_INDICATION_OFFSET, ADAP_INDICATION_LENGTH, ENC_BIG_ENDIAN);
  proto_item_append_text(parameter_item, " (Indication: %u)", tvb_get_ntohl(parameter_tvb, ADAP_INDICATION_OFFSET));
}

static void
dissect_unknown_parameter(tvbuff_t *parameter_tvb, proto_tree *parameter_tree, proto_item *parameter_item)
{
  guint16 type, parameter_value_length;

  type                   = tvb_get_ntohs(parameter_tvb, PARAMETER_TYPE_OFFSET);
  parameter_value_length = tvb_get_ntohs(parameter_tvb, PARAMETER_LENGTH_OFFSET) - PARAMETER_HEADER_LENGTH;

  if (parameter_value_length > 0)
    proto_tree_add_item(parameter_tree, hf_parameter_value, parameter_tvb, PARAMETER_VALUE_OFFSET, parameter_value_length, ENC_NA);

  proto_item_append_text(parameter_item, " (Type %u, value length: %u byte%s)", type, parameter_value_length, plurality(parameter_value_length, "", "s"));
}

#define HEARTBEAT_INFO_PARAMETER_ID               0x0001
#define IPV4ADDRESS_PARAMETER_ID                  0x0005
#define IPV6ADDRESS_PARAMETER_ID                  0x0006
#define STATE_COOKIE_PARAMETER_ID                 0x0007
#define UNREC_PARA_PARAMETER_ID                   0x0008
#define COOKIE_PRESERVATIVE_PARAMETER_ID          0x0009
#define HOSTNAME_ADDRESS_PARAMETER_ID             0x000b
#define SUPPORTED_ADDRESS_TYPES_PARAMETER_ID      0x000c
#define OUTGOING_SSN_RESET_REQUEST_PARAMETER_ID   0x000d
#define INCOMING_SSN_RESET_REQUEST_PARAMETER_ID   0x000e
#define SSN_TSN_RESET_REQUEST_PARAMETER_ID        0x000f
#define RE_CONFIGURATION_RESPONSE_PARAMETER_ID    0x0010
#define ADD_OUTGOING_STREAMS_REQUEST_PARAMETER_ID 0x0011
#define ADD_INCOMING_STREAMS_REQUEST_PARAMETER_ID 0x0012
#define ECN_PARAMETER_ID                          0x8000
#define NONCE_SUPPORTED_PARAMETER_ID              0x8001
#define RANDOM_PARAMETER_ID                       0x8002
#define CHUNKS_PARAMETER_ID                       0x8003
#define HMAC_ALGO_PARAMETER_ID                    0x8004
#define SUPPORTED_EXTENSIONS_PARAMETER_ID         0x8008
#define FORWARD_TSN_SUPPORTED_PARAMETER_ID        0xC000
#define ADD_IP_ADDRESS_PARAMETER_ID               0xC001
#define DEL_IP_ADDRESS_PARAMETER_ID               0xC002
#define ERROR_CAUSE_INDICATION_PARAMETER_ID       0xC003
#define SET_PRIMARY_ADDRESS_PARAMETER_ID          0xC004
#define SUCCESS_REPORT_PARAMETER_ID               0xC005
#define ADAP_LAYER_INDICATION_PARAMETER_ID        0xC006

static const value_string parameter_identifier_values[] = {
  { HEARTBEAT_INFO_PARAMETER_ID,               "Heartbeat info"               },
  { IPV4ADDRESS_PARAMETER_ID,                  "IPv4 address"                 },
  { IPV6ADDRESS_PARAMETER_ID,                  "IPv6 address"                 },
  { STATE_COOKIE_PARAMETER_ID,                 "State cookie"                 },
  { UNREC_PARA_PARAMETER_ID,                   "Unrecognized parameter"       },
  { COOKIE_PRESERVATIVE_PARAMETER_ID,          "Cookie preservative"          },
  { HOSTNAME_ADDRESS_PARAMETER_ID,             "Hostname address"             },
  { OUTGOING_SSN_RESET_REQUEST_PARAMETER_ID,   "Outgoing SSN reset request"   },
  { INCOMING_SSN_RESET_REQUEST_PARAMETER_ID,   "Incoming SSN reset request"   },
  { SSN_TSN_RESET_REQUEST_PARAMETER_ID,        "SSN/TSN reset request"        },
  { RE_CONFIGURATION_RESPONSE_PARAMETER_ID,    "Re-configuration response"    },
  { ADD_OUTGOING_STREAMS_REQUEST_PARAMETER_ID, "Add outgoing streams request" },
  { ADD_INCOMING_STREAMS_REQUEST_PARAMETER_ID, "Add incoming streams request" },
  { SUPPORTED_ADDRESS_TYPES_PARAMETER_ID,      "Supported address types"      },
  { ECN_PARAMETER_ID,                          "ECN"                          },
  { NONCE_SUPPORTED_PARAMETER_ID,              "Nonce supported"              },
  { RANDOM_PARAMETER_ID,                       "Random"                       },
  { CHUNKS_PARAMETER_ID,                       "Authenticated Chunk list"     },
  { HMAC_ALGO_PARAMETER_ID,                    "Requested HMAC Algorithm"     },
  { SUPPORTED_EXTENSIONS_PARAMETER_ID,         "Supported Extensions"         },
  { FORWARD_TSN_SUPPORTED_PARAMETER_ID,        "Forward TSN supported"        },
  { ADD_IP_ADDRESS_PARAMETER_ID,               "Add IP address"               },
  { DEL_IP_ADDRESS_PARAMETER_ID,               "Delete IP address"            },
  { ERROR_CAUSE_INDICATION_PARAMETER_ID,       "Error cause indication"       },
  { SET_PRIMARY_ADDRESS_PARAMETER_ID,          "Set primary address"          },
  { SUCCESS_REPORT_PARAMETER_ID,               "Success report"               },
  { ADAP_LAYER_INDICATION_PARAMETER_ID,        "Adaptation Layer Indication"  },
  { 0,                                         NULL                           } };

#define SCTP_PARAMETER_BIT_1  0x8000
#define SCTP_PARAMETER_BIT_2 0x4000

static const true_false_string sctp_parameter_bit_1_value = {
  "Skip parameter and continue processing of the chunk",
  "Stop processing of chunk"
};

static const true_false_string sctp_parameter_bit_2_value = {
  "Do report",
  "Do not report"
};

static void
dissect_parameter(tvbuff_t *parameter_tvb, packet_info *pinfo,
                  proto_tree *chunk_tree, proto_item *additional_item,
                  gboolean dissecting_init_init_ack_chunk,
                  gboolean final_parameter)
{
  guint16 type, length, padding_length, reported_length;
  proto_item *parameter_item, *type_item;
  proto_tree *parameter_tree, *type_tree;

  type            = tvb_get_ntohs(parameter_tvb, PARAMETER_TYPE_OFFSET);
  length          = tvb_get_ntohs(parameter_tvb, PARAMETER_LENGTH_OFFSET);
  reported_length = tvb_reported_length(parameter_tvb);
  padding_length  = reported_length - length;

  parameter_tree = proto_tree_add_subtree_format(chunk_tree, parameter_tvb, PARAMETER_HEADER_OFFSET, -1,
            ett_sctp_chunk_parameter, &parameter_item, "%s parameter",
            val_to_str_const(type, parameter_identifier_values, "Unknown"));
  if (final_parameter) {
    if (padding_length > 0) {
      expert_add_info(pinfo, parameter_item, &ei_sctp_parameter_padding);
    }
  } else {
    if (reported_length % 4) {
      expert_add_info_format(pinfo, parameter_item, &ei_sctp_parameter_length, "Parameter length is not padded to a multiple of 4 bytes (length=%d)", reported_length);
    }
  }

  if (!(chunk_tree || (dissecting_init_init_ack_chunk && (type == IPV4ADDRESS_PARAMETER_ID || type == IPV6ADDRESS_PARAMETER_ID))))
    return;

  if (chunk_tree) {
    type_item = proto_tree_add_item(parameter_tree, hf_parameter_type,   parameter_tvb, PARAMETER_TYPE_OFFSET,   PARAMETER_TYPE_LENGTH,   ENC_BIG_ENDIAN);
    type_tree = proto_item_add_subtree(type_item, ett_sctp_parameter_type);
    proto_tree_add_item(type_tree, hf_parameter_bit_1,  parameter_tvb, PARAMETER_TYPE_OFFSET,  PARAMETER_TYPE_LENGTH,  ENC_BIG_ENDIAN);
    proto_tree_add_item(type_tree, hf_parameter_bit_2,  parameter_tvb, PARAMETER_TYPE_OFFSET,  PARAMETER_TYPE_LENGTH,  ENC_BIG_ENDIAN);
    proto_tree_add_item(parameter_tree, hf_parameter_length, parameter_tvb, PARAMETER_LENGTH_OFFSET, PARAMETER_LENGTH_LENGTH, ENC_BIG_ENDIAN);
    /* XXX - add expert info if length is bogus? */
  } else {
    parameter_item = NULL;
    parameter_tree = NULL;
  }

  switch(type) {
  case HEARTBEAT_INFO_PARAMETER_ID:
    dissect_heartbeat_info_parameter(parameter_tvb, parameter_tree, parameter_item);
    break;
  case IPV4ADDRESS_PARAMETER_ID:
    dissect_ipv4_parameter(parameter_tvb, parameter_tree, parameter_item, additional_item, dissecting_init_init_ack_chunk);
    break;
  case IPV6ADDRESS_PARAMETER_ID:
    dissect_ipv6_parameter(parameter_tvb, parameter_tree, parameter_item, additional_item, dissecting_init_init_ack_chunk);
    break;
  case STATE_COOKIE_PARAMETER_ID:
    dissect_state_cookie_parameter(parameter_tvb, parameter_tree, parameter_item);
    break;
  case UNREC_PARA_PARAMETER_ID:
    dissect_unrecognized_parameters_parameter(parameter_tvb, pinfo,  parameter_tree);
    break;
  case COOKIE_PRESERVATIVE_PARAMETER_ID:
    dissect_cookie_preservative_parameter(parameter_tvb, parameter_tree, parameter_item);
    break;
  case HOSTNAME_ADDRESS_PARAMETER_ID:
    dissect_hostname_parameter(parameter_tvb, parameter_tree, parameter_item);
    break;
  case SUPPORTED_ADDRESS_TYPES_PARAMETER_ID:
    dissect_supported_address_types_parameter(parameter_tvb, parameter_tree, parameter_item);
    break;
  case OUTGOING_SSN_RESET_REQUEST_PARAMETER_ID:
    dissect_outgoing_ssn_reset_request_parameter(parameter_tvb, parameter_tree, parameter_item);
    break;
  case INCOMING_SSN_RESET_REQUEST_PARAMETER_ID:
    dissect_incoming_ssn_reset_request_parameter(parameter_tvb, parameter_tree, parameter_item);
    break;
  case SSN_TSN_RESET_REQUEST_PARAMETER_ID:
    dissect_ssn_tsn_reset_request_parameter(parameter_tvb, parameter_tree, parameter_item);
    break;
  case RE_CONFIGURATION_RESPONSE_PARAMETER_ID:
    dissect_re_configuration_response_parameter(parameter_tvb, parameter_tree, parameter_item);
    break;
  case ADD_OUTGOING_STREAMS_REQUEST_PARAMETER_ID:
    dissect_add_outgoing_streams_parameter(parameter_tvb, parameter_tree, parameter_item);
    break;
  case ADD_INCOMING_STREAMS_REQUEST_PARAMETER_ID:
    dissect_add_incoming_streams_parameter(parameter_tvb, parameter_tree, parameter_item);
    break;
  case ECN_PARAMETER_ID:
    dissect_ecn_parameter(parameter_tvb);
    break;
  case NONCE_SUPPORTED_PARAMETER_ID:
    dissect_nonce_supported_parameter(parameter_tvb);
    break;
  case RANDOM_PARAMETER_ID:
    dissect_random_parameter(parameter_tvb, parameter_tree);
    break;
  case CHUNKS_PARAMETER_ID:
    dissect_chunks_parameter(parameter_tvb, parameter_tree, parameter_item);
    break;
  case HMAC_ALGO_PARAMETER_ID:
    dissect_hmac_algo_parameter(parameter_tvb, parameter_tree, parameter_item);
    break;
  case SUPPORTED_EXTENSIONS_PARAMETER_ID:
    dissect_supported_extensions_parameter(parameter_tvb, parameter_tree, parameter_item);
    break;
  case FORWARD_TSN_SUPPORTED_PARAMETER_ID:
    dissect_forward_tsn_supported_parameter(parameter_tvb);
    break;
  case ADD_IP_ADDRESS_PARAMETER_ID:
    dissect_add_ip_address_parameter(parameter_tvb, pinfo, parameter_tree, parameter_item);
    break;
  case DEL_IP_ADDRESS_PARAMETER_ID:
    dissect_del_ip_address_parameter(parameter_tvb, pinfo, parameter_tree, parameter_item);
    break;
  case ERROR_CAUSE_INDICATION_PARAMETER_ID:
    dissect_error_cause_indication_parameter(parameter_tvb, pinfo, parameter_tree);
    break;
  case SET_PRIMARY_ADDRESS_PARAMETER_ID:
    dissect_set_primary_address_parameter(parameter_tvb, pinfo, parameter_tree, parameter_item);
    break;
  case SUCCESS_REPORT_PARAMETER_ID:
    dissect_success_report_parameter(parameter_tvb, parameter_tree, parameter_item);
    break;
  case ADAP_LAYER_INDICATION_PARAMETER_ID:
    dissect_adap_indication_parameter(parameter_tvb, parameter_tree, parameter_item);
    break;
  default:
    dissect_unknown_parameter(parameter_tvb, parameter_tree, parameter_item);
    break;
  }

  if (padding_length > 0) {
    proto_tree_add_item(parameter_tree, hf_parameter_padding, parameter_tvb, PARAMETER_HEADER_OFFSET + length, padding_length, ENC_NA);
  }
}

static void
dissect_parameters(tvbuff_t *parameters_tvb, packet_info *pinfo, proto_tree *tree, proto_item *additional_item, gboolean dissecting_init_init_ack_chunk)
{
  gint offset, length, total_length, remaining_length;
  tvbuff_t *parameter_tvb;
  gboolean final_parameter;

  offset = 0;
  remaining_length = tvb_reported_length_remaining(parameters_tvb, offset);
  while (remaining_length > 0) {
    if ((offset > 0) && additional_item)
      proto_item_append_text(additional_item, " ");

    length       = tvb_get_ntohs(parameters_tvb, offset + PARAMETER_LENGTH_OFFSET);
    total_length = ADD_PADDING(length);

    /*  If we have less bytes than we need, throw an exception while dissecting
     *  the parameter--not when generating the parameter_tvb below.
     */
    total_length = MIN(total_length, remaining_length);

    /* create a tvb for the parameter including the padding bytes */
    parameter_tvb  = tvb_new_subset(parameters_tvb, offset, MIN(total_length, tvb_captured_length_remaining(parameters_tvb, offset)), total_length);
    /* get rid of the handled parameter */
    offset += total_length;
    remaining_length = tvb_reported_length_remaining(parameters_tvb, offset);
    if (remaining_length > 0) {
      final_parameter = FALSE;
    } else {
      final_parameter = TRUE;
    }
    dissect_parameter(parameter_tvb, pinfo, tree, additional_item, dissecting_init_init_ack_chunk, final_parameter);
  }
}


/*
 * Code to handle error causes for ABORT and ERROR chunks
 */


#define CAUSE_CODE_LENGTH            2
#define CAUSE_LENGTH_LENGTH          2
#define CAUSE_HEADER_LENGTH          (CAUSE_CODE_LENGTH + CAUSE_LENGTH_LENGTH)

#define CAUSE_HEADER_OFFSET          0
#define CAUSE_CODE_OFFSET            CAUSE_HEADER_OFFSET
#define CAUSE_LENGTH_OFFSET          (CAUSE_CODE_OFFSET + CAUSE_CODE_LENGTH)
#define CAUSE_INFO_OFFSET            (CAUSE_LENGTH_OFFSET + CAUSE_LENGTH_LENGTH)


#define CAUSE_STREAM_IDENTIFIER_LENGTH 2
#define CAUSE_RESERVED_LENGTH 2
#define CAUSE_STREAM_IDENTIFIER_OFFSET CAUSE_INFO_OFFSET
#define CAUSE_RESERVED_OFFSET          (CAUSE_STREAM_IDENTIFIER_OFFSET + CAUSE_STREAM_IDENTIFIER_LENGTH)

static void
dissect_invalid_stream_identifier_cause(tvbuff_t *cause_tvb, proto_tree *cause_tree, proto_item *cause_item)
{
  proto_tree_add_item(cause_tree, hf_cause_stream_identifier, cause_tvb, CAUSE_STREAM_IDENTIFIER_OFFSET, CAUSE_STREAM_IDENTIFIER_LENGTH, ENC_BIG_ENDIAN);
  proto_tree_add_item(cause_tree, hf_cause_reserved,          cause_tvb, CAUSE_RESERVED_OFFSET,          CAUSE_RESERVED_LENGTH,          ENC_BIG_ENDIAN);
  proto_item_append_text(cause_item, " (SID: %u)", tvb_get_ntohs(cause_tvb, CAUSE_STREAM_IDENTIFIER_OFFSET));
}

#define CAUSE_NUMBER_OF_MISSING_PARAMETERS_LENGTH 4
#define CAUSE_MISSING_PARAMETER_TYPE_LENGTH       2

#define CAUSE_NUMBER_OF_MISSING_PARAMETERS_OFFSET CAUSE_INFO_OFFSET
#define CAUSE_FIRST_MISSING_PARAMETER_TYPE_OFFSET (CAUSE_NUMBER_OF_MISSING_PARAMETERS_OFFSET + \
                                                   CAUSE_NUMBER_OF_MISSING_PARAMETERS_LENGTH )

static void
dissect_missing_mandatory_parameters_cause(tvbuff_t *cause_tvb, proto_tree *cause_tree)
{
  guint32 number_of_missing_parameters, missing_parameter_number;
  guint   offset;

  number_of_missing_parameters = tvb_get_ntohl(cause_tvb, CAUSE_NUMBER_OF_MISSING_PARAMETERS_OFFSET);
  proto_tree_add_item(cause_tree, hf_cause_number_of_missing_parameters, cause_tvb, CAUSE_NUMBER_OF_MISSING_PARAMETERS_OFFSET, CAUSE_NUMBER_OF_MISSING_PARAMETERS_LENGTH, ENC_BIG_ENDIAN);
  offset = CAUSE_FIRST_MISSING_PARAMETER_TYPE_OFFSET;
  for(missing_parameter_number = 0; missing_parameter_number < number_of_missing_parameters; missing_parameter_number++) {
    proto_tree_add_item(cause_tree, hf_cause_missing_parameter_type, cause_tvb, offset, CAUSE_MISSING_PARAMETER_TYPE_LENGTH, ENC_BIG_ENDIAN);
    offset +=  CAUSE_MISSING_PARAMETER_TYPE_LENGTH;
  }
}

#define CAUSE_MEASURE_OF_STALENESS_LENGTH 4
#define CAUSE_MEASURE_OF_STALENESS_OFFSET CAUSE_INFO_OFFSET

static void
dissect_stale_cookie_error_cause(tvbuff_t *cause_tvb, proto_tree *cause_tree, proto_item *cause_item)
{
  proto_tree_add_item(cause_tree, hf_cause_measure_of_staleness, cause_tvb, CAUSE_MEASURE_OF_STALENESS_OFFSET, CAUSE_MEASURE_OF_STALENESS_LENGTH, ENC_BIG_ENDIAN);
  proto_item_append_text(cause_item, " (Measure: %u usec)", tvb_get_ntohl(cause_tvb, CAUSE_MEASURE_OF_STALENESS_OFFSET));
}

static void
dissect_out_of_resource_cause(tvbuff_t *cause_tvb _U_)
{
}

static void
dissect_unresolvable_address_cause(tvbuff_t *cause_tvb, packet_info *pinfo, proto_tree *cause_tree, proto_item *cause_item)
{
  guint16 parameter_length;
  tvbuff_t *parameter_tvb;

  parameter_length = tvb_get_ntohs(cause_tvb, CAUSE_LENGTH_OFFSET) - CAUSE_HEADER_LENGTH;
  parameter_tvb    = tvb_new_subset(cause_tvb, CAUSE_INFO_OFFSET,
                                    MIN(parameter_length, tvb_captured_length_remaining(cause_tvb, CAUSE_INFO_OFFSET)),
                                    MIN(parameter_length, tvb_reported_length_remaining(cause_tvb, CAUSE_INFO_OFFSET)));
  proto_item_append_text(cause_item, " (Address: ");
  dissect_parameter(parameter_tvb, pinfo, cause_tree, cause_item, FALSE, FALSE);
  proto_item_append_text(cause_item, ")");
}

static gboolean
dissect_sctp_chunk(tvbuff_t *chunk_tvb, packet_info *pinfo, proto_tree *tree, proto_tree *sctp_tree, sctp_half_assoc_t *assoc, gboolean useinfo);

static void
dissect_unrecognized_chunk_type_cause(tvbuff_t *cause_tvb,  packet_info *pinfo, proto_tree *cause_tree, proto_item *cause_item)
{
  guint16 chunk_length;
  guint8 unrecognized_type;
  tvbuff_t *unrecognized_chunk_tvb;

  chunk_length = tvb_get_ntohs(cause_tvb, CAUSE_LENGTH_OFFSET) - CAUSE_HEADER_LENGTH;
  unrecognized_chunk_tvb = tvb_new_subset(cause_tvb, CAUSE_INFO_OFFSET,
                                          MIN(chunk_length, tvb_captured_length_remaining(cause_tvb, CAUSE_INFO_OFFSET)),
                                          MIN(chunk_length, tvb_reported_length_remaining(cause_tvb, CAUSE_INFO_OFFSET)));
  dissect_sctp_chunk(unrecognized_chunk_tvb, pinfo, cause_tree,cause_tree, NULL, FALSE);
  unrecognized_type   = tvb_get_guint8(unrecognized_chunk_tvb, CHUNK_TYPE_OFFSET);
  proto_item_append_text(cause_item, " (Type: %u (%s))", unrecognized_type, val_to_str_const(unrecognized_type, chunk_type_values, "unknown"));
}

static void
dissect_invalid_mandatory_parameter_cause(tvbuff_t *cause_tvb _U_)
{
}

static void
dissect_unrecognized_parameters_cause(tvbuff_t *cause_tvb, packet_info *pinfo, proto_tree *cause_tree)
{
  guint16 cause_info_length;
  tvbuff_t *unrecognized_parameters_tvb;

  cause_info_length = tvb_get_ntohs(cause_tvb, CAUSE_LENGTH_OFFSET) - CAUSE_HEADER_LENGTH;

  unrecognized_parameters_tvb = tvb_new_subset(cause_tvb, CAUSE_INFO_OFFSET,
                                               MIN(cause_info_length, tvb_captured_length_remaining(cause_tvb, CAUSE_INFO_OFFSET)),
                                               MIN(cause_info_length, tvb_reported_length_remaining(cause_tvb, CAUSE_INFO_OFFSET)));
  dissect_parameters(unrecognized_parameters_tvb, pinfo, cause_tree, NULL, FALSE);
}

#define CAUSE_TSN_LENGTH 4
#define CAUSE_TSN_OFFSET CAUSE_INFO_OFFSET

static void
dissect_no_user_data_cause(tvbuff_t *cause_tvb, proto_tree *cause_tree, proto_item *cause_item)
{
  proto_tree_add_item(cause_tree, hf_cause_tsn, cause_tvb, CAUSE_TSN_OFFSET, CAUSE_TSN_LENGTH, ENC_BIG_ENDIAN);
  proto_item_append_text(cause_item, " (TSN: %u)", tvb_get_ntohl(cause_tvb, CAUSE_TSN_OFFSET));
}

static void
dissect_cookie_received_while_shutting_down_cause(tvbuff_t *cause_tvb _U_)
{
}

static void
dissect_restart_with_new_address_cause(tvbuff_t *cause_tvb, packet_info *pinfo, proto_tree *cause_tree, proto_item *cause_item)
{
  guint16 cause_info_length;
  tvbuff_t *parameter_tvb;

  cause_info_length = tvb_get_ntohs(cause_tvb, CAUSE_LENGTH_OFFSET) - CAUSE_HEADER_LENGTH;
  parameter_tvb     = tvb_new_subset(cause_tvb, CAUSE_INFO_OFFSET,
                                     MIN(cause_info_length, tvb_captured_length_remaining(cause_tvb, CAUSE_INFO_OFFSET)),
                                     MIN(cause_info_length, tvb_reported_length_remaining(cause_tvb, CAUSE_INFO_OFFSET)));
  proto_item_append_text(cause_item, " (New addresses: ");
  dissect_parameters(parameter_tvb, pinfo, cause_tree, cause_item, FALSE);
  proto_item_append_text(cause_item, ")");
}

static void
dissect_user_initiated_abort_cause(tvbuff_t *cause_tvb, proto_tree *cause_tree)
{
  guint16 cause_info_length;

  cause_info_length = tvb_get_ntohs(cause_tvb, CAUSE_LENGTH_OFFSET) - CAUSE_HEADER_LENGTH;
  if (cause_info_length > 0)
    proto_tree_add_item(cause_tree, hf_cause_info, cause_tvb, CAUSE_INFO_OFFSET, cause_info_length, ENC_NA);
}

static void
dissect_protocol_violation_cause(tvbuff_t *cause_tvb, proto_tree *cause_tree)
{
  guint16 cause_info_length;

  cause_info_length = tvb_get_ntohs(cause_tvb, CAUSE_LENGTH_OFFSET) - CAUSE_HEADER_LENGTH;
  if (cause_info_length > 0)
    proto_tree_add_item(cause_tree, hf_cause_info, cause_tvb, CAUSE_INFO_OFFSET, cause_info_length, ENC_NA);
}

static void
dissect_delete_last_address_cause(tvbuff_t *cause_tvb, packet_info *pinfo, proto_tree *cause_tree, proto_item *cause_item)
{
  guint16 cause_info_length;
  tvbuff_t *parameter_tvb;

  cause_info_length = tvb_get_ntohs(cause_tvb, CAUSE_LENGTH_OFFSET) - CAUSE_HEADER_LENGTH;
  parameter_tvb     = tvb_new_subset(cause_tvb, CAUSE_INFO_OFFSET,
                                     MIN(cause_info_length, tvb_captured_length_remaining(cause_tvb, CAUSE_INFO_OFFSET)),
                                     MIN(cause_info_length, tvb_reported_length_remaining(cause_tvb, CAUSE_INFO_OFFSET)));
  proto_item_append_text(cause_item, " (Last address: ");
  dissect_parameter(parameter_tvb, pinfo, cause_tree, cause_item, FALSE, FALSE);
  proto_item_append_text(cause_item, ")");
}

static void
dissect_resource_outage_cause(tvbuff_t *cause_tvb, packet_info *pinfo, proto_tree *cause_tree)
{
  guint16 cause_info_length;
  tvbuff_t *parameter_tvb;

  cause_info_length = tvb_get_ntohs(cause_tvb, CAUSE_LENGTH_OFFSET) - CAUSE_HEADER_LENGTH;
  parameter_tvb     = tvb_new_subset(cause_tvb, CAUSE_INFO_OFFSET,
                                     MIN(cause_info_length, tvb_captured_length_remaining(cause_tvb, CAUSE_INFO_OFFSET)),
                                     MIN(cause_info_length, tvb_reported_length_remaining(cause_tvb, CAUSE_INFO_OFFSET)));
  dissect_parameter(parameter_tvb, pinfo, cause_tree, NULL, FALSE, FALSE);
}

static void
dissect_delete_source_address_cause(tvbuff_t *cause_tvb, packet_info *pinfo, proto_tree *cause_tree, proto_item *cause_item)
{
  guint16 cause_info_length;
  tvbuff_t *parameter_tvb;

  cause_info_length = tvb_get_ntohs(cause_tvb, CAUSE_LENGTH_OFFSET) - CAUSE_HEADER_LENGTH;
  parameter_tvb     = tvb_new_subset(cause_tvb, CAUSE_INFO_OFFSET,
                                     MIN(cause_info_length, tvb_captured_length_remaining(cause_tvb, CAUSE_INFO_OFFSET)),
                                     MIN(cause_info_length, tvb_reported_length_remaining(cause_tvb, CAUSE_INFO_OFFSET)));
  proto_item_append_text(cause_item, " (Deleted address: ");
  dissect_parameter(parameter_tvb, pinfo, cause_tree, cause_item, FALSE, FALSE);
  proto_item_append_text(cause_item, ")");
}

static void
dissect_request_refused_cause(tvbuff_t *cause_tvb, packet_info *pinfo, proto_tree *cause_tree)
{
  guint16 cause_info_length;
  tvbuff_t *parameter_tvb;

  cause_info_length = tvb_get_ntohs(cause_tvb, CAUSE_LENGTH_OFFSET) - CAUSE_HEADER_LENGTH;
  parameter_tvb     = tvb_new_subset(cause_tvb, CAUSE_INFO_OFFSET,
                                     MIN(cause_info_length, tvb_captured_length_remaining(cause_tvb, CAUSE_INFO_OFFSET)),
                                     MIN(cause_info_length, tvb_reported_length_remaining(cause_tvb, CAUSE_INFO_OFFSET)));
  dissect_parameter(parameter_tvb, pinfo, cause_tree, NULL, FALSE, FALSE);
}

static void
dissect_unsupported_hmac_id_cause(tvbuff_t *cause_tvb, packet_info *pinfo _U_, proto_tree *cause_tree)
{
  proto_tree_add_item(cause_tree, hf_hmac_id, cause_tvb, CAUSE_INFO_OFFSET, HMAC_ID_LENGTH, ENC_BIG_ENDIAN);
}

static void
dissect_unknown_cause(tvbuff_t *cause_tvb, proto_tree *cause_tree, proto_item *cause_item)
{
  guint16 cause_info_length;

  cause_info_length = tvb_get_ntohs(cause_tvb, CAUSE_LENGTH_OFFSET) - CAUSE_HEADER_LENGTH;
  if (cause_info_length > 0)
    proto_tree_add_item(cause_tree, hf_cause_info, cause_tvb, CAUSE_INFO_OFFSET, cause_info_length, ENC_NA);
  proto_item_append_text(cause_item, " (Code: %u, information length: %u byte%s)", tvb_get_ntohs(cause_tvb, CAUSE_CODE_OFFSET), cause_info_length, plurality(cause_info_length, "", "s"));
}

#define INVALID_STREAM_IDENTIFIER                  0x01
#define MISSING_MANDATORY_PARAMETERS               0x02
#define STALE_COOKIE_ERROR                         0x03
#define OUT_OF_RESOURCE                            0x04
#define UNRESOLVABLE_ADDRESS                       0x05
#define UNRECOGNIZED_CHUNK_TYPE                    0x06
#define INVALID_MANDATORY_PARAMETER                0x07
#define UNRECOGNIZED_PARAMETERS                    0x08
#define NO_USER_DATA                               0x09
#define COOKIE_RECEIVED_WHILE_SHUTTING_DOWN        0x0a
#define RESTART_WITH_NEW_ADDRESSES                 0x0b
#define USER_INITIATED_ABORT                       0x0c
#define PROTOCOL_VIOLATION                         0x0d
#define REQUEST_TO_DELETE_LAST_ADDRESS             0x00a0
#define OPERATION_REFUSED_DUE_TO_RESOURCE_SHORTAGE 0x00a1
#define REQUEST_TO_DELETE_SOURCE_ADDRESS           0x00a2
#define ABORT_DUE_TO_ILLEGAL_ASCONF                0x00a3
#define REQUEST_REFUSED                            0x00a4
#define UNSUPPORTED_HMAC_ID                        0x0105

static const value_string cause_code_values[] = {
  { INVALID_STREAM_IDENTIFIER,                  "Invalid stream identifier" },
  { MISSING_MANDATORY_PARAMETERS,               "Missing mandatory parameter" },
  { STALE_COOKIE_ERROR,                         "Stale cookie error" },
  { OUT_OF_RESOURCE,                            "Out of resource" },
  { UNRESOLVABLE_ADDRESS,                       "Unresolvable address" },
  { UNRECOGNIZED_CHUNK_TYPE,                    "Unrecognized chunk type" },
  { INVALID_MANDATORY_PARAMETER,                "Invalid mandatory parameter" },
  { UNRECOGNIZED_PARAMETERS,                    "Unrecognized parameters" },
  { NO_USER_DATA,                               "No user data" },
  { COOKIE_RECEIVED_WHILE_SHUTTING_DOWN,        "Cookie received while shutting down" },
  { RESTART_WITH_NEW_ADDRESSES,                 "Restart of an association with new addresses" },
  { USER_INITIATED_ABORT,                       "User initiated ABORT" },
  { PROTOCOL_VIOLATION,                         "Protocol violation" },
  { REQUEST_TO_DELETE_LAST_ADDRESS,             "Request to delete last address" },
  { OPERATION_REFUSED_DUE_TO_RESOURCE_SHORTAGE, "Operation refused due to resource shortage" },
  { REQUEST_TO_DELETE_SOURCE_ADDRESS,           "Request to delete source address" },
  { ABORT_DUE_TO_ILLEGAL_ASCONF,                "Association Aborted due to illegal ASCONF-ACK" },
  { REQUEST_REFUSED,                            "Request refused - no authorization" },
  { UNSUPPORTED_HMAC_ID,                        "Unsupported HMAC identifier" },
  { 0,                                          NULL } };


static void
dissect_error_cause(tvbuff_t *cause_tvb, packet_info *pinfo, proto_tree *chunk_tree)
{
  guint16 code, length, padding_length;
  proto_item *cause_item;
  proto_tree *cause_tree;

  code           = tvb_get_ntohs(cause_tvb, CAUSE_CODE_OFFSET);
  length         = tvb_get_ntohs(cause_tvb, CAUSE_LENGTH_OFFSET);
  padding_length = tvb_reported_length(cause_tvb) - length;

  cause_tree = proto_tree_add_subtree_format(chunk_tree, cause_tvb, CAUSE_HEADER_OFFSET, -1,
            ett_sctp_chunk_cause, &cause_item, "%s cause", val_to_str_const(code, cause_code_values, "Unknown"));

  proto_tree_add_item(cause_tree, hf_cause_code, cause_tvb,   CAUSE_CODE_OFFSET,   CAUSE_CODE_LENGTH,   ENC_BIG_ENDIAN);
  proto_tree_add_item(cause_tree, hf_cause_length, cause_tvb, CAUSE_LENGTH_OFFSET, CAUSE_LENGTH_LENGTH, ENC_BIG_ENDIAN);
  /* XXX - add expert info if length is bogus? */

  switch(code) {
  case INVALID_STREAM_IDENTIFIER:
    dissect_invalid_stream_identifier_cause(cause_tvb, cause_tree, cause_item);
    break;
  case MISSING_MANDATORY_PARAMETERS:
    dissect_missing_mandatory_parameters_cause(cause_tvb, cause_tree);
    break;
  case STALE_COOKIE_ERROR:
    dissect_stale_cookie_error_cause(cause_tvb, cause_tree, cause_item);
    break;
  case OUT_OF_RESOURCE:
    dissect_out_of_resource_cause(cause_tvb);
    break;
  case UNRESOLVABLE_ADDRESS:
    dissect_unresolvable_address_cause(cause_tvb, pinfo, cause_tree, cause_item);
    break;
  case UNRECOGNIZED_CHUNK_TYPE:
    dissect_unrecognized_chunk_type_cause(cause_tvb, pinfo, cause_tree, cause_item);
    break;
  case INVALID_MANDATORY_PARAMETER:
    dissect_invalid_mandatory_parameter_cause(cause_tvb);
    break;
  case UNRECOGNIZED_PARAMETERS:
    dissect_unrecognized_parameters_cause(cause_tvb, pinfo, cause_tree);
    break;
  case NO_USER_DATA:
    dissect_no_user_data_cause(cause_tvb, cause_tree, cause_item);
    break;
  case COOKIE_RECEIVED_WHILE_SHUTTING_DOWN:
    dissect_cookie_received_while_shutting_down_cause(cause_tvb);
    break;
  case RESTART_WITH_NEW_ADDRESSES:
    dissect_restart_with_new_address_cause(cause_tvb, pinfo, cause_tree, cause_item);
    break;
  case USER_INITIATED_ABORT:
    dissect_user_initiated_abort_cause(cause_tvb, cause_tree);
    break;
  case PROTOCOL_VIOLATION:
    dissect_protocol_violation_cause(cause_tvb, cause_tree);
    break;
  case REQUEST_TO_DELETE_LAST_ADDRESS:
    dissect_delete_last_address_cause(cause_tvb, pinfo, cause_tree, cause_item);
    break;
  case OPERATION_REFUSED_DUE_TO_RESOURCE_SHORTAGE:
    dissect_resource_outage_cause(cause_tvb, pinfo, cause_tree);
    break;
  case REQUEST_TO_DELETE_SOURCE_ADDRESS:
    dissect_delete_source_address_cause(cause_tvb, pinfo, cause_tree, cause_item);
    break;
  case REQUEST_REFUSED:
    dissect_request_refused_cause(cause_tvb, pinfo, cause_tree);
    break;
  case UNSUPPORTED_HMAC_ID:
    dissect_unsupported_hmac_id_cause(cause_tvb, pinfo, cause_tree);
    break;
  default:
    dissect_unknown_cause(cause_tvb, cause_tree, cause_item);
    break;
  }

  if (padding_length > 0)
    proto_tree_add_item(cause_tree, hf_cause_padding, cause_tvb, CAUSE_HEADER_OFFSET + length, padding_length, ENC_NA);
}

static void
dissect_error_causes(tvbuff_t *causes_tvb, packet_info *pinfo, proto_tree *tree)
{
  gint offset, length, total_length, remaining_length;
  tvbuff_t *cause_tvb;

  offset = 0;
  while((remaining_length = tvb_reported_length_remaining(causes_tvb, offset))) {
    length       = tvb_get_ntohs(causes_tvb, offset + CAUSE_LENGTH_OFFSET);
    total_length = ADD_PADDING(length);

    /*  If we have less bytes than we need, throw an exception while dissecting
     *  the cause--not when generating the causes_tvb below.
     */
    total_length = MIN(total_length, remaining_length);

    /* create a tvb for the parameter including the padding bytes */
    cause_tvb    = tvb_new_subset(causes_tvb, offset, MIN(total_length, tvb_captured_length_remaining(causes_tvb, offset)), total_length);

    dissect_error_cause(cause_tvb, pinfo, tree);

    /* get rid of the handled cause */
    offset += total_length;
  }
}


/*
 * Code to actually dissect the packets
*/

static gboolean try_heuristic_first = FALSE;

static gboolean
dissect_payload(tvbuff_t *payload_tvb, packet_info *pinfo, proto_tree *tree, guint32 ppi)
{
  guint32 low_port, high_port;
  heur_dtbl_entry_t *hdtbl_entry;

  if (enable_ulp_dissection) {
    if (try_heuristic_first) {
      /* do lookup with the heuristic subdissector table */
      if (dissector_try_heuristic(sctp_heur_subdissector_list, payload_tvb, pinfo, tree, &hdtbl_entry, GUINT_TO_POINTER(ppi)))
         return TRUE;
    }

    /* Do lookups with the subdissector table.

       When trying port numbers, we try the port number with the lower value
       first, followed by the port number with the higher value.  This means
       that, for packets where a dissector is registered for *both* port
       numbers, and where there's no match on the PPI:

    1) we pick the same dissector for traffic going in both directions;

    2) we prefer the port number that's more likely to be the right
       one (as that prefers well-known ports to reserved ports);

       although there is, of course, no guarantee that any such strategy
       will always pick the right port number.

       XXX - we ignore port numbers of 0, as some dissectors use a port
       number of 0 to disable the port. */
    if (dissector_try_uint_new(sctp_ppi_dissector_table, ppi, payload_tvb, pinfo, tree, TRUE, GUINT_TO_POINTER(ppi)))
      return TRUE;
    if (pinfo->srcport > pinfo->destport) {
      low_port = pinfo->destport;
      high_port = pinfo->srcport;
    } else {
      low_port = pinfo->srcport;
      high_port = pinfo->destport;
    }
    if (low_port != 0 &&
        dissector_try_uint_new(sctp_port_dissector_table, low_port, payload_tvb, pinfo, tree, TRUE, GUINT_TO_POINTER(ppi)))
      return TRUE;
    if (high_port != 0 &&
        dissector_try_uint_new(sctp_port_dissector_table, high_port, payload_tvb, pinfo, tree, TRUE, GUINT_TO_POINTER(ppi)))
      return TRUE;

    if (!try_heuristic_first) {
      /* do lookup with the heuristic subdissector table */
      if (dissector_try_heuristic(sctp_heur_subdissector_list, payload_tvb, pinfo, tree, &hdtbl_entry, GUINT_TO_POINTER(ppi)))
         return TRUE;
    }
  }
  /* Oh, well, we don't know this; dissect it as data. */
  call_data_dissector(payload_tvb, pinfo, tree);
  return TRUE;
}

#define DATA_CHUNK_TSN_LENGTH         4
#define DATA_CHUNK_STREAM_ID_LENGTH   2
#define DATA_CHUNK_STREAM_SEQ_NUMBER_LENGTH 2
#define DATA_CHUNK_PAYLOAD_PROTOCOL_ID_LENGTH 4
#define I_DATA_CHUNK_RESERVED_LENGTH 2
#define I_DATA_CHUNK_MID_LENGTH 4
#define I_DATA_CHUNK_PAYLOAD_PROTOCOL_ID_LENGTH 4
#define I_DATA_CHUNK_FSN_LENGTH 4

#define DATA_CHUNK_TSN_OFFSET         (CHUNK_VALUE_OFFSET + 0)
#define DATA_CHUNK_STREAM_ID_OFFSET   (DATA_CHUNK_TSN_OFFSET + DATA_CHUNK_TSN_LENGTH)
#define DATA_CHUNK_STREAM_SEQ_NUMBER_OFFSET (DATA_CHUNK_STREAM_ID_OFFSET + \
                                             DATA_CHUNK_STREAM_ID_LENGTH)
#define DATA_CHUNK_PAYLOAD_PROTOCOL_ID_OFFSET (DATA_CHUNK_STREAM_SEQ_NUMBER_OFFSET + \
                                               DATA_CHUNK_STREAM_SEQ_NUMBER_LENGTH)
#define DATA_CHUNK_PAYLOAD_OFFSET     (DATA_CHUNK_PAYLOAD_PROTOCOL_ID_OFFSET + \
                                       DATA_CHUNK_PAYLOAD_PROTOCOL_ID_LENGTH)
#define I_DATA_CHUNK_RESERVED_OFFSET  (DATA_CHUNK_STREAM_ID_OFFSET + \
                                       DATA_CHUNK_STREAM_ID_LENGTH)
#define I_DATA_CHUNK_MID_OFFSET       (I_DATA_CHUNK_RESERVED_OFFSET + \
                                       I_DATA_CHUNK_RESERVED_LENGTH)
#define I_DATA_CHUNK_PAYLOAD_PROTOCOL_ID_OFFSET (I_DATA_CHUNK_MID_OFFSET + \
                                                 I_DATA_CHUNK_MID_LENGTH)
#define I_DATA_CHUNK_FSN_OFFSET       (I_DATA_CHUNK_MID_OFFSET + \
                                       I_DATA_CHUNK_MID_LENGTH)
#define I_DATA_CHUNK_PAYLOAD_OFFSET   (I_DATA_CHUNK_PAYLOAD_PROTOCOL_ID_OFFSET + \
                                       I_DATA_CHUNK_PAYLOAD_PROTOCOL_ID_LENGTH)

#define DATA_CHUNK_HEADER_LENGTH      (CHUNK_HEADER_LENGTH + \
                                       DATA_CHUNK_TSN_LENGTH + \
                                       DATA_CHUNK_STREAM_ID_LENGTH + \
                                       DATA_CHUNK_STREAM_SEQ_NUMBER_LENGTH + \
                                       DATA_CHUNK_PAYLOAD_PROTOCOL_ID_LENGTH)
#define I_DATA_CHUNK_HEADER_LENGTH    (CHUNK_HEADER_LENGTH + \
                                       DATA_CHUNK_TSN_LENGTH + \
                                       DATA_CHUNK_STREAM_ID_LENGTH + \
                                       I_DATA_CHUNK_RESERVED_LENGTH + \
                                       I_DATA_CHUNK_MID_LENGTH +\
                                       I_DATA_CHUNK_PAYLOAD_PROTOCOL_ID_LENGTH)

#define SCTP_DATA_CHUNK_E_BIT 0x01
#define SCTP_DATA_CHUNK_B_BIT 0x02
#define SCTP_DATA_CHUNK_U_BIT 0x04
#define SCTP_DATA_CHUNK_I_BIT 0x08

/* table to hold fragmented SCTP messages */
static GHashTable *frag_table = NULL;


typedef struct _frag_key {
  guint16 sport;
  guint16 dport;
  guint32 verification_tag;
  guint16 stream_id;
  guint32 stream_seq_num;
  guint8 u_bit;
} frag_key;


static gint
frag_equal(gconstpointer k1, gconstpointer k2)
{
  const frag_key *key1 = (const frag_key *) k1;
  const frag_key *key2 = (const frag_key *) k2;

  return ( (key1->sport == key2->sport) &&
           (key1->dport == key2->dport) &&
           (key1->verification_tag == key2->verification_tag) &&
           (key1->stream_id == key2->stream_id) &&
           (key1->stream_seq_num == key2->stream_seq_num) &&
           (key1->u_bit == key2->u_bit)
           ? TRUE : FALSE);
}


static guint
frag_hash(gconstpointer k)
{
  const frag_key *key = (const frag_key *) k;

  return key->sport ^ key->dport ^ key->verification_tag ^
         key->stream_id ^ key->stream_seq_num ^ key->u_bit;
}



static void
frag_free_msgs(sctp_frag_msg *msg)
{
  sctp_frag_be *beginend;
  sctp_fragment *fragment;

  /* free all begins */
  while (msg->begins) {
    beginend = msg->begins;
    msg->begins = msg->begins->next;
    g_free(beginend);
  }

  /* free all ends */
  while (msg->ends) {
    beginend = msg->ends;
    msg->ends = msg->ends->next;
    g_free(beginend);
  }

  /* free all fragments */
  while (msg->fragments) {
    fragment = msg->fragments;
    msg->fragments = msg->fragments->next;
    g_free(fragment->data);
    g_free(fragment);
  }

  /* msg->messages is wmem_ allocated, no need to free it */

  g_free(msg);
}

static void
sctp_init(void)
{
  frag_table = g_hash_table_new_full(frag_hash, frag_equal,
      (GDestroyNotify)g_free, (GDestroyNotify)frag_free_msgs);
  num_assocs = 0;
  assoc_info_list = NULL;
}

static void
sctp_cleanup(void)
{
  g_hash_table_destroy(frag_table);
}


static sctp_frag_msg*
find_message(guint16 stream_id, guint32 stream_seq_num, guint8 u_bit)
{
  frag_key key;

  key.sport = sctp_info.sport;
  key.dport = sctp_info.dport;
  key.verification_tag = sctp_info.verification_tag;
  key.stream_id = stream_id;
  key.stream_seq_num = stream_seq_num;
  key.u_bit = u_bit;

  return (sctp_frag_msg *)g_hash_table_lookup(frag_table, &key);
}


static sctp_fragment*
find_fragment(guint32 tsn, guint16 stream_id, guint32 stream_seq_num, guint8 u_bit)
{
  sctp_frag_msg *msg;
  sctp_fragment *next_fragment;

  msg = find_message(stream_id, stream_seq_num, u_bit);

  if (msg) {
    next_fragment = msg->fragments;
    while (next_fragment) {
      if (next_fragment->tsn == tsn)
        return next_fragment;
      next_fragment = next_fragment->next;
    }
  }

  return NULL;
}


static sctp_fragment *
add_fragment(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, guint32 tsn,
             guint16 stream_id, guint32 stream_seq_num, guint8 b_bit, guint8 e_bit,
             guint8 u_bit, guint32 ppi, gboolean is_idata)
{
  sctp_frag_msg *msg;
  sctp_fragment *fragment, *last_fragment;
  sctp_frag_be *beginend, *last_beginend;
  frag_key *key;

  /* Don't try to do reassembly on error packets */
  if (pinfo->flags.in_error_pkt)
    return NULL;

  /* lookup message. if not found, create it */
  msg = find_message(stream_id, stream_seq_num, u_bit);

  if (!msg) {
    msg = (sctp_frag_msg *)g_malloc (sizeof (sctp_frag_msg));
    msg->begins = NULL;
    msg->ends = NULL;
    msg->fragments = NULL;
    msg->messages = NULL;
    msg->next = NULL;
    if (is_idata)
      if (b_bit)
        msg->ppi = ppi;
      else
        msg->ppi = 0;
    else
      msg->ppi = ppi;

    key = (frag_key *)g_malloc(sizeof (frag_key));
    key->sport = sctp_info.sport;
    key->dport = sctp_info.dport;
    key->verification_tag = sctp_info.verification_tag;
    key->stream_id = stream_id;
    key->stream_seq_num = stream_seq_num;
    key->u_bit = u_bit;

    g_hash_table_insert(frag_table, key, msg);
  } else {
      if (b_bit)
        msg->ppi = ppi;
  }

  /* lookup segment. if not found, create it */
  fragment = find_fragment(tsn, stream_id, stream_seq_num, u_bit);

  if (fragment) {
    /* this fragment is already known.
     * compare frame number to check if it's a duplicate
     */
    if (fragment->frame_num == pinfo->num) {
      return fragment;
    } else {
      /* There already is a fragment having the same ports, v_tag,
       * stream id, stream_seq_num and tsn but it appeared in a different
       * frame, so it must be a duplicate fragment. Maybe a retransmission?
       * Mark it as duplicate and return NULL.
       *
       * Note: This won't happen if TSN analysis is on: the caller will have
       * detected the retransmission and not pass it to the reassembly code.
       */
      col_append_str(pinfo->cinfo, COL_INFO, "(Duplicate Message Fragment) ");

      proto_tree_add_uint(tree, hf_sctp_duplicate, tvb, 0, 0, fragment->frame_num);
      return NULL;
    }
  }

  /* There is no point in storing a fragment with no data in it */
  if (tvb_captured_length(tvb) == 0)
    return NULL;

  /* create new fragment */
  fragment = (sctp_fragment *)g_malloc (sizeof (sctp_fragment));
  fragment->frame_num = pinfo->num;
  fragment->tsn = tsn;
  fragment->len = tvb_captured_length(tvb);
  fragment->ppi = msg->ppi;
  fragment->next = NULL;
  fragment->data = (unsigned char *)g_malloc (fragment->len);
  tvb_memcpy(tvb, fragment->data, 0, fragment->len);

  /* add new fragment to linked list. sort ascending by tsn */
  if (!msg->fragments)
    msg->fragments = fragment;
  else {
    if (msg->fragments->tsn > fragment->tsn) {
      fragment->next = msg->fragments;
      msg->fragments = fragment;
    } else {
        last_fragment = msg->fragments;
        while (last_fragment->next &&
               last_fragment->next->tsn < fragment->tsn)
          last_fragment = last_fragment->next;

        fragment->next = last_fragment->next;
        last_fragment->next = fragment;
    }
  }


  /* save begin or end if necessary */
  if (b_bit && !e_bit) {
    beginend = (sctp_frag_be *)g_malloc (sizeof (sctp_frag_be));
    beginend->fragment = fragment;
    beginend->next = NULL;

    /* add begin to linked list. sort descending by tsn */
    if (!msg->begins)
      msg->begins = beginend;
    else {
      if (msg->begins->fragment->tsn < beginend->fragment->tsn) {
        beginend->next = msg->begins;
        msg->begins = beginend;
      } else {
        last_beginend = msg->begins;
        while (last_beginend->next &&
               last_beginend->next->fragment->tsn > beginend->fragment->tsn)
          last_beginend = last_beginend->next;

        beginend->next = last_beginend->next;
       last_beginend->next = beginend;
      }
    }

  }

  if (!b_bit && e_bit) {
    beginend = (sctp_frag_be *)g_malloc (sizeof (sctp_frag_be));
    beginend->fragment = fragment;
    beginend->next = NULL;

    /* add end to linked list. sort ascending by tsn */
    if (!msg->ends)
      msg->ends = beginend;
    else {
      if (msg->ends->fragment->tsn > beginend->fragment->tsn) {
        beginend->next = msg->ends;
        msg->ends = beginend;
      } else {
        last_beginend = msg->ends;
        while (last_beginend->next &&
               last_beginend->next->fragment->tsn < beginend->fragment->tsn)
          last_beginend = last_beginend->next;

        beginend->next = last_beginend->next;
        last_beginend->next = beginend;
      }
    }

  }

  return fragment;
}

static tvbuff_t*
fragment_reassembly(tvbuff_t *tvb, sctp_fragment *fragment,
                    packet_info *pinfo, proto_tree *tree, guint16 stream_id,
                    guint32 stream_seq_num, guint8 u_bit)
{
  sctp_frag_msg *msg;
  sctp_complete_msg *message, *last_message;
  sctp_fragment *frag_i, *last_frag, *first_frag;
  sctp_frag_be *begin, *end, *beginend;
  guint32 len, offset = 0;
  tvbuff_t *new_tvb = NULL;
  proto_item *item;
  proto_tree *ptree;

  msg = find_message(stream_id, stream_seq_num, u_bit);

  if (!msg) {
    /* no message, we can't do anything */
    return NULL;
  }

  /* check if fragment is part of an already reassembled message */
  for (message = msg->messages;
       message &&
       !(message->begin <= fragment->tsn && message->end >= fragment->tsn) &&
       !(message->begin > message->end &&
       (message->begin <= fragment->tsn || message->end >= fragment->tsn));
         message = message->next);

  if (message) {
    /* we found the reassembled message this fragment belongs to */
    if (fragment == message->reassembled_in) {

      /* this is the last fragment, create data source */
      new_tvb = tvb_new_child_real_data(tvb, message->data, message->len, message->len);
      add_new_data_source(pinfo, new_tvb, "Reassembled SCTP Message");

      /* display reassembly info */
      item = proto_tree_add_item(tree, hf_sctp_fragments, tvb, 0, -1, ENC_NA);
      ptree = proto_item_add_subtree(item, ett_sctp_fragments);
      proto_item_append_text(item, " (%u bytes, %u fragments): ",
                             message->len, message->end - message->begin + 1);

      if (message->begin > message->end) {
        for (frag_i = find_fragment(message->begin, stream_id, stream_seq_num, u_bit);
             frag_i;
             frag_i = frag_i->next) {

          proto_tree_add_uint_format(ptree, hf_sctp_fragment, new_tvb, offset, frag_i->len,
                                     frag_i->frame_num, "Frame: %u, payload: %u-%u (%u bytes)",
                                     frag_i->frame_num, offset, offset + frag_i->len - 1, frag_i->len);
          offset += frag_i->len;

          mark_frame_as_depended_upon(pinfo, frag_i->frame_num);
        }

        for (frag_i = msg->fragments;
             frag_i && frag_i->tsn <= message->end;
             frag_i = frag_i->next) {

          proto_tree_add_uint_format(ptree, hf_sctp_fragment, new_tvb, offset, frag_i->len,
                                     frag_i->frame_num, "Frame: %u, payload: %u-%u (%u bytes)",
                                     frag_i->frame_num, offset, offset + frag_i->len - 1, frag_i->len);
          offset += frag_i->len;

          mark_frame_as_depended_upon(pinfo, frag_i->frame_num);
        }
      } else {
        for (frag_i = find_fragment(message->begin, stream_id, stream_seq_num, u_bit);
             frag_i && frag_i->tsn <= message->end;
             frag_i = frag_i->next) {

          proto_tree_add_uint_format(ptree, hf_sctp_fragment, new_tvb, offset, frag_i->len,
                                     frag_i->frame_num, "Frame: %u, payload: %u-%u (%u bytes)",
                                     frag_i->frame_num, offset, offset + frag_i->len - 1, frag_i->len);
          offset += frag_i->len;

          mark_frame_as_depended_upon(pinfo, frag_i->frame_num);
        }
      }

      return new_tvb;
    }

    /* this is not the last fragment,
     * so let the user know the frame where the reassembly is
     */
    col_append_str(pinfo->cinfo, COL_INFO, "(Message Fragment) ");

    proto_tree_add_uint(tree, hf_sctp_reassembled_in, tvb, 0, 0, message->reassembled_in->frame_num);
    return NULL;
  }

  /* this fragment has not been reassembled, yet
   * check now if we can reassemble it
   * at first look for the first and last tsn of the msg
   */
  for (begin = msg->begins;
       begin && begin->fragment->tsn > fragment->tsn;
       begin = begin->next);

  /* in case begin still is null, set it to first (highest) begin
   * maybe the message tsn restart at 0 in between
   */
  if (!begin)
    begin = msg->begins;

  for (end = msg->ends;
       end && end->fragment->tsn < fragment->tsn;
       end = end->next);

  /* in case end still is null, set it to first (lowest) end
   * maybe the message tsn restart at 0 in between
   */
  if (!end)
    end = msg->ends;

  if (!begin || !end || !msg->fragments ||
      (begin->fragment->tsn > end->fragment->tsn && msg->fragments->tsn)) {
    /* begin and end have not been collected, yet
     * or there might be a tsn restart but the first fragment hasn't a tsn of 0
     * just mark as fragment
     */

    col_append_str(pinfo->cinfo, COL_INFO, "(Message Fragment) ");

    return NULL;
  }

  /* we found possible begin and end
   * look for the first fragment and then try to get to the end
   */
  first_frag = begin->fragment;

  /* while looking if all fragments are there
   * we can calculate the overall length that
   * we need in case of success
   */
  len = first_frag->len;

  /* check if begin is past end
   * this can happen if there has been a tsn restart
   * or we just got the wrong begin and end
   * so give it a try
  */
  if (begin->fragment->tsn > end->fragment->tsn) {
    for (last_frag = first_frag, frag_i = first_frag->next;
         frag_i && frag_i->tsn == (last_frag->tsn + 1);
         last_frag = frag_i, frag_i = frag_i->next) len += frag_i->len;

    /* check if we reached the last possible tsn
     * if yes, restart and continue
     */
    if ((last_frag->tsn + 1)) {
      /* there are just fragments missing */
      col_append_str(pinfo->cinfo, COL_INFO, "(Message Fragment) ");

      return NULL;
    }

    /* we got all fragments until the last possible tsn
     * and the first is 0 if we got here
     */

    len += msg->fragments->len;
    for (last_frag = msg->fragments, frag_i = last_frag->next;
         frag_i && frag_i->tsn < end->fragment->tsn && frag_i->tsn == (last_frag->tsn + 1);
         last_frag = frag_i, frag_i = frag_i->next) len += frag_i->len;

  } else {
    for (last_frag = first_frag, frag_i = first_frag->next;
         frag_i && frag_i->tsn < end->fragment->tsn && frag_i->tsn == (last_frag->tsn + 1);
         last_frag = frag_i, frag_i = frag_i->next) len += frag_i->len;
  }

  if (!frag_i || frag_i != end->fragment || frag_i->tsn != (last_frag->tsn + 1)) {
    /* we need more fragments. just mark as fragment */
    col_append_str(pinfo->cinfo, COL_INFO, "(Message Fragment) ");

    return NULL;
  }

  /* ok, this message is complete, we can reassemble it
   * but at first don't forget to add the length of the last fragment
   */
  len += frag_i->len;

  message = wmem_new(wmem_file_scope(), sctp_complete_msg);
  message->begin = begin->fragment->tsn;
  message->end = end->fragment->tsn;
  message->reassembled_in = fragment;
  message->len = len;
  message->data = (unsigned char *)wmem_alloc(wmem_file_scope(), len);
  message->next = NULL;

  /* now copy all fragments */
  if (begin->fragment->tsn > end->fragment->tsn) {
    /* a tsn restart has occurred */
    for (frag_i = first_frag;
         frag_i;
         frag_i = frag_i->next) {

      if (frag_i->len && frag_i->data)
        memcpy(message->data + offset, frag_i->data, frag_i->len);
      offset += frag_i->len;

      /* release fragment data */
      g_free(frag_i->data);
      frag_i->data = NULL;
    }

    for (frag_i = msg->fragments;
         frag_i && frag_i->tsn <= end->fragment->tsn;
         frag_i = frag_i->next) {

      if (frag_i->len && frag_i->data)
        memcpy(message->data + offset, frag_i->data, frag_i->len);
      offset += frag_i->len;

      /* release fragment data */
      g_free(frag_i->data);
      frag_i->data = NULL;
    }

  } else {
    for (frag_i = first_frag;
         frag_i && frag_i->tsn <= end->fragment->tsn;
         frag_i = frag_i->next) {

      if (frag_i->len && frag_i->data)
        memcpy(message->data + offset, frag_i->data, frag_i->len);
      offset += frag_i->len;

      /* release fragment data */
      g_free(frag_i->data);
      frag_i->data = NULL;
    }
  }

  /* save message */
  if (!msg->messages) {
    msg->messages = message;
  } else {
    for (last_message = msg->messages;
         last_message->next;
         last_message = last_message->next);
    last_message->next = message;
  }

  /* remove begin and end from list */
  if (msg->begins == begin) {
    msg->begins = begin->next;
  } else {
    for (beginend = msg->begins;
         beginend && beginend->next != begin;
         beginend = beginend->next);
    if (beginend && beginend->next == begin)
      beginend->next = begin->next;
  }
  g_free(begin);

  if (msg->ends == end) {
    msg->ends = end->next;
  } else {
    for (beginend = msg->ends;
         beginend && beginend->next != end;
         beginend = beginend->next);
    if (beginend && beginend->next == end)
      beginend->next = end->next;
  }
   g_free(end);

  /* create data source */
  new_tvb = tvb_new_child_real_data(tvb, message->data, len, len);
  add_new_data_source(pinfo, new_tvb, "Reassembled SCTP Message");

  /* display reassembly info */
  item = proto_tree_add_item(tree, hf_sctp_fragments, tvb, 0, -1, ENC_NA);
  ptree = proto_item_add_subtree(item, ett_sctp_fragments);
  proto_item_append_text(item, " (%u bytes, %u fragments): ",
                         message->len, message->end - message->begin + 1);

  if (message->begin > message->end) {
    for (frag_i = find_fragment(message->begin, stream_id, stream_seq_num, u_bit);
         frag_i;
         frag_i = frag_i->next) {

      proto_tree_add_uint_format(ptree, hf_sctp_fragment, new_tvb, offset, frag_i->len,
                                 frag_i->frame_num, "Frame: %u, payload: %u-%u (%u bytes)",
                                 frag_i->frame_num, offset, offset + frag_i->len - 1, frag_i->len);
      offset += frag_i->len;
    }

    for (frag_i = msg->fragments;
         frag_i && frag_i->tsn <= message->end;
         frag_i = frag_i->next) {

      proto_tree_add_uint_format(ptree, hf_sctp_fragment, new_tvb, offset, frag_i->len,
                                 frag_i->frame_num, "Frame: %u, payload: %u-%u (%u bytes)",
                                 frag_i->frame_num, offset, offset + frag_i->len - 1, frag_i->len);
      offset += frag_i->len;
    }
  } else {
    for (frag_i = find_fragment(message->begin, stream_id, stream_seq_num, u_bit);
         frag_i && frag_i->tsn <= message->end;
         frag_i = frag_i->next) {

      proto_tree_add_uint_format(ptree, hf_sctp_fragment, new_tvb, offset, frag_i->len,
                                 frag_i->frame_num, "Frame: %u, payload: %u-%u (%u bytes)",
                                 frag_i->frame_num, offset, offset + frag_i->len - 1, frag_i->len);
      offset += frag_i->len;
    }
  }

  /* it's not fragmented anymore */
  pinfo->fragmented = FALSE;

  return new_tvb;
}

static void
export_sctp_data_chunk(packet_info *pinfo, tvbuff_t *tvb, const gchar *proto_name)
{
  exp_pdu_data_t *exp_pdu_data = export_pdu_create_common_tags(pinfo, proto_name, EXP_PDU_TAG_PROTO_NAME);

  exp_pdu_data->tvb_captured_length = tvb_captured_length(tvb);
  exp_pdu_data->tvb_reported_length = tvb_reported_length(tvb);
  exp_pdu_data->pdu_tvb = tvb;

  tap_queue_packet(exported_pdu_tap, pinfo, exp_pdu_data);

}

static gboolean
dissect_fragmented_payload(tvbuff_t *payload_tvb, packet_info *pinfo, proto_tree *tree,
                           proto_tree *chunk_tree, guint32 tsn, guint32 ppi, guint16 stream_id,
                           guint32 stream_seq_num, guint8 b_bit, guint8 e_bit, guint8 u_bit, gboolean is_idata)
{
  sctp_fragment *fragment;
  tvbuff_t *new_tvb = NULL;

  /*
   * If this is a short frame, then we can't, and don't, do
   * reassembly on it.  We just give up.
   */
  if (tvb_reported_length(payload_tvb) > tvb_captured_length(payload_tvb))
    return TRUE;

  /* add fragment to list of known fragments. returns NULL if segment is a duplicate */
  fragment = add_fragment(payload_tvb, pinfo, chunk_tree, tsn, stream_id, stream_seq_num, b_bit, e_bit, u_bit, ppi, is_idata);

  if (fragment)
    new_tvb = fragment_reassembly(payload_tvb, fragment, pinfo, chunk_tree, stream_id, stream_seq_num, u_bit);

  /* pass reassembled data to next dissector, if possible */
  if (new_tvb){
    wmem_list_frame_t *cur;
    guint proto_id;
    const gchar *proto_name;
    gboolean retval;
    void *tmp;

    cur = wmem_list_tail(pinfo->layers);
    retval = dissect_payload(new_tvb, pinfo, tree, ppi);
    cur = wmem_list_frame_next(cur);
    if (cur) {
      tmp = wmem_list_frame_data(cur);
      proto_id = GPOINTER_TO_UINT(tmp);
      proto_name = proto_get_protocol_filter_name(proto_id);
      if(strcmp(proto_name, "data") != 0){
        if (have_tap_listener(exported_pdu_tap)){
          export_sctp_data_chunk(pinfo,payload_tvb, proto_name);
        }
      }
    }
    return retval;
  }

  /* no reassembly done, do nothing */
  return TRUE;
}

static const true_false_string sctp_data_chunk_e_bit_value = {
  "Last segment",
  "Not the last segment"
};

static const true_false_string sctp_data_chunk_b_bit_value = {
  "First segment",
  "Subsequent segment"
};

static const true_false_string sctp_data_chunk_u_bit_value = {
  "Unordered delivery",
  "Ordered delivery"
};

static const true_false_string sctp_data_chunk_i_bit_value = {
  "Send SACK immediately",
  "Possibly delay SACK"
};

static gboolean
dissect_data_chunk(tvbuff_t *chunk_tvb,
                   guint16 chunk_length,
                   packet_info *pinfo,
                   proto_tree *tree,
                   proto_tree *chunk_tree,
                   proto_item *chunk_item,
                   proto_item *flags_item,
                   sctp_half_assoc_t *ha,
                   gboolean is_idata)
{
  guint number_of_ppid;
  volatile guint32 payload_proto_id;
  tvbuff_t *payload_tvb;
  proto_tree *flags_tree;
  guint8 e_bit, b_bit, u_bit;
  guint16 stream_id;
  guint32 tsn, ppid, stream_seq_num = 0;
  proto_item *tsn_item = NULL;
  gboolean call_subdissector = FALSE;
  gboolean is_retransmission;
  guint16 header_length;
  guint16 payload_offset;

  if (is_idata) {
    if (chunk_length < I_DATA_CHUNK_HEADER_LENGTH) {
      proto_item_append_text(chunk_item, ", bogus chunk length %u < %u)", chunk_length, I_DATA_CHUNK_HEADER_LENGTH);
      return TRUE;
    }
    payload_proto_id  = tvb_get_ntohl(chunk_tvb, I_DATA_CHUNK_PAYLOAD_PROTOCOL_ID_OFFSET);
  } else {
    if (chunk_length < DATA_CHUNK_HEADER_LENGTH) {
      proto_item_append_text(chunk_item, ", bogus chunk length %u < %u)", chunk_length, DATA_CHUNK_HEADER_LENGTH);
      return TRUE;
    }
    payload_proto_id  = tvb_get_ntohl(chunk_tvb, DATA_CHUNK_PAYLOAD_PROTOCOL_ID_OFFSET);
  }

  /* insert the PPID in the pinfo structure if it is not already there and there is still room */
  for(number_of_ppid = 0; number_of_ppid < MAX_NUMBER_OF_PPIDS; number_of_ppid++) {
    void *tmp = p_get_proto_data(pinfo->pool, pinfo, proto_sctp, number_of_ppid);
    ppid = GPOINTER_TO_UINT(tmp);
    if ((ppid == LAST_PPID) || (ppid == payload_proto_id))
      break;
  }
  if ((number_of_ppid < MAX_NUMBER_OF_PPIDS) && (ppid == LAST_PPID))
    p_add_proto_data(pinfo->pool, pinfo, proto_sctp, number_of_ppid, GUINT_TO_POINTER(payload_proto_id));

  e_bit = tvb_get_guint8(chunk_tvb, CHUNK_FLAGS_OFFSET) & SCTP_DATA_CHUNK_E_BIT;
  b_bit = tvb_get_guint8(chunk_tvb, CHUNK_FLAGS_OFFSET) & SCTP_DATA_CHUNK_B_BIT;
  u_bit = tvb_get_guint8(chunk_tvb, CHUNK_FLAGS_OFFSET) & SCTP_DATA_CHUNK_U_BIT;
  tsn = tvb_get_ntohl(chunk_tvb, DATA_CHUNK_TSN_OFFSET);

  if (chunk_tree) {
    if (is_idata)
      proto_item_set_len(chunk_item, I_DATA_CHUNK_HEADER_LENGTH);
    else
      proto_item_set_len(chunk_item, DATA_CHUNK_HEADER_LENGTH);
    flags_tree  = proto_item_add_subtree(flags_item, ett_sctp_data_chunk_flags);
    proto_tree_add_item(flags_tree, hf_data_chunk_e_bit,             chunk_tvb, CHUNK_FLAGS_OFFSET,                    CHUNK_FLAGS_LENGTH,                    ENC_BIG_ENDIAN);
    proto_tree_add_item(flags_tree, hf_data_chunk_b_bit,             chunk_tvb, CHUNK_FLAGS_OFFSET,                    CHUNK_FLAGS_LENGTH,                    ENC_BIG_ENDIAN);
    proto_tree_add_item(flags_tree, hf_data_chunk_u_bit,             chunk_tvb, CHUNK_FLAGS_OFFSET,                    CHUNK_FLAGS_LENGTH,                    ENC_BIG_ENDIAN);
    proto_tree_add_item(flags_tree, hf_data_chunk_i_bit,             chunk_tvb, CHUNK_FLAGS_OFFSET,                    CHUNK_FLAGS_LENGTH,                    ENC_BIG_ENDIAN);
    tsn_item = proto_tree_add_item(chunk_tree, hf_data_chunk_tsn,    chunk_tvb, DATA_CHUNK_TSN_OFFSET,                 DATA_CHUNK_TSN_LENGTH,                 ENC_BIG_ENDIAN);
    proto_tree_add_item(chunk_tree, hf_data_chunk_stream_id,         chunk_tvb, DATA_CHUNK_STREAM_ID_OFFSET,           DATA_CHUNK_STREAM_ID_LENGTH,           ENC_BIG_ENDIAN);
    if (is_idata) {
      proto_tree_add_item(chunk_tree, hf_idata_chunk_reserved, chunk_tvb, I_DATA_CHUNK_RESERVED_OFFSET, I_DATA_CHUNK_RESERVED_LENGTH, ENC_BIG_ENDIAN);
      proto_tree_add_item(chunk_tree, hf_idata_chunk_mid, chunk_tvb, I_DATA_CHUNK_MID_OFFSET, I_DATA_CHUNK_MID_LENGTH, ENC_BIG_ENDIAN);
      if (b_bit)
        proto_tree_add_item(chunk_tree, hf_data_chunk_payload_proto_id,  chunk_tvb, I_DATA_CHUNK_PAYLOAD_PROTOCOL_ID_OFFSET, I_DATA_CHUNK_PAYLOAD_PROTOCOL_ID_LENGTH, ENC_BIG_ENDIAN);
      else
        proto_tree_add_item(chunk_tree, hf_idata_chunk_fsn, chunk_tvb, I_DATA_CHUNK_FSN_OFFSET, I_DATA_CHUNK_FSN_LENGTH, ENC_BIG_ENDIAN);
    } else {
      proto_tree_add_item(chunk_tree, hf_data_chunk_stream_seq_number, chunk_tvb, DATA_CHUNK_STREAM_SEQ_NUMBER_OFFSET,   DATA_CHUNK_STREAM_SEQ_NUMBER_LENGTH,   ENC_BIG_ENDIAN);
      proto_tree_add_item(chunk_tree, hf_data_chunk_payload_proto_id,  chunk_tvb, DATA_CHUNK_PAYLOAD_PROTOCOL_ID_OFFSET, DATA_CHUNK_PAYLOAD_PROTOCOL_ID_LENGTH, ENC_BIG_ENDIAN);
    }
    proto_item_append_text(chunk_item, "(%s, ", (u_bit) ? "unordered" : "ordered");
    if (b_bit) {
      if (e_bit)
        proto_item_append_text(chunk_item, "complete");
      else
        proto_item_append_text(chunk_item, "first");
    } else {
      if (e_bit)
        proto_item_append_text(chunk_item, "last");
      else
        proto_item_append_text(chunk_item, "middle");
    }

    if (is_idata) {
      if (b_bit)
        proto_item_append_text(chunk_item, " segment, TSN: %u, SID: %u, MID: %u, payload length: %u byte%s)",
                               tvb_get_ntohl(chunk_tvb, DATA_CHUNK_TSN_OFFSET),
                               tvb_get_ntohs(chunk_tvb, DATA_CHUNK_STREAM_ID_OFFSET),
                               tvb_get_ntohl(chunk_tvb, I_DATA_CHUNK_MID_OFFSET),
                               chunk_length - I_DATA_CHUNK_HEADER_LENGTH, plurality(chunk_length - I_DATA_CHUNK_HEADER_LENGTH, "", "s"));
      else
        proto_item_append_text(chunk_item, " segment, TSN: %u, SID: %u, MID: %u, FSN: %u, payload length: %u byte%s)",
                               tvb_get_ntohl(chunk_tvb, DATA_CHUNK_TSN_OFFSET),
                               tvb_get_ntohs(chunk_tvb, DATA_CHUNK_STREAM_ID_OFFSET),
                               tvb_get_ntohl(chunk_tvb, I_DATA_CHUNK_MID_OFFSET),
                               tvb_get_ntohl(chunk_tvb, I_DATA_CHUNK_FSN_OFFSET),
                               chunk_length - I_DATA_CHUNK_HEADER_LENGTH, plurality(chunk_length - I_DATA_CHUNK_HEADER_LENGTH, "", "s"));
    } else
      proto_item_append_text(chunk_item, " segment, TSN: %u, SID: %u, SSN: %u, PPID: %u, payload length: %u byte%s)",
                             tvb_get_ntohl(chunk_tvb, DATA_CHUNK_TSN_OFFSET),
                             tvb_get_ntohs(chunk_tvb, DATA_CHUNK_STREAM_ID_OFFSET),
                             tvb_get_ntohs(chunk_tvb, DATA_CHUNK_STREAM_SEQ_NUMBER_OFFSET),
                             payload_proto_id,
                             chunk_length - DATA_CHUNK_HEADER_LENGTH, plurality(chunk_length - DATA_CHUNK_HEADER_LENGTH, "", "s"));
  }

  is_retransmission = sctp_tsn(pinfo, chunk_tvb, tsn_item, ha, tsn);

  if (is_idata) {
    header_length = I_DATA_CHUNK_HEADER_LENGTH;
    payload_offset = I_DATA_CHUNK_PAYLOAD_OFFSET;
  } else {
    header_length = DATA_CHUNK_HEADER_LENGTH;
    payload_offset = DATA_CHUNK_PAYLOAD_OFFSET;
  }
  payload_tvb = tvb_new_subset(chunk_tvb, payload_offset,
                                 MIN(chunk_length - header_length, tvb_captured_length_remaining(chunk_tvb, payload_offset)),
                                 MIN(chunk_length - header_length, tvb_reported_length_remaining(chunk_tvb, payload_offset)));

  /* Is this a fragment? */
  if (b_bit && e_bit) {
    /* No - just call the subdissector. */
    if (!is_retransmission)
      call_subdissector = TRUE;
  } else {
    /* Yes. */
    pinfo->fragmented = TRUE;

    /* if reassembly is off just mark as fragment for next dissector and proceed */
    if (!use_reassembly)
    {
      /*  Don't pass on non-first fragments since the next dissector will
       *  almost certainly not understand the data.
       */
      if (b_bit) {
        if (!is_retransmission)
          call_subdissector = TRUE;
      } else
        return FALSE;
    }

  }

  if (call_subdissector) {
    /* This isn't a fragment or reassembly is off and it's the first fragment */

    volatile gboolean retval = FALSE;

    TRY {
      wmem_list_frame_t *cur;
      guint proto_id;
      const gchar *proto_name;
      void *tmp;

      cur = wmem_list_tail(pinfo->layers);
      retval = dissect_payload(payload_tvb, pinfo, tree, payload_proto_id);
      cur = wmem_list_frame_next(cur);
      if (cur) {
        tmp = wmem_list_frame_data(cur);
        proto_id = GPOINTER_TO_UINT(tmp);
        proto_name = proto_get_protocol_filter_name(proto_id);
        if (strcmp(proto_name, "data") != 0){
          if (have_tap_listener(exported_pdu_tap)){
            export_sctp_data_chunk(pinfo,payload_tvb, proto_name);
          }
        }
      }
    }
    CATCH_NONFATAL_ERRORS {
      /*
       * Somebody threw an exception that means that there was a problem
       * dissecting the payload; that means that a dissector was found,
       * so we don't need to dissect the payload as data or update the
       * protocol or info columns.
       *
       * Just show the exception and then continue dissecting chunks.
       */
      show_exception(payload_tvb, pinfo, tree, EXCEPT_CODE, GET_MESSAGE);
    }
    ENDTRY;

    return retval;

  } else if (is_retransmission) {
    col_append_str(pinfo->cinfo, COL_INFO, "(retransmission) ");
    return FALSE;
  } else {

    /* The logic above should ensure this... */
    DISSECTOR_ASSERT(use_reassembly);

    stream_id = tvb_get_ntohs(chunk_tvb, DATA_CHUNK_STREAM_ID_OFFSET);
    if (is_idata) {
      /* The stream_seq_num variable is used to hold the MID, the tsn variable holds the FSN*/
      stream_seq_num = tvb_get_ntohl(chunk_tvb, I_DATA_CHUNK_MID_OFFSET);
      if (b_bit) {
        tsn = 0;
      } else {
        tsn = tvb_get_ntohl(chunk_tvb, I_DATA_CHUNK_FSN_OFFSET);
        payload_proto_id = 0;
      }
    } else {
      /* if unordered set stream_seq_num to 0 for easier handling */
      if (u_bit)
        stream_seq_num = 0;
      else
        stream_seq_num = tvb_get_ntohs(chunk_tvb, DATA_CHUNK_STREAM_SEQ_NUMBER_OFFSET);
    }
    /* start reassembly */
    return dissect_fragmented_payload(payload_tvb, pinfo, tree, chunk_tree, tsn, payload_proto_id, stream_id, stream_seq_num, b_bit, e_bit, u_bit, is_idata);
  }

}

#define INIT_CHUNK_INITIATE_TAG_LENGTH               4
#define INIT_CHUNK_ADV_REC_WINDOW_CREDIT_LENGTH      4
#define INIT_CHUNK_NUMBER_OF_OUTBOUND_STREAMS_LENGTH 2
#define INIT_CHUNK_NUMBER_OF_INBOUND_STREAMS_LENGTH  2
#define INIT_CHUNK_INITIAL_TSN_LENGTH                4
#define INIT_CHUNK_FIXED_PARAMTERS_LENGTH            (CHUNK_HEADER_LENGTH + \
                                                      INIT_CHUNK_INITIATE_TAG_LENGTH + \
                                                      INIT_CHUNK_ADV_REC_WINDOW_CREDIT_LENGTH + \
                                                      INIT_CHUNK_NUMBER_OF_OUTBOUND_STREAMS_LENGTH + \
                                                      INIT_CHUNK_NUMBER_OF_INBOUND_STREAMS_LENGTH + \
                                                      INIT_CHUNK_INITIAL_TSN_LENGTH)

#define INIT_CHUNK_INITIATE_TAG_OFFSET               CHUNK_VALUE_OFFSET
#define INIT_CHUNK_ADV_REC_WINDOW_CREDIT_OFFSET      (INIT_CHUNK_INITIATE_TAG_OFFSET + \
                                                      INIT_CHUNK_INITIATE_TAG_LENGTH )
#define INIT_CHUNK_NUMBER_OF_OUTBOUND_STREAMS_OFFSET (INIT_CHUNK_ADV_REC_WINDOW_CREDIT_OFFSET + \
                                                      INIT_CHUNK_ADV_REC_WINDOW_CREDIT_LENGTH )
#define INIT_CHUNK_NUMBER_OF_INBOUND_STREAMS_OFFSET  (INIT_CHUNK_NUMBER_OF_OUTBOUND_STREAMS_OFFSET + \
                                                      INIT_CHUNK_NUMBER_OF_OUTBOUND_STREAMS_LENGTH )
#define INIT_CHUNK_INITIAL_TSN_OFFSET                (INIT_CHUNK_NUMBER_OF_INBOUND_STREAMS_OFFSET + \
                                                      INIT_CHUNK_NUMBER_OF_INBOUND_STREAMS_LENGTH )
#define INIT_CHUNK_VARIABLE_LENGTH_PARAMETER_OFFSET  (INIT_CHUNK_INITIAL_TSN_OFFSET + \
                                                      INIT_CHUNK_INITIAL_TSN_LENGTH )

static void
dissect_init_chunk(tvbuff_t *chunk_tvb, guint16 chunk_length, packet_info *pinfo, proto_tree *chunk_tree, proto_item *chunk_item)
{
  tvbuff_t *parameters_tvb;
  proto_item *hidden_item;

  if (chunk_length < INIT_CHUNK_FIXED_PARAMTERS_LENGTH) {
    proto_item_append_text(chunk_item, ", bogus chunk length %u < %u)", chunk_length, INIT_CHUNK_FIXED_PARAMTERS_LENGTH);
    return;
  }

  if (chunk_tree) {
    /* handle fixed parameters */
    proto_tree_add_item(chunk_tree, hf_init_chunk_initiate_tag,               chunk_tvb, INIT_CHUNK_INITIATE_TAG_OFFSET,               INIT_CHUNK_INITIATE_TAG_LENGTH,               ENC_BIG_ENDIAN);
    hidden_item = proto_tree_add_item(chunk_tree, hf_initiate_tag,            chunk_tvb, INIT_CHUNK_INITIATE_TAG_OFFSET,               INIT_CHUNK_INITIATE_TAG_LENGTH,               ENC_BIG_ENDIAN);
    PROTO_ITEM_SET_HIDDEN(hidden_item);
    proto_tree_add_item(chunk_tree, hf_init_chunk_adv_rec_window_credit,      chunk_tvb, INIT_CHUNK_ADV_REC_WINDOW_CREDIT_OFFSET,      INIT_CHUNK_ADV_REC_WINDOW_CREDIT_LENGTH,      ENC_BIG_ENDIAN);
    proto_tree_add_item(chunk_tree, hf_init_chunk_number_of_outbound_streams, chunk_tvb, INIT_CHUNK_NUMBER_OF_OUTBOUND_STREAMS_OFFSET, INIT_CHUNK_NUMBER_OF_OUTBOUND_STREAMS_LENGTH, ENC_BIG_ENDIAN);
    proto_tree_add_item(chunk_tree, hf_init_chunk_number_of_inbound_streams,  chunk_tvb, INIT_CHUNK_NUMBER_OF_INBOUND_STREAMS_OFFSET,  INIT_CHUNK_NUMBER_OF_INBOUND_STREAMS_LENGTH,  ENC_BIG_ENDIAN);
    proto_tree_add_item(chunk_tree, hf_init_chunk_initial_tsn,                chunk_tvb, INIT_CHUNK_INITIAL_TSN_OFFSET,                INIT_CHUNK_INITIAL_TSN_LENGTH,                ENC_BIG_ENDIAN);

    proto_item_append_text(chunk_item, " (Outbound streams: %u, inbound streams: %u)",
                           tvb_get_ntohs(chunk_tvb, INIT_CHUNK_NUMBER_OF_OUTBOUND_STREAMS_OFFSET),
                           tvb_get_ntohs(chunk_tvb, INIT_CHUNK_NUMBER_OF_INBOUND_STREAMS_OFFSET));
  }

  /* handle variable parameters */
  chunk_length -= INIT_CHUNK_FIXED_PARAMTERS_LENGTH;
  parameters_tvb = tvb_new_subset(chunk_tvb, INIT_CHUNK_VARIABLE_LENGTH_PARAMETER_OFFSET,
                                  MIN(chunk_length, tvb_captured_length_remaining(chunk_tvb, INIT_CHUNK_VARIABLE_LENGTH_PARAMETER_OFFSET)),
                                  MIN(chunk_length, tvb_reported_length_remaining(chunk_tvb, INIT_CHUNK_VARIABLE_LENGTH_PARAMETER_OFFSET)));
  dissect_parameters(parameters_tvb, pinfo, chunk_tree, NULL, TRUE);
}

static void
dissect_init_ack_chunk(tvbuff_t *chunk_tvb, guint16 chunk_length, packet_info *pinfo, proto_tree *chunk_tree, proto_item *chunk_item)
{
  tvbuff_t *parameters_tvb;
  proto_item *hidden_item;

  if (chunk_length < INIT_CHUNK_FIXED_PARAMTERS_LENGTH) {
    proto_item_append_text(chunk_item, ", bogus chunk length %u < %u)",
                           chunk_length,
                           INIT_CHUNK_FIXED_PARAMTERS_LENGTH);
    return;
  }
  if (chunk_tree) {
    /* handle fixed parameters */
    proto_tree_add_item(chunk_tree, hf_initack_chunk_initiate_tag,               chunk_tvb, INIT_CHUNK_INITIATE_TAG_OFFSET,               INIT_CHUNK_INITIATE_TAG_LENGTH,               ENC_BIG_ENDIAN);
    hidden_item = proto_tree_add_item(chunk_tree, hf_initiate_tag,                      chunk_tvb, INIT_CHUNK_INITIATE_TAG_OFFSET,               INIT_CHUNK_INITIATE_TAG_LENGTH,               ENC_BIG_ENDIAN);
    PROTO_ITEM_SET_HIDDEN(hidden_item);
    proto_tree_add_item(chunk_tree, hf_initack_chunk_adv_rec_window_credit,      chunk_tvb, INIT_CHUNK_ADV_REC_WINDOW_CREDIT_OFFSET,      INIT_CHUNK_ADV_REC_WINDOW_CREDIT_LENGTH,      ENC_BIG_ENDIAN);
    proto_tree_add_item(chunk_tree, hf_initack_chunk_number_of_outbound_streams, chunk_tvb, INIT_CHUNK_NUMBER_OF_OUTBOUND_STREAMS_OFFSET, INIT_CHUNK_NUMBER_OF_OUTBOUND_STREAMS_LENGTH, ENC_BIG_ENDIAN);
    proto_tree_add_item(chunk_tree, hf_initack_chunk_number_of_inbound_streams,  chunk_tvb, INIT_CHUNK_NUMBER_OF_INBOUND_STREAMS_OFFSET,  INIT_CHUNK_NUMBER_OF_INBOUND_STREAMS_LENGTH,  ENC_BIG_ENDIAN);
    proto_tree_add_item(chunk_tree, hf_initack_chunk_initial_tsn,                chunk_tvb, INIT_CHUNK_INITIAL_TSN_OFFSET,                INIT_CHUNK_INITIAL_TSN_LENGTH,                ENC_BIG_ENDIAN);

    proto_item_append_text(chunk_item, " (Outbound streams: %u, inbound streams: %u)",
                           tvb_get_ntohs(chunk_tvb, INIT_CHUNK_NUMBER_OF_OUTBOUND_STREAMS_OFFSET),
                           tvb_get_ntohs(chunk_tvb, INIT_CHUNK_NUMBER_OF_INBOUND_STREAMS_OFFSET));
  }
  /* handle variable paramters */
  chunk_length -= INIT_CHUNK_FIXED_PARAMTERS_LENGTH;
  parameters_tvb = tvb_new_subset(chunk_tvb, INIT_CHUNK_VARIABLE_LENGTH_PARAMETER_OFFSET,
                                  MIN(chunk_length, tvb_captured_length_remaining(chunk_tvb, INIT_CHUNK_VARIABLE_LENGTH_PARAMETER_OFFSET)),
                                  MIN(chunk_length, tvb_reported_length_remaining(chunk_tvb, INIT_CHUNK_VARIABLE_LENGTH_PARAMETER_OFFSET)));
  dissect_parameters(parameters_tvb, pinfo, chunk_tree, NULL, TRUE);
}

#define SCTP_SACK_CHUNK_NS_BIT                  0x01
#define SACK_CHUNK_CUMULATIVE_TSN_ACK_LENGTH    4
#define SACK_CHUNK_ADV_REC_WINDOW_CREDIT_LENGTH 4
#define SACK_CHUNK_NUMBER_OF_GAP_BLOCKS_LENGTH  2
#define SACK_CHUNK_NUMBER_OF_DUP_TSNS_LENGTH    2
#define SACK_CHUNK_GAP_BLOCK_LENGTH             4
#define SACK_CHUNK_GAP_BLOCK_START_LENGTH       2
#define SACK_CHUNK_GAP_BLOCK_END_LENGTH         2
#define SACK_CHUNK_DUP_TSN_LENGTH               4

#define SACK_CHUNK_CUMULATIVE_TSN_ACK_OFFSET (CHUNK_VALUE_OFFSET + 0)
#define SACK_CHUNK_ADV_REC_WINDOW_CREDIT_OFFSET (SACK_CHUNK_CUMULATIVE_TSN_ACK_OFFSET + \
                                                 SACK_CHUNK_CUMULATIVE_TSN_ACK_LENGTH)
#define SACK_CHUNK_NUMBER_OF_GAP_BLOCKS_OFFSET (SACK_CHUNK_ADV_REC_WINDOW_CREDIT_OFFSET + \
                                                SACK_CHUNK_ADV_REC_WINDOW_CREDIT_LENGTH)
#define SACK_CHUNK_NUMBER_OF_DUP_TSNS_OFFSET (SACK_CHUNK_NUMBER_OF_GAP_BLOCKS_OFFSET + \
                                              SACK_CHUNK_NUMBER_OF_GAP_BLOCKS_LENGTH)
#define SACK_CHUNK_GAP_BLOCK_OFFSET (SACK_CHUNK_NUMBER_OF_DUP_TSNS_OFFSET + \
                                     SACK_CHUNK_NUMBER_OF_DUP_TSNS_LENGTH)


static void
dissect_sack_chunk(packet_info *pinfo, tvbuff_t *chunk_tvb, proto_tree *chunk_tree, proto_item *chunk_item, proto_item *flags_item, sctp_half_assoc_t *ha)
{
  guint16 number_of_gap_blocks, number_of_dup_tsns;
  guint16 gap_block_number, dup_tsn_number, start, end;
  gint gap_block_offset, dup_tsn_offset;
  guint32 cum_tsn_ack;
  proto_tree *block_tree;
  proto_tree *flags_tree;
  proto_item *ctsa_item;
  proto_item *a_rwnd_item;
  proto_tree *acks_tree;
  guint32 tsns_gap_acked = 0;
  guint32 a_rwnd;
  guint16 last_end;

  flags_tree  = proto_item_add_subtree(flags_item, ett_sctp_sack_chunk_flags);
  proto_tree_add_item(flags_tree, hf_sack_chunk_ns,                    chunk_tvb, CHUNK_FLAGS_OFFSET,                      CHUNK_FLAGS_LENGTH,                      ENC_BIG_ENDIAN);
  ctsa_item = proto_tree_add_item(chunk_tree, hf_sack_chunk_cumulative_tsn_ack,    chunk_tvb, SACK_CHUNK_CUMULATIVE_TSN_ACK_OFFSET,    SACK_CHUNK_CUMULATIVE_TSN_ACK_LENGTH,    ENC_BIG_ENDIAN);
  a_rwnd_item = proto_tree_add_item(chunk_tree, hf_sack_chunk_adv_rec_window_credit, chunk_tvb, SACK_CHUNK_ADV_REC_WINDOW_CREDIT_OFFSET, SACK_CHUNK_ADV_REC_WINDOW_CREDIT_LENGTH, ENC_BIG_ENDIAN);
  proto_tree_add_item(chunk_tree, hf_sack_chunk_number_of_gap_blocks,  chunk_tvb, SACK_CHUNK_NUMBER_OF_GAP_BLOCKS_OFFSET,  SACK_CHUNK_NUMBER_OF_GAP_BLOCKS_LENGTH,  ENC_BIG_ENDIAN);
  proto_tree_add_item(chunk_tree, hf_sack_chunk_number_of_dup_tsns,    chunk_tvb, SACK_CHUNK_NUMBER_OF_DUP_TSNS_OFFSET,    SACK_CHUNK_NUMBER_OF_DUP_TSNS_LENGTH,    ENC_BIG_ENDIAN);

  a_rwnd = tvb_get_ntohl(chunk_tvb, SACK_CHUNK_ADV_REC_WINDOW_CREDIT_OFFSET);
  if (a_rwnd == 0)
      expert_add_info(pinfo, a_rwnd_item, &ei_sctp_sack_chunk_adv_rec_window_credit);


  /* handle the gap acknowledgement blocks */
  number_of_gap_blocks = tvb_get_ntohs(chunk_tvb, SACK_CHUNK_NUMBER_OF_GAP_BLOCKS_OFFSET);
  gap_block_offset     = SACK_CHUNK_GAP_BLOCK_OFFSET;
  cum_tsn_ack          = tvb_get_ntohl(chunk_tvb, SACK_CHUNK_CUMULATIVE_TSN_ACK_OFFSET);

  acks_tree = proto_item_add_subtree(ctsa_item,ett_sctp_ack);
  sctp_ack_block(pinfo, ha, chunk_tvb, acks_tree, NULL, cum_tsn_ack);

  last_end = 0;
  for(gap_block_number = 0; gap_block_number < number_of_gap_blocks; gap_block_number++) {
    proto_item *pi;
    proto_tree *pt;
    guint32 tsn_start;

    start = tvb_get_ntohs(chunk_tvb, gap_block_offset);
    end   = tvb_get_ntohs(chunk_tvb, gap_block_offset + SACK_CHUNK_GAP_BLOCK_START_LENGTH);
    tsn_start = cum_tsn_ack + start;

    block_tree = proto_tree_add_subtree_format(chunk_tree, chunk_tvb, gap_block_offset, SACK_CHUNK_GAP_BLOCK_LENGTH,
                        ett_sctp_sack_chunk_gap_block, NULL, "Gap Acknowledgement for TSN %u to %u", cum_tsn_ack + start, cum_tsn_ack + end);

    pi = proto_tree_add_item(block_tree, hf_sack_chunk_gap_block_start, chunk_tvb, gap_block_offset, SACK_CHUNK_GAP_BLOCK_START_LENGTH, ENC_BIG_ENDIAN);
    pt = proto_item_add_subtree(pi, ett_sctp_sack_chunk_gap_block_start);
    pi = proto_tree_add_uint(pt, hf_sack_chunk_gap_block_start_tsn,
                             chunk_tvb, gap_block_offset,SACK_CHUNK_GAP_BLOCK_START_LENGTH, cum_tsn_ack + start);
    PROTO_ITEM_SET_GENERATED(pi);

    pi = proto_tree_add_item(block_tree, hf_sack_chunk_gap_block_end, chunk_tvb, gap_block_offset + SACK_CHUNK_GAP_BLOCK_START_LENGTH, SACK_CHUNK_GAP_BLOCK_END_LENGTH,   ENC_BIG_ENDIAN);
    pt = proto_item_add_subtree(pi, ett_sctp_sack_chunk_gap_block_end);
    pi = proto_tree_add_uint(pt, hf_sack_chunk_gap_block_end_tsn, chunk_tvb,
                             gap_block_offset + SACK_CHUNK_GAP_BLOCK_START_LENGTH, SACK_CHUNK_GAP_BLOCK_END_LENGTH, cum_tsn_ack + end);
    PROTO_ITEM_SET_GENERATED(pi);

    sctp_ack_block(pinfo, ha, chunk_tvb, block_tree, &tsn_start, cum_tsn_ack + end);
    gap_block_offset += SACK_CHUNK_GAP_BLOCK_LENGTH;

    tsns_gap_acked += (end+1 - start);

    /* Check validity */
    if (start > end) {
       expert_add_info(pinfo, pi, &ei_sctp_sack_chunk_gap_block_malformed);
    }
    if (last_end > start) {
       expert_add_info(pinfo, pi, &ei_sctp_sack_chunk_gap_block_out_of_order);
    }
    last_end = end;
  }

  if (tsns_gap_acked) {
    proto_item *pi;

    pi = proto_tree_add_uint(chunk_tree, hf_sack_chunk_number_tsns_gap_acked, chunk_tvb, 0, 0, tsns_gap_acked);
    PROTO_ITEM_SET_GENERATED(pi);

    /*  If there are a huge number of GAP ACKs, warn the user.  100 is a random
     *  number: it could be tuned.
     */
    if (tsns_gap_acked > 100)
      expert_add_info(pinfo, pi, &ei_sctp_sack_chunk_number_tsns_gap_acked_100);

  }


  /* handle the duplicate TSNs */
  number_of_dup_tsns = tvb_get_ntohs(chunk_tvb, SACK_CHUNK_NUMBER_OF_DUP_TSNS_OFFSET);
  dup_tsn_offset     = SACK_CHUNK_GAP_BLOCK_OFFSET + number_of_gap_blocks * SACK_CHUNK_GAP_BLOCK_LENGTH;
  for(dup_tsn_number = 0; dup_tsn_number < number_of_dup_tsns; dup_tsn_number++) {
    proto_tree_add_item(chunk_tree, hf_sack_chunk_duplicate_tsn, chunk_tvb, dup_tsn_offset, SACK_CHUNK_DUP_TSN_LENGTH, ENC_BIG_ENDIAN);
    dup_tsn_offset += SACK_CHUNK_DUP_TSN_LENGTH;
  }

  proto_item_append_text(chunk_item, " (Cumulative TSN: %u, a_rwnd: %u, gaps: %u, duplicate TSNs: %u)",
                         tvb_get_ntohl(chunk_tvb, SACK_CHUNK_CUMULATIVE_TSN_ACK_OFFSET),
                         a_rwnd,
                         number_of_gap_blocks, number_of_dup_tsns);
}

/* NE: Dissect nr-sack chunk */
#define SCTP_NR_SACK_CHUNK_NS_BIT                  0x01
#define NR_SACK_CHUNK_CUMULATIVE_TSN_ACK_LENGTH       4
#define NR_SACK_CHUNK_ADV_REC_WINDOW_CREDIT_LENGTH    4
#define NR_SACK_CHUNK_NUMBER_OF_GAP_BLOCKS_LENGTH     2
#define NR_SACK_CHUNK_NUMBER_OF_NR_GAP_BLOCKS_LENGTH  2
#define NR_SACK_CHUNK_NUMBER_OF_DUP_TSNS_LENGTH       2
#define NR_SACK_CHUNK_RESERVED_LENGTH                 2
#define NR_SACK_CHUNK_GAP_BLOCK_LENGTH                4
#define NR_SACK_CHUNK_GAP_BLOCK_START_LENGTH          2
#define NR_SACK_CHUNK_GAP_BLOCK_END_LENGTH            2
#define NR_SACK_CHUNK_NR_GAP_BLOCK_LENGTH             4
#define NR_SACK_CHUNK_NR_GAP_BLOCK_START_LENGTH       2
#define NR_SACK_CHUNK_NR_GAP_BLOCK_END_LENGTH         2
#define NR_SACK_CHUNK_DUP_TSN_LENGTH                  4

#define NR_SACK_CHUNK_CUMULATIVE_TSN_ACK_OFFSET        (CHUNK_VALUE_OFFSET + 0)
#define NR_SACK_CHUNK_ADV_REC_WINDOW_CREDIT_OFFSET     (NR_SACK_CHUNK_CUMULATIVE_TSN_ACK_OFFSET + \
                                                        NR_SACK_CHUNK_CUMULATIVE_TSN_ACK_LENGTH)
#define NR_SACK_CHUNK_NUMBER_OF_GAP_BLOCKS_OFFSET      (NR_SACK_CHUNK_ADV_REC_WINDOW_CREDIT_OFFSET + \
                                                        NR_SACK_CHUNK_ADV_REC_WINDOW_CREDIT_LENGTH)
#define NR_SACK_CHUNK_NUMBER_OF_NR_GAP_BLOCKS_OFFSET   (NR_SACK_CHUNK_NUMBER_OF_GAP_BLOCKS_OFFSET + \
                                                        NR_SACK_CHUNK_NUMBER_OF_GAP_BLOCKS_LENGTH)
#define NR_SACK_CHUNK_NUMBER_OF_DUP_TSNS_OFFSET        (NR_SACK_CHUNK_NUMBER_OF_NR_GAP_BLOCKS_OFFSET + \
                                                        NR_SACK_CHUNK_NUMBER_OF_NR_GAP_BLOCKS_LENGTH)
#define NR_SACK_CHUNK_RESERVED_OFFSET                  (NR_SACK_CHUNK_NUMBER_OF_DUP_TSNS_OFFSET + \
                                                        NR_SACK_CHUNK_NUMBER_OF_DUP_TSNS_LENGTH)
#define NR_SACK_CHUNK_GAP_BLOCK_OFFSET                 (NR_SACK_CHUNK_RESERVED_OFFSET + \
                                                        NR_SACK_CHUNK_RESERVED_LENGTH)

static void
dissect_nr_sack_chunk(packet_info *pinfo, tvbuff_t *chunk_tvb, proto_tree *chunk_tree, proto_item *chunk_item, proto_item *flags_item, sctp_half_assoc_t *ha)
{
  guint16 number_of_gap_blocks, number_of_dup_tsns;
  guint16 number_of_nr_gap_blocks;
  guint16 gap_block_number, nr_gap_block_number, dup_tsn_number, start, end;
  gint gap_block_offset, nr_gap_block_offset, dup_tsn_offset;
  guint32 cum_tsn_ack;
  proto_tree *block_tree;
  proto_tree *flags_tree;
  proto_item *ctsa_item;
  proto_tree *acks_tree;
  guint32 tsns_gap_acked = 0;
  guint32 tsns_nr_gap_acked = 0;
  guint16 last_end;

  flags_tree  = proto_item_add_subtree(flags_item, ett_sctp_nr_sack_chunk_flags);
  proto_tree_add_item(flags_tree, hf_nr_sack_chunk_ns, chunk_tvb, CHUNK_FLAGS_OFFSET, CHUNK_FLAGS_LENGTH, ENC_BIG_ENDIAN);

  ctsa_item = proto_tree_add_item(chunk_tree, hf_nr_sack_chunk_cumulative_tsn_ack, chunk_tvb, NR_SACK_CHUNK_CUMULATIVE_TSN_ACK_OFFSET, NR_SACK_CHUNK_CUMULATIVE_TSN_ACK_LENGTH, ENC_BIG_ENDIAN);
  proto_tree_add_item(chunk_tree, hf_nr_sack_chunk_adv_rec_window_credit, chunk_tvb, NR_SACK_CHUNK_ADV_REC_WINDOW_CREDIT_OFFSET, NR_SACK_CHUNK_ADV_REC_WINDOW_CREDIT_LENGTH, ENC_BIG_ENDIAN);

  proto_tree_add_item(chunk_tree, hf_nr_sack_chunk_number_of_gap_blocks, chunk_tvb, NR_SACK_CHUNK_NUMBER_OF_GAP_BLOCKS_OFFSET,  NR_SACK_CHUNK_NUMBER_OF_GAP_BLOCKS_LENGTH, ENC_BIG_ENDIAN);

  proto_tree_add_item(chunk_tree, hf_nr_sack_chunk_number_of_nr_gap_blocks,  chunk_tvb, NR_SACK_CHUNK_NUMBER_OF_NR_GAP_BLOCKS_OFFSET, NR_SACK_CHUNK_NUMBER_OF_NR_GAP_BLOCKS_LENGTH, ENC_BIG_ENDIAN);
  proto_tree_add_item(chunk_tree, hf_nr_sack_chunk_number_of_dup_tsns, chunk_tvb, NR_SACK_CHUNK_NUMBER_OF_DUP_TSNS_OFFSET, NR_SACK_CHUNK_NUMBER_OF_DUP_TSNS_LENGTH, ENC_BIG_ENDIAN);
  proto_tree_add_item(chunk_tree, hf_nr_sack_chunk_reserved, chunk_tvb, NR_SACK_CHUNK_RESERVED_OFFSET, NR_SACK_CHUNK_RESERVED_LENGTH, ENC_BIG_ENDIAN);


  number_of_gap_blocks = tvb_get_ntohs(chunk_tvb, NR_SACK_CHUNK_NUMBER_OF_GAP_BLOCKS_OFFSET);
  gap_block_offset     = NR_SACK_CHUNK_GAP_BLOCK_OFFSET;
  cum_tsn_ack          = tvb_get_ntohl(chunk_tvb, NR_SACK_CHUNK_CUMULATIVE_TSN_ACK_OFFSET);

  acks_tree = proto_item_add_subtree(ctsa_item,ett_sctp_ack);
  sctp_ack_block(pinfo, ha, chunk_tvb, acks_tree, NULL, cum_tsn_ack);

  last_end = 0;
  for(gap_block_number = 0; gap_block_number < number_of_gap_blocks; gap_block_number++) {
    proto_item *pi;
    proto_tree *pt;
    guint32 tsn_start;

    start = tvb_get_ntohs(chunk_tvb, gap_block_offset);
    end   = tvb_get_ntohs(chunk_tvb, gap_block_offset + NR_SACK_CHUNK_GAP_BLOCK_START_LENGTH);
    tsn_start = cum_tsn_ack + start;

    block_tree = proto_tree_add_subtree_format(chunk_tree, chunk_tvb, gap_block_offset, NR_SACK_CHUNK_GAP_BLOCK_LENGTH,
                        ett_sctp_nr_sack_chunk_gap_block, NULL, "Gap Acknowledgement for TSN %u to %u", cum_tsn_ack + start, cum_tsn_ack + end);

    pi = proto_tree_add_item(block_tree, hf_nr_sack_chunk_gap_block_start, chunk_tvb, gap_block_offset, NR_SACK_CHUNK_GAP_BLOCK_START_LENGTH, ENC_BIG_ENDIAN);
    pt = proto_item_add_subtree(pi, ett_sctp_nr_sack_chunk_gap_block_start);
    pi = proto_tree_add_uint(pt, hf_nr_sack_chunk_gap_block_start_tsn,
                             chunk_tvb, gap_block_offset,NR_SACK_CHUNK_GAP_BLOCK_START_LENGTH, cum_tsn_ack + start);
    PROTO_ITEM_SET_GENERATED(pi);

    pi = proto_tree_add_item(block_tree, hf_nr_sack_chunk_gap_block_end, chunk_tvb, gap_block_offset + NR_SACK_CHUNK_GAP_BLOCK_START_LENGTH, NR_SACK_CHUNK_GAP_BLOCK_END_LENGTH,   ENC_BIG_ENDIAN);
    pt = proto_item_add_subtree(pi, ett_sctp_nr_sack_chunk_gap_block_end);
    pi = proto_tree_add_uint(pt, hf_nr_sack_chunk_gap_block_end_tsn, chunk_tvb,
                               gap_block_offset + NR_SACK_CHUNK_GAP_BLOCK_START_LENGTH, NR_SACK_CHUNK_GAP_BLOCK_END_LENGTH, cum_tsn_ack + end);
    PROTO_ITEM_SET_GENERATED(pi);

    sctp_ack_block(pinfo, ha, chunk_tvb, block_tree, &tsn_start, cum_tsn_ack + end);
    gap_block_offset += NR_SACK_CHUNK_GAP_BLOCK_LENGTH;
    tsns_gap_acked += (end - start) + 1;

    /* Check validity */
    if (start > end) {
       expert_add_info(pinfo, pi, &ei_sctp_sack_chunk_gap_block_malformed);
    }
    if (last_end > start) {
       expert_add_info(pinfo, pi, &ei_sctp_sack_chunk_gap_block_out_of_order);
    }
    last_end = end;
  }

  if (tsns_gap_acked) {
    proto_item *pi;

    pi = proto_tree_add_uint(chunk_tree, hf_nr_sack_chunk_number_tsns_gap_acked, chunk_tvb, 0, 0, tsns_gap_acked);
    PROTO_ITEM_SET_GENERATED(pi);

    /*  If there are a huge number of GAP ACKs, warn the user.  100 is a random
     *  number: it could be tuned.
     */
    if (tsns_gap_acked > 100)
      expert_add_info(pinfo, pi, &ei_sctp_nr_sack_chunk_number_tsns_gap_acked_100);

  }

  /* NE: handle the nr-sack chunk's nr-gap blocks */
  number_of_nr_gap_blocks = tvb_get_ntohs(chunk_tvb, NR_SACK_CHUNK_NUMBER_OF_NR_GAP_BLOCKS_OFFSET);
  nr_gap_block_offset     = gap_block_offset;

  last_end = 0;
  for(nr_gap_block_number = 0; nr_gap_block_number < number_of_nr_gap_blocks; nr_gap_block_number++) {
    proto_item *pi;
    proto_tree *pt;
    /*guint32 tsn_start;*/

    start = tvb_get_ntohs(chunk_tvb, nr_gap_block_offset);
    end   = tvb_get_ntohs(chunk_tvb, nr_gap_block_offset + NR_SACK_CHUNK_NR_GAP_BLOCK_START_LENGTH);
    /*tsn_start = cum_tsn_ack + start;*/

    block_tree = proto_tree_add_subtree_format(chunk_tree, chunk_tvb, nr_gap_block_offset, NR_SACK_CHUNK_NR_GAP_BLOCK_LENGTH,
            ett_sctp_nr_sack_chunk_nr_gap_block, NULL, "NR-Gap Acknowledgement for TSN %u to %u", cum_tsn_ack + start, cum_tsn_ack + end);

    pi = proto_tree_add_item(block_tree, hf_nr_sack_chunk_nr_gap_block_start, chunk_tvb, nr_gap_block_offset, NR_SACK_CHUNK_NR_GAP_BLOCK_START_LENGTH, ENC_BIG_ENDIAN);
    pt = proto_item_add_subtree(pi, ett_sctp_nr_sack_chunk_nr_gap_block_start);
    pi = proto_tree_add_uint(pt, hf_nr_sack_chunk_nr_gap_block_start_tsn,
                             chunk_tvb, nr_gap_block_offset, NR_SACK_CHUNK_NR_GAP_BLOCK_START_LENGTH, cum_tsn_ack + start);
    PROTO_ITEM_SET_GENERATED(pi);

    pi = proto_tree_add_item(block_tree, hf_nr_sack_chunk_nr_gap_block_end, chunk_tvb, nr_gap_block_offset + NR_SACK_CHUNK_NR_GAP_BLOCK_START_LENGTH, NR_SACK_CHUNK_NR_GAP_BLOCK_END_LENGTH,   ENC_BIG_ENDIAN);
    pt = proto_item_add_subtree(pi, ett_sctp_nr_sack_chunk_nr_gap_block_end);
    pi = proto_tree_add_uint(pt, hf_nr_sack_chunk_nr_gap_block_end_tsn, chunk_tvb,
                             nr_gap_block_offset + NR_SACK_CHUNK_NR_GAP_BLOCK_START_LENGTH, NR_SACK_CHUNK_NR_GAP_BLOCK_END_LENGTH, cum_tsn_ack + end);
    PROTO_ITEM_SET_GENERATED(pi);

    /* sctp_ack_block(pinfo, ha, chunk_tvb, block_tree, &tsn_start, cum_tsn_ack + end); */
    nr_gap_block_offset += NR_SACK_CHUNK_NR_GAP_BLOCK_LENGTH;
    tsns_nr_gap_acked += (end - start) + 1;

    /* Check validity */
    if (start > end) {
       expert_add_info(pinfo, pi, &ei_sctp_sack_chunk_gap_block_malformed);
    }
    if (last_end > start) {
       expert_add_info(pinfo, pi, &ei_sctp_sack_chunk_gap_block_out_of_order);
    }
    last_end = end;
  }

  if (tsns_nr_gap_acked) {
    proto_item *pi;

    pi = proto_tree_add_uint(chunk_tree, hf_nr_sack_chunk_number_tsns_nr_gap_acked, chunk_tvb, 0, 0, tsns_nr_gap_acked);
    PROTO_ITEM_SET_GENERATED(pi);

    /*  If there are a huge number of GAP ACKs, warn the user.  100 is a random
     *  number: it could be tuned.
     */
    if (tsns_nr_gap_acked > 100)
      expert_add_info(pinfo, pi, &ei_sctp_nr_sack_chunk_number_tsns_nr_gap_acked_100);
  }

  /* handle the duplicate TSNs */
  number_of_dup_tsns = tvb_get_ntohs(chunk_tvb, NR_SACK_CHUNK_NUMBER_OF_DUP_TSNS_OFFSET);
  dup_tsn_offset     = NR_SACK_CHUNK_GAP_BLOCK_OFFSET + number_of_gap_blocks * NR_SACK_CHUNK_GAP_BLOCK_LENGTH
    + number_of_nr_gap_blocks * NR_SACK_CHUNK_NR_GAP_BLOCK_LENGTH;


  for(dup_tsn_number = 0; dup_tsn_number < number_of_dup_tsns; dup_tsn_number++) {
    proto_tree_add_item(chunk_tree, hf_sack_chunk_duplicate_tsn, chunk_tvb, dup_tsn_offset, NR_SACK_CHUNK_DUP_TSN_LENGTH, ENC_BIG_ENDIAN);
    dup_tsn_offset += NR_SACK_CHUNK_DUP_TSN_LENGTH;
  }

  proto_item_append_text(chunk_item, " (Cumulative TSN: %u, a_rwnd: %u, gaps: %u, nr-gaps: %u, duplicate TSNs: %u)",
                         tvb_get_ntohl(chunk_tvb, NR_SACK_CHUNK_CUMULATIVE_TSN_ACK_OFFSET),
                         tvb_get_ntohl(chunk_tvb, NR_SACK_CHUNK_ADV_REC_WINDOW_CREDIT_OFFSET),
                         number_of_gap_blocks, number_of_nr_gap_blocks, number_of_dup_tsns);
}

#define HEARTBEAT_CHUNK_INFO_OFFSET CHUNK_VALUE_OFFSET

static void
dissect_heartbeat_chunk(tvbuff_t *chunk_tvb, guint16 chunk_length, packet_info *pinfo, proto_tree *chunk_tree, proto_item *chunk_item)
{
  tvbuff_t   *parameter_tvb;

  if (chunk_tree) {
    proto_item_append_text(chunk_item, " (Information: %u byte%s)", chunk_length - CHUNK_HEADER_LENGTH, plurality(chunk_length - CHUNK_HEADER_LENGTH, "", "s"));
    parameter_tvb  = tvb_new_subset(chunk_tvb, HEARTBEAT_CHUNK_INFO_OFFSET,
                                    MIN(chunk_length - CHUNK_HEADER_LENGTH, tvb_captured_length_remaining(chunk_tvb, HEARTBEAT_CHUNK_INFO_OFFSET)),
                                    MIN(chunk_length - CHUNK_HEADER_LENGTH, tvb_reported_length_remaining(chunk_tvb, HEARTBEAT_CHUNK_INFO_OFFSET)));
    /* FIXME: Parameters or parameter? */
    dissect_parameter(parameter_tvb, pinfo, chunk_tree, NULL, FALSE, TRUE);
  }
}

#define HEARTBEAT_ACK_CHUNK_INFO_OFFSET CHUNK_VALUE_OFFSET

static void
dissect_heartbeat_ack_chunk(tvbuff_t *chunk_tvb, guint16 chunk_length, packet_info *pinfo, proto_tree *chunk_tree, proto_item *chunk_item)
{
  tvbuff_t   *parameter_tvb;

  if (chunk_tree) {
    proto_item_append_text(chunk_item, " (Information: %u byte%s)", chunk_length - CHUNK_HEADER_LENGTH, plurality(chunk_length - CHUNK_HEADER_LENGTH, "", "s"));
    parameter_tvb  = tvb_new_subset(chunk_tvb, HEARTBEAT_ACK_CHUNK_INFO_OFFSET,
                                    MIN(chunk_length - CHUNK_HEADER_LENGTH, tvb_captured_length_remaining(chunk_tvb, HEARTBEAT_ACK_CHUNK_INFO_OFFSET)),
                                    MIN(chunk_length - CHUNK_HEADER_LENGTH, tvb_reported_length_remaining(chunk_tvb, HEARTBEAT_ACK_CHUNK_INFO_OFFSET)));
    /* FIXME: Parameters or parameter? */
    dissect_parameter(parameter_tvb, pinfo, chunk_tree, NULL, FALSE, TRUE);
  }
}

#define ABORT_CHUNK_FIRST_ERROR_CAUSE_OFFSET 4
#define SCTP_ABORT_CHUNK_T_BIT               0x01

static void
dissect_abort_chunk(tvbuff_t *chunk_tvb, guint16 chunk_length, packet_info *pinfo, proto_tree *chunk_tree, proto_item *flags_item)
{
  tvbuff_t *causes_tvb;
  proto_tree *flags_tree;

  sctp_info.vtag_reflected =
      (tvb_get_guint8(chunk_tvb, CHUNK_FLAGS_OFFSET) & SCTP_ABORT_CHUNK_T_BIT) != 0;

  if (chunk_tree) {
    flags_tree  = proto_item_add_subtree(flags_item, ett_sctp_abort_chunk_flags);
    proto_tree_add_item(flags_tree, hf_abort_chunk_t_bit, chunk_tvb, CHUNK_FLAGS_OFFSET, CHUNK_FLAGS_LENGTH, ENC_BIG_ENDIAN);
    causes_tvb  = tvb_new_subset(chunk_tvb, CHUNK_VALUE_OFFSET,
                                 MIN(chunk_length - CHUNK_HEADER_LENGTH, tvb_captured_length_remaining(chunk_tvb, CHUNK_VALUE_OFFSET)),
                                 MIN(chunk_length - CHUNK_HEADER_LENGTH, tvb_reported_length_remaining(chunk_tvb, CHUNK_VALUE_OFFSET)));
    dissect_error_causes(causes_tvb, pinfo, chunk_tree);
  }
}

#define SHUTDOWN_CHUNK_CUMULATIVE_TSN_ACK_OFFSET CHUNK_VALUE_OFFSET
#define SHUTDOWN_CHUNK_CUMULATIVE_TSN_ACK_LENGTH 4

static void
dissect_shutdown_chunk(tvbuff_t *chunk_tvb, proto_tree *chunk_tree, proto_item *chunk_item)
{
  if (chunk_tree) {
    proto_tree_add_item(chunk_tree, hf_shutdown_chunk_cumulative_tsn_ack, chunk_tvb, SHUTDOWN_CHUNK_CUMULATIVE_TSN_ACK_OFFSET, SHUTDOWN_CHUNK_CUMULATIVE_TSN_ACK_LENGTH, ENC_BIG_ENDIAN);
    proto_item_append_text(chunk_item, " (Cumulative TSN ack: %u)", tvb_get_ntohl(chunk_tvb, SHUTDOWN_CHUNK_CUMULATIVE_TSN_ACK_OFFSET));
  }
}

static void
dissect_shutdown_ack_chunk(tvbuff_t *chunk_tvb _U_)
{
}

#define ERROR_CAUSE_IND_CAUSES_OFFSET CHUNK_VALUE_OFFSET

static void
dissect_error_chunk(tvbuff_t *chunk_tvb, guint16 chunk_length, packet_info *pinfo, proto_tree *chunk_tree)
{
  tvbuff_t *causes_tvb;

  if (chunk_tree) {
    causes_tvb = tvb_new_subset(chunk_tvb, ERROR_CAUSE_IND_CAUSES_OFFSET,
                                MIN(chunk_length - CHUNK_HEADER_LENGTH, tvb_captured_length_remaining(chunk_tvb, ERROR_CAUSE_IND_CAUSES_OFFSET)),
                                MIN(chunk_length - CHUNK_HEADER_LENGTH, tvb_reported_length_remaining(chunk_tvb, ERROR_CAUSE_IND_CAUSES_OFFSET)));
    dissect_error_causes(causes_tvb, pinfo, chunk_tree);
  }
}

#define COOKIE_OFFSET CHUNK_VALUE_OFFSET

static void
dissect_cookie_echo_chunk(tvbuff_t *chunk_tvb, guint16 chunk_length, proto_tree *chunk_tree, proto_item *chunk_item)
{
  if (chunk_tree) {
    proto_tree_add_item(chunk_tree, hf_cookie, chunk_tvb, COOKIE_OFFSET, chunk_length - CHUNK_HEADER_LENGTH, ENC_NA);
    proto_item_append_text(chunk_item, " (Cookie length: %u byte%s)", chunk_length - CHUNK_HEADER_LENGTH, plurality(chunk_length - CHUNK_HEADER_LENGTH, "", "s"));
  }
}

static void
dissect_cookie_ack_chunk(tvbuff_t *chunk_tvb _U_)
{
}

#define ECNE_CHUNK_LOWEST_TSN_OFFSET CHUNK_VALUE_OFFSET
#define ECNE_CHUNK_LOWEST_TSN_LENGTH 4

static void
dissect_ecne_chunk(tvbuff_t *chunk_tvb, proto_tree *chunk_tree, proto_item *chunk_item)
{
    proto_tree_add_item(chunk_tree, hf_ecne_chunk_lowest_tsn, chunk_tvb, ECNE_CHUNK_LOWEST_TSN_OFFSET, ECNE_CHUNK_LOWEST_TSN_LENGTH, ENC_BIG_ENDIAN);
    proto_item_append_text(chunk_item, " (Lowest TSN: %u)", tvb_get_ntohl(chunk_tvb, ECNE_CHUNK_LOWEST_TSN_OFFSET));
}

#define CWR_CHUNK_LOWEST_TSN_OFFSET CHUNK_VALUE_OFFSET
#define CWR_CHUNK_LOWEST_TSN_LENGTH 4

static void
dissect_cwr_chunk(tvbuff_t *chunk_tvb, proto_tree *chunk_tree, proto_item *chunk_item)
{
    proto_tree_add_item(chunk_tree, hf_cwr_chunk_lowest_tsn, chunk_tvb, CWR_CHUNK_LOWEST_TSN_OFFSET, CWR_CHUNK_LOWEST_TSN_LENGTH, ENC_BIG_ENDIAN);
    proto_item_append_text(chunk_item, " (Lowest TSN: %u)", tvb_get_ntohl(chunk_tvb, CWR_CHUNK_LOWEST_TSN_OFFSET));
}

#define SCTP_SHUTDOWN_COMPLETE_CHUNK_T_BIT 0x01


static const true_false_string sctp_shutdown_complete_chunk_t_bit_value = {
  "Tag reflected",
  "Tag not reflected"
};


static void
dissect_shutdown_complete_chunk(tvbuff_t *chunk_tvb, proto_tree *chunk_tree, proto_item *flags_item)
{
  proto_tree *flags_tree;

  sctp_info.vtag_reflected =
      (tvb_get_guint8(chunk_tvb, CHUNK_FLAGS_OFFSET) & SCTP_SHUTDOWN_COMPLETE_CHUNK_T_BIT) != 0;

  if (chunk_tree) {
    flags_tree  = proto_item_add_subtree(flags_item, ett_sctp_shutdown_complete_chunk_flags);
    proto_tree_add_item(flags_tree, hf_shutdown_complete_chunk_t_bit, chunk_tvb, CHUNK_FLAGS_OFFSET, CHUNK_FLAGS_LENGTH, ENC_BIG_ENDIAN);
  }
}

#define FORWARD_TSN_CHUNK_TSN_LENGTH 4
#define FORWARD_TSN_CHUNK_SID_LENGTH 2
#define FORWARD_TSN_CHUNK_SSN_LENGTH 2
#define FORWARD_TSN_CHUNK_TSN_OFFSET CHUNK_VALUE_OFFSET
#define FORWARD_TSN_CHUNK_SID_OFFSET 0
#define FORWARD_TSN_CHUNK_SSN_OFFSET (FORWARD_TSN_CHUNK_SID_OFFSET + FORWARD_TSN_CHUNK_SID_LENGTH)

static void
dissect_forward_tsn_chunk(tvbuff_t *chunk_tvb, guint16 chunk_length, proto_tree *chunk_tree, proto_item *chunk_item)
{
  guint   offset;
  guint16 number_of_affected_streams, affected_stream;

  /* FIXME */
  if (chunk_length < CHUNK_HEADER_LENGTH + FORWARD_TSN_CHUNK_TSN_LENGTH) {
    proto_item_append_text(chunk_item, ", bogus chunk length %u < %u)",
                           chunk_length,
                           CHUNK_HEADER_LENGTH + FORWARD_TSN_CHUNK_TSN_LENGTH);
    return;
  }
  if (chunk_tree) {
    proto_tree_add_item(chunk_tree, hf_forward_tsn_chunk_tsn, chunk_tvb, FORWARD_TSN_CHUNK_TSN_OFFSET, FORWARD_TSN_CHUNK_TSN_LENGTH, ENC_BIG_ENDIAN);
    number_of_affected_streams = (chunk_length - CHUNK_HEADER_LENGTH - FORWARD_TSN_CHUNK_TSN_LENGTH) /
                                 (FORWARD_TSN_CHUNK_SID_LENGTH + FORWARD_TSN_CHUNK_SSN_LENGTH);
    offset = CHUNK_VALUE_OFFSET + FORWARD_TSN_CHUNK_TSN_LENGTH;

    for(affected_stream = 0;  affected_stream < number_of_affected_streams; affected_stream++) {
        proto_tree_add_item(chunk_tree, hf_forward_tsn_chunk_sid, chunk_tvb, offset + FORWARD_TSN_CHUNK_SID_OFFSET, FORWARD_TSN_CHUNK_SID_LENGTH, ENC_BIG_ENDIAN);
        proto_tree_add_item(chunk_tree, hf_forward_tsn_chunk_ssn, chunk_tvb, offset + FORWARD_TSN_CHUNK_SSN_OFFSET, FORWARD_TSN_CHUNK_SSN_LENGTH, ENC_BIG_ENDIAN);
        offset = offset + (FORWARD_TSN_CHUNK_SID_LENGTH + FORWARD_TSN_CHUNK_SSN_LENGTH);
    }
    proto_item_append_text(chunk_item, "(Cumulative TSN: %u)", tvb_get_ntohl(chunk_tvb, FORWARD_TSN_CHUNK_TSN_OFFSET));
  }
}

#define I_FORWARD_TSN_CHUNK_TSN_LENGTH   4
#define I_FORWARD_TSN_CHUNK_SID_LENGTH   2
#define I_FORWARD_TSN_CHUNK_FLAGS_LENGTH 2
#define I_FORWARD_TSN_CHUNK_MID_LENGTH   4
#define I_FORWARD_TSN_CHUNK_TSN_OFFSET   CHUNK_VALUE_OFFSET
#define I_FORWARD_TSN_CHUNK_SID_OFFSET   0
#define I_FORWARD_TSN_CHUNK_FLAGS_OFFSET (I_FORWARD_TSN_CHUNK_SID_OFFSET + I_FORWARD_TSN_CHUNK_SID_LENGTH)
#define I_FORWARD_TSN_CHUNK_MID_OFFSET   (I_FORWARD_TSN_CHUNK_FLAGS_OFFSET + I_FORWARD_TSN_CHUNK_FLAGS_LENGTH)

#define SCTP_I_FORWARD_TSN_CHUNK_U_BIT    0x0001
#define SCTP_I_FORWARD_TSN_CHUNK_RES_MASK 0xfffe

static const true_false_string sctp_i_forward_tsn_chunk_u_bit_value = {
  "Unordered messages",
  "Ordered messages"
};

static void
dissect_i_forward_tsn_chunk(tvbuff_t *chunk_tvb, guint16 chunk_length, proto_tree *chunk_tree, proto_item *chunk_item)
{
  guint   offset;
  guint16 number_of_affected_streams, affected_stream;
  proto_tree *flags_tree;
  proto_item *flags_item = NULL;

  /* FIXME */
  if (chunk_length < CHUNK_HEADER_LENGTH + I_FORWARD_TSN_CHUNK_TSN_LENGTH) {
    proto_item_append_text(chunk_item, ", bogus chunk length %u < %u)",
                           chunk_length,
                           CHUNK_HEADER_LENGTH + I_FORWARD_TSN_CHUNK_TSN_LENGTH);
    return;
  }
  if (chunk_tree) {
    proto_tree_add_item(chunk_tree, hf_i_forward_tsn_chunk_tsn, chunk_tvb, I_FORWARD_TSN_CHUNK_TSN_OFFSET, I_FORWARD_TSN_CHUNK_TSN_LENGTH, ENC_BIG_ENDIAN);
    number_of_affected_streams = (chunk_length - CHUNK_HEADER_LENGTH - I_FORWARD_TSN_CHUNK_TSN_LENGTH) /
                                 (I_FORWARD_TSN_CHUNK_SID_LENGTH + I_FORWARD_TSN_CHUNK_FLAGS_LENGTH + I_FORWARD_TSN_CHUNK_MID_LENGTH);
    offset = CHUNK_VALUE_OFFSET + I_FORWARD_TSN_CHUNK_TSN_LENGTH;

    for(affected_stream = 0;  affected_stream < number_of_affected_streams; affected_stream++) {
        proto_tree_add_item(chunk_tree, hf_i_forward_tsn_chunk_sid, chunk_tvb, offset + I_FORWARD_TSN_CHUNK_SID_OFFSET, I_FORWARD_TSN_CHUNK_SID_LENGTH, ENC_BIG_ENDIAN);
        flags_item = proto_tree_add_item(chunk_tree, hf_i_forward_tsn_chunk_flags, chunk_tvb, offset + I_FORWARD_TSN_CHUNK_FLAGS_OFFSET, I_FORWARD_TSN_CHUNK_FLAGS_LENGTH, ENC_BIG_ENDIAN);
        flags_tree  = proto_item_add_subtree(flags_item, ett_sctp_i_forward_tsn_chunk_flags);
        proto_tree_add_item(flags_tree, hf_i_forward_tsn_chunk_res, chunk_tvb, offset + I_FORWARD_TSN_CHUNK_FLAGS_OFFSET, I_FORWARD_TSN_CHUNK_FLAGS_LENGTH, ENC_BIG_ENDIAN);
        proto_tree_add_item(flags_tree, hf_i_forward_tsn_chunk_u_bit, chunk_tvb, offset + I_FORWARD_TSN_CHUNK_FLAGS_OFFSET, I_FORWARD_TSN_CHUNK_FLAGS_LENGTH, ENC_BIG_ENDIAN);
        proto_tree_add_item(chunk_tree, hf_i_forward_tsn_chunk_mid, chunk_tvb, offset + I_FORWARD_TSN_CHUNK_MID_OFFSET, I_FORWARD_TSN_CHUNK_MID_LENGTH, ENC_BIG_ENDIAN);
        offset += (I_FORWARD_TSN_CHUNK_SID_LENGTH + I_FORWARD_TSN_CHUNK_FLAGS_LENGTH + I_FORWARD_TSN_CHUNK_MID_LENGTH);
    }
    proto_item_append_text(chunk_item, "(Cumulative TSN: %u)", tvb_get_ntohl(chunk_tvb, I_FORWARD_TSN_CHUNK_TSN_OFFSET));
  }
}

#define RE_CONFIG_PARAMETERS_OFFSET CHUNK_HEADER_LENGTH

static void
dissect_re_config_chunk(tvbuff_t *chunk_tvb, guint16 chunk_length, packet_info *pinfo, proto_tree *chunk_tree, proto_item *chunk_item _U_)
{
  tvbuff_t *parameters_tvb;

  parameters_tvb = tvb_new_subset(chunk_tvb, RE_CONFIG_PARAMETERS_OFFSET,
                                  MIN(chunk_length - CHUNK_HEADER_LENGTH, tvb_captured_length_remaining(chunk_tvb, RE_CONFIG_PARAMETERS_OFFSET)),
                                  MIN(chunk_length - CHUNK_HEADER_LENGTH, tvb_reported_length_remaining(chunk_tvb, RE_CONFIG_PARAMETERS_OFFSET)));
  dissect_parameters(parameters_tvb, pinfo, chunk_tree, NULL, FALSE);
}

#define SHARED_KEY_ID_LENGTH 2

#define SHARED_KEY_ID_OFFSET PARAMETER_VALUE_OFFSET
#define HMAC_ID_OFFSET       (SHARED_KEY_ID_OFFSET + SHARED_KEY_ID_LENGTH)
#define HMAC_OFFSET          (HMAC_ID_OFFSET + HMAC_ID_LENGTH)

static void
dissect_auth_chunk(tvbuff_t *chunk_tvb, guint16 chunk_length, proto_tree *chunk_tree, proto_item *chunk_item _U_)
{
  guint hmac_length;

  hmac_length = chunk_length - CHUNK_HEADER_LENGTH - HMAC_ID_LENGTH - SHARED_KEY_ID_LENGTH;
  proto_tree_add_item(chunk_tree, hf_shared_key_id, chunk_tvb, SHARED_KEY_ID_OFFSET, SHARED_KEY_ID_LENGTH, ENC_BIG_ENDIAN);
  proto_tree_add_item(chunk_tree, hf_hmac_id,       chunk_tvb, HMAC_ID_OFFSET,       HMAC_ID_LENGTH,       ENC_BIG_ENDIAN);
  if (hmac_length > 0)
    proto_tree_add_item(chunk_tree, hf_hmac,    chunk_tvb, HMAC_OFFSET,    hmac_length,    ENC_NA);
}

#define SCTP_SEQUENCE_NUMBER_LENGTH    4
#define SEQUENCE_NUMBER_OFFSET    CHUNK_VALUE_OFFSET
#define ASCONF_CHUNK_PARAMETERS_OFFSET (SEQUENCE_NUMBER_OFFSET + SCTP_SEQUENCE_NUMBER_LENGTH)

static void
dissect_asconf_chunk(tvbuff_t *chunk_tvb, guint16 chunk_length, packet_info *pinfo, proto_tree *chunk_tree, proto_item *chunk_item)
{
  tvbuff_t *parameters_tvb;

  if (chunk_length < CHUNK_HEADER_LENGTH + SCTP_SEQUENCE_NUMBER_LENGTH) {
    proto_item_append_text(chunk_item, ", bogus chunk length %u < %u)",
                           chunk_length,
                           CHUNK_HEADER_LENGTH + SCTP_SEQUENCE_NUMBER_LENGTH);
    return;
  }
  if (chunk_tree) {
    proto_tree_add_item(chunk_tree, hf_asconf_seq_nr, chunk_tvb, SEQUENCE_NUMBER_OFFSET, SCTP_SEQUENCE_NUMBER_LENGTH, ENC_BIG_ENDIAN);
  }
  chunk_length -= CHUNK_HEADER_LENGTH + SCTP_SEQUENCE_NUMBER_LENGTH;
  parameters_tvb = tvb_new_subset(chunk_tvb, ASCONF_CHUNK_PARAMETERS_OFFSET,
                                  MIN(chunk_length, tvb_captured_length_remaining(chunk_tvb, ASCONF_CHUNK_PARAMETERS_OFFSET)),
                                  MIN(chunk_length, tvb_reported_length_remaining(chunk_tvb, ASCONF_CHUNK_PARAMETERS_OFFSET)));
  dissect_parameters(parameters_tvb, pinfo, chunk_tree, NULL, FALSE);
}

#define ASCONF_ACK_CHUNK_PARAMETERS_OFFSET (SEQUENCE_NUMBER_OFFSET + SCTP_SEQUENCE_NUMBER_LENGTH)

static void
dissect_asconf_ack_chunk(tvbuff_t *chunk_tvb, guint16 chunk_length, packet_info *pinfo, proto_tree *chunk_tree, proto_item *chunk_item)
{
  tvbuff_t *parameters_tvb;

  if (chunk_length < CHUNK_HEADER_LENGTH + SCTP_SEQUENCE_NUMBER_LENGTH) {
    proto_item_append_text(chunk_item, ", bogus chunk length %u < %u)",
                           chunk_length + CHUNK_HEADER_LENGTH,
                           CHUNK_HEADER_LENGTH + SCTP_SEQUENCE_NUMBER_LENGTH);
    return;
  }
  if (chunk_tree) {
    proto_tree_add_item(chunk_tree, hf_asconf_ack_seq_nr, chunk_tvb, SEQUENCE_NUMBER_OFFSET, SCTP_SEQUENCE_NUMBER_LENGTH, ENC_BIG_ENDIAN);
  }
  chunk_length -= CHUNK_HEADER_LENGTH + SCTP_SEQUENCE_NUMBER_LENGTH;
  parameters_tvb = tvb_new_subset(chunk_tvb, ASCONF_ACK_CHUNK_PARAMETERS_OFFSET,
                                  MIN(chunk_length, tvb_captured_length_remaining(chunk_tvb, ASCONF_ACK_CHUNK_PARAMETERS_OFFSET)),
                                  MIN(chunk_length, tvb_reported_length_remaining(chunk_tvb, ASCONF_ACK_CHUNK_PARAMETERS_OFFSET)));
  dissect_parameters(parameters_tvb, pinfo, chunk_tree, NULL, FALSE);
}

#define PKTDROP_CHUNK_BANDWIDTH_LENGTH      4
#define PKTDROP_CHUNK_QUEUESIZE_LENGTH      4
#define PKTDROP_CHUNK_TRUNCATED_SIZE_LENGTH 2
#define PKTDROP_CHUNK_RESERVED_SIZE_LENGTH  2

#define PKTDROP_CHUNK_HEADER_LENGTH (CHUNK_HEADER_LENGTH + \
                   PKTDROP_CHUNK_BANDWIDTH_LENGTH + \
                   PKTDROP_CHUNK_QUEUESIZE_LENGTH + \
                   PKTDROP_CHUNK_TRUNCATED_SIZE_LENGTH + \
                   PKTDROP_CHUNK_RESERVED_SIZE_LENGTH)

#define PKTDROP_CHUNK_BANDWIDTH_OFFSET      CHUNK_VALUE_OFFSET
#define PKTDROP_CHUNK_QUEUESIZE_OFFSET      (PKTDROP_CHUNK_BANDWIDTH_OFFSET + PKTDROP_CHUNK_BANDWIDTH_LENGTH)
#define PKTDROP_CHUNK_TRUNCATED_SIZE_OFFSET (PKTDROP_CHUNK_QUEUESIZE_OFFSET + PKTDROP_CHUNK_QUEUESIZE_LENGTH)
#define PKTDROP_CHUNK_RESERVED_SIZE_OFFSET  (PKTDROP_CHUNK_TRUNCATED_SIZE_OFFSET + PKTDROP_CHUNK_TRUNCATED_SIZE_LENGTH)
#define PKTDROP_CHUNK_DATA_FIELD_OFFSET     (PKTDROP_CHUNK_RESERVED_SIZE_OFFSET + PKTDROP_CHUNK_RESERVED_SIZE_LENGTH)

#define SCTP_PKTDROP_CHUNK_M_BIT 0x01
#define SCTP_PKTDROP_CHUNK_B_BIT 0x02
#define SCTP_PKTDROP_CHUNK_T_BIT 0x04

static const true_false_string sctp_pktdropk_m_bit_value = {
  "Source is a middlebox",
  "Source is an endhost"
};

static const true_false_string sctp_pktdropk_b_bit_value = {
  "SCTP checksum was incorrect",
  "SCTP checksum was correct"
};

static const true_false_string sctp_pktdropk_t_bit_value = {
  "Packet is truncated",
  "Packet is not truncated"
};

static void
dissect_pktdrop_chunk(tvbuff_t *chunk_tvb, guint16 chunk_length, packet_info *pinfo, proto_tree *chunk_tree, proto_item *chunk_item, proto_item *flags_item)
{
  tvbuff_t *data_field_tvb;
  proto_tree *flags_tree;

  if (chunk_length < PKTDROP_CHUNK_HEADER_LENGTH) {
    proto_item_append_text(chunk_item, ", bogus chunk length %u < %u)",
                           chunk_length,
                           PKTDROP_CHUNK_HEADER_LENGTH);
    return;
  }
  chunk_length -= PKTDROP_CHUNK_HEADER_LENGTH;
  data_field_tvb = tvb_new_subset(chunk_tvb, PKTDROP_CHUNK_DATA_FIELD_OFFSET,
                                  MIN(chunk_length, tvb_captured_length_remaining(chunk_tvb, PKTDROP_CHUNK_DATA_FIELD_OFFSET)),
                                  MIN(chunk_length, tvb_reported_length_remaining(chunk_tvb, PKTDROP_CHUNK_DATA_FIELD_OFFSET)));

  if (chunk_tree) {
    flags_tree  = proto_item_add_subtree(flags_item, ett_sctp_pktdrop_chunk_flags);

    proto_tree_add_item(flags_tree, hf_pktdrop_chunk_m_bit,            chunk_tvb, CHUNK_FLAGS_OFFSET,                  CHUNK_FLAGS_LENGTH,                  ENC_BIG_ENDIAN);
    proto_tree_add_item(flags_tree, hf_pktdrop_chunk_b_bit,            chunk_tvb, CHUNK_FLAGS_OFFSET,                  CHUNK_FLAGS_LENGTH,                  ENC_BIG_ENDIAN);
    proto_tree_add_item(flags_tree, hf_pktdrop_chunk_t_bit,            chunk_tvb, CHUNK_FLAGS_OFFSET,                  CHUNK_FLAGS_LENGTH,                  ENC_BIG_ENDIAN);
    proto_tree_add_item(chunk_tree, hf_pktdrop_chunk_bandwidth,        chunk_tvb, PKTDROP_CHUNK_BANDWIDTH_OFFSET,      PKTDROP_CHUNK_BANDWIDTH_LENGTH,      ENC_BIG_ENDIAN);
    proto_tree_add_item(chunk_tree, hf_pktdrop_chunk_queuesize,        chunk_tvb, PKTDROP_CHUNK_QUEUESIZE_OFFSET,      PKTDROP_CHUNK_QUEUESIZE_LENGTH,      ENC_BIG_ENDIAN);
    proto_tree_add_item(chunk_tree, hf_pktdrop_chunk_truncated_length, chunk_tvb, PKTDROP_CHUNK_TRUNCATED_SIZE_OFFSET, PKTDROP_CHUNK_TRUNCATED_SIZE_LENGTH, ENC_BIG_ENDIAN);
    proto_tree_add_item(chunk_tree, hf_pktdrop_chunk_reserved,         chunk_tvb, PKTDROP_CHUNK_RESERVED_SIZE_OFFSET,  PKTDROP_CHUNK_RESERVED_SIZE_LENGTH,  ENC_BIG_ENDIAN);

    if (chunk_length > 0) {
      /* XXX - We should dissect what we can and catch the BoundsError when we run out of bytes */
      if (tvb_get_guint8(chunk_tvb, CHUNK_FLAGS_OFFSET) & SCTP_PKTDROP_CHUNK_T_BIT)
        proto_tree_add_item(chunk_tree, hf_pktdrop_chunk_data_field,   chunk_tvb, PKTDROP_CHUNK_DATA_FIELD_OFFSET,     chunk_length,                   ENC_NA);
      else {
        gboolean save_in_error_pkt = pinfo->flags.in_error_pkt;
        pinfo->flags.in_error_pkt = TRUE;

        dissect_sctp_packet(data_field_tvb, pinfo, chunk_tree, TRUE);

        pinfo->flags.in_error_pkt = save_in_error_pkt;
      }
    }
  }
}

static void
dissect_pad_chunk(tvbuff_t *chunk_tvb, guint16 chunk_length, proto_tree *chunk_tree, proto_item *chunk_item)
{
  guint16 value_length;

  if (chunk_tree) {
    value_length = chunk_length - CHUNK_HEADER_LENGTH;
    proto_tree_add_item(chunk_tree, hf_pad_chunk_padding_data, chunk_tvb, CHUNK_VALUE_OFFSET, value_length, ENC_NA);
    proto_item_append_text(chunk_item, " (Padding data length: %u byte%s)", value_length, plurality(value_length, "", "s"));
  }
}

static void
dissect_unknown_chunk(tvbuff_t *chunk_tvb, guint16 chunk_length, guint8 chunk_type, proto_tree *chunk_tree, proto_item *chunk_item)
{
  guint16 value_length;

  if (chunk_tree) {
    value_length = chunk_length - CHUNK_HEADER_LENGTH;
    if (value_length > 0)
      proto_tree_add_item(chunk_tree, hf_chunk_value, chunk_tvb, CHUNK_VALUE_OFFSET, value_length, ENC_NA);
    proto_item_append_text(chunk_item, " (Type: %u, value length: %u byte%s)", chunk_type, value_length, plurality(value_length, "", "s"));
  }
}

#define SCTP_CHUNK_BIT_1 0x80
#define SCTP_CHUNK_BIT_2 0x40

static const true_false_string sctp_chunk_bit_1_value = {
  "Skip chunk and continue processing of the packet",
  "Stop processing of the packet"
};

static const true_false_string sctp_chunk_bit_2_value = {
  "Do report",
  "Do not report"
};


static gboolean
dissect_sctp_chunk(tvbuff_t *chunk_tvb,
                   packet_info *pinfo,
                   proto_tree *tree,
                   proto_tree *sctp_tree,
                   sctp_half_assoc_t *ha,
                   gboolean useinfo)
{
  guint8 type;
  guint16 length, padding_length, reported_length;
  gboolean result;
  proto_item *flags_item, *chunk_item, *type_item, *length_item;
  proto_tree *chunk_tree, *type_tree;

  result = FALSE;

  /* first extract the chunk header */
  type            = tvb_get_guint8(chunk_tvb, CHUNK_TYPE_OFFSET);
  length          = tvb_get_ntohs(chunk_tvb, CHUNK_LENGTH_OFFSET);
  reported_length = tvb_reported_length(chunk_tvb);
  padding_length  = reported_length - length;

  if (useinfo)
    col_append_fstr(pinfo->cinfo, COL_INFO, "%s ", val_to_str_const(type, chunk_type_values, "RESERVED"));

  /* create proto_tree stuff */
  chunk_tree   = proto_tree_add_subtree_format(sctp_tree, chunk_tvb, CHUNK_HEADER_OFFSET, reported_length,
                    ett_sctp_chunk, &chunk_item, "%s chunk",
                    val_to_str_const(type, chunk_type_values, "RESERVED"));

  if (tree) {
    /* then insert the chunk header components into the protocol tree */
    type_item  = proto_tree_add_item(chunk_tree, hf_chunk_type, chunk_tvb, CHUNK_TYPE_OFFSET, CHUNK_TYPE_LENGTH, ENC_BIG_ENDIAN);
    type_tree  = proto_item_add_subtree(type_item, ett_sctp_chunk_type);
    proto_tree_add_item(type_tree, hf_chunk_bit_1,  chunk_tvb, CHUNK_TYPE_OFFSET,  CHUNK_TYPE_LENGTH,  ENC_BIG_ENDIAN);
    proto_tree_add_item(type_tree, hf_chunk_bit_2,  chunk_tvb, CHUNK_TYPE_OFFSET,  CHUNK_TYPE_LENGTH,  ENC_BIG_ENDIAN);
    flags_item = proto_tree_add_item(chunk_tree, hf_chunk_flags, chunk_tvb, CHUNK_FLAGS_OFFSET, CHUNK_FLAGS_LENGTH, ENC_BIG_ENDIAN);
  } else {
    chunk_tree = NULL;
    chunk_item = NULL;
    flags_item = NULL;
  }

  if (length < CHUNK_HEADER_LENGTH) {
    if (tree) {
      proto_tree_add_uint_format_value(chunk_tree, hf_chunk_length, chunk_tvb, CHUNK_LENGTH_OFFSET, CHUNK_LENGTH_LENGTH, length,
                                 "%u (invalid, should be >= %u)", length, CHUNK_HEADER_LENGTH);
      proto_item_append_text(chunk_item, ", bogus chunk length %u < %u)", length, CHUNK_HEADER_LENGTH);
    }

    if (type == SCTP_DATA_CHUNK_ID)
      result = TRUE;
    return result;
  }

  length_item = proto_tree_add_uint(chunk_tree, hf_chunk_length, chunk_tvb, CHUNK_LENGTH_OFFSET, CHUNK_LENGTH_LENGTH, length);
  if (length > reported_length && !pinfo->flags.in_error_pkt) {
    expert_add_info_format(pinfo, length_item, &ei_sctp_chunk_length_bad, "Chunk length (%d) is longer than remaining data (%d) in the packet.", length, reported_length);
    /* We'll almost certainly throw an exception shortly... */
  }

  /*
  length -= CHUNK_HEADER_LENGTH;
  */

  /* now dissect the chunk value */
  switch(type) {
  case SCTP_DATA_CHUNK_ID:
    result = dissect_data_chunk(chunk_tvb, length, pinfo, tree, chunk_tree, chunk_item, flags_item, ha, FALSE);
    break;
  case SCTP_I_DATA_CHUNK_ID:
    result = dissect_data_chunk(chunk_tvb, length, pinfo, tree, chunk_tree, chunk_item, flags_item, ha, TRUE);
    break;
  case SCTP_INIT_CHUNK_ID:
    dissect_init_chunk(chunk_tvb, length, pinfo, chunk_tree, chunk_item);
    break;
  case SCTP_INIT_ACK_CHUNK_ID:
    dissect_init_ack_chunk(chunk_tvb, length, pinfo, chunk_tree, chunk_item);
    break;
  case SCTP_SACK_CHUNK_ID:
    dissect_sack_chunk(pinfo, chunk_tvb, chunk_tree, chunk_item, flags_item, ha);
    break;
  case SCTP_HEARTBEAT_CHUNK_ID:
    dissect_heartbeat_chunk(chunk_tvb, length, pinfo, chunk_tree, chunk_item);
    break;
  case SCTP_HEARTBEAT_ACK_CHUNK_ID:
    dissect_heartbeat_ack_chunk(chunk_tvb, length, pinfo, chunk_tree, chunk_item);
    break;
  case SCTP_ABORT_CHUNK_ID:
    dissect_abort_chunk(chunk_tvb, length, pinfo, chunk_tree, flags_item);
    break;
  case SCTP_SHUTDOWN_CHUNK_ID:
    dissect_shutdown_chunk(chunk_tvb, chunk_tree, chunk_item);
    break;
  case SCTP_SHUTDOWN_ACK_CHUNK_ID:
    dissect_shutdown_ack_chunk(chunk_tvb);
    break;
  case SCTP_ERROR_CHUNK_ID:
    dissect_error_chunk(chunk_tvb, length, pinfo, chunk_tree);
    break;
  case SCTP_COOKIE_ECHO_CHUNK_ID:
    dissect_cookie_echo_chunk(chunk_tvb, length, chunk_tree, chunk_item);
    break;
  case SCTP_COOKIE_ACK_CHUNK_ID:
    dissect_cookie_ack_chunk(chunk_tvb);
    break;
  case SCTP_ECNE_CHUNK_ID:
    dissect_ecne_chunk(chunk_tvb, chunk_tree, chunk_item);
    break;
  case SCTP_CWR_CHUNK_ID:
    dissect_cwr_chunk(chunk_tvb, chunk_tree, chunk_item);
    break;
  case SCTP_SHUTDOWN_COMPLETE_CHUNK_ID:
    dissect_shutdown_complete_chunk(chunk_tvb, chunk_tree, flags_item);
    break;
  case SCTP_FORWARD_TSN_CHUNK_ID:
    dissect_forward_tsn_chunk(chunk_tvb, length, chunk_tree, chunk_item);
    break;
  case SCTP_RE_CONFIG_CHUNK_ID:
    dissect_re_config_chunk(chunk_tvb, length, pinfo, chunk_tree, chunk_item);
    break;
  case SCTP_AUTH_CHUNK_ID:
    dissect_auth_chunk(chunk_tvb, length, chunk_tree, chunk_item);
    break;
  case SCTP_NR_SACK_CHUNK_ID:
    dissect_nr_sack_chunk(pinfo, chunk_tvb, chunk_tree, chunk_item, flags_item, ha);
    break;
  case SCTP_ASCONF_ACK_CHUNK_ID:
    dissect_asconf_ack_chunk(chunk_tvb, length, pinfo, chunk_tree, chunk_item);
    break;
  case SCTP_ASCONF_CHUNK_ID:
    dissect_asconf_chunk(chunk_tvb, length, pinfo, chunk_tree, chunk_item);
    break;
  case SCTP_I_FORWARD_TSN_CHUNK_ID:
    dissect_i_forward_tsn_chunk(chunk_tvb, length, chunk_tree, chunk_item);
    break;
  case SCTP_PKTDROP_CHUNK_ID:
    col_set_writable(pinfo->cinfo, -1, FALSE);
    dissect_pktdrop_chunk(chunk_tvb, length, pinfo, chunk_tree, chunk_item, flags_item);
    col_set_writable(pinfo->cinfo, -1, TRUE);
    break;
  case SCTP_PAD_CHUNK_ID:
    dissect_pad_chunk(chunk_tvb, length, chunk_tree, chunk_item);
    break;
  default:
    dissect_unknown_chunk(chunk_tvb, length, type, chunk_tree, chunk_item);
    break;
  }

  if (padding_length > 0)
    proto_tree_add_item(chunk_tree, hf_chunk_padding, chunk_tvb, CHUNK_HEADER_OFFSET + length, padding_length, ENC_NA);

  if (useinfo && ((type == SCTP_DATA_CHUNK_ID) || show_always_control_chunks))
    col_set_fence(pinfo->cinfo, COL_INFO);

  return result;
}

static void
dissect_sctp_chunks(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *sctp_item, proto_tree *sctp_tree, sctp_half_assoc_t *ha, gboolean encapsulated)
{
  tvbuff_t *chunk_tvb;
  guint16 length, total_length, remaining_length;
  gint last_offset, offset;
  gboolean sctp_item_length_set;
  assoc_info_t tmpinfo;
  infodata_t id_dir;

  /* the common header of the datagram is already handled */
  last_offset = 0;
  offset = COMMON_HEADER_LENGTH;
  sctp_item_length_set = FALSE;

  while((remaining_length = tvb_reported_length_remaining(tvb, offset))) {
    /* extract the chunk length and compute number of padding bytes */
    length         = tvb_get_ntohs(tvb, offset + CHUNK_LENGTH_OFFSET);
    total_length   = ADD_PADDING(length);

    /*  If we have less bytes than we need, throw an exception while dissecting
     *  the chunk--not when generating the chunk_tvb below.
     */
    total_length = MIN(total_length, remaining_length);

    /* create a tvb for the chunk including the padding bytes */
    chunk_tvb = tvb_new_subset(tvb, offset, MIN(total_length, tvb_captured_length_remaining(tvb, offset)), total_length);

    /* save it in the sctp_info structure */
    if (!encapsulated) {
      if (sctp_info.number_of_tvbs < MAXIMUM_NUMBER_OF_TVBS)
        sctp_info.tvb[sctp_info.number_of_tvbs++] = chunk_tvb;
      else
        sctp_info.incomplete = TRUE;
    }

    tmpinfo.assoc_index = -1;
    tmpinfo.sport = sctp_info.sport;
    tmpinfo.dport = sctp_info.dport;
    tmpinfo.vtag_reflected = FALSE;
    if (tvb_get_guint8(chunk_tvb, CHUNK_TYPE_OFFSET) == SCTP_ABORT_CHUNK_ID) {
      if ((tvb_get_guint8(chunk_tvb, CHUNK_FLAGS_OFFSET) & SCTP_ABORT_CHUNK_T_BIT) != 0) {
        tmpinfo.vtag_reflected = TRUE;
      }
    }
    if (tvb_get_guint8(chunk_tvb, CHUNK_TYPE_OFFSET) == SCTP_SHUTDOWN_COMPLETE_CHUNK_ID) {
      if ((tvb_get_guint8(chunk_tvb, CHUNK_FLAGS_OFFSET) & SCTP_SHUTDOWN_COMPLETE_CHUNK_T_BIT) != 0){
        tmpinfo.vtag_reflected = TRUE;
      }
    }
    if (tmpinfo.vtag_reflected) {
      tmpinfo.verification_tag2 = sctp_info.verification_tag;
      tmpinfo.verification_tag1 = 0;
    } else {
      tmpinfo.verification_tag1 = sctp_info.verification_tag;
      tmpinfo.verification_tag2 = 0;
    }
    if (tvb_get_guint8(chunk_tvb, CHUNK_TYPE_OFFSET) == SCTP_INIT_CHUNK_ID) {
      tmpinfo.initiate_tag = tvb_get_ntohl(sctp_info.tvb[0], 4);
    } else {
      tmpinfo.initiate_tag = 0;
    }

    id_dir = find_assoc_index(&tmpinfo, PINFO_FD_VISITED(pinfo));
    sctp_info.assoc_index = id_dir.assoc_index;
    sctp_info.direction = id_dir.direction;

    /* call dissect_sctp_chunk for the actual work */
    if (dissect_sctp_chunk(chunk_tvb, pinfo, tree, sctp_tree, ha, !encapsulated) && (tree)) {
      proto_item_set_len(sctp_item, offset - last_offset + DATA_CHUNK_HEADER_LENGTH);
      sctp_item_length_set = TRUE;
      offset += total_length;
      last_offset = offset;
      if (tvb_reported_length_remaining(tvb, offset) > 0) {
        sctp_item = proto_tree_add_item(tree, proto_sctp, tvb, offset, -1, ENC_NA);
        sctp_tree = proto_item_add_subtree(sctp_item, ett_sctp);
        sctp_item_length_set = FALSE;
      }
    } else {
      /* get rid of the dissected chunk */
      offset += total_length;
    }
  }
  if (!sctp_item_length_set && (tree)) {
    proto_item_set_len(sctp_item, offset - last_offset);
  }
}

static void
dissect_sctp_packet(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, gboolean encapsulated)
{
  guint32 checksum = 0, calculated_crc32c = 0, calculated_adler32 = 0;
  guint16 source_port, destination_port;
  guint captured_length, reported_length;
  gboolean crc32c_correct = FALSE, adler32_correct = FALSE;
  proto_item *sctp_item, *hidden_item;
  proto_tree *sctp_tree;
  guint32 vtag;
  sctp_half_assoc_t *ha = NULL;
  proto_item *pi, *vt = NULL;

  captured_length = tvb_captured_length(tvb);
  reported_length = tvb_reported_length(tvb);
  checksum        = tvb_get_ntohl(tvb, CHECKSUM_OFFSET);
  sctp_info.checksum_zero = (checksum == 0);

  /* Only try to checksum the packet if we have all of it */
  if (captured_length == reported_length) {

    switch(sctp_checksum) {
    case SCTP_CHECKSUM_NONE:
      break;
    case SCTP_CHECKSUM_ADLER32:
      calculated_adler32           = sctp_adler32(tvb, captured_length);
      adler32_correct              = (checksum == calculated_adler32);
      sctp_info.adler32_calculated = TRUE;
      sctp_info.adler32_correct    = adler32_correct;
      break;
    case SCTP_CHECKSUM_CRC32C:
      calculated_crc32c            = sctp_crc32c(tvb, captured_length);
      crc32c_correct               = (checksum == calculated_crc32c);
      sctp_info.crc32c_calculated  = TRUE;
      sctp_info.crc32c_correct     = crc32c_correct;
      break;
    case SCTP_CHECKSUM_AUTOMATIC:
      calculated_adler32           = sctp_adler32(tvb, captured_length);
      adler32_correct              = (checksum == calculated_adler32);
      calculated_crc32c            = sctp_crc32c(tvb, captured_length);
      crc32c_correct               = (checksum == calculated_crc32c);
      sctp_info.adler32_calculated = TRUE;
      sctp_info.adler32_correct    = adler32_correct;
      sctp_info.crc32c_calculated  = TRUE;
      sctp_info.crc32c_correct     = crc32c_correct;
      break;
    }
  }

  source_port      = tvb_get_ntohs(tvb, SOURCE_PORT_OFFSET);
  destination_port = tvb_get_ntohs(tvb, DESTINATION_PORT_OFFSET);
  vtag             = tvb_get_ntohl(tvb,VERIFICATION_TAG_OFFSET);

  ha = get_half_assoc(pinfo, source_port, destination_port, vtag);

  /* In the interest of speed, if "tree" is NULL, don't do any work not
     necessary to generate protocol tree items. */
  if (tree) {

    /* create the sctp protocol tree */
    if (show_port_numbers)
      sctp_item = proto_tree_add_protocol_format(tree, proto_sctp, tvb, 0, -1,
                                                 "Stream Control Transmission Protocol, Src Port: %s (%u), Dst Port: %s (%u)",
                                                 sctp_port_to_display(wmem_packet_scope(), source_port), source_port,
                                                 sctp_port_to_display(wmem_packet_scope(), destination_port), destination_port);
    else
      sctp_item = proto_tree_add_item(tree, proto_sctp, tvb, 0, -1, ENC_NA);

    sctp_tree = proto_item_add_subtree(sctp_item, ett_sctp);

    /* add the components of the common header to the protocol tree */
    proto_tree_add_item(sctp_tree, hf_source_port,      tvb, SOURCE_PORT_OFFSET,      SOURCE_PORT_LENGTH,      ENC_BIG_ENDIAN);
    proto_tree_add_item(sctp_tree, hf_destination_port, tvb, DESTINATION_PORT_OFFSET, DESTINATION_PORT_LENGTH, ENC_BIG_ENDIAN);
    vt = proto_tree_add_item(sctp_tree, hf_verification_tag, tvb, VERIFICATION_TAG_OFFSET, VERIFICATION_TAG_LENGTH, ENC_BIG_ENDIAN);
    hidden_item = proto_tree_add_item(sctp_tree, hf_port, tvb, SOURCE_PORT_OFFSET,      SOURCE_PORT_LENGTH,      ENC_BIG_ENDIAN);
    PROTO_ITEM_SET_HIDDEN(hidden_item);
    hidden_item = proto_tree_add_item(sctp_tree, hf_port, tvb, DESTINATION_PORT_OFFSET, DESTINATION_PORT_LENGTH, ENC_BIG_ENDIAN);
    PROTO_ITEM_SET_HIDDEN(hidden_item);
  } else {
    sctp_tree = NULL;
    sctp_item = NULL;
  }

  if (captured_length == reported_length) {
    /* We have the whole packet */

    switch(sctp_checksum) {
    case SCTP_CHECKSUM_NONE:
      proto_tree_add_checksum(sctp_tree, tvb, CHECKSUM_OFFSET, hf_checksum, hf_checksum_status, &ei_sctp_bad_sctp_checksum, pinfo, 0,
                              ENC_BIG_ENDIAN, PROTO_CHECKSUM_NO_FLAGS);
      break;
    case SCTP_CHECKSUM_ADLER32:
      proto_tree_add_checksum(sctp_tree, tvb, CHECKSUM_OFFSET, hf_checksum_adler, hf_checksum_status, &ei_sctp_bad_sctp_checksum, pinfo, calculated_adler32,
                              ENC_BIG_ENDIAN, PROTO_CHECKSUM_VERIFY);
      break;
    case SCTP_CHECKSUM_CRC32C:
      proto_tree_add_checksum(sctp_tree, tvb, CHECKSUM_OFFSET, hf_checksum_crc32c, hf_checksum_status, &ei_sctp_bad_sctp_checksum, pinfo, calculated_crc32c,
                              ENC_BIG_ENDIAN, PROTO_CHECKSUM_VERIFY);
      break;
    case SCTP_CHECKSUM_AUTOMATIC:
      if ((adler32_correct) && !(crc32c_correct))
        proto_tree_add_checksum(sctp_tree, tvb, CHECKSUM_OFFSET, hf_checksum_adler, hf_checksum_status, &ei_sctp_bad_sctp_checksum, pinfo, calculated_adler32,
                              ENC_BIG_ENDIAN, PROTO_CHECKSUM_VERIFY);
      else if ((!adler32_correct) && (crc32c_correct))
        proto_tree_add_checksum(sctp_tree, tvb, CHECKSUM_OFFSET, hf_checksum_crc32c, hf_checksum_status, &ei_sctp_bad_sctp_checksum, pinfo, calculated_crc32c,
                              ENC_BIG_ENDIAN, PROTO_CHECKSUM_VERIFY);
      else {
        proto_tree_add_checksum(sctp_tree, tvb, CHECKSUM_OFFSET, hf_checksum_adler, hf_checksum_status, &ei_sctp_bad_sctp_checksum, pinfo, calculated_adler32,
                              ENC_BIG_ENDIAN, PROTO_CHECKSUM_VERIFY);
        proto_tree_add_checksum(sctp_tree, tvb, CHECKSUM_OFFSET, hf_checksum_crc32c, hf_checksum_status, &ei_sctp_bad_sctp_checksum, pinfo, calculated_crc32c,
                              ENC_BIG_ENDIAN, PROTO_CHECKSUM_VERIFY);
      }
      break;
    }
  } else {
    /* We don't have the whole packet so we can't verify the checksum */
      proto_tree_add_checksum(sctp_tree, tvb, CHECKSUM_OFFSET, hf_checksum, hf_checksum_status, &ei_sctp_bad_sctp_checksum, pinfo, 0,
                              ENC_BIG_ENDIAN, PROTO_CHECKSUM_NO_FLAGS);
  }

  /* add all chunks of the sctp datagram to the protocol tree */
  dissect_sctp_chunks(tvb, pinfo, tree, sctp_item, sctp_tree, ha, encapsulated);

  /* add assoc_index and move it behind the verification tag */
  pi = proto_tree_add_uint(sctp_tree, hf_sctp_assoc_index, tvb, 0 , 0, sctp_info.assoc_index);
  PROTO_ITEM_SET_GENERATED(pi);
  proto_tree_move_item(sctp_tree, vt, pi);
}

static gboolean
capture_sctp(const guchar *pd _U_, int offset _U_, int len _U_, capture_packet_info_t *cpinfo, const union wtap_pseudo_header *pseudo_header _U_)
{
  capture_dissector_increment_count(cpinfo, proto_sctp);
  return TRUE;
}

static int
dissect_sctp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
  guint16 source_port, destination_port;
  guint      number_of_ppid;

  /* Extract the common header */
  source_port      = tvb_get_ntohs(tvb, SOURCE_PORT_OFFSET);
  destination_port = tvb_get_ntohs(tvb, DESTINATION_PORT_OFFSET);

  /* update pi structure */
  pinfo->ptype    = PT_SCTP;
  pinfo->srcport  = source_port;
  pinfo->destport = destination_port;

  /* make entry in the Protocol column on summary display */
  col_set_str(pinfo->cinfo, COL_PROTOCOL, "SCTP");

  /* Clear entries in Info column on summary display */
  col_set_str(pinfo->cinfo, COL_INFO, "");


  for(number_of_ppid = 0; number_of_ppid < MAX_NUMBER_OF_PPIDS; number_of_ppid++) {
    p_add_proto_data(pinfo->pool, pinfo, proto_sctp, number_of_ppid, GUINT_TO_POINTER(LAST_PPID));
  }

  /*  The tvb array in struct _sctp_info is huge: currently 2k pointers.
   *  We know (by the value of 'number_of_tvbs') which of these entries have
   *  been used, so don't memset() the array.  This saves us from zeroing out
   *  8k (4-byte pointers) or 16k (8-byte pointers) of memory every time we
   *  dissect a packet (saving quite a bit of time!).
   */
  sctp_info.incomplete = 0;
  sctp_info.adler32_calculated = 0;
  sctp_info.adler32_correct = 0;
  sctp_info.crc32c_calculated = 0;
  sctp_info.crc32c_correct = 0;
  sctp_info.vtag_reflected = 0;
  sctp_info.number_of_tvbs = 0;
  sctp_info.verification_tag = tvb_get_ntohl(tvb, VERIFICATION_TAG_OFFSET);

  sctp_info.sport = pinfo->srcport;
  sctp_info.dport = pinfo->destport;
  set_address(&sctp_info.ip_src, pinfo->src.type, pinfo->src.len, pinfo->src.data);
  set_address(&sctp_info.ip_dst, pinfo->dst.type, pinfo->dst.len, pinfo->dst.data);

  dissect_sctp_packet(tvb, pinfo, tree, FALSE);
  if (!pinfo->flags.in_error_pkt && sctp_info.number_of_tvbs > 0)
    tap_queue_packet(sctp_tap, pinfo, &sctp_info);

  return tvb_captured_length(tvb);
}

/* Register the protocol with Wireshark */
void
proto_register_sctp(void)
{

  /* Setup list of header fields */
  static hf_register_info hf[] = {
    { &hf_source_port,                              { "Source port",                                    "sctp.srcport",                                         FT_UINT16,  BASE_PT_SCTP, NULL,                                        0x0,                                NULL, HFILL } },
    { &hf_destination_port,                         { "Destination port",                               "sctp.dstport",                                         FT_UINT16,  BASE_PT_SCTP, NULL,                                        0x0,                                NULL, HFILL } },
    { &hf_port,                                     { "Port",                                           "sctp.port",                                            FT_UINT16,  BASE_PT_SCTP, NULL,                                        0x0,                                NULL, HFILL } },
    { &hf_verification_tag,                         { "Verification tag",                               "sctp.verification_tag",                                FT_UINT32,  BASE_HEX,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_checksum,                                 { "Checksum",                                       "sctp.checksum",                                        FT_UINT32,  BASE_HEX,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_checksum_adler,                           { "Checksum (Adler)",                               "sctp.checksum",                                        FT_UINT32,  BASE_HEX,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_checksum_crc32c,                          { "Checksum (CRC32C)",                              "sctp.checksum",                                        FT_UINT32,  BASE_HEX,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_checksum_status,                          { "Checksum Status",                                "sctp.checksum.status",                                 FT_UINT8,   BASE_NONE, VALS(proto_checksum_vals),                      0x0,                                NULL, HFILL } },
    { &hf_chunk_type,                               { "Chunk type",                                     "sctp.chunk_type",                                      FT_UINT8,   BASE_DEC,  VALS(chunk_type_values),                        0x0,                                NULL, HFILL } },
    { &hf_chunk_flags,                              { "Chunk flags",                                    "sctp.chunk_flags",                                     FT_UINT8,   BASE_HEX,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_chunk_bit_1,                              { "Bit",                                            "sctp.chunk_bit_1",                                     FT_BOOLEAN, 8,         TFS(&sctp_chunk_bit_1_value),                   SCTP_CHUNK_BIT_1,                   NULL, HFILL } },
    { &hf_chunk_bit_2,                              { "Bit",                                            "sctp.chunk_bit_2",                                     FT_BOOLEAN, 8,         TFS(&sctp_chunk_bit_2_value),                   SCTP_CHUNK_BIT_2,                   NULL, HFILL } },
    { &hf_chunk_length,                             { "Chunk length",                                   "sctp.chunk_length",                                    FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_chunk_padding,                            { "Chunk padding",                                  "sctp.chunk_padding",                                   FT_BYTES,   BASE_NONE, NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_chunk_value,                              { "Chunk value",                                    "sctp.chunk_value",                                     FT_BYTES,   BASE_NONE, NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_cookie,                                   { "Cookie",                                         "sctp.cookie",                                          FT_BYTES,   BASE_NONE, NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_initiate_tag,                             { "Initiate tag",                                   "sctp.initiate_tag",                                    FT_UINT32,  BASE_HEX,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_init_chunk_initiate_tag,                  { "Initiate tag",                                   "sctp.init_initiate_tag",                               FT_UINT32,  BASE_HEX,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_init_chunk_adv_rec_window_credit,         { "Advertised receiver window credit (a_rwnd)",     "sctp.init_credit",                                     FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_init_chunk_number_of_outbound_streams,    { "Number of outbound streams",                     "sctp.init_nr_out_streams",                             FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_init_chunk_number_of_inbound_streams,     { "Number of inbound streams",                      "sctp.init_nr_in_streams",                              FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_init_chunk_initial_tsn,                   { "Initial TSN",                                    "sctp.init_initial_tsn",                                FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_initack_chunk_initiate_tag,               { "Initiate tag",                                   "sctp.initack_initiate_tag",                            FT_UINT32,  BASE_HEX,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_initack_chunk_adv_rec_window_credit,      { "Advertised receiver window credit (a_rwnd)",     "sctp.initack_credit",                                  FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_initack_chunk_number_of_outbound_streams, { "Number of outbound streams",                     "sctp.initack_nr_out_streams",                          FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_initack_chunk_number_of_inbound_streams,  { "Number of inbound streams",                      "sctp.initack_nr_in_streams",                           FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_initack_chunk_initial_tsn,                { "Initial TSN",                                    "sctp.initack_initial_tsn",                             FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
#if 0
    { &hf_cumulative_tsn_ack,                       { "Cumulative TSN Ack",                             "sctp.cumulative_tsn_ack",                              FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
#endif
    { &hf_data_chunk_tsn,                           { "Transmission sequence number",                   "sctp.data_tsn",                                        FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_data_chunk_stream_id,                     { "Stream identifier",                              "sctp.data_sid",                                        FT_UINT16,  BASE_HEX,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_data_chunk_stream_seq_number,             { "Stream sequence number",                         "sctp.data_ssn",                                        FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_data_chunk_payload_proto_id,              { "Payload protocol identifier",                    "sctp.data_payload_proto_id",                           FT_UINT32,  BASE_DEC,  VALS(sctp_payload_proto_id_values),             0x0,                                NULL, HFILL } },
    { &hf_idata_chunk_reserved,                     { "Reserved",                                       "sctp.data_reserved",                                   FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_idata_chunk_mid,                          { "Message identifier",                             "sctp.data_mid",                                        FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_idata_chunk_fsn,                          { "Fragment sequence number",                       "sctp.data_fsn",                                        FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_data_chunk_e_bit,                         { "E-Bit",                                          "sctp.data_e_bit",                                      FT_BOOLEAN, 8,         TFS(&sctp_data_chunk_e_bit_value),              SCTP_DATA_CHUNK_E_BIT,              NULL, HFILL } },
    { &hf_data_chunk_b_bit,                         { "B-Bit",                                          "sctp.data_b_bit",                                      FT_BOOLEAN, 8,         TFS(&sctp_data_chunk_b_bit_value),              SCTP_DATA_CHUNK_B_BIT,              NULL, HFILL } },
    { &hf_data_chunk_u_bit,                         { "U-Bit",                                          "sctp.data_u_bit",                                      FT_BOOLEAN, 8,         TFS(&sctp_data_chunk_u_bit_value),              SCTP_DATA_CHUNK_U_BIT,              NULL, HFILL } },
    { &hf_data_chunk_i_bit,                         { "I-Bit",                                          "sctp.data_i_bit",                                      FT_BOOLEAN, 8,         TFS(&sctp_data_chunk_i_bit_value),              SCTP_DATA_CHUNK_I_BIT,              NULL, HFILL } },
    { &hf_sack_chunk_ns,                            { "Nounce sum",                                     "sctp.sack_nounce_sum",                                 FT_UINT8,   BASE_DEC,  NULL,                                           SCTP_SACK_CHUNK_NS_BIT,             NULL, HFILL } },
    { &hf_sack_chunk_cumulative_tsn_ack,            { "Cumulative TSN ACK",                             "sctp.sack_cumulative_tsn_ack",                         FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_sack_chunk_adv_rec_window_credit,         { "Advertised receiver window credit (a_rwnd)",     "sctp.sack_a_rwnd",                                     FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_sack_chunk_number_of_gap_blocks,          { "Number of gap acknowledgement blocks",           "sctp.sack_number_of_gap_blocks",                       FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_sack_chunk_number_of_dup_tsns,            { "Number of duplicated TSNs",                      "sctp.sack_number_of_duplicated_tsns",                  FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_sack_chunk_gap_block_start,               { "Start",                                          "sctp.sack_gap_block_start",                            FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_sack_chunk_gap_block_start_tsn,           { "Start TSN",                                      "sctp.sack_gap_block_start_tsn",                        FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_sack_chunk_gap_block_end,                 { "End",                                            "sctp.sack_gap_block_end",                              FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_sack_chunk_gap_block_end_tsn,             { "End TSN",                                        "sctp.sack_gap_block_end_tsn",                          FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_sack_chunk_number_tsns_gap_acked,         { "Number of TSNs in gap acknowledgement blocks",   "sctp.sack_number_of_tsns_gap_acked",                   FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_sack_chunk_duplicate_tsn,                 { "Duplicate TSN",                                  "sctp.sack_duplicate_tsn",                              FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_nr_sack_chunk_ns,                         { "Nounce sum",                                     "sctp.nr_sack_nounce_sum",                              FT_UINT8,   BASE_DEC,  NULL,                                           SCTP_NR_SACK_CHUNK_NS_BIT,          NULL, HFILL } },
    { &hf_nr_sack_chunk_cumulative_tsn_ack,         { "Cumulative TSN ACK",                             "sctp.nr_sack_cumulative_tsn_ack",                      FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_nr_sack_chunk_adv_rec_window_credit,      { "Advertised receiver window credit (a_rwnd)",     "sctp.nr_sack_a_rwnd",                                  FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_nr_sack_chunk_number_of_gap_blocks,       { "Number of gap acknowledgement blocks",           "sctp.nr_sack_number_of_gap_blocks",                    FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_nr_sack_chunk_number_of_nr_gap_blocks,    { "Number of nr-gap acknowledgement blocks",        "sctp.nr_sack_number_of_nr_gap_blocks",                 FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_nr_sack_chunk_number_of_dup_tsns,         { "Number of duplicated TSNs",                      "sctp.nr_sack_number_of_duplicated_tsns",               FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_nr_sack_chunk_reserved,                   { "Reserved",                                       "sctp.nr_sack_reserved",                                FT_UINT16,  BASE_HEX,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_nr_sack_chunk_gap_block_start,            { "Start",                                          "sctp.nr_sack_gap_block_start",                         FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_nr_sack_chunk_gap_block_start_tsn,        { "Start TSN",                                      "sctp.nr_sack_gap_block_start_tsn",                     FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_nr_sack_chunk_gap_block_end,              { "End",                                            "sctp.nr_sack_gap_block_end",                           FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_nr_sack_chunk_gap_block_end_tsn,          { "End TSN",                                        "sctp.nr_sack_gap_block_end_tsn",                       FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_nr_sack_chunk_number_tsns_gap_acked,      { "Number of TSNs in gap acknowledgement blocks",   "sctp.nr_sack_number_of_tsns_gap_acked",                FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_nr_sack_chunk_nr_gap_block_start,         { "Start",                                          "sctp.nr_sack_nr_gap_block_start",                      FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_nr_sack_chunk_nr_gap_block_start_tsn,     { "Start TSN",                                      "sctp.nr_sack_nr_gap_block_start_tsn",                  FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_nr_sack_chunk_nr_gap_block_end,           { "End",                                            "sctp.nr_sack_nr_gap_block_end",                        FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_nr_sack_chunk_nr_gap_block_end_tsn,       { "End TSN",                                        "sctp.nr_sack_nr_gap_block_end_tsn",                    FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_nr_sack_chunk_number_tsns_nr_gap_acked,   { "Number of TSNs in nr-gap acknowledgement blocks","sctp.nr_sack_number_of_tsns_nr_gap_acked",             FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
#if 0
    { &hf_nr_sack_chunk_duplicate_tsn,              { "Duplicate TSN",                                  "sctp.nr_sack_duplicate_tsn",                           FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
#endif
    { &hf_shutdown_chunk_cumulative_tsn_ack,        { "Cumulative TSN Ack",                             "sctp.shutdown_cumulative_tsn_ack",                     FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_ecne_chunk_lowest_tsn,                    { "Lowest TSN",                                     "sctp.ecne_lowest_tsn",                                 FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_cwr_chunk_lowest_tsn,                     { "Lowest TSN",                                     "sctp.cwr_lowest_tsn",                                  FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_shutdown_complete_chunk_t_bit,            { "T-Bit",                                          "sctp.shutdown_complete_t_bit",                         FT_BOOLEAN, 8,         TFS(&sctp_shutdown_complete_chunk_t_bit_value), SCTP_SHUTDOWN_COMPLETE_CHUNK_T_BIT, NULL, HFILL } },
    { &hf_abort_chunk_t_bit,                        { "T-Bit",                                          "sctp.abort_t_bit",                                     FT_BOOLEAN, 8,         TFS(&sctp_shutdown_complete_chunk_t_bit_value), SCTP_ABORT_CHUNK_T_BIT,             NULL, HFILL } },
    { &hf_forward_tsn_chunk_tsn,                    { "New cumulative TSN",                             "sctp.forward_tsn_tsn",                                 FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_forward_tsn_chunk_sid,                    { "Stream identifier",                              "sctp.forward_tsn_sid",                                 FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_forward_tsn_chunk_ssn,                    { "Stream sequence number",                         "sctp.forward_tsn_ssn",                                 FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_i_forward_tsn_chunk_tsn,                  { "New cumulative TSN",                             "sctp.i_forward_tsn_tsn",                               FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_i_forward_tsn_chunk_sid,                  { "Stream identifier",                              "sctp.i_forward_tsn_sid",                               FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_i_forward_tsn_chunk_flags,                { "Flags",                                          "sctp.i_forward_tsn_flags",                             FT_UINT16,  BASE_HEX,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_i_forward_tsn_chunk_res,                  { "Reserved",                                       "sctp.i_forward_tsn_res",                               FT_UINT16,  BASE_DEC,  NULL,                                           SCTP_I_FORWARD_TSN_CHUNK_RES_MASK,  NULL, HFILL } },
    { &hf_i_forward_tsn_chunk_u_bit,                { "U-Bit",                                          "sctp.i_forward_tsn_u_bit",                             FT_BOOLEAN, 16,        TFS(&sctp_i_forward_tsn_chunk_u_bit_value),     SCTP_I_FORWARD_TSN_CHUNK_U_BIT,     NULL, HFILL } },
    { &hf_i_forward_tsn_chunk_mid,                  { "Message identifier",                             "sctp.forward_tsn_mid",                                 FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_parameter_type,                           { "Parameter type",                                 "sctp.parameter_type",                                  FT_UINT16,  BASE_HEX,  VALS(parameter_identifier_values),              0x0,                                NULL, HFILL } },
    { &hf_parameter_length,                         { "Parameter length",                               "sctp.parameter_length",                                FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_parameter_value,                          { "Parameter value",                                "sctp.parameter_value",                                 FT_BYTES,   BASE_NONE, NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_parameter_padding,                        { "Parameter padding",                              "sctp.parameter_padding",                               FT_BYTES,   BASE_NONE, NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_parameter_bit_1,                          { "Bit",                                            "sctp.parameter_bit_1",                                 FT_BOOLEAN, 16,        TFS(&sctp_parameter_bit_1_value),               SCTP_PARAMETER_BIT_1,               NULL, HFILL } },
    { &hf_parameter_bit_2,                          { "Bit",                                            "sctp.parameter_bit_2",                                 FT_BOOLEAN, 16,        TFS(&sctp_parameter_bit_2_value),               SCTP_PARAMETER_BIT_2,               NULL, HFILL } },
    { &hf_ipv4_address,                             { "IP Version 4 address",                           "sctp.parameter_ipv4_address",                          FT_IPv4,    BASE_NONE, NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_ipv6_address,                             { "IP Version 6 address",                           "sctp.parameter_ipv6_address",                          FT_IPv6,    BASE_NONE, NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_heartbeat_info,                           { "Heartbeat information",                          "sctp.parameter_heartbeat_information",                 FT_BYTES,   BASE_NONE, NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_state_cookie,                             { "State cookie",                                   "sctp.parameter_state_cookie",                          FT_BYTES,   BASE_NONE, NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_cookie_preservative_increment,            { "Suggested Cookie life-span increment (msec)",    "sctp.parameter_cookie_preservative_incr",              FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_hostname,                                 { "Hostname",                                       "sctp.parameter_hostname",                              FT_STRING,  BASE_NONE, NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_supported_address_type,                   { "Supported address type",                         "sctp.parameter_supported_addres_type",                 FT_UINT16,  BASE_DEC,  VALS(address_types_values),                     0x0,                                NULL, HFILL } },
    { &hf_stream_reset_req_seq_nr,                  { "Re-configuration request sequence number",       "sctp.parameter_reconfig_request_sequence_number",      FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_stream_reset_rsp_seq_nr,                  { "Re-configuration response sequence number",      "sctp.parameter_reconfig_response_sequence_number",     FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_senders_last_assigned_tsn,                { "Senders last assigned TSN",                      "sctp.parameter_senders_last_assigned_tsn",             FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_senders_next_tsn,                         { "Senders next TSN",                               "sctp.parameter_senders_next_tsn",                      FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_receivers_next_tsn,                       { "Receivers next TSN",                             "sctp.parameter_receivers_next_tsn",                    FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_stream_reset_rsp_result,                  { "Result",                                         "sctp.parameter_reconfig_response_result",              FT_UINT32,  BASE_DEC,  VALS(stream_reset_result_values),               0x0,                                NULL, HFILL } },
    { &hf_stream_reset_sid,                         { "Stream Identifier",                              "sctp.parameter_reconfig_sid",                          FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_add_outgoing_streams_number_streams,      { "Number of streams",                              "sctp.parameter_add_outgoing_streams_number",           FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_add_outgoing_streams_reserved,            { "Reserved",                                       "sctp.parameter_add_outgoing_streams_reserved",         FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_add_incoming_streams_number_streams,      { "Number of streams",                              "sctp.parameter_add_incoming_streams_number",           FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_add_incoming_streams_reserved,            { "Reserved",                                       "sctp.parameter_add_incoming_streams_reserved",         FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_asconf_seq_nr,                            { "Sequence number",                                "sctp.asconf_seq_nr_number",                            FT_UINT32,  BASE_HEX,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_asconf_ack_seq_nr,                        { "Sequence number",                                "sctp.asconf_ack_seq_nr_number",                        FT_UINT32,  BASE_HEX,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_correlation_id,                           { "Correlation_id",                                 "sctp.correlation_id",                                  FT_UINT32,  BASE_HEX,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_adap_indication,                          { "Indication",                                     "sctp.adapation_layer_indication",                      FT_UINT32,  BASE_HEX,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_random_number,                            { "Random number",                                  "sctp.random_number",                                   FT_BYTES,   BASE_NONE, NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_chunks_to_auth,                           { "Chunk type",                                     "sctp.chunk_type_to_auth",                              FT_UINT8,   BASE_DEC,  VALS(chunk_type_values),                        0x0,                                NULL, HFILL } },
    { &hf_hmac_id,                                  { "HMAC identifier",                                "sctp.hmac_id",                                         FT_UINT16,  BASE_DEC,  VALS(hmac_id_values),                           0x0,                                NULL, HFILL } },
    { &hf_hmac,                                     { "HMAC",                                           "sctp.hmac",                                            FT_BYTES,   BASE_NONE, NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_shared_key_id,                            { "Shared key identifier",                          "sctp.shared_key_id",                                   FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_supported_chunk_type,                     { "Supported chunk type",                           "sctp.supported_chunk_type",                            FT_UINT8,   BASE_DEC,  VALS(chunk_type_values),                        0x0,                                NULL, HFILL } },
    { &hf_cause_code,                               { "Cause code",                                     "sctp.cause_code",                                      FT_UINT16,  BASE_HEX,  VALS(cause_code_values),                        0x0,                                NULL, HFILL } },
    { &hf_cause_length,                             { "Cause length",                                   "sctp.cause_length",                                    FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_cause_info,                               { "Cause information",                              "sctp.cause_information",                               FT_BYTES,   BASE_NONE, NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_cause_padding,                            { "Cause padding",                                  "sctp.cause_padding",                                   FT_BYTES,   BASE_NONE, NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_cause_stream_identifier,                  { "Stream identifier",                              "sctp.cause_stream_identifier",                         FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_cause_reserved,                           { "Reserved",                                       "sctp.cause_reserved",                                  FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_cause_number_of_missing_parameters,       { "Number of missing parameters",                   "sctp.cause_nr_of_missing_parameters",                  FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_cause_missing_parameter_type,             { "Missing parameter type",                         "sctp.cause_missing_parameter_type",                    FT_UINT16,  BASE_HEX,  VALS(parameter_identifier_values),              0x0,                                NULL, HFILL } },
    { &hf_cause_measure_of_staleness,               { "Measure of staleness in usec",                   "sctp.cause_measure_of_staleness",                      FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_cause_tsn,                                { "TSN",                                            "sctp.cause_tsn",                                       FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_pktdrop_chunk_m_bit,                      { "M-Bit",                                          "sctp.pckdrop_m_bit",                                   FT_BOOLEAN, 8,         TFS(&sctp_pktdropk_m_bit_value),                SCTP_PKTDROP_CHUNK_M_BIT,           NULL, HFILL } },
    { &hf_pktdrop_chunk_b_bit,                      { "B-Bit",                                          "sctp.pckdrop_b_bit",                                   FT_BOOLEAN, 8,         TFS(&sctp_pktdropk_b_bit_value),                SCTP_PKTDROP_CHUNK_B_BIT,           NULL, HFILL } },
    { &hf_pktdrop_chunk_t_bit,                      { "T-Bit",                                          "sctp.pckdrop_t_bit",                                   FT_BOOLEAN, 8,         TFS(&sctp_pktdropk_t_bit_value),                SCTP_PKTDROP_CHUNK_T_BIT,           NULL, HFILL } },
    { &hf_pktdrop_chunk_bandwidth,                  { "Bandwidth",                                      "sctp.pktdrop_bandwidth",                               FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_pktdrop_chunk_queuesize,                  { "Queuesize",                                      "sctp.pktdrop_queuesize",                               FT_UINT32,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_pktdrop_chunk_truncated_length,           { "Truncated length",                               "sctp.pktdrop_truncated_length",                        FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_pktdrop_chunk_reserved,                   { "Reserved",                                       "sctp.pktdrop_reserved",                                FT_UINT16,  BASE_DEC,  NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_pktdrop_chunk_data_field,                 { "Data field",                                     "sctp.pktdrop_datafield",                               FT_BYTES,   BASE_NONE, NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_pad_chunk_padding_data,                   { "Padding data",                                   "sctp.padding_data",                                    FT_BYTES,   BASE_NONE, NULL,                                           0x0,                                NULL, HFILL } },

    { &hf_sctp_fragment,                            { "SCTP Fragment",                                  "sctp.fragment",                                        FT_FRAMENUM, BASE_NONE, NULL,                                          0x0,                                NULL, HFILL } },
    { &hf_sctp_fragments,                           { "Reassembled SCTP Fragments",                     "sctp.fragments",                                       FT_NONE,    BASE_NONE, NULL,                                           0x0,                                NULL, HFILL } },
    { &hf_sctp_reassembled_in,                      { "Reassembled Message in frame",                   "sctp.reassembled_in",                                  FT_FRAMENUM, BASE_NONE, NULL,                                          0x0,                                NULL, HFILL } },
    { &hf_sctp_duplicate,                           { "Fragment already seen in frame",                 "sctp.duplicate",                                       FT_FRAMENUM, BASE_NONE, NULL,                                          0x0,                                NULL, HFILL } },

    { &hf_sctp_data_rtt,                            { "The RTT to SACK was",                            "sctp.data_rtt",                                        FT_RELATIVE_TIME, BASE_NONE, NULL,                                     0x0,                                NULL, HFILL } },
    { &hf_sctp_sack_rtt,                            { "The RTT since DATA was",                         "sctp.sack_rtt",                                        FT_RELATIVE_TIME, BASE_NONE, NULL,                                     0x0,                                NULL, HFILL } },
    { &hf_sctp_rto,                                 { "Retransmitted after",                            "sctp.retransmission_time",                             FT_RELATIVE_TIME, BASE_NONE, NULL,                                     0x0,                                NULL, HFILL } },
    { &hf_sctp_retransmission,                      { "This TSN is a retransmission of one in frame",   "sctp.retransmission",                                  FT_FRAMENUM, BASE_NONE, NULL,                                          0x0,                                NULL, HFILL } },
    { &hf_sctp_retransmitted,                       { "This TSN is retransmitted in frame",             "sctp.retransmitted",                                   FT_FRAMENUM, BASE_NONE, NULL,                                          0x0,                                NULL, HFILL } },
    { &hf_sctp_retransmitted_count,                 { "TSN was retransmitted this many times",          "sctp.retransmitted_count",                             FT_UINT32, BASE_DEC, NULL,                                             0x0,                                NULL, HFILL } },
    { &hf_sctp_acked,                               { "This chunk is acked in frame",                   "sctp.acked",                                           FT_FRAMENUM, BASE_NONE, NULL,                                          0x0,                                NULL, HFILL } },
    { &hf_sctp_ack_tsn,                             { "Acknowledges TSN",                               "sctp.ack",                                             FT_UINT32, BASE_DEC, NULL,                                             0x0,                                NULL, HFILL } },
    { &hf_sctp_ack_frame,                           { "Acknowledges TSN in frame",                      "sctp.ack_frame",                                       FT_FRAMENUM, BASE_NONE, FRAMENUM_TYPE(FT_FRAMENUM_ACK),                0x0,                                NULL, HFILL } },
    { &hf_sctp_retransmitted_after_ack,             { "Chunk was acked prior to retransmission",        "sctp.retransmitted_after_ack",                         FT_FRAMENUM, BASE_NONE, NULL,                                          0x0,                                NULL, HFILL } },
    { &hf_sctp_assoc_index,                         { "Association index",                              "sctp.assoc_index",                                     FT_UINT16, BASE_DEC, NULL,                                             0x0,                                NULL, HFILL } }

 };

  /* Setup protocol subtree array */
  static gint *ett[] = {
    &ett_sctp,
    &ett_sctp_chunk,
    &ett_sctp_chunk_parameter,
    &ett_sctp_chunk_cause,
    &ett_sctp_chunk_type,
    &ett_sctp_data_chunk_flags,
    &ett_sctp_sack_chunk_flags,
    &ett_sctp_nr_sack_chunk_flags,
    &ett_sctp_abort_chunk_flags,
    &ett_sctp_shutdown_complete_chunk_flags,
    &ett_sctp_pktdrop_chunk_flags,
    &ett_sctp_parameter_type,
    &ett_sctp_sack_chunk_gap_block,
    &ett_sctp_sack_chunk_gap_block_start,
    &ett_sctp_sack_chunk_gap_block_end,
    &ett_sctp_nr_sack_chunk_gap_block,
    &ett_sctp_nr_sack_chunk_gap_block_start,
    &ett_sctp_nr_sack_chunk_gap_block_end,
    &ett_sctp_nr_sack_chunk_nr_gap_block,
    &ett_sctp_nr_sack_chunk_nr_gap_block_start,
    &ett_sctp_nr_sack_chunk_nr_gap_block_end,
    &ett_sctp_unrecognized_parameter_parameter,
    &ett_sctp_i_forward_tsn_chunk_flags,
    &ett_sctp_fragments,
    &ett_sctp_fragment,
    &ett_sctp_ack,
    &ett_sctp_acked,
    &ett_sctp_tsn,
    &ett_sctp_tsn_retransmission,
    &ett_sctp_tsn_retransmitted_count,
    &ett_sctp_tsn_retransmitted
  };

  static ei_register_info ei[] = {
      { &ei_sctp_tsn_retransmitted, { "sctp.retransmission.expert", PI_SEQUENCE, PI_NOTE, "Retransmitted TSN", EXPFILL }},
      { &ei_sctp_retransmitted_after_ack, { "sctp.retransmitted_after_ack.expert", PI_SEQUENCE, PI_WARN, "This TSN was acked prior to this retransmission (reneged ack?).", EXPFILL }},
      { &ei_sctp_tsn_retransmitted_more_than_twice, { "sctp.retransmission.more_than_twice", PI_SEQUENCE, PI_WARN, "This TSN was retransmitted more than 2 times.", EXPFILL }},
      { &ei_sctp_parameter_padding, { "sctp.parameter_padding.expert", PI_MALFORMED, PI_NOTE, "The padding of this final parameter should be the padding of the chunk.", EXPFILL }},
      { &ei_sctp_parameter_length, { "sctp.parameter_length.bad", PI_MALFORMED, PI_ERROR, "Parameter length bad", EXPFILL }},
      { &ei_sctp_sack_chunk_adv_rec_window_credit, { "sctp.sack_a_rwnd.expert", PI_SEQUENCE, PI_NOTE, "Zero Advertised Receiver Window Credit", EXPFILL }},
      { &ei_sctp_sack_chunk_gap_block_malformed, { "sctp.sack_gap_block_malformed", PI_PROTOCOL, PI_ERROR, "Malformed gap block.", EXPFILL }},
      { &ei_sctp_sack_chunk_gap_block_out_of_order, { "sctp.sack_gap_block_out_of_order", PI_PROTOCOL, PI_WARN, "Gap blocks not in strict order.", EXPFILL }},
      { &ei_sctp_sack_chunk_number_tsns_gap_acked_100, { "sctp.sack_number_of_tsns_gap_acked.100", PI_SEQUENCE, PI_WARN, "More than 100 TSNs were gap-acknowledged in this SACK.", EXPFILL }},
      { &ei_sctp_nr_sack_chunk_number_tsns_gap_acked_100, { "sctp.nr_sack_number_of_tsns_gap_acked.100", PI_SEQUENCE, PI_WARN, "More than 100 TSNs were gap-acknowledged in this NR-SACK.", EXPFILL }},
      { &ei_sctp_nr_sack_chunk_number_tsns_nr_gap_acked_100, { "sctp.nr_sack_number_of_tsns_nr_gap_acked.100", PI_SEQUENCE, PI_WARN, "More than 100 TSNs were nr-gap-acknowledged in this NR-SACK.", EXPFILL }},
      { &ei_sctp_chunk_length_bad, { "sctp.chunk_length.bad", PI_MALFORMED, PI_ERROR, "Chunk length bad", EXPFILL }},
      { &ei_sctp_bad_sctp_checksum, { "sctp.checksum_bad.expert", PI_CHECKSUM, PI_ERROR, "Bad SCTP checksum.", EXPFILL }},
  };

  static const enum_val_t sctp_checksum_options[] = {
    { "none",      "None",        SCTP_CHECKSUM_NONE },
    { "adler-32",  "Adler 32",    SCTP_CHECKSUM_ADLER32 },
    { "crc-32c",   "CRC 32c",     SCTP_CHECKSUM_CRC32C },
    { "automatic", "Automatic",   SCTP_CHECKSUM_AUTOMATIC},
    { NULL, NULL, 0 }
  };

  /* Decode As handling */
  static build_valid_func sctp_da_src_values[1] = {sctp_src_value};
  static build_valid_func sctp_da_dst_values[1] = {sctp_dst_value};
  static build_valid_func sctp_da_both_values[2] = {sctp_src_value, sctp_dst_value};
  static decode_as_value_t sctp_da_port_values[3] = {{sctp_src_prompt, 1, sctp_da_src_values}, {sctp_dst_prompt, 1, sctp_da_dst_values}, {sctp_both_prompt, 2, sctp_da_both_values}};
  static decode_as_t sctp_da_port = {"sctp", "Transport", "sctp.port", 3, 2, sctp_da_port_values, "SCTP", "port(s) as",
                                     decode_as_default_populate_list, decode_as_default_reset, decode_as_default_change, NULL};

  static build_valid_func sctp_da_ppi_build_value1[1] = {sctp_ppi_value1};
  static build_valid_func sctp_da_ppi_build_value2[1] = {sctp_ppi_value2};
  static decode_as_value_t sctp_da_ppi_values[2] = {{sctp_ppi_prompt1, 1, sctp_da_ppi_build_value1}, {sctp_ppi_prompt2, 1, sctp_da_ppi_build_value2}};
  static decode_as_t sctp_da_ppi = {"sctp", "SCTP(PPID)", "sctp.ppi", 2, 0, sctp_da_ppi_values, "SCTP", NULL,
                                    decode_as_default_populate_list, decode_as_default_reset, decode_as_default_change, NULL};

  /* UAT for header fields */
  static uat_field_t custom_types_uat_fields[] = {
    UAT_FLD_NONE(type_fields, type_id, "Chunk ID", "IANA chunk type ID"),
    UAT_FLD_CSTRING(type_fields, type_name, "Type name", "Chunk Type name"),
    UAT_FLD_VS(type_fields, type_enable, "Visibility", chunk_enabled, "Hide or show the type in the chunk statistics"),
    UAT_END_FIELDS
  };

  module_t *sctp_module;
  expert_module_t* expert_sctp;
  uat_t* chunk_types_uat;

  chunk_types_uat = uat_new("Chunk types for the statistics dialog",
                            sizeof(type_field_t),
                            "statistics_chunk_types",
                            TRUE,
                            &type_fields,
                            &num_type_fields,
                            0,
                            NULL,
                            sctp_chunk_type_copy_cb,
                            sctp_chunk_type_update_cb,
                            sctp_chunk_type_free_cb,
                            NULL,
                            custom_types_uat_fields
);

  /* Register the protocol name and description */
  proto_sctp = proto_register_protocol("Stream Control Transmission Protocol", "SCTP", "sctp");
  sctp_module = prefs_register_protocol(proto_sctp, NULL);
  prefs_register_bool_preference(sctp_module, "show_port_numbers_in_tree",
                         "Show port numbers in the protocol tree",
                         "Show source and destination port numbers in the protocol tree",
                         &show_port_numbers);
  /* FIXME
  prefs_register_bool_preference(sctp_module, "show_chunk_types_in_tree",
                         "Show chunk types in the protocol tree",
                         "Show chunk types in the protocol tree",
                         &show_chunk_types);
  */
  prefs_register_enum_preference(sctp_module, "checksum", "Checksum type",
                         "The type of checksum used in SCTP packets",
                         &sctp_checksum, sctp_checksum_options, FALSE);
  prefs_register_bool_preference(sctp_module, "show_always_control_chunks",
                         "Show always control chunks",
                         "Show always SCTP control chunks in the Info column",
                         &show_always_control_chunks);
  prefs_register_bool_preference(sctp_module, "try_heuristic_first",
                         "Try heuristic sub-dissectors first",
                         "Try to decode a packet using an heuristic sub-dissector before using a sub-dissector registered to a specific port or PPI",
                         &try_heuristic_first);
  prefs_register_bool_preference(sctp_module, "reassembly",
                         "Reassemble fragmented SCTP user messages",
                         "Whether fragmented SCTP user messages should be reassembled",
                         &use_reassembly);
  prefs_register_bool_preference(sctp_module, "tsn_analysis",
                         "Enable TSN analysis",
                         "Match TSNs and their SACKs",
                         &enable_tsn_analysis);
  prefs_register_bool_preference(sctp_module, "ulp_dissection",
                         "Dissect upper layer protocols",
                         "Dissect upper layer protocols",
                         &enable_ulp_dissection);
  prefs_register_uat_preference_qt(sctp_module, "statistics_chunk_types",
                         "Select the chunk types for the statistics dialog",
                         "Select the chunk types for the statistics dialog",
                         chunk_types_uat);

  /* Required function calls to register the header fields and subtrees used */
  proto_register_field_array(proto_sctp, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));
  expert_sctp = expert_register_protocol(proto_sctp);
  expert_register_field_array(expert_sctp, ei, array_length(ei));

  sctp_tap = register_tap("sctp");
  exported_pdu_tap = find_tap_id(EXPORT_PDU_TAP_NAME_LAYER_3);
  /* subdissector code */
  sctp_port_dissector_table = register_dissector_table("sctp.port", "SCTP port", proto_sctp, FT_UINT16, BASE_DEC);
  sctp_ppi_dissector_table  = register_dissector_table("sctp.ppi",  "SCTP payload protocol identifier", proto_sctp, FT_UINT32, BASE_HEX);

  register_dissector("sctp", dissect_sctp, proto_sctp);
  sctp_heur_subdissector_list = register_heur_dissector_list("sctp", proto_sctp);

  register_init_routine(sctp_init);
  register_cleanup_routine(sctp_cleanup);

  dirs_by_ptvtag = wmem_tree_new_autoreset(wmem_epan_scope(), wmem_file_scope());
  dirs_by_ptaddr = wmem_tree_new_autoreset(wmem_epan_scope(), wmem_file_scope());

  register_decode_as(&sctp_da_port);
  register_decode_as(&sctp_da_ppi);

  register_conversation_table(proto_sctp, FALSE, sctp_conversation_packet, sctp_hostlist_packet);
}

void
proto_reg_handoff_sctp(void)
{
  dissector_handle_t sctp_handle;

  sctp_handle = find_dissector("sctp");
  dissector_add_uint("wtap_encap", WTAP_ENCAP_SCTP, sctp_handle);
  dissector_add_uint("ip.proto", IP_PROTO_SCTP, sctp_handle);
  dissector_add_uint("udp.port", UDP_TUNNELING_PORT, sctp_handle);
  register_capture_dissector("ip.proto", IP_PROTO_SCTP, capture_sctp, proto_sctp);
  register_capture_dissector("ipv6.nxt", IP_PROTO_SCTP, capture_sctp, proto_sctp);
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
