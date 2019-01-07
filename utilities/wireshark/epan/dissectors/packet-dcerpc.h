/* packet-dcerpc.h
 * Copyright 2001, Todd Sabin <tas@webspan.net>
 * Copyright 2003, Tim Potter <tpot@samba.org>
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

#ifndef __PACKET_DCERPC_H__
#define __PACKET_DCERPC_H__

#include <epan/conversation.h>
#include "ws_symbol_export.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Data representation.
 */
#define DREP_LITTLE_ENDIAN	0x10

#define DREP_EBCDIC		0x01

/*
 * Data representation to integer byte order.
 */
#define DREP_ENC_INTEGER(drep)	\
	(((drep)[0] & DREP_LITTLE_ENDIAN) ? ENC_LITTLE_ENDIAN : ENC_BIG_ENDIAN)

/*
 * Data representation to (octet-string) character encoding.
 */
#define DREP_ENC_CHAR(drep)	\
	(((drep)[0] & DREP_EBCDIC) ? ENC_EBCDIC|ENC_NA : ENC_ASCII|ENC_NA)

#ifdef PT_R4
/* now glib always includes signal.h and on linux PPC
 * signal.h defines PT_R4
*/
#undef PT_R4
#endif

#define DCERPC_UUID_NULL { 0,0,0, {0,0,0,0,0,0,0,0} }

/* %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x */
#define DCERPC_UUID_STR_LEN 36+1

typedef struct _e_ctx_hnd {
    guint32 attributes;
    e_guid_t uuid;
} e_ctx_hnd;

typedef struct _e_dce_cn_common_hdr_t {
    guint8 rpc_ver;
    guint8 rpc_ver_minor;
    guint8 ptype;
    guint8 flags;
    guint8 drep[4];
    guint16 frag_len;
    guint16 auth_len;
    guint32 call_id;
} e_dce_cn_common_hdr_t;

typedef struct _e_dce_dg_common_hdr_t {
    guint8 rpc_ver;
    guint8 ptype;
    guint8 flags1;
    guint8 flags2;
    guint8 drep[3];
    guint8 serial_hi;
    e_guid_t obj_id;
    e_guid_t if_id;
    e_guid_t act_id;
    guint32 server_boot;
    guint32 if_ver;
    guint32 seqnum;
    guint16 opnum;
    guint16 ihint;
    guint16 ahint;
    guint16 frag_len;
    guint16 frag_num;
    guint8 auth_proto;
    guint8 serial_lo;
} e_dce_dg_common_hdr_t;

typedef struct _dcerpc_auth_info {
  guint8 auth_pad_len;
  guint8 auth_level;
  guint8 auth_type;
  guint32 auth_size;
  tvbuff_t *auth_data;
} dcerpc_auth_info;

typedef struct dcerpcstat_tap_data
{
	const char *prog;
	e_guid_t uuid;
	guint16 ver;
	int num_procedures;
} dcerpcstat_tap_data_t;

/* Private data passed to subdissectors from the main DCERPC dissector.
 * One unique instance of this structure is created for each
 * DCERPC request/response transaction when we see the initial request
 * of the transaction.
 * These instances are persistent and will remain available until the
 * capture file is closed and a new one is read.
 *
 * For transactions where we never saw the request (missing from the trace)
 * the dcerpc runtime will create a temporary "fake" such structure to pass
 * to the response dissector. These fake structures are not persistent
 * and can not be used to keep data hanging around.
 */
typedef struct _dcerpc_call_value {
    e_guid_t uuid;          /* interface UUID */
    guint16 ver;            /* interface version */
    e_guid_t object_uuid;   /* optional object UUID (or DCERPC_UUID_NULL) */
    guint16 opnum;
    guint32 req_frame;
    nstime_t req_time;
    guint32 rep_frame;
    guint32 max_ptr;
    void *se_data;          /* This holds any data with se allocation scope
                             * that we might want to keep
                             * for this request/response transaction.
                             * The pointer is initialized to NULL and must be
                             * checked before being dereferenced.
                             * This is useful for such things as when we
                             * need to pass persistent data from the request
                             * to the reply, such as LSA/OpenPolicy2() that
                             * uses this to pass the domain name from the
                             * request to the reply.
                             */
    void *private_data;      /* XXX This will later be renamed as ep_data */
    e_ctx_hnd *pol;	     /* policy handle tracked between request/response*/
#define DCERPC_IS_NDR64 0x00000001
    guint32 flags;	     /* flags for this transaction */
} dcerpc_call_value;

typedef struct _dcerpc_info {
	conversation_t *conv;	/* Which TCP stream we are in */
	guint32 call_id;	/* Call ID for this call */
	guint64 transport_salt; /* e.g. FID for DCERPC over SMB */
	guint8 ptype;       /* packet type: PDU_REQ, PDU_RESP, ... */
	gboolean conformant_run;
	gboolean no_align; /* are data aligned? (default yes) */
	gint32 conformant_eaten; /* how many bytes did the conformant run eat?*/
	guint32 array_max_count;	/* max_count for conformant arrays */
	guint32 array_max_count_offset;
	guint32 array_offset;
	guint32 array_offset_offset;
	guint32 array_actual_count;
	guint32 array_actual_count_offset;
	int hf_index;
	dcerpc_call_value *call_data;
    const char *dcerpc_procedure_name;	/* Used by PIDL to store the name of the current dcerpc procedure */
	void *private_data;
} dcerpc_info;

#define PDU_REQ         0
#define PDU_PING        1
#define PDU_RESP        2
#define PDU_FAULT       3
#define PDU_WORKING     4
#define PDU_NOCALL      5
#define PDU_REJECT      6
#define PDU_ACK         7
#define PDU_CL_CANCEL   8
#define PDU_FACK        9
#define PDU_CANCEL_ACK 10
#define PDU_BIND       11
#define PDU_BIND_ACK   12
#define PDU_BIND_NAK   13
#define PDU_ALTER      14
#define PDU_ALTER_ACK  15
#define PDU_AUTH3      16
#define PDU_SHUTDOWN   17
#define PDU_CO_CANCEL  18
#define PDU_ORPHANED   19
#define PDU_RTS        20

/*
 * helpers for packet-dcerpc.c and packet-dcerpc-ndr.c
 * If you're writing a subdissector, you almost certainly want the
 * NDR functions below.
 */
guint16 dcerpc_tvb_get_ntohs (tvbuff_t *tvb, gint offset, guint8 *drep);
guint32 dcerpc_tvb_get_ntohl (tvbuff_t *tvb, gint offset, guint8 *drep);
void dcerpc_tvb_get_uuid (tvbuff_t *tvb, gint offset, guint8 *drep, e_guid_t *uuid);
WS_DLL_PUBLIC
int dissect_dcerpc_uint8 (tvbuff_t *tvb, gint offset, packet_info *pinfo,
                          proto_tree *tree, guint8 *drep,
                          int hfindex, guint8 *pdata);
WS_DLL_PUBLIC
int dissect_dcerpc_uint16 (tvbuff_t *tvb, gint offset, packet_info *pinfo,
                           proto_tree *tree, guint8 *drep,
                           int hfindex, guint16 *pdata);
WS_DLL_PUBLIC
int dissect_dcerpc_uint32 (tvbuff_t *tvb, gint offset, packet_info *pinfo,
                           proto_tree *tree, guint8 *drep,
                           int hfindex, guint32 *pdata);
WS_DLL_PUBLIC
int dissect_dcerpc_uint64 (tvbuff_t *tvb, gint offset, packet_info *pinfo,
                           proto_tree *tree, dcerpc_info *di, guint8 *drep,
                           int hfindex, guint64 *pdata);
int dissect_dcerpc_float  (tvbuff_t *tvb, gint offset, packet_info *pinfo,
                           proto_tree *tree, guint8 *drep,
                           int hfindex, gfloat *pdata);
int dissect_dcerpc_double (tvbuff_t *tvb, gint offset, packet_info *pinfo,
                           proto_tree *tree, guint8 *drep,
                           int hfindex, gdouble *pdata);
int dissect_dcerpc_time_t (tvbuff_t *tvb, gint offset, packet_info *pinfo,
                           proto_tree *tree, guint8 *drep,
                           int hfindex, guint32 *pdata);
WS_DLL_PUBLIC
int dissect_dcerpc_uuid_t (tvbuff_t *tvb, gint offset, packet_info *pinfo,
                           proto_tree *tree, guint8 *drep,
                           int hfindex, e_guid_t *pdata);

/*
 * NDR routines for subdissectors.
 */
WS_DLL_PUBLIC
int dissect_ndr_uint8 (tvbuff_t *tvb, gint offset, packet_info *pinfo,
                       proto_tree *tree, dcerpc_info *di, guint8 *drep,
                       int hfindex, guint8 *pdata);
int PIDL_dissect_uint8 (tvbuff_t *tvb, gint offset, packet_info *pinfo, proto_tree *tree, dcerpc_info *di, guint8 *drep, int hfindex, guint32 param);
int PIDL_dissect_uint8_val (tvbuff_t *tvb, gint offset, packet_info *pinfo, proto_tree *tree, dcerpc_info *di, guint8 *drep, int hfindex, guint32 param, guint8 *pval);
WS_DLL_PUBLIC
int dissect_ndr_uint16 (tvbuff_t *tvb, gint offset, packet_info *pinfo,
                        proto_tree *tree, dcerpc_info *di, guint8 *drep,
                        int hfindex, guint16 *pdata);
int PIDL_dissect_uint16 (tvbuff_t *tvb, gint offset, packet_info *pinfo, proto_tree *tree, dcerpc_info *di, guint8 *drep, int hfindex, guint32 param);
int PIDL_dissect_uint16_val (tvbuff_t *tvb, gint offset, packet_info *pinfo, proto_tree *tree, dcerpc_info *di, guint8 *drep, int hfindex, guint32 param, guint16 *pval);
WS_DLL_PUBLIC
int dissect_ndr_uint32 (tvbuff_t *tvb, gint offset, packet_info *pinfo,
                        proto_tree *tree, dcerpc_info *di, guint8 *drep,
                        int hfindex, guint32 *pdata);
int PIDL_dissect_uint32 (tvbuff_t *tvb, gint offset, packet_info *pinfo, proto_tree *tree, dcerpc_info *di, guint8 *drep, int hfindex, guint32 param);
int PIDL_dissect_uint32_val (tvbuff_t *tvb, gint offset, packet_info *pinfo, proto_tree *tree, dcerpc_info *di, guint8 *drep, int hfindex, guint32 param, guint32 *rval);
WS_DLL_PUBLIC
int dissect_ndr_duint32 (tvbuff_t *tvb, gint offset, packet_info *pinfo,
                        proto_tree *tree, dcerpc_info *di, guint8 *drep,
                        int hfindex, guint64 *pdata);
WS_DLL_PUBLIC
int dissect_ndr_uint64 (tvbuff_t *tvb, gint offset, packet_info *pinfo,
                        proto_tree *tree, dcerpc_info *di, guint8 *drep,
                        int hfindex, guint64 *pdata);
int PIDL_dissect_uint64 (tvbuff_t *tvb, gint offset, packet_info *pinfo, proto_tree *tree, dcerpc_info *di, guint8 *drep, int hfindex, guint32 param);
int PIDL_dissect_uint64_val (tvbuff_t *tvb, gint offset, packet_info *pinfo, proto_tree *tree, dcerpc_info *di, guint8 *drep, int hfindex, guint32 param, guint64 *pval);
WS_DLL_PUBLIC
int dissect_ndr_float (tvbuff_t *tvb, gint offset, packet_info *pinfo,
                        proto_tree *tree, dcerpc_info *di, guint8 *drep,
                        int hfindex, gfloat *pdata);
WS_DLL_PUBLIC
int dissect_ndr_double (tvbuff_t *tvb, gint offset, packet_info *pinfo,
                        proto_tree *tree, dcerpc_info *di, guint8 *drep,
                        int hfindex, gdouble *pdata);

WS_DLL_PUBLIC
int dissect_ndr_time_t (tvbuff_t *tvb, gint offset, packet_info *pinfo,
                        proto_tree *tree, dcerpc_info *di, guint8 *drep,
                        int hfindex, guint32 *pdata);
WS_DLL_PUBLIC
int dissect_ndr_uuid_t (tvbuff_t *tvb, gint offset, packet_info *pinfo,
                        proto_tree *tree, dcerpc_info *di, guint8 *drep,
                        int hfindex, e_guid_t *pdata);
int dissect_ndr_ctx_hnd (tvbuff_t *tvb, gint offset, packet_info *pinfo,
                        proto_tree *tree, dcerpc_info *di, guint8 *drep,
                        int hfindex, e_ctx_hnd *pdata);

#define FT_UINT1632 FT_UINT32
typedef guint32 guint1632;

WS_DLL_PUBLIC
int dissect_ndr_uint1632 (tvbuff_t *tvb, gint offset, packet_info *pinfo,
		        proto_tree *tree, dcerpc_info *di, guint8 *drep,
		        int hfindex, guint1632 *pdata);

typedef guint64 guint3264;

WS_DLL_PUBLIC
int dissect_ndr_uint3264 (tvbuff_t *tvb, gint offset, packet_info *pinfo,
		        proto_tree *tree, dcerpc_info *di, guint8 *drep,
		        int hfindex, guint3264 *pdata);

typedef int (dcerpc_dissect_fnct_t)(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, dcerpc_info *di, guint8 *drep);
typedef int (dcerpc_dissect_fnct_blk_t)(tvbuff_t *tvb, int offset, int length, packet_info *pinfo, proto_tree *tree, dcerpc_info *di, guint8 *drep);

typedef void (dcerpc_callback_fnct_t)(packet_info *pinfo, proto_tree *tree, proto_item *item, dcerpc_info *di, tvbuff_t *tvb, int start_offset, int end_offset, void *callback_args);

#define NDR_POINTER_REF		1
#define NDR_POINTER_UNIQUE	2
#define NDR_POINTER_PTR		3

int dissect_ndr_pointer_cb(tvbuff_t *tvb, gint offset, packet_info *pinfo,
			   proto_tree *tree, dcerpc_info *di, guint8 *drep,
			   dcerpc_dissect_fnct_t *fnct, int type, const char *text,
			   int hf_index, dcerpc_callback_fnct_t *callback,
			   void *callback_args);

int dissect_ndr_pointer(tvbuff_t *tvb, gint offset, packet_info *pinfo,
			proto_tree *tree, dcerpc_info *di, guint8 *drep,
			dcerpc_dissect_fnct_t *fnct, int type, const char *text,
			int hf_index);
int dissect_deferred_pointers(packet_info *pinfo, tvbuff_t *tvb, int offset, dcerpc_info *di, guint8 *drep);
int dissect_ndr_embedded_pointer(tvbuff_t *tvb, gint offset, packet_info *pinfo,
			proto_tree *tree, dcerpc_info *di, guint8 *drep,
			dcerpc_dissect_fnct_t *fnct, int type, const char *text,
			int hf_index);
int dissect_ndr_toplevel_pointer(tvbuff_t *tvb, gint offset, packet_info *pinfo,
			proto_tree *tree, dcerpc_info *di, guint8 *drep,
			dcerpc_dissect_fnct_t *fnct, int type, const char *text,
			int hf_index);

/* dissect a NDR unidimensional conformant array */
int dissect_ndr_ucarray(tvbuff_t *tvb, gint offset, packet_info *pinfo,
                        proto_tree *tree, dcerpc_info *di, guint8 *drep,
                        dcerpc_dissect_fnct_t *fnct);

int dissect_ndr_ucarray_block(tvbuff_t *tvb, gint offset, packet_info *pinfo,
                              proto_tree *tree, dcerpc_info *di, guint8 *drep,
                              dcerpc_dissect_fnct_blk_t *fnct);

/* dissect a NDR unidimensional conformant and varying array
 * each byte in the array is processed separately
 */
int dissect_ndr_ucvarray(tvbuff_t *tvb, gint offset, packet_info *pinfo,
                         proto_tree *tree, dcerpc_info *di, guint8 *drep,
                         dcerpc_dissect_fnct_t *fnct);

int dissect_ndr_ucvarray_block(tvbuff_t *tvb, gint offset, packet_info *pinfo,
                               proto_tree *tree, dcerpc_info *di, guint8 *drep,
                               dcerpc_dissect_fnct_blk_t *fnct);

/* dissect a NDR unidimensional varying array */
int dissect_ndr_uvarray(tvbuff_t *tvb, gint offset, packet_info *pinfo,
                        proto_tree *tree, dcerpc_info *di, guint8 *drep,
                        dcerpc_dissect_fnct_t *fnct);

int dissect_ndr_byte_array(tvbuff_t *tvb, int offset, packet_info *pinfo,
                           proto_tree *tree, dcerpc_info *di, guint8 *drep);

int dissect_ndr_cvstring(tvbuff_t *tvb, int offset, packet_info *pinfo,
			 proto_tree *tree, dcerpc_info *di, guint8 *drep, int size_is,
			 int hfinfo, gboolean add_subtree,
			 char **data);
int dissect_ndr_char_cvstring(tvbuff_t *tvb, int offset, packet_info *pinfo,
                           proto_tree *tree, dcerpc_info *di, guint8 *drep);
int dissect_ndr_wchar_cvstring(tvbuff_t *tvb, int offset, packet_info *pinfo,
                            proto_tree *tree, dcerpc_info *di, guint8 *drep);
int PIDL_dissect_cvstring(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, dcerpc_info *di, guint8 *drep, int chsize, int hfindex, guint32 param);

int dissect_ndr_cstring(tvbuff_t *tvb, int offset, packet_info *pinfo,
                        proto_tree *tree, dcerpc_info *di, guint8 *drep, int size_is,
                        int hfindex, gboolean add_subtree, char **data);
int dissect_ndr_vstring(tvbuff_t *tvb, int offset, packet_info *pinfo,
			 proto_tree *tree, dcerpc_info *di, guint8 *drep, int size_is,
			 int hfinfo, gboolean add_subtree,
			 char **data);
int dissect_ndr_char_vstring(tvbuff_t *tvb, int offset, packet_info *pinfo,
                           proto_tree *tree, dcerpc_info *di, guint8 *drep);
int dissect_ndr_wchar_vstring(tvbuff_t *tvb, int offset, packet_info *pinfo,
                            proto_tree *tree, dcerpc_info *di, guint8 *drep);

typedef struct _dcerpc_sub_dissector {
    guint16 num;
    const gchar   *name;
    dcerpc_dissect_fnct_t *dissect_rqst;
    dcerpc_dissect_fnct_t *dissect_resp;
} dcerpc_sub_dissector;

/* registration function for subdissectors */
WS_DLL_PUBLIC
void dcerpc_init_uuid (int proto, int ett, e_guid_t *uuid, guint16 ver, dcerpc_sub_dissector *procs, int opnum_hf);
WS_DLL_PUBLIC
const char *dcerpc_get_proto_name(e_guid_t *uuid, guint16 ver);
WS_DLL_PUBLIC
int dcerpc_get_proto_hf_opnum(e_guid_t *uuid, guint16 ver);
WS_DLL_PUBLIC
dcerpc_sub_dissector *dcerpc_get_proto_sub_dissector(e_guid_t *uuid, guint16 ver);

/* Create a opnum, name value_string from a subdissector list */

value_string *value_string_from_subdissectors(dcerpc_sub_dissector *sd);

/* Decode As... functionality */
/* remove all bindings */
WS_DLL_PUBLIC void decode_dcerpc_reset_all(void);
typedef void (*decode_add_show_list_func)(gpointer data, gpointer user_data);
WS_DLL_PUBLIC void decode_dcerpc_add_show_list(decode_add_show_list_func func, gpointer user_data);


/* the registered subdissectors. With MSVC and a
 * libwireshark.dll, we need a special declaration.
 */
/* Key: guid_key *
 * Value: dcerpc_uuid_value *
 */
WS_DLL_PUBLIC GHashTable *dcerpc_uuids;

typedef struct _dcerpc_uuid_value {
    protocol_t *proto;
    int proto_id;
    int ett;
    const gchar *name;
    dcerpc_sub_dissector *procs;
    int opnum_hf;
} dcerpc_uuid_value;

/* Authenticated pipe registration functions and miscellanea */

typedef tvbuff_t *(dcerpc_decode_data_fnct_t)(tvbuff_t *data_tvb,
					      tvbuff_t *auth_tvb,
					      int offset,
					      packet_info *pinfo,
					      dcerpc_auth_info *auth_info);

typedef struct _dcerpc_auth_subdissector_fns {

	/* Dissect credentials and verifiers */

	dcerpc_dissect_fnct_t *bind_fn;
	dcerpc_dissect_fnct_t *bind_ack_fn;
	dcerpc_dissect_fnct_t *auth3_fn;
	dcerpc_dissect_fnct_t *req_verf_fn;
	dcerpc_dissect_fnct_t *resp_verf_fn;

	/* Decrypt encrypted requests/response PDUs */

	dcerpc_decode_data_fnct_t *req_data_fn;
	dcerpc_decode_data_fnct_t *resp_data_fn;

} dcerpc_auth_subdissector_fns;

void register_dcerpc_auth_subdissector(guint8 auth_level, guint8 auth_type,
				       dcerpc_auth_subdissector_fns *fns);

/* all values needed to (re-)build a dcerpc binding */
typedef struct decode_dcerpc_bind_values_s {
    /* values of a typical conversation */
    address addr_a;
    address addr_b;
    port_type ptype;
    guint32 port_a;
    guint32 port_b;
    /* dcerpc conversation specific */
    guint16 ctx_id;
    guint64 transport_salt;
    /* corresponding "interface" */
    GString *ifname;
    e_guid_t uuid;
    guint16 ver;
} decode_dcerpc_bind_values_t;

WS_DLL_PUBLIC guint64 dcerpc_get_transport_salt(packet_info *pinfo);
WS_DLL_PUBLIC void dcerpc_set_transport_salt(guint64 dcetransportsalt, packet_info *pinfo);

/* Authentication services */

/*
 * For MS-specific SSPs (Security Service Provider), see
 *
 * http://msdn.microsoft.com/library/en-us/rpc/rpc/authentication_level_constants.asp
 */

#define DCE_C_RPC_AUTHN_PROTOCOL_NONE		0
#define DCE_C_RPC_AUTHN_PROTOCOL_KRB5		1
#define DCE_C_RPC_AUTHN_PROTOCOL_SPNEGO         9
#define DCE_C_RPC_AUTHN_PROTOCOL_NTLMSSP	10
#define DCE_C_RPC_AUTHN_PROTOCOL_GSS_SCHANNEL	14
#define DCE_C_RPC_AUTHN_PROTOCOL_GSS_KERBEROS	16
#define DCE_C_RPC_AUTHN_PROTOCOL_DPA		17
#define DCE_C_RPC_AUTHN_PROTOCOL_MSN		18
#define DCE_C_RPC_AUTHN_PROTOCOL_DIGEST		21
#define DCE_C_RPC_AUTHN_PROTOCOL_SEC_CHAN       68
#define DCE_C_RPC_AUTHN_PROTOCOL_MQ		100

/* Protection levels */

#define DCE_C_AUTHN_LEVEL_NONE		1
#define DCE_C_AUTHN_LEVEL_CONNECT	2
#define DCE_C_AUTHN_LEVEL_CALL		3
#define DCE_C_AUTHN_LEVEL_PKT		4
#define DCE_C_AUTHN_LEVEL_PKT_INTEGRITY	5
#define DCE_C_AUTHN_LEVEL_PKT_PRIVACY	6

void
init_ndr_pointer_list(dcerpc_info *di);



/* These defines are used in the PIDL conformance files when using
 * the PARAM_VALUE directive.
 */
/* Policy handle tracking. Describes in which function a handle is
 * opened/closed.  See "winreg.cnf" for example.
 *
 * The guint32 param is divided up into multiple fields
 *
 * +--------+--------+--------+--------+
 * | Flags  | Type   |        |        |
 * +--------+--------+--------+--------+
 */
/* Flags : */
#define PIDL_POLHND_OPEN		0x80000000
#define PIDL_POLHND_CLOSE		0x40000000
/* To "save" a pointer to the string in dcv->private_data */
#define PIDL_STR_SAVE			0x20000000
/* To make this value appear on the summary line for the packet */
#define PIDL_SET_COL_INFO		0x10000000

/* Type */
#define PIDL_POLHND_TYPE_MASK		0x00ff0000
#define PIDL_POLHND_TYPE_SAMR_USER	0x00010000
#define PIDL_POLHND_TYPE_SAMR_CONNECT	0x00020000
#define PIDL_POLHND_TYPE_SAMR_DOMAIN	0x00030000
#define PIDL_POLHND_TYPE_SAMR_GROUP	0x00040000
#define PIDL_POLHND_TYPE_SAMR_ALIAS	0x00050000

#define PIDL_POLHND_TYPE_LSA_POLICY	0x00060000
#define PIDL_POLHND_TYPE_LSA_ACCOUNT	0x00070000
#define PIDL_POLHND_TYPE_LSA_SECRET	0x00080000
#define PIDL_POLHND_TYPE_LSA_DOMAIN	0x00090000

/* a structure we store for all policy handles we track */
typedef struct pol_value {
	struct pol_value *next;          /* Next entry in hash bucket */
	guint32 open_frame, close_frame; /* Frame numbers for open/close */
	guint32 first_frame;             /* First frame in which this instance was seen */
	guint32 last_frame;              /* Last frame in which this instance was seen */
	char *name;			 /* Name of policy handle */
	guint32 type;			 /* policy handle type */
} pol_value;


extern int hf_dcerpc_drep_byteorder;
extern int hf_dcerpc_ndr_padding;

#define FAKE_DCERPC_INFO_STRUCTURE      \
    /* Fake dcerpc_info structure */    \
    dcerpc_info di;                     \
    dcerpc_call_value call_data;        \
                                        \
    di.conformant_run = FALSE;          \
    di.no_align = TRUE;                 \
                                        \
	/* we need di->call_data->flags.NDR64 == 0 */  \
    call_data.flags = 0;                \
	di.call_data = &call_data;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* packet-dcerpc.h */
