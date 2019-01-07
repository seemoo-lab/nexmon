/* packet-fcswils.c
 * Routines for FC Inter-switch link services
 * Copyright 2001, Dinesh G Dutt <ddutt@cisco.com>
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
#include <epan/to_str.h>
#include "packet-fc.h"
#include "packet-fcswils.h"
#include "packet-fcct.h"

void proto_register_fcswils(void);
void proto_reg_handoff_fcswils(void);

/*
 * See the FC-SW specifications.
 */

#define FC_SWILS_RPLY               0x0
#define FC_SWILS_REQ                0x1
#define FC_SWILS_RSCN_DEVENTRY_SIZE 20

/* Zone name has the structure:
 * name_len (1 byte), rsvd (3 bytes), name (m bytes), fill (n bytes)
 * name_len excludes the 4 initial bytes before the name
 */
#define ZONENAME_LEN(x, y)  (tvb_get_guint8(x, y)+4)

/* Initialize the protocol and registered fields */
static int proto_fcswils                  = -1;
static int hf_swils_opcode                = -1;
static int hf_swils_elp_rev               = -1;
static int hf_swils_elp_flags             = -1;
static int hf_swils_elp_r_a_tov           = -1;
static int hf_swils_elp_e_d_tov           = -1;
static int hf_swils_elp_req_epn           = -1;
static int hf_swils_elp_req_esn           = -1;
static int hf_swils_elp_clsf_svcp         = -1;
static int hf_swils_elp_clsf_rcvsz        = -1;
static int hf_swils_elp_clsf_conseq       = -1;
static int hf_swils_elp_clsf_e2e          = -1;
static int hf_swils_elp_clsf_openseq      = -1;
static int hf_swils_elp_cls1_svcp         = -1;
static int hf_swils_elp_cls1_rcvsz        = -1;
static int hf_swils_elp_cls2_svcp         = -1;
static int hf_swils_elp_cls2_rcvsz        = -1;
static int hf_swils_elp_cls3_svcp         = -1;
static int hf_swils_elp_cls3_rcvsz        = -1;
static int hf_swils_elp_isl_fc_mode       = -1;
static int hf_swils_elp_fcplen            = -1;
static int hf_swils_elp_b2bcredit         = -1;
static int hf_swils_elp_compat1           = -1;
static int hf_swils_elp_compat2           = -1;
static int hf_swils_elp_compat3           = -1;
static int hf_swils_elp_compat4           = -1;
static int hf_swils_efp_rec_type          = -1;
static int hf_swils_efp_dom_id            = -1;
static int hf_swils_efp_switch_name       = -1;
static int hf_swils_efp_mcast_grpno       = -1;
/* static int hf_swils_efp_alias_token       = -1; */
static int hf_swils_efp_record_len        = -1;
static int hf_swils_efp_payload_len       = -1;
static int hf_swils_efp_pswitch_pri       = -1;
static int hf_swils_efp_pswitch_name      = -1;
static int hf_swils_dia_switch_name       = -1;
static int hf_swils_rdi_payload_len       = -1;
static int hf_swils_rdi_req_sname         = -1;
/* static int hf_swils_fspfh_cmd             = -1; */
static int hf_swils_fspfh_rev             = -1;
static int hf_swils_fspfh_ar_num          = -1;
static int hf_swils_fspfh_auth_type       = -1;
static int hf_swils_fspfh_dom_id          = -1;
static int hf_swils_fspfh_auth            = -1;
static int hf_swils_hlo_options           = -1;
static int hf_swils_hlo_hloint            = -1;
static int hf_swils_hlo_deadint           = -1;
static int hf_swils_hlo_rcv_domid         = -1;
static int hf_swils_hlo_orig_pidx         = -1;
static int hf_swils_ldrec_linkid          = -1;
static int hf_swils_ldrec_out_pidx        = -1;
static int hf_swils_ldrec_nbr_pidx        = -1;
static int hf_swils_ldrec_link_type       = -1;
static int hf_swils_ldrec_link_cost       = -1;
static int hf_swils_lsrh_lsr_type         = -1;
static int hf_swils_lsrh_lsid             = -1;
static int hf_swils_lsrh_adv_domid        = -1;
static int hf_swils_lsrh_ls_incid         = -1;
static int hf_swils_esc_pdesc_vendorid    = -1;
static int hf_swils_esc_swvendorid        = -1;
static int hf_swils_esc_protocolid        = -1;
static int hf_swils_rscn_evtype           = -1;
static int hf_swils_rscn_addrfmt          = -1;
static int hf_swils_rscn_detectfn         = -1;
static int hf_swils_rscn_affectedport     = -1;
static int hf_swils_rscn_portstate        = -1;
static int hf_swils_rscn_portid           = -1;
static int hf_swils_rscn_pwwn             = -1;
static int hf_swils_rscn_nwwn             = -1;
static int hf_swils_zone_activezonenm     = -1;
static int hf_swils_zone_objname          = -1;
static int hf_swils_zone_objtype          = -1;
static int hf_swils_zone_mbrtype          = -1;
static int hf_swils_zone_protocol         = -1;
static int hf_swils_zone_mbrid            = -1;
static int hf_swils_zone_mbrid_fcwwn      = -1;
static int hf_swils_zone_mbrid_fc         = -1;
static int hf_swils_zone_mbrid_uint       = -1;
static int hf_swils_zone_status           = -1;
static int hf_swils_zone_reason           = -1;
static int hf_swils_aca_domainid          = -1;
static int hf_swils_sfc_opcode            = -1;
static int hf_swils_sfc_zonenm            = -1;
static int hf_swils_rjt                   = -1;
static int hf_swils_rjtdet                = -1;
static int hf_swils_rjtvendor             = -1;
static int hf_swils_zone_mbrid_lun        = -1;
static int hf_swils_ess_rev               = -1;
static int hf_swils_ess_len               = -1;
static int hf_swils_ess_numobj            = -1;
static int hf_swils_interconnect_list_len = -1;
static int hf_swils_ess_vendorname        = -1;
static int hf_swils_ess_modelname         = -1;
static int hf_swils_ess_relcode           = -1;
static int hf_swils_ess_vendorspecific    = -1;
static int hf_swils_ess_cap_type          = -1;
static int hf_swils_ess_cap_subtype       = -1;
static int hf_swils_ess_cap_numentries    = -1;
static int hf_swils_ess_cap_svc           = -1;
static int hf_swils_ess_dns_obj0h         = -1;
static int hf_swils_ess_dns_obj1h         = -1;
static int hf_swils_ess_dns_obj2h         = -1;
static int hf_swils_ess_dns_obj3h         = -1;
static int hf_swils_ess_dns_zlacc         = -1;
static int hf_swils_ess_dns_vendor        = -1;
static int hf_swils_ess_fctlr_rscn        = -1;
static int hf_swils_ess_fctlr_vendor      = -1;
static int hf_swils_ess_fcs_basic         = -1;
static int hf_swils_ess_fcs_platform      = -1;
static int hf_swils_ess_fcs_topology      = -1;
static int hf_swils_ess_fcs_enhanced      = -1;
static int hf_swils_ess_fzs_enh_supp      = -1;
static int hf_swils_ess_fzs_enh_ena       = -1;
static int hf_swils_ess_fzs_mr            = -1;
static int hf_swils_ess_fzs_zsdb_supp     = -1;
static int hf_swils_ess_fzs_zsdb_ena      = -1;
static int hf_swils_ess_fzs_adc_supp      = -1;
static int hf_swils_ess_fzs_hardzone      = -1;
static int hf_swils_mrra_rev              = -1;
static int hf_swils_mrra_size             = -1;
static int hf_swils_mrra_vendorid         = -1;
static int hf_swils_mrra_reply            = -1;
static int hf_swils_mrra_reply_size       = -1;
static int hf_swils_mrra_waittime         = -1;
static int hf_swils_ess_cap_t10           = -1;
static int hf_swils_ess_cap_vendorobj     = -1;
static int hf_swils_ess_fzs_defzone       = -1;
static int hf_swils_ess_cap_len           = -1;
static int hf_swils_mrra_vendorinfo       = -1;

/* Generated from convert_proto_tree_add_text.pl */
static int hf_swils_lsrh_lsr_age = -1;
static int hf_swils_zone_full_zone_set_length = -1;
static int hf_swils_zone_num_zoning_objects = -1;
static int hf_swils_lsrec_number_of_links = -1;
static int hf_swils_sfc_zoneset_length = -1;
static int hf_swils_zone_vendor_unique = -1;
static int hf_swils_zone_num_members = -1;
static int hf_swils_zone_active_zoneset_length = -1;
static int hf_swils_lsack_num_of_lsr_headers = -1;
static int hf_swils_lsrh_lsr_length = -1;
static int hf_swils_esc_payload_length = -1;
static int hf_swils_lsupdate_num_of_lsrs = -1;
static int hf_swils_zone_mbr_identifier_length = -1;
static int hf_swils_zone_mbrflags = -1;
static int hf_swils_lsrh_options = -1;
static int hf_swils_domain_id_list_length = -1;
static int hf_swils_lsack_flags = -1;
static int hf_swils_rscn_num_entries = -1;
static int hf_swils_requested_domain_id = -1;
static int hf_swils_lsrh_checksum = -1;
static int hf_swils_granted_domain_id = -1;
static int hf_swils_lsupdate_flags = -1;


static expert_field ei_swils_efp_record_len = EI_INIT;
static expert_field ei_swils_no_exchange = EI_INIT;
static expert_field ei_swils_zone_mbrid = EI_INIT;

/* Initialize the subtree pointers */
static gint ett_fcswils             = -1;
static gint ett_fcswils_swacc       = -1;
static gint ett_fcswils_swrjt       = -1;
static gint ett_fcswils_elp         = -1;
static gint ett_fcswils_efp         = -1;
static gint ett_fcswils_efplist     = -1;
static gint ett_fcswils_dia         = -1;
static gint ett_fcswils_rdi         = -1;
static gint ett_fcswils_fspfhdr     = -1;
static gint ett_fcswils_hlo         = -1;
static gint ett_fcswils_lsrec       = -1;
static gint ett_fcswils_lsrechdr    = -1;
static gint ett_fcswils_ldrec       = -1;
static gint ett_fcswils_lsu         = -1;
static gint ett_fcswils_lsa         = -1;
static gint ett_fcswils_bf          = -1;
static gint ett_fcswils_rcf         = -1;
static gint ett_fcswils_rscn        = -1;
static gint ett_fcswils_rscn_dev    = -1;
static gint ett_fcswils_drlir       = -1;
static gint ett_fcswils_mr          = -1;
static gint ett_fcswils_zoneobjlist = -1;
static gint ett_fcswils_zoneobj     = -1;
static gint ett_fcswils_zonembr     = -1;
static gint ett_fcswils_aca         = -1;
static gint ett_fcswils_rca         = -1;
static gint ett_fcswils_sfc         = -1;
static gint ett_fcswils_ufc         = -1;
static gint ett_fcswils_esc         = -1;
static gint ett_fcswils_esc_pdesc   = -1;
static gint ett_fcswils_ieinfo      = -1;
static gint ett_fcswils_capinfo     = -1;

static const value_string fc_swils_opcode_key_val[] = {
    {FC_SWILS_SWRJT   , "SW_RJT"},
    {FC_SWILS_SWACC   , "SW_ACC"},
    {FC_SWILS_ELP     , "ELP"},
    {FC_SWILS_EFP     , "EFP"},
    {FC_SWILS_DIA     , "DIA"},
    {FC_SWILS_RDI     , "RDI"},
    {FC_SWILS_HLO     , "HLO"},
    {FC_SWILS_LSU     , "LSU"},
    {FC_SWILS_LSA     , "LSA"},
    {FC_SWILS_BF      , "BF"},
    {FC_SWILS_RCF     , "RCF"},
    {FC_SWILS_RSCN    , "SW_RSCN"},
    {FC_SWILS_DRLIR   , "DRLIR"},
    {FC_SWILS_DSCN    , "DSCN"},
    {FC_SWILS_LOOPD   , "LOOPD"},
    {FC_SWILS_MR      , "MR"},
    {FC_SWILS_ACA     , "ACA"},
    {FC_SWILS_RCA     , "RCA"},
    {FC_SWILS_SFC     , "SFC"},
    {FC_SWILS_UFC     , "UFC"},
    {FC_SWILS_ESC     , "ESC"},
    {FC_SWILS_ESS     , "ESS"},
    {FC_SWILS_MRRA    , "MRRA"},
    {FC_SWILS_AUTH_ILS, "AUTH_ILS"},
    {0, NULL},
};

static const value_string fc_swils_rjt_val [] = {
    {FC_SWILS_RJT_INVCODE   , "Invalid Cmd Code"},
    {FC_SWILS_RJT_INVVER    , "Invalid Revision"},
    {FC_SWILS_RJT_LOGERR    , "Logical Error"},
    {FC_SWILS_RJT_INVSIZE   , "Invalid Size"},
    {FC_SWILS_RJT_LOGBSY    , "Logical Busy"},
    {FC_SWILS_RJT_PROTERR   , "Protocol Error"},
    {FC_SWILS_RJT_GENFAIL   , "Unable to Perform"},
    {FC_SWILS_RJT_CMDNOTSUPP, "Unsupported Cmd"},
    {FC_SWILS_RJT_VENDUNIQ  , "Vendor Unique Err"},
    {0, NULL},
};

static const value_string fc_swils_deterr_val [] = {
    {FC_SWILS_RJT_NODET     , "No Additional Details"},
    {FC_SWILS_RJT_CLSF_ERR  , "Class F Svc Param Err"},
    {FC_SWILS_RJT_CLSN_ERR  , "Class N Svc Param Err"},
    {FC_SWILS_RJT_INVFC_CODE, "Unknown Flow Ctrl Code"},
    {FC_SWILS_RJT_INVFC_PARM, "Invalid Flow Ctrl Parm"},
    {FC_SWILS_RJT_INV_PNAME , "Invalid Port Name"},
    {FC_SWILS_RJT_INV_SNAME , "Invalid Switch Name"},
    {FC_SWILS_RJT_TOV_MSMTCH, "R_A_/E_D_TOV Mismatch"},
    {FC_SWILS_RJT_INV_DIDLST, "Invalid Domain ID List"},
    {FC_SWILS_RJT_CMD_INPROG, "Cmd Already in Progress"},
    {FC_SWILS_RJT_OORSRC    , "Insufficient Resources"},
    {FC_SWILS_RJT_NO_DID    , "Domain ID Unavailable"},
    {FC_SWILS_RJT_INV_DID   , "Invalid Domain ID"},
    {FC_SWILS_RJT_NO_REQ    , "Request Not Supported"},
    {FC_SWILS_RJT_NOLNK_PARM, "Link Parm Not Estd."},
    {FC_SWILS_RJT_NO_REQDID , "Group of Domain IDs Unavail"},
    {FC_SWILS_RJT_EP_ISOL   , "E_Port Isolated"},
    {0, NULL}
};

static const value_string fcswils_elp_fc_val[] = {
    {FC_SWILS_ELP_FC_VENDOR, "Vendor Unique"},
    {FC_SWILS_ELP_FC_RRDY  , "R_RDY Flow Ctrl"},
    {0, NULL},
};

static const value_string fcswils_rectype_val[] = {
    {FC_SWILS_LRECTYPE_DOMAIN, "Domain ID Record"},
    {FC_SWILS_LRECTYPE_MCAST , "Multicast ID Record"},
    {0, NULL},
};

static const value_string fc_swils_link_type_val[] = {
    {0x01, "P2P Link"},
    {0xF0, "Vendor Specific"},
    {0xF1, "Vendor Specific"},
    {0xF2, "Vendor Specific"},
    {0xF3, "Vendor Specific"},
    {0xF4, "Vendor Specific"},
    {0xF5, "Vendor Specific"},
    {0xF6, "Vendor Specific"},
    {0xF7, "Vendor Specific"},
    {0xF8, "Vendor Specific"},
    {0xF9, "Vendor Specific"},
    {0xFA, "Vendor Specific"},
    {0xFB, "Vendor Specific"},
    {0xFC, "Vendor Specific"},
    {0xFD, "Vendor Specific"},
    {0xFE, "Vendor Specific"},
    {0xFF, "Vendor Specific"},
    {0, NULL},
};

static const value_string fc_swils_fspf_linkrec_val[] = {
    {FC_SWILS_LSR_SLR, "Switch Link Record"},
    {FC_SWILS_LSR_ARS, "AR Summary Record"},
    {0, NULL},
};

static const value_string fc_swils_fspf_lsrflags_val[] = {
    {0x0, "LSR is for a Topology Update"},
    {0x1, "LSR is for Initial DB Sync | Not the last seq in DB sync"},
    {0x2, "Last Seq in DB Sync. LSU has no LSRs"},
    {0x3, "LSR is for Initial DB Sync | Last Seq in DB Sync"},
    {0, NULL},
};

static const value_string fc_swils_rscn_portstate_val[] = {
    {0, "No Additional Info"},
    {1, "Port is online"},
    {2, "Port is offline"},
    {0, NULL},
};

static const value_string fc_swils_rscn_addrfmt_val[] = {
    {0, "Port Addr Format"},
    {1, "Area Addr Format"},
    {2, "Domain Addr Format"},
    {3, "Fabric Addr Format"},
    {0, NULL},
};

static const value_string fc_swils_rscn_detectfn_val[] = {
    {1, "Fabric Detected"},
    {2, "N_Port Detected"},
    {0, NULL},
};

static const value_string fc_swils_esc_protocol_val[] = {
    {0, "Reserved"},
    {1, "FSPF-Backbone Protocol"},
    {2, "FSPF Protocol"},
    {0, NULL},
};

static const value_string fc_swils_zoneobj_type_val[] = {
    {0, "Reserved"},
    {FC_SWILS_ZONEOBJ_ZONESET  , "Zone Set"},
    {FC_SWILS_ZONEOBJ_ZONE     , "Zone"},
    {FC_SWILS_ZONEOBJ_ZONEALIAS, "Zone Alias"},
    {0, NULL},
};

const value_string fc_swils_zonembr_type_val[] = {
    {0                        , "Reserved"},
    {FC_SWILS_ZONEMBR_WWN     , "WWN"},
    {FC_SWILS_ZONEMBR_DP      , "Domain/Physical Port (0x00ddpppp)"},
    {FC_SWILS_ZONEMBR_FCID    , "FC Address"},
    {FC_SWILS_ZONEMBR_ALIAS   , "Zone Alias"},
    {FC_SWILS_ZONEMBR_WWN_LUN , "WWN+LUN"},
    {FC_SWILS_ZONEMBR_DP_LUN  , "Domain/Physical Port+LUN"},
    {FC_SWILS_ZONEMBR_FCID_LUN, "FCID+LUN"},
    {0, NULL},
};

static const value_string fc_swils_mr_rsp_val[] = {
    {0, "Successful"},
    {1, "Fabric Busy"},
    {2, "Failed"},
    {0, NULL},
};

static const value_string fc_swils_mr_reason_val[] = {
    {0x0, "No Reason"},
    {0x1, "Invalid Data Length"},
    {0x2, "Unsupported Command"},
    {0x3, "Reserved"},
    {0x4, "Not Authorized"},
    {0x5, "Invalid Request"},
    {0x6, "Fabric Changing"},
    {0x7, "Update Not Staged"},
    {0x8, "Invalid Zone Set Format"},
    {0x9, "Invalid Data"},
    {0xA, "Cannot Merge"},
    {0, NULL},
};

static const value_string fc_swils_sfc_op_val[] = {
    {0, "Reserved"},
    {1, "Reserved"},
    {2, "Reserved"},
    {3, "Activate Zone Set"},
    {4, "Deactivate Zone Set"},
    {0, NULL},
};

typedef struct _zonename {
    guint32  namelen:8,
        rsvd:24;
    gchar   *name;
    gchar   *pad;
} zonename_t;

typedef struct _fcswils_conv_key {
    guint32 conv_idx;
} fcswils_conv_key_t;

typedef struct _fcswils_conv_data {
    guint32 opcode;
} fcswils_conv_data_t;

static GHashTable *fcswils_req_hash = NULL;

/* list of commands for each commandset */
typedef void (*fcswils_dissector_t)(tvbuff_t *tvb, packet_info* pinfo, proto_tree *tree, guint8 isreq);

typedef struct _fcswils_func_table_t {
    fcswils_dissector_t func;
} fcswils_func_table_t;

static dissector_handle_t fcsp_handle;

static gint get_zoneobj_len(tvbuff_t *tvb, gint offset);

/*
 * Hash Functions
 */
static gint
fcswils_equal(gconstpointer v, gconstpointer w)
{
    const fcswils_conv_key_t *v1 = (const fcswils_conv_key_t *)v;
    const fcswils_conv_key_t *v2 = (const fcswils_conv_key_t *)w;

    return (v1->conv_idx == v2->conv_idx);
}

static guint
fcswils_hash(gconstpointer v)
{
    const fcswils_conv_key_t *key = (const fcswils_conv_key_t *)v;
    guint val;

    val = key->conv_idx;

    return val;
}

/*
 * Protocol initialization
 */
static void
fcswils_init_protocol(void)
{
    fcswils_req_hash = g_hash_table_new(fcswils_hash, fcswils_equal);
}

static void
fcswils_cleanup_protocol(void)
{
    g_hash_table_destroy(fcswils_req_hash);
}

static guint8 *
zonenm_to_str(tvbuff_t *tvb, gint offset)
{
    int len = tvb_get_guint8(tvb, offset);
    return tvb_get_string_enc(wmem_packet_scope(), tvb, offset+4, len, ENC_ASCII);
}

/* Offset points to the start of the zone object */
static gint
get_zoneobj_len(tvbuff_t *tvb, gint offset)
{
    gint   numrec, numrec1;
    guint8 objtype;
    gint   i, j, len;

    /* zone object structure is:
     * type (1 byte), protocol (1 byte), rsvd (2 bytes), obj name (x bytes),
     * num of zone mbrs (4 bytes ), list of zone members (each member if of
     * variable length).
     *
     * zone member structure is:
     * type (1 byte), rsvd (1 byte), flags (1 byte), id_len (1 byte),
     * id (id_len bytes)
     */
    objtype = tvb_get_guint8(tvb, offset);
    len = 4 + ZONENAME_LEN(tvb, offset+4); /* length upto num_of_mbrs field */
    numrec = tvb_get_ntohl(tvb, offset+len); /* gets us num of zone mbrs */

    len += 4;                   /* + num_mbrs */
    for (i = 0; i < numrec; i++) {
        if (objtype == FC_SWILS_ZONEOBJ_ZONESET) {
            len += 4 + ZONENAME_LEN(tvb, offset+4+len); /* length upto num_of_mbrs field */
            numrec1 = tvb_get_ntohl(tvb, offset+len);

            len += 4;
            for (j = 0; j < numrec1; j++) {
                len += 4 + tvb_get_guint8(tvb, offset+3+len);
            }
        }
        else {
            len += 4 + tvb_get_guint8(tvb, offset+3+len);
        }
    }

    return len;
}

#define MAX_INTERCONNECT_ELEMENT_INFO_LEN  252
static int
dissect_swils_interconnect_element_info(tvbuff_t *tvb, proto_tree *tree, int offset)
{

    int len, max_len = MAX_INTERCONNECT_ELEMENT_INFO_LEN;

    if (tree) {
        proto_tree_add_item(tree, hf_swils_interconnect_list_len, tvb, offset+3, 1, ENC_BIG_ENDIAN);
        len = tvb_strsize(tvb, offset+4);
        proto_tree_add_item(tree, hf_swils_ess_vendorname, tvb, offset+4, len, ENC_ASCII|ENC_NA);
        offset += (4 + len);
        max_len -= len;
        len = tvb_strsize(tvb, offset);
        proto_tree_add_item(tree, hf_swils_ess_modelname, tvb, offset, len, ENC_ASCII|ENC_NA);
        offset += len;
        max_len -= len;
        len = tvb_strsize(tvb, offset);
        proto_tree_add_item(tree, hf_swils_ess_relcode, tvb, offset, len, ENC_ASCII|ENC_NA);
        offset += len;
        max_len -= len;
        while (max_len > 0) {
            /* Vendor specific field is a set of one or more null-terminated
             * strings
             */
            len = tvb_strsize(tvb, offset);
            proto_tree_add_item(tree, hf_swils_ess_vendorspecific, tvb, offset, len, ENC_ASCII|ENC_NA);
            offset += len;
            max_len -= len;
        }
    }

    return TRUE;
}

static void
dissect_swils_ess_capability(tvbuff_t *tvb, proto_tree *tree, int offset,
                             guint8 srvr_type)
{
    if (tree) {
        switch (srvr_type) {
        case FCCT_GSRVR_DNS:
            proto_tree_add_item(tree, hf_swils_ess_dns_zlacc, tvb, offset+3,
                                1, ENC_BIG_ENDIAN);
            proto_tree_add_item(tree, hf_swils_ess_dns_obj3h, tvb, offset+3,
                                1, ENC_BIG_ENDIAN);
            proto_tree_add_item(tree, hf_swils_ess_dns_obj2h, tvb, offset+3,
                                1, ENC_BIG_ENDIAN);
            proto_tree_add_item(tree, hf_swils_ess_dns_obj1h, tvb, offset+3,
                                1, ENC_BIG_ENDIAN);
            proto_tree_add_item(tree, hf_swils_ess_dns_obj0h, tvb, offset+3,
                                1, ENC_BIG_ENDIAN);
            proto_tree_add_item(tree, hf_swils_ess_dns_vendor, tvb,
                                offset+4, 4, ENC_BIG_ENDIAN);
            break;
        case FCCT_GSRVR_FCTLR:
            proto_tree_add_item(tree, hf_swils_ess_fctlr_rscn, tvb,
                                offset+3, 1, ENC_BIG_ENDIAN);
            proto_tree_add_item(tree, hf_swils_ess_fctlr_vendor, tvb,
                                offset+4, 4, ENC_BIG_ENDIAN);
            break;
        case FCCT_GSRVR_FCS:
            proto_tree_add_item(tree, hf_swils_ess_fcs_basic, tvb,
                                offset+3, 1, ENC_BIG_ENDIAN);
            proto_tree_add_item(tree, hf_swils_ess_fcs_platform, tvb,
                                offset+3, 1, ENC_BIG_ENDIAN);
            proto_tree_add_item(tree, hf_swils_ess_fcs_topology, tvb,
                                offset+3, 1, ENC_BIG_ENDIAN);
            proto_tree_add_item(tree, hf_swils_ess_fcs_enhanced, tvb,
                                offset+3, 1, ENC_BIG_ENDIAN);
            break;
        case FCCT_GSRVR_FZS:
            proto_tree_add_item(tree, hf_swils_ess_fzs_enh_supp, tvb,
                                offset+3, 1, ENC_BIG_ENDIAN);
            proto_tree_add_item(tree, hf_swils_ess_fzs_enh_ena, tvb,
                                offset+3, 1, ENC_BIG_ENDIAN);
            proto_tree_add_item(tree, hf_swils_ess_fzs_mr, tvb, offset+3,
                                1, ENC_BIG_ENDIAN);
            proto_tree_add_item(tree, hf_swils_ess_fzs_defzone, tvb,
                                offset+3, 1, ENC_BIG_ENDIAN);
            proto_tree_add_item(tree, hf_swils_ess_fzs_zsdb_supp, tvb,
                                offset+3, 1, ENC_BIG_ENDIAN);
            proto_tree_add_item(tree, hf_swils_ess_fzs_zsdb_ena, tvb,
                                offset+3, 1, ENC_BIG_ENDIAN);
            proto_tree_add_item(tree, hf_swils_ess_fzs_adc_supp, tvb,
                                offset+3, 1, ENC_BIG_ENDIAN);
            proto_tree_add_item(tree, hf_swils_ess_fzs_hardzone, tvb,
                                offset+3, 1, ENC_BIG_ENDIAN);
            break;
        default:
            break;
        }
    }

    return;
}

static int
dissect_swils_ess_capability_obj(tvbuff_t *tvb, proto_tree *tree, int offset)
{
    int         i = 0, num_entries = 0, len = 0, total_len = 0;
    guint8      type, subtype, srvr_type;
    proto_tree *capinfo_tree = NULL;

    if (tree) {
        /*
         * Structure of capability object is: WK type (2B), WK subtype(2),
         * rsvd (1), num_cap_entries (1), entry_1 (8) ... entry_n (8)
         */
        /* Compute length first to create subtree of cap object */
        type = tvb_get_guint8(tvb, offset);
        if (type != FCCT_GSTYPE_VENDOR) {
            num_entries = tvb_get_guint8(tvb, offset+3);
            total_len = 4 + (num_entries*8);
            capinfo_tree = proto_tree_add_subtree_format(tree, tvb, offset,
                                     total_len, ett_fcswils_capinfo, NULL, "Capability Object (%s)",
                                     val_to_str(type, fc_ct_gstype_vals,
                                                "Unknown (0x%x)"));
        } else {
            i = tvb_get_guint8(tvb, offset+3);
            i += 12;

            capinfo_tree = proto_tree_add_subtree_format(tree, tvb, offset,
                                     i, ett_fcswils_capinfo, NULL, "Capability Object (Vendor-specific 0x%x)",
                                     type);
        }

        proto_tree_add_item(capinfo_tree, hf_swils_ess_cap_type, tvb, offset, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item(capinfo_tree, hf_swils_ess_cap_subtype, tvb, offset+1,
                            1, ENC_BIG_ENDIAN);
        subtype = tvb_get_guint8(tvb, offset+1);

        if (type != FCCT_GSTYPE_VENDOR) {
            srvr_type = get_gs_server(type, subtype);
            proto_tree_add_uint(capinfo_tree, hf_swils_ess_cap_svc, tvb, offset, 2,
                                srvr_type);
            proto_tree_add_item(capinfo_tree, hf_swils_ess_cap_numentries, tvb,
                                offset+3, 1, ENC_BIG_ENDIAN);
            offset += 4;
            len += 4;

            while ((num_entries > 0) && tvb_bytes_exist(tvb, offset, 8)) {
                dissect_swils_ess_capability(tvb, capinfo_tree, offset, srvr_type);
                num_entries--;
                offset += 8;
                len += 8;
            }
        } else {
            /* Those damn T11 guys defined another format for
             * Vendor-specific objects.
             */
            proto_tree_add_item(capinfo_tree, hf_swils_ess_cap_len, tvb, offset+3,
                                1, ENC_BIG_ENDIAN);
            proto_tree_add_item(capinfo_tree, hf_swils_ess_cap_t10, tvb, offset+4,
                                8, ENC_ASCII|ENC_NA);
            i -= 8;          /* reduce length by t10 object size */
            offset += 12;
            len += 12;

            while ((i > 0) && tvb_bytes_exist(tvb, offset, 8)) {
                proto_tree_add_item(capinfo_tree, hf_swils_ess_cap_vendorobj,
                                    tvb, offset, 8, ENC_NA);
                i -= 8;
                offset += 8;
                len += 12;
            }
        }
    }
    return len;
}

static void
dissect_swils_nullpayload(tvbuff_t *tvb _U_, packet_info* pinfo _U_, proto_tree *tree _U_,
                          guint8 isreq _U_)
{
    /* Common dissector for those ILSs without a payload */
    return;
}

static void
dissect_swils_elp(tvbuff_t *tvb, packet_info* pinfo _U_, proto_tree *elp_tree, guint8 isreq _U_)
{

    /* Set up structures needed to add the protocol subtree and manage it */
    /* Response i.e. SW_ACC for an ELP has the same format as the request */
    /* We skip the initial 4 bytes as we don't care about the opcode */
    int          offset = 4;
    const gchar *flags;
    guint32 r_a_tov;
    guint32 e_d_tov;
    guint16 isl_flwctrl_mode;
    guint8  clsf_svcparm[6], cls1_svcparm[2], cls2_svcparm[2], cls3_svcparm[2];

    if (elp_tree) {
        offset += 4;
        proto_tree_add_item(elp_tree, hf_swils_elp_rev, tvb, offset++, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item(elp_tree, hf_swils_elp_flags, tvb, offset, 2, ENC_NA);
        offset += 3;
        r_a_tov = tvb_get_ntohl(tvb, offset);
        proto_tree_add_uint_format_value(elp_tree, hf_swils_elp_r_a_tov, tvb, offset, 4,
                                   r_a_tov, "%d msecs", r_a_tov);
        offset += 4;
        e_d_tov = tvb_get_ntohl(tvb, offset);
        proto_tree_add_uint_format_value(elp_tree, hf_swils_elp_e_d_tov, tvb, offset, 4,
                                   e_d_tov, "%d msecs", e_d_tov);
        offset += 4;
        proto_tree_add_item(elp_tree, hf_swils_elp_req_epn, tvb, offset, 8, ENC_NA);
        offset += 8;
        proto_tree_add_item(elp_tree, hf_swils_elp_req_esn, tvb, offset, 8, ENC_NA);
        offset += 8;

        tvb_memcpy(tvb, clsf_svcparm, offset, 6);
        if (clsf_svcparm[0] & 0x80) {
            if (clsf_svcparm[4] & 0x20) {
                flags="Class F Valid | X_ID Interlock";
            } else {
                flags="Class F Valid | No X_ID Interlk";
            }
        } else {
            flags="Class F Invld";
        }
        proto_tree_add_bytes_format_value(elp_tree, hf_swils_elp_clsf_svcp, tvb, offset, 6,
                                    clsf_svcparm, "(%s)", flags);
        offset += 6;

        proto_tree_add_item(elp_tree, hf_swils_elp_clsf_rcvsz, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;
        proto_tree_add_item(elp_tree, hf_swils_elp_clsf_conseq, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;
        proto_tree_add_item(elp_tree, hf_swils_elp_clsf_e2e, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;
        proto_tree_add_item(elp_tree, hf_swils_elp_clsf_openseq, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 4;

        tvb_memcpy(tvb, cls1_svcparm, offset, 2);
        if (cls1_svcparm[0] & 0x80) {
#define MAX_FLAGS_LEN 40
            char *flagsbuf;
            gint stroff, returned_length;

            flagsbuf=(char *)wmem_alloc(wmem_packet_scope(), MAX_FLAGS_LEN);
            stroff = 0;

            returned_length = g_snprintf(flagsbuf+stroff, MAX_FLAGS_LEN-stroff,
                                         "Class 1 Valid");
            stroff += MIN(returned_length, MAX_FLAGS_LEN-stroff);
            if (cls1_svcparm[0] & 0x40) {
                returned_length = g_snprintf(flagsbuf+stroff, MAX_FLAGS_LEN-stroff, " | IMX");
                stroff += MIN(returned_length, MAX_FLAGS_LEN-stroff);
            }
            if (cls1_svcparm[0] & 0x20) {
                returned_length = g_snprintf(flagsbuf+stroff, MAX_FLAGS_LEN-stroff, " | IPS");
                stroff += MIN(returned_length, MAX_FLAGS_LEN-stroff);
            }
            if (cls1_svcparm[0] & 0x10) {
                /*returned_length =*/ g_snprintf(flagsbuf+stroff, MAX_FLAGS_LEN-stroff, " | LKS");
            }
            flags=flagsbuf;
        }
        else {
            flags="Class 1 Invalid";
        }

        proto_tree_add_bytes_format_value(elp_tree, hf_swils_elp_cls1_svcp, tvb, offset, 2,
                                    NULL, "(%s)", flags);
        offset += 2;
        if (cls1_svcparm[0] & 0x80) {
            proto_tree_add_item(elp_tree, hf_swils_elp_cls1_rcvsz, tvb, offset, 2, ENC_BIG_ENDIAN);
        }
        offset += 2;

        tvb_memcpy(tvb, cls2_svcparm, offset, 2);
        if (cls2_svcparm[0] & 0x80) {
            if (cls2_svcparm[0] & 0x08) {
                flags="Class 2 Valid | Seq Delivery";
            }
            else {
                flags="Class 2 Valid | No Seq Delivery";
            }
        }
        else {
            flags="Class 2 Invld";
        }

        proto_tree_add_bytes_format_value(elp_tree, hf_swils_elp_cls2_svcp, tvb, offset, 2,
                                    cls2_svcparm,
                                    "(%s)", flags);
        offset += 2;

        if (cls2_svcparm[0] & 0x80) {
            proto_tree_add_item(elp_tree, hf_swils_elp_cls2_rcvsz, tvb, offset, 2, ENC_BIG_ENDIAN);
        }
        offset += 2;

        tvb_memcpy(tvb, cls3_svcparm, offset, 2);
        if (cls3_svcparm[0] & 0x80) {
            if (cls3_svcparm[0] & 0x08) {
                flags="Class 3 Valid | Seq Delivery";
            }
            else {
                flags="Class 3 Valid | No Seq Delivery";
            }
        }
        else {
            flags="Class 3 Invld";
        }
        proto_tree_add_bytes_format_value(elp_tree, hf_swils_elp_cls3_svcp, tvb, offset, 2,
                                    cls3_svcparm,
                                    "(%s)", flags);
        offset += 2;

        if (cls3_svcparm[0] & 0x80) {
            proto_tree_add_item(elp_tree, hf_swils_elp_cls3_rcvsz, tvb, offset, 2, ENC_BIG_ENDIAN);
        }
        offset += 22;

        isl_flwctrl_mode = tvb_get_ntohs(tvb, offset);
        proto_tree_add_string(elp_tree, hf_swils_elp_isl_fc_mode, tvb, offset, 2,
                              val_to_str_const(isl_flwctrl_mode, fcswils_elp_fc_val, "Vendor Unique"));
        offset += 2;
        proto_tree_add_item(elp_tree, hf_swils_elp_fcplen, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;
        proto_tree_add_item(elp_tree, hf_swils_elp_b2bcredit, tvb, offset, 4, ENC_BIG_ENDIAN);
        offset += 4;
        proto_tree_add_item(elp_tree, hf_swils_elp_compat1, tvb, offset, 4, ENC_BIG_ENDIAN);
        offset += 4;
        proto_tree_add_item(elp_tree, hf_swils_elp_compat2, tvb, offset, 4, ENC_BIG_ENDIAN);
        offset += 4;
        proto_tree_add_item(elp_tree, hf_swils_elp_compat3, tvb, offset, 4, ENC_BIG_ENDIAN);
        offset += 4;
        proto_tree_add_item(elp_tree, hf_swils_elp_compat4, tvb, offset, 4, ENC_BIG_ENDIAN);
    }

}

static void
dissect_swils_efp(tvbuff_t *tvb, packet_info* pinfo, proto_tree *efp_tree, guint8 isreq _U_)
{

/* Set up structures needed to add the protocol subtree and manage it */
    proto_tree  *lrec_tree;
    proto_item  *rec_item;
    int          num_listrec = 0;
    int          offset      = 1; /* Skip opcode */
    guint8       reclen;
    guint16      payload_len;
    guint8       rec_type;

    reclen = tvb_get_guint8(tvb, offset);
    rec_item = proto_tree_add_uint(efp_tree, hf_swils_efp_record_len, tvb, offset, 1, reclen);
    offset += 1;
    payload_len = tvb_get_ntohs(tvb, offset);
    if (payload_len < FC_SWILS_EFP_SIZE) {
        proto_tree_add_uint_format_value(efp_tree, hf_swils_efp_payload_len,
                                       tvb, offset, 2, payload_len,
                                       "%u (bogus, must be >= %u)",
                                       payload_len, FC_SWILS_EFP_SIZE);
        return;
    }
    proto_tree_add_item(efp_tree, hf_swils_efp_payload_len, tvb, offset, 2, ENC_BIG_ENDIAN);
    offset += 5;       /* skip 3 reserved bytes, too */
    proto_tree_add_item(efp_tree, hf_swils_efp_pswitch_pri, tvb,
                            offset, 1, ENC_BIG_ENDIAN);
    offset += 1;
    proto_tree_add_item(efp_tree, hf_swils_efp_pswitch_name, tvb, offset, 8, ENC_NA);
    offset += 8;

    if (reclen == 0) {
        expert_add_info(pinfo, rec_item, &ei_swils_efp_record_len);
        return;
    }
    /* Add List Records now */
    if (efp_tree) {
        num_listrec = (payload_len - FC_SWILS_EFP_SIZE)/reclen;
        while (num_listrec-- > 0) {
            rec_type = tvb_get_guint8(tvb, offset);
            lrec_tree = proto_tree_add_subtree(efp_tree, tvb, offset, -1,
                                        ett_fcswils_efplist, NULL,
                                        val_to_str(rec_type,
                                                   fcswils_rectype_val,
                                                   "Unknown record type (0x%02x)"));
            proto_tree_add_uint(lrec_tree, hf_swils_efp_rec_type, tvb, offset, 1,
                                rec_type);
            switch (rec_type) {

            case FC_SWILS_LRECTYPE_DOMAIN:
                proto_tree_add_item(lrec_tree, hf_swils_efp_dom_id, tvb, offset+1, 1, ENC_BIG_ENDIAN);
                proto_tree_add_item(lrec_tree, hf_swils_efp_switch_name, tvb, offset+8, 8, ENC_NA);
                break;

            case FC_SWILS_LRECTYPE_MCAST:
                proto_tree_add_item(lrec_tree, hf_swils_efp_mcast_grpno, tvb, offset+1, 1, ENC_BIG_ENDIAN);
                break;
            }
            offset += reclen;
        }
    }
}

static void
dissect_swils_dia(tvbuff_t *tvb, packet_info* pinfo _U_, proto_tree *dia_tree, guint8 isreq _U_)
{
    /* Set up structures needed to add the protocol subtree and manage it */
    int offset = 0;

    if (dia_tree) {
        proto_tree_add_item(dia_tree, hf_swils_dia_switch_name, tvb, offset+4,
                              8, ENC_NA);
    }
}

static void
dissect_swils_rdi(tvbuff_t *tvb, packet_info* pinfo _U_, proto_tree *rdi_tree, guint8 isreq)
{
    /* Set up structures needed to add the protocol subtree and manage it */
    int offset = 0;
    int i, plen, numrec;

    if (rdi_tree) {
        plen = tvb_get_ntohs(tvb, offset+2);

        proto_tree_add_item(rdi_tree, hf_swils_rdi_payload_len, tvb, offset+2, 2, ENC_BIG_ENDIAN);
        proto_tree_add_item(rdi_tree, hf_swils_rdi_req_sname, tvb, offset+4, 8, ENC_NA);

        /* 12 is the length of the initial header and 4 is the size of each
         * domain request record.
         */
        numrec = (plen - 12)/4;
        offset = 12;
        for (i = 0; i < numrec; i++) {
            if (isreq) {
                proto_tree_add_item(rdi_tree, hf_swils_requested_domain_id, tvb, offset+3, 1, ENC_BIG_ENDIAN);
            }
            else {
                proto_tree_add_item(rdi_tree, hf_swils_granted_domain_id, tvb, offset+3, 1, ENC_BIG_ENDIAN);
            }
            offset += 4;
        }
    }
}

static void
dissect_swils_fspf_hdr(tvbuff_t *tvb, proto_tree *tree, int offset)
{
    proto_tree *fspfh_tree;

    if (tree) {
        /* 20 is the size of FSPF header */
        fspfh_tree = proto_tree_add_subtree(tree, tvb, offset, 20, ett_fcswils_fspfhdr, NULL, "FSPF Header");

        proto_tree_add_item(fspfh_tree, hf_swils_fspfh_rev, tvb, offset+4,
                            1, ENC_BIG_ENDIAN);
        proto_tree_add_item(fspfh_tree, hf_swils_fspfh_ar_num, tvb,
                            offset+5, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item(fspfh_tree, hf_swils_fspfh_auth_type, tvb,
                            offset+6, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item(fspfh_tree, hf_swils_fspfh_dom_id, tvb, offset+11,
                            1, ENC_BIG_ENDIAN);
        proto_tree_add_item(fspfh_tree, hf_swils_fspfh_auth, tvb, offset+12,
                            8, ENC_NA);
    }
}

static void
dissect_swils_fspf_lsrechdr(tvbuff_t *tvb, proto_tree *tree, int offset)
{
    proto_tree_add_item(tree, hf_swils_lsrh_lsr_type, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_uint_format_value(tree, hf_swils_lsrh_lsr_age, tvb, offset+2, 2,
                        tvb_get_ntohs(tvb, offset+2), "%d secs", tvb_get_ntohs(tvb, offset+2));
    proto_tree_add_item(tree, hf_swils_lsrh_options, tvb, offset+4, 4, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_swils_lsrh_lsid, tvb, offset+11, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_swils_lsrh_adv_domid, tvb, offset+15, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_swils_lsrh_ls_incid, tvb, offset+16, 4, ENC_BIG_ENDIAN);
    proto_tree_add_checksum(tree, tvb, offset+20, hf_swils_lsrh_checksum, -1, NULL, NULL, 0, ENC_BIG_ENDIAN, PROTO_CHECKSUM_NO_FLAGS);
    proto_tree_add_item(tree, hf_swils_lsrh_lsr_length, tvb, offset+22, 2, ENC_BIG_ENDIAN);
}

static void
dissect_swils_fspf_ldrec(tvbuff_t *tvb, proto_tree *tree, int offset)
{
    proto_tree_add_item(tree, hf_swils_ldrec_linkid, tvb, offset+1, 3, ENC_NA);
    proto_tree_add_item(tree, hf_swils_ldrec_out_pidx, tvb, offset+5, 3, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_swils_ldrec_nbr_pidx, tvb, offset+9, 3, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_swils_ldrec_link_type, tvb, offset+12, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_swils_ldrec_link_cost, tvb, offset+14, 2, ENC_BIG_ENDIAN);
}

static void
dissect_swils_fspf_lsrec(tvbuff_t *tvb, proto_tree *tree, int offset,
                         int num_lsrec)
{
    int         i, j, num_ldrec;
    proto_tree *lsrec_tree, *ldrec_tree, *lsrechdr_tree;

    if (tree) {
        for (j = 0; j < num_lsrec; j++) {
            num_ldrec = tvb_get_ntohs(tvb, offset+26);
            lsrec_tree = proto_tree_add_subtree_format(tree, tvb, offset, (28+num_ldrec*16),
                                        ett_fcswils_lsrec, NULL, "Link State Record %d (Domain %d)", j,
                                        tvb_get_guint8(tvb, offset+15));

            lsrechdr_tree = proto_tree_add_subtree(lsrec_tree, tvb, offset, 24,
                                        ett_fcswils_lsrechdr, NULL, "Link State Record Header");

            dissect_swils_fspf_lsrechdr(tvb, lsrechdr_tree, offset);
            proto_tree_add_item(tree, hf_swils_lsrec_number_of_links, tvb, offset+26, 2, ENC_BIG_ENDIAN);
            offset += 28;

            for (i = 0; i < num_ldrec; i++) {
                ldrec_tree = proto_tree_add_subtree_format(lsrec_tree, tvb, offset, 16,
                                             ett_fcswils_ldrec, NULL, "Link Descriptor %d "
                                             "(Neighbor domain %d)", i,
                                             tvb_get_guint8(tvb, offset+3));
                dissect_swils_fspf_ldrec(tvb, ldrec_tree, offset);
                offset += 16;
            }
        }
    }
}

static void
dissect_swils_hello(tvbuff_t *tvb, packet_info* pinfo _U_, proto_tree *hlo_tree, guint8 isreq _U_)
{
    /* Set up structures needed to add the protocol subtree and manage it */
    int offset = 0;

    if (hlo_tree) {
        dissect_swils_fspf_hdr(tvb, hlo_tree, offset);

        proto_tree_add_item(hlo_tree, hf_swils_hlo_options, tvb, offset+20, 4, ENC_NA);
        proto_tree_add_item(hlo_tree, hf_swils_hlo_hloint, tvb, offset+24, 4, ENC_BIG_ENDIAN);
        proto_tree_add_item(hlo_tree, hf_swils_hlo_deadint, tvb, offset+28, 4, ENC_BIG_ENDIAN);
        proto_tree_add_item(hlo_tree, hf_swils_hlo_rcv_domid, tvb, offset+35, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item(hlo_tree, hf_swils_hlo_orig_pidx, tvb, offset+37, 3, ENC_BIG_ENDIAN);
    }
}

static void
dissect_swils_lsupdate(tvbuff_t *tvb, packet_info* pinfo _U_, proto_tree *lsu_tree, guint8 isreq _U_)
{
    /* Set up structures needed to add the protocol subtree and manage it */
    int offset = 0;
    int num_lsrec;

    if (lsu_tree) {
        dissect_swils_fspf_hdr(tvb, lsu_tree, offset);

        proto_tree_add_item(lsu_tree, hf_swils_lsupdate_flags, tvb, offset+23, 1, ENC_BIG_ENDIAN);
        num_lsrec = tvb_get_ntohl(tvb, offset+24);

        proto_tree_add_item(lsu_tree, hf_swils_lsupdate_num_of_lsrs, tvb, offset+24, 4, ENC_BIG_ENDIAN);

        offset = 28;
        dissect_swils_fspf_lsrec(tvb, lsu_tree, offset, num_lsrec);
    }
}

static void
dissect_swils_lsack(tvbuff_t *tvb, packet_info* pinfo _U_, proto_tree *lsa_tree, guint8 isreq _U_)
{
    /* Set up structures needed to add the protocol subtree and manage it */
    int         offset = 0;
    int         num_lsrechdr, i;
    proto_tree *lsrechdr_tree;

    if (lsa_tree) {
        dissect_swils_fspf_hdr(tvb, lsa_tree, offset);

        proto_tree_add_item(lsa_tree, hf_swils_lsack_flags, tvb, offset+23, 1, ENC_BIG_ENDIAN);
        num_lsrechdr = tvb_get_ntohl(tvb, offset+24);

        proto_tree_add_item(lsa_tree, hf_swils_lsack_num_of_lsr_headers, tvb, offset+24, 4, ENC_BIG_ENDIAN);

        offset = 28;

        for (i = 0; i < num_lsrechdr; i++) {
            lsrechdr_tree = proto_tree_add_subtree_format(lsa_tree, tvb, offset, 24,
                                        ett_fcswils_lsrechdr, NULL, "Link State Record Header (Domain %d)",
                                        tvb_get_guint8(tvb, offset+15));
            dissect_swils_fspf_lsrechdr(tvb, lsrechdr_tree, offset);
            offset += 24;
        }
    }
}

static void
dissect_swils_rscn(tvbuff_t *tvb, packet_info* pinfo _U_, proto_tree *rscn_tree, guint8 isreq)
{
    /* Set up structures needed to add the protocol subtree and manage it */
    int         offset = 0;
    proto_tree *dev_tree;
    int         numrec, i;

    if (rscn_tree) {
        if (!isreq)
            return;

        proto_tree_add_item(rscn_tree, hf_swils_rscn_evtype, tvb, offset+4,
                            1, ENC_BIG_ENDIAN);
        proto_tree_add_item(rscn_tree, hf_swils_rscn_addrfmt, tvb, offset+4,
                            1, ENC_BIG_ENDIAN);
        proto_tree_add_item(rscn_tree, hf_swils_rscn_affectedport, tvb,
                              offset+5, 3, ENC_NA);
        proto_tree_add_item(rscn_tree, hf_swils_rscn_detectfn, tvb,
                            offset+8, 4, ENC_BIG_ENDIAN);
        numrec = tvb_get_ntohl(tvb, offset+12);

        if (!tvb_bytes_exist(tvb, offset+16, FC_SWILS_RSCN_DEVENTRY_SIZE*numrec)) {
            /* Some older devices do not include device entry information. */
            return;
        }

        proto_tree_add_item(rscn_tree, hf_swils_rscn_num_entries, tvb, offset+12, 4, ENC_BIG_ENDIAN);

        offset = 16;
        for (i = 0; i < numrec; i++) {
            dev_tree = proto_tree_add_subtree_format(rscn_tree, tvb, offset, 20,
                                        ett_fcswils_rscn_dev, NULL, "Device Entry %d", i);

            proto_tree_add_item(dev_tree, hf_swils_rscn_portstate, tvb, offset, 1, ENC_BIG_ENDIAN);
            proto_tree_add_item(dev_tree, hf_swils_rscn_portid, tvb, offset+1, 3, ENC_NA);
            proto_tree_add_item(dev_tree, hf_swils_rscn_pwwn, tvb, offset+4, 8, ENC_NA);
            proto_tree_add_item(dev_tree, hf_swils_rscn_nwwn, tvb, offset+12, 8, ENC_NA);
            offset += 20;
        }
    }
}

/*
 * Merge Request contains zoning objects organized in the following format:
 *
 * Zone Set Object
 *      |
 *      +---------------- Zone Object
 *      |                      |
 *      +--                    +---------------- Zone Member
 *      |                      |                     |
 *      +--                    +----                 +-----
 *
 * So the decoding of the zone merge request is based on this structure
 */

static void
dissect_swils_zone_mbr(tvbuff_t *tvb, packet_info* pinfo, proto_tree *zmbr_tree, int offset)
{
    guint8  mbrtype;
    int     idlen;
    proto_item* ti;

    mbrtype = tvb_get_guint8(tvb, offset);
    ti = proto_tree_add_uint(zmbr_tree, hf_swils_zone_mbrtype, tvb,
                        offset, 1, mbrtype);
    proto_tree_add_item(zmbr_tree, hf_swils_zone_mbrflags, tvb, offset+2, 1, ENC_BIG_ENDIAN);
    idlen = tvb_get_guint8(tvb, offset+3);
    proto_tree_add_item(zmbr_tree, hf_swils_zone_mbr_identifier_length, tvb, offset+3, 1, ENC_BIG_ENDIAN);
    switch (mbrtype) {
    case FC_SWILS_ZONEMBR_WWN:
        proto_tree_add_item(zmbr_tree, hf_swils_zone_mbrid_fcwwn, tvb,
                              offset+4, 8, ENC_NA);
        break;
    case FC_SWILS_ZONEMBR_DP:
        proto_tree_add_item(zmbr_tree, hf_swils_zone_mbrid_uint, tvb,
                              offset+4, 4, ENC_BIG_ENDIAN);
        break;
    case FC_SWILS_ZONEMBR_FCID:
        proto_tree_add_item(zmbr_tree, hf_swils_zone_mbrid_fc, tvb,
                              offset+4, 3, ENC_NA);
        break;
    case FC_SWILS_ZONEMBR_ALIAS:
        proto_tree_add_string(zmbr_tree, hf_swils_zone_mbrid, tvb,
                              offset+4, idlen, zonenm_to_str(tvb, offset+4));
        break;
    case FC_SWILS_ZONEMBR_WWN_LUN:
        proto_tree_add_item(zmbr_tree, hf_swils_zone_mbrid_fcwwn, tvb,
                              offset+4, 8, ENC_NA);
        proto_tree_add_item(zmbr_tree, hf_swils_zone_mbrid_lun, tvb,
                            offset+12, 8, ENC_NA);
        break;
    case FC_SWILS_ZONEMBR_DP_LUN:
        proto_tree_add_item(zmbr_tree, hf_swils_zone_mbrid_uint, tvb,
                              offset+4, 4, ENC_BIG_ENDIAN);
        proto_tree_add_item(zmbr_tree, hf_swils_zone_mbrid_lun, tvb,
                            offset+8, 8, ENC_NA);
        break;
    case FC_SWILS_ZONEMBR_FCID_LUN:
        proto_tree_add_item(zmbr_tree, hf_swils_zone_mbrid_fc, tvb,
                              offset+4, 3, ENC_NA);
        proto_tree_add_item(zmbr_tree, hf_swils_zone_mbrid_lun, tvb,
                            offset+8, 8, ENC_NA);
        break;
    default:
        expert_add_info(pinfo, ti, &ei_swils_zone_mbrid);

    }
}

static void
dissect_swils_zone_obj(tvbuff_t *tvb, packet_info* pinfo, proto_tree *zobj_tree, int offset)
{
    proto_tree *zmbr_tree;
    int         mbrlen, numrec, i, objtype;
    char       *str;

    objtype = tvb_get_guint8(tvb, offset);

    proto_tree_add_item(zobj_tree, hf_swils_zone_objtype, tvb, offset,
                        1, ENC_BIG_ENDIAN);
    proto_tree_add_item(zobj_tree, hf_swils_zone_protocol, tvb,
                        offset+1, 1, ENC_BIG_ENDIAN);
    str = zonenm_to_str(tvb, offset+4);
    proto_tree_add_string(zobj_tree, hf_swils_zone_objname, tvb,
                          offset+4, ZONENAME_LEN(tvb, offset+4), str);

    numrec = tvb_get_ntohl(tvb, offset+4+ZONENAME_LEN(tvb, offset+4));
    proto_tree_add_item(zobj_tree, hf_swils_zone_num_members, tvb, offset+4+ZONENAME_LEN(tvb,offset+4), 4, ENC_BIG_ENDIAN);

    offset += 8 + ZONENAME_LEN(tvb, offset+4);
    for (i = 0; i < numrec; i++) {
        if (objtype == FC_SWILS_ZONEOBJ_ZONESET) {
            dissect_swils_zone_obj(tvb, pinfo, zobj_tree, offset);
            offset += get_zoneobj_len(tvb, offset);
        }
        else {
            mbrlen = 4 + tvb_get_guint8(tvb, offset+3);
            zmbr_tree = proto_tree_add_subtree_format(zobj_tree, tvb, offset, mbrlen,
                                        ett_fcswils_zonembr, NULL, "Zone Member %d", i);
            dissect_swils_zone_mbr(tvb, pinfo, zmbr_tree, offset);
            offset += mbrlen;
        }
    }
}

static void
dissect_swils_mergereq(tvbuff_t *tvb, packet_info* pinfo, proto_tree *mr_tree, guint8 isreq)
{
    /* Set up structures needed to add the protocol subtree and manage it */
    int         offset = 0;
    proto_tree *zobjlist_tree, *zobj_tree;
    int         numrec, i, zonesetlen, objlistlen, objlen;
    char       *str;

        if (isreq) {
            /* zonesetlen is the size of the zoneset including the zone name */
            zonesetlen = tvb_get_ntohs(tvb, offset+2);
            proto_tree_add_item(mr_tree, hf_swils_zone_active_zoneset_length, tvb, offset+2, 2, ENC_BIG_ENDIAN);

            if (zonesetlen) {
                str = zonenm_to_str(tvb, offset+4);
                proto_tree_add_string(mr_tree, hf_swils_zone_activezonenm, tvb,
                                      offset+4, ZONENAME_LEN(tvb, offset+4),
                                      str);

                /* objlistlen gives the size of the active zoneset object list */
                objlistlen = zonesetlen - ZONENAME_LEN(tvb, offset+4);
                /* Offset = start of the active zoneset zoning object list */
                offset = offset + (4 + ZONENAME_LEN(tvb, offset+4));
                numrec = tvb_get_ntohl(tvb, offset);

                zobjlist_tree = proto_tree_add_subtree(mr_tree, tvb, offset, objlistlen,
                                            ett_fcswils_zoneobjlist, NULL, "Active Zone Set");

                proto_tree_add_item(zobjlist_tree, hf_swils_zone_num_zoning_objects, tvb, offset, 4, ENC_BIG_ENDIAN);

                offset += 4;
                for (i = 0; i < numrec; i++) {
                    objlen = get_zoneobj_len(tvb, offset);
                    zobj_tree = proto_tree_add_subtree_format(zobjlist_tree, tvb, offset+4, objlen,
                                                ett_fcswils_zoneobj, NULL, "Zone Object %d", i);
                    dissect_swils_zone_obj(tvb, pinfo, zobj_tree, offset);
                    offset += objlen;
                }
            }
            else {
                offset += 4;
            }

            zonesetlen = tvb_get_ntohl(tvb, offset);
            proto_tree_add_item(mr_tree, hf_swils_zone_full_zone_set_length, tvb, offset, 4, ENC_BIG_ENDIAN);

            if (zonesetlen) {
                objlistlen = zonesetlen;
                /* Offset = start of the active zoneset zoning object list */
                offset += 4;
                numrec = tvb_get_ntohl(tvb, offset);

                zobjlist_tree = proto_tree_add_subtree(mr_tree, tvb, offset, objlistlen,
                                            ett_fcswils_zoneobjlist, NULL, "Full Zone Set");

                proto_tree_add_item(zobjlist_tree, hf_swils_zone_num_zoning_objects, tvb, offset, 4, ENC_BIG_ENDIAN);
                offset += 4;
                for (i = 0; i < numrec; i++) {
                    objlen = get_zoneobj_len(tvb, offset);
                    zobj_tree = proto_tree_add_subtree_format(zobjlist_tree, tvb, offset,
                                                objlen, ett_fcswils_zoneobj, NULL, "Zone Object %d", i);
                    dissect_swils_zone_obj(tvb, pinfo, zobj_tree, offset);
                    offset += objlen;
                }
            }
        }
        else {
            proto_tree_add_item(mr_tree, hf_swils_zone_status, tvb,
                                offset+5, 1, ENC_BIG_ENDIAN);
            proto_tree_add_item(mr_tree, hf_swils_zone_reason, tvb,
                                offset+6, 1, ENC_BIG_ENDIAN);
            proto_tree_add_item(mr_tree, hf_swils_zone_vendor_unique, tvb, offset+7, 1, ENC_BIG_ENDIAN);
        }
}

static void
dissect_swils_aca(tvbuff_t *tvb, packet_info* pinfo _U_, proto_tree *aca_tree, guint8 isreq)
{
    /* Set up structures needed to add the protocol subtree and manage it */
    int offset = 0;
    int numrec, plen, i;

    if (aca_tree) {
        if (isreq) {
            plen = tvb_get_ntohs(tvb, offset+2);
            proto_tree_add_item(aca_tree, hf_swils_domain_id_list_length, tvb, offset+2, 2, ENC_BIG_ENDIAN);
            numrec = plen/4;
            offset = 4;

            for (i = 0; i < numrec; i++) {
                proto_tree_add_uint_format(aca_tree, hf_swils_aca_domainid,
                                           tvb, offset+3, 1,
                                           tvb_get_guint8(tvb, offset+3),
                                           "Domain ID %d: %d", i,
                                           tvb_get_guint8(tvb, offset+3));
                offset += 4;
            }
        }
        else {
            proto_tree_add_item(aca_tree, hf_swils_zone_status, tvb,
                                offset+5, 1, ENC_BIG_ENDIAN);
            proto_tree_add_item(aca_tree, hf_swils_zone_reason, tvb,
                                offset+6, 1, ENC_BIG_ENDIAN);
            proto_tree_add_item(aca_tree, hf_swils_zone_vendor_unique, tvb, offset+7, 1, ENC_BIG_ENDIAN);
        }
    }
}

static void
dissect_swils_rca(tvbuff_t *tvb, packet_info* pinfo _U_, proto_tree *rca_tree, guint8 isreq)
{
    /* Set up structures needed to add the protocol subtree and manage it */
    int offset = 0;

    if (rca_tree) {
        if (!isreq) {
            proto_tree_add_item(rca_tree, hf_swils_zone_status, tvb,
                                offset+5, 1, ENC_BIG_ENDIAN);
            proto_tree_add_item(rca_tree, hf_swils_zone_reason, tvb,
                                offset+6, 1, ENC_BIG_ENDIAN);
            proto_tree_add_item(rca_tree, hf_swils_zone_vendor_unique, tvb, offset+7, 1, ENC_BIG_ENDIAN);
        }
    }
}

static void
dissect_swils_sfc(tvbuff_t *tvb, packet_info* pinfo, proto_tree *sfc_tree, guint8 isreq)
{
    /* Set up structures needed to add the protocol subtree and manage it */
    int         offset = 0;
    proto_tree *zobjlist_tree, *zobj_tree;
    int         numrec, i, zonesetlen, objlistlen, objlen;
    char       *str;

        if (isreq) {
            proto_tree_add_item(sfc_tree, hf_swils_sfc_opcode, tvb, offset+1, 1, ENC_BIG_ENDIAN);

            zonesetlen = tvb_get_ntohs(tvb, offset+2);
            proto_tree_add_item(sfc_tree, hf_swils_sfc_zoneset_length, tvb, offset+2, 2, ENC_BIG_ENDIAN);

            if (zonesetlen) {
                str = zonenm_to_str(tvb, offset+4);
                proto_tree_add_string(sfc_tree, hf_swils_sfc_zonenm, tvb,
                                      offset+4, ZONENAME_LEN(tvb, offset+4),
                                      str);

                /* objlistlen gives the size of the active zoneset object list */
                objlistlen = zonesetlen - ZONENAME_LEN(tvb, offset+4);
                /* Offset = start of the active zoneset zoning object list */
                offset = offset + (4 + ZONENAME_LEN(tvb, offset+4));
                numrec = tvb_get_ntohl(tvb, offset);

                zobjlist_tree = proto_tree_add_subtree(sfc_tree, tvb, offset, objlistlen,
                                            ett_fcswils_zoneobjlist, NULL, "Zone Set");

                proto_tree_add_item(zobjlist_tree, hf_swils_zone_num_zoning_objects, tvb, offset, 4, ENC_BIG_ENDIAN);

                offset += 4;
                for (i = 0; i < numrec; i++) {
                    objlen = get_zoneobj_len(tvb, offset);
                    zobj_tree = proto_tree_add_subtree_format(zobjlist_tree, tvb, offset, objlen,
                                                ett_fcswils_zoneobj, NULL, "Zone Object %d", i);
                    dissect_swils_zone_obj(tvb, pinfo, zobj_tree, offset);
                    offset += objlen;
                }
            }
            else {
                offset += 4;
            }

            zonesetlen = tvb_get_ntohl(tvb, offset);
            proto_tree_add_item(sfc_tree, hf_swils_zone_full_zone_set_length, tvb, offset, 4, ENC_BIG_ENDIAN);

            if (zonesetlen) {
                objlistlen = zonesetlen;
                /* Offset = start of the active zoneset zoning object list */
                offset += 4;
                numrec = tvb_get_ntohl(tvb, offset);

                zobjlist_tree = proto_tree_add_subtree(sfc_tree, tvb, offset, objlistlen,
                                            ett_fcswils_zoneobjlist, NULL, "Full Zone Set");

                proto_tree_add_item(zobjlist_tree, hf_swils_zone_num_zoning_objects, tvb, offset, 4, ENC_BIG_ENDIAN);
                offset += 4;
                for (i = 0; i < numrec; i++) {
                    objlen = get_zoneobj_len(tvb, offset);
                    zobj_tree = proto_tree_add_subtree_format(zobjlist_tree, tvb, offset, objlen,
                                                ett_fcswils_zoneobj, NULL, "Zone Object %d", i);
                    dissect_swils_zone_obj(tvb, pinfo, zobj_tree, offset);
                    offset += objlen;
                }
            }
        }
        else {
            proto_tree_add_item(sfc_tree, hf_swils_zone_status, tvb,
                                offset+5, 1, ENC_BIG_ENDIAN);
            proto_tree_add_item(sfc_tree, hf_swils_zone_reason, tvb,
                                offset+6, 1, ENC_BIG_ENDIAN);
            proto_tree_add_item(sfc_tree, hf_swils_zone_vendor_unique, tvb, offset+7, 1, ENC_BIG_ENDIAN);
        }
}

static void
dissect_swils_ufc(tvbuff_t *tvb, packet_info* pinfo _U_, proto_tree *ufc_tree, guint8 isreq)
{
    /* Set up structures needed to add the protocol subtree and manage it */
    int offset = 0;

    if (ufc_tree) {
        if (!isreq) {
            proto_tree_add_item(ufc_tree, hf_swils_zone_status, tvb,
                                offset+5, 1, ENC_BIG_ENDIAN);
            proto_tree_add_item(ufc_tree, hf_swils_zone_reason, tvb,
                                offset+6, 1, ENC_BIG_ENDIAN);
            proto_tree_add_item(ufc_tree, hf_swils_zone_vendor_unique, tvb, offset+7, 1, ENC_BIG_ENDIAN);
        }
    }
}

static void
dissect_swils_esc(tvbuff_t *tvb, packet_info* pinfo _U_, proto_tree *esc_tree, guint8 isreq)
{
    /* Set up structures needed to add the protocol subtree and manage it */
    int         offset = 0;
    int         i, numrec, plen;
    proto_tree *pdesc_tree;

    if (esc_tree) {
        if (isreq) {
            plen = tvb_get_ntohs(tvb, offset+2);
            proto_tree_add_item(esc_tree, hf_swils_esc_payload_length, tvb, offset+2, 2, ENC_BIG_ENDIAN);
            proto_tree_add_item(esc_tree, hf_swils_esc_swvendorid, tvb,
                                offset+4, 8, ENC_ASCII|ENC_NA);
            numrec = (plen - 12)/12;
            offset = 12;

            for (i = 0; i < numrec; i++) {
                pdesc_tree = proto_tree_add_subtree_format(esc_tree, tvb, offset, 12,
                                            ett_fcswils_esc_pdesc, NULL, "Protocol Descriptor %d", i);
                proto_tree_add_item(pdesc_tree, hf_swils_esc_pdesc_vendorid, tvb,
                                    offset, 8, ENC_ASCII|ENC_NA);
                proto_tree_add_item(pdesc_tree, hf_swils_esc_protocolid,
                                    tvb, offset+10, 2, ENC_BIG_ENDIAN);
                offset += 12;
            }
        }
        else {
            proto_tree_add_item(esc_tree, hf_swils_esc_swvendorid, tvb,
                                offset+4, 8, ENC_ASCII|ENC_NA);
            pdesc_tree = proto_tree_add_subtree(esc_tree, tvb, offset+12, 12,
                                        ett_fcswils_esc_pdesc, NULL, "Accepted Protocol Descriptor");

            proto_tree_add_item(pdesc_tree, hf_swils_esc_pdesc_vendorid, tvb,
                                offset+12, 8, ENC_ASCII|ENC_NA);
            proto_tree_add_item(pdesc_tree, hf_swils_esc_protocolid,
                                tvb, offset+22, 2, ENC_BIG_ENDIAN);
        }
    }
}

static void
dissect_swils_drlir(tvbuff_t *tvb _U_, packet_info* pinfo _U_, proto_tree *drlir_tree _U_,
                    guint8 isreq _U_)
{
    /* Set up structures needed to add the protocol subtree and manage it */
    return;
}

static void
dissect_swils_swrjt(tvbuff_t *tvb, packet_info* pinfo _U_, proto_tree *swrjt_tree, guint8 isreq _U_)
{
    /* Set up structures needed to add the protocol subtree and manage it */
    int offset = 0;

    if (swrjt_tree) {
        proto_tree_add_item(swrjt_tree, hf_swils_rjt, tvb, offset+5, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item(swrjt_tree, hf_swils_rjtdet, tvb, offset+6, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item(swrjt_tree, hf_swils_rjtvendor, tvb, offset+7,
                            1, ENC_BIG_ENDIAN);
    }
}

static void
dissect_swils_ess(tvbuff_t *tvb, packet_info* pinfo _U_, proto_tree *ess_tree, guint8 isreq _U_)
{
    int         offset      = 0;
    gint16      numcapobj   = 0;
    gint        len         = 0;
    gint        capobjlen   = 0;
    proto_tree *ieinfo_tree;

    if (!ess_tree) {
        return;
    }

    proto_tree_add_item(ess_tree, hf_swils_ess_rev, tvb, offset+4, 4, ENC_BIG_ENDIAN);
    proto_tree_add_item(ess_tree, hf_swils_ess_len, tvb, offset+8, 4, ENC_BIG_ENDIAN);
    len = tvb_get_ntohl(tvb, offset+8);

    ieinfo_tree = proto_tree_add_subtree(ess_tree, tvb, offset+12,
                             MAX_INTERCONNECT_ELEMENT_INFO_LEN+4,
                             ett_fcswils_ieinfo, NULL, "Interconnect Element Info");
    dissect_swils_interconnect_element_info(tvb, ieinfo_tree, offset+12);
    len -= 256;                /* the interconnect obj above is 256 bytes */
    offset += 268;

    proto_tree_add_item(ess_tree, hf_swils_ess_numobj, tvb, offset, 2, ENC_BIG_ENDIAN);
    numcapobj = tvb_get_ntohs(tvb, offset);

    len -= 4;                  /* 2B numcapobj + 2B rsvd */
    offset += 4;

    while ((len > 0) && (numcapobj > 0)) {
        capobjlen = dissect_swils_ess_capability_obj(tvb, ess_tree, offset);
        numcapobj--;
        len -= capobjlen;
        offset += capobjlen;
    }
}

static void
dissect_swils_mrra(tvbuff_t *tvb, packet_info* pinfo _U_, proto_tree *tree, guint8 isreq)
{

    int offset = 0;

    if (!tree) {
        return;
    }

    if (isreq) {
        proto_tree_add_item(tree, hf_swils_mrra_rev, tvb, offset+4, 4, ENC_BIG_ENDIAN);
        proto_tree_add_item(tree, hf_swils_mrra_size, tvb, offset+8, 4, ENC_BIG_ENDIAN);
        proto_tree_add_item(tree, hf_swils_mrra_vendorid, tvb, offset+12, 8, ENC_ASCII|ENC_NA);
        proto_tree_add_item(tree, hf_swils_mrra_vendorinfo, tvb, offset+20,
                            8, ENC_NA);
    } else {
        proto_tree_add_item(tree, hf_swils_mrra_vendorid, tvb, offset+4,
                            8, ENC_ASCII|ENC_NA);
        proto_tree_add_item(tree, hf_swils_mrra_reply, tvb, offset+12,
                            4, ENC_BIG_ENDIAN);
        proto_tree_add_item(tree, hf_swils_mrra_reply_size, tvb, offset+16,
                            4, ENC_BIG_ENDIAN);
        proto_tree_add_item(tree, hf_swils_mrra_waittime, tvb, offset+20,
                            4, ENC_BIG_ENDIAN);
    }


}

static fcswils_func_table_t fcswils_func_table[FC_SWILS_MAXCODE] = {
    /* 0x00 */ {NULL},
    /* 0x01 */ {dissect_swils_swrjt},
    /* 0x02 */ {NULL},
    /* 0x03 */ {NULL},
    /* 0x04 */ {NULL},
    /* 0x05 */ {NULL},
    /* 0x06 */ {NULL},
    /* 0x07 */ {NULL},
    /* 0x08 */ {NULL},
    /* 0x09 */ {NULL},
    /* 0x0a */ {NULL},
    /* 0x0b */ {NULL},
    /* 0x0c */ {NULL},
    /* 0x0d */ {NULL},
    /* 0x0e */ {NULL},
    /* 0x0f */ {NULL},
    /* 0x10 */ {dissect_swils_elp},
    /* 0x11 */ {dissect_swils_efp},
    /* 0x12 */ {dissect_swils_dia},
    /* 0x13 */ {dissect_swils_rdi},
    /* 0x14 */ {dissect_swils_hello},
    /* 0x15 */ {dissect_swils_lsupdate},
    /* 0x16 */ {dissect_swils_lsack},
    /* 0x17 */ {dissect_swils_nullpayload},
    /* 0x18 */ {dissect_swils_nullpayload},
    /* 0x19 */ {NULL},
    /* 0x1a */ {NULL},
    /* 0x1b */ {dissect_swils_rscn},
    /* 0x1c */ {NULL},
    /* 0x1d */ {NULL},
    /* 0x1e */ {dissect_swils_drlir},
    /* 0x1f */ {NULL},
    /* 0x20 */ {NULL /*dissect_swils_dscn*/},
    /* 0x21 */ {NULL /*dissect_swils_loopd*/},
    /* 0x22 */ {dissect_swils_mergereq},
    /* 0x23 */ {dissect_swils_aca},
    /* 0x24 */ {dissect_swils_rca},
    /* 0x25 */ {dissect_swils_sfc},
    /* 0x26 */ {dissect_swils_ufc},
    /* 0x27 */ {NULL},
    /* 0x28 */ {NULL},
    /* 0x29 */ {NULL},
    /* 0x2a */ {NULL},
    /* 0x2b */ {NULL},
    /* 0x2c */ {NULL},
    /* 0x2d */ {NULL},
    /* 0x2e */ {NULL},
    /* 0x2f */ {NULL},
    /* 0x30 */ {dissect_swils_esc},
    /* 0x31 */ {dissect_swils_ess},
    /* 0x32 */ {NULL},
    /* 0x33 */ {NULL},
    /* 0x34 */ {dissect_swils_mrra}
};

/* Code to actually dissect the packets */
static int
dissect_fcswils(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data)
{
    proto_item          *ti            = NULL;
    guint8               opcode;
    guint8               failed_opcode = 0;
    int                  offset        = 0;
    conversation_t      *conversation;
    fcswils_conv_data_t *cdata;
    fcswils_conv_key_t   ckey, *req_key;
    proto_tree          *swils_tree    = NULL;
    guint8               isreq         = FC_SWILS_REQ;
    tvbuff_t            *next_tvb;
    fc_hdr *fchdr;

    /* Reject the packet if data is NULL */
    if (data == NULL)
        return 0;
    fchdr = (fc_hdr *)data;

    /* Make entries in Protocol column and Info column on summary display */
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "SW_ILS");

    /* decoding of this is done by each individual opcode handler */
    opcode = tvb_get_guint8(tvb, 0);

    ti = proto_tree_add_protocol_format(tree, proto_fcswils, tvb, 0, -1, "SW_ILS");
    swils_tree = proto_item_add_subtree(ti, ett_fcswils);

    /* Register conversation if this is not a response */
    if ((opcode != FC_SWILS_SWACC) && (opcode != FC_SWILS_SWRJT)) {
        conversation = find_conversation(pinfo->num, &pinfo->src, &pinfo->dst,
                                         pinfo->ptype, fchdr->oxid,
                                         fchdr->rxid, NO_PORT2);
        if (!conversation) {
            conversation = conversation_new(pinfo->num, &pinfo->src, &pinfo->dst,
                                            pinfo->ptype, fchdr->oxid,
                                            fchdr->rxid, NO_PORT2);
        }

        ckey.conv_idx = conversation->conv_index;

        cdata = (fcswils_conv_data_t *)g_hash_table_lookup(fcswils_req_hash,
                                                           &ckey);
        if (cdata) {
            /* Since we never free the memory used by an exchange, this maybe a
             * case of another request using the same exchange as a previous
             * req.
             */
            cdata->opcode = opcode;
        }
        else {
            req_key = wmem_new(wmem_file_scope(), fcswils_conv_key_t);
            req_key->conv_idx = conversation->conv_index;

            cdata = wmem_new(wmem_file_scope(), fcswils_conv_data_t);
            cdata->opcode = opcode;

            g_hash_table_insert(fcswils_req_hash, req_key, cdata);
        }
    }
    else {
        /* Opcode is ACC or RJT */
        conversation = find_conversation(pinfo->num, &pinfo->src, &pinfo->dst,
                                         pinfo->ptype, fchdr->oxid,
                                         fchdr->rxid, NO_PORT2);
        isreq = FC_SWILS_RPLY;
        if (!conversation) {
            if (tree && (opcode == FC_SWILS_SWACC)) {
                /* No record of what this accept is for. Can't decode */
                proto_tree_add_expert_format(swils_tree, pinfo, &ei_swils_no_exchange, tvb, 0, -1, "No record of Exchg. Unable to decode SW_ACC");
                return 0;
            }
        }
        else {
            ckey.conv_idx = conversation->conv_index;

            cdata = (fcswils_conv_data_t *)g_hash_table_lookup(fcswils_req_hash, &ckey);

            if (cdata != NULL) {
                if (opcode == FC_SWILS_SWACC)
                    opcode = cdata->opcode;
                else
                    failed_opcode = cdata->opcode;
            }

            if (tree) {
                if ((cdata == NULL) && (opcode != FC_SWILS_SWRJT)) {
                    /* No record of what this accept is for. Can't decode */
                    proto_tree_add_expert_format(swils_tree, pinfo, &ei_swils_no_exchange, tvb, 0, -1, "No record of SW_ILS Req. Unable to decode SW_ACC");
                    return 0;
                }
            }
        }
    }

    if (isreq == FC_SWILS_REQ) {
        col_add_str(pinfo->cinfo, COL_INFO,
                    val_to_str(opcode, fc_swils_opcode_key_val, "0x%x"));
    }
    else if (opcode == FC_SWILS_SWRJT) {
        col_add_fstr(pinfo->cinfo, COL_INFO, "SW_RJT (%s)",
                        val_to_str(failed_opcode, fc_swils_opcode_key_val, "0x%x"));
    }
    else {
        col_add_fstr(pinfo->cinfo, COL_INFO, "SW_ACC (%s)",
                        val_to_str(opcode, fc_swils_opcode_key_val, "0x%x"));
    }

    proto_tree_add_item(swils_tree, hf_swils_opcode, tvb, offset, 1, ENC_BIG_ENDIAN);

    if ((opcode < FC_SWILS_MAXCODE) && fcswils_func_table[opcode].func) {
        fcswils_func_table[opcode].func(tvb, pinfo, swils_tree, isreq);
    } else if (opcode == FC_SWILS_AUTH_ILS) {
        /* This is treated differently */
        if (isreq && fcsp_handle)
            call_dissector(fcsp_handle, tvb, pinfo, swils_tree);
    } else {
        /* data dissector */
        next_tvb = tvb_new_subset_remaining(tvb, offset+4);
        call_data_dissector(next_tvb, pinfo, tree);
    }

    return tvb_captured_length(tvb);
}

/* Register the protocol with Wireshark */

void
proto_register_fcswils(void)
{
    static hf_register_info hf[] = {
        { &hf_swils_opcode,
          {"Cmd Code", "swils.opcode",
           FT_UINT8, BASE_HEX, VALS(fc_swils_opcode_key_val), 0x0,
           NULL, HFILL}},

        { &hf_swils_elp_rev,
          {"Revision", "swils.elp.rev",
           FT_UINT8, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_elp_flags,
          {"Flag", "swils.elp.flag",
           FT_BYTES, BASE_NONE, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_elp_r_a_tov,
          {"R_A_TOV", "swils.elp.ratov",
           FT_UINT32, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_elp_e_d_tov,
          {"E_D_TOV", "swils.elp.edtov",
           FT_UINT32, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_elp_req_epn,
          {"Req Eport Name", "swils.elp.reqepn",
           FT_FCWWN, BASE_NONE, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_elp_req_esn,
          {"Req Switch Name", "swils.elp.reqesn",
           FT_FCWWN, BASE_NONE, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_elp_clsf_svcp,
          {"Class F Svc Parameters", "swils.elp.clsfp",
           FT_BYTES, BASE_NONE, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_elp_clsf_rcvsz,
          {"Max Class F Frame Size", "swils.elp.clsfrsz",
           FT_UINT16, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_elp_clsf_conseq,
          {"Class F Max Concurrent Seq", "swils.elp.clsfcs",
           FT_UINT16, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_elp_clsf_e2e,
          {"Class F E2E Credit", "swils.elp.cfe2e",
           FT_UINT16, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_elp_clsf_openseq,
          {"Class F Max Open Seq", "swils.elp.oseq",
           FT_UINT16, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_elp_cls1_svcp,
          {"Class 1 Svc Parameters", "swils.elp.cls1p",
           FT_BYTES, BASE_NONE, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_elp_cls1_rcvsz,
          {"Class 1 Frame Size", "swils.elp.cls1rsz",
           FT_UINT16, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_elp_cls2_svcp,
          {"Class 2 Svc Parameters", "swils.elp.cls2p",
           FT_BYTES, BASE_NONE, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_elp_cls2_rcvsz,
          {"Class 2 Frame Size", "swils.elp.cls2rsz",
           FT_UINT16, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_elp_cls3_svcp,
          {"Class 3 Svc Parameters", "swils.elp.cls3p",
           FT_BYTES, BASE_NONE, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_elp_cls3_rcvsz,
          {"Class 3 Frame Size", "swils.elp.cls3rsz",
           FT_UINT16, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_elp_isl_fc_mode,
          {"ISL Flow Ctrl Mode", "swils.elp.fcmode",
           FT_STRING, BASE_NONE, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_elp_fcplen,
          {"Flow Ctrl Param Len", "swils.elp.fcplen",
           FT_UINT16, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_elp_b2bcredit,
          {"B2B Credit", "swils.elp.b2b",
           FT_UINT32, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_elp_compat1,
          {"Compatibility Param 1", "swils.elp.compat1",
           FT_UINT32, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_elp_compat2,
          {"Compatibility Param 2", "swils.elp.compat2",
           FT_UINT32, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_elp_compat3,
          {"Compatibility Param 3", "swils.elp.compat3",
           FT_UINT32, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_elp_compat4,
          {"Compatibility Param 4", "swils.elp.compat4",
           FT_UINT32, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_efp_rec_type,
          {"Record Type", "swils.efp.rectype",
           FT_UINT8, BASE_HEX, VALS(fcswils_rectype_val), 0x0,
           NULL, HFILL}},

        { &hf_swils_efp_dom_id,
          {"Domain ID", "swils.efp.domid",
           FT_UINT8, BASE_HEX, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_efp_switch_name,
          {"Switch Name", "swils.efp.sname",
           FT_FCWWN, BASE_NONE, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_efp_mcast_grpno,
          {"Mcast Grp#", "swils.efp.mcastno",
           FT_UINT8, BASE_HEX, NULL, 0x0,
           NULL, HFILL}},

#if 0
        { &hf_swils_efp_alias_token,
          {"Alias Token", "swils.efp.aliastok",
           FT_BYTES, BASE_NONE, NULL, 0x0,
           NULL, HFILL}},
#endif

        { &hf_swils_efp_record_len,
          {"Record Len", "swils.efp.recordlen",
           FT_UINT8, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_efp_payload_len,
          {"Payload Len", "swils.efp.payloadlen",
           FT_UINT16, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_efp_pswitch_pri,
          {"Principal Switch Priority", "swils.efp.psprio",
           FT_UINT8, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_efp_pswitch_name,
          {"Principal Switch Name", "swils.efp.psname",
           FT_FCWWN, BASE_NONE, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_dia_switch_name,
          {"Switch Name", "swils.dia.sname",
           FT_FCWWN, BASE_NONE, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_rdi_payload_len,
          {"Payload Len", "swils.rdi.len",
           FT_UINT16, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_rdi_req_sname,
          {"Req Switch Name", "swils.rdi.reqsn",
           FT_FCWWN, BASE_NONE, NULL, 0x0,
           NULL, HFILL}},

#if 0
        { &hf_swils_fspfh_cmd,
          {"Command:", "swils.fspf.cmd",
           FT_UINT8, BASE_HEX, NULL, 0x0,
           NULL, HFILL}},
#endif

        { &hf_swils_fspfh_rev,
          {"Version", "swils.fspf.ver",
           FT_UINT8, BASE_HEX, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_fspfh_ar_num,
          {"AR Number", "swils.fspf.arnum",
           FT_UINT8, BASE_HEX, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_fspfh_auth_type,
          {"Authentication Type", "swils.fspf.authtype",
           FT_UINT8, BASE_HEX, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_fspfh_dom_id,
          {"Originating Domain ID", "swils.fspf.origdomid",
           FT_UINT8, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_fspfh_auth,
          {"Authentication", "swils.fspf.auth",
           FT_BYTES, BASE_NONE, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_hlo_options,
          {"Options", "swils.hlo.options",
           FT_BYTES, BASE_NONE, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_hlo_hloint,
          {"Hello Interval (secs)", "swils.hlo.hloint",
           FT_UINT32, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_hlo_deadint,
          {"Dead Interval (secs)", "swils.hlo.deadint",
           FT_UINT32, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_hlo_rcv_domid,
          {"Recipient Domain ID", "swils.hlo.rcvdomid",
           FT_UINT8, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_hlo_orig_pidx,
          {"Originating Port Idx", "swils.hlo.origpidx",
           FT_UINT24, BASE_HEX, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_lsrh_lsr_type,
          {"LSR Type", "swils.lsr.type",
           FT_UINT8, BASE_HEX, VALS(fc_swils_fspf_linkrec_val), 0x0,
           NULL, HFILL}},

        { &hf_swils_lsrh_lsid,
          {"Link State Id", "swils.ls.id",
           FT_UINT8, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_lsrh_adv_domid,
          {"Advertising Domain Id", "swils.lsr.advdomid",
           FT_UINT8, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_lsrh_ls_incid,
          {"LS Incarnation Number", "swils.lsr.incid",
           FT_UINT32, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_ldrec_linkid,
          {"Link ID", "swils.ldr.linkid",
           FT_BYTES, SEP_DOT, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_ldrec_out_pidx,
          {"Output Port Idx", "swils.ldr.out_portidx",
           FT_UINT24, BASE_HEX, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_ldrec_nbr_pidx,
          {"Neighbor Port Idx", "swils.ldr.nbr_portidx",
           FT_UINT24, BASE_HEX, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_ldrec_link_type,
          {"Link Type", "swils.ldr.linktype",
           FT_UINT8, BASE_HEX, VALS(fc_swils_link_type_val), 0x0,
           NULL, HFILL}},

        { &hf_swils_ldrec_link_cost,
          {"Link Cost", "swils.ldr.linkcost",
           FT_UINT16, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_rscn_evtype,
          {"Event Type", "swils.rscn.evtype",
           FT_UINT8, BASE_DEC, VALS(fc_swils_rscn_portstate_val), 0xF0,
           NULL, HFILL}},

        { &hf_swils_rscn_addrfmt,
          {"Address Format", "swils.rscn.addrfmt",
           FT_UINT8, BASE_DEC, VALS(fc_swils_rscn_addrfmt_val), 0x0F,
           NULL, HFILL}},

        { &hf_swils_rscn_affectedport,
          {"Affected Port ID", "swils.rscn.affectedport",
           FT_BYTES, SEP_DOT, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_rscn_detectfn,
          {"Detection Function", "swils.rscn.detectfn",
           FT_UINT32, BASE_HEX, VALS(fc_swils_rscn_detectfn_val), 0x0,
           NULL, HFILL}},

        { &hf_swils_rscn_portstate,
          {"Port State", "swils.rscn.portstate",
           FT_UINT8, BASE_HEX, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_rscn_portid,
          {"Port Id", "swils.rscn.portid",
           FT_BYTES, SEP_DOT, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_rscn_pwwn,
          {"Port WWN", "swils.rscn.pwwn",
           FT_FCWWN, BASE_NONE, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_rscn_nwwn,
          {"Node WWN", "swils.rscn.nwwn",
           FT_FCWWN, BASE_NONE, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_esc_swvendorid,
          {"Switch Vendor ID", "swils.esc.swvendor",
           FT_STRING, BASE_NONE, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_esc_pdesc_vendorid,
          {"Vendor ID", "swils.esc.vendorid",
           FT_STRING, BASE_NONE, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_esc_protocolid,
          {"Protocol ID", "swils.esc.protocol",
           FT_UINT16, BASE_HEX, VALS(fc_swils_esc_protocol_val), 0x0,
           NULL, HFILL}},

        { &hf_swils_zone_activezonenm,
          {"Active Zoneset Name", "swils.mr.activezonesetname",
           FT_STRING, BASE_NONE, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_zone_objname,
          {"Zone Object Name", "swils.zone.zoneobjname",
           FT_STRING, BASE_NONE, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_zone_objtype,
          {"Zone Object Type", "swils.zone.zoneobjtype",
           FT_UINT8, BASE_HEX, VALS(fc_swils_zoneobj_type_val), 0x0,
           NULL, HFILL}},

        { &hf_swils_zone_mbrtype,
          {"Zone Member Type", "swils.zone.mbrtype",
           FT_UINT8, BASE_HEX, VALS(fc_swils_zonembr_type_val), 0x0,
           NULL, HFILL}},

        { &hf_swils_zone_protocol,
          {"Zone Protocol", "swils.zone.protocol",
           FT_UINT8, BASE_HEX, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_zone_mbrid,
          {"Member Identifier", "swils.zone.mbrid",
           FT_STRING, BASE_NONE, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_zone_mbrid_fcwwn,
          {"Member Identifier", "swils.zone.mbrid.fcwwn",
           FT_FCWWN, BASE_NONE, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_zone_mbrid_fc,
          {"Member Identifier", "swils.zone.mbrid.fc",
           FT_BYTES, SEP_DOT, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_zone_mbrid_uint,
          {"Member Identifier", "swils.zone.mbrid.uint",
           FT_UINT32, BASE_HEX, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_zone_status,
          {"Zone Command Status", "swils.zone.status",
           FT_UINT8, BASE_HEX, VALS(fc_swils_mr_rsp_val), 0x0,
           "Applies to MR, ACA, RCA, SFC, UFC", HFILL}},

        { &hf_swils_zone_reason,
          {"Zone Command Reason Code", "swils.zone.reason",
           FT_UINT8, BASE_HEX, VALS(fc_swils_mr_reason_val), 0x0,
           "Applies to MR, ACA, RCA, SFC, UFC", HFILL}},

        { &hf_swils_aca_domainid,
          {"Known Domain ID", "swils.aca.domainid",
           FT_UINT8, BASE_HEX, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_sfc_opcode,
          {"Operation Request", "swils.sfc.opcode",
           FT_UINT8, BASE_HEX, VALS(fc_swils_sfc_op_val), 0x0,
           NULL, HFILL}},

        { &hf_swils_sfc_zonenm,
          {"Zone Set Name", "swils.sfc.zonename",
           FT_STRING, BASE_NONE, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_rjt,
          {"Reason Code", "swils.rjt.reason",
           FT_UINT8, BASE_HEX, VALS(fc_swils_rjt_val), 0x0,
           NULL, HFILL}},

        { &hf_swils_rjtdet,
          {"Reason Code Explanantion", "swils.rjt.reasonexpl",
           FT_UINT8, BASE_HEX, VALS(fc_swils_deterr_val), 0x0,
           NULL, HFILL}},

        { &hf_swils_rjtvendor,
          {"Vendor Unique Error Code", "swils.rjt.vendor",
           FT_UINT8, BASE_HEX, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_zone_mbrid_lun,
          {"LUN", "swils.zone.lun",
           FT_BYTES, BASE_NONE, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_ess_rev,
          {"Revision", "swils.ess.revision",
           FT_UINT32, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_ess_len,
          {"Payload Length", "swils.ess.leb",
           FT_UINT32, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_ess_numobj,
          {"Number of Capability Objects", "swils.ess.numobj",
           FT_UINT16, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_interconnect_list_len,
          {"List Length", "swils.ess.listlen",
           FT_UINT8, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_ess_vendorname,
          {"Vendor Name", "swils.ess.vendorname",
           FT_STRING, BASE_NONE, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_ess_modelname,
          {"Model Name", "swils.ess.modelname",
           FT_STRING, BASE_NONE, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_ess_relcode,
          {"Release Code", "swils.ess.relcode",
           FT_STRING, BASE_NONE, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_ess_vendorspecific,
          {"Vendor Specific", "swils.ess.vendorspecific",
           FT_STRING, BASE_NONE, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_ess_cap_type,
          {"Type", "swils.ess.capability.type",
           FT_UINT8, BASE_DEC, VALS(fc_ct_gstype_vals), 0x0,
           NULL, HFILL}},

        { &hf_swils_ess_cap_subtype,
          {"Subtype", "swils.ess.capability.subtype",
           FT_UINT8, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_ess_cap_numentries,
          {"Number of Entries", "swils.ess.capability.numentries",
           FT_UINT8, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_ess_cap_svc,
          {"Service Name", "swils.ess.capability.service",
           FT_UINT8, BASE_DEC, VALS(fc_ct_gsserver_vals), 0x0,
           NULL, HFILL}},

        { &hf_swils_ess_dns_obj0h,
          {"Name Server Entry Object 00h Support", "swils.ess.capability.dns.obj0h",
           FT_BOOLEAN, 8, NULL, 0x1,
           NULL, HFILL}},

        { &hf_swils_ess_dns_obj1h,
          {"Name Server Entry Object 01h Support", "swils.ess.capability.dns.obj1h",
           FT_BOOLEAN, 8, NULL, 0x2,
           NULL, HFILL}},

        { &hf_swils_ess_dns_obj2h,
          {"Name Server Entry Object 02h Support", "swils.ess.capability.dns.obj2h",
           FT_BOOLEAN, 8, NULL, 0x4,
           NULL, HFILL}},

        { &hf_swils_ess_dns_obj3h,
          {"Name Server Entry Object 03h Support", "swils.ess.capability.dns.obj3h",
           FT_BOOLEAN, 8, NULL, 0x8,
           NULL, HFILL}},

        { &hf_swils_ess_dns_zlacc,
          {"GE_PT Zero Length Accepted", "swils.ess.capability.dns.zlacc",
           FT_BOOLEAN, 8, NULL, 0x10,
           NULL, HFILL}},

        { &hf_swils_ess_dns_vendor,
          {"Vendor Specific Flags", "swils.ess.capability.dns.vendor",
           FT_UINT32, BASE_HEX, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_ess_fctlr_rscn,
          {"SW_RSCN Supported", "swils.ess.capability.fctlr.rscn",
           FT_BOOLEAN, 8, NULL, 0x1,
           NULL, HFILL}},

        { &hf_swils_ess_fctlr_vendor,
          {"Vendor Specific Flags", "swils.ess.capability.fctlr.vendor",
           FT_UINT32, BASE_HEX, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_ess_fcs_basic,
          {"Basic Configuration Services", "swils.ess.capability.fcs.basic",
           FT_BOOLEAN, 8, NULL, 0x1,
           NULL, HFILL}},

        { &hf_swils_ess_fcs_platform,
          {"Platform Configuration Services", "swils.ess.capability.fcs.platform",
           FT_BOOLEAN, 8, NULL, 0x2,
           NULL, HFILL}},

        { &hf_swils_ess_fcs_topology,
          {"Topology Discovery Services", "swils.ess.capability.fcs.topology",
           FT_BOOLEAN, 8, NULL, 0x4,
           NULL, HFILL}},

        { &hf_swils_ess_fcs_enhanced,
          {"Enhanced Configuration Services", "swils.ess.capability.fcs.enhanced",
           FT_BOOLEAN, 8, NULL, 0x8,
           NULL, HFILL}},

        { &hf_swils_ess_fzs_enh_supp,
          {"Enhanced Zoning Supported", "swils.ess.capability.fzs.ezonesupp",
           FT_BOOLEAN, 8, NULL, 0x1,
           NULL, HFILL}},

        { &hf_swils_ess_fzs_enh_ena,
          {"Enhanced Zoning Enabled", "swils.ess.capability.fzs.ezoneena",
           FT_BOOLEAN, 8, NULL, 0x2,
           NULL, HFILL}},

        { &hf_swils_ess_fzs_mr,
          {"Merge Control Setting", "swils.ess.capability.fzs.mr",
           FT_BOOLEAN, 8, NULL, 0x4,
           NULL, HFILL}},

        { &hf_swils_ess_fzs_defzone,
          {"Default Zone Setting", "swils.ess.capability.fzs.defzone",
           FT_BOOLEAN, 8, NULL, 0x8,
           NULL, HFILL}},

        { &hf_swils_ess_fzs_zsdb_supp,
          {"Zoneset Database Supported", "swils.ess.capability.fzs.zsdbsupp",
           FT_BOOLEAN, 8, NULL, 0x10,
           NULL, HFILL}},

        { &hf_swils_ess_fzs_zsdb_ena,
          {"Zoneset Database Enabled", "swils.ess.capability.fzs.zsdbena",
           FT_BOOLEAN, 8, NULL, 0x20,
           NULL, HFILL}},

        { &hf_swils_ess_fzs_adc_supp,
          {"Active Direct Command Supported", "swils.ess.capability.fzs.adcsupp",
           FT_BOOLEAN, 8, NULL, 0x40,
           NULL, HFILL}},

        { &hf_swils_ess_fzs_hardzone,
          {"Hard Zoning Supported", "swils.ess.capability.fzs.hardzone",
           FT_BOOLEAN, 8, NULL, 0x80,
           NULL, HFILL}},

        { &hf_swils_ess_cap_len,
          {"Length", "swils.ess.capability.length",
           FT_UINT8, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_ess_cap_t10,
          {"T10 Vendor ID", "swils.ess.capability.t10id",
           FT_STRING, BASE_NONE, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_ess_cap_vendorobj,
          {"Vendor-Specific Info", "swils.ess.capability.vendorobj",
           FT_BYTES, BASE_NONE, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_mrra_rev,
          {"Revision", "swils.mrra.revision",
           FT_UINT32, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_mrra_size,
          {"Merge Request Size", "swils.mrra.size",
           FT_UINT32, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_mrra_vendorid,
          {"Vendor ID", "swils.mrra.vendorid",
           FT_STRING, BASE_NONE, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_mrra_vendorinfo,
          {"Vendor-Specific Info", "swils.mrra.vendorinfo",
           FT_BYTES, BASE_NONE, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_mrra_reply,
          {"MRRA Response", "swils.mrra.reply",
           FT_UINT32, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_mrra_reply_size,
          {"Maximum Resources Available", "swils.mrra.replysize",
           FT_UINT32, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

        { &hf_swils_mrra_waittime,
          {"Waiting Period (secs)", "swils.mrra.waittime",
           FT_UINT32, BASE_DEC, NULL, 0x0,
           NULL, HFILL}},

      /* Generated from convert_proto_tree_add_text.pl */
      { &hf_swils_requested_domain_id, { "Requested Domain ID", "swils.requested_domain_id", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL }},
      { &hf_swils_granted_domain_id, { "Granted Domain ID", "swils.granted_domain_id", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL }},
      { &hf_swils_lsrh_lsr_age, { "LSR Age", "swils.lsr.age", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }},
      { &hf_swils_lsrh_options, { "Options", "swils.lsr.options", FT_UINT32, BASE_HEX, NULL, 0x0, NULL, HFILL }},
      { &hf_swils_lsrh_checksum, { "Checksum", "swils.lsr.checksum", FT_UINT16, BASE_HEX, NULL, 0x0, NULL, HFILL }},
      { &hf_swils_lsrh_lsr_length, { "LSR Length", "swils.lsr.length", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }},
      { &hf_swils_lsrec_number_of_links, { "Number of Links", "swils.lsr.number_of_links", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }},
      { &hf_swils_lsupdate_flags, { "Flags", "swils.lsupdate.flags", FT_UINT8, BASE_HEX, VALS(fc_swils_fspf_lsrflags_val), 0x0, NULL, HFILL }},
      { &hf_swils_lsupdate_num_of_lsrs, { "Num of LSRs", "swils.lsupdate.num_of_lsrs", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
      { &hf_swils_lsack_flags, { "Flags", "swils.lsack.flags", FT_UINT8, BASE_HEX, VALS(fc_swils_fspf_lsrflags_val), 0x0, NULL, HFILL }},
      { &hf_swils_lsack_num_of_lsr_headers, { "Num of LSR Headers", "swils.lsack.num_of_lsr_headers", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
      { &hf_swils_rscn_num_entries, { "Num Entries", "swils.rscn.num_entries", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
      { &hf_swils_zone_mbrflags, { "Flags", "swils.zone.flags", FT_UINT8, BASE_HEX, NULL, 0x0, NULL, HFILL }},
      { &hf_swils_zone_mbr_identifier_length, { "Identifier Length", "swils.zone.identifier_length", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL }},
      { &hf_swils_zone_num_members, { "4", "swils.zone.num_members", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
      { &hf_swils_zone_active_zoneset_length, { "Active ZoneSet Length", "swils.zone.active_zoneset_length", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }},
      { &hf_swils_zone_num_zoning_objects, { "Number of zoning objects", "swils.zone.num_zoning_objects", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
      { &hf_swils_zone_full_zone_set_length, { "Full Zone Set Length", "swils.zone.full_zone_set_length", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
      { &hf_swils_zone_vendor_unique, { "Vendor Unique", "swils.zone.vendor_unique", FT_UINT8, BASE_HEX, NULL, 0x0, NULL, HFILL }},
      { &hf_swils_domain_id_list_length, { "Domain ID List Length", "swils.aca.domain_id_list_length", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }},
      { &hf_swils_sfc_zoneset_length, { "ZoneSet Length", "swils.sfc.zoneset_length", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }},
      { &hf_swils_esc_payload_length, { "Payload Length", "swils.esc.payload_length", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }},
    };

    static gint *ett[] = {
        &ett_fcswils,
        &ett_fcswils_swacc,
        &ett_fcswils_swrjt,
        &ett_fcswils_elp,
        &ett_fcswils_efp,
        &ett_fcswils_efplist,
        &ett_fcswils_dia,
        &ett_fcswils_rdi,
        &ett_fcswils_fspfhdr,
        &ett_fcswils_hlo,
        &ett_fcswils_lsrec,
        &ett_fcswils_lsrechdr,
        &ett_fcswils_ldrec,
        &ett_fcswils_lsu,
        &ett_fcswils_lsa,
        &ett_fcswils_bf,
        &ett_fcswils_rcf,
        &ett_fcswils_rscn,
        &ett_fcswils_rscn_dev,
        &ett_fcswils_drlir,
        &ett_fcswils_mr,
        &ett_fcswils_zoneobjlist,
        &ett_fcswils_zoneobj,
        &ett_fcswils_zonembr,
        &ett_fcswils_aca,
        &ett_fcswils_rca,
        &ett_fcswils_sfc,
        &ett_fcswils_ufc,
        &ett_fcswils_esc,
        &ett_fcswils_esc_pdesc,
        &ett_fcswils_ieinfo,
        &ett_fcswils_capinfo
    };

    static ei_register_info ei[] = {
        { &ei_swils_efp_record_len, { "swils.efp.recordlen.zero", PI_UNDECODED, PI_NOTE, "Record length is zero", EXPFILL }},
        { &ei_swils_no_exchange, { "swils.no_exchange", PI_UNDECODED, PI_WARN, "No record of Exchg. Unable to decode", EXPFILL }},
        { &ei_swils_zone_mbrid, { "swils.zone.mbrid.unknown_type", PI_PROTOCOL, PI_WARN, "Unknown member type format", EXPFILL }},
    };

    expert_module_t* expert_fcswils;

    proto_fcswils = proto_register_protocol("Fibre Channel SW_ILS", "FC-SWILS", "swils");

    proto_register_field_array(proto_fcswils, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
    expert_fcswils = expert_register_protocol(proto_fcswils);
    expert_register_field_array(expert_fcswils, ei, array_length(ei));
    register_init_routine(&fcswils_init_protocol);
    register_cleanup_routine(&fcswils_cleanup_protocol);
}

void
proto_reg_handoff_fcswils(void)
{
    dissector_handle_t swils_handle;

    swils_handle = create_dissector_handle(dissect_fcswils, proto_fcswils);
    dissector_add_uint("fc.ftype", FC_FTYPE_SWILS, swils_handle);

    fcsp_handle = find_dissector_add_dependency("fcsp", proto_fcswils);
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
