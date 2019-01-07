/* packet-umts_fp.c
 * Routines for UMTS FP disassembly
 *
 * Martin Mathieson
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
#include <epan/prefs.h>
#include <epan/uat.h>
#include <epan/conversation.h>
#include <epan/addr_resolv.h>
#include <epan/proto_data.h>

#include <wsutil/crc7.h> /* For FP data header and control frame CRC. */
#include <wsutil/crc16-plain.h> /* For FP Payload CRC. */
#include <wsutil/crc11.h> /* For FP EDCH header CRC. */

#include "packet-umts_fp.h"
#include "packet-nbap.h"
#include "packet-rrc.h"

/* The Frame Protocol (FP) is described in:
 * 3GPP TS 25.427 (for dedicated channels)
 * 3GPP TS 25.435 (for common/shared channels)
 *
 * TODO:
 *  - IUR interface-specific formats
 *  - do CRC verification before further parsing
 *    - Set the logical channel properly for non multiplexed, channels
 *     for channels that doesn't have the C/T flag! This should be based
 * on the RRC message RadioBearerSetup.
 */
void proto_register_fp(void);
void proto_reg_handoff_fp(void);


#define UMTS_FP_IPV4 1
#define UMTS_FP_IPV6 2

/* Initialize the protocol and registered fields. */

int proto_fp = -1;
extern int proto_umts_mac;
extern int proto_rlc;

static int hf_fp_release = -1;
static int hf_fp_release_version = -1;
static int hf_fp_release_year = -1;
static int hf_fp_release_month = -1;
static int hf_fp_channel_type = -1;
static int hf_fp_division = -1;
static int hf_fp_direction = -1;
static int hf_fp_ddi_config = -1;
static int hf_fp_ddi_config_ddi = -1;
static int hf_fp_ddi_config_macd_pdu_size = -1;

static int hf_fp_header_crc = -1;
static int hf_fp_ft = -1;
static int hf_fp_cfn = -1;
static int hf_fp_pch_cfn = -1;
static int hf_fp_pch_toa = -1;
static int hf_fp_cfn_control = -1;
static int hf_fp_toa = -1;
static int hf_fp_tfi = -1;
static int hf_fp_usch_tfi = -1;
static int hf_fp_cpch_tfi = -1;
static int hf_fp_propagation_delay = -1;
static int hf_fp_tb = -1;
static int hf_fp_chan_zero_tbs = -1;
static int hf_fp_received_sync_ul_timing_deviation = -1;
static int hf_fp_pch_pi = -1;
static int hf_fp_pch_tfi = -1;
static int hf_fp_fach_tfi = -1;
static int hf_fp_transmit_power_level = -1;
static int hf_fp_paging_indication_bitmap = -1;
static int hf_fp_pdsch_set_id = -1;
static int hf_fp_rx_timing_deviation = -1;
static int hf_fp_dch_e_rucch_flag = -1;
static int hf_fp_dch_control_frame_type = -1;
static int hf_fp_dch_rx_timing_deviation = -1;
static int hf_fp_quality_estimate = -1;
static int hf_fp_payload_crc = -1;
static int hf_fp_edch_header_crc = -1;
static int hf_fp_edch_fsn = -1;
static int hf_fp_edch_subframe = -1;
static int hf_fp_edch_number_of_subframes = -1;
static int hf_fp_edch_harq_retransmissions = -1;
static int hf_fp_edch_subframe_number = -1;
static int hf_fp_edch_number_of_mac_es_pdus = -1;
static int hf_fp_edch_ddi = -1;
static int hf_fp_edch_subframe_header = -1;
static int hf_fp_edch_number_of_mac_d_pdus = -1;
static int hf_fp_edch_pdu_padding = -1;
static int hf_fp_edch_tsn = -1;
static int hf_fp_edch_mac_es_pdu = -1;

static int hf_fp_edch_user_buffer_size = -1;
static int hf_fp_edch_no_macid_sdus = -1;
static int hf_fp_edch_number_of_mac_is_pdus = -1;
static int hf_fp_edch_mac_is_pdu = -1;

static int hf_fp_edch_e_rnti = -1;
static int hf_fp_edch_macis_descriptors = -1;
static int hf_fp_edch_macis_lchid = -1;
static int hf_fp_edch_macis_length = -1;
static int hf_fp_edch_macis_flag = -1;

static int hf_fp_frame_seq_nr = -1;
static int hf_fp_hsdsch_pdu_block_header = -1;
/* static int hf_fp_hsdsch_pdu_block = -1; */
static int hf_fp_flush = -1;
static int hf_fp_fsn_drt_reset = -1;
static int hf_fp_drt_indicator = -1;
static int hf_fp_fach_indicator = -1;
static int hf_fp_total_pdu_blocks = -1;
static int hf_fp_drt = -1;
static int hf_fp_hrnti = -1;
static int hf_fp_rach_measurement_result = -1;
static int hf_fp_lchid = -1;
static int hf_fp_pdu_length_in_block = -1;
static int hf_fp_pdus_in_block = -1;
static int hf_fp_cmch_pi = -1;
static int hf_fp_user_buffer_size = -1;
static int hf_fp_hsdsch_credits = -1;
static int hf_fp_hsdsch_max_macd_pdu_len = -1;
static int hf_fp_hsdsch_max_macdc_pdu_len = -1;
static int hf_fp_hsdsch_interval = -1;
static int hf_fp_hsdsch_calculated_rate = -1;
static int hf_fp_hsdsch_unlimited_rate = -1;
static int hf_fp_hsdsch_repetition_period = -1;
static int hf_fp_hsdsch_data_padding = -1;
static int hf_fp_hsdsch_new_ie_flags = -1;
static int hf_fp_hsdsch_new_ie_flag[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
static int hf_fp_hsdsch_drt = -1;
static int hf_fp_hsdsch_entity = -1;
static int hf_fp_hsdsch_physical_layer_category = -1;
static int hf_fp_timing_advance = -1;
static int hf_fp_num_of_pdu = -1;
static int hf_fp_mac_d_pdu_len = -1;
static int hf_fp_mac_d_pdu = -1;
static int hf_fp_data = -1;
static int hf_fp_crcis = -1;
static int hf_fp_crci[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
static int hf_fp_common_control_frame_type = -1;
static int hf_fp_t1 = -1;
static int hf_fp_t2 = -1;
static int hf_fp_t3 = -1;
static int hf_fp_ul_sir_target = -1;
static int hf_fp_pusch_set_id = -1;
static int hf_fp_activation_cfn = -1;
static int hf_fp_duration = -1;
static int hf_fp_power_offset = -1;
static int hf_fp_code_number = -1;
static int hf_fp_spreading_factor = -1;
static int hf_fp_mc_info = -1;

static int hf_fp_rach_new_ie_flags = -1;
static int hf_fp_rach_new_ie_flag_unused[7] = {-1, -1, -1, -1, -1, -1, -1 };
static int hf_fp_rach_ext_propagation_delay_present = -1;
static int hf_fp_rach_cell_portion_id_present = -1;
static int hf_fp_rach_angle_of_arrival_present = -1;
static int hf_fp_rach_ext_rx_sync_ul_timing_deviation_present = -1;
static int hf_fp_rach_ext_rx_timing_deviation_present = -1;

static int hf_fp_cell_portion_id = -1;
static int hf_fp_ext_propagation_delay = -1;
static int hf_fp_angle_of_arrival = -1;
static int hf_fp_ext_received_sync_ul_timing_deviation = -1;

static int hf_fp_radio_interface_parameter_update_flag[5] = {-1, -1, -1, -1, -1};
static int hf_fp_dpc_mode = -1;
static int hf_fp_tpc_po = -1;
static int hf_fp_multiple_rl_set_indicator = -1;
static int hf_fp_max_ue_tx_pow = -1;
static int hf_fp_congestion_status = -1;
static int hf_fp_e_rucch_present = -1;
static int hf_fp_extended_bits_present = -1;
static int hf_fp_extended_bits = -1;
static int hf_fp_spare_extension = -1;
static int hf_fp_ul_setup_frame = -1;
static int hf_fp_dl_setup_frame = -1;

/* Subtrees. */
static int ett_fp = -1;
static int ett_fp_release = -1;
static int ett_fp_data = -1;
static int ett_fp_crcis = -1;
static int ett_fp_ddi_config = -1;
static int ett_fp_edch_subframe_header = -1;
static int ett_fp_edch_subframe = -1;
static int ett_fp_edch_maces = -1;
static int ett_fp_edch_macis_descriptors = -1;
static int ett_fp_hsdsch_new_ie_flags = -1;
static int ett_fp_rach_new_ie_flags = -1;
static int ett_fp_hsdsch_pdu_block_header = -1;

static expert_field ei_fp_hsdsch_common_experimental_support = EI_INIT;
static expert_field ei_fp_hsdsch_common_t3_not_implemented = EI_INIT;
static expert_field ei_fp_channel_type_unknown = EI_INIT;
static expert_field ei_fp_ddi_not_defined = EI_INIT;
static expert_field ei_fp_stop_hsdpa_transmission = EI_INIT;
static expert_field ei_fp_hsdsch_entity_not_specified = EI_INIT;
static expert_field ei_fp_expecting_tdd = EI_INIT;
static expert_field ei_fp_bad_payload_checksum = EI_INIT;
static expert_field ei_fp_e_rnti_t2_edch_frames = EI_INIT;
static expert_field ei_fp_crci_no_subdissector = EI_INIT;
static expert_field ei_fp_timing_adjustmentment_reported = EI_INIT;
static expert_field ei_fp_mac_is_sdus_miscount = EI_INIT;
static expert_field ei_fp_maybe_srb = EI_INIT;
static expert_field ei_fp_transport_channel_type_unknown = EI_INIT;
static expert_field ei_fp_unable_to_locate_ddi_entry = EI_INIT;
static expert_field ei_fp_e_rnti_first_entry = EI_INIT;
static expert_field ei_fp_bad_header_checksum = EI_INIT;
static expert_field ei_fp_crci_error_bit_set_for_tb = EI_INIT;
static expert_field ei_fp_spare_extension = EI_INIT;
static expert_field ei_fp_no_per_frame_info = EI_INIT;

static dissector_handle_t rlc_bcch_handle;
static dissector_handle_t mac_fdd_dch_handle;
static dissector_handle_t mac_fdd_rach_handle;
static dissector_handle_t mac_fdd_fach_handle;
static dissector_handle_t mac_fdd_pch_handle;
static dissector_handle_t mac_fdd_edch_handle;
static dissector_handle_t mac_fdd_edch_type2_handle;
static dissector_handle_t mac_fdd_hsdsch_handle;
static dissector_handle_t fp_handle;

static proto_tree *top_level_tree = NULL;

/* Variables used for preferences */
static gboolean preferences_call_mac_dissectors = TRUE;
static gboolean preferences_show_release_info = TRUE;
static gboolean preferences_payload_checksum = TRUE;
static gboolean preferences_header_checksum = TRUE;

#define UMTS_FP_USE_UAT 1

#ifdef UMTS_FP_USE_UAT
/* UAT entry structure. */
typedef struct {
   guint8 protocol;
   gchar *srcIP;
   guint16 src_port;
   gchar *dstIP;
   guint16 dst_port;
   guint8 interface_type;
   guint8 division;
   guint8 rlc_mode;
   guint8 channel_type;
} uat_umts_fp_ep_and_ch_record_t;

static uat_umts_fp_ep_and_ch_record_t *uat_umts_fp_ep_and_ch_records = NULL;

static uat_t *umts_fp_uat = NULL;
static guint  num_umts_fp_ep_and_ch_items = 0;

#endif /* UMTS_FP_USE_UAT */
/* E-DCH (T1) channel header information */
struct edch_t1_subframe_info
{
    guint8  subframe_number;
    guint8  number_of_mac_es_pdus;
    guint8  ddi[64];
    guint16 number_of_mac_d_pdus[64];
};

/* E-DCH (T2) channel header information */
struct edch_t2_subframe_info
{
    guint8  subframe_number;
    guint8  number_of_mac_is_pdus;
    guint8  number_of_mac_is_sdus[16];
    guint8  mac_is_lchid[16][16];
    guint16 mac_is_length[16][16];
};


static const value_string channel_type_vals[] =
{
    { CHANNEL_RACH_FDD,         "RACH_FDD" },
    { CHANNEL_RACH_TDD,         "RACH_TDD" },
    { CHANNEL_FACH_FDD,         "FACH_FDD" },
    { CHANNEL_FACH_TDD,         "FACH_TDD" },
    { CHANNEL_DSCH_FDD,         "DSCH_FDD" },
    { CHANNEL_DSCH_TDD,         "DSCH_TDD" },
    { CHANNEL_USCH_TDD_384,     "USCH_TDD_384" },
    { CHANNEL_USCH_TDD_128,     "USCH_TDD_128" },
    { CHANNEL_PCH,              "PCH" },
    { CHANNEL_CPCH,             "CPCH" },
    { CHANNEL_BCH,              "BCH" },
    { CHANNEL_DCH,              "DCH" },
    { CHANNEL_HSDSCH,           "HSDSCH" },
    { CHANNEL_IUR_CPCHF,        "IUR CPCHF" },
    { CHANNEL_IUR_FACH,         "IUR FACH" },
    { CHANNEL_IUR_DSCH,         "IUR DSCH" },
    { CHANNEL_EDCH,             "EDCH" },
    { CHANNEL_RACH_TDD_128,     "RACH_TDD_128" },
    { CHANNEL_HSDSCH_COMMON,    "HSDSCH-COMMON" },
    { CHANNEL_HSDSCH_COMMON_T3, "HSDSCH-COMMON-T3" },
    { CHANNEL_EDCH_COMMON,      "EDCH-COMMON"},
    { 0, NULL }
};

static const value_string division_vals[] =
{
    { Division_FDD,      "FDD"},
    { Division_TDD_384,  "TDD-384"},
    { Division_TDD_128,  "TDD-128"},
    { Division_TDD_768,  "TDD-768"},
    { 0, NULL }
};


static const value_string data_control_vals[] = {
    { 0,   "Data" },
    { 1,   "Control" },
    { 0,   NULL }
};

static const value_string direction_vals[] = {
    { 0,   "Downlink" },
    { 1,   "Uplink" },
    { 0,   NULL }
};

static const value_string crci_vals[] = {
    { 0,   "Correct" },
    { 1,   "Not correct" },
    { 0,   NULL }
};

static const value_string paging_indication_vals[] = {
    { 0,   "no PI-bitmap in payload" },
    { 1,   "PI-bitmap in payload" },
    { 0,   NULL }
};

static const value_string spreading_factor_vals[] = {
    { 0,   "4"},
    { 1,   "8"},
    { 2,   "16"},
    { 3,   "32"},
    { 4,   "64"},
    { 5,   "128"},
    { 6,   "256"},
    { 0,   NULL }
};

static const value_string congestion_status_vals[] = {
    { 0,   "No TNL congestion"},
    { 1,   "Reserved for future use"},
    { 2,   "TNL congestion - detected by delay build-up"},
    { 3,   "TNL congestion - detected by frame loss"},
    { 0,   NULL }
};

static const value_string e_rucch_flag_vals[] = {
    { 0,   "Conventional E-RUCCH reception" },
    { 1,   "TA Request reception" },
    { 0,   NULL }
};

static const value_string hsdshc_mac_entity_vals[] = {
    { entity_not_specified,    "Unspecified (assume MAC-hs)" },
    { hs,                      "MAC-hs" },
    { ehs,                     "MAC-ehs" },
    { 0,   NULL }
};

/* TODO: add and use */
#if 0
static const value_string segmentation_status_vals[] = {
    { 0,    "" },
    { 1,    "" },
    { 2,    "" },
    { 3,    "" },
    { 0,   NULL }
};
#endif

static const value_string lchid_vals[] = {
    {  0,   "Logical Channel 1" },
    {  1,   "Logical Channel 2" },
    {  2,   "Logical Channel 3" },
    {  3,   "Logical Channel 4" },
    {  4,   "Logical Channel 5" },
    {  5,   "Logical Channel 6" },
    {  6,   "Logical Channel 7" },
    {  7,   "Logical Channel 8" },
    {  8,   "Logical Channel 9" },
    {  9,   "Logical Channel 10" },
    { 10,   "Logical Channel 11" },
    { 11,   "Logical Channel 12" },
    { 12,   "Logical Channel 13" },
    { 13,   "Logical Channel 14" },
    { 14,   "CCCH (SRB0)" },
    { 15,   "E-RNTI being included (FDD only)" },
    { 0,   NULL }
};

/* Dedicated control types */
#define DCH_OUTER_LOOP_POWER_CONTROL            1
#define DCH_TIMING_ADJUSTMENT                   2
#define DCH_DL_SYNCHRONISATION                  3
#define DCH_UL_SYNCHRONISATION                  4

#define DCH_DL_NODE_SYNCHRONISATION             6
#define DCH_UL_NODE_SYNCHRONISATION             7
#define DCH_RX_TIMING_DEVIATION                 8
#define DCH_RADIO_INTERFACE_PARAMETER_UPDATE    9
#define DCH_TIMING_ADVANCE                     10
#define DCH_TNL_CONGESTION_INDICATION          11

static const value_string dch_control_frame_type_vals[] = {
    { DCH_OUTER_LOOP_POWER_CONTROL,         "OUTER LOOP POWER CONTROL" },
    { DCH_TIMING_ADJUSTMENT,                "TIMING ADJUSTMENT" },
    { DCH_DL_SYNCHRONISATION,               "DL SYNCHRONISATION" },
    { DCH_UL_SYNCHRONISATION,               "UL SYNCHRONISATION" },
    { 5,                                    "Reserved Value" },
    { DCH_DL_NODE_SYNCHRONISATION,          "DL NODE SYNCHRONISATION" },
    { DCH_UL_NODE_SYNCHRONISATION,          "UL NODE SYNCHRONISATION" },
    { DCH_RX_TIMING_DEVIATION,              "RX TIMING DEVIATION" },
    { DCH_RADIO_INTERFACE_PARAMETER_UPDATE, "RADIO INTERFACE PARAMETER UPDATE" },
    { DCH_TIMING_ADVANCE,                   "TIMING ADVANCE" },
    { DCH_TNL_CONGESTION_INDICATION,        "TNL CONGESTION INDICATION" },
    { 0,   NULL }
};


/* Common channel control types */
#define COMMON_OUTER_LOOP_POWER_CONTROL                1
#define COMMON_TIMING_ADJUSTMENT                       2
#define COMMON_DL_SYNCHRONISATION                      3
#define COMMON_UL_SYNCHRONISATION                      4

#define COMMON_DL_NODE_SYNCHRONISATION                 6
#define COMMON_UL_NODE_SYNCHRONISATION                 7
#define COMMON_DYNAMIC_PUSCH_ASSIGNMENT                8
#define COMMON_TIMING_ADVANCE                          9
#define COMMON_HS_DSCH_Capacity_Request               10
#define COMMON_HS_DSCH_Capacity_Allocation            11
#define COMMON_HS_DSCH_Capacity_Allocation_Type_2     12

static const value_string common_control_frame_type_vals[] = {
    { COMMON_OUTER_LOOP_POWER_CONTROL,            "OUTER LOOP POWER CONTROL" },
    { COMMON_TIMING_ADJUSTMENT,                   "TIMING ADJUSTMENT" },
    { COMMON_DL_SYNCHRONISATION,                  "DL SYNCHRONISATION" },
    { COMMON_UL_SYNCHRONISATION,                  "UL SYNCHRONISATION" },
    { 5,                                          "Reserved Value" },
    { COMMON_DL_NODE_SYNCHRONISATION,             "DL NODE SYNCHRONISATION" },
    { COMMON_UL_NODE_SYNCHRONISATION,             "UL NODE SYNCHRONISATION" },
    { COMMON_DYNAMIC_PUSCH_ASSIGNMENT,            "DYNAMIC PUSCH ASSIGNMENT" },
    { COMMON_TIMING_ADVANCE,                      "TIMING ADVANCE" },
    { COMMON_HS_DSCH_Capacity_Request,            "HS-DSCH Capacity Request" },
    { COMMON_HS_DSCH_Capacity_Allocation,         "HS-DSCH Capacity Allocation" },
    { COMMON_HS_DSCH_Capacity_Allocation_Type_2,  "HS-DSCH Capacity Allocation Type 2" },
    { 0,   NULL }
};

/* 0 to 7*/
static const guint8 hsdsch_macdflow_id_rlc_map[] = {
	RLC_UM,                   /*0 SRB */
	RLC_AM,                   /*1 Interactive PS*/
	RLC_AM,                   /*2 Interatcive PS*/
	RLC_UNKNOWN_MODE,         /*3 ???*/
	RLC_AM,                   /*4 Streaming PS*/
	RLC_UNKNOWN_MODE,
	RLC_UNKNOWN_MODE,
	RLC_UNKNOWN_MODE
};

/* Mapping hsdsch MACd-FlowId to MAC_CONTENT, basically flowid = 1 (0) => SRB*/
/* 1 to 8*/
static const guint8 hsdsch_macdflow_id_mac_content_map[] = {
	MAC_CONTENT_DCCH,	/*1 SRB */
	MAC_CONTENT_PS_DTCH, /*2 Interactive PS*/
	MAC_CONTENT_PS_DTCH,	/*3 Interatcive PS*/
	RLC_UNKNOWN_MODE, /*4 ???*/
	MAC_CONTENT_PS_DTCH,	/*5 Streaming PS*/
	RLC_UNKNOWN_MODE,
	RLC_UNKNOWN_MODE,
	RLC_UNKNOWN_MODE
	};

/* Make fake logical channel id's based on MACdFlow-ID's, 
* XXXX Bug 12121 expanded the number of entries to 8(+2),
* not at all sure what the proper value should be 0xfF?
*/
static const guint8 fake_lchid_macd_flow[] = {1,9,14,11,0,12,0,0};

/* Dissect message parts */
static int dissect_tb_data(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                           int offset, struct fp_info *p_fp_info,
                           dissector_handle_t *data_handle,
                           void *data);

static int dissect_macd_pdu_data(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                                 int offset, guint16 length, guint16 number_of_pdus, struct fp_info *p_fp_info,
                                 void *data);
static int dissect_macd_pdu_data_type_2(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                                        int offset, guint16 length, guint16 number_of_pdus, struct fp_info * fpi,
                                        void *data);

static int dissect_crci_bits(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                             fp_info *p_fp_info, int offset);
static void dissect_spare_extension_and_crc(tvbuff_t *tvb, packet_info *pinfo,
                                            proto_tree *tree, guint8 dch_crc_present,
                                            int offset, guint header_length);
/* Dissect common control messages */
static int dissect_common_outer_loop_power_control(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb,
                                                   int offset, struct fp_info *p_fp_info);
static int dissect_common_timing_adjustment(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb,
                                            int offset, struct fp_info *p_fp_info);
static int dissect_common_dl_node_synchronisation(packet_info *pinfo, proto_tree *tree,
                                                  tvbuff_t *tvb, int offset);
static int dissect_common_ul_node_synchronisation(packet_info *pinfo, proto_tree *tree,
                                                  tvbuff_t *tvb, int offset);
static int dissect_common_dl_synchronisation(packet_info *pinfo, proto_tree *tree,
                                            tvbuff_t *tvb, int offset,
                                            struct fp_info *p_fp_info);
static int dissect_common_ul_synchronisation(packet_info *pinfo, proto_tree *tree,
                                            tvbuff_t *tvb, int offset,
                                            struct fp_info *p_fp_info);
static int dissect_common_timing_advance(packet_info *pinfo, proto_tree *tree,
                                         tvbuff_t *tvb, int offset);
static int dissect_hsdpa_capacity_request(packet_info *pinfo, proto_tree *tree,
                                          tvbuff_t *tvb, int offset);
static int dissect_hsdpa_capacity_allocation(packet_info *pinfo, proto_tree *tree,
                                             tvbuff_t *tvb, int offset,
                                             struct fp_info *p_fp_info);
static int dissect_hsdpa_capacity_allocation_type_2(packet_info *pinfo, proto_tree *tree,
                                                    tvbuff_t *tvb, int offset);
static void dissect_common_control(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                                   int offset, struct fp_info *p_fp_info);
static int dissect_common_dynamic_pusch_assignment(packet_info *pinfo, proto_tree *tree,
                                                   tvbuff_t *tvb, int offset);

/* Dissect common channel types */
static void dissect_rach_channel_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                                      int offset, struct fp_info *p_fp_info,
                                      void *data);
static void dissect_fach_channel_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                                      int offset, struct fp_info *p_fp_info,
                                      void *data);
static void dissect_dsch_channel_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                                      int offset, struct fp_info *p_fp_info);
static void dissect_usch_channel_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                                      int offset, struct fp_info *p_fp_info);
static void dissect_pch_channel_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                                     int offset, struct fp_info *p_fp_info,
                                     void *data);
static void dissect_cpch_channel_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                                      int offset, struct fp_info *p_fp_info);
static void dissect_bch_channel_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                                     int offset, struct fp_info *p_fp_info);
static void dissect_iur_dsch_channel_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                                          int offset, struct fp_info *p_fp_info);
static void dissect_hsdsch_channel_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                                        int offset, struct fp_info *p_fp_info,
                                        void *data);
static void dissect_hsdsch_type_2_channel_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                                               int offset, struct fp_info *p_fp_info,
                                               void *data);
static void dissect_hsdsch_common_channel_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                                               int offset,
                                               struct fp_info *p_fp_info,
                                               void *data);

/* Dissect DCH control messages */
static int dissect_dch_timing_adjustment(proto_tree *tree, packet_info *pinfo,
                                         tvbuff_t *tvb, int offset);
static int dissect_dch_rx_timing_deviation(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset,
                                           struct fp_info *p_fp_info);
static int dissect_dch_dl_synchronisation(proto_tree *tree, packet_info *pinfo,
                                          tvbuff_t *tvb, int offset);
static int dissect_dch_ul_synchronisation(proto_tree *tree, packet_info *pinfo,
                                          tvbuff_t *tvb, int offset);
static int dissect_dch_outer_loop_power_control(proto_tree *tree, packet_info *pinfo,
                                                tvbuff_t *tvb, int offset);
static int dissect_dch_dl_node_synchronisation(proto_tree *tree, packet_info *pinfo,
                                               tvbuff_t *tvb, int offset);
static int dissect_dch_ul_node_synchronisation(proto_tree *tree, packet_info *pinfo,
                                               tvbuff_t *tvb, int offset);
static int dissect_dch_radio_interface_parameter_update(proto_tree *tree, packet_info *pinfo,
                                                        tvbuff_t *tvb, int offset);
static int dissect_dch_timing_advance(proto_tree *tree, packet_info *pinfo,
                                      tvbuff_t *tvb, int offset, struct fp_info *p_fp_info);
static int dissect_dch_tnl_congestion_indication(proto_tree *tree, packet_info *pinfo,
                                                 tvbuff_t *tvb, int offset);


static void dissect_dch_control_frame(proto_tree *tree, packet_info *pinfo, tvbuff_t *tvb,
                                      int offset, struct fp_info *p_fp_info);


/* Dissect a DCH channel */
static void dissect_dch_channel_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                                     int offset, struct fp_info *p_fp_info,
                                     void *data);

/* Dissect dedicated channels */
static void dissect_e_dch_channel_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                                       int offset, struct fp_info *p_fp_info,
                                       gboolean is_common, rlc_info  *rlcinf,
                                       void *data);

static void dissect_e_dch_t2_or_common_channel_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                                                    int offset, struct fp_info *p_fp_info,
                                                    int number_of_subframes,
                                                    gboolean is_common,
                                                    guint16 header_crc,
                                                    proto_item * header_crc_pi,
                                                    void *data);

/* Main dissection function */
static int dissect_fp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data);

/*
 * CRNC sends data downlink on uplink parameters.
 */
void
set_umts_fp_conv_data(conversation_t *conversation, umts_fp_conversation_info_t *umts_fp_conversation_info)
{

    if (conversation == NULL) {
        return;
     }

    conversation_add_proto_data(conversation, proto_fp, umts_fp_conversation_info);
}


static int
get_tb_count(struct fp_info *p_fp_info)
{
    int chan, tb_count = 0;
    for (chan = 0; chan < p_fp_info->num_chans; chan++) {
        tb_count += p_fp_info->chan_num_tbs[chan];
    }
    return tb_count;
}

static gboolean verify_control_frame_crc(tvbuff_t * tvb, packet_info * pinfo, proto_item * pi, guint16 frame_crc)
{
    guint8 crc = 0;
    guint8 * data = NULL;
    /* Get data. */
    data = (guint8 *)tvb_memdup(wmem_packet_scope(), tvb, 0, tvb_reported_length(tvb));
    /* Include only FT flag bit in CRC calculation. */
    data[0] = data[0] & 1;
    /* Calculate crc7 sum. */
    crc = crc7update(0, data, tvb_reported_length(tvb));
    crc = crc7finalize(crc); /* finalize crc */
    if (frame_crc == crc) {
        proto_item_append_text(pi, " [correct]");
        return TRUE;
    } else {
        proto_item_append_text(pi, " [incorrect, should be 0x%x]", crc);
        expert_add_info(pinfo, pi, &ei_fp_bad_header_checksum);
        return FALSE;
    }
}
static gboolean verify_header_crc(tvbuff_t * tvb, packet_info * pinfo, proto_item * pi, guint16 header_crc, guint header_length)
{
    guint8 crc = 0;
    guint8 * data = NULL;
    /* Get data of header with first byte removed. */
    data = (guint8 *)tvb_memdup(wmem_packet_scope(), tvb, 1, header_length-1);
    /* Calculate crc7 sum. */
    crc = crc7update(0, data, header_length-1);
    crc = crc7finalize(crc); /* finalize crc */
    if (header_crc == crc) {
        proto_item_append_text(pi, " [correct]");
        return TRUE;
    } else {
        proto_item_append_text(pi, " [incorrect, should be 0x%x]", crc);
        expert_add_info(pinfo, pi, &ei_fp_bad_header_checksum);
        return FALSE;
    }
}
static gboolean verify_header_crc_edch(tvbuff_t * tvb, packet_info * pinfo, proto_item * pi, guint16 header_crc, guint header_length)
{
    guint16 crc = 0;
    guint8 * data = NULL;
    /* First create new subset of header with first byte removed. */
    tvbuff_t * headtvb = tvb_new_subset_length(tvb, 1, header_length-1);
    /* Get data of header with first byte removed. */
    data = (guint8 *)tvb_memdup(wmem_packet_scope(), headtvb, 0, header_length-1);
    /* Remove first 4 bits of the remaining data which are Header CRC cont. */
    data[0] = data[0] & 0x0f;
    crc = crc11_307_noreflect_noxor(data, header_length-1);
    if (header_crc == crc) {
        proto_item_append_text(pi, " [correct]");
        return TRUE;
    } else {
        proto_item_append_text(pi, " [incorrect, should be 0x%x]", crc);
        expert_add_info(pinfo, pi, &ei_fp_bad_header_checksum);
        return FALSE;
    }
}

/* Dissect the TBs of a UL data frame*/
static int
dissect_tb_data(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                int offset, struct fp_info *p_fp_info,
                dissector_handle_t *data_handle, void *data)
{
    int         chan, num_tbs   = 0;
    int         bit_offset      = 0;
    int         crci_bit_offset = (offset+1)<<3; /* Current offset + Quality estimate of 1 byte at the end*/
    guint       data_bits       = 0;
    guint8      crci_bit        = 0;
    proto_item *tree_ti         = NULL;
    proto_tree *data_tree       = NULL;
    gboolean    dissected       = FALSE;

    if (tree) {
        /* Add data subtree */
        tree_ti =  proto_tree_add_item(tree, hf_fp_data, tvb, offset, -1, ENC_NA);
        proto_item_set_text(tree_ti, "TB data for %u chans", p_fp_info->num_chans);
        data_tree = proto_item_add_subtree(tree_ti, ett_fp_data);
    }

    /* Calculate offset to CRCI bits */

    if (p_fp_info->is_uplink) {
        for (chan=0; chan < p_fp_info->num_chans; chan++) {
            int n;
            for (n=0; n < p_fp_info->chan_num_tbs[chan]; n++) {
                /* Advance bit offset */
                crci_bit_offset += p_fp_info->chan_tf_size[chan];
                /* Pad out to next byte */
                if (crci_bit_offset % 8) {
                    crci_bit_offset += (8 - (crci_bit_offset % 8));
                }
            }
        }
    }
    /* Now for the TB data */
    for (chan=0; chan < p_fp_info->num_chans; chan++) {
        int n;
        p_fp_info->cur_chan = chan;    /*Set current channel?*/
        /* Clearly show channels with no TBs */
        if (p_fp_info->chan_num_tbs[chan] == 0) {
            proto_item *no_tb_ti = proto_tree_add_uint(data_tree, hf_fp_chan_zero_tbs, tvb,
                                                       offset+(bit_offset/8),
                                                       0, chan+1);
            proto_item_append_text(no_tb_ti, " (of size %d)",
                                   p_fp_info->chan_tf_size[chan]);
            PROTO_ITEM_SET_GENERATED(no_tb_ti);
        }

        /* Show TBs from non-empty channels */
        pinfo->fd->subnum = chan; /* set subframe number to current TB */
        for (n=0; n < p_fp_info->chan_num_tbs[chan]; n++) {

            proto_item *ti;
            p_fp_info->cur_tb = chan;    /*Set current transport block?*/
            if (data_tree) {
                ti = proto_tree_add_item(data_tree, hf_fp_tb, tvb,
                                         offset + (bit_offset/8),
                                         ((bit_offset % 8) + p_fp_info->chan_tf_size[chan] + 7) / 8,
                                         ENC_NA);
                proto_item_set_text(ti, "TB (chan %u, tb %u, %u bits)",
                                    chan+1, n+1, p_fp_info->chan_tf_size[chan]);
            }

            if (preferences_call_mac_dissectors /*&& !rlc_is_ciphered(pinfo)*/ && data_handle &&
                (p_fp_info->chan_tf_size[chan] > 0)) {
                tvbuff_t *next_tvb;
                proto_item *item;
                /* If this is DL we should not care about crci bits (since they don't exists)*/
                if (p_fp_info->is_uplink) {


                    if ( p_fp_info->channel == CHANNEL_RACH_FDD) {    /*In RACH we don't have any QE field, hence go back 8 bits.*/
                        crci_bit = tvb_get_bits8(tvb, crci_bit_offset+n-8, 1);
                        item = proto_tree_add_item(data_tree, hf_fp_crci[n%8], tvb, (crci_bit_offset+n-8)/8, 1, ENC_BIG_ENDIAN);
                        PROTO_ITEM_SET_GENERATED(item);
                    } else {
                        crci_bit = tvb_get_bits8(tvb, crci_bit_offset+n, 1);
                        item = proto_tree_add_item(data_tree, hf_fp_crci[n%8], tvb, (crci_bit_offset+n)/8, 1, ENC_BIG_ENDIAN);
                        PROTO_ITEM_SET_GENERATED(item);
                    }
                }

                if (crci_bit == 0 || !p_fp_info->is_uplink) {
                    next_tvb = tvb_new_subset(tvb, offset + bit_offset/8,
                                              ((bit_offset % 8) + p_fp_info->chan_tf_size[chan] + 7) / 8, -1);


                    /****************/
                    /* TODO: maybe this decision can be based only on info available in fp_info */
                    call_dissector_with_data(*data_handle, next_tvb, pinfo, top_level_tree, data);
                    dissected = TRUE;
                } else {
                    proto_tree_add_expert(tree, pinfo, &ei_fp_crci_no_subdissector, tvb, offset + bit_offset/8,
                                               ((bit_offset % 8) + p_fp_info->chan_tf_size[chan] + 7) / 8);
                }

            }
            num_tbs++;

            /* Advance bit offset */
            bit_offset += p_fp_info->chan_tf_size[chan];
            data_bits  += p_fp_info->chan_tf_size[chan];

            /* Pad out to next byte */
            if (bit_offset % 8) {
                bit_offset += (8 - (bit_offset % 8));
            }
        }
    }

    if (dissected == FALSE) {
        col_append_fstr(pinfo->cinfo, COL_INFO, "(%u bits in %u tbs)",
                        data_bits, num_tbs);
    }

    /* Data tree should cover entire length */
    if (data_tree) {
        proto_item_set_len(tree_ti, bit_offset/8);
        proto_item_append_text(tree_ti, " (%u bits in %u tbs)", data_bits, num_tbs);
    }

    /* Move offset past TBs (we know it's already padded out to next byte) */
    offset += (bit_offset / 8);

    return offset;
}


/* Dissect the MAC-d PDUs of an HS-DSCH (type 1) frame.
   Length is in bits, and payload is offset by 4 bits of padding */
static int
dissect_macd_pdu_data(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                      int offset, guint16 length, guint16 number_of_pdus,
                      struct fp_info *p_fp_info, void *data)
{
    int         pdu;
    int         bit_offset = 0;
    proto_item *pdus_ti    = NULL;
    proto_tree *data_tree  = NULL;
    gboolean    dissected  = FALSE;

    /* Add data subtree */
    if (tree) {
        pdus_ti =  proto_tree_add_item(tree, hf_fp_data, tvb, offset, -1, ENC_NA);
        proto_item_set_text(pdus_ti, "%u MAC-d PDUs of %u bits", number_of_pdus, length);
        data_tree = proto_item_add_subtree(pdus_ti, ett_fp_data);
    }

    /* Now for the PDUs */
    for (pdu=0; pdu < number_of_pdus; pdu++) {
        proto_item *pdu_ti;

        if (data_tree) {
            /* Show 4 bits padding at start of PDU */
            proto_tree_add_item(data_tree, hf_fp_hsdsch_data_padding, tvb, offset+(bit_offset/8), 1, ENC_BIG_ENDIAN);

        }
        bit_offset += 4;

        /* Data bytes! */
        if (data_tree) {
            pinfo->fd->subnum = pdu; /* set subframe number to current TB */
            p_fp_info->cur_tb = pdu;    /*Set TB (PDU) index correctly*/
            pdu_ti = proto_tree_add_item(data_tree, hf_fp_mac_d_pdu, tvb,
                                         offset + (bit_offset/8),
                                         ((bit_offset % 8) + length + 7) / 8,
                                         ENC_NA);
            proto_item_set_text(pdu_ti, "MAC-d PDU (PDU %u)", pdu+1);
        }
        if (preferences_call_mac_dissectors /*&& !rlc_is_ciphered(pinfo)*/) {
            tvbuff_t *next_tvb;
            next_tvb = tvb_new_subset(tvb, offset + bit_offset/8,
                                      ((bit_offset % 8) + length + 7)/8, -1);
            call_dissector_with_data(mac_fdd_hsdsch_handle, next_tvb, pinfo, top_level_tree, data);
            dissected = TRUE;
        }

        /* Advance bit offset */
        bit_offset += length;

        /* Pad out to next byte */
        if (bit_offset % 8) {
            bit_offset += (8 - (bit_offset % 8));
        }
    }

    /* Data tree should cover entire length */
    proto_item_set_len(pdus_ti, bit_offset/8);

    /* Move offset past PDUs (we know it's already padded out to next byte) */
    offset += (bit_offset / 8);

    /* Show summary in info column */
    if (dissected == FALSE) {
        col_append_fstr(pinfo->cinfo, COL_INFO, "   %u PDUs of %u bits",
                        number_of_pdus, length);
    }

    return offset;
}


/* Dissect the MAC-d PDUs of an HS-DSCH (type 2) frame.
   Length is in bytes, and payload is byte-aligned (no padding) */
static int
dissect_macd_pdu_data_type_2(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                             int offset, guint16 length, guint16 number_of_pdus,
                             struct fp_info *fpi, void *data)
{
    int         pdu;
    proto_item *pdus_ti      = NULL;
    proto_tree *data_tree    = NULL;
    int         first_offset = offset;
    gboolean    dissected    = FALSE;

    /* Add data subtree */
    if (tree) {
        pdus_ti =  proto_tree_add_item(tree, hf_fp_data, tvb, offset, -1, ENC_NA);
        proto_item_set_text(pdus_ti, "%u MAC-d PDUs of %u bytes", number_of_pdus, length);
        data_tree = proto_item_add_subtree(pdus_ti, ett_fp_data);
    }

    /* Now for the PDUs */
    for (pdu=0; pdu < number_of_pdus; pdu++) {
        proto_item *pdu_ti;

        /* Data bytes! */
        if (data_tree) {
            pdu_ti = proto_tree_add_item(data_tree, hf_fp_mac_d_pdu, tvb,
                                         offset, length, ENC_NA);
            proto_item_set_text(pdu_ti, "MAC-d PDU (PDU %u)", pdu+1);

        }

        if (preferences_call_mac_dissectors  /*&& !rlc_is_ciphered(pinfo)*/) {

            tvbuff_t *next_tvb = tvb_new_subset(tvb, offset, length, -1);


            fpi->cur_tb = pdu;    /*Set proper pdu index for MAC and higher layers*/
            call_dissector_with_data(mac_fdd_hsdsch_handle, next_tvb, pinfo, top_level_tree, data);
            dissected = TRUE;
        }

        /* Advance offset */
        offset += length;
    }

    /* Data tree should cover entire length */
    proto_item_set_len(pdus_ti, offset-first_offset);

    /* Show summary in info column */
    if (!dissected) {
        col_append_fstr(pinfo->cinfo, COL_INFO, "   %u PDUs of %u bits",
                        number_of_pdus, length*8);
    }

    return offset;
}

/* Dissect CRCI bits (uplink) */
static int
dissect_crci_bits(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                  fp_info *p_fp_info, int offset)
{
    int         n, num_tbs;
    proto_item *ti         = NULL;
    proto_tree *crcis_tree = NULL;
    guint       errors     = 0;

    num_tbs = get_tb_count(p_fp_info);


    /* Add CRCIs subtree */
    if (tree) {
        ti =  proto_tree_add_item(tree, hf_fp_crcis, tvb, offset, (num_tbs+7)/8, ENC_NA);
        proto_item_set_text(ti, "CRCI bits for %u tbs", num_tbs);
        crcis_tree = proto_item_add_subtree(ti, ett_fp_crcis);
    }

    /* CRCIs */
    for (n=0; n < num_tbs; n++) {
        int bit = (tvb_get_guint8(tvb, offset+(n/8)) >> (7-(n%8))) & 0x01;
        proto_tree_add_item(crcis_tree, hf_fp_crci[n%8], tvb, offset+(n/8),
                            1, ENC_BIG_ENDIAN);

        if (bit == 1) {
            errors++;
            expert_add_info(pinfo, ti, &ei_fp_crci_error_bit_set_for_tb);
        }
    }

    if (tree) {
        /* Highlight range of bytes covered by indicator bits */
        proto_item_set_len(ti, (num_tbs+7) / 8);

        /* Show error count in root text */
        proto_item_append_text(ti, " (%u errors)", errors);
    }

    offset += ((num_tbs+7) / 8);
    return offset;
}


static void
dissect_spare_extension_and_crc(tvbuff_t *tvb, packet_info *pinfo,
                                proto_tree *tree, guint8 dch_crc_present,
                                int offset, guint header_length)
{
    int         crc_size = 0;
    int         remain   = tvb_captured_length_remaining(tvb, offset);

    /* Payload CRC (optional) */
    if ((dch_crc_present == 1) || ((dch_crc_present == 2) && (remain >= 2))) {
        crc_size = 2;
    }

    if (remain > crc_size) {
        proto_item *ti;
        ti = proto_tree_add_item(tree, hf_fp_spare_extension, tvb,
                                 offset, remain-crc_size, ENC_NA);
        proto_item_append_text(ti, " (%u octets)", remain-crc_size);
        expert_add_info_format(pinfo, ti, &ei_fp_spare_extension, "Spare Extension present (%u bytes)", remain-crc_size);
        offset += remain-crc_size;
    }

    if (crc_size) {
     proto_item * pi = proto_tree_add_item(tree, hf_fp_payload_crc, tvb, offset, crc_size,
                            ENC_BIG_ENDIAN);
        if (preferences_payload_checksum) {
            guint16 calc_crc, read_crc;
            guint8 * data = (guint8 *)tvb_memdup(wmem_packet_scope(), tvb, header_length, offset-header_length);
            calc_crc = crc16_8005_noreflect_noxor(data, offset-header_length);
            read_crc = tvb_get_bits16(tvb, offset*8, 16, ENC_BIG_ENDIAN);

            if (calc_crc == read_crc) {
                proto_item_append_text(pi, " [correct]");
            } else {
                proto_item_append_text(pi, " [incorrect, should be 0x%x]", calc_crc);
                expert_add_info(pinfo, pi, &ei_fp_bad_payload_checksum);
            }
        }
    }
}

/***********************************************************/
/* Common control message types                            */

static int
dissect_common_outer_loop_power_control(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb,
                                        int offset, struct fp_info *p_fp_info _U_)
{
    return dissect_dch_outer_loop_power_control(tree, pinfo, tvb, offset);
}


static int
dissect_common_timing_adjustment(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb,
                                 int offset, struct fp_info *p_fp_info)
{
    if (p_fp_info->channel != CHANNEL_PCH) {
        guint8 cfn;
        gint16 toa;

        /* CFN control */
        cfn = tvb_get_guint8(tvb, offset);
        proto_tree_add_item(tree, hf_fp_cfn_control, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset++;

        /* ToA */
        toa = tvb_get_ntohs(tvb, offset);
        proto_tree_add_item(tree, hf_fp_toa, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;

        col_append_fstr(pinfo->cinfo, COL_INFO, "   CFN=%u, ToA=%d", cfn, toa);
    }
    else {
        guint16 cfn;
        gint32 toa;

        /* PCH CFN is 12 bits */
        cfn = (tvb_get_ntohs(tvb, offset) >> 4);
        proto_tree_add_item(tree, hf_fp_pch_cfn, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;

        /* 4 bits of padding follow... */

        /* 20 bits of ToA (followed by 4 padding bits) */
        toa = ((int)(tvb_get_ntoh24(tvb, offset) << 8)) / 4096;
        proto_tree_add_int(tree, hf_fp_pch_toa, tvb, offset, 3, toa);
        offset += 3;

        col_append_fstr(pinfo->cinfo, COL_INFO, "   CFN=%u, ToA=%d", cfn, toa);
    }
    return offset;
}

static int
dissect_common_dl_node_synchronisation(packet_info *pinfo, proto_tree *tree,
                                       tvbuff_t *tvb, int offset)
{
    /* T1 (3 bytes) */
    guint32 t1 = tvb_get_ntoh24(tvb, offset);
    proto_tree_add_item(tree, hf_fp_t1, tvb, offset, 3, ENC_BIG_ENDIAN);
    offset += 3;

    col_append_fstr(pinfo->cinfo, COL_INFO, "   T1=%u", t1);

    return offset;
}

static int
dissect_common_ul_node_synchronisation(packet_info *pinfo, proto_tree *tree,
                                       tvbuff_t *tvb, int offset)
{
    guint32 t1, t2, t3;

    /* T1 (3 bytes) */
    t1 = tvb_get_ntoh24(tvb, offset);
    proto_tree_add_item(tree, hf_fp_t1, tvb, offset, 3, ENC_BIG_ENDIAN);
    offset += 3;

    /* T2 (3 bytes) */
    t2 = tvb_get_ntoh24(tvb, offset);
    proto_tree_add_item(tree, hf_fp_t2, tvb, offset, 3, ENC_BIG_ENDIAN);
    offset += 3;

    /* T3 (3 bytes) */
    t3 = tvb_get_ntoh24(tvb, offset);
    proto_tree_add_item(tree, hf_fp_t3, tvb, offset, 3, ENC_BIG_ENDIAN);
    offset += 3;

    col_append_fstr(pinfo->cinfo, COL_INFO, "   T1=%u T2=%u, T3=%u",
                    t1, t2, t3);

    return offset;
}

static int
dissect_common_dl_synchronisation(packet_info *pinfo, proto_tree *tree,
                                  tvbuff_t *tvb, int offset, struct fp_info *p_fp_info)
{
    guint16 cfn;

    if (p_fp_info->channel != CHANNEL_PCH) {
        /* CFN control */
        cfn = tvb_get_guint8(tvb, offset);
        proto_tree_add_item(tree, hf_fp_cfn_control, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset++;
    }
    else {
        /* PCH CFN is 12 bits */
        cfn = (tvb_get_ntohs(tvb, offset) >> 4);
        proto_tree_add_item(tree, hf_fp_pch_cfn, tvb, offset, 2, ENC_BIG_ENDIAN);

        /* 4 bits of padding follow... */
        offset += 2;
    }

    col_append_fstr(pinfo->cinfo, COL_INFO, "   CFN=%u", cfn);

    return offset;
}

static int
dissect_common_ul_synchronisation(packet_info *pinfo, proto_tree *tree,
                                  tvbuff_t *tvb, int offset, struct fp_info *p_fp_info)
{
    return dissect_common_timing_adjustment(pinfo, tree, tvb, offset, p_fp_info);
}

static int
dissect_common_timing_advance(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset)
{
    guint8  cfn;
    guint16 timing_advance;

    /* CFN control */
    cfn = tvb_get_guint8(tvb, offset);
    proto_tree_add_item(tree, hf_fp_cfn_control, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;

    /* Timing Advance */
    timing_advance = (tvb_get_guint8(tvb, offset) & 0x3f) * 4;
    proto_tree_add_uint(tree, hf_fp_timing_advance, tvb, offset, 1, timing_advance);
    offset++;

    col_append_fstr(pinfo->cinfo, COL_INFO, " CFN = %u, TA = %u",
                    cfn, timing_advance);

    return offset;
}

static int
dissect_hsdpa_capacity_request(packet_info *pinfo, proto_tree *tree,
                               tvbuff_t *tvb, int offset)
{
    guint8  priority;
    guint16 user_buffer_size;

    /* CmCH-PI */
    priority = (tvb_get_guint8(tvb, offset) & 0x0f);
    proto_tree_add_item(tree, hf_fp_cmch_pi, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;

    /* User buffer size */
    user_buffer_size = tvb_get_ntohs(tvb, offset);
    proto_tree_add_item(tree, hf_fp_user_buffer_size, tvb, offset, 2, ENC_BIG_ENDIAN);
    offset += 2;

    col_append_fstr(pinfo->cinfo, COL_INFO, "      CmCH-PI=%u  User-Buffer-Size=%u",
                    priority, user_buffer_size);

    return offset;
}

static int
dissect_hsdpa_capacity_allocation(packet_info *pinfo, proto_tree *tree,
                                  tvbuff_t *tvb, int offset,
                                  struct fp_info *p_fp_info)
{
    proto_item *ti;
    proto_item *rate_ti;
    guint16     max_pdu_length;
    guint8      repetition_period;
    guint8      interval;
    guint64     credits;

    /* Congestion status (introduced sometime during R6...) */
    if ((p_fp_info->release == 6) || (p_fp_info->release == 7)) {
        proto_tree_add_bits_item(tree, hf_fp_congestion_status, tvb,
                                 offset*8 + 2, 2, ENC_BIG_ENDIAN);
    }

    /* CmCH-PI */
    proto_tree_add_item(tree, hf_fp_cmch_pi, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;

    /* Max MAC-d PDU length (13 bits) */
    max_pdu_length = tvb_get_ntohs(tvb, offset) >> 3;
    proto_tree_add_item(tree, hf_fp_hsdsch_max_macd_pdu_len, tvb, offset, 2, ENC_BIG_ENDIAN);
    offset++;

    /* HS-DSCH credits (11 bits) */
    ti = proto_tree_add_bits_ret_val(tree, hf_fp_hsdsch_credits, tvb,
                                     offset*8 + 5, 11, &credits, ENC_BIG_ENDIAN);
    offset += 2;

    /* Interesting values */
    if (credits == 0) {
        proto_item_append_text(ti, " (stop transmission)");
        expert_add_info(pinfo, ti, &ei_fp_stop_hsdpa_transmission);
    }
    if (credits == 2047) {
        proto_item_append_text(ti, " (unlimited)");
    }

    /* HS-DSCH Interval */
    interval = tvb_get_guint8(tvb, offset);
    ti = proto_tree_add_uint(tree, hf_fp_hsdsch_interval, tvb, offset, 1, interval*10);
    offset++;
    if (interval == 0) {
        proto_item_append_text(ti, " (none of the credits shall be used)");
    }

    /* HS-DSCH Repetition period */
    repetition_period = tvb_get_guint8(tvb, offset);
    ti = proto_tree_add_item(tree, hf_fp_hsdsch_repetition_period, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;
    if (repetition_period == 0) {
        proto_item_append_text(ti, " (unlimited repetition period)");
    }

    /* Calculated and show effective rate enabled */
    if (credits == 2047) {
        rate_ti = proto_tree_add_item(tree, hf_fp_hsdsch_unlimited_rate, tvb, 0, 0, ENC_NA);
        PROTO_ITEM_SET_GENERATED(rate_ti);
    }
    else {
        if (interval != 0) {
            /* Cast on credits is safe, since we know it won't exceed 10^11 */
            rate_ti = proto_tree_add_uint(tree, hf_fp_hsdsch_calculated_rate, tvb, 0, 0,
                                          (guint16)credits * max_pdu_length * (1000 / (interval*10)));
            PROTO_ITEM_SET_GENERATED(rate_ti);
        }
    }

    col_append_fstr(pinfo->cinfo, COL_INFO,
                    "   Max-PDU-len=%u  Credits=%u  Interval=%u  Rep-Period=%u",
                    max_pdu_length, (guint16)credits, interval, repetition_period);

    return offset;
}

static int
dissect_hsdpa_capacity_allocation_type_2(packet_info *pinfo, proto_tree *tree,
                                         tvbuff_t *tvb, int offset)
{
    proto_item *ti;
    proto_item *rate_ti;
    guint16     max_pdu_length;
    guint8      repetition_period;
    guint8      interval;
    guint16     credits;

    /* Congestion status */
    proto_tree_add_bits_item(tree, hf_fp_congestion_status, tvb,
                            offset*8 + 2, 2, ENC_BIG_ENDIAN);

    /* CmCH-PI */
    proto_tree_add_item(tree, hf_fp_cmch_pi, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;

    /* 5 spare bits follow here */

    /* Max MAC-d/c PDU length (11 bits) */
    max_pdu_length = tvb_get_ntohs(tvb, offset) & 0x7ff;
    proto_tree_add_item(tree, hf_fp_hsdsch_max_macdc_pdu_len, tvb, offset, 2, ENC_BIG_ENDIAN);
    offset += 2;

    /* HS-DSCH credits (16 bits) */
    credits = (tvb_get_ntohs(tvb, offset));
    ti = proto_tree_add_uint(tree, hf_fp_hsdsch_credits, tvb,
                             offset, 2, credits);
    offset += 2;

    /* Interesting values */
    if (credits == 0) {
        proto_item_append_text(ti, " (stop transmission)");
        expert_add_info(pinfo, ti, &ei_fp_stop_hsdpa_transmission);
    }
    if (credits == 65535) {
        proto_item_append_text(ti, " (unlimited)");
    }

    /* HS-DSCH Interval */
    interval = tvb_get_guint8(tvb, offset);
    ti = proto_tree_add_uint(tree, hf_fp_hsdsch_interval, tvb, offset, 1, interval*10);
    offset++;
    if (interval == 0) {
        proto_item_append_text(ti, " (none of the credits shall be used)");
    }

    /* HS-DSCH Repetition period */
    repetition_period = tvb_get_guint8(tvb, offset);
    ti = proto_tree_add_item(tree, hf_fp_hsdsch_repetition_period, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;
    if (repetition_period == 0) {
        proto_item_append_text(ti, " (unlimited repetition period)");
    }

    /* Calculated and show effective rate enabled */
    if (credits == 65535) {
        rate_ti = proto_tree_add_item(tree, hf_fp_hsdsch_unlimited_rate, tvb, 0, 0, ENC_NA);
        PROTO_ITEM_SET_GENERATED(rate_ti);
    }
    else {
        if (interval != 0) {
            rate_ti = proto_tree_add_uint(tree, hf_fp_hsdsch_calculated_rate, tvb, 0, 0,
                                          credits * max_pdu_length * (1000 / (interval*10)));
            PROTO_ITEM_SET_GENERATED(rate_ti);
        }
    }

    col_append_fstr(pinfo->cinfo, COL_INFO,
                    "   Max-PDU-len=%u  Credits=%u  Interval=%u  Rep-Period=%u",
                    max_pdu_length, credits, interval, repetition_period);

    return offset;
}



static int
dissect_common_dynamic_pusch_assignment(packet_info *pinfo, proto_tree *tree,
                                        tvbuff_t *tvb, int offset)
{
    guint8 pusch_set_id;
    guint8 activation_cfn;
    guint8 duration;

    /* PUSCH Set Id */
    pusch_set_id = tvb_get_guint8(tvb, offset);
    proto_tree_add_item(tree, hf_fp_pusch_set_id, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;

    /* Activation CFN */
    activation_cfn = tvb_get_guint8(tvb, offset);
    proto_tree_add_item(tree, hf_fp_activation_cfn, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;

    /* Duration */
    duration = tvb_get_guint8(tvb, offset) * 10;
    proto_tree_add_uint(tree, hf_fp_duration, tvb, offset, 1, duration);
    offset++;

    col_append_fstr(pinfo->cinfo, COL_INFO,
                    "   PUSCH Set Id=%u  Activation CFN=%u  Duration=%u",
                    pusch_set_id, activation_cfn, duration);

    return offset;
}





/* Dissect the control part of a common channel message */
static void
dissect_common_control(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                       int offset, struct fp_info *p_fp_info)
{
    /* Common control frame type */
    guint8 control_frame_type = tvb_get_guint8(tvb, offset);
    proto_tree_add_item(tree, hf_fp_common_control_frame_type, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;

    col_append_str(pinfo->cinfo, COL_INFO,
                   val_to_str_const(control_frame_type, common_control_frame_type_vals, "Unknown"));

    /* Frame-type specific dissection */
    switch (control_frame_type) {
        case COMMON_OUTER_LOOP_POWER_CONTROL:
            /*offset =*/ dissect_common_outer_loop_power_control(pinfo, tree, tvb, offset, p_fp_info);
            break;
        case COMMON_TIMING_ADJUSTMENT:
            /*offset =*/ dissect_common_timing_adjustment(pinfo, tree, tvb, offset, p_fp_info);
            break;
        case COMMON_DL_SYNCHRONISATION:
            /*offset =*/ dissect_common_dl_synchronisation(pinfo, tree, tvb, offset, p_fp_info);
            break;
        case COMMON_UL_SYNCHRONISATION:
            /*offset =*/ dissect_common_ul_synchronisation(pinfo, tree, tvb, offset, p_fp_info);
            break;
        case COMMON_DL_NODE_SYNCHRONISATION:
            /*offset =*/ dissect_common_dl_node_synchronisation(pinfo, tree, tvb, offset);
            break;
        case COMMON_UL_NODE_SYNCHRONISATION:
            /*offset =*/ dissect_common_ul_node_synchronisation(pinfo, tree, tvb, offset);
            break;
        case COMMON_DYNAMIC_PUSCH_ASSIGNMENT:
            /*offset =*/ dissect_common_dynamic_pusch_assignment(pinfo, tree, tvb, offset);
            break;
        case COMMON_TIMING_ADVANCE:
            /*offset =*/ dissect_common_timing_advance(pinfo, tree, tvb, offset);
            break;
        case COMMON_HS_DSCH_Capacity_Request:
            /*offset =*/ dissect_hsdpa_capacity_request(pinfo, tree, tvb, offset);
            break;
        case COMMON_HS_DSCH_Capacity_Allocation:
            /*offset =*/ dissect_hsdpa_capacity_allocation(pinfo, tree, tvb, offset, p_fp_info);
            break;
        case COMMON_HS_DSCH_Capacity_Allocation_Type_2:
            /*offset =*/ dissect_hsdpa_capacity_allocation_type_2(pinfo, tree, tvb, offset);
            break;

        default:
            break;
    }

     /* There is no Spare Extension nor payload crc in common control!? */
   /* dissect_spare_extension_and_crc(tvb, pinfo, tree, 0, offset);
    */
}



/**************************/
/* Dissect a RACH channel */
static void
dissect_rach_channel_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                          int offset, struct fp_info *p_fp_info, void *data)
{
    gboolean is_control_frame;
    guint16 header_crc = 0;
    proto_item * header_crc_pi = NULL;
    guint header_length = 0;

    /* Header CRC */
    header_crc = tvb_get_bits8(tvb, 0, 7);
    header_crc_pi = proto_tree_add_item(tree, hf_fp_header_crc, tvb, offset, 1, ENC_BIG_ENDIAN);

    /* Frame Type */
    is_control_frame = tvb_get_guint8(tvb, offset) & 0x01;
    proto_tree_add_item(tree, hf_fp_ft, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;

    col_append_str(pinfo->cinfo, COL_INFO, is_control_frame ? " [Control] " : " [Data] ");

    if (is_control_frame) {
        dissect_common_control(tvb, pinfo, tree, offset, p_fp_info);
        /* For control frame the header CRC is actually frame CRC covering all
         * bytes except the first */
        if (preferences_header_checksum) {
            verify_control_frame_crc(tvb, pinfo, header_crc_pi, header_crc);
        }
    }
    else {
        guint8      cfn;
        guint32     propagation_delay                    = 0;
        proto_item *propagation_delay_ti                 = NULL;
        guint32     received_sync_ul_timing_deviation    = 0;
        proto_item *received_sync_ul_timing_deviation_ti = NULL;
        proto_item *rx_timing_deviation_ti               = NULL;
        guint16     rx_timing_deviation                  = 0;

        /* DATA */

        /* CFN */
        cfn = tvb_get_guint8(tvb, offset);
        proto_tree_add_item(tree, hf_fp_cfn, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset++;

        col_append_fstr(pinfo->cinfo, COL_INFO, "CFN=%03u ", cfn);

        /* TFI */
        proto_tree_add_item(tree, hf_fp_tfi, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset++;

        if (p_fp_info->channel == CHANNEL_RACH_FDD) {
            /* Propagation delay */
            propagation_delay = tvb_get_guint8(tvb, offset);
            propagation_delay_ti = proto_tree_add_uint(tree, hf_fp_propagation_delay, tvb, offset, 1,
                                                       propagation_delay*3);
            offset++;
        }

        /* Should be TDD 3.84 or 7.68 */
        if (p_fp_info->channel == CHANNEL_RACH_TDD) {
            /* Rx Timing Deviation */
            rx_timing_deviation = tvb_get_guint8(tvb, offset);
            rx_timing_deviation_ti = proto_tree_add_item(tree, hf_fp_rx_timing_deviation, tvb, offset, 1, ENC_BIG_ENDIAN);
            offset++;
        }

        if (p_fp_info->channel == CHANNEL_RACH_TDD_128) {
            /* Received SYNC UL Timing Deviation */
            received_sync_ul_timing_deviation = tvb_get_guint8(tvb, offset);
            received_sync_ul_timing_deviation_ti =
                 proto_tree_add_item(tree, hf_fp_received_sync_ul_timing_deviation, tvb, offset, 1, ENC_BIG_ENDIAN);
            offset++;
        }

        header_length = offset;

        /* TB data */
        offset = dissect_tb_data(tvb, pinfo, tree, offset, p_fp_info, &mac_fdd_rach_handle, data);

        /* CRCIs */
        offset = dissect_crci_bits(tvb, pinfo, tree, p_fp_info, offset);

        /* Info introduced in R6 */
        /* only check if it looks as if they are present */
        if (((p_fp_info->release == 6) || (p_fp_info->release == 7)) &&
            (tvb_reported_length_remaining(tvb, offset) > 2))
        {
            int n;
            guint8 flags;
            /* guint8 flag_bytes = 0; */

            gboolean cell_portion_id_present                 = FALSE;
            gboolean ext_propagation_delay_present           = FALSE;
            gboolean angle_of_arrival_present                = FALSE;
            gboolean ext_rx_sync_ul_timing_deviation_present = FALSE;
            gboolean ext_rx_timing_deviation_present         = FALSE;

            /* New IE flags (assume mandatory for now) */
            do {
                proto_item *new_ie_flags_ti;
                proto_tree *new_ie_flags_tree;
                guint ies_found = 0;

                /* Add new IE flags subtree */
                new_ie_flags_ti = proto_tree_add_string_format(tree, hf_fp_rach_new_ie_flags, tvb, offset, 1,
                                                              "", "New IE flags");
                new_ie_flags_tree = proto_item_add_subtree(new_ie_flags_ti, ett_fp_rach_new_ie_flags);

                /* Read next byte */
                flags = tvb_get_guint8(tvb, offset);
                /* flag_bytes++ */

                /* Dissect individual bits */
                for (n=0; n < 8; n++) {
                    switch (n) {
                        case 6:
                            switch (p_fp_info->division) {
                                case Division_FDD:
                                    /* Ext propagation delay */
                                    ext_propagation_delay_present = TRUE;
                                    proto_tree_add_item(new_ie_flags_tree, hf_fp_rach_ext_propagation_delay_present,
                                                        tvb, offset, 1, ENC_BIG_ENDIAN);
                                    break;
                                case Division_TDD_128:
                                    /* Ext Rx Sync UL Timing */
                                    ext_rx_sync_ul_timing_deviation_present = TRUE;
                                    proto_tree_add_item(new_ie_flags_tree, hf_fp_rach_ext_rx_sync_ul_timing_deviation_present,
                                                        tvb, offset, 1, ENC_BIG_ENDIAN);

                                    break;
                                default:
                                    /* Not defined */
                                    proto_tree_add_item(new_ie_flags_tree, hf_fp_rach_new_ie_flag_unused[6],
                                                        tvb, offset, 1, ENC_BIG_ENDIAN);
                                    break;
                            }
                            break;
                        case 7:
                            switch (p_fp_info->division) {
                                case Division_FDD:
                                    /* Cell Portion ID */
                                    cell_portion_id_present = TRUE;
                                    proto_tree_add_item(new_ie_flags_tree, hf_fp_rach_cell_portion_id_present,
                                                        tvb, offset, 1, ENC_BIG_ENDIAN);
                                    break;
                                case Division_TDD_128:
                                    /* AOA */
                                    angle_of_arrival_present = TRUE;
                                    proto_tree_add_item(new_ie_flags_tree, hf_fp_rach_angle_of_arrival_present,
                                                        tvb, offset, 1, ENC_BIG_ENDIAN);
                                    break;
                                case Division_TDD_384:
                                case Division_TDD_768:
                                    /* Extended Rx Timing Deviation */
                                    ext_rx_timing_deviation_present = TRUE;
                                    proto_tree_add_item(new_ie_flags_tree, hf_fp_rach_ext_rx_timing_deviation_present,
                                                        tvb, offset, 1, ENC_BIG_ENDIAN);
                                    break;
                            }
                            break;

                        default:
                            /* No defined meanings */
                            /* Visual Studio Code Analyzer wrongly thinks n can be 7 here. It can't */
                            proto_tree_add_item(new_ie_flags_tree, hf_fp_rach_new_ie_flag_unused[n],
                                                tvb, offset, 1, ENC_BIG_ENDIAN);
                            break;
                    }
                    if ((flags >> (7-n)) & 0x01) {
                        ies_found++;
                    }
                }
                offset++;

                proto_item_append_text(new_ie_flags_ti, " (%u IEs found)", ies_found);

                /* Last bit set will indicate another flags byte follows... */
            } while (0); /*((flags & 0x01) && (flag_bytes < 31));*/

            /* Cell Portion ID */
            if (cell_portion_id_present) {
                    proto_tree_add_item(tree, hf_fp_cell_portion_id, tvb, offset, 1, ENC_BIG_ENDIAN);
                    offset++;
            }

            /* Ext Rx Timing Deviation */
            if (ext_rx_timing_deviation_present) {
                guint8 extra_bits;
                guint bits_to_extend;
                switch (p_fp_info->division) {
                    case Division_TDD_384:
                        bits_to_extend = 1;
                        break;
                    case Division_TDD_768:
                        bits_to_extend = 2;
                        break;

                    default:
                        /* TODO: report unexpected division type */
                        bits_to_extend = 1;
                        break;
                }
                extra_bits = tvb_get_guint8(tvb, offset) &
                                 ((bits_to_extend == 1) ? 0x01 : 0x03);
                rx_timing_deviation = (extra_bits << 8) | (rx_timing_deviation);
                proto_item_append_text(rx_timing_deviation_ti,
                                       " (extended to 0x%x)",
                                       rx_timing_deviation);
                proto_tree_add_bits_item(tree, hf_fp_extended_bits, tvb,
                                         offset*8 + (8-bits_to_extend), bits_to_extend, ENC_BIG_ENDIAN);
                offset++;
            }

            /* Ext propagation delay. */
            if (ext_propagation_delay_present) {
                guint16 extra_bits = tvb_get_ntohs(tvb, offset) & 0x03ff;
                proto_tree_add_item(tree, hf_fp_ext_propagation_delay, tvb, offset, 2, ENC_BIG_ENDIAN);

                /* Adding 10 bits to original 8 */
                proto_item_append_text(propagation_delay_ti, " (extended to %u)",
                                       ((extra_bits << 8) | propagation_delay) * 3);
                offset += 2;
            }

            /* Angle of Arrival (AOA) */
            if (angle_of_arrival_present) {
                proto_tree_add_item(tree, hf_fp_angle_of_arrival, tvb, offset, 2, ENC_BIG_ENDIAN);
                offset += 2;
            }

            /* Ext. Rx Sync UL Timing Deviation */
            if (ext_rx_sync_ul_timing_deviation_present) {
                guint16 extra_bits;

                /* Ext received Sync UL Timing Deviation */
                extra_bits = tvb_get_ntohs(tvb, offset) & 0x1fff;
                proto_tree_add_item(tree, hf_fp_ext_received_sync_ul_timing_deviation, tvb, offset, 2, ENC_BIG_ENDIAN);

                /* Adding 13 bits to original 8 */
                proto_item_append_text(received_sync_ul_timing_deviation_ti, " (extended to %u)",
                                       (extra_bits << 8) | received_sync_ul_timing_deviation);
                offset += 2;
            }
        }
        if (preferences_header_checksum) {
            verify_header_crc(tvb, pinfo, header_crc_pi, header_crc, header_length);
        }
        /* Spare Extension and Payload CRC */
        dissect_spare_extension_and_crc(tvb, pinfo, tree, 1, offset, header_length);
    }
}


/**************************/
/* Dissect a FACH channel */
static void
dissect_fach_channel_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                          int offset, struct fp_info *p_fp_info, void *data)
{
    gboolean is_control_frame;
    guint16 header_crc = 0;
    proto_item * header_crc_pi = NULL;
    guint header_length = 0;

    /* Header CRC */
    header_crc = tvb_get_bits8(tvb, 0, 7);
    header_crc_pi = proto_tree_add_item(tree, hf_fp_header_crc, tvb, offset, 1, ENC_BIG_ENDIAN);

    /* Frame Type */
    is_control_frame = tvb_get_guint8(tvb, offset) & 0x01;
    proto_tree_add_item(tree, hf_fp_ft, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;

    col_append_str(pinfo->cinfo, COL_INFO, is_control_frame ? " [Control] " : " [Data] ");

    if (is_control_frame) {
        dissect_common_control(tvb, pinfo, tree, offset, p_fp_info);
        /* For control frame the header CRC is actually frame CRC covering all
         * bytes except the first */
        if (preferences_header_checksum) {
            verify_control_frame_crc(tvb, pinfo, header_crc_pi, header_crc);
        }
    }
    else {
        guint8 cfn;
        /* DATA */

        /* CFN */
        cfn = tvb_get_guint8(tvb, offset);
        proto_tree_add_item(tree, hf_fp_cfn, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset++;

        col_append_fstr(pinfo->cinfo, COL_INFO, "CFN=%03u ", cfn);

        /* TFI */
        proto_tree_add_item(tree, hf_fp_fach_tfi, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset++;

        /* Transmit power level */
        proto_tree_add_float(tree, hf_fp_transmit_power_level, tvb, offset, 1,
                             (float)(int)(tvb_get_guint8(tvb, offset)) / 10);
        offset++;
        header_length = offset;

        /* TB data */
        offset = dissect_tb_data(tvb, pinfo, tree, offset, p_fp_info, &mac_fdd_fach_handle, data);

        /* New IE flags (if it looks as though they are present) */
        if ((p_fp_info->release == 7) &&
            (tvb_reported_length_remaining(tvb, offset) > 2)) {

            guint8 flags = tvb_get_guint8(tvb, offset);
            guint8 aoa_present = flags & 0x01;
            offset++;

            if (aoa_present) {
                proto_tree_add_item(tree, hf_fp_angle_of_arrival, tvb, offset, 2, ENC_BIG_ENDIAN);
                offset += 2;
            }
        }
        if (preferences_header_checksum) {
            verify_header_crc(tvb, pinfo, header_crc_pi, header_crc, header_length);
        }
        /* Spare Extension and Payload CRC */
        dissect_spare_extension_and_crc(tvb, pinfo, tree, 1, offset, header_length);
    }
}


/**************************/
/* Dissect a DSCH channel */
static void
dissect_dsch_channel_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                          int offset, struct fp_info *p_fp_info)
{
    gboolean is_control_frame;

    /* Header CRC */
    proto_tree_add_item(tree, hf_fp_header_crc, tvb, offset, 1, ENC_BIG_ENDIAN);

    /* Frame Type */
    is_control_frame = tvb_get_guint8(tvb, offset) & 0x01;
    proto_tree_add_item(tree, hf_fp_ft, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;

    col_append_str(pinfo->cinfo, COL_INFO, is_control_frame ? " [Control] " : " [Data] ");

    if (is_control_frame) {
        dissect_common_control(tvb, pinfo, tree, offset, p_fp_info);
    }
    else {
        guint8 cfn;
        guint header_length = 0;

        /* DATA */

        /* CFN */
        cfn = tvb_get_guint8(tvb, offset);
        proto_tree_add_item(tree, hf_fp_cfn, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset++;

        col_append_fstr(pinfo->cinfo, COL_INFO, "CFN=%03u ", cfn);

        /* TFI */
        proto_tree_add_item(tree, hf_fp_tfi, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset++;


        /* Other fields depend upon release & FDD/TDD settings */
        if (((p_fp_info->release == 99) || (p_fp_info->release == 4)) &&
             (p_fp_info->channel == CHANNEL_DSCH_FDD)) {

            /* Power offset */
            proto_tree_add_float(tree, hf_fp_power_offset, tvb, offset, 1,
                                 (float)(-32.0) +
                                  ((float)(int)(tvb_get_guint8(tvb, offset)) * (float)(0.25)));
            offset++;

            /* Code number */
            proto_tree_add_item(tree, hf_fp_code_number, tvb, offset, 1, ENC_BIG_ENDIAN);
            offset++;

            /* Spreading Factor (3 bits) */
            proto_tree_add_item(tree, hf_fp_spreading_factor, tvb, offset, 1, ENC_BIG_ENDIAN);

            /* MC info (4 bits)*/
            proto_tree_add_item(tree, hf_fp_mc_info, tvb, offset, 1, ENC_BIG_ENDIAN);

            /* Last bit of this byte is spare */
            offset++;
        }
        else {
            /* Normal case */

            /* PDSCH Set Id */
            proto_tree_add_item(tree, hf_fp_pdsch_set_id, tvb, offset, 1, ENC_BIG_ENDIAN);
            offset++;

            /* Transmit power level */
            proto_tree_add_float(tree, hf_fp_transmit_power_level, tvb, offset, 1,
                                 (float)(int)(tvb_get_guint8(tvb, offset)) / 10);
            offset++;
        }
        header_length = offset;
        /* TB data */
        offset = dissect_tb_data(tvb, pinfo, tree, offset, p_fp_info, NULL, NULL);

        /* Spare Extension and Payload CRC */
        dissect_spare_extension_and_crc(tvb, pinfo, tree, 1, offset, header_length);
    }
}


/**************************/
/* Dissect a USCH channel */
static void
dissect_usch_channel_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                          int offset, struct fp_info *p_fp_info)
{
    gboolean is_control_frame;

    /* Header CRC */
    proto_tree_add_item(tree, hf_fp_header_crc, tvb, offset, 1, ENC_BIG_ENDIAN);

    /* Frame Type */
    is_control_frame = tvb_get_guint8(tvb, offset) & 0x01;
    proto_tree_add_item(tree, hf_fp_ft, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;

    col_append_str(pinfo->cinfo, COL_INFO, is_control_frame ? " [Control] " : " [Data] ");

    if (is_control_frame) {
        dissect_common_control(tvb, pinfo, tree, offset, p_fp_info);
    }
    else {
        guint cfn;
        guint16 rx_timing_deviation;
        proto_item *rx_timing_deviation_ti;
        guint header_length = 0;

        /* DATA */

        /* CFN */
        cfn = tvb_get_guint8(tvb, offset);
        proto_tree_add_item(tree, hf_fp_cfn, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset++;

        col_append_fstr(pinfo->cinfo, COL_INFO, "CFN=%03u ", cfn);

        /* TFI */
        proto_tree_add_item(tree, hf_fp_usch_tfi, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset++;

        /* Rx Timing Deviation */
        rx_timing_deviation = tvb_get_guint8(tvb, offset);
        rx_timing_deviation_ti = proto_tree_add_item(tree, hf_fp_rx_timing_deviation,
                                                     tvb, offset, 1, ENC_BIG_ENDIAN);
        offset++;
        header_length = offset;
        /* TB data */
        offset = dissect_tb_data(tvb, pinfo, tree, offset, p_fp_info, NULL, NULL);

        /* QE */
        proto_tree_add_item(tree, hf_fp_quality_estimate, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset++;

        /* CRCIs */
        offset = dissect_crci_bits(tvb, pinfo, tree, p_fp_info, offset);

        /* New IEs */
        if ((p_fp_info->release == 7) &&
            (tvb_reported_length_remaining(tvb, offset) > 2)) {

            guint8 flags = tvb_get_guint8(tvb, offset);
            guint8 bits_extended = flags & 0x01;
            offset++;

            if (bits_extended) {
                guint8 extra_bits = tvb_get_guint8(tvb, offset) & 0x03;
                proto_item_append_text(rx_timing_deviation_ti,
                                       " (extended to %u)",
                                       (rx_timing_deviation << 2) | extra_bits);
            }
            offset++;
        }

        /* Spare Extension and Payload CRC */
        dissect_spare_extension_and_crc(tvb, pinfo, tree, 1, offset, header_length);
    }
}



/**************************/
/* Dissect a PCH channel  */
static void
dissect_pch_channel_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                         int offset, struct fp_info *p_fp_info, void *data)
{
    gboolean is_control_frame;
    guint16  pch_cfn;
    gboolean paging_indication;
    guint16 header_crc = 0;
    proto_item * header_crc_pi = NULL;

    /* Header CRC */
    header_crc = tvb_get_bits8(tvb, 0, 7);
    header_crc_pi = proto_tree_add_item(tree, hf_fp_header_crc, tvb, offset, 1, ENC_BIG_ENDIAN);

    /* Frame Type */
    is_control_frame = tvb_get_guint8(tvb, offset) & 0x01;
    proto_tree_add_item(tree, hf_fp_ft, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;

    col_append_str(pinfo->cinfo, COL_INFO, is_control_frame ? " [Control] " : " [Data] ");

    if (is_control_frame) {
        dissect_common_control(tvb, pinfo, tree, offset, p_fp_info);
        /* For control frame the header CRC is actually frame CRC covering all
         * bytes except the first */
        if (preferences_header_checksum) {
            verify_control_frame_crc(tvb, pinfo, header_crc_pi, header_crc);
        }
    }
    else {
        guint header_length = 0;
        /* DATA */

        /* 12-bit CFN value */
        proto_tree_add_item(tree, hf_fp_pch_cfn, tvb, offset, 2, ENC_BIG_ENDIAN);
        pch_cfn = (tvb_get_ntohs(tvb, offset) & 0xfff0) >> 4;
        offset++;

        col_append_fstr(pinfo->cinfo, COL_INFO, "CFN=%04u ", pch_cfn);

        /* Paging indication */
        proto_tree_add_item(tree, hf_fp_pch_pi, tvb, offset, 1, ENC_BIG_ENDIAN);
        paging_indication = tvb_get_guint8(tvb, offset) & 0x01;
        offset++;

        /* 5-bit TFI */
        proto_tree_add_item(tree, hf_fp_pch_tfi, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset++;
        header_length = offset;
        /* Optional paging indications */
        if (paging_indication) {
            proto_item *ti;
            ti = proto_tree_add_item(tree, hf_fp_paging_indication_bitmap, tvb,
                                     offset,
                                     (p_fp_info->paging_indications+7) / 8,
                                     ENC_NA);
            proto_item_append_text(ti, " (%u bits)", p_fp_info->paging_indications);
            offset += ((p_fp_info->paging_indications+7) / 8);
        }

        /* TB data */
        offset = dissect_tb_data(tvb, pinfo, tree, offset, p_fp_info, &mac_fdd_pch_handle, data);

        if (preferences_header_checksum) {
            verify_header_crc(tvb, pinfo, header_crc_pi, header_crc, header_length);
        }
        /* Spare Extension and Payload CRC */
        dissect_spare_extension_and_crc(tvb, pinfo, tree, 1, offset, header_length);
    }
}


/**************************/
/* Dissect a CPCH channel */
static void
dissect_cpch_channel_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                          int offset, struct fp_info *p_fp_info)
{
    gboolean is_control_frame;

    /* Header CRC */
    proto_tree_add_item(tree, hf_fp_header_crc, tvb, offset, 1, ENC_BIG_ENDIAN);

    /* Frame Type */
    is_control_frame = tvb_get_guint8(tvb, offset) & 0x01;
    proto_tree_add_item(tree, hf_fp_ft, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;

    col_append_str(pinfo->cinfo, COL_INFO, is_control_frame ? " [Control] " : " [Data] ");

    if (is_control_frame) {
        dissect_common_control(tvb, pinfo, tree, offset, p_fp_info);
    }
    else {
        guint cfn;
        guint header_length = 0;
        /* DATA */

        /* CFN */
        cfn = tvb_get_guint8(tvb, offset);
        proto_tree_add_item(tree, hf_fp_cfn, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset++;

        col_append_fstr(pinfo->cinfo, COL_INFO, "CFN=%03u ", cfn);

        /* TFI */
        proto_tree_add_item(tree, hf_fp_cpch_tfi, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset++;

        /* Propagation delay */
        proto_tree_add_uint(tree, hf_fp_propagation_delay, tvb, offset, 1,
                            tvb_get_guint8(tvb, offset) * 3);
        offset++;
        header_length = offset; /* XXX this might be wrong */
        /* TB data */
        offset = dissect_tb_data(tvb, pinfo, tree, offset, p_fp_info, NULL, NULL);

        /* CRCIs */
        offset = dissect_crci_bits(tvb, pinfo, tree, p_fp_info, offset);

        /* Spare Extension and Payload CRC */
        dissect_spare_extension_and_crc(tvb, pinfo, tree, 1, offset, header_length);
    }
}


/**************************/
/* Dissect a BCH channel  */
static void
dissect_bch_channel_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                         int offset, struct fp_info *p_fp_info)
{
    gboolean is_control_frame;

    /* Header CRC */
    proto_tree_add_item(tree, hf_fp_header_crc, tvb, offset, 1, ENC_BIG_ENDIAN);

    /* Frame Type */
    is_control_frame = tvb_get_guint8(tvb, offset) & 0x01;
    proto_tree_add_item(tree, hf_fp_ft, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;

    col_append_str(pinfo->cinfo, COL_INFO, is_control_frame ? " [Control] " : " [Data] ");

    if (is_control_frame) {
        dissect_common_control(tvb, pinfo, tree, offset, p_fp_info);
    }
}


/********************************/
/* Dissect an IUR DSCH channel  */
static void
dissect_iur_dsch_channel_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                              int offset, struct fp_info *p_fp_info)
{
    gboolean is_control_frame;

    /* Header CRC */
    proto_tree_add_item(tree, hf_fp_header_crc, tvb, offset, 1, ENC_BIG_ENDIAN);

    /* Frame Type */
    is_control_frame = tvb_get_guint8(tvb, offset) & 0x01;
    proto_tree_add_item(tree, hf_fp_ft, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;

    col_append_str(pinfo->cinfo, COL_INFO, is_control_frame ? " [Control] " : " [Data] ");

    if (is_control_frame) {
        dissect_common_control(tvb, pinfo, tree, offset, p_fp_info);
    }
    else {
        /* TODO: DATA */
    }
}




/************************/
/* DCH control messages */

static int
dissect_dch_timing_adjustment(proto_tree *tree, packet_info *pinfo, tvbuff_t *tvb, int offset)
{
    guint8      control_cfn;
    gint16      toa;
    proto_item *toa_ti;

    /* CFN control */
    control_cfn = tvb_get_guint8(tvb, offset);
    proto_tree_add_item(tree, hf_fp_cfn_control, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;

    /* ToA */
    toa = tvb_get_ntohs(tvb, offset);
    toa_ti = proto_tree_add_item(tree, hf_fp_toa, tvb, offset, 2, ENC_BIG_ENDIAN);
    offset += 2;

    expert_add_info_format(pinfo, toa_ti, &ei_fp_timing_adjustmentment_reported, "Timing adjustmentment reported (%f ms)", (float)(toa / 8));

    col_append_fstr(pinfo->cinfo, COL_INFO,
                    " CFN = %u, ToA = %d", control_cfn, toa);

    return offset;
}

static int
dissect_dch_rx_timing_deviation(packet_info *pinfo, proto_tree *tree,
                                tvbuff_t *tvb, int offset,
                                struct fp_info *p_fp_info)
{
    guint16     timing_deviation;
    gint        timing_deviation_chips;
    proto_item *timing_deviation_ti;

    /* CFN control */
    proto_tree_add_item(tree, hf_fp_cfn_control, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;

    /* Rx Timing Deviation */
    timing_deviation = tvb_get_guint8(tvb, offset);
    timing_deviation_ti = proto_tree_add_item(tree, hf_fp_dch_rx_timing_deviation, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;

    /* May be extended in R7, but in this case there are at least 2 bytes remaining */
    if ((p_fp_info->release == 7) &&
        (tvb_reported_length_remaining(tvb, offset) >= 2)) {

        /* New IE flags */
        guint64 extended_bits_present;
        guint64 e_rucch_present;

        /* Read flags */
        proto_tree_add_bits_ret_val(tree, hf_fp_e_rucch_present, tvb,
                                    offset*8 + 6, 1, &e_rucch_present, ENC_BIG_ENDIAN);
        proto_tree_add_bits_ret_val(tree, hf_fp_extended_bits_present, tvb,
                                    offset*8 + 7, 1, &extended_bits_present, ENC_BIG_ENDIAN);
        offset++;

        /* Optional E-RUCCH */
        if (e_rucch_present) {

            /* Value of bit_offset depends upon division type */
            int bit_offset;

            switch (p_fp_info->division) {
                case Division_TDD_384:
                    bit_offset = 6;
                    break;
                case Division_TDD_768:
                    bit_offset = 5;
                    break;
                default:
                    {
                        proto_tree_add_expert(tree, pinfo, &ei_fp_expecting_tdd, tvb, 0, 0);
                        bit_offset = 6;
                    }
            }

            proto_tree_add_item(tree, hf_fp_dch_e_rucch_flag, tvb, offset, 1, ENC_BIG_ENDIAN);
            proto_tree_add_bits_item(tree, hf_fp_dch_e_rucch_flag, tvb,
                                     offset*8 + bit_offset, 1, ENC_BIG_ENDIAN);
        }

        /* Timing deviation may be extended by another:
           - 1 bits (3.84 TDD)    OR
           - 2 bits (7.68 TDD)
        */
        if (extended_bits_present) {
            guint8 extra_bits;
            guint bits_to_extend;
            switch (p_fp_info->division) {
                case Division_TDD_384:
                    bits_to_extend = 1;
                    break;
                case Division_TDD_768:
                    bits_to_extend = 2;
                    break;

                default:
                    /* TODO: report unexpected division type */
                    bits_to_extend = 1;
                    break;
            }
            extra_bits = tvb_get_guint8(tvb, offset) &
                             ((bits_to_extend == 1) ? 0x01 : 0x03);
            timing_deviation = (extra_bits << 8) | (timing_deviation);
            proto_item_append_text(timing_deviation_ti,
                                   " (extended to 0x%x)",
                                   timing_deviation);
            proto_tree_add_bits_item(tree, hf_fp_extended_bits, tvb,
                                     offset*8 + (8-bits_to_extend), bits_to_extend, ENC_BIG_ENDIAN);
            offset++;
        }
    }

    timing_deviation_chips = (timing_deviation*4) - 1024;
    proto_item_append_text(timing_deviation_ti, " (%d chips)",
                           timing_deviation_chips);

    col_append_fstr(pinfo->cinfo, COL_INFO, " deviation = %u (%d chips)",
                    timing_deviation, timing_deviation_chips);

    return offset;
}

static int
dissect_dch_dl_synchronisation(proto_tree *tree, packet_info *pinfo, tvbuff_t *tvb, int offset)
{
    /* CFN control */
    guint cfn = tvb_get_guint8(tvb, offset);
    proto_tree_add_item(tree, hf_fp_cfn_control, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;

    col_append_fstr(pinfo->cinfo, COL_INFO, " CFN = %u", cfn);

    return offset;
}

static int
dissect_dch_ul_synchronisation(proto_tree *tree, packet_info *pinfo, tvbuff_t *tvb, int offset)
{
    guint8 cfn;
    gint16 toa;

    /* CFN control */
    cfn = tvb_get_guint8(tvb, offset);
    proto_tree_add_item(tree, hf_fp_cfn_control, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;

    /* ToA */
    toa = tvb_get_ntohs(tvb, offset);
    proto_tree_add_item(tree, hf_fp_toa, tvb, offset, 2, ENC_BIG_ENDIAN);
    offset += 2;

    col_append_fstr(pinfo->cinfo, COL_INFO, " CFN = %u, ToA = %d",
                    cfn, toa);

    return offset;
}

static int
dissect_dch_outer_loop_power_control(proto_tree *tree, packet_info *pinfo, tvbuff_t *tvb, int offset)
{
    /* UL SIR target */
    float target = (float)-8.2 + ((float)0.1 * (float)(int)(tvb_get_guint8(tvb, offset)));
    proto_tree_add_float(tree, hf_fp_ul_sir_target, tvb, offset, 1, target);
    offset++;

    col_append_fstr(pinfo->cinfo, COL_INFO, " UL SIR Target = %f", target);

    return offset;
}

static int
dissect_dch_dl_node_synchronisation(proto_tree *tree, packet_info *pinfo, tvbuff_t *tvb, int offset)
{
    return dissect_common_dl_node_synchronisation(pinfo, tree, tvb, offset);
}

static int
dissect_dch_ul_node_synchronisation(proto_tree *tree, packet_info *pinfo, tvbuff_t *tvb, int offset)
{
    return dissect_common_ul_node_synchronisation(pinfo, tree, tvb, offset);
}

static int
dissect_dch_radio_interface_parameter_update(proto_tree *tree, packet_info *pinfo _U_, tvbuff_t *tvb, int offset)
{
    int    n;
    guint8 value;

    /* Show defined flags in these 2 bytes */
    for (n=4; n >= 0; n--) {
        proto_tree_add_item(tree, hf_fp_radio_interface_parameter_update_flag[n], tvb, offset, 2, ENC_BIG_ENDIAN);
    }
    offset += 2;

    /* CFN  */
    tvb_get_guint8(tvb, offset);
    proto_tree_add_item(tree, hf_fp_cfn, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;

    /* DPC mode */
    proto_tree_add_item(tree, hf_fp_dpc_mode, tvb, offset, 1, ENC_BIG_ENDIAN);

    /* TPC PO */
    proto_tree_add_item(tree, hf_fp_tpc_po, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;

    /* Multiple RL sets indicator */
    proto_tree_add_item(tree, hf_fp_multiple_rl_set_indicator, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset += 2;

    /* MAX_UE_TX_POW */
    value = (tvb_get_guint8(tvb, offset) & 0x7f);
    proto_tree_add_int(tree, hf_fp_max_ue_tx_pow, tvb, offset, 1, -55 + value);
    offset++;

    return offset;
}

static int
dissect_dch_timing_advance(proto_tree *tree, packet_info *pinfo,
                           tvbuff_t *tvb, int offset, struct fp_info *p_fp_info)
{
    guint8      cfn;
    guint16     timing_advance;
    proto_item *timing_advance_ti;

    /* CFN control */
    cfn = tvb_get_guint8(tvb, offset);
    proto_tree_add_item(tree, hf_fp_cfn_control, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;

    /* Timing Advance */
    timing_advance = (tvb_get_guint8(tvb, offset) & 0x3f) * 4;
    timing_advance_ti = proto_tree_add_uint(tree, hf_fp_timing_advance, tvb, offset, 1, timing_advance);
    offset++;

    if ((p_fp_info->release == 7) &&
        (tvb_reported_length_remaining(tvb, offset) > 0)) {

        /* New IE flags */
        guint8 flags = tvb_get_guint8(tvb, offset);
        guint8 extended_bits = flags & 0x01;
        offset++;

        if (extended_bits) {
            guint8 extra_bit = tvb_get_guint8(tvb, offset) & 0x01;
            proto_item_append_text(timing_advance_ti, " (extended to %u)",
                                   (timing_advance << 1) | extra_bit);
        }
        offset++;
    }

    col_append_fstr(pinfo->cinfo, COL_INFO, " CFN = %u, TA = %u",
                    cfn, timing_advance);

    return offset;
}

static int
dissect_dch_tnl_congestion_indication(proto_tree *tree, packet_info *pinfo, tvbuff_t *tvb, int offset)
{
    guint64 status;

    /* Congestion status */
    proto_tree_add_bits_ret_val(tree, hf_fp_congestion_status, tvb,
                                offset*8 + 6, 2, &status, ENC_BIG_ENDIAN);
    offset++;

    col_append_fstr(pinfo->cinfo, COL_INFO, " status = %s",
                    val_to_str_const((guint16)status, congestion_status_vals, "unknown"));

    return offset;
}




/* DCH control frame */
static void
dissect_dch_control_frame(proto_tree *tree, packet_info *pinfo, tvbuff_t *tvb,
                          int offset, struct fp_info *p_fp_info)
{
    /* Control frame type */
    guint8 control_frame_type = tvb_get_guint8(tvb, offset);
    proto_tree_add_item(tree, hf_fp_dch_control_frame_type, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;

    col_append_str(pinfo->cinfo, COL_INFO,
                   val_to_str_const(control_frame_type,
                                    dch_control_frame_type_vals, "Unknown"));

    switch (control_frame_type) {
        case DCH_TIMING_ADJUSTMENT:
            /*offset =*/ dissect_dch_timing_adjustment(tree, pinfo, tvb, offset);
            break;
        case DCH_RX_TIMING_DEVIATION:
            /*offset =*/ dissect_dch_rx_timing_deviation(pinfo, tree, tvb, offset, p_fp_info);
            break;
        case DCH_DL_SYNCHRONISATION:
            /*offset =*/ dissect_dch_dl_synchronisation(tree, pinfo, tvb, offset);
            break;
        case DCH_UL_SYNCHRONISATION:
            /*offset =*/ dissect_dch_ul_synchronisation(tree, pinfo, tvb, offset);
            break;
        case DCH_OUTER_LOOP_POWER_CONTROL:
            /*offset =*/ dissect_dch_outer_loop_power_control(tree, pinfo, tvb, offset);
            break;
        case DCH_DL_NODE_SYNCHRONISATION:
            /*offset =*/ dissect_dch_dl_node_synchronisation(tree, pinfo, tvb, offset);
            break;
        case DCH_UL_NODE_SYNCHRONISATION:
            /*offset =*/ dissect_dch_ul_node_synchronisation(tree, pinfo, tvb, offset);
            break;
        case DCH_RADIO_INTERFACE_PARAMETER_UPDATE:
            /*offset =*/ dissect_dch_radio_interface_parameter_update(tree, pinfo, tvb, offset);
            break;
        case DCH_TIMING_ADVANCE:
            /*offset =*/ dissect_dch_timing_advance(tree, pinfo, tvb, offset, p_fp_info);
            break;
        case DCH_TNL_CONGESTION_INDICATION:
            /*offset =*/ dissect_dch_tnl_congestion_indication(tree, pinfo, tvb, offset);
            break;
    }

    /* Spare Extension */
   /* dissect_spare_extension_and_crc(tvb, pinfo, tree, 0, offset);
    */
}

/*******************************/
/* Dissect a DCH channel       */
static void
dissect_dch_channel_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                         int offset, struct fp_info *p_fp_info, void *data)
{
    gboolean is_control_frame;
    guint8   cfn;
    guint header_length = 0;
    guint16 header_crc = 0;
    proto_item * header_crc_pi = NULL;

    /* Header CRC */
    header_crc = tvb_get_bits8(tvb, 0, 7);
    header_crc_pi = proto_tree_add_item(tree, hf_fp_header_crc, tvb, offset, 1, ENC_BIG_ENDIAN);

    /* Frame Type */
    is_control_frame = tvb_get_guint8(tvb, offset) & 0x01;
    proto_tree_add_item(tree, hf_fp_ft, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;

    col_append_str(pinfo->cinfo, COL_INFO,
                   is_control_frame ? " [Control] " :
                                      ((p_fp_info->is_uplink) ? " [ULData] " :
                                                                " [DLData] " ));

    if (is_control_frame) {
        /* DCH control frame */
        dissect_dch_control_frame(tree, pinfo, tvb, offset, p_fp_info);
        /* For control frame the header CRC is actually frame CRC covering all
         * bytes except the first */
        if (preferences_header_checksum) {
            verify_control_frame_crc(tvb, pinfo, header_crc_pi, header_crc);
        }
    } else {
        /************************/
        /* DCH data here        */
        int chan;
        /* CFN */
        proto_tree_add_item(tree, hf_fp_cfn, tvb, offset, 1, ENC_BIG_ENDIAN);
        cfn = tvb_get_guint8(tvb, offset);
        offset++;

        col_append_fstr(pinfo->cinfo, COL_INFO, "CFN=%03u ", cfn);

        /* One TFI for each channel */
        for (chan=0; chan < p_fp_info->num_chans; chan++) {
            proto_tree_add_item(tree, hf_fp_tfi, tvb, offset, 1, ENC_BIG_ENDIAN);
            offset++;
        }
        header_length = offset;
        /* Dissect TB data */
        offset = dissect_tb_data(tvb, pinfo, tree, offset, p_fp_info, &mac_fdd_dch_handle, data);

        /* QE (uplink only) */
        if (p_fp_info->is_uplink) {
            proto_tree_add_item(tree, hf_fp_quality_estimate, tvb, offset, 1, ENC_BIG_ENDIAN);
            offset++;
        }

        /* CRCI bits (uplink only) */
        if (p_fp_info->is_uplink) {
            offset = dissect_crci_bits(tvb, pinfo, tree, p_fp_info, offset);
        }
        if (preferences_header_checksum) {
            verify_header_crc(tvb, pinfo, header_crc_pi, header_crc, header_length);
        }
        /* Spare extension and payload CRC (optional) */
        dissect_spare_extension_and_crc(tvb, pinfo, tree,
                                        p_fp_info->dch_crc_present, offset, header_length);
    }
}

/**********************************/
/* Dissect an E-DCH channel       */
static void
dissect_e_dch_channel_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                           int offset, struct fp_info *p_fp_info,
                           gboolean is_common, rlc_info  *rlcinf,
                           void *data)
{
    gboolean is_control_frame;
    guint8   number_of_subframes;
    guint8   cfn;
    int      n;
    struct   edch_t1_subframe_info subframes[16];
    guint16 header_crc = 0;
    proto_item * header_crc_pi = NULL;
    guint header_length = 0;

    if (p_fp_info->edch_type == 1) {
        col_append_str(pinfo->cinfo, COL_INFO, " (T2)");
    }

    /* Header CRC */
     /* the bitmask doesn't properly handle this delicate case, do manually */
    header_crc = (tvb_get_bits8(tvb, offset*8, 7) << 4) + tvb_get_bits8(tvb, offset*8+8, 4);

    /* Frame Type */
    is_control_frame = tvb_get_guint8(tvb, offset) & 0x01;

    col_append_str(pinfo->cinfo, COL_INFO, is_control_frame ? " [Control] " : " [Data] ");

    if (is_control_frame) {
        /* DCH control frame */

        /* For control frame the header CRC is actually frame CRC covering all
         * bytes except the first */
        header_crc = tvb_get_bits8(tvb, 0, 7);
        header_crc_pi = proto_tree_add_item(tree, hf_fp_header_crc, tvb, 0, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item(tree, hf_fp_ft, tvb, 0, 1, ENC_BIG_ENDIAN);
        offset++;
        if (preferences_header_checksum) {
            verify_control_frame_crc(tvb, pinfo, header_crc_pi, header_crc);
        }
        dissect_dch_control_frame(tree, pinfo, tvb, offset, p_fp_info);
    }
    else {
        /********************************/
        /* E-DCH data here              */
        guint  bit_offset = 0;
        guint  total_pdus = 0;
        guint  total_bits = 0;
        gboolean dissected = FALSE;

        header_crc_pi = proto_tree_add_uint_format(tree, hf_fp_edch_header_crc, tvb,
                offset, 2, header_crc,
                "%u%u%u%u%u%u%u.%u%u%u%u.... = E-DCH Header CRC: 0x%x",
                (header_crc >> 10) & 1,
                (header_crc >> 9) & 1,
                (header_crc >> 8) & 1,
                (header_crc >> 7) & 1,
                (header_crc >> 6) & 1,
                (header_crc >> 5) & 1,
                (header_crc >> 4) & 1,
                (header_crc >> 3) & 1,
                (header_crc >> 2) & 1,
                (header_crc >> 1) & 1,
                (header_crc >> 0) & 1, header_crc);
        proto_tree_add_item(tree, hf_fp_ft, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset++;
        /* FSN */
        proto_tree_add_item(tree, hf_fp_edch_fsn, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset++;

        /* Number of subframes.
           This was 3 bits in early releases, is 4 bits offset by 1 in later releases  */
        if ((p_fp_info->release >= 6) &&
            ((p_fp_info->release_year > 2005) ||
             ((p_fp_info->release_year == 2005) && (p_fp_info->release_month >= 9)))) {

            /* Use 4 bits plus offset of 1 */
            number_of_subframes = (tvb_get_guint8(tvb, offset) & 0x0f) + 1;
        }
        else {
            /* Use 3 bits only */
            number_of_subframes = (tvb_get_guint8(tvb, offset) & 0x07);
        }
        proto_tree_add_uint(tree, hf_fp_edch_number_of_subframes, tvb, offset, 1,
                            number_of_subframes);

        offset++;

        /* CFN */
        cfn = tvb_get_guint8(tvb, offset);
        proto_tree_add_item(tree, hf_fp_cfn, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset++;

        /* Remainder of T2 or common data frames differ here... */
        if (p_fp_info->edch_type == 1) {
            dissect_e_dch_t2_or_common_channel_info(tvb, pinfo, tree, offset, p_fp_info,
                                                    number_of_subframes,
                                                    is_common, header_crc,
                                                    header_crc_pi, data);
            return;
        }

        /* EDCH subframe header list */
        for (n=0; n < number_of_subframes; n++) {
            int i;
            int start_offset = offset;
            proto_item *subframe_header_ti;
            proto_tree *subframe_header_tree;

            /* Add subframe header subtree */
            subframe_header_ti = proto_tree_add_string_format(tree, hf_fp_edch_subframe_header, tvb, offset, 0,
                                                              "", "Subframe");
            subframe_header_tree = proto_item_add_subtree(subframe_header_ti, ett_fp_edch_subframe_header);

            /* Number of HARQ Retransmissions */
            proto_tree_add_item(subframe_header_tree, hf_fp_edch_harq_retransmissions, tvb,
                                offset, 1, ENC_BIG_ENDIAN);

            /* Subframe number */
            subframes[n].subframe_number = (tvb_get_guint8(tvb, offset) & 0x07);
            proto_tree_add_bits_item(subframe_header_tree, hf_fp_edch_subframe_number, tvb,
                                     offset*8+5, 1, ENC_BIG_ENDIAN);
            offset++;

            /* Number of MAC-es PDUs */
            subframes[n].number_of_mac_es_pdus = (tvb_get_guint8(tvb, offset) & 0xf0) >> 4;
            proto_tree_add_item(subframe_header_tree, hf_fp_edch_number_of_mac_es_pdus,
                                tvb, offset, 1, ENC_BIG_ENDIAN);
            bit_offset = 4;

            proto_item_append_text(subframe_header_ti, " %u header (%u MAC-es PDUs)",
                                   subframes[n].subframe_number,
                                   subframes[n].number_of_mac_es_pdus);

            /* Details of each MAC-es PDU */
            for (i=0; i < subframes[n].number_of_mac_es_pdus; i++) {
                guint64 ddi;
                guint64 n_pdus;    /*Size of the PDU*/

                proto_item *ddi_ti;
                gint ddi_size = -1;
                int     p;

                /* DDI (6 bits) */
                ddi_ti = proto_tree_add_bits_ret_val(subframe_header_tree, hf_fp_edch_ddi, tvb,
                                                     offset*8 + bit_offset, 6, &ddi, ENC_BIG_ENDIAN);

                rlcinf->rbid[i] = (guint8)ddi;
                /********************************/
                /* Look up data in higher layers*/
                /* Look up the size from this DDI value */
                for (p=0; p < p_fp_info->no_ddi_entries; p++) {
                    if (ddi == p_fp_info->edch_ddi[p]) {
                        ddi_size = p_fp_info->edch_macd_pdu_size[p];

                        break;
                    }
                }

                if (ddi_size == -1) {
                    expert_add_info_format(pinfo, ddi_ti, &ei_fp_ddi_not_defined, "DDI %u not defined for this UE!", (guint)ddi);
                    return;
                }
                else {
                    proto_item_append_text(ddi_ti, " (%d bits)", ddi_size);
                }

                subframes[n].ddi[i] = (guint8)ddi;
                bit_offset += 6;

                /* Number of MAC-d PDUs (6 bits) */
                proto_tree_add_bits_ret_val(subframe_header_tree, hf_fp_edch_number_of_mac_d_pdus, tvb,
                                            offset*8 + bit_offset, 6, &n_pdus, ENC_BIG_ENDIAN);

                subframes[n].number_of_mac_d_pdus[i] = (guint8)n_pdus;
                bit_offset += 6;
            }

            offset += ((bit_offset+7)/8);

            /* Tree should cover entire subframe header */
            proto_item_set_len(subframe_header_ti, offset - start_offset);
        }
        header_length = offset;
        /* EDCH subframes */
        for (n=0; n < number_of_subframes; n++) {
            int i;
            proto_item *subframe_ti;
            proto_tree *subframe_tree;
            guint bits_in_subframe = 0;
            guint mac_d_pdus_in_subframe = 0;
            guint    lchid=0;    /*Logcial channel id*/
            umts_mac_info *macinf;
            bit_offset = 0;

            macinf = (umts_mac_info *)p_get_proto_data(wmem_file_scope(), pinfo, proto_umts_mac, 0);
            if (!macinf) {
                macinf = wmem_new0(wmem_packet_scope(), umts_mac_info);
            }
            /* Add subframe subtree */
            subframe_ti = proto_tree_add_string_format(tree, hf_fp_edch_subframe, tvb, offset, 0,
                                                       "", "Subframe %u data", subframes[n].subframe_number);
            subframe_tree = proto_item_add_subtree(subframe_ti, ett_fp_edch_subframe);

            for (i=0; i < subframes[n].number_of_mac_es_pdus; i++) {
                int         m;
                guint16     size = 0;
                /* guint8      tsn; */
                guint       send_size;
                proto_item  *ti;
                int         macd_idx;
                proto_tree  *maces_tree = NULL;

                /** TODO: Merge these two loops? **/
                /* Look up mac-d pdu size for this ddi */
                for (m=0; m < p_fp_info->no_ddi_entries; m++) {
                    if (subframes[n].ddi[i] == p_fp_info->edch_ddi[m]) {
                        size = p_fp_info->edch_macd_pdu_size[m];
                        break;
                    }
                }
                /* Look up logicalchannel id for this DDI value */
                for (m=0; m < p_fp_info->no_ddi_entries; m++) {
                    if (subframes[n].ddi[i] == p_fp_info->edch_ddi[m]) {
                        lchid = p_fp_info->edch_lchId[m];
                        break;
                    }
                }

                if (m == p_fp_info->no_ddi_entries) {
                    /* Not found.  Oops */
                    expert_add_info(pinfo, NULL, &ei_fp_unable_to_locate_ddi_entry);
                    return;
                }

                /* Send MAC-dd PDUs together as one MAC-es PDU */
                send_size = size * subframes[n].number_of_mac_d_pdus[i];

                /* 2 bits spare */
                proto_tree_add_item(subframe_tree, hf_fp_edch_pdu_padding, tvb,
                                    offset + (bit_offset/8),
                                    1, ENC_BIG_ENDIAN);
                bit_offset += 2;

                /* TSN */
                /* tsn = (tvb_get_guint8(tvb, offset + (bit_offset/8)) & 0x3f); */
                proto_tree_add_item(subframe_tree, hf_fp_edch_tsn, tvb,
                                    offset + (bit_offset/8),
                                    1, ENC_BIG_ENDIAN);
                bit_offset += 6;

                /* PDU */
                if (subframe_tree) {
                    ti = proto_tree_add_item(subframe_tree, hf_fp_edch_mac_es_pdu, tvb,
                                             offset + (bit_offset/8),
                                             ((bit_offset % 8) + send_size + 7) / 8,
                                             ENC_NA);
                    proto_item_append_text(ti, " (%u * %u = %u bits, PDU %d)",
                                           size, subframes[n].number_of_mac_d_pdus[i],
                                           send_size, n);
                    maces_tree = proto_item_add_subtree(ti, ett_fp_edch_maces);
                }
                for (macd_idx = 0; macd_idx < subframes[n].number_of_mac_d_pdus[i]; macd_idx++) {

                    if (preferences_call_mac_dissectors /*&& !rlc_is_ciphered(pinfo)*/) {
                        tvbuff_t *next_tvb;
                        pinfo->fd->subnum = macd_idx; /* set subframe number to current TB */
                        /* create new TVB and pass further on */
                        next_tvb = tvb_new_subset(tvb, offset + bit_offset/8,
                                ((bit_offset % 8) + size + 7) / 8, -1);


                        /*This was all previously stored in [0] rather than [macd_idx] and cur_tb wasn't updated!*/
                        /*Set up information needed for MAC and lower layers*/
                        macinf->content[macd_idx] = lchId_type_table[lchid];     /*Set the proper Content type for the mac layer.*/
                        macinf->lchid[macd_idx] = lchid;
                        rlcinf->mode[macd_idx] = lchId_rlc_map[lchid]; /* Set RLC mode by lchid to RLC_MODE map in nbap.h */

                        /* Set U-RNTI to ComuncationContext signaled from nbap*/
                        rlcinf->urnti[macd_idx] = p_fp_info->com_context_id;
                        rlcinf->rbid[macd_idx] = lchid; /*subframes[n].ddi[i];*/    /*Save the DDI value for RLC*/
                        /*g_warning("========Setting RBID:%d for lchid:%d", subframes[n].ddi[i], lchid);*/
                        /* rlcinf->mode[0] = RLC_AM;*/
                        rlcinf->li_size[macd_idx] = RLC_LI_7BITS;

#if 0
                        /*If this entry exists, SECRUITY_MODE is completed*/
                        if ( rrc_ciph_inf && g_tree_lookup(rrc_ciph_inf, GINT_TO_POINTER((gint)p_fp_info->com_context_id)) ) {
                            rlcinf->ciphered[macd_idx] = TRUE;
                        } else {
                            rlcinf->ciphered[macd_idx] = FALSE;
                        }
#endif
                        rlcinf->ciphered[macd_idx] = FALSE;
                        rlcinf->deciphered[macd_idx] = FALSE;
                        p_fp_info->cur_tb = macd_idx;    /*Set the transport block index (NOTE: This and not subnum is used in MAC dissector!)*/

                        /* TODO: use maces_tree? */
                        call_dissector_with_data(mac_fdd_edch_handle, next_tvb, pinfo, top_level_tree, data);
                        dissected = TRUE;
                    }
                    else {
                        /* Just add as a MAC-d PDU */
                        proto_tree_add_item(maces_tree, hf_fp_mac_d_pdu, tvb,
                                            offset + (bit_offset/8),
                                            ((bit_offset % 8) + size + 7) / 8,
                                            ENC_NA);
                    }
                    bit_offset += size;
                }

                bits_in_subframe += send_size;
                mac_d_pdus_in_subframe += subframes[n].number_of_mac_d_pdus[i];

                /* Pad out to next byte */
                if (bit_offset % 8) {
                    bit_offset += (8 - (bit_offset % 8));
                }
            }

            if (tree) {
                /* Tree should cover entire subframe */
                proto_item_set_len(subframe_ti, bit_offset/8);
                /* Append summary info to subframe label */
                proto_item_append_text(subframe_ti, " (%u bits in %u MAC-d PDUs)",
                                       bits_in_subframe, mac_d_pdus_in_subframe);
            }
            total_pdus += mac_d_pdus_in_subframe;
            total_bits += bits_in_subframe;

            offset += (bit_offset/8);
        }

        /* Report number of subframes in info column
         * do this only if no other dissector was called */
        if (dissected == FALSE) {
            col_append_fstr(pinfo->cinfo, COL_INFO,
                            " CFN = %03u   (%u bits in %u pdus in %u subframes)",
                            cfn, total_bits, total_pdus, number_of_subframes);
        }
        /* Add data summary to info column */
        /*col_append_fstr(pinfo->cinfo, COL_INFO, " (%u bytes in %u SDUs in %u MAC-is PDUs in %u subframes)",
                        total_bytes, macis_sdus_found, macis_pdus, number_of_subframes);*/
        if (preferences_header_checksum) {
            verify_header_crc_edch(tvb, pinfo, header_crc_pi, header_crc, header_length);
        }
        /* Spare extension and payload CRC (optional) */
        dissect_spare_extension_and_crc(tvb, pinfo, tree,
                                        p_fp_info->dch_crc_present, offset, header_length);
    }
}

/* Dissect the remainder of the T2 or common frame that differs from T1 */
static void
dissect_e_dch_t2_or_common_channel_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                                        int offset, struct fp_info *p_fp_info,
                                        int number_of_subframes,
                                        gboolean is_common,
                                        guint16 header_crc,
                                        proto_item * header_crc_pi,
                                        void *data)
{
    int      n;
    int      pdu_no;
    guint64  total_macis_sdus;
    guint16  macis_sdus_found = 0;
    guint16  macis_pdus       = 0;
    gboolean F                = TRUE; /* We want to continue loop if get E-RNTI indication... */
    gint     bit_offset;
    proto_item *subframe_macis_descriptors_ti = NULL;
    static struct edch_t2_subframe_info subframes[16];
    guint header_length = 0;
    /* User Buffer size */
    proto_tree_add_bits_item(tree, hf_fp_edch_user_buffer_size, tvb, offset*8,
                             18, ENC_BIG_ENDIAN);
    offset += 2;

    /* Spare is in-between... */

    /* Total number of MAC-is SDUs */
    proto_tree_add_bits_ret_val(tree, hf_fp_edch_no_macid_sdus, tvb, offset*8+4,
                                12, &total_macis_sdus, ENC_BIG_ENDIAN);
    offset += 2;

    if (is_common) {
        /* E-RNTI */
        proto_tree_add_item(tree, hf_fp_edch_e_rnti, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;
    }

    bit_offset = offset*8;
    /* EDCH subframe header list */
    for (n=0; n < number_of_subframes; n++) {
        guint64    subframe_number;
        guint64    no_of_macis_pdus;
        proto_item *subframe_header_ti;
        proto_tree *subframe_header_tree;

        /* Add subframe header subtree */
        subframe_header_ti = proto_tree_add_string_format(tree, hf_fp_edch_subframe_header, tvb, offset, 0,
                                                          "", "Subframe");
        subframe_header_tree = proto_item_add_subtree(subframe_header_ti, ett_fp_edch_subframe_header);

        /* Spare bit */
        bit_offset++;

        if (!is_common) {
            /* Number of HARQ Retransmissions */
            proto_tree_add_item(subframe_header_tree, hf_fp_edch_harq_retransmissions, tvb,
                                bit_offset/8, 1, ENC_BIG_ENDIAN);
            bit_offset += 4;
        }

        /* Subframe number */
        proto_tree_add_bits_ret_val(subframe_header_tree, hf_fp_edch_subframe_number, tvb,
                                    bit_offset, 3, &subframe_number, ENC_BIG_ENDIAN);
        subframes[n].subframe_number = (guint8)subframe_number;
        bit_offset += 3;

        /* Number of MAC-is PDUs */
        proto_tree_add_bits_ret_val(subframe_header_tree, hf_fp_edch_number_of_mac_is_pdus, tvb,
                                    bit_offset, 4, &no_of_macis_pdus, ENC_BIG_ENDIAN);
        bit_offset += 4;
        subframes[n].number_of_mac_is_pdus = (guint8)no_of_macis_pdus;
        macis_pdus += subframes[n].number_of_mac_is_pdus;

        /* Next 4 bits are spare for T2*/
        if (!is_common) {
            bit_offset += 4;
        }

        /* Show summary in root */
        proto_item_append_text(subframe_header_ti, " (SFN %u, %u MAC-is PDUs)",
                               subframes[n].subframe_number, subframes[n].number_of_mac_is_pdus);
        proto_item_set_len(subframe_header_ti, is_common ? 1 : 2);
    }
    offset = bit_offset / 8;


    /* MAC-is PDU descriptors for each subframe follow */
    for (n=0; n < number_of_subframes; n++) {
        proto_tree *subframe_macis_descriptors_tree;

        /* Add subframe header subtree */
        subframe_macis_descriptors_ti = proto_tree_add_string_format(tree, hf_fp_edch_macis_descriptors, tvb, offset, 0,
                                                                     "", "MAC-is descriptors (SFN %u)", subframes[n].subframe_number);
        proto_item_set_len(subframe_macis_descriptors_ti, subframes[n].number_of_mac_is_pdus*2);
        subframe_macis_descriptors_tree = proto_item_add_subtree(subframe_macis_descriptors_ti,
                                                                 ett_fp_edch_macis_descriptors);

        /* Find a sequence of descriptors for each MAC-is PDU in this subframe */
        for (pdu_no=0; pdu_no < subframes[n].number_of_mac_is_pdus; pdu_no++) {
            proto_item *f_ti = NULL;

            subframes[n].number_of_mac_is_sdus[pdu_no] = 0;

            do {
                /* Check we haven't gone past the limit */
                if (macis_sdus_found++ > total_macis_sdus) {
                    expert_add_info_format(pinfo, f_ti, &ei_fp_mac_is_sdus_miscount, "Found too many (%u) MAC-is SDUs - header said there were %u", macis_sdus_found, (guint16)total_macis_sdus);
                }

                /* LCH-ID */
                subframes[n].mac_is_lchid[pdu_no][subframes[n].number_of_mac_is_sdus[pdu_no]] = (tvb_get_guint8(tvb, offset) & 0xf0) >> 4;
                proto_tree_add_item(subframe_macis_descriptors_tree, hf_fp_edch_macis_lchid, tvb, offset, 1, ENC_BIG_ENDIAN);
                if (subframes[n].mac_is_lchid[pdu_no][subframes[n].number_of_mac_is_sdus[pdu_no]] == 15) {
                    proto_item *ti;

                    /* 4 bits of spare */
                    offset++;

                    /* E-RNTI */
                    ti = proto_tree_add_item(tree, hf_fp_edch_e_rnti, tvb, offset, 2, ENC_BIG_ENDIAN);
                    offset += 2;

                    /* This is only allowed if:
                       - it's the common case AND
                       - it's the first descriptor */
                    if (!is_common) {
                        expert_add_info(pinfo, ti, &ei_fp_e_rnti_t2_edch_frames);
                    }
                    if (subframes[n].number_of_mac_is_sdus[pdu_no] > 0) {
                        expert_add_info(pinfo, ti, &ei_fp_e_rnti_first_entry);
                    }
                    continue;
                }

                /* Length */
                subframes[n].mac_is_length[pdu_no][subframes[n].number_of_mac_is_sdus[pdu_no]] = (tvb_get_ntohs(tvb, offset) & 0x0ffe) >> 1;
                proto_tree_add_item(subframe_macis_descriptors_tree, hf_fp_edch_macis_length, tvb, offset, 2, ENC_BIG_ENDIAN);
                offset++;

                /* Flag */
                F = tvb_get_guint8(tvb, offset) & 0x01;
                f_ti = proto_tree_add_item(subframe_macis_descriptors_tree, hf_fp_edch_macis_flag, tvb, offset, 1, ENC_BIG_ENDIAN);

                subframes[n].number_of_mac_is_sdus[pdu_no]++;

                offset++;
            } while (F == 0);
        }
    }

    /* Check overall count of MAC-is SDUs */
    if (macis_sdus_found != total_macis_sdus) {
        expert_add_info_format(pinfo, subframe_macis_descriptors_ti, &ei_fp_mac_is_sdus_miscount, "Frame contains %u MAC-is SDUs - header said there would be %u!", macis_sdus_found, (guint16)total_macis_sdus);
    }
    header_length = offset;
    /* Now PDUs */
    for (n=0; n < number_of_subframes; n++) {

        /* MAC-is PDU */
        for (pdu_no=0; pdu_no < subframes[n].number_of_mac_is_pdus; pdu_no++) {
            int i;
            guint length = 0;
            umts_mac_is_info * mac_is_info = wmem_new(wmem_file_scope(), umts_mac_is_info);

            mac_is_info->number_of_mac_is_sdus = subframes[n].number_of_mac_is_sdus[pdu_no];
            DISSECTOR_ASSERT(subframes[n].number_of_mac_is_sdus[pdu_no] <= MAX_MAC_FRAMES);
            for (i = 0; i < subframes[n].number_of_mac_is_sdus[pdu_no]; i++) {
                mac_is_info->sdulength[i] = subframes[n].mac_is_length[pdu_no][i];
                mac_is_info->lchid[i] = subframes[n].mac_is_lchid[pdu_no][i];
                length += subframes[n].mac_is_length[pdu_no][i];
            }

            /* Call MAC for this PDU if configured to */
            if (preferences_call_mac_dissectors) {
                p_add_proto_data(wmem_file_scope(), pinfo, proto_umts_mac, 0, mac_is_info);
                call_dissector_with_data(mac_fdd_edch_type2_handle, tvb_new_subset_remaining(tvb, offset), pinfo, top_level_tree, data);
            }
            else {
                /* Still show data if not decoding as MAC PDU */
                proto_tree_add_item(tree, hf_fp_edch_mac_is_pdu, tvb, offset, length, ENC_NA);
            }

            /* get_mac_tsn_size in packet-umts_mac.h, gets the global_mac_tsn_size preference in umts_mac.c */
            if (get_mac_tsn_size() == MAC_TSN_14BITS) {
                offset += length + 2; /* Plus 2 bytes for TSN 14 bits and SS 2 bit. */
            } else {
                offset += length + 1; /* Plus 1 byte for TSN 6 bits and SS 2 bit. */
            }
        }
    }
    if (preferences_header_checksum) {
        verify_header_crc_edch(tvb, pinfo, header_crc_pi, header_crc, header_length);
    }
    /* Spare extension and payload CRC (optional) */
    dissect_spare_extension_and_crc(tvb, pinfo, tree,
                                    p_fp_info->dch_crc_present, offset, header_length);
}



/**********************************************************/
/* Dissect an HSDSCH channel                              */
/* The data format corresponds to the format              */
/* described in R5 and R6, and frame type 1 in Release 7. */
static void
dissect_hsdsch_channel_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                            int offset, struct fp_info *p_fp_info, void *data)
{
    gboolean is_control_frame;
    guint header_length = 0;
    guint16 header_crc = 0;
    proto_item * header_crc_pi = NULL;

    /* Header CRC */
    header_crc = tvb_get_bits8(tvb, 0, 7);
    header_crc_pi = proto_tree_add_item(tree, hf_fp_header_crc, tvb, offset, 1, ENC_BIG_ENDIAN);

    /* Frame Type */
    is_control_frame = tvb_get_guint8(tvb, offset) & 0x01;
    proto_tree_add_item(tree, hf_fp_ft, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;

    col_append_str(pinfo->cinfo, COL_INFO, is_control_frame ? " [Control] " : " [Data] ");

    if (is_control_frame) {
        dissect_common_control(tvb, pinfo, tree, offset, p_fp_info);
        /* For control frame the header CRC is actually frame CRC covering all
         * bytes except the first */
        if (preferences_header_checksum) {
            verify_control_frame_crc(tvb, pinfo, header_crc_pi, header_crc);
        }
    }
    else {
        guint8 number_of_pdus;
        guint16 pdu_length;
        guint16 user_buffer_size;
        int i;
        umts_mac_info *macinf;
        rlc_info *rlcinf;

        rlcinf = (rlc_info *)p_get_proto_data(wmem_file_scope(), pinfo, proto_rlc, 0);
        if (!rlcinf) {
            rlcinf = wmem_new0(wmem_packet_scope(), rlc_info);
        }
        macinf = (umts_mac_info *)p_get_proto_data(wmem_file_scope(), pinfo, proto_umts_mac, 0);
        if (!macinf) {
            macinf = wmem_new0(wmem_packet_scope(), umts_mac_info);
        }

        /**************************************/
        /* HS-DCH data here (type 1 in R7)    */

        /* Frame Seq Nr */
        if ((p_fp_info->release == 6) ||
            (p_fp_info->release == 7)) {

            guint8 frame_seq_no = (tvb_get_guint8(tvb, offset) & 0xf0) >> 4;
            proto_tree_add_item(tree, hf_fp_frame_seq_nr, tvb, offset, 1, ENC_BIG_ENDIAN);

            col_append_fstr(pinfo->cinfo, COL_INFO, "  seqno=%u", frame_seq_no);
        }

        /* CmCH-PI */
        proto_tree_add_item(tree, hf_fp_cmch_pi, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset++;

        /* MAC-d PDU Length (13 bits) */
        pdu_length = (tvb_get_ntohs(tvb, offset) >> 3);
        proto_tree_add_item(tree, hf_fp_mac_d_pdu_len, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;
        macinf->pdu_len = pdu_length;

        if ((p_fp_info->release == 6) ||
            (p_fp_info->release == 7)) {

            /* Flush bit */
            proto_tree_add_item(tree, hf_fp_flush, tvb, offset-1, 1, ENC_BIG_ENDIAN);

            /* FSN/DRT reset bit */
            proto_tree_add_item(tree, hf_fp_fsn_drt_reset, tvb, offset-1, 1, ENC_BIG_ENDIAN);
        }


        /* Num of PDUs */
        number_of_pdus = tvb_get_guint8(tvb, offset);
        proto_tree_add_item(tree, hf_fp_num_of_pdu, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset++;

        /* User buffer size */
        user_buffer_size = tvb_get_ntohs(tvb, offset);
        proto_tree_add_item(tree, hf_fp_user_buffer_size, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;

        header_length = offset;


        /************************/
        /*Configure the pdus*/
        for (i=0;i<number_of_pdus && i<MIN(MAX_MAC_FRAMES, MAX_RLC_CHANS); i++) {
            macinf->content[i] = hsdsch_macdflow_id_mac_content_map[p_fp_info->hsdsch_macflowd_id]; /*MAC_CONTENT_PS_DTCH;*/
            macinf->lchid[i] = fake_lchid_macd_flow[p_fp_info->hsdsch_macflowd_id];/*Faked logical channel id 255 used as a mark if it doesn't exist...*/
            macinf->fake_chid[i] = TRUE;    /**/
            macinf->macdflow_id[i] = p_fp_info->hsdsch_macflowd_id;    /*Save the flow ID (+1 to make it human readable (it's zero indexed!))*/
            /*Figure out RLC_MODE based on MACd-flow-ID, basically MACd-flow-ID = 0 then it's SRB0 == UM else AM*/
            rlcinf->mode[i] = hsdsch_macdflow_id_rlc_map[p_fp_info->hsdsch_macflowd_id];


            /*Check if this is multiplexed (signaled by RRC)*/
            if ( /*!rlc_is_ciphered(pinfo) &&*/ p_fp_info->hsdhsch_macfdlow_is_mux[p_fp_info->hsdsch_macflowd_id] ) {
                macinf->ctmux[i] = TRUE;
            } else if (p_fp_info->hsdsch_macflowd_id == 0) {              /*MACd-flow = 0 is often SRB */
                expert_add_info(pinfo, NULL, &ei_fp_maybe_srb);
            } else {
                    macinf->ctmux[i] = FALSE;    /*Either it's multiplexed and not signled or it's not MUX*/
            }
            rlcinf->urnti[i] = p_fp_info->com_context_id;
            rlcinf->li_size[i] = RLC_LI_7BITS;
            rlcinf->deciphered[i] = FALSE;
            rlcinf->ciphered[i] = FALSE;
            rlcinf->rbid[i] = macinf->lchid[i];

#if 0
            /*When a flow has been reconfigured rlc needs to be reset.
             * This needs more work though since we must figure out when the re-configuration becomes
             * active based on the CFN value
             * */
            /*Indicate we need to reset stream*/
            if (p_fp_info->reset_frag) {
                rlc_reset_channel(rlcinf->mode[i], macinf->lchid[i], p_fp_info->is_uplink,  rlcinf->urnti[i] );
                p_fp_info->reset_frag = FALSE;

            }
#endif
        }


        /* MAC-d PDUs */
        offset = dissect_macd_pdu_data(tvb, pinfo, tree, offset, pdu_length,
                                       number_of_pdus, p_fp_info, data);

        col_append_fstr(pinfo->cinfo, COL_INFO, "  %ux%u-bit PDUs  User-Buffer-Size=%u",
                        number_of_pdus, pdu_length, user_buffer_size);

        /* Extra IEs (if there is room for them) */
        if (((p_fp_info->release == 6) ||
             (p_fp_info->release == 7)) &&
            (tvb_reported_length_remaining(tvb, offset) > 2)) {

            int n;
            guint8 flags;
            /* guint8 flag_bytes = 0; */

            /* New IE flags */
            do {
                proto_item *new_ie_flags_ti;
                proto_tree *new_ie_flags_tree;
                guint ies_found = 0;

                /* Add new IE flags subtree */
                new_ie_flags_ti = proto_tree_add_string_format(tree, hf_fp_hsdsch_new_ie_flags, tvb, offset, 1,
                                                              "", "New IE flags");
                new_ie_flags_tree = proto_item_add_subtree(new_ie_flags_ti, ett_fp_hsdsch_new_ie_flags);

                /* Read next byte */
                flags = tvb_get_guint8(tvb, offset);
                /* flag_bytes++; */

                /* Dissect individual bits */
                for (n=0; n < 8; n++) {
                    proto_tree_add_item(new_ie_flags_tree, hf_fp_hsdsch_new_ie_flag[n], tvb, offset, 1, ENC_BIG_ENDIAN);
                    if ((flags >> (7-n)) & 0x01) {
                        ies_found++;
                    }
                }
                offset++;

                proto_item_append_text(new_ie_flags_ti, " (%u IEs found)", ies_found);

                /* Last bit set will indicate another flags byte follows... */
            } while (0); /*((flags & 0x01) && (flag_bytes < 31));*/

            if (1) /*(flags & 0x8) */ {
                /* DRT is shown as mandatory in the diagram (3GPP TS 25.435 V6.3.0),
                   but the description below it states that
                   it should depend upon the first bit.  The detailed description of
                   New IE flags doesn't agree, so treat as mandatory for now... */
                proto_tree_add_item(tree, hf_fp_hsdsch_drt, tvb, offset, 2, ENC_BIG_ENDIAN);
                offset += 2;
            }
        }
        if (preferences_header_checksum) {
            verify_header_crc(tvb, pinfo, header_crc_pi, header_crc, header_length);
        }
        /* Spare Extension and Payload CRC */
        dissect_spare_extension_and_crc(tvb, pinfo, tree, 1, offset, header_length);
    }
}


/******************************************/
/* Dissect an HSDSCH type 2 channel       */
/* (introduced in Release 7)              */
/* N.B. there is currently no support for */
/* frame type 3 (IuR only?)               */
static void
dissect_hsdsch_type_2_channel_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                                   int offset, struct fp_info *p_fp_info,
                                   void *data)
{
    gboolean is_control_frame;
    guint16 header_crc = 0;
    proto_item * header_crc_pi = NULL;
    guint16 header_length = 0;

    /* Header CRC */
    header_crc = tvb_get_bits8(tvb, 0, 7);
    header_crc_pi = proto_tree_add_item(tree, hf_fp_header_crc, tvb, offset, 1, ENC_BIG_ENDIAN);

    /* Frame Type */
    is_control_frame = tvb_get_guint8(tvb, offset) & 0x01;
    proto_tree_add_item(tree, hf_fp_ft, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;

    col_append_str(pinfo->cinfo, COL_INFO, is_control_frame ? " [Control] " : " [Data] ");

    if (is_control_frame) {
        dissect_common_control(tvb, pinfo, tree, offset, p_fp_info);
        /* For control frame the header CRC is actually frame CRC covering all
         * bytes except the first */
        if (preferences_header_checksum) {
            verify_control_frame_crc(tvb, pinfo, header_crc_pi, header_crc);
        }
    }
    else {
        guint8 number_of_pdu_blocks;
        gboolean drt_present = FALSE;
        gboolean fach_present = FALSE;
        guint16 user_buffer_size;
        int n;
        guint j;

        #define MAX_PDU_BLOCKS 31
        guint64 lchid[MAX_PDU_BLOCKS];
        guint64 pdu_length[MAX_PDU_BLOCKS];
        guint64 no_of_pdus[MAX_PDU_BLOCKS];

        umts_mac_info *macinf;
        rlc_info *rlcinf;

        rlcinf = (rlc_info *)p_get_proto_data(wmem_file_scope(), pinfo, proto_rlc, 0);
        if (!rlcinf) {
            rlcinf = wmem_new0(wmem_packet_scope(), rlc_info);
        }
        macinf = (umts_mac_info *)p_get_proto_data(wmem_file_scope(), pinfo, proto_umts_mac, 0);
        if (!macinf) {
            macinf = wmem_new0(wmem_packet_scope(), umts_mac_info);
        }

        /********************************/
        /* HS-DCH type 2 data here      */

        col_append_str(pinfo->cinfo, COL_INFO, "(ehs)");

        /* Frame Seq Nr (4 bits) */
        if ((p_fp_info->release == 6) ||
            (p_fp_info->release == 7)) {

            guint8 frame_seq_no = (tvb_get_guint8(tvb, offset) & 0xf0) >> 4;
            proto_tree_add_item(tree, hf_fp_frame_seq_nr, tvb, offset, 1, ENC_BIG_ENDIAN);

            col_append_fstr(pinfo->cinfo, COL_INFO, "  seqno=%u", frame_seq_no);
        }

        /* CmCH-PI (4 bits) */
        proto_tree_add_item(tree, hf_fp_cmch_pi, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset++;

        /* Total number of PDU blocks (5 bits) */
        number_of_pdu_blocks = (tvb_get_guint8(tvb, offset) >> 3);
        proto_tree_add_item(tree, hf_fp_total_pdu_blocks, tvb, offset, 1, ENC_BIG_ENDIAN);

        if (p_fp_info->release == 7) {
            /* Flush bit */
            proto_tree_add_item(tree, hf_fp_flush, tvb, offset, 1, ENC_BIG_ENDIAN);

            /* FSN/DRT reset bit */
            proto_tree_add_item(tree, hf_fp_fsn_drt_reset, tvb, offset, 1, ENC_BIG_ENDIAN);

            /* DRT Indicator */
            drt_present = tvb_get_guint8(tvb, offset) & 0x01;
            proto_tree_add_item(tree, hf_fp_drt_indicator, tvb, offset, 1, ENC_BIG_ENDIAN);
        }
        offset++;

        /* FACH Indicator flag */
        fach_present = (tvb_get_guint8(tvb, offset) & 0x80) >> 7;
        proto_tree_add_item(tree, hf_fp_fach_indicator, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset++;

        /* User buffer size */
        user_buffer_size = tvb_get_ntohs(tvb, offset);
        proto_tree_add_item(tree, hf_fp_user_buffer_size, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;

        col_append_fstr(pinfo->cinfo, COL_INFO, "  User-Buffer-Size=%u", user_buffer_size);



        /********************************************************************/
        /* Now read number_of_pdu_blocks header entries                     */
        for (n=0; n < number_of_pdu_blocks; n++) {
            proto_item *pdu_block_header_ti;
            proto_tree *pdu_block_header_tree;
            int        block_header_start_offset = offset;

            /* Add PDU block header subtree */
            pdu_block_header_ti = proto_tree_add_string_format(tree, hf_fp_hsdsch_pdu_block_header,
                                                               tvb, offset, 0,
                                                               "",
                                                               "PDU Block Header");
            pdu_block_header_tree = proto_item_add_subtree(pdu_block_header_ti,
                                                           ett_fp_hsdsch_pdu_block_header);

            /* MAC-d/c PDU length in this block (11 bits) */
            proto_tree_add_bits_ret_val(pdu_block_header_tree, hf_fp_pdu_length_in_block, tvb,
                                        (offset*8) + ((n % 2) ? 4 : 0), 11,
                                        &pdu_length[n], ENC_BIG_ENDIAN);
            if ((n % 2) == 0)
                offset++;
            else
                offset += 2;


            /* # PDUs in this block (4 bits) */
            proto_tree_add_bits_ret_val(pdu_block_header_tree, hf_fp_pdus_in_block, tvb,
                                        (offset*8) + ((n % 2) ? 0 : 4), 4,
                                        &no_of_pdus[n], ENC_BIG_ENDIAN);
            if ((n % 2) == 0) {
                offset++;
            }

            /* Logical channel ID in block (4 bits) */
            proto_tree_add_bits_ret_val(pdu_block_header_tree, hf_fp_lchid, tvb,
                                        (offset*8) + ((n % 2) ? 4 : 0), 4,
                                        &lchid[n], ENC_BIG_ENDIAN);
            if ((n % 2) == 1) {
                offset++;
            }
            else {
                if (n == (number_of_pdu_blocks-1)) {
                    /* Byte is padded out for last block */
                    offset++;
                }
            }

            /* Append summary to header tree root */
            proto_item_append_text(pdu_block_header_ti,
                                   " (lch:%u, %u pdus of %u bytes)",
                                   (guint16)lchid[n],
                                   (guint16)no_of_pdus[n],
                                   (guint16)pdu_length[n]);

            /* Set length of header tree item */
            if (((n % 2) == 0) && (n < (number_of_pdu_blocks-1))) {
                proto_item_set_len(pdu_block_header_ti,
                                   offset - block_header_start_offset+1);
            }
            else {
                proto_item_set_len(pdu_block_header_ti,
                                   offset - block_header_start_offset);
            }
        }

        if (header_length == 0) {
            header_length = offset;
        }
        /**********************************************/
        /* Optional fields indicated by earlier flags */
        if (drt_present) {
            /* DRT */
            proto_tree_add_item(tree, hf_fp_drt, tvb, offset, 2, ENC_BIG_ENDIAN);
            offset += 2;
        }

        if (fach_present) {
            /* H-RNTI: */
            proto_tree_add_item(tree, hf_fp_hrnti, tvb, offset, 2, ENC_BIG_ENDIAN);
            offset += 2;

            /* RACH Measurement Result */
            proto_tree_add_item(tree, hf_fp_rach_measurement_result, tvb, offset, 2, ENC_BIG_ENDIAN);
            offset++;
        }


        /********************************************************************/
        /* Now read the MAC-d/c PDUs for each block using info from headers */
        for (n=0; n < number_of_pdu_blocks; n++) {
            for (j=0;j<no_of_pdus[n];j++) {

                /*Configure (signal to lower layers) the PDU!*/
                macinf->content[j] = lchId_type_table[lchid[n]+1];/*hsdsch_macdflow_id_mac_content_map[p_fp_info->hsdsch_macflowd_id];*/ /*MAC_CONTENT_PS_DTCH;*/
                macinf->lchid[j] = (guint8)lchid[n]+1;    /*Add 1 since C/T is zero indexed? ie C/T =0 => L-CHID = 1*/
                macinf->macdflow_id[j] = p_fp_info->hsdsch_macflowd_id;
                /*Figure out RLC_MODE based on MACd-flow-ID, basically MACd-flow-ID = 0 then it's SRB0 == UM else AM*/
                rlcinf->mode[j] = lchId_rlc_map[lchid[n]+1];/*hsdsch_macdflow_id_rlc_map[p_fp_info->hsdsch_macflowd_id];*/

                macinf->ctmux[n] = FALSE;

                rlcinf->li_size[j] = RLC_LI_7BITS;

                /** Configure ciphering **/
#if 0
                /*If this entry exists, SECRUITY_MODE is completed*/
                if ( rrc_ciph_inf && g_tree_lookup(rrc_ciph_inf, GINT_TO_POINTER((gint)p_fp_info->com_context_id)) ) {
                    rlcinf->ciphered[j] = TRUE;
                } else {
                    rlcinf->ciphered[j] = FALSE;
                }
#endif
                rlcinf->ciphered[j] = FALSE;
                rlcinf->deciphered[j] = FALSE;
                rlcinf->rbid[j] = (guint8)lchid[n]+1;

                rlcinf->urnti[j] = p_fp_info->com_context_id;    /*Set URNIT to comuncation context id*/
            }

            /* Add PDU block header subtree */
            offset = dissect_macd_pdu_data_type_2(tvb, pinfo, tree, offset,
                                                  (guint16)pdu_length[n],
                                                  (guint16)no_of_pdus[n],
                                                  p_fp_info, data);
        }
        if (preferences_header_checksum) {
            verify_header_crc(tvb, pinfo, header_crc_pi, header_crc, header_length);
        }
        /* Spare Extension and Payload CRC */
        dissect_spare_extension_and_crc(tvb, pinfo, tree, 1, offset, header_length);
    }
}
/**
* Dissect and CONFIGURE hsdsch_common channel.
*
* This will dissect hsdsch common channels of type 2, so this is
* very similar to regular type two (ehs) the difference being how
* the configuration is done. NOTE: VERY EXPERIMENTAL.
*
* @param tvb the tv buffer of the current data
* @param pinfo the packet info of the current data
* @param tree the tree to append this item to
* @param offset the offset in the tvb
* @param p_fp_info FP-packet information
*/
static
void dissect_hsdsch_common_channel_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                                        int offset, struct fp_info *p_fp_info,
                                        void *data)
{
    gboolean is_control_frame;
    guint16 header_crc = 0;
    proto_item * header_crc_pi = NULL;
    guint header_length = 0;

    /* Header CRC */
    header_crc = tvb_get_bits8(tvb, 0, 7);
    header_crc_pi = proto_tree_add_item(tree, hf_fp_header_crc, tvb, offset, 1, ENC_BIG_ENDIAN);

    /* Frame Type */
    is_control_frame = tvb_get_guint8(tvb, offset) & 0x01;
    proto_tree_add_item(tree, hf_fp_ft, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;

    col_append_str(pinfo->cinfo, COL_INFO, is_control_frame ? " [Control] " : " [Data] ");

    if (is_control_frame) {
        dissect_common_control(tvb, pinfo, tree, offset, p_fp_info);
        /* For control frame the header CRC is actually frame CRC covering all
         * bytes except the first */
        if (preferences_header_checksum) {
            verify_control_frame_crc(tvb, pinfo, header_crc_pi, header_crc);
        }
    }
    else {
        guint8 number_of_pdu_blocks;
        gboolean drt_present = FALSE;
        gboolean fach_present = FALSE;
        guint16 user_buffer_size;
        int n;
        guint j;

        #define MAX_PDU_BLOCKS 31
        guint64 lchid[MAX_PDU_BLOCKS];
        guint64 pdu_length[MAX_PDU_BLOCKS];
        guint64 no_of_pdus[MAX_PDU_BLOCKS];
        guint8 newieflags = 0;

        umts_mac_info *macinf;
        rlc_info *rlcinf;

        rlcinf = (rlc_info *)p_get_proto_data(wmem_file_scope(), pinfo, proto_rlc, 0);
        if (!rlcinf) {
            rlcinf = wmem_new0(wmem_packet_scope(), rlc_info);
        }
        macinf = (umts_mac_info *)p_get_proto_data(wmem_file_scope(), pinfo, proto_umts_mac, 0);
        if (!macinf) {
            macinf = wmem_new0(wmem_packet_scope(), umts_mac_info);
        }
        /********************************/
        /* HS-DCH type 2 data here      */

        col_append_str(pinfo->cinfo, COL_INFO, "(ehs)");

        /* Frame Seq Nr (4 bits) */
        if ((p_fp_info->release == 6) ||
            (p_fp_info->release == 7)) {

            guint8 frame_seq_no = (tvb_get_guint8(tvb, offset) & 0xf0) >> 4;
            proto_tree_add_item(tree, hf_fp_frame_seq_nr, tvb, offset, 1, ENC_BIG_ENDIAN);

            col_append_fstr(pinfo->cinfo, COL_INFO, "  seqno=%u", frame_seq_no);
        }

        /* CmCH-PI (4 bits) */
        proto_tree_add_item(tree, hf_fp_cmch_pi, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset++;

        /* Total number of PDU blocks (5 bits) */
        number_of_pdu_blocks = (tvb_get_guint8(tvb, offset) >> 3);
        proto_tree_add_item(tree, hf_fp_total_pdu_blocks, tvb, offset, 1, ENC_BIG_ENDIAN);

        if (p_fp_info->release == 7) {
            /* Flush bit */
            proto_tree_add_item(tree, hf_fp_flush, tvb, offset, 1, ENC_BIG_ENDIAN);

            /* FSN/DRT reset bit */
            proto_tree_add_item(tree, hf_fp_fsn_drt_reset, tvb, offset, 1, ENC_BIG_ENDIAN);

            /* DRT Indicator */
            drt_present = tvb_get_guint8(tvb, offset) & 0x01;
            proto_tree_add_item(tree, hf_fp_drt_indicator, tvb, offset, 1, ENC_BIG_ENDIAN);
        }
        offset++;

        /* FACH Indicator flag */
        fach_present = (tvb_get_guint8(tvb, offset) & 0x80) >> 7;
        proto_tree_add_item(tree, hf_fp_fach_indicator, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset++;

        /* User buffer size */
        user_buffer_size = tvb_get_ntohs(tvb, offset);
        proto_tree_add_item(tree, hf_fp_user_buffer_size, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;

        col_append_fstr(pinfo->cinfo, COL_INFO, "  User-Buffer-Size=%u", user_buffer_size);


        /********************************************************************/
        /* Now read number_of_pdu_blocks header entries                     */
        for (n=0; n < number_of_pdu_blocks; n++) {
            proto_item *pdu_block_header_ti;
            proto_tree *pdu_block_header_tree;
            int        block_header_start_offset = offset;

            /* Add PDU block header subtree */
            pdu_block_header_ti = proto_tree_add_string_format(tree, hf_fp_hsdsch_pdu_block_header,
                                                               tvb, offset, 0,
                                                               "",
                                                               "PDU Block Header");
            pdu_block_header_tree = proto_item_add_subtree(pdu_block_header_ti,
                                                           ett_fp_hsdsch_pdu_block_header);

            /* MAC-d/c PDU length in this block (11 bits) */
            proto_tree_add_bits_ret_val(pdu_block_header_tree, hf_fp_pdu_length_in_block, tvb,
                                        (offset*8) + ((n % 2) ? 4 : 0), 11,
                                        &pdu_length[n], ENC_BIG_ENDIAN);
            if ((n % 2) == 0)
                offset++;
            else
                offset += 2;


            /* # PDUs in this block (4 bits) */
            proto_tree_add_bits_ret_val(pdu_block_header_tree, hf_fp_pdus_in_block, tvb,
                                        (offset*8) + ((n % 2) ? 0 : 4), 4,
                                        &no_of_pdus[n], ENC_BIG_ENDIAN);
            if ((n % 2) == 0) {
                offset++;
            }

            /* Logical channel ID in block (4 bits) */
            proto_tree_add_bits_ret_val(pdu_block_header_tree, hf_fp_lchid, tvb,
                                        (offset*8) + ((n % 2) ? 4 : 0), 4,
                                        &lchid[n], ENC_BIG_ENDIAN);
            if ((n % 2) == 1) {
                offset++;
            }
            else {
                if (n == (number_of_pdu_blocks-1)) {
                    /* Byte is padded out for last block */
                    offset++;
                }
            }

            /* Append summary to header tree root */
            proto_item_append_text(pdu_block_header_ti,
                                   " (lch:%u, %u pdus of %u bytes)",
                                   (guint16)lchid[n],
                                   (guint16)no_of_pdus[n],
                                   (guint16)pdu_length[n]);

            /* Set length of header tree item */
            if (((n % 2) == 0) && (n < (number_of_pdu_blocks-1))) {
                proto_item_set_len(pdu_block_header_ti,
                                   offset - block_header_start_offset+1);
            }
            else {
                proto_item_set_len(pdu_block_header_ti,
                                   offset - block_header_start_offset);
            }
        }
        if (header_length == 0) {
            header_length = offset;
        }

        /**********************************************/
        /* Optional fields indicated by earlier flags */
        if (drt_present) {
            /* DRT */
            proto_tree_add_item(tree, hf_fp_drt, tvb, offset, 2, ENC_BIG_ENDIAN);
            offset += 2;
        }

        if (fach_present) {
            /* H-RNTI: */
            proto_tree_add_item(tree, hf_fp_hrnti, tvb, offset, 2, ENC_BIG_ENDIAN);
            offset += 2;

            /* RACH Measurement Result */
            proto_tree_add_item(tree, hf_fp_rach_measurement_result, tvb, offset, 1, ENC_BIG_ENDIAN);
            offset++;
        }

        /********************************************************************/
        /* Now read the MAC-d/c PDUs for each block using info from headers */
        for (n=0; n < number_of_pdu_blocks; n++) {
            tvbuff_t *next_tvb;
            for (j=0; j<no_of_pdus[n]; j++) {
                /* If all bits are set, then this is BCCH or PCCH according to: 25.435 paragraph: 6.2.7.31 */
                if (lchid[n] == 0xF) {
                    /* In the very few test cases I've seen, this seems to be
                     * BCCH with transparent MAC layer. Therefore skip right to
                     * rlc_bcch and hope for the best. */
                    next_tvb = tvb_new_subset_length(tvb, offset, (gint)pdu_length[n]);
                    call_dissector_with_data(rlc_bcch_handle, next_tvb, pinfo, top_level_tree, data);
                    offset += (gint)pdu_length[n];
                } else { /* Else go for CCCH UM, this seems to work. */
                    p_fp_info->hsdsch_entity = ehs; /* HSDSCH type 2 */
                    /* TODO: use cur_tb or subnum everywhere. */
                    p_fp_info->cur_tb = j; /* set cur_tb for MAC */
                    pinfo->fd->subnum = j; /* set subframe number for RRC */
                    macinf->content[j] = MAC_CONTENT_CCCH;
                    macinf->lchid[j] = (guint8)lchid[n]+1; /*Add 1 since it is zero indexed? */
                    macinf->macdflow_id[j] = p_fp_info->hsdsch_macflowd_id;
                    macinf->ctmux[j] = FALSE;

                    rlcinf->li_size[j] = RLC_LI_7BITS;
                    rlcinf->ciphered[j] = FALSE;
                    rlcinf->deciphered[j] = FALSE;
                    rlcinf->rbid[j] = (guint8)lchid[n]+1;
                    rlcinf->urnti[j] = p_fp_info->channel; /*We need to fake urnti*/

                    next_tvb = tvb_new_subset_length(tvb, offset, (gint)pdu_length[n]);
                    call_dissector_with_data(mac_fdd_hsdsch_handle, next_tvb, pinfo, top_level_tree, data);

                    offset += (gint)pdu_length[n];
                }
            }
        }

        /* New IE Flags */
        newieflags = tvb_get_guint8(tvb, offset);
        /* If newieflags == 0000 0010 then this indicates that there is a
         * HS-DSCH physical layer category and no other New IE flags. */
        if (newieflags == 2) {
            /* HS-DSCH physical layer category presence bit. */
            proto_tree_add_uint(tree, hf_fp_hsdsch_new_ie_flag[6], tvb, offset, 1, newieflags);
            offset++;
            /* HS-DSCH physical layer category. */
            proto_tree_add_bits_item(tree, hf_fp_hsdsch_physical_layer_category, tvb, offset*8, 6, ENC_BIG_ENDIAN);
            offset++;
        }
        if (preferences_header_checksum) {
            verify_header_crc(tvb, pinfo, header_crc_pi, header_crc, header_length);
        }
        /* Spare Extension and Payload CRC */
        dissect_spare_extension_and_crc(tvb, pinfo, tree, 1, offset, header_length);
    }
}
static gboolean
heur_dissect_fp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    struct fp_info *p_fp_info;

    p_fp_info = (fp_info *)p_get_proto_data(wmem_file_scope(), pinfo, proto_fp, 0);

    /* if no FP info is present, this might be FP in a pcap(ng) file */
    if (!p_fp_info) {
        /* We only know the header length of control frames, so check that bit first */
        int offset = 0, length;
        guint8 oct, calc_crc = 0, crc;
        unsigned char *buf;

        oct = tvb_get_guint8(tvb, offset);
        crc = oct & 0xfe;
        if ((oct & 0x01) == 1) {
            /*
             * 6.3.2.1 Frame CRC
             * Description: It is the result of the CRC applied to the remaining part of the frame,
             * i.e. from bit 0 of the first byte of the header (the FT IE) to bit 0 of the last byte of the payload,
             * with the corresponding generator polynomial: G(D) = D7+D6+D2+1. See subclause 7.2.
             */
            length =  tvb_reported_length(tvb);
            buf = (unsigned char *)tvb_memdup(wmem_packet_scope(), tvb, 0, length);
            buf[0] = 01;

            calc_crc = crc7update(calc_crc, buf, length);
            if (calc_crc == crc) {
                /* assume this is FP, set conversatio dissector to catch the data frames too */
                conversation_set_dissector(find_or_create_conversation(pinfo), fp_handle);
                dissect_fp(tvb, pinfo, tree, data);
                return TRUE;
            }
        }
        return FALSE;
    }

    /* if FP info is present, check that it really is an ethernet link */
    if (p_fp_info->link_type != FP_Link_Ethernet) {
        return FALSE;
    }

    /* discriminate 'lower' UDP layer from 'user data' UDP layer
     * (i.e. if an FP over UDP packet contains a user UDP packet */
    if (p_fp_info->srcport != pinfo->srcport ||
        p_fp_info->destport != pinfo->destport)
        return FALSE;

    /* assume this is FP */
    dissect_fp(tvb, pinfo, tree, data);
    return TRUE;
}
static guint8 fakes =5; /*[] ={1,5,8};*/
static guint8 fake_map[256];

 /*
 * TODO: This need to be fixed!
 * Basically you would want the actual RRC messages, that sooner or later maps
 * transport channel id's to logical id's or RAB IDs
 * to set the proper logical channel/RAB ID, but for now we make syntethic ones.
 * */

static guint8
make_fake_lchid(packet_info *pinfo _U_, gint trchld)
{
    if ( fake_map[trchld] == 0) {
        fake_map[trchld] = fakes;
        fakes++;
    }
    return fake_map[trchld];
}

/*
 * july 2012:
 * Alot of configuration has been move into the actual dissecting functions
 * since most of the configuration/signalign has to be set per tb (pdu) rather
 * for the channel!
 */
static fp_info *
fp_set_per_packet_inf_from_conv(umts_fp_conversation_info_t *p_conv_data,
                                tvbuff_t *tvb, packet_info *pinfo,
                                proto_tree *tree _U_)
{
    fp_info  *fpi;
    guint8    tfi, c_t;
    int       offset = 0, i=0, j=0, num_tbs, chan, tb_size, tb_bit_off;
    gboolean  is_control_frame;
    umts_mac_info *macinf;
    rlc_info *rlcinf;
    guint8 fake_lchid=0;
    gint *cur_val=NULL;

    fpi = wmem_new0(wmem_file_scope(), fp_info);
    p_add_proto_data(wmem_file_scope(), pinfo, proto_fp, 0, fpi);

    fpi->iface_type = p_conv_data->iface_type;
    fpi->division = p_conv_data->division;
    fpi->release = 7;               /* Set values greater then the checks performed */
    fpi->release_year = 2006;
    fpi->release_month = 12;
    fpi->channel = p_conv_data->channel;
    fpi->dch_crc_present = p_conv_data->dch_crc_present;
    /*fpi->paging_indications;*/
    fpi->link_type = FP_Link_Ethernet;

#if 0
    /*Only do this the first run, signals that we need to reset the RLC fragtable*/
    if (!pinfo->fd->flags.visited &&  p_conv_data->reset_frag ) {
        fpi->reset_frag = p_conv_data->reset_frag;
        p_conv_data->reset_frag = FALSE;
    }
#endif
    /* remember 'lower' UDP layer port information so we can later
     * differentiate 'lower' UDP layer from 'user data' UDP layer */
    fpi->srcport = pinfo->srcport;
    fpi->destport = pinfo->destport;

    fpi->com_context_id = p_conv_data->com_context_id;

    if (pinfo->link_dir == P2P_DIR_UL) {
        fpi->is_uplink = TRUE;
    } else {
        fpi->is_uplink = FALSE;
    }

    is_control_frame = tvb_get_guint8(tvb, offset) & 0x01;

    switch (fpi->channel) {
        case CHANNEL_HSDSCH: /* HS-DSCH - High Speed Downlink Shared Channel */
            fpi->hsdsch_entity = p_conv_data->hsdsch_entity;
            macinf = wmem_new0(wmem_file_scope(), umts_mac_info);
            fpi->hsdsch_macflowd_id = p_conv_data->hsdsch_macdflow_id;
           macinf->content[0] = hsdsch_macdflow_id_mac_content_map[p_conv_data->hsdsch_macdflow_id]; /*MAC_CONTENT_PS_DTCH;*/
            macinf->lchid[0] = p_conv_data->hsdsch_macdflow_id;
            /*macinf->content[0] = lchId_type_table[p_conv_data->edch_lchId[0]];*/
            p_add_proto_data(wmem_file_scope(), pinfo, proto_umts_mac, 0, macinf);

            rlcinf = wmem_new0(wmem_file_scope(), rlc_info);

            /*Figure out RLC_MODE based on MACd-flow-ID, basically MACd-flow-ID = 0 then it's SRB0 == UM else AM*/
            rlcinf->mode[0] = hsdsch_macdflow_id_rlc_map[p_conv_data->hsdsch_macdflow_id];

            if (fpi->hsdsch_entity == hs /*&& !rlc_is_ciphered(pinfo)*/) {
                for (i=0; i<MAX_NUM_HSDHSCH_MACDFLOW; i++) {
                    /*Figure out if this channel is multiplexed (signaled from RRC)*/
                    if ((cur_val=(gint *)g_tree_lookup(hsdsch_muxed_flows, GINT_TO_POINTER((gint)p_conv_data->hrnti))) != NULL) {
                        j = 1 << i;
                        fpi->hsdhsch_macfdlow_is_mux[i] = j & *cur_val;
                    } else {
                        fpi->hsdhsch_macfdlow_is_mux[i] = FALSE;
                    }

                }
            }
            /* Make configurable ?(available in NBAP?) */
            /* urnti[MAX_RLC_CHANS] */
            /*
            switch (p_conv_data->rlc_mode) {
                case FP_RLC_TM:
                    rlcinf->mode[0] = RLC_TM;
                    break;
                case FP_RLC_UM:
                    rlcinf->mode[0] = RLC_UM;
                    break;
                case FP_RLC_AM:
                    rlcinf->mode[0] = RLC_AM;
                    break;
                case FP_RLC_MODE_UNKNOWN:
                default:
                    rlcinf->mode[0] = RLC_UNKNOWN_MODE;
                    break;
            }*/
            /* rbid[MAX_RLC_CHANS] */
            /* For RLC re-assembly to work we urnti signaled from NBAP */
            rlcinf->urnti[0] = fpi->com_context_id;
            rlcinf->li_size[0] = RLC_LI_7BITS;
            rlcinf->ciphered[0] = FALSE;
            rlcinf->deciphered[0] = FALSE;
            p_add_proto_data(wmem_file_scope(), pinfo, proto_rlc, 0, rlcinf);


            return fpi;

        case CHANNEL_EDCH:
            /*Most configuration is now done in the actual dissecting function*/
            macinf = wmem_new0(wmem_file_scope(), umts_mac_info);
            rlcinf = wmem_new0(wmem_file_scope(), rlc_info);
            fpi->no_ddi_entries = p_conv_data->no_ddi_entries;
            for (i=0; i<fpi->no_ddi_entries; i++) {
                fpi->edch_ddi[i] = p_conv_data->edch_ddi[i];    /*Set the DDI value*/
                fpi->edch_macd_pdu_size[i] = p_conv_data->edch_macd_pdu_size[i];    /*Set the size*/
                fpi->edch_lchId[i] = p_conv_data->edch_lchId[i];    /*Set the channel id for this entry*/
                /*macinf->content[i] = lchId_type_table[p_conv_data->edch_lchId[i]]; */    /*Set the proper Content type for the mac layer.*/
            /*    rlcinf->mode[i] = lchId_rlc_map[p_conv_data->edch_lchId[i]];*/ /* Set RLC mode by lchid to RLC_MODE map in nbap.h */

            }
            fpi->edch_type = p_conv_data->edch_type;

           /* macinf = wmem_new0(wmem_file_scope(), umts_mac_info);
            macinf->content[0] = MAC_CONTENT_PS_DTCH;*/
            p_add_proto_data(wmem_file_scope(), pinfo, proto_umts_mac, 0, macinf);


            /* For RLC re-assembly to work we need a urnti signaled from NBAP */
            rlcinf->urnti[0] = fpi->com_context_id;
           /* rlcinf->mode[0] = RLC_AM;*/
            rlcinf->li_size[0] = RLC_LI_7BITS;
            rlcinf->ciphered[0] = FALSE;
            rlcinf->deciphered[0] = FALSE;

            p_add_proto_data(wmem_file_scope(), pinfo, proto_rlc, 0, rlcinf);

            return fpi;

        case CHANNEL_PCH:
            fpi->paging_indications = p_conv_data->paging_indications;
            fpi->num_chans = p_conv_data->num_dch_in_flow;
            /* Set offset to point to first TFI
             */
            if (is_control_frame) {
                /* control frame, we're done */
                return fpi;
            }
            /* Set offset to TFI */
            offset = 3;
            break;
        case CHANNEL_DCH:
            fpi->num_chans = p_conv_data->num_dch_in_flow;
            if (is_control_frame) {
                /* control frame, we're done */
                return fpi;
            }

            rlcinf = wmem_new0(wmem_file_scope(), rlc_info);
            macinf = wmem_new0(wmem_file_scope(), umts_mac_info);
            offset = 2;    /*To correctly read the tfi*/
            fakes  = 5; /* Reset fake counter. */
            for (chan=0; chan < fpi->num_chans; chan++) {    /*Iterate over the what channels*/
                    /*Iterate over the transport blocks*/
                   /*tfi = tvb_get_guint8(tvb, offset);*/
                   /*TFI is 5 bits according to 3GPP TS 25.321, paragraph 6.2.4.4*/
                    tfi = tvb_get_bits8(tvb, 3+offset*8, 5);

                   /*Figure out the number of tbs and size*/
                   num_tbs = (fpi->is_uplink) ? p_conv_data->fp_dch_channel_info[chan].ul_chan_num_tbs[tfi] : p_conv_data->fp_dch_channel_info[chan].dl_chan_num_tbs[tfi];
                   tb_size=  (fpi->is_uplink) ? p_conv_data->fp_dch_channel_info[i].ul_chan_tf_size[tfi] :    p_conv_data->fp_dch_channel_info[i].dl_chan_tf_size[tfi];

                    /*TODO: This stuff has to be reworked!*/
                    /*Generates a fake logical channel id for non multiplexed channel*/
                    if ( p_conv_data->dchs_in_flow_list[chan] != 31 && (p_conv_data->dchs_in_flow_list[chan] == 24 &&
                     tb_size != 340) ) {
                        fake_lchid = make_fake_lchid(pinfo, p_conv_data->dchs_in_flow_list[chan]);
                    }
                    tb_bit_off = (2+p_conv_data->num_dch_in_flow)*8;    /*Point to the C/T of first TB*/
                    /*Set configuration for individual blocks*/
                    for (j=0; j < num_tbs && j+chan < MAX_MAC_FRAMES; j++) {
                        /*Set transport channel id (useful for debugging)*/
                        macinf->trchid[j+chan] = p_conv_data->dchs_in_flow_list[chan];

                        /*Transport Channel m31 and 24 might be multiplexed!*/
                        if ( p_conv_data->dchs_in_flow_list[chan] == 31 || p_conv_data->dchs_in_flow_list[chan] == 24) {

                            /****** MUST FIGURE OUT IF THIS IS REALLY MULTIPLEXED OR NOT*******/
                            /*If Trchid == 31 and only on TB, we have no multiplexing*/
                            if (0/*p_conv_data->dchs_in_flow_list[chan] == 31 && num_tbs == 1*/) {
                                macinf->ctmux[j+chan] = FALSE;/*Set TRUE if this channel is multiplexed (ie. C/T flag exists)*/

                                macinf->lchid[j+chan] = 1;

                                macinf->content[j+chan] = lchId_type_table[1];    /*Base MAC content on logical channel id (Table is in packet-nbap.h)*/
                                rlcinf->mode[j+chan] = lchId_rlc_map[1];    /*Based RLC mode on logical channel id*/

                            }
                            /*Indicate we don't have multiplexing.*/
                            else if (p_conv_data->dchs_in_flow_list[chan] == 24 && tb_size != 340) {
                                macinf->ctmux[j+chan] = FALSE;/*Set TRUE if this channel is multiplexed (ie. C/T flag exists)*/

                                /*g_warning("settin this for %d", pinfo->num);*/
                                macinf->lchid[j+chan] = fake_lchid;
                                macinf->fake_chid[j+chan] = TRUE;
                                macinf->content[j+chan] = MAC_CONTENT_PS_DTCH; /*lchId_type_table[fake_lchid];*/    /*Base MAC content on logical channel id (Table is in packet-nbap.h)*/
                                rlcinf->mode[j+chan] = RLC_AM;/*lchId_rlc_map[fake_lchid];*/    /*Based RLC mode on logical channel id*/
                            }
                            /*We have multiplexing*/
                            else {
                                macinf->ctmux[j+chan] = TRUE;/*Set TRUE if this channel is multiplexed (ie. C/T flag exists)*/

                                /* Peek at C/T, different RLC params for different logical channels */
                                /*C/T is 4 bits according to 3GPP TS 25.321, paragraph 9.2.1, from MAC header (not FP)*/
                                c_t = (tvb_get_bits8(tvb, tb_bit_off/*(2+p_conv_data->num_dch_in_flow)*8*/, 4) + 1) % 0xf;    /* c_t = tvb_get_guint8(tvb, offset);*/
                                macinf->lchid[j+chan] = c_t;

                                macinf->content[j+chan] = lchId_type_table[c_t];    /*Base MAC content on logical channel id (Table is in packet-nbap.h)*/
                                rlcinf->mode[j+chan] = lchId_rlc_map[c_t];    /*Based RLC mode on logical channel id*/
                            }
                        } else {
                            fake_lchid = make_fake_lchid(pinfo, p_conv_data->dchs_in_flow_list[chan]);
                            macinf->ctmux[j+chan] = FALSE;/*Set TRUE if this channel is multiplexed (ie. C/T flag exists)*/
                            /*macinf->content[j+chan] = MAC_CONTENT_CS_DTCH;*/
                            macinf->content[j+chan] = lchId_type_table[fake_lchid];


                            rlcinf->mode[j+chan] = lchId_rlc_map[fake_lchid];

                            /*Generate virtual logical channel id*/
                            /************************/
                            /*TODO: Once proper lchid is always set, this has to be removed*/
                            macinf->fake_chid[j+chan] = TRUE;
                            macinf->lchid[j+chan] = fake_lchid;  /*make_fake_lchid(pinfo, p_conv_data->dchs_in_flow_list[chan]);*/
                            /************************/
                        }

                        /*** Set rlc info ***/
                        rlcinf->urnti[j+chan] = p_conv_data->com_context_id;
                        rlcinf->li_size[j+chan] = RLC_LI_7BITS;
#if 0
                        /*If this entry exists, SECRUITY_MODE is completed (signled by RRC)*/
                        if ( rrc_ciph_inf && g_tree_lookup(rrc_ciph_inf, GINT_TO_POINTER((gint)p_conv_data->com_context_id)) != NULL ) {
                            rlcinf->ciphered[j+chan] = TRUE;
                        } else {
                            rlcinf->ciphered[j+chan] = FALSE;
                        }
#endif
                        rlcinf->ciphered[j+chan] = FALSE;
                        rlcinf->deciphered[j+chan] = FALSE;
                        rlcinf->rbid[j+chan] = macinf->lchid[j+chan];


                        /*Step over this TB and it's C/T flag.*/
                        tb_bit_off += tb_size+4;
                    }

                    offset++;
            }
            p_add_proto_data(wmem_file_scope(), pinfo, proto_umts_mac, 0, macinf);
            p_add_proto_data(wmem_file_scope(), pinfo, proto_rlc, 0, rlcinf);
            /* Set offset to point to first TFI
             * the Number of TFI's = number of DCH's in the flow
             */
            offset = 2;
            break;
        case CHANNEL_FACH_FDD:
            fpi->num_chans = p_conv_data->num_dch_in_flow;
            if (is_control_frame) {
                /* control frame, we're done */
                return fpi;
            }
            /* Set offset to point to first TFI
             * the Number of TFI's = number of DCH's in the flow
             */
            offset = 2;
            /* Set MAC data */
            macinf = wmem_new0(wmem_file_scope(), umts_mac_info);
            macinf->ctmux[0]   = 1;
            macinf->content[0] = MAC_CONTENT_DCCH;
            p_add_proto_data(wmem_file_scope(), pinfo, proto_umts_mac, 0, macinf);
            /* Set RLC data */
            rlcinf = wmem_new0(wmem_file_scope(), rlc_info);
            /* Make configurable ?(avaliable in NBAP?) */
            /* For RLC re-assembly to work we need to fake urnti */
            rlcinf->urnti[0] = fpi->channel;
            rlcinf->mode[0] = RLC_AM;
            /* rbid[MAX_RLC_CHANS] */
            rlcinf->li_size[0] = RLC_LI_7BITS;
            rlcinf->ciphered[0] = FALSE;
            rlcinf->deciphered[0] = FALSE;
            p_add_proto_data(wmem_file_scope(), pinfo, proto_rlc, 0, rlcinf);
            break;

        case CHANNEL_RACH_FDD:
            fpi->num_chans = p_conv_data->num_dch_in_flow;
            if (is_control_frame) {
                /* control frame, we're done */
                return fpi;
            }
            /* Set offset to point to first TFI
             * the Number of TFI's = number of DCH's in the flow
             */
            offset = 2;
            /* set MAC data */
            macinf = wmem_new0(wmem_file_scope(), umts_mac_info);
            rlcinf = wmem_new0(wmem_file_scope(), rlc_info);
            for ( chan = 0; chan < fpi->num_chans; chan++ ) {
                    macinf->ctmux[chan]   = 1;
                    macinf->content[chan] = MAC_CONTENT_DCCH;
                    rlcinf->urnti[chan] = fpi->com_context_id;    /*Note that MAC probably will change this*/
            }



            p_add_proto_data(wmem_file_scope(), pinfo, proto_umts_mac, 0, macinf);
            p_add_proto_data(wmem_file_scope(), pinfo, proto_rlc, 0, rlcinf);
            break;
        case CHANNEL_HSDSCH_COMMON:
                rlcinf = wmem_new0(wmem_file_scope(), rlc_info);
                macinf = wmem_new0(wmem_file_scope(), umts_mac_info);
                p_add_proto_data(wmem_file_scope(), pinfo, proto_umts_mac, 0, macinf);
                p_add_proto_data(wmem_file_scope(), pinfo, proto_rlc, 0, rlcinf);
            break;
        default:
            expert_add_info(pinfo, NULL, &ei_fp_transport_channel_type_unknown);
            return NULL;
    }

    /* Peek at the packet as the per packet info seems not to take the tfi into account */
    for (i=0; i<fpi->num_chans; i++) {
        tfi = tvb_get_guint8(tvb, offset);

        /*TFI is 5 bits according to 3GPP TS 25.321, paragraph 6.2.4.4*/
        /*tfi = tvb_get_bits8(tvb, offset*8, 5);*/
        if (pinfo->link_dir == P2P_DIR_UL) {
            fpi->chan_tf_size[i] = p_conv_data->fp_dch_channel_info[i].ul_chan_tf_size[tfi];
            fpi->chan_num_tbs[i] = p_conv_data->fp_dch_channel_info[i].ul_chan_num_tbs[tfi];
        } else {
            fpi->chan_tf_size[i] = p_conv_data->fp_dch_channel_info[i].dl_chan_tf_size[tfi];
            fpi->chan_num_tbs[i] = p_conv_data->fp_dch_channel_info[i].dl_chan_num_tbs[tfi];
        }
        offset++;
    }


    return fpi;
}

/*****************************/
/* Main dissection function. */
static int
dissect_fp_common(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data)
{
    proto_tree       *fp_tree;
    proto_item       *ti;
    gint              offset = 0;
    struct fp_info   *p_fp_info;
    rlc_info         *rlcinf;
    conversation_t   *p_conv;
    umts_fp_conversation_info_t *p_conv_data = NULL;

    /* Append this protocol name rather than replace. */
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "FP");

    /* Create fp tree. */
    ti = proto_tree_add_item(tree, proto_fp, tvb, offset, -1, ENC_NA);
    fp_tree = proto_item_add_subtree(ti, ett_fp);

    top_level_tree = tree;

    /* Look for packet info! */
    p_fp_info = (struct fp_info *)p_get_proto_data(wmem_file_scope(), pinfo, proto_fp, 0);

    /* Check if we have conversation info */
    p_conv = (conversation_t *)find_conversation(pinfo->num, &pinfo->net_dst, &pinfo->net_src,
                               pinfo->ptype,
                               pinfo->destport, pinfo->srcport, NO_ADDR_B);


    if (p_conv) {
         /*Find correct conversation, basically find the one that's closest to this frame*/
#if 0
         while (p_conv->next != NULL && p_conv->next->setup_frame < pinfo->num) {
            p_conv = p_conv->next;
         }
#endif

        p_conv_data = (umts_fp_conversation_info_t *)conversation_get_proto_data(p_conv, proto_fp);

        if (p_conv_data) {
            /*Figure out the direction of the link*/
            if (addresses_equal(&(pinfo->net_dst), (&p_conv_data->crnc_address))) {

                proto_item *item= proto_tree_add_uint(fp_tree, hf_fp_ul_setup_frame,
                                                      tvb, 0, 0, p_conv_data->ul_frame_number);

                PROTO_ITEM_SET_GENERATED(item);
                /* CRNC -> Node B */
                pinfo->link_dir=P2P_DIR_UL;
                if (p_fp_info == NULL) {
                    p_fp_info = fp_set_per_packet_inf_from_conv(p_conv_data, tvb, pinfo, fp_tree);
                }
            }
            else {
                /* Maybe the frame number should be stored in the proper location already in nbap?, in ul_frame_number*/
                proto_item *item= proto_tree_add_uint(fp_tree, hf_fp_dl_setup_frame,
                                                       tvb, 0, 0, p_conv_data->ul_frame_number);

                PROTO_ITEM_SET_GENERATED(item);
                pinfo->link_dir=P2P_DIR_DL;
                if (p_fp_info == NULL) {
                    p_fp_info = fp_set_per_packet_inf_from_conv(p_conv_data, tvb, pinfo, fp_tree);
                }
            }
        }

    }

    if (pinfo->p2p_dir == P2P_DIR_UNKNOWN) {
        if (pinfo->link_dir == P2P_DIR_UL) {
            pinfo->p2p_dir = P2P_DIR_RECV;
        } else {
            pinfo->p2p_dir = P2P_DIR_SENT;
        }
    }

    /* Can't dissect anything without it... */
    if (p_fp_info == NULL) {
        proto_tree_add_expert(fp_tree, pinfo, &ei_fp_no_per_frame_info, tvb, offset, -1);
        return 1;
    }

    rlcinf = (rlc_info *)p_get_proto_data(wmem_file_scope(), pinfo, proto_rlc, 0);
    if (!rlcinf) {
        rlcinf = wmem_new0(wmem_packet_scope(), rlc_info);
    }

    /* Show release information */
    if (preferences_show_release_info) {
        proto_item *release_ti;
        proto_tree *release_tree;
        proto_item *temp_ti;

        release_ti = proto_tree_add_item(fp_tree, hf_fp_release, tvb, 0, 0, ENC_NA);
        PROTO_ITEM_SET_GENERATED(release_ti);
        proto_item_append_text(release_ti, " R%u (%d/%d)",
                               p_fp_info->release, p_fp_info->release_year, p_fp_info->release_month);
        release_tree = proto_item_add_subtree(release_ti, ett_fp_release);

        temp_ti = proto_tree_add_uint(release_tree, hf_fp_release_version, tvb, 0, 0, p_fp_info->release);
        PROTO_ITEM_SET_GENERATED(temp_ti);

        temp_ti = proto_tree_add_uint(release_tree, hf_fp_release_year, tvb, 0, 0, p_fp_info->release_year);
        PROTO_ITEM_SET_GENERATED(temp_ti);

        temp_ti = proto_tree_add_uint(release_tree, hf_fp_release_month, tvb, 0, 0, p_fp_info->release_month);
        PROTO_ITEM_SET_GENERATED(temp_ti);
    }

    /* Show channel type in info column, tree */
    col_set_str(pinfo->cinfo, COL_INFO,
                val_to_str_const(p_fp_info->channel,
                                 channel_type_vals,
                                 "Unknown channel type"));
    if (p_conv_data) {
        int i;
        col_append_fstr(pinfo->cinfo, COL_INFO, "(%u", p_conv_data->dchs_in_flow_list[0]);
        for (i=1; i < p_conv_data->num_dch_in_flow; i++) {
            col_append_fstr(pinfo->cinfo, COL_INFO, ",%u", p_conv_data->dchs_in_flow_list[i]);
        }
        col_append_fstr(pinfo->cinfo, COL_INFO, ") ");
    }
    proto_item_append_text(ti, " (%s)",
                           val_to_str_const(p_fp_info->channel,
                                            channel_type_vals,
                                            "Unknown channel type"));

    /* Add channel type as a generated field */
    ti = proto_tree_add_uint(fp_tree, hf_fp_channel_type, tvb, 0, 0, p_fp_info->channel);
    PROTO_ITEM_SET_GENERATED(ti);

    /* Add division type as a generated field */
    if (p_fp_info->release == 7) {
        ti = proto_tree_add_uint(fp_tree, hf_fp_division, tvb, 0, 0, p_fp_info->division);
        PROTO_ITEM_SET_GENERATED(ti);
    }

    /* Add link direction as a generated field */
    ti = proto_tree_add_uint(fp_tree, hf_fp_direction, tvb, 0, 0, p_fp_info->is_uplink);
    PROTO_ITEM_SET_GENERATED(ti);

    /* Don't currently handle IuR-specific formats, but it's useful to even see
       the channel type and direction */
    if (p_fp_info->iface_type == IuR_Interface) {
        return 1;
    }

    /* Show DDI config info */
    if (p_fp_info->no_ddi_entries > 0) {
        int n;
        proto_item *ddi_config_ti;
        proto_tree *ddi_config_tree;

        ddi_config_ti = proto_tree_add_string_format(fp_tree, hf_fp_ddi_config, tvb, offset, 0,
                                                     "", "DDI Config (");
        PROTO_ITEM_SET_GENERATED(ddi_config_ti);
        ddi_config_tree = proto_item_add_subtree(ddi_config_ti, ett_fp_ddi_config);

        /* Add each entry */
        for (n=0; n < p_fp_info->no_ddi_entries; n++) {
            proto_item_append_text(ddi_config_ti, "%s%u->%ubits",
                                   (n == 0) ? "" : "  ",
                                   p_fp_info->edch_ddi[n], p_fp_info->edch_macd_pdu_size[n]);
            ti = proto_tree_add_uint(ddi_config_tree, hf_fp_ddi_config_ddi, tvb, 0, 0,
                                p_fp_info->edch_ddi[n]);
            PROTO_ITEM_SET_GENERATED(ti);
            ti = proto_tree_add_uint(ddi_config_tree, hf_fp_ddi_config_macd_pdu_size, tvb, 0, 0,
                                p_fp_info->edch_macd_pdu_size[n]);
            PROTO_ITEM_SET_GENERATED(ti);

        }
        proto_item_append_text(ddi_config_ti, ")");
    }

    /*************************************/
    /* Dissect according to channel type */
    switch (p_fp_info->channel) {
        case CHANNEL_RACH_TDD:
        case CHANNEL_RACH_TDD_128:
        case CHANNEL_RACH_FDD:
            dissect_rach_channel_info(tvb, pinfo, fp_tree, offset, p_fp_info,
                                      data);
            break;
        case CHANNEL_DCH:
            dissect_dch_channel_info(tvb, pinfo, fp_tree, offset, p_fp_info,
                                     data);
            break;
        case CHANNEL_FACH_FDD:
        case CHANNEL_FACH_TDD:
            dissect_fach_channel_info(tvb, pinfo, fp_tree, offset, p_fp_info,
                                      data);
            break;
        case CHANNEL_DSCH_FDD:
        case CHANNEL_DSCH_TDD:
            dissect_dsch_channel_info(tvb, pinfo, fp_tree, offset, p_fp_info);
            break;
        case CHANNEL_USCH_TDD_128:
        case CHANNEL_USCH_TDD_384:
            dissect_usch_channel_info(tvb, pinfo, fp_tree, offset, p_fp_info);
            break;
        case CHANNEL_PCH:
            dissect_pch_channel_info(tvb, pinfo, fp_tree, offset, p_fp_info,
                                     data);
            break;
        case CHANNEL_CPCH:
            dissect_cpch_channel_info(tvb, pinfo, fp_tree, offset, p_fp_info);
            break;
        case CHANNEL_BCH:
            dissect_bch_channel_info(tvb, pinfo, fp_tree, offset, p_fp_info);
            break;
        case CHANNEL_HSDSCH:
            /* Show configured MAC HS-DSCH entity in use */
            if (fp_tree)
            {
                proto_item *entity_ti;
                entity_ti = proto_tree_add_uint(fp_tree, hf_fp_hsdsch_entity,
                                                tvb, 0, 0,
                                                p_fp_info->hsdsch_entity);
                PROTO_ITEM_SET_GENERATED(entity_ti);
            }
            switch (p_fp_info->hsdsch_entity) {
                case entity_not_specified:
                case hs:
                    /* This is the pre-R7 default */
                    dissect_hsdsch_channel_info(tvb, pinfo, fp_tree, offset, p_fp_info, data);
                    break;
                case ehs:
                    dissect_hsdsch_type_2_channel_info(tvb, pinfo, fp_tree, offset, p_fp_info, data);
                    break;
                default:
                    /* Report Error */
                    expert_add_info(pinfo, NULL, &ei_fp_hsdsch_entity_not_specified);
                    break;
            }
            break;
        case CHANNEL_HSDSCH_COMMON:
            expert_add_info(pinfo, NULL, &ei_fp_hsdsch_common_experimental_support);
            /*if (FALSE)*/
            dissect_hsdsch_common_channel_info(tvb, pinfo, fp_tree, offset, p_fp_info, data);

            break;
        case CHANNEL_HSDSCH_COMMON_T3:
            expert_add_info(pinfo, NULL, &ei_fp_hsdsch_common_t3_not_implemented);

            /* TODO: */
            break;
        case CHANNEL_IUR_CPCHF:
            /* TODO: */
            break;
        case CHANNEL_IUR_FACH:
            /* TODO: */
            break;
        case CHANNEL_IUR_DSCH:
            dissect_iur_dsch_channel_info(tvb, pinfo, fp_tree, offset, p_fp_info);
            break;
        case CHANNEL_EDCH:
        case CHANNEL_EDCH_COMMON:
            dissect_e_dch_channel_info(tvb, pinfo, fp_tree, offset, p_fp_info,
                                       p_fp_info->channel == CHANNEL_EDCH_COMMON,
                                       rlcinf, data);
            break;

        default:
            expert_add_info(pinfo, NULL, &ei_fp_channel_type_unknown);
            break;
    }
    return tvb_captured_length(tvb);
}

static int
dissect_fp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    return dissect_fp_common(tvb, pinfo, tree, NULL);
}

static int
dissect_fp_aal2(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data)
{
    return dissect_fp_common(tvb, pinfo, tree, data);
}

#ifdef UMTS_FP_USE_UAT
UAT_VS_DEF(uat_umts_fp_ep_and_ch_records, protocol, uat_umts_fp_ep_and_ch_record_t, guint8, UMTS_FP_IPV4, "IPv4")
UAT_CSTRING_CB_DEF(uat_umts_fp_ep_and_ch_records, srcIP, uat_umts_fp_ep_and_ch_record_t)
UAT_DEC_CB_DEF(uat_umts_fp_ep_and_ch_records, src_port, uat_umts_fp_ep_and_ch_record_t)
UAT_CSTRING_CB_DEF(uat_umts_fp_ep_and_ch_records, dstIP, uat_umts_fp_ep_and_ch_record_t)
UAT_DEC_CB_DEF(uat_umts_fp_ep_and_ch_records, dst_port, uat_umts_fp_ep_and_ch_record_t)
UAT_VS_DEF(uat_umts_fp_ep_and_ch_records, interface_type, uat_umts_fp_ep_and_ch_record_t, guint8, IuB_Interface, "IuB Interface")
UAT_VS_DEF(uat_umts_fp_ep_and_ch_records, division, uat_umts_fp_ep_and_ch_record_t, guint8, Division_FDD, "Division FDD")
UAT_VS_DEF(uat_umts_fp_ep_and_ch_records, rlc_mode, uat_umts_fp_ep_and_ch_record_t, guint8, FP_RLC_TM, "RLC mode")
UAT_VS_DEF(uat_umts_fp_ep_and_ch_records, channel_type, uat_umts_fp_ep_and_ch_record_t, guint8, CHANNEL_RACH_FDD, "RACH FDD")

static void *uat_umts_fp_record_copy_cb(void *n, const void *o, size_t siz _U_) {
    uat_umts_fp_ep_and_ch_record_t *new_rec = (uat_umts_fp_ep_and_ch_record_t *)n;
    const uat_umts_fp_ep_and_ch_record_t *old_rec = (const uat_umts_fp_ep_and_ch_record_t *)o;

    new_rec->srcIP = (old_rec->srcIP) ? g_strdup(old_rec->srcIP) : NULL;
    new_rec->dstIP = (old_rec->dstIP) ? g_strdup(old_rec->dstIP) : NULL;

    return new_rec;
}

static void uat_umts_fp_record_free_cb(void*r) {
    uat_umts_fp_ep_and_ch_record_t *rec = (uat_umts_fp_ep_and_ch_record_t *)r;

    g_free(rec->srcIP);
    g_free(rec->dstIP);
}
/*
 * Set up UAT predefined conversations for specified channels
 *  typedef struct {
 *    guint8 protocol;
 *    gchar *srcIP;
 *    guint16 src_port;
 *    gchar *dstIP;
 *    guint16 dst_port;
 *    guint8 interface_type;
 *    guint8 division;
 *    guint8 rlc_mode;
 *    guint8 channel_type;
 * } uat_umts_fp_ep_and_ch_record_t;
 */
static void
umts_fp_init_protocol(void)
{
    guint32 hosta_addr[4];
    guint32 hostb_addr[4];
    address     src_addr, dst_addr;
    conversation_t *conversation;
    umts_fp_conversation_info_t *umts_fp_conversation_info;
    guint i, j, num_tf;

    for (i=0; i<num_umts_fp_ep_and_ch_items; i++) {
        /* check if we have a conversation allready */

        /* Convert the strings to ADDR */
        if (uat_umts_fp_ep_and_ch_records[i].protocol == UMTS_FP_IPV4) {
            if ((uat_umts_fp_ep_and_ch_records[i].srcIP) && (!str_to_ip(uat_umts_fp_ep_and_ch_records[i].srcIP, &hosta_addr))) {
                continue; /* parsing failed, skip this entry */
            }
            if ((uat_umts_fp_ep_and_ch_records[i].dstIP) && (!str_to_ip(uat_umts_fp_ep_and_ch_records[i].dstIP, &hostb_addr))) {
                continue; /* parsing failed, skip this entry */
            }
            set_address(&src_addr, AT_IPv4, 4, &hosta_addr);
            set_address(&dst_addr, AT_IPv4, 4, &hostb_addr);
        } else {
            continue; /* Not implemented yet */
        }
        conversation = find_conversation(1, &src_addr, &dst_addr, PT_UDP, uat_umts_fp_ep_and_ch_records[i].src_port, 0, NO_ADDR2|NO_PORT2);
        if (conversation == NULL) {
            /* It's not part of any conversation - create a new one. */
            conversation = conversation_new(1, &src_addr, &dst_addr, PT_UDP, uat_umts_fp_ep_and_ch_records[i].src_port, 0, NO_ADDR2|NO_PORT2);
            if (conversation == NULL)
                continue;
            conversation_set_dissector(conversation, fp_handle);
            switch (uat_umts_fp_ep_and_ch_records[i].channel_type) {
            case CHANNEL_RACH_FDD:
                /* set up conversation info for RACH FDD channels */
                umts_fp_conversation_info = wmem_new0(wmem_file_scope(), umts_fp_conversation_info_t);
                /* Fill in the data */
                umts_fp_conversation_info->iface_type        = (enum fp_interface_type)uat_umts_fp_ep_and_ch_records[i].interface_type;
                umts_fp_conversation_info->division          = (enum division_type) uat_umts_fp_ep_and_ch_records[i].division;
                umts_fp_conversation_info->channel           = uat_umts_fp_ep_and_ch_records[i].channel_type;
                umts_fp_conversation_info->dl_frame_number   = 0;
                umts_fp_conversation_info->ul_frame_number   = 1;
                copy_address_wmem(wmem_file_scope(), &(umts_fp_conversation_info->crnc_address), &src_addr);
                umts_fp_conversation_info->crnc_port         = uat_umts_fp_ep_and_ch_records[i].src_port;
                umts_fp_conversation_info->rlc_mode          = (enum fp_rlc_mode) uat_umts_fp_ep_and_ch_records[i].rlc_mode;
                /*Save unique UE-identifier */
                umts_fp_conversation_info->com_context_id = 1;


                /* DCH's in this flow */
                umts_fp_conversation_info->dch_crc_present = 2;
                /* Set data for First or single channel */
                umts_fp_conversation_info->fp_dch_channel_info[0].num_ul_chans = num_tf = 1;

                for (j = 0; j < num_tf; j++) {
                    umts_fp_conversation_info->fp_dch_channel_info[0].ul_chan_tf_size[j] = 168;
                    umts_fp_conversation_info->fp_dch_channel_info[0].ul_chan_num_tbs[j] = 1;
                }

                umts_fp_conversation_info->dchs_in_flow_list[0] = 1;
                umts_fp_conversation_info->num_dch_in_flow=1;
                set_umts_fp_conv_data(conversation, umts_fp_conversation_info);
            default:
                break;
            }
        }
    }
}


#endif /* UMTS_FP_USE_UAT */

void proto_register_fp(void)
{
    static hf_register_info hf[] =
        {
            { &hf_fp_release,
              { "Release",
                "fp.release", FT_NONE, BASE_NONE, NULL, 0x0,
                "Release information", HFILL
              }
            },
            { &hf_fp_release_version,
              { "Release Version",
                "fp.release.version", FT_UINT8, BASE_DEC, NULL, 0x0,
                "3GPP Release number", HFILL
              }
            },
            { &hf_fp_release_year,
              { "Release year",
                "fp.release.year", FT_UINT16, BASE_DEC, NULL, 0x0,
                NULL, HFILL
              }
            },
            { &hf_fp_release_month,
              { "Release month",
                "fp.release.month", FT_UINT8, BASE_DEC, NULL, 0x0,
                NULL, HFILL
              }
            },
            { &hf_fp_channel_type,
              { "Channel Type",
                "fp.channel-type", FT_UINT8, BASE_HEX, VALS(channel_type_vals), 0x0,
                NULL, HFILL
              }
            },
            { &hf_fp_division,
              { "Division",
                "fp.division", FT_UINT8, BASE_HEX, VALS(division_vals), 0x0,
                "Radio division type", HFILL
              }
            },
            { &hf_fp_direction,
              { "Direction",
                "fp.direction", FT_UINT8, BASE_HEX, VALS(direction_vals), 0x0,
                "Link direction", HFILL
              }
            },
            { &hf_fp_ddi_config,
              { "DDI Config",
                "fp.ddi-config", FT_STRING, BASE_NONE, NULL, 0x0,
                "DDI Config (for E-DCH)", HFILL
              }
            },
            { &hf_fp_ddi_config_ddi,
              { "DDI",
                "fp.ddi-config.ddi", FT_UINT8, BASE_DEC, NULL, 0x0,
                NULL, HFILL
              }
            },
            { &hf_fp_ddi_config_macd_pdu_size,
              { "MACd PDU Size",
                "fp.ddi-config.macd-pdu-size", FT_UINT16, BASE_DEC, NULL, 0x0,
                NULL, HFILL
              }
            },


            { &hf_fp_header_crc,
              { "Header CRC",
                "fp.header-crc", FT_UINT8, BASE_HEX, NULL, 0xfe,
                NULL, HFILL
              }
            },
            { &hf_fp_ft,
              { "Frame Type",
                "fp.ft", FT_UINT8, BASE_HEX, VALS(data_control_vals), 0x01,
                NULL, HFILL
              }
            },
            { &hf_fp_cfn,
              { "CFN",
                "fp.cfn", FT_UINT8, BASE_DEC, NULL, 0x0,
                "Connection Frame Number", HFILL
              }
            },
            { &hf_fp_pch_cfn,
              { "CFN (PCH)",
                "fp.pch.cfn", FT_UINT16, BASE_DEC, NULL, 0xfff0,
                "PCH Connection Frame Number", HFILL
              }
            },
            { &hf_fp_pch_toa,
              { "ToA (PCH)",
                "fp.pch.toa", FT_INT24, BASE_DEC, NULL, 0x0,
                "PCH Time of Arrival", HFILL
              }
            },
            { &hf_fp_cfn_control,
              { "CFN control",
                "fp.cfn-control", FT_UINT8, BASE_DEC, NULL, 0x0,
                "Connection Frame Number Control", HFILL
              }
            },
            { &hf_fp_toa,
              { "ToA",
                "fp.toa", FT_INT16, BASE_DEC, NULL, 0x0,
                "Time of arrival (units are 125 microseconds)", HFILL
              }
            },
            { &hf_fp_tb,
              { "TB",
                "fp.tb", FT_BYTES, BASE_NONE, NULL, 0x0,
                "Transport Block", HFILL
              }
            },
            { &hf_fp_chan_zero_tbs,
              { "No TBs for channel",
                "fp.channel-with-zero-tbs", FT_UINT32, BASE_DEC, NULL, 0x0,
                "Channel with 0 TBs", HFILL
              }
            },
            { &hf_fp_tfi,
              { "TFI",
                "fp.tfi", FT_UINT8, BASE_DEC, NULL, 0x0,
                "Transport Format Indicator", HFILL
              }
            },
            { &hf_fp_usch_tfi,
              { "TFI",
                "fp.usch.tfi", FT_UINT8, BASE_DEC, NULL, 0x1f,
                "USCH Transport Format Indicator", HFILL
              }
            },
            { &hf_fp_cpch_tfi,
              { "TFI",
                "fp.cpch.tfi", FT_UINT8, BASE_DEC, NULL, 0x1f,
                "CPCH Transport Format Indicator", HFILL
              }
            },
            { &hf_fp_propagation_delay,
              { "Propagation Delay",
                "fp.propagation-delay", FT_UINT8, BASE_DEC, NULL, 0x0,
                NULL, HFILL
              }
            },
            { &hf_fp_dch_control_frame_type,
              { "Control Frame Type",
                "fp.dch.control.frame-type", FT_UINT8, BASE_HEX, VALS(dch_control_frame_type_vals), 0x0,
                "DCH Control Frame Type", HFILL
              }
            },
            { &hf_fp_dch_rx_timing_deviation,
              { "Rx Timing Deviation",
                "fp.dch.control.rx-timing-deviation", FT_UINT8, BASE_DEC, 0, 0x0,
                "DCH Rx Timing Deviation", HFILL
              }
            },
            { &hf_fp_quality_estimate,
              { "Quality Estimate",
                "fp.dch.quality-estimate", FT_UINT8, BASE_DEC, 0, 0x0,
                NULL, HFILL
              }
            },
            { &hf_fp_payload_crc,
              { "Payload CRC",
                "fp.payload-crc", FT_UINT16, BASE_HEX, 0, 0x0,
                NULL, HFILL
              }
            },
            { &hf_fp_common_control_frame_type,
              { "Control Frame Type",
                "fp.common.control.frame-type", FT_UINT8, BASE_HEX, VALS(common_control_frame_type_vals), 0x0,
                "Common Control Frame Type", HFILL
              }
            },
            { &hf_fp_crci[0],
              { "CRCI",
                "fp.crci", FT_UINT8, BASE_HEX, VALS(crci_vals), 0x80,
                "CRC correctness indicator", HFILL
              }
            },
            { &hf_fp_crci[1],
              { "CRCI",
                "fp.crci", FT_UINT8, BASE_HEX, VALS(crci_vals), 0x40,
                "CRC correctness indicator", HFILL
              }
            },
            { &hf_fp_crci[2],
              { "CRCI",
                "fp.crci", FT_UINT8, BASE_HEX, VALS(crci_vals), 0x20,
                "CRC correctness indicator", HFILL
              }
            },
            { &hf_fp_crci[3],
              { "CRCI",
                "fp.crci", FT_UINT8, BASE_HEX, VALS(crci_vals), 0x10,
                "CRC correctness indicator", HFILL
              }
            },
            { &hf_fp_crci[4],
              { "CRCI",
                "fp.crci", FT_UINT8, BASE_HEX, VALS(crci_vals), 0x08,
                "CRC correctness indicator", HFILL
              }
            },
            { &hf_fp_crci[5],
              { "CRCI",
                "fp.crci", FT_UINT8, BASE_HEX, VALS(crci_vals), 0x04,
                "CRC correctness indicator", HFILL
              }
            },
            { &hf_fp_crci[6],
              { "CRCI",
                "fp.crci", FT_UINT8, BASE_HEX, VALS(crci_vals), 0x02,
                "CRC correctness indicator", HFILL
              }
            },
            { &hf_fp_crci[7],
              { "CRCI",
                "fp.crci", FT_UINT8, BASE_HEX, VALS(crci_vals), 0x01,
                "CRC correctness indicator", HFILL
              }
            },
            { &hf_fp_received_sync_ul_timing_deviation,
              { "Received SYNC UL Timing Deviation",
                "fp.rx-sync-ul-timing-deviation", FT_UINT8, BASE_DEC, 0, 0x0,
                NULL, HFILL
              }
            },
            { &hf_fp_pch_pi,
              { "Paging Indication",
                "fp.pch.pi", FT_UINT8, BASE_DEC, VALS(paging_indication_vals), 0x01,
                "Indicates if the PI Bitmap is present", HFILL
              }
            },
            { &hf_fp_pch_tfi,
              { "TFI",
                "fp.pch.tfi", FT_UINT8, BASE_DEC, 0, 0x1f,
                "PCH Transport Format Indicator", HFILL
              }
            },
            { &hf_fp_fach_tfi,
              { "TFI",
                "fp.fach.tfi", FT_UINT8, BASE_DEC, 0, 0x1f,
                "FACH Transport Format Indicator", HFILL
              }
            },
            { &hf_fp_transmit_power_level,
              { "Transmit Power Level",
                "fp.transmit-power-level", FT_FLOAT, BASE_NONE, 0, 0x0,
                "Transmit Power Level (dB)", HFILL
              }
            },
            { &hf_fp_pdsch_set_id,
              { "PDSCH Set Id",
                "fp.pdsch-set-id", FT_UINT8, BASE_DEC, 0, 0x0,
                "A pointer to the PDSCH Set which shall be used to transmit", HFILL
              }
            },
            { &hf_fp_paging_indication_bitmap,
              { "Paging Indications bitmap",
                "fp.pch.pi-bitmap", FT_NONE, BASE_NONE, NULL, 0x0,
                "Paging Indication bitmap", HFILL
              }
            },
            { &hf_fp_rx_timing_deviation,
              { "Rx Timing Deviation",
                "fp.common.control.rx-timing-deviation", FT_UINT8, BASE_DEC, 0, 0x0,
                "Common Rx Timing Deviation", HFILL
              }
            },
            { &hf_fp_dch_e_rucch_flag,
              { "E-RUCCH Flag",
                "fp.common.control.e-rucch-flag", FT_UINT8, BASE_DEC, VALS(e_rucch_flag_vals), 0x0,
                NULL, HFILL
              }
            },
            { &hf_fp_edch_header_crc,
              { "E-DCH Header CRC",
                "fp.edch.header-crc", FT_UINT16, BASE_HEX, 0, 0,
                NULL, HFILL
              }
            },
            { &hf_fp_edch_fsn,
              { "FSN",
                "fp.edch.fsn", FT_UINT8, BASE_DEC, 0, 0x0f,
                "E-DCH Frame Sequence Number", HFILL
              }
            },
            { &hf_fp_edch_number_of_subframes,
              { "No of subframes",
                "fp.edch.no-of-subframes", FT_UINT8, BASE_DEC, 0, 0x0f,
                "E-DCH Number of subframes", HFILL
              }
            },
            { &hf_fp_edch_harq_retransmissions,
              { "No of HARQ Retransmissions",
                "fp.edch.no-of-harq-retransmissions", FT_UINT8, BASE_DEC, 0, 0x78,
                "E-DCH Number of HARQ retransmissions", HFILL
              }
            },
            { &hf_fp_edch_subframe_number,
              { "Subframe number",
                "fp.edch.subframe-number", FT_UINT8, BASE_DEC, 0, 0x0,
                "E-DCH Subframe number", HFILL
              }
            },
            { &hf_fp_edch_number_of_mac_es_pdus,
              { "Number of Mac-es PDUs",
                "fp.edch.number-of-mac-es-pdus", FT_UINT8, BASE_DEC, 0, 0xf0,
                NULL, HFILL
              }
            },
            { &hf_fp_edch_ddi,
              { "DDI",
                "fp.edch.ddi", FT_UINT8, BASE_DEC, 0, 0x0,
                "E-DCH Data Description Indicator", HFILL
              }
            },
            { &hf_fp_edch_subframe,
              { "Subframe",
                "fp.edch.subframe", FT_STRING, BASE_NONE, NULL, 0x0,
                "EDCH Subframe", HFILL
              }
            },
            { &hf_fp_edch_subframe_header,
              { "Subframe header",
                "fp.edch.subframe-header", FT_STRING, BASE_NONE, NULL, 0x0,
                "EDCH Subframe header", HFILL
              }
            },
            { &hf_fp_edch_number_of_mac_d_pdus,
              { "Number of Mac-d PDUs",
                "fp.edch.number-of-mac-d-pdus", FT_UINT8, BASE_DEC, 0, 0x0,
                NULL, HFILL
              }
            },
            { &hf_fp_edch_pdu_padding,
              { "Padding",
                "fp.edch-data-padding", FT_UINT8, BASE_DEC, 0, 0xc0,
                "E-DCH padding before PDU", HFILL
              }
            },
            { &hf_fp_edch_tsn,
              { "TSN",
                "fp.edch-tsn", FT_UINT8, BASE_DEC, 0, 0x3f,
                "E-DCH Transmission Sequence Number", HFILL
              }
            },
            { &hf_fp_edch_mac_es_pdu,
              { "MAC-es PDU",
                "fp.edch.mac-es-pdu", FT_NONE, BASE_NONE, NULL, 0x0,
                NULL, HFILL
              }
            },

            { &hf_fp_edch_user_buffer_size,
              { "User Buffer Size",
                "fp.edch.user-buffer-size", FT_UINT24, BASE_DEC, NULL, 0x0,
                NULL, HFILL
              }
            },
            { &hf_fp_edch_no_macid_sdus,
              { "No of MAC-is SDUs",
                "fp.edch.no-macis-sdus", FT_UINT16, BASE_DEC, NULL, 0x0,
                NULL, HFILL
              }
            },
            { &hf_fp_edch_number_of_mac_is_pdus,
              { "Number of Mac-is PDUs",
                "fp.edch.number-of-mac-is-pdus", FT_UINT8, BASE_DEC, 0, 0x0,
                NULL, HFILL
              }
            },
            { &hf_fp_edch_mac_is_pdu,
              { "Mac-is PDU",
                "fp.edch.mac-is-pdu", FT_BYTES, BASE_NONE, 0, 0x0,
                NULL, HFILL
              }
            },
            { &hf_fp_edch_e_rnti,
              { "E-RNTI",
                "fp.edch.e-rnti", FT_UINT16, BASE_DEC, 0, 0x0,
                NULL, HFILL
              }
            },

            { &hf_fp_edch_macis_descriptors,
              { "MAC-is Descriptors",
                "fp.edch.mac-is.descriptors", FT_STRING, BASE_NONE, NULL, 0x0,
                NULL, HFILL
              }
            },
            { &hf_fp_edch_macis_lchid,
              { "LCH-ID",
                "fp.edch.mac-is.lchid", FT_UINT8, BASE_HEX, VALS(lchid_vals), 0xf0,
                NULL, HFILL
              }
            },
            { &hf_fp_edch_macis_length,
              { "Length",
                "fp.edch.mac-is.length", FT_UINT16, BASE_DEC, 0, 0x0ffe,
                NULL, HFILL
              }
            },
            { &hf_fp_edch_macis_flag,
              { "Flag",
                "fp.edch.mac-is.lchid", FT_UINT8, BASE_HEX, 0, 0x01,
                "Indicates if another entry follows", HFILL
              }
            },
            { &hf_fp_frame_seq_nr,
              { "Frame Seq Nr",
                "fp.frame-seq-nr", FT_UINT8, BASE_DEC, 0, 0xf0,
                "Frame Sequence Number", HFILL
              }
            },
            { &hf_fp_hsdsch_pdu_block_header,
              { "PDU block header",
                "fp.hsdsch.pdu-block-header", FT_STRING, BASE_NONE, NULL, 0x0,
                "HS-DSCH type 2 PDU block header", HFILL
              }
            },
#if 0
            { &hf_fp_hsdsch_pdu_block,
              { "PDU block",
                "fp.hsdsch.pdu-block", FT_STRING, BASE_NONE, NULL, 0x0,
                "HS-DSCH type 2 PDU block data", HFILL
              }
            },
#endif
            { &hf_fp_flush,
              { "Flush",
                "fp.flush", FT_UINT8, BASE_DEC, 0, 0x04,
                "Whether all PDUs for this priority queue should be removed", HFILL
              }
            },
            { &hf_fp_fsn_drt_reset,
              { "FSN-DRT reset",
                "fp.fsn-drt-reset", FT_UINT8, BASE_DEC, 0, 0x02,
                "FSN/DRT Reset Flag", HFILL
              }
            },
            { &hf_fp_drt_indicator,
              { "DRT Indicator",
                "fp.drt-indicator", FT_UINT8, BASE_DEC, 0, 0x01,
                NULL, HFILL
              }
            },
            { &hf_fp_fach_indicator,
              { "FACH Indicator",
                "fp.fach-indicator", FT_UINT8, BASE_DEC, 0, 0x80,
                NULL, HFILL
              }
            },
            { &hf_fp_total_pdu_blocks,
              { "PDU Blocks",
                "fp.pdu_blocks", FT_UINT8, BASE_DEC, 0, 0xf8,
                "Total number of PDU blocks", HFILL
              }
            },
            { &hf_fp_drt,
              { "DelayRefTime",
                "fp.drt", FT_UINT16, BASE_DEC, 0, 0x0,
                NULL, HFILL
              }
            },
            { &hf_fp_hrnti,
              { "HRNTI",
                "fp.hrnti", FT_UINT16, BASE_DEC, 0, 0x0,
                NULL, HFILL
              }
            },
            { &hf_fp_rach_measurement_result,
              { "RACH Measurement Result",
                "fp.rach-measurement-result", FT_UINT16, BASE_DEC, 0, 0x0,
                NULL, HFILL
              }
            },
            { &hf_fp_lchid,
              { "Logical Channel ID",
                "fp.lchid", FT_UINT8, BASE_DEC, NULL, 0x0,
                NULL, HFILL
              }
            },
            { &hf_fp_pdu_length_in_block,
              { "PDU length in block",
                "fp.pdu-length-in-block", FT_UINT8, BASE_DEC, 0, 0x0,
                "Length of each PDU in this block in bytes", HFILL
              }
            },
            { &hf_fp_pdus_in_block,
              { "PDUs in block",
                "fp.no-pdus-in-block", FT_UINT8, BASE_DEC, 0, 0x0,
                "Number of PDUs in block", HFILL
              }
            },
            { &hf_fp_cmch_pi,
              { "CmCH-PI",
                "fp.cmch-pi", FT_UINT8, BASE_DEC, 0, 0x0f,
                "Common Transport Channel Priority Indicator", HFILL
              }
            },
            { &hf_fp_user_buffer_size,
              { "User buffer size",
                "fp.user-buffer-size", FT_UINT16, BASE_DEC, 0, 0x0,
                "User buffer size in octets", HFILL
              }
            },
            { &hf_fp_hsdsch_credits,
              { "HS-DSCH Credits",
                "fp.hsdsch-credits", FT_UINT16, BASE_DEC, 0, 0x0,
                NULL, HFILL
              }
            },
            { &hf_fp_hsdsch_max_macd_pdu_len,
              { "Max MAC-d PDU Length",
                "fp.hsdsch.max-macd-pdu-len", FT_UINT16, BASE_DEC, 0, 0xfff8,
                "Maximum MAC-d PDU Length in bits", HFILL
              }
            },
            { &hf_fp_hsdsch_max_macdc_pdu_len,
              { "Max MAC-d/c PDU Length",
                "fp.hsdsch.max-macdc-pdu-len", FT_UINT16, BASE_DEC, 0, 0x07ff,
                "Maximum MAC-d/c PDU Length in bits", HFILL
              }
            },
            { &hf_fp_hsdsch_interval,
              { "HS-DSCH Interval in milliseconds",
                "fp.hsdsch-interval", FT_UINT8, BASE_DEC, 0, 0x0,
                NULL, HFILL
              }
            },
            { &hf_fp_hsdsch_calculated_rate,
              { "Calculated rate allocation (bps)",
                "fp.hsdsch-calculated-rate", FT_UINT32, BASE_DEC, 0, 0x0,
                "Calculated rate RNC is allowed to send in bps", HFILL
              }
            },
            { &hf_fp_hsdsch_unlimited_rate,
              { "Unlimited rate",
                "fp.hsdsch-unlimited-rate", FT_NONE, BASE_NONE, 0, 0x0,
                "No restriction on rate at which date may be sent", HFILL
              }
            },
            { &hf_fp_hsdsch_repetition_period,
              { "HS-DSCH Repetition Period",
                "fp.hsdsch-repetition-period", FT_UINT8, BASE_DEC, 0, 0x0,
                "HS-DSCH Repetition Period in milliseconds", HFILL
              }
            },
            { &hf_fp_hsdsch_data_padding,
              { "Padding",
                "fp.hsdsch-data-padding", FT_UINT8, BASE_DEC, 0, 0xf0,
                "HS-DSCH Repetition Period in milliseconds", HFILL
              }
            },
            { &hf_fp_hsdsch_new_ie_flags,
              { "New IEs flags",
                "fp.hsdsch.new-ie-flags", FT_STRING, BASE_NONE, 0, 0x0,
                NULL, HFILL
              }
            },
            { &hf_fp_hsdsch_new_ie_flag[0],
              { "DRT IE present",
                "fp.hsdsch.new-ie-flag", FT_UINT8, BASE_DEC, 0, 0x80,
                NULL, HFILL
              }
            },
            { &hf_fp_hsdsch_new_ie_flag[1],
              { "New IE present",
                "fp.hsdsch.new-ie-flag", FT_UINT8, BASE_DEC, 0, 0x40,
                NULL, HFILL
              }
            },
            { &hf_fp_hsdsch_new_ie_flag[2],
              { "New IE present",
                "fp.hsdsch.new-ie-flag", FT_UINT8, BASE_DEC, 0, 0x20,
                NULL, HFILL
              }
            },
            { &hf_fp_hsdsch_new_ie_flag[3],
              { "New IE present",
                "fp.hsdsch.new-ie-flag", FT_UINT8, BASE_DEC, 0, 0x10,
                NULL, HFILL
              }
            },
            { &hf_fp_hsdsch_new_ie_flag[4],
              { "New IE present",
                "fp.hsdsch.new-ie-flag", FT_UINT8, BASE_DEC, 0, 0x08,
                NULL, HFILL
              }
            },
            { &hf_fp_hsdsch_new_ie_flag[5],
              { "New IE present",
                "fp.hsdsch.new-ie-flag", FT_UINT8, BASE_DEC, 0, 0x04,
                NULL, HFILL
              }
            },
            { &hf_fp_hsdsch_new_ie_flag[6],
              { "HS-DSCH physical layer category present",
                "fp.hsdsch.new-ie-flag", FT_UINT8, BASE_DEC, 0, 0x02,
                NULL, HFILL
              }
            },
            { &hf_fp_hsdsch_new_ie_flag[7],
              { "Another new IE flags byte",
                "fp.hsdsch.new-ie-flags-byte", FT_UINT8, BASE_DEC, 0, 0x01,
                "Another new IE flagsbyte", HFILL
              }
            },
            { &hf_fp_hsdsch_drt,
              { "DRT",
                "fp.hsdsch.drt", FT_UINT8, BASE_DEC, 0, 0xf0,
                "Delay Reference Time", HFILL
              }
            },
            { &hf_fp_hsdsch_entity,
              { "HS-DSCH Entity",
                "fp.hsdsch.entity", FT_UINT8, BASE_DEC, VALS(hsdshc_mac_entity_vals), 0x0,
                "Type of MAC entity for this HS-DSCH channel", HFILL
              }
            },
            { &hf_fp_timing_advance,
              { "Timing advance",
                "fp.timing-advance", FT_UINT8, BASE_DEC, 0, 0x3f,
                "Timing advance in chips", HFILL
              }
            },
            { &hf_fp_num_of_pdu,
              { "Number of PDUs",
                "fp.hsdsch.num-of-pdu", FT_UINT8, BASE_DEC, 0, 0x0,
                "Number of PDUs in the payload", HFILL
              }
            },
            { &hf_fp_mac_d_pdu_len,
              { "MAC-d PDU Length",
                "fp.hsdsch.mac-d-pdu-len", FT_UINT16, BASE_DEC, 0, 0xfff8,
                "MAC-d PDU Length in bits", HFILL
              }
            },
            { &hf_fp_mac_d_pdu,
              { "MAC-d PDU",
                "fp.mac-d-pdu", FT_BYTES, BASE_NONE, NULL, 0x0,
                NULL, HFILL
              }
            },
            { &hf_fp_data,
              { "Data",
                "fp.data", FT_BYTES, BASE_NONE, NULL, 0x0,
                NULL, HFILL
              }
            },
            { &hf_fp_crcis,
              { "CRCIs",
                "fp.crcis", FT_BYTES, BASE_NONE, NULL, 0x0,
                "CRC Indicators for uplink TBs", HFILL
              }
            },
            { &hf_fp_t1,
              { "T1",
                "fp.t1", FT_UINT24, BASE_DEC, NULL, 0x0,
                "RNC frame number indicating time it sends frame", HFILL
              }
            },
            { &hf_fp_t2,
              { "T2",
                "fp.t2", FT_UINT24, BASE_DEC, NULL, 0x0,
                "NodeB frame number indicating time it received DL Sync", HFILL
              }
            },
            { &hf_fp_t3,
              { "T3",
                "fp.t3", FT_UINT24, BASE_DEC, NULL, 0x0,
                "NodeB frame number indicating time it sends frame", HFILL
              }
            },
            { &hf_fp_ul_sir_target,
              { "UL_SIR_TARGET",
                "fp.ul-sir-target", FT_FLOAT, BASE_NONE, 0, 0x0,
                "Value (in dB) of the SIR target to be used by the UL inner loop power control", HFILL
              }
            },
            { &hf_fp_pusch_set_id,
              { "PUSCH Set Id",
                "fp.pusch-set-id", FT_UINT8, BASE_DEC, NULL, 0x0,
                "Identifies PUSCH Set from those configured in NodeB", HFILL
              }
            },
            { &hf_fp_activation_cfn,
              { "Activation CFN",
                "fp.activation-cfn", FT_UINT8, BASE_DEC, NULL, 0x0,
                "Activation Connection Frame Number", HFILL
              }
            },
            { &hf_fp_duration,
              { "Duration (ms)",
                "fp.pusch-set-id", FT_UINT8, BASE_DEC, NULL, 0x0,
                "Duration of the activation period of the PUSCH Set", HFILL
              }
            },
            { &hf_fp_power_offset,
              { "Power offset",
                "fp.power-offset", FT_FLOAT, BASE_NONE, NULL, 0x0,
                "Power offset (in dB)", HFILL
              }
            },
            { &hf_fp_code_number,
              { "Code number",
                "fp.code-number", FT_UINT8, BASE_DEC, NULL, 0x0,
                NULL, HFILL
              }
            },
            { &hf_fp_spreading_factor,
              { "Spreading factor",
                "fp.spreading-factor", FT_UINT8, BASE_DEC, VALS(spreading_factor_vals), 0xf0,
                NULL, HFILL
              }
            },
            { &hf_fp_mc_info,
              { "MC info",
                "fp.mc-info", FT_UINT8, BASE_DEC, NULL, 0x0e,
                NULL, HFILL
              }
            },
            { &hf_fp_rach_new_ie_flags,
              { "New IEs flags",
                "fp.rach.new-ie-flags", FT_STRING, BASE_NONE, 0, 0x0,
                NULL, HFILL
              }
            },
            { &hf_fp_rach_new_ie_flag_unused[0],
              { "New IE present",
                "fp.rach.new-ie-flag", FT_UINT8, BASE_DEC, 0, 0x80,
                NULL, HFILL
              }
            },
            { &hf_fp_rach_new_ie_flag_unused[1],
              { "New IE present",
                "fp.rach.new-ie-flag", FT_UINT8, BASE_DEC, 0, 0x40,
                NULL, HFILL
              }
            },
            { &hf_fp_rach_new_ie_flag_unused[2],
              { "New IE present",
                "fp.rach.new-ie-flag", FT_UINT8, BASE_DEC, 0, 0x20,
                "New IE present (unused)", HFILL
              }
            },
            { &hf_fp_rach_new_ie_flag_unused[3],
              { "New IE present",
                "fp.rach.new-ie-flag", FT_UINT8, BASE_DEC, 0, 0x10,
                "New IE present (unused)", HFILL
              }
            },
            { &hf_fp_rach_new_ie_flag_unused[4],
              { "New IE present",
                "fp.rach.new-ie-flag", FT_UINT8, BASE_DEC, 0, 0x08,
                "New IE present (unused)", HFILL
              }
            },
            { &hf_fp_rach_new_ie_flag_unused[5],
              { "New IE present",
                "fp.rach.new-ie-flag", FT_UINT8, BASE_DEC, 0, 0x04,
                "New IE present (unused)", HFILL
              }
            },
            { &hf_fp_rach_new_ie_flag_unused[6],
              { "New IE present",
                "fp.rach.new-ie-flag", FT_UINT8, BASE_DEC, 0, 0x02,
                "New IE present (unused)", HFILL
              }
            },
            { &hf_fp_rach_cell_portion_id_present,
              { "Cell portion ID present",
                "fp.rach.cell-portion-id-present", FT_UINT8, BASE_DEC, 0, 0x01,
                NULL, HFILL
              }
            },
            { &hf_fp_rach_angle_of_arrival_present,
              { "Angle of arrival present",
                "fp.rach.angle-of-arrival-present", FT_UINT8, BASE_DEC, 0, 0x01,
                NULL, HFILL
              }
            },
            { &hf_fp_rach_ext_propagation_delay_present,
              { "Ext Propagation Delay Present",
                "fp.rach.ext-propagation-delay-present", FT_UINT8, BASE_DEC, 0, 0x02,
                NULL, HFILL
              }
            },
            { &hf_fp_rach_ext_rx_sync_ul_timing_deviation_present,
              { "Ext Received Sync UL Timing Deviation present",
                "fp.rach.ext-rx-sync-ul-timing-deviation-present", FT_UINT8, BASE_DEC, 0, 0x02,
                NULL, HFILL
              }
            },
            { &hf_fp_rach_ext_rx_timing_deviation_present,
              { "Ext Rx Timing Deviation present",
                "fp.rach.ext-rx-timing-deviation-present", FT_UINT8, BASE_DEC, 0, 0x01,
                NULL, HFILL
              }
            },
            { &hf_fp_cell_portion_id,
              { "Cell Portion ID",
                "fp.cell-portion-id", FT_UINT8, BASE_DEC, NULL, 0x3f,
                NULL, HFILL
              }
            },
            { &hf_fp_ext_propagation_delay,
              { "Ext Propagation Delay",
                "fp.ext-propagation-delay", FT_UINT16, BASE_DEC, NULL, 0x03ff,
                NULL, HFILL
              }
            },
            { &hf_fp_angle_of_arrival,
              { "Angle of Arrival",
                "fp.angle-of-arrival", FT_UINT16, BASE_DEC, NULL, 0x03ff,
                NULL, HFILL
              }
            },
            { &hf_fp_ext_received_sync_ul_timing_deviation,
              { "Ext Received SYNC UL Timing Deviation",
                "fp.ext-received-sync-ul-timing-deviation", FT_UINT16, BASE_DEC, NULL, 0x1fff,
                NULL, HFILL
              }
            },


            { &hf_fp_radio_interface_parameter_update_flag[0],
              { "CFN valid",
                "fp.radio-interface-param.cfn-valid", FT_UINT16, BASE_DEC, 0, 0x0001,
                NULL, HFILL
              }
            },
            { &hf_fp_radio_interface_parameter_update_flag[1],
              { "TPC PO valid",
                "fp.radio-interface-param.tpc-po-valid", FT_UINT16, BASE_DEC, 0, 0x0002,
                NULL, HFILL
              }
            },
            { &hf_fp_radio_interface_parameter_update_flag[2],
              { "DPC mode valid",
                "fp.radio-interface-param.dpc-mode-valid", FT_UINT16, BASE_DEC, 0, 0x0004,
                NULL, HFILL
              }
            },
            { &hf_fp_radio_interface_parameter_update_flag[3],
              { "RL sets indicator valid",
                "fp.radio-interface_param.rl-sets-indicator-valid", FT_UINT16, BASE_DEC, 0, 0x0020,
                NULL, HFILL
              }
            },
            { &hf_fp_radio_interface_parameter_update_flag[4],
              { "MAX_UE_TX_POW valid",
                "fp.radio-interface-param.max-ue-tx-pow-valid", FT_UINT16, BASE_DEC, 0, 0x0040,
                "MAX UE TX POW valid", HFILL
              }
            },
            { &hf_fp_dpc_mode,
              { "DPC Mode",
                "fp.dpc-mode", FT_UINT8, BASE_DEC, NULL, 0x20,
                "DPC Mode to be applied in the uplink", HFILL
              }
            },
            { &hf_fp_tpc_po,
              { "TPC PO",
                "fp.tpc-po", FT_UINT8, BASE_DEC, NULL, 0x1f,
                NULL, HFILL
              }
            },
            { &hf_fp_multiple_rl_set_indicator,
              { "Multiple RL sets indicator",
                "fp.multiple-rl-sets-indicator", FT_UINT8, BASE_DEC, NULL, 0x80,
                NULL, HFILL
              }
            },
            { &hf_fp_max_ue_tx_pow,
              { "MAX_UE_TX_POW",
                "fp.max-ue-tx-pow", FT_INT8, BASE_DEC, NULL, 0x0,
                "Max UE TX POW (dBm)", HFILL
              }
            },
            { &hf_fp_congestion_status,
              { "Congestion Status",
                "fp.congestion-status", FT_UINT8, BASE_DEC, VALS(congestion_status_vals), 0x0,
                NULL, HFILL
              }
            },
            { &hf_fp_e_rucch_present,
              { "E-RUCCH Present",
                "fp.erucch-present", FT_UINT8, BASE_DEC, NULL, 0x0,
                NULL, HFILL
              }
            },
            { &hf_fp_extended_bits_present,
              { "Extended Bits Present",
                "fp.extended-bits-present", FT_UINT8, BASE_DEC, NULL, 0x0,
                NULL, HFILL
              }
            },
            { &hf_fp_extended_bits,
              { "Extended Bits",
                "fp.extended-bits", FT_UINT8, BASE_HEX, NULL, 0x0,
                NULL, HFILL
              }
            },
            { &hf_fp_spare_extension,
              { "Spare Extension",
                "fp.spare-extension", FT_NONE, BASE_NONE, NULL, 0x0,
                NULL, HFILL
              }
            },
            { &hf_fp_ul_setup_frame,
              { "UL setup frame",
                "fp.ul.setup_frame", FT_FRAMENUM, BASE_NONE, NULL, 0x0,
                NULL, HFILL
              }
            },
            { &hf_fp_dl_setup_frame,
              { "DL setup frame",
                "fp.dl.setup_frame", FT_FRAMENUM, BASE_NONE, NULL, 0x0,
                NULL, HFILL
              }
            },
            { &hf_fp_hsdsch_physical_layer_category,
              { "HS-DSCH physical layer category",
                "fp.hsdsch.physical_layer_category", FT_UINT8, BASE_DEC, NULL, 0x0,
                NULL, HFILL
              }
            }
        };


    static gint *ett[] =
    {
        &ett_fp,
        &ett_fp_data,
        &ett_fp_crcis,
        &ett_fp_ddi_config,
        &ett_fp_edch_subframe_header,
        &ett_fp_edch_subframe,
        &ett_fp_edch_maces,
        &ett_fp_edch_macis_descriptors,
        &ett_fp_hsdsch_new_ie_flags,
        &ett_fp_rach_new_ie_flags,
        &ett_fp_hsdsch_pdu_block_header,
        &ett_fp_release
    };

    static ei_register_info ei[] = {
        { &ei_fp_bad_header_checksum, { "fp.header.bad_checksum.", PI_CHECKSUM, PI_WARN, "Bad header checksum.", EXPFILL }},
        { &ei_fp_crci_no_subdissector, { "fp.crci.no_subdissector", PI_UNDECODED, PI_NOTE, "Not sent to subdissectors as CRCI is set", EXPFILL }},
        { &ei_fp_crci_error_bit_set_for_tb, { "fp.crci.error_bit_set_for_tb", PI_CHECKSUM, PI_WARN, "CRCI error bit set for TB", EXPFILL }},
        { &ei_fp_spare_extension, { "fp.spare-extension.expert", PI_UNDECODED, PI_WARN, "Spare Extension present (%u bytes)", EXPFILL }},
        { &ei_fp_bad_payload_checksum, { "fp.payload-crc.bad.", PI_CHECKSUM, PI_WARN, "Bad payload checksum.", EXPFILL }},
        { &ei_fp_stop_hsdpa_transmission, { "fp.stop_hsdpa_transmission", PI_RESPONSE_CODE, PI_NOTE, "Stop HSDPA transmission", EXPFILL }},
        { &ei_fp_timing_adjustmentment_reported, { "fp.timing_adjustmentment_reported", PI_SEQUENCE, PI_WARN, "Timing adjustmentment reported (%f ms)", EXPFILL }},
        { &ei_fp_expecting_tdd, { "fp.expecting_tdd", PI_MALFORMED, PI_NOTE, "Error: expecting TDD-384 or TDD-768", EXPFILL }},
        { &ei_fp_ddi_not_defined, { "fp.ddi_not_defined", PI_MALFORMED, PI_ERROR, "DDI %u not defined for this UE!", EXPFILL }},
        { &ei_fp_unable_to_locate_ddi_entry, { "fp.unable_to_locate_ddi_entry", PI_UNDECODED, PI_ERROR, "Unable to locate DDI entry.", EXPFILL }},
        { &ei_fp_mac_is_sdus_miscount, { "fp.mac_is_sdus.miscount", PI_MALFORMED, PI_ERROR, "Found too many (%u) MAC-is SDUs - header said there were %u", EXPFILL }},
        { &ei_fp_e_rnti_t2_edch_frames, { "fp.e_rnti.t2_edch_frames", PI_MALFORMED, PI_ERROR, "E-RNTI not supposed to appear for T2 EDCH frames", EXPFILL }},
        { &ei_fp_e_rnti_first_entry, { "fp.e_rnti.first_entry", PI_MALFORMED, PI_ERROR, "E-RNTI must be first entry among descriptors", EXPFILL }},
        { &ei_fp_maybe_srb, { "fp.maybe_srb", PI_PROTOCOL, PI_NOTE, "Found MACd-Flow = 0 and not MUX detected. (This might be SRB)", EXPFILL }},
        { &ei_fp_transport_channel_type_unknown, { "fp.transport_channel_type.unknown", PI_UNDECODED, PI_WARN, "Unknown transport channel type", EXPFILL }},
        { &ei_fp_hsdsch_entity_not_specified, { "fp.hsdsch_entity_not_specified", PI_MALFORMED, PI_ERROR, "HSDSCH Entity not specified", EXPFILL }},
        { &ei_fp_hsdsch_common_experimental_support, { "fp.hsdsch_common.experimental_support", PI_DEBUG, PI_WARN, "HSDSCH COMMON - Experimental support!", EXPFILL }},
        { &ei_fp_hsdsch_common_t3_not_implemented, { "fp.hsdsch_common_t3.not_implemented", PI_DEBUG, PI_ERROR, "HSDSCH COMMON T3 - Not implemeneted!", EXPFILL }},
        { &ei_fp_channel_type_unknown, { "fp.channel_type.unknown", PI_MALFORMED, PI_ERROR, "Unknown channel type", EXPFILL }},
        { &ei_fp_no_per_frame_info, { "fp.no_per_frame_info", PI_UNDECODED, PI_ERROR, "Can't dissect FP frame because no per-frame info was attached!", EXPFILL }},
    };

    module_t *fp_module;
    expert_module_t *expert_fp;

#ifdef UMTS_FP_USE_UAT
    /* Define a UAT to set channel configuration data */

  static const value_string umts_fp_proto_type_vals[] = {
    { UMTS_FP_IPV4, "IPv4" },
    { UMTS_FP_IPV6, "IPv6" },
    { 0x00, NULL }
  };
  static const value_string umts_fp_uat_channel_type_vals[] = {
    { CHANNEL_RACH_FDD, "RACH FDD" },
    { 0x00, NULL }
  };
  static const value_string umts_fp_uat_interface_type_vals[] = {
    { IuB_Interface, "IuB Interface" },
    { 0x00, NULL }
  };
  static const value_string umts_fp_uat_division_type_vals[] = {
    { Division_FDD, "Division FDD" },
    { 0x00, NULL }
  };

  static const value_string umts_fp_uat_rlc_mode_vals[] = {
    { FP_RLC_TM, "FP RLC TM" },
    { 0x00, NULL }
  };

  static uat_field_t umts_fp_uat_flds[] = {
      UAT_FLD_VS(uat_umts_fp_ep_and_ch_records, protocol, "IP address type", umts_fp_proto_type_vals, "IPv4 or IPv6"),
      UAT_FLD_CSTRING(uat_umts_fp_ep_and_ch_records, srcIP, "RNC IP Address", "Source Address"),
      UAT_FLD_DEC(uat_umts_fp_ep_and_ch_records, src_port, "RNC port for this channel", "Source port"),
      UAT_FLD_CSTRING(uat_umts_fp_ep_and_ch_records, dstIP, "NodeB IP Address", "Destination Address"),
      UAT_FLD_DEC(uat_umts_fp_ep_and_ch_records, dst_port, "NodeB port for this channel", "Destination port"),
      UAT_FLD_VS(uat_umts_fp_ep_and_ch_records, interface_type, "Interface type", umts_fp_uat_interface_type_vals, "Interface type used"),
      UAT_FLD_VS(uat_umts_fp_ep_and_ch_records, division, "division", umts_fp_uat_division_type_vals, "Division type used"),
      UAT_FLD_VS(uat_umts_fp_ep_and_ch_records, channel_type, "Channel type", umts_fp_uat_channel_type_vals, "Channel type used"),
      UAT_FLD_VS(uat_umts_fp_ep_and_ch_records, rlc_mode, "RLC mode", umts_fp_uat_rlc_mode_vals, "RLC mode used"),
      UAT_END_FIELDS
    };
#endif /* UMTS_FP_USE_UAT */
    /* Register protocol. */
    proto_fp = proto_register_protocol("FP", "FP", "fp");
    proto_register_field_array(proto_fp, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
    expert_fp = expert_register_protocol(proto_fp);
    expert_register_field_array(expert_fp, ei, array_length(ei));

    /* Allow other dissectors to find this one by name. */
    fp_handle = register_dissector("fp", dissect_fp, proto_fp);

    /* Preferences */
    fp_module = prefs_register_protocol(proto_fp, NULL);

    /* Determines whether release information should be displayed */
    prefs_register_bool_preference(fp_module, "show_release_info",
                                   "Show reported release info",
                                   "Show reported release info",
                                   &preferences_show_release_info);

    /* Determines whether MAC dissector should be called for payloads */
    prefs_register_bool_preference(fp_module, "call_mac",
                                   "Call MAC dissector for payloads",
                                   "Call MAC dissector for payloads",
                                   &preferences_call_mac_dissectors);
     /* Determines whether or not to validate FP payload checksums */
    prefs_register_bool_preference(fp_module, "payload_checksum",
                                    "Validate FP payload checksums",
                                    "Validate FP payload checksums",
                                    &preferences_payload_checksum);
     /* Determines whether or not to validate FP header checksums */
    prefs_register_bool_preference(fp_module, "header_checksum",
                                    "Validate FP header checksums",
                                    "Validate FP header checksums",
                                    &preferences_header_checksum);
     /* Determines whether or not to validate FP header checksums */
    prefs_register_obsolete_preference(fp_module, "udp_heur");
#ifdef UMTS_FP_USE_UAT

  umts_fp_uat = uat_new("Endpoint and Channel Configuration",
            sizeof(uat_umts_fp_ep_and_ch_record_t),   /* record size */
            "umts_fp_ep_and_channel_cnf",     /* filename */
            TRUE,                             /* from_profile */
            &uat_umts_fp_ep_and_ch_records,   /* data_ptr */
            &num_umts_fp_ep_and_ch_items,     /* numitems_ptr */
            UAT_AFFECTS_DISSECTION,           /* affects dissection of packets, but not set of named fields */
            NULL,                             /* help */
            uat_umts_fp_record_copy_cb,       /* copy callback */
            NULL,                             /* update callback */
            uat_umts_fp_record_free_cb,       /* free callback */
            NULL,                             /* post update callback */
            umts_fp_uat_flds);                /* UAT field definitions */

  prefs_register_uat_preference(fp_module,
                                "epandchannelconfigurationtable",
                                "Endpoints and Radio Channels configuration",
                                "Preconfigured endpoint and Channels data",
                                umts_fp_uat);

  register_init_routine(&umts_fp_init_protocol);
#endif

}


void proto_reg_handoff_fp(void)
{
    dissector_handle_t fp_aal2_handle;

    rlc_bcch_handle           = find_dissector_add_dependency("rlc.bcch", proto_fp);
    mac_fdd_rach_handle       = find_dissector_add_dependency("mac.fdd.rach", proto_fp);
    mac_fdd_fach_handle       = find_dissector_add_dependency("mac.fdd.fach", proto_fp);
    mac_fdd_pch_handle        = find_dissector_add_dependency("mac.fdd.pch", proto_fp);
    mac_fdd_dch_handle        = find_dissector_add_dependency("mac.fdd.dch", proto_fp);
    mac_fdd_edch_handle       = find_dissector_add_dependency("mac.fdd.edch", proto_fp);
    mac_fdd_edch_type2_handle = find_dissector_add_dependency("mac.fdd.edch.type2", proto_fp);
    mac_fdd_hsdsch_handle     = find_dissector_add_dependency("mac.fdd.hsdsch", proto_fp);

    heur_dissector_add("udp", heur_dissect_fp, "FP over UDP", "fp_udp", proto_fp, HEURISTIC_DISABLE);

    fp_aal2_handle = create_dissector_handle(dissect_fp_aal2, proto_fp);
    dissector_add_uint("atm.aal2.type", TRAF_UMTS_FP, fp_aal2_handle);
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
