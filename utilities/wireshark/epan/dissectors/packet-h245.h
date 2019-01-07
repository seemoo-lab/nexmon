/* Do not modify this file. Changes will be overwritten.                      */
/* Generated automatically by the ASN.1 to Wireshark dissector compiler       */
/* packet-h245.h                                                              */
/* asn2wrs.py -p h245 -c ./h245.cnf -s ./packet-h245-template -D . -O ../.. MULTIMEDIA-SYSTEM-CONTROL.asn */

/* Input file: packet-h245-template.h */

#line 1 "./asn1/h245/packet-h245-template.h"
/* packet-h245.h
 * Routines for h245 packet dissection
 * Copyright 2005, Anders Broman <anders.broman@ericsson.com>
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

#ifndef PACKET_H245_H
#define PACKET_H245_H

#include "ws_symbol_export.h"

typedef enum _h245_msg_type {
	H245_TermCapSet,
	H245_TermCapSetAck,
	H245_TermCapSetRjc,
	H245_TermCapSetRls,
	H245_OpenLogChn,
	H245_OpenLogChnCnf,
	H245_OpenLogChnAck,
	H245_OpenLogChnRjc,
	H245_CloseLogChn,
	H245_CloseLogChnAck,
	H245_MastSlvDet,
	H245_MastSlvDetAck,
	H245_MastSlvDetRjc,
	H245_MastSlvDetRls,
        H245_OTHER
} h245_msg_type;

typedef struct _h245_packet_info {
        h245_msg_type msg_type;         /* type of message */
        gchar frame_label[50];          /* the Frame label used by graph_analysis, what is a abreviation of cinfo */
        gchar comment[50];                      /* the Frame Comment used by graph_analysis, what is a message desc */
} h245_packet_info;

/*
 * h223 LC info
 */

typedef enum {
	al_nonStandard,
	al1Framed,
	al1NotFramed,
	al2WithoutSequenceNumbers,
	al2WithSequenceNumbers,
	al3,
	/*...*/
	/* al?M: unimplemented annex C adaptation layers */
	al1M,
	al2M,
	al3M
} h223_al_type;

typedef struct {
	guint8 control_field_octets;
	guint32 send_buffer_size;
} h223_al3_params;

typedef struct {
	h223_al_type al_type;
	gpointer al_params;
	gboolean segmentable;
	dissector_handle_t subdissector;
} h223_lc_params;

typedef enum {
	H245_nonStandardDataType,
	H245_nullData,
	H245_videoData,
	H245_audioData,
	H245_data,
	H245_encryptionData,
	/*...,*/
	H245_h235Control,
	H245_h235Media,
	H245_multiplexedStream,
	H245_redundancyEncoding,
	H245_multiplePayloadStream,
	H245_fec
} h245_lc_data_type_enum;

typedef struct {
	h245_lc_data_type_enum data_type;
	gpointer               params;
} h245_lc_data_type;

/*
 * h223 MUX info
 */

typedef struct _h223_mux_element h223_mux_element;
struct _h223_mux_element {
    h223_mux_element* sublist; /* if NULL, use vc instead */
    guint16 vc;
    guint16 repeat_count; /* 0 == untilClosingFlag */
    h223_mux_element* next;
};

#include <epan/packet_info.h>
#include <epan/dissectors/packet-per.h>

typedef void (*h223_set_mc_handle_t) ( packet_info* pinfo, guint8 mc, h223_mux_element* me, circuit_type ctype, guint32 circuit_id );
WS_DLL_PUBLIC void h245_set_h223_set_mc_handle( h223_set_mc_handle_t handle );

typedef void (*h223_add_lc_handle_t) ( packet_info* pinfo, guint16 lc, h223_lc_params* params, circuit_type ctype, guint32 circuit_id );
WS_DLL_PUBLIC void h245_set_h223_add_lc_handle( h223_add_lc_handle_t handle );


/*--- Included file: packet-h245-exp.h ---*/
#line 1 "./asn1/h245/packet-h245-exp.h"
extern const value_string h245_Capability_vals[];
extern const value_string DataProtocolCapability_vals[];
extern const value_string h245_TransportAddress_vals[];
extern const value_string h245_UnicastAddress_vals[];
extern const value_string h245_MulticastAddress_vals[];
int dissect_h245_Capability(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_);
WS_DLL_PUBLIC int dissect_h245_H223Capability(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_);
int dissect_h245_QOSCapability(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_);
int dissect_h245_DataProtocolCapability(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_);
int dissect_h245_T38FaxProfile(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_);
int dissect_h245_OpenLogicalChannel(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_);
int dissect_h245_H223LogicalChannelParameters(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_);
int dissect_h245_TransportAddress(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_);
int dissect_h245_UnicastAddress(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_);
int dissect_h245_MulticastAddress(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_);

/*--- End of included file: packet-h245-exp.h ---*/
#line 126 "./asn1/h245/packet-h245-template.h"
void dissect_h245_FastStart_OLC(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, char *codec_str);


#endif  /* PACKET_H245_H */


