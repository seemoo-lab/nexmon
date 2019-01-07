/* packet-gsm_a_bssmap.c
 * Routines for GSM A Interface BSSMAP dissection
 *
 * Copyright 2003, Michael Lum <mlum [AT] telostech.com>
 * In association with Telos Technology Inc.
 *
 * Updated to 3GPP TS 48.008 version 9.8.0 Release 9
 * Copyright 2008, Anders Broman <anders.broman [at] ericsson.com
 * Copyright 2012, Pascal Quantin <pascal.quantin [at] gmail.com
 * Title        3GPP            Other
 *
 *   Reference [2]
 *   Mobile-services Switching Centre - Base Station System
 *   (MSC - BSS) interface;
 *   Layer 3 specification
 *   (GSM 08.08 version 7.7.0 Release 1998) TS 100 590 v7.7.0
 *   3GPP TS 48.008 version 8.4.0 Release 8
 *   3GPP TS 48.008 version 9.8.0 Release 9
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
#include <epan/tap.h>
#include <epan/expert.h>
#include <epan/asn1.h>

#include <wsutil/str_util.h>

#include "packet-bssgp.h"
#include "packet-gsm_a_common.h"
#include "packet-e212.h"
#include "packet-ranap.h"
#include "packet-rrc.h"
#include "packet-rtcp.h"
#include "packet-rtp.h"
#include "packet-gsm_map.h"

void proto_register_gsm_a_bssmap(void);
void proto_reg_handoff_gsm_a_bssmap(void);

/* PROTOTYPES/FORWARDS */

/* TS 48.008 3.2.2.1 Message Type */
const value_string gsm_a_bssmap_msg_strings[] = {
    { 0x01, "Assignment Request" },
    { 0x02, "Assignment Complete" },
    { 0x03, "Assignment Failure" },
    { 0x04, "VGCS/VBS Setup" },
    { 0x05, "VGCS/VBS Setup Ack" },
    { 0x06, "VGCS/VBS Setup Refuse" },
    { 0x07, "VGCS/VBS Assignment Request" },
    { 0x08, "Channel Modify request" },

    { 0x09, "Unallocated" },
    { 0x0a, "Unallocated" },
    { 0x0b, "Unallocated" },
    { 0x0c, "Unallocated" },
    { 0x0d, "Unallocated" },
    { 0x0e, "Unallocated" },
    { 0x0f, "Unallocated" },

    { 0x10, "Handover Request" },
    { 0x11, "Handover Required" },
    { 0x12, "Handover Request Acknowledge" },
    { 0x13, "Handover Command" },
    { 0x14, "Handover Complete" },
    { 0x15, "Handover Succeeded" },
    { 0x16, "Handover Failure" },
    { 0x17, "Handover Performed" },
    { 0x18, "Handover Candidate Enquire" },
    { 0x19, "Handover Candidate Response" },
    { 0x1a, "Handover Required Reject" },
    { 0x1b, "Handover Detect" },
    { 0x1c, "VGCS/VBS Assignment Result" },
    { 0x1d, "VGCS/VBS Assignment Failure" },
    { 0x1e, "VGCS/VBS Queuing Indication" },
    { 0x1f, "Uplink Request" },
    { 0x20, "Clear Command" },
    { 0x21, "Clear Complete" },
    { 0x22, "Clear Request" },
    { 0x23, "Reserved" },
    { 0x24, "Reserved" },
    { 0x25, "SAPI 'n' Reject" },
    { 0x26, "Confusion" },
    { 0x27, "Uplink Request Acknowledge" },
    { 0x28, "Suspend" },
    { 0x29, "Resume" },
    /* This value (2a) was allocated in an earlier phase of the protocol and shall not be used in the future. */
    { 0x2a, "Connection Oriented Information(Obsolete)" },
    { 0x2b, "Perform Location Request" },
    { 0x2c, "LSA Information" },
    { 0x2d, "Perform Location Response" },
    { 0x2e, "Perform Location Abort" },
    { 0x2f, "Common Id" },
    { 0x30, "Reset" },
    { 0x31, "Reset Acknowledge" },
    { 0x32, "Overload" },
    { 0x33, "Reserved" },
    { 0x34, "Reset Circuit" },
    { 0x35, "Reset Circuit Acknowledge" },
    { 0x36, "MSC Invoke Trace" },
    { 0x37, "BSS Invoke Trace" },

    { 0x38, "Unallocated" },
    { 0x39, "Unallocated" },

    { 0x3a, "Connectionless Information" },
    { 0x3b, "VGCS/VBS Assignment Status" },
    { 0x3c, "VGCS/VBS Area Cell Info" },
    { 0x3d, "Reset IP Resource" },
    { 0x3e, "Reset IP Resource Acknowledge" },

    { 0x3f, "Unallocated" },

    { 0x40, "Block" },
    { 0x41, "Blocking Acknowledge" },
    { 0x42, "Unblock" },
    { 0x43, "Unblocking Acknowledge" },
    { 0x44, "Circuit Group Block" },
    { 0x45, "Circuit Group Blocking Acknowledge" },
    { 0x46, "Circuit Group Unblock" },
    { 0x47, "Circuit Group Unblocking Acknowledge" },
    { 0x48, "Unequipped Circuit" },
    { 0x49, "Uplink Request Confirmation" },
    { 0x4a, "Uplink Release Indication" },
    { 0x4b, "Uplink Reject Command" },
    { 0x4c, "Uplink Release Command" },
    { 0x4d, "Uplink Seized Command" },
    { 0x4e, "Change Circuit" },
    { 0x4f, "Change Circuit Acknowledge" },
    { 0x50, "Resource Request" },
    { 0x51, "Resource Indication" },
    { 0x52, "Paging" },
    { 0x53, "Cipher Mode Command" },
    { 0x54, "Classmark Update" },
    { 0x55, "Cipher Mode Complete" },
    { 0x56, "Queuing Indication" },
    { 0x57, "Complete Layer 3 Information" },
    { 0x58, "Classmark Request" },
    { 0x59, "Cipher Mode Reject" },
    { 0x5a, "Load Indication" },

    { 0x5b, "Unallocated" },
    { 0x5c, "Unallocated" },
    { 0x5d, "Unallocated" },
    { 0x5e, "Unallocated" },
    { 0x5f, "Unallocated" },

    { 0x60, "VGCS Additional Information" },
    { 0x61, "VGCS SMS" },
    { 0x62, "Notification Data" },
    { 0x63, "Uplink Application Data" },

    { 0x64, "Unallocated" },
    { 0x65, "Unallocated" },
    { 0x66, "Unallocated" },
    { 0x67, "Unallocated" },
    { 0x68, "Unallocated" },
    { 0x69, "Unallocated" },
    { 0x6a, "Unallocated" },
    { 0x6b, "Unallocated" },
    { 0x6c, "Unallocated" },
    { 0x6d, "Unallocated" },
    { 0x6e, "Unallocated" },
    { 0x6f, "Unallocated" },

    { 0x70, "Internal Handover Required" },
    { 0x71, "Internal Handover Required Reject" },
    { 0x72, "Internal Handover Command" },
    { 0x73, "Internal Handover Enquiry" },
    { 0x74, "LCLS-Connect-Control" },
    { 0x75, "LCLS-Connect-Control-Ack" },
    { 0x76, "LCLS-Notification" },
    { 0x77, "Unallocated" },
    { 0x78, "Reroute Command" },
    { 0x79, "Reroute Complete" },
    { 0, NULL }
};

static value_string_ext gsm_a_bssmap_msg_strings_ext = VALUE_STRING_EXT_INIT(gsm_a_bssmap_msg_strings);

static const value_string gsm_bssmap_elem_strings[] = {
    { BE_UDEF_0,                         "Undefined" },
    { BE_CIC,                            "Circuit Identity Code" },
    { BE_RSVD_1,                         "Reserved" },
    { BE_RES_AVAIL,                      "Resource Available" },
    { BE_CAUSE,                          "Cause" },
    { BE_CELL_ID,                        "Cell Identifier" },
    { BE_PRIO,                           "Priority" },
    { BE_L3_HEADER_INFO,                 "Layer 3 Header Information" },
    { BE_IMSI,                           "IMSI" },
    { BE_TMSI,                           "TMSI" },
    { BE_ENC_INFO,                       "Encryption Information" },
    { BE_CHAN_TYPE,                      "Channel Type" },
    { BE_PERIODICITY,                    "Periodicity" },
    { BE_EXT_RES_IND,                    "Extended Resource Indicator" },
    { BE_NUM_MS,                         "Number Of MSs" },
    { BE_RSVD_2,                         "Reserved" },
    { BE_RSVD_3,                         "Reserved" },
    { BE_RSVD_4,                         "Reserved" },
    { BE_CM_INFO_2,                      "Classmark Information Type 2" },
    { BE_CM_INFO_3,                      "Classmark Information Type 3" },
    { BE_INT_BAND,                       "Interference Band To Be Used" },
    { BE_RR_CAUSE,                       "RR Cause" },
    { BE_RSVD_5,                         "Reserved" },
    { BE_L3_INFO,                        "Layer 3 Information" },
    { BE_DLCI,                           "DLCI" },
    { BE_DOWN_DTX_FLAG,                  "Downlink DTX Flag" },
    { BE_CELL_ID_LIST,                   "Cell Identifier List" },
    { BE_RESP_REQ,                       "Response Request" },
    { BE_RES_IND_METHOD,                 "Resource Indication Method" },
    { BE_CM_INFO_1,                      "Classmark Information Type 1" },
    { BE_CIC_LIST,                       "Circuit Identity Code List" },
    { BE_DIAG,                           "Diagnostic" },
    { BE_L3_MSG,                         "Layer 3 Message Contents" },
    { BE_CHOSEN_CHAN,                    "Chosen Channel" },
    { BE_TOT_RES_ACC,                    "Total Resource Accessible" },
    { BE_CIPH_RESP_MODE,                 "Cipher Response Mode" },
    { BE_CHAN_NEEDED,                    "Channel Needed" },
    { BE_TRACE_TYPE,                     "Trace Type" },
    { BE_TRIGGERID,                      "TriggerID" },
    { BE_TRACE_REF,                      "Trace Reference" },
    { BE_TRANSID,                        "TransactionID" },
    { BE_MID,                            "Mobile Identity" },
    { BE_OMCID,                          "OMCID" },
    { BE_FOR_IND,                        "Forward Indicator" },
    { BE_CHOSEN_ENC_ALG,                 "Chosen Encryption Algorithm" },
    { BE_CCT_POOL,                       "Circuit Pool" },
    { BE_CCT_POOL_LIST,                  "Circuit Pool List" },
    { BE_TIME_IND,                       "Time Indication" },
    { BE_RES_SIT,                        "Resource Situation" },
    { BE_CURR_CHAN_1,                    "Current Channel Type 1" },
    { BE_QUE_IND,                        "Queuing Indicator" },
    { BE_ASS_REQ,                        "Assignment Requirement" },
    { BE_UDEF_52,                        "Undefined" },
    { BE_TALKER_FLAG,                    "Talker Flag" },
    { BE_CONN_REL_REQ,                   "Connection Release Requested" },
    { BE_GROUP_CALL_REF,                 "Group Call Reference" },
    { BE_EMLPP_PRIO,                     "eMLPP Priority" },
    { BE_CONF_EVO_IND,                   "Configuration Evolution Indication" },
    { BE_OLD2NEW_INFO,                   "Old BSS to New BSS Information" },
    { BE_LSA_ID,                         "LSA Identifier" },
    { BE_LSA_ID_LIST,                    "LSA Identifier List" },
    { BE_LSA_INFO,                       "LSA Information" },
    { BE_LCS_QOS,                        "LCS QoS" },
    { BE_LSA_ACC_CTRL,                   "LSA access control suppression" },
    { BE_SPEECH_VER,                     "Speech Version" },
    { BE_UDEF_65,                        "Undefined" },
    { BE_UDEF_66,                        "Undefined" },
    { BE_LCS_PRIO,                       "LCS Priority" },
    { BE_LOC_TYPE,                       "Location Type" },
    { BE_LOC_EST,                        "Location Estimate" },
    { BE_POS_DATA,                       "Positioning Data" },
    { BE_LCS_CAUSE,                      "LCS Cause" },
    { BE_LCS_CLIENT,                     "LCS Client Type" },
    { BE_APDU,                           "APDU" },
    { BE_NE_ID,                          "Network Element Identity" },
    { BE_GPS_ASSIST_DATA,                "GPS Assistance Data" },
    { BE_DECIPH_KEYS,                    "Deciphering Keys" },
    { BE_RET_ERR_REQ,                    "Return Error Request" },
    { BE_RET_ERR_CAUSE,                  "Return Error Cause" },
    { BE_SEG,                            "Segmentation" },
    { BE_SERV_HO,                        "Service Handover" },
    { BE_SRC_RNC_TO_TAR_RNC_UMTS,        "Source RNC to target RNC transparent information (UMTS)" },
    { BE_SRC_RNC_TO_TAR_RNC_CDMA,        "Source RNC to target RNC transparent information (cdma2000)" },
    { BE_GERAN_CLS_M,                    "GERAN Classmark" },
    { BE_GERAN_BSC_CONT,                 "GERAN BSC Container" },
    { BE_VEL_EST,                        "Velocity Estimate" },
    { BE_UDEF_86,                        "Undefined" },
    { BE_UDEF_87,                        "Undefined" },
    { BE_UDEF_88,                        "Undefined" },
    { BE_UDEF_89,                        "Undefined" },
    { BE_UDEF_90,                        "Undefined" },
    { BE_UDEF_91,                        "Undefined" },
    { BE_UDEF_92,                        "Undefined" },
    { BE_UDEF_93,                        "Undefined" },
    { BE_UDEF_94,                        "Undefined" },
    { BE_UDEF_95,                        "Undefined" },
    { BE_UDEF_96,                        "Undefined" },
    { BE_NEW_BSS_TO_OLD_BSS_INF,         "New BSS to Old BSS Information" },
    { BE_UDEF_98,                        "Undefined" },
    { BE_INTER_SYS_INF,                  "Inter-System Information" },
    { BE_SNA_ACC_INF,                    "SNA Access Information" },
    { BE_VSTK_RAND_INF,                  "VSTK_RAND Information" },
    { BE_VSTK_INF,                       "VSTK Information" },
    { BE_PAGING_INF,                     "Paging Information" },
    { BE_IMEI,                           "IMEI" },
    { BE_VGCS_FEAT_FLG,                  "VGCS Feature Flags" },
    { BE_TALKER_PRI,                     "Talker Priority" },
    { BE_EMRG_SET_IND,                   "Emergency Set Indication" },
    { BE_TALKER_ID,                      "Talker Identity" },
    { BE_CELL_ID_LIST_SEG,               "Cell Identifier List Segment" },
    { BE_SMS_TO_VGCS,                    "SMS to VGCS" },
    { BE_VGCS_TALKER_MOD,                "VGCS Talker Mode" },
    { BE_VGS_VBS_CELL_STAT,              "VGCS/VBS Cell Status" },
    { BE_CELL_ID_LST_SEG_F_EST_CELLS,    "Cell Identifier List Segment for established cells" },
    { BE_CELL_ID_LST_SEG_F_CELL_TB_EST,  "Cell Identifier List Segment for cells to be established" },
    { BE_CELL_ID_LST_SEG_F_REL_CELL,     "Cell Identifier List Segment for released cells - no user present" },
    { BE_CELL_ID_LST_SEG_F_NOT_EST_CELL, "Cell Identifier List Segment for not established cells - no establishment possible" },
    { BE_GANSS_ASS_DTA,                  "GANSS Assistance Data" },
    { BE_GANSS_POS_DTA,                  "GANSS Positioning Data" },
    { BE_GANSS_LOC_TYP,                  "GANSS Location Type" },
    { BE_APP_DATA,                       "Application Data" },
    { BE_DATA_ID,                        "Data Identity" },
    { BE_APP_DATA_INF,                   "Application Data Information" },
    { BE_MSISDN,                         "MSISDN" },
    { BE_AOIP_TRANS_LAY_ADD,             "AoIP Transport Layer Address" },
    { BE_SPEECH_CODEC_LST,               "Speech Codec List" },
    { BE_SPEECH_CODEC,                   "Speech Codec" },
    { BE_CALL_ID,                        "Call Identifier" },
    { BE_CALL_ID_LST,                    "Call Identifier List" },
    { BE_A_ITF_SEL_FOR_RESET,            "A-Interface Selector for RESET" },
    { BE_UDEF_130,                       "Undefined" },
    { BE_KC128,                          "Kc128" },
    { BE_CSG_ID,                         "CSG Identifier" },
    { BE_REDIR_ATT_FLG,                  "Redirect Attempt Flag" },                                      /* 3.2.2.111    */
    { BE_REROUTE_REJ_CAUSE,              "Reroute Reject Cause" },                                       /* 3.2.2.112    */
    { BE_SEND_SEQN,                      "Send Sequence Number" },                                       /* 3.2.2.113    */
    { BE_REROUTE_OUTCOME,                "Reroute complete outcome" },                                   /* 3.2.2.114    */
    { BE_GLOBAL_CALL_REF,                "Global Call Reference" },                                      /* 3.2.2.115    */
    { BE_LCLS_CONF,                      "LCLS-Configuration" },                                         /* 3.2.2.116    */
    { BE_LCLS_CON_STATUS_CONTROL,        "LCLS-Connection-Status-Control" },                             /* 3.2.2.117    */
    { BE_LCLS_CORR_NOT_NEEDED,           "LCLS-Correlation-Not-Needed" },                                /* 3.2.2.118    */
    { BE_LCLS_BSS_STATUS,                "LCLS-BSS-Status" },                                            /* 3.2.2.119    */
    { BE_LCLS_BREAK_REQ,                 "LCLS-Break-Request" },                                         /* 3.2.2.120    */
    { BE_CSFB_IND,                       "CSFB Indication" },                                            /* 3.2.2.121    */
#if 0
    { BE_CS_TO_PS_SRVCC,                 "CS to PS SRVCC" },                                             /* 3.2.2.122    */
    { BE_SRC_ENB_2_TGT_ENB_TRANSP_INF,   "Source eNB to target eNB transparent information (E-UTRAN)" }, /*3.2.2.123     */
    { BE_CS_TO_PS_SRVCC_IND,             "CS to PS SRVCC Indication" },                                  /* 3.2.2.124    */
    { BE_CN_TO_MS_TRANSP,                "CN to MS transparent information" },                           /* 3.2.2.125    */
#endif
    { BE_SELECTED_PLMN_ID,               "Selected PLMN ID" },                                           /* 3.2.2.126    */
    { 0, NULL }
};
value_string_ext gsm_bssmap_elem_strings_ext = VALUE_STRING_EXT_INIT(gsm_bssmap_elem_strings);

/* 3.2.3 Signalling Field Element Coding */
static const value_string bssmap_field_element_ids[] = {

    { 0x1,  "BSSMAP Field Element: Extra information" },                    /* 3.2.3.1  */
    { 0x2,  "BSSMAP Field Element: Current Channel Type 2" },               /* 3.2.2.2  */
    { 0x3,  "BSSMAP Field Element: Target cell radio information" },        /* 3.2.3.3  */
    { 0x4,  "BSSMAP Field Element: GPRS Suspend information" },             /* 3.2.3.4  */
    { 0x5,  "BSSMAP Field Element: MultiRate configuration information" },  /* 3.2.3.5  */
    { 0x6,  "BSSMAP Field Element: Dual Transfer Mode information" },       /* 3.2.3.6  */
    { 0x7,  "BSSMAP Field Element: Inter RAT Handover Info" },              /* 3.2.3.7  */
    /*{ 0x7,    "UE Capability information" },*/                            /* 3.2.3.7  */
    { 0x8,  "BSSMAP Field Element: cdma2000 Capability Information" },      /* 3.2.3.8  */
    { 0x9,  "BSSMAP Field Element: Downlink Cell Load Information" },       /* 3.2.3.9  */
    { 0xa,  "BSSMAP Field Element: Uplink Cell Load Information" },         /* 3.2.3.10 */
    { 0xb,  "BSSMAP Field Element: Cell Load Information Group" },          /* 3.2.3.11 */
    { 0xc,  "BSSMAP Field Element: Cell Load Information" },                /* 3.2.3.12 */
    { 0x0d, "BSSMAP Field Element: PS Indication" },                        /* 3.2.3.13 */
    { 0x0e, "BSSMAP Field Element: DTM Handover Command Indication" },      /* 3.2.3.14 */
    { 0x6f, "VGCS talker mode" }, /* although technically not a Field Element,
                                     this IE can appear in Old BSS to New BSS information */
    { 0, NULL }
};

static const value_string bssap_cc_values[] = {
    { 0x00,     "not further specified" },
    { 0x80,     "FACCH or SDCCH" },
    { 0xc0,     "SACCH" },
    { 0,        NULL } };

static const value_string bssap_sapi_values[] = {
    { 0x00,     "RR/MM/CC" },
    { 0x03,     "SMS" },
    { 0,        NULL } };

static const value_string gsm_a_be_cell_id_disc_vals[] = {
    { 0,        "The whole Cell Global Identification, CGI, is used to identify the cells."},
    { 1,        "Location Area Code, LAC, and Cell Identify, CI, is used to identify the cells."},
    { 2,        "Cell Identity, CI, is used to identify the cells."},
    { 3,        "No cell is associated with the transaction."},
    { 4,        "Location Area Identification, LAI, is used to identify all cells within a Location Area."},
    { 5,        "Location Area Code, LAC, is used to identify all cells within a location area."},
    { 6,        "All cells on the BSS are identified."},
    { 7,        "Reserved"},
    { 8,        "Intersystem Handover to UTRAN or cdma2000. PLMN-ID, LAC, and RNC-ID, are encoded to identify the target RNC."},
    { 9,        "Intersystem Handover to UTRAN or cdma2000. The RNC-ID is coded to identify the target RNC."},
    { 10,       "Intersystem Handover to UTRAN or cdma2000. LAC and RNC-ID are encoded to identify the target RNC."},
    { 11,       "Serving Area Identity, SAI, is used to identify the Serving Area of UE within UTRAN or cdma2000"},
    { 12,       "LAC, RNC-ID (or Extended RNC-ID) and Cell Identity, CI, is used to identify a UTRAN cell for cell load information"},
    { 13,       "Reserved"},
    { 14,       "Reserved"},
    { 15,       "Reserved"},
    { 0,    NULL }
};
static value_string_ext gsm_a_be_cell_id_disc_vals_ext = VALUE_STRING_EXT_INIT(gsm_a_be_cell_id_disc_vals);

#if 0
static const value_string gsm_a_rr_channel_needed_vals[] = {
    { 0x00,     "Any channel"},
    { 0x01,     "SDCCH"},
    { 0x02,     "TCH/F (Full rate)"},
    { 0x03,     "TCH/H or TCH/F (Dual rate)"},
    { 0,    NULL }
};
#endif

static const value_string bssmap_positioning_methods[] = {
    {  0, "Timing Advance" },
    {  1, "Reserved (Note)" },
    {  2, "Reserved (Note)" },
    {  3, "Mobile Assisted E-OTD" },
    {  4, "Mobile Based E-OTD" },
    {  5, "Mobile Assisted GPS" },
    {  6, "Mobile Based GPS" },
    {  7, "Conventional GPS" },
    {  8, "U-TDOA" },
    {  9, "Reserved for UTRAN use only" },
    { 10, "Reserved for UTRAN use only" },
    { 11, "Reserved for UTRAN use only" },
    { 12, "Cell ID" },
    { 0, NULL}
};

/* Positioning Method definitions */
static const value_string bssmap_positioning_method_vals[] = {
    { 0, "reserved" },
    { 1, "Mobile Assisted E-OTD" },
    { 2, "Mobile Based E-OTD" },
    { 3, "Assisted GPS" },
    { 4, "Assisted GANSS" },
    { 5, "Assisted GPS and Assisted GANSS" },
    { 0, NULL}
};

static const value_string bssmap_positioning_methods_usage[] = {
    { 0, "Attempted unsuccessfully due to failure or interruption" },
    { 1, "Attempted successfully - results not used to generate location" },
    { 2, "Attempted successfully - results used to verify but not generate location" },
    { 3, "Attempted successfully - results used to generate location" },
    { 4, "Attempted successfully - case where MS supports multiple mobile based positioning methods and the actual method or methods used by the MS cannot be determined" },
    { 0, NULL}
};

/* Location Information definitions */
static const value_string bssmap_location_information_vals[] = {
    { 0, "current geographic location" },
    { 1, "location assistance information for the target MS" },
    { 2, "deciphering keys for broadcast assistance data for the target MS" },
    { 0, NULL}
};

static const true_false_string bssmap_chan_type_extension_value = {
    "Additional Octet",
    "Last Octet"
};

static const true_false_string bssmap_cause_extension_value = {
    "Two Octets",
    "One Octet"
};

/* Current Channel Type */
static const value_string chan_mode_vals[] = {
    { 0, "signalling only" },
    { 1, "speech (full rate or half rate)" },
    { 6, "data, 14.5 kbit/s radio interface rate" },
    { 3, "data, 12.0 kbit/s radio interface rate" },
    { 4, "data, 6.0 kbit/s radio interface rate" },
    { 5, "data, 3.6 kbit/s radio interface rate" },
    { 0x0f, "reserved" },
    { 0, NULL}
};

static const value_string fe_cur_chan_type2_chan_field_vals[] = {
    {  1, "SDCCH" },
    {  8, "1 Full rate TCH" },
    {  9, "1 Half rate TCH" },
    { 10, "2 Full Rate TCHs" },
    { 11, "3 Full Rate TCHs" },
    { 12, "4 Full Rate TCHs" },
    { 13, "5 Full Rate TCHs" },
    { 14, "6 Full Rate TCHs" },
    { 15, "7 Full Rate TCHs" },
    {  4, "8 Full Rate TCHs" },
    {  0, "reserved" },
    {  0, NULL}
};

/* Initialize the protocol and registered fields */
static int proto_a_bssmap = -1;

static int hf_gsm_a_bssmap_msg_type = -1;
int hf_gsm_a_bssmap_elem_id = -1;
static int hf_gsm_a_bssmap_field_elem_id = -1;
static int hf_gsm_a_bssmap_cell_ci = -1;
static int hf_gsm_a_bssmap_cell_lac = -1;
static int hf_gsm_a_bssmap_sac = -1;
static int hf_gsm_a_bssmap_dlci_cc = -1;
static int hf_gsm_a_bssmap_dlci_spare = -1;
static int hf_gsm_a_bssmap_dlci_sapi = -1;
static int hf_gsm_a_bssmap_cause = -1;
static int hf_gsm_a_bssmap_be_cell_id_disc = -1;
static int hf_gsm_a_bssmap_pci = -1;
static int hf_gsm_a_bssmap_qa = -1;
static int hf_gsm_a_bssmap_pvi = -1;
static int hf_gsm_a_bssmap_interference_bands = -1;
static int hf_gsm_a_bssmap_lsa_only = -1;
static int hf_gsm_a_bssmap_act = -1;
static int hf_gsm_a_bssmap_pref = -1;
static int hf_gsm_a_bssmap_lsa_inf_prio = -1;
static int hf_gsm_a_bssmap_seq_len = -1;
static int hf_gsm_a_bssmap_seq_no = -1;
static int hf_gsm_a_bssap_cell_id_list_seg_cell_id_disc = -1;
static int hf_gsm_a_bssap_res_ind_method = -1;
static int hf_gsm_a_bssap_cic_list_range = -1;
static int hf_gsm_a_bssap_cic_list_status = -1;
static int hf_gsm_a_bssap_diag_error_pointer = -1;
static int hf_gsm_a_bssap_diag_msg_rcv = -1;
static int hf_gsm_a_bssmap_ch_mode = -1;
static int hf_gsm_a_bssmap_cur_ch_mode = -1;
static int hf_gsm_a_bssmap_channel = -1;
static int hf_gsm_a_bssmap_trace_trigger_id = -1;
static int hf_gsm_a_bssmap_trace_priority_indication = -1;
static int hf_gsm_a_bssmap_trace_bss_record_type = -1;
static int hf_gsm_a_bssmap_trace_msc_record_type = -1;
static int hf_gsm_a_bssmap_trace_invoking_event = -1;
static int hf_gsm_a_bssmap_trace_reference = -1;
static int hf_gsm_a_bssmap_trace_omc_id = -1;
static int hf_gsm_a_bssmap_be_rnc_id = -1;
static int hf_gsm_a_bssmap_apdu_protocol_id = -1;
static int hf_gsm_a_bssmap_periodicity = -1;
static int hf_gsm_a_bssmap_sm = -1;
static int hf_gsm_a_bssmap_tarr = -1;
static int hf_gsm_a_bssmap_tot_no_of_fullr_ch = -1;
static int hf_gsm_a_bssmap_tot_no_of_hr_ch = -1;
static int hf_gsm_a_bssmap_smi = -1;
static int hf_gsm_a_bssmap_lsa_id = -1;
static int hf_gsm_a_bssmap_ep = -1;
static int hf_gsm_a_bssmap_lcs_pri = -1;
static int hf_gsm_a_bssmap_num_ms = -1;
static int hf_gsm_a_bssmap_talker_pri = -1;
static int hf_gsm_a_bssmap_rr_mode = -1;
static int hf_gsm_a_bssmap_group_cipher_key_nb = -1;
static int hf_gsm_a_bssmap_vgcs_vbs_cell_status = -1;
static int hf_gsm_a_bssmap_paging_cause = -1;
static int hf_gsm_a_bssmap_paging_inf_flg = -1;
static int hf_gsm_a_bssmap_serv_ho_inf = -1;
static int hf_gsm_a_bssmap_max_nb_traffic_chan = -1;
static int hf_gsm_a_bssmap_acceptable_chan_coding_bit5 = -1;
static int hf_gsm_a_bssmap_acceptable_chan_coding_bit4 = -1;
static int hf_gsm_a_bssmap_acceptable_chan_coding_bit3 = -1;
static int hf_gsm_a_bssmap_acceptable_chan_coding_bit2 = -1;
static int hf_gsm_a_bssmap_acceptable_chan_coding_bit1 = -1;
static int hf_gsm_a_bssmap_allowed_data_rate_bit8 = -1;
static int hf_gsm_a_bssmap_allowed_data_rate_bit7 = -1;
static int hf_gsm_a_bssmap_allowed_data_rate_bit6 = -1;
static int hf_gsm_a_bssmap_allowed_data_rate_bit5 = -1;
static int hf_gsm_a_bssmap_allowed_data_rate_bit4 = -1;
static int hf_gsm_a_bssmap_vstk_rand = -1;
static int hf_gsm_a_bssmap_vstk = -1;
static int hf_gsm_a_bssmap_spare_bits = -1;
static int hf_gsm_a_bssmap_tpind = -1;
static int hf_gsm_a_bssmap_asind_b2 = -1;
static int hf_gsm_a_bssmap_asind_b3 = -1;
static int hf_gsm_a_bssmap_bss_res = -1;
static int hf_gsm_a_bssmap_tcp = -1;
static int hf_gsm_a_bssmap_filler_bits = -1;
static int hf_gsm_a_bssmap_method = -1;
static int hf_gsm_a_bssmap_ganss_id = -1;
static int hf_gsm_a_bssmap_usage = -1;
static int hf_gsm_a_bssmap_data_id = -1;
static int hf_gsm_a_bssmap_bt_ind = -1;
static int hf_gsm_a_bssmap_aoip_trans_ipv4 = -1;
static int hf_gsm_a_bssmap_aoip_trans_ipv6 = -1;
static int hf_gsm_a_bssmap_aoip_trans_port = -1;
static int hf_gsm_a_bssmap_fi = -1;
static int hf_gsm_a_bssmap_tf = -1;
static int hf_gsm_a_bssmap_pi = -1;
static int hf_gsm_a_bssmap_pt = -1;
static int hf_gsm_a_bssap_speech_codec = -1;
static int hf_gsm_a_bssap_extended_codec = -1;
static int hf_gsm_a_bssap_extended_codec_r2 = -1;
static int hf_gsm_a_bssap_extended_codec_r3 = -1;
static int hf_gsm_a_bssmap_fi2 = -1;
static int hf_gsm_a_bssmap_tf2 = -1;
static int hf_gsm_a_bssmap_pi2 = -1;
static int hf_gsm_a_bssmap_pt2 = -1;
static int hf_gsm_a_bssmap_call_id = -1;
static int hf_gsm_a_bssmap_spare = -1;
static int hf_gsm_a_bssmap_positioning_data_discriminator = -1;
static int hf_gsm_a_bssmap_positioning_method = -1;
static int hf_gsm_a_bssmap_positioning_method_usage = -1;
static int hf_gsm_a_bssmap_location_type_location_information = -1;
static int hf_gsm_a_bssmap_location_type_positioning_method = -1;
static int hf_gsm_a_bssmap_chan_type_extension = -1;
static int hf_gsm_a_bssmap_cause_extension = -1;
static int hf_gsm_a_bssmap_ass_req = -1;
static int hf_gsm_a_bssmap_emlpp_prio = -1;
static int hf_gsm_a_bssmap_rip = -1;
static int hf_gsm_a_bssmap_rtd = -1;
static int hf_gsm_a_bssmap_kc128 = -1;
static int hf_gsm_a_bssmap_csg_id = -1;
static int hf_gsm_a_bssmap_cell_access_mode = -1;
static int hf_fe_extra_info_prec = -1;
static int hf_fe_extra_info_lcs = -1;
static int hf_fe_extra_info_ue_prob = -1;
static int hf_fe_extra_info_spare = -1;
static int hf_fe_cur_chan_type2_chan_mode = -1;
static int hf_fe_cur_chan_type2_chan_mode_spare = -1;
static int hf_fe_cur_chan_type2_chan_field = -1;
static int hf_fe_cur_chan_type2_chan_field_spare = -1;
static int hf_fe_target_radio_cell_info_rxlev_ncell = -1;
static int hf_fe_target_radio_cell_info_rxlev_ncell_spare = -1;
static int hf_fe_dtm_info_dtm_ind = -1;
static int hf_fe_dtm_info_sto_ind = -1;
static int hf_fe_dtm_info_egprs_ind = -1;
static int hf_fe_dtm_info_spare_bits = -1;
static int hf_fe_cell_load_info_cell_capacity_class = -1;
static int hf_fe_cell_load_info_load_value = -1;
static int hf_fe_cell_load_info_rt_load_value = -1;
static int hf_fe_cell_load_info_nrt_load_information_value = -1;
static int hf_fe_ps_indication = -1;
static int hf_fe_dtm_ho_command_ind_spare = -1;
static int hf_gsm_a_bssmap_speech_data_ind = -1;
static int hf_gsm_a_bssmap_channel_rate_and_type = -1;
static int hf_gsm_a_bssmap_perm_speech_v_ind = -1;
static int hf_gsm_a_bssmap_reroute_rej_cause = -1;
static int hf_gsm_a_bssmap_send_seqn = -1;
static int hf_gsm_a_bssmap_reroute_outcome = -1;
static int hf_gsm_a_bssmap_lcls_conf = -1;
static int hf_gsm_a_bssmap_lcls_con_status_control = -1;
static int hf_gsm_a_bssmap_lcls_bss_status = -1;
static int hf_gsm_a_bssmap_selected_plmn_id = -1;

/* Generated from convert_proto_tree_add_text.pl */
static int hf_gsm_a_bssmap_message_elements = -1;
static int hf_gsm_a_bssmap_full_rate_channels_available = -1;
static int hf_gsm_a_bssmap_tch_6_4_8kb = -1;
static int hf_gsm_a_bssmap_speech_version_id = -1;
static int hf_gsm_a_bssmap_cell_id_unknown_format = -1;
static int hf_gsm_a_bssmap_tch_12kb = -1;
static int hf_gsm_a_bssmap_all_call_identifiers_resources_released = -1;
static int hf_gsm_a_bssmap_pcm_multiplexer = -1;
static int hf_gsm_a_bssmap_talker_identity_field = -1;
static int hf_gsm_a_bssmap_qri = -1;
static int hf_gsm_a_bssmap_cause_value = -1;
static int hf_gsm_a_bssmap_algorithm_identifier = -1;
static int hf_gsm_a_bssmap_tch_14_5_14_4kb = -1;
static int hf_gsm_a_bssmap_national_cause = -1;
static int hf_gsm_a_bssmap_cause_class = -1;
static int hf_gsm_a_bssmap_rate = -1;
static int hf_gsm_a_bssmap_tch_14_5kb = -1;
static int hf_gsm_a_bssmap_tch_12_9kb = -1;
static int hf_gsm_a_bssmap_s0_s7 = -1;
static int hf_gsm_a_bssmap_tio = -1;
static int hf_gsm_a_bssmap_priority_level = -1;
static int hf_gsm_a_bssmap_cause16 = -1;
static int hf_gsm_a_bssmap_enc_info_key = -1;
static int hf_gsm_a_bssmap_unknown_format = -1;
static int hf_gsm_a_bssmap_timeslot = -1;
static int hf_gsm_a_bssmap_transparent_service = -1;
static int hf_gsm_a_bssmap_tch_6kb = -1;
static int hf_gsm_a_bssmap_circuit_pool_number = -1;
static int hf_gsm_a_bssmap_ti_flag = -1;
static int hf_gsm_a_bssmap_half_rate_channels_available = -1;
static int hf_gsm_a_bssmap_imeisv_included = -1;
static int hf_gsm_a_bssmap_bss_activate_downlink = -1;
static int hf_gsm_a_bssmap_apdu = -1;
static int hf_gsm_a_bssmap_s0_s15 = -1;
static int hf_gsm_a_bssmap_layer_3_information_value = -1;
static int hf_gsm_a_bssmap_gsm_a5_1 = -1;
static int hf_gsm_a_bssmap_gsm_a5_2 = -1;
static int hf_gsm_a_bssmap_gsm_a5_3 = -1;
static int hf_gsm_a_bssmap_gsm_a5_4 = -1;
static int hf_gsm_a_bssmap_gsm_a5_5 = -1;
static int hf_gsm_a_bssmap_gsm_a5_6 = -1;
static int hf_gsm_a_bssmap_gsm_a5_7 = -1;
static int hf_gsm_a_bssmap_no_encryption = -1;
static int hf_gsm_a_bssmap_data_channel_rate_and_type = -1;
static int hf_gsm_a_bssmap_cell_discriminator = -1;
static int hf_gsm_a_bssmap_layer3_message_contents = -1;
static int hf_gsm_a_bssmap_forward_indicator = -1;

static expert_field ei_gsm_a_bssmap_extraneous_data = EI_INIT;
static expert_field ei_gsm_a_bssmap_not_decoded_yet = EI_INIT;
static expert_field ei_gsm_a_bssap_unknown_codec = EI_INIT;
static expert_field ei_gsm_a_bssmap_bogus_length = EI_INIT;
static expert_field ei_gsm_a_bssmap_missing_mandatory_element = EI_INIT;

/* Initialize the subtree pointers */
static gint ett_bssmap_msg = -1;
static gint ett_cell_list = -1;
static gint ett_dlci = -1;
static gint ett_codec_lst = -1;
static gint ett_bss_to_bss_info = -1;

static dissector_handle_t gsm_bsslap_handle = NULL;
static dissector_handle_t dtap_handle;
static dissector_handle_t bssgp_handle;
static dissector_handle_t rrc_handle;

static proto_tree *g_tree;
static guint8 cell_discriminator = 0x0f;  /* tracks whether handover is to UMTS */

static guint16
be_field_element_dissect(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_);

#if 0
This enum has been moved to packet-gsm_a_common to
make it possible to use element dissecton from this dissector
in other dissectors.
It is left here as a comment for easier reference.

Note this enum must be of the same size as the element decoding list

typedef enum
{
    BE_UDEF_0,                          /* Undefined */
    BE_CIC,                             /* Circuit Identity Code */
    BE_RSVD_1,                          /* Reserved */
    BE_RES_AVAIL,                       /* Resource Available */
    BE_CAUSE,                           /* Cause */
    BE_CELL_ID,                         /* Cell Identifier */
    BE_PRIO,                            /* Priority */
    BE_L3_HEADER_INFO,                  /* Layer 3 Header Information */
    BE_IMSI,                            /* IMSI */
    BE_TMSI,                            /* TMSI */
    BE_ENC_INFO,                        /* Encryption Information */
    BE_CHAN_TYPE,                       /* Channel Type */
    BE_PERIODICITY,                     /* Periodicity */
    BE_EXT_RES_IND,                     /* Extended Resource Indicator */
    BE_NUM_MS,                          /* Number Of MSs */
    BE_RSVD_2,                          /* Reserved */
    BE_RSVD_3,                          /* Reserved */
    BE_RSVD_4,                          /* Reserved */
    BE_CM_INFO_2,                       /* Classmark Information Type 2 */
    BE_CM_INFO_3,                       /* Classmark Information Type 3 */
    BE_INT_BAND,                        /* Interference Band To Be Used */
    BE_RR_CAUSE,                        /* RR Cause */
    BE_RSVD_5,                          /* Reserved */
    BE_L3_INFO,                         /* Layer 3 Information */
    BE_DLCI,                            /* DLCI */
    BE_DOWN_DTX_FLAG,                   /* Downlink DTX Flag */
    BE_CELL_ID_LIST,                    /* Cell Identifier List */
    BE_RESP_REQ,                        /* Response Request */
    BE_RES_IND_METHOD,                  /* Resource Indication Method */
    BE_CM_INFO_1,                       /* Classmark Information Type 1 */
    BE_CIC_LIST,                        /* Circuit Identity Code List */
    BE_DIAG,                            /* Diagnostic */
    BE_L3_MSG,                          /* Layer 3 Message Contents */
    BE_CHOSEN_CHAN,                     /* Chosen Channel */
    BE_TOT_RES_ACC,                     /* Total Resource Accessible */
    BE_CIPH_RESP_MODE,                  /* Cipher Response Mode */
    BE_CHAN_NEEDED,                     /* Channel Needed */
    BE_TRACE_TYPE,                      /* Trace Type */
    BE_TRIGGERID,                       /* TriggerID */
    BE_TRACE_REF,                       /* Trace Reference */
    BE_TRANSID,                         /* TransactionID */
    BE_MID,                             /* Mobile Identity */
    BE_OMCID,                           /* OMCID */
    BE_FOR_IND,                         /* Forward Indicator */
    BE_CHOSEN_ENC_ALG,                  /* Chosen Encryption Algorithm */
    BE_CCT_POOL,                        /* Circuit Pool */
    BE_CCT_POOL_LIST,                   /* Circuit Pool List */
    BE_TIME_IND,                        /* Time Indication */
    BE_RES_SIT,                         /* Resource Situation */
    BE_CURR_CHAN_1,                     /* Current Channel Type 1 */
    BE_QUE_IND,                         /* Queueing Indicator */
    BE_ASS_REQ,                         /* Assignment Requirement */
    BE_UDEF_52,                         /* Undefined */
    BE_TALKER_FLAG,                     /* Talker Flag */
    BE_CONN_REL_REQ,                    /* Connection Release Requested */
    BE_GROUP_CALL_REF,                  /* Group Call Reference */
    BE_EMLPP_PRIO,                      /* eMLPP Priority */
    BE_CONF_EVO_IND,                    /* Configuration Evolution Indication */
    BE_OLD2NEW_INFO,                    /* Old BSS to New BSS Information */
    BE_LSA_ID,                          /* LSA Identifier */
    BE_LSA_ID_LIST,                     /* LSA Identifier List */
    BE_LSA_INFO,                        /* LSA Information */
    BE_LCS_QOS,                         /* LCS QoS */
    BE_LSA_ACC_CTRL,                    /* LSA access control suppression */
    BE_SPEECH_VER,                      /* Speech Version */
    BE_UDEF_65,                         /* Undefined */
    BE_UDEF_66,                         /* Undefined */
    BE_LCS_PRIO,                        /* LCS Priority */
    BE_LOC_TYPE,                        /* Location Type */
    BE_LOC_EST,                         /* Location Estimate */
    BE_POS_DATA,                        /* Positioning Data */
    BE_LCS_CAUSE,                       /* 3.2.2.66 LCS Cause */
    BE_LCS_CLIENT,                      /* 10.14 LCS Client Type */
    BE_APDU,                            /* APDU */
    BE_NE_ID,                           /* Network Element Identity */
    BE_GPS_ASSIST_DATA,                 /* GPS Assistance Data */
    BE_DECIPH_KEYS,                     /* Deciphering Keys */
    BE_RET_ERR_REQ,                     /* Return Error Request */
    BE_RET_ERR_CAUSE,                   /* Return Error Cause */
    BE_SEG,                             /* Segmentation */
    BE_SERV_HO,                         /* Service Handover */
    BE_SRC_RNC_TO_TAR_RNC_UMTS,         /* Source RNC to target RNC transparent information (UMTS) */
    BE_SRC_RNC_TO_TAR_RNC_CDMA,         /* Source RNC to target RNC transparent information (cdma2000) */
    BE_GERAN_CLS_M,                     /* GERAN Classmark */
    BE_GERAN_BSC_CONT,                  /* GERAN BSC Container */
    BE_VEL_EST,                         /* Velocity Estimate */
    BE_UDEF_86,                         /* Undefined */
    BE_UDEF_87,                         /* Undefined */
    BE_UDEF_88,                         /* Undefined */
    BE_UDEF_89,                         /* Undefined */
    BE_UDEF_90,                         /* Undefined */
    BE_UDEF_91,                         /* Undefined */
    BE_UDEF_92,                         /* Undefined */
    BE_UDEF_93,                         /* Undefined */
    BE_UDEF_94,                         /* Undefined */
    BE_UDEF_95,                         /* Undefined */
    BE_UDEF_96,                         /* Undefined */
    BE_NEW_BSS_TO_OLD_BSS_INF,          /* New BSS to Old BSS Information */
    BE_UDEF_98,                         /* Undefined */
    BE_INTER_SYS_INF,                   /* Inter-System Information */
    BE_SNA_ACC_INF,                     /* SNA Access Information */
    BE_VSTK_RAND_INF,                   /* VSTK_RAND Information */
    BE_VSTK_INF,                        /* VSTK Information */
    BE_PAGING_INF,                      /* Paging Information */
    BE_IMEI,                            /* IMEI */
    BE_VGCS_FEAT_FLG,                   /* VGCS Feature Flags */
    BE_TALKER_PRI,                      /* Talker Priority */
    BE_EMRG_SET_IND,                    /* Emergency Set Indication */
    BE_TALKER_ID,                       /* Talker Identity */
    BE_CELL_ID_LIST_SEG,                /* Cell Identifier List Segment */
    BE_SMS_TO_VGCS,                     /* SMS to VGCS */
    BE_VGCS_TALKER_MOD,                 /* VGCS Talker Mode */
    BE_VGS_VBS_CELL_STAT,               /* VGCS/VBS Cell Status */
    BE_CELL_ID_LST_SEG_F_EST_CELLS,     /* Cell Identifier List Segment for established cells */
    BE_CELL_ID_LST_SEG_F_CELL_TB_EST,   /* Cell Identifier List Segment for cells to be established */
    BE_CELL_ID_LST_SEG_F_REL_CELL,      /* Cell Identifier List Segment for released cells - no user present */
    BE_CELL_ID_LST_SEG_F_NOT_EST_CELL,  /* Cell Identifier List Segment for not established cells - no establishment possible */
    BE_GANSS_ASS_DTA,                   /* GANSS Assistance Data */
    BE_GANSS_POS_DTA,                   /* GANSS Positioning Data */
    BE_GANSS_LOC_TYP,                   /* GANSS Location Type */
    BE_APP_DATA,                        /* Application Data */
    BE_DATA_ID,                         /* Data Identity */
    BE_APP_DATA_INF,                    /* Application Data Information */
    BE_MSISDN,                          /* MSISDN */
    BE_AOIP_TRANS_LAY_ADD,              /* AoIP Transport Layer Address */
    BE_SPEECH_CODEC_LST,                /* Speech Codec List */
    BE_SPEECH_CODEC,                    /* Speech Codec */
    BE_CALL_ID,                         /* Call Identifier */
    BE_CALL_ID_LST,                     /* Call Identifier List */
    BE_A_ITF_SEL_FOR_RESET,             /* A-Interface Selector for RESET */
    BE_UDEF_130,                        /* Undefined */
    BE_KC128,                           /* Kc128 */
    BE_CSG_ID,                          /* CSG Identifier */
    BE_REDIR_ATT_FLG,                   /* Redirect Attempt Flag               3.2.2.111    */
    BE_REROUTE_REJ_CAUSE,               /* Reroute Reject Cause                3.2.2.112    */
    BE_SEND_SEQN,                       /* Send Sequence Number                3.2.2.113    */
    BE_REROUTE_OUTCOME,                 /* Reroute complete outcome            3.2.2.114    */
    BE_GLOBAL_CALL_REF,                 /* Global Call Reference               3.2.2.115    */
    BE_LCLS_CONF,                       /* LCLS-Configuration                  3.2.2.116    */
    BE_LCLS_CON_STATUS_CONTROL,         /* LCLS-Connection-Status-Control      3.2.2.117    */
    BE_LCLS_CORR_NOT_NEEDED,            /* LCLS-Correlation-Not-Needed         3.2.2.118    */
    BE_LCLS_BSS_STATUS,                 /* LCLS-BSS-Status                     3.2.2.119    */
    BE_LCLS_BREAK_REQ,                  /* LCLS-Break-Request                  3.2.2.120    */
    BE_CSFB_IND,                        /* CSFB Indication                     3.2.2.121    */
    BE_CS_TO_PS_SRVCC,                  /* CS to PS SRVCC                      3.2.2.122    */
    BE_SRC_ENB_2_TGT_ENB_TRANSP_INF,    /* Source eNB to target eNB transparent information (E-UTRAN)" 3.2.2.123    */
    BE_CS_TO_PS_SRVCC_IND,              /* CS to PS SRVCC Indication           3.2.2.124    */
    BE_CN_TO_MS_TRANSP,                 /* CN to MS transparent information    3.2.2.125    */
    BE_SELECTED_PLMN_ID,                /* Selected PLMN ID                    3.2.2.126    */
    BE_NONE                             /* NONE */
}
bssmap_elem_idx_t;
#endif

#define NUM_GSM_BSSMAP_ELEM (sizeof(gsm_bssmap_elem_strings)/sizeof(value_string))
gint ett_gsm_bssmap_elem[NUM_GSM_BSSMAP_ELEM];

/*
 * [2] 3.2.2.2 Circuit Identity Code
 */
static guint16
be_cic(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string, int string_len)
{
    guint32 curr_offset;
    guint32 value;

    curr_offset = offset;

    value = tvb_get_ntohs(tvb, curr_offset);

    proto_tree_add_item(tree, hf_gsm_a_bssmap_pcm_multiplexer, tvb, curr_offset, 2, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_timeslot, tvb, curr_offset, 2, ENC_BIG_ENDIAN);
    curr_offset += 2;

    if (add_string)
        g_snprintf(add_string, string_len, " - (%u) (0x%04x)", value, value);

    /* no length check possible */

    return(curr_offset - offset);
}
/*
 * 3.2.2.3  Connection Release Requested
 * No Data
 */

/*
 * 3.2.2.4  Resource Available
 */
static guint16
be_res_avail(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;
    guint16 value;
    int     i;
    proto_item* ti;

    curr_offset = offset;

    for (i=0; i < 5; i++) {
        value = tvb_get_ntohl(tvb, curr_offset);
        ti = proto_tree_add_uint_format(tree, hf_gsm_a_bssmap_full_rate_channels_available, tvb, curr_offset, 2, value, "Number of full rate channels available in band %u %u",i+1,value);
        proto_item_set_len(ti, len);
        curr_offset+=2;
        ti = proto_tree_add_uint_format(tree, hf_gsm_a_bssmap_half_rate_channels_available, tvb, curr_offset, 2, value, "Number of half rate channels available in band %u %u",i+1, value);
        proto_item_set_len(ti, len);
        curr_offset+=2;
    }


    return(len);
}
/*
 * [2] 3.2.2.5 Cause
 */
static const value_string cause_class_vals[] = {
    { 0, "Normal Event"},
    { 1, "Normal Event"},
    { 2, "Resource Unavailable"},
    { 3, "Service or option not available"},
    { 4, "Service or option not implemented"},
    { 5, "Invalid message (e.g., parameter out of range)"},
    { 6, "Protocol error"},
    { 7, "Interworking"},
    { 0, NULL }
};

static const range_string gsm_a_bssap_cause_rvals[] = {
    { 0x00,     0x00, "Radio interface message failure" },
    { 0x01,     0x01, "Radio interface failure" },
    { 0x02,     0x02, "Uplink quality" },
    { 0x03,     0x03, "Uplink strength" },
    { 0x04,     0x04, "Downlink quality" },
    { 0x05,     0x05, "Downlink strength" },
    { 0x06,     0x06, "Distance" },
    { 0x07,     0x07, "O and M intervention" },
    { 0x08,     0x08, "Response to MSC invocation" },
    { 0x09,     0x09, "Call control" },
    { 0x0a,     0x0a, "Radio interface failure, reversion to old channel" },
    { 0x0b,     0x0b, "Handover successful" },
    { 0x0c,     0x0c, "Better Cell" },
    { 0x0d,     0x0d, "Directed Retry" },
    { 0x0e,     0x0e, "Joined group call channel" },
    { 0x0f,     0x0f, "Traffic" },
    { 0x10,     0x10, "Reduce load in serving cell" },
    { 0x11,     0x11, "Traffic load in target cell higher than in source cell" },
    { 0x12,     0x12, "Relocation triggered" },
    { 0x14,     0x14, "Requested option not authorised" },
    { 0x15,     0x15, "Alternative channel configuration requested " },
    { 0x16,     0x16, "Call Identifier already allocated" },
    { 0x17,     0x17, "INTERNAL HANDOVER ENQUIRY reject" },
    { 0x18,     0x18, "Redundancy Level not adequate" },
    { 0x19,     0x1f, "Reserved for national use" },
    { 0x20,     0x20, "Equipment failure" },
    { 0x21,     0x21, "No radio resource available" },
    { 0x22,     0x22, "Requested terrestrial resource unavailable" },
    { 0x23,     0x23, "CCCH overload" },
    { 0x24,     0x24, "Processor overload" },
    { 0x25,     0x25, "BSS not equipped" },
    { 0x26,     0x26, "MS not equipped" },
    { 0x27,     0x27, "Invalid cell" },
    { 0x28,     0x28, "Traffic Load" },
    { 0x29,     0x29, "Preemption" },
    { 0x2a,     0x2a, "DTM Handover - SGSN Failure" },
    { 0x2b,     0x2b, "DTM Handover - PS Allocation failure" },
    { 0x2c,     0x2f, "Reserved for national use" },
    { 0x30,     0x30, "Requested transcoding/rate adaption unavailable" },
    { 0x31,     0x31, "Circuit pool mismatch" },
    { 0x32,     0x32, "Switch circuit pool" },
    { 0x33,     0x33, "Requested speech version unavailable" },
    { 0x34,     0x34, "LSA not allowed" },
    { 0x35,     0x35, "Requested Codec Type or Codec Configuration unavailable" },
    { 0x36,     0x36, "Requested A-Interface Type unavailable" },
    { 0x37,     0x37, "Invalid CSG cell" },
    { 0x38,     0x3e, "Reserved for international use" },
    { 0x3f,     0x3f, "Requested Redundancy Level not available" },
    { 0x40,     0x40, "Ciphering algorithm not supported" },
    { 0x41,     0x41, "GERAN Iu-mode failure" },
    { 0x42,     0x42, "Incoming Relocation Not Supported Due To PUESBINE Feature" },
    { 0x43,     0x43, "Access Restricted Due to Shared Networks" },
    { 0x44,     0x44, "Requested Codec Type or Codec Configuration not supported" },
    { 0x45,     0x45, "Requested A-Interface Type not supported" },
    { 0x46,     0x46, "Requested Redundancy Level not supported" },
    { 0x47,     0x47, "Reserved for international use" },
    { 0x48,     0x4f, "Reserved for national use" },
    { 0x50,     0x50, "Terrestrial circuit already allocated" },
    { 0x51,     0x51, "Invalid message contents" },
    { 0x52,     0x52, "Information element or field missing" },
    { 0x53,     0x53, "Incorrect value" },
    { 0x54,     0x54, "Unknown Message type" },
    { 0x55,     0x55, "Unknown Information Element" },
    { 0x56,     0x56, "DTM Handover - Invalid PS Indication" },
    { 0x57,     0x57, "Call Identifier already allocated" },
    { 0x58,     0x5f, "Reserved for national use" },
    { 0x60,     0x60, "Protocol Error between BSS and MSC" },
    { 0x61,     0x61, "VGCS/VBS call non existent" },
    { 0x62,     0x62, "DTM Handover - Timer Expiry" },
    { 0x63,     0x67, "Reserved for international use" },
    { 0x68,     0x6f, "Reserved for national use" },
    { 0x70,     0x77, "Reserved for international use" },
    { 0x78,     0x7f, "Reserved for national use" },

    { 0, 0, NULL },
};

static guint16
be_cause(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len, gchar *add_string, int string_len)
{
    guint8       oct;
    guint32      curr_offset;
    const gchar *str = NULL;

    curr_offset = offset;

    oct = tvb_get_guint8(tvb, curr_offset);

    proto_tree_add_item(tree, hf_gsm_a_bssmap_cause_extension, tvb, curr_offset, 1, ENC_BIG_ENDIAN);

    if (oct & 0x80)
    {
        /* 2 octet cause */

        if ((oct & 0x0f) == 0x00)
        {
            /* national cause */

            proto_tree_add_item(tree, hf_gsm_a_bssmap_cause_class, tvb, curr_offset, 1, ENC_NA);
            proto_tree_add_item(tree, hf_gsm_a_bssmap_national_cause, tvb, curr_offset, 1, ENC_NA);
            curr_offset++;

            proto_tree_add_item(tree, hf_gsm_a_bssmap_cause_value, tvb, curr_offset, 1, ENC_NA);
            curr_offset++;

            if (add_string)
                g_snprintf(add_string, string_len, " - (National Cause)");
        }
        else
        {
            proto_tree_add_item(tree, hf_gsm_a_bssmap_cause16, tvb, curr_offset, 2, ENC_BIG_ENDIAN);
            curr_offset+=2;
        }
    }
    else
    {
        proto_tree_add_item(tree, hf_gsm_a_bssmap_cause,
            tvb, curr_offset, 1, ENC_NA);

        curr_offset++;

        if (add_string)
            g_snprintf(add_string, string_len, " - (%u) %s", oct & 0x7f, str);
    }

    EXTRANEOUS_DATA_CHECK(len, curr_offset - offset, pinfo, &ei_gsm_a_bssmap_extraneous_data);

    return(curr_offset - offset);
}
/*
 * 3.2.2.6  IMSI
 * IMSI coded as the value part of the Mobile Identity IE defined in 3GPP TS 24.008 (NOTE 1)
 * NOTE 1:  The Type of identity field in the Mobile Identity IE shall be ignored by the receiver.
 * Dissected in packet-gsm_a_common.c (de_mid)
 */

/*
 * [2] 3.2.2.7 TMSI
 */
static guint16
be_tmsi(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len, gchar *add_string, int string_len)
{
    guint32 curr_offset;
    guint32 value;

    curr_offset = offset;

    value = tvb_get_ntohl(tvb, curr_offset);

    proto_tree_add_uint(tree, hf_gsm_a_tmsi,
        tvb, curr_offset, 4,
        value);

    if (add_string)
    g_snprintf(add_string, string_len, " - (0x%04x)", value);

    curr_offset += 4;

    EXTRANEOUS_DATA_CHECK(len, curr_offset - offset, pinfo, &ei_gsm_a_bssmap_extraneous_data);

    return(curr_offset - offset);
}

/*
 * [2] 3.2.2.8 Number Of MSs
 */
static guint16
be_num_ms(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    proto_tree_add_item(tree, hf_gsm_a_bssmap_num_ms, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;

    EXTRANEOUS_DATA_CHECK(len, curr_offset - offset, pinfo, &ei_gsm_a_bssmap_extraneous_data);

    return(curr_offset - offset);
}
/*
 * [2] 3.2.2.9 Layer 3 Header Information
 */
static guint16
be_l3_header_info(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset  = offset;

    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, curr_offset<<3, 4, ENC_BIG_ENDIAN);

    proto_tree_add_item(tree, hf_gsm_a_L3_protocol_discriminator, tvb, curr_offset, 1, ENC_BIG_ENDIAN);

    curr_offset++;

    NO_MORE_DATA_CHECK(len);

    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, curr_offset<<3, 4, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_ti_flag, tvb, curr_offset, 1, ENC_NA);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_tio, tvb, curr_offset, 1, ENC_NA);
    curr_offset++;

    EXTRANEOUS_DATA_CHECK(len, curr_offset - offset, pinfo, &ei_gsm_a_bssmap_extraneous_data);

    return(curr_offset - offset);
}

/*
 * [2] 3.2.2.10 Encryption Information
 */
static const true_false_string tfs_permitted_not_permitted = { "Permitted", "Not permitted" };

static guint16
be_enc_info(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset = offset;

    proto_tree_add_item(tree, hf_gsm_a_bssmap_gsm_a5_7, tvb, curr_offset, 1, ENC_NA);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_gsm_a5_6, tvb, curr_offset, 1, ENC_NA);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_gsm_a5_5, tvb, curr_offset, 1, ENC_NA);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_gsm_a5_4, tvb, curr_offset, 1, ENC_NA);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_gsm_a5_3, tvb, curr_offset, 1, ENC_NA);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_gsm_a5_2, tvb, curr_offset, 1, ENC_NA);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_gsm_a5_1, tvb, curr_offset, 1, ENC_NA);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_no_encryption, tvb, curr_offset, 1, ENC_NA);
    curr_offset++;

    NO_MORE_DATA_CHECK(len);

    proto_tree_add_item(tree, hf_gsm_a_bssmap_enc_info_key, tvb, curr_offset, len - (curr_offset - offset), ENC_NA);

    curr_offset += len - (curr_offset - offset);

    EXTRANEOUS_DATA_CHECK(len, curr_offset - offset, pinfo, &ei_gsm_a_bssmap_extraneous_data);

    return(curr_offset - offset);
}

/*
 * [2] 3.2.2.11 Channel Type
 */

static const value_string gsm_a_bssap_speech_data_ind_vals[] = {
    { 0x0,  "Reserved"},
    { 0x1,  "Speech"},
    { 0x2,  "Data"},
    { 0x3,  "Signalling"},
    { 0x4,  "Speech + CTM Text Telephony"},
    { 0x5,  "Reserved"},
    { 0x6,  "Reserved"},
    { 0x7,  "Reserved"},
    { 0x8,  "Reserved"},
    { 0x9,  "Reserved"},
    { 0xa,  "Reserved"},
    { 0xb,  "Reserved"},
    { 0xc,  "Reserved"},
    { 0xd,  "Reserved"},
    { 0xe,  "Reserved"},
    { 0xf,  "Reserved"},
    { 0,    NULL }
};

/* Channel Rate and Type */
static const value_string gsm_a_bssap_channel_rate_and_type_vals[] = {
    { 0x08,  "Full rate TCH channel Bm.  Prefer full rate TCH"},
    { 0x09,  "Half rate TCH channel Lm.  Prefer half rate TCH"},
    { 0x0a,  "Full or Half rate channel, Full rate preferred changes allowed after first allocation"},
    { 0x0b,  "Full or Half rate channel, Half rate preferred changes allowed after first allocation"},
    { 0x0f,  "Full or Half rate channel, changes allowed after first allocation"},
    { 0x1a,  "Full or Half rate channel, Full rate preferred changes between full and half rate not allowed after first allocation"},
    { 0x1b,  "Full or Half rate channel, Half rate preferred changes between full and half rate not allowed after first allocation"},
    { 0x1f,  "Full or Half rate channel, changes between full and half rate not allowed after first allocation"},
    { 0,    NULL }
};

static const range_string gsm_a_bssap_channel_rate_and_type_rvals[] = {
    { 0x00,     0x00, "SDCCH or Full rate TCH channel Bm or Half rate TCH channel Lm" },
    { 0x01,     0x01, "SDCCH" },
    { 0x02,     0x02, "SDCCH or Full rate TCH channel Bm" },
    { 0x03,     0x03, "Half rate TCH channel Lm" },
    { 0x04,     0x07, "Reserved" },
    { 0x08,     0x08, "Full rate TCH channel Bm" },
    { 0x09,     0x09, "Half rate TCH channel Lm" },
    { 0x0a,     0x0a, "Full or Half rate TCH channel, Full rate preferred, changes allowed also after first channel allocation as a result of the request" },
    { 0x0b,     0x0b, "Full or Half rate TCH channel, Half rate preferred, changes allowed also after first channel allocation as a result of the request" },
    { 0x0c,     0x19, "Reserved" },
    { 0x1a,     0x1a, "Full or Half rate TCH channel, Full rate preferred, changes allowed also after first channel allocation as a result of the request" },
    { 0x1b,     0x1b, "Full or Half rate TCH channel, Half rate preferred, changes allowed also after first channel allocation as a result of the request" },
    { 0x1c,     0x1f, "Reserved" },
    { 0x20,     0x27, "Full rate TCH channels in a multislot configuration, changes by the BSS of the the number of TCHs and if applicable the used radio interface rate per channel allowed after first channel allocation as a result of the request" },
    { 0x28,     0x2f, "Reserved" },
    { 0x30,     0x37, "Full rate TCH channels in a multislot configuration, changes by the BSS of the number of TCHs or the used radio interface rate per channel not allowed after first channel allocation as a result of the request" },
    { 0x38,     0xff, "Reserved" },
    { 0, 0, NULL },
};

static const true_false_string tfs_non_transparent_transparent = {"Non-Transparent", "Transparent"};

guint16
be_chan_type(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len, gchar *add_string, int string_len)
{
    guint8       oct;
    guint8       sdi;
    guint8       num_chan;
    guint32      curr_offset = offset;
    const gchar *str;

    oct = tvb_get_guint8(tvb, curr_offset);
    sdi = oct & 0x0f;

    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, curr_offset<<3, 4, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_speech_data_ind, tvb, curr_offset, 1, ENC_BIG_ENDIAN);

    if (add_string)
        g_snprintf(add_string, string_len, " - (%s)", val_to_str_const(tvb_get_guint8(tvb, curr_offset) & 0x0f,
                gsm_a_bssap_speech_data_ind_vals,
                "Unknown"));

    curr_offset++;

    NO_MORE_DATA_CHECK(len);

    oct = tvb_get_guint8(tvb, curr_offset);

    if ((sdi == 0x01)||(sdi == 0x04))
    {
        /* speech */
        proto_tree_add_item(tree, hf_gsm_a_bssmap_channel_rate_and_type, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
        curr_offset++;
        NO_MORE_DATA_CHECK(len);

        do
        {
            proto_tree_add_item(tree, hf_gsm_a_bssmap_chan_type_extension, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
            proto_tree_add_item(tree, hf_gsm_a_bssmap_perm_speech_v_ind, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
            curr_offset++;
        }
        while ((len - (curr_offset - offset)) > 0);
    }
    else if (sdi == 0x02)
    {
        /* data */

        num_chan = 0;

        if ((oct >= 0x20) && (oct <= 0x27))
        {
            num_chan = (oct - 0x20) + 1;
        }
        else if ((oct >= 0x30) && (oct <= 0x37))
        {
            num_chan = (oct - 0x30) + 1;
        }

        if (num_chan > 0)
        {
            proto_tree_add_uint_format_value(tree, hf_gsm_a_bssmap_data_channel_rate_and_type,
                tvb, curr_offset, 1, oct, "Max channels %u, %s",
                num_chan, rval_to_str_const(oct, gsm_a_bssap_channel_rate_and_type_rvals, "Reserved"));
        }
        else
        {
            proto_tree_add_item(tree, hf_gsm_a_bssmap_data_channel_rate_and_type,
                tvb, curr_offset, 1, ENC_NA);
        }

        curr_offset++;

        NO_MORE_DATA_CHECK(len);

        oct = tvb_get_guint8(tvb, curr_offset);

        proto_tree_add_item(tree, hf_gsm_a_bssmap_chan_type_extension, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item(tree, hf_gsm_a_bssmap_transparent_service, tvb, curr_offset, 1, ENC_BIG_ENDIAN);

        if (num_chan == 0)
        {
            if (oct & 0x40)
            {
                /* non-transparent */

                switch (oct & 0x3f)
                {
                case 0x00: str = "12 kbit/s if the channel is a full rate TCH, or 6 kbit/s if the channel is a half rate TCH"; break;
                case 0x18: str = "14.5 kbit/s"; break;
                case 0x10: str = "12 kbits/s"; break;
                case 0x11: str = "6 kbits/s"; break;
                case 0x31: str = "29 kbit/s"; break;
                case 0x34: str = "43,5 kbit/s"; break;
                default:
                    str = "Reserved";
                    break;
                }
            }
            else
            {
                switch (oct & 0x3f)
                {
                case 0x18: str = "14.4 kbit/s"; break;
                case 0x10: str = "9.6kbit/s"; break;
                case 0x11: str = "4.8kbit/s"; break;
                case 0x12: str = "2.4kbit/s"; break;
                case 0x13: str = "1.2Kbit/s"; break;
                case 0x14: str = "600 bit/s"; break;
                case 0x15: str = "1200/75 bit/s (1200 network-to-MS / 75 MS-to-network)"; break;
                case 0x39: str = "28,8 kbit/s"; break;
                case 0x3a: str = "32,0 kbit/s"; break;
                default:
                    str = "Reserved";
                    break;
                }
            }
        }
        else
        {
            if (oct & 0x40)
            {
                /* non-transparent */

                switch (oct & 0x3f)
                {
                case 0x16: str = "58 kbit/s (4x14.5 kbit/s)"; break;
                case 0x14: str = "48.0 / 43.5 kbit/s (4x12 kbit/s or 3x14.5 kbit/s)"; break;
                case 0x13: str = "36.0 / 29.0 kbit/s (3x12 kbit/s or 2x14.5 kbit/s)"; break;
                case 0x12: str = "24.0 / 24.0 (4x6 kbit/s or 2x12 kbit/s)"; break;
                case 0x11: str = "18.0 / 14.5 kbit/s (3x6 kbit/s or 1x14.5 kbit/s)"; break;
                case 0x10: str = "12.0 / 12.0 kbit/s (2x6 kbit/s or 1x12 kbit/s)"; break;
                default:
                    str = "Reserved";
                    break;
                }
            }
            else
            {
                switch (oct & 0x3f)
                {
                case 0x1f: str = "64 kbit/s, bit transparent"; break;
                case 0x1e: str = "56 kbit/s, bit transparent"; break;
                case 0x1d: str = "56 kbit/s"; break;
                case 0x1c: str = "48 kbit/s"; break;
                case 0x1b: str = "38.4 kbit/s"; break;
                case 0x1a: str = "28.8 kbit/s"; break;
                case 0x19: str = "19.2 kbit/s"; break;
                case 0x18: str = "14.4 kbit/s"; break;
                case 0x10: str = "9.6 kbit/s"; break;
                default:
                    str = "Reserved";
                    break;
                }
            }
        }

        proto_tree_add_uint_format_value(tree, hf_gsm_a_bssmap_rate,
            tvb, curr_offset, 1, oct & 0x3f, "%s", str);

        curr_offset++;

        NO_MORE_DATA_CHECK(len);

        proto_tree_add_item(tree, hf_gsm_a_bssmap_chan_type_extension, tvb, curr_offset, 1, ENC_BIG_ENDIAN);

        proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, (curr_offset<<3)+1, 3, ENC_BIG_ENDIAN);

        if (num_chan == 0)
        {
            proto_tree_add_item(tree, hf_gsm_a_bssmap_tch_14_5kb, tvb, curr_offset, 1, ENC_NA);
            proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, (curr_offset<<3)+6, 1, ENC_BIG_ENDIAN);
            proto_tree_add_item(tree, hf_gsm_a_bssmap_tch_12kb, tvb, curr_offset, 1, ENC_NA);
            proto_tree_add_item(tree, hf_gsm_a_bssmap_tch_6kb, tvb, curr_offset, 1, ENC_NA);
        }
        else
        {
            proto_tree_add_item(tree, hf_gsm_a_bssmap_tch_14_5_14_4kb, tvb, curr_offset, 1, ENC_NA);
            proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, (curr_offset<<3)+6, 1, ENC_BIG_ENDIAN);
            proto_tree_add_item(tree, hf_gsm_a_bssmap_tch_12_9kb, tvb, curr_offset, 1, ENC_NA);
            proto_tree_add_item(tree, hf_gsm_a_bssmap_tch_6_4_8kb, tvb, curr_offset, 1, ENC_NA);
        }

        curr_offset++;
    }
    else if (sdi == 0x03)
    {
        /* signalling */
        proto_tree_add_item(tree, hf_gsm_a_bssmap_data_channel_rate_and_type,
            tvb, curr_offset, 1, ENC_NA);

        curr_offset++;

        NO_MORE_DATA_CHECK(len);

        proto_tree_add_item(tree, hf_gsm_a_bssmap_spare, tvb, curr_offset, len - (curr_offset - offset), ENC_NA);

        curr_offset += len - (curr_offset - offset);
    }
    else
    {
        /* unknown format */

        proto_tree_add_item(tree, hf_gsm_a_bssmap_unknown_format, tvb, curr_offset, len - (curr_offset - offset), ENC_NA);

        curr_offset += len - (curr_offset - offset);
    }

    EXTRANEOUS_DATA_CHECK(len, curr_offset - offset, pinfo, &ei_gsm_a_bssmap_extraneous_data);

    return(curr_offset - offset);
}
/*
 * 3.2.2.12 Periodicity
 */
static guint16
be_periodicity(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;
    proto_tree_add_item(tree, hf_gsm_a_bssmap_periodicity, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;

    return(curr_offset - offset);
}
/*
 * 3.2.2.13 Extended Resource Indicator
 */
#if 0
static const true_false_string bssmap_tarr_vals = {
   "The total number of accessible channels is requested",
   "No extra Resource Information is requested"
};
#endif
static guint16
be_ext_res_ind(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, curr_offset<<3, 6, ENC_BIG_ENDIAN);
    /* the Subsequent Mode field */
    proto_tree_add_item(tree, hf_gsm_a_bssmap_sm, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    /* Total Accessible Resource Requested field */
    proto_tree_add_item(tree, hf_gsm_a_bssmap_tarr, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;


    return(curr_offset - offset);
}
/*
 * 3.2.2.14 Total Resource Accessible
 */
static guint16
be_tot_res_acc(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    /* Total number of accessible full rate channels */
    proto_tree_add_item(tree, hf_gsm_a_bssmap_tot_no_of_fullr_ch, tvb, curr_offset, 2, ENC_BIG_ENDIAN);
    curr_offset+=2;
    /* Total number of accessible half rate channels */
    proto_tree_add_item(tree, hf_gsm_a_bssmap_tot_no_of_hr_ch, tvb, curr_offset, 2, ENC_BIG_ENDIAN);
    /*curr_offset+=2;*/


    return(len);
}
/*
 * 3.2.2.15 LSA Identifier
 * The octets 3-5 are coded as specified in 3GPP TS 23.003, 'Identification of Localised Service Area'. Bit 8 of octet 3 is the MSB.
 */
static guint16
be_lsa_id(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    /* TS 23.003:
     * The LSA ID consists of 24 bits, numbered from 0 to 23, with bit 0 being the LSB.
     * Bit 0 indicates whether the LSA is a PLMN significant number or a universal LSA.
     * If the bit is set to 0 the LSA is a PLMN significant number; if it is set to
     * 1 it is a universal LSA.
     */
    proto_tree_add_item(tree, hf_gsm_a_bssmap_lsa_id, tvb, curr_offset, 3, ENC_BIG_ENDIAN);
    /*curr_offset+=3;*/


    return(len);
}


/*
 * 3.2.2.16 LSA Identifier List
 */
static guint16
be_lsa_id_list(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, curr_offset<<3, 7, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_ep, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;

    /* LSA identification 1 - n */

    while (curr_offset-offset < len) {
        proto_tree_add_item(tree, hf_gsm_a_bssmap_lsa_id, tvb, curr_offset, 3, ENC_BIG_ENDIAN);
        curr_offset+=3;
    }

    return(len);
}
/*
 * [2] 3.2.2.17 Cell Identifier
 * Formats everything after the discriminator, shared function
 */
guint16
be_cell_id_aux(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len, gchar *add_string, int string_len, guint8 disc)
{
    guint32 value;
    guint32 curr_offset;

    if (add_string)
    add_string[0] = '\0';
    curr_offset = offset;

    switch (disc)
    {
    case 0x00:
        /* FALLTHRU */

    case 0x04:
        /* FALLTHRU */

    case 0x08:  /* For intersystem handover from GSM to UMTS or cdma2000: */
        /* FALLTHRU */
    case 0xb:
        /* Serving Area Identity, SAI, is used to identify the Serving Area of UE
         * within UTRAN or cdma2000.
         * Coding of Cell Identification for Cell identification discriminator = 1011
         * The coding of SAI is defined in 3GPP TS 25.413, without the protocol extension
         * container.
         * TS 25.413:
         * SAI ::= SEQUENCE {
         * pLMNidentity PLMNidentity,
         * lAC LAC,
         * sAC SAC,
         * iE-Extensions ProtocolExtensionContainer { {SAI-ExtIEs} } OPTIONAL
         * }
         */
        /* FALLTHRU */
    case 0x0c:  /* For identification of a UTRAN cell for cell load information: */
        if (disc != 0x0b)
            curr_offset = dissect_e212_mcc_mnc(tvb, pinfo, tree, curr_offset, E212_NONE, TRUE);
        else
            curr_offset = dissect_e212_mcc_mnc(tvb, pinfo, tree, curr_offset, E212_NONE, FALSE);
        /* FALLTHRU */

    case 0x01:
    case 0x05:
    case 0x0a: /*For intersystem handover from GSM to UMTS or cdma2000: */
        /* LAC */
        value = tvb_get_ntohs(tvb, curr_offset);
        proto_tree_add_item(tree, hf_gsm_a_bssmap_cell_lac, tvb, curr_offset, 2, ENC_BIG_ENDIAN);
        curr_offset += 2;

        if (add_string)
            g_snprintf(add_string, string_len, " - LAC (0x%04x)", value);
        /* FALLTHRU */
        if (disc == 0x0b) {
            /* If SAI, SAC follows */
            proto_tree_add_item(tree, hf_gsm_a_bssmap_sac, tvb, curr_offset, 2, ENC_BIG_ENDIAN);
            curr_offset += 2;
            break;
        }

    case 0x09: /* For intersystem handover from GSM to UMTS or cdma2000: */

        if ((disc == 0x08) ||(disc == 0x09) || (disc == 0x0a)|| (disc == 0x0c)) {
            /* RNC-ID
             * The octets 9-10 are coded as the RNC-ID (0..4095) or the
             * Extended RNC-ID (4096..65535) specified in 3GPP TS 25.413 [31]:
             * XXX is this a PER encoded number?
             */
            value = tvb_get_ntohs(tvb, curr_offset);
            proto_tree_add_item(tree, hf_gsm_a_bssmap_be_rnc_id, tvb, curr_offset, 2, ENC_BIG_ENDIAN);
            curr_offset += 2;

            if (add_string)
            {
                if (add_string[0] == '\0')
                {
                    g_snprintf(add_string, string_len, " - RNC-ID (%u)", value);
                }
                else
                {
                    g_snprintf(add_string, string_len, "%s/RNC-ID (%u)", add_string, value);
                }
            }
            break;
        }

        if ((disc == 0x04) || (disc == 0x05) || (disc == 0x08)) break;

        /* FALLTHRU */

    case 0x02:
        /* CI */

        value = tvb_get_ntohs(tvb, curr_offset);
        proto_tree_add_uint(tree, hf_gsm_a_bssmap_cell_ci, tvb,
            curr_offset, 2, value);

        curr_offset += 2;

        if (add_string)
        {
            if (add_string[0] == '\0')
            {
                g_snprintf(add_string, string_len, " - CI (%u)", value);
            }
            else
            {
                g_snprintf(add_string, string_len, "%s/CI (%u)", add_string, value);
            }
        }
        break;
    default:
        proto_tree_add_item(tree, hf_gsm_a_bssmap_cell_id_unknown_format, tvb, curr_offset, len, ENC_NA);

        curr_offset += (len);
        break;
    }

    return(curr_offset - offset);
}

static guint16
be_cell_id(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string, int string_len)
{
    guint8  oct;
    guint8  disc;
    guint32 curr_offset;

    curr_offset = offset;

    oct = tvb_get_guint8(tvb, curr_offset);

    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, curr_offset<<3, 4, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_be_cell_id_disc, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    disc = oct&0x0f;
    cell_discriminator = disc; /* may be required later */
    curr_offset++;

    NO_MORE_DATA_CHECK(len);

    curr_offset +=
    be_cell_id_aux(tvb, tree, pinfo, curr_offset, len - (curr_offset - offset), add_string, string_len, disc);

    /* no length check possible */

    return(curr_offset - offset);
}

/*
 * [2] 3.2.2.18 Priority
 */
static const true_false_string bssmap_pci_value = {
   "This allocation request may preempt an existing connection",
   "This allocation request shall not preempt an existing connection"
};

static const true_false_string bssmap_pvi_value = {
   "This connection might be preempted by another allocation request",
   "This connection shall not be preempted by another allocation request"
};

static const range_string bssmap_prio_rvals[] = {
    { 0x00,     0x00, "Spare" },
    { 0x01,     0x0E, "1 is highest" },
    { 0x0F,     0x0f, "priority not used" },
    { 0, 0, NULL },
};

guint16
be_prio(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string, int string_len)
{
    guint8       oct;
    guint32      curr_offset;

    curr_offset = offset;

    oct = tvb_get_guint8(tvb, curr_offset);

    proto_tree_add_item(tree, hf_gsm_a_b8spare, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_pci, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_priority_level, tvb, curr_offset, 1, ENC_NA);

    if (add_string)
        g_snprintf(add_string, string_len, " - (%u)", (oct & 0x3c) >> 2);

    proto_tree_add_item(tree, hf_gsm_a_bssmap_qa, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_pvi, tvb, curr_offset, 1, ENC_BIG_ENDIAN);

    curr_offset++;

    /* no length check possible */

    return(curr_offset - offset);
}
/*
 * 3.2.2.19 Classmark Information Type 2
 * The classmark octets 3, 4 and 5 are coded in the same way as the
 * equivalent octets in the Mobile station classmark 2 element of
 * 3GPP TS 24.008
 * dissected in packet-gsm_a_common.c
 */
/*
 * 3.2.2.20 Classmark Information Type 3
 * The classmark octets 3 to 34 are coded in the same way as the
 * equivalent octets in the Mobile station classmark 3 element of
 * 3GPP TS 24.008.
 * dissected in packet-gsm_a_common.c
 */
/*
 * 3.2.2.21 Interference Band To Be Used
 */
static guint16
be_int_band(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, curr_offset<<3, 3, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_interference_bands, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;


    return(curr_offset - offset);
}
/*
 * 3.2.2.22 RR Cause
 * Octet 2 is coded as the equivalent field from 3GPP TS 24.008
 * Dissected in packet-gsm_a_rr.c
 */
/*
 * 3.2.2.23 LSA Information
 */

static const true_false_string bssmap_lsa_only_value = {
   "Access to the LSAs that are defined ",
   "Allowing emergency call"
};

static guint16
be_lsa_info(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, curr_offset<<3, 7, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_lsa_only, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;

    while (curr_offset-offset < len) {
        /* LSA identification and attributes */
        /* 8    7   6   5    4  3   2   1
         * spare    act pref priority
         */
        proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, curr_offset<<3, 2, ENC_BIG_ENDIAN);
        proto_tree_add_item(tree, hf_gsm_a_bssmap_act, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item(tree, hf_gsm_a_bssmap_pref, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item(tree, hf_gsm_a_bssmap_lsa_inf_prio, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
        curr_offset++;
        proto_tree_add_item(tree, hf_gsm_a_bssmap_lsa_id, tvb, curr_offset, 3, ENC_BIG_ENDIAN);
        curr_offset+=3;
    }

    return(len);
}
/*
 * [2] 3.2.2.24 Layer 3 Information
 */
static guint16
be_l3_info(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_)
{
    guint32   curr_offset;
    tvbuff_t *l3_tvb;
    proto_item* ti;

    curr_offset = offset;

    proto_tree_add_bytes_format(tree, hf_gsm_a_bssmap_layer_3_information_value, tvb, curr_offset, len, NULL,
        "Layer 3 Information value");

    /*
     * dissect the embedded DTAP message
     */
    l3_tvb = tvb_new_subset_length(tvb, curr_offset, len);

    /* This information element carries a radio interface message.
       In the case of an Intersystem handover to UMTS,
       this information element contains a HANDOVER TO UTRAN COMMAND message
       as defined in 3GPP TS 25.331.
       In the case of an Inter BSC handover,
       it contains an RR HANDOVER COMMAND message as defined in 3GPP TS 44.018.
       In the case of an Intersystem handover to cdma2000,
       this information element contains the HANDOVER TO CDMA2000 COMMAND message,
       as defined in 3GPP TS 44.018. */

    /* note that we can't (from this PDU alone) determine whether a handover is to UMTS or cdma2000
       Maybe if cdma2000 support is added later, a preference option would select dissection of cdma2000 or UMTS.
       If SCCP trace is enabled (and the cell discriminator has correctly appeared in an earlier PDU)
       then we will have remembered the discriminator */
    if ( cell_discriminator == 0xFF)
    {
        ti = proto_tree_add_uint_format(tree, hf_gsm_a_bssmap_cell_discriminator, l3_tvb, curr_offset, 1, cell_discriminator,
            "Cell Discriminator not initialised, try enabling the SCCP protocol option [Trace Associations], \n or maybe the file does not contain the PDUs needed for SCCP trace");
        proto_item_set_len(ti, len);
    }
    else if ((cell_discriminator & 0x0f) < 8) {
        ti = proto_tree_add_uint(tree, hf_gsm_a_bssmap_cell_discriminator, l3_tvb, curr_offset, 1, cell_discriminator);
        /* cell_discriminator is a preference, so value should be known, but keeping presence of field consistent for filtering */
        PROTO_ITEM_SET_HIDDEN(ti);

        /* GSM */
        call_dissector(dtap_handle, l3_tvb, pinfo, g_tree);
    }
    else if ((cell_discriminator & 0x0f) < 13) {
        ti = proto_tree_add_uint(tree, hf_gsm_a_bssmap_cell_discriminator, l3_tvb, curr_offset, 1, cell_discriminator);
        /* cell_discriminator is a preference, so value should be known, but keeping presence of field consistent for filtering */
        PROTO_ITEM_SET_HIDDEN(ti);

        /* UMTS or CDMA 2000 */
        dissect_rrc_HandoverToUTRANCommand_PDU(l3_tvb, pinfo, g_tree, NULL);
    }
    else{
        ti = proto_tree_add_uint_format(tree, hf_gsm_a_bssmap_cell_discriminator, l3_tvb, curr_offset, 1, cell_discriminator,
                        "Unrecognised Cell Discriminator %x",cell_discriminator);
        proto_item_set_len(ti, len);
    }
    curr_offset += len;

    EXTRANEOUS_DATA_CHECK(len, curr_offset - offset, pinfo, &ei_gsm_a_bssmap_extraneous_data);

    return(curr_offset - offset);
}

/*
 * [2] 3.2.2.25 DLCI
 */
static guint16
be_dlci(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint8      oct;
    guint32     curr_offset;
    proto_tree *subtree;

    curr_offset = offset;

    subtree =
    proto_tree_add_subtree(tree, tvb, curr_offset, 1,
        ett_dlci, NULL, "Data Link Connection Identifier");

    oct = tvb_get_guint8(tvb, curr_offset);

    proto_tree_add_uint(subtree, hf_gsm_a_bssmap_dlci_cc, tvb, curr_offset, 1, oct);
    proto_tree_add_uint(subtree, hf_gsm_a_bssmap_dlci_spare, tvb, curr_offset, 1, oct);
    proto_tree_add_uint(subtree, hf_gsm_a_bssmap_dlci_sapi, tvb, curr_offset, 1, oct);

    curr_offset++;

    /* no length check possible */

    return(curr_offset - offset);
}

/*
 * [2] 3.2.2.26 Downlink DTX Flag
 */
static guint16
be_down_dtx_flag(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset  = offset;

    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, curr_offset<<3, 7, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_bss_activate_downlink, tvb, curr_offset, 1, ENC_NA);
    curr_offset++;

    /* no length check possible */

    return(curr_offset - offset);
}

/*
 * [2] 3.2.2.27 Cell Identifier List
 */
guint16
be_cell_id_list(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len, gchar *add_string, int string_len)
{
    guint8      oct;
    guint16     consumed;
    guint8      disc;
    guint8      num_cells;
    guint32     curr_offset;
    proto_item *item    = NULL;
    proto_tree *subtree = NULL;

    curr_offset = offset;

    oct = tvb_get_guint8(tvb, curr_offset);

    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, curr_offset<<3, 4, ENC_BIG_ENDIAN);

    disc = oct & 0x0f;
    cell_discriminator = disc; /* may be required later */
    proto_tree_add_item(tree, hf_gsm_a_bssmap_be_cell_id_disc, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;

    NO_MORE_DATA_CHECK(len);

    num_cells = 0;
    do
    {
        subtree =
        proto_tree_add_subtree_format(tree,
            tvb, curr_offset, -1,
            ett_cell_list, &item, "Cell %u",
            num_cells + 1);

        if (add_string)
            add_string[0] = '\0';

        consumed =
            be_cell_id_aux(tvb, subtree, pinfo, curr_offset, len - (curr_offset - offset), add_string, string_len, disc);

        if (add_string && add_string[0] != '\0')
            proto_item_append_text(item, "%s", add_string);

        proto_item_set_len(item, consumed);

        curr_offset += consumed;

        num_cells++;
    }
    while ((len - (curr_offset - offset)) > 0 && consumed > 0);

    if (add_string) {
        g_snprintf(add_string, string_len, " - %u cell%s",
            num_cells, plurality(num_cells, "", "s"));
    }

    EXTRANEOUS_DATA_CHECK(len, curr_offset - offset, pinfo, &ei_gsm_a_bssmap_extraneous_data);

    return(curr_offset - offset);
}
/*
 * 3.2.2.27a    Cell Identifier List Segment
 */

static const value_string gsm_a_bssap_cell_id_list_seg_cell_id_disc_vals[] = {
    { 0x0,  "The whole Cell Global Identification, CGI, is used to identify the cells"},
    { 0x1,  "Location Area Code, LAC, and Cell Identify, CI, is used to identify the cells within a given MCC and MNC"},
    { 0x2,  "Cell Identity, CI, is used to identify the cells within a given MCC and MNC and LAC"},
    { 0x3,  "No cell is associated with the transaction"},
    { 0x4,  "Location Area Identification, LAI, is used to identify all cells within a Location Area"},
    { 0x5,  "Location Area Code, LAC, is used to identify all cells within a location area"},
    { 0x6,  "All cells on the BSS are identified"},
    { 0x7,  "MCC and MNC, is used to identify all cells within the given MCC and MNC"},
    { 0,    NULL }
};

static guint16
be_cell_id_list_seg(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    /* Sequence Length */
    proto_tree_add_item(tree, hf_gsm_a_bssmap_seq_len, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    /* Sequence Number */
    proto_tree_add_item(tree, hf_gsm_a_bssmap_seq_no, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;

    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, curr_offset<<3, 4, ENC_BIG_ENDIAN);
    /* Cell identification discriminator */
    proto_tree_add_item(tree, hf_gsm_a_bssap_cell_id_list_seg_cell_id_disc, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;
    proto_tree_add_expert(tree, pinfo, &ei_gsm_a_bssmap_not_decoded_yet, tvb, curr_offset, len-2);


    return(len);
}

/*
 * 3.2.2.27b    Cell Identifier List Segment for established cells
 */
static guint16
be_cell_id_lst_seg_f_est_cells(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, curr_offset<<3, 4, ENC_BIG_ENDIAN);
    /* Cell identification discriminator */
    proto_tree_add_item(tree, hf_gsm_a_bssap_cell_id_list_seg_cell_id_disc, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;

    proto_tree_add_expert(tree, pinfo, &ei_gsm_a_bssmap_not_decoded_yet, tvb, curr_offset, len-1);


    return(len);
}
/*
 * 3.2.2.27c    Cell Identifier List Segment for cells to be established
 */
static guint16
be_cell_id_lst_seg_f_cell_tb_est(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, curr_offset<<3, 4, ENC_BIG_ENDIAN);
    /* Cell identification discriminator */
    proto_tree_add_item(tree, hf_gsm_a_bssap_cell_id_list_seg_cell_id_disc, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;

    proto_tree_add_expert(tree, pinfo, &ei_gsm_a_bssmap_not_decoded_yet, tvb, curr_offset, len-1);


    return(len);
}
/*
 * 3.2.2.27d    (void)
 */
/*
 * 3.2.2.27e    Cell Identifier List Segment for released cells - no user present
 */
static guint16
be_cell_id_lst_seg_f_rel_cell(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, curr_offset<<3, 4, ENC_BIG_ENDIAN);
    /* Cell identification discriminator */
    proto_tree_add_item(tree, hf_gsm_a_bssap_cell_id_list_seg_cell_id_disc, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;

    proto_tree_add_expert(tree, pinfo, &ei_gsm_a_bssmap_not_decoded_yet, tvb, curr_offset, len-1);


    return(len);
}
/*
 * 3.2.2.27f    Cell Identifier List Segment for not established cells - no establishment possible
 */
static guint16
be_cell_id_lst_seg_f_not_est_cell(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, curr_offset<<3, 4, ENC_BIG_ENDIAN);
    /* Cell identification discriminator */
    proto_tree_add_item(tree, hf_gsm_a_bssap_cell_id_list_seg_cell_id_disc, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;

    proto_tree_add_expert(tree, pinfo, &ei_gsm_a_bssmap_not_decoded_yet, tvb, curr_offset, len-1);


    return(len);
}
/*
 * 3.2.2.28 Response Request
 * No data
 */
/*
 * 3.2.2.29 Resource Indication Method
 */
static const value_string gsm_a_bssap_resource_indication_vals[] = {
    { 0x0,  "Spontaneous resource information expected"},
    { 0x1,  "One single resource information expected"},
    { 0x2,  "Periodic resource information expected"},
    { 0x3,  "No cell is associated with the transaction"},
    { 0x4,  "No resource information expected"},
    { 0,    NULL }
};
static guint16
be_res_ind_method(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, curr_offset<<3, 4, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssap_res_ind_method, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;

    return(len);
}

/*
 * 3.2.2.30 Classmark Information Type 1
 * coded in the same way as the equivalent octet in the classmark 1 element of 3GPP TS 24.008
 * dissected in packet-gsm_a_common.c
 */
/*
 * 3.2.2.31 Circuit Identity Code List
 */
static guint16
be_cic_list(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    proto_tree_add_item(tree, hf_gsm_a_bssap_cic_list_range, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssap_cic_list_status, tvb, (curr_offset+1), (len-1), ENC_NA);

    return(len);
}
/*
 * 3.2.2.32 Diagnostics
 */
static guint16
be_diag(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len , gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    proto_tree_add_item(tree, hf_gsm_a_bssap_diag_error_pointer, tvb, curr_offset, 2, ENC_BIG_ENDIAN);
    curr_offset += 2;
    NO_MORE_DATA_CHECK(len);
    proto_tree_add_item(tree, hf_gsm_a_bssap_diag_msg_rcv, tvb, curr_offset, (len-2), ENC_NA);

    return(len);
}
/*
 * [2] 3.2.2.33 Chosen Channel
 */
static const value_string gsm_a_bssmap_ch_mode_vals[] = {
    { 0,    "no channel mode indication" },
    { 9,    "speech (full rate or half rate)" },
    { 14,   "data, 14.5 kbit/s radio interface rate" },
    { 11,   "data, 12.0 kbit/s radio interface rate" },
    { 12,   "data, 6.0 kbit/s radio interface rate" },
    { 13,   "data, 3.6 kbit/s radio interface rate" },
    { 8,    "signalling only" },
    { 1,    "data, 29.0 kbit/s radio interface rate" },
    { 2,    "data, 32.0 kbit/s radio interface rate" },
    { 3,    "data, 43.5 kbit/s radio interface rate" },
    { 4,    "data, 43.5 kbit/s downlink and 14.5 kbit/s uplink" },
    { 5,    "data, 29.0 kbit/s downlink and 14.5 kbit/s uplink" },
    { 6,    "data, 43.5 kbit/s downlink and 29.0 kbit/s uplink" },
    { 7,    "data, 14.5 kbit/s downlink and 43.5 kbit/s uplink" },
    { 10,   "data, 14.5 kbit/s downlink and 29.0 kbit/s uplink" },
    { 15,   "data, 29.0 kbit/s downlink and 43.5 kbit/s uplink" },
    { 0, NULL }
};
static const value_string gsm_a_bssmap_channel_vals[] = {
    { 0,    "None(Current Channel Type 1 - Reserved)" },
    { 1,    "SDCCH" },
    { 2,    "Reserved" },
    { 3,    "Reserved" },
    { 5,    "Reserved" },
    { 6,    "Reserved" },
    { 7,    "Reserved" },
    { 8,    "1 Full rate TCH" },
    { 9,    "1 Half rate TCH" },
    { 10,   "2 Full Rate TCHs" },
    { 11,   "3 Full Rate TCHs" },
    { 12,   "4 Full Rate TCHs" },
    { 13,   "5 Full Rate TCHs" },
    { 14,   "6 Full Rate TCHs" },
    { 15,   "7 Full Rate TCHs" },
    { 4,    "8 Full Rate TCHs" },
    { 0, NULL }
};
static const value_string gsm_a_bssmap_trace_bss_record_type_vals[] = {
    { 0,    "Basic" },
    { 1,    "Handover" },
    { 2,    "Radio" },
    { 3,    "No BSS Trace" },
    { 0, NULL }
};
static const value_string gsm_a_bssmap_trace_msc_record_type_vals[] = {
    { 0,    "Basic" },
    { 1,    "Detailed (optional)" },
    { 2,    "Spare" },
    { 3,    "No MSC Trace" },
    { 0, NULL }
};
static const value_string gsm_a_bssmap_trace_invoking_event_vals[] = {
    { 0,    "MOC, MTC, SMS MO, SMS MT, PDS MO, PDS MT, SS, Location Updates, IMSI attach, IMSI detach" },
    { 1,    "MOC, MTC, SMS_MO, SMS_MT, PDS MO, PDS MT, SS only" },
    { 2,    "Location updates, IMSI attach IMSI detach only" },
    { 3,    "Operator definable" },
    { 0, NULL }
};
static guint16
be_chosen_chan(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    /* Channel mode */
    proto_tree_add_item(tree, hf_gsm_a_bssmap_ch_mode, tvb, curr_offset, 1, ENC_BIG_ENDIAN);

    proto_tree_add_item(tree, hf_gsm_a_bssmap_channel, tvb, curr_offset, 1, ENC_BIG_ENDIAN);

    curr_offset++;

    /* no length check possible */

    return(curr_offset - offset);
}

/*
 * [2] 3.2.2.34 Cipher Response Mode
 */
static guint16
be_ciph_resp_mode(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset = offset;

    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, curr_offset<<3, 7, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_imeisv_included, tvb, curr_offset, 1, ENC_NA);

    curr_offset++;

    /* no length check possible */

    return(curr_offset - offset);
}


/*
 * [2] 3.2.2.35 Layer 3 Message Contents
 */
static guint16
be_l3_msg(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_)
{
    tvbuff_t *l3_tvb;
    guint16 word;

    proto_tree_add_bytes_format(tree, hf_gsm_a_bssmap_layer3_message_contents, tvb, offset, len, NULL,
        "Layer 3 Message Contents");

    /*
     * dissect the embedded DTAP message
     */
    l3_tvb = tvb_new_subset_length(tvb, offset, len);

    /* Some vendors do:
     * Octets 3-12 contain the unchanged radio interface layer 3 message contents, as received from the radio interface.
     * When received in the CIPHER MODE COMPLETE message, this IE contains the mobile identity IE with identity type set to IMEISV.
     * The mobile identity IE is a variable length element and includes a length indicator, which is set to 9 if the type is IMEISV.
     *
     */
    word = tvb_get_ntohs(tvb, offset);
    if(word==0x1709){
        /* start the dissection from byte 3 */
        de_mid(l3_tvb, tree, pinfo, 2, 9, NULL, 0);
        return(len);
    }
    /* Octet j (j = 3, 4, ..., n) is the unchanged octet j of a radio interface layer 3 message
     * as defined in 3GPP TS 24.008, n is equal to the length of that radio interface layer 3 message. */
    call_dissector(dtap_handle, l3_tvb, pinfo, g_tree);

    return(len);
}

/*
 * [2] 3.2.2.36 Channel Needed
 */
static guint16
be_cha_needed(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    /* no length check possible */
    proto_tree_add_bits_item(tree, hf_gsm_a_rr_chnl_needed_ch1, tvb, (curr_offset<<3)+6, 2, ENC_BIG_ENDIAN);

    curr_offset++;

    return(curr_offset - offset);
}
/*
 * 3.2.2.37 Trace Type
 * coded as the MSC/BSS Trace Type specified in 3GPP TS 52.008
 */
static guint16
be_trace_type(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;
    gint    bit_offset;

    bit_offset = (offset<<3);
    curr_offset = offset;

    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_trace_priority_indication, tvb, bit_offset, 1, ENC_BIG_ENDIAN);
    bit_offset ++;
    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, bit_offset, 1, ENC_BIG_ENDIAN);
    bit_offset ++;
    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_trace_bss_record_type, tvb, bit_offset, 2, ENC_BIG_ENDIAN);
    bit_offset += 2;
    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_trace_msc_record_type, tvb, bit_offset, 2, ENC_BIG_ENDIAN);
    bit_offset += 2;
    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_trace_invoking_event, tvb, bit_offset, 2, ENC_BIG_ENDIAN);
    /*bit_offset += 2;*/
    curr_offset++;

    /* no length check possible */

    return(curr_offset - offset);
}

/*
 * [2] 3.2.2.38 TriggerID
 */
static guint16
be_trace_trigger_id(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    proto_tree_add_item(tree, hf_gsm_a_bssmap_trace_trigger_id, tvb, curr_offset, len, ENC_ASCII|ENC_NA);
    curr_offset += len;

    /* no length check possible */

    return(curr_offset - offset);
}

    /* 3.2.2.39 Trace Reference */
static guint16
be_trace_reference(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;


    proto_tree_add_item(tree, hf_gsm_a_bssmap_trace_reference, tvb, curr_offset, 2, ENC_BIG_ENDIAN);
    curr_offset +=2;

    /* no length check possible */

    return(curr_offset - offset);
}
    /* 3.2.2.40 TransactionID */
static guint16
be_trace_transaction_id(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;


    if (len == 1)
    {
        proto_tree_add_item(tree, hf_gsm_a_bssmap_trace_reference, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
        curr_offset ++;
    }
    else
    {
        proto_tree_add_item(tree, hf_gsm_a_bssmap_trace_reference, tvb, curr_offset, 2, ENC_BIG_ENDIAN);
        curr_offset +=2;
    }

    EXTRANEOUS_DATA_CHECK(len, curr_offset - offset, pinfo, &ei_gsm_a_bssmap_extraneous_data);

    return(curr_offset - offset);
}
/*
 * 3.2.2.41 Mobile Identity (IMSI, IMEISV or IMEI as coded in 3GPP TS 24.008)
 * Dissected in packet-gsm_a_common.c
 */
/*
 * 3.2.2.42 OMCID
 * For the OMC identity, see 3GPP TS 52.021
 */
static guint16
be_trace_omc_id(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    proto_tree_add_item(tree, hf_gsm_a_bssmap_trace_omc_id, tvb, curr_offset, len, ENC_ASCII|ENC_NA);
    curr_offset += len;

    /* no length check possible */

    return(curr_offset - offset);
}
/*
 * [2] 3.2.2.43 Forward Indicator
 */
static const range_string forward_indicator_rvals[] = {
    { 0x00,     0x00, "Reserved" },
    { 0x01,     0x01, "forward to subsequent BSS, no trace at MSC" },
    { 0x02,     0x02, "forward to subsequent BSS, and trace at MSC" },
    { 0x03,     0x0F, "Reserved" },
    { 0, 0, NULL },
};

static guint16
be_for_ind(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32      curr_offset = offset;

    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, curr_offset<<3, 4, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_forward_indicator, tvb, curr_offset, 1, ENC_NA);
    curr_offset++;

    /* no length check possible */

    return(curr_offset - offset);
}

/*
 * [2] 3.2.2.44 Chosen Encryption Algorithm
 */
static const value_string gsm_a_bssmap_algorithm_id_vals[] = {
    { 1,    "No encryption used" },
    { 2,    "GSM A5/1" },
    { 3,    "GSM A5/2" },
    { 4,    "GSM A5/3" },
    { 5,    "GSM A5/4" },
    { 6,    "GSM A5/5" },
    { 7,    "GSM A5/6" },
    { 8,    "GSM A5/7" },
    { 0, NULL }
};

static guint16
be_chosen_enc_alg(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string, int string_len)
{
    guint8       oct;
    guint32      curr_offset;

    curr_offset = offset;

    oct = tvb_get_guint8(tvb, curr_offset);

    proto_tree_add_item(tree, hf_gsm_a_bssmap_algorithm_identifier, tvb, curr_offset, 1, ENC_NA);

    curr_offset++;

    if (add_string)
        g_snprintf(add_string, string_len, " - %s", val_to_str_const(oct, gsm_a_bssmap_algorithm_id_vals, "Unknown"));

    /* no length check possible */

    return(curr_offset - offset);
}

/*
 * [2] 3.2.2.45 Circuit Pool
 */
static guint16
be_cct_pool(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string, int string_len)
{
    guint8       oct;
    guint32      curr_offset;
    proto_item*  ti;

    curr_offset = offset;

    oct = tvb_get_guint8(tvb, curr_offset);

    ti = proto_tree_add_item(tree, hf_gsm_a_bssmap_circuit_pool_number, tvb, curr_offset, 1, ENC_NA);
    if (oct <= 50)
    {
        /* No extra string */
    }
    else if ((oct >= 0x80) && (oct <= 0x8f))
    {
        proto_item_append_text(ti, ", for national/local use");
    }
    else
    {
        proto_item_append_text(ti, ", reserved for future international use");
    }

    curr_offset++;

    if (add_string)
        g_snprintf(add_string, string_len, " - (%u)", oct);

    /* no length check possible */

    return(curr_offset - offset);
}
/*
 * 3.2.2.46 Circuit Pool List
 * 3.2.2.47 Time Indication
 * 3.2.2.48 Resource Situation
 */
/*
 * [2] 3.2.2.49 Current Channel Type 1
 */
static guint16
be_curr_chan_1(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    /* Channel mode */
    proto_tree_add_item(tree, hf_gsm_a_bssmap_cur_ch_mode, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    /* Channel */
    proto_tree_add_item(tree, hf_gsm_a_bssmap_channel, tvb, curr_offset, 1, ENC_BIG_ENDIAN);

    curr_offset++;

    /* no length check possible */

    return(curr_offset - offset);
}

/*
 * [2] 3.2.2.50 Queuing Indicator
 */
static guint16
be_que_ind(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset = offset;

    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, curr_offset<<3, 6, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_qri, tvb, curr_offset, 1, ENC_NA);

    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, (curr_offset<<3)+7, 1, ENC_BIG_ENDIAN);

    curr_offset++;

    /* no length check possible */

    return(curr_offset - offset);
}

/*
 * [2] 3.2.2.51 Speech Version
 */
static const range_string speech_version_id_rvals[] = {
    { 0x01,     0x01, "GSM speech full rate version 1" },
    { 0x02,     0x04, "Reserved" },
    { 0x05,     0x05, "GSM speech half rate version 1" },
    { 0x06,     0x10, "Reserved" },
    { 0x11,     0x11, "GSM speech full rate version 1" },
    { 0x12,     0x14, "Reserved" },
    { 0x15,     0x15, "GSM speech half rate version 2" },
    { 0x16,     0x20, "Reserved" },
    { 0x21,     0x21, "GSM speech full rate version 3 (AMR)" },
    { 0x22,     0x24, "Reserved" },
    { 0x25,     0x25, "GSM speech half rate version 3 (AMR)" },
    { 0x26,     0x40, "Reserved" },
    { 0x41,     0x41, "GSM speech full rate version 4" },
    { 0x42,     0x42, "GSM speech full rate version 5" },
    { 0x43,     0x44, "Reserved" },
    { 0x45,     0x45, "GSM speech half rate version 6" },
    { 0x46,     0x46, "GSM speech half rate version 4" },
    { 0x47,     0x7f, "Reserved" },

    { 0, 0, NULL },
};

static const range_string speech_version_id_short_rvals[] = {
    { 0x01,     0x01, "FR1" },
    { 0x05,     0x05, "HR1" },
    { 0x11,     0x11, "FR12" },
    { 0x15,     0x15, "HR2" },
    { 0x21,     0x21, "FR3 (AMR)" },
    { 0x25,     0x25, "HR3 (AMR)" },
    { 0x41,     0x41, "OFR AMR-WB" },
    { 0x42,     0x42, "FR AMR-WB" },
    { 0x45,     0x45, "OHR AMR" },
    { 0x46,     0x46, "OHR AMR-WB" },

    { 0, 0, NULL },
};

static guint16
be_speech_ver(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string, int string_len)
{
    guint8       oct;
    guint32      curr_offset;

    curr_offset = offset;

    oct = tvb_get_guint8(tvb, curr_offset);

    proto_tree_add_item(tree, hf_gsm_a_b8spare, tvb, curr_offset, 1, ENC_BIG_ENDIAN);

    /* The bits 7-1 of octet 2 are coded in the same way as the permitted speech version identifier
     * in the Channel type information element.
     */
    proto_tree_add_item(tree, hf_gsm_a_bssmap_speech_version_id, tvb, curr_offset, 1, ENC_NA);
    curr_offset++;

    if (add_string)
        g_snprintf(add_string, string_len, " - (%s)", rval_to_str_const(oct & 0x7f, speech_version_id_short_rvals, "Reserved"));

    /* no length check possible */

    return(curr_offset - offset);
}
/*
 * 3.2.2.52 Assignment Requirement
 */
static const value_string gsm_a_bssmap_assignment_requirement_vals[] = {
    { 0x00, "Delay allowed" },
    { 0x01, "Immediate and the resources shall not be de-allocated until the end of the call (channel establishment on demand forbidden by the MSC)" },
    { 0x02, "Immediate and the resources may further be de-allocated by the BSS (channel establishment on demand permitted by the MSC)." },
    { 0, NULL }
};
static guint16
be_ass_req(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    proto_tree_add_item(tree, hf_gsm_a_bssmap_ass_req, tvb, offset, 1, ENC_BIG_ENDIAN);

    return 1;
}
/*
 * 3.2.2.53 (void)
 */
/*
 * 3.2.2.54 Talker Flag
 * No data
 */
/*
 * 3.2.2.55 Group Call Reference
 * The octets 3-7 are coded in the same way as the octets 2-6 in the
 * Descriptive group or broadcast call reference information element as defined in 3GPP TS 24.008.
 * dissected in packet-gsm_a_common.c (de_d_gb_call_ref)
 */
/*
 * 3.2.2.56 eMLPP Priority
 * The call priority field (bit 3 to 1 of octet 2) is coded in the same way as the call priority field
 * (bit 3 to 1 of octet 5) in the Descriptive group or broadcast call reference information element as
 * defined in 3GPP TS 24.008.
 */
static const value_string gsm_a_bssmap_call_priority_vals[] = {
    { 0x00, "No priority applied" },
    { 0x01, "Call priority level 4" },
    { 0x02, "Call priority level 3" },
    { 0x03, "Call priority level 2" },
    { 0x04, "Call priority level 1" },
    { 0x05, "Call priority level 0" },
    { 0x06, "Call priority level B" },
    { 0x07, "Call priority level A" },
    { 0, NULL }
};

guint16
be_emlpp_prio(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, offset << 3, 5, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_emlpp_prio, tvb, offset, 1, ENC_BIG_ENDIAN);

    return 1;
}

/*
 * 3.2.2.57 Configuration Evolution Indication
 */
static const value_string gsm_a_bssmap_smi_vals[] = {
    { 0,    "No Modification is allowed" },
    { 1,    "Modification is allowed and maximum number of TCH/F is 1" },
    { 2,    "Modification is allowed and maximum number of TCH/F is 2" },
    { 3,    "Modification is allowed and maximum number of TCH/F is 3" },
    { 4,    "Modification is allowed and maximum number of TCH/F is 4" },
    { 5,    "Reserved" },
    { 6,    "Reserved" },
    { 7,    "Reserved" },
    { 0, NULL }
};

static guint16
be_conf_evo_ind(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, curr_offset<<3, 4, ENC_BIG_ENDIAN);
    /* Subsequent Modification Indication */
    proto_tree_add_item(tree, hf_gsm_a_bssmap_smi, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;

    return(curr_offset - offset);
}
/*
 * 3.2.2.58 Old BSS to New BSS information
 */
/* This function is only called from other protocols (e.g. RANAP),
   internally, the Field Element dissector is called directly */
void
bssmap_old_bss_to_new_bss_info(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo)
{
    guint16 len;
    if (!tree) {
        return;
    }

    g_tree = tree;

    len = tvb_reported_length(tvb);
    be_field_element_dissect(tvb, tree, pinfo, 0, len, NULL, 0);

    g_tree = NULL;
}
/*
 * 3.2.2.59 (void)
 * 3.2.2.60 LCS QoS
 * (The QoS octets 3 to n are coded in the same way as the equivalent octets
 * in the LCS QoS element of 3GPP TS 49.031.)
 */

/*
 * 3.2.2.61 LSA Access Control Suppression
 */
/*
 * 3.2.2.62 LCS Priority
 *  The Priority octets 3 to n are coded in the same way as the equivalent octets
 *  in the LCS Priority element of 3GPP TS 49.031.
 */
/* Location Information definitions */
static const value_string lcs_priority_vals[] = {
    { 0, "highest" },
    { 1, "normal" },
    { 0, NULL}
};

static guint16
be_lcs_prio(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    /* This octet is coded as the LCS-Priority octet in 3GPP TS 29.002 */
    proto_tree_add_item(tree, hf_gsm_a_bssmap_lcs_pri, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;

    return(curr_offset - offset);
}

/*
 * 3.2.2.63 Location Type (Location Type element of 3GPP TS 49.031 BSSAP-LE.)
 */
static guint16
be_loc_type(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;
    guint8  location_information;

    curr_offset = offset;

    /* Extract the location information and add to protocol tree */
    location_information = tvb_get_guint8(tvb, offset);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_location_type_location_information, tvb, offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;

    if (location_information == 1 || location_information == 2)
    {
        /* protocol method  */
        proto_tree_add_item(tree, hf_gsm_a_bssmap_location_type_positioning_method, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
        curr_offset++;
    }

    return(curr_offset - offset);
}

/*
 * 3.2.2.64 Location Estimate
 * The Location Estimate field is composed of 1 or more octets with an internal structure
 * according to 3GPP TS 23.032.
 */
static guint16
be_loc_est(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    tvbuff_t *data_tvb;
    guint32   curr_offset;

    curr_offset = offset;

    data_tvb = tvb_new_subset_length(tvb, curr_offset, len);
    dissect_geographical_description(data_tvb, pinfo, tree);

    return(len);
}
/*
 * 3.2.2.65 Positioning Data
 * Positioning Data element of 3GPP TS 49.031 BSSAP-LE.
 */
static guint16
be_pos_data(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;
    guint8  i;
    guint64 pos_data_disc;
    gint    bit_offset;

    curr_offset = offset;

    /* Spare bits */
    bit_offset = (offset<<3);
    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare, tvb, bit_offset, 4, ENC_BIG_ENDIAN);
    bit_offset += 4;

    /* Extract the positioning data discriminator and add to protocol tree */
    proto_tree_add_bits_ret_val(tree, hf_gsm_a_bssmap_positioning_data_discriminator, tvb, bit_offset, 4, &pos_data_disc, ENC_BIG_ENDIAN);
    bit_offset += 4;
    curr_offset++;

    if (pos_data_disc == 0)
    {
        /* Extract the positioning methods and add to protocol tree */
        for (i = 0; i < len-1; i++)
        {
            /* Extract the positioning method and add to protocol tree */
            proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_positioning_method, tvb, bit_offset, 5, ENC_BIG_ENDIAN);
            bit_offset += 5;
            /* Extract the usage and add to protocol tree */
            proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_positioning_method_usage, tvb, bit_offset, 3, ENC_BIG_ENDIAN);
            bit_offset += 3;
            curr_offset++;
        }
    }

    return(curr_offset - offset);
}
/*
 * 3.2.2.66 LCS Cause
 * LCS Cause element of 3GPP TS 49.031 BSSAP-LE.
 * Dissected in packet-gsm_bssap_le.c
 */

/*
 * 3.2.2.67 LCS Client Type
 * LCS Client Type element of 3GPP TS 49.031 BSSAP-LE.
 * Dissected in packet-gsm_bssap_le.c
 */

/*
 * 3.2.2.68 3GPP TS 48.008 version 6.9.0 Release 6
 */

/* BSSLAP the embedded message is as defined in 3GPP TS 48.071
 * LLP the embedded message contains a Facility Information Element as defined in 3GPP TS 44.071
 *      excluding the Facility IEI and length of Facility IEI octets defined in 3GPP TS 44.071.
 * SMLCPP the embedded message is as defined in 3GPP TS 48.031
 */
static const value_string gsm_a_apdu_protocol_id_strings[] = {
    { 0,    "reserved" },
    { 1,    "BSSLAP" },
    { 2,    "LLP" },
    { 3,    "SMLCPP" },
    { 0, NULL }
};

static guint16
be_apdu(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_)
{
    guint32   curr_offset;
    guint8    apdu_protocol_id;
    tvbuff_t *APDU_tvb;

    curr_offset = offset;

    proto_tree_add_bytes_format(tree, hf_gsm_a_bssmap_apdu, tvb, curr_offset, len, NULL, "APDU");

    /*
     * dissect the embedded APDU message
     * if someone writes a TS 09.31 dissector
     *
     * The APDU octets 4 to n are coded in the same way as the
     * equivalent octet in the APDU element of 3GPP TS 49.031 BSSAP-LE.
     */

    apdu_protocol_id = tvb_get_guint8(tvb,curr_offset);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_apdu_protocol_id, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;
    len--;

    switch (apdu_protocol_id) {
    case 1:
        /* BSSLAP
         * the embedded message is as defined in 3GPP TS 08.71(3GPP TS 48.071 version 7.2.0 Release 7)
         */
        APDU_tvb = tvb_new_subset_length(tvb, curr_offset, len);
        if (gsm_bsslap_handle)
            call_dissector(gsm_bsslap_handle, APDU_tvb, pinfo, g_tree);
        break;
    case 2:
        /* LLP
         * The embedded message contains a Facility Information Element as defined in 3GPP TS 04.71
         * excluding the Facility IEI and length of Facility IEI octets defined in 3GPP TS 04.71.(3GPP TS 44.071).
         */
        break;
    case 3:
        /* SMLCPP
         * The embedded message is as defined in 3GPP TS 08.31(TS 48.031).
         */
        break;
    default:
        break;
    }

    curr_offset += len;

    EXTRANEOUS_DATA_CHECK(len, curr_offset - offset, pinfo, &ei_gsm_a_bssmap_extraneous_data);

    return(curr_offset - offset);
}
/*
 * 3.2.2.69 Network Element Identity
 * Network Element Identity element of 3GPP TS 49.031 BSSAP-LE.
 */
/*
 * 3.2.2.70 GPS Assistance Data
 * Requested GPS Data element of 3GPP TS 49.031 BSSAP-LE.
 */
static guint16
be_gps_assist_data(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    proto_tree_add_expert(tree, pinfo, &ei_gsm_a_bssmap_not_decoded_yet, tvb, curr_offset, len);


    return(len);
}
/*
 * 3.2.2.71 Deciphering Keys
 * Deciphering Key element of 3GPP TS 49.031 BSSAP-LE.
 * Dissected in packet-gsm_bssmap_le.c
 */

 /* 3.2.2.72 Return Error Request
  * Return Error Request element of 3GPP TS 49.031 BSSAP-LE.
  */
static guint16
be_ret_err_req(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    proto_tree_add_expert(tree, pinfo, &ei_gsm_a_bssmap_not_decoded_yet, tvb, curr_offset, len);

    return(len);
}
/*
 * 3.2.2.73 Return Error Cause
 * Return Error Cause element of 3GPP TS 49.031 BSSAP-LE.
 */
static guint16
be_ret_err_cause(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    proto_tree_add_expert(tree, pinfo, &ei_gsm_a_bssmap_not_decoded_yet, tvb, curr_offset, len);

    return(len);
}
/*
 * 3.2.2.74 Segmentation
 * Segmentation element of 3GPP TS 49.031 BSSAP-LE.
 */
static guint16
be_seg(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    proto_tree_add_expert(tree, pinfo, &ei_gsm_a_bssmap_not_decoded_yet, tvb, curr_offset, len);

    return(len);
}
/*
 * 3.2.2.75 Service Handover
 */
#if 0
static const value_string gsm_a_bssmap_serv_ho_inf_vals[] = {
    { 0,    "Handover to UTRAN or cdma2000 should be performed - Handover to UTRAN or cdma2000 is preferred" },
    { 1,    "Handover to UTRAN or cdma2000 should not be performed - Handover to GSM is preferred" },
    { 2,    "Handover to UTRAN or cdma2000 shall not be performed - " },
    { 3,    "no information available for service based handover" },
    { 4,    "no information available for service based handover" },
    { 5,    "no information available for service based handover" },
    { 6,    "no information available for service based handover" },
    { 7,    "no information available for service based handover" },
    { 0, NULL }
};
#endif
static guint16
be_serv_ho(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    /* Service Handover information */
    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, curr_offset<<3, 5, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_serv_ho_inf, tvb, curr_offset+1, 1, ENC_BIG_ENDIAN);
    curr_offset++;
    return(len);
}

/*
 * 3.2.2.76 Source RNC to target RNC transparent information (UMTS)
 */

static guint16
be_src_rnc_to_tar_rnc_umts(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    tvbuff_t    *container_tvb;
    guint32 curr_offset;

    curr_offset = offset;

    /* The Source RNC to Target RNC transparent Information value is encoded as
     * the Source RNC to Target RNC Transparent Container IE as defined in relevant
     * RANAP specification 3GPP TS 25.413, excluding RANAP tag
     */
    container_tvb = tvb_new_subset_length(tvb, curr_offset, len);
    dissect_ranap_SourceRNC_ToTargetRNC_TransparentContainer_PDU(container_tvb, pinfo, tree, NULL);

    return(len);
}
/*
 * 3.2.2.77 Source RNC to target RNC transparent information (cdma2000)
 */
static guint16
be_src_rnc_to_tar_rnc_cdma(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    proto_tree_add_expert(tree, pinfo, &ei_gsm_a_bssmap_not_decoded_yet, tvb, curr_offset, len);
    /* The Source RNC to Target RNC transparent Information value (structure and encoding)
     * for cdma2000 is defined in relevant specifications.
     */

    return(len);
}
/*
 * 3.2.2.78 GERAN Classmark
 */
static const value_string gsm_a_max_nb_traffic_chan_vals[] = {
    { 0x00, "1 TCH" },
    { 0x01, "2 TCHs" },
    { 0x02, "3 TCHs" },
    { 0x03, "4 TCHs" },
    { 0x04, "5 TCHs" },
    { 0x05, "6 TCHs" },
    { 0,    NULL }
};
static const true_false_string gsm_a_bssmap_accept_not_accept_vals = {
    "Acceptable",
    "Not acceptable"
};
static guint16
be_geran_cls_m(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    if (len > 2) {
        de_sup_codec_list(tvb, tree, pinfo, curr_offset, (len-2), NULL, 0);
    }
    curr_offset += len-2;
    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, curr_offset<<3, 5, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_max_nb_traffic_chan, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;
    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, curr_offset<<3, 3, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_acceptable_chan_coding_bit5, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_acceptable_chan_coding_bit4, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_acceptable_chan_coding_bit3, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_acceptable_chan_coding_bit2, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_acceptable_chan_coding_bit1, tvb, curr_offset, 1, ENC_BIG_ENDIAN);

    return(len);
}
/*
 * 3.2.2.79 GERAN BSC Container
 */
static guint16
be_geran_bsc_cont(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    proto_tree_add_item(tree, hf_gsm_a_bssap_speech_codec, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;
    NO_MORE_DATA_CHECK(len);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_allowed_data_rate_bit8, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_allowed_data_rate_bit7, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_allowed_data_rate_bit6, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_allowed_data_rate_bit5, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_allowed_data_rate_bit4, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_max_nb_traffic_chan, tvb, curr_offset, 1, ENC_BIG_ENDIAN);

    return(len);
}
/*
 * 3.2.2.80 New BSS to Old BSS Information
 */
/* This function is only called from other protocols (e.g. RANAP),
   internally, the Field Element dissector is called directly */
void
bssmap_new_bss_to_old_bss_info(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo)
{
    guint16 len;
    if (!tree) {
        return;
    }

    g_tree = tree;

    len = tvb_reported_length(tvb);
    be_field_element_dissect(tvb, tree, pinfo, 0, len, NULL, 0);

    g_tree = NULL;
}


/*
 * 3.2.2.81 Inter-System Information
 */
static guint16
be_inter_sys_inf(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    tvbuff_t *new_tvb;

    new_tvb = tvb_new_subset_length(tvb, offset, len);

    if (new_tvb) {
        dissect_ranap_InterSystemInformation_TransparentContainer_PDU(new_tvb, pinfo, tree, NULL);
    }

    return(len);
}
/*
 * 3.2.2.82 SNA Access Information
 */
static guint16
be_sna_acc_inf(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    proto_tree_add_expert(tree, pinfo, &ei_gsm_a_bssmap_not_decoded_yet, tvb, curr_offset, len);

    return(len);
}

/*
 * 3.2.2.83 VSTK_RAND Information
 */
static guint16
be_vstk_rand_info(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;
    guint64 vstk_rand;

    curr_offset = offset;

    vstk_rand = tvb_get_ntoh40(tvb, curr_offset);
    vstk_rand >>= 4;
    proto_tree_add_uint64(tree, hf_gsm_a_bssmap_vstk_rand, tvb, curr_offset, 5, vstk_rand);
    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, (((curr_offset+4)<<3)+4), 4, ENC_BIG_ENDIAN);

    return(len);
}
/*
 * 3.2.2.84 VSTK information
 */
static guint16
be_vstk_info(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    proto_tree_add_item(tree, hf_gsm_a_bssmap_vstk, tvb, offset, 16, ENC_NA);

    return(len);
}
/*
 * 3.2.2.85 Paging Information
 */
/*
* If the VGCS/VBS flag is set to zero, the mobile station to be paged is not a member of any VGCS/VBS-group.
* If the VGCS/VBS flag is set to one, the mobile station to be paged is a member of a VGCS/VBS-group.
*/
static const true_false_string bssmap_paging_inf_flg_value = {
   "A member of a VGCS/VBS-group",
   "Not a member of any VGCS/VBS-group"
};

static const value_string gsm_a_bssmap_paging_cause_vals[] = {
    { 0,    "Paging is for mobile terminating call" },
    { 1,    "Paging is for a short message" },
    { 2,    "Paging is for a USSD" },
    { 3,    "Spare" },
    { 0, NULL }
};

static guint16
be_paging_inf(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, curr_offset<<3, 5, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_paging_cause, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_paging_inf_flg, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;

    return(curr_offset-offset);
}
/*
 * 3.2.2.86 IMEI
 * Use same dissector as IMSI 3.2.2.6
 */

/*
 * 3.2.2.87 Velocity Estimate
 */
static guint16
be_vel_est(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len, gchar *add_string, int string_len)
{
    dissect_description_of_velocity(tvb, tree, pinfo, offset, len, add_string, string_len);

    return(len);
}
/*
 * 3.2.2.88 VGCS Feature Flags
 */

/* Bit 1 is the talker priority indicator (TP Ind). */
static const true_false_string gsm_bssmap_tpind_vals = {
    "Talker Priority not supported" ,
    "Talker Priority supported"
};
/* Bits 2 and 3 are the A-interface resource sharing indicator (AS Ind). */
static const true_false_string gsm_bssmap_asind_b2_vals = {
    "A-interface circuit sharing" ,
    "No A-interface circuit sharing"
};

static const true_false_string gsm_bssmap_asind_b3_vals = {
    "A-interface link sharing" ,
    "No A-interface link sharing"
};

/* Bit 4 is the group or broadcast call re-establishment by the BSS indicator (Bss Res). */
static const true_false_string gsm_bssmap_bss_res_vals = {
    "Re-establishment of the group or broadcast call by the BSS" ,
    "No re-establishment of the group or broadcast call by the BSS"
};

/* Bit 5 is the Talker Channel Parameter (TCP). */
static const true_false_string gsm_bssmap_bss_tcp_vals = {
    "Talker channel parameter is applicable to this call, talker shall be established and maintained on a dedicated channel" ,
    "Talker channel parameter is not applicable to this call"
};

static guint16
be_vgcs_feat_flg(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, curr_offset<<3, 3, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_tcp, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_bss_res, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_asind_b3, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_asind_b2, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_tpind, tvb, curr_offset, 1, ENC_BIG_ENDIAN);

    curr_offset++;

    return(curr_offset-offset);
}
/*
 * 3.2.2.89 Talker Priority
 */
static const value_string gsm_a_bssmap_talker_pri_vals[] = {
    { 0,    "Normal Priority" },
    { 1,    "Privileged Priority" },
    { 2,    "Emergency Priority" },
    { 3,    "Reserved for future use" },
    { 0, NULL }
};

static guint16
be_talker_pri(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    proto_tree_add_item(tree, hf_gsm_a_bssmap_talker_pri, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;

    EXTRANEOUS_DATA_CHECK(len, curr_offset - offset, pinfo, &ei_gsm_a_bssmap_extraneous_data);

    return(curr_offset - offset);
}

/*
 * 3.2.2.90 Emergency Set Indication
 * No data
 */
/*
 * 3.2.2.91 Talker Identity
 */
static guint16
be_talker_id(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, curr_offset<<3, 5, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_filler_bits, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;
    proto_tree_add_item(tree, hf_gsm_a_bssmap_talker_identity_field, tvb, curr_offset, len-1, ENC_NA);

    return(len);
}
/*
 * 3.2.2.92 SMS to VGCS
 */
static guint16
be_sms_to_vgcs(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_)
{
    /* The SMS content field is coded as follows -  this field contains
     * the RP-DATA message as defined in 3GPP TS 24.011.
     */
    rp_data_n_ms(tvb, tree, pinfo, offset, len);


    return(len);
}
/*
 * 3.2.2.93 VGCS talker mode
 */
static const value_string gsm_a_bssmap_rr_mode_vals[] = {
    { 0, "dedicated mode (i.e. dedicated channel)"},
    { 1, "group transmit mode (i.e. voice group channel)"},
    { 0, NULL }
};
static const value_string gsm_a_bssmap_group_cipher_key_nb_vals[] = {
    { 0x0, "no ciphering"},
    { 0x1, "cipher key number 1"},
    { 0x2, "cipher key number 2"},
    { 0x3, "cipher key number 3"},
    { 0x4, "cipher key number 4"},
    { 0x5, "cipher key number 5"},
    { 0x6, "cipher key number 6"},
    { 0x7, "cipher key number 7"},
    { 0x8, "cipher key number 8"},
    { 0x9, "cipher key number 9"},
    { 0xa, "cipher key number A"},
    { 0xb, "cipher key number B"},
    { 0xc, "cipher key number C"},
    { 0xd, "cipher key number D"},
    { 0xe, "cipher key number E"},
    { 0xf, "cipher key number F"},
    { 0,    NULL }
};
static guint16
be_vgcs_talker_mode(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    proto_tree_add_item(tree, hf_gsm_a_bssmap_rr_mode, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_group_cipher_key_nb, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, ((curr_offset<<3)+6), 2, ENC_BIG_ENDIAN);

    return(len);
}
/*
 * 3.2.2.94 VGCS/VBS Cell Status
 */
static const value_string gsm_a_bssmap_vgcs_vbs_cell_status_vals[] = {
    { 0, "Cell is established for the voice group or broadcast call"},
    { 1, "Cell is not established for the voice group or broadcast call. Establishment by the BSS is to be attempted"},
    { 2, "Cell is released for the voice group or broadcast call because no user is present"},
    { 3, "Cell is not established for the voice group or broadcast call. No establishment by the BSS is to be attempted"},
    { 4, "Reserved"},
    { 5, "Reserved"},
    { 6, "Reserved"},
    { 7, "Reserved"},
    { 0, NULL }
};
static guint16
be_vgcs_vbs_cell_status(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, (curr_offset<<3), 5, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_vgcs_vbs_cell_status, tvb, curr_offset, 1, ENC_BIG_ENDIAN);

    return(len);
}
/*
 * 3.2.2.95 GANSS Assistance Data
 * The GANSS Assistance Data octets 3 to n are coded as the Requested GANSS Data element of 3GPP TS 49.031 (BSSAP-LE)
 * XXX move to packet-gsm_bssmap_le.c
 */
guint16
be_ganss_ass_dta(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    proto_tree_add_expert(tree, pinfo, &ei_gsm_a_bssmap_not_decoded_yet, tvb, curr_offset, len);

    return(len);
}
/*
 * 3.2.2.96 GANSS Positioning Data
 * XXX move to packet-gsm_bssmap_le.c
 */

static const value_string gsm_a_bssmap_method_vals[] = {
    { 0x00,     "MS-Based" },
    { 0x01,     "MS-Assisted" },
    { 0x02,     "Conventional" },
    { 0x03,     "Reserved" },
    { 0,        NULL }
};

static const value_string gsm_a_bssmap_ganss_id_vals[] = {
    { 0x00,     "Galileo" },
    { 0x01,     "Satellite Based Augmentation Systems (SBAS)" },
    { 0x02,     "Modernized GPS" },
    { 0x03,     "Quasi Zenith Satellite System (QZSS)" },
    { 0x04,     "GLONASS" },
    { 0,        NULL }
};

static const value_string gsm_a_bssmap_usage_vals[] = {
    { 0x00,     "Attempted unsuccessfully due to failure or interruption" },
    { 0x01,     "Attempted successfully: results not used to generate location" },
    { 0x02,     "Attempted successfully: results used to verify but not generate location" },
    { 0x03,     "Attempted successfully: results used to generate location" },
    { 0x04,     "Attempted successfully: case where MS supports multiple mobile based positioning methods and the actual method or methods used by the MS cannot be determined" },
    { 0,        NULL }
};

guint16
be_ganss_pos_dta(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    proto_tree_add_item(tree, hf_gsm_a_bssmap_method, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_ganss_id, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_usage, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;

    return(curr_offset-offset);
}
/*
 * 3.2.2.97 GANSS Location Type
 */
guint16
be_ganss_loc_type(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    proto_tree_add_expert(tree, pinfo, &ei_gsm_a_bssmap_not_decoded_yet, tvb, curr_offset, len);

    return(len);
}
/*
 * 3.2.2.98 Application Data
 */
static guint16
be_app_data(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    proto_tree_add_expert(tree, pinfo, &ei_gsm_a_bssmap_not_decoded_yet, tvb, curr_offset, len);

    return(len);
}
/*
 * 3.2.2.99 Data Identity
 */
static guint16
be_app_data_id(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_)
{
    proto_tree_add_item(tree, hf_gsm_a_bssmap_data_id, tvb, offset, 1, ENC_BIG_ENDIAN);

    return(len);
}
/*
 * 3.2.2.100    Application Data Information
 */
static const true_false_string gsm_a_bssmap_bt_ind_val = {
    "BSS has already transmitted the application data to cells",
    "BSS has not transmitted the application data to cells"
};
static guint16
be_app_data_inf(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, (curr_offset<<3), 7, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_bt_ind, tvb, curr_offset, 1, ENC_BIG_ENDIAN);

    return(len);
}
/*
 * 3.2.2.101    MSISDN
 * Octets 3-12 contain the digits of an MSISDN, coded as in 3GPP TS 24.008, Calling party BCD number, octets 4 - 13.
 */
 static guint16
 be_msisdn(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_)
 {
    tvbuff_t    *new_tvb;

    new_tvb = tvb_new_subset_length(tvb, offset, len);
    if (new_tvb) {
        dissect_gsm_map_msisdn(new_tvb, pinfo , tree);
    }

    return len;
}
 /*
  * 3.2.2.102   AoIP Transport Layer Address
  */
static guint16
be_aoip_trans_lay_add(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;
    guint8  addr_type;
    guint32 rtp_ipv4_address;
    guint16 rtp_port;
    address rtp_dst_addr;
    struct e_in6_addr rtp_addr_ipv6;

    curr_offset = offset;

    /* This Information Element provides either an IPv4 or and IPv6 Address and UDP port value
     * for the Transport Layer information of the connection end point.
     * The Length differentiates between IPv4 and IPv6.
     */
    switch (len) {
        case 6:
            /* IPv4 */
            addr_type = 1;
            proto_tree_add_item(tree, hf_gsm_a_bssmap_aoip_trans_ipv4, tvb, curr_offset, 4, ENC_BIG_ENDIAN);
            rtp_ipv4_address = tvb_get_ipv4(tvb, curr_offset);
            curr_offset+=4;
            break;
        case 18:
            /* IPv6 */
            addr_type = 2;
            proto_tree_add_item(tree, hf_gsm_a_bssmap_aoip_trans_ipv6, tvb, curr_offset, 16, ENC_NA);
            tvb_get_ipv6(tvb, offset + 5, &rtp_addr_ipv6);
            curr_offset+=16;
            break;
        default:
            /* Bogus */
            proto_tree_add_expert_format(tree, pinfo, &ei_gsm_a_bssmap_bogus_length, tvb, curr_offset, len, "Bogus length %u",len);
            return(len);
    }
    proto_tree_add_item(tree, hf_gsm_a_bssmap_aoip_trans_port, tvb, curr_offset, 2, ENC_BIG_ENDIAN);
    rtp_port = tvb_get_ntohs(tvb,curr_offset);
    curr_offset+=2;

    switch (addr_type) {
    case 1:
        /* IPv4 */
        rtp_dst_addr.type = AT_IPv4;
        rtp_dst_addr.len = 4;
        rtp_dst_addr.data = (guint8 *)&rtp_ipv4_address;
        break;
    case 2:
        /* IPv6 */
        rtp_dst_addr.type = AT_IPv6;
        rtp_dst_addr.len = 16;
        rtp_dst_addr.data = (guint8 *)&rtp_addr_ipv6;
        break;
    }

    if ((!pinfo->fd->flags.visited) && rtp_port != 0) {
        rtp_add_address(pinfo, &rtp_dst_addr, rtp_port, 0, "BSS MAP", pinfo->num, FALSE, 0);
        rtcp_add_address(pinfo, &rtp_dst_addr, rtp_port+1, 0, "BSS MAP", pinfo->num);
    }
    return(curr_offset - offset);
}
/*
 * 3.2.2.103    Speech Codec List
 */
/*
FR_AMR is coded '011'
S11, S13 and S15 are reserved and coded with zeroes.
HR_AMR is coded '100'
S6 - S7 and S11 - S15 are reserved and coded with zeroes.
OHR_AMR is coded '011'
S11, S13 and S15 are reserved and coded with zeroes.

FR_AMR-WB is coded '001'
S0 is set to '1' S1 - S7 are reserved and coded with zeroes.
OFR_AMR-WB is coded '100'
S0, S2, S4 indicates the supported Codec Configurations. S1, S3, S5, S6, S7 are reserved and coded with zeroes.
OHR_AMR-WB is coded '101'
S0 is set to '1' S1 - S7 are reserved and coded with zeroes.


8   7   6   5   4   3   2   1
FI  PI  PT  TF  Codec Type     (FR_AMR-WB or OFR_AMR-WB or OHR_AMR-WB)
S7  S6  S5  S4  S3  S2  S1  S0


*/
static const true_false_string bssmap_fi_vals = {
   "AoIP with compressed speech via RTP/UDP/IP is supported by the BSS/Preferred by the MSC",
   "AoIP with Compressed speech via RTP/UDP/IP is not supported by the BSS/Not Preferred by the MSC"
};
static const true_false_string bssmap_tf_vals = {
    "TFO supported by the BSS or TFO support is preferred by the MSC for this Codec Type",
    "TFO is not supported by the BSS or TFO support is not preferred by the MSC for this Codec Type"
};
static const true_false_string bssmap_pi_vals = {
    "Transport of PCM over A-Interface via RTP/UDP/IP is supported by the BSS or preferred by the MSC for this Codec Type",
    "PCM over A interface with IP as transport is not supported by the BSS or not preferred by the MSC for this Codec Type"
};
static const true_false_string bssmap_pt_vals = {
    "Transport of PCM over A-Interface via TDM is supported by the BSS or preferred by the MSC",
    "PCM over A-Interface with TDM as transport is not supported by the BSS or not preferred by the MSC for this Codec Type"
};
/* 26.103 Table 6.3-1: Coding of the selected Codec_Type (long form) */
static const value_string bssap_speech_codec_values[] = {
    { 0x00,     "GSM FR" },
    { 0x01,     "GSM HR" },
    { 0x02,     "GSM EFR" },
    { 0x03,     "FR_AMR" },
    { 0x04,     "HR_AMR" },
    { 0x05,     "UMTS AMR" },
    { 0x06,     "UMTS AMR 2" },
    { 0x07,     "TDMA EFR" },
    { 0x08,     "PDC EFR" },
    { 0x09,     "FR_AMR-WB" },
    { 0x0a,     "UMTS AMR-WB" },
    { 0x0b,     "OHR_AMR" },
    { 0x0c,     "OFR_AMR-WB" },
    { 0x0d,     "OHR_AMR-WB" },
    { 0x0e,     "Reserved" },
    { 0x0f,     "Codec Extension" },
    { 0xfd,     "CSData" },
    { 0xfe,     "MuMe2" },
    { 0xff,     "MuMe" },
    { 0,        NULL }
};
static value_string_ext bssap_speech_codec_values_ext = VALUE_STRING_EXT_INIT(bssap_speech_codec_values);

static const value_string bssap_extended_codec_values[] = {
    { 0xfd,     "CSData" },
    { 0,        NULL }
};

static guint16
be_speech_codec_lst(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset, consumed = 0;
    guint8 codec;
    guint8 number = 0;
    proto_item  *item = NULL;
    proto_tree  *subtree = NULL;

    curr_offset = offset;

    while (curr_offset-offset < len) {
        number++;
        consumed = 0;
        subtree = proto_tree_add_subtree_format(tree, tvb, curr_offset, 1,
                    ett_codec_lst, &item, "Speech Codec Element %u",number);
        codec = tvb_get_guint8(tvb,curr_offset)&0x0f;
        switch (codec) {
            case 0:
                /* GSM_FR is coded "0000" */
                /* fall through */
            case 1:
                /* GSM_HR is coded "0001" */
                /* fall through */
            case 2:
                /* GSM_EFR is coded "0010" */
                /* fall through */
                /* FI indicates Full IP */
                proto_tree_add_item(subtree, hf_gsm_a_bssmap_fi, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                /* PI indicates PCMoIP */
                proto_tree_add_item(subtree, hf_gsm_a_bssmap_pi, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                /* PT indicates PCMoTDM */
                proto_tree_add_item(subtree, hf_gsm_a_bssmap_pt, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                /* TF indicates TFO support */
                proto_tree_add_item(subtree, hf_gsm_a_bssmap_tf, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                /* Codec Type */
                proto_tree_add_item(subtree, hf_gsm_a_bssap_speech_codec, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                proto_item_append_text(item, " - %s",
                                       val_to_str_const(tvb_get_guint8(tvb, curr_offset) & 0x0f,
                                                        bssap_speech_codec_values,
                                                        "Unknown"));
                curr_offset++;
                consumed++;
                break;
            case 3:
                /* fall through */
            case 4:
                /* fall through */
            case 0xb:
                /* FR_AMR is coded '011'
                 * HR_AMR is coded '100'
                 * OHR_AMR is coded '1011'
                 */
                /* FI indicates Full IP */
                proto_tree_add_item(subtree, hf_gsm_a_bssmap_fi, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                /* PI indicates PCMoIP */
                proto_tree_add_item(subtree, hf_gsm_a_bssmap_pi, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                /* PT indicates PCMoTDM */
                proto_tree_add_item(subtree, hf_gsm_a_bssmap_pt, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                /* TF indicates TFO support */
                proto_tree_add_item(subtree, hf_gsm_a_bssmap_tf, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                /* Codec Type */
                proto_tree_add_item(subtree, hf_gsm_a_bssap_speech_codec, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                proto_item_append_text(item, " - %s",
                                       val_to_str_const(tvb_get_guint8(tvb, curr_offset) & 0x0f,
                                                        bssap_speech_codec_values,
                                                        "Unknown"));
                curr_offset++;
                consumed++;
                proto_tree_add_item(subtree, hf_gsm_a_bssmap_s0_s15, tvb, curr_offset, 2, ENC_BIG_ENDIAN);
                curr_offset+=2;
                consumed+=2;
                break;
            case 0x9:
                /* fall through */
            case 0xc:
                /* fall through */
            case 0xd:
                /* FR_AMR-WB is coded '1001'
                 * OFR_AMR-WB is coded '1100'
                 * OHR_AMR-WB is coded '1101'
                 */
                /* FI indicates Full IP */
                proto_tree_add_item(subtree, hf_gsm_a_bssmap_fi, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                /* PI indicates PCMoIP */
                proto_tree_add_item(subtree, hf_gsm_a_bssmap_pi, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                /* PT indicates PCMoTDM */
                proto_tree_add_item(subtree, hf_gsm_a_bssmap_pt, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                /* TF indicates TFO support */
                proto_tree_add_item(subtree, hf_gsm_a_bssmap_tf, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                /* Codec Type */
                proto_tree_add_item(subtree, hf_gsm_a_bssap_speech_codec, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                proto_item_append_text(item, " - %s",
                                       val_to_str_const(tvb_get_guint8(tvb, curr_offset) & 0x0f,
                                                        bssap_speech_codec_values,
                                                        "Unknown"));
                curr_offset++;
                consumed++;
                proto_tree_add_item(subtree, hf_gsm_a_bssmap_s0_s7, tvb, curr_offset, 1, ENC_NA);
                curr_offset++;
                consumed++;
                break;
            case 0xf:
                /* Currently (3GPP TS 48.008 version 9.4.0 Release 9) CSData Codec Type is the only extended one */
                /* PI indicates PCMoIP */
                proto_tree_add_item(subtree, hf_gsm_a_bssmap_pi, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                /* PT indicates PCMoTDM */
                proto_tree_add_item(subtree, hf_gsm_a_bssmap_pt, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                /* Codec Type */
                proto_tree_add_item(subtree, hf_gsm_a_bssap_speech_codec, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                curr_offset++;
                consumed++;
                /* Codec Extension */
                proto_tree_add_item(subtree, hf_gsm_a_bssap_extended_codec, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                proto_item_append_text(item, " - %s",
                                       val_to_str_const(tvb_get_guint8(tvb, curr_offset),
                                                        bssap_extended_codec_values,
                                                        "Unknown"));
                curr_offset++;
                consumed++;
                proto_tree_add_item(subtree, hf_gsm_a_bssap_extended_codec_r2, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                proto_tree_add_item(subtree, hf_gsm_a_bssap_extended_codec_r3, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                curr_offset++;
                consumed++;
                break;
            default:
                proto_tree_add_expert(subtree, pinfo, &ei_gsm_a_bssap_unknown_codec, tvb, curr_offset, 2);
                curr_offset+=2;
                consumed+=2;
                break;
        }
    }
    proto_item_set_len(item, consumed);
    return(len);
}
/*
 * 3.2.2.104    Speech Codec
 */
static const true_false_string bssmap_fi2_vals = {
   "AoIP with compressed speech via RTP/UDP/IP is selected for this Codec Type",
   "Compressed speech via RTP/UDP/IP is not selected for this Codec Type"
};
static const true_false_string bssmap_tf2_vals = {
    "TFO Support is selected for this Codec Type",
    "TFO Support is not selected for this Codec Type"
};
static const true_false_string bssmap_pi2_vals = {
    "PCM over A-Interface via RTP/UPD/IP is selected for this Codec Type",
    "PCM over A interface with RTP/UDP/IP is not selected for this Codec Type"
};
static const true_false_string bssmap_pt2_vals = {
    "PCM over A-Interface with TDM as transport is selected for this Codec Type",
    "PCM over A-Interface with TDM as transport is not selected for this Codec Type"
};
static guint16
be_speech_codec(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset, consumed = 0;
    guint8 codec;
    guint8 number = 0;
    proto_item  *item = NULL;
    proto_tree  *subtree = NULL;

    curr_offset = offset;

    while (curr_offset-offset < len) {
        number++;
        consumed = 0;
        subtree = proto_tree_add_subtree_format(tree, tvb, curr_offset, 1, ett_codec_lst, &item,
                        "Speech Codec Element %u",number);
        codec = tvb_get_guint8(tvb,curr_offset)&0x0f;
        switch (codec) {
            case 0:
                /* GSM_FR is coded "0000" */
                /* fall through */
            case 1:
                /* GSM_HR is coded "0001" */
                /* fall through */
            case 2:
                /* GSM_EFR is coded "0010" */
                /* fall through */
                /* FI indicates Full IP */
                proto_tree_add_item(subtree, hf_gsm_a_bssmap_fi, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                /* PI indicates PCMoIP */
                proto_tree_add_item(subtree, hf_gsm_a_bssmap_pi, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                /* PT indicates PCMoTDM */
                proto_tree_add_item(subtree, hf_gsm_a_bssmap_pt, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                /* TF indicates TFO support */
                proto_tree_add_item(subtree, hf_gsm_a_bssmap_tf, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                /* Codec Type */
                proto_tree_add_item(subtree, hf_gsm_a_bssap_speech_codec, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                proto_item_append_text(item, " - %s",
                                       val_to_str_const(tvb_get_guint8(tvb, curr_offset) & 0x0f,
                                                        bssap_speech_codec_values,
                                                        "Unknown"));
                curr_offset++;
                consumed++;
                break;
            case 3:
                /* fall through */
            case 4:
                /* fall through */
            case 0xb:
                /* FR_AMR is coded '011'
                 * HR_AMR is coded '100'
                 * OHR_AMR is coded '1011'
                 */
                /* FI indicates Full IP */
                proto_tree_add_item(subtree, hf_gsm_a_bssmap_fi2, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                /* PI indicates PCMoIP */
                proto_tree_add_item(subtree, hf_gsm_a_bssmap_pi2, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                /* PT indicates PCMoTDM */
                proto_tree_add_item(subtree, hf_gsm_a_bssmap_pt2, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                /* TF indicates TFO support */
                proto_tree_add_item(subtree, hf_gsm_a_bssmap_tf2, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                /* Codec Type */
                proto_tree_add_item(subtree, hf_gsm_a_bssap_speech_codec, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                proto_item_append_text(item, " - %s",
                                       val_to_str_const(tvb_get_guint8(tvb, curr_offset) & 0x0f,
                                                        bssap_speech_codec_values,
                                                        "Unknown"));
                curr_offset++;
                consumed++;
                proto_tree_add_item(subtree, hf_gsm_a_bssmap_s0_s15, tvb, curr_offset, 2, ENC_BIG_ENDIAN);
                curr_offset+=2;
                consumed+=2;
                break;
            case 0x9:
                /* fall through */
            case 0xc:
                /* fall through */
            case 0xd:
                /* FR_AMR-WB is coded '1001'
                 * OFR_AMR-WB is coded '1100'
                 * OHR_AMR-WB is coded '1101'
                 */
                /* FI indicates Full IP */
                proto_tree_add_item(subtree, hf_gsm_a_bssmap_fi2, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                /* PI indicates PCMoIP */
                proto_tree_add_item(subtree, hf_gsm_a_bssmap_pi2, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                /* PT indicates PCMoTDM */
                proto_tree_add_item(subtree, hf_gsm_a_bssmap_pt2, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                /* TF indicates TFO support */
                proto_tree_add_item(subtree, hf_gsm_a_bssmap_tf2, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                /* Codec Type */
                proto_tree_add_item(subtree, hf_gsm_a_bssap_speech_codec, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                proto_item_append_text(item, " - %s",
                                       val_to_str_const(tvb_get_guint8(tvb, curr_offset) & 0x0f,
                                                        bssap_speech_codec_values,
                                                        "Unknown"));
                curr_offset++;
                consumed++;
                proto_tree_add_item(subtree, hf_gsm_a_bssmap_s0_s7, tvb, curr_offset, 1, ENC_NA);
                curr_offset++;
                consumed++;
                break;
            case 0xf:
                /* Currently (3GPP TS 48.008 version 9.4.0 Release 9) CSData Codec Type is the only extended one */
                /* PI indicates PCMoIP */
                proto_tree_add_item(subtree, hf_gsm_a_bssmap_pi, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                /* PT indicates PCMoTDM */
                proto_tree_add_item(subtree, hf_gsm_a_bssmap_pt, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                /* Codec Type */
                proto_tree_add_item(subtree, hf_gsm_a_bssap_speech_codec, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                curr_offset++;
                consumed++;
                /* Codec Extension */
                proto_tree_add_item(subtree, hf_gsm_a_bssap_extended_codec, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                curr_offset++;
                consumed++;
                proto_tree_add_item(subtree, hf_gsm_a_bssap_extended_codec_r2, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                proto_tree_add_item(subtree, hf_gsm_a_bssap_extended_codec_r3, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                curr_offset++;
                consumed++;
                break;
            default:
                proto_tree_add_expert(subtree, pinfo, &ei_gsm_a_bssap_unknown_codec, tvb, curr_offset, 2);
                curr_offset+=2;
                consumed+=2;
                break;
        }
    }
    proto_item_set_len(item, consumed);
    return(len);
}
/*
 * 3.2.2.105    Call Identifier
 */
static guint16
be_call_id(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;

    /* Call Identifier  (least significant bits)    octet 2
     * Call Identifier  octet 3
     * Call Identifier  octet 4
     * Call Identifier (most significant bits)  octet 5
     */
    proto_tree_add_item(tree, hf_gsm_a_bssmap_call_id, tvb, curr_offset, 4, ENC_LITTLE_ENDIAN);
    curr_offset+=4;

    return(curr_offset - offset);
}
/*
 * 3.2.2.106    Call Identifier List
 */
static guint16
be_call_id_lst(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;
    curr_offset = offset;

    if (len==0) {
        proto_tree_add_item(tree, hf_gsm_a_bssmap_all_call_identifiers_resources_released, tvb, curr_offset, len, ENC_NA);
    }
    while (curr_offset-offset < len) {
        proto_tree_add_item(tree, hf_gsm_a_bssmap_call_id, tvb, curr_offset, 4, ENC_LITTLE_ENDIAN);
        curr_offset+=4;
    }

    return(len);
}
/*
 * 3.2.2.107    A-Interface Selector for RESET
 */
static const true_false_string gsm_a_bssmap_rip_value = {
    "all calls associated to IP links shall be cleared",
    "calls associated to IP links shall not be cleared"
};
static const true_false_string gsm_a_bssmap_rtd_value = {
    "all calls associated to TDM links shall be cleared",
    "calls associated to TDM links shall not be cleared"
};
static guint16
be_a_itf_sel_for_reset(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;
    curr_offset = offset;

    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, (curr_offset<<3), 6, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_rip, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_gsm_a_bssmap_rtd, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;

    return(curr_offset - offset);
}
/*
 * 3.2.2.109    Kc128
 */
static guint16
be_kc128(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;
    curr_offset = offset;

    proto_tree_add_item(tree, hf_gsm_a_bssmap_kc128, tvb, curr_offset, 16, ENC_NA);
    curr_offset+=16;

    return(curr_offset - offset);
}
/*
 * 3.2.2.110    CSG Identifier
 */
static const crumb_spec_t gsm_a_bssmap_csg_id_crumbs[] = {
    { 0, 24},
    { 29, 3},
    { 0, 0}
};
static const true_false_string gsm_a_bssmap_cell_access_mode_value = {
    "Hybrid cell",
    "CSG cell"
};
static guint16
be_csg_id(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_)
{
    guint32 bit_offset;
    bit_offset = offset<<3;

    proto_tree_add_split_bits_item_ret_val(tree, hf_gsm_a_bssmap_csg_id, tvb, bit_offset, gsm_a_bssmap_csg_id_crumbs, NULL);
    bit_offset += 32;
    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_spare_bits, tvb, bit_offset, 7, ENC_BIG_ENDIAN);
    bit_offset += 7;
    proto_tree_add_bits_item(tree, hf_gsm_a_bssmap_cell_access_mode, tvb, bit_offset, 1, ENC_BIG_ENDIAN);

    return(len);
}

/*
 * 3.2.2.111 Redirect Attempt Flag
 * No data
 */

/*
 * 3.2.2.112 Reroute Reject Cause
 */
/*
 * Reroute Reject cause value (octet 2)
 */


static const value_string gsm_a_bssap_reroute_rej_cause_vals[] = {
    { 0x0,     "Reserved" },
    { 0x1,     "Reserved" },
    { 0x2,     "Reserved" },
    { 0x3,     "Reserved" },
    { 0x4,     "Reserved" },
    { 0x5,     "Reserved" },
    { 0x6,     "Reserved" },
    { 0x7,     "Reserved" },
    { 0x8,     "Reserved" },
    { 0x9,     "Reserved" },
    { 0xa,     "Reserved" },
    { 0xb,     "PLMN not allowed" },
    { 0xc,     "Location area not allowed" },
    { 0xd,     "Roaming not allowed in this location area" },
    { 0xe,     "GPRS services not allowed in this PLMN" },
    { 0xf,     "No suitable cell in location area" },
    { 0x10,    "CS/PS domain registration coordination required" },
    { 0,        NULL }
};

static guint16
be_reroute_rej_cause(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;
    curr_offset = offset;

    proto_tree_add_item(tree, hf_gsm_a_bssmap_reroute_rej_cause, tvb, curr_offset, 1, ENC_NA);
    curr_offset+=1;

    return(curr_offset - offset);

}

/*
 * 3.2.2.113 Send Sequence Number
 */
static guint16
be_send_seqn(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;
    curr_offset = offset;

    proto_tree_add_item(tree, hf_gsm_a_bssmap_send_seqn, tvb, curr_offset, 1, ENC_NA);
    curr_offset+=1;

    return(curr_offset - offset);

}

/*
 * 3.2.2.114 Reroute complete outcome
 */
static const value_string gsm_a_bssap_reroute_outcome_vals[] = {
    { 0x0,     "Reserved" },
    { 0x1,     "MS is accepted" },
    { 0x2,     "MS is not accepted" },
    { 0x3,     "MS is already registered" },
    { 0,        NULL }
};

static guint16
be_reroute_outcome(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;
    curr_offset = offset;

    proto_tree_add_item(tree, hf_gsm_a_bssmap_reroute_outcome, tvb, curr_offset, 1, ENC_NA);
    curr_offset+=1;

    return(curr_offset - offset);

}

/*
 * 3.2.2.115 Global Call Reference
 */
static guint16
be_global_call_ref(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;
    curr_offset = offset;

    /* Global Call Reference Identifier */
    proto_tree_add_expert_format(tree, pinfo, &ei_gsm_a_bssmap_not_decoded_yet, tvb, curr_offset, len, "Field Element not decoded yet");

    return len;

}
/*
 * 3.2.2.116 LCLS-Configuration
 */
static const value_string gsm_a_bssap_lcls_conf_vals[] = {
    { 0x0,     "Connect both-way" },
    { 0x1,     "Connect both-way and bi-cast UL to the core network" },
    { 0x2,     "Connect both-way and send access DL from the core network" },
    { 0x3,     "Connect both-way and send access DL from the core network, block local DL user data" },
    { 0x4,     "Connect both-way and bi-cast UL to the core network with send access DL from the core network" },
    { 0x5,     "Connect both-way and bi-cast UL to the core network with send access DL from the core network, block local DL user data" },
    { 0,        NULL }
};

static guint16
be_lcls_conf(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;
    curr_offset = offset;

    proto_tree_add_item(tree, hf_gsm_a_bssmap_lcls_conf, tvb, curr_offset, 1, ENC_NA);
    curr_offset+=1;

    return(curr_offset - offset);

}

/*
 * 3.2.2.117 LCLS-Connection-Status-Control
 */
static const value_string gsm_a_bssap_lcls_con_status_control_vals[] = {
    { 0x0,     "Connect" },
    { 0x1,     "Do not connect" },
    { 0x2,     "Release LCLS" },
    { 0x3,     "Bi-cast UL at Handover" },
    { 0x4,     "Bi-cast UL and receive DL data at Handover" },
    { 0,        NULL }
};

static guint16
be_lcls_con_status_control(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;
    curr_offset = offset;

    proto_tree_add_item(tree, hf_gsm_a_bssmap_lcls_con_status_control, tvb, curr_offset, 1, ENC_NA);
    curr_offset+=1;

    return(curr_offset - offset);

}

/*
 * 3.2.2.118 LCLS-Correlation-Not-Needed
 * No data
 */

/*
 * 3.2.2.119 LCLS-BSS-Status
 */
static const value_string gsm_a_bssmap_lcls_bss_status_vals[] = {
    { 0x0,     "Call not yet locally switched" },
    { 0x1,     "Call not possible to be locally switched" },
    { 0x2,     "Call is no longer locally switched" },
    { 0x3,     "Requested LCLS configuration is not supported" },
    { 0x4,     "Call is locally switched with requested LCLS configuration" },
    { 0,        NULL }
};

static guint16
be_lcls_bss_status(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;
    curr_offset = offset;

    proto_tree_add_item(tree, hf_gsm_a_bssmap_lcls_bss_status, tvb, curr_offset, 1, ENC_NA);
    curr_offset+=1;

    return(curr_offset - offset);

}

/*
 * 3.2.2.120 LCLS-Break-Request
 * No data
 */

/*
 * 3.2.2.121 CSFB Indication
 * No data
 */

/*
 * 3.2.2.123 Source eNB to target eNB transparent information (E-UTRAN)
 */

/*
 * 3.2.2.124 CS to PS SRVCC Indication
 */

/*
 * 3.2.2.125 CN to MS transparent information
 */

/*
 * 3.2.2.126 Selected PLMN ID
 */
static guint16
be_selected_plmn_id(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{

    proto_tree_add_string(tree, hf_gsm_a_bssmap_selected_plmn_id, tvb, offset, 3, dissect_e212_mcc_mnc_wmem_packet_str(tvb, pinfo, tree, offset, E212_NONE, TRUE));
    return 3;

}


guint16 (*bssmap_elem_fcn[])(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len, gchar *add_string, int string_len) = {
    NULL,               /* Undefined */
    be_cic,             /* Circuit Identity Code */
    NULL,               /* Reserved */
    be_res_avail,       /* Resource Available */
    be_cause,           /* Cause */
    be_cell_id,         /* Cell Identifier */
    be_prio,            /* Priority */
    be_l3_header_info,  /* Layer 3 Header Information */
    de_mid,             /* IMSI */
    be_tmsi,            /* TMSI */
    be_enc_info,        /* Encryption Information */
    be_chan_type,       /* Channel Type */
    be_periodicity,     /* Periodicity */
    be_ext_res_ind,     /* Extended Resource Indicator */
    be_num_ms,          /* Number Of MSs */
    NULL,               /* Reserved */
    NULL,               /* Reserved */
    NULL,               /* Reserved */
    de_ms_cm_2,         /* Classmark Information Type 2 */
    de_ms_cm_3,         /* Classmark Information Type 3 */
    be_int_band,        /* Interference Band To Be Used */
    de_rr_cause,        /* RR Cause */
    NULL,               /* Reserved */
    be_l3_info,         /* Layer 3 Information */
    be_dlci,            /* DLCI */
    be_down_dtx_flag,   /* Downlink DTX Flag */
    be_cell_id_list,    /* Cell Identifier List */
    NULL                /* no associated data */,  /* Response Request */
    be_res_ind_method,  /* Resource Indication Method */
    de_ms_cm_1,         /* Classmark Information Type 1 */
    be_cic_list,        /* Circuit Identity Code List */
    be_diag,            /* Diagnostics */
    be_l3_msg,          /* Layer 3 Message Contents */
    be_chosen_chan,     /* Chosen Channel */
    be_tot_res_acc,     /* Total Resource Accessible */
    be_ciph_resp_mode,  /* Cipher Response Mode */
    be_cha_needed,      /* Channel Needed */
    be_trace_type,      /* Trace Type */
    be_trace_trigger_id,/* TriggerID */
    be_trace_reference, /* Trace Reference */
    be_trace_transaction_id, /* TransactionID */
    de_mid,             /* Mobile Identity */
    be_trace_omc_id,    /* OMCID */
    be_for_ind,         /* Forward Indicator */
    be_chosen_enc_alg,  /* Chosen Encryption Algorithm */
    be_cct_pool,        /* Circuit Pool */
    NULL,               /* Circuit Pool List */
    NULL,               /* Time Indication */
    NULL,               /* Resource Situation */
    be_curr_chan_1,     /* Current Channel Type 1 */
    be_que_ind,         /* Queueing Indicator */
    be_ass_req,         /* Assignment Requirement */
    NULL,               /* Undefined */
    NULL                /* no associated data */,   /* Talker Flag */
    NULL                /* no associated data */,   /* Connection Release Requested */
    de_d_gb_call_ref,   /* Group Call Reference */
    be_emlpp_prio,      /* eMLPP Priority */
    be_conf_evo_ind,    /* Configuration Evolution Indication */
    be_field_element_dissect,   /* Old BSS to New BSS Information */
    be_lsa_id,          /* LSA Identifier */
    be_lsa_id_list,     /* LSA Identifier List */
    be_lsa_info,        /* LSA Information */
    NULL,               /* LCS QoS Dissected in packet-gsm_bssmap_le.c*/
    NULL,               /* LSA access control suppression */
    be_speech_ver,      /* Speech Version */
    NULL,               /* Undefined */
    NULL,               /* Undefined */
    be_lcs_prio,        /* LCS Priority */
    be_loc_type,        /* Location Type */
    be_loc_est,         /* Location Estimate */
    be_pos_data,        /* Positioning Data */
    NULL,               /* 3.2.2.66 LCS Cause Dissected in packet-gsm_bssmap_le.c */
    NULL,               /* LCS Client Type Dissected in packet-gsm_bssmap_le.c */
    be_apdu,            /* APDU */
    NULL,               /* Network Element Identity */
    be_gps_assist_data, /* GPS Assistance Data */
    NULL,               /* Deciphering Keys (dissected in packet-gsm_bssmap_le)*/
    be_ret_err_req,     /* Return Error Request */
    be_ret_err_cause,   /* Return Error Cause */
    be_seg,             /* Segmentation */
    be_serv_ho,         /* Service Handover */
    be_src_rnc_to_tar_rnc_umts, /* Source RNC to target RNC transparent information (UMTS) */
    be_src_rnc_to_tar_rnc_cdma, /* Source RNC to target RNC transparent information (cdma2000) */
    be_geran_cls_m,     /* GERAN Classmark */
    be_geran_bsc_cont,  /* GERAN BSC Container */
    be_vel_est,         /* Velocity Estimate */
    NULL,               /* Undefined */
    NULL,               /* Undefined */
    NULL,               /* Undefined */
    NULL,               /* Undefined */
    NULL,               /* Undefined */
    NULL,               /* Undefined */
    NULL,               /* Undefined */
    NULL,               /* Undefined */
    NULL,               /* Undefined */
    NULL,               /* Undefined */
    NULL,               /* Undefined */
    be_field_element_dissect,   /* New BSS to Old BSS Information */
    NULL,               /* Undefined */
    be_inter_sys_inf,   /*  Inter-System Information */
    be_sna_acc_inf,     /* SNA Access Information */
    be_vstk_rand_info,  /* VSTK_RAND Information */
    be_vstk_info,       /* VSTK Information */
    be_paging_inf,      /* Paging Information */
    de_mid,             /* 3.2.2.86 IMEI (use same dissector as IMSI)*/
    be_vgcs_feat_flg,   /* VGCS Feature Flags */
    be_talker_pri,      /* Talker Priority */
    NULL,               /* no data Emergency Set Indication */
    be_talker_id,       /* Talker Identity */
    be_cell_id_list_seg,    /* Cell Identifier List Segment */
    be_sms_to_vgcs,     /* SMS to VGCS */
    be_vgcs_talker_mode,/*  VGCS Talker Mode */
    be_vgcs_vbs_cell_status,            /* VGCS/VBS Cell Status */
    be_cell_id_lst_seg_f_est_cells,     /* Cell Identifier List Segment for established cells */
    be_cell_id_lst_seg_f_cell_tb_est,   /* Cell Identifier List Segment for cells to be established */
    be_cell_id_lst_seg_f_rel_cell,      /* Cell Identifier List Segment for released cells - no user present */
    be_cell_id_lst_seg_f_not_est_cell,  /* Cell Identifier List Segment for not established cells - no establishment possible */
    be_ganss_ass_dta,   /*  GANSS Assistance Data */
    be_ganss_pos_dta,   /*  GANSS Positioning Data */
    be_ganss_loc_type,  /* GANSS Location Type */
    be_app_data,        /* Application Data */
    be_app_data_id,     /* Data Identity */
    be_app_data_inf,    /*  Application Data Information */
    be_msisdn,          /* MSISDN */
    be_aoip_trans_lay_add,  /*  AoIP Transport Layer Address */
    be_speech_codec_lst,/* Speech Codec List */
    be_speech_codec,    /* Speech Codec */
    be_call_id,         /* Call Identifier */
    be_call_id_lst,     /* Call Identifier List */
    be_a_itf_sel_for_reset, /* A-Interface Selector for RESET */
    NULL,               /* Undefined */
    be_kc128,           /* Kc128 */
    be_csg_id,                          /* CSG Identifier */
    NULL,                               /* Redirect Attempt Flag                3.2.2.111    No data */
    be_reroute_rej_cause,               /* Reroute Reject Cause                 3.2.2.112    */
    be_send_seqn,                       /* Send Sequence Number                 3.2.2.113    */
    be_reroute_outcome,                 /* Reroute complete outcome             3.2.2.114    */
    be_global_call_ref,                 /* Global Call Reference                3.2.2.115    */
    be_lcls_conf,                       /* LCLS-Configuration                   3.2.2.116    */
    be_lcls_con_status_control,         /* LCLS-Connection-Status-Control       3.2.2.117    */
    NULL,                               /* LCLS-Correlation-Not-Needed          3.2.2.118    No data */
    be_lcls_bss_status,                 /* LCLS-BSS-Status                      3.2.2.119    */
    NULL,                               /* LCLS-Break-Request                   3.2.2.120    No data */
    NULL,                               /* CSFB Indication                      3.2.2.121    No data */
#if 0
    BE_CS_TO_PS_SRVCC,                  /* CS to PS SRVCC                       3.2.2.122    */
    BE_SRC_ENB_2_TGT_ENB_TRANSP_INF,    /* Source eNB to target eNB transparent information (E-UTRAN)" 3.2.2.123    */
    BE_CS_TO_PS_SRVCC_IND,              /* CS to PS SRVCC Indication            3.2.2.124    */
    BE_CN_TO_MS_TRANSP,                 /* CN to MS transparent information     3.2.2.125    */
#endif
    be_selected_plmn_id,                /* Selected PLMN ID                     3.2.2.126    */
    NULL                                /* NONE */

};
/* 3.2.3    Signalling Field Element Coding */
/* 3.2.3.1  Extra information */
static const value_string fe_extra_info_prec_vals[] = {
    { 0, "The old BSS recommends that this allocation request should not cause a pre-emption an existing connection" },
    { 1, "The old BSS recommends that this allocation request is allowed to preempt an existing connection based on the information supplied in the Priority information element, if available" },
    { 0, NULL}
};

static const value_string fe_extra_info_lcs_vals[] = {
    { 0, "No ongoing LCS procedure" },
    { 1, "An ongoing LCS procedure was interrupted by handover. The new BSS may notify the SMLC when the handover is completed" },
    { 0, NULL}
};

static const value_string fe_extra_info_ue_prob_vals[] = {
    { 0, "This MS supports handover to UMTS" },
    { 1, "This MS does not support handover to UMTS" },
    { 0, NULL}
};

static guint16
be_fe_extra_info(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;
    proto_tree_add_item(tree, hf_fe_extra_info_prec, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_fe_extra_info_lcs, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_fe_extra_info_ue_prob, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_fe_extra_info_spare, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;

    return(curr_offset - offset);
}

/* 3.2.3.2  Current Channel type 2 */
static guint16
be_fe_cur_chan_type2(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;
    proto_tree_add_item(tree, hf_fe_cur_chan_type2_chan_mode, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_fe_cur_chan_type2_chan_mode_spare, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;
    proto_tree_add_item(tree, hf_fe_cur_chan_type2_chan_field, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_fe_cur_chan_type2_chan_field_spare, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;

    return(curr_offset - offset);
}

/* 3.2.3.3  Target cell radio information */
static guint16
be_fe_target_radio_cell_info(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;
    proto_tree_add_item(tree, hf_fe_target_radio_cell_info_rxlev_ncell, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_fe_target_radio_cell_info_rxlev_ncell_spare, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;

    return(curr_offset - offset);
}

/* 3.2.3.4  GPRS Suspend Information */
static guint16
be_fe_gprs_suspend_info(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset = offset;

    /* This Field Element contains the contents of the Gb interface SUSPEND ACK PDU,
       Call the BSSGP dissector here, assuming that the encoding is per 48.018 */


    bssgp_suspend_ack(tvb, tree, pinfo, offset, len);
    curr_offset += len;

    return(curr_offset - offset);
}

/* 3.2.3.5  MultiRate configuration Information */

/* 3.2.3.6  Dual Transfer Mode information */
static const value_string gsm_a_bssmap_dtm_info_dtm_ind_vals[] = {
    { 0,    "The MS has resources allocated exclusively for the CS domain in the old cell" },
    { 1,    "The MS has resources allocated for both the CS and PS domains in the old cell" },
    { 0, NULL }
};

static const value_string gsm_a_bssmap_dtm_info_sto_ind_vals[] = {
    { 0,    "The MS is in multislot operation in the old cell" },
    { 1,    "The MS is in single timeslot operation in the old cell" },
    { 0, NULL }
};

static const value_string gsm_a_bssmap_dtm_info_egprs_ind_vals[] = {
    { 0,    "The MS has no TBF using E-GPRS in the old cell" },
    { 1,    "The MS has a TBF using E-GPRS in the old cell" },
    { 0, NULL }
};

static guint16
be_fe_dual_transfer_mode_info(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;
    proto_tree_add_item(tree, hf_fe_dtm_info_dtm_ind, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_fe_dtm_info_sto_ind, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_fe_dtm_info_egprs_ind, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_fe_dtm_info_spare_bits, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;

    return(curr_offset - offset);
}

/* 3.2.3.7  Inter RAT Handover Info */
static guint16
be_fe_inter_rat_handover_info(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    tvbuff_t    *container_tvb;

    /* Octets 3-n are encoded as Inter RAT Handover Info as defined in 3GPP TS 25.331 */
    container_tvb = tvb_new_subset_length(tvb, offset, len);
    dissect_rrc_InterRATHandoverInfo_PDU(container_tvb, pinfo, tree, NULL);

    return len;
}

/* 3.2.3.8  cdma2000 Capability Information */

/* 3.2.3.9  Downlink Cell Load Information */

/* 3.2.3.10 Uplink Cell Load Information */


static const value_string gsm_a_bssmap_cell_load_nrt_vals[] = {
    { 0,    "NRT Load is low" },
    { 1,    "NRT load is medium" },
    { 2,    "NRT load is high. (Probability to admit a new user is low.)" },
    { 3,    "NRT overload. (Probability to admit a new user is low, packets are discarded and the source is recommended to reduce the data flow.)" },
    { 0, NULL }
};

/* 3.2.3.11 Cell Load Information Group */
static guint16
be_fe_cell_load_info_group(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;
    curr_offset += be_cell_id(tvb, tree, pinfo, curr_offset, len, NULL, 0);
    curr_offset += be_field_element_dissect(tvb, tree, pinfo, curr_offset, len + offset - curr_offset, NULL, 0);

    return(curr_offset - offset);
}

/* 3.2.3.12 Cell Load Information */
static guint16
be_fe_cell_load_info(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;
    proto_tree_add_item(tree, hf_fe_cell_load_info_cell_capacity_class, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;
    proto_tree_add_item(tree, hf_fe_cell_load_info_load_value, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;
    proto_tree_add_item(tree, hf_fe_cell_load_info_rt_load_value, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;
    proto_tree_add_item(tree, hf_fe_cell_load_info_nrt_load_information_value, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;

    return(curr_offset - offset);
}

/* 3.2.3.13 PS Indication */
static guint16
be_fe_ps_indication(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;
    proto_tree_add_item(tree, hf_fe_ps_indication, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;

    return(curr_offset - offset);
}

/* 3.2.3.14 DTM Handover Command Indication */
static guint16
be_fe_dtm_ho_command_ind(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    curr_offset = offset;
    proto_tree_add_item(tree, hf_fe_dtm_ho_command_ind_spare, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    curr_offset++;

    return(curr_offset - offset);
}

static guint16 (*bssmap_bss_to_bss_element_fcn[])(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_) = {
    be_fe_extra_info,              /* { 0x01,       "Extra information" }, */
    be_fe_cur_chan_type2,          /* { 0x02,       "Current Channel Type 2" }, */
    be_fe_target_radio_cell_info,  /* { 0x03,       "Target cell radio information" }, */
    be_fe_gprs_suspend_info,       /* { 0x04,       "GPRS Suspend information" }, */
    de_rr_multirate_conf,          /* { 0x05,       "MultiRate configuration information" }, */
    be_fe_dual_transfer_mode_info, /* { 0x06,       "Dual Transfer Mode Information" }, */
    be_fe_inter_rat_handover_info, /* { 0x07,       "Inter RAT Handover Info" }, */
    NULL,                          /* { 0x08,       "cdma2000 Capability Information" }, */
    be_fe_cell_load_info,          /* { 0x09,       "Downlink Cell Load Information" }, */
    be_fe_cell_load_info,          /* { 0x0a,       "Uplink Cell Load Information" }, */
    be_fe_cell_load_info_group,    /* { 0x0b,       "Cell Load Information Group" }, */
    be_fe_cell_load_info,          /* { 0x0c,       "Cell Load Information" }, */
    be_fe_ps_indication,           /* { 0x0d,       "PS Indication" }, */
    be_fe_dtm_ho_command_ind,      /* { 0x0e,       "DTM Handover Command Indication" }, */
    be_vgcs_talker_mode,           /* { 0x6f,       "VGCS talker mode" }, */ /* not really a field element
                                                     but does appear in old bss to new bss info */
    NULL,   /* NONE */
};

#define NUM_BSS_ELEMENT_FCNS   (int)(sizeof(bssmap_bss_to_bss_element_fcn)/(sizeof bssmap_bss_to_bss_element_fcn[0]))

static guint16
be_field_element_dissect(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len _U_, gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset, ie_len, fe_start_offset;
    gint idx;
    const gchar *str;
    proto_item *item = NULL;
    proto_tree *  bss_to_bss_tree = NULL;

    curr_offset = offset;


    while (curr_offset - offset + 2 < len) {
        guint8 oct;
        /*
         * add name
         */
        oct = tvb_get_guint8(tvb, curr_offset++);

        str = try_val_to_str_idx((guint32) oct, bssmap_field_element_ids, &idx);
        ie_len = tvb_get_guint8(tvb, curr_offset++);

        if (!str)
            str = "Unknown";

        /*
         * add Field Element name
         */
        item = proto_tree_add_uint_format(tree, hf_gsm_a_bssmap_field_elem_id,
        tvb, curr_offset - 2, ie_len + 2, oct, "%s (%X)", str, oct);

        bss_to_bss_tree = proto_item_add_subtree(item, ett_bss_to_bss_info);
        fe_start_offset = curr_offset;

        /*
         * decode field element
         */
        if (idx < 0 || idx >= NUM_BSS_ELEMENT_FCNS ||
           (bssmap_bss_to_bss_element_fcn[idx] == NULL))
        {
            proto_tree_add_expert_format(bss_to_bss_tree, pinfo, &ei_gsm_a_bssmap_not_decoded_yet,
                tvb, curr_offset, ie_len, "Field Element not decoded");
            curr_offset += ie_len;
        }
        else
        {
            /* dissect the field element */
            curr_offset += (*bssmap_bss_to_bss_element_fcn[idx])(tvb, bss_to_bss_tree, pinfo, curr_offset, ie_len, NULL, 0);

            EXTRANEOUS_DATA_CHECK(ie_len, curr_offset - fe_start_offset, pinfo, &ei_gsm_a_bssmap_extraneous_data);

        }
    }
    return len;
}
/* MESSAGE FUNCTIONS */

/*
 *  [2] 3.2.1.1 ASSIGNMENT REQUEST
 * 48.008 8.4.0
 */
static void
bssmap_ass_req(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Channel Type 3.2.2.11    MSC-BSS     M   5-13 */
    ELEM_MAND_TLV(BE_CHAN_TYPE, GSM_A_PDU_TYPE_BSSMAP, BE_CHAN_TYPE, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Layer 3 Header Information   3.2.2.9     MSC-BSS     O (note 3)  4 */
    ELEM_OPT_TLV(BE_L3_HEADER_INFO, GSM_A_PDU_TYPE_BSSMAP, BE_L3_HEADER_INFO, NULL);
    /* Priority 3.2.2.18    MSC-BSS     O   3 */
    ELEM_OPT_TLV(BE_PRIO, GSM_A_PDU_TYPE_BSSMAP, BE_PRIO, NULL);
    /* Circuit Identity Code    3.2.2.2     MSC-BSS     O (note 1, 12   3 */
    ELEM_OPT_TV(BE_CIC, GSM_A_PDU_TYPE_BSSMAP, BE_CIC, NULL);
    /* Downlink DTX Flag    3.2.2.26    MSC-BSS     O (note 2)  2 */
    ELEM_OPT_TV(BE_DOWN_DTX_FLAG, GSM_A_PDU_TYPE_BSSMAP, BE_DOWN_DTX_FLAG, NULL);
    /* Interference Band To Be Used 3.2.2.21    MSC-BSS     O   2 */
    ELEM_OPT_TV(BE_INT_BAND, GSM_A_PDU_TYPE_BSSMAP, BE_INT_BAND, NULL);
    /* Classmark Information 2  3.2.2.19    MSC-BSS     O (note 4)  4-5 */
    ELEM_OPT_TLV(BE_CM_INFO_2, GSM_A_PDU_TYPE_BSSMAP, BE_CM_INFO_2, NULL);
    /* Group Call Reference 3.2.2.55    MSC-BSS     O (note 5)  7 */
    ELEM_OPT_TLV(BE_GROUP_CALL_REF, GSM_A_PDU_TYPE_BSSMAP, BE_GROUP_CALL_REF, NULL);
    /* Talker Flag  3.2.2.54    MSC-BSS     O (note 6)  1 */
    ELEM_OPT_T(BE_TALKER_FLAG, GSM_A_PDU_TYPE_BSSMAP, BE_TALKER_FLAG, NULL);
    /* Configuration Evolution Indication   3.2.2.57    MSC-BSS O (note 7)  2 */
    ELEM_OPT_TV(BE_CONF_EVO_IND, GSM_A_PDU_TYPE_BSSMAP, BE_CONF_EVO_IND, NULL);
    /* LSA Access Control Suppression   3.2.2.61    MSC-BSS O (note 8)  2 */
    ELEM_OPT_TV(BE_LSA_ACC_CTRL, GSM_A_PDU_TYPE_BSSMAP, BE_LSA_ACC_CTRL, NULL);
    /* Service Handover 3.2.2.75    MSC-BSS O (note 9)  3 */
    ELEM_OPT_TLV(BE_SERV_HO, GSM_A_PDU_TYPE_BSSMAP, BE_SERV_HO, NULL);
    /* Encryption Information   3.2.2.10    MSC-BSS O (note 10) 3-n */
    ELEM_OPT_TLV(BE_ENC_INFO, GSM_A_PDU_TYPE_BSSMAP, BE_ENC_INFO, NULL);
    /* Talker Priority  3.2.2.89    MSC-BSS O (note 11) 2 */
    ELEM_OPT_TV(BE_TALKER_PRI, GSM_A_PDU_TYPE_BSSMAP, BE_TALKER_PRI, NULL);
    /* AoIP Transport Layer Address (MGW)   3.2.2.102   MSC-BSS O (note 12) 10-22 */
    ELEM_OPT_TLV(BE_AOIP_TRANS_LAY_ADD, GSM_A_PDU_TYPE_BSSMAP, BE_AOIP_TRANS_LAY_ADD, NULL);
    /* Codec List (MSC Preferred)   3.2.2.103   MSC-BSS O (note 13) 3-n */
    ELEM_OPT_TLV(BE_SPEECH_CODEC_LST, GSM_A_PDU_TYPE_BSSMAP, BE_SPEECH_CODEC_LST, "(MSC Preferred)");
    /* Call Identifier  3.2.2.105   MSC-BSS O (note 12) 5 */
    ELEM_OPT_TV(BE_CALL_ID, GSM_A_PDU_TYPE_BSSMAP, BE_CALL_ID, NULL);
    /* Kc128  3.2.2.109   MSC-BSS C (note 15) 17 */
    ELEM_OPT_TV(BE_KC128, GSM_A_PDU_TYPE_BSSMAP, BE_KC128, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 *  [2] 3.2.1.2 ASSIGNMENT COMPLETE
 */
static void
bssmap_ass_complete(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* RR Cause 3.2.2.22    BSS-MSC     O   2 */
    ELEM_OPT_TV(BE_RR_CAUSE, GSM_A_PDU_TYPE_BSSMAP, BE_RR_CAUSE, NULL);
    /* Circuit Identity Code    3.2.2.2 BSS-MSC O (note 4)  3 */
    ELEM_OPT_TV(BE_CIC, GSM_A_PDU_TYPE_BSSMAP, BE_CIC, NULL);
    /* Cell Identifier  3.2.2.17    BSS-MSC     O (note 1)  3-10 */
    ELEM_OPT_TLV(BE_CELL_ID, GSM_A_PDU_TYPE_BSSMAP, BE_CELL_ID, NULL);
    /* Chosen Channel   3.2.2.33    BSS-MSC     O (note 3)  2 */
    ELEM_OPT_TV(BE_CHOSEN_CHAN, GSM_A_PDU_TYPE_BSSMAP, BE_CHOSEN_CHAN, NULL);
    /* Chosen Encryption Algorithm  3.2.2.44    BSS-MSC     O (note 5)  2 */
    ELEM_OPT_TV(BE_CHOSEN_ENC_ALG, GSM_A_PDU_TYPE_BSSMAP, BE_CHOSEN_ENC_ALG, NULL);
    /* Circuit Pool 3.2.2.45    BSS-MSC     O (note 2)  2 */
    ELEM_OPT_TV(BE_CCT_POOL, GSM_A_PDU_TYPE_BSSMAP, BE_CCT_POOL, NULL);
    /* Speech Version (Chosen)  3.2.2.51    BSS-MSC     O (note 6)  2 */
    ELEM_OPT_TV(BE_SPEECH_VER, GSM_A_PDU_TYPE_BSSMAP, BE_SPEECH_VER, " (Chosen)");
    /* LSA Identifier   3.2.2.15    BSS-MSC O (note 7)  5 */
    ELEM_OPT_TLV(BE_LSA_ID, GSM_A_PDU_TYPE_BSSMAP, BE_LSA_ID, NULL);
    /* Talker Priority  3.2.2.89    BSS-MSC O (note 8)  2 */
    ELEM_OPT_TV(BE_TALKER_PRI, GSM_A_PDU_TYPE_BSSMAP, BE_TALKER_PRI, NULL);
    /* AoIP Transport Layer Address (BSS)   3.2.2.102   BSS-MSC O (note 9)  10-22 */
    ELEM_OPT_TLV(BE_AOIP_TRANS_LAY_ADD, GSM_A_PDU_TYPE_BSSMAP, BE_AOIP_TRANS_LAY_ADD, NULL);
    /* Speech Codec (Chosen)    3.2.2.104   BSS-MSC O (note 10)     3-5 */
    ELEM_OPT_TLV(BE_SPEECH_CODEC, GSM_A_PDU_TYPE_BSSMAP, BE_SPEECH_CODEC, "(Chosen)");
    /* Codec List (BSS supported)   3.2.2.103   MSC-BSS O (note 11) 3-n */
    ELEM_OPT_TLV(BE_SPEECH_CODEC_LST, GSM_A_PDU_TYPE_BSSMAP, BE_SPEECH_CODEC_LST, "(BSS Supported)");

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 *  [2] 3.2.1.3 ASSIGNMENT FAILURE
 */
static void
bssmap_ass_failure(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Cause    3.2.2.5     BSS-MSC     M   3-4 */
    ELEM_MAND_TLV(BE_CAUSE, GSM_A_PDU_TYPE_BSSMAP, BE_CAUSE, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* RR Cause 3.2.2.22    BSS-MSC     O   2 */
    ELEM_OPT_TV(BE_RR_CAUSE, GSM_A_PDU_TYPE_BSSMAP, BE_RR_CAUSE, NULL);
    /* Circuit Pool 3.2.2.45    BSS-MSC     O (note 1)  2 */
    ELEM_OPT_TV(BE_CCT_POOL, GSM_A_PDU_TYPE_BSSMAP, BE_CCT_POOL, NULL);
    /* Circuit Pool List    3.2.2.46    BSS-MSC     O (note 2)  V */
    ELEM_OPT_TLV(BE_CCT_POOL_LIST, GSM_A_PDU_TYPE_BSSMAP, BE_CCT_POOL_LIST, NULL);
    /* Talker Priority  3.2.2.89    BSS-MSC O (note 3)  2 */
    ELEM_OPT_TV(BE_TALKER_PRI, GSM_A_PDU_TYPE_BSSMAP, BE_TALKER_PRI, NULL);
    /* Codec List (BSS Supported)   3.2.2.103   BSS-MSC O (note 4)  3-n */
    ELEM_OPT_TLV(BE_SPEECH_CODEC_LST, GSM_A_PDU_TYPE_BSSMAP, BE_SPEECH_CODEC_LST, "(BSS Supported)");

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 *  [2] 3.2.1.4 BLOCK
 */
static void
bssmap_block(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Circuit Identity Code    3.2.2.2     both    M   3*/
    ELEM_MAND_TV(BE_CIC, GSM_A_PDU_TYPE_BSSMAP, BE_CIC, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Cause    3.2.2.5     both    M   3-4  */
    ELEM_MAND_TLV(BE_CAUSE, GSM_A_PDU_TYPE_BSSMAP, BE_CAUSE, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Connection Release Requested 3.2.2.3 MSC-BSS O   1 */
    ELEM_OPT_T(BE_CONN_REL_REQ, GSM_A_PDU_TYPE_BSSMAP, BE_CONN_REL_REQ, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 *  [2] 3.2.1.5 BLOCKING ACKNOWLEDGE
 */

static void
bssmap_block_ack(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Circuit Identity Code    3.2.2.2     both    M   3 */
    ELEM_MAND_TV(BE_CIC, GSM_A_PDU_TYPE_BSSMAP, BE_CIC, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 *  [2] 3.2.1.6 UNBLOCK
 */
static void
bssmap_unblock(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Circuit Identity Code    3.2.2.2     both    M   3  */
    ELEM_MAND_TV(BE_CIC, GSM_A_PDU_TYPE_BSSMAP, BE_CIC, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 *  [2] 3.2.1.7 UNBLOCKING ACKNOWLEDGE
 */
static void
bssmap_unblock_ack(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Circuit Identity Code    3.2.2.2     both    M   3 */
    ELEM_MAND_TV(BE_CIC, GSM_A_PDU_TYPE_BSSMAP, BE_CIC, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 *  [2] 3.2.1.8 HANDOVER REQUEST
 */
static void
bssmap_ho_req(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Channel Type 3.2.2.11    MSC-BSS     M   5-13  */
    ELEM_MAND_TLV(BE_CHAN_TYPE, GSM_A_PDU_TYPE_BSSMAP, BE_CHAN_TYPE, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Encryption Information   3.2.2.10    MSC-BSS     M (note 1)  3-n */
    ELEM_MAND_TLV(BE_ENC_INFO, GSM_A_PDU_TYPE_BSSMAP, BE_ENC_INFO, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    /* Classmark Information 1 3.2.2.30 MSC-BSS M# 2
     * or
     * Classmark Information 2 3.2.2.19 MSC-BSS M (note 6)  4-5
     */
    ELEM_OPT_TV(BE_CM_INFO_1, GSM_A_PDU_TYPE_BSSMAP, BE_CM_INFO_1, NULL);

    ELEM_OPT_TLV(BE_CM_INFO_2, GSM_A_PDU_TYPE_BSSMAP, BE_CM_INFO_2, NULL);
    /* Cell Identifier (Serving)    3.2.2.17    MSC-BSS     M (note 20) 5-10  */
    ELEM_MAND_TLV(BE_CELL_ID, GSM_A_PDU_TYPE_BSSMAP, BE_CELL_ID, " (Serving)", ei_gsm_a_bssmap_missing_mandatory_element);
    /* Priority 3.2.2.18    MSC-BSS     O   3  */
    ELEM_OPT_TLV(BE_PRIO, GSM_A_PDU_TYPE_BSSMAP, BE_PRIO, NULL);
    /* Circuit Identity Code    3.2.2.2     MSC-BSS     O (note 7, 28   3 */
    ELEM_OPT_TV(BE_CIC, GSM_A_PDU_TYPE_BSSMAP, BE_CIC, NULL);
    /* Downlink DTX Flag    3.2.2.26    MSC-BSS     O (note 3)  2 */
    ELEM_OPT_TV(BE_DOWN_DTX_FLAG, GSM_A_PDU_TYPE_BSSMAP, BE_DOWN_DTX_FLAG, NULL);
    /* Cell Identifier (Target) 3.2.2.17    MSC-BSS     M (note 17) 3-10 */
    ELEM_MAND_TLV(BE_CELL_ID, GSM_A_PDU_TYPE_BSSMAP, BE_CELL_ID, " (Target)", ei_gsm_a_bssmap_missing_mandatory_element);
    /* Interference Band To Be Used 3.2.2.21    MSC-BSS     O   2 */
    ELEM_OPT_TV(BE_INT_BAND, GSM_A_PDU_TYPE_BSSMAP, BE_INT_BAND, NULL);
    /* Cause    3.2.2.5     MSC-BSS     O (note 9)   3-4 */
    ELEM_OPT_TLV(BE_CAUSE, GSM_A_PDU_TYPE_BSSMAP, BE_CAUSE, NULL);
    /* Classmark Information 3  3.2.2.20    MSC-BSS     O (note 4)   3-34 */
    ELEM_OPT_TLV(BE_CM_INFO_3, GSM_A_PDU_TYPE_BSSMAP, BE_CM_INFO_3, NULL);
    /* Current Channel type 1   3.2.2.49    MSC-BSS     O (note 8)  2 */
    ELEM_OPT_TV(BE_CURR_CHAN_1, GSM_A_PDU_TYPE_BSSMAP, BE_CURR_CHAN_1, NULL);
    /* Speech Version (Used)    3.2.2.51    MSC-BSS     O (note 10) 2 */
    ELEM_OPT_TV(BE_SPEECH_VER, GSM_A_PDU_TYPE_BSSMAP, BE_SPEECH_VER, " (Used)");
    /* Group Call Reference 3.2.2.55    MSC-BSS     O (note 5)  7 */
    ELEM_OPT_TLV(BE_GROUP_CALL_REF, GSM_A_PDU_TYPE_BSSMAP, BE_GROUP_CALL_REF, NULL);
    /* Talker Flag  3.2.2.54    MSC-BSS     O (note 11) 1 */
    ELEM_OPT_T(BE_TALKER_FLAG, GSM_A_PDU_TYPE_BSSMAP, BE_TALKER_FLAG, NULL);
    /* Configuration Evolution Indication   3.2.2.57    MSC-BSS O (note 12) 2 */
    ELEM_OPT_TV(BE_CONF_EVO_IND, GSM_A_PDU_TYPE_BSSMAP, BE_CONF_EVO_IND, NULL);
    /* Chosen Encryption Algorithm (Serving)    3.2.2.44    MSC-BSS O (note 2)  2 */
    ELEM_OPT_TV(BE_CHOSEN_ENC_ALG, GSM_A_PDU_TYPE_BSSMAP, BE_CHOSEN_ENC_ALG, " (Serving)");
    /* Old BSS to New BSS Information   3.2.2.58    MSC-BSS O (note 13) 2-n */
    ELEM_OPT_TLV(BE_OLD2NEW_INFO, GSM_A_PDU_TYPE_BSSMAP, BE_OLD2NEW_INFO, NULL);
    /* LSA Information  3.2.2.23    MSC-BSS O (note 14) 3+4n */
    ELEM_OPT_TLV(BE_LSA_INFO, GSM_A_PDU_TYPE_BSSMAP, BE_LSA_INFO, NULL);
    /* LSA Access Control Suppression   3.2.2.61    MSC-BSS O (note 15)     2 */
    ELEM_OPT_TV(BE_LSA_ACC_CTRL, GSM_A_PDU_TYPE_BSSMAP, BE_LSA_ACC_CTRL, NULL);
    /* Service Handover 3.2.2.75    MSC-BSS O (note 21) 3 */
    ELEM_OPT_TLV(BE_SERV_HO, GSM_A_PDU_TYPE_BSSMAP, BE_SERV_HO, NULL);
    /* IMSI 3.2.2.6 MSC-BSC O (note 16) 3-10 */
    ELEM_OPT_TLV(BE_IMSI, GSM_A_PDU_TYPE_BSSMAP, BE_IMSI, NULL);
    /* Source RNC to target RNC transparent information (UMTS)  3.2.2.76    MSC-BSS O (note 18) n-m */
    ELEM_OPT_TLV(BE_SRC_RNC_TO_TAR_RNC_UMTS, GSM_A_PDU_TYPE_BSSMAP, BE_SRC_RNC_TO_TAR_RNC_UMTS, NULL);
    /* Source RNC to target RNC transparent information (cdma2000)  3.2.2.77    MSC-BSS O (note 19) n-m */
    ELEM_OPT_TLV(BE_SRC_RNC_TO_TAR_RNC_CDMA, GSM_A_PDU_TYPE_BSSMAP, BE_SRC_RNC_TO_TAR_RNC_CDMA, NULL);
    /* SNA Access Information   3.2.2.82    MSC-BSC O (note 22) 2+n */
    ELEM_OPT_TLV(BE_SNA_ACC_INF, GSM_A_PDU_TYPE_BSSMAP, BE_SNA_ACC_INF, NULL);
    /* Talker Priority  3.2.2.89    MSC-BSC O (note 23) 2 */
    ELEM_OPT_TV(BE_TALKER_PRI, GSM_A_PDU_TYPE_BSSMAP, BE_TALKER_PRI, NULL);
    /* AoIP Transport Layer Address (MGW)   3.2.2.102   MSC-BSS O (note 24) 10-22 */
    ELEM_OPT_TLV(BE_AOIP_TRANS_LAY_ADD, GSM_A_PDU_TYPE_BSSMAP, BE_AOIP_TRANS_LAY_ADD, NULL);
    /* Codec List (MSC Preferred)   3.2.2.103   MSC-BSS O (note 25) 3-n */
    ELEM_OPT_TLV(BE_SPEECH_CODEC_LST, GSM_A_PDU_TYPE_BSSMAP, BE_SPEECH_CODEC_LST, "(MSC Preferred)");
    /* Call Identifier  3.2.2.105   MSC-BSS O (note 24) 5 */
    ELEM_OPT_TV(BE_CALL_ID, GSM_A_PDU_TYPE_BSSMAP, BE_CALL_ID, NULL);
    /* Kc128  3.2.2.109   MSC-BSS C (note 27) 17 */
    ELEM_OPT_TV(BE_KC128, GSM_A_PDU_TYPE_BSSMAP, BE_KC128, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 *  [2] 3.2.1.9 HANDOVER REQUIRED
 */
static void
bssmap_ho_reqd(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Cause    3.2.2.5     BSS-MSC     M    3-4 */
    ELEM_MAND_TLV(BE_CAUSE, GSM_A_PDU_TYPE_BSSMAP, BE_CAUSE, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Response Request 3.2.2.28    BSS-MSC     O (note 8)  1 */
    ELEM_OPT_T(BE_RESP_REQ, GSM_A_PDU_TYPE_BSSMAP, BE_RESP_REQ, NULL);
    /* Cell Identifier List (Preferred) 3.2.2.27    BSS-MSC     M (note 4)  2n+3 to 7n+3 */
    ELEM_MAND_TLV(BE_CELL_ID_LIST, GSM_A_PDU_TYPE_BSSMAP, BE_CELL_ID_LIST, " (Preferred)", ei_gsm_a_bssmap_missing_mandatory_element);
    /* Circuit Pool List    3.2.2.46    BSS-MSC     O (note 1)  V */
    ELEM_OPT_TLV(BE_CCT_POOL_LIST, GSM_A_PDU_TYPE_BSSMAP, BE_CCT_POOL_LIST, NULL);
    /* Current Channel Type 1   3.2.2.49    BSS-MSC     O (note 2)  2 */
    ELEM_OPT_TV(BE_CURR_CHAN_1, GSM_A_PDU_TYPE_BSSMAP, BE_CURR_CHAN_1, NULL);
    /* Speech Version (Used)    3.2.2.51    BSS-MSC     O (note 3)  2 */
    ELEM_OPT_TV(BE_SPEECH_VER, GSM_A_PDU_TYPE_BSSMAP, BE_SPEECH_VER, " (Used)");
    /* Queueing Indicator   3.2.2.50    BSS-MSC     O   2 */
    ELEM_OPT_TV(BE_QUE_IND, GSM_A_PDU_TYPE_BSSMAP, BE_QUE_IND, NULL);
    /* Old BSS to New BSS Information   3.2.2.58    BSS-MSC O   2-n */
    ELEM_OPT_TLV(BE_OLD2NEW_INFO, GSM_A_PDU_TYPE_BSSMAP, BE_OLD2NEW_INFO, NULL);
    /* Source RNC to target RNC transparent information (UMTS)  3.2.2.76    BSS-MSC O (note 5)  3-m */
    ELEM_OPT_TLV(BE_SRC_RNC_TO_TAR_RNC_UMTS, GSM_A_PDU_TYPE_BSSMAP, BE_SRC_RNC_TO_TAR_RNC_UMTS, NULL);
    /* Source RNC to target RNC transparent information (cdma2000)  3.2.2.77    BSS-MSC O (note 6)  n-m */
    ELEM_OPT_TLV(BE_SRC_RNC_TO_TAR_RNC_CDMA, GSM_A_PDU_TYPE_BSSMAP, BE_SRC_RNC_TO_TAR_RNC_CDMA, NULL);
    /* GERAN Classmark  3.2.2.78    BSS-MSC O (note 7)  V */
    ELEM_OPT_TLV(BE_GERAN_CLS_M, GSM_A_PDU_TYPE_BSSMAP, BE_GERAN_CLS_M, NULL);
    /* Talker Priority  3.2.2.89    BSS-MSC O (note 9)  2 */
    ELEM_OPT_TV(BE_TALKER_PRI, GSM_A_PDU_TYPE_BSSMAP, BE_TALKER_PRI, NULL);
    /* Speech Codec (Used)  3.2.2.104   BSS-MSC O (note 10) 3-5 */
    ELEM_OPT_TLV(BE_SPEECH_CODEC, GSM_A_PDU_TYPE_BSSMAP, BE_SPEECH_CODEC, "(Used)");
    /* CSG Identifier  3.2.2.110   BSS-MSC O (note 11) 7 */
    ELEM_OPT_TLV(BE_CSG_ID, GSM_A_PDU_TYPE_BSSMAP, BE_CSG_ID, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 *  [2] 3.2.1.10 HANDOVER REQUEST ACKNOWLEDGE
 */
static void
bssmap_ho_req_ack(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Layer 3 Information  3.2.2.24    BSS-MSC     M (note 1)  11-n */
    ELEM_MAND_TLV(BE_L3_INFO, GSM_A_PDU_TYPE_BSSMAP, BE_L3_INFO, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Chosen Channel   3.2.2.33    BSS-MSC     O (note 4)  2 */
    ELEM_OPT_TV(BE_CHOSEN_CHAN, GSM_A_PDU_TYPE_BSSMAP, BE_CHOSEN_CHAN, NULL);
    /* Chosen Encryption Algorithm  3.2.2.44    BSS-MSC     O (note 5)  2 */
    ELEM_OPT_TV(BE_CHOSEN_ENC_ALG, GSM_A_PDU_TYPE_BSSMAP, BE_CHOSEN_ENC_ALG, NULL);
    /* Circuit Pool 3.2.2.45    BSS-MSC     O (note 2)  2 */
    ELEM_OPT_TV(BE_CCT_POOL, GSM_A_PDU_TYPE_BSSMAP, BE_CCT_POOL, NULL);
    /* Speech Version (Chosen)  3.2.2.51    BSS-MSC     O (note 6)  2 */
    ELEM_OPT_TV(BE_SPEECH_VER, GSM_A_PDU_TYPE_BSSMAP, BE_SPEECH_VER, " (Chosen)");
    /* Circuit Identity Code    3.2.2.2     BSS-MSC     O (note 3)  3 */
    ELEM_OPT_TV(BE_CIC, GSM_A_PDU_TYPE_BSSMAP, BE_CIC, NULL);
    /* LSA Identifier   3.2.2.15    BSS-MSC O (note 7)  5 */
    ELEM_OPT_TLV(BE_LSA_ID, GSM_A_PDU_TYPE_BSSMAP, BE_LSA_ID, NULL);
    /* New BSS to Old BSS Information   3.2.2.80    BSS-MSC O (note 8)  2-n */
    ELEM_OPT_TLV(BE_NEW_BSS_TO_OLD_BSS_INF, GSM_A_PDU_TYPE_BSSMAP, BE_NEW_BSS_TO_OLD_BSS_INF, NULL);
    /* Inter-System Information 3.2.2.81    BSS-MSC O (note 9)  2-n */
    ELEM_OPT_TLV(BE_INTER_SYS_INF, GSM_A_PDU_TYPE_BSSMAP, BE_INTER_SYS_INF, NULL);
    /* Talker Priority  3.2.2.89    BSS-MSC O (note 10) 2 */
    ELEM_OPT_TV(BE_TALKER_PRI, GSM_A_PDU_TYPE_BSSMAP, BE_TALKER_PRI, NULL);
    /* AoIP Transport Layer Address (BSS)   3.2.2.102   BSS-MSC O (note 11) 10-22 */
    ELEM_OPT_TLV(BE_AOIP_TRANS_LAY_ADD, GSM_A_PDU_TYPE_BSSMAP, BE_AOIP_TRANS_LAY_ADD, NULL);
    /* Codec List (BSS Supported)   3.2.2.103   BSS-MSC O (note 12) 3-n */
    ELEM_OPT_TLV(BE_SPEECH_CODEC_LST, GSM_A_PDU_TYPE_BSSMAP, BE_SPEECH_CODEC_LST, "(BSS Supported)");
    /* Speech Codec (Chosen)    3.2.2.104   BSS-MSC O (note 12) 3-5 */
    ELEM_OPT_TLV(BE_SPEECH_CODEC, GSM_A_PDU_TYPE_BSSMAP, BE_SPEECH_CODEC, "(Chosen)");

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 *  [2] 3.2.1.11 HANDOVER COMMAND
 */
static void
bssmap_ho_cmd(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Layer 3 Information  3.2.2.24    MSC-BSS     M (note 1)  11-n */
    ELEM_MAND_TLV(BE_L3_INFO, GSM_A_PDU_TYPE_BSSMAP, BE_L3_INFO, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Cell Identifier  3.2.2.17    MSC-BSS     O   3-10 */
    ELEM_OPT_TLV(BE_CELL_ID, GSM_A_PDU_TYPE_BSSMAP, BE_CELL_ID, NULL);
    /* New BSS to Old BSS Information   3.2.2.80    MSC-BSS     O (note 2)  2-n */
    ELEM_OPT_TLV(BE_NEW_BSS_TO_OLD_BSS_INF, GSM_A_PDU_TYPE_BSSMAP, BE_NEW_BSS_TO_OLD_BSS_INF, NULL);
    /* Talker Priority  3.2.2.89    MSC-BSS O (note 3)  2 */
    ELEM_OPT_TV(BE_TALKER_PRI, GSM_A_PDU_TYPE_BSSMAP, BE_TALKER_PRI, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 *  [2] 3.2.1.12 HANDOVER COMPLETE
 */
static void
bssmap_ho_complete(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* RR Cause 3.2.2.22    BSS-MSC O   2 */
    ELEM_OPT_TV(BE_RR_CAUSE, GSM_A_PDU_TYPE_BSSMAP, BE_RR_CAUSE, NULL);
    /* Talker Priority  3.2.2.89    BSS-MSC O (note 1)  2 */
    ELEM_OPT_TV(BE_TALKER_PRI, GSM_A_PDU_TYPE_BSSMAP, BE_TALKER_PRI, NULL);
    /* Speech Codec (Chosen)    3.2.2.nn    BSS-MSC O (note 2)  3-5 */
    ELEM_OPT_TLV(BE_SPEECH_CODEC, GSM_A_PDU_TYPE_BSSMAP, BE_SPEECH_CODEC, "(Chosen)");
    /* Chosen Encryption Algorithm  3.2.2.44    BSS-MSC O (note 4)  2 */
    ELEM_OPT_TV(BE_CHOSEN_ENC_ALG, GSM_A_PDU_TYPE_BSSMAP, BE_CHOSEN_ENC_ALG, NULL);
    /* Chosen Channel   3.2.2.33    BSS-MSC O (note 5)  2 */
    ELEM_OPT_TV(BE_CHOSEN_CHAN, GSM_A_PDU_TYPE_BSSMAP, BE_CHOSEN_CHAN, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 * 3.2.1.13 HANDOVER SUCCEEDED
 */
static void
bssmap_ho_succ(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Talker Priority  3.2.2.89    MSC-BSS O (note 1)  2 */
    ELEM_OPT_TV(BE_TALKER_PRI, GSM_A_PDU_TYPE_BSSMAP, BE_TALKER_PRI, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 *  [2] 3.2.1.14 HANDOVER CANDIDATE ENQUIRE
 */
static void
bssmap_ho_cand_enq(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Number Of Mss    3.2.2.8     MSC-BSS     M   2 */
    ELEM_MAND_TV(BE_NUM_MS, GSM_A_PDU_TYPE_BSSMAP, BE_NUM_MS, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    /* Cell Identifier List 3.2.2.27    MSC-BSS     M   2n+3 to 7n+3 */
    ELEM_MAND_TLV(BE_CELL_ID_LIST, GSM_A_PDU_TYPE_BSSMAP, BE_CELL_ID_LIST, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    /* Cell Identifier  3.2.2.17    MSC-BSS     M   3-10 */
    ELEM_MAND_TLV(BE_CELL_ID, GSM_A_PDU_TYPE_BSSMAP, BE_CELL_ID, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 *  [2] 3.2.1.15 HANDOVER CANDIDATE RESPONSE
 */
static void
bssmap_ho_cand_resp(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Number Of Mss    3.2.2.8     BSS-MSC     M   2 */
    ELEM_MAND_TV(BE_NUM_MS, GSM_A_PDU_TYPE_BSSMAP, BE_NUM_MS, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    /* Cell Identifier  3.2.2.17    BSS-MSC     M   3-10 */
    ELEM_MAND_TLV(BE_CELL_ID, GSM_A_PDU_TYPE_BSSMAP, BE_CELL_ID, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 *  [2] 3.2.1.16 HANDOVER FAILURE
 */
static void
bssmap_ho_failure(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Cause    3.2.2.5     BSS-MSC     M   3-4 */
    ELEM_MAND_TLV(BE_CAUSE, GSM_A_PDU_TYPE_BSSMAP, BE_CAUSE, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* RR Cause 3.2.2.22    BSS-MSC     O   2 */
    ELEM_OPT_TV(BE_RR_CAUSE, GSM_A_PDU_TYPE_BSSMAP, BE_RR_CAUSE, NULL);
    /* Circuit Pool 3.2.2.45    BSS-MSC     O (note 1)  2 */
    ELEM_OPT_TV(BE_CCT_POOL, GSM_A_PDU_TYPE_BSSMAP, BE_CCT_POOL, NULL);
    /* Circuit Pool List    3.2.2.46    BSS-MSC     O (note 2)  V */
    ELEM_OPT_TLV(BE_CCT_POOL_LIST, GSM_A_PDU_TYPE_BSSMAP, BE_CCT_POOL_LIST, NULL);
    /* GERAN Classmark  3.2.2.78    BSS-MSC O (note 3)  V */
    ELEM_OPT_TLV(BE_GERAN_CLS_M, GSM_A_PDU_TYPE_BSSMAP, BE_GERAN_CLS_M, NULL);
    /* New BSS to Old BSS Information   3.2.2.80    BSS-MSC     O (note 4)  2-n */
    ELEM_OPT_TLV(BE_NEW_BSS_TO_OLD_BSS_INF, GSM_A_PDU_TYPE_BSSMAP, BE_NEW_BSS_TO_OLD_BSS_INF, NULL);
    /* Inter-System Information 3.2.2.81    BSS-MSC     O (note 5)  2-n */
    ELEM_OPT_TLV(BE_INTER_SYS_INF, GSM_A_PDU_TYPE_BSSMAP, BE_INTER_SYS_INF, NULL);
    /* Talker Priority  3.2.2.89    BSS-MSC O (note 6)  2 */
    ELEM_OPT_TV(BE_TALKER_PRI, GSM_A_PDU_TYPE_BSSMAP, BE_TALKER_PRI, NULL);
    /* Codec List (BSS Supported)   3.2.2.103   BSS-MSC O (note 7)  3-n */
    ELEM_OPT_TLV(BE_SPEECH_CODEC_LST, GSM_A_PDU_TYPE_BSSMAP, BE_SPEECH_CODEC_LST, "(BSS Supported)");

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 * 3.2.1.17 RESOURCE REQUEST
 */
static void
bssmap_res_req(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Periodicity  3.2.2.12    MSC-BSS     M   2   */
    ELEM_MAND_TV(BE_PERIODICITY, GSM_A_PDU_TYPE_BSSMAP, BE_PERIODICITY, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Resource Indication Method   3.2.2.29    MSC-BSS     M   2  */
    ELEM_MAND_TV(BE_RES_IND_METHOD, GSM_A_PDU_TYPE_BSSMAP, BE_RES_IND_METHOD, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Cell Identifier  3.2.2.17    MSC-BSS     M   3-10  */
    ELEM_MAND_TLV(BE_CELL_ID, GSM_A_PDU_TYPE_BSSMAP, BE_CELL_ID, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Extended Resource Indicator  3.2.2.13    MSC-BSS     O   2  */
    ELEM_MAND_TV(BE_EXT_RES_IND, GSM_A_PDU_TYPE_BSSMAP, BE_EXT_RES_IND, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 * 3.2.1.18 RESOURCE INDICATION
 */
static void
bssmap_res_ind(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Resource Indication Method   3.2.2.29    BSS-MSC M   2 */
    ELEM_MAND_TV(BE_RES_IND_METHOD, GSM_A_PDU_TYPE_BSSMAP, BE_RES_IND_METHOD, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Resource Available   3.2.2.4 BSS-MSC O (note 1)  21 */
    ELEM_MAND_TV(BE_RES_AVAIL, GSM_A_PDU_TYPE_BSSMAP, BE_RES_AVAIL, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Cell Identifier  3.2.2.17    BSS-MSC M   3-10  */
    ELEM_MAND_TLV(BE_CELL_ID, GSM_A_PDU_TYPE_BSSMAP, BE_CELL_ID, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Total Resource Accessible    3.2.2.14    BSS-MSC O (note 2)  5 */
    ELEM_MAND_TV(BE_TOT_RES_ACC, GSM_A_PDU_TYPE_BSSMAP, BE_TOT_RES_ACC, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}
/*
 *  [2] 3.2.1.19 PAGING
 */
static void
bssmap_paging(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* IMSI 3.2.2.6 MSC-BSS M   3-10 */
    ELEM_MAND_TLV(BE_IMSI, GSM_A_PDU_TYPE_BSSMAP, BE_IMSI, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* TMSI 3.2.2.7 MSC-BSS O (note 1)  6 */
    ELEM_OPT_TLV(BE_TMSI, GSM_A_PDU_TYPE_BSSMAP, BE_TMSI, NULL);
    /* Cell Identifier List 3.2.2.27    MSC-BSS M   3 to 3+7n */
    ELEM_MAND_TLV(BE_CELL_ID_LIST, GSM_A_PDU_TYPE_BSSMAP, BE_CELL_ID_LIST, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Channel Needed   3.2.2.36    MSC-BSS O (note 2)  2 */
    ELEM_OPT_TV(BE_CHAN_NEEDED, GSM_A_PDU_TYPE_BSSMAP, BE_CHAN_NEEDED, NULL);
    /* eMLPP Priority   3.2.2.56    MSC-BSS O (note 3)  2 */
    ELEM_OPT_TV(BE_EMLPP_PRIO, GSM_A_PDU_TYPE_BSSMAP, BE_EMLPP_PRIO, NULL);
    /* Paging Information   3.2.2.85    MSC-BSS O   2 */
    ELEM_OPT_TV(BE_PAGING_INF, GSM_A_PDU_TYPE_BSSMAP, BE_PAGING_INF, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 *  [2] 3.2.1.20 CLEAR REQUEST
 */
static void
bssmap_clear_req(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Cause    3.2.2.5 BSS-MSC M   3-4 */
    ELEM_MAND_TLV(BE_CAUSE, GSM_A_PDU_TYPE_BSSMAP, BE_CAUSE, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 *  [2] 3.2.1.21 CLEAR COMMAND
 */
static void
bssmap_clear_cmd(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Layer 3 Header Information   3.2.2.9 MSC-BSS O (note)    4 */
    ELEM_OPT_TLV(BE_L3_HEADER_INFO, GSM_A_PDU_TYPE_BSSMAP, BE_L3_HEADER_INFO, NULL);
    /* Cause    3.2.2.5 MSC-BSS M   3-4 */
    ELEM_MAND_TLV(BE_CAUSE, GSM_A_PDU_TYPE_BSSMAP, BE_CAUSE, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* CSFB Indication 3.2.2.121 MSC-BSS O 1 */
    ELEM_OPT_T(BE_CSFB_IND, GSM_A_PDU_TYPE_BSSMAP, BE_CSFB_IND, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}
/*
 * 3.2.1.22 CLEAR COMPLETE
 * No data
 */

/*
 *  [2] 3.2.1.23 RESET
 */
void
bssmap_reset(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Cause    3.2.2.5 Both    M   3-4 */
    ELEM_MAND_TLV(BE_CAUSE, GSM_A_PDU_TYPE_BSSMAP, BE_CAUSE, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* A-Interface Selector for RESET  3.2.2.107   Both O 2 */
    ELEM_OPT_TV(BE_A_ITF_SEL_FOR_RESET, GSM_A_PDU_TYPE_BSSMAP, BE_A_ITF_SEL_FOR_RESET, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 * 3.2.1.24 RESET ACKNOWLEDGE
 */
static void
bssmap_reset_ack(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* A-Interface Selector for RESET  3.2.2.107   Both O 2 */
    ELEM_OPT_TV(BE_A_ITF_SEL_FOR_RESET, GSM_A_PDU_TYPE_BSSMAP, BE_A_ITF_SEL_FOR_RESET, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 *  [2] 3.2.1.25 HANDOVER PERFORMED
 */
static void
bssmap_ho_performed(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Cause    3.2.2.5 BSS-MSC M   3-4 */
    ELEM_MAND_TLV(BE_CAUSE, GSM_A_PDU_TYPE_BSSMAP, BE_CAUSE, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Cell Identifier  3.2.2.17    BSS-MSC M (note 5)  3-10 */
    ELEM_MAND_TLV(BE_CELL_ID, GSM_A_PDU_TYPE_BSSMAP, BE_CELL_ID, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Chosen Channel   3.2.2.33    BSS-MSC O (note 1)  2 */
    ELEM_OPT_TV(BE_CHOSEN_CHAN, GSM_A_PDU_TYPE_BSSMAP, BE_CHOSEN_CHAN, NULL);
    /* Chosen Encryption Algorithm  3.2.2.44    BSS-MSC O (note 2)  2 */
    ELEM_OPT_TV(BE_CHOSEN_ENC_ALG, GSM_A_PDU_TYPE_BSSMAP, BE_CHOSEN_ENC_ALG, NULL);
    /* Speech Version (Chosen)  3.2.2.51    BSS-MSC O (note 3)  2 */
    ELEM_OPT_TV(BE_SPEECH_VER, GSM_A_PDU_TYPE_BSSMAP, BE_SPEECH_VER, " (Chosen)");
    /* LSA Identifier   3.2.2.15    BSS-MSC O (note 4)  5 */
    ELEM_OPT_TLV(BE_LSA_ID, GSM_A_PDU_TYPE_BSSMAP, BE_LSA_ID, NULL);
    /* Talker Priority  3.2.2.89    BSS-MSC O (note 6)  2 */
    ELEM_OPT_TV(BE_TALKER_PRI, GSM_A_PDU_TYPE_BSSMAP, BE_TALKER_PRI, NULL);
    /* Codec List (BSS Supported) (serving cell)    3.2.2.103   BSS-MSC O (note 7)  3-n */
    ELEM_OPT_TLV(BE_SPEECH_CODEC_LST, GSM_A_PDU_TYPE_BSSMAP, BE_SPEECH_CODEC_LST, "(BSS Supported)");
    /* Speech Codec (Chosen)    3.2.2.104   BSS-MSC O (note 8)  3-5 */
    ELEM_OPT_TLV(BE_SPEECH_CODEC, GSM_A_PDU_TYPE_BSSMAP, BE_SPEECH_CODEC, "(Chosen)");

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 *  [2] 3.2.1.26 OVERLOAD
 */
static void
bssmap_overload(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Cause    3.2.2.5 Both    M   3-4 */
    ELEM_MAND_TLV(BE_CAUSE, GSM_A_PDU_TYPE_BSSMAP, BE_CAUSE, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Cell Identifier  3.2.2.17    BSS-MSC     O   3-10 */
    ELEM_OPT_TLV(BE_CELL_ID, GSM_A_PDU_TYPE_BSSMAP, BE_CELL_ID, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}
/*
 * 3.2.1.27 MSC INVOKE TRACE
 */
static void
bssmap_msc_invoke_trace(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Trace Type  3.2.2.37    MSC-BSS     M   2 */
    ELEM_MAND_TV(BE_TRACE_TYPE, GSM_A_PDU_TYPE_BSSMAP, BE_TRACE_TYPE, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Triggerid    3.2.2.38    MSC-BSS     O   3-22 */
    ELEM_OPT_TLV(BE_TRIGGERID, GSM_A_PDU_TYPE_BSSMAP, BE_TRIGGERID, NULL);
    /* Trace Reference  3.2.2.39    MSC-BSS     M   3 */
    ELEM_MAND_TV(BE_TRACE_REF, GSM_A_PDU_TYPE_BSSMAP, BE_TRACE_REF, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Transactionid    3.2.2.40    MSC-BSS     O   4 */
    ELEM_OPT_TLV(BE_TRANSID, GSM_A_PDU_TYPE_BSSMAP, BE_TRANSID, NULL);
    /* Mobile Identity  3.2.2.41    MSC-BSS     O   3-10 */
    ELEM_OPT_TLV(BE_MID, GSM_A_PDU_TYPE_BSSMAP, BE_MID, NULL);
    /* OMCId    3.2.2.42    MSC-BSS     O   3-22 */
    ELEM_OPT_TLV(BE_OMCID, GSM_A_PDU_TYPE_BSSMAP, BE_OMCID, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 * 3.2.1.28 BSS INVOKE TRACE
 */
static void
bssmap_bss_invoke_trace(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Trace Type   3.2.2.37    Both    M   2 */
    ELEM_MAND_TV(BE_TRACE_TYPE, GSM_A_PDU_TYPE_BSSMAP, BE_TRACE_TYPE, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Forward Indicator    3.2.2.43    Both    O   2 */
    ELEM_OPT_TV(BE_FOR_IND, GSM_A_PDU_TYPE_BSSMAP, BE_FOR_IND, NULL);
    /* Triggerid    3.2.2.38    Both    O   3-22 */
    ELEM_OPT_TLV(BE_TRIGGERID, GSM_A_PDU_TYPE_BSSMAP, BE_TRIGGERID, NULL);
    /* Trace Reference  3.2.2.39    Both    M   3 */
    ELEM_MAND_TV(BE_TRACE_REF, GSM_A_PDU_TYPE_BSSMAP, BE_TRACE_REF, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* TransactionId    3.2.2.40    Both    O   4 */
    ELEM_OPT_TLV(BE_TRANSID, GSM_A_PDU_TYPE_BSSMAP, BE_TRANSID, NULL);
    /* OMCId    3.2.2.42    Both    O   3-22 */
    ELEM_OPT_TLV(BE_OMCID, GSM_A_PDU_TYPE_BSSMAP, BE_OMCID, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 *  [2] 3.2.1.29 CLASSMARK UPDATE
 */
static void
bssmap_cm_upd(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Classmark Information Type 2 3.2.2.19    Both    M   4-5 */
    ELEM_MAND_TLV(BE_CM_INFO_2, GSM_A_PDU_TYPE_BSSMAP, BE_CM_INFO_2, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Classmark Information Type 3 3.2.2.20    Both    O (note 1)  3-34 */
    ELEM_OPT_TLV(BE_CM_INFO_3, GSM_A_PDU_TYPE_BSSMAP, BE_CM_INFO_3, NULL);
    /* Talker Priority  3.2.2.89    Both    O (note 2)  2 */
    ELEM_OPT_TV(BE_TALKER_PRI, GSM_A_PDU_TYPE_BSSMAP, BE_TALKER_PRI, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 *  [2] 3.2.1.30 CIPHER MODE COMMAND
 */
static void
bssmap_ciph_mode_cmd(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Layer 3 Header Information   3.2.2.9 MSC-BSS O (note)    4 */
    ELEM_OPT_TLV(BE_L3_HEADER_INFO, GSM_A_PDU_TYPE_BSSMAP, BE_L3_HEADER_INFO, NULL);
    /* Encryption Information   3.2.2.10    MSC-BSS M   3-n */
    ELEM_MAND_TLV(BE_ENC_INFO, GSM_A_PDU_TYPE_BSSMAP, BE_ENC_INFO, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Cipher Response Mode 3.2.2.34    MSC-BSS O   2 */
    ELEM_OPT_TV(BE_CIPH_RESP_MODE, GSM_A_PDU_TYPE_BSSMAP, BE_CIPH_RESP_MODE, NULL);
    /* Kc128  3.2.2.109   MSC-BSS C (note 2) 17 */
    ELEM_OPT_TV(BE_KC128, GSM_A_PDU_TYPE_BSSMAP, BE_KC128, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 *  [2] 3.2.1.31 CIPHER MODE COMPLETE
 */
static void
bssmap_ciph_mode_complete(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Layer 3 Message Contents 3.2.2.35    BSS-MSC O   2-n  */
    ELEM_OPT_TLV(BE_L3_MSG, GSM_A_PDU_TYPE_BSSMAP, BE_L3_MSG, NULL);
    /* Chosen Encryption Algorithm  3.2.2.44    BSS-MSC O (note)    2 */
    ELEM_OPT_TV(BE_CHOSEN_ENC_ALG, GSM_A_PDU_TYPE_BSSMAP, BE_CHOSEN_ENC_ALG, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 * [2] 3.2.1.32 COMPLETE LAYER 3 INFORMATION
 * Updated to 3GPP TS 48.008 version 11.5.0 Release 11
 */
static void
bssmap_cl3_info(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 consumed;
    guint32 curr_offset;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Cell Identifier  3.2.2.17    BSS-MSC     M   3-10 */
    ELEM_MAND_TLV(BE_CELL_ID, GSM_A_PDU_TYPE_BSSMAP, BE_CELL_ID, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Layer 3 Information  3.2.2.24    BSS-MSC     M   3-n  */
    ELEM_MAND_TLV(BE_L3_INFO, GSM_A_PDU_TYPE_BSSMAP, BE_L3_INFO, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Chosen Channel   3.2.2.33    BSS-MSC     O (note 1)  2 */
    ELEM_OPT_TV(BE_CHOSEN_CHAN, GSM_A_PDU_TYPE_BSSMAP, BE_CHOSEN_CHAN, NULL);
    /* LSA Identifier List  3.2.2.16    BSS-MSC O (note 2)  3+3n */
    ELEM_OPT_TLV(BE_LSA_ID_LIST, GSM_A_PDU_TYPE_BSSMAP, BE_LSA_ID_LIST, NULL);
    /* APDU 3.2.2.68    BSS-MSC O (note 3)  3-n */
    ELEM_OPT_TLV_E(BE_APDU, GSM_A_PDU_TYPE_BSSMAP, BE_APDU, NULL);
    /* Codec List (BSS Supported)   3.2.2.103   BSS-MSC O (note 4)  3-n */
    ELEM_OPT_TLV(BE_SPEECH_CODEC_LST, GSM_A_PDU_TYPE_BSSMAP, BE_SPEECH_CODEC_LST, "(BSS Supported)");
    /* Redirect Attempt Flag 3.2.2.111 BSS-MSC O (note 5) 1 */
    ELEM_OPT_T(BE_REDIR_ATT_FLG, GSM_A_PDU_TYPE_BSSMAP, BE_REDIR_ATT_FLG, NULL);
    /* Send Sequence Number 3.2.2.113 BSS-MSC O (note 6) 2 */
    ELEM_OPT_TV(BE_SEND_SEQN, GSM_A_PDU_TYPE_BSSMAP, BE_SEND_SEQN, NULL);
    /* IMSI 3.2.2.6 BSS-MSC O (note 7) 3-10 */
    ELEM_OPT_TLV(BE_IMSI, GSM_A_PDU_TYPE_BSSMAP, BE_IMSI, NULL);
    /* Selected PLMN ID 3.2.2.126 BSS-MSC O (note 8) 4 */
    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}
/*
 * 3.2.1.33 QUEUEING INDICATION
 * No data
 */

/*
 * [2] 3.2.1.34 SAPI "n" REJECT
 */
static void
bssmap_sapi_rej(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 consumed;
    guint32 curr_offset;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* DLCI 3.2.2.25    BSS-MSC M   2 */
    ELEM_MAND_TV(BE_DLCI, GSM_A_PDU_TYPE_BSSMAP, BE_DLCI, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Cause    3.2.2.5 BSS-MSC M   3-4 */
    ELEM_MAND_TLV(BE_CAUSE, GSM_A_PDU_TYPE_BSSMAP, BE_CAUSE, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}
/* 3.2.1.35 (void)
 * 3.2.1.36 (void)
 */
/*
 *  [2] 3.2.1.37 HANDOVER REQUIRED REJECT
 */
static void
bssmap_ho_reqd_rej(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Cause    3.2.2.5 MSC-BSS M   3-4  */
    ELEM_MAND_TLV(BE_CAUSE, GSM_A_PDU_TYPE_BSSMAP, BE_CAUSE, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* New BSS to Old BSS Information   3.2.2.78    MSC-BSS     O (note 1)  2-n */
    ELEM_OPT_TLV(BE_NEW_BSS_TO_OLD_BSS_INF, GSM_A_PDU_TYPE_BSSMAP, BE_NEW_BSS_TO_OLD_BSS_INF, NULL);
    /* Talker Priority  3.2.2.89    MSC-BSS O (note 2)  2 */
    ELEM_OPT_TV(BE_TALKER_PRI, GSM_A_PDU_TYPE_BSSMAP, BE_TALKER_PRI, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 *  [2] 3.2.1.38 RESET CIRCUIT
 */
static void
bssmap_reset_cct(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Circuit Identity Code    3.2.2.2 Both    M   3 */
    ELEM_MAND_TV(BE_CIC, GSM_A_PDU_TYPE_BSSMAP, BE_CIC, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Cause    3.2.2.5 Both    M   3-4 */
    ELEM_MAND_TLV(BE_CAUSE, GSM_A_PDU_TYPE_BSSMAP, BE_CAUSE, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 *  [2] 3.2.1.39 RESET CIRCUIT ACKNOWLEDGE
 */
static void
bssmap_reset_cct_ack(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Circuit Identity 3.2.2.2 Both    M   3 */
    ELEM_MAND_TV(BE_CIC, GSM_A_PDU_TYPE_BSSMAP, BE_CIC, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 * 3.2.1.40 HANDOVER DETECT
 */
static void
bssmap_ho_det(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Talker Priority  3.2.2.89    BSS-MSC O (note 1)  2 */
    ELEM_OPT_TV(BE_TALKER_PRI, GSM_A_PDU_TYPE_BSSMAP, BE_TALKER_PRI, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}
/*
 *  [2] 3.2.1.41 CIRCUIT GROUP BLOCK
 */
static void
bssmap_cct_group_block(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Cause    3.2.2.5 Both    M   3-4 */
    ELEM_MAND_TLV(BE_CAUSE, GSM_A_PDU_TYPE_BSSMAP, BE_CAUSE, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Circuit Identity Code    3.2.2.2 Both    M   3 */
    ELEM_MAND_TV(BE_CIC, GSM_A_PDU_TYPE_BSSMAP, BE_CIC, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Circuit Identity Code List   3.2.2.31    Both    M   4-35 */
    ELEM_MAND_TLV(BE_CIC_LIST, GSM_A_PDU_TYPE_BSSMAP, BE_CIC_LIST, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 *  [2] 3.2.1.42 CIRCUIT GROUP BLOCKING ACKNOWLEDGE
 */
static void
bssmap_cct_group_block_ack(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Circuit Identity Code    3.2.2.2 Both    M   3 */
    ELEM_MAND_TV(BE_CIC, GSM_A_PDU_TYPE_BSSMAP, BE_CIC, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Circuit Identity Code List   3.2.2.31    Both    M   4-35 */
    ELEM_MAND_TLV(BE_CIC_LIST, GSM_A_PDU_TYPE_BSSMAP, BE_CIC_LIST, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 *  [2] 3.2.1.43 CIRCUIT GROUP UNBLOCK
 */
static void
bssmap_cct_group_unblock(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Circuit Identity Code    3.2.2.2 Both    M   3 */
    ELEM_MAND_TV(BE_CIC, GSM_A_PDU_TYPE_BSSMAP, BE_CIC, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Circuit Identity Code List   3.2.2.31    Both    M   4-35 */
    ELEM_MAND_TLV(BE_CIC_LIST, GSM_A_PDU_TYPE_BSSMAP, BE_CIC_LIST, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 *  [2] 3.2.1.44 CIRCUIT GROUP UNBLOCKING ACKNOWLEDGE
 */
static void
bssmap_cct_group_unblock_ack(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Circuit Identity Code    3.2.2.2 Both    M   3 */
    ELEM_MAND_TV(BE_CIC, GSM_A_PDU_TYPE_BSSMAP, BE_CIC, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Circuit Identity Code List   3.2.2.31    Both    M   4-35 */
    ELEM_MAND_TLV(BE_CIC_LIST, GSM_A_PDU_TYPE_BSSMAP, BE_CIC_LIST, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 *  [2] 3.2.1.45 CONFUSION
 */
static void
bssmap_confusion(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Cause    3.2.2.5 Both    M   3-4 */
    ELEM_MAND_TLV(BE_CAUSE, GSM_A_PDU_TYPE_BSSMAP, BE_CAUSE, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Diagnostics  3.2.2.32    Both    M   4-n */
    ELEM_MAND_TLV(BE_DIAG, GSM_A_PDU_TYPE_BSSMAP, BE_DIAG, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}
/*
 * 3.2.1.46 CLASSMARK REQUEST
 */
static void
bssmap_cls_m_req(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Talker Priority  3.2.2.89    MSC-BSS O (note 1)  2 */
    ELEM_OPT_TV(BE_TALKER_PRI, GSM_A_PDU_TYPE_BSSMAP, BE_TALKER_PRI, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}
/*
 *  [2] 3.2.1.47 UNEQUIPPED CIRCUIT
 */
static void
bssmap_unequipped_cct(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Circuit Identity Code    3.2.2.2     Both    M   3 */
    ELEM_MAND_TV(BE_CIC, GSM_A_PDU_TYPE_BSSMAP, BE_CIC, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Circuit Identity Code List   3.2.2.31    Both    O   4-35 */
    ELEM_OPT_TLV(BE_CIC_LIST, GSM_A_PDU_TYPE_BSSMAP, BE_CIC_LIST, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 *  [2] 3.2.1.48 CIPHER MODE REJECT
 */
static void
bssmap_ciph_mode_rej(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Cause    3.2.2.5 BSS-MSC M   3-4 */
    ELEM_MAND_TLV(BE_CAUSE, GSM_A_PDU_TYPE_BSSMAP, BE_CAUSE, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 *  [2] 3.2.1.49 LOAD INDICATION
 */
static void
bssmap_load_ind(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Time Indication  3.2.2.47    Both    M   2 */
    ELEM_MAND_TV(BE_TIME_IND, GSM_A_PDU_TYPE_BSSMAP, BE_TIME_IND, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Cell Identifier  3.2.2.17    Both    M   3-10 */
    ELEM_MAND_TLV(BE_CELL_ID, GSM_A_PDU_TYPE_BSSMAP, BE_CELL_ID, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Cell Identifier List (Target)    3.2.2.27    Both    M   3 to 3+7n */
    ELEM_MAND_TLV(BE_CELL_ID_LIST, GSM_A_PDU_TYPE_BSSMAP, BE_CELL_ID_LIST, " (Target)", ei_gsm_a_bssmap_missing_mandatory_element);
    /* Resource Situation   3.2.2.48    Both    O (note 1)  4-N */
    ELEM_OPT_TLV(BE_RES_SIT, GSM_A_PDU_TYPE_BSSMAP, BE_RES_SIT, NULL);
    /* Cause    3.2.2.5 Both    O (note 2)  4-5 */
    ELEM_OPT_TLV(BE_CAUSE, GSM_A_PDU_TYPE_BSSMAP, BE_CAUSE, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}
/*
 * 3.2.1.50 VGCS/VBS SETUP
 */
 static void
 bssmap_vgcs_vbs_setup(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Group Call Reference 3.2.2.55    MSC-BSS M   7 */
    ELEM_MAND_TLV(BE_GROUP_CALL_REF, GSM_A_PDU_TYPE_BSSMAP, BE_GROUP_CALL_REF, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Priority 3.2.2.18    MSC-BSS O   3 */
    ELEM_OPT_TLV(BE_PRIO, GSM_A_PDU_TYPE_BSSMAP, BE_PRIO, NULL);
    /* VGCS Feature Flags   3.2.2.88    MSC-BSS O   3 */
    ELEM_OPT_TLV(BE_VGCS_FEAT_FLG, GSM_A_PDU_TYPE_BSSMAP, BE_VGCS_FEAT_FLG, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 * 3.2.1.51 VGCS/VBS SETUP ACK
 */
 static void
bssmap_vgcs_vbs_setup_ack(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* VGCS Feature Flags   3.2.2.88    BSS-MSC O(note 1)   3 */
    ELEM_OPT_TLV(BE_VGCS_FEAT_FLG, GSM_A_PDU_TYPE_BSSMAP, BE_VGCS_FEAT_FLG, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

 /*
 * 3.2.1.52 VGCS/VBS SETUP REFUSE
 */
 static void
bssmap_vgcs_vbs_setup_refuse(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Cause    3.2.2.5 BSS-MSC M   3-4 */
    ELEM_MAND_TLV(BE_CAUSE, GSM_A_PDU_TYPE_BSSMAP, BE_CAUSE, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 * 3.2.1.53 VGCS/VBS ASSIGNMENT REQUEST
 */
static void
bssmap_vgcs_vbs_ass_req(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Channel Type 3.2.2.11    MSC-BSS M (note 2)  5-13 */
    ELEM_MAND_TLV(BE_CHAN_TYPE, GSM_A_PDU_TYPE_BSSMAP, BE_CHAN_TYPE, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Assignment Requirement   3.2.2.52    MSC-BSS M   2 */
    ELEM_MAND_TV(BE_ASS_REQ, GSM_A_PDU_TYPE_BSSMAP, BE_ASS_REQ, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Cell Identifier  3.2.2.17    MSC-BSS M   3-10 */
    ELEM_MAND_TLV(BE_CELL_ID, GSM_A_PDU_TYPE_BSSMAP, BE_CELL_ID, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Group Call Reference 3.2.2.55    MSC-BSS M   7 */
    ELEM_MAND_TLV(BE_GROUP_CALL_REF, GSM_A_PDU_TYPE_BSSMAP, BE_GROUP_CALL_REF, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Priority 3.2.2.18    MSC-BSS O   3 */
    ELEM_OPT_TLV(BE_PRIO, GSM_A_PDU_TYPE_BSSMAP, BE_PRIO, NULL);
    /* Circuit Identity Code    3.2.2.2 MSC-BSS O (note  4, 5)  3 */
    ELEM_OPT_TV(BE_CIC, GSM_A_PDU_TYPE_BSSMAP, BE_CIC, NULL);
    /* Downlink DTX Flag    3.2.2.26    MSC-BSS O (note 2, 4)   2 */
    ELEM_OPT_TV(BE_DOWN_DTX_FLAG, GSM_A_PDU_TYPE_BSSMAP, BE_DOWN_DTX_FLAG, NULL);
    /* Encryption Information   3.2.2.10    MSC-BSS O   3-n */
    ELEM_OPT_TLV(BE_ENC_INFO, GSM_A_PDU_TYPE_BSSMAP, BE_ENC_INFO, NULL);
    /* VSTK_RAND    3.2.2.83    MSC-BSS O (note 1)  7 */
    ELEM_OPT_TLV(BE_VSTK_RAND_INF, GSM_A_PDU_TYPE_BSSMAP, BE_VSTK_RAND_INF, NULL);
    /* VSTK 3.2.2.84    MSC-BSS O (note 1)  18 */
    ELEM_OPT_TLV(BE_VSTK_INF, GSM_A_PDU_TYPE_BSSMAP, BE_VSTK_INF, NULL);
    /* Cell Identifier List Segment 3.2.2.27a   MSC-BSS O (note 3)  4-? */
    ELEM_OPT_TLV(BE_CELL_ID_LIST_SEG, GSM_A_PDU_TYPE_BSSMAP, BE_CELL_ID_LIST_SEG, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}
/*
 * 3.2.1.54 VGCS/VBS ASSIGNMENT RESULT
 */
static void
bssmap_vgcs_vbs_ass_res(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Channel Type 3.2.2.11    BSS-MSC M (note 3, 4)   5 */
    ELEM_MAND_TLV(BE_CHAN_TYPE, GSM_A_PDU_TYPE_BSSMAP, BE_CHAN_TYPE, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Cell Identifier  3.2.2.17    BSS-MSC M   3-10 */
    ELEM_MAND_TLV(BE_CELL_ID, GSM_A_PDU_TYPE_BSSMAP, BE_CELL_ID, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Chosen Channel   3.2.2.33    BSS-MSC O (note 2)  2 */
    ELEM_OPT_TV(BE_CHOSEN_CHAN, GSM_A_PDU_TYPE_BSSMAP, BE_CHOSEN_CHAN, NULL);
    /* Circuit Identity Code    3.2.2.2 BSS-MSC O (note 5)  3 */
    ELEM_OPT_TV(BE_CIC, GSM_A_PDU_TYPE_BSSMAP, BE_CIC, NULL);
    /* Circuit Pool 3.2.2.45    BSS-MSC O (note 1)  2 */
    ELEM_OPT_TV(BE_CCT_POOL, GSM_A_PDU_TYPE_BSSMAP, BE_CCT_POOL, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}
/*
 * 3.2.1.55 VGCS/VBS ASSIGNMENT FAILURE
 */
static void
bssmap_vgcs_vbs_ass_fail(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Cause    3.2.2.5 BSS-MSC M   3-4  */
    ELEM_OPT_TLV(BE_CAUSE, GSM_A_PDU_TYPE_BSSMAP, BE_CAUSE, NULL);
    /* Circuit Pool 3.2.2.45    BSS-MSC O (note 1)  2 */
    ELEM_OPT_TV(BE_CCT_POOL, GSM_A_PDU_TYPE_BSSMAP, BE_CCT_POOL, NULL);
    /* Circuit Pool List    3.2.2.46    BSS-MSC O (note 2)  V */
    ELEM_OPT_TLV(BE_CCT_POOL_LIST, GSM_A_PDU_TYPE_BSSMAP, BE_CCT_POOL_LIST, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}
/*
 * 3.2.1.56 VGCS/VBS QUEUING INDICATION
 * No data
 */
/*
 * 3.2.1.57 UPLINK REQUEST
 */
static void
bssmap_uplink_req(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Talker Priority  3.2.2.89    BSS-MSC O (note 1)  2 */
    ELEM_OPT_TV(BE_TALKER_PRI, GSM_A_PDU_TYPE_BSSMAP, BE_TALKER_PRI, NULL);
    /* Cell Identifier  3.2.2.17    BSS-MSC O (note 1)  3-10 */
    ELEM_MAND_TLV(BE_CELL_ID, GSM_A_PDU_TYPE_BSSMAP, BE_CELL_ID, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Layer 3 Information  3.2.2.24    BSS-MSC O (note 1,3)    3-n */
    ELEM_OPT_TLV(BE_L3_INFO, GSM_A_PDU_TYPE_BSSMAP, BE_L3_INFO, NULL);
    /* Mobile Identity  3.2.2.41    BSS-MSC O (note 1,2)    3-n */
    ELEM_OPT_TLV(BE_MID, GSM_A_PDU_TYPE_COMMON, DE_MID, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}
/*
 * 3.2.1.58 UPLINK REQUEST ACKNOWLEDGE
 */
static void
bssmap_uplink_req_ack(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Talker Priority  3.2.2.89    MSC-BSS O (note 1)  2 */
    ELEM_OPT_TV(BE_TALKER_PRI, GSM_A_PDU_TYPE_BSSMAP, BE_TALKER_PRI, NULL);
    /* Emergency set indication 3.2.2.90    MSC-BSS O (note 1)  1 */
    ELEM_OPT_T(BE_EMRG_SET_IND, GSM_A_PDU_TYPE_BSSMAP, BE_EMRG_SET_IND, NULL);
    /* Talker Identity  3.2.2.91    MSC-BSS O   3-20 */
    ELEM_MAND_TLV(BE_TALKER_ID, GSM_A_PDU_TYPE_BSSMAP, BE_TALKER_ID, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}
/*
 * 3.2.1.59 UPLINK REQUEST CONFIRMATION
 */
static void
bssmap_uplink_req_conf(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Cell Identifier  3.2.2.17    BSS-MSC M   3-10 */
    ELEM_MAND_TLV(BE_CELL_ID, GSM_A_PDU_TYPE_BSSMAP, BE_CELL_ID, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Talker Identity  3.2.2.91    BSS-MSC O   3-20 */
    ELEM_OPT_TLV(BE_TALKER_ID, GSM_A_PDU_TYPE_BSSMAP, BE_TALKER_ID, NULL);
    /* Layer 3 Information  3.2.2.24    BSS-MSC M   3-n */
    ELEM_MAND_TLV(BE_L3_INFO, GSM_A_PDU_TYPE_BSSMAP, BE_L3_INFO, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}
/*
 * 3.2.1.59a    UPLINK APPLICATION DATA
 */
static void
bssmap_uplink_app_data(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Cell Identifier  3.2.2.17    BSS-MSC M   3-10 */
    ELEM_MAND_TLV(BE_CELL_ID, GSM_A_PDU_TYPE_BSSMAP, BE_CELL_ID, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Layer 3 Information  3.2.2.24    BSS-MSC M   3-n */
    ELEM_MAND_TLV(BE_L3_INFO, GSM_A_PDU_TYPE_BSSMAP, BE_L3_INFO, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Application Data information 3.2.2.100   BSS-MSC M   3 */
    ELEM_MAND_TLV(BE_APP_DATA_INF, GSM_A_PDU_TYPE_BSSMAP, BE_APP_DATA_INF, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}
/*
 * 3.2.1.60 UPLINK RELEASE INDICATION
 */
static void
bssmap_uplink_rel_ind(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Cause    3.2.2.5 BSS-MSC M   3-4 */
    ELEM_MAND_TLV(BE_CAUSE, GSM_A_PDU_TYPE_BSSMAP, BE_CAUSE, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Talker Priority  3.2.2.89    BSS-MSC O (note 1)  2 */
    ELEM_OPT_TV(BE_TALKER_PRI, GSM_A_PDU_TYPE_BSSMAP, BE_TALKER_PRI, NULL);


    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}
/*
 * 3.2.1.61 UPLINK REJECT COMMAND
 */
static void
bssmap_uplink_rej_cmd(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Cause    3.2.2.5 MSC-BSS M   3-4 */
    ELEM_MAND_TLV(BE_CAUSE, GSM_A_PDU_TYPE_BSSMAP, BE_CAUSE, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Current Talker Priority  3.2.2.89    MSC-BSS O (note 1)  2 */
    ELEM_OPT_TV(BE_TALKER_PRI, GSM_A_PDU_TYPE_BSSMAP, BE_TALKER_PRI, "Current");
    /* Rejected Talker Priority 3.2.2.89    MSC-BSS O (note 1)  2 */
    ELEM_OPT_TV(BE_TALKER_PRI, GSM_A_PDU_TYPE_BSSMAP, BE_TALKER_PRI, "Rejected");
    /* Talker Identity  3.2.2.91    MSC-BSS O   3-20 */
    ELEM_OPT_TLV(BE_TALKER_ID, GSM_A_PDU_TYPE_BSSMAP, BE_TALKER_ID, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}
/*
 * 3.2.1.62 UPLINK RELEASE COMMAND
 */
static void
bssmap_uplink_rel_cmd(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Cause    3.2.2.5 MSC-BSS M   3-4 */
    ELEM_OPT_TLV(BE_CAUSE, GSM_A_PDU_TYPE_BSSMAP, BE_CAUSE, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 * 3.2.1.63 UPLINK SEIZED COMMAND
 */
static void
bssmap_uplink_seized_cmd(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Cause    3.2.2.5 MSC-BSS M   3-4 */
    ELEM_OPT_TLV(BE_CAUSE, GSM_A_PDU_TYPE_BSSMAP, BE_CAUSE, NULL);
    /* Talker Priority  3.2.2.89    MSC-BSS O (note 1)  2 */
    ELEM_OPT_TV(BE_TALKER_PRI, GSM_A_PDU_TYPE_BSSMAP, BE_TALKER_PRI, NULL);
    /* Emergency set indication 3.2.2.90    MSC-BSS O (note 1)  1 */
    ELEM_OPT_T(BE_EMRG_SET_IND, GSM_A_PDU_TYPE_BSSMAP, BE_EMRG_SET_IND, NULL);
    /* Talker Identity  3.2.2.91    MSC-BSS O   3-20 */
    ELEM_MAND_TLV(BE_TALKER_ID, GSM_A_PDU_TYPE_BSSMAP, BE_TALKER_ID, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}
/*
 * 3.2.1.64 SUSPEND
 */
static void
bssmap_sus(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* DLCI 3.2.2.25    BSS-MSC M   2 */
    ELEM_MAND_TV(BE_DLCI, GSM_A_PDU_TYPE_BSSMAP, BE_DLCI, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}
/*
 * 3.2.1.65 RESUME
 */
static void
bssmap_res(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* DLCI 3.2.2.25    BSS-MSC M   2 */
    ELEM_MAND_TV(BE_DLCI, GSM_A_PDU_TYPE_BSSMAP, BE_DLCI, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}
/*
 *  [2] 3.2.1.66 CHANGE CIRCUIT
 */
static void
bssmap_change_cct(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Cause    3.2.2.5 MSC-BSS M   3-4 */
    ELEM_MAND_TLV(BE_CAUSE, GSM_A_PDU_TYPE_BSSMAP, BE_CAUSE, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 *  [2] 3.2.1.67 CHANGE CIRCUIT ACKNOWLEDGE
 */
static void
bssmap_change_cct_ack(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Circuit identity 3.2.2.2 BSS-MSC M   3 */
    ELEM_MAND_TV(BE_CIC, GSM_A_PDU_TYPE_BSSMAP, BE_CIC, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 *  [2] 3.2.1.68 Common ID
 */
static void
bssmap_common_id(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* IMSI 3.2.2.6 MSC-BSS M   3-10 */
    ELEM_MAND_TLV(BE_IMSI, GSM_A_PDU_TYPE_BSSMAP, BE_IMSI, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* SNA Access Information   3.2.2.82    MSC-BSC O (note)    2+n */
    ELEM_OPT_TLV(BE_SNA_ACC_INF, GSM_A_PDU_TYPE_BSSMAP, BE_SNA_ACC_INF, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 *  [2] 3.2.1.69 LSA INFORMATION
 */
static void
bssmap_lsa_info(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* LSA Information  3.2.2.23    MSC-BSS M   3+4n */
    ELEM_MAND_TLV(BE_LSA_INFO, GSM_A_PDU_TYPE_BSSMAP, BE_LSA_INFO, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 *  [2] 3.2.1.70 (void)
 */
void
bssmap_conn_oriented(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    ELEM_MAND_TLV_E(BE_APDU, GSM_A_PDU_TYPE_BSSMAP, BE_APDU, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    ELEM_OPT_TLV(BE_SEG, GSM_A_PDU_TYPE_BSSMAP, BE_SEG, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 * 3.2.1.71 PERFORM LOCATION REQUEST
 */
static void
bssmap_perf_loc_req(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Location Type 3.2.2.63 M 3-n */
    ELEM_MAND_TLV(BE_LOC_TYPE, GSM_A_PDU_TYPE_BSSMAP, BE_LOC_TYPE, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Cell Identifier 3.2.2.17 O 5-10 */
    ELEM_MAND_TLV(BE_CELL_ID, GSM_A_PDU_TYPE_BSSMAP, BE_CELL_ID, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Classmark Information Type 3 3.2.2.20 O 3-14 */
    ELEM_OPT_TLV(BE_CM_INFO_3, GSM_A_PDU_TYPE_BSSMAP, BE_CM_INFO_3, NULL);
    /* LCS Client Type 3.2.2.67 C (note 3) 3-n */
    ELEM_OPT_TLV(BE_LCS_CLIENT, GSM_PDU_TYPE_BSSMAP_LE, DE_BMAPLE_LCS_CLIENT_TYPE, NULL);
    /* Chosen Channel 3.2.2.33 O 2 */
    ELEM_OPT_TV(BE_CHOSEN_CHAN, GSM_A_PDU_TYPE_BSSMAP, BE_CHOSEN_CHAN, NULL);
    /* LCS Priority 3.2.2.62 O 3-n */
    ELEM_OPT_TLV(BE_LCS_PRIO, GSM_A_PDU_TYPE_BSSMAP, BE_LCS_PRIO, NULL);
    /* LCS QoS 3.2.2.60 C (note 1) 3-n */
    ELEM_OPT_TLV(BE_LCS_QOS, GSM_PDU_TYPE_BSSMAP_LE, DE_BMAPLE_LCSQOS, NULL);
    /* GPS Assistance Data 3.2.2.70 C (note 2) 3-n */
    ELEM_OPT_TLV(BE_GPS_ASSIST_DATA, GSM_A_PDU_TYPE_BSSMAP, BE_GPS_ASSIST_DATA, NULL);
    /* APDU 3.2.2.68 O 3-n */
    ELEM_OPT_TLV_E(BE_APDU, GSM_A_PDU_TYPE_BSSMAP, BE_APDU, NULL);
    /* IMSI 3.2.2.6 O (note 4)  5-10 */
    ELEM_OPT_TLV(BE_IMSI, GSM_A_PDU_TYPE_BSSMAP, BE_IMSI, NULL);
    /* IMEI 3.2.2.86    O (note 4)  10 (use same decode as IMSI) */
    ELEM_OPT_TLV(BE_IMEI, GSM_A_PDU_TYPE_BSSMAP, BE_IMEI, NULL);
    /* GANSS Location Type  3.2.2.97    C   3 */
    ELEM_OPT_TLV(BE_GANSS_LOC_TYP, GSM_A_PDU_TYPE_BSSMAP, BE_GANSS_LOC_TYP, NULL);
    /* GANSS Assistance Data    3.2.2.95    C (note 5)  3-n */
    ELEM_OPT_TLV(BE_GANSS_ASS_DTA, GSM_A_PDU_TYPE_BSSMAP, BE_GANSS_ASS_DTA, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}
/*
 * 3.2.1.72 PERFORM LOCATION RESPONSE
 */
static void
bssmap_perf_loc_res(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Location Estimate 3.2.2.64 C (note 1) 3-n */
    ELEM_OPT_TLV(BE_LOC_EST, GSM_A_PDU_TYPE_BSSMAP, BE_LOC_EST, NULL);
    /* Positioning Data 3.2.2.65 O 3-n */
    ELEM_OPT_TLV(BE_POS_DATA, GSM_A_PDU_TYPE_BSSMAP, BE_POS_DATA, NULL);
    /* Deciphering Keys 3.2.2.71 C (note 2) 3-n */
    ELEM_OPT_TLV(BE_DECIPH_KEYS, GSM_PDU_TYPE_BSSMAP_LE, DE_BMAPLE_DECIPH_KEYS, NULL);
    /* LCS Cause 3.2.2.66 C (note 3) 3-n */
    ELEM_OPT_TLV(BE_LCS_CAUSE, GSM_PDU_TYPE_BSSMAP_LE, DE_BMAPLE_LCS_CAUSE, NULL);
    /* Velocity Estimate    3.2.2.87    O   3-n */
    ELEM_OPT_TLV(BE_VEL_EST, GSM_A_PDU_TYPE_BSSMAP, BE_VEL_EST, NULL);
    /* GANSS Positioning Data   3.2.2.96    O   3-n */
    ELEM_OPT_TLV(BE_GANSS_POS_DTA, GSM_A_PDU_TYPE_BSSMAP, BE_GANSS_POS_DTA, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}
/*
 * 3.2.1.73 PERFORM LOCATION ABORT
 */
void
bssmap_perf_loc_abort(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* LCS Cause 3.2.2.66 M 3-n */
    ELEM_OPT_TLV(BE_LCS_CAUSE, GSM_PDU_TYPE_BSSMAP_LE, DE_BMAPLE_LCS_CAUSE, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 * 3.2.1.74 CONNECTIONLESS INFORMATION
 *
Network Element Identity (source)   3.2.2.69    Both    M   3-n
Network Element Identity (target)   3.2.2.69    Both    M   3-n
APDU    3.2.2.68    Both    M   3-n
Segmentation    3.2,2,74    Both    C (note 1)  5
Return Error Request    3.2.2.72    Both    C (note 2)  3-n
Return Error Cause  3.2.2.73    Both    C (note 3)  3-n
*/

/*
 * 3.2.1.75 CHANNEL MODIFY REQUEST
 */

static void
bssmap_chan_mod_req(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Cause    3.2.2.5 BSS-MSC M   3-4 */
    ELEM_MAND_TLV(BE_CAUSE, GSM_A_PDU_TYPE_BSSMAP, BE_CAUSE, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}
/*
 * 3.2.1.76 EMERGENCY RESET INDICATION
 */
/*
Cell Identifier 3.2.2.17    BSS-MSC O   3-10
ELEM_MAND_TLV(BE_CELL_ID, GSM_A_PDU_TYPE_BSSMAP, BE_CELL_ID, NULL);
Layer 3 Information 3.2.2.24    BSS-MSC O (note 2)  3-n
    ELEM_OPT_TLV(BE_L3_INFO, GSM_A_PDU_TYPE_BSSMAP, BE_L3_INFO, NULL);
Mobile Identity 3.2.2.41    BSS-MSC O (note 1)  3-n
*/
/*
 * 3.2.1.77 EMERGENCY RESET COMMAND
 * No data
 */
/*
 * 3.2.1.78 VGCS ADDITIONAL INFORMATION
 */
static void
bssmap_vgcs_add_inf(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Talker Identity  3.2.2.91    MSC-BSS M   3-20 */
    ELEM_MAND_TLV(BE_TALKER_ID, GSM_A_PDU_TYPE_BSSMAP, BE_TALKER_ID, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}
/*
 * 3.2.1.79 VGCS/VBS AREA CELL INFO
 */
static void
bssmap_vgcs_vbs_area_cell_info(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Cell Identifier List Segment    3.2.2.27a   MSC-BSS M   4-? */
    ELEM_OPT_TLV(BE_CELL_ID_LIST_SEG, GSM_A_PDU_TYPE_BSSMAP, BE_CELL_ID_LIST_SEG, NULL);
    /* Assignment Requirement  3.2.2.52    MSC-BSS O   2 */
    ELEM_MAND_TV(BE_ASS_REQ, GSM_A_PDU_TYPE_BSSMAP, BE_ASS_REQ, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}
/*
 * 3.2.1.80 VGCS/VBS ASSIGNMENT STATUS
 */
static void
bssmap_vgcs_vbs_assign_status(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Cell Identifier List Segment for established cells  3.2.2.27b   BSS-MSC O (note 1)  3-? */
    ELEM_OPT_TLV(BE_CELL_ID_LST_SEG_F_EST_CELLS, GSM_A_PDU_TYPE_BSSMAP, BE_CELL_ID_LST_SEG_F_EST_CELLS, NULL);
    /* Cell Identifier List Segment for cells to be established    3.2.2.27c   BSS-MSC O (note 1)  3-? */
    ELEM_OPT_TLV(BE_CELL_ID_LST_SEG_F_CELL_TB_EST, GSM_A_PDU_TYPE_BSSMAP, BE_CELL_ID_LST_SEG_F_CELL_TB_EST, NULL);
    /* Cell Identifier List Segment for released cells - no user present   3.2.2.27e   BSS-MSC O (note 1)  3-? */
    ELEM_OPT_TLV(BE_CELL_ID_LST_SEG_F_REL_CELL, GSM_A_PDU_TYPE_BSSMAP, BE_CELL_ID_LST_SEG_F_REL_CELL, NULL);
    /* Cell Identifier List Segment for not established cells - no establishment possible  3.2.2.27f   BSS-MSC O (note 1)  3-? */
    ELEM_OPT_TLV(BE_CELL_ID_LST_SEG_F_NOT_EST_CELL, GSM_A_PDU_TYPE_BSSMAP, BE_CELL_ID_LST_SEG_F_NOT_EST_CELL, NULL);
    /* VGCS/VBS Cell Status    3.2.2.94    BSS-MSC O (note 2)  3 */
    ELEM_OPT_TLV(BE_VGS_VBS_CELL_STAT, GSM_A_PDU_TYPE_BSSMAP, BE_VGS_VBS_CELL_STAT, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}
/*
 * 3.2.1.81 VGCS SMS
 */
static void
bssmap_vgcs_sms(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* SMS to VGCS  3.2.2.92    MSC-BSS M   2-250 */
    ELEM_MAND_TLV(BE_SMS_TO_VGCS, GSM_A_PDU_TYPE_BSSMAP, BE_SMS_TO_VGCS, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}
/*
 * 3.2.1.82 NOTIFICATION DATA
 */
static void
bssmap_notification_data(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Application Data 3.2.2.98    MSC-BSS M   11 */
    ELEM_MAND_TLV(BE_APP_DATA_INF, GSM_A_PDU_TYPE_BSSMAP, BE_APP_DATA_INF, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Data Identity    3.2.2.99    MSC-BSS M   3 */
    ELEM_MAND_TLV(BE_DATA_ID, GSM_A_PDU_TYPE_BSSMAP, BE_DATA_ID, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* MSISDN   3.2.2.101   MSC-BSS O   2-12 */
    ELEM_MAND_TLV(BE_MSISDN, GSM_A_PDU_TYPE_BSSMAP, BE_MSISDN, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}
/*
 * 3.2.1.83 INTERNAL HANDOVER REQUIRED
 */
static void
bssmap_int_ho_req(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Cause    3.2.2.5     BSS-MSC     M    3-4 */
    ELEM_MAND_TLV(BE_CAUSE, GSM_A_PDU_TYPE_BSSMAP, BE_CAUSE, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Cell Identifier  3.2.2.17    BSS-MSC     M   4-10 */
    ELEM_MAND_TLV(BE_CELL_ID, GSM_A_PDU_TYPE_BSSMAP, BE_CELL_ID, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* AoIP Transport Layer Address (BSS)   3.2.2.nn    BSS-MSC C (Note 1)  10-22 */
    ELEM_OPT_TLV(BE_AOIP_TRANS_LAY_ADD, GSM_A_PDU_TYPE_BSSMAP, BE_AOIP_TRANS_LAY_ADD, NULL);
    /* Codec List (BSS Supported)   3.2.2.103    BSS-MSC M   3-n */
    ELEM_OPT_TLV(BE_SPEECH_CODEC_LST, GSM_A_PDU_TYPE_BSSMAP, BE_SPEECH_CODEC_LST, "(BSS Supported)");

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}
/*
 * 3.2.1.84 INTERNAL HANDOVER REQUIRED REJECT
 */
static void
bssmap_int_ho_req_rej(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Cause    3.2.2.5     MSC-BSS     M    3-4 */
    ELEM_MAND_TLV(BE_CAUSE, GSM_A_PDU_TYPE_BSSMAP, BE_CAUSE, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Codec List (MSC Preferred)   3.2.2.nn    MSC-BSS O   3-n */
    ELEM_OPT_TLV(BE_SPEECH_CODEC_LST, GSM_A_PDU_TYPE_BSSMAP, BE_SPEECH_CODEC_LST, "(BSS Supported)");

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 * 3.2.1.85 INTERNAL HANDOVER COMMAND
 */
static void
bssmap_int_ho_cmd(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Speech Codec (MSC Chosen)    3.2.2.nn    MSC-BSS M (note 1)  3-n */
    ELEM_OPT_TLV(BE_SPEECH_CODEC, GSM_A_PDU_TYPE_BSSMAP, BE_SPEECH_CODEC, "(Chosen)");
    /* Circuit Identity Code    3.2.2.2     MSC-BSS     C (note 2)  3 */
    ELEM_OPT_TV(BE_CIC, GSM_A_PDU_TYPE_BSSMAP, BE_CIC, NULL);
    /* AoIP Transport Layer Address (MGW)   3.2.2.102    MSC-BSS C (note 2)  10-22 */
    ELEM_OPT_TLV(BE_AOIP_TRANS_LAY_ADD, GSM_A_PDU_TYPE_BSSMAP, BE_AOIP_TRANS_LAY_ADD, NULL);
    /* Call Identifier  3.2.2.105   MSC-BSS C (note 4) 5 */
    ELEM_OPT_TV(BE_CALL_ID, GSM_A_PDU_TYPE_BSSMAP, BE_CALL_ID, NULL);
    /* Downlink DTX Flag  3.2.2.26   MSC-BSS O (note 5) 2 */
    ELEM_OPT_TV(BE_DOWN_DTX_FLAG, GSM_A_PDU_TYPE_BSSMAP, BE_DOWN_DTX_FLAG, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}
/*
 * 3.2.1.86 INTERNAL HANDOVER ENQUIRY
 */

static void
bssmap_int_ho_enq(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Speech Codec (MSC Chosen)    3.2.2.104   MSC-BSS M   3-n */
    ELEM_OPT_TLV(BE_SPEECH_CODEC, GSM_A_PDU_TYPE_BSSMAP, BE_SPEECH_CODEC, "(Chosen)");

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}
/*
 * 3.2.1.87 RESET IP RESOURCE
 */
static void
bssmap_reset_ip_res(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Cause    3.2.2.5 Both    M   3-4 */
    ELEM_MAND_TLV(BE_CAUSE, GSM_A_PDU_TYPE_BSSMAP, BE_CAUSE, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Call Identifier List 3.2.2.106   Both    M   6-n */
    ELEM_OPT_TLV(BE_CALL_ID_LST, GSM_A_PDU_TYPE_BSSMAP, BE_CALL_ID_LST, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}
/*
 * 3.2.1.88 RESET IP RESOURCE ACKNOWLEDGE
 */
static void
bssmap_reset_ip_res_ack(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Call Identifier List 3.2.2.106   Both    M   6-n */
    ELEM_OPT_TLV(BE_CALL_ID_LST, GSM_A_PDU_TYPE_BSSMAP, BE_CALL_ID_LST, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 * 3.2.1.89 REROUTE COMMAND
 */
static void
bssmap_reroute_cmd(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Initial Layer 3 Information 3.2.2.24 MSC-BSS M (note 1) 3-n */
    ELEM_MAND_TLV(BE_L3_INFO, GSM_A_PDU_TYPE_BSSMAP, BE_L3_INFO, " (Initial)", ei_gsm_a_bssmap_missing_mandatory_element);
    /* Reroute Reject Cause 3.2.2.112 MSC-BSS M (note 2) 2 */
    ELEM_MAND_TV(BE_REROUTE_REJ_CAUSE, GSM_A_PDU_TYPE_BSSMAP, BE_REROUTE_REJ_CAUSE, NULL, ei_gsm_a_bssmap_missing_mandatory_element);
    /* Layer 3 Information 3.2.2.24 MSC-BSS O (note3) 3-n */
    ELEM_OPT_TLV(BE_L3_INFO, GSM_A_PDU_TYPE_BSSMAP, BE_L3_INFO, NULL);
    /* Send Sequence Number 3.2.2.113 MSC-BSS O (note 4) 2 */
    ELEM_OPT_TV(BE_SEND_SEQN, GSM_A_PDU_TYPE_BSSMAP, BE_SEND_SEQN, NULL);
    /* IMSI 3.2.2.6 MSC-BSS O (note 5) 3-10 */
    ELEM_OPT_TLV(BE_IMSI, GSM_A_PDU_TYPE_BSSMAP, BE_IMSI, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

/*
 * 3.2.1.90 REROUTE COMPLETE
 */

static void
bssmap_reroute_complete(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Reroute complete outcome 3.2.2.114 MSC-BSS M 2 */
    ELEM_MAND_TV(BE_REROUTE_OUTCOME, GSM_A_PDU_TYPE_BSSMAP, BE_REROUTE_OUTCOME, NULL, ei_gsm_a_bssmap_missing_mandatory_element);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_gsm_a_bssmap_extraneous_data);
}

#if 0
/*
 * 3.2.1.91 LCLS-CONNECT-CONTROL
 */

    /* LCLS-Configuration 3.2.2.116 MSC-BSS O (note1) 2 */
    /* LCLS-Connection-Status-Control 3.2.2.117 MSC-BSS O (note1) 2 */

/*
 * 3.2.1.92 LCLS-CONNECT-CONTROL-ACK
 */

    /* LCLS-BSS-Status 3.2.2.119 BSS-MSC M 2 */


/*
 * 3.2.1.93 LCLS-NOTIFICATION
 */

    /* LCLS-BSS-Status 3.2.2.119 BSS-MSC O (note 1) 2 */
    /* LCLS-Break-Request 3.2.2.120 BSS-MSC O (note 1) 1 */

#endif
#define NUM_GSM_BSSMAP_MSG (int)(sizeof(gsm_a_bssmap_msg_strings)/sizeof(value_string))
static gint ett_gsm_bssmap_msg[NUM_GSM_BSSMAP_MSG];

static void (*bssmap_msg_fcn[])(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len) = {
    bssmap_ass_req,                     /* Assignment Request */
    bssmap_ass_complete,                /* Assignment Complete */
    bssmap_ass_failure,                 /* Assignment Failure */
    bssmap_vgcs_vbs_setup,              /* VGCS/VBS Setup */
    bssmap_vgcs_vbs_setup_ack,          /* VGCS/VBS Setup Ack */
    bssmap_vgcs_vbs_setup_refuse,       /* VGCS/VBS Setup Refuse */
    bssmap_vgcs_vbs_ass_req,            /* VGCS/VBS Assignment Request */
    bssmap_chan_mod_req,                /* 0x08 Channel Modify request */
    NULL,                               /* Unallocated */
    NULL,                               /* Unallocated */
    NULL,                               /* Unallocated */
    NULL,                               /* Unallocated */
    NULL,                               /* Unallocated */
    NULL,                               /* Unallocated */
    NULL,                               /* Unallocated */
    bssmap_ho_req,                      /* 0x10 Handover Request */
    bssmap_ho_reqd,                     /* Handover Required */
    bssmap_ho_req_ack,                  /* Handover Request Acknowledge */
    bssmap_ho_cmd,                      /* Handover Command */
    bssmap_ho_complete,                 /* Handover Complete */
    bssmap_ho_succ,                     /* Handover Succeeded */
    bssmap_ho_failure,                  /* Handover Failure */
    bssmap_ho_performed,                /* Handover Performed */
    bssmap_ho_cand_enq,                 /* Handover Candidate Enquire */
    bssmap_ho_cand_resp,                /* Handover Candidate Response */
    bssmap_ho_reqd_rej,                 /* Handover Required Reject */
    bssmap_ho_det,                      /* 0x1b Handover Detect */
    bssmap_vgcs_vbs_ass_res,            /* 0x1c VGCS/VBS Assignment Result */
    bssmap_vgcs_vbs_ass_fail,           /* 0x1d VGCS/VBS Assignment Failure */
    NULL,                               /* 0x1e No dsta VGCS/VBS Queuing Indication */
    bssmap_uplink_req,                  /* 0x1f Uplink Request */

    bssmap_clear_cmd,                   /* 0x20 Clear Command */
    NULL /* no associated data */,      /* Clear Complete */
    bssmap_clear_req,                   /* Clear Request */
    NULL,                               /* Reserved */
    NULL,                               /* Reserved */
    bssmap_sapi_rej,                    /* SAPI 'n' Reject */
    bssmap_confusion,                   /* Confusion */
    bssmap_uplink_req_ack,              /* Uplink Request Acknowledge */
    bssmap_sus,                         /* Suspend */
    bssmap_res,                         /* Resume */
    bssmap_conn_oriented,               /* Connection Oriented Information */
    bssmap_perf_loc_req,                /* Perform Location Request */
    bssmap_lsa_info,                    /* LSA Information */
    bssmap_perf_loc_res,                /* Perform Location Response */
    bssmap_perf_loc_abort,              /* Perform Location Abort */
    bssmap_common_id,                   /* Common Id */
    bssmap_reset,                       /* Reset */
    bssmap_reset_ack,                   /* Reset Acknowledge */
    bssmap_overload,                    /* Overload */
    NULL,                               /* Reserved */
    bssmap_reset_cct,                   /* Reset Circuit */
    bssmap_reset_cct_ack,               /* Reset Circuit Acknowledge */
    bssmap_msc_invoke_trace,            /* MSC Invoke Trace */
    bssmap_bss_invoke_trace,            /* 0x37 BSS Invoke Trace */

    NULL,                               /* 0x38 unallocated */
    NULL,                               /* 0x39 unallocated */

    NULL,                               /* 0x3a Connectionless Information */
    bssmap_vgcs_vbs_assign_status,      /* 0x3b VGCS/VBS ASSIGNMENT STATUS */
    bssmap_vgcs_vbs_area_cell_info,     /* 0x3c VGCS/VBS AREA CELL INFO */
    bssmap_reset_ip_res,                /* 0x3d 3.2.1.87 RESET IP RESOURCE */
    bssmap_reset_ip_res_ack,            /* 0x3e 3.2.1.88 RESET IP RESOURCE ACKNOWLEDGE */
    NULL,                               /* 0x3f VGCS/VBS AREA CELL INFO */
    bssmap_block,                       /* Block */
    bssmap_block_ack,                   /* Blocking Acknowledge */
    bssmap_unblock,                     /* Unblock */
    bssmap_unblock_ack,                 /* Unblocking Acknowledge */
    bssmap_cct_group_block,             /* Circuit Group Block */
    bssmap_cct_group_block_ack,         /* Circuit Group Blocking Acknowledge */
    bssmap_cct_group_unblock,           /* Circuit Group Unblock */
    bssmap_cct_group_unblock_ack,       /* Circuit Group Unblocking Acknowledge */
    bssmap_unequipped_cct,              /* Unequipped Circuit */
    bssmap_uplink_req_conf,             /* Uplink Request Confirmation */
    bssmap_uplink_rel_ind,              /* Uplink Release Indication */
    bssmap_uplink_rej_cmd,              /* Uplink Reject Command */
    bssmap_uplink_rel_cmd,              /* Uplink Release Command */
    bssmap_uplink_seized_cmd,           /* Uplink Seized Command */
    bssmap_change_cct,                  /* Change Circuit */
    bssmap_change_cct_ack,              /* Change Circuit Acknowledge */
    bssmap_res_req,                     /* Resource Request */
    bssmap_res_ind,                     /* Resource Indication */
    bssmap_paging,                      /* Paging */
    bssmap_ciph_mode_cmd,               /* Cipher Mode Command */
    bssmap_cm_upd,                      /* Classmark Update */
    bssmap_ciph_mode_complete,          /* Cipher Mode Complete */
    NULL /* no associated data */,      /* Queuing Indication */
    bssmap_cl3_info,                    /* Complete Layer 3 Information */
    bssmap_cls_m_req /* no associated data */,  /* Classmark Request */
    bssmap_ciph_mode_rej,               /* Cipher Mode Reject */
    bssmap_load_ind,                    /* 0x5a Load Indication */

    NULL,                               /* 0x5b unallocated */
    NULL,                               /* 0x5c unallocated */
    NULL,                               /* 0x5d unallocated */
    NULL,                               /* 0x5e unallocated */
    NULL,                               /* 0x5f unallocated */

    bssmap_vgcs_add_inf,                /* 0x60 VGCS Additional Information */
    bssmap_vgcs_sms,                    /* 0x61 VGCS SMS */
    bssmap_notification_data,           /* 0x62 Notification Data*/
    bssmap_uplink_app_data,             /* 0x63 Uplink Application Data */

    NULL,                               /* 0x64 unallocated */
    NULL,                               /* 0x65 unallocated */
    NULL,                               /* 0x66 unallocated */
    NULL,                               /* 0x67 unallocated */
    NULL,                               /* 0x68 unallocated */
    NULL,                               /* 0x69 unallocated */
    NULL,                               /* 0x6a unallocated */
    NULL,                               /* 0x6b unallocated */
    NULL,                               /* 0x6c unallocated */
    NULL,                               /* 0x6d unallocated */
    NULL,                               /* 0x6e unallocated */
    NULL,                               /* 0x6f unallocated */

    bssmap_int_ho_req,                  /* 0x70 Internal Handover Required */
    bssmap_int_ho_req_rej,              /* 0x71 Internal Handover Required Reject */
    bssmap_int_ho_cmd,                  /* 0x72 Internal Handover Command */
    bssmap_int_ho_enq,                  /* 0x73 Internal Handover Enquiry */

    NULL,                               /* 0x74 LCLS-Connect-Control */
    NULL,                               /* 0x75 LCLS-Connect-Control-Ack */
    NULL,                               /* 0x76 LCLS-Notification */
    NULL,                               /* 0x77 Unallocated */
    bssmap_reroute_cmd,                 /* 0x78 Reroute Command */
    bssmap_reroute_complete,            /* 0x79 Reroute Complete */

    NULL,   /* NONE */
};
#define NUM_BSSMAP_MSG_FCNS     (int)(sizeof(bssmap_msg_fcn)/sizeof(bssmap_msg_fcn[0]))

static int
dissect_bssmap(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data)
{
    static gsm_a_tap_rec_t  tap_rec[4];
    static gsm_a_tap_rec_t  *tap_p;
    static guint      tap_current = 0;
    guint8            oct;
    guint32           offset, saved_offset;
    guint32           len;
    gint              idx;
    proto_item       *bssmap_item = NULL;
    proto_tree       *bssmap_tree = NULL;
    const gchar      *str;
    sccp_msg_info_t*  sccp_msg_p = (sccp_msg_info_t*)data;

    if (!(sccp_msg_p && sccp_msg_p->data.co.assoc)) {
        sccp_msg_p = NULL;
    }

    col_append_str(pinfo->cinfo, COL_INFO, "(BSSMAP) ");

    /*
     * set tap record pointer
     */
    tap_current++;
    if (tap_current >= 4)
    {
        tap_current = 0;
    }
    tap_p = &tap_rec[tap_current];


    offset = 0;
    saved_offset = offset;

    g_tree = tree;

    len = tvb_reported_length(tvb);

    /*
     * add BSSMAP message name
     */
    oct = tvb_get_guint8(tvb, offset++);

    str = try_val_to_str_idx_ext((guint32) oct, &gsm_a_bssmap_msg_strings_ext, &idx);

    if (sccp_msg_p && !sccp_msg_p->data.co.label) {
        sccp_msg_p->data.co.label = wmem_strdup(wmem_file_scope(),
                                                val_to_str_ext((guint32)oct,
                                                &gsm_a_bssmap_msg_strings_ext,
                                                "BSSMAP (0x%02x)"));
    }

    /*
     * create the protocol tree
     */
    /* The first two conditions are actually the same, i.e. if str != NULL,
     * idx >= 0, but checking idx makes it obvious we won' t use a potentially
     * negative idx in the else case.
     */
    if (str == NULL || idx < 0 || idx >= NUM_GSM_BSSMAP_MSG)
    {
        bssmap_item =
        proto_tree_add_protocol_format(tree, proto_a_bssmap, tvb, 0, len,
            "GSM A-I/F BSSMAP - Unknown BSSMAP Message Type (0x%02x)",
            oct);

        bssmap_tree = proto_item_add_subtree(bssmap_item, ett_bssmap_msg);
    }
    else
    {
        bssmap_item =
        proto_tree_add_protocol_format(tree, proto_a_bssmap, tvb, 0, -1,
            "GSM A-I/F BSSMAP - %s",
            str);

        bssmap_tree = proto_item_add_subtree(bssmap_item, ett_gsm_bssmap_msg[idx]);

        col_append_fstr(pinfo->cinfo, COL_INFO, "%s ", str);

        /*
         * add BSSMAP message name
         */
        proto_tree_add_uint_format(bssmap_tree, hf_gsm_a_bssmap_msg_type,
        tvb, saved_offset, 1, oct, "Message Type %s",str);
    }

    tap_p->pdu_type = GSM_A_PDU_TYPE_BSSMAP;
    tap_p->message_type = oct;

    tap_queue_packet(gsm_a_tap, pinfo, tap_p);

    if (str == NULL) return len;

    if ((len - offset) <= 0) return len;

    /*
     * decode elements
     */
    if (idx < 0 || idx >= NUM_BSSMAP_MSG_FCNS || bssmap_msg_fcn[idx] == NULL) {
        proto_tree_add_bytes_format(bssmap_tree, hf_gsm_a_bssmap_message_elements,
            tvb, offset, len - offset, NULL, "Message Elements");
    }else{
        if (sccp_msg_p && ((sccp_msg_p->data.co.assoc->app_info & 0xCD00) == 0xCD00)) {
            cell_discriminator = sccp_msg_p->data.co.assoc->app_info & 0xFF;
        }else{
            cell_discriminator = 0xFF;
        }
        (*bssmap_msg_fcn[idx])(tvb, bssmap_tree, pinfo, offset, len - offset);
        if (sccp_msg_p) {
            sccp_msg_p->data.co.assoc->app_info = cell_discriminator | 0xCDF0;
        }
    }
    g_tree = NULL;
    return len;
}

/* Register the protocol with Wireshark */
void
proto_register_gsm_a_bssmap(void)
{
    guint       i;
    guint       last_offset;

    /* Setup list of header fields */

    static hf_register_info hf[] =
    {
    { &hf_gsm_a_bssmap_msg_type,
        { "BSSMAP Message Type",    "gsm_a.bssmap.msgtype",
        FT_UINT8, BASE_HEX|BASE_EXT_STRING, &gsm_a_bssmap_msg_strings_ext, 0x0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_elem_id,
        { "Element ID", "gsm_a.bssmap.elem_id",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_field_elem_id,
        { "Field Element ID",   "gsm_a.bssmap.field_elem_id",
        FT_UINT8, BASE_HEX, VALS(bssmap_field_element_ids), 0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_cell_ci,
        { "Cell CI",    "gsm_a.bssmap.cell_ci",
        FT_UINT16, BASE_HEX_DEC, 0, 0x0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_cell_lac,
        { "Cell LAC",   "gsm_a.bssmap.cell_lac",
        FT_UINT16, BASE_HEX_DEC, 0, 0x0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_sac,
        { "SAC",    "gsm_a.bssmap.sac",
        FT_UINT16, BASE_HEX, 0, 0x0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_dlci_cc,
        { "Control Channel", "gsm_a.bssmap.dlci.cc",
        FT_UINT8, BASE_HEX, VALS(bssap_cc_values), 0xc0,
        NULL, HFILL}
    },
    { &hf_gsm_a_bssmap_dlci_spare,
        { "Spare", "gsm_a.bssmap.dlci.spare",
        FT_UINT8, BASE_HEX, NULL, 0x38,
        NULL, HFILL}
    },
    { &hf_gsm_a_bssmap_dlci_sapi,
        { "SAPI", "gsm_a.bssmap.dlci.sapi",
        FT_UINT8, BASE_HEX, VALS(bssap_sapi_values), 0x07,
        NULL, HFILL}
    },
    { &hf_gsm_a_bssmap_cause,
        { "BSSMAP Cause",   "gsm_a.bssmap.cause",
        FT_UINT8, BASE_HEX|BASE_RANGE_STRING, RVALS(gsm_a_bssap_cause_rvals), 0x7F,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_be_cell_id_disc,
        { "Cell identification discriminator","gsm_a.bssmap.be.cell_id_disc",
        FT_UINT8,BASE_DEC|BASE_EXT_STRING,  &gsm_a_be_cell_id_disc_vals_ext, 0x0f,
        NULL, HFILL }
    },
        { &hf_gsm_a_bssmap_pci,
        { "Preemption Capability indicator(PCI)","gsm_a.bssmap.pci",
        FT_BOOLEAN,8, TFS(&bssmap_pci_value), 0x40,
        NULL, HFILL }
    },
        { &hf_gsm_a_bssmap_qa,
        { "Queuing Allowed Indicator(QA)","gsm_a.bssmap.qa",
        FT_BOOLEAN,8, TFS(&tfs_allowed_not_allowed), 0x02,
        NULL, HFILL }
    },
        { &hf_gsm_a_bssmap_pvi,
        { "Preemption Vulnerability Indicator(PVI)","gsm_a.bssmap.pvi",
        FT_BOOLEAN,8, TFS(&bssmap_pvi_value), 0x01,
        NULL, HFILL }
    },
        { &hf_gsm_a_bssmap_interference_bands,
        { "Acceptable interference bands","gsm_a.bssmap.interference_bands",
        FT_UINT8, BASE_HEX, NULL, 0x01f,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_lsa_only,
        { "LSA only","gsm_a.bssmap.lsa_only",
        FT_BOOLEAN,8, TFS(&bssmap_lsa_only_value), 0x01,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_act,
        { "Active mode support","gsm_a.bssmap.act",
        FT_BOOLEAN,8, NULL, 0x20,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_pref,
        { "Preferential access","gsm_a.bssmap.pref",
        FT_BOOLEAN,8, NULL, 0x10,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_lsa_inf_prio,
        { "Priority","gsm_a.bssmap.lsa_inf_prio",
        FT_UINT8,BASE_DEC, NULL, 0x0f,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_seq_len,
        { "Sequence Length","gsm_a.bssmap.seq_len",
        FT_UINT8,BASE_DEC, NULL, 0xf0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_seq_no,
        { "Sequence Number","gsm_a.bssmap.seq_no",
        FT_UINT8,BASE_DEC, NULL, 0xf,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssap_cell_id_list_seg_cell_id_disc,
        { "Cell identification discriminator","gsm_a.bssmap.cell_id_list_seg_cell_id_disc",
        FT_UINT8,BASE_DEC, VALS(gsm_a_bssap_cell_id_list_seg_cell_id_disc_vals), 0xf,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssap_res_ind_method,
        { "Resource indication method","gsm_a.bssmap.res_ind_method",
        FT_UINT8,BASE_DEC, VALS(gsm_a_bssap_resource_indication_vals), 0xf,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssap_cic_list_range,
        { "Range","gsm_a.bssmap.cic_list_range",
        FT_UINT8, BASE_DEC, NULL, 0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssap_cic_list_status,
        { "Status","gsm_a.bssmap.cic_list_status",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssap_diag_error_pointer,
        { "Error pointer","gsm_a.bssmap.diag_error_pointer",
        FT_UINT16, BASE_HEX, NULL, 0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssap_diag_msg_rcv,
        { "Message received","gsm_a.bssmap.cic_list_status",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_ch_mode,
        { "Channel mode","gsm_a.bssmap.cch_mode",
        FT_UINT8,BASE_DEC,  VALS(gsm_a_bssmap_ch_mode_vals), 0xf0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_cur_ch_mode,
    { "Channel Mode", "gsm_a.bssmap.fe_cur_chan_type2.chan_mode",
        FT_UINT8, BASE_HEX, VALS(chan_mode_vals), 0xf0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_channel,
        { "Channel","gsm_a.bssmap.channel",
        FT_UINT8,BASE_DEC,  VALS(gsm_a_bssmap_channel_vals), 0x0f,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_trace_trigger_id,
        { "Priority Indication","gsm_a.bssmap.trace_trigger_id",
        FT_STRING, BASE_NONE, NULL, 0x0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_trace_priority_indication,
        { "Priority Indication","gsm_a.bssmap.trace_priority_indication",
        FT_UINT8,BASE_DEC,  NULL, 0x00,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_trace_bss_record_type,
        { "BSS Record Type","gsm_a.bssmap.bss_record__type",
        FT_UINT8,BASE_DEC,  VALS(gsm_a_bssmap_trace_bss_record_type_vals), 0x00,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_trace_msc_record_type,
        { "MSC Record Type","gsm_a.bssmap.msc_record_type",
        FT_UINT8,BASE_DEC,  VALS(gsm_a_bssmap_trace_msc_record_type_vals), 0x00,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_trace_invoking_event,
        { "Invoking Event","gsm_a.bssmap.trace_invoking_event",
        FT_UINT8,BASE_DEC,  VALS(gsm_a_bssmap_trace_invoking_event_vals), 0x0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_trace_reference,
        { "Trace Reference","gsm_a.bssmap.trace_id",
        FT_UINT16,BASE_DEC,  NULL, 0x0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_trace_omc_id,
        { "OMC ID","gsm_a.bssmap.trace_omc_id",
        FT_STRING, BASE_NONE, NULL, 0x0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_be_rnc_id,
        { "RNC-ID","gsm_a.bssmap.be.rnc_id",
        FT_UINT16,BASE_DEC,  NULL, 0x0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_apdu_protocol_id,
        { "Protocol ID", "gsm_a.bssmap.apdu_protocol_id",
        FT_UINT8, BASE_DEC, VALS(gsm_a_apdu_protocol_id_strings), 0x0,
        "APDU embedded protocol id", HFILL }
    },
    { &hf_gsm_a_bssmap_periodicity,
        { "Periodicity", "gsm_a.bssmap.periodicity",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_sm,
        { "Subsequent Mode","gsm_a.bssmap.sm",
        FT_BOOLEAN,8, NULL, 0x02,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_tarr,
        { "Total Accessible Resource Requested","gsm_a.bssmap.tarr",
        FT_BOOLEAN,8, NULL, 0x01,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_tot_no_of_fullr_ch,
        { "Total number of accessible full rate channels", "gsm_a.bssmap.tot_no_of_fullr_ch",
        FT_UINT16, BASE_DEC, NULL, 0x0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_tot_no_of_hr_ch,
        { "Total number of accessible half rate channels", "gsm_a.bssmap.tot_no_of_hr_ch",
        FT_UINT16, BASE_DEC, NULL, 0x0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_lsa_id,
        { "Identification of Localised Service Area", "gsm_a.bssmap.lsa_id",
        FT_UINT24, BASE_HEX, NULL, 0x0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_ep,
        { "EP", "gsm_a.bssmap.ep",
        FT_UINT8, BASE_DEC, NULL, 0x01,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_smi,
        { "Subsequent Modification Indication(SMI)", "gsm_a.bssmap.smi",
        FT_UINT8, BASE_DEC, VALS(gsm_a_bssmap_smi_vals), 0x0f,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_lcs_pri,
        { "Periodicity", "gsm_a.bssmap.lcs_pri",
        FT_UINT8, BASE_DEC, VALS(lcs_priority_vals), 0x0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_num_ms,
        { "Number of handover candidates", "gsm_a.bssmap.num_ms",
        FT_UINT8, BASE_DEC,NULL, 0x0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_talker_pri,
        { "Priority", "gsm_a.bssmap.talker_pri",
        FT_UINT8, BASE_DEC,VALS(gsm_a_bssmap_talker_pri_vals), 0x03,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_rr_mode,
        { "RR mode", "gsm_a.bssmap.rr_mode",
        FT_UINT8, BASE_DEC, VALS(gsm_a_bssmap_rr_mode_vals), 0xc0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_group_cipher_key_nb,
        { "Group cipher key number", "gsm_a.bssmap.group_cipher_key_nb",
        FT_UINT8, BASE_DEC, VALS(gsm_a_bssmap_group_cipher_key_nb_vals), 0x3c,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_vgcs_vbs_cell_status,
        { "Status", "gsm_a.bssmap.vgcs_vbs_cell_status",
        FT_UINT8, BASE_DEC, VALS(gsm_a_bssmap_vgcs_vbs_cell_status_vals), 0x07,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_paging_cause,
        { "Paging Cause", "gsm_a.bssmap.paging_cause",
        FT_UINT8, BASE_DEC,VALS(gsm_a_bssmap_paging_cause_vals), 0x06,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_paging_inf_flg,
        { "VGCS/VBS flag","gsm_a.bssmap.paging_inf_flg",
        FT_BOOLEAN,8, TFS(&bssmap_paging_inf_flg_value), 0x01,
        "If 1, a member of a VGCS/VBS-group", HFILL }
    },
    { &hf_gsm_a_bssmap_serv_ho_inf,
        { "Service Handover information", "gsm_a.bssmap.serv_ho_inf",
        FT_UINT8, BASE_HEX, NULL, 0x07,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_max_nb_traffic_chan,
        { "Maximum Number of Traffic Channels", "gsm_a.bssmap.max_nb_traffic_chan",
        FT_UINT8, BASE_DEC, VALS(gsm_a_max_nb_traffic_chan_vals), 0x07,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_acceptable_chan_coding_bit5,
        { "TCH/F43.2", "gsm_a.bssmap.acceptable_chan_coding_bit5",
        FT_BOOLEAN, 8, TFS(&gsm_a_bssmap_accept_not_accept_vals), 0x10,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_acceptable_chan_coding_bit4,
        { "TCH/F43.2", "gsm_a.bssmap.acceptable_chan_coding_bit4",
        FT_BOOLEAN, 8, TFS(&gsm_a_bssmap_accept_not_accept_vals), 0x08,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_acceptable_chan_coding_bit3,
        { "TCH/F43.2", "gsm_a.bssmap.acceptable_chan_coding_bit3",
        FT_BOOLEAN, 8, TFS(&gsm_a_bssmap_accept_not_accept_vals), 0x04,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_acceptable_chan_coding_bit2,
        { "TCH/F43.2", "gsm_a.bssmap.acceptable_chan_coding_bit2",
        FT_BOOLEAN, 8, TFS(&gsm_a_bssmap_accept_not_accept_vals), 0x02,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_acceptable_chan_coding_bit1,
        { "TCH/F43.2", "gsm_a.bssmap.acceptable_chan_coding_bit1",
        FT_BOOLEAN, 8, TFS(&gsm_a_bssmap_accept_not_accept_vals), 0x01,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_allowed_data_rate_bit8,
        { "43.5 kbit/s (TCH/F43.2)", "gsm_a.bssmap.allowed_data_rate_bit8",
        FT_BOOLEAN, 8, TFS(&tfs_allowed_not_allowed), 0x80,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_allowed_data_rate_bit7,
        { "32.0 kbit/s (TCH/F32.0)", "gsm_a.bssmap.allowed_data_rate_bit7",
        FT_BOOLEAN, 8, TFS(&tfs_allowed_not_allowed), 0x40,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_allowed_data_rate_bit6,
        { "29.0 kbit/s (TCH/F28.8)", "gsm_a.bssmap.allowed_data_rate_bit6",
        FT_BOOLEAN, 8, TFS(&tfs_allowed_not_allowed), 0x20,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_allowed_data_rate_bit5,
        { "14.5/14.4 kbit/s (TCH/F14.4)", "gsm_a.bssmap.allowed_data_rate_bit5",
        FT_BOOLEAN, 8, TFS(&tfs_allowed_not_allowed), 0x10,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_allowed_data_rate_bit4,
        { "12.0/9.6 kbit/s (TCH F/9.6)", "gsm_a.bssmap.allowed_data_rate_bit4",
        FT_BOOLEAN, 8, TFS(&tfs_allowed_not_allowed), 0x08,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_vstk_rand,
        { "VSTK_RAND", "gsm_a.bssmap.vstk_rand",
        FT_UINT64, BASE_HEX, NULL, 0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_vstk,
        { "VSTK", "gsm_a.bssmap.vstk",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }
    },

    { &hf_gsm_a_bssmap_spare_bits,
        { "Spare bit(s)", "gsm_a.bssmap.spare_bits",
        FT_UINT8, BASE_HEX, NULL, 0x0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_tpind,
        { "Talker priority indicator (TP Ind)","gsm_a.bssmap.tpind",
        FT_BOOLEAN,8, TFS(&gsm_bssmap_tpind_vals), 0x01,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_asind_b2,
        { "A-interface resource sharing indicator (AS Ind) bit 2","gsm_a.bssmap.asind_b2",
        FT_BOOLEAN,8, TFS(&gsm_bssmap_asind_b2_vals), 0x02,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_asind_b3,
        { "A-interface resource sharing indicator (AS Ind) bit 3","gsm_a.bssmap.asind_b3",
        FT_BOOLEAN,8, TFS(&gsm_bssmap_asind_b3_vals), 0x04,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_bss_res,
        { "Group or broadcast call re-establishment by the BSS indicator","gsm_a.bssmap.bss_res",
        FT_BOOLEAN,8, TFS(&gsm_bssmap_bss_res_vals), 0x08,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_tcp,
        { "Talker Channel Parameter (TCP)","gsm_a.bssmap.tcp",
        FT_BOOLEAN,8, TFS(&gsm_bssmap_bss_tcp_vals), 0x10,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_filler_bits,
        { "Filler Bits","gsm_a.bssmap.filler_bits",
        FT_UINT8, BASE_DEC,NULL, 0x07,
        NULL, HFILL }
    },
        { &hf_gsm_a_bssmap_method,
        { "Method","gsm_a.bssmap.method",
        FT_UINT8, BASE_DEC,VALS(gsm_a_bssmap_method_vals), 0xc0,
        NULL, HFILL }
    },
        { &hf_gsm_a_bssmap_ganss_id,
        { "GANSS Id","gsm_a.bssmap.ganss_id",
        FT_UINT8, BASE_DEC,VALS(gsm_a_bssmap_ganss_id_vals), 0x38,
        NULL, HFILL }
    },
        { &hf_gsm_a_bssmap_usage,
        { "Usage","gsm_a.bssmap.usage",
        FT_UINT8, BASE_DEC,VALS(gsm_a_bssmap_usage_vals), 0x07,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_data_id,
        { "Data Identity","gsm_a.bssmap.data_id",
        FT_UINT8, BASE_DEC_HEX, NULL, 0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_bt_ind,
        { "BT Ind","gsm_a.bssmap.bt_ind",
        FT_BOOLEAN, 8, TFS(&gsm_a_bssmap_bt_ind_val), 0x01,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_aoip_trans_ipv4,
        { "Transport Layer Address (IPv4)","gsm_a.bssmap.aoip_trans_ipv4",
        FT_IPv4,BASE_NONE,  NULL, 0x0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_aoip_trans_ipv6,
        { "Transport Layer Address (IPv6)","gsm_a.bssmap.aoip_trans_ipv6",
        FT_IPv6,BASE_NONE,  NULL, 0x0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_aoip_trans_port,
        { "UDP Port","gsm_a.bssmap.aoip_trans_port",
        FT_UINT16, BASE_DEC,NULL, 0x0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_fi,
        { "FI(Full IP)","gsm_a.bssmap.fi",
        FT_BOOLEAN,8, TFS(&bssmap_fi_vals), 0x80,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_pi,
        { "PI","gsm_a.bssmap.pi",
        FT_BOOLEAN,8, TFS(&bssmap_pi_vals), 0x40,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_pt,
        { "PT","gsm_a.bssmap.pt",
        FT_BOOLEAN,8, TFS(&bssmap_pt_vals), 0x20,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_tf,
        { "TF","gsm_a.bssmap.tf",
        FT_BOOLEAN,8, TFS(&bssmap_tf_vals), 0x10,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssap_speech_codec,
        { "Codec Type","gsm_a.bssmap.speech_codec",
        FT_UINT8, BASE_DEC|BASE_EXT_STRING, &bssap_speech_codec_values_ext, 0x0f,
        NULL, HFILL }
    },
        { &hf_gsm_a_bssap_extended_codec,
        { "Extended Codec Type","gsm_a.bssmap.extended_codec",
        FT_UINT8, BASE_DEC, VALS(bssap_extended_codec_values), 0x0,
        NULL, HFILL }
    },
        { &hf_gsm_a_bssap_extended_codec_r2,
        { "Redundancy Level 2","gsm_a.bssmap.r2",
        FT_BOOLEAN,8, TFS(&tfs_supported_not_supported), 0x80,
        NULL, HFILL }
    },
        { &hf_gsm_a_bssap_extended_codec_r3,
        { "Redundancy Level 3","gsm_a.bssmap.r3",
        FT_BOOLEAN,8, TFS(&tfs_supported_not_supported), 0x40,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_fi2,
        { "FI(Full IP)","gsm_a.bssmap.fi2",
        FT_BOOLEAN,8, TFS(&bssmap_fi2_vals), 0x80,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_pi2,
        { "PI","gsm_a.bssmap.pi2",
        FT_BOOLEAN,8, TFS(&bssmap_pi2_vals), 0x40,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_pt2,
        { "PT","gsm_a.bssmap.pt2",
        FT_BOOLEAN,8, TFS(&bssmap_pt2_vals), 0x20,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_tf2,
        { "TF","gsm_a.bssmap.tf2",
        FT_BOOLEAN,8, TFS(&bssmap_tf2_vals), 0x10,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_call_id,
        { "Call Identifier","gsm_a.bssmap.callid",
        FT_UINT32, BASE_DEC,NULL, 0x0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_spare,
        { "Spare", "gsm_a.bssmap.spare",
        FT_UINT8, BASE_HEX, NULL, 0x0,
        NULL, HFILL}
    },
    { &hf_gsm_a_bssmap_positioning_data_discriminator,
        { "Positioning Data Discriminator", "gsm_a.bssmap.posData.discriminator",
        FT_UINT8, BASE_HEX, NULL, 0x0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_positioning_method,
        { "Positioning method", "gsm_a.bssmap.posData.method",
        FT_UINT8, BASE_HEX, VALS(bssmap_positioning_methods), 0x0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_positioning_method_usage,
        { "Usage", "gsm_a.bssmap.posData.usage",
        FT_UINT8, BASE_HEX, VALS(bssmap_positioning_methods_usage), 0x0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_location_type_location_information,
        { "Location Information", "gsm_a.bssmap.locationType.locationInformation",
        FT_UINT8, BASE_HEX, VALS(bssmap_location_information_vals), 0x0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_location_type_positioning_method,
        { "Positioning Method", "gsm_a.bssmap.locationType.positioningMethod",
        FT_UINT8, BASE_HEX, VALS(bssmap_positioning_method_vals), 0x0,
        NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_chan_type_extension,
        { "Extension", "gsm_a.bssmap.chanType.permittedIndicator.extension",
            FT_BOOLEAN, 8, TFS(&bssmap_chan_type_extension_value), 0x80,
            NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_cause_extension,
        { "Extension", "gsm_a.bssmap.causeType.extension",
            FT_BOOLEAN, 8, TFS(&bssmap_cause_extension_value), 0x80,
            NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_ass_req,
        { "Assignment Requirement", "gsm_a.bssmap.assignment_requirement",
            FT_UINT8, BASE_HEX, VALS(gsm_a_bssmap_assignment_requirement_vals), 0x0,
            NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_emlpp_prio,
        { "eMLPP Priority", "gsm_a.bssmap.emlpp_priority",
            FT_UINT8, BASE_HEX, VALS(gsm_a_bssmap_call_priority_vals), 0x07,
            NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_rip,
        { "RIP", "gsm_a.bssmap.rip",
            FT_BOOLEAN, 8, TFS(&gsm_a_bssmap_rip_value), 0x02,
            NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_rtd,
        { "RTD", "gsm_a.bssmap.rtd",
            FT_BOOLEAN, 8, TFS(&gsm_a_bssmap_rtd_value), 0x01,
            NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_kc128,
        { "Kc128", "gsm_a.bssmap.kc128",
            FT_BYTES, BASE_NONE, NULL, 0,
            NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_csg_id,
        { "CSG Identity", "gsm_a.bssmap.csg_id",
            FT_UINT32, BASE_DEC, NULL, 0,
            NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_cell_access_mode,
        { "Cell Access Mode", "gsm_a.bssmap.cell_access_mode",
            FT_BOOLEAN, BASE_NONE, TFS(&gsm_a_bssmap_cell_access_mode_value), 0,
            NULL, HFILL }
    },

    { &hf_fe_extra_info_prec,
        { "Pre-emption Recommendation", "gsm_a.bssmap.fe_extra_info.prec",
            FT_UINT8, BASE_DEC, VALS(fe_extra_info_prec_vals), 0x01,
            NULL, HFILL }
    },
    { &hf_fe_extra_info_lcs,
        { "LCS Information", "gsm_a.bssmap.fe_extra_info.lcs",
            FT_UINT8, BASE_DEC, VALS(fe_extra_info_lcs_vals), 0x02,
            NULL, HFILL }
    },
    { &hf_fe_extra_info_ue_prob,
        { "UE support of UMTS", "gsm_a.bssmap.fe_extra_info.ue_prob",
            FT_UINT8, BASE_DEC, VALS(fe_extra_info_ue_prob_vals), 0x04,
            NULL, HFILL }
    },
    { &hf_fe_extra_info_spare,
        { "Extra Information Spare bits", "gsm_a.bssmap.fe_extra_info.spare",
            FT_UINT8, BASE_HEX, NULL, 0xf8,
            NULL, HFILL }
    },
    { &hf_fe_cur_chan_type2_chan_mode,
        { "Channel Mode", "gsm_a.bssmap.fe_cur_chan_type2.chan_mode",
             FT_UINT8, BASE_HEX, VALS(chan_mode_vals), 0x0f,
             NULL, HFILL }
    },
    { &hf_fe_cur_chan_type2_chan_mode_spare,
        { "Channel Mode Spare bits", "gsm_a.bssmap.fe_cur_chan_type2_chan_mode.spare",
            FT_UINT8, BASE_HEX, NULL, 0xf0,
            NULL, HFILL }
    },
    { &hf_fe_cur_chan_type2_chan_field,
        { "Channel Field", "gsm_a.bssmap.fe_cur_chan_type2.chan_field",
            FT_UINT8, BASE_HEX, VALS(fe_cur_chan_type2_chan_field_vals),0x0f,
            NULL, HFILL }
    },
    { &hf_fe_cur_chan_type2_chan_field_spare,
        { "Channel field Spare bits", "gsm_a.bssmap.fe_cur_chan_type2_chan_field.spare",
            FT_UINT8, BASE_HEX, NULL, 0xf0,
            NULL, HFILL }
    },
    { &hf_fe_target_radio_cell_info_rxlev_ncell,
        { "RXLEV-NCELL", "gsm_a.bssmap.fe_target_radio_cell_info.rxlev_ncell",
            FT_UINT8, BASE_HEX|BASE_EXT_STRING, &gsm_a_rr_rxlev_vals_ext, 0x3f,
            NULL, HFILL }
    },
    { &hf_fe_target_radio_cell_info_rxlev_ncell_spare,
        { "RXLEV-NCELL Spare bits", "gsm_a.bssmap.fe_target_radio_cell_info.rxlev_ncell_spare",
            FT_UINT8, BASE_HEX, NULL, 0xc0,
            NULL, HFILL }
    },
    { &hf_fe_dtm_info_dtm_ind,
        { "DTM indicator", "gsm_a.bssmap.fe_dtm_info.dtm_ind",
            FT_UINT8, BASE_HEX, VALS(gsm_a_bssmap_dtm_info_dtm_ind_vals), 0x01,
            NULL, HFILL }
    },
    { &hf_fe_dtm_info_sto_ind,
        { "Time Slot Operation indicator", "gsm_a.bssmap.fe_dtm_info.sto_ind",
            FT_UINT8, BASE_HEX, VALS(gsm_a_bssmap_dtm_info_sto_ind_vals), 0x02,
            NULL, HFILL }
    },
    { &hf_fe_dtm_info_egprs_ind,
        { "EGPRS indicator", "gsm_a.bssmap.fe_dtm_info.egprs_ind",
            FT_UINT8, BASE_HEX, VALS(gsm_a_bssmap_dtm_info_egprs_ind_vals), 0x04,
            NULL, HFILL }
    },
    { &hf_fe_dtm_info_spare_bits,
        { "DTM Info Spare bits", "gsm_a.bssmap.fe_dtm_info.spare_bits",
            FT_UINT8, BASE_HEX, NULL, 0xf8,
            NULL, HFILL }
    },
    { &hf_fe_cell_load_info_cell_capacity_class,
        { "Cell capacity class", "gsm_a.bssmap.fe_cell_load_info.cell_capacity_class",
            FT_UINT8, BASE_DEC, NULL, 0,
            NULL, HFILL }
    },
    { &hf_fe_cell_load_info_load_value,
        { "Load value", "gsm_a.bssmap.fe_cell_load_info.load_info",
            FT_UINT8, BASE_DEC, NULL, 0,
            NULL, HFILL }
    },
    { &hf_fe_cell_load_info_rt_load_value,
        { "Realtime load value", "gsm_a.bssmap.fe_cell_load_info.rt_load_value",
            FT_UINT8, BASE_DEC, NULL, 0,
            NULL, HFILL }
    },
    { &hf_fe_cell_load_info_nrt_load_information_value,
        { "Non-Realtime load information value", "gsm_a.bssmap.fe_cell_load_info.nrt_load_info_value",
            FT_UINT8, BASE_HEX, VALS(gsm_a_bssmap_cell_load_nrt_vals), 0,
            NULL, HFILL }
    },
    { &hf_fe_ps_indication,
        { "PS Indication", "gsm_a.bssmap.fe_ps_indication.value",
            FT_UINT8, BASE_HEX, NULL, 0,
            NULL, HFILL }
    },
    { &hf_fe_dtm_ho_command_ind_spare,
        { "Spare octet", "gsm_a.bssmap.fe_dtm_ho_command_ind.spare",
            FT_UINT8, BASE_HEX, NULL, 0,
            NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_speech_data_ind,
        { "Speech/Data Indicator", "gsm_a.bssmap.speech_data_ind",
            FT_UINT8, BASE_DEC, VALS(gsm_a_bssap_speech_data_ind_vals), 0x0f,
            NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_channel_rate_and_type,
        { "Channel Rate and Type", "gsm_a.bssmap.perm_speech_v_ind",
            FT_UINT8, BASE_DEC, VALS(gsm_a_bssap_channel_rate_and_type_vals), 0x0,
            NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_perm_speech_v_ind,
        { "Permitted speech version indication", "gsm_a.bssmap.perm_speech_v_ind",
            FT_UINT8, BASE_HEX|BASE_RANGE_STRING, RVALS(speech_version_id_rvals), 0x7f,
            NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_reroute_rej_cause,
        { "Reroute Reject cause", "gsm_a.bssmap.reroute_rej_cause",
            FT_UINT8, BASE_HEX, VALS(gsm_a_bssap_reroute_rej_cause_vals), 0x0,
            NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_send_seqn,
        { "Send Sequence Number", "gsm_a.bssmap.reroute_rej_cause",
            FT_UINT8, BASE_HEX, NULL, 0xc0,
            NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_reroute_outcome,
        { "Outcome", "gsm_a.bssmap.reroute_outcome",
            FT_UINT8, BASE_HEX, VALS(gsm_a_bssap_reroute_outcome_vals), 0x0,
            NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_lcls_conf,
        { "LCLS-Configuration", "gsm_a.bssmap.lcls_conf",
            FT_UINT8, BASE_HEX, VALS(gsm_a_bssap_lcls_conf_vals), 0x0f,
            NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_lcls_con_status_control,
        { "LCLS-Connection-Status-Control", "gsm_a.bssmap.lcls_con_status_control",
            FT_UINT8, BASE_HEX, VALS(gsm_a_bssap_lcls_con_status_control_vals), 0x0f,
            NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_lcls_bss_status,
        { "LCLS-BSS-Status", "gsm_a.bssmap.lcls_bss_status",
            FT_UINT8, BASE_HEX, VALS(gsm_a_bssmap_lcls_bss_status_vals), 0x0f,
            NULL, HFILL }
    },
    { &hf_gsm_a_bssmap_selected_plmn_id,
        { "Selected PLMN ID", "gsm_a.bssmap.selected_plmn_id",
            FT_STRING, BASE_NONE, NULL, 0,
            NULL, HFILL }
    },

      /* Generated from convert_proto_tree_add_text.pl */
      { &hf_gsm_a_bssmap_pcm_multiplexer, { "PCM Multiplexer", "gsm_a_bssmap.pcm_multiplexer", FT_UINT16, BASE_DEC, NULL, 0xffe0, NULL, HFILL }},
      { &hf_gsm_a_bssmap_timeslot, { "Timeslot", "gsm_a_bssmap.timeslot", FT_UINT16, BASE_DEC, NULL, 0x001f, NULL, HFILL }},
      { &hf_gsm_a_bssmap_full_rate_channels_available, { "Number of full rate channels available in band", "gsm_a_bssmap.full_rate_channels_available", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
      { &hf_gsm_a_bssmap_half_rate_channels_available, { "Number of half rate channels available in band", "gsm_a_bssmap.half_rate_channels_available", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
      { &hf_gsm_a_bssmap_cause_class, { "Cause Class", "gsm_a_bssmap.cause_class", FT_UINT8, BASE_DEC, VALS(cause_class_vals), 0x70, NULL, HFILL }},
      { &hf_gsm_a_bssmap_national_cause, { "National Cause", "gsm_a_bssmap.national_cause", FT_UINT8, BASE_DEC, NULL, 0x0f, NULL, HFILL }},
      { &hf_gsm_a_bssmap_cause_value, { "Cause Value", "gsm_a_bssmap.cause_value", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL }},
      { &hf_gsm_a_bssmap_cause16, { "Cause", "gsm_a_bssmap.cause", FT_UINT16, BASE_DEC, NULL, 0x07ff, NULL, HFILL }},
      { &hf_gsm_a_bssmap_ti_flag, { "TI flag", "gsm_a_bssmap.ti_flag", FT_BOOLEAN, 8, TFS(&tfs_allocated_by_receiver_sender), 0x08, NULL, HFILL }},
      { &hf_gsm_a_bssmap_tio, { "TIO", "gsm_a_bssmap.tio", FT_UINT8, BASE_DEC, NULL, 0x07, NULL, HFILL }},
      { &hf_gsm_a_bssmap_enc_info_key, { "Key", "gsm_a_bssmap.enc_info_key", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }},
      { &hf_gsm_a_bssmap_transparent_service, { "Service", "gsm_a_bssmap.transparent_service", FT_BOOLEAN, 8, TFS(&tfs_non_transparent_transparent), 0x40, NULL, HFILL }},
      { &hf_gsm_a_bssmap_rate, { "Rate", "gsm_a_bssmap.rate", FT_UINT8, BASE_DEC, NULL, 0x3f, NULL, HFILL }},
      { &hf_gsm_a_bssmap_tch_14_5kb, { "14.5 kbit/s (TCH/F14.4)", "gsm_a_bssmap.tch_14_5kb", FT_BOOLEAN, 8, TFS(&tfs_allowed_not_allowed), 0x08, NULL, HFILL }},
      { &hf_gsm_a_bssmap_tch_12kb, { "12.0 kbit/s (TCH F/9.6)", "gsm_a_bssmap.tch_12kb", FT_BOOLEAN, 8, TFS(&tfs_allowed_not_allowed), 0x02, NULL, HFILL }},
      { &hf_gsm_a_bssmap_tch_6kb, { "6.0 kbit/s (TCH F/4.8)", "gsm_a_bssmap.tch_6kb", FT_BOOLEAN, 8, TFS(&tfs_allowed_not_allowed), 0x01, NULL, HFILL }},
      { &hf_gsm_a_bssmap_tch_14_5_14_4kb, { "14.5/14.4 kbit/s (TCH/F14.4)", "gsm_a_bssmap.tch_14_5_14_4kb", FT_BOOLEAN, 8, NULL, 0x08, NULL, HFILL }},
      { &hf_gsm_a_bssmap_tch_12_9kb, { "12.0/9.6 kbit/s (TCH F/9.6)", "gsm_a_bssmap.tch_12_9kb", FT_BOOLEAN, 8, NULL, 0x02, NULL, HFILL }},
      { &hf_gsm_a_bssmap_tch_6_4_8kb, { "6.0/4.8 kbit/s (TCH F/4.8)", "gsm_a_bssmap.tch_6_4_8kb", FT_BOOLEAN, 8, NULL, 0x01, NULL, HFILL }},
      { &hf_gsm_a_bssmap_unknown_format, { "Unknown format", "gsm_a_bssmap.unknown_format", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }},
      { &hf_gsm_a_bssmap_cell_id_unknown_format, { "Cell ID - Unknown format", "gsm_a_bssmap.cell_id.unknown_format", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }},
      { &hf_gsm_a_bssmap_priority_level, { "Priority Level", "gsm_a_bssmap.priority_level", FT_UINT8, BASE_DEC|BASE_RANGE_STRING, RVALS(bssmap_prio_rvals), 0x3c, NULL, HFILL }},
      { &hf_gsm_a_bssmap_layer_3_information_value, { "Layer 3 Information value", "gsm_a_bssmap.layer_3_information_value", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }},
      { &hf_gsm_a_bssmap_bss_activate_downlink, { "BSS can to activate DTX in the downlink direction", "gsm_a_bssmap.bss_activate_downlink", FT_BOOLEAN, 8, TFS(&tfs_yes_no), 0x01, NULL, HFILL }},
      { &hf_gsm_a_bssmap_imeisv_included, { "IMEISV must be included by the mobile station", "gsm_a_bssmap.imeisv_included", FT_BOOLEAN, 8, TFS(&tfs_yes_no), 0x01, NULL, HFILL }},
      { &hf_gsm_a_bssmap_algorithm_identifier, { "Algorithm Identifier", "gsm_a_bssmap.algorithm_identifier", FT_UINT8, BASE_DEC, VALS(gsm_a_bssmap_algorithm_id_vals), 0x0, NULL, HFILL }},
      { &hf_gsm_a_bssmap_circuit_pool_number, { "Circuit pool number", "gsm_a_bssmap.circuit_pool_number", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL }},
      { &hf_gsm_a_bssmap_qri, { "qri recommended queuing", "gsm_a_bssmap.qri", FT_BOOLEAN, 8, TFS(&tfs_allowed_not_allowed), 0x02, NULL, HFILL }},
      { &hf_gsm_a_bssmap_speech_version_id, { "Speech version identifier", "gsm_a_bssmap.speech_version_id", FT_UINT8, BASE_DEC|BASE_RANGE_STRING, RVALS(speech_version_id_rvals), 0x7f, NULL, HFILL }},
      { &hf_gsm_a_bssmap_apdu, { "APDU", "gsm_a_bssmap.apdu", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }},
      { &hf_gsm_a_bssmap_talker_identity_field, { "Talker Identity field", "gsm_a_bssmap.talker_identity_field", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }},
      { &hf_gsm_a_bssmap_s0_s15, { "S0 - S15", "gsm_a_bssmap.s0_s15", FT_UINT16, BASE_HEX, NULL, 0x0, NULL, HFILL }},
      { &hf_gsm_a_bssmap_s0_s7, { "S0 - S7", "gsm_a_bssmap.s0_s7", FT_UINT8, BASE_HEX, NULL, 0x0, NULL, HFILL }},
      { &hf_gsm_a_bssmap_all_call_identifiers_resources_released, { "all resources and references associated to all Call Identifiers in use between the BSC and the MSC need to be released", "gsm_a_bssmap.all_call_identifiers_resources_released", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }},
      { &hf_gsm_a_bssmap_message_elements, { "Message Elements", "gsm_a_bssmap.message_elements", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }},
      { &hf_gsm_a_bssmap_layer3_message_contents, { "Layer 3 Message Contents", "gsm_a_bssmap.layer3_message_contents", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }},
      { &hf_gsm_a_bssmap_gsm_a5_1, { "GSM A5/1", "gsm_a_bssmap.gsm_a5_1", FT_BOOLEAN, 8, TFS(&tfs_permitted_not_permitted), 0x02, NULL, HFILL }},
      { &hf_gsm_a_bssmap_gsm_a5_2, { "GSM A5/2", "gsm_a_bssmap.gsm_a5_2", FT_BOOLEAN, 8, TFS(&tfs_permitted_not_permitted), 0x04, NULL, HFILL }},
      { &hf_gsm_a_bssmap_gsm_a5_3, { "GSM A5/3", "gsm_a_bssmap.gsm_a5_3", FT_BOOLEAN, 8, TFS(&tfs_permitted_not_permitted), 0x08, NULL, HFILL }},
      { &hf_gsm_a_bssmap_gsm_a5_4, { "GSM A5/4", "gsm_a_bssmap.gsm_a5_4", FT_BOOLEAN, 8, TFS(&tfs_permitted_not_permitted), 0x10, NULL, HFILL }},
      { &hf_gsm_a_bssmap_gsm_a5_5, { "GSM A5/5", "gsm_a_bssmap.gsm_a5_5", FT_BOOLEAN, 8, TFS(&tfs_permitted_not_permitted), 0x20, NULL, HFILL }},
      { &hf_gsm_a_bssmap_gsm_a5_6, { "GSM A5/6", "gsm_a_bssmap.gsm_a5_6", FT_BOOLEAN, 8, TFS(&tfs_permitted_not_permitted), 0x40, NULL, HFILL }},
      { &hf_gsm_a_bssmap_gsm_a5_7, { "GSM A5/7", "gsm_a_bssmap.gsm_a5_7", FT_BOOLEAN, 8, TFS(&tfs_permitted_not_permitted), 0x80, NULL, HFILL }},
      { &hf_gsm_a_bssmap_no_encryption, { "No encryption", "gsm_a_bssmap.no_encryption", FT_BOOLEAN, 8, TFS(&tfs_permitted_not_permitted), 0x01, NULL, HFILL }},
      { &hf_gsm_a_bssmap_data_channel_rate_and_type, { "Channel rate and type", "gsm_a_bssmap.channel_rate_and_type", FT_UINT8, BASE_DEC|BASE_RANGE_STRING, RVALS(gsm_a_bssap_channel_rate_and_type_rvals), 0x7f, NULL, HFILL }},
      { &hf_gsm_a_bssmap_cell_discriminator, { "Cell Discriminator", "gsm_a_bssmap.cell_discriminator", FT_UINT8, BASE_HEX, NULL, 0x0, NULL, HFILL }},
      { &hf_gsm_a_bssmap_forward_indicator, { "Forward indicator", "gsm_a_bssmap.forward_indicator", FT_UINT8, BASE_DEC|BASE_RANGE_STRING, RVALS(forward_indicator_rvals), 0x0f, NULL, HFILL }},
    };

    expert_module_t* expert_gsm_a_bssmap;

    static ei_register_info ei[] = {
        { &ei_gsm_a_bssmap_extraneous_data, { "gsm_a_bssmap.extraneous_data", PI_PROTOCOL, PI_NOTE, "Extraneous Data, dissector bug or later version spec(report to wireshark.org)", EXPFILL }},
        { &ei_gsm_a_bssmap_not_decoded_yet, { "gsm_a_bssmap.not_decoded_yet", PI_UNDECODED, PI_WARN, "Not decoded yet", EXPFILL }},
        { &ei_gsm_a_bssap_unknown_codec, { "gsm_a_bssmap.unknown_codec", PI_PROTOCOL, PI_WARN, "Unknown codec - the rest of the dissection my be suspect", EXPFILL }},
        { &ei_gsm_a_bssmap_bogus_length, { "gsm_a_bssmap.bogus_length", PI_PROTOCOL, PI_WARN, "Bogus length", EXPFILL }},
        { &ei_gsm_a_bssmap_missing_mandatory_element, { "gsm_a_bssmap.missing_mandatory_element", PI_PROTOCOL, PI_WARN, "Missing Mandatory element, rest of dissection is suspect", EXPFILL }},
    };

    /* Setup protocol subtree array */
#define NUM_INDIVIDUAL_ELEMS    5
    gint *ett[NUM_INDIVIDUAL_ELEMS + NUM_GSM_BSSMAP_MSG +
          NUM_GSM_BSSMAP_ELEM];

    ett[0] = &ett_bssmap_msg;
    ett[1] = &ett_cell_list;
    ett[2] = &ett_dlci;
    ett[3] = &ett_codec_lst,
    ett[4] = &ett_bss_to_bss_info,

    last_offset = NUM_INDIVIDUAL_ELEMS;

    for (i=0; i < NUM_GSM_BSSMAP_MSG; i++, last_offset++)
    {
        ett_gsm_bssmap_msg[i] = -1;
        ett[last_offset] = &ett_gsm_bssmap_msg[i];
    }

    for (i=0; i < NUM_GSM_BSSMAP_ELEM; i++, last_offset++)
    {
        ett_gsm_bssmap_elem[i] = -1;
        ett[last_offset] = &ett_gsm_bssmap_elem[i];
    }

    /* Register the protocol name and description */

    proto_a_bssmap =
        proto_register_protocol("GSM A-I/F BSSMAP", "GSM BSSMAP", "gsm_a.bssmap");

    proto_register_field_array(proto_a_bssmap, hf, array_length(hf));

    proto_register_subtree_array(ett, array_length(ett));

    expert_gsm_a_bssmap = expert_register_protocol(proto_a_bssmap);
    expert_register_field_array(expert_gsm_a_bssmap, ei, array_length(ei));

    register_dissector("gsm_a_bssmap", dissect_bssmap, proto_a_bssmap);
}


void
proto_reg_handoff_gsm_a_bssmap(void)
{
    dissector_handle_t bssmap_handle;

    bssmap_handle = find_dissector("gsm_a_bssmap");
    dissector_add_uint("bssap.pdu_type",  GSM_A_PDU_TYPE_BSSMAP, bssmap_handle);

    dtap_handle       = find_dissector_add_dependency("gsm_a_dtap", proto_a_bssmap);
    gsm_bsslap_handle = find_dissector_add_dependency("gsm_bsslap", proto_a_bssmap);
    bssgp_handle      = find_dissector_add_dependency("bssgp", proto_a_bssmap);
    rrc_handle        = find_dissector_add_dependency("rrc", proto_a_bssmap);

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
