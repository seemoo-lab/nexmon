/* packet-dcom.c
 * Routines for DCOM generics
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/* A few words about DCOM:
 *
 * DCOM uses DCERPC as its underlying "transport" protocol.
 *
 * The DCOM dissectors are called by DCERPC request and response calls.
 * DCOM uses a small header after the DCERPC calls.
 * (for a DCERPC request  call it's called "this",
 *  for a DCERPC response call it's called "that")
 *
 * DCOM defines itself some interfaces: oxid, remact, remunk and others
 *
 * Implemented is currently "only" a static dissection of packet fields
 * (no "object recognition" included)
 *
 * User's of DCOM can define their own interface's using Microsoft IDL.
 *
 * Hint: The registered DCOM interface names can be found in the
 * windows registry at: "HKEY_CLASSES_ROOT\Interface"
 *
 *
 * Resources on the web:
 *
 * "Understanding the DCOM Wire Protocol by Analyzing Network Data Packets"
 * http:// www.microsoft.com/msj/0398/dcom.htm
 *
 * "Distributed Component Object Model Protocol -- DCOM/1.0"
 * http://www.microsoft.com/com/resources/specs.asp (link is currently down)
 *
 */

/* Files involved dissecting DCOM:
 *
 * packet-dcom.c: generic DCOM things (this, that, ...) and
 *    generic DCOM datatype (DWORD, VARIANT, ...)
 *
 * DCOM common Interfaces:
 * packet-dcom-oxid.c:	   IOXIDResolver
 * packet-dcom-remact.c:   IRemoteActivation
 * packet-dcom-remunk.c:   IRemUnknown, IRemUnknown2
 * packet-dcom-dispatch.c: IDispatch
 * packet-dcom-sysact.c:   ISystemActivator
 */

#include "config.h"

#include <string.h>

#include <epan/packet.h>
#include <epan/exceptions.h>
#include <epan/addr_resolv.h>
#include <wsutil/inet_aton.h>
#include <epan/expert.h>
#include <epan/prefs.h>
#include "packet-dcerpc.h"
#include "packet-dcom.h"

void proto_register_dcom (void);
void proto_reg_handoff_dcom (void);

static int proto_dcom = -1;

gboolean dcom_prefs_display_unmarshalling_details = FALSE;


static gint ett_dcom_this = -1;
/* static int hf_dcom_this_version_major = -1; */
/* static int hf_dcom_this_version_minor = -1; */
static int hf_dcom_this_flags = -1;
static int hf_dcom_this_res = -1;
static int hf_dcom_this_cid = -1;

static gint ett_dcom_that = -1;
static int hf_dcom_that_flags = -1;

static gint ett_dcom_extent = -1;
static int hf_dcom_extent = -1;
static int hf_dcom_extent_array_count = -1;
static int hf_dcom_extent_array_res = -1;
static int hf_dcom_extent_size = -1;
static int hf_dcom_extent_id = -1;

static int hf_dcom_hresult = -1;
static int hf_dcom_tobedone = -1;
static int hf_dcom_nospec = -1;
static int hf_dcom_array_size = -1;
static int hf_dcom_pointer_val = -1;

/* COMVERSION */
static int hf_dcom_version_major = -1;
static int hf_dcom_version_minor = -1;

static gint ett_dcom_lpwstr = -1;
static int hf_dcom_max_count = -1;
static int hf_dcom_offset = -1;
static int hf_dcom_byte_length = -1;
/* static int hf_dcom_actual_count = -1; */

static gint ett_dcom_objref = -1;
static int hf_dcom_objref = -1;
static int hf_dcom_objref_signature = -1;
static int hf_dcom_objref_flags = -1;
int hf_dcom_iid = -1;
int hf_dcom_clsid = -1;
static int hf_dcom_objref_resolver_address = -1;
static int hf_dcom_objref_cbextension = -1;
static int hf_dcom_objref_size = -1;

static gint ett_dcom_stdobjref = -1;
static int hf_dcom_stdobjref = -1;
static int hf_dcom_stdobjref_flags = -1;
static int hf_dcom_stdobjref_public_refs = -1;
int hf_dcom_oxid = -1;
int hf_dcom_oid = -1;
int hf_dcom_ipid = -1;


static gint ett_dcom_custobjref = -1;
static int hf_dcom_custobjref = -1;

static gint ett_dcom_dualstringarray = -1;
static gint ett_dcom_dualstringarray_binding = -1;
static int hf_dcom_dualstringarray_num_entries = -1;
static int hf_dcom_dualstringarray_security_offset = -1;
static int hf_dcom_dualstringarray_string = -1;
static int hf_dcom_dualstringarray_string_network_addr = -1;
static int hf_dcom_dualstringarray_string_tower_id = -1;
static int hf_dcom_dualstringarray_security = -1;
static int hf_dcom_dualstringarray_security_authn_svc = -1;
static int hf_dcom_dualstringarray_security_authz_svc = -1;
static int hf_dcom_dualstringarray_security_princ_name = -1;

static gint ett_dcom_interface_pointer = -1;
static int hf_dcom_interface_pointer = -1;
static int hf_dcom_ip_cnt_data = -1;

static gint ett_dcom_safearray = -1;
static int hf_dcom_safearray = -1;
static int hf_dcom_sa_dims32 = -1;
static int hf_dcom_sa_dims16 = -1;
static int hf_dcom_sa_features = -1;
static int hf_dcom_sa_element_size = -1;
static int hf_dcom_sa_locks = -1;
static int hf_dcom_sa_vartype32 = -1;
static int hf_dcom_sa_vartype16 = -1;
static int hf_dcom_sa_elements = -1;
static int hf_dcom_sa_bound_elements = -1;
static int hf_dcom_sa_low_bound = -1;

static gint ett_dcom_sa_features = -1;
static int hf_dcom_sa_features_auto = -1;
static int hf_dcom_sa_features_static = -1;
static int hf_dcom_sa_features_embedded = -1;
static int hf_dcom_sa_features_fixedsize = -1;
static int hf_dcom_sa_features_record = -1;
static int hf_dcom_sa_features_have_iid = -1;
static int hf_dcom_sa_features_have_vartype = -1;
static int hf_dcom_sa_features_bstr = -1;
static int hf_dcom_sa_features_unknown = -1;
static int hf_dcom_sa_features_dispatch = -1;
static int hf_dcom_sa_features_variant = -1;

static gint ett_dcom_variant = -1;
/* static int hf_dcom_variant = -1; */
static int hf_dcom_variant_type = -1;
static int hf_dcom_variant_size = -1;
static int hf_dcom_variant_rpc_res = -1;
static int hf_dcom_variant_wres = -1;
static int hf_dcom_variant_type32 = -1;

static int hf_dcom_vt_bool = -1;
static int hf_dcom_vt_i1 = -1;
static int hf_dcom_vt_i2 = -1;
static int hf_dcom_vt_i4 = -1;
static int hf_dcom_vt_i8 = -1;	/* only inside a SAFEARRAY, not in VARIANTs */
static int hf_dcom_vt_cy = -1;
static int hf_dcom_vt_ui1 = -1;
static int hf_dcom_vt_ui2 = -1;
static int hf_dcom_vt_ui4 = -1;
static int hf_dcom_vt_ui8 = -1;
static int hf_dcom_vt_r4 = -1;
static int hf_dcom_vt_r8 = -1;
static int hf_dcom_vt_date = -1;
static int hf_dcom_vt_bstr = -1;
static int hf_dcom_vt_byref = -1;
static int hf_dcom_vt_dispatch = -1;

static expert_field ei_dcom_dissetion_incomplete = EI_INIT;
static expert_field ei_dcom_no_spec = EI_INIT;
static expert_field ei_dcom_hresult_expert = EI_INIT;
static expert_field ei_dcom_dualstringarray_mult_ip = EI_INIT;

/* this/that extension UUIDs */
static e_guid_t uuid_debug_ext =    { 0xf1f19680, 0x4d2a, 0x11ce, { 0xa6, 0x6a, 0x00, 0x20, 0xaf, 0x6e, 0x72, 0xf4} };
static e_guid_t uuid_ext_error_ext ={ 0xf1f19681, 0x4d2a, 0x11ce, { 0xa6, 0x6a, 0x00, 0x20, 0xaf, 0x6e, 0x72, 0xf4} };

/* general DCOM UUIDs */
static const e_guid_t ipid_rem_unknown =  { 0x00000131, 0x1234, 0x5678, { 0xCA, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46} };
static const e_guid_t iid_unknown =	  { 0x00000000, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46} };
static const e_guid_t uuid_null =	  { 0x00000000, 0x0000, 0x0000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} };
static const e_guid_t iid_class_factory = { 0x00000001, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46} };
#if 0
static const e_guid_t iid_act_prop_in =   { 0x000001A2, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46} };
static const e_guid_t iid_act_prop_out =  { 0x000001A3, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46} };
#endif

static GList *dcom_machines   = NULL;
static GList *dcom_interfaces = NULL;
static GList *dcom_marshalers = NULL;

static const value_string dcom_thisthat_flag_vals[] = {
	{ 0, "INFO_NULL" },
	{ 1, "INFO_LOCAL" },
	{ 0,  NULL }
};

#if 0
static const value_string dcom_boolean_flag_vals[] = {
	{ 0x00000001, "TRUE" },
	{ 0x00000000, "FALSE" },
	{ 0,  NULL }
};
#endif

#ifdef DEBUG
void dcom_interface_dump(void) {
	dcom_machine_t *machine;
	dcom_object_t *object;
	dcom_interface_t *interf;
	GList *machines;
	GList *objects;
	GList *interfaces;


	for(machines = dcom_machines; machines != NULL; machines = g_list_next(machines)) {
		machine = (dcom_machine_t *)machines->data;
		/*g_warning("Machine(#%4u): IP:%s", machine->first_packet, address_to_str(wmem_packet_scope(), &machine->ip));*/

		for(objects = machine->objects; objects != NULL; objects = g_list_next(objects)) {
			object = (dcom_object_t *)objects->data;
			/*g_warning(" Object(#%4u): OID:0x%" G_GINT64_MODIFIER "x private:%p", object->first_packet, object->oid, object->private_data);*/

			for(interfaces = object->interfaces; interfaces != NULL; interfaces = g_list_next(interfaces)) {
				interf = (dcom_interface_t *)interfaces->data;
				/*g_warning("  Interface(#%4u): iid:%s",
					  interf->first_packet, guids_resolve_guid_to_str(&interf->iid));
				g_warning("   ipid:%s", guids_resolve_guid_to_str(&interf->ipid));*/
			}
		}
	}
}
#endif

dcom_interface_t *dcom_interface_find(packet_info *pinfo _U_, const address *addr _U_, e_guid_t *ipid)
{
	dcom_interface_t *interf;
	GList *interfaces;


	if(memcmp(ipid, &uuid_null, sizeof(uuid_null)) == 0)
	{
		return NULL;
	}

	for(interfaces = dcom_interfaces; interfaces != NULL; interfaces = g_list_next(interfaces)) {
		interf = (dcom_interface_t *)interfaces->data;

		if(memcmp(&interf->ipid, ipid, sizeof(e_guid_t)) == 0) {
			return interf;
		}
	}

	return NULL;
}


dcom_interface_t *dcom_interface_new(packet_info *pinfo, const address *addr, e_guid_t *iid, guint64 oxid, guint64 oid, e_guid_t *ipid)
{
	GList *dcom_iter;
	dcom_machine_t *machine;
	dcom_object_t *object;
	dcom_interface_t *interf;


	if( memcmp(iid, &uuid_null, sizeof(uuid_null)) == 0 ||
	    memcmp(ipid, &uuid_null, sizeof(uuid_null)) == 0)
	{
		return NULL;
	}

	if(oxid == 0 || oid == 0) {
		/*g_warning("interface_new#%u", pinfo->num);*/

		interf = wmem_new(wmem_file_scope(), dcom_interface_t);
		interf->parent = NULL;
		interf->private_data = NULL;
		interf->first_packet = pinfo->num;
		interf->iid = *iid;
		interf->ipid = *ipid;

		dcom_interfaces = g_list_append(dcom_interfaces, interf);
		return interf;
	}

	/* find machine */
	dcom_iter = dcom_machines;
	while(dcom_iter != NULL) {
		machine = (dcom_machine_t *)dcom_iter->data;
		if(cmp_address(&machine->ip, addr) == 0) {
			break;
		}
		dcom_iter = g_list_next(dcom_iter);
	}

	/* create new machine if not found */
	if(dcom_iter == NULL) {
		machine = g_new(dcom_machine_t,1);
		copy_address(&machine->ip, addr);
		machine->objects = NULL;
		machine->first_packet = pinfo->num;
		dcom_machines = g_list_append(dcom_machines, machine);
	}

	/* find object */
	dcom_iter = machine->objects;
	while(dcom_iter != NULL) {
		object = (dcom_object_t *)dcom_iter->data;
		if(object->oid == oid) {
			break;
		}
		dcom_iter = g_list_next(dcom_iter);
	}

	/* create new object if not found */
	if(dcom_iter == NULL) {
		object = g_new(dcom_object_t,1);
		object->parent = machine;
		object->interfaces = NULL;
		object->private_data = NULL;
		object->first_packet = pinfo->num;
		object->oid = oid;
		object->oxid = oxid;

		machine->objects = g_list_append(machine->objects, object);
	}

	/* find interface */
	dcom_iter = object->interfaces;
	while(dcom_iter != NULL) {
		interf = (dcom_interface_t *)dcom_iter->data;
		if(memcmp(&interf->ipid, ipid, sizeof(e_guid_t)) == 0) {
			break;
		}
		dcom_iter = g_list_next(dcom_iter);
	}

	/* create new interface if not found */
	if(dcom_iter == NULL) {
		interf = g_new(dcom_interface_t,1);
		interf->parent = object;
		interf->private_data = NULL;
		interf->first_packet = pinfo->num;
		interf->iid = *iid;
		interf->ipid = *ipid;

		object->interfaces = g_list_append(object->interfaces, interf);
		dcom_interfaces = g_list_append(dcom_interfaces, interf);
	}

	return interf;
}


/*
 * Flag bits in connection-oriented PDU header.
 */
#define WIRESHARK_FADF_AUTO		0x0001
#define WIRESHARK_FADF_STATIC		0x0002
#define WIRESHARK_FADF_EMBEDDED		0x0004
#define WIRESHARK_FADF_FIXEDSIZE	0x0010
#define WIRESHARK_FADF_RECORD		0x0020
#define WIRESHARK_FADF_HAVEIID		0x0040
#define WIRESHARK_FADF_HAVEVARTYPE	0x0080
#define WIRESHARK_FADF_BSTR		0x0100
#define WIRESHARK_FADF_UNKNOWN		0x0200
#define WIRESHARK_FADF_DISPATCH		0x0400
#define WIRESHARK_FADF_VARIANT		0x0800


typedef enum {
	WIRESHARK_VT_EMPTY	     = 0,
	WIRESHARK_VT_NULL	     = 1,
	WIRESHARK_VT_I2		     = 2,
	WIRESHARK_VT_I4		     = 3,
	WIRESHARK_VT_R4		     = 4,
	WIRESHARK_VT_R8		     = 5,
	WIRESHARK_VT_CY		     = 6,
	WIRESHARK_VT_DATE	     = 7,
	WIRESHARK_VT_BSTR	     = 8,
	WIRESHARK_VT_DISPATCH	     = 9,
	WIRESHARK_VT_ERROR	     = 10,
	WIRESHARK_VT_BOOL	     = 11,
	WIRESHARK_VT_VARIANT	     = 12,
	WIRESHARK_VT_UNKNOWN	     = 13,
	WIRESHARK_VT_DECIMAL	     = 14,
	WIRESHARK_VT_I1		     = 16,
	WIRESHARK_VT_UI1	     = 17,
	WIRESHARK_VT_UI2	     = 18,
	WIRESHARK_VT_UI4	     = 19,
	WIRESHARK_VT_I8		     = 20,
	WIRESHARK_VT_UI8	     = 21,
	WIRESHARK_VT_INT	     = 22,
	WIRESHARK_VT_UINT	     = 23,
	WIRESHARK_VT_VOID	     = 24,
	WIRESHARK_VT_HRESULT	     = 25,
	WIRESHARK_VT_PTR	     = 26,
	WIRESHARK_VT_SAFEARRAY	     = 27,
	WIRESHARK_VT_CARRAY	     = 28,
	WIRESHARK_VT_USERDEFINED     = 29,
	WIRESHARK_VT_LPSTR	     = 30,
	WIRESHARK_VT_LPWSTR	     = 31,
	WIRESHARK_VT_RECORD	     = 36,
	WIRESHARK_VT_FILETIME	     = 64,
	WIRESHARK_VT_BLOB	     = 65,
	WIRESHARK_VT_STREAM	     = 66,
	WIRESHARK_VT_STORAGE	     = 67,
	WIRESHARK_VT_STREAMED_OBJECT = 68,
	WIRESHARK_VT_STORED_OBJECT   = 69,
	WIRESHARK_VT_BLOB_OBJECT     = 70,
	WIRESHARK_VT_CF		     = 71,
	WIRESHARK_VT_CLSID	     = 72,

	WIRESHARK_VT_BSTR_BLOB	     = 0x0fff,

	WIRESHARK_VT_VECTOR	     = 0x1000,
	WIRESHARK_VT_ARRAY	     = 0x2000,
	WIRESHARK_VT_BYREF	     = 0x4000,
	WIRESHARK_VT_RESERVED	     = 0x8000,

	WIRESHARK_VT_ILLEGAL	     = 0xffff,
	WIRESHARK_VT_ILLEGALMASKED   = 0x0fff,
	WIRESHARK_VT_TYPEMASK	     = 0x0fff
} dcom_vartype_t;

const value_string dcom_variant_type_vals[] = {
	{ WIRESHARK_VT_EMPTY,		"VT_EMPTY"},
	{ WIRESHARK_VT_NULL,		"VT_NULL"},
	{ WIRESHARK_VT_I2,		"VT_I2"},
	{ WIRESHARK_VT_I4,		"VT_I4"},
	{ WIRESHARK_VT_R4,		"VT_R4"},
	{ WIRESHARK_VT_R8,		"VT_R8"},
	{ WIRESHARK_VT_CY,		"VT_CY"},
	{ WIRESHARK_VT_DATE,		"VT_DATE"},
	{ WIRESHARK_VT_BSTR,		"VT_BSTR"},
	{ WIRESHARK_VT_DISPATCH,	"VT_DISPATCH"},
	{ WIRESHARK_VT_ERROR,		"VT_ERROR"},
	{ WIRESHARK_VT_BOOL,		"VT_BOOL"},
	{ WIRESHARK_VT_I1,		"VT_I1"},
	{ WIRESHARK_VT_UI1,		"VT_UI1"},
	{ WIRESHARK_VT_UI2,		"VT_UI2"},
	{ WIRESHARK_VT_UI4,		"VT_UI4"},
	{ WIRESHARK_VT_I8,		"VT_I8"},
	{ WIRESHARK_VT_UI8,		"VT_UI8"},
	{ WIRESHARK_VT_ARRAY,		"VT_ARRAY"},
	{ WIRESHARK_VT_UNKNOWN,		"VT_UNKNOWN"},
	{ WIRESHARK_VT_USERDEFINED,	"VT_USERDEFINED"},

	/* XXX: this could be done better */
	{ WIRESHARK_VT_ARRAY | WIRESHARK_VT_I2,	     "VT_ARRAY|VT_I2"},
	{ WIRESHARK_VT_ARRAY | WIRESHARK_VT_I4,	     "VT_ARRAY|VT_I4"},
	{ WIRESHARK_VT_ARRAY | WIRESHARK_VT_R4,	     "VT_ARRAY|VT_R4"},
	{ WIRESHARK_VT_ARRAY | WIRESHARK_VT_R8,	     "VT_ARRAY|VT_R8"},
	{ WIRESHARK_VT_ARRAY | WIRESHARK_VT_DATE,    "VT_ARRAY|VT_DATE"},
	{ WIRESHARK_VT_ARRAY | WIRESHARK_VT_BSTR,    "VT_ARRAY|VT_BSTR"},
	{ WIRESHARK_VT_ARRAY | WIRESHARK_VT_ERROR,   "VT_ARRAY|VT_ERROR"},
	{ WIRESHARK_VT_ARRAY | WIRESHARK_VT_BOOL,    "VT_ARRAY|VT_BOOL"},
	{ WIRESHARK_VT_ARRAY | WIRESHARK_VT_I1,	     "VT_ARRAY|VT_I1"},
	{ WIRESHARK_VT_ARRAY | WIRESHARK_VT_UI1,     "VT_ARRAY|VT_UI1"},
	{ WIRESHARK_VT_ARRAY | WIRESHARK_VT_UI2,     "VT_ARRAY|VT_UI2"},
	{ WIRESHARK_VT_ARRAY | WIRESHARK_VT_UI4,     "VT_ARRAY|VT_UI4"},
	{ WIRESHARK_VT_ARRAY | WIRESHARK_VT_I8,	     "VT_ARRAY|VT_I8"},
	{ WIRESHARK_VT_ARRAY | WIRESHARK_VT_UI8,     "VT_ARRAY|VT_UI8"},

	{ WIRESHARK_VT_BYREF | WIRESHARK_VT_I2,	     "VT_BYREF|VT_I2"},
	{ WIRESHARK_VT_BYREF | WIRESHARK_VT_BSTR,    "VT_BYREF|VT_BSTR"},
	{ WIRESHARK_VT_BYREF | WIRESHARK_VT_VARIANT, "VT_BYREF|VT_VARIANT"},
	{ 0,	      NULL }
/* XXX: append more types here */
};



/* we need an extension mechanism here (be able to append entries by user protocol) */
const value_string dcom_hresult_vals[] = {
	{ 0x00000000, "S_OK" },
	{ 0x00000001, "S_FALSE" },
	{ 0x8000FFFF, "E_UNEXPECTED" },
	{ 0x80004001, "E_NOTIMPL" },
	{ 0x80004002, "E_NOINTERFACE" },
	{ 0x80004003, "E_POINTER" },
	{ 0x80004004, "E_ABORT" },
	{ 0x80004005, "E_FAIL" },
	{ 0x80070005, "E_ACCESSDENIED" },
	{ 0x80070006, "E_HANDLE" },
	{ 0x8007000E, "E_OUTOFMEMORY" },
	{ 0x80070057, "E_INVALIDARG" },

	{ 0x80010108, "RPC_E_DISCONNECTED" },
	{ 0x80010113, "RPC_E_INVALID_IPID" },
	{ 0x8001011F, "RPC_E_TIMEOUT" },

	{ 0x80020003, "DISP_E_MEMBERNOTFOUND" },
	{ 0x80020004, "DISP_E_PARAMNOTFOUND" },
	{ 0x80020005, "DISP_E_TYPEMISMATCH" },
	{ 0x80020006, "DISP_E_UNKNOWNNAME" },
	{ 0x80020008, "DISP_E_BADVARTYPE" },
	{ 0x80020009, "DISP_E_EXCEPTION" },
	{ 0x8002000A, "DISP_E_OVERFLOW" },

	{ 0x8002801D, "TYPE_E_LIBNOTREGISTERED" },

	{ 0x80040154, "REGDB_E_CLASSNOTREG" },
	{ 0x80040201, "CO_E_FAILEDTOGETSECCTX" },

/* following are CBA application specific values */
	{ 0x0004CA00, "CBA_S_PERSISTPENDING" },
	{ 0x0004CA01, "CBA_S_ESTABLISHING" },
	{ 0x0004CA02, "CBA_S_NOCONNECTION" },
	{ 0x0004CA03, "CBA_S_VALUEBUFFERED" },
	{ 0x0004CA04, "CBA_S_VALUEUNCERTAIN" },
	{ 0x0004CA05, "CBA_S_NOCONNECTIONDATA" },
	{ 0x0004CA06, "CBA_S_FRAMEEMPTY" },

	{ 0x8004CB00, "CBA_E_MALFORMED" },
	{ 0x8004CB01, "CBA_E_UNKNOWNOBJECT" },
	{ 0x8004CB02, "CBA_E_UNKNOWNMEMBER" },
	{ 0x8004CB03, "CBA_E_TYPEMISMATCH" },
	{ 0x8004CB04, "CBA_E_INVALIDENUMVALUE" },
	{ 0x8004CB05, "CBA_E_INVALIDID" },
	{ 0x8004CB06, "CBA_E_INVALIDEPSILON" },
	{ 0x8004CB07, "CBA_E_INVALIDSUBSTITUTE" },
	{ 0x8004CB08, "CBA_E_INVALIDCONNECTION" },
	{ 0x8004CB09, "CBA_E_INVALIDCOOKIE" },
	{ 0x8004CB0A, "CBA_E_TIMEVALUEUNSUPPORTED" },
	{ 0x8004CB0B, "CBA_E_QOSTYPEUNSUPPORTED" },
	{ 0x8004CB0C, "CBA_E_QOSVALUEUNSUPPORTED" },
	{ 0x8004CB0D, "CBA_E_PERSISTRUNNING" },
	{ 0x8004CB0E, "CBA_E_INUSE" },
	{ 0x8004CB0F, "CBA_E_NOTAPPLICABLE" },
	{ 0x8004CB10, "CBA_E_NONACCESSIBLE" },
	{ 0x8004CB11, "CBA_E_DEFECT" },
	{ 0x8004CB12, "CBA_E_LIMITVIOLATION" },
	{ 0x8004CB13, "CBA_E_QOSTYPENOTAPPLICABLE" },
	{ 0x8004CB14, "CBA_E_QCNOTAPPLICABLE" },
	{ 0x8004CB15, "CBA_E_ACCESSBLOCKED" },
	{ 0x8004CB16, "CBA_E_COUNTEXCEEDED" },
	{ 0x8004CB17, "CBA_E_SIZEEXCEEDED" },
	{ 0x8004CB18, "CBA_E_OUTOFPARTNERACCOS" },
	{ 0x8004CB19, "CBA_E_OUTOFACCOPAIRS" },
	{ 0x8004CB1A, "CBA_E_ITEMTOOLARGE" },
	{ 0x8004CB1B, "CBA_E_CRDATALENGTH" },
	{ 0x8004CB1C, "CBA_E_FLAGUNSUPPORTED" },
	{ 0x8004CB1D, "CBA_E_CAPACITYEXCEEDED" },
	{ 0x8004CB1E, "CBA_E_SUBELEMENTMISMATCH" },
	{ 0x8004CB1F, "CBA_E_STATIONFAILURE" },
	{ 0x8004CB20, "CBA_E_NOTROUTABLE" },
	{ 0x8004CB21, "CBA_E_DISCONNECTRUNNING" },
	{ 0x8004CB22, "CBA_E_LOCATIONCHANGED" },
	{ 0x8004CB23, "CBA_E_FRAMECOUNTUNSUPPORTED" },
	{ 0x8004CB24, "CBA_E_LINKFAILURE" },
	{ 0x8004CB25, "CBA_E_MODECHANGE" },

	{ 0x80080004, "CO_E_BAD_PATH" },

	{ 0,	      NULL }
};

static const value_string dcom_objref_flag_vals[] = {
	{ 0x1, "OBJREF_STANDARD" },
	{ 0x2, "OBJREF_HANDLER" },
	{ 0x4, "OBJREF_CUSTOM" },
	{ 0,   NULL }
};

static const value_string dcom_objref_signature_vals[] = {
	{ 0x574f454d, "MEOW" },
	{ 0,	      NULL }
};

/* although flags, they doesn't seem to be used as such */
static const value_string dcom_stdobjref_flag_vals[] = {
	{ 0x0000, "SORF_NULL" },
	{ 0x0001, "SORF_OXRES1" },
	{ 0x0020, "SORF_OXRES2" },
	{ 0x0040, "SORF_OXRES3" },
	{ 0x0080, "SORF_OXRES4" },
	{ 0x0100, "SORF_OXRES5" },
	{ 0x0200, "SORF_OXRES6" },
	{ 0x0400, "SORF_OXRES7" },
	{ 0x0800, "SORF_OXRES8" },
	{ 0x1000, "SORF_NOPING" },
	{ 0,	  NULL }
};

static const value_string dcom_dcerpc_pointer_vals[] = {
	{ 0x72657355, "User" },
	{ 0x42535452, "BSTR" },
	{ 0x00000000, "NULL" },
	{ 0,	      NULL }
};

static const value_string dcom_dualstringarray_authz[] = {
	{ 0x0000, "RPC_C_AUTHZ_NONE" },
	{ 0x0001, "RPC_C_AUTHZ_NAME"},
	{ 0x0002, "RPC_C_AUTHZ_DCE"},
	{ 0xffff, "Default"},
	{ 0,	  NULL}
};

static const value_string dcom_dualstringarray_authn[] = {
	{  00, "RPC_C_AUTHN_NONE" },
	{   1, "RPC_C_AUTHN_DCE_PRIVATE"},
	{   2, "RPC_C_AUTHN_DCE_PUBLIC"},
	{   4, "RPC_C_AUTHN_DEC_PUBLIC"},
	{   9, "RPC_C_AUTHN_GSS_NEGOTIATE"},
	{  10, "RPC_C_AUTH_WINNT"},
	{  14, "RPC_C_AUTHN_GSS_SCHANNEL"},
	{  16, "RPC_C_AUTHN_GSS_KERBEROS"},
	{  17, "RPC_C_AUTHN_MSN"},
	{  18, "RPC_C_AUTHN_DPA"},
	{ 100, "RPC_C_AUTHN_MQ"},
	{ 0xffff, "RPC_C_AUTHN_DEFAULT"},
	{ 0,   NULL}
};

const value_string dcom_protseq_vals[] = {
	{ 0x04, "NCACN_DNET_NSP" },
	{ 0x07, "NCACN_IP_TCP" },
	{ 0x08, "NCADG_IP_UDP" },
	{ 0x09, "NCACN_IP" },
	{ 0x0C, "NCACN_SPX" },
	{ 0x0D, "NCACN_NB_IPX" },
	{ 0x0E, "NCADG_IPX" },
	{ 0x12, "NCACN_NB_NB" },
	{ 0x1F, "NCACN_HTTP" },
	{ 0,	NULL }
};

static const value_string dcom_vt_bool_vals[] = {
	{ 0x0000, "FALSE" },
	{ 0xFFFF, "TRUE" },
	{ 0,	  NULL }
};



/* dissect extension to DCOM "this" and "that" */
static int
dissect_dcom_extent(tvbuff_t *tvb, int offset,
	packet_info *pinfo, proto_tree *tree, dcerpc_info *di, guint8 *drep)
{
	guint32 u32ArraySize;
	guint32 u32ArraySize2;
	guint32 u32Pointer;
	guint32 u32VariableOffset;
	guint32 u32Idx;
	guint32 u32SubStart;
	proto_item *sub_item;
	proto_tree *sub_tree;

	guint32 u32ArrayCount;
	guint32 u32ArrayRes;

	guint32 u32ExtentSize;
	e_guid_t uuidExtend;
	const char *uuid_name;


	offset = dissect_dcom_dcerpc_pointer(tvb, offset, pinfo, tree, di, drep, &u32Pointer);

	if (u32Pointer == 0) {
		return offset;
	}

	offset = dissect_dcom_DWORD(tvb, offset, pinfo, tree, di, drep,
			hf_dcom_extent_array_count, &u32ArrayCount);
	offset = dissect_dcom_DWORD(tvb, offset, pinfo, tree, di, drep,
			hf_dcom_extent_array_res, &u32ArrayRes);

	offset = dissect_dcom_dcerpc_pointer(tvb, offset, pinfo, tree, di, drep, &u32Pointer);

	if (u32Pointer == 0) {
		return offset;
	}

	offset = dissect_dcom_dcerpc_array_size(tvb, offset, pinfo, tree, di, drep,
						&u32ArraySize);

	u32VariableOffset = offset + u32ArraySize*4;

	u32Idx = 1;
	while (u32ArraySize--) {
		sub_item = proto_tree_add_item(tree, hf_dcom_extent, tvb, offset, 0, ENC_NA);
		sub_tree = proto_item_add_subtree(sub_item, ett_dcom_extent);
		u32SubStart = offset;

		offset = dissect_dcom_dcerpc_pointer(tvb, offset, pinfo, sub_tree, di, drep, &u32Pointer);

		if(u32Pointer != 0) {
			u32VariableOffset = dissect_dcom_DWORD(tvb, u32VariableOffset, pinfo, sub_tree, di, drep,
							       hf_dcom_extent_size, &u32ExtentSize);

			dissect_dcom_UUID(tvb, u32VariableOffset, pinfo, NULL, di, drep,
					  hf_dcom_extent_id, &uuidExtend);

			/* look for a registered uuid name */
			if((uuid_name = guids_get_uuid_name(&uuidExtend)) != NULL) {
				proto_tree_add_guid_format_value(sub_tree, hf_dcom_extent_id, tvb,
								 offset, sizeof(e_guid_t), (e_guid_t *) &uuidExtend,
								 "%s (%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x)",
								 uuid_name,
								 uuidExtend.data1, uuidExtend.data2, uuidExtend.data3,
								 uuidExtend.data4[0], uuidExtend.data4[1],
								 uuidExtend.data4[2], uuidExtend.data4[3],
								 uuidExtend.data4[4], uuidExtend.data4[5],
								 uuidExtend.data4[6], uuidExtend.data4[7]);
				u32VariableOffset += 16;
			} else {
				u32VariableOffset = dissect_dcom_UUID(tvb, u32VariableOffset, pinfo, sub_tree, di, drep,
								      hf_dcom_extent_id, &uuidExtend);
			}


			u32VariableOffset = dissect_dcom_dcerpc_array_size(tvb, u32VariableOffset, pinfo, sub_tree, di, drep,
									   &u32ArraySize2);
			u32VariableOffset = dissect_dcom_nospec_data(tvb, u32VariableOffset, pinfo, sub_tree, drep, u32ArraySize2);

			/* update subtree header */
			if(uuid_name != NULL) {
				proto_item_append_text(sub_item, "[%u]: %s, Bytes=%u",
						       u32Idx, uuid_name, u32ArraySize2);
			} else {
				proto_item_append_text(sub_item, "[%u]: Bytes=%u",
						       u32Idx, u32ArraySize2);
			}
			proto_item_set_len(sub_item, offset - u32SubStart);
		} else {
			/* update subtree header */
			proto_item_append_text(sub_item, "[%u]: NULL", u32Idx);
			proto_item_set_len(sub_item, offset - u32SubStart);
		}

		u32Idx++;
	}

	return u32VariableOffset;
}


/* dissect DCOM "this" (start of every DCOM request) */
int
dissect_dcom_this(tvbuff_t *tvb, int offset,
	packet_info *pinfo, proto_tree *tree, dcerpc_info *di, guint8 *drep)
{
	guint16 u16VersionMajor;
	guint16 u16VersionMinor;
	guint32 u32Flags;
	guint32 u32Res;
	e_guid_t uuidCausality;
	proto_item *sub_item;
	proto_tree *sub_tree;
	guint32 u32SubStart;
	proto_item *pi;

	sub_item = proto_tree_add_protocol_format(tree, proto_dcom, tvb, offset, 0,
						  "DCOM, ORPCThis");
	sub_tree = proto_item_add_subtree(sub_item, ett_dcom_this);

	offset = dissect_dcom_COMVERSION(tvb, offset, pinfo, sub_tree, di, drep,
				&u16VersionMajor, &u16VersionMinor);
	u32SubStart = offset - 4;

	offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
			hf_dcom_this_flags, &u32Flags);
	offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
			hf_dcom_this_res, &u32Res);

	offset = dissect_dcom_UUID(tvb, offset, pinfo, sub_tree, di, drep,
			hf_dcom_this_cid, &uuidCausality);

	offset = dissect_dcom_extent(tvb, offset, pinfo, sub_tree, di, drep);

	/* update subtree header */
	proto_item_append_text(sub_item, ", V%u.%u, Causality ID: %s",
		u16VersionMajor, u16VersionMinor, guids_resolve_guid_to_str(&uuidCausality));
	proto_item_set_len(sub_item, offset - u32SubStart);

	if(memcmp(&di->call_data->object_uuid, &uuid_null, sizeof(uuid_null)) != 0) {
		pi = proto_tree_add_guid_format(tree, hf_dcom_ipid, tvb, offset, 0,
			(e_guid_t *) &di->call_data->object_uuid,
			"Object UUID/IPID: %s", guids_resolve_guid_to_str(&di->call_data->object_uuid));
		PROTO_ITEM_SET_GENERATED(pi);
	}

	return offset;
}


/* dissect DCOM "that" (start of every DCOM response) */
int
dissect_dcom_that(tvbuff_t *tvb, int offset,
	packet_info *pinfo, proto_tree *tree, dcerpc_info *di, guint8 *drep) {
	guint32 u32Flags;
	proto_item *sub_item;
	proto_tree *sub_tree;
	guint32 u32SubStart;
	proto_item *pi;

	sub_item = proto_tree_add_protocol_format(tree, proto_dcom, tvb, offset, 0,
						  "DCOM, ORPCThat");
	sub_tree = proto_item_add_subtree(sub_item, ett_dcom_that);

	offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
				    hf_dcom_that_flags, &u32Flags);
	u32SubStart = offset - 4;

	offset = dissect_dcom_extent(tvb, offset, pinfo, sub_tree, di, drep);

	/* update subtree header */
	proto_item_set_len(sub_item, offset - u32SubStart);

	if(memcmp(&di->call_data->object_uuid, &uuid_null, sizeof(uuid_null)) != 0) {
		pi = proto_tree_add_guid_format(tree, hf_dcom_ipid, tvb, offset, 0,
			(e_guid_t *) &di->call_data->object_uuid,
			"Object UUID/IPID: %s", guids_resolve_guid_to_str(&di->call_data->object_uuid));
		PROTO_ITEM_SET_GENERATED(pi);
	}

	return offset;
}


/* dissect simple dcom request, DCOM "this" only */
int
dissect_dcom_simple_rqst(tvbuff_t *tvb, int offset,
	packet_info *pinfo, proto_tree *tree, dcerpc_info *di, guint8 *drep)
{

	offset = dissect_dcom_this(tvb, offset, pinfo, tree, di, drep);

	return offset;
}


/* dissect simple dcom response, DCOM "that" and returned HRESULT only */
int
dissect_dcom_simple_resp(tvbuff_t *tvb, int offset,
	packet_info *pinfo, proto_tree *tree, dcerpc_info *di, guint8 *drep)
{
	guint32 u32HResult;


	offset = dissect_dcom_that(tvb, offset, pinfo, tree, di, drep);

	offset = dissect_dcom_HRESULT(tvb, offset, pinfo, tree, di, drep,
				      &u32HResult);

	col_append_fstr(pinfo->cinfo, COL_INFO, " -> %s",
			val_to_str(u32HResult, dcom_hresult_vals, "Unknown (0x%08x)") );


	return offset;
}


/* dissect a dcerpc array size */
int
dissect_dcom_dcerpc_array_size(tvbuff_t *tvb, gint offset, packet_info *pinfo,
		       proto_tree *tree, dcerpc_info *di, guint8 *drep, guint32 *pu32ArraySize)
{


	/* en-/disable this by preference setting */
	if (!dcom_prefs_display_unmarshalling_details) {
		/* this will read in the data, but prevent output to tree */
		tree = NULL;
	}

	offset = dissect_ndr_uint32(tvb, offset, pinfo, tree, di, drep,
							hf_dcom_array_size, pu32ArraySize);

	return offset;
}


/* dissect a dcerpc pointer value */
int
dissect_dcom_dcerpc_pointer(tvbuff_t *tvb, gint offset, packet_info *pinfo,
		       proto_tree *tree, dcerpc_info *di, guint8 *drep, guint32 *pu32Pointer)
{

	/* en-/disable this by preference setting */
	if (!dcom_prefs_display_unmarshalling_details) {
		/* this will read in the data, but prevent output to tree */
		tree = NULL;
	}

	offset = dissect_ndr_uint32(tvb, offset, pinfo, tree, di, drep,
								hf_dcom_pointer_val, pu32Pointer);

	return offset;
}


/* mark data as "ToBeDone" */
/* XXX: handout data to generic "unknown data" dissector? */
extern int
dissect_dcom_tobedone_data(tvbuff_t *tvb, int offset,
	packet_info *pinfo, proto_tree *tree, guint8 *drep _U_, int length)
{
	proto_item *item;


	item = proto_tree_add_item(tree, hf_dcom_tobedone, tvb, offset, length, ENC_NA);
	PROTO_ITEM_SET_GENERATED(item);
	expert_add_info(pinfo, item, &ei_dcom_dissetion_incomplete);

	offset += length;

	return offset;
}


/* mark data as "No Specification Available" */
/* XXX: handout data to generic "unknown data" dissector? */
extern int
dissect_dcom_nospec_data(tvbuff_t *tvb, int offset,
	packet_info *pinfo, proto_tree *tree, guint8 *drep _U_, int length)
{
	proto_item *item;


	item = proto_tree_add_item(tree, hf_dcom_nospec, tvb, offset, length, ENC_NA);
	PROTO_ITEM_SET_GENERATED(item);
	expert_add_info(pinfo, item, &ei_dcom_no_spec);

	offset += length;

	return offset;
}


/* dissect an indexed WORD, something like: "FieldName[1]: 0x1234" */
int
dissect_dcom_indexed_WORD(tvbuff_t *tvb, int offset,	packet_info *pinfo,
					 proto_tree *tree, dcerpc_info *di, guint8 *drep,
					 int hfindex, guint16 * pu16WORD, int field_index)
{
	guint16 u16WORD;


	/* dissect the WORD, but don't add to tree */
	dissect_dcom_WORD(tvb, offset, pinfo, NULL /*tree*/, di, drep,
					hfindex, &u16WORD);

	if (tree) {
		/* special formatted output of indexed value */
		proto_tree_add_uint_format(tree, hfindex, tvb, offset, 2, u16WORD,
			"%s[%u]: 0x%04x",
			proto_registrar_get_name(hfindex),
			field_index, u16WORD);
	}

	offset += 2;

	if (pu16WORD)
		*pu16WORD = u16WORD;

	return offset;
}


/* dissect an indexed DWORD, something like: "FieldName[1]: 0x12345678" */
int
dissect_dcom_indexed_DWORD(tvbuff_t *tvb, int offset,	packet_info *pinfo,
					 proto_tree *tree, dcerpc_info *di, guint8 *drep,
					 int hfindex, guint32 * pu32DWORD, int field_index)
{
	guint32 u32DWORD;


	/* dissect the DWORD, but don't add to tree */
	dissect_dcom_DWORD(tvb, offset, pinfo, NULL /*tree*/, di, drep,
					hfindex, &u32DWORD);

	if (tree) {
		/* special formatted output of indexed value */
		proto_tree_add_uint_format(tree, hfindex, tvb, offset, 4, u32DWORD,
			"%s[%u]: 0x%08x",
			proto_registrar_get_name(hfindex),
			field_index, u32DWORD);
	}

	offset += 4;

	if (pu32DWORD)
		*pu32DWORD = u32DWORD;

	return offset;
}


/* dissect hresult field of a usual DCOM call (create "raw" item) */
int
dissect_dcom_HRESULT_item(tvbuff_t *tvb, int offset,	packet_info *pinfo,
					 proto_tree *tree, dcerpc_info *di, guint8 *drep,
					 guint32 * pu32HResult, int field_index, proto_item **item)
{
	guint32 u32HResult;

	/* dissect the DWORD, but don't add to tree */
	offset = dissect_dcom_DWORD(tvb, offset, pinfo, NULL /*tree*/, di, drep,
		    field_index, &u32HResult);

	if (tree) {
		/* special formatted output of indexed value */
		*item = proto_tree_add_item (tree, field_index, tvb, offset-4, 4, DREP_ENC_INTEGER(drep));
	}

	if (pu32HResult)
		*pu32HResult = u32HResult;

	return offset;
}


/* dissect hresult field of a usual DCOM call (separate method, because often used) */
int
dissect_dcom_HRESULT(tvbuff_t *tvb, int offset,	packet_info *pinfo,
					 proto_tree *tree, dcerpc_info *di, guint8 *drep,
					 guint32 * pu32HResult)
{
	guint32 u32HResult;
	proto_item *item = NULL;

	/* dissect the DWORD, but don't add to tree */
	offset = dissect_dcom_DWORD(tvb, offset, pinfo, NULL /*tree*/, di, drep,
		    hf_dcom_hresult, &u32HResult);

	if (tree) {
		/* special formatted output of indexed value */
		item = proto_tree_add_item (tree, hf_dcom_hresult, tvb, offset-4, 4, DREP_ENC_INTEGER(drep));
	}

	/* expert info only if severity is set */
	/* XXX - move this to the callers of this function, to provide a more detailed error output */
	if(u32HResult & 0x80000000) {
		expert_add_info_format(pinfo, item, &ei_dcom_hresult_expert, "Hresult: %s",
			val_to_str(u32HResult, dcom_hresult_vals, "Unknown (0x%x)"));
	}
	if (pu32HResult)
		*pu32HResult = u32HResult;

	return offset;
}


/* partial results of indexed DCOM subcalls (e.g.: from a kind of array) */
int
dissect_dcom_indexed_HRESULT(tvbuff_t *tvb, int offset,	packet_info *pinfo,
					 proto_tree *tree, dcerpc_info *di, guint8 *drep,
					 guint32 * pu32HResult, int field_index)
{
	guint32 u32HResult;
	proto_item *item = NULL;


	/* dissect the DWORD, but don't add to tree */
	offset = dissect_dcom_DWORD(tvb, offset, pinfo, NULL /*tree*/, di, drep,
		    hf_dcom_hresult, &u32HResult);

	if (tree) {
		/* special formatted output of indexed value */
		item = proto_tree_add_uint_format(tree, hf_dcom_hresult, tvb, offset-4, 4, u32HResult,
			"HResult[%u]: %s (0x%08x)", field_index,
			val_to_str_const(u32HResult, dcom_hresult_vals, "Unknown"),
			u32HResult);
	}
	/* expert info only if severity flag is set */
	/* XXX - move this to the callers of this function, to provide a more detailed error output */
	if(u32HResult & 0x80000000) {
		expert_add_info_format(pinfo, item, &ei_dcom_hresult_expert, "Hresult: %s",
			val_to_str(u32HResult, dcom_hresult_vals, "Unknown (0x%x)"));
	}
	if (pu32HResult)
		*pu32HResult = u32HResult;

	return offset;
}



int
dissect_dcom_COMVERSION(tvbuff_t *tvb, int offset, packet_info *pinfo,
						proto_tree *tree, dcerpc_info *di, guint8 *drep,
						guint16	* pu16VersionMajor, guint16 * pu16VersionMinor)
{

	offset = dissect_dcom_WORD(tvb, offset, pinfo, tree, di, drep,
			hf_dcom_version_major, pu16VersionMajor);
	offset = dissect_dcom_WORD(tvb, offset, pinfo, tree, di, drep,
			hf_dcom_version_minor, pu16VersionMinor);

	return offset;
}


int
dissect_dcom_SAFEARRAY(tvbuff_t *tvb, int offset, packet_info *pinfo,
						proto_tree *tree, dcerpc_info *di, guint8 *drep, int hfindex _U_, sa_callback_t sacb)
{
	guint32 u32Dims;
	guint16 u16Dims;
	guint16 u16Features;
	guint32 u32ElementSize;
	guint32 u32VarType;
	guint32 u32Elements;
	guint32 u32Pointer;
	guint32 u32BoundElements;
	guint32 u32LowBound;
	gchar cData[100];
	guint32 u32ArraySize;
	guint32 u32VariableOffset;
	guint32 u32Data;
	guint16 u16Data;
	guint8	u8Data;
	guint16 u16Locks;
	guint16 u16VarType;
	proto_item *sub_item;
	proto_tree *sub_tree;
	guint32 u32SubStart;
	guint32 u32TmpOffset;

	static const int * features[] = {
		&hf_dcom_sa_features_variant,
		&hf_dcom_sa_features_dispatch,
		&hf_dcom_sa_features_unknown,
		&hf_dcom_sa_features_bstr,
		&hf_dcom_sa_features_have_vartype,
		&hf_dcom_sa_features_have_iid,
		&hf_dcom_sa_features_record,
		&hf_dcom_sa_features_fixedsize,
		&hf_dcom_sa_features_embedded,
		&hf_dcom_sa_features_static,
		&hf_dcom_sa_features_auto,
		NULL
	};


	/* XXX: which alignment do we need here? */

	sub_item = proto_tree_add_item(tree, hf_dcom_safearray, tvb, offset, 0, ENC_NA);
	sub_tree = proto_item_add_subtree(sub_item, ett_dcom_safearray);
	u32SubStart = offset;

	offset = dissect_dcom_dcerpc_pointer(tvb, offset, pinfo, sub_tree, di, drep, &u32Pointer);
	offset = dissect_dcom_dcerpc_pointer(tvb, offset, pinfo, sub_tree, di, drep, &u32Pointer);

	offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
			hf_dcom_sa_dims32, &u32Dims);
	offset = dissect_dcom_WORD(tvb, offset, pinfo, sub_tree, di, drep,
			hf_dcom_sa_dims16, &u16Dims);

	/* feature flags */
	u32TmpOffset = dissect_dcom_WORD(tvb, offset, pinfo, NULL, di, drep, -1, &u16Features);

	proto_tree_add_bitmask_value_with_flags(sub_tree, tvb, offset, hf_dcom_sa_features,
								ett_dcom_sa_features, features, u16Features, BMT_NO_APPEND);
	offset = u32TmpOffset;

	offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
			hf_dcom_sa_element_size, &u32ElementSize);
	offset = dissect_dcom_WORD(tvb, offset, pinfo, sub_tree, di, drep,
			hf_dcom_sa_locks, &u16Locks);
	offset = dissect_dcom_WORD(tvb, offset, pinfo, sub_tree, di, drep,
			hf_dcom_sa_vartype16, &u16VarType);
	offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
			hf_dcom_sa_vartype32, &u32VarType);
	offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
			hf_dcom_sa_elements, &u32Elements);
	offset = dissect_dcom_dcerpc_pointer(tvb, offset, pinfo, sub_tree, di, drep, &u32Pointer);

	u32BoundElements = 0;
	while(u32Dims--) {
		offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
							hf_dcom_sa_bound_elements, &u32BoundElements);
		offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
							hf_dcom_sa_low_bound, &u32LowBound);
	}

	offset = dissect_dcom_dcerpc_array_size(tvb, offset, pinfo, sub_tree, di, drep, &u32ArraySize);

	tvb_ensure_bytes_exist(tvb, offset, u32ArraySize * u32ElementSize);
	u32VariableOffset = offset + u32ArraySize * u32ElementSize;

	if(sacb) {
		sacb(tvb, offset, pinfo, tree, di, drep, u32VarType, u32ArraySize);
	}

	while(u32ArraySize--) {
		switch(u32VarType) {
			case(WIRESHARK_VT_ERROR):
				offset = dissect_dcom_HRESULT(tvb, offset, pinfo, sub_tree, di, drep,
									&u32Data);
				break;
			case(WIRESHARK_VT_I1):
				offset = dissect_dcom_BYTE(tvb, offset, pinfo, sub_tree, di, drep,
									hf_dcom_vt_i1, &u8Data);
				break;
			case(WIRESHARK_VT_I2):
				offset = dissect_dcom_WORD(tvb, offset, pinfo, sub_tree, di, drep,
									hf_dcom_vt_i2, &u16Data);
				break;
			case(WIRESHARK_VT_I4):
				offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
									hf_dcom_vt_i4, &u32Data);
				break;
			case(WIRESHARK_VT_I8):
				offset = dissect_dcom_I8(tvb, offset, pinfo, sub_tree, di, drep,
									hf_dcom_vt_i8, NULL);
				/* take care of the 8 byte alignment */
				u32VariableOffset = offset;
				break;
			case(WIRESHARK_VT_BSTR):
				offset = dissect_dcom_dcerpc_pointer(tvb, offset, pinfo, sub_tree, di, drep, &u32Pointer);
				if (u32Pointer) {
					u32VariableOffset = dissect_dcom_BSTR(tvb, u32VariableOffset, pinfo, sub_tree, di, drep,
									hf_dcom_vt_bstr, cData, sizeof(cData) );
				}
				break;
			default:
				/* XXX: other types possible, but still not implemented:
				VT_UNKNOWN
				VT_DISPATCH
				VT_VARIANT
				VT_RECORD
				VT_UNKNOWN|VT_RESERVED
				*/
				u32VariableOffset = dissect_dcom_tobedone_data(tvb, u32VariableOffset, pinfo, sub_tree, drep,
								10000);
		}
	}

	/* update subtree header */
	proto_item_append_text(sub_item, ": Elements: %u/%u VarType: %s",
			       u32Elements, u32BoundElements,
			       val_to_str(u32VarType, dcom_variant_type_vals, "Unknown (0x%08x)") );

	proto_item_set_len(sub_item, u32VariableOffset - u32SubStart);

	return u32VariableOffset;
}



int
dissect_dcom_VARTYPE(tvbuff_t *tvb, int offset,	packet_info *pinfo,
					proto_tree *tree, dcerpc_info *di, guint8 *drep,
					guint16 *pu16VarType)
{

	offset = dissect_dcom_WORD(tvb, offset, pinfo, tree, di, drep,
			hf_dcom_variant_type, pu16VarType);

	return offset;
}


int
dissect_dcom_VARIANT(tvbuff_t *tvb, int offset, packet_info *pinfo,
					 proto_tree *tree, dcerpc_info *di, guint8 *drep, int hfindex)
{
	guint32	u32Size;
	guint32 u32RPCRes;
	guint16 u16Res;
	guint32 u32SubStart;
	proto_item *sub_item;
	proto_tree *sub_tree;
	guint16 u16VarType;
	guint32 u32VarType;

	guint8	u8Data;
	guint16 u16Data;
	guint32 u32Data;
	guint64 u64Data;
	gint64 cyData;
	gchar cData[500];
	guint32 u32Pointer;
	gfloat	f32Data;
	gdouble f64Data;


	/* alignment of 8 needed for a VARIANT */
	if (offset % 8) {
		offset += 8 - (offset % 8);
	}

	sub_item = proto_tree_add_item(tree, hfindex, tvb, offset, 0, ENC_BIG_ENDIAN);
	sub_tree = proto_item_add_subtree(sub_item, ett_dcom_variant);
	u32SubStart = offset;

	/* the following size can be VERY confusing:
	 * It is NOT the maximum size of the variant, as one could expect,
	 * but the current size of the variant padded to 8 bytes.
	 * BUT: The following data does not start AFTER this padding,
	 * it starts just after the variant-data (without padding)!!! */
	/* Conclusion: the size given here can be LONGER than the actual size */
	offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
			hf_dcom_variant_size, &u32Size);
	offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
			hf_dcom_variant_rpc_res, &u32RPCRes);
	offset = dissect_dcom_VARTYPE(tvb, offset, pinfo, sub_tree, di, drep,
						&u16VarType);
	offset = dissect_dcom_WORD(tvb, offset, pinfo, sub_tree, di, drep,
			hf_dcom_variant_wres, &u16Res);
	offset = dissect_dcom_WORD(tvb, offset, pinfo, sub_tree, di, drep,
			hf_dcom_variant_wres, &u16Res);
	offset = dissect_dcom_WORD(tvb, offset, pinfo, sub_tree, di, drep,
			hf_dcom_variant_wres, &u16Res);

	/* 32 bit VarType (slightly different to the 16 bit one) */
	offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
			hf_dcom_variant_type32, &u32VarType);

	if (u32VarType & WIRESHARK_VT_BYREF) {
		u32VarType &=~WIRESHARK_VT_BYREF;
		offset = dissect_dcom_dcerpc_pointer(tvb, offset, pinfo, sub_tree, di, drep, &u32Pointer);
	}

	switch (u32VarType) {
		case(WIRESHARK_VT_EMPTY):
			break;
		case(WIRESHARK_VT_BOOL):
			offset = dissect_dcom_VARIANT_BOOL(tvb, offset, pinfo, sub_tree, di, drep,
								hf_dcom_vt_bool, &u16Data);
			break;
		case(WIRESHARK_VT_I1):
			offset = dissect_dcom_BYTE(tvb, offset, pinfo, sub_tree, di, drep,
								hf_dcom_vt_i1, &u8Data);
			break;
		case(WIRESHARK_VT_UI1):
			offset = dissect_dcom_BYTE(tvb, offset, pinfo, sub_tree, di, drep,
								hf_dcom_vt_ui1, &u8Data);
			break;
		case(WIRESHARK_VT_I2):
			offset = dissect_dcom_WORD(tvb, offset, pinfo, sub_tree, di, drep,
								hf_dcom_vt_i2, &u16Data);
			break;
		case(WIRESHARK_VT_UI2):
			offset = dissect_dcom_WORD(tvb, offset, pinfo, sub_tree, di, drep,
								hf_dcom_vt_ui2, &u16Data);
			break;
		case(WIRESHARK_VT_I4):
			offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
								hf_dcom_vt_i4, &u32Data);
			break;
		case(WIRESHARK_VT_I8):
			offset = dissect_dcom_I8(tvb, offset, pinfo, sub_tree, di, drep,
								hf_dcom_vt_i8, &u64Data);
			break;
		case(WIRESHARK_VT_CY):
				offset = dissect_dcom_I8(tvb, offset, pinfo, NULL, di, drep,
						0, &cyData);
				proto_tree_add_int64_format(sub_tree, hf_dcom_vt_cy, tvb, offset - 8,
						8, cyData, "%s: %" G_GINT64_FORMAT ".%.04" G_GINT64_FORMAT,
						proto_registrar_get_name(hf_dcom_vt_cy),
						cyData / 10000, ABS(cyData % 10000));
			break;
		case(WIRESHARK_VT_UI4):
			offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
								hf_dcom_vt_ui4, &u32Data);
			break;
		case(WIRESHARK_VT_UI8):
			offset = dissect_dcom_I8(tvb, offset, pinfo, sub_tree, di, drep,
					hf_dcom_vt_ui8, &u64Data);
			break;
		case(WIRESHARK_VT_R4):
			offset = dissect_dcom_FLOAT(tvb, offset, pinfo, sub_tree, di, drep,
								hf_dcom_vt_r4, &f32Data);
			break;
		case(WIRESHARK_VT_R8):
			offset = dissect_dcom_DOUBLE(tvb, offset, pinfo, sub_tree, di, drep,
								hf_dcom_vt_r8, &f64Data);
			break;
		case(WIRESHARK_VT_DATE):
			offset = dissect_dcom_DATE(tvb, offset, pinfo, sub_tree, di, drep,
								hf_dcom_vt_date, &f64Data);
			break;
		case(WIRESHARK_VT_BSTR):
			offset = dissect_dcom_dcerpc_pointer(tvb, offset, pinfo, sub_tree, di, drep, &u32Pointer);
			if (u32Pointer) {
				offset = dissect_dcom_BSTR(tvb, offset, pinfo, sub_tree, di, drep,
								hf_dcom_vt_bstr, cData, sizeof(cData) );
			}
			break;
		case(WIRESHARK_VT_DISPATCH):
			offset = dissect_dcom_dcerpc_pointer(tvb, offset, pinfo, sub_tree, di, drep, &u32Pointer);
			if (u32Pointer) {
				offset = dissect_dcom_MInterfacePointer(tvb, offset, pinfo, sub_tree, di, drep,
								hf_dcom_vt_dispatch, NULL); /* XXX - how to handle this? */
			}
			break;
		case(WIRESHARK_VT_ARRAY):
			offset = dissect_dcom_SAFEARRAY(tvb, offset, pinfo, sub_tree, di, drep,
								0, NULL);
			break;
		case(WIRESHARK_VT_ERROR):
			offset = dissect_dcom_HRESULT(tvb, offset, pinfo, sub_tree, di, drep,
								0);
			break;
		case(WIRESHARK_VT_VARIANT):
			offset = dissect_dcom_dcerpc_pointer(tvb, offset, pinfo, sub_tree, di, drep, &u32Pointer);
			if (u32Pointer) {
			    offset = dissect_dcom_VARIANT(tvb, offset, pinfo, sub_tree, di, drep,
								    hf_dcom_vt_byref /* must be BYREF */);
			}
			break;
		case(WIRESHARK_VT_UNKNOWN):
			offset = dissect_dcom_dcerpc_pointer(tvb, offset, pinfo, sub_tree, di, drep, &u32Pointer);
			break;
		default:
			/* XXX: add more types here! */
			offset = dissect_dcom_tobedone_data(tvb, offset, pinfo, sub_tree, drep,
							10000);
	}

	/* update subtree header */
	proto_item_append_text(sub_item, ": %s",
		val_to_str(u16VarType, dcom_variant_type_vals, "Unknown (0x%08x)") );

	proto_item_set_len(sub_item, offset - u32SubStart);

	return offset;
}


int
dissect_dcom_UUID(tvbuff_t *tvb, int offset,
	packet_info *pinfo, proto_tree *tree, dcerpc_info *di, guint8 *drep,
	int hfindex, e_guid_t *pdata)
{
	const gchar *uuid_name;
	header_field_info *hfi;
	e_guid_t uuid;


	/* get the UUID, but don't put it into the tree */
	offset = dissect_ndr_uuid_t(tvb, offset, pinfo, NULL, di, drep,
						hfindex, &uuid);

	/* add to the tree */
	hfi = proto_registrar_get_nth(hfindex);
	uuid_name = guids_get_uuid_name(&uuid);
	if(uuid_name) {
		proto_tree_add_guid_format(tree, hfindex, tvb, offset-16, 16, (e_guid_t *) &uuid,
			  "%s: %s (%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x)",
			  hfi->name, uuid_name,
			  uuid.data1, uuid.data2, uuid.data3,
			  uuid.data4[0], uuid.data4[1],
			  uuid.data4[2], uuid.data4[3],
			  uuid.data4[4], uuid.data4[5],
			  uuid.data4[6], uuid.data4[7]);
	} else {
		proto_tree_add_guid_format(tree, hfindex, tvb, offset-16, 16, (e_guid_t *) &uuid,
			  "%s: %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			  hfi->name,
			  uuid.data1, uuid.data2, uuid.data3,
			  uuid.data4[0], uuid.data4[1],
			  uuid.data4[2], uuid.data4[3],
			  uuid.data4[4], uuid.data4[5],
			  uuid.data4[6], uuid.data4[7]);
	}

	if(pdata != NULL) {
		*pdata = uuid;
	}

	return offset;
}


int
dissect_dcom_append_UUID(tvbuff_t *tvb, int offset,
	packet_info *pinfo, proto_tree *tree, dcerpc_info *di, guint8 *drep,
	int hfindex, int field_index, e_guid_t *uuid)
{
	const gchar *uuid_name;
	proto_item *pi;
	header_field_info *hfi;


	/* XXX - this is far from being performance optimized! */

	/* get the UUID, but don't put it into the tree */
	offset = dissect_ndr_uuid_t(tvb, offset, pinfo, NULL, di, drep,
						hfindex, uuid);

	/* look for a registered uuid name */
	uuid_name = guids_get_uuid_name(uuid);

	/* add to the tree */
	hfi = proto_registrar_get_nth(hfindex);
	pi = proto_tree_add_guid_format(tree, hfindex, tvb, offset-16, 16, (e_guid_t *) uuid, "%s", hfi->name);

	if (field_index != -1) {
		proto_item_append_text(pi, "[%u]: ", field_index);
	} else {
		proto_item_append_text(pi, ": ");
	}

	if(uuid_name) {
		proto_item_append_text(pi, "%s (", uuid_name);
	}

	proto_item_append_text(pi, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			  uuid->data1, uuid->data2, uuid->data3,
			  uuid->data4[0], uuid->data4[1],
			  uuid->data4[2], uuid->data4[3],
			  uuid->data4[4], uuid->data4[5],
			  uuid->data4[6], uuid->data4[7]);

	if(uuid_name) {
		proto_item_append_text(pi, ")");
	}

	/* update column info now */
	if (field_index != -1) {
		col_append_fstr(pinfo->cinfo, COL_INFO, " %s[%u]=%s",
			hfi->name, field_index, (uuid_name) ? uuid_name : "???");
	} else {
		col_append_fstr(pinfo->cinfo, COL_INFO, " %s=%s",
			hfi->name, (uuid_name) ? uuid_name : "???");
	}

	return offset;
}


/* get a wide character string from tvb (zero terminated or limited through inLength) */
/* the string will be converted to ASCII if possible or simple hexdump otherwise */
/* outLength is in output bytes including zero termination output */
static int
dcom_tvb_get_nwstringz0(tvbuff_t *tvb, gint offset, guint32 inLength, gchar *pszStr, guint32 outLength, gboolean *isPrintable)
{
	guint32 u32Idx;
	guint32 u32IdxA;
	guint32 u32IdxW;
	guint32 inLengthWithoutNullDelimiter = 0;

	guint8	u8Tmp1;
	guint8	u8Tmp2;


	*isPrintable = TRUE;
	inLengthWithoutNullDelimiter = inLength == 0 ? 0 : inLength -1;

	/* we must have at least the space for the zero termination */
	DISSECTOR_ASSERT(outLength >= 1);

	/* determine length and printablility of the string */
	for(u32Idx = 0; u32Idx < inLengthWithoutNullDelimiter; u32Idx+=2) {
		/* the marshalling direction of a WCHAR is fixed! */
		u8Tmp1 = tvb_get_guint8(tvb, offset+u32Idx);
		u8Tmp2 = tvb_get_guint8(tvb, offset+u32Idx+1);

		/* is this the zero termination? */
		if (u8Tmp1 == 0 && u8Tmp2 == 0) {
			u32Idx+=2;
			break;
		}

		/* is this character printable? */
		/* 10 = New Line, 13 = Carriage Return */
		/* XXX - there are probably more printable chars than isprint() */
		if(!(g_ascii_isprint(u8Tmp1) || u8Tmp1 == 10 || u8Tmp1 == 13)|| u8Tmp2 != 0) {
			*isPrintable = FALSE;
		}
	}

	/* u32Idx now contains the string length in bytes */
	/* (including optional zero termination) */

	/* if this is a printable string? */
	if(*isPrintable == TRUE) {
		/* convert to ascii (every "2nd char") */
		/* XXX - is it possible to convert to UTF8, so the output functions work with it? */
		for(u32IdxA = 0, u32IdxW = 0;
		    u32IdxW < u32Idx && u32IdxA < outLength-2;
		    u32IdxW+=2, u32IdxA++) {
			pszStr[u32IdxA] = tvb_get_guint8(tvb, offset+u32IdxW);
		}
	} else {
		/* convert to hexdump */
		for(u32IdxA = 0, u32IdxW = 0;
		    u32IdxW < u32Idx && u32IdxA < outLength-2;
		    u32IdxW++, u32IdxA+=2) {
			g_snprintf(&pszStr[u32IdxA], 3, "%02X", tvb_get_guint8(tvb, offset+u32IdxW));
		}
	}

	/* zero terminate the string, space must be available */
	DISSECTOR_ASSERT(u32IdxA < outLength);
	pszStr[u32IdxA] = 0;

	return offset + u32Idx;
}


/* dissect a LPWSTR into a given buffer */
/* use FT_STRING for hfindex */
/* u32MaxStr is maximum length of string (including trailing zero) */
int
dissect_dcom_indexed_LPWSTR(tvbuff_t *tvb, gint offset, packet_info *pinfo,
		       proto_tree *tree, dcerpc_info *di, guint8 *drep, int hfindex,
					   gchar *pszStr, guint32 u32MaxStr, int field_index)
{
	guint32 u32MaxCount;
	guint32 u32Offset;
	guint32 u32ArraySize;
	guint32 u32StrStart;
	proto_item *sub_item;
	proto_tree *sub_tree;
	guint32 u32SubStart;
	gboolean isPrintable;


	/* alignment of 4 needed */
	if (offset % 4) {
		offset += 4 - (offset % 4);
	}

	/* add subtree item */
	sub_item = proto_tree_add_string(tree, hfindex, tvb, offset, 0, "");
	sub_tree = proto_item_add_subtree(sub_item, ett_dcom_lpwstr);
	u32SubStart = offset;

	offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
			hf_dcom_max_count, &u32MaxCount);
	offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
			hf_dcom_offset, &u32Offset);
	offset = dissect_dcom_dcerpc_array_size(tvb, offset, pinfo, sub_tree, di, drep,
			&u32ArraySize);

	u32StrStart = offset;
	offset = dcom_tvb_get_nwstringz0(tvb, offset, u32ArraySize*2, pszStr, u32MaxStr, &isPrintable);

	proto_tree_add_string(sub_tree, hfindex, tvb, u32StrStart, offset - u32StrStart, pszStr);

	/* update subtree header */
	if (field_index != -1) {
		proto_item_set_text(sub_item, "%s[%u]: %s%s%s",
			proto_registrar_get_name(hfindex),
			field_index,
			isPrintable ? "\"" : "", pszStr, isPrintable ? "\"" : "");
	} else {
		proto_item_append_text(sub_item, "%s%s%s",
				       isPrintable ? "\"" : "", pszStr, isPrintable ? "\"" : "");
	}
	proto_item_set_len(sub_item, offset - u32SubStart);

	return offset;
}


int
dissect_dcom_LPWSTR(tvbuff_t *tvb, gint offset, packet_info *pinfo,
		       proto_tree *tree, dcerpc_info *di, guint8 *drep, int hfindex,
					   gchar *pszStr, guint32 u32MaxStr)
{


	return dissect_dcom_indexed_LPWSTR(tvb, offset, pinfo, tree, di, drep,
						hfindex, pszStr, u32MaxStr, -1);
}


/* dissect a BSTR to tree and into a given buffer (use FT_STRING for hfindex) */
/* u32MaxStr is maximum length of string (including trailing zero) */
/* (Hint: the BSTR space is always as long as the maximum size) */
int
dissect_dcom_BSTR(tvbuff_t *tvb, gint offset, packet_info *pinfo,
		       proto_tree *tree, dcerpc_info *di, guint8 *drep, int hfindex,
					   gchar *pszStr, guint32 u32MaxStr)
{
	guint32 u32MaxCount;
	guint32 u32ArraySize;
	gint strStart, subStart, realOffset;
	proto_item *sub_item;
	proto_tree *sub_tree;
	guint32 u32ByteLength;
	gboolean isPrintable;

	/* alignment of 4 needed */
	if (offset % 4) {
		offset += 4 - (offset % 4);
	}

	/* add subtree item */
	sub_item = proto_tree_add_string(tree, hfindex, tvb, offset, 0, "");
	sub_tree = proto_item_add_subtree(sub_item, ett_dcom_lpwstr);
	subStart = offset;

	offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
			hf_dcom_max_count, &u32MaxCount);
	offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
			hf_dcom_byte_length, &u32ByteLength);
	offset = dissect_dcom_dcerpc_array_size(tvb, offset, pinfo, sub_tree, di, drep,
			&u32ArraySize);

	if ((guint32)offset + u32ArraySize*2 > G_MAXINT)
		return offset;

	realOffset = offset + u32ArraySize*2;

	strStart = offset;
	offset = dcom_tvb_get_nwstringz0(tvb, offset, u32ArraySize*2, pszStr, u32MaxStr, &isPrintable);

	proto_tree_add_string(sub_tree, hfindex, tvb, strStart, offset - strStart, pszStr);

	/* update subtree header */
	proto_item_append_text(sub_item, "%s%s%s",
	isPrintable ? "\"" : "", pszStr, isPrintable ? "\"" : "");
	if (realOffset <= subStart) {
		/* XXX - expert info */
		return offset;
	}
	proto_item_set_len(sub_item, realOffset - subStart);

	return realOffset;
}


/* dissect an DUALSTRINGARRAY */
int
dissect_dcom_DUALSTRINGARRAY(tvbuff_t *tvb, gint offset, packet_info *pinfo,
		       proto_tree *tree, dcerpc_info *di, guint8 *drep, int hfindex, gchar *ip)
{
	guint16	u16NumEntries;
	guint16	u16SecurityOffset;
	gchar	szStr[1000];
	guint32	u32MaxStr = sizeof(szStr);
	guint32	u32Start;
	guint16 u16TowerId;
	guint16 u16SecurityAuthnSvc;
	guint16 u16SecurityAuthzSvc;
	proto_item *sub_item;
	proto_tree *sub_tree;
	guint32	u32SubStart;
	guint32	u32StringBindings = 0;
	guint32	u32SecurityBindings = 0;
	proto_item *subsub_item;
	proto_tree *subsub_tree;
	guint32	u32SubSubStart;
	gboolean isPrintable;
	guint32 first_ip = 0;
	guint32 curr_ip = 0;
	struct in_addr		ipaddr;
	proto_item *pi;


	/* add subtree header */
	sub_item = proto_tree_add_item(tree, hfindex, tvb, offset, 0, ENC_BIG_ENDIAN);
	sub_tree = proto_item_add_subtree(sub_item, ett_dcom_dualstringarray);

	offset = dissect_dcom_WORD(tvb, offset, pinfo, sub_tree, di, drep,
			hf_dcom_dualstringarray_num_entries, &u16NumEntries);
	/* from here, alignment is ok */
	u32SubStart = offset - 2;
	offset = dissect_dcom_WORD(tvb, offset, pinfo, sub_tree, di, drep,
			hf_dcom_dualstringarray_security_offset, &u16SecurityOffset);

	/* STRINGBINDINGs until first wchar zero */
	while ( tvb_get_ntohs(tvb, offset) ) {
		u32StringBindings++;

		subsub_item = proto_tree_add_item(sub_tree, hf_dcom_dualstringarray_string, tvb, offset, 0, ENC_NA);
		subsub_tree = proto_item_add_subtree(subsub_item, ett_dcom_dualstringarray_binding);
		u32SubSubStart = offset;

		offset = dissect_dcom_WORD(tvb, offset, pinfo, subsub_tree, di, drep,
							hf_dcom_dualstringarray_string_tower_id, &u16TowerId);
		u32Start = offset;
	/* we don't know the (zero terminated) input length, use the buffer length instead */
		offset = dcom_tvb_get_nwstringz0(tvb, offset, u32MaxStr, szStr, u32MaxStr, &isPrintable);
		pi = proto_tree_add_string(subsub_tree, hf_dcom_dualstringarray_string_network_addr,
			tvb, u32Start, offset - u32Start, szStr);

	/* convert ip address (if it is dotted decimal) */
	/* XXX - this conversion is ugly */
		if (inet_aton(szStr, &ipaddr)) {
			if(get_host_ipaddr(szStr, &curr_ip)) {

				/*expert_add_info_format(pinfo, NULL, PI_UNDECODED, PI_WARN, "DUALSTRINGARRAY: IP:%s",
				  ip_to_str( (guint8 *) &curr_ip));*/

				if(first_ip == 0) {
					if(ip != NULL) {
						memcpy(ip, &curr_ip, sizeof(curr_ip));
					}
					first_ip = curr_ip;
				} else {
					if(first_ip != curr_ip) {
						address first_ip_addr, curr_ip_addr;

						set_address(&first_ip_addr, AT_IPv4, 4, &first_ip);
						set_address(&curr_ip_addr, AT_IPv4, 4, &curr_ip);
						expert_add_info_format(pinfo, pi, &ei_dcom_dualstringarray_mult_ip,
								       "DUALSTRINGARRAY: multiple IP's %s %s",
								       address_to_str(wmem_packet_scope(), &first_ip_addr), address_to_str(wmem_packet_scope(), &curr_ip_addr));
					}
				}
			}
		}

		proto_item_append_text(subsub_item, "[%u]: TowerId=%s, NetworkAddr=\"%s\"",
			u32StringBindings,
			val_to_str(u16TowerId, dcom_protseq_vals, "Unknown (0x%04x"),
			szStr);
		proto_item_set_len(subsub_item, offset - u32SubSubStart);
	}
	offset += 2;

	/* SECURITYBINDINGs until first wchar zero */
	while ( tvb_get_ntohs(tvb, offset) ) {
		u32SecurityBindings++;

		subsub_item = proto_tree_add_item(sub_tree, hf_dcom_dualstringarray_security, tvb, offset, 0, ENC_NA);
		subsub_tree = proto_item_add_subtree(subsub_item, ett_dcom_dualstringarray_binding);
		u32SubSubStart = offset;

		offset = dissect_dcom_WORD(tvb, offset, pinfo, subsub_tree, di, drep,
			hf_dcom_dualstringarray_security_authn_svc,
						&u16SecurityAuthnSvc);
		offset = dissect_dcom_WORD(tvb, offset, pinfo, subsub_tree, di, drep,
			hf_dcom_dualstringarray_security_authz_svc,
						&u16SecurityAuthzSvc);

		u32Start = offset;
	/* we don't know the (zero terminated) input length, use the buffer length instead */
		offset = dcom_tvb_get_nwstringz0(tvb, offset, u32MaxStr, szStr, u32MaxStr, &isPrintable);
		proto_tree_add_string(subsub_tree, hf_dcom_dualstringarray_security_princ_name,
			tvb, u32Start, offset - u32Start, szStr);

		proto_item_append_text(subsub_item, "[%u]: AuthnSvc=0x%04x, AuthzSvc=0x%04x, PrincName=\"%s\"",
			u32SecurityBindings, u16SecurityAuthnSvc, u16SecurityAuthzSvc, szStr);
		proto_item_set_len(subsub_item, offset - u32SubSubStart);
	}
	offset += 2;

	/* append info to subtree header */
	proto_item_append_text(sub_item, ": STRINGBINDINGs=%u, SECURITYBINDINGs=%u",
		u32StringBindings, u32SecurityBindings);
	proto_item_set_len(sub_item, offset - u32SubStart);

	return offset;
}


/* dissect an STDOBJREF */
int
dissect_dcom_STDOBJREF(tvbuff_t *tvb, gint offset, packet_info *pinfo,
		       proto_tree *tree, dcerpc_info *di, guint8 *drep, int hfindex _U_,
		       guint64 *oxid, guint64 *oid, e_guid_t *ipid)
{
	guint32	u32Flags;
	guint32	u32PublicRefs;
	proto_item *sub_item;
	proto_tree *sub_tree;
	guint32	u32SubStart;


	/* add subtree header */
	sub_item = proto_tree_add_item(tree, hf_dcom_stdobjref, tvb, offset, 0, ENC_NA);
	sub_tree = proto_item_add_subtree(sub_item, ett_dcom_stdobjref);

	offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
			hf_dcom_stdobjref_flags, &u32Flags);
	/* from here, alignment is ok */
	u32SubStart = offset - 4;
	offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
			hf_dcom_stdobjref_public_refs, &u32PublicRefs);
	offset = dissect_dcom_ID(tvb, offset, pinfo, sub_tree, di, drep,
			hf_dcom_oxid, oxid);
	offset = dissect_dcom_ID(tvb, offset, pinfo, sub_tree, di, drep,
			hf_dcom_oid, oid);
	offset = dissect_dcom_UUID(tvb, offset, pinfo, sub_tree, di, drep,
			hf_dcom_ipid, ipid);

	/* append info to subtree header */
	proto_item_append_text(sub_item, ": PublicRefs=%u IPID=%s",
		u32PublicRefs, guids_resolve_guid_to_str(ipid));
	proto_item_set_len(sub_item, offset - u32SubStart);

	return offset;
}



/*
 *
 *static void
 *print_uuid(const e_guid_t* uuid)
*{
 *    proto_tree_add_debug_text(NULL, "UUID:(%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x)\n",
 *            uuid->data1, uuid->data2, uuid->data3,
 *            uuid->data4[0], uuid->data4[1],
 *            uuid->data4[2], uuid->data4[3],
 *            uuid->data4[4], uuid->data4[5],
 *            uuid->data4[6], uuid->data4[7]);
 *
 *    return;
 *}
 *
 */

int
dcom_register_rountine(dcom_dissect_fn_t routine, e_guid_t* uuid)
{
	dcom_marshaler_t *marshaler;


	/* check if exists already */
	if (dcom_get_rountine_by_uuid(uuid))
		return -1;

	marshaler = wmem_new(wmem_file_scope(), dcom_marshaler_t);
	if (!marshaler) {
		/*memory error*/
		return -1;
	}

	marshaler->parent = NULL;
	marshaler->private_data = NULL;
	marshaler->uuid = *uuid;
	marshaler->routine = routine;

	dcom_marshalers = g_list_append(dcom_marshalers, marshaler);

	return 0;
}


dcom_dissect_fn_t
dcom_get_rountine_by_uuid(const e_guid_t* uuid)
{
	dcom_marshaler_t *marsh;
	GList *marshalers;

	if(memcmp(uuid, &uuid_null, sizeof(uuid_null)) == 0) {
		return NULL;
	}

	for(marshalers = dcom_marshalers; marshalers!= NULL;
		    marshalers = g_list_next(marshalers)) {
		marsh = (dcom_marshaler_t *)marshalers->data;
		/*print_uuid(&marsh->uuid);*/
		/*print_uuid(uuid);*/
		if(memcmp(&marsh->uuid, uuid, sizeof(e_guid_t)) == 0) {
		    return marsh->routine;
		}
	}

	return NULL;
}

/* dissect an CUSTOM */
static int
dissect_dcom_CUSTOBJREF(tvbuff_t *tvb, gint offset, packet_info *pinfo,
		               proto_tree *tree, dcerpc_info *di, guint8 *drep, int hfindex,
		               e_guid_t *clsid, e_guid_t *iid)
{
	guint32    u32CBExtension;
	guint32    u32Size;
	guint32    u32SubStart;
	proto_item *sub_item;
	proto_tree *sub_tree;
	dcom_dissect_fn_t routine = NULL;


	/* add subtree header */
	hfindex = hf_dcom_custobjref;
	sub_item = proto_tree_add_item(tree, hfindex, tvb, offset, 0, ENC_NA);
	sub_tree = proto_item_add_subtree(sub_item, ett_dcom_custobjref);

	u32SubStart = offset;
	offset = dissect_dcom_UUID(tvb, offset, pinfo, sub_tree, di, drep,
		    hf_dcom_clsid, clsid);
	offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
		    hf_dcom_objref_cbextension, &u32CBExtension);
	offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
		    hf_dcom_objref_size, &u32Size);

	/* the following data depends on the iid, get the routine by iid */
	routine = dcom_get_rountine_by_uuid(iid);
	if (routine){
		offset = routine(tvb, offset, pinfo, sub_tree, di, drep, 0);
	}

	/* append info to subtree header */
	/* proto_item_append_text(sub_item, ": ActivationPropertiesIn"); */
	proto_item_set_len(sub_item, offset - u32SubStart);

	return offset;
}

/* dissect an OBJREF */
int
dissect_dcom_OBJREF(tvbuff_t *tvb, gint offset, packet_info *pinfo,
		       proto_tree *tree, dcerpc_info *di, guint8 *drep, int hfindex, dcom_interface_t **interf)
{
	guint32	u32Signature;
	guint32	u32Flags;
	e_guid_t iid;
	e_guid_t clsid;
	proto_item *sub_item;
	proto_tree *sub_tree;
	guint32	u32SubStart;
	guint64 oxid = 0;
	guint64 oid = 0;
	e_guid_t ipid;
	dcom_interface_t *dcom_if = NULL;
	gchar ip[4];

	memset(&ipid, 0, sizeof(ipid));
	memset(ip, 0, sizeof(ip));

	/* add subtree header */
	sub_item = proto_tree_add_item(tree, hf_dcom_objref, tvb, offset, 0, ENC_NA);
	sub_tree = proto_item_add_subtree(sub_item, ett_dcom_objref);

	offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
			hf_dcom_objref_signature, &u32Signature);
	/* from here, alignment is ok */
	u32SubStart = offset - 4;
	offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
			hf_dcom_objref_flags, &u32Flags);
	offset = dissect_dcom_UUID(tvb, offset, pinfo, sub_tree, di, drep,
			hf_dcom_iid, &iid);

	switch(u32Flags) {
		case(0x1):	/* standard */
			offset = dissect_dcom_STDOBJREF(tvb, offset, pinfo, sub_tree, di, drep, hfindex,
								&oxid, &oid, &ipid);
			offset = dissect_dcom_DUALSTRINGARRAY(tvb, offset, pinfo, sub_tree, di, drep,
								hf_dcom_objref_resolver_address, ip);
		break;
		case(0x2):	/* handler (untested) */
			offset = dissect_dcom_STDOBJREF(tvb, offset, pinfo, sub_tree, di, drep, hfindex,
								&oxid, &oid, &iid);
			offset = dissect_dcom_UUID(tvb, offset, pinfo, sub_tree, di, drep,
								hf_dcom_clsid, &clsid);
			offset = dissect_dcom_DUALSTRINGARRAY(tvb, offset, pinfo, sub_tree, di, drep,
								hf_dcom_objref_resolver_address, ip);
		break;
		case(0x4):	/* custom */
		    offset = dissect_dcom_CUSTOBJREF(tvb, offset, pinfo, sub_tree, di, drep, hfindex,
		                        &clsid, &iid);
		    break;
	}

	if(u32Flags == 0x1 || u32Flags == 0x2) {
		/* add interface instance to database (we currently only handle IPv4) */
		if(pinfo->net_src.type == AT_IPv4) {
			address addr;

			set_address(&addr, AT_IPv4, 4, ip);
			dcom_if = dcom_interface_new(pinfo,
						     &addr,
						     &iid, oxid, oid, &ipid);
		}
	}

	if(interf != NULL) {
		*interf = dcom_if;
	}

	/* append info to subtree header */
	proto_item_set_len(sub_item, offset - u32SubStart);

	return offset;
}

/* dissect an MInterfacePointer */
int
dissect_dcom_MInterfacePointer(tvbuff_t *tvb, gint offset, packet_info *pinfo,
		       proto_tree *tree, dcerpc_info *di, guint8 *drep, int hfindex, dcom_interface_t **interf)
{
	guint32	u32CntData;
	guint32	u32ArraySize;
	proto_item *sub_item;
	proto_tree *sub_tree;
	guint32	u32SubStart;


	if (!hfindex) {
		hfindex = hf_dcom_interface_pointer;
	}

	/* add subtree header */
	sub_item = proto_tree_add_item(tree, hfindex, tvb, offset, 0, ENC_BIG_ENDIAN);
	sub_tree = proto_item_add_subtree(sub_item, ett_dcom_interface_pointer);

	offset = dissect_dcom_dcerpc_array_size(tvb, offset, pinfo, sub_tree, di, drep, &u32ArraySize);
	u32SubStart = offset - 4; /* should use this trick to deal with align pad if any */

	offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
			hf_dcom_ip_cnt_data, &u32CntData);

	offset = dissect_dcom_OBJREF(tvb, offset, pinfo, sub_tree, di, drep, hfindex, interf);

	/* append info to subtree header */
	proto_item_set_len(sub_item, offset - u32SubStart);

	return offset;
}

/* dissect a pointer to a MInterfacePointer */
int
dissect_dcom_PMInterfacePointer(tvbuff_t *tvb, gint offset, packet_info *pinfo,
		       proto_tree *tree, dcerpc_info *di, guint8 *drep, int hfindex, dcom_interface_t **interf)
{
	guint32 u32Pointer;


	offset = dissect_dcom_dcerpc_pointer(tvb, offset, pinfo, tree, di, drep, &u32Pointer);

	if (u32Pointer) {
		offset = dissect_dcom_MInterfacePointer(tvb, offset, pinfo, tree, di, drep, hfindex, interf);
	} else {
		if(interf != NULL) {
			*interf = NULL;
		}
	}

	return offset;
}

static void dcom_cleanup(void) {
	if (dcom_machines != NULL) {
		GList *machines;

		for(machines = dcom_machines; machines != NULL; machines = g_list_next(machines)) {
			dcom_machine_t *machine = (dcom_machine_t *)machines->data;

			if (machine->objects != NULL) {
				GList *objects;
				for(objects = machine->objects; objects != NULL; objects = g_list_next(objects)) {
					dcom_object_t *object = (dcom_object_t *)objects->data;

					if (object->interfaces != NULL) {
						GList *interface;
						for(interface = object->interfaces; interface != NULL; interface = g_list_next(interface)) {
							g_free(interface->data);
							interface->data = NULL; /* for good measure */
						}
						g_list_free(object->interfaces);
						object->interfaces = NULL;  /* for good measure */
					}

					g_free(objects->data);
					objects->data = NULL; /* for good measure */
				}
				g_list_free(machine->objects);
				free_address(&machine->ip);
				machine->objects = NULL; /* for good measure */
			}

			g_free(machines->data);
			machines->data = NULL; /* for good measure */
		}
		g_list_free(dcom_machines);
		dcom_machines = NULL;
	}

	/*  The data in dcom_interfaces is wmem_file_scoped so there's no need to free
	 *  the data pointers.
	 */
	if (dcom_interfaces != NULL) {
		g_list_free(dcom_interfaces);
		dcom_interfaces = NULL;
	}
}

void
proto_register_dcom (void)
{
	static hf_register_info hf_dcom_this_array[] = {
#if 0
		{ &hf_dcom_this_version_major,
		{ "VersionMajor", "dcom.this.version_major", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }},
#endif
#if 0
		{ &hf_dcom_this_version_minor,
		{ "VersionMinor", "dcom.this.version_minor", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }},
#endif
		{ &hf_dcom_this_flags,
		{ "Flags", "dcom.this.flags", FT_UINT32, BASE_HEX, VALS(dcom_thisthat_flag_vals), 0x0, NULL, HFILL }},
		{ &hf_dcom_this_res,
		{ "Reserved", "dcom.this.res", FT_UINT32, BASE_HEX, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_this_cid,
		{ "Causality ID", "dcom.this.uuid", FT_GUID, BASE_NONE, NULL, 0x0, NULL, HFILL }}
	};

	static hf_register_info hf_dcom_that_array[] = {
		{ &hf_dcom_that_flags,
		{ "Flags", "dcom.that.flags", FT_UINT32, BASE_HEX, VALS(dcom_thisthat_flag_vals), 0x0, NULL, HFILL }}
	};

	static hf_register_info hf_dcom_extent_array[] = {
		{ &hf_dcom_extent,
		{ "Extension", "dcom.extent", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_extent_array_count,
		{ "Extension Count", "dcom.extent.array_count", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_extent_array_res,
		{ "Reserved", "dcom.extent.array_res", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_extent_size,
		{ "Extension Size", "dcom.extent.size", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_extent_id,
		{ "Extension Id", "dcom.extent.id", FT_GUID, BASE_NONE, NULL, 0x0, NULL, HFILL }}
	};

	static hf_register_info hf_dcom_array[] = {
		{ &hf_dcom_version_major,
		{ "VersionMajor", "dcom.version_major", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_version_minor,
		{ "VersionMinor", "dcom.version_minor", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_hresult,
		{ "HResult", "dcom.hresult", FT_UINT32, BASE_HEX, VALS(dcom_hresult_vals), 0x0, NULL, HFILL }},
		{ &hf_dcom_max_count,
		{ "MaxCount", "dcom.max_count", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_offset,
		{ "Offset", "dcom.offset", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_byte_length,
		{ "ByteLength", "dcom.byte_length", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
#if 0
		{ &hf_dcom_actual_count,
		{ "ActualCount", "dcom.actual_count", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
#endif
		{ &hf_dcom_tobedone,
		{ "To Be Done", "dcom.tobedone", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_nospec,
		{ "No Specification Available", "dcom.nospec", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }},
#if 0
		{ &hf_dcom_variant,
		{ "Variant", "dcom.variant", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }},
#endif
		{ &hf_dcom_variant_type,
		{ "VarType", "dcom.variant_type", FT_UINT16, BASE_HEX, VALS(dcom_variant_type_vals), 0x0, NULL, HFILL }},
		{ &hf_dcom_variant_type32,
		{ "VarType32", "dcom.variant_type32", FT_UINT32, BASE_HEX, VALS(dcom_variant_type_vals), 0x0, NULL, HFILL }},
		{ &hf_dcom_variant_size,
		{ "Size", "dcom.variant_size", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_variant_rpc_res,
		{ "RPC-Reserved", "dcom.variant_rpc_res", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_variant_wres,
		{ "Reserved", "dcom.variant_wres", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_array_size,
		{ "(ArraySize)", "dcom.array_size", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_pointer_val,
		{ "(PointerVal)", "dcom.pointer_val", FT_UINT32, BASE_HEX, VALS(dcom_dcerpc_pointer_vals), 0x0, NULL, HFILL }}
	};

	static hf_register_info hf_dcom_interface_pointer_array[] = {
		{ &hf_dcom_interface_pointer,
		{ "InterfacePointer", "dcom.ifp", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_ip_cnt_data,
		{ "CntData", "dcom.ip_cnt_data", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }}
	};

	static hf_register_info hf_dcom_objref_array[] = {
		{ &hf_dcom_objref,
		{ "OBJREF", "dcom.objref", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_objref_signature,
		{ "Signature", "dcom.objref.signature", FT_UINT32, BASE_HEX, VALS(dcom_objref_signature_vals), 0x0, NULL, HFILL }},
		{ &hf_dcom_objref_flags,
		{ "Flags", "dcom.objref.flags", FT_UINT32, BASE_HEX, VALS(dcom_objref_flag_vals), 0x0, NULL, HFILL }},
		{ &hf_dcom_iid,
		{ "IID", "dcom.iid", FT_GUID, BASE_NONE, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_clsid,
		{ "CLSID", "dcom.clsid", FT_GUID, BASE_NONE, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_objref_resolver_address,
		{ "ResolverAddress", "dcom.objref.resolver_address", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_objref_cbextension,
		{ "CBExtension", "dcom.objref.cbextension", FT_UINT32, BASE_DEC, NULL, 0x0, "Size of extension data", HFILL }},
		{ &hf_dcom_objref_size,
		{ "Size", "dcom.objref.size", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }}
	};

	static hf_register_info hf_dcom_stdobjref_array[] = {
		{ &hf_dcom_stdobjref,
		{ "STDOBJREF", "dcom.stdobjref", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_stdobjref_flags,
		{ "Flags", "dcom.stdobjref.flags", FT_UINT32, BASE_HEX, VALS(dcom_stdobjref_flag_vals), 0x0, NULL, HFILL }},
		{ &hf_dcom_stdobjref_public_refs,
		{ "PublicRefs", "dcom.stdobjref.public_refs", FT_UINT32, BASE_HEX, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_oxid,
		{ "OXID", "dcom.oxid", FT_UINT64, BASE_HEX, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_oid,
		{ "OID", "dcom.oid", FT_UINT64, BASE_HEX, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_ipid,
		{ "IPID", "dcom.ipid", FT_GUID, BASE_NONE, NULL, 0x0, NULL, HFILL }}
	};

	static hf_register_info hf_dcom_custobjref_array[] = {
		{ &hf_dcom_custobjref,
		{ "CUSTOMOBJREF", "dcom.custobjref", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }},
	};


	static hf_register_info hf_dcom_dualstringarray_array[] = {
		{ &hf_dcom_dualstringarray_num_entries,
		{ "NumEntries", "dcom.dualstringarray.num_entries", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_dualstringarray_security_offset,
		{ "SecurityOffset", "dcom.dualstringarray.security_offset", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_dualstringarray_string,
		{ "StringBinding", "dcom.dualstringarray.string", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_dualstringarray_string_tower_id,
		{ "TowerId", "dcom.dualstringarray.tower_id", FT_UINT16, BASE_HEX, VALS(dcom_protseq_vals), 0x0, NULL, HFILL }},
		{ &hf_dcom_dualstringarray_string_network_addr,
		{ "NetworkAddr", "dcom.dualstringarray.network_addr", FT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_dualstringarray_security,
		{ "SecurityBinding", "dcom.dualstringarray.security", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_dualstringarray_security_authn_svc,
		{ "AuthnSvc", "dcom.dualstringarray.security_authn_svc", FT_UINT16, BASE_HEX, VALS(dcom_dualstringarray_authn), 0x0, NULL, HFILL }},
		{ &hf_dcom_dualstringarray_security_authz_svc,
		{ "AuthzSvc", "dcom.dualstringarray.security_authz_svc", FT_UINT16, BASE_HEX, VALS(dcom_dualstringarray_authz), 0x0, NULL, HFILL }},
		{ &hf_dcom_dualstringarray_security_princ_name,
		{ "PrincName", "dcom.dualstringarray.security_princ_name", FT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL }}
	};

	static hf_register_info hf_dcom_vt_array[] = {
		{ &hf_dcom_vt_bool,
		{ "VT_BOOL", "dcom.vt.bool", FT_UINT16, BASE_HEX, VALS(dcom_vt_bool_vals), 0x0, NULL, HFILL }},
		{ &hf_dcom_vt_i1,
		{ "VT_I1", "dcom.vt.i1", FT_INT8, BASE_DEC, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_vt_i2,
		{ "VT_I2", "dcom.vt.i2", FT_INT16, BASE_DEC, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_vt_i4,
		{ "VT_I4", "dcom.vt.i4", FT_INT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_vt_i8,
		{ "VT_I8", "dcom.vt.i8", FT_INT64, BASE_DEC, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_vt_cy,
		{ "VT_CY", "dcom.vt.cy", FT_INT64, BASE_DEC, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_vt_ui1,
		{ "VT_UI1", "dcom.vt.ui1", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_vt_ui2,
		{ "VT_UI2", "dcom.vt.ui2", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_vt_ui4,
		{ "VT_UI4", "dcom.vt.ui4", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_vt_ui8,
		{ "VT_UI8", "dcom.vt.ui8", FT_UINT64, BASE_DEC, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_vt_r4,
		{ "VT_R4", "dcom.vt.r4", FT_FLOAT, BASE_NONE, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_vt_r8,
		{ "VT_R8", "dcom.vt.r8", FT_DOUBLE, BASE_NONE, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_vt_date,
		{ "VT_DATE", "dcom.vt.date", FT_DOUBLE, BASE_NONE, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_vt_bstr,
		{ "VT_BSTR", "dcom.vt.bstr", FT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_vt_byref,
		{ "BYREF", "dcom.vt.byref", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_vt_dispatch,
		{ "VT_DISPATCH", "dcom.vt.dispatch", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }}
	};

	static hf_register_info hf_dcom_sa_array[] = {
		{ &hf_dcom_safearray,
		{ "SAFEARRAY", "dcom.sa", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_sa_dims32,
		{ "Dims32", "dcom.sa.dims32", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_sa_dims16,
		{ "Dims16", "dcom.sa.dims16", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_sa_features,
		{ "Features", "dcom.sa.features", FT_UINT16, BASE_HEX, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_sa_element_size,
		{ "ElementSize", "dcom.sa.element_size", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_sa_locks,
		{ "Locks", "dcom.sa.locks", FT_UINT16, BASE_HEX, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_sa_vartype32,
		{ "VarType32", "dcom.sa.vartype", FT_UINT32, BASE_DEC, VALS(dcom_variant_type_vals), 0x0, NULL, HFILL }},
		{ &hf_dcom_sa_vartype16,
		{ "VarType16", "dcom.sa.vartype", FT_UINT16, BASE_DEC, VALS(dcom_variant_type_vals), 0x0, NULL, HFILL }},
		{ &hf_dcom_sa_elements,
		{ "Elements", "dcom.sa.elements", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_sa_bound_elements,
		{ "BoundElements", "dcom.sa.bound_elements", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
		{ &hf_dcom_sa_low_bound,
		{ "LowBound", "dcom.sa.low_bound", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},

		{ &hf_dcom_sa_features_auto,
		{ "AUTO", "dcom.sa.features_auto", FT_BOOLEAN, 16, TFS (&tfs_set_notset), WIRESHARK_FADF_AUTO, NULL, HFILL }},
		{ &hf_dcom_sa_features_static,
		{ "STATIC", "dcom.sa.features_static", FT_BOOLEAN, 16, TFS (&tfs_set_notset), WIRESHARK_FADF_STATIC, NULL, HFILL }},
		{ &hf_dcom_sa_features_embedded,
		{ "EMBEDDED", "dcom.sa.features_embedded", FT_BOOLEAN, 16, TFS (&tfs_set_notset), WIRESHARK_FADF_EMBEDDED, NULL, HFILL }},
		{ &hf_dcom_sa_features_fixedsize,
		{ "FIXEDSIZE", "dcom.sa.features_fixedsize", FT_BOOLEAN, 16, TFS (&tfs_set_notset), WIRESHARK_FADF_FIXEDSIZE, NULL, HFILL }},
		{ &hf_dcom_sa_features_record,
		{ "RECORD", "dcom.sa.features_record", FT_BOOLEAN, 16, TFS (&tfs_set_notset), WIRESHARK_FADF_RECORD, NULL, HFILL }},
		{ &hf_dcom_sa_features_have_iid,
		{ "HAVEIID", "dcom.sa.features_have_iid", FT_BOOLEAN, 16, TFS (&tfs_set_notset), WIRESHARK_FADF_HAVEIID, NULL, HFILL }},
		{ &hf_dcom_sa_features_have_vartype,
		{ "HAVEVARTYPE", "dcom.sa.features_have_vartype", FT_BOOLEAN, 16, TFS (&tfs_set_notset), WIRESHARK_FADF_HAVEVARTYPE, NULL, HFILL }},
		{ &hf_dcom_sa_features_bstr,
		{ "BSTR", "dcom.sa.features_bstr", FT_BOOLEAN, 16, TFS (&tfs_set_notset), WIRESHARK_FADF_BSTR, NULL, HFILL }},
		{ &hf_dcom_sa_features_unknown,
		{ "UNKNOWN", "dcom.sa.features_unknown", FT_BOOLEAN, 16, TFS (&tfs_set_notset), WIRESHARK_FADF_UNKNOWN, NULL, HFILL }},
		{ &hf_dcom_sa_features_dispatch,
		{ "DISPATCH", "dcom.sa.features_dispatch", FT_BOOLEAN, 16, TFS (&tfs_set_notset), WIRESHARK_FADF_DISPATCH, NULL, HFILL }},
		{ &hf_dcom_sa_features_variant,
		{ "VARIANT", "dcom.sa.features_variant", FT_BOOLEAN, 16, TFS (&tfs_set_notset), WIRESHARK_FADF_VARIANT, NULL, HFILL }}
	};

	static gint *ett_dcom[] = {
		&ett_dcom_this,
		&ett_dcom_that,
		&ett_dcom_extent,
		&ett_dcom_lpwstr,
		&ett_dcom_interface_pointer,
		&ett_dcom_objref,
		&ett_dcom_stdobjref,
		&ett_dcom_custobjref,
		&ett_dcom_dualstringarray,
		&ett_dcom_dualstringarray_binding,
		&ett_dcom_variant,
		&ett_dcom_safearray,
		&ett_dcom_sa_features,
	};

	static ei_register_info ei[] = {
		{ &ei_dcom_dissetion_incomplete, { "dcom.dissetion_incomplete", PI_UNDECODED, PI_WARN, "Dissection incomplete", EXPFILL }},
		{ &ei_dcom_no_spec, { "dcom.no_spec", PI_UNDECODED, PI_NOTE, "No specification available, dissection not possible", EXPFILL }},
		{ &ei_dcom_hresult_expert, { "dcom.hresult.expert", PI_RESPONSE_CODE, PI_NOTE, "Hresult", EXPFILL }},
		{ &ei_dcom_dualstringarray_mult_ip, { "dcom.dualstringarray.mult_ip", PI_UNDECODED, PI_NOTE, "DUALSTRINGARRAY Multiple IP", EXPFILL }},
	};

	module_t *dcom_module;
	expert_module_t* expert_dcom;

	/* currently, the DCOM protocol "itself" has no real protocol dissector */
	/* we only need this, to register some generic elements */
	proto_dcom = proto_register_protocol ("DCOM", "DCOM", "dcom");
	proto_register_field_array(proto_dcom, hf_dcom_this_array, array_length(hf_dcom_this_array));
	proto_register_field_array(proto_dcom, hf_dcom_that_array, array_length(hf_dcom_that_array));
	proto_register_field_array(proto_dcom, hf_dcom_extent_array, array_length(hf_dcom_extent_array));
	proto_register_field_array(proto_dcom, hf_dcom_array, array_length(hf_dcom_array));
	proto_register_field_array(proto_dcom, hf_dcom_objref_array, array_length(hf_dcom_objref_array));
	proto_register_field_array(proto_dcom, hf_dcom_stdobjref_array, array_length(hf_dcom_stdobjref_array));
	proto_register_field_array(proto_dcom, hf_dcom_custobjref_array, array_length(hf_dcom_custobjref_array));
	proto_register_field_array(proto_dcom, hf_dcom_dualstringarray_array, array_length(hf_dcom_dualstringarray_array));
	proto_register_field_array(proto_dcom, hf_dcom_interface_pointer_array, array_length(hf_dcom_interface_pointer_array));
	proto_register_field_array(proto_dcom, hf_dcom_vt_array, array_length(hf_dcom_vt_array));
	proto_register_field_array(proto_dcom, hf_dcom_sa_array, array_length(hf_dcom_sa_array));
	proto_register_subtree_array (ett_dcom, array_length (ett_dcom));

	expert_dcom = expert_register_protocol(proto_dcom);
	expert_register_field_array(expert_dcom, ei, array_length(ei));

	/* preferences */
	dcom_module = prefs_register_protocol(proto_dcom, NULL);

	prefs_register_bool_preference(dcom_module, "display_unmarshalling_details",
		"Display DCOM unmarshalling details",
		"Display some DCOM unmarshalled fields "
		"usually hidden",
		&dcom_prefs_display_unmarshalling_details);

	register_cleanup_routine(dcom_cleanup);
}


void
proto_reg_handoff_dcom (void)
{
	/* register some "well known" UUID's */
	guids_add_uuid(&uuid_debug_ext, "Debug Information Body Extension");
	guids_add_uuid(&uuid_ext_error_ext, "Extended Error Info Body Extension");
	guids_add_uuid(&ipid_rem_unknown, "IRemUnknown");
	guids_add_uuid(&iid_unknown, "IUnknown");
	guids_add_uuid(&uuid_null, "NULL");
	guids_add_uuid(&iid_class_factory, "IClassFactory");

	/* Currently, we have nothing to register for DCOM */
}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: t
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 noexpandtab:
 * :indentSize=8:tabSize=8:noTabs=false:
 */
