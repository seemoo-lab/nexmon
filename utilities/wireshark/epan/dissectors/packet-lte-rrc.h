/* Do not modify this file. Changes will be overwritten.                      */
/* Generated automatically by the ASN.1 to Wireshark dissector compiler       */
/* packet-lte-rrc.h                                                           */
/* asn2wrs.py -L -p lte-rrc -c ./lte-rrc.cnf -s ./packet-lte-rrc-template -D . -O ../.. EUTRA-InterNodeDefinitions.asn EUTRA-RRC-Definitions.asn EUTRA-Sidelink-Preconf.asn EUTRA-UE-Variables.asn PC5-RRC-Definitions.asn NBIOT-InterNodeDefinitions.asn NBIOT-RRC-Definitions.asn NBIOT-UE-Variables.asn */

/* Input file: packet-lte-rrc-template.h */

#line 1 "./asn1/lte-rrc/packet-lte-rrc-template.h"
/* packet-llc-rrc-template.h
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

#ifndef PACKET_LTE_RRC_H
#define PACKET_LTE_RRC_H


/*--- Included file: packet-lte-rrc-exp.h ---*/
#line 1 "./asn1/lte-rrc/packet-lte-rrc-exp.h"
int dissect_lte_rrc_HandoverCommand(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_);
int dissect_lte_rrc_HandoverPreparationInformation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_);
int dissect_lte_rrc_UERadioAccessCapabilityInformation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_);
int dissect_lte_rrc_UE_EUTRA_Capability(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_);
int dissect_lte_rrc_HandoverPreparationInformation_NB(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_);
int dissect_lte_rrc_UERadioAccessCapabilityInformation_NB(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_);
int dissect_lte_rrc_HandoverCommand_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_);
int dissect_lte_rrc_HandoverPreparationInformation_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_);
int dissect_lte_rrc_UERadioAccessCapabilityInformation_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_);
int dissect_lte_rrc_UE_EUTRA_Capability_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_);
int dissect_lte_rrc_HandoverPreparationInformation_NB_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_);
int dissect_lte_rrc_UERadioAccessCapabilityInformation_NB_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_);

/*--- End of included file: packet-lte-rrc-exp.h ---*/
#line 27 "./asn1/lte-rrc/packet-lte-rrc-template.h"

#endif  /* PACKET_LTE_RRC_H */
