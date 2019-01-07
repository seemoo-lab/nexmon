/* packet-fc-sb3.c
 * Routines for Fibre Channel Single Byte Protocol (SBCCS); used in FICON.
 * This decoder is for FC-SB3 version 1.4
 * Copyright 2003, Dinesh G Dutt <ddutt@cisco.com>
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
#include "packet-fc.h"
#include "packet-fcsb3.h"

void proto_register_fcsbccs(void);
void proto_reg_handoff_fcsbccs(void);

/* Initialize the protocol and registered fields */
static int proto_fc_sbccs = -1;
static int hf_sbccs_chid = -1;
static int hf_sbccs_cuid = -1;
static int hf_sbccs_devaddr = -1;
static int hf_sbccs_ccw = -1;
static int hf_sbccs_token = -1;
static int hf_sbccs_dib_iucnt = -1;
static int hf_sbccs_dib_datacnt = -1;
static int hf_sbccs_dib_ccw_cmd = -1;
static int hf_sbccs_dib_ccw_cnt = -1;
static int hf_sbccs_dib_residualcnt = -1;
static int hf_sbccs_dib_qtuf = -1;
static int hf_sbccs_dib_qtu = -1;
static int hf_sbccs_dib_dtuf = -1;
static int hf_sbccs_dib_dtu = -1;
static int hf_sbccs_dib_ctlfn = -1;
static int hf_sbccs_lrc = -1;
static int hf_sbccs_dib_iupacing = -1;
static int hf_sbccs_dev_xcp_code = -1;
static int hf_sbccs_prg_pth_errcode = -1;
static int hf_sbccs_prg_rsp_errcode = -1;
static int hf_sbccs_dib_ctccntr = -1;
static int hf_sbccs_dib_lprcode = -1;
static int hf_sbccs_dib_tin_imgid_cnt = -1;
static int hf_sbccs_dib_lrjcode = -1;
static int hf_sbccs_dib_ioprio = -1;
static int hf_sbccs_dib_linkctlfn = -1;
static int hf_sbccs_iui = -1;
static int hf_sbccs_iui_as = -1;
static int hf_sbccs_iui_es = -1;
static int hf_sbccs_iui_val = -1;
static int hf_sbccs_dhflags = -1;
static int hf_sbccs_dhflags_end = -1;
static int hf_sbccs_dhflags_chaining = -1;
static int hf_sbccs_dhflags_earlyend = -1;
static int hf_sbccs_dhflags_nocrc = -1;
static int hf_sbccs_dib_ccw_flags = -1;
static int hf_sbccs_dib_ccw_flags_cd = -1;
static int hf_sbccs_dib_ccw_flags_cc = -1;
static int hf_sbccs_dib_ccw_flags_sli = -1;
static int hf_sbccs_dib_ccw_flags_crr = -1;
static int hf_sbccs_dib_cmdflags = -1;
static int hf_sbccs_dib_cmdflags_du = -1;
static int hf_sbccs_dib_cmdflags_coc = -1;
static int hf_sbccs_dib_cmdflags_syr = -1;
static int hf_sbccs_dib_cmdflags_rex = -1;
static int hf_sbccs_dib_cmdflags_sss = -1;
static int hf_sbccs_dib_statusflags = -1;
static int hf_sbccs_dib_statusflags_ffc = -1;
static int hf_sbccs_dib_statusflags_ci = -1;
static int hf_sbccs_dib_statusflags_cr = -1;
static int hf_sbccs_dib_statusflags_lri = -1;
static int hf_sbccs_dib_statusflags_rv = -1;
static int hf_sbccs_dib_status = -1;
static int hf_sbccs_dib_status_attention = -1;
static int hf_sbccs_dib_status_modifier = -1;
static int hf_sbccs_dib_status_cue = -1;
static int hf_sbccs_dib_status_busy = -1;
static int hf_sbccs_dib_status_channelend = -1;
static int hf_sbccs_dib_status_deviceend = -1;
static int hf_sbccs_dib_status_unit_check = -1;
static int hf_sbccs_dib_status_unit_exception = -1;
static int hf_sbccs_dib_ctlparam = -1;
static int hf_sbccs_dib_ctlparam_rc = -1;
static int hf_sbccs_dib_ctlparam_ru = -1;
static int hf_sbccs_dib_ctlparam_ro = -1;
static int hf_sbccs_dib_linkctlinfo = -1;
static int hf_sbccs_dib_linkctlinfo_ctcconn = -1;
static int hf_sbccs_dib_linkctlinfo_ecrcg = -1;
static int hf_sbccs_logical_path = -1;

/* Initialize the subtree pointers */
static gint ett_fc_sbccs = -1;
static gint ett_sbccs_iui = -1;
static gint ett_sbccs_dhflags = -1;
static gint ett_sbccs_dib_ccw_flags = -1;
static gint ett_sbccs_dib_cmdflags = -1;
static gint ett_sbccs_dib_statusflags = -1;
static gint ett_sbccs_dib_status = -1;
static gint ett_sbccs_dib_ctlparam = -1;
static gint ett_sbccs_dib_linkctlinfo = -1;

#if 0
typedef struct {
    guint32 conv_id;
    guint32 task_id;
} sb3_task_id_t;
#endif

static const value_string fc_sbccs_iu_val[] = {
    {FC_SBCCS_IU_DATA,            "Data"},
    {FC_SBCCS_IU_CMD_HDR,         "Command Header"},
    {FC_SBCCS_IU_STATUS,          "Status"},
    {FC_SBCCS_IU_CTL,             "Control"},
    {FC_SBCCS_IU_CMD_DATA,        "Command Header & Data"},
    {FC_SBCCS_IU_CMD_LINK_CTL,    "Link Control"},
    {0x6,                         "Reserved"},
    {0x7,                         "Reserved"},
    {0x0,                         NULL},
};

static const value_string fc_sbccs_dib_cmd_val[] = {
    { 0, "Reserved"},
    { 1, "Write"},
    { 2, "Read"},
    { 3, "Control"},
    { 4, "Sense"},
    { 5, "Write (Modifier)"},
    { 6, "Read (Modifier)"},
    { 7, "Control (Modifier)"},
    { 8, "Reserved"},
    { 9, "Write (Modifier)"},
    {10, "Read (Modifier)"},
    {11, "Control (Modifier)"},
    {12, "Read Backward"},
    {13, "Write (Modifier)"},
    {14, "Read (Modifier)"},
    {15, "Control (Modifier)"},
    {0, NULL},
};

static const value_string fc_sbccs_dib_ctl_fn_val[] = {
    {FC_SBCCS_CTL_FN_CTL_END,   "Control End"},
    {FC_SBCCS_CTL_FN_CMD_RSP,   "Command Response"},
    {FC_SBCCS_CTL_FN_STK_STS,   "Stack Status"},
    {FC_SBCCS_CTL_FN_CANCEL,    "Cancel"},
    {FC_SBCCS_CTL_FN_SYS_RST,   "System Reset"},
    {FC_SBCCS_CTL_FN_SEL_RST,   "Selective Reset"},
    {FC_SBCCS_CTL_FN_REQ_STS,   "Request Status"},
    {FC_SBCCS_CTL_FN_DEV_XCP,   "Device Level Exception"},
    {FC_SBCCS_CTL_FN_STS_ACC,   "Status Accepted"},
    {FC_SBCCS_CTL_FN_DEV_ACK,   "Device-Level Ack"},
    {FC_SBCCS_CTL_FN_PRG_PTH,   "Purge Path"},
    {FC_SBCCS_CTL_FN_PRG_RSP,   "Purge Path Response"},
    {0, NULL},
};

static const value_string fc_sbccs_dib_dev_xcpcode_val[] = {
    {1, "Address Exception"},
    {0, NULL},
};

static const value_string fc_sbccs_dib_purge_path_err_val[] = {
    { 0, "Error Code Xfer Not Supported"},
    { 1, "SB-3 Protocol Timeout"},
    { 2, "SB-3 Link Failure"},
    { 3, "Reserved"},
    { 4, "SB-3 Offline Condition"},
    { 5, "FC-PH Link Failure"},
    { 6, "SB-3 Length Error"},
    { 7, "LRC Error"},
    { 8, "SB-3 CRC Error"},
    { 9, "IU Count Error"},
    {10, "SB-3 Link Level Protocol Error"},
    {11, "SB-3 Device Level Protocol Error"},
    {12, "Receive ABTS"},
    {13, "Cancel Function Timeout"},
    {14, "Abnormal Termination of Xchg"},
    {15, "Reserved"},
    {0,  NULL},
};

static const value_string fc_sbccs_dib_purge_path_rsp_err_val[] = {
    { 0, "No Errors"},
    { 1, "SB-3 Protocol Timeout"},
    { 2, "SB-3 Link Failure"},
    { 3, "Logical Path Timeout Error"},
    { 4, "SB-3 Offline Condition"},
    { 5, "FC-PH Link Failure"},
    { 6, "SB-3 Length Error"},
    { 7, "LRC Error"},
    { 8, "SB-3 CRC Error"},
    { 9, "IU Count Error"},
    {10, "SB-3 Link Level Protocol Error"},
    {11, "SB-3 Device Level Protocol Error"},
    {12, "Receive ABTS"},
    {13, "Reserved"},
    {14, "Abnormal Termination of Xchg"},
    {15, "Logical Path Not Estd"},
    {16, "Test Init Result Error"},
    {0,  NULL},
};

static const value_string fc_sbccs_dib_link_ctl_fn_val[] = {
    {FC_SBCCS_LINK_CTL_FN_ELP,  "ELP"},
    {FC_SBCCS_LINK_CTL_FN_RLP,  "RLP"},
    {FC_SBCCS_LINK_CTL_FN_TIN,  "TIN"},
    {FC_SBCCS_LINK_CTL_FN_LPE,  "LPE"},
    {FC_SBCCS_LINK_CTL_FN_LPR,  "LPR"},
    {FC_SBCCS_LINK_CTL_FN_TIR,  "TIR"},
    {FC_SBCCS_LINK_CTL_FN_LRJ,  "LRJ"},
    {FC_SBCCS_LINK_CTL_FN_LBY,  "LBY"},
    {FC_SBCCS_LINK_CTL_FN_LACK, "LACK"},
    {0, NULL},
};

static const value_string fc_sbccs_dib_lpr_errcode_val[] = {
    {0x0, "Response to RLP"},
    {0x1, "Optional Features Conflict"},
    {0x2, "Out of Resources"},
    {0x3, "Device Init In Progress"},
    {0x4, "No CU Image"},
    {0x0, NULL},
};

static const value_string fc_sbccs_dib_lrj_errcode_val[] = {
    {0x6, "Logical Path Not Estd"},
    {0x9, "Protocol Error"},
    {0x0, NULL},
};

static void
dissect_iui_flags (proto_tree *parent_tree, tvbuff_t *tvb, int offset, guint16 flags)
{
    static const int * iui_flags[] = {
        &hf_sbccs_iui_as,
        &hf_sbccs_iui_es,
        &hf_sbccs_iui_val,
        NULL
    };

    proto_tree_add_bitmask_value_with_flags(parent_tree, tvb, offset, hf_sbccs_iui,
                                   ett_sbccs_iui, iui_flags, flags, BMT_NO_FALSE|BMT_NO_TFS);
}

static void
dissect_linkctlinfo (proto_tree *parent_tree, tvbuff_t *tvb, int offset, guint16 flags)
{
    static const int * linkctlinfo_flags[] = {
        &hf_sbccs_dib_linkctlinfo_ctcconn,
        &hf_sbccs_dib_linkctlinfo_ecrcg,
        NULL
    };

    proto_tree_add_bitmask_value_with_flags(parent_tree, tvb, offset, hf_sbccs_dib_linkctlinfo,
                                   ett_sbccs_dib_linkctlinfo, linkctlinfo_flags, flags, BMT_NO_FALSE|BMT_NO_TFS);
}


static void
dissect_dh_flags (proto_tree *parent_tree, tvbuff_t *tvb, int offset, guint16 flags)
{
    static const int * dh_flags[] = {
        &hf_sbccs_dhflags_end,
        &hf_sbccs_dhflags_chaining,
        &hf_sbccs_dhflags_earlyend,
        &hf_sbccs_dhflags_nocrc,
        NULL
    };

    proto_tree_add_bitmask_value_with_flags(parent_tree, tvb, offset, hf_sbccs_dhflags,
                                   ett_sbccs_dhflags, dh_flags, flags, BMT_NO_FALSE|BMT_NO_TFS);
}


static void
dissect_ccw_flags (proto_tree *parent_tree, tvbuff_t *tvb, int offset, guint8 flags)
{
    static const int * ccw_flags[] = {
        &hf_sbccs_dib_ccw_flags_cd,
        &hf_sbccs_dib_ccw_flags_cc,
        &hf_sbccs_dib_ccw_flags_sli,
        &hf_sbccs_dib_ccw_flags_crr,
        NULL
    };

    proto_tree_add_bitmask_value_with_flags(parent_tree, tvb, offset, hf_sbccs_dib_ccw_flags,
                                   ett_sbccs_dib_ccw_flags, ccw_flags, flags, BMT_NO_FALSE|BMT_NO_TFS);
}


static void
dissect_cmd_flags (proto_tree *parent_tree, tvbuff_t *tvb, int offset, guint8 flags)
{
    static const int * cmd_flags[] = {
        &hf_sbccs_dib_cmdflags_du,
        &hf_sbccs_dib_cmdflags_coc,
        &hf_sbccs_dib_cmdflags_syr,
        &hf_sbccs_dib_cmdflags_rex,
        &hf_sbccs_dib_cmdflags_sss,
        NULL
    };

    proto_tree_add_bitmask_value_with_flags(parent_tree, tvb, offset, hf_sbccs_dib_cmdflags,
                                   ett_sbccs_dib_cmdflags, cmd_flags, flags, BMT_NO_FALSE|BMT_NO_TFS);
}

static const value_string status_ffc_val[] = {
    { 0, "" },
    { 1, "FFC:Queuing Information Valid" },
    { 2, "FFC:Resetting Event" },
    { 0, NULL }
};


static void
dissect_status_flags (proto_tree *parent_tree, tvbuff_t *tvb, int offset, guint8 flags)
{
    static const int * status_flags[] = {
        &hf_sbccs_dib_statusflags_ffc,
        &hf_sbccs_dib_statusflags_ci,
        &hf_sbccs_dib_statusflags_cr,
        &hf_sbccs_dib_statusflags_lri,
        &hf_sbccs_dib_statusflags_rv,
        NULL
    };

    proto_tree_add_bitmask_value_with_flags(parent_tree, tvb, offset, hf_sbccs_dib_statusflags,
                                   ett_sbccs_dib_statusflags, status_flags, flags, BMT_NO_FALSE|BMT_NO_TFS);
}


static void
dissect_status (packet_info *pinfo, proto_tree *parent_tree, tvbuff_t *tvb, int offset, guint8 flags)
{
    static const int * status_flags[] = {
        &hf_sbccs_dib_status_attention,
        &hf_sbccs_dib_status_modifier,
        &hf_sbccs_dib_status_cue,
        &hf_sbccs_dib_status_busy,
        &hf_sbccs_dib_status_channelend,
        &hf_sbccs_dib_status_deviceend,
        &hf_sbccs_dib_status_unit_check,
        &hf_sbccs_dib_status_unit_exception,
        NULL
    };
    proto_tree_add_bitmask_value_with_flags(parent_tree, tvb, offset, hf_sbccs_dib_status,
                                   ett_sbccs_dib_status, status_flags, flags, BMT_NO_FALSE|BMT_NO_TFS);

    if (flags & 0x80) {
        col_append_str(pinfo->cinfo, COL_INFO, "  Attention");
    }

    if (flags & 0x40) {
        col_append_str(pinfo->cinfo, COL_INFO, "  Status Modifier");
    }

    if (flags & 0x20) {
        col_append_str(pinfo->cinfo, COL_INFO, "  Control-Unit End");
    }

    if (flags & 0x10) {
        col_append_str(pinfo->cinfo, COL_INFO, "  Busy");
    }
    if (flags & 0x08) {
        col_append_str(pinfo->cinfo, COL_INFO, "  Channel End");
    }

    if (flags & 0x04) {
        col_append_str(pinfo->cinfo, COL_INFO, "  Device End");
    }

    if (flags & 0x02) {
        col_append_str(pinfo->cinfo, COL_INFO, "  Unit Check");
    }

    if (flags & 0x01) {
        col_append_str(pinfo->cinfo, COL_INFO, "  Unit Exception");
    }
}


static void
dissect_sel_rst_param (proto_tree *parent_tree, tvbuff_t *tvb, int offset, guint32 flags)
{
    static const int * rst_param_flags[] = {
        &hf_sbccs_dib_ctlparam_rc,
        &hf_sbccs_dib_ctlparam_ru,
        &hf_sbccs_dib_ctlparam_ro,
        NULL
    };

    proto_tree_add_bitmask_value_with_flags(parent_tree, tvb, offset, hf_sbccs_dib_ctlparam,
                                   ett_sbccs_dib_ctlparam, rst_param_flags, flags, BMT_NO_FALSE|BMT_NO_TFS);
}

static void get_fc_sbccs_conv_data (tvbuff_t *tvb, guint offset,
                                    guint16 *ch_cu_id, guint16 *dev_addr,
                                    guint16 *ccw)
{
    *ch_cu_id  = *dev_addr = *ccw = 0;

    *ch_cu_id  = (tvb_get_guint8 (tvb, offset+1)) << 8;
    *ch_cu_id |= tvb_get_guint8 (tvb, offset+3);
    *dev_addr  = tvb_get_ntohs (tvb, offset+4);
    *ccw       = tvb_get_ntohs (tvb, offset+10);
}

/* Decode both the SB-3 and basic IU header */
static void
dissect_fc_sbccs_sb3_iu_hdr (tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                             guint offset)
{
    proto_tree *sb3hdr_tree;
    proto_tree *iuhdr_tree;
    guint8      iui, dhflags;
    guint       type;

    /* Decode the basic SB3 and IU header and determine type of frame */
    type = get_fc_sbccs_iu_type (tvb, offset);

    col_add_str (pinfo->cinfo, COL_INFO, val_to_str (type, fc_sbccs_iu_val,
                                                         "0x%x"));

    if (tree) {
        /* Dissect SB3 header first */
        sb3hdr_tree = proto_tree_add_subtree(tree, tvb, offset, FC_SBCCS_SB3_HDR_SIZE,
                                     ett_fc_sbccs, NULL, "SB-3 Header");

        proto_tree_add_item (sb3hdr_tree, hf_sbccs_chid, tvb, offset+1, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item (sb3hdr_tree, hf_sbccs_cuid, tvb, offset+3, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item (sb3hdr_tree, hf_sbccs_devaddr, tvb, offset+4, 2, ENC_BIG_ENDIAN);

        /* Dissect IU Header */
        iuhdr_tree = proto_tree_add_subtree(tree, tvb, offset + FC_SBCCS_SB3_HDR_SIZE,
                                     FC_SBCCS_IU_HDR_SIZE, ett_fc_sbccs, NULL, "IU Header");
        offset += FC_SBCCS_SB3_HDR_SIZE;

        iui = tvb_get_guint8 (tvb, offset);
        dissect_iui_flags(iuhdr_tree, tvb, offset, iui);

        dhflags = tvb_get_guint8 (tvb, offset+1);
        dissect_dh_flags(iuhdr_tree, tvb, offset+1, dhflags);

        proto_tree_add_item (iuhdr_tree, hf_sbccs_ccw, tvb, offset+2, 2, ENC_BIG_ENDIAN);
        proto_tree_add_item (iuhdr_tree, hf_sbccs_token, tvb, offset+5, 3, ENC_BIG_ENDIAN);
    }
}

static void dissect_fc_sbccs_dib_data_hdr (tvbuff_t *tvb,
                                           packet_info *pinfo _U_,
                                           proto_tree *tree, guint offset)
{
    if (tree) {
        proto_tree_add_item (tree, hf_sbccs_dib_iucnt, tvb, offset+9, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item (tree, hf_sbccs_dib_datacnt, tvb, offset+10, 2, ENC_BIG_ENDIAN);
        proto_tree_add_item (tree, hf_sbccs_lrc, tvb, offset+12, 4, ENC_BIG_ENDIAN);
    }
}

static void dissect_fc_sbccs_dib_cmd_hdr (tvbuff_t *tvb, packet_info *pinfo,
                                          proto_tree *tree, guint offset)
{
    guint8 flags;

    col_append_fstr (pinfo->cinfo, COL_INFO,
                         ": %s", val_to_str (tvb_get_guint8 (tvb, offset),
                                             fc_sbccs_dib_cmd_val,
                                             "0x%x"));

    if (tree) {
        proto_tree_add_item (tree, hf_sbccs_dib_ccw_cmd, tvb, offset, 1, ENC_BIG_ENDIAN);

        flags = tvb_get_guint8 (tvb, offset+1);
        dissect_ccw_flags(tree, tvb, offset+1, flags);

        proto_tree_add_item (tree, hf_sbccs_dib_ccw_cnt, tvb, offset+2, 2, ENC_BIG_ENDIAN);
        proto_tree_add_item (tree, hf_sbccs_dib_ioprio, tvb, offset+5, 1, ENC_BIG_ENDIAN);

        flags = tvb_get_guint8 (tvb, offset+7);
        dissect_cmd_flags(tree, tvb, offset+7, flags);

        proto_tree_add_item (tree, hf_sbccs_dib_iucnt, tvb, offset+9, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item (tree, hf_sbccs_dib_datacnt, tvb, offset+10, 2, ENC_BIG_ENDIAN);
        proto_tree_add_item (tree, hf_sbccs_lrc, tvb, offset+12, 4, ENC_BIG_ENDIAN);

    }
}

static void dissect_fc_sbccs_dib_status_hdr (tvbuff_t *tvb, packet_info *pinfo,
                                             proto_tree *tree, guint offset)
{
    guint8    flags;
    gboolean  rv_valid, qparam_valid;
    tvbuff_t *next_tvb;
    guint16   supp_status_cnt = 0;

    if (tree) {
        flags = tvb_get_guint8 (tvb, offset);
        rv_valid = flags & 0x1; /* if residual count is valid */
        qparam_valid = (((flags & 0xE0) >> 5) == 0x1); /* From the FFC field */
        dissect_status_flags(tree, tvb, offset, flags);

        flags = tvb_get_guint8 (tvb, offset+1);
        dissect_status(pinfo, tree, tvb, offset+1, flags);

        if (rv_valid) {
            proto_tree_add_item (tree, hf_sbccs_dib_residualcnt, tvb, offset+2,
                                 2, ENC_BIG_ENDIAN);
        }
        else {
            proto_tree_add_item (tree, hf_sbccs_dib_iupacing, tvb, offset+3,
                                 1, ENC_BIG_ENDIAN);
        }

        if (qparam_valid) {
            proto_tree_add_item (tree, hf_sbccs_dib_qtuf, tvb, offset+4, 1, ENC_BIG_ENDIAN);
            proto_tree_add_item (tree, hf_sbccs_dib_qtu, tvb, offset+4, 2, ENC_BIG_ENDIAN);
        }

        proto_tree_add_item (tree, hf_sbccs_dib_dtuf, tvb, offset+6, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item (tree, hf_sbccs_dib_dtu, tvb, offset+6, 2, ENC_BIG_ENDIAN);

        proto_tree_add_item (tree, hf_sbccs_dib_iucnt, tvb, offset+9, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item (tree, hf_sbccs_dib_datacnt, tvb, offset+10, 2, ENC_BIG_ENDIAN);
        proto_tree_add_item (tree, hf_sbccs_lrc, tvb, offset+12, 4, ENC_BIG_ENDIAN);
    }

    supp_status_cnt = tvb_get_ntohs (tvb, offset+10);

    if (supp_status_cnt) {
        next_tvb = tvb_new_subset_remaining (tvb, offset+FC_SBCCS_DIB_LRC_HDR_SIZE);
        call_data_dissector(next_tvb, pinfo, tree);
    }
}

static void dissect_fc_sbccs_dib_ctl_hdr (tvbuff_t *tvb, packet_info *pinfo,
                                          proto_tree *tree, guint offset)
{
    guint8 ctlfn;

    ctlfn = tvb_get_guint8 (tvb, offset);
    col_append_fstr (pinfo->cinfo, COL_INFO,
                         ": %s",
                         val_to_str (ctlfn,
                                     fc_sbccs_dib_ctl_fn_val,
                                     "0x%x"));

    if (tree) {
        proto_tree_add_item (tree, hf_sbccs_dib_ctlfn, tvb, offset, 1, ENC_BIG_ENDIAN);

        /* Control Function Parameter is to be interpreted in some cases */
        switch (ctlfn) {
        case FC_SBCCS_CTL_FN_SEL_RST:
            dissect_sel_rst_param(tree, tvb, offset+1, tvb_get_ntoh24 (tvb, offset+1));
            break;
        case FC_SBCCS_CTL_FN_DEV_XCP:
            proto_tree_add_item (tree, hf_sbccs_dev_xcp_code, tvb, offset+1,
                                 1, ENC_BIG_ENDIAN);
            break;
        case FC_SBCCS_CTL_FN_PRG_PTH:
            proto_tree_add_item (tree, hf_sbccs_prg_pth_errcode, tvb, offset+1,
                                 1, ENC_BIG_ENDIAN);
            break;
        default:
            proto_tree_add_item (tree, hf_sbccs_dib_ctlparam, tvb, offset+1,
                                 3, ENC_BIG_ENDIAN);
            break;
        }

        proto_tree_add_item (tree, hf_sbccs_dib_iucnt, tvb, offset+9, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item (tree, hf_sbccs_dib_datacnt, tvb, offset+10, 2, ENC_BIG_ENDIAN);
        proto_tree_add_item (tree, hf_sbccs_lrc, tvb, offset+12, 4, ENC_BIG_ENDIAN);

        if (ctlfn == FC_SBCCS_CTL_FN_PRG_RSP) {
            /* Need to decode the LESBs */
            proto_tree_add_item (tree, hf_sbccs_prg_rsp_errcode, tvb, offset+60,
                                 1, ENC_BIG_ENDIAN);
        }
    }
}

static void dissect_fc_sbccs_dib_link_hdr (tvbuff_t *tvb, packet_info *pinfo,
                                           proto_tree *tree, guint offset)
{
    guint8  link_ctl;
    guint16 ctl_info;
    guint   link_payload_len, i;

    col_append_fstr (pinfo->cinfo, COL_INFO,
                         ": %s",
                         val_to_str (tvb_get_guint8 (tvb, offset+1),
                                     fc_sbccs_dib_link_ctl_fn_val,
                                     "0x%x"));

    if (tree) {
        link_ctl = tvb_get_guint8 (tvb, offset+1);
        proto_tree_add_item (tree, hf_sbccs_dib_linkctlfn, tvb, offset+1, 1, ENC_BIG_ENDIAN);

        ctl_info = tvb_get_ntohs (tvb, offset+2);
        switch (link_ctl) {
        case FC_SBCCS_LINK_CTL_FN_ELP:
        case FC_SBCCS_LINK_CTL_FN_LPE:
            dissect_linkctlinfo(tree, tvb, offset+2, ctl_info);
            break;
        case FC_SBCCS_LINK_CTL_FN_LPR:
            proto_tree_add_item (tree, hf_sbccs_dib_lprcode, tvb, offset+2, 1,
                                 ENC_BIG_ENDIAN);
            break;
        case FC_SBCCS_LINK_CTL_FN_TIN:
            proto_tree_add_item (tree, hf_sbccs_dib_tin_imgid_cnt, tvb,
                                 offset+3, 1, ENC_BIG_ENDIAN);
            break;
        case FC_SBCCS_LINK_CTL_FN_TIR:
            proto_tree_add_item (tree, hf_sbccs_dib_tin_imgid_cnt, tvb,
                                 offset+3, 1, ENC_BIG_ENDIAN);
            break;
        case FC_SBCCS_LINK_CTL_FN_LRJ:
            proto_tree_add_item (tree, hf_sbccs_dib_lrjcode, tvb, offset+2,
                                 1, ENC_BIG_ENDIAN);
            break;
        default:
            /* Do Nothing */
            break;
        }

        proto_tree_add_item (tree, hf_sbccs_dib_ctccntr, tvb, offset+4, 2, ENC_BIG_ENDIAN);
        proto_tree_add_item (tree, hf_sbccs_dib_iucnt, tvb, offset+9, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item (tree, hf_sbccs_dib_datacnt, tvb, offset+10, 2, ENC_BIG_ENDIAN);
        proto_tree_add_item (tree, hf_sbccs_lrc, tvb, offset+12, 4, ENC_BIG_ENDIAN);

        if (link_ctl == FC_SBCCS_LINK_CTL_FN_TIR) {
            link_payload_len = tvb_get_ntohs (tvb, offset+10);
            i = 0;
            offset += 16;

            while (i < link_payload_len) {
                proto_tree_add_bytes_format(tree, hf_sbccs_logical_path, tvb, offset, 4,
                                     NULL, "Logical Paths %d-%d: %s",
                                     i*8, ((i+4)*8) - 1,
                                     tvb_bytes_to_str_punct(wmem_packet_scope(), tvb, offset, 4, ':'));
                i += 4;
                offset += 4;
            }
        }
    }
}

static int dissect_fc_sbccs (tvbuff_t *tvb, packet_info *pinfo,
                              proto_tree *tree, void* data _U_)
{
    guint8          type;
    guint16         ch_cu_id, dev_addr, ccw;
    guint           offset   = 0;
    proto_item     *ti;
    proto_tree     *sb3_tree = NULL;
    proto_tree     *dib_tree = NULL;
    tvbuff_t       *next_tvb;
    conversation_t *conversation;
#if 0
    sb3_task_id_t   task_key;
#endif

    /* Make entries in Protocol column and Info column on summary display */
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "FC-SB3");

    /* Decode the basic SB3 and IU header and determine type of frame */
    type = get_fc_sbccs_iu_type (tvb, offset);
    get_fc_sbccs_conv_data (tvb, offset, &ch_cu_id, &dev_addr, &ccw);

    col_add_str (pinfo->cinfo, COL_INFO, val_to_str (type, fc_sbccs_iu_val,
                                                         "0x%x"));

    /* Retrieve conversation state to determine expected payload */
    conversation = find_conversation (pinfo->num, &pinfo->src, &pinfo->dst,
                                      PT_SBCCS, ch_cu_id, dev_addr, 0);

    if (conversation) {
#if 0
        task_key.conv_id = conversation->index;
        task_key.task_id = ccw;
#endif
    }
    else if ((type == FC_SBCCS_IU_CMD_HDR) ||
             (type != FC_SBCCS_IU_CMD_DATA)) {
#if 0
        conversation =
#endif
                       conversation_new (pinfo->num, &pinfo->src, &pinfo->dst,
                                         PT_SBCCS, ch_cu_id, dev_addr, 0);
#if 0
        task_key.conv_id = conversation->index;
        task_key.task_id = ccw;
#endif
    }

    if (tree) {
        ti = proto_tree_add_protocol_format (tree, proto_fc_sbccs, tvb, 0, -1,
                                             "FC-SB3");
        sb3_tree = proto_item_add_subtree (ti, ett_fc_sbccs);

        dissect_fc_sbccs_sb3_iu_hdr (tvb, pinfo, sb3_tree, offset);
        offset += (FC_SBCCS_SB3_HDR_SIZE + FC_SBCCS_IU_HDR_SIZE);

        dib_tree = proto_tree_add_subtree(sb3_tree, tvb, offset,
                                  FC_SBCCS_DIB_LRC_HDR_SIZE, ett_fc_sbccs, NULL, "DIB Header");
    }
    else {
        offset += (FC_SBCCS_SB3_HDR_SIZE + FC_SBCCS_IU_HDR_SIZE);
    }

    switch (type) {
    case FC_SBCCS_IU_DATA:
        dissect_fc_sbccs_dib_data_hdr (tvb, pinfo, dib_tree, offset);
        break;
    case FC_SBCCS_IU_CMD_HDR:
    case FC_SBCCS_IU_CMD_DATA:
        dissect_fc_sbccs_dib_cmd_hdr (tvb, pinfo, dib_tree, offset);
        break;
    case FC_SBCCS_IU_STATUS:
        dissect_fc_sbccs_dib_status_hdr (tvb, pinfo, dib_tree, offset);
        break;
    case FC_SBCCS_IU_CTL:
        dissect_fc_sbccs_dib_ctl_hdr (tvb, pinfo, dib_tree, offset);
        break;
    case FC_SBCCS_IU_CMD_LINK_CTL:
        dissect_fc_sbccs_dib_link_hdr (tvb, pinfo, dib_tree, offset);
        break;
    default:
        next_tvb = tvb_new_subset_remaining (tvb, offset);
        call_data_dissector(next_tvb, pinfo, dib_tree);
        break;
    }

    if ((get_fc_sbccs_iu_type (tvb, 0) != FC_SBCCS_IU_CTL) &&
        (get_fc_sbccs_iu_type (tvb, 0) != FC_SBCCS_IU_CMD_LINK_CTL))  {
        next_tvb = tvb_new_subset_remaining (tvb, offset+FC_SBCCS_DIB_LRC_HDR_SIZE);
        call_data_dissector(next_tvb, pinfo, tree);
    }
    return tvb_captured_length(tvb);
}

/* Register the protocol with Wireshark */

/* this format is required because a script is used to build the C function
   that calls all the protocol registration.
*/

void
proto_register_fcsbccs (void)
{
    /* Setup list of header fields  See Section 1.6.1 for details*/
    static hf_register_info hf[] = {
        { &hf_sbccs_chid,
          { "Channel Image ID", "fcsb3.chid",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL}},

        { &hf_sbccs_cuid,
          { "Control Unit Image ID", "fcsb3.cuid",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL}},

        { &hf_sbccs_devaddr,
          { "Device Address", "fcsb3.devaddr",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL}},

        { &hf_sbccs_iui,
          { "Information Unit Identifier", "fcsb3.iui",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL}},

        { &hf_sbccs_dhflags,
          { "DH Flags", "fcsb3.dhflags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL}},

        { &hf_sbccs_ccw,
          { "CCW Number", "fcsb3.ccw",
            FT_UINT16, BASE_HEX, NULL, 0x0,
            NULL, HFILL}},

        { &hf_sbccs_token,
          { "Token", "fcsb3.token",
            FT_UINT24, BASE_DEC, NULL, 0x0,
            NULL, HFILL}},

        { &hf_sbccs_dib_iucnt,
          { "DIB IU Count", "fcsb3.iucnt",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL}},

        { &hf_sbccs_dib_datacnt,
          { "DIB Data Byte Count", "fcsb3.databytecnt",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL}},

        { &hf_sbccs_dib_ccw_cmd,
          { "CCW Command", "fcsb3.ccwcmd",
            FT_UINT8, BASE_HEX, VALS (fc_sbccs_dib_cmd_val), 0x0,
            NULL, HFILL}},

        { &hf_sbccs_dib_ccw_cnt,
          { "CCW Count", "fcsb3.ccwcnt",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL}},

        { &hf_sbccs_dib_ioprio,
          { "I/O Priority", "fcsb3.ioprio",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL}},

        { &hf_sbccs_dib_status,
          { "Status", "fcsb3.status",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL}},

        { &hf_sbccs_dib_residualcnt,
          { "Residual Count", "fcsb3.residualcnt",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL}},

        { &hf_sbccs_dib_iupacing,
          { "IU Pacing", "fcsb3.iupacing",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL}},

        { &hf_sbccs_dib_qtuf,
          { "Queue-Time Unit Factor", "fcsb3.qtuf",
            FT_UINT8, BASE_DEC, NULL, 0xF0,
            NULL, HFILL}},

        { &hf_sbccs_dib_qtu,
          { "Queue-Time Unit", "fcsb3.qtu",
            FT_UINT16, BASE_DEC, NULL, 0xFFF,
            NULL, HFILL}},

        { &hf_sbccs_dib_dtuf,
          { "Defer-Time Unit Function", "fcsb3.dtuf",
            FT_UINT8, BASE_DEC, NULL, 0xF0,
            NULL, HFILL}},

        { &hf_sbccs_dib_dtu,
          { "Defer-Time Unit", "fcsb3.dtu",
            FT_UINT16, BASE_DEC, NULL, 0xFFF,
            NULL, HFILL}},

        { &hf_sbccs_dib_ctlfn,
          { "Control Function", "fcsb3.ctlfn",
            FT_UINT8, BASE_HEX, VALS (fc_sbccs_dib_ctl_fn_val), 0x0,
            NULL, HFILL}},

        { &hf_sbccs_dib_linkctlfn,
          { "Link Control Function", "fcsb3.linkctlfn",
            FT_UINT8, BASE_HEX, VALS (fc_sbccs_dib_link_ctl_fn_val), 0x0,
            NULL, HFILL}},

        { &hf_sbccs_dib_ctccntr,
          { "CTC Counter", "fcsb3.ctccntr",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL}},

        { &hf_sbccs_lrc,
          { "LRC", "fcsb3.lrc",
            FT_UINT32, BASE_HEX, NULL, 0x0,
            NULL, HFILL}},

        { &hf_sbccs_dev_xcp_code,
          { "Device Level Exception Code", "fcsb3.dip.xcpcode",
            FT_UINT8, BASE_DEC, VALS (fc_sbccs_dib_dev_xcpcode_val), 0x0,
            NULL, HFILL}},

        { &hf_sbccs_prg_pth_errcode,
          { "Purge Path Error Code", "fcsb3.purgepathcode",
            FT_UINT8, BASE_DEC, VALS (fc_sbccs_dib_purge_path_err_val), 0x0,
            NULL, HFILL}},

        { &hf_sbccs_prg_rsp_errcode,
          { "Purge Path Response Error Code", "fcsb3.purgepathrspcode",
            FT_UINT8, BASE_DEC, VALS (fc_sbccs_dib_purge_path_rsp_err_val), 0x0,
            NULL, HFILL}},

        { &hf_sbccs_dib_lprcode,
          { "LPR Reason Code", "fcsb3.lprcode",
            FT_UINT8, BASE_DEC, VALS (fc_sbccs_dib_lpr_errcode_val), 0xF,
            NULL, HFILL}},

        { &hf_sbccs_dib_tin_imgid_cnt,
          { "TIN Image ID", "fcsb3.tinimageidcnt",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL}},

        { &hf_sbccs_dib_lrjcode,
          { "LRJ Reaspn Code", "fcsb3.lrjcode",
            FT_UINT8, BASE_HEX, VALS (fc_sbccs_dib_lrj_errcode_val), 0x7F,
            NULL, HFILL}},

        { &hf_sbccs_iui_as,
          { "AS", "fcsb3.iui.as",
            FT_BOOLEAN, 8, TFS(&tfs_set_notset), 0x10,
            NULL, HFILL}},

        { &hf_sbccs_iui_es,
          { "ES", "fcsb3.iui.es",
            FT_BOOLEAN, 8, TFS(&tfs_set_notset), 0x08,
            NULL, HFILL}},

        { &hf_sbccs_iui_val,
          { "Val", "fcsb3.iui.val",
            FT_UINT8, BASE_HEX, VALS(fc_sbccs_iu_val), 0x07,
            NULL, HFILL}},

        { &hf_sbccs_dhflags_end,
          { "End", "fcsb3.dhflags.end",
            FT_BOOLEAN, 8, TFS(&tfs_set_notset), 0x80,
            NULL, HFILL}},

        { &hf_sbccs_dhflags_chaining,
          { "Chaining", "fcsb3.dhflags.chaining",
            FT_BOOLEAN, 8, TFS(&tfs_set_notset), 0x10,
            NULL, HFILL}},

        { &hf_sbccs_dhflags_earlyend,
          { "Early End", "fcsb3.dhflags.earlyend",
            FT_BOOLEAN, 8, TFS(&tfs_set_notset), 0x08,
            NULL, HFILL}},

        { &hf_sbccs_dhflags_nocrc,
          { "No CRC", "fcsb3.dhflags.nocrc",
            FT_BOOLEAN, 8, TFS(&tfs_set_notset), 0x04,
            NULL, HFILL}},

        { &hf_sbccs_dib_ccw_flags,
          { "CCW Control Flags", "fcsb3.ccwflags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL}},

        { &hf_sbccs_dib_ccw_flags_cd,
          { "CD", "fcsb3.ccwflags.cd",
            FT_BOOLEAN, 8, TFS(&tfs_set_notset), 0x80,
            NULL, HFILL}},

        { &hf_sbccs_dib_ccw_flags_cc,
          { "CC", "fcsb3.ccwflags.cc",
            FT_BOOLEAN, 8, TFS(&tfs_set_notset), 0x40,
            NULL, HFILL}},

        { &hf_sbccs_dib_ccw_flags_sli,
          { "SLI", "fcsb3.ccwflags.sli",
            FT_BOOLEAN, 8, TFS(&tfs_set_notset), 0x20,
            NULL, HFILL}},

        { &hf_sbccs_dib_ccw_flags_crr,
          { "CRR", "fcsb3.ccwflags.crr",
            FT_BOOLEAN, 8, TFS(&tfs_set_notset), 0x08,
            NULL, HFILL}},

        { &hf_sbccs_dib_cmdflags,
          { "Command Flags", "fcsb3.cmdflags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL}},

        { &hf_sbccs_dib_cmdflags_du,
          { "DU", "fcsb3.cmdflags.du",
            FT_BOOLEAN, 8, TFS(&tfs_set_notset), 0x10,
            NULL, HFILL}},

        { &hf_sbccs_dib_cmdflags_coc,
          { "COC", "fcsb3.cmdflags.coc",
            FT_BOOLEAN, 8, TFS(&tfs_set_notset), 0x08,
            NULL, HFILL}},

        { &hf_sbccs_dib_cmdflags_syr,
          { "SYR", "fcsb3.cmdflags.syr",
            FT_BOOLEAN, 8, TFS(&tfs_set_notset), 0x04,
            NULL, HFILL}},

        { &hf_sbccs_dib_cmdflags_rex,
          { "REX", "fcsb3.cmdflags.rex",
            FT_BOOLEAN, 8, TFS(&tfs_set_notset), 0x02,
            NULL, HFILL}},

        { &hf_sbccs_dib_cmdflags_sss,
          { "SSS", "fcsb3.cmdflags.sss",
            FT_BOOLEAN, 8, TFS(&tfs_set_notset), 0x01,
            NULL, HFILL}},

        { &hf_sbccs_dib_statusflags,
          { "Status Flags", "fcsb3.statusflags",
            FT_UINT8, BASE_HEX, NULL, 0,
            NULL, HFILL}},

        { &hf_sbccs_dib_statusflags_ffc,
          { "FFC", "fcsb3.statusflags.ffc",
            FT_UINT8, BASE_HEX, VALS(status_ffc_val), 0xE0,
            NULL, HFILL}},

        { &hf_sbccs_dib_statusflags_ci,
          { "CI", "fcsb3.statusflags.ci",
            FT_BOOLEAN, 8, TFS(&tfs_set_notset), 0x10,
            NULL, HFILL}},

        { &hf_sbccs_dib_statusflags_cr,
          { "CR", "fcsb3.statusflags.cr",
            FT_BOOLEAN, 8, TFS(&tfs_set_notset), 0x04,
            NULL, HFILL}},

        { &hf_sbccs_dib_statusflags_lri,
          { "LRI", "fcsb3.statusflags.lri",
            FT_BOOLEAN, 8, TFS(&tfs_set_notset), 0x02,
            NULL, HFILL}},

        { &hf_sbccs_dib_statusflags_rv,
          { "RV", "fcsb3.statusflags.rv",
            FT_BOOLEAN, 8, TFS(&tfs_set_notset), 0x01,
            NULL, HFILL}},

        { &hf_sbccs_dib_status_attention,
          { "Attention", "fcsb3.status.attention",
            FT_BOOLEAN, 8, TFS(&tfs_set_notset), 0x80,
            NULL, HFILL}},

        { &hf_sbccs_dib_status_modifier,
          { "Status Modifier", "fcsb3.status.modifier",
            FT_BOOLEAN, 8, TFS(&tfs_set_notset), 0x40,
            NULL, HFILL}},

        { &hf_sbccs_dib_status_cue,
          { "Control-Unit End", "fcsb3.status.cue",
            FT_BOOLEAN, 8, TFS(&tfs_set_notset), 0x20,
            NULL, HFILL}},

        { &hf_sbccs_dib_status_busy,
          { "Busy", "fcsb3.status.busy",
            FT_BOOLEAN, 8, TFS(&tfs_set_notset), 0x10,
            NULL, HFILL}},

        { &hf_sbccs_dib_status_channelend,
          { "Channel End", "fcsb3.status.channel_end",
            FT_BOOLEAN, 8, TFS(&tfs_set_notset), 0x08,
            NULL, HFILL}},

        { &hf_sbccs_dib_status_deviceend,
          { "Device End", "fcsb3.status.device_end",
            FT_BOOLEAN, 8, TFS(&tfs_set_notset), 0x04,
            NULL, HFILL}},

        { &hf_sbccs_dib_status_unit_check,
          { "Unit Check", "fcsb3.status.unit_check",
            FT_BOOLEAN, 8, TFS(&tfs_set_notset), 0x02,
            NULL, HFILL}},

        { &hf_sbccs_dib_status_unit_exception,
          { "Unit Exception", "fcsb3.status.unitexception",
            FT_BOOLEAN, 8, TFS(&tfs_set_notset), 0x01,
            NULL, HFILL}},

        { &hf_sbccs_dib_ctlparam,
          { "Control Parameters", "fcsb3.ctlparam",
            FT_UINT24, BASE_HEX, NULL, 0x0,
            NULL, HFILL}},

        { &hf_sbccs_dib_ctlparam_rc,
          { "RC", "fcsb3.ctlparam.rc",
            FT_BOOLEAN, 24, TFS(&tfs_set_notset), 0x80,
            NULL, HFILL}},

        { &hf_sbccs_dib_ctlparam_ru,
          { "RU", "fcsb3.ctlparam.ru",
            FT_BOOLEAN, 24, TFS(&tfs_set_notset), 0x10,
            NULL, HFILL}},

        { &hf_sbccs_dib_ctlparam_ro,
          { "RO", "fcsb3.ctlparam.ro",
            FT_BOOLEAN, 24, TFS(&tfs_set_notset), 0x08,
            NULL, HFILL}},

        { &hf_sbccs_dib_linkctlinfo,
          { "Link Control Information", "fcsb3.linkctlinfo",
            FT_UINT16, BASE_HEX, NULL, 0x0,
            NULL, HFILL}},

        { &hf_sbccs_dib_linkctlinfo_ctcconn,
          { "CTC Conn", "fcsb3.linkctlinfo.ctc_conn",
            FT_BOOLEAN, 16, TFS(&tfs_supported_not_supported), 0x80,
            NULL, HFILL}},

        { &hf_sbccs_dib_linkctlinfo_ecrcg,
          { "Enhanced CRC Generation", "fcsb3.linkctlinfo.ecrcg",
            FT_BOOLEAN, 16, TFS(&tfs_supported_not_supported), 0x01,
            NULL, HFILL}},

        { &hf_sbccs_logical_path,
          { "Logical Path", "fcsb3.logical_path",
            FT_BYTES, SEP_COLON, NULL, 0x0,
            NULL, HFILL}},
    };


    /* Setup protocol subtree array */
    static gint *ett[] = {
        &ett_fc_sbccs,
        &ett_sbccs_iui,
        &ett_sbccs_dhflags,
        &ett_sbccs_dib_ccw_flags,
        &ett_sbccs_dib_cmdflags,
        &ett_sbccs_dib_statusflags,
        &ett_sbccs_dib_status,
        &ett_sbccs_dib_ctlparam,
        &ett_sbccs_dib_linkctlinfo,
    };

    /* Register the protocol name and description */
    proto_fc_sbccs = proto_register_protocol ("Fibre Channel Single Byte Command",
                                              "FC-SB3", "fcsb3");

    proto_register_field_array(proto_fc_sbccs, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
}

void
proto_reg_handoff_fcsbccs (void)
{
    dissector_handle_t fc_sbccs_handle;

    fc_sbccs_handle = create_dissector_handle (dissect_fc_sbccs,
                                               proto_fc_sbccs);

    dissector_add_uint("fc.ftype", FC_FTYPE_SBCCS, fc_sbccs_handle);
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
