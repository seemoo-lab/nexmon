/* packet-rpc.c
 * Routines for rpc dissection
 * Copyright 1999, Uwe Girlich <Uwe.Girlich@philosys.de>
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * Copied from packet-smb.c
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

#include <stdio.h>
#include <epan/packet.h>
#include <epan/expert.h>
#include <epan/exceptions.h>
#include <wsutil/pint.h>
#include <wsutil/str_util.h>
#include <epan/prefs.h>
#include <epan/reassemble.h>
#include <epan/tap.h>
#include <epan/stat_tap_ui.h>
#include <epan/srt_table.h>
#include <epan/strutil.h>
#include <epan/show_exception.h>

#include "packet-rpc.h"
#include "packet-tcp.h"
#include "packet-nfs.h"
#include "packet-dcerpc.h"
#include "packet-gssapi.h"

/*
 * See:
 *
 *	RFC 1831, "RPC: Remote Procedure Call Protocol Specification
 *	Version 2";
 *
 *	RFC 1832, "XDR: External Data Representation Standard";
 *
 *	RFC 2203, "RPCSEC_GSS Protocol Specification".
 *
 * See also
 *
 *	RFC 2695, "Authentication Mechanisms for ONC RPC"
 *
 *	although we don't currently dissect AUTH_DES or AUTH_KERB.
 *
 *	RFC 5531, "Appendix C: Current Number Assignments" defines AUTH_RSA.
 *	AUTH_RSA is not implemented for any known RPC-protocols. The Gluster
 *	protocols (ab)use AUTH_RSA for their own AUTH-flavor. AUTH_RSA is
 *	therefore dissected as the inofficial AUTH_GLUSTER.
 */
void proto_register_rpc(void);
void proto_reg_handoff_rpc(void);

/* desegmentation of RPC over TCP */
static gboolean rpc_desegment = TRUE;

/* defragmentation of fragmented RPC over TCP records */
static gboolean rpc_defragment = TRUE;

/* try to dissect RPC packets for programs that are not known
 * (proprietary ones) by wireshark.
 */
static gboolean rpc_dissect_unknown_programs = FALSE;

/* try to find RPC fragment start if normal decode fails
 * (good when starting decode of mid-stream capture)
 */
static gboolean rpc_find_fragment_start = FALSE;

static int rpc_tap = -1;

static dissector_handle_t spnego_krb5_wrap_handle = NULL;

static const value_string rpc_msg_type[] = {
	{ RPC_CALL,		     "Call" },
	{ RPC_REPLY,		     "Reply" },
	{ 0, NULL }
};

static const value_string rpc_reply_state[] = {
	{ MSG_ACCEPTED,		     "accepted" },
	{ MSG_DENIED,		     "denied" },
	{ 0, NULL }
};

const value_string rpc_auth_flavor[] = {
	{ AUTH_NULL,		     "AUTH_NULL" },
	{ AUTH_UNIX,		     "AUTH_UNIX" },
	{ AUTH_SHORT,		     "AUTH_SHORT" },
	{ AUTH_DES,		     "AUTH_DES" },
	{ AUTH_RSA,		     "AUTH_RSA/Gluster" },
	{ RPCSEC_GSS,		     "RPCSEC_GSS" },
	{ AUTH_GSSAPI,		     "AUTH_GSSAPI" },
	{ RPCSEC_GSS_KRB5,	     "RPCSEC_GSS_KRB5" },
	{ RPCSEC_GSS_KRB5I,	     "RPCSEC_GSS_KRB5I" },
	{ RPCSEC_GSS_KRB5P,	     "RPCSEC_GSS_KRB5P" },
	{ RPCSEC_GSS_LIPKEY,	     "RPCSEC_GSS_LIPKEY" },
	{ RPCSEC_GSS_LIPKEY_I,	     "RPCSEC_GSS_LIPKEY_I" },
	{ RPCSEC_GSS_LIPKEY_P,	     "RPCSEC_GSS_LIPKEY_P" },
	{ RPCSEC_GSS_SPKM3,	     "RPCSEC_GSS_SPKM3" },
	{ RPCSEC_GSS_SPKM3I,	     "RPCSEC_GSS_SPKM3I" },
	{ RPCSEC_GSS_SPKM3P,	     "RPCSEC_GSS_SPKM3P" },
	{ AUTH_GLUSTERFS,	     "AUTH_GLUSTERFS" },
	{ 0, NULL }
};

static const value_string rpc_authgss_proc[] = {
	{ RPCSEC_GSS_DATA,	     "RPCSEC_GSS_DATA" },
	{ RPCSEC_GSS_INIT,	     "RPCSEC_GSS_INIT" },
	{ RPCSEC_GSS_CONTINUE_INIT,  "RPCSEC_GSS_CONTINUE_INIT" },
	{ RPCSEC_GSS_DESTROY,	     "RPCSEC_GSS_DESTROY" },
	{ 0, NULL }
};

static const value_string rpc_authgssapi_proc[] = {
	{ AUTH_GSSAPI_EXIT,	     "AUTH_GSSAPI_EXIT" },
	{ AUTH_GSSAPI_INIT,	     "AUTH_GSSAPI_INIT" },
	{ AUTH_GSSAPI_CONTINUE_INIT, "AUTH_GSSAPI_CONTINUE_INIT" },
	{ AUTH_GSSAPI_MSG,	     "AUTH_GSSAPI_MSG" },
	{ AUTH_GSSAPI_DESTROY,	     "AUTH_GSSAPI_DESTROY" },
	{ 0, NULL }
};

const value_string rpc_authgss_svc[] = {
	{ RPCSEC_GSS_SVC_NONE,	     "rpcsec_gss_svc_none" },
	{ RPCSEC_GSS_SVC_INTEGRITY,  "rpcsec_gss_svc_integrity" },
	{ RPCSEC_GSS_SVC_PRIVACY,    "rpcsec_gss_svc_privacy" },
	{ 0, NULL }
};

static const value_string rpc_accept_state[] = {
	{ SUCCESS,		     "RPC executed successfully" },
	{ PROG_UNAVAIL,		     "remote hasn't exported program" },
	{ PROG_MISMATCH,	     "remote can't support version #" },
	{ PROC_UNAVAIL,		     "program can't support procedure" },
	{ GARBAGE_ARGS,		     "procedure can't decode params" },
	{ SYSTEM_ERROR,		     "system errors like memory allocation failure" },
	{ 0, NULL }
};

static const value_string rpc_reject_state[] = {
	{ RPC_MISMATCH,		     "RPC_MISMATCH" },
	{ AUTH_ERROR,		     "AUTH_ERROR" },
	{ 0, NULL }
};

static const value_string rpc_auth_state[] = {
	{ AUTH_BADCRED,		     "bad credential (seal broken)" },
	{ AUTH_REJECTEDCRED,	     "client must begin new session" },
	{ AUTH_BADVERF,		     "bad verifier (seal broken)" },
	{ AUTH_REJECTEDVERF,	     "verifier expired or replayed" },
	{ AUTH_TOOWEAK,		     "rejected for security reasons" },
	{ RPCSEC_GSSCREDPROB,	     "GSS credential problem" },
	{ RPCSEC_GSSCTXPROB,	     "GSS context problem" },
	{ 0, NULL }
};

static const value_string rpc_authdes_namekind[] = {
	{ AUTHDES_NAMEKIND_FULLNAME, "ADN_FULLNAME" },
	{ AUTHDES_NAMEKIND_NICKNAME, "ADN_NICKNAME" },
	{ 0, NULL }
};

/* the protocol number */
static int proto_rpc = -1;
static int hf_rpc_reqframe = -1;
static int hf_rpc_repframe = -1;
static int hf_rpc_lastfrag = -1;
static int hf_rpc_fraglen = -1;
static int hf_rpc_xid = -1;
static int hf_rpc_msgtype = -1;
static int hf_rpc_version = -1;
static int hf_rpc_version_min = -1;
static int hf_rpc_version_max = -1;
static int hf_rpc_program = -1;
static int hf_rpc_programversion = -1;
static int hf_rpc_programversion_min = -1;
static int hf_rpc_programversion_max = -1;
static int hf_rpc_procedure = -1;
static int hf_rpc_auth_flavor = -1;
static int hf_rpc_auth_length = -1;
static int hf_rpc_auth_machinename = -1;
static int hf_rpc_auth_stamp = -1;
static int hf_rpc_auth_lk_owner = -1;
static int hf_rpc_auth_pid = -1;
static int hf_rpc_auth_uid = -1;
static int hf_rpc_auth_gid = -1;
static int hf_rpc_authgss_v = -1;
static int hf_rpc_authgss_proc = -1;
static int hf_rpc_authgss_seq = -1;
static int hf_rpc_authgss_svc = -1;
static int hf_rpc_authgss_ctx = -1;
static int hf_rpc_authgss_ctx_create_frame = -1;
static int hf_rpc_authgss_ctx_destroy_frame = -1;
static int hf_rpc_authgss_ctx_len = -1;
static int hf_rpc_authgss_major = -1;
static int hf_rpc_authgss_minor = -1;
static int hf_rpc_authgss_window = -1;
static int hf_rpc_authgss_token_length = -1;
static int hf_rpc_authgss_data_length = -1;
static int hf_rpc_authgss_data = -1;
static int hf_rpc_authgss_token = -1;
static int hf_rpc_authgss_checksum = -1;
static int hf_rpc_authgssapi_v = -1;
static int hf_rpc_authgssapi_msg = -1;
static int hf_rpc_authgssapi_msgv = -1;
static int hf_rpc_authgssapi_handle = -1;
static int hf_rpc_authgssapi_isn = -1;
static int hf_rpc_authdes_namekind = -1;
static int hf_rpc_authdes_netname = -1;
static int hf_rpc_authdes_convkey = -1;
static int hf_rpc_authdes_window = -1;
static int hf_rpc_authdes_nickname = -1;
static int hf_rpc_authdes_timestamp = -1;
static int hf_rpc_authdes_windowverf = -1;
static int hf_rpc_authdes_timeverf = -1;
static int hf_rpc_state_accept = -1;
static int hf_rpc_state_reply = -1;
static int hf_rpc_state_reject = -1;
static int hf_rpc_state_auth = -1;
static int hf_rpc_dup = -1;
static int hf_rpc_call_dup = -1;
static int hf_rpc_reply_dup = -1;
static int hf_rpc_value_follows = -1;
static int hf_rpc_array_len = -1;
static int hf_rpc_time = -1;
static int hf_rpc_fragments = -1;
static int hf_rpc_fragment = -1;
static int hf_rpc_fragment_overlap = -1;
static int hf_rpc_fragment_overlap_conflict = -1;
static int hf_rpc_fragment_multiple_tails = -1;
static int hf_rpc_fragment_too_long_fragment = -1;
static int hf_rpc_fragment_error = -1;
static int hf_rpc_fragment_count = -1;
static int hf_rpc_reassembled_length = -1;
static int hf_rpc_unknown_body = -1;

/* Generated from convert_proto_tree_add_text.pl */
static int hf_rpc_opaque_data = -1;
static int hf_rpc_no_values = -1;
static int hf_rpc_continuation_data = -1;
static int hf_rpc_fill_bytes = -1;
static int hf_rpc_argument_length = -1;
static int hf_rpc_fragment_data = -1;
static int hf_rpc_opaque_length = -1;

static gint ett_rpc = -1;
static gint ett_rpc_unknown_program = -1;
static gint ett_rpc_fragments = -1;
static gint ett_rpc_fragment = -1;
static gint ett_rpc_fraghdr = -1;
static gint ett_rpc_string = -1;
static gint ett_rpc_cred = -1;
static gint ett_rpc_verf = -1;
static gint ett_rpc_gids = -1;
static gint ett_rpc_gss_token = -1;
static gint ett_rpc_gss_data = -1;
static gint ett_rpc_array = -1;
static gint ett_rpc_authgssapi_msg = -1;
static gint ett_gss_context = -1;
static gint ett_gss_wrap = -1;

static expert_field ei_rpc_cannot_dissect = EI_INIT;

static dissector_handle_t rpc_tcp_handle;
static dissector_handle_t rpc_handle;
static dissector_handle_t gssapi_handle;
static dissector_handle_t data_handle;

static dissector_table_t  subdissector_call_table;
static dissector_table_t  subdissector_reply_table;


static guint max_rpc_tcp_pdu_size = 4 * 1024 * 1024;

static const fragment_items rpc_frag_items = {
	&ett_rpc_fragment,
	&ett_rpc_fragments,
	&hf_rpc_fragments,
	&hf_rpc_fragment,
	&hf_rpc_fragment_overlap,
	&hf_rpc_fragment_overlap_conflict,
	&hf_rpc_fragment_multiple_tails,
	&hf_rpc_fragment_too_long_fragment,
	&hf_rpc_fragment_error,
	&hf_rpc_fragment_count,
	NULL,
	&hf_rpc_reassembled_length,
	/* Reassembled data field */
	NULL,
	"fragments"
};

/* Hash table with info on RPC program numbers */
GHashTable *rpc_progs = NULL;

typedef gboolean (*rec_dissector_t)(tvbuff_t *, packet_info *, proto_tree *,
	tvbuff_t *, fragment_head *, gboolean, guint32, gboolean);

static void show_rpc_fraginfo(tvbuff_t *tvb, tvbuff_t *frag_tvb, proto_tree *tree,
			      guint32 rpc_rm, fragment_head *ipfd_head, packet_info *pinfo);
static char *rpc_proc_name_internal(wmem_allocator_t *allocator, guint32 prog,
	guint32 vers, guint32 proc);


static guint32 rpc_program = 0;
static guint32 rpc_version = 0;
static gint32 rpc_min_proc = -1;
static gint32 rpc_max_proc = -1;

static void
rpcstat_find_procs(const gchar *table_name _U_, ftenum_t selector_type _U_, gpointer key, gpointer value _U_, gpointer user_data _U_)
{
	rpc_proc_info_key *k = (rpc_proc_info_key *)key;

	if (k->prog != rpc_program) {
		return;
	}
	if (k->vers != rpc_version) {
		return;
	}
	if (rpc_min_proc == -1) {
		rpc_min_proc = k->proc;
		rpc_max_proc = k->proc;
	}
	if ((gint32)k->proc < rpc_min_proc) {
		rpc_min_proc = k->proc;
	}
	if ((gint32)k->proc > rpc_max_proc) {
		rpc_max_proc = k->proc;
	}
}

static void
rpcstat_init(struct register_srt* srt, GArray* srt_array, srt_gui_init_cb gui_callback, void* gui_data)
{
	rpcstat_tap_data_t* tap_data = (rpcstat_tap_data_t*)get_srt_table_param_data(srt);
	srt_stat_table *rpc_srt_table;
	int i, hf_index;
	header_field_info *hfi;
	static char table_name[100];

	DISSECTOR_ASSERT(tap_data);

	hf_index=rpc_prog_hf(tap_data->program, tap_data->version);
	hfi=proto_registrar_get_nth(hf_index);

	g_snprintf(table_name, sizeof(table_name), "%s Version %u", tap_data->prog, tap_data->version);
	rpc_srt_table = init_srt_table(table_name, NULL, srt_array, tap_data->num_procedures, NULL, hfi->abbrev, gui_callback, gui_data, tap_data);
	for (i = 0; i < rpc_srt_table->num_procs; i++)
	{
		char *proc_name = rpc_proc_name_internal(NULL, tap_data->program, tap_data->version, i);
		init_srt_table_row(rpc_srt_table, i, proc_name);
		wmem_free(NULL, proc_name);
	}
}

static int
rpcstat_packet(void *pss, packet_info *pinfo, epan_dissect_t *edt _U_, const void *prv)
{
	guint i = 0;
	srt_stat_table *rpc_srt_table;
	srt_data_t *data = (srt_data_t *)pss;
	const rpc_call_info_value *ri = (const rpc_call_info_value *)prv;
	rpcstat_tap_data_t* tap_data;

	rpc_srt_table = g_array_index(data->srt_array, srt_stat_table*, i);
	tap_data = (rpcstat_tap_data_t*)rpc_srt_table->table_specific_data;

	if ((int)ri->proc >= rpc_srt_table->num_procs) {
		/* don't handle this since its outside of known table */
		return 0;
	}
	/* we are only interested in reply packets */
	if (ri->request) {
		return 0;
	}
	/* we are only interested in certain program/versions */
	if ( (ri->prog != tap_data->program) || (ri->vers != tap_data->version) ) {
		return 0;
	}

	add_srt_table_data(rpc_srt_table, ri->proc, &ri->req_time, pinfo);
	return 1;

}

static guint
rpcstat_param(register_srt_t* srt, const char* opt_arg, char** err)
{
	int pos = 0;
	int program, version;
	rpcstat_tap_data_t* tap_data;

	if (sscanf(opt_arg, ",%d,%d%n", &program, &version, &pos) == 2)
	{
		tap_data = g_new0(rpcstat_tap_data_t, 1);

		tap_data->prog    = rpc_prog_name(program);
		tap_data->program = program;
		tap_data->version = version;

		set_srt_table_param_data(srt, tap_data);

		rpc_program  = tap_data->program;
		rpc_version  = tap_data->version;
		rpc_min_proc = -1;
		rpc_max_proc = -1;
		/* Need to run over both dissector tables */
		dissector_table_foreach ("rpc.call", rpcstat_find_procs, NULL);
		dissector_table_foreach ("rpc.reply", rpcstat_find_procs, NULL);

		tap_data->num_procedures = rpc_max_proc+1;
		if (rpc_min_proc == -1) {
			*err = g_strdup_printf("Program:%u version:%u isn't supported", rpc_program, rpc_version);
		}
	}
	else
	{
		*err = g_strdup("<program>,<version>[,<filter>]");
	}

	return pos;
}





/***********************************/
/* Hash array with procedure names */
/***********************************/

/* compare 2 keys */
static gint
rpc_proc_equal(gconstpointer k1, gconstpointer k2)
{
	const rpc_proc_info_key* key1 = (const rpc_proc_info_key*) k1;
	const rpc_proc_info_key* key2 = (const rpc_proc_info_key*) k2;

	return ((key1->prog == key2->prog &&
		key1->vers == key2->vers &&
		key1->proc == key2->proc) ?
	TRUE : FALSE);
}

/* calculate a hash key */
static guint
rpc_proc_hash(gconstpointer k)
{
	const rpc_proc_info_key* key = (const rpc_proc_info_key*) k;

	return (key->prog ^ (key->vers<<16) ^ (key->proc<<24));
}


/*	return the name associated with a previously registered procedure. */
static char *
rpc_proc_name_internal(wmem_allocator_t *allocator, guint32 prog, guint32 vers, guint32 proc)
{
	rpc_proc_info_key key;
	dissector_handle_t dissect_function;
	char *procname;

	key.prog = prog;
	key.vers = vers;
	key.proc = proc;

	/* Look at both tables for possible procedure names */
	if ((dissect_function = dissector_get_custom_table_handle(subdissector_call_table, &key)) != NULL)
		procname = wmem_strdup(allocator, dissector_handle_get_dissector_name(dissect_function));
	else if ((dissect_function = dissector_get_custom_table_handle(subdissector_reply_table, &key)) != NULL)
		procname = wmem_strdup(allocator, dissector_handle_get_dissector_name(dissect_function));
	else {
		/* happens only with strange program versions or
		   non-existing dissectors */
		procname = wmem_strdup_printf(allocator, "proc-%u", key.proc);
	}
	return procname;
}

const char *
rpc_proc_name(guint32 prog, guint32 vers, guint32 proc)
{
	return rpc_proc_name_internal(wmem_packet_scope(), prog, vers, proc);
}

/*----------------------------------------*/
/* end of Hash array with procedure names */
/*----------------------------------------*/


/*********************************/
/* Hash array with program names */
/*********************************/

static void
rpc_prog_free_val(gpointer v)
{
	rpc_prog_info_value *value = (rpc_prog_info_value*)v;

	g_array_free(value->procedure_hfs, TRUE);
	g_free(value);
}

void
rpc_init_prog(int proto, guint32 prog, int ett, size_t nvers,
    const rpc_prog_vers_info *versions)
{
	rpc_prog_info_value *value;
	size_t versidx;
	const vsff *proc;

	value = (rpc_prog_info_value *) g_malloc(sizeof(rpc_prog_info_value));
	value->proto = find_protocol_by_id(proto);
	value->proto_id = proto;
	value->ett = ett;
	value->progname = proto_get_protocol_short_name(value->proto);
	value->procedure_hfs = g_array_new(FALSE, TRUE, sizeof (int));

	g_hash_table_insert(rpc_progs,GUINT_TO_POINTER(prog),value);

	/*
	 * Now register each of the versions of the program.
	 */
	for (versidx = 0; versidx < nvers; versidx++) {
		/*
		 * Add the operation number hfinfo value for this version.
		 */
		value->procedure_hfs = g_array_set_size(value->procedure_hfs,
		    versions[versidx].vers);
		g_array_insert_val(value->procedure_hfs,
		    versions[versidx].vers, *versions[versidx].procedure_hf);

		for (proc = versions[versidx].proc_table; proc->strptr != NULL;
		    proc++) {
			rpc_proc_info_key key;

			key.prog = prog;
			key.vers = versions[versidx].vers;
			key.proc = proc->value;

			if (proc->dissect_call == NULL) {
				fprintf(stderr, "OOPS: No call handler for %s version %u procedure %s\n",
				    proto_get_protocol_long_name(value->proto),
				    versions[versidx].vers,
				    proc->strptr);

				/* Abort out if desired - but don't throw an exception here! */
				if (getenv("WIRESHARK_ABORT_ON_DISSECTOR_BUG") != NULL)
					REPORT_DISSECTOR_BUG("RPC: No call handler!");

				continue;
			}
			dissector_add_custom_table_handle("rpc.call", g_memdup(&key, sizeof(rpc_proc_info_key)),
						create_dissector_handle_with_name(proc->dissect_call, value->proto_id, proc->strptr));

			if (proc->dissect_reply == NULL) {
				fprintf(stderr, "OOPS: No reply handler for %s version %u procedure %s\n",
				    proto_get_protocol_long_name(value->proto),
				    versions[versidx].vers,
				    proc->strptr);

				/* Abort out if desired - but don't throw an exception here! */
				if (getenv("WIRESHARK_ABORT_ON_DISSECTOR_BUG") != NULL)
					REPORT_DISSECTOR_BUG("RPC: No reply handler!");

				continue;
			}
			dissector_add_custom_table_handle("rpc.reply", g_memdup(&key, sizeof(rpc_proc_info_key)),
					create_dissector_handle_with_name(proc->dissect_reply, value->proto_id, proc->strptr));
		}
	}
}



/*	return the hf_field associated with a previously registered program.
*/
int
rpc_prog_hf(guint32 prog, guint32 vers)
{
	rpc_prog_info_value     *rpc_prog;

	if ((rpc_prog = (rpc_prog_info_value *)g_hash_table_lookup(rpc_progs,GUINT_TO_POINTER(prog)))) {
		return g_array_index(rpc_prog->procedure_hfs, int, vers);
	}
	return -1;
}

/*	return the name associated with a previously registered program. This
	should probably eventually be expanded to use the rpc YP/NIS map
	so that it can give names for programs not handled by wireshark */
const char *
rpc_prog_name(guint32 prog)
{
	const char *progname = NULL;
	rpc_prog_info_value     *rpc_prog;

	if ((rpc_prog = (rpc_prog_info_value *)g_hash_table_lookup(rpc_progs,GUINT_TO_POINTER(prog))) == NULL) {
		progname = "Unknown";
	}
	else {
		progname = rpc_prog->progname;
	}
	return progname;
}


/*--------------------------------------*/
/* end of Hash array with program names */
/*--------------------------------------*/

/* One of these structures are created for each conversation that contains
 * RPC and contains the state we need to maintain for the conversation.
 */
typedef struct _rpc_conv_info_t {
	wmem_tree_t *xids;
} rpc_conv_info_t;


/* we can not hang this off the conversation structure above since the context
   will be reused across all tcp connections between the client and the server.
   a global tree for all contexts should still be unlikely to have collissions
   here.
*/
wmem_tree_t *authgss_contexts = NULL;

unsigned int
rpc_roundup(unsigned int a)
{
	unsigned int mod = a % 4;
	unsigned int ret;
	ret = a + ((mod)? 4-mod : 0);
	/* Check for overflow */
	if (ret < a)
		THROW(ReportedBoundsError);
	return ret;
}


int
dissect_rpc_bool(tvbuff_t *tvb, proto_tree *tree,
		 int hfindex, int offset)
{
	proto_tree_add_item(tree, hfindex, tvb, offset, 4, ENC_BIG_ENDIAN);
	return offset + 4;
}


int
dissect_rpc_uint32(tvbuff_t *tvb, proto_tree *tree,
		   int hfindex, int offset)
{
	proto_tree_add_item(tree, hfindex, tvb, offset, 4, ENC_BIG_ENDIAN);
	return offset + 4;
}


int
dissect_rpc_uint64(tvbuff_t *tvb, proto_tree *tree,
		   int hfindex, int offset)
{
	header_field_info	*hfinfo;

	hfinfo = proto_registrar_get_nth(hfindex);
	DISSECTOR_ASSERT(hfinfo->type == FT_UINT64);
	proto_tree_add_item(tree, hfindex, tvb, offset, 8, ENC_BIG_ENDIAN);

	return offset + 8;
}

/*
 * We want to make this function available outside this file and
 * allow callers to pass a dissection function for the opaque data
 */
int
dissect_rpc_opaque_data(tvbuff_t *tvb, int offset,
			proto_tree *tree,
			packet_info *pinfo,
			int hfindex,
			gboolean fixed_length, guint32 length,
			gboolean string_data, const char **string_buffer_ret,
			dissect_function_t *dissect_it)
{
	int data_offset;
	proto_item *string_item = NULL;
	proto_tree *string_tree = NULL;

	guint32 string_length;
	guint32 string_length_full;
	guint32 string_length_packet;
	guint32 string_length_captured;
	guint32 string_length_copy;

	int fill_truncated;
	guint32 fill_length;
	guint32 fill_length_packet;
	guint32 fill_length_captured;
	guint32 fill_length_copy;

	int exception = 0;
	/* int string_item_offset; */

	char *string_buffer = NULL;
	const char *string_buffer_print = NULL;

	if (fixed_length) {
		string_length = length;
		data_offset = offset;
	}
	else {
		string_length = tvb_get_ntohl(tvb,offset);
		data_offset = offset + 4;
	}
	string_length_captured = tvb_captured_length_remaining(tvb, data_offset);
	string_length_packet = tvb_reported_length_remaining(tvb, data_offset);
	string_length_full = rpc_roundup(string_length);
	if (string_length_captured < string_length) {
		/* truncated string */
		string_length_copy = string_length_captured;
		fill_truncated = 2;
		fill_length = 0;
		fill_length_copy = 0;
		if (string_length_packet < string_length)
			exception = ReportedBoundsError;
		else
			exception = BoundsError;
	}
	else {
		/* full string data */
		string_length_copy = string_length;
		fill_length = string_length_full - string_length;
		fill_length_captured = tvb_captured_length_remaining(tvb,
		    data_offset + string_length);
		fill_length_packet = tvb_reported_length_remaining(tvb,
		    data_offset + string_length);
		if (fill_length_captured < fill_length) {
			/* truncated fill bytes */
			fill_length_copy = fill_length_packet;
			fill_truncated = 1;
			if (fill_length_packet < fill_length)
				exception = ReportedBoundsError;
			else
				exception = BoundsError;
		}
		else {
			/* full fill bytes */
			fill_length_copy = fill_length;
			fill_truncated = 0;
		}
	}

	/*
	 * If we were passed a dissection routine, make a TVB of the data
	 * and call the dissection routine
	 */

	if (dissect_it) {
		tvbuff_t *opaque_tvb;

		opaque_tvb = tvb_new_subset(tvb, data_offset, string_length_copy,
					    string_length);

		return (*dissect_it)(opaque_tvb, offset, pinfo, tree, NULL);

	}

	if (string_data) {
		string_buffer = tvb_get_string_enc(wmem_packet_scope(), tvb, data_offset, string_length_copy, ENC_ASCII);
	} else {
		string_buffer = (char *)tvb_memcpy(tvb, wmem_alloc(wmem_packet_scope(), string_length_copy+1), data_offset, string_length_copy);
	}
	string_buffer[string_length_copy] = '\0';
	/* calculate a nice printable string */
	if (string_length) {
		if (string_length != string_length_copy) {
			if (string_data) {
				char *formatted;

				formatted = format_text(string_buffer, strlen(string_buffer));
				/* copy over the data and append <TRUNCATED> */
				string_buffer_print=wmem_strdup_printf(wmem_packet_scope(), "%s%s", formatted, RPC_STRING_TRUNCATED);
			} else {
				string_buffer_print=RPC_STRING_DATA RPC_STRING_TRUNCATED;
			}
		} else {
			if (string_data) {
				string_buffer_print =
				    wmem_strdup(wmem_packet_scope(), format_text(string_buffer, strlen(string_buffer)));
			} else {
				string_buffer_print=RPC_STRING_DATA;
			}
		}
	} else {
		string_buffer_print=RPC_STRING_EMPTY;
	}

	if (tree) {
		/* string_item_offset = offset; */
		string_tree = proto_tree_add_subtree_format(tree, tvb,offset, -1,
		    ett_rpc_string, &string_item, "%s: %s", proto_registrar_get_name(hfindex),
		    string_buffer_print);
	}
	if (!fixed_length) {
		proto_tree_add_uint(string_tree, hf_rpc_opaque_length, tvb,offset, 4, string_length);
		offset += 4;
	}

	if (string_tree) {
		if (string_data) {
			proto_tree_add_string_format(string_tree,
			    hfindex, tvb, offset, string_length_copy,
			    string_buffer,
			    "contents: %s", string_buffer_print);
		} else {
			proto_tree_add_bytes_format(string_tree,
			    hfindex, tvb, offset, string_length_copy,
			    string_buffer,
			    "contents: %s", string_buffer_print);
		}
	}

	offset += string_length_copy;

	if (fill_length) {
		if (string_tree) {
			if (fill_truncated) {
				proto_tree_add_bytes_format_value(string_tree, hf_rpc_fill_bytes, tvb,
				offset, fill_length_copy, NULL, "opaque data<TRUNCATED>");
			}
			else {
				proto_tree_add_bytes_format_value(string_tree, hf_rpc_fill_bytes, tvb,
				offset, fill_length_copy, NULL, "opaque data");
			}
		}
		offset += fill_length_copy;
	}

	if (string_item)
		proto_item_set_end(string_item, tvb, offset);

	if (string_buffer_ret != NULL)
		*string_buffer_ret = string_buffer_print;

	/*
	 * If the data was truncated, throw the appropriate exception,
	 * so that dissection stops and the frame is properly marked.
	 */
	if (exception != 0)
		THROW(exception);
	return offset;
}


int
dissect_rpc_string(tvbuff_t *tvb, proto_tree *tree,
		   int hfindex, int offset, const char **string_buffer_ret)
{
	offset = dissect_rpc_opaque_data(tvb, offset, tree, NULL,
	    hfindex, FALSE, 0, TRUE, string_buffer_ret, NULL);
	return offset;
}


int
dissect_rpc_data(tvbuff_t *tvb, proto_tree *tree,
		 int hfindex, int offset)
{
	offset = dissect_rpc_opaque_data(tvb, offset, tree, NULL,
					 hfindex, FALSE, 0, FALSE, NULL, NULL);
	return offset;
}


int
dissect_rpc_bytes(tvbuff_t *tvb, proto_tree *tree,
		  int hfindex, int offset, guint32 length,
		  gboolean string_data, const char **string_buffer_ret)
{
	offset = dissect_rpc_opaque_data(tvb, offset, tree, NULL,
	    hfindex, TRUE, length, string_data, string_buffer_ret, NULL);
	return offset;
}


int
dissect_rpc_list(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
		 int offset, dissect_function_t *rpc_list_dissector, void *data)
{
	guint32 value_follows;

	while (1) {
		value_follows = tvb_get_ntohl(tvb, offset);
		proto_tree_add_boolean(tree,hf_rpc_value_follows, tvb,
			offset, 4, value_follows);
		offset += 4;
		if (value_follows == 1) {
			offset = rpc_list_dissector(tvb, offset, pinfo, tree,
						    data);
		}
		else {
			break;
		}
	}

	return offset;
}

int
dissect_rpc_array(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
	int offset, dissect_function_t *rpc_array_dissector,
	int hfindex)
{
	proto_item* lock_item;
	proto_tree* lock_tree;
	guint32	num;

	num = tvb_get_ntohl(tvb, offset);

	lock_item = proto_tree_add_item(tree, hfindex, tvb, offset, -1, ENC_NA);

	lock_tree = proto_item_add_subtree(lock_item, ett_rpc_array);

	if(num == 0) {
		proto_tree_add_item(lock_tree, hf_rpc_no_values, tvb, offset, 4, ENC_NA);
		offset += 4;

		proto_item_set_end(lock_item, tvb, offset);

		return offset;
	}

	offset = dissect_rpc_uint32(tvb, lock_tree,
			hf_rpc_array_len, offset);

	while (num--) {
		offset = rpc_array_dissector(tvb, offset, pinfo, lock_tree, NULL);
	}

	proto_item_set_end(lock_item, tvb, offset);
	return offset;
}

static int
dissect_rpc_authunix_groups(tvbuff_t* tvb, proto_tree* tree, int offset)
{
	guint gids_count;
	guint gids_i;
	guint gids_entry;
	proto_item *gitem = NULL;
	proto_tree *gtree = NULL;

	gids_count = tvb_get_ntohl(tvb,offset);
	gtree = proto_tree_add_subtree_format(tree, tvb, offset,
			4+gids_count*4, ett_rpc_gids, &gitem, "Auxiliary GIDs (%d)", gids_count);
	offset += 4;

	/* first, open with [ */
	if (tree && gids_count > 0)
		proto_item_append_text(gitem, " [");

	for (gids_i = 0 ; gids_i < gids_count; gids_i++) {
		gids_entry = tvb_get_ntohl(tvb,offset);
		if (gtree) {
			proto_tree_add_uint(gtree, hf_rpc_auth_gid, tvb,
				offset, 4, gids_entry);
		}

		/* add at most 16 GIDs to the text */
		if (tree && gids_i < 16) {
			if (gids_i > 0)
				proto_item_append_text(gitem, ", ");

			proto_item_append_text(gitem, "%d", gids_entry);
		} else if (tree && gids_i == 16) {
			proto_item_append_text(gitem, "...");
		}
		offset += 4;
	}

	/* finally, close with ] */
	if (tree && gids_count > 0)
		proto_item_append_text(gitem, "]");

	return offset;
}

static int
dissect_rpc_authunix_cred(tvbuff_t* tvb, proto_tree* tree, int offset)
{
	guint stamp;
	guint uid;
	guint gid;

	stamp = tvb_get_ntohl(tvb,offset);
	proto_tree_add_uint(tree, hf_rpc_auth_stamp, tvb,
			    offset, 4, stamp);
	offset += 4;

	offset = dissect_rpc_string(tvb, tree,
			hf_rpc_auth_machinename, offset, NULL);

	uid = tvb_get_ntohl(tvb,offset);
	proto_tree_add_uint(tree, hf_rpc_auth_uid, tvb,
			    offset, 4, uid);
	offset += 4;

	gid = tvb_get_ntohl(tvb,offset);
	proto_tree_add_uint(tree, hf_rpc_auth_gid, tvb,
			    offset, 4, gid);
	offset += 4;

	offset = dissect_rpc_authunix_groups(tvb, tree, offset);

	return offset;
}

typedef struct _gssauth_context_info_t {
	guint32	create_frame;
	guint32 destroy_frame;
} gssauth_context_info_t;


static int
dissect_rpc_authgss_context(proto_tree *tree, tvbuff_t *tvb, int offset,
			    packet_info *pinfo, rpc_conv_info_t *rpc_conv_info _U_,
			    gboolean is_create, gboolean is_destroy)
{
	proto_item *context_item;
	proto_tree *context_tree;
	int old_offset = offset;
	int context_offset;
	guint32 context_length;
	gssauth_context_info_t *context_info;
	wmem_tree_key_t tkey[2];
	guint32 key[4] = {0,0,0,0};

	context_tree = proto_tree_add_subtree(tree, tvb, offset, -1,
						ett_gss_context, &context_item, "GSS Context");

	context_length = tvb_get_ntohl(tvb, offset);
	proto_tree_add_item(context_tree, hf_rpc_authgss_ctx_len, tvb, offset, 4, ENC_BIG_ENDIAN);
	offset += 4;

	context_offset = offset;
	proto_tree_add_item(context_tree, hf_rpc_authgss_ctx, tvb, offset, context_length, ENC_NA);
	offset += context_length;

	offset = (offset + 3) & 0xffffffc;

	if (context_length > 16) {
		/* we only track contexts up to 16 bytes in size */
		return offset;
	}

	tvb_memcpy(tvb, key, context_offset, context_length);
	tkey[0].length = 4;
	tkey[0].key    = &key[0];
	tkey[1].length = 0;
	tkey[1].key  = NULL;

	context_info = (gssauth_context_info_t *)wmem_tree_lookup32_array(authgss_contexts, &tkey[0]);
	if(context_info == NULL) {
		tvb_memcpy(tvb, key, context_offset, context_length);
		tkey[0].length = 4;
		tkey[0].key    = &key[0];
		tkey[1].length = 0;
		tkey[1].key  = NULL;

	   	context_info = wmem_new(wmem_file_scope(), gssauth_context_info_t);
		context_info->create_frame  = 0;
		context_info->destroy_frame = 0;
		wmem_tree_insert32_array(authgss_contexts, &tkey[0], context_info);
	}
	if (is_create) {
		context_info->create_frame = pinfo->num;
	}
	if (is_destroy) {
		context_info->destroy_frame = pinfo->num;
	}

	if (context_info->create_frame) {
		proto_item *it;
		it = proto_tree_add_uint(context_tree, hf_rpc_authgss_ctx_create_frame, tvb, 0, 0, context_info->create_frame);
		PROTO_ITEM_SET_GENERATED(it);
	}

	if (context_info->destroy_frame) {
		proto_item *it;
		it = proto_tree_add_uint(context_tree, hf_rpc_authgss_ctx_destroy_frame, tvb, 0, 0, context_info->destroy_frame);
		PROTO_ITEM_SET_GENERATED(it);
	}

	proto_item_set_len(context_item, offset - old_offset);

	return offset;
}

static int
dissect_rpc_authgss_cred(tvbuff_t* tvb, proto_tree* tree, int offset,
			 packet_info *pinfo, rpc_conv_info_t *rpc_conv_info)
{
	guint agc_v;
	guint agc_proc;
	guint agc_seq;
	guint agc_svc;

	agc_v = tvb_get_ntohl(tvb, offset);
	proto_tree_add_uint(tree, hf_rpc_authgss_v,
			    tvb, offset, 4, agc_v);
	offset += 4;

	agc_proc = tvb_get_ntohl(tvb, offset);
	proto_tree_add_uint(tree, hf_rpc_authgss_proc,
			    tvb, offset, 4, agc_proc);
	offset += 4;

	agc_seq = tvb_get_ntohl(tvb, offset);
	proto_tree_add_uint(tree, hf_rpc_authgss_seq,
			    tvb, offset, 4, agc_seq);
	offset += 4;

	agc_svc = tvb_get_ntohl(tvb, offset);
	proto_tree_add_uint(tree, hf_rpc_authgss_svc,
			    tvb, offset, 4, agc_svc);
	offset += 4;

	offset = dissect_rpc_authgss_context(tree, tvb, offset, pinfo, rpc_conv_info, FALSE, agc_proc == RPCSEC_GSS_DESTROY ? TRUE : FALSE);

	return offset;
}

static int
dissect_rpc_authdes_desblock(tvbuff_t *tvb, proto_tree *tree,
			     int hfindex, int offset)
{
	proto_tree_add_item(tree, hfindex, tvb, offset, 8, ENC_BIG_ENDIAN);

	return offset + 8;
}

static int
dissect_rpc_authdes_cred(tvbuff_t* tvb, proto_tree* tree, int offset)
{
	guint adc_namekind;
	guint window = 0;
	guint nickname = 0;

	adc_namekind = tvb_get_ntohl(tvb, offset);
	proto_tree_add_uint(tree, hf_rpc_authdes_namekind,
			    tvb, offset, 4, adc_namekind);
	offset += 4;

	switch(adc_namekind)
	{
	case AUTHDES_NAMEKIND_FULLNAME:
		offset = dissect_rpc_string(tvb, tree,
			hf_rpc_authdes_netname, offset, NULL);
		offset = dissect_rpc_authdes_desblock(tvb, tree,
			hf_rpc_authdes_convkey, offset);
		window = tvb_get_ntohl(tvb, offset);
		proto_tree_add_uint(tree, hf_rpc_authdes_window, tvb, offset, 4,
			window);
		offset += 4;
		break;

	case AUTHDES_NAMEKIND_NICKNAME:
		nickname = tvb_get_ntohl(tvb, offset);
		proto_tree_add_uint(tree, hf_rpc_authdes_nickname, tvb, offset, 4,
			nickname);
		offset += 4;
		break;
	}

	return offset;
}

static int
dissect_rpc_authgluster_cred(tvbuff_t* tvb, proto_tree* tree, int offset)
{
	proto_tree_add_item(tree, hf_rpc_auth_lk_owner, tvb, offset,
			    8, ENC_NA);
	offset += 8;

	offset = dissect_rpc_uint32(tvb, tree, hf_rpc_auth_pid, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_rpc_auth_uid, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_rpc_auth_gid, offset);
	offset = dissect_rpc_authunix_groups(tvb, tree, offset);

	return offset;
}

static int
dissect_rpc_authglusterfs_v2_cred(tvbuff_t* tvb, proto_tree* tree, int offset)
{
	int len;

	offset = dissect_rpc_uint32(tvb, tree, hf_rpc_auth_pid, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_rpc_auth_uid, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_rpc_auth_gid, offset);
	offset = dissect_rpc_authunix_groups(tvb, tree, offset);

	len = tvb_get_ntohl(tvb, offset);
	offset += 4;

	proto_tree_add_item(tree, hf_rpc_auth_lk_owner, tvb, offset,
			    len, ENC_NA);
	offset += len;

	return offset;
}

static int
dissect_rpc_authgssapi_cred(tvbuff_t* tvb, proto_tree* tree, int offset)
{
	proto_tree_add_item(tree, hf_rpc_authgssapi_v, tvb, offset, 4, ENC_BIG_ENDIAN);
	offset += 4;

	proto_tree_add_item(tree, hf_rpc_authgssapi_msg, tvb, offset, 4, ENC_BIG_ENDIAN);
	offset += 4;

	offset = dissect_rpc_data(tvb, tree, hf_rpc_authgssapi_handle,
			offset);

	return offset;
}

static int
dissect_rpc_cred(tvbuff_t* tvb, proto_tree* tree, int offset,
		 packet_info *pinfo, rpc_conv_info_t *rpc_conv_info)
{
	guint flavor;
	guint length;

	proto_tree *ctree;

	flavor = tvb_get_ntohl(tvb,offset);
	length = tvb_get_ntohl(tvb,offset+4);
	length = rpc_roundup(length);

	if (tree) {
		ctree = proto_tree_add_subtree(tree, tvb, offset,
					    8+length, ett_rpc_cred, NULL, "Credentials");
		proto_tree_add_uint(ctree, hf_rpc_auth_flavor, tvb,
				    offset, 4, flavor);
		proto_tree_add_uint(ctree, hf_rpc_auth_length, tvb,
				    offset+4, 4, length);

		switch (flavor) {
		case AUTH_UNIX:
			dissect_rpc_authunix_cred(tvb, ctree, offset+8);
			break;
		/*
		case AUTH_SHORT:

		break;
		*/
		case AUTH_DES:
			dissect_rpc_authdes_cred(tvb, ctree, offset+8);
			break;

		case AUTH_RSA:
			/* AUTH_RSA is (ab)used by Gluster */
			dissect_rpc_authgluster_cred(tvb, ctree, offset+8);
			break;

		case RPCSEC_GSS:
			dissect_rpc_authgss_cred(tvb, ctree, offset+8, pinfo, rpc_conv_info);
			break;

		case AUTH_GLUSTERFS:
			dissect_rpc_authglusterfs_v2_cred(tvb, ctree, offset+8);
			break;

		case AUTH_GSSAPI:
			dissect_rpc_authgssapi_cred(tvb, ctree, offset+8);
			break;

		default:
			if (length)
				proto_tree_add_item(ctree, hf_rpc_opaque_data, tvb, offset+8, length, ENC_NA);
		break;
		}
	}
	offset += 8 + length;

	return offset;
}

int
dissect_rpc_opaque_auth(tvbuff_t* tvb, proto_tree* tree, int offset,
			packet_info *pinfo)
{
	conversation_t *conv = NULL;
	rpc_conv_info_t *conv_info = NULL;

	if (pinfo->ptype == PT_TCP)
		conv = find_conversation(pinfo->num, &pinfo->src,
				&pinfo->dst, pinfo->ptype, pinfo->srcport,
				pinfo->destport, 0);

	if (conv)
		conv_info = (rpc_conv_info_t *)conversation_get_proto_data(conv,
							proto_rpc);

	return dissect_rpc_cred(tvb, tree, offset, pinfo, conv_info);
}

/*
 * XDR opaque object, the contents of which are interpreted as a GSS-API
 * token.
 */
static int
dissect_rpc_authgss_token(tvbuff_t* tvb, proto_tree* tree, int offset,
			  packet_info *pinfo, int hfindex)
{
	guint32 opaque_length, rounded_length;
	gint len_consumed, length, reported_length;
	tvbuff_t *new_tvb;

	proto_item *gitem;
	proto_tree *gtree = NULL;

	opaque_length = tvb_get_ntohl(tvb, offset);
	rounded_length = rpc_roundup(opaque_length);

	gitem = proto_tree_add_item(tree, hfindex, tvb, offset, 4+rounded_length, ENC_NA);
	gtree = proto_item_add_subtree(gitem, ett_rpc_gss_token);
	proto_tree_add_uint(gtree, hf_rpc_authgss_token_length,
			    tvb, offset, 4, opaque_length);
	offset += 4;

	if (opaque_length != 0) {
		length = tvb_captured_length_remaining(tvb, offset);
		reported_length = tvb_reported_length_remaining(tvb, offset);
		DISSECTOR_ASSERT(length >= 0);
		DISSECTOR_ASSERT(reported_length >= 0);
		if (length > reported_length)
			length = reported_length;
		if ((guint32)length > opaque_length)
			length = opaque_length;
		if ((guint32)reported_length > opaque_length)
			reported_length = opaque_length;
		new_tvb = tvb_new_subset(tvb, offset, length, reported_length);
		len_consumed = call_dissector(gssapi_handle, new_tvb, pinfo, gtree);
		offset += len_consumed;
	}
	offset = rpc_roundup(offset);
	return offset;
}

/* AUTH_DES verifiers are asymmetrical, so we need to know what type of
 * verifier we're decoding (CALL or REPLY).
 */
static int
dissect_rpc_verf(tvbuff_t* tvb, proto_tree* tree, int offset, int msg_type,
		 packet_info *pinfo)
{
	guint flavor;
	guint length;

	proto_tree *vtree;

	flavor = tvb_get_ntohl(tvb,offset);
	length = tvb_get_ntohl(tvb,offset+4);
	length = rpc_roundup(length);

	if (tree) {
		vtree = proto_tree_add_subtree(tree, tvb, offset,
					    8+length, ett_rpc_verf, NULL, "Verifier");
		proto_tree_add_uint(vtree, hf_rpc_auth_flavor, tvb,
				    offset, 4, flavor);

		switch (flavor) {
		case AUTH_UNIX:
			proto_tree_add_uint(vtree, hf_rpc_auth_length, tvb,
					    offset+4, 4, length);
			dissect_rpc_authunix_cred(tvb, vtree, offset+8);
			break;
		case AUTH_DES:
			proto_tree_add_uint(vtree, hf_rpc_auth_length, tvb,
				offset+4, 4, length);

			if (msg_type == RPC_CALL)
			{
				guint window;

				dissect_rpc_authdes_desblock(tvb, vtree,
					hf_rpc_authdes_timestamp, offset+8);
				window = tvb_get_ntohl(tvb, offset+16);
				proto_tree_add_uint(vtree, hf_rpc_authdes_windowverf, tvb,
					offset+16, 4, window);
			}
			else
			{
				/* must be an RPC_REPLY */
				guint nickname;

				dissect_rpc_authdes_desblock(tvb, vtree,
					hf_rpc_authdes_timeverf, offset+8);
				nickname = tvb_get_ntohl(tvb, offset+16);
				proto_tree_add_uint(vtree, hf_rpc_authdes_nickname, tvb,
				 	offset+16, 4, nickname);
			}
			break;
		case RPCSEC_GSS:
			dissect_rpc_authgss_token(tvb, vtree, offset+4, pinfo, hf_rpc_authgss_token);
			break;
		default:
			proto_tree_add_uint(vtree, hf_rpc_auth_length, tvb,
					    offset+4, 4, length);
			if (length)
				proto_tree_add_item(vtree, hf_rpc_opaque_data, tvb, offset+8, length, ENC_NA);
			break;
		}
	}
	offset += 8 + length;

	return offset;
}

static int
dissect_rpc_authgss_initarg(tvbuff_t* tvb, proto_tree* tree, int offset,
			    packet_info *pinfo)
{
	return dissect_rpc_authgss_token(tvb, tree, offset, pinfo, hf_rpc_authgss_token);
}

static int
dissect_rpc_authgss_initres(tvbuff_t* tvb, proto_tree* tree, int offset,
			    packet_info *pinfo, rpc_conv_info_t *rpc_conv_info)
{
	int major, minor, window;

	offset = dissect_rpc_authgss_context(tree, tvb, offset, pinfo, rpc_conv_info, TRUE, FALSE);

	major = tvb_get_ntohl(tvb,offset);
	proto_tree_add_uint(tree, hf_rpc_authgss_major, tvb,
			    offset, 4, major);
	offset += 4;

	minor = tvb_get_ntohl(tvb,offset);
	proto_tree_add_uint(tree, hf_rpc_authgss_minor, tvb,
			    offset, 4, minor);
	offset += 4;

	window = tvb_get_ntohl(tvb,offset);
	proto_tree_add_uint(tree, hf_rpc_authgss_window, tvb,
			    offset, 4, window);
	offset += 4;

	offset = dissect_rpc_authgss_token(tvb, tree, offset, pinfo, hf_rpc_authgss_token);

	return offset;
}

static int
dissect_rpc_authgssapi_initarg(tvbuff_t* tvb, proto_tree* tree, int offset,
			       packet_info *pinfo)
{
	guint version;
	proto_tree *mtree;

	mtree = proto_tree_add_subtree(tree, tvb, offset, -1,
		ett_rpc_authgssapi_msg, NULL, "AUTH_GSSAPI Msg");

	version = tvb_get_ntohl(tvb, offset);

	proto_tree_add_uint(mtree, hf_rpc_authgssapi_msgv, tvb, offset, 4, version);
	offset += 4;

	offset = dissect_rpc_authgss_token(tvb, mtree, offset, pinfo, hf_rpc_authgss_token);

	return offset;
}

static int
dissect_rpc_authgssapi_initres(tvbuff_t* tvb, proto_tree* tree, int offset,
			       packet_info *pinfo)
{
	guint version;
	guint major, minor;
	proto_tree *mtree;

	mtree = proto_tree_add_subtree(tree, tvb, offset, -1,
		    ett_rpc_authgssapi_msg, NULL, "AUTH_GSSAPI Msg");

	version = tvb_get_ntohl(tvb,offset);
	proto_tree_add_uint(mtree, hf_rpc_authgssapi_msgv, tvb,
				    offset, 4, version);
	offset += 4;

	offset = dissect_rpc_data(tvb, mtree, hf_rpc_authgssapi_handle,
			offset);

	major = tvb_get_ntohl(tvb,offset);
	proto_tree_add_uint(mtree, hf_rpc_authgss_major, tvb,
				    offset, 4, major);
	offset += 4;

	minor = tvb_get_ntohl(tvb,offset);
	proto_tree_add_uint(mtree, hf_rpc_authgss_minor, tvb,
				    offset, 4, minor);
	offset += 4;

	offset = dissect_rpc_authgss_token(tvb, mtree, offset, pinfo, hf_rpc_authgss_token);

	offset = dissect_rpc_data(tvb, mtree, hf_rpc_authgssapi_isn, offset);

	return offset;
}

static int
dissect_auth_gssapi_data(tvbuff_t *tvb, proto_tree *tree, int offset)
{
	offset = dissect_rpc_data(tvb, tree, hf_rpc_authgss_data,
			offset);
	return offset;
}

static int
call_dissect_function(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
	int offset, dissector_handle_t dissect_function, const char *progname,
	rpc_call_info_value *rpc_call)
{
	const char *saved_proto;
	tvbuff_t *next_tvb;

	if (dissect_function != NULL) {
		/* set the current protocol name */
		saved_proto = pinfo->current_proto;
		if (progname != NULL)
			pinfo->current_proto = progname;

		/* call the dissector for the next level */
		next_tvb = tvb_new_subset_remaining(tvb, offset);
		offset += call_dissector_with_data(dissect_function, next_tvb, pinfo, tree, rpc_call);

		/* restore the protocol name */
		pinfo->current_proto = saved_proto;
	}

	return offset;
}


static int
dissect_rpc_authgss_integ_data(tvbuff_t *tvb, packet_info *pinfo,
	proto_tree *tree, int offset,
	dissector_handle_t dissect_function,
	const char *progname, rpc_call_info_value *rpc_call)
{
	guint32 length, rounded_length, seq;

	proto_tree *gtree;

	length = tvb_get_ntohl(tvb, offset);
	rounded_length = rpc_roundup(length);
	seq = tvb_get_ntohl(tvb, offset+4);

	gtree = proto_tree_add_subtree(tree, tvb, offset,
				    4+rounded_length, ett_rpc_gss_data, NULL, "GSS Data");
	proto_tree_add_uint(gtree, hf_rpc_authgss_data_length,
			    tvb, offset, 4, length);
	proto_tree_add_uint(gtree, hf_rpc_authgss_seq,
			    tvb, offset+4, 4, seq);
	offset += 8;

	if (dissect_function != NULL) {
		/* offset = */
		call_dissect_function(tvb, pinfo, gtree, offset,
				      dissect_function, progname, rpc_call);
	}
	offset += rounded_length - 4;
	offset = dissect_rpc_authgss_token(tvb, tree, offset, pinfo, hf_rpc_authgss_checksum);

	return offset;
}

static int
dissect_rpc_authgss_priv_data(tvbuff_t *tvb, proto_tree *tree, int offset,
			      packet_info *pinfo, gssapi_encrypt_info_t* gssapi_encrypt)
{
	int length;
	/* int return_offset; */

	length = tvb_get_ntohl(tvb, offset);
	proto_tree_add_uint(tree, hf_rpc_authgss_data_length,
				    tvb, offset, 4, length);
	offset += 4;

	proto_tree_add_item(tree, hf_rpc_authgss_data, tvb, offset, length,
				ENC_NA);


	/* can't decrypt if we don't have SPNEGO */
	if (!spnego_krb5_wrap_handle) {
		offset += length;
		return offset;
	}

	/* return_offset = */ call_dissector_with_data(spnego_krb5_wrap_handle,
		             tvb_new_subset_remaining(tvb, offset),
			     pinfo, tree, gssapi_encrypt);

	if (!gssapi_encrypt->gssapi_decrypted_tvb) {
		/* failed to decrypt the data */
		offset += length;
		return offset;
	}

	offset += length;
	return offset;
}

static address null_address = ADDRESS_INIT_NONE;

/*
 * Attempt to find a conversation for a call and, if we don't find one,
 * create a new one.
 */
static conversation_t *
get_conversation_for_call(packet_info *pinfo)
{
	conversation_t *conversation;

	/*
	 * If the transport is connection-oriented (TCP or Infiniband),
	 * we use the addresses and ports of both endpoints, because
	 * the addresses and ports of the two endpoints should be
	 * the same for a call and its matching reply.  (XXX - what
	 * if the connection is broken and re-established between
	 * the call and the reply?  Or will the call be re-made on
	 * the new connection?)
	 *
	 * If the transport is connectionless, we don't worry
	 * about the address to which the call was sent, because
	 * there's no guarantee that the reply will come from the
	 * address to which the call was sent.  We also don't
	 * worry about the port *from* which the call was sent,
	 * because some clients (*cough* OS X NFS client *cough*)
	 * might send retransmissions from a different port from
	 * the original request.
	 */
	if (pinfo->ptype == PT_TCP || pinfo->ptype == PT_IBQP) {
		conversation = find_conversation(pinfo->num,
		    &pinfo->src, &pinfo->dst, pinfo->ptype,
		    pinfo->srcport, pinfo->destport, 0);
	} else {
		/*
		 * XXX - you currently still have to pass a non-null
		 * pointer for the second address argument even
		 * if you use NO_ADDR_B.
		 */
		conversation = find_conversation(pinfo->num,
		    &pinfo->src, &null_address, pinfo->ptype,
		    pinfo->destport, 0, NO_ADDR_B|NO_PORT_B);
	}

	if (conversation == NULL) {
		if (pinfo->ptype == PT_TCP || pinfo->ptype == PT_IBQP) {
			conversation = conversation_new(pinfo->num,
			    &pinfo->src, &pinfo->dst, pinfo->ptype,
			    pinfo->srcport, pinfo->destport, 0);
		} else {
			conversation = conversation_new(pinfo->num,
			    &pinfo->src, &null_address, pinfo->ptype,
			    pinfo->destport, 0, NO_ADDR2|NO_PORT2);
		}
	}
	return conversation;
}

static conversation_t *
find_conversation_for_reply(packet_info *pinfo)
{
	conversation_t *conversation;

	/*
	 * If the transport is connection-oriented (TCP or Infiniband),
	 * we use the addresses and ports of both endpoints, because
	 * the addresses and ports of the two endpoints should be
	 * the same for a call and its matching reply.  (XXX - what
	 * if the connection is broken and re-established between
	 * the call and the reply?  Or will the call be re-made on
	 * the new connection?)
	 *
	 * If the transport is connectionless, we don't worry
	 * about the address from which the reply was sent,
	 * because there's no guarantee that the call was sent
	 * to the address from which the reply came.  We also
	 * don't worry about the port *to* which the reply was
	 * sent, because some clients (*cough* OS X NFS client
	 * *cough*) might send call retransmissions from a
	 * different port from the original request, so replies
	 * to the original call and a retransmission of the call
	 * might be sent to different ports.
	 */
	if (pinfo->ptype == PT_TCP || pinfo->ptype == PT_IBQP) {
		conversation = find_conversation(pinfo->num,
		    &pinfo->src, &pinfo->dst, pinfo->ptype,
		    pinfo->srcport, pinfo->destport, 0);
	} else {
		/*
		 * XXX - you currently still have to pass a non-null
		 * pointer for the second address argument even
		 * if you use NO_ADDR_B.
		 */
		conversation = find_conversation(pinfo->num,
		    &pinfo->dst, &null_address, pinfo->ptype,
		    pinfo->srcport, 0, NO_ADDR_B|NO_PORT_B);
	}
	return conversation;
}

static conversation_t *
new_conversation_for_reply(packet_info *pinfo)
{
	conversation_t *conversation;

	if (pinfo->ptype == PT_TCP || pinfo->ptype == PT_IBQP) {
		conversation = conversation_new(pinfo->num,
		    &pinfo->src, &pinfo->dst, pinfo->ptype,
		    pinfo->srcport, pinfo->destport, 0);
	} else {
		conversation = conversation_new(pinfo->num,
		    &pinfo->dst, &null_address, pinfo->ptype,
		    pinfo->srcport, 0, NO_ADDR2|NO_PORT2);
	}
	return conversation;
}

static conversation_t *
get_conversation_for_tcp(packet_info *pinfo)
{
	conversation_t *conversation;

	/*
	 * We know this is running over TCP, so the conversation should
	 * not wildcard either address or port, regardless of whether
	 * this is a call or reply.
	 */
	conversation = find_conversation(pinfo->num,
	    &pinfo->src, &pinfo->dst, pinfo->ptype,
	    pinfo->srcport, pinfo->destport, 0);
	if (conversation == NULL) {
		/*
		 * It's not part of any conversation - create a new one.
		 */
		conversation = conversation_new(pinfo->num,
		    &pinfo->src, &pinfo->dst, pinfo->ptype,
		    pinfo->srcport, pinfo->destport, 0);
	}
	return conversation;
}

/*
 * Dissect the arguments to an indirect call; used by the portmapper/RPCBIND
 * dissector for the CALLIT procedure.
 *
 * Record these in the same table as the direct calls
 * so we can find it when dissecting an indirect call reply.
 * (There should not be collissions between xid between direct and
 * indirect calls.)
 */
int
dissect_rpc_indir_call(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
		       int offset, int args_id, guint32 prog, guint32 vers, guint32 proc)
{
	conversation_t* conversation;
	rpc_proc_info_key key;
	rpc_call_info_value *rpc_call;
	dissector_handle_t dissect_function = NULL;
	rpc_conv_info_t *rpc_conv_info=NULL;
	guint32 xid;

	key.prog = prog;
	key.vers = vers;
	key.proc = proc;
	if ((dissect_function = dissector_get_custom_table_handle(subdissector_call_table, &key)) != NULL) {
		/*
		 * Establish a conversation for the call, for use when
		 * matching calls with replies.
		 */
		conversation = get_conversation_for_call(pinfo);

		/*
		 * Do we already have a state structure for this conv
		 */
		rpc_conv_info = (rpc_conv_info_t *)conversation_get_proto_data(conversation, proto_rpc);
		if (!rpc_conv_info) {
			/* No.  Attach that information to the conversation, and add
			 * it to the list of information structures.
			 */
			rpc_conv_info = wmem_new(wmem_file_scope(), rpc_conv_info_t);
			rpc_conv_info->xids=wmem_tree_new(wmem_file_scope());

			conversation_add_proto_data(conversation, proto_rpc, rpc_conv_info);
		}

		/* Make the dissector for this conversation the non-heuristic
		   RPC dissector. */
		conversation_set_dissector(conversation,
		    (pinfo->ptype == PT_TCP) ? rpc_tcp_handle : rpc_handle);

		/* Dissectors for RPC procedure calls and replies shouldn't
		   create new tvbuffs, and we don't create one ourselves,
		   so we should have been handed the tvbuff for this RPC call;
		   as such, the XID is at offset 0 in this tvbuff. */
		/* look up the request */
		xid = tvb_get_ntohl(tvb, offset);
		rpc_call = (rpc_call_info_value *)wmem_tree_lookup32(rpc_conv_info->xids, xid);
		if (rpc_call == NULL) {
			/* We didn't find it; create a new entry.
			   Prepare the value data.
			   Not all of it is needed for handling indirect
			   calls, so we set a bunch of items to 0. */
			rpc_call = wmem_new(wmem_file_scope(), rpc_call_info_value);
			rpc_call->req_num = 0;
			rpc_call->rep_num = 0;
			rpc_call->prog = prog;
			rpc_call->vers = vers;
			rpc_call->proc = proc;
			rpc_call->private_data = NULL;

			/*
			 * XXX - what about RPCSEC_GSS?
			 * Do we have to worry about it?
			 */
			rpc_call->flavor = FLAVOR_NOT_GSSAPI;
			rpc_call->gss_proc = 0;
			rpc_call->gss_svc = 0;
			/* store it */
			wmem_tree_insert32(rpc_conv_info->xids, xid, (void *)rpc_call);
		}
	}
	else {
		/* We don't know the procedure.
		   Happens only with strange program versions or
		   non-existing dissectors.
		   Just show the arguments as opaque data. */
		offset = dissect_rpc_data(tvb, tree, args_id,
		    offset);
		return offset;
	}

	if ( tree )
	{
		proto_tree_add_item(tree, hf_rpc_argument_length, tvb, offset, 4, ENC_BIG_ENDIAN);
	}
	offset += 4;

	/* Dissect the arguments */
	offset = call_dissect_function(tvb, pinfo, tree, offset,
			dissect_function, NULL, rpc_call);
	return offset;
}

/*
 * Dissect the results in an indirect reply; used by the portmapper/RPCBIND
 * dissector.
 */
int
dissect_rpc_indir_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
			int offset, int result_id, int prog_id, int vers_id, int proc_id)
{
	conversation_t* conversation;
	rpc_call_info_value *rpc_call;
	const char *procname=NULL;
	dissector_handle_t dissect_function = NULL;
	rpc_conv_info_t *rpc_conv_info=NULL;
	rpc_proc_info_key	key;
	guint32 xid;

	/*
	 * Look for the matching call in the xid table.
	 * A reply must match a call that we've seen, and the
	 * reply must be sent to the same address that the call came
	 * from, and must come from the port to which the call was sent.
	 */
	conversation = find_conversation_for_reply(pinfo);
	if (conversation == NULL) {
		/* We haven't seen an RPC call for that conversation,
		   so we can't check for a reply to that call.
		   Just show the reply stuff as opaque data. */
		offset = dissect_rpc_data(tvb, tree, result_id,
		    offset);
		return offset;
	}
	/*
	 * Do we already have a state structure for this conv
	 */
	rpc_conv_info = (rpc_conv_info_t *)conversation_get_proto_data(conversation, proto_rpc);
	if (!rpc_conv_info) {
		/* No.  Attach that information to the conversation, and add
		 * it to the list of information structures.
		 */
		rpc_conv_info = wmem_new(wmem_file_scope(), rpc_conv_info_t);
		rpc_conv_info->xids=wmem_tree_new(wmem_file_scope());
		conversation_add_proto_data(conversation, proto_rpc, rpc_conv_info);
	}

	/* The XIDs of the call and reply must match. */
	xid = tvb_get_ntohl(tvb, 0);
	rpc_call = (rpc_call_info_value *)wmem_tree_lookup32(rpc_conv_info->xids, xid);
	if (rpc_call == NULL) {
		/* The XID doesn't match a call from that
		   conversation, so it's probably not an RPC reply.
		   Just show the reply stuff as opaque data. */
		offset = dissect_rpc_data(tvb, tree, result_id,
		    offset);
		return offset;
	}

	key.prog = rpc_call->prog;
	key.vers = rpc_call->vers;
	key.proc = rpc_call->proc;

	dissect_function = dissector_get_custom_table_handle(subdissector_reply_table, &key);
	if (dissect_function != NULL) {
		procname = dissector_handle_get_dissector_name(dissect_function);
	}
	else {
		procname=wmem_strdup_printf(wmem_packet_scope(), "proc-%u", rpc_call->proc);
	}

	if ( tree )
	{
		proto_item *tmp_item;

		/* Put the program, version, and procedure into the tree. */
		tmp_item=proto_tree_add_uint_format(tree, prog_id, tvb,
			0, 0, rpc_call->prog, "Program: %s (%u)",
			rpc_prog_name(rpc_call->prog), rpc_call->prog);
		PROTO_ITEM_SET_GENERATED(tmp_item);

		tmp_item=proto_tree_add_uint(tree, vers_id, tvb, 0, 0, rpc_call->vers);
		PROTO_ITEM_SET_GENERATED(tmp_item);

		tmp_item=proto_tree_add_uint_format(tree, proc_id, tvb,
			0, 0, rpc_call->proc, "Procedure: %s (%u)",
			procname, rpc_call->proc);
		PROTO_ITEM_SET_GENERATED(tmp_item);
	}

	if (dissect_function == NULL) {
		/* We don't know how to dissect the reply procedure.
		   Just show the reply stuff as opaque data. */
		offset = dissect_rpc_data(tvb, tree, result_id,
		    offset);
		return offset;
	}

	/* Put the length of the reply value into the tree. */
	proto_tree_add_item(tree, hf_rpc_argument_length, tvb, offset, 4, ENC_BIG_ENDIAN);
	offset += 4;

	/* Dissect the return value */
	offset = call_dissect_function(tvb, pinfo, tree, offset,
			dissect_function, NULL, rpc_call);
	return offset;
}

/*
 * Just mark this as a continuation of an earlier packet.
 */
static void
dissect_rpc_continuation(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	proto_item *rpc_item;
	proto_tree *rpc_tree;

	col_set_str(pinfo->cinfo, COL_PROTOCOL, "RPC");
	col_set_str(pinfo->cinfo, COL_INFO, "Continuation");

	if (tree) {
		rpc_item = proto_tree_add_item(tree, proto_rpc, tvb, 0, -1, ENC_NA);
		rpc_tree = proto_item_add_subtree(rpc_item, ett_rpc);
		proto_tree_add_item(rpc_tree, hf_rpc_continuation_data, tvb, 0, -1, ENC_NA);
	}
}


int
dissect_rpc_void(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_)
{
	return 0;
}

int
dissect_rpc_unknown(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_)
{
	guint captured_length = tvb_captured_length(tvb);

	proto_tree_add_item(tree, hf_rpc_unknown_body, tvb, 0, captured_length, ENC_NA);
	return captured_length;
}

static rpc_prog_info_value *
looks_like_rpc_call(tvbuff_t *tvb, int offset)
{
	guint32 rpc_prog_key;
	rpc_prog_info_value *rpc_prog;

	if (!tvb_bytes_exist(tvb, offset, 16)) {
		/* Captured data in packet isn't enough to let us
		   tell. */
		return NULL;
	}

	/* XID can be anything, so don't check it.
	   We already have the message type.
	   Check whether an RPC version number of 2 is in the
	   location where it would be, and that an RPC program
	   number we know about is in the location where it would be.

	   Sun's snoop just checks for a message direction of RPC_CALL
	   and a version number of 2, which is a bit of a weak heuristic.

	   We could conceivably check for any of the program numbers
	   in the list at

		ftp://ftp.tau.ac.il/pub/users/eilon/rpc/rpc

	   and report it as RPC (but not dissect the payload if
	   we don't have a subdissector) if it matches. */
	rpc_prog_key = tvb_get_ntohl(tvb, offset + 12);

	/* we only dissect RPC version 2 */
	if (tvb_get_ntohl(tvb, offset + 8) != 2)
		return NULL;

	/* Do we know this program? */
	rpc_prog = (rpc_prog_info_value *)g_hash_table_lookup(rpc_progs, GUINT_TO_POINTER(rpc_prog_key));
	if (rpc_prog == NULL) {
		guint32 version;

		/*
		 * No.
		 * If the user has specified that he wants to try to
		 * dissect even completely unknown RPC program numbers,
		 * then let him do that.
		 */
		if (!rpc_dissect_unknown_programs) {
			/* They didn't, so just fail. */
			return NULL;
		}

		/*
		 * The user wants us to try to dissect even
		 * unknown program numbers.
		 *
		 * Use some heuristics to keep from matching any
		 * packet with a 2 in the appropriate location.
		 * We check that the program number is neither
		 * 0 nor -1, and that the version is <= 10, which
		 * is better than nothing.
		 */
		if (rpc_prog_key == 0 || rpc_prog_key == 0xffffffff)
			return FALSE;
		version = tvb_get_ntohl(tvb, offset+16);
		if (version > 10)
			return NULL;

		rpc_prog = wmem_new(wmem_packet_scope(), rpc_prog_info_value);
		rpc_prog->proto = NULL;
		rpc_prog->proto_id = 0;
		rpc_prog->ett = ett_rpc_unknown_program;
		rpc_prog->progname = wmem_strdup_printf(wmem_packet_scope(), "Unknown RPC program %u", rpc_prog_key);
	}

	return rpc_prog;
}

static rpc_call_info_value *
looks_like_rpc_reply(tvbuff_t *tvb, packet_info *pinfo, int offset)
{
	unsigned int xid;
	conversation_t *conversation;
	rpc_conv_info_t *rpc_conv_info;
	rpc_call_info_value *rpc_call;

	/* A reply must match a call that we've seen, and the reply
	   must be sent to the same address that the call came from,
	   and must come from the port to which the call was sent.

	   If the transport is connection-oriented (we check, for
	   now, only for "pinfo->ptype" of PT_TCP), we take
	   into account the port from which the call was sent
	   and the address to which the call was sent, because
	   the addresses and ports of the two endpoints should be
	   the same for all calls and replies.

	   If the transport is connectionless, we don't worry
	   about the address from which the reply was sent,
	   because there's no guarantee that the call was sent
	   to the address from which the reply came.  We also
	   don't worry about the port *to* which the reply was
	   sent, because some clients (*cough* OS X NFS client
	   *cough*) might send retransmissions from a
	   different port from the original request, so replies
	   to the original call and a retransmission of the call
	   might be sent to different ports.

	   Sun's snoop just checks for a message direction of RPC_REPLY
	   and a status of MSG_ACCEPTED or MSG_DENIED, which is a bit
	   of a weak heuristic. */
	xid = tvb_get_ntohl(tvb, offset);
	conversation = find_conversation_for_reply(pinfo);
	if (conversation != NULL) {
		/*
		 * We have a conversation; try to find an RPC
		 * state structure for the conversation.
		 */
		rpc_conv_info = (rpc_conv_info_t *)conversation_get_proto_data(conversation, proto_rpc);
		if (rpc_conv_info != NULL)
			rpc_call = (rpc_call_info_value *)wmem_tree_lookup32(rpc_conv_info->xids, xid);
		else
			rpc_call = NULL;
	} else {
		/*
		 * We don't have a conversation, so we obviously
		 * don't have an RPC state structure for it.
		 */
		rpc_conv_info = NULL;
		rpc_call = NULL;
	}

	if (rpc_call == NULL) {
		/*
		 * We don't have a conversation, or we have one but
		 * there's no RPC state information for it, or
		 * we have RPC state information but the XID doesn't
		 * match a call from the conversation, so it's
		 * probably not an RPC reply.
		 *
		 * Unless we're permitted to scan for embedded records
		 * and this is a connection-oriented transport,
		 * give up.
		 */
		if (((! rpc_find_fragment_start) || (pinfo->ptype != PT_TCP)) && (pinfo->ptype != PT_IBQP)) {
			return NULL;
		}

		/*
		 * Do we have a conversation to which to attach
		 * that information?
		 */
		if (conversation == NULL) {
			/*
			 * It's not part of any conversation - create
			 * a new one.
			 */
			conversation = new_conversation_for_reply(pinfo);
		}

		/*
		 * Do we have RPC information for that conversation?
		 */
		if (rpc_conv_info == NULL) {
			/*
			 * No.  Create a new RPC information structure,
			 * and attach it to the conversation.
			 */
			rpc_conv_info = wmem_new(wmem_file_scope(), rpc_conv_info_t);
			rpc_conv_info->xids=wmem_tree_new(wmem_file_scope());

			conversation_add_proto_data(conversation, proto_rpc, rpc_conv_info);
		}

		/*
		 * Define a dummy call for this reply.
		 */
		rpc_call = wmem_new0(wmem_file_scope(), rpc_call_info_value);
		rpc_call->rep_num = pinfo->num;
		rpc_call->xid = xid;
		rpc_call->flavor = FLAVOR_NOT_GSSAPI;  /* total punt */
		rpc_call->req_time = pinfo->abs_ts;

		/* store it */
		wmem_tree_insert32(rpc_conv_info->xids, xid, (void *)rpc_call);
	}

	/* pass rpc_info to subdissectors */
	rpc_call->request = FALSE;
	return rpc_call;
}

static gboolean
dissect_rpc_message(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
		    tvbuff_t *frag_tvb, fragment_head *ipfd_head, gboolean is_tcp,
		    guint32 rpc_rm, gboolean first_pdu)
{
	guint32	msg_type;
	rpc_call_info_value *rpc_call;
	rpc_prog_info_value *rpc_prog;
	guint32 rpc_prog_key;

	unsigned int xid;
	unsigned int rpcvers;
	unsigned int prog = 0;
	unsigned int vers = 0;
	unsigned int proc = 0;
	flavor_t flavor = FLAVOR_UNKNOWN;
	unsigned int gss_proc = 0;
	unsigned int gss_svc = 0;
	protocol_t *proto = NULL;
	int	proto_id = 0;
	int	ett = 0;
	int	procedure_hf;

	unsigned int reply_state;
	unsigned int accept_state;
	unsigned int reject_state;

	const char *msg_type_name;
	const char *progname;
	const char *procname;

	unsigned int vers_low;
	unsigned int vers_high;

	unsigned int auth_state;

	proto_item *rpc_item = NULL;
	proto_tree *rpc_tree = NULL;

	proto_item *pitem = NULL;
	proto_tree *ptree = NULL;
	int offset = (is_tcp && tvb == frag_tvb) ? 4 : 0;

	rpc_proc_info_key	key;
	conversation_t* conversation;
	nstime_t ns;

	dissector_handle_t dissect_function;
	gboolean dissect_rpc_flag = TRUE;

	rpc_conv_info_t *rpc_conv_info=NULL;
	gssapi_encrypt_info_t gssapi_encrypt;

	/*
	 * Check to see whether this looks like an RPC call or reply.
	 */
	if (!tvb_bytes_exist(tvb, offset, 8)) {
		/* Captured data in packet isn't enough to let us tell. */
		return FALSE;
	}

	/* both directions need at least this */
	msg_type = tvb_get_ntohl(tvb, offset + 4);

	switch (msg_type) {

	case RPC_CALL:
		/* Check for RPC call. */
		rpc_prog = looks_like_rpc_call(tvb, offset);
		if (rpc_prog == NULL)
			return FALSE;
		rpc_call = NULL;
		break;

	case RPC_REPLY:
		/* Check for RPC reply. */
		rpc_call = looks_like_rpc_reply(tvb, pinfo, offset);
		if (rpc_call == NULL)
			return FALSE;
		rpc_prog = NULL;
		break;

	default:
		/* The putative message type field contains neither
		   RPC_CALL nor RPC_REPLY, so it's not an RPC call or
		   reply. */
		return FALSE;
	}

	if (is_tcp) {
		/*
		 * This is RPC-over-TCP; check if this is the last
		 * fragment.
		 */
		if (!(rpc_rm & RPC_RM_LASTFRAG)) {
			/*
			 * This isn't the last fragment.
			 * If we're doing reassembly, just return
			 * TRUE to indicate that this looks like
			 * the beginning of an RPC message,
			 * and let them do fragment reassembly.
			 */
			if (rpc_defragment)
				return TRUE;
		}
	}

	col_set_str(pinfo->cinfo, COL_PROTOCOL, "RPC");

	rpc_item = proto_tree_add_item(tree, proto_rpc, tvb, 0, -1,
		    ENC_NA);
	rpc_tree = proto_item_add_subtree(rpc_item, ett_rpc);

	if (is_tcp) {
		show_rpc_fraginfo(tvb, frag_tvb, rpc_tree, rpc_rm,
		    ipfd_head, pinfo);
	}

	xid      = tvb_get_ntohl(tvb, offset);
	proto_tree_add_item(rpc_tree,hf_rpc_xid, tvb,
			offset, 4, ENC_BIG_ENDIAN);

	msg_type_name = val_to_str(msg_type,rpc_msg_type,"%u");
	if (rpc_tree) {
		proto_tree_add_uint(rpc_tree, hf_rpc_msgtype, tvb,
			offset+4, 4, msg_type);
		proto_item_append_text(rpc_item, ", Type:%s XID:0x%08x", msg_type_name, xid);
	}

	offset += 8;

	switch (msg_type) {

	case RPC_CALL:
		proto = rpc_prog->proto;
		proto_id = rpc_prog->proto_id;
		ett = rpc_prog->ett;
		progname = rpc_prog->progname;

		rpcvers = tvb_get_ntohl(tvb, offset);
		if (rpc_tree) {
			proto_tree_add_uint(rpc_tree,
				hf_rpc_version, tvb, offset, 4, rpcvers);
		}

		prog = tvb_get_ntohl(tvb, offset + 4);

		if (rpc_tree) {
			proto_tree_add_uint_format_value(rpc_tree,
				hf_rpc_program, tvb, offset+4, 4, prog,
				"%s (%u)", progname, prog);
		}

		/* Set the protocol name to the underlying
		   program name. */
		col_set_str(pinfo->cinfo, COL_PROTOCOL, progname);

		vers = tvb_get_ntohl(tvb, offset+8);
		if (rpc_tree) {
			proto_tree_add_uint(rpc_tree,
				hf_rpc_programversion, tvb, offset+8, 4, vers);
		}

		proc = tvb_get_ntohl(tvb, offset+12);

		key.prog = prog;
		key.vers = vers;
		key.proc = proc;

		if ((dissect_function = dissector_get_custom_table_handle(subdissector_call_table, &key)) != NULL) {
			procname = dissector_handle_get_dissector_name(dissect_function);
		}
		else {
			/* happens only with unknown program or version
			 * numbers
			 */
			dissect_function = data_handle;
			procname=wmem_strdup_printf(wmem_packet_scope(), "proc-%u", proc);
		}

		/* Check for RPCSEC_GSS and AUTH_GSSAPI */
		if (tvb_bytes_exist(tvb, offset+16, 4)) {
			switch (tvb_get_ntohl(tvb, offset+16)) {

			case RPCSEC_GSS:
				/*
				 * It's GSS-API authentication...
				 */
				if (tvb_bytes_exist(tvb, offset+28, 8)) {
					/*
					 * ...and we have the procedure
					 * and service information for it.
					 */
					flavor = FLAVOR_GSSAPI;
					gss_proc = tvb_get_ntohl(tvb, offset+28);
					gss_svc = tvb_get_ntohl(tvb, offset+36);
				} else {
					/*
					 * ...but the procedure and service
					 * information isn't available.
					 */
					flavor = FLAVOR_GSSAPI_NO_INFO;
				}
				break;

			case AUTH_GSSAPI:
				/*
				 * AUTH_GSSAPI flavor.  If auth_msg is TRUE,
				 * then this is an AUTH_GSSAPI message and
				 * not an application level message.
				 */
				if (tvb_bytes_exist(tvb, offset+28, 4)) {
					if (tvb_get_ntohl(tvb, offset+28)) {
						flavor = FLAVOR_AUTHGSSAPI_MSG;
						gss_proc = proc;
						procname = val_to_str(gss_proc,
						    rpc_authgssapi_proc, "Unknown (%d)");
					} else {
						flavor = FLAVOR_AUTHGSSAPI;
					}
				}
				break;

			default:
				/*
				 * It's not GSS-API authentication.
				 */
				flavor = FLAVOR_NOT_GSSAPI;
				break;
			}
		}

		if (rpc_tree) {
			proto_tree_add_uint_format_value(rpc_tree,
				hf_rpc_procedure, tvb, offset+12, 4, proc,
				"%s (%u)", procname, proc);
		}

		/* Print the program version, procedure name, and message type (call or reply). */
		if (first_pdu)
			col_clear(pinfo->cinfo, COL_INFO);
		else
			col_append_str(pinfo->cinfo, COL_INFO, "  ; ");
		/* Special case for NFSv4 - if the type is COMPOUND, do not print the procedure name */
		if (vers==4 && prog==NFS_PROGRAM && !strcmp(procname, "COMPOUND"))
			col_append_fstr(pinfo->cinfo, COL_INFO,"V%u %s", vers,
					msg_type_name);
		else
			col_append_fstr(pinfo->cinfo, COL_INFO,"V%u %s %s",
					vers, procname, msg_type_name);

		/*
		 * Establish a conversation for the call, for use when
		 * matching calls with replies.
		 */
		conversation = get_conversation_for_call(pinfo);

		/*
		 * Do we already have a state structure for this conv
		 */
		rpc_conv_info = (rpc_conv_info_t *)conversation_get_proto_data(conversation, proto_rpc);
		if (!rpc_conv_info) {
			/* No.  Attach that information to the conversation, and add
			 * it to the list of information structures.
			 */
			rpc_conv_info = wmem_new(wmem_file_scope(), rpc_conv_info_t);
			rpc_conv_info->xids=wmem_tree_new(wmem_file_scope());
			conversation_add_proto_data(conversation, proto_rpc, rpc_conv_info);
		}


		/* Make the dissector for this conversation the non-heuristic
		   RPC dissector. */
		conversation_set_dissector(conversation,
			(pinfo->ptype == PT_TCP) ? rpc_tcp_handle : rpc_handle);

		/* look up the request */
		rpc_call = (rpc_call_info_value *)wmem_tree_lookup32(rpc_conv_info->xids, xid);
		if (rpc_call) {
			/* We've seen a request with this XID, with the same
			   source and destination, before - but was it
			   *this* request? */
			if (pinfo->num != rpc_call->req_num) {
				/* No, so it's a duplicate request.
				   Mark it as such. */
				col_prepend_fstr(pinfo->cinfo, COL_INFO,
						 "[RPC retransmission of #%d]",
						 rpc_call->req_num);
				proto_tree_add_item(rpc_tree, hf_rpc_dup, tvb,
						    0, 0, ENC_NA);
				proto_tree_add_uint(rpc_tree, hf_rpc_call_dup,
						    tvb, 0,0, rpc_call->req_num);
			}
			if(rpc_call->rep_num){
				col_append_fstr(pinfo->cinfo, COL_INFO," (Reply In %d)", rpc_call->rep_num);
			}
		} else {
			/* Prepare the value data.
			   "req_num" and "rep_num" are frame numbers;
			   frame numbers are 1-origin, so we use 0
			   to mean "we don't yet know in which frame
			   the reply for this call appears". */
			rpc_call = wmem_new(wmem_file_scope(), rpc_call_info_value);
			rpc_call->req_num = pinfo->num;
			rpc_call->rep_num = 0;
			rpc_call->prog = prog;
			rpc_call->vers = vers;
			rpc_call->proc = proc;
			rpc_call->private_data = NULL;
			rpc_call->xid = xid;
			rpc_call->flavor = flavor;
			rpc_call->gss_proc = gss_proc;
			rpc_call->gss_svc = gss_svc;
			rpc_call->req_time = pinfo->abs_ts;

			/* store it */
			wmem_tree_insert32(rpc_conv_info->xids, xid, (void *)rpc_call);
		}

		if(rpc_call->rep_num){
			proto_item *tmp_item;

			tmp_item=proto_tree_add_uint_format(rpc_tree, hf_rpc_reqframe,
			    tvb, 0, 0, rpc_call->rep_num,
			    "The reply to this request is in frame %u",
			    rpc_call->rep_num);
			PROTO_ITEM_SET_GENERATED(tmp_item);
		}

		offset += 16;

		offset = dissect_rpc_cred(tvb, rpc_tree, offset, pinfo, rpc_conv_info);
		/* pass rpc_info to subdissectors */
		rpc_call->request=TRUE;

		if (gss_proc == RPCSEC_GSS_DESTROY) {
			/* there is no verifier for GSS destroy packets */
			break;
		}

		offset = dissect_rpc_verf(tvb, rpc_tree, offset, msg_type, pinfo);

		/* go to the next dissector */

		break;	/* end of RPC call */

	case RPC_REPLY:
		/* we know already the type from the calling routine,
		   and we already have "rpc_call" set above. */
		key.prog = prog = rpc_call->prog;
		key.vers = vers = rpc_call->vers;
		key.proc = proc = rpc_call->proc;
		flavor = rpc_call->flavor;
		gss_proc = rpc_call->gss_proc;
		gss_svc = rpc_call->gss_svc;

		dissect_function = dissector_get_custom_table_handle(subdissector_reply_table, &key);
		if (dissect_function != NULL) {
			procname = dissector_handle_get_dissector_name(dissect_function);
		}
		else {
			/* happens only with unknown program or version
			 * numbers
			 */
			dissect_function = data_handle;
			procname=wmem_strdup_printf(wmem_packet_scope(), "proc-%u", rpc_call->proc);
		}

		/*
		 * If this is an AUTH_GSSAPI message, then the RPC procedure
		 * is not an application procedure, but rather an auth level
		 * procedure, so it would be misleading to print the RPC
		 * procname.  Replace the RPC procname with the corresponding
		 * AUTH_GSSAPI procname.
		 */
		if (flavor == FLAVOR_AUTHGSSAPI_MSG) {
			procname = val_to_str_const(gss_proc, rpc_authgssapi_proc, "(null)");
		}

		rpc_prog_key = prog;
		if ((rpc_prog = (rpc_prog_info_value *)g_hash_table_lookup(rpc_progs,GUINT_TO_POINTER(rpc_prog_key))) == NULL) {
			proto = NULL;
			proto_id = 0;
			ett = 0;
			progname = "Unknown";
		}
		else {
			proto = rpc_prog->proto;
			proto_id = rpc_prog->proto_id;
			ett = rpc_prog->ett;
			progname = rpc_prog->progname;

			/* Set the protocol name to the underlying
			   program name. */
			col_set_str(pinfo->cinfo, COL_PROTOCOL, progname);
		}

		/* Print the program version, procedure name, and message type (call or reply). */
		if (first_pdu)
			col_clear(pinfo->cinfo, COL_INFO);
		else
			col_append_str(pinfo->cinfo, COL_INFO, "  ; ");
		/* Special case for NFSv4 - if the type is COMPOUND, do not print the procedure name */
		if (vers==4 && prog==NFS_PROGRAM && !strcmp(procname, "COMPOUND"))
			col_append_fstr(pinfo->cinfo, COL_INFO,"V%u %s",
					vers, msg_type_name);
		else
			col_append_fstr(pinfo->cinfo, COL_INFO,"V%u %s %s",
					vers, procname, msg_type_name);

		if (rpc_tree) {
			proto_item *tmp_item;
			tmp_item=proto_tree_add_uint_format_value(rpc_tree,
				hf_rpc_program, tvb, 0, 0, prog,
				"%s (%u)", progname, prog);
			PROTO_ITEM_SET_GENERATED(tmp_item);
			tmp_item=proto_tree_add_uint(rpc_tree,
				hf_rpc_programversion, tvb, 0, 0, vers);
			PROTO_ITEM_SET_GENERATED(tmp_item);
			tmp_item=proto_tree_add_uint_format_value(rpc_tree,
				hf_rpc_procedure, tvb, 0, 0, proc,
				"%s (%u)", procname, proc);
			PROTO_ITEM_SET_GENERATED(tmp_item);
		}

		reply_state = tvb_get_ntohl(tvb,offset);
		if (rpc_tree) {
			proto_tree_add_uint(rpc_tree, hf_rpc_state_reply, tvb,
				offset, 4, reply_state);
		}
		offset += 4;

		/* Indicate the frame to which this is a reply. */
		if (rpc_call->req_num) {
			proto_item *tmp_item;

			tmp_item=proto_tree_add_uint_format(rpc_tree, hf_rpc_repframe,
			    tvb, 0, 0, rpc_call->req_num,
			    "This is a reply to a request in frame %u",
			    rpc_call->req_num);
			PROTO_ITEM_SET_GENERATED(tmp_item);

			nstime_delta(&ns, &pinfo->abs_ts, &rpc_call->req_time);
			tmp_item=proto_tree_add_time(rpc_tree, hf_rpc_time, tvb, offset, 0,
				&ns);
			PROTO_ITEM_SET_GENERATED(tmp_item);

			col_append_fstr(pinfo->cinfo, COL_INFO," (Call In %d)", rpc_call->req_num);
		}


		if (rpc_call->rep_num == 0) {
			/* We have not yet seen a reply to that call, so
			   this must be the first reply; remember its
			   frame number. */
			rpc_call->rep_num = pinfo->num;
		} else {
			/* We have seen a reply to this call - but was it
			   *this* reply? */
			if (rpc_call->rep_num != pinfo->num) {
				proto_item *tmp_item;

				/* No, so it's a duplicate reply.
				   Mark it as such. */
				col_prepend_fstr(pinfo->cinfo, COL_INFO,
						"[RPC duplicate of #%d]", rpc_call->rep_num);
				tmp_item=proto_tree_add_item(rpc_tree,
					hf_rpc_dup, tvb, 0,0, ENC_NA);
				PROTO_ITEM_SET_GENERATED(tmp_item);

				tmp_item=proto_tree_add_uint(rpc_tree,
					hf_rpc_reply_dup, tvb, 0,0, rpc_call->rep_num);
				PROTO_ITEM_SET_GENERATED(tmp_item);
			}
		}

		switch (reply_state) {

		case MSG_ACCEPTED:
			offset = dissect_rpc_verf(tvb, rpc_tree, offset, msg_type, pinfo);
			accept_state = tvb_get_ntohl(tvb,offset);
			if (rpc_tree) {
				proto_tree_add_uint(rpc_tree, hf_rpc_state_accept, tvb,
					offset, 4, accept_state);
			}
			offset += 4;
			switch (accept_state) {

			case SUCCESS:
				/* go to the next dissector */
				break;

			case PROG_MISMATCH:
				vers_low = tvb_get_ntohl(tvb,offset);
				vers_high = tvb_get_ntohl(tvb,offset+4);
				if (rpc_tree) {
					proto_tree_add_uint(rpc_tree,
						hf_rpc_programversion_min,
						tvb, offset, 4, vers_low);
					proto_tree_add_uint(rpc_tree,
						hf_rpc_programversion_max,
						tvb, offset+4, 4, vers_high);
				}
				offset += 8;

				/*
				 * There's no protocol reply, so don't
				 * try to dissect it.
				 */
				dissect_rpc_flag = FALSE;
				break;

			default:
				/*
				 * There's no protocol reply, so don't
				 * try to dissect it.
				 */
				dissect_rpc_flag = FALSE;
				break;
			}
			break;

		case MSG_DENIED:
			reject_state = tvb_get_ntohl(tvb,offset);
			if (rpc_tree) {
				proto_tree_add_uint(rpc_tree,
					hf_rpc_state_reject, tvb, offset, 4,
					reject_state);
			}
			offset += 4;

			if (reject_state==RPC_MISMATCH) {
				vers_low = tvb_get_ntohl(tvb,offset);
				vers_high = tvb_get_ntohl(tvb,offset+4);
				if (rpc_tree) {
					proto_tree_add_uint(rpc_tree,
						hf_rpc_version_min,
						tvb, offset, 4, vers_low);
					proto_tree_add_uint(rpc_tree,
						hf_rpc_version_max,
						tvb, offset+4, 4, vers_high);
				}
				offset += 8;
			} else if (reject_state==AUTH_ERROR) {
				auth_state = tvb_get_ntohl(tvb,offset);
				if (rpc_tree) {
					proto_tree_add_uint(rpc_tree,
						hf_rpc_state_auth, tvb, offset, 4,
						auth_state);
				}
				offset += 4;
			}

			/*
			 * There's no protocol reply, so don't
			 * try to dissect it.
			 */
			dissect_rpc_flag = FALSE;
			break;

		default:
			/*
			 * This isn't a valid reply state, so we have
			 * no clue what's going on; don't try to dissect
			 * the protocol reply.
			 */
			dissect_rpc_flag = FALSE;
			break;
		}
		break; /* end of RPC reply */

	default:
		/*
		 * The switch statement at the top returned if
		 * this was neither an RPC call nor a reply.
		 */
		DISSECTOR_ASSERT_NOT_REACHED();
	}

	/* now we know, that RPC was shorter */
	if (rpc_item) {
		if (offset < 0)
			THROW(ReportedBoundsError);
		tvb_ensure_bytes_exist(tvb, offset, 0);
		proto_item_set_end(rpc_item, tvb, offset);
	}

	if (!dissect_rpc_flag) {
		/*
		 * There's no RPC call or reply here; just dissect
		 * whatever's left as data.
		 */
		call_dissector(data_handle,
		    tvb_new_subset_remaining(tvb, offset), pinfo, rpc_tree);
		return TRUE;
	}

	/* we must queue this packet to the tap system before we actually
	   call the subdissectors since short packets (i.e. nfs read reply)
	   will cause an exception and execution would never reach the call
	   to tap_queue_packet() in that case
	*/
	tap_queue_packet(rpc_tap, pinfo, rpc_call);


	/* If this is encrypted data we have to try to decrypt the data first before we
	 * we create a tree.
	 * the reason for this is because if we can decrypt the data we must create the
	 * item/tree for the next protocol using the decrypted tvb and not the current
	 * tvb.
	 */
	memset(&gssapi_encrypt, 0, sizeof(gssapi_encrypt));
	gssapi_encrypt.decrypt_gssapi_tvb=DECRYPT_GSSAPI_NORMAL;

	if (flavor == FLAVOR_GSSAPI && gss_proc == RPCSEC_GSS_DATA && gss_svc == RPCSEC_GSS_SVC_PRIVACY) {
		proto_tree *gss_tree;

		gss_tree = proto_tree_add_subtree(tree, tvb, offset, -1, ett_gss_wrap, NULL, "GSS-Wrap");

		offset = dissect_rpc_authgss_priv_data(tvb, gss_tree, offset, pinfo, &gssapi_encrypt);
		if (gssapi_encrypt.gssapi_decrypted_tvb) {
			proto_tree_add_item(gss_tree, hf_rpc_authgss_seq, gssapi_encrypt.gssapi_decrypted_tvb, 0, 4, ENC_BIG_ENDIAN);

			/* Switcheroo to the new tvb that contains the decrypted payload */
			tvb = gssapi_encrypt.gssapi_decrypted_tvb;
			offset = 4;
		}
	}


	/* create here the program specific sub-tree */
	if (tree && (flavor != FLAVOR_AUTHGSSAPI_MSG)) {
		proto_item *tmp_item;

		pitem = proto_tree_add_item(tree, proto_id, tvb, offset, tvb_reported_length_remaining(tvb, offset), ENC_NA);
		ptree = proto_item_add_subtree(pitem, ett);

		tmp_item=proto_tree_add_uint(ptree,
				hf_rpc_programversion, tvb, 0, 0, vers);
		PROTO_ITEM_SET_GENERATED(tmp_item);
		if (rpc_prog && (rpc_prog->procedure_hfs->len > vers) )
			procedure_hf = g_array_index(rpc_prog->procedure_hfs, int, vers);
		else {
			/*
			 * No such element in the GArray.
			 */
			procedure_hf = 0;
		}
		if (procedure_hf != 0 && procedure_hf != -1) {
			tmp_item=proto_tree_add_uint(ptree,
				procedure_hf, tvb, 0, 0, proc);
			PROTO_ITEM_SET_GENERATED(tmp_item);
		} else {
			tmp_item=proto_tree_add_uint_format_value(ptree,
				hf_rpc_procedure, tvb, 0, 0, proc,
				"%s (%u)", procname, proc);
			PROTO_ITEM_SET_GENERATED(tmp_item);
		}
	}

	/* proto==0 if this is an unknown program */
	if( (proto==0) || !proto_is_protocol_enabled(proto)){
		dissect_function = data_handle;
	}

	/*
	 * Don't call any subdissector if we have no more date to dissect.
	 */
	if (tvb_reported_length_remaining(tvb, offset) == 0) {
		return TRUE;
	}

	/*
	 * Handle RPCSEC_GSS and AUTH_GSSAPI specially.
	 */
	switch (flavor) {

	case FLAVOR_UNKNOWN:
		/*
		 * We don't know the authentication flavor, so we can't
		 * dissect the payload.
		 */
		proto_tree_add_expert_format(ptree, pinfo, &ei_rpc_cannot_dissect, tvb, offset, -1,
		    "Unknown authentication flavor - cannot dissect");
		return TRUE;

	case FLAVOR_NOT_GSSAPI:
		/*
		 * It's not GSS-API authentication.  Just dissect the
		 * payload.
		 */
		offset = call_dissect_function(tvb, pinfo, ptree, offset,
				dissect_function, progname, rpc_call);
		break;

	case FLAVOR_GSSAPI_NO_INFO:
		/*
		 * It's GSS-API authentication, but we don't have the
		 * procedure and service information, so we can't dissect
		 * the payload.
		 */
		proto_tree_add_expert_format(ptree, pinfo, &ei_rpc_cannot_dissect, tvb, offset, -1,
		    "GSS-API authentication, but procedure and service unknown - cannot dissect");
		return TRUE;

	case FLAVOR_GSSAPI:
		/*
		 * It's GSS-API authentication, and we have the procedure
		 * and service information; process the GSS-API stuff,
		 * and process the payload if there is any.
		 */
		switch (gss_proc) {

		case RPCSEC_GSS_INIT:
		case RPCSEC_GSS_CONTINUE_INIT:
			if (msg_type == RPC_CALL) {
				offset = dissect_rpc_authgss_initarg(tvb,
					ptree, offset, pinfo);
			}
			else {
				offset = dissect_rpc_authgss_initres(tvb,
					ptree, offset, pinfo, rpc_conv_info);
			}
			break;

		case RPCSEC_GSS_DATA:
			if (gss_svc == RPCSEC_GSS_SVC_NONE) {
				offset = call_dissect_function(tvb,
						pinfo, ptree, offset,
						dissect_function,
						progname, rpc_call);
			}
			else if (gss_svc == RPCSEC_GSS_SVC_INTEGRITY) {
				offset = dissect_rpc_authgss_integ_data(tvb,
						pinfo, ptree, offset,
						dissect_function,
						progname, rpc_call);
			}
			else if (gss_svc == RPCSEC_GSS_SVC_PRIVACY) {
				if (gssapi_encrypt.gssapi_decrypted_tvb) {
					call_dissect_function(
						gssapi_encrypt.gssapi_decrypted_tvb,
						pinfo, ptree, 4,
						dissect_function,
						progname, rpc_call);
					offset = tvb_reported_length(gssapi_encrypt.gssapi_decrypted_tvb);
				}
			}
			break;

		default:
			break;
		}
		break;

	case FLAVOR_AUTHGSSAPI_MSG:
		/*
		 * This is an AUTH_GSSAPI message.  It contains data
		 * only for the authentication procedure and not for the
		 * application level RPC procedure.  Reset the column
		 * protocol and info fields to indicate that this is
		 * an RPC auth level message, then process the args.
		 */
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "RPC");
		col_add_fstr(pinfo->cinfo, COL_INFO,
				"%s %s XID 0x%x",
				val_to_str(gss_proc, rpc_authgssapi_proc, "Unknown (%d)"),
				msg_type_name, xid);

		switch (gss_proc) {

		case AUTH_GSSAPI_INIT:
		case AUTH_GSSAPI_CONTINUE_INIT:
		case AUTH_GSSAPI_MSG:
			if (msg_type == RPC_CALL) {
			    offset = dissect_rpc_authgssapi_initarg(tvb,
				rpc_tree, offset, pinfo);
			} else {
			    offset = dissect_rpc_authgssapi_initres(tvb,
				rpc_tree, offset, pinfo);
			}
			break;

		case AUTH_GSSAPI_DESTROY:
			offset = dissect_rpc_data(tvb, rpc_tree,
			    hf_rpc_authgss_data, offset);
			break;

		case AUTH_GSSAPI_EXIT:
			break;
		}

		/* Adjust the length to account for the auth message. */
		if (rpc_item) {
			proto_item_set_end(rpc_item, tvb, offset);
		}
		break;

	case FLAVOR_AUTHGSSAPI:
		/*
		 * An RPC with AUTH_GSSAPI authentication.  The data
		 * portion is always private, so don't call the dissector.
		 */
		offset = dissect_auth_gssapi_data(tvb, ptree, offset);
		break;
	}

	if (tvb_reported_length_remaining(tvb, offset) > 0) {
		/*
		 * dissect any remaining bytes (incomplete dissection) as pure
		 * data in the ptree
		 */

		call_dissector(data_handle,
			       tvb_new_subset_remaining(tvb, offset), pinfo, ptree);
	}

	/* XXX this should really loop over all fhandles registred for the frame */
	if(nfs_fhandle_reqrep_matching){
		switch (msg_type) {
		case RPC_CALL:
			if(rpc_call && rpc_call->rep_num){
				dissect_fhandle_hidden(pinfo,
						ptree, rpc_call->rep_num);
			}
			break;
		case RPC_REPLY:
			if(rpc_call && rpc_call->req_num){
				dissect_fhandle_hidden(pinfo,
						ptree, rpc_call->req_num);
			}
			break;
		}
	}

	return TRUE;
}

static gboolean
dissect_rpc_heur(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
	return dissect_rpc_message(tvb, pinfo, tree, NULL, NULL, FALSE, 0,
	    TRUE);
}

static int
dissect_rpc(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
	if (!dissect_rpc_message(tvb, pinfo, tree, NULL, NULL, FALSE, 0,
	    TRUE)) {
		if (tvb_reported_length(tvb) != 0)
			dissect_rpc_continuation(tvb, pinfo, tree);
	}
	return tvb_captured_length(tvb);
}


/* Defragmentation of RPC-over-TCP records */
/* table to hold defragmented RPC records */
static reassembly_table rpc_fragment_table;

/*
 * XXX - can we eliminate this by defining our own key-handling functions
 * for rpc_fragment_table?  (Note that those functions must look at
 * not only the addresses of the endpoints, but the ports of the endpoints,
 * so that they don't try to combine fragments from different TCP
 * connections.)
 */
static GHashTable *rpc_reassembly_table = NULL;

typedef struct _rpc_fragment_key {
	guint32 conv_id;
	guint32 seq;
	guint32 offset;
	guint32 port;
	/* xxx */
	guint32 start_seq;
} rpc_fragment_key;

static guint
rpc_fragment_hash(gconstpointer k)
{
	const rpc_fragment_key *key = (const rpc_fragment_key *)k;

	return key->conv_id + key->seq;
}

static gint
rpc_fragment_equal(gconstpointer k1, gconstpointer k2)
{
	const rpc_fragment_key *key1 = (const rpc_fragment_key *)k1;
	const rpc_fragment_key *key2 = (const rpc_fragment_key *)k2;

	return key1->conv_id == key2->conv_id &&
	    key1->seq == key2->seq && key1->port == key2->port;
}

static void
show_rpc_fragheader(tvbuff_t *tvb, proto_tree *tree, guint32 rpc_rm)
{
	proto_tree *hdr_tree;
	guint32 fraglen;

	if (tree) {
		fraglen = rpc_rm & RPC_RM_FRAGLEN;

		hdr_tree = proto_tree_add_subtree_format(tree, tvb, 0, 4,
		    ett_rpc_fraghdr, NULL, "Fragment header: %s%u %s",
		    (rpc_rm & RPC_RM_LASTFRAG) ? "Last fragment, " : "",
		    fraglen, plurality(fraglen, "byte", "bytes"));

		proto_tree_add_boolean(hdr_tree, hf_rpc_lastfrag, tvb, 0, 4,
		    rpc_rm);
		proto_tree_add_uint(hdr_tree, hf_rpc_fraglen, tvb, 0, 4,
		    rpc_rm);
	}
}

static void
show_rpc_fragment(tvbuff_t *tvb, proto_tree *tree, guint32 rpc_rm)
{
	if (tree) {
		/*
		 * Show the fragment header and the data for the fragment.
		 */
		show_rpc_fragheader(tvb, tree, rpc_rm);
		proto_tree_add_item(tree, hf_rpc_fragment_data, tvb, 4, -1, ENC_NA);
	}
}

static void
make_frag_tree(tvbuff_t *tvb, proto_tree *tree, int proto, gint ett,
	       guint32 rpc_rm)
{
	proto_item *frag_item;
	proto_tree *frag_tree;

	if (tree == NULL)
		return;		/* nothing to do */

	frag_item = proto_tree_add_protocol_format(tree, proto, tvb, 0, -1,
	    "%s Fragment", proto_get_protocol_name(proto));
	frag_tree = proto_item_add_subtree(frag_item, ett);
	show_rpc_fragment(tvb, frag_tree, rpc_rm);
}

void
show_rpc_fraginfo(tvbuff_t *tvb, tvbuff_t *frag_tvb, proto_tree *tree,
		  guint32 rpc_rm, fragment_head *ipfd_head, packet_info *pinfo)
{
	proto_item *frag_tree_item;

	if (tree == NULL)
		return;		/* don't do any work */

	if (tvb != frag_tvb) {
		/*
		 * This message was not all in one fragment,
		 * so show the fragment header *and* the data
		 * for the fragment (which is the last fragment),
		 * and a tree with information about all fragments.
		 */
		show_rpc_fragment(frag_tvb, tree, rpc_rm);

		/*
		 * Show a tree with information about all fragments.
		 */
		show_fragment_tree(ipfd_head, &rpc_frag_items, tree, pinfo, tvb, &frag_tree_item);
	} else {
		/*
		 * This message was all in one fragment, so just show
		 * the fragment header.
		 */
		show_rpc_fragheader(tvb, tree, rpc_rm);
	}
}

static gboolean
call_message_dissector(tvbuff_t *tvb, tvbuff_t *rec_tvb, packet_info *pinfo,
		       proto_tree *tree, tvbuff_t *frag_tvb, rec_dissector_t dissector,
		       fragment_head *ipfd_head, guint32 rpc_rm, gboolean first_pdu)
{
	const char *saved_proto;
	volatile gboolean rpc_succeeded;

	saved_proto = pinfo->current_proto;
	rpc_succeeded = FALSE;
	TRY {
		rpc_succeeded = (*dissector)(rec_tvb, pinfo, tree,
		    frag_tvb, ipfd_head, TRUE, rpc_rm, first_pdu);
	}
	CATCH_NONFATAL_ERRORS {
		/*
		 * Somebody threw an exception that means that there
		 * was a problem dissecting the payload; that means
		 * that a dissector was found, so we don't need to
		 * dissect the payload as data or update the protocol
		 * or info columns.
		 *
		 * Just show the exception and then continue dissecting.
		 */
		show_exception(tvb, pinfo, tree, EXCEPT_CODE, GET_MESSAGE);
		pinfo->current_proto = saved_proto;

		/*
		 * We treat this as a "successful" dissection of
		 * an RPC packet, as "dissect_rpc_message()"
		 * *did* decide it was an RPC packet, throwing
		 * an exception while dissecting it as such.
		 */
		rpc_succeeded = TRUE;
	}
	ENDTRY;
	return rpc_succeeded;
}

static int
dissect_rpc_fragment(tvbuff_t *tvb, int offset, packet_info *pinfo,
		     proto_tree *tree, rec_dissector_t dissector, gboolean is_heur,
		     int proto, int ett, gboolean first_pdu, struct tcpinfo *tcpinfo)
{
	guint32 seq;
	guint32 rpc_rm;
	guint32 len;
	gint32 seglen;
	gint tvb_len, tvb_reported_len;
	tvbuff_t *frag_tvb;
	conversation_t *conversation = NULL;
	gboolean rpc_succeeded;
	gboolean save_fragmented;
	rpc_fragment_key old_rfk, *rfk, *new_rfk;
	fragment_head *ipfd_head;
	tvbuff_t *rec_tvb;

	if (pinfo == NULL || tcpinfo == NULL) {
		return 0;
	}

	seq = tcpinfo->seq + offset;

	/*
	 * Get the record mark.
	 */
	if (!tvb_bytes_exist(tvb, offset, 4)) {
		/*
		 * XXX - we should somehow arrange to handle
		 * a record mark split across TCP segments.
		 */
		return 0;	/* not enough to tell if it's valid */
	}
	rpc_rm = tvb_get_ntohl(tvb, offset);

	len = rpc_rm & RPC_RM_FRAGLEN;

	/*
	 * Do TCP desegmentation, if enabled.
	 *
	 * reject fragments bigger than this preference setting.
	 * This is arbitrary, but should at least prevent
	 * some crashes from either packets with really
	 * large RPC-over-TCP fragments or from stuff that's
	 * not really valid for this protocol.
	 */
	if (len > max_rpc_tcp_pdu_size)
		return 0;	/* pretend it's not valid */

	if (rpc_desegment) {
		seglen = tvb_reported_length_remaining(tvb, offset + 4);

		if ((gint)len > seglen && pinfo->can_desegment) {
			/*
			 * This frame doesn't have all of the
			 * data for this message, but we can do
			 * reassembly on it.
			 *
			 * If this is a heuristic dissector, check
			 * whether it looks like the beginning of
			 * an RPC call or reply.  If not, then, just
			 * return 0 - we don't want to try to get
			 * more data, as that's too likely to cause
			 * us to misidentify this as valid.  Otherwise,
			 * mark this conversation as being for RPC and
			 * try to get more data.
			 *
			 * If this isn't a heuristic dissector,
			 * we've already identified this conversation
			 * as containing data for this protocol, as we
			 * saw valid data in previous frames.  Try to
			 * get more data.
			 */
			if (is_heur) {
				guint32 msg_type;

				if (!tvb_bytes_exist(tvb, offset + 4, 8)) {
					/*
					 * Captured data in packet isn't
					 * enough to let us tell.
					 */
					return 0;
				}

				/* both directions need at least this */
				msg_type = tvb_get_ntohl(tvb, offset + 4 + 4);

				switch (msg_type) {

				case RPC_CALL:
					/* Check for RPC call. */
					if (looks_like_rpc_call(tvb,
					    offset + 4) == NULL) {
						/* Doesn't look like a call. */
						return 0;
					}
					break;

				case RPC_REPLY:
					/* Check for RPC reply. */
					if (looks_like_rpc_reply(tvb, pinfo,
					    offset + 4) == NULL) {
						/* Doesn't look like a reply. */
						return 0;
					}
					break;

				default:
					/* The putative message type field
					   contains neither RPC_CALL nor
					   RPC_REPLY, so it's not an RPC
					   call or reply. */
					return 0;
				}

				/* Get this conversation, creating it if
				   it doesn't already exist, and make the
				   dissector for it the non-heuristic RPC
				   dissector for RPC-over-TCP. */
				conversation = get_conversation_for_tcp(pinfo);
				conversation_set_dissector(conversation,
				    rpc_tcp_handle);
			}

			/* Try to get more data. */
			pinfo->desegment_offset = offset;
			pinfo->desegment_len = len - seglen;
			return -((gint32) pinfo->desegment_len);
		}
	}
	len += 4;	/* include record mark */
	tvb_len = tvb_captured_length_remaining(tvb, offset);
	tvb_reported_len = tvb_reported_length_remaining(tvb, offset);
	if (tvb_len > (gint)len)
		tvb_len = len;
	if (tvb_reported_len > (gint)len)
		tvb_reported_len = len;
	frag_tvb = tvb_new_subset(tvb, offset, tvb_len,
	    tvb_reported_len);

	/*
	 * If we're not defragmenting, just hand this to the
	 * disssector.
	 *
	 * We defragment only if we should (rpc_defragment true) *and*
	 * we can (tvb_len == tvb_reported_len, so that we have all the
	 * data in the fragment).
	 */
	if (!rpc_defragment || tvb_len != tvb_reported_len) {
		/*
		 * This is the first fragment we've seen, and it's also
		 * the last fragment; that means the record wasn't
		 * fragmented.  Hand the dissector the tvbuff for the
		 * fragment as the tvbuff for the record.
		 */
		rec_tvb = frag_tvb;
		ipfd_head = NULL;

		/*
		 * Mark this as fragmented, so if somebody throws an
		 * exception, we don't report it as a malformed frame.
		 */
		save_fragmented = pinfo->fragmented;
		pinfo->fragmented = TRUE;
		rpc_succeeded = call_message_dissector(tvb, rec_tvb, pinfo,
		    tree, frag_tvb, dissector, ipfd_head, rpc_rm, first_pdu);
		pinfo->fragmented = save_fragmented;
		if (!rpc_succeeded)
			return 0;	/* not RPC */
		return len;
	}

	/*
	 * First, we check to see if this fragment is part of a record
	 * that we're in the process of defragmenting.
	 *
	 * The key is the conversation ID for the conversation to which
	 * the packet belongs and the current sequence number.
	 *
	 * We must first find the conversation and, if we don't find
	 * one, create it.
	 */
	if (conversation == NULL)
		conversation = get_conversation_for_tcp(pinfo);
	old_rfk.conv_id = conversation->conv_index;
	old_rfk.seq = seq;
	old_rfk.port = pinfo->srcport;
	rfk = (rpc_fragment_key *)g_hash_table_lookup(rpc_reassembly_table, &old_rfk);

	if (rfk == NULL) {
		/*
		 * This fragment was not found in our table, so it doesn't
		 * contain a continuation of a higher-level PDU.
		 * Is it the last fragment?
		 */
		if (!(rpc_rm & RPC_RM_LASTFRAG)) {
			/*
			 * This isn't the last fragment, so we don't
			 * have the complete record.
			 *
			 * It's the first fragment we've seen, so if
			 * it's truly the first fragment of the record,
			 * and it has enough data, the dissector can at
			 * least check whether it looks like a valid
			 * message, as it contains the start of the
			 * message.
			 *
			 * The dissector should not dissect anything
			 * if the "last fragment" flag isn't set in
			 * the record marker, so it shouldn't throw
			 * an exception.
			 */
			if (!(*dissector)(frag_tvb, pinfo, tree, frag_tvb,
			    NULL, TRUE, rpc_rm, first_pdu))
				return 0;	/* not valid */

			/*
			 * OK, now start defragmentation with that
			 * fragment.  Add this fragment, and set up
			 * next packet/sequence number as well.
			 *
			 * We must remember this fragment.
			 */

			rfk = wmem_new(wmem_file_scope(), rpc_fragment_key);
			rfk->conv_id = conversation->conv_index;
			rfk->seq = seq;
			rfk->port = pinfo->srcport;
			rfk->offset = 0;
			rfk->start_seq = seq;
			g_hash_table_insert(rpc_reassembly_table, rfk, rfk);

			/*
			 * Start defragmentation.
			 */
			ipfd_head = fragment_add_multiple_ok(&rpc_fragment_table,
			    tvb, offset + 4,
			    pinfo, rfk->start_seq, NULL,
			    rfk->offset, len - 4, TRUE);

			/*
			 * Make sure that defragmentation isn't complete;
			 * it shouldn't be, as this is the first fragment
			 * we've seen, and the "last fragment" bit wasn't
			 * set on it.
			 */
			if (ipfd_head == NULL) {
				new_rfk = wmem_new(wmem_file_scope(), rpc_fragment_key);
				new_rfk->conv_id = rfk->conv_id;
				new_rfk->seq = seq + len;
				new_rfk->port = pinfo->srcport;
				new_rfk->offset = rfk->offset + len - 4;
				new_rfk->start_seq = rfk->start_seq;
				g_hash_table_insert(rpc_reassembly_table, new_rfk,
					new_rfk);

				/*
				 * This is part of a fragmented record,
				 * but it's not the first part.
				 * Show it as a record marker plus data, under
				 * a top-level tree for this protocol.
				 */
				col_set_str(pinfo->cinfo, COL_PROTOCOL, "RPC");
				col_set_str(pinfo->cinfo, COL_INFO, "Fragment");
				make_frag_tree(frag_tvb, tree, proto, ett,rpc_rm);

				/*
				 * No more processing need be done, as we don't
				 * have a complete record.
				 */
				return len;
			} else {
				/* oddly, we have a first fragment, not marked as last,
				 * but which the defragmenter thinks is complete.
				 * So rather than creating a fragment reassembly tree,
				 * we simply throw away the partial fragment structure
				 * and fall though to our "sole fragment" processing below.
				 */
			}
		}

		/*
		 * This is the first fragment we've seen, and it's also
		 * the last fragment; that means the record wasn't
		 * fragmented.  Hand the dissector the tvbuff for the
		 * fragment as the tvbuff for the record.
		 */
		rec_tvb = frag_tvb;
		ipfd_head = NULL;
	} else {
		/*
		 * OK, this fragment was found, which means it continues
		 * a record.  This means we must defragment it.
		 * Add it to the defragmentation lists.
		 */
		ipfd_head = fragment_add_multiple_ok(&rpc_fragment_table,
		    tvb, offset + 4, pinfo, rfk->start_seq, NULL,
		    rfk->offset, len - 4, !(rpc_rm & RPC_RM_LASTFRAG));

		if (ipfd_head == NULL) {
			/*
			 * fragment_add_multiple_ok() returned NULL.
			 * This means that defragmentation is not
			 * completed yet.
			 *
			 * We must add an entry to the hash table with
			 * the sequence number following this fragment
			 * as the starting sequence number, so that when
			 * we see that fragment we'll find that entry.
			 *
			 * XXX - as TCP stream data is not currently
			 * guaranteed to be provided in order to dissectors,
			 * RPC fragments aren't guaranteed to be provided
			 * in order, either.
			 */
			new_rfk = wmem_new(wmem_file_scope(), rpc_fragment_key);
			new_rfk->conv_id = rfk->conv_id;
			new_rfk->seq = seq + len;
			new_rfk->port = pinfo->srcport;
			new_rfk->offset = rfk->offset + len - 4;
			new_rfk->start_seq = rfk->start_seq;
			g_hash_table_insert(rpc_reassembly_table, new_rfk,
			    new_rfk);

			/*
			 * This is part of a fragmented record,
			 * but it's not the first part.
			 * Show it as a record marker plus data, under
			 * a top-level tree for this protocol,
			 * but don't hand it to the dissector
			 */
			col_set_str(pinfo->cinfo, COL_PROTOCOL, "RPC");
			col_set_str(pinfo->cinfo, COL_INFO, "Fragment");
			make_frag_tree(frag_tvb, tree, proto, ett, rpc_rm);

			/*
			 * No more processing need be done, as we don't
			 * have a complete record.
			 */
			return len;
		}

		/*
		 * It's completely defragmented.
		 *
		 * We only call subdissector for the last fragment.
		 * XXX - this assumes in-order delivery of RPC
		 * fragments, which requires in-order delivery of TCP
		 * segments.
		 */
		if (!(rpc_rm & RPC_RM_LASTFRAG)) {
			/*
			 * Well, it's defragmented, but this isn't
			 * the last fragment; this probably means
			 * this isn't the first pass, so we don't
			 * need to start defragmentation.
			 *
			 * This is part of a fragmented record,
			 * but it's not the first part.
			 * Show it as a record marker plus data, under
			 * a top-level tree for this protocol,
			 * but don't show it to the dissector.
			 */
			col_set_str(pinfo->cinfo, COL_PROTOCOL, "RPC");
			col_set_str(pinfo->cinfo, COL_INFO, "Fragment");
			make_frag_tree(frag_tvb, tree, proto, ett, rpc_rm);

			/*
			 * No more processing need be done, as we
			 * only disssect the data with the last
			 * fragment.
			 */
			return len;
		}

		/*
		 * OK, this is the last segment.
		 * Create a tvbuff for the defragmented
		 * record.
		 */

		/*
		 * Create a new TVB structure for
		 * defragmented data.
		 */
		rec_tvb = tvb_new_chain(tvb, ipfd_head->tvb_data);

		/*
		 * Add defragmented data to the data source list.
		 */
		add_new_data_source(pinfo, rec_tvb, "Defragmented");
	}

	/*
	 * We have something to hand to the RPC message
	 * dissector.
	 */
	if (!call_message_dissector(tvb, rec_tvb, pinfo, tree,
	    frag_tvb, dissector, ipfd_head, rpc_rm, first_pdu))
		return 0;	/* not RPC */
	return len;
}  /* end of dissect_rpc_fragment() */

/**
 * Scans tvb, starting at given offset, to see if we can find
 * what looks like a valid RPC-over-TCP reply header.
 *
 * @param tvb Buffer to inspect for RPC reply header.
 * @param offset Offset to begin search of tvb at.
 *
 * @return -1 if no reply header found, else offset to start of header
 *         (i.e., to the RPC record mark field).
 */

static int
find_rpc_over_tcp_reply_start(tvbuff_t *tvb, int offset)
{

	/*
	 * Looking for partial header sequence.  From beginning of
	 * stream-style header, including "record mark", full ONC-RPC
	 * looks like:
	 *    BE int32    record mark (rfc 1831 sec. 10)
	 *    ?  int32    XID (rfc 1831 sec. 8)
	 *    BE int32    msg_type (ibid sec. 8, call = 0, reply = 1)
	 *
	 * -------------------------------------------------------------
	 * Then reply-specific fields are
	 *    BE int32    reply_stat (ibid, accept = 0, deny = 1)
	 *
	 * Then, assuming accepted,
	 *   opaque_auth
	 *    BE int32    auth_flavor (ibid, none = 0)
	 *    BE int32    ? auth_len (ibid, none = 0)
	 *
	 *    BE int32    accept_stat (ibid, success = 0, errs are 1..5 in rpc v2)
	 *
	 * -------------------------------------------------------------
	 * Or, call-specific fields are
	 *    BE int32    rpc_vers (rfc 1831 sec 8, always == 2)
	 *    BE int32    prog (NFS == 000186A3)
	 *    BE int32    prog_ver (NFS v2/3 == 2 or 3)
	 *    BE int32    proc_id (NFS, <= 256 ???)
	 *   opaque_auth
	 *    ...
	 */

	/* Initially, we search only for something matching the template
	 * of a successful reply with no auth verifier.
	 * Our first qualification test is search for a string of zero bytes,
	 * corresponding the four guint32 values
	 *    reply_stat
	 *    auth_flavor
	 *    auth_len
	 *    accept_stat
	 *
	 * If this string of zeros matches, then we go back and check the
	 * preceding msg_type and record_mark fields.
	 */

	const gint     cbZeroTail = 4 * 4;     /* four guint32s of zeros */
	const gint     ibPatternStart = 3 * 4;    /* offset of zero fill from reply start */
	const guint8 * pbWholeBuf;    /* all of tvb, from offset onwards */
	const int      NoMatch = -1;

	gint     ibSearchStart;       /* offset of search start, in case of false hits. */

	const    guint8 * pbBuf;

	gint     cbInBuf;       /* bytes in tvb, from offset onwards */

	guint32  ulMsgType;
	guint32  ulRecMark;

	int      i;


	cbInBuf = tvb_reported_length_remaining(tvb, offset);

	/* start search at first possible location */
	ibSearchStart = ibPatternStart;

	if (cbInBuf < (cbZeroTail + ibSearchStart)) {
		/* nothing to search, so claim no RPC */
		return (NoMatch);
	}

	pbWholeBuf = tvb_get_ptr(tvb, offset, cbInBuf);
	if (pbWholeBuf == NULL) {
		/* probably never take this, as get_ptr seems to assert */
		return (NoMatch);
	}

	while ((cbInBuf - ibSearchStart) > cbZeroTail) {
		/* First test for long tail of zeros, starting at the back.
		 * A failure lets us skip the maximum possible buffer amount.
		 */
		pbBuf = pbWholeBuf + ibSearchStart + cbZeroTail - 1;
		for (i = cbZeroTail; i > 0;  i --)
			{
			if (*pbBuf != 0)
				{
				/* match failure.  Since we need N contiguous zeros,
				 * we can increment next match start so zero testing
				 * begins right after this failure spot.
				 */
				ibSearchStart += i;
				pbBuf = NULL;
				break;
				}

			pbBuf --;
			}

		if (pbBuf == NULL) {
			continue;
		}

		/* got a match in zero-fill region, verify reply ID and
		 * record mark fields */
		ulMsgType = pntoh32 (pbWholeBuf + ibSearchStart - 4);
		ulRecMark = pntoh32 (pbWholeBuf + ibSearchStart - ibPatternStart);

		if ((ulMsgType == RPC_REPLY) &&
			 ((ulRecMark & ~0x80000000) <= (unsigned) max_rpc_tcp_pdu_size)) {
			/* looks ok, try dissect */
			return (offset + ibSearchStart - ibPatternStart);
		}

		/* no match yet, nor egregious miss either.  Inch along to next try */
		ibSearchStart ++;
	}

	return (NoMatch);

}  /* end of find_rpc_over_tcp_reply_start() */

/**
 * Scans tvb for what looks like a valid RPC call / reply header.
 * If found, calls standard dissect_rpc_fragment() logic to digest
 * the (hopefully valid) fragment.
 *
 * With any luck, one invocation of this will be sufficient to get
 * us back in alignment with the stream, and no further calls to
 * this routine will be needed for a given conversation.  As if.  :-)
 *
 * Can return:
 *       Same as dissect_rpc_fragment().  Will return zero (no frame)
 *       if no valid RPC header is found.
 */

static int
find_and_dissect_rpc_fragment(tvbuff_t *tvb, int offset, packet_info *pinfo,
			      proto_tree *tree, rec_dissector_t dissector,
			      gboolean is_heur,
			      int proto, int ett, struct tcpinfo* tcpinfo)
{

	int   offReply;
	int   len;

	offReply = find_rpc_over_tcp_reply_start(tvb, offset);
	if (offReply < 0) {
		/* could search for request, but not needed (or testable) thus far */
		return (0);    /* claim no RPC */
	}

	len = dissect_rpc_fragment(tvb, offReply,
				   pinfo, tree,
				   dissector, is_heur, proto, ett,
				   TRUE /* force first-pdu state */, tcpinfo);

	/* misses are reported as-is */
	if (len == 0)
	{
		return (0);
	}

	/* returning a non-zero length, correct it to reflect the extra offset
	 * we found necessary
	 */
	if (len > 0) {
		len += offReply - offset;
	}
	else {
		/* negative length seems to only be used as a flag,
		 * don't mess it up until found necessary
		 */
/*      len -= offReply - offset; */
	}

	return (len);

}  /* end of find_and_dissect_rpc_fragment */


/*
 * Returns TRUE if it looks like ONC RPC, FALSE otherwise.
 */
static gboolean
dissect_rpc_tcp_common(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
		       gboolean is_heur, struct tcpinfo* tcpinfo)
{
	int offset = 0;
	gboolean saw_rpc = FALSE;
	gboolean first_pdu = TRUE;
	int len;

	while (tvb_reported_length_remaining(tvb, offset) != 0) {
		/*
		 * Process this fragment.
		 */
		len = dissect_rpc_fragment(tvb, offset, pinfo, tree,
		    dissect_rpc_message, is_heur, proto_rpc, ett_rpc,
		    first_pdu, tcpinfo);

		if ((len == 0) && first_pdu && rpc_find_fragment_start) {
			/*
			 * Try discarding some leading bytes from tvb, on assumption
			 * that we are looking at the middle of a stream-based transfer
			 */
			len = find_and_dissect_rpc_fragment(tvb, offset, pinfo, tree,
				 dissect_rpc_message, is_heur, proto_rpc, ett_rpc,
				 tcpinfo);
		}

		first_pdu = FALSE;
		if (len < 0) {
			/*
			 * dissect_rpc_fragment() thinks this is ONC RPC,
			 * but we need more data from the TCP stream for
			 * this fragment.
			 */
			return TRUE;
		}
		if (len == 0) {
			/*
			 * It's not RPC.  Stop processing.
			 */
			break;
		}

		/*  Set fences so whatever the subdissector put in the
		 *  Protocol and Info columns stay there.  This is useful
		 *  when the subdissector clears the column (which it
		 *  might have to do if it runs over some other protocol
		 *  too) and there are multiple PDUs in one frame.
		 */
		col_set_fence(pinfo->cinfo, COL_PROTOCOL);
		col_set_fence(pinfo->cinfo, COL_INFO);

		/* PDU tracking
		  If the length indicates that the PDU continues beyond
		  the end of this tvb, then tell TCP about it so that it
		  knows where the next PDU starts.
		  This is to help TCP detect when PDUs are not aligned to
		  segment boundaries and allow it to find RPC headers
		  that starts in the middle of a TCP segment.
		*/
		if(!pinfo->fd->flags.visited){
			if(len>tvb_reported_length_remaining(tvb, offset)){
				pinfo->want_pdu_tracking=2;
				pinfo->bytes_until_next_pdu=len-tvb_reported_length_remaining(tvb, offset);
			}
		}
		offset += len;
		saw_rpc = TRUE;
	}
	return saw_rpc;
}

static gboolean
dissect_rpc_tcp_heur(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data)
{
	struct tcpinfo* tcpinfo = (struct tcpinfo *)data;

	return dissect_rpc_tcp_common(tvb, pinfo, tree, TRUE, tcpinfo);
}

static int
dissect_rpc_tcp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data)
{
	struct tcpinfo* tcpinfo = (struct tcpinfo *)data;

	if (!dissect_rpc_tcp_common(tvb, pinfo, tree, FALSE, tcpinfo))
		dissect_rpc_continuation(tvb, pinfo, tree);

	return tvb_reported_length(tvb);
}

/* Discard any state we've saved. */
static void
rpc_init_protocol(void)
{
	rpc_reassembly_table = g_hash_table_new(rpc_fragment_hash,
	    rpc_fragment_equal);
	reassembly_table_init(&rpc_fragment_table,
	    &addresses_ports_reassembly_table_functions);
}

static void
rpc_cleanup_protocol(void)
{
	reassembly_table_destroy(&rpc_fragment_table);
	g_hash_table_destroy(rpc_reassembly_table);
}

/* Tap statistics */
typedef enum
{
	PROGRAM_NAME_COLUMN,
	PROGRAM_NUM_COLUMN,
	VERSION_COLUMN,
	CALLS_COLUMN,
	MIN_SRT_COLUMN,
	MAX_SRT_COLUMN,
	AVG_SRT_COLUMN
} rpc_prog_stat_columns;

static stat_tap_table_item rpc_prog_stat_fields[] = {
	{TABLE_ITEM_STRING, TAP_ALIGN_LEFT, "Program", "%-25s"},
	{TABLE_ITEM_UINT, TAP_ALIGN_RIGHT, "Program Num", "%u"},
	{TABLE_ITEM_UINT, TAP_ALIGN_RIGHT, "Version", "%u"},
	{TABLE_ITEM_UINT, TAP_ALIGN_RIGHT, "Calls", "%u"},
	{TABLE_ITEM_FLOAT, TAP_ALIGN_RIGHT, "Min SRT (s)", "%.2f"},
	{TABLE_ITEM_FLOAT, TAP_ALIGN_RIGHT, "Max SRT (s)", "%.2f"},
	{TABLE_ITEM_FLOAT, TAP_ALIGN_RIGHT, "Avg SRT (s)", "%.2f"}
};

static void rpc_prog_stat_init(stat_tap_table_ui* new_stat, new_stat_tap_gui_init_cb gui_callback, void* gui_data)
{
	int num_fields = sizeof(rpc_prog_stat_fields)/sizeof(stat_tap_table_item);
	stat_tap_table* table;

	table = new_stat_tap_init_table("ONC-RPC Program Statistics", num_fields, 0, NULL, gui_callback, gui_data);
	new_stat_tap_add_table(new_stat, table);

}

static gboolean
rpc_prog_stat_packet(void *tapdata, packet_info *pinfo _U_, epan_dissect_t *edt _U_, const void *rciv_ptr)
{
	new_stat_data_t* stat_data = (new_stat_data_t*)tapdata;
	const rpc_call_info_value *ri = (const rpc_call_info_value *)rciv_ptr;
	int num_fields = sizeof(rpc_prog_stat_fields)/sizeof(stat_tap_table_item);
	nstime_t delta;
	double delta_s = 0.0;
	guint call_count;
	guint element;
	gboolean found = FALSE;
	stat_tap_table* table;
	stat_tap_table_item_type* item_data;

	table = g_array_index(stat_data->stat_tap_data->tables, stat_tap_table*, 0);

	for (element = 0; element < table->num_elements; element++)
	{
		stat_tap_table_item_type *program_data, *version_data;
		program_data = new_stat_tap_get_field_data(table, element, PROGRAM_NUM_COLUMN);
		version_data = new_stat_tap_get_field_data(table, element, VERSION_COLUMN);

		if ((ri->prog == program_data->value.uint_value) && (ri->vers == version_data->value.uint_value)) {
			found = TRUE;
			break;
		}
	}

	if (!found) {
		/* Add a new row */
		stat_tap_table_item_type items[sizeof(rpc_prog_stat_fields)/sizeof(stat_tap_table_item)];
		memset(items, 0, sizeof(items));

		items[PROGRAM_NAME_COLUMN].type = TABLE_ITEM_STRING;
		items[PROGRAM_NAME_COLUMN].value.string_value = g_strdup(rpc_prog_name(ri->prog));
		items[PROGRAM_NUM_COLUMN].type = TABLE_ITEM_UINT;
		items[PROGRAM_NUM_COLUMN].value.uint_value = ri->prog;
		items[VERSION_COLUMN].type = TABLE_ITEM_UINT;
		items[VERSION_COLUMN].value.uint_value = ri->vers;
		items[CALLS_COLUMN].type = TABLE_ITEM_UINT;
		items[MIN_SRT_COLUMN].type = TABLE_ITEM_FLOAT;
		items[MAX_SRT_COLUMN].type = TABLE_ITEM_FLOAT;
		items[AVG_SRT_COLUMN].type = TABLE_ITEM_FLOAT;

		new_stat_tap_init_table_row(table, element, num_fields, items);
	}

	/* we are only interested in reply packets */
	if (ri->request) {
		return FALSE;
	}

	item_data = new_stat_tap_get_field_data(table, element, CALLS_COLUMN);
	item_data->value.uint_value++;
	call_count = item_data->value.uint_value;
	new_stat_tap_set_field_data(table, element, CALLS_COLUMN, item_data);

	/* calculate time delta between request and reply */
	nstime_delta(&delta, &pinfo->abs_ts, &ri->req_time);
	delta_s = nstime_to_sec(&delta);

	item_data = new_stat_tap_get_field_data(table, element, MIN_SRT_COLUMN);
	if (item_data->value.float_value == 0.0 || delta_s < item_data->value.float_value) {
		item_data->value.float_value = delta_s;
		new_stat_tap_set_field_data(table, element, MIN_SRT_COLUMN, item_data);
	}

	item_data = new_stat_tap_get_field_data(table, element, MAX_SRT_COLUMN);
	if (item_data->value.float_value == 0.0 || delta_s > item_data->value.float_value) {
		item_data->value.float_value = delta_s;
		new_stat_tap_set_field_data(table, element, MAX_SRT_COLUMN, item_data);
	}

	item_data = new_stat_tap_get_field_data(table, element, AVG_SRT_COLUMN);
	item_data->user_data.float_value += delta_s;
	item_data->value.float_value = item_data->user_data.float_value / call_count;
	new_stat_tap_set_field_data(table, element, AVG_SRT_COLUMN, item_data);

	return TRUE;
}

static void
rpc_prog_stat_reset(stat_tap_table* table)
{
	guint element;
	stat_tap_table_item_type* item_data;

	for (element = 0; element < table->num_elements; element++)
	{
		item_data = new_stat_tap_get_field_data(table, element, CALLS_COLUMN);
		item_data->value.uint_value = 0;
		new_stat_tap_set_field_data(table, element, CALLS_COLUMN, item_data);
		item_data = new_stat_tap_get_field_data(table, element, MIN_SRT_COLUMN);
		item_data->value.float_value = 0.0;
		new_stat_tap_set_field_data(table, element, MIN_SRT_COLUMN, item_data);
		item_data = new_stat_tap_get_field_data(table, element, MAX_SRT_COLUMN);
		item_data->value.float_value = 0.0;
		new_stat_tap_set_field_data(table, element, MAX_SRT_COLUMN, item_data);
		item_data = new_stat_tap_get_field_data(table, element, AVG_SRT_COLUMN);
		item_data->value.float_value = 0.0;
		new_stat_tap_set_field_data(table, element, AVG_SRT_COLUMN, item_data);
	}
}

static void
rpc_prog_stat_free_table_item(stat_tap_table* table _U_, guint row _U_, guint column, stat_tap_table_item_type* field_data)
{
	if (column != PROGRAM_NAME_COLUMN) return;
	g_free((char*)field_data->value.string_value);
}

/* will be called once from register.c at startup time */
void
proto_register_rpc(void)
{
	static hf_register_info hf[] = {
		{ &hf_rpc_reqframe, {
			"Request Frame", "rpc.reqframe", FT_FRAMENUM, BASE_NONE,
			NULL, 0, NULL, HFILL }},
		{ &hf_rpc_repframe, {
			"Reply Frame", "rpc.repframe", FT_FRAMENUM, BASE_NONE,
			NULL, 0, NULL, HFILL }},
		{ &hf_rpc_lastfrag, {
			"Last Fragment", "rpc.lastfrag", FT_BOOLEAN, 32,
			TFS(&tfs_yes_no), RPC_RM_LASTFRAG, NULL, HFILL }},
		{ &hf_rpc_fraglen, {
			"Fragment Length", "rpc.fraglen", FT_UINT32, BASE_DEC,
			NULL, RPC_RM_FRAGLEN, NULL, HFILL }},
		{ &hf_rpc_xid, {
			"XID", "rpc.xid", FT_UINT32, BASE_HEX_DEC,
			NULL, 0, NULL, HFILL }},
		{ &hf_rpc_msgtype, {
			"Message Type", "rpc.msgtyp", FT_UINT32, BASE_DEC,
			VALS(rpc_msg_type), 0, NULL, HFILL }},
		{ &hf_rpc_state_reply, {
			"Reply State", "rpc.replystat", FT_UINT32, BASE_DEC,
			VALS(rpc_reply_state), 0, NULL, HFILL }},
		{ &hf_rpc_state_accept, {
			"Accept State", "rpc.state_accept", FT_UINT32, BASE_DEC,
			VALS(rpc_accept_state), 0, NULL, HFILL }},
		{ &hf_rpc_state_reject, {
			"Reject State", "rpc.state_reject", FT_UINT32, BASE_DEC,
			VALS(rpc_reject_state), 0, NULL, HFILL }},
		{ &hf_rpc_state_auth, {
			"Auth State", "rpc.state_auth", FT_UINT32, BASE_DEC,
			VALS(rpc_auth_state), 0, NULL, HFILL }},
		{ &hf_rpc_version, {
			"RPC Version", "rpc.version", FT_UINT32, BASE_DEC,
			NULL, 0, NULL, HFILL }},
		{ &hf_rpc_version_min, {
			"RPC Version (Minimum)", "rpc.version.min", FT_UINT32,
			BASE_DEC, NULL, 0, "Program Version (Minimum)", HFILL }},
		{ &hf_rpc_version_max, {
			"RPC Version (Maximum)", "rpc.version.max", FT_UINT32,
			BASE_DEC, NULL, 0, NULL, HFILL }},
		{ &hf_rpc_program, {
			"Program", "rpc.program", FT_UINT32, BASE_DEC,
			NULL, 0, NULL, HFILL }},
		{ &hf_rpc_programversion, {
			"Program Version", "rpc.programversion", FT_UINT32,
			BASE_DEC, NULL, 0, NULL, HFILL }},
		{ &hf_rpc_programversion_min, {
			"Program Version (Minimum)", "rpc.programversion.min", FT_UINT32,
			BASE_DEC, NULL, 0, NULL, HFILL }},
		{ &hf_rpc_programversion_max, {
			"Program Version (Maximum)", "rpc.programversion.max", FT_UINT32,
			BASE_DEC, NULL, 0, NULL, HFILL }},
		{ &hf_rpc_procedure, {
			"Procedure", "rpc.procedure", FT_UINT32, BASE_DEC,
			NULL, 0, NULL, HFILL }},
		{ &hf_rpc_auth_flavor, {
			"Flavor", "rpc.auth.flavor", FT_UINT32, BASE_DEC,
			VALS(rpc_auth_flavor), 0, NULL, HFILL }},
		{ &hf_rpc_auth_length, {
			"Length", "rpc.auth.length", FT_UINT32, BASE_DEC,
			NULL, 0, NULL, HFILL }},
		{ &hf_rpc_auth_stamp, {
			"Stamp", "rpc.auth.stamp", FT_UINT32, BASE_HEX,
			NULL, 0, NULL, HFILL }},
		{ &hf_rpc_auth_lk_owner, {
			"Lock Owner", "rpc.auth.lk_owner", FT_BYTES, BASE_NONE,
			NULL, 0, NULL, HFILL }},
		{ &hf_rpc_auth_pid, {
			"PID", "rpc.auth.pid", FT_UINT32, BASE_DEC,
			NULL, 0, NULL, HFILL }},
		{ &hf_rpc_auth_uid, {
			"UID", "rpc.auth.uid", FT_UINT32, BASE_DEC,
			NULL, 0, NULL, HFILL }},
		{ &hf_rpc_auth_gid, {
			"GID", "rpc.auth.gid", FT_UINT32, BASE_DEC,
			NULL, 0, NULL, HFILL }},
		{ &hf_rpc_authgss_v, {
			"GSS Version", "rpc.authgss.version", FT_UINT32,
			BASE_DEC, NULL, 0, NULL, HFILL }},
		{ &hf_rpc_authgss_proc, {
			"GSS Procedure", "rpc.authgss.procedure", FT_UINT32,
			BASE_DEC, VALS(rpc_authgss_proc), 0, NULL, HFILL }},
		{ &hf_rpc_authgss_seq, {
			"GSS Sequence Number", "rpc.authgss.seqnum", FT_UINT32,
			BASE_DEC, NULL, 0, NULL, HFILL }},
		{ &hf_rpc_authgss_svc, {
			"GSS Service", "rpc.authgss.service", FT_UINT32,
			BASE_DEC, VALS(rpc_authgss_svc), 0, NULL, HFILL }},
		{ &hf_rpc_authgss_ctx, {
			"GSS Context", "rpc.authgss.context", FT_BYTES,
			BASE_NONE, NULL, 0, NULL, HFILL }},
		{ &hf_rpc_authgss_ctx_create_frame, {
			"Created in frame", "rpc.authgss.context.created_frame", FT_FRAMENUM,
			BASE_NONE, NULL, 0, NULL, HFILL }},
		{ &hf_rpc_authgss_ctx_destroy_frame, {
			"Destroyed in frame", "rpc.authgss.context.destroyed_frame", FT_FRAMENUM,
			BASE_NONE, NULL, 0, NULL, HFILL }},
		{ &hf_rpc_authgss_ctx_len, {
			"GSS Context Length", "rpc.authgss.context.length", FT_UINT32,
			BASE_DEC, NULL, 0, NULL, HFILL }},
		{ &hf_rpc_authgss_major, {
			"GSS Major Status", "rpc.authgss.major", FT_UINT32,
			BASE_DEC, NULL, 0, NULL, HFILL }},
		{ &hf_rpc_authgss_minor, {
			"GSS Minor Status", "rpc.authgss.minor", FT_UINT32,
			BASE_DEC, NULL, 0, NULL, HFILL }},
		{ &hf_rpc_authgss_window, {
			"GSS Sequence Window", "rpc.authgss.window", FT_UINT32,
			BASE_DEC, NULL, 0, NULL, HFILL }},
		{ &hf_rpc_authgss_token_length, {
			"GSS Token Length", "rpc.authgss.token_length", FT_UINT32,
			BASE_DEC, NULL, 0, NULL, HFILL }},
		{ &hf_rpc_authgss_data_length, {
			"Length", "rpc.authgss.data.length", FT_UINT32,
			BASE_DEC, NULL, 0, NULL, HFILL }},
		{ &hf_rpc_authgss_data, {
			"GSS Data", "rpc.authgss.data", FT_BYTES,
			BASE_NONE, NULL, 0, NULL, HFILL }},
		{ &hf_rpc_authgss_checksum, {
			"GSS Checksum", "rpc.authgss.checksum", FT_BYTES,
			BASE_NONE, NULL, 0, NULL, HFILL }},
		{ &hf_rpc_authgss_token, {
			"GSS Token", "rpc.authgss.token", FT_BYTES,
			BASE_NONE, NULL, 0, NULL, HFILL }},
		{ &hf_rpc_authgssapi_v, {
			"AUTH_GSSAPI Version", "rpc.authgssapi.version",
			FT_UINT32, BASE_DEC, NULL, 0, NULL,
			HFILL }},
		{ &hf_rpc_authgssapi_msg, {
			"AUTH_GSSAPI Message", "rpc.authgssapi.message",
			FT_BOOLEAN, BASE_NONE, TFS(&tfs_yes_no), 0x0, NULL,
			HFILL }},
		{ &hf_rpc_authgssapi_msgv, {
			"Msg Version", "rpc.authgssapi.msgversion",
			FT_UINT32, BASE_DEC, NULL, 0, NULL,
			HFILL }},
		{ &hf_rpc_authgssapi_handle, {
			"Client Handle", "rpc.authgssapi.handle",
			FT_BYTES, BASE_NONE, NULL, 0, NULL, HFILL }},
		{ &hf_rpc_authgssapi_isn, {
			"Signed ISN", "rpc.authgssapi.isn",
			FT_BYTES, BASE_NONE, NULL, 0, NULL, HFILL }},
		{ &hf_rpc_authdes_namekind, {
			"Namekind", "rpc.authdes.namekind", FT_UINT32, BASE_DEC,
			VALS(rpc_authdes_namekind), 0, NULL, HFILL }},
		{ &hf_rpc_authdes_netname, {
			"Netname", "rpc.authdes.netname", FT_STRING,
			BASE_NONE, NULL, 0, NULL, HFILL }},
		{ &hf_rpc_authdes_convkey, {
			"Conversation Key (encrypted)", "rpc.authdes.convkey", FT_UINT64,
			BASE_HEX, NULL, 0, NULL, HFILL }},
		{ &hf_rpc_authdes_window, {
			"Window (encrypted)", "rpc.authdes.window", FT_UINT32,
			BASE_HEX, NULL, 0, "Windows (encrypted)", HFILL }},
		{ &hf_rpc_authdes_nickname, {
			"Nickname", "rpc.authdes.nickname", FT_UINT32,
			BASE_HEX, NULL, 0, NULL, HFILL }},
		{ &hf_rpc_authdes_timestamp, {
			"Timestamp (encrypted)", "rpc.authdes.timestamp", FT_UINT64,
			BASE_HEX, NULL, 0, NULL, HFILL }},
		{ &hf_rpc_authdes_windowverf, {
			"Window verifier (encrypted)", "rpc.authdes.windowverf", FT_UINT32,
			BASE_HEX, NULL, 0, NULL, HFILL }},
		{ &hf_rpc_authdes_timeverf, {
			"Timestamp verifier (encrypted)", "rpc.authdes.timeverf", FT_UINT64,
			BASE_HEX, NULL, 0, NULL, HFILL }},
		{ &hf_rpc_auth_machinename, {
			"Machine Name", "rpc.auth.machinename", FT_STRING,
			BASE_NONE, NULL, 0, NULL, HFILL }},
		{ &hf_rpc_dup, {
			"Duplicate Call/Reply", "rpc.dup", FT_NONE, BASE_NONE,
			NULL, 0, NULL, HFILL }},
		{ &hf_rpc_call_dup, {
			"Duplicate to the call in", "rpc.call.dup", FT_FRAMENUM, BASE_NONE,
			NULL, 0, "This is a duplicate to the call in frame", HFILL }},
		{ &hf_rpc_reply_dup, {
			"Duplicate to the reply in", "rpc.reply.dup", FT_FRAMENUM, BASE_NONE,
			NULL, 0, "This is a duplicate to the reply in frame", HFILL }},
		{ &hf_rpc_value_follows, {
			"Value Follows", "rpc.value_follows", FT_BOOLEAN, BASE_NONE,
			TFS(&tfs_yes_no), 0x0, NULL, HFILL }},
		{ &hf_rpc_array_len, {
			"num", "rpc.array.len", FT_UINT32, BASE_DEC,
			NULL, 0, "Length of RPC array", HFILL }},

          /* Generated from convert_proto_tree_add_text.pl */
          { &hf_rpc_opaque_length, { "length", "rpc.opaque_length", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
          { &hf_rpc_fill_bytes, { "fill bytes", "rpc.fill_bytes", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }},
          { &hf_rpc_no_values, { "no values", "rpc.array_no_values", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }},
          { &hf_rpc_opaque_data, { "opaque data", "rpc.opaque_data", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }},
          { &hf_rpc_argument_length, { "Argument length", "rpc.argument_length", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
          { &hf_rpc_continuation_data, { "Continuation data", "rpc.continuation_data", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }},
          { &hf_rpc_fragment_data, { "Fragment Data", "rpc.fragment_data", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }},

		{ &hf_rpc_time, {
			"Time from request", "rpc.time", FT_RELATIVE_TIME, BASE_NONE,
			NULL, 0, "Time between Request and Reply for ONC-RPC calls", HFILL }},

		{ &hf_rpc_fragment_overlap,
		{ "Fragment overlap",	"rpc.fragment.overlap", FT_BOOLEAN, BASE_NONE, NULL, 0x0,
			"Fragment overlaps with other fragments", HFILL }},

		{ &hf_rpc_fragment_overlap_conflict,
		{ "Conflicting data in fragment overlap",	"rpc.fragment.overlap.conflict", FT_BOOLEAN, BASE_NONE, NULL, 0x0,
			"Overlapping fragments contained conflicting data", HFILL }},

		{ &hf_rpc_fragment_multiple_tails,
		{ "Multiple tail fragments found",	"rpc.fragment.multipletails", FT_BOOLEAN, BASE_NONE, NULL, 0x0,
			"Several tails were found when defragmenting the packet", HFILL }},

		{ &hf_rpc_fragment_too_long_fragment,
		{ "Fragment too long",	"rpc.fragment.toolongfragment", FT_BOOLEAN, BASE_NONE, NULL, 0x0,
			"Fragment contained data past end of packet", HFILL }},

		{ &hf_rpc_fragment_error,
		{ "Defragmentation error", "rpc.fragment.error", FT_FRAMENUM, BASE_NONE, NULL, 0x0,
			"Defragmentation error due to illegal fragments", HFILL }},

		{ &hf_rpc_fragment_count,
		{ "Fragment count", "rpc.fragment.count", FT_UINT32, BASE_DEC, NULL, 0x0,
			NULL, HFILL }},

		{ &hf_rpc_fragment,
		{ "RPC Fragment", "rpc.fragment", FT_FRAMENUM, BASE_NONE, NULL, 0x0,
			NULL, HFILL }},

		{ &hf_rpc_fragments,
		{ "RPC Fragments", "rpc.fragments", FT_NONE, BASE_NONE, NULL, 0x0,
			NULL, HFILL }},

		{ &hf_rpc_reassembled_length,
		{ "Reassembled RPC length", "rpc.reassembled.length", FT_UINT32, BASE_DEC, NULL, 0x0,
			"The total length of the reassembled payload", HFILL }},

		{ &hf_rpc_unknown_body,
		{ "Unknown RPC call/reply body", "rpc.unknown_body", FT_NONE, BASE_NONE, NULL, 0x0,
			NULL, HFILL }},
	};
	static gint *ett[] = {
		&ett_rpc,
		&ett_rpc_fragments,
		&ett_rpc_fragment,
		&ett_rpc_fraghdr,
		&ett_rpc_string,
		&ett_rpc_cred,
		&ett_rpc_verf,
		&ett_rpc_gids,
		&ett_rpc_gss_token,
		&ett_rpc_gss_data,
		&ett_rpc_array,
		&ett_rpc_authgssapi_msg,
		&ett_rpc_unknown_program,
		&ett_gss_context,
		&ett_gss_wrap,
	};
	static ei_register_info ei[] = {
		{ &ei_rpc_cannot_dissect, { "rpc.cannot_dissect", PI_UNDECODED, PI_WARN, "Cannot dissect", EXPFILL }},
	};

	module_t *rpc_module;
	expert_module_t* expert_rpc;

	static tap_param rpc_prog_stat_params[] = {
		{ PARAM_FILTER, "filter", "Filter", NULL, TRUE }
	};

	static stat_tap_table_ui rpc_prog_stat_table = {
		REGISTER_STAT_GROUP_UNSORTED,
		"ONC-RPC Programs",
		"rpc",
		"rpc,programs",
		rpc_prog_stat_init,
		rpc_prog_stat_packet,
		rpc_prog_stat_reset,
		rpc_prog_stat_free_table_item,
		NULL,
		sizeof(rpc_prog_stat_fields)/sizeof(stat_tap_table_item), rpc_prog_stat_fields,
		sizeof(rpc_prog_stat_params)/sizeof(tap_param), rpc_prog_stat_params,
		NULL,
		0
	};

	proto_rpc = proto_register_protocol("Remote Procedure Call", "RPC", "rpc");

	subdissector_call_table = register_custom_dissector_table("rpc.call", "RPC Call Functions", proto_rpc, rpc_proc_hash, rpc_proc_equal);
	subdissector_reply_table = register_custom_dissector_table("rpc.reply", "RPC Reply Functions", proto_rpc, rpc_proc_hash, rpc_proc_equal);

	/* this is a dummy dissector for all those unknown rpc programs */
	proto_register_field_array(proto_rpc, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
	expert_rpc = expert_register_protocol(proto_rpc);
	expert_register_field_array(expert_rpc, ei, array_length(ei));
	register_init_routine(&rpc_init_protocol);
	register_cleanup_routine(&rpc_cleanup_protocol);

	rpc_module = prefs_register_protocol(proto_rpc, NULL);
	prefs_register_bool_preference(rpc_module, "desegment_rpc_over_tcp",
	    "Reassemble RPC over TCP messages\nspanning multiple TCP segments",
	    "Whether the RPC dissector should reassemble messages spanning multiple TCP segments."
	    " To use this option, you must also enable \"Allow subdissectors to reassemble TCP streams\" in the TCP protocol settings.",
		&rpc_desegment);
	prefs_register_bool_preference(rpc_module, "defragment_rpc_over_tcp",
		"Reassemble fragmented RPC-over-TCP messages",
		"Whether the RPC dissector should defragment RPC-over-TCP messages.",
		&rpc_defragment);

	prefs_register_uint_preference(rpc_module, "max_tcp_pdu_size", "Maximum size of a RPC-over-TCP PDU",
		"Set the maximum size of RPCoverTCP PDUs. "
		" If the size field of the record marker is larger "
		"than this value it will not be considered a valid RPC PDU.",
		 10, &max_rpc_tcp_pdu_size);

	prefs_register_bool_preference(rpc_module, "dissect_unknown_programs",
		"Dissect unknown RPC program numbers",
		"Whether the RPC dissector should attempt to dissect RPC PDUs containing programs that are not known to Wireshark. This will make the heuristics significantly weaker and elevate the risk for falsely identifying and misdissecting packets significantly.",
		&rpc_dissect_unknown_programs);

	prefs_register_bool_preference(rpc_module, "find_fragment_start",
		"Attempt to locate start-of-fragment in partial RPC-over-TCP captures",
		"Whether the RPC dissector should attempt to locate RPC PDU boundaries when initial fragment alignment is not known.  This may cause false positives, or slow operation.",
		&rpc_find_fragment_start);

	register_dissector("rpc", dissect_rpc, proto_rpc);
	register_dissector("rpc-tcp", dissect_rpc_tcp, proto_rpc);
	rpc_tap = register_tap("rpc");

	register_srt_table(proto_rpc, NULL, 1, rpcstat_packet, rpcstat_init, rpcstat_param);
	register_stat_tap_table_ui(&rpc_prog_stat_table);

	/*
	 * Init the hash tables.  Dissectors for RPC protocols must
	 * have a "handoff registration" routine that registers the
	 * protocol with RPC; they must not do it in their protocol
	 * registration routine, as their protocol registration
	 * routine might be called before this routine is called and
	 * thus might be called before the hash tables are initialized,
	 * but it's guaranteed that all protocol registration routines
	 * will be called before any handoff registration routines
	 * are called.
	 */
	rpc_progs = g_hash_table_new_full(g_direct_hash, g_direct_equal,
			NULL, rpc_prog_free_val);

	authgss_contexts=wmem_tree_new_autoreset(wmem_epan_scope(), wmem_file_scope());
}

void
proto_reg_handoff_rpc(void)
{
	/* tcp/udp port 111 is used by portmapper which is an onc-rpc service.
	   we register onc-rpc on this port so that we can choose RPC in
	   the list offered by DecodeAs, and so that traffic to or from
	   port 111 from or to a higher-numbered port is dissected as RPC
	   even if there's a dissector registered on the other port (it's
	   probably RPC traffic from some randomly-chosen port that happens
	   to match some port for which we have a dissector)
	*/
	rpc_tcp_handle = find_dissector("rpc-tcp");
	dissector_add_uint("tcp.port", 111, rpc_tcp_handle);
	rpc_handle = find_dissector("rpc");
	dissector_add_uint("udp.port", 111, rpc_handle);

	heur_dissector_add("tcp", dissect_rpc_tcp_heur, "RPC over TCP", "rpc_tcp", proto_rpc, HEURISTIC_ENABLE);
	heur_dissector_add("udp", dissect_rpc_heur, "RPC over UDP", "rpc_udp", proto_rpc, HEURISTIC_ENABLE);
	gssapi_handle = find_dissector_add_dependency("gssapi", proto_rpc);
	spnego_krb5_wrap_handle = find_dissector_add_dependency("spnego-krb5-wrap", proto_rpc);
	data_handle = find_dissector("data");
}

/*
 * Editor modelines
 *
 * Local Variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: t
 * End:
 *
 * ex: set shiftwidth=8 tabstop=8 noexpandtab:
 * :indentSize=8:tabSize=8:noTabs=false:
 */
