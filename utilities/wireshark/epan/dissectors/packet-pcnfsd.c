/* packet-pcnfsd.c
 * Routines for PCNFSD dissection
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * Copied from packet-ypbind.c
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
Protocol information comes from the book
    "NFS Illustrated" by Brent Callaghan, ISBN 0-201-32570-5
*/


#include "config.h"


#include "packet-rpc.h"
#include "packet-pcnfsd.h"

void proto_register_pcnfsd(void);
void proto_reg_handoff_pcnfsd(void);

static int proto_pcnfsd = -1;
static int hf_pcnfsd_procedure_v1 = -1;
static int hf_pcnfsd_procedure_v2 = -1;
static int hf_pcnfsd_auth_client = -1;
static int hf_pcnfsd_auth_ident_obscure = -1;
static int hf_pcnfsd_auth_ident_clear = -1;
static int hf_pcnfsd_auth_password_obscure = -1;
static int hf_pcnfsd_auth_password_clear = -1;
static int hf_pcnfsd_comment = -1;
static int hf_pcnfsd_status = -1;
static int hf_pcnfsd_uid = -1;
static int hf_pcnfsd_gid = -1;
static int hf_pcnfsd_gids_count = -1;
static int hf_pcnfsd_homedir = -1;
static int hf_pcnfsd_def_umask = -1;
static int hf_pcnfsd_mapreq = -1;
static int hf_pcnfsd_mapreq_status = -1;
static int hf_pcnfsd_username = -1;


static gint ett_pcnfsd = -1;
static gint ett_pcnfsd_auth_ident = -1;
static gint ett_pcnfsd_auth_password = -1;
static gint ett_pcnfsd_gids = -1;

static int
dissect_pcnfsd_username(tvbuff_t *tvb, int offset, proto_tree *tree)
{
    return dissect_rpc_string(tvb, tree, hf_pcnfsd_username, offset, NULL);
}

#define MAP_REQ_UID     0
#define MAP_REQ_GID     1
#define MAP_REQ_UNAME   2
#define MAP_REQ_GNAME   3

static const value_string names_mapreq[] =
{
    { MAP_REQ_UID,   "MAP_REQ_UID"   },
    { MAP_REQ_GID,   "MAP_REQ_GID"   },
    { MAP_REQ_UNAME, "MAP_REQ_UNAME" },
    { MAP_REQ_GNAME, "MAP_REQ_GNAME" },
    { 0,  NULL    }
};

static int
dissect_pcnfsd2_dissect_mapreq_arg_item(tvbuff_t *tvb, int offset,
    packet_info *pinfo _U_, proto_tree *tree, void* data _U_)
{
    proto_tree_add_item(tree, hf_pcnfsd_mapreq, tvb, offset, 4, ENC_BIG_ENDIAN);
    offset += 4;

    offset = dissect_rpc_uint32(tvb, tree, hf_pcnfsd_uid, offset);

    offset = dissect_pcnfsd_username(tvb, offset, tree);

    return offset;
}

static int
dissect_pcnfsd2_mapid_call(tvbuff_t *tvb, packet_info *pinfo,
    proto_tree *tree, void* data _U_)
{
    int offset = 0;
    offset = dissect_rpc_string(tvb, tree, hf_pcnfsd_comment, offset, NULL);

    offset = dissect_rpc_list(tvb, pinfo, tree, offset,
          dissect_pcnfsd2_dissect_mapreq_arg_item, NULL);

    return offset;
}

#define MAP_RES_OK      0
#define MAP_RES_UNKNOWN 1
#define MAP_RES_DENIED  2

static const value_string names_maprstat[] =
{
    { MAP_RES_OK,      "MAP_RES_OK" },
    { MAP_RES_UNKNOWN, "MAP_RES_UNKNOWN"    },
    { MAP_RES_DENIED,  "MAP_RES_DENIED" },
    { 0,    NULL    }
};

static int
dissect_pcnfsd2_dissect_mapreq_res_item(tvbuff_t *tvb, int offset,
    packet_info *pinfo _U_, proto_tree *tree, void* data _U_)
{
    proto_tree_add_item(tree, hf_pcnfsd_mapreq, tvb, offset, 4, ENC_BIG_ENDIAN);
    offset += 4;

    proto_tree_add_item(tree, hf_pcnfsd_mapreq_status, tvb, offset, 4, ENC_BIG_ENDIAN);
    offset += 4;

    offset = dissect_rpc_uint32(tvb, tree, hf_pcnfsd_uid, offset);

    offset = dissect_pcnfsd_username(tvb, offset, tree);

    return offset;
}

static int
dissect_pcnfsd2_mapid_reply(tvbuff_t *tvb, packet_info *pinfo,
    proto_tree *tree, void* data _U_)
{
    int offset = 0;
    offset = dissect_rpc_string(tvb, tree, hf_pcnfsd_comment, offset, NULL);

    offset = dissect_rpc_list(tvb, pinfo, tree, offset,
          dissect_pcnfsd2_dissect_mapreq_res_item, NULL);

    return offset;
}

/* "NFS Illustrated 14.7.13 */
static char *
pcnfsd_decode_obscure(const char* data, int len)
{
    char *decoded_buf;
    char *decoded_data;

    decoded_buf = (char *)wmem_alloc(wmem_packet_scope(), len);
    decoded_data = decoded_buf;
    for ( ; len>0 ; len--, data++, decoded_data++) {
        *decoded_data = (*data ^ 0x5b) & 0x7f;
    }
    return decoded_buf;
}


/* "NFS Illustrated" 14.7.13 */
static int
dissect_pcnfsd2_auth_call(tvbuff_t *tvb, packet_info *pinfo _U_,
    proto_tree *tree, void* data _U_)
{
    int         newoffset;
    const char *ident         = NULL;
    const char *ident_decoded;
    proto_item *ident_item;
    proto_tree *ident_tree;
    const char *password      = NULL;
    proto_item *password_item = NULL;
    proto_tree *password_tree = NULL;
    int offset = 0;

    offset = dissect_rpc_string(tvb, tree,
        hf_pcnfsd_auth_client, offset, NULL);

    ident_tree = proto_tree_add_subtree(tree, tvb,
                offset, -1, ett_pcnfsd_auth_ident, &ident_item, "Authentication Ident");

    newoffset = dissect_rpc_string(tvb, ident_tree,
        hf_pcnfsd_auth_ident_obscure, offset, &ident);
    proto_item_set_len(ident_item, newoffset-offset);

    if (ident) {
        /* Only attempt to decode the ident if it has been specified */
        if (strcmp(ident, RPC_STRING_EMPTY) != 0)
            ident_decoded = pcnfsd_decode_obscure(ident, (int)strlen(ident));
        else
            ident_decoded = ident;

        if (ident_tree)
            proto_tree_add_string(ident_tree,
                hf_pcnfsd_auth_ident_clear,
                tvb, offset+4, (gint)strlen(ident_decoded), ident_decoded);
    }
    if (ident_item) {
        proto_item_set_text(ident_item, "Authentication Ident: %s",
            ident);
    }

    offset = newoffset;

    password_tree = proto_tree_add_subtree(tree, tvb,
                offset, -1, ett_pcnfsd_auth_password, NULL, "Authentication Password");

    newoffset = dissect_rpc_string(tvb, password_tree,
        hf_pcnfsd_auth_password_obscure, offset, &password);
    if (password_item) {
        proto_item_set_len(password_item, newoffset-offset);
    }

    if (password) {
        /* Only attempt to decode the password if it has been specified */
        if (strcmp(password, RPC_STRING_EMPTY))
            pcnfsd_decode_obscure(password, (int)strlen(password));

        if (password_tree)
            proto_tree_add_string(password_tree,
                hf_pcnfsd_auth_password_clear,
                tvb, offset+4, (gint)strlen(password), password);
    }
    if (password_item) {
        proto_item_set_text(password_item, "Authentication Password: %s",
            password);
    }

    offset = newoffset;

    offset = dissect_rpc_string(tvb, tree,
        hf_pcnfsd_comment, offset, NULL);

    return offset;
}


/* "NFS Illustrated" 14.7.13 */
static int
dissect_pcnfsd2_auth_reply(tvbuff_t *tvb, packet_info *pinfo _U_,
    proto_tree *tree, void* data _U_)
{
    int         gids_count;
    proto_tree *gtree;
    int         gids_i;
    int offset = 0;

    offset = dissect_rpc_uint32(tvb, tree, hf_pcnfsd_status, offset);
    offset = dissect_rpc_uint32(tvb, tree, hf_pcnfsd_uid, offset);
    offset = dissect_rpc_uint32(tvb, tree, hf_pcnfsd_gid, offset);
    gids_count = tvb_get_ntohl(tvb,offset+0);
    gtree = proto_tree_add_subtree_format(tree, tvb,
            offset, 4+gids_count*4, ett_pcnfsd_gids, NULL, "Group IDs: %d", gids_count);

    proto_tree_add_item(gtree, hf_pcnfsd_gids_count, tvb, offset, 4, ENC_BIG_ENDIAN);

    offset += 4;
    for (gids_i = 0 ; gids_i < gids_count ; gids_i++) {
        offset = dissect_rpc_uint32(tvb, gtree,
                hf_pcnfsd_gid, offset);
    }
    offset = dissect_rpc_string(tvb, tree,
        hf_pcnfsd_homedir, offset, NULL);
    /* should be signed int32 */
    offset = dissect_rpc_uint32(tvb, tree, hf_pcnfsd_def_umask, offset);
    offset = dissect_rpc_string(tvb, tree,
        hf_pcnfsd_comment, offset, NULL);

    return offset;
}


/* "NFS Illustrated", 14.6 */
/* proc number, "proc name", dissect_request, dissect_reply */
static const vsff pcnfsd1_proc[] = {
    { 0,    "NULL",     dissect_rpc_void,    dissect_rpc_void },
    { 1,    "AUTH",     dissect_rpc_unknown, dissect_rpc_unknown },
    { 2,    "PR_INIT",  dissect_rpc_unknown, dissect_rpc_unknown },
    { 3,    "PR_START", dissect_rpc_unknown, dissect_rpc_unknown },
    { 0,    NULL,       NULL,                NULL }
};
static const value_string pcnfsd1_proc_vals[] = {
    { 0,    "NULL" },
    { 1,    "AUTH" },
    { 2,    "PR_INIT" },
    { 3,    "PR_START" },
    { 0,    NULL }
};
/* end of PCNFS version 1 */


/* "NFS Illustrated", 14.7 */
static const vsff pcnfsd2_proc[] = {
    {  0,   "NULL",
    dissect_rpc_void, dissect_rpc_void },
    {  1,   "INFO",       dissect_rpc_unknown, dissect_rpc_unknown },
    {  2,   "PR_INIT",    dissect_rpc_unknown, dissect_rpc_unknown },
    {  3,   "PR_START",   dissect_rpc_unknown, dissect_rpc_unknown },
    {  4,   "PR_LIST",    dissect_rpc_unknown, dissect_rpc_unknown },
    {  5,   "PR_QUEUE",   dissect_rpc_unknown, dissect_rpc_unknown },
    {  6,   "PR_STATUS",  dissect_rpc_unknown, dissect_rpc_unknown },
    {  7,   "PR_CANCEL",  dissect_rpc_unknown, dissect_rpc_unknown },
    {  8,   "PR_ADMIN",   dissect_rpc_unknown, dissect_rpc_unknown },
    {  9,   "PR_REQUEUE", dissect_rpc_unknown, dissect_rpc_unknown },
    { 10,   "PR_HOLD",    dissect_rpc_unknown, dissect_rpc_unknown },
    { 11,   "PR_RELEASE", dissect_rpc_unknown, dissect_rpc_unknown },
    { 12,   "MAPID",
    dissect_pcnfsd2_mapid_call, dissect_pcnfsd2_mapid_reply },
    { 13,   "AUTH",
    dissect_pcnfsd2_auth_call,  dissect_pcnfsd2_auth_reply },
    { 14,   "ALERT",      dissect_rpc_unknown, dissect_rpc_unknown },
    { 0,    NULL,         NULL,               NULL }
};
static const value_string pcnfsd2_proc_vals[] = {
    {  0,   "NULL" },
    {  1,   "INFO" },
    {  2,   "PR_INIT" },
    {  3,   "PR_START" },
    {  4,   "PR_LIST" },
    {  5,   "PR_QUEUE" },
    {  6,   "PR_STATUS" },
    {  7,   "PR_CANCEL" },
    {  8,   "PR_ADMIN" },
    {  9,   "PR_REQUEUE" },
    { 10,   "PR_HOLD" },
    { 11,   "PR_RELEASE" },
    { 12,   "MAPID" },
    { 13,   "AUTH" },
    { 14,   "ALERT" },
    { 0,    NULL }
};
/* end of PCNFS version 2 */

static const rpc_prog_vers_info pcnfsd_vers_info[] = {
	{ 1, pcnfsd1_proc, &hf_pcnfsd_procedure_v1 },
	{ 2, pcnfsd2_proc, &hf_pcnfsd_procedure_v2 },
};

void
proto_register_pcnfsd(void)
{
    static hf_register_info hf[] = {
        { &hf_pcnfsd_procedure_v1, {
                "V1 Procedure", "pcnfsd.procedure_v1", FT_UINT32, BASE_DEC,
                VALS(pcnfsd1_proc_vals), 0, NULL, HFILL }},
        { &hf_pcnfsd_procedure_v2, {
                "V2 Procedure", "pcnfsd.procedure_v2", FT_UINT32, BASE_DEC,
                VALS(pcnfsd2_proc_vals), 0, NULL, HFILL }},
        { &hf_pcnfsd_auth_client, {
                "Authentication Client", "pcnfsd.auth.client", FT_STRING, BASE_NONE,
                NULL, 0, NULL, HFILL }},
        { &hf_pcnfsd_auth_ident_obscure, {
                "Obscure Ident", "pcnfsd.auth.ident.obscure", FT_STRING, BASE_NONE,
                NULL, 0, "Authentication Obscure Ident", HFILL }},
        { &hf_pcnfsd_auth_ident_clear, {
                "Clear Ident", "pcnfsd.auth.ident.clear", FT_STRING, BASE_NONE,
                NULL, 0, "Authentication Clear Ident", HFILL }},
        { &hf_pcnfsd_auth_password_obscure, {
                "Obscure Password", "pcnfsd.auth.password.obscure", FT_STRING, BASE_NONE,
                NULL, 0, "Authentication Obscure Password", HFILL }},
        { &hf_pcnfsd_auth_password_clear, {
                "Clear Password", "pcnfsd.auth.password.clear", FT_STRING, BASE_NONE,
                NULL, 0, "Authentication Clear Password", HFILL }},
        { &hf_pcnfsd_comment, {
                "Comment", "pcnfsd.comment", FT_STRING, BASE_NONE,
                NULL, 0, NULL, HFILL }},
        { &hf_pcnfsd_status, {
                "Reply Status", "pcnfsd.status", FT_UINT32, BASE_DEC,
                NULL, 0, "Status", HFILL }},
        { &hf_pcnfsd_uid, {
                "User ID", "pcnfsd.uid", FT_UINT32, BASE_DEC,
                NULL, 0, NULL, HFILL }},
        { &hf_pcnfsd_gid, {
                "Group ID", "pcnfsd.gid", FT_UINT32, BASE_DEC,
                NULL, 0, NULL, HFILL }},
        { &hf_pcnfsd_gids_count, {
                "Group ID Count", "pcnfsd.gids.count", FT_UINT32, BASE_DEC,
                NULL, 0, NULL, HFILL }},
        { &hf_pcnfsd_homedir, {
                "Home Directory", "pcnfsd.homedir", FT_STRING, BASE_NONE,
                NULL, 0, NULL, HFILL }},
        { &hf_pcnfsd_def_umask, {
                "def_umask", "pcnfsd.def_umask", FT_UINT32, BASE_OCT,
                NULL, 0, NULL, HFILL }},
        { &hf_pcnfsd_mapreq, {
                "Request", "pcnfsd.mapreq", FT_UINT32, BASE_DEC,
                VALS(names_mapreq), 0, NULL, HFILL }},
        { &hf_pcnfsd_mapreq_status, {
                "Status", "pcnfsd.mapreq_status", FT_UINT32, BASE_DEC,
                VALS(names_maprstat), 0, NULL, HFILL }},
        { &hf_pcnfsd_username, {
                "User name", "pcnfsd.username", FT_STRING, BASE_NONE,
                NULL, 0, "pcnfsd.username", HFILL }},
    };

    static gint *ett[] = {
        &ett_pcnfsd,
        &ett_pcnfsd_auth_ident,
        &ett_pcnfsd_auth_password,
        &ett_pcnfsd_gids
    };

    proto_pcnfsd = proto_register_protocol("PC NFS",
                                           "PCNFSD", "pcnfsd");
    proto_register_field_array(proto_pcnfsd, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
}

void
proto_reg_handoff_pcnfsd(void)
{
    /* Register the protocol as RPC */
    rpc_init_prog(proto_pcnfsd, PCNFSD_PROGRAM, ett_pcnfsd,
                  G_N_ELEMENTS(pcnfsd_vers_info), pcnfsd_vers_info);
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
