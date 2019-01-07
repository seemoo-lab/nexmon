/* Do not modify this file. Changes will be overwritten.                      */
/* Generated automatically by the ASN.1 to Wireshark dissector compiler       */
/* packet-rrc.h                                                               */
/* asn2wrs.py -p rrc -c ./rrc.cnf -s ./packet-rrc-template -D . -O ../.. Class-definitions.asn PDU-definitions.asn InformationElements.asn Constant-definitions.asn Internode-definitions.asn */

/* Input file: packet-rrc-template.h */

#line 1 "./asn1/rrc/packet-rrc-template.h"
/* packet-rrc-template.h
 * Copyright 2009, Anders Broman <anders.broman@ericsson.com>
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

#ifndef PACKET_RRC_H
#define PACKET_RRC_H

#include <epan/asn1.h>    /* Needed for non asn1 dissectors?*/

extern int proto_rrc;

/*--- Included file: packet-rrc-exp.h ---*/
#line 1 "./asn1/rrc/packet-rrc-exp.h"
int dissect_rrc_InterRATHandoverInfo(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_);
int dissect_rrc_HandoverToUTRANCommand_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_);
int dissect_rrc_InterRATHandoverInfo_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_);
int dissect_rrc_MasterInformationBlock_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_);
int dissect_rrc_SysInfoType1_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_);
int dissect_rrc_SysInfoType2_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_);
int dissect_rrc_SysInfoType3_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_);
int dissect_rrc_SysInfoType7_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_);
int dissect_rrc_SysInfoType12_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_);
int dissect_rrc_ToTargetRNC_Container_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_);
int dissect_rrc_TargetRNC_ToSourceRNC_Container_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_);

/*--- End of included file: packet-rrc-exp.h ---*/
#line 30 "./asn1/rrc/packet-rrc-template.h"

enum rrc_message_type {
  RRC_MESSAGE_TYPE_INVALID    = 0,
  RRC_MESSAGE_TYPE_PCCH        = 1,
  RRC_MESSAGE_TYPE_UL_CCCH,
  RRC_MESSAGE_TYPE_DL_CCCH,
  RRC_MESSAGE_TYPE_UL_DCCH,
  RRC_MESSAGE_TYPE_DL_DCCH,
  RRC_MESSAGE_TYPE_BCCH_FACH
};

#define MAX_RRC_FRAMES    64
typedef struct rrc_info
{
  enum rrc_message_type msgtype[MAX_RRC_FRAMES];
  guint16 hrnti[MAX_RRC_FRAMES];
} rrc_info;

/*Struct for storing ciphering information*/
typedef struct rrc_ciph_info_
{
  int seq_no[31][2];    /*Indicates for each Rbid when ciphering starts*/
  GTree * /*guint32*/ start_cs;    /*Start value for CS counter*/
  GTree * /*guint32*/ start_ps;    /*Start value for PS counter*/
  guint32 conf_algo_indicator;    /*Indicates which type of ciphering algorithm used*/
  guint32 int_algo_indiccator;    /*Indicates which type of integrity algorithm used*/
  unsigned int setup_frame;    /*Store which frame contained this information*/
  guint32 ps_conf_counters[31][2];    /*This should also be made for CS*/

} rrc_ciphering_info;

extern GTree * hsdsch_muxed_flows;
extern GTree * rrc_ciph_inf;

#endif  /* PACKET_RRC_H */
