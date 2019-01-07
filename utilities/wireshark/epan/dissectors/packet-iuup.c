/* packet-iuup.c
 * IuUP Protocol 3GPP TS 25.415 V6.2.0 (2005-03)
 *
 * (c) 2005 Luis E. Garcia Ontanon <luis@ontanon.org>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


/*
   Patch by Polystar (Peter Vestman, Petter Edblom):
      Corrected rfci handling in rate control messages
      Added crc6 and crc10 checks for header and payload
*/

#include "config.h"

#include <epan/packet.h>
#include <epan/prefs.h>
#include <epan/expert.h>
#include <epan/crc10-tvb.h>
#include <wsutil/crc10.h>
#include <wsutil/crc6.h>

void proto_reg_handoff_iuup(void);
void proto_register_iuup(void);

typedef struct _iuup_rfci_t {
    guint id;
    guint sum_len;
    guint num_of_subflows;
    struct {
        guint len;
    } subflow[8];
    struct _iuup_rfci_t* next;
} iuup_rfci_t;

typedef struct {
    guint32 id;
    guint num_of_subflows;
    iuup_rfci_t* rfcis;
    iuup_rfci_t* last_rfci;
} iuup_circuit_t;

static int proto_iuup = -1;

static int hf_iuup_direction = -1;
static int hf_iuup_circuit_id = -1;

static int hf_iuup_pdu_type = -1;
static int hf_iuup_frame_number = -1;
static int hf_iuup_fqc = -1;
static int hf_iuup_rfci = -1;
static int hf_iuup_hdr_crc = -1;
static int hf_iuup_payload_crc = -1;

static int hf_iuup_ack_nack = -1;
static int hf_iuup_frame_number_t14 = -1;
static int hf_iuup_mode_version = -1;
static int hf_iuup_procedure_indicator = -1;
static int hf_iuup_error_cause_val = -1;

static int hf_iuup_init_ti = -1;
static int hf_iuup_init_subflows_per_rfci = -1;
static int hf_iuup_init_chain_ind = -1;

static int hf_iuup_error_distance = -1;
static int hf_iuup_errorevt_cause_val = -1;

static int hf_iuup_time_align = -1;
static int hf_iuup_spare_bytes = -1;
static int hf_iuup_spare_03 = -1;
/* static int hf_iuup_spare_0f = -1; */
/* static int hf_iuup_spare_c0 = -1; */
static int hf_iuup_spare_e0 = -1;
static int hf_iuup_spare_ff = -1;

static int hf_iuup_delay = -1;
static int hf_iuup_advance = -1;
static int hf_iuup_delta = -1;

static int hf_iuup_mode_versions = -1;
static int hf_iuup_mode_versions_a[] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};


static int hf_iuup_data_pdu_type = -1;

static int hf_iuup_num_rfci_ind = -1;

static int hf_iuup_payload = -1;

static int hf_iuup_init_rfci_ind = -1;
static int hf_iuup_init_rfci[] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};

static int hf_iuup_init_rfci_flow_len[64][8] = {
    {-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1}
};

static int hf_iuup_init_rfci_li[] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
static int hf_iuup_init_rfci_lri[] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
static int hf_iuup_init_ipti[] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};

static int hf_iuup_rfci_subflow[64][8] = {
    {-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1}
};

static int hf_iuup_rfci_ratectl[] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};


static gint ett_iuup = -1;
static gint ett_rfci = -1;
static gint ett_ipti = -1;
static gint ett_support = -1;
static gint ett_time = -1;
static gint ett_rfciinds = -1;
static gint ett_payload = -1;
static gint ett_payload_subflows = -1;

static expert_field ei_iuup_hdr_crc_bad = EI_INIT;
static expert_field ei_iuup_payload_crc_bad = EI_INIT;
static expert_field ei_iuup_payload_undecoded = EI_INIT;
static expert_field ei_iuup_error_response = EI_INIT;
static expert_field ei_iuup_ack_nack = EI_INIT;
static expert_field ei_iuup_time_align = EI_INIT;
static expert_field ei_iuup_procedure_indicator = EI_INIT;
static expert_field ei_iuup_pdu_type = EI_INIT;

static GHashTable* circuits = NULL;

static gboolean dissect_fields = FALSE;
static gboolean two_byte_pseudoheader = FALSE;
static guint global_dynamic_payload_type = 0;


#define PDUTYPE_DATA_WITH_CRC 0
#define PDUTYPE_DATA_NO_CRC 1
#define PDUTYPE_DATA_CONTROL_PROC 14

static const value_string iuup_pdu_types[] = {
    {PDUTYPE_DATA_WITH_CRC,"Data with CRC"},
    {PDUTYPE_DATA_NO_CRC,"Data without CRC"},
    {PDUTYPE_DATA_CONTROL_PROC,"Control Procedure"},
    {0,NULL}
};

static const value_string iuup_colinfo_pdu_types[] = {
    {PDUTYPE_DATA_WITH_CRC,"Data (CRC)"},
    {PDUTYPE_DATA_NO_CRC,"Data (no CRC)"},
    {PDUTYPE_DATA_CONTROL_PROC,""},
    {0,NULL}
};

#define ACKNACK_ACK 0x4
#define ACKNACK_NACK 0x8
#define ACKNACK_RESERVED 0xc
#define ACKNACK_PROC 0x0

static const value_string iuup_acknack_vals[] = {
    {ACKNACK_PROC >> 2,"Procedure"},
    {ACKNACK_ACK >> 2,"ACK"},
    {ACKNACK_NACK  >> 2,"NACK"},
    {ACKNACK_RESERVED  >> 2,"Reserved"},
    {0,NULL}
};

static const value_string iuup_colinfo_acknack_vals[] = {
    {ACKNACK_PROC,""},
    {ACKNACK_ACK,"ACK "},
    {ACKNACK_NACK,"NACK "},
    {ACKNACK_RESERVED,"Reserved "},
    {0,NULL}
};

#define PROC_INIT 0
#define PROC_RATE 1
#define PROC_TIME 2
#define PROC_ERROR 3

static const value_string iuup_procedures[] = {
    {PROC_INIT,"Initialization"},
    {PROC_RATE,"Rate Control"},
    {PROC_TIME,"Time Alignment"},
    {PROC_ERROR,"Error Event"},
    {4,"Reserved(4)"},
    {5,"Reserved(5)"},
    {6,"Reserved(6)"},
    {7,"Reserved(7)"},
    {8,"Reserved(8)"},
    {9,"Reserved(9)"},
    {10,"Reserved(10)"},
    {11,"Reserved(11)"},
    {12,"Reserved(12)"},
    {13,"Reserved(13)"},
    {14,"Reserved(14)"},
    {15,"Reserved(15)"},
    {0,NULL}
};

static const value_string iuup_colinfo_procedures[] = {
    {PROC_INIT,"Initialization "},
    {PROC_RATE,"Rate Control "},
    {PROC_TIME,"Time Alignment "},
    {PROC_ERROR,"Error Event "},
    {0,NULL}
};


static const value_string iuup_error_distances[] = {
    {0, "Reporting local error"},
    {1, "First forwarding of error event report"},
    {2, "Second forwarding of error event report"},
    {3, "Reserved"},
    {0,NULL}
};

static const value_string iuup_error_causes[] = {
    {0, "CRC error of frame header"},
    {1, "CRC error of frame payload"},
    {2, "Unexpected frame number"},
    {3, "Frame loss"},
    {4, "PDU type unknown"},
    {5, "Unknown procedure"},
    {6, "Unknown reserved value"},
    {7, "Unknown field"},
    {8, "Frame too short"},
    {9, "Missing fields"},
    {16, "Unexpected PDU type"},
    {18, "Unexpected procedure"},
    {19, "Unexpected RFCI"},
    {20, "Unexpected value"},
    {42, "Initialisation failure"},
    {43, "Initialisation failure (network error, timer expiry)"},
    {44, "Initialisation failure (Iu UP function error, repeated NACK)"},
    {45, "Rate control failure"},
    {46, "Error event failure"},
    {47, "Time Alignment not supported"},
    {48, "Requested Time Alignment not possible"},
    {49, "Iu UP Mode version not supported"},
    {0,NULL}
};

static const value_string iuup_rfci_indicator[] = {
    {0, "RFCI allowed"},
    {1, "RFCI barred"},
    {0,NULL}
};


static const value_string iuup_ti_vals[] = {
    {0, "IPTIs not present"},
    {1, "IPTIs present in frame"},
    {0,NULL}
};

static const value_string iuup_mode_version_support[] = {
    {0, "not supported"},
    {1, "supported"},
    {0,NULL}
};

static const value_string iuup_init_rfci_li_vals[] = {
    {0, "one octet used"},
    {1, "two octets used"},
    {0,NULL}
};

static const value_string iuup_init_chain_ind_vals[] = {
    {0, "this frame is the last frame for the procedure"},
    {1, "additional frames will be sent for the procedure"},
    {0,NULL}
};

static const value_string iuup_init_lri_vals[] = {
    {0, "Not last RFCI"},
    {1, "Last RFCI in current frame"},
    {0,NULL}
};

static const value_string iuup_payload_pdu_type[] = {
    {0, "PDU type 0"},
    {1, "PDU type 1"},
    {0,NULL}
};

static const value_string iuup_fqcs[] = {
    {0, "Frame Good"},
    {1, "Frame BAD"},
    {2, "Frame bad due to radio"},
    {3, "spare"},
    {0,NULL}
};


static proto_item*
iuup_proto_tree_add_bits(proto_tree* tree, int hf, tvbuff_t* tvb, int offset, int bit_offset, guint bits, guint8** buf) {
    static const guint8 masks[] = {0x00,0x80,0xc0,0xe0,0xf0,0xf8,0xfc,0xfe};
    int len = (bits + bit_offset)/8 + (((bits + bit_offset)%8) ? 0 : 1);
    guint8* shifted_buffer;
    proto_item* pi;
    int i;

    DISSECTOR_ASSERT(bit_offset < 8);

    shifted_buffer = (guint8 *)tvb_memdup(wmem_packet_scope(),tvb,offset,len+1);

    for(i = 0; i < len; i++) {
        shifted_buffer[i] <<= bit_offset;
        shifted_buffer[i] |= (shifted_buffer[i+1] & masks[bit_offset]) >> (8 - bit_offset);
    }

    shifted_buffer[len] <<=  bit_offset;
    shifted_buffer[len] &= masks[(bits + bit_offset)%8];

    if (buf)
        *buf = shifted_buffer;

    pi = proto_tree_add_bytes(tree, hf, tvb, offset, len + (((bits + bit_offset)%8) ? 1 : 0) , shifted_buffer);
    proto_item_append_text(pi, " (%i Bits)", bits);

    return pi;
}

static void dissect_iuup_payload(tvbuff_t* tvb, packet_info* pinfo, proto_tree* tree, guint rfci_id _U_, int offset, guint32 circuit_id) {
    iuup_circuit_t* iuup_circuit;
    iuup_rfci_t *rfci;
    int last_offset = tvb_reported_length(tvb) - 1;
    guint bit_offset = 0;
    proto_item* pi;

    pi = proto_tree_add_item(tree,hf_iuup_payload,tvb,offset,-1,ENC_NA);

    if ( ! dissect_fields ) {
        return;
    } else if ( ! circuit_id
                || ! ( iuup_circuit  = (iuup_circuit_t *)g_hash_table_lookup(circuits,GUINT_TO_POINTER(circuit_id)) ) ) {
        expert_add_info(pinfo, pi, &ei_iuup_payload_undecoded);
        return;
    }

    for(rfci = iuup_circuit->rfcis; rfci; rfci = rfci->next)
        if ( rfci->id == rfci_id )
            break;

    if (!rfci) {
        expert_add_info(pinfo, pi, &ei_iuup_payload_undecoded);
        return;
    }

    tree = proto_item_add_subtree(pi,ett_payload);


    do {
        guint i;
        guint subflows = rfci->num_of_subflows;
        proto_tree* flow_tree;

        flow_tree = proto_tree_add_subtree(tree,tvb,offset,-1,ett_payload_subflows,NULL,"Payload Frame");

        bit_offset = 0;

        for(i = 0; i < subflows; i++) {

            if (! rfci->subflow[i].len)
                continue;

            iuup_proto_tree_add_bits(flow_tree, hf_iuup_rfci_subflow[rfci->id][i], tvb,
                                offset + (bit_offset/8),
                                bit_offset % 8,
                                rfci->subflow[i].len,
                                NULL);

            bit_offset += rfci->subflow[i].len;
        }

        offset += (bit_offset / 8) + ((bit_offset % 8) ? 1 : 0);

    } while (offset <= last_offset);
}

static guint dissect_rfcis(tvbuff_t* tvb, packet_info* pinfo _U_, proto_tree* tree, int* offset, iuup_circuit_t* iuup_circuit) {
    proto_item* pi;
    proto_tree* pt;
    guint8 oct;
    guint c = 0;
    guint i;

    do {
        iuup_rfci_t *rfci = wmem_new0(wmem_file_scope(), iuup_rfci_t);
        guint len = 0;

        DISSECTOR_ASSERT(c < 64);

        pi = proto_tree_add_item(tree,hf_iuup_init_rfci_ind,tvb,*offset,-1,ENC_NA);
        pt = proto_item_add_subtree(pi,ett_rfci);

        proto_tree_add_item(pt,hf_iuup_init_rfci_lri[c],tvb,*offset,1,ENC_BIG_ENDIAN);
        proto_tree_add_item(pt,hf_iuup_init_rfci_li[c],tvb,*offset,1,ENC_BIG_ENDIAN);
        proto_tree_add_item(pt,hf_iuup_init_rfci[c],tvb,*offset,1,ENC_BIG_ENDIAN);

        oct = tvb_get_guint8(tvb,*offset);
        rfci->id = oct & 0x3f;
        rfci->num_of_subflows = iuup_circuit->num_of_subflows;

        len = (oct & 0x40) ? 2 : 1;
        proto_item_set_text(pi,"RFCI %i Initialization",rfci->id);
        proto_item_set_len(pi,(len*iuup_circuit->num_of_subflows)+1);

        (*offset)++;

        for(i = 0; i < iuup_circuit->num_of_subflows; i++) {
            guint subflow_len;

            if (len == 2) {
                subflow_len = tvb_get_ntohs(tvb,*offset);
            } else {
                subflow_len = tvb_get_guint8(tvb,*offset);
            }

            rfci->subflow[i].len = subflow_len;
            rfci->sum_len += subflow_len;

            proto_tree_add_uint(pt,hf_iuup_init_rfci_flow_len[c][i],tvb,*offset,len,subflow_len);

            (*offset) += len;
        }


        if (iuup_circuit->last_rfci) {
            iuup_circuit->last_rfci = iuup_circuit->last_rfci->next = rfci;
        } else {
            iuup_circuit->last_rfci = iuup_circuit->rfcis = rfci;
        }

        c++;
    } while ( ! (oct & 0x80) );

    return c - 1;
}

static void dissect_iuup_init(tvbuff_t* tvb, packet_info* pinfo, proto_tree* tree, guint32 circuit_id) {
    int offset = 4;
    guint8 oct = tvb_get_guint8(tvb,offset);
    guint n = (oct & 0x0e) >> 1;
    gboolean ti = oct & 0x10;
    guint i;
    guint rfcis;
    proto_item* pi;
    proto_tree* support_tree = NULL;
    proto_tree* iptis_tree;
    iuup_circuit_t* iuup_circuit = NULL;

    if (circuit_id) {
        iuup_circuit = (iuup_circuit_t *)g_hash_table_lookup(circuits,GUINT_TO_POINTER(circuit_id));

        if (iuup_circuit) {
            g_hash_table_remove(circuits,GUINT_TO_POINTER(circuit_id));
        }

        iuup_circuit = wmem_new0(wmem_file_scope(), iuup_circuit_t);
    } else {
        iuup_circuit = wmem_new0(wmem_packet_scope(), iuup_circuit_t);
    }

    iuup_circuit->id = circuit_id;
    iuup_circuit->num_of_subflows = n;
    iuup_circuit->rfcis = NULL;
    iuup_circuit->last_rfci = NULL;

    if (circuit_id) {
        g_hash_table_insert(circuits,GUINT_TO_POINTER(iuup_circuit->id),iuup_circuit);
    }

    if (tree) {
        proto_tree_add_item(tree,hf_iuup_spare_e0,tvb,offset,1,ENC_BIG_ENDIAN);
        proto_tree_add_item(tree,hf_iuup_init_ti,tvb,offset,1,ENC_BIG_ENDIAN);
        proto_tree_add_item(tree,hf_iuup_init_subflows_per_rfci,tvb,offset,1,ENC_BIG_ENDIAN);
        proto_tree_add_item(tree,hf_iuup_init_chain_ind,tvb,offset,1,ENC_BIG_ENDIAN);
    }

    offset++;

    rfcis = dissect_rfcis(tvb, pinfo, tree, &offset, iuup_circuit);

    if (!tree) return;

    if (ti) {
        iptis_tree = proto_tree_add_subtree(tree,tvb,offset,(rfcis/2)+(rfcis%2),ett_ipti,NULL,"IPTIs");

        for (i = 0; i <= rfcis; i++) {
            proto_tree_add_item(iptis_tree,hf_iuup_init_ipti[i],tvb,offset,1,ENC_BIG_ENDIAN);
            if ((i%2)) {
                offset++;
            }
        }

        if ((i%2)) {
            offset++;
        }
    }

    if (tree) {
        pi = proto_tree_add_item(tree,hf_iuup_mode_versions,tvb,offset,2,ENC_BIG_ENDIAN);
        support_tree = proto_item_add_subtree(pi,ett_support);

        for (i = 0; i < 16; i++) {
            proto_tree_add_item(support_tree,hf_iuup_mode_versions_a[i],tvb,offset,2,ENC_BIG_ENDIAN);
        }

    }

    offset += 2;

    proto_tree_add_item(tree,hf_iuup_data_pdu_type,tvb,offset,1,ENC_BIG_ENDIAN);

}

static void dissect_iuup_ratectl(tvbuff_t* tvb, packet_info* pinfo _U_, proto_tree* tree) {
    guint num = tvb_get_guint8(tvb,4) & 0x3f;
    guint i;
    proto_item* pi;
    proto_tree* inds_tree;
    int offset = 4;

    pi = proto_tree_add_item(tree,hf_iuup_num_rfci_ind,tvb,4,1,ENC_BIG_ENDIAN);
    inds_tree = proto_item_add_subtree(pi,ett_rfciinds);

    for (i = 0; i < num; i++) {
        if (! (i % 8) ) offset++;
        proto_tree_add_item(inds_tree,hf_iuup_rfci_ratectl[i],tvb,offset,1,ENC_BIG_ENDIAN);
    }

}

static void add_hdr_crc(tvbuff_t* tvb, packet_info* pinfo, proto_item* iuup_tree, guint16 crccheck)
{
    proto_item *crc_item;

    crc_item = proto_tree_add_item(iuup_tree,hf_iuup_hdr_crc,tvb,2,1,ENC_BIG_ENDIAN);
    if (crccheck) {
        proto_item_append_text(crc_item, "%s", " [incorrect]");
        expert_add_info(pinfo, crc_item, &ei_iuup_hdr_crc_bad);
    }
}

static guint16
update_crc10_by_bytes_iuup(tvbuff_t *tvb, int offset, int length)
{
    guint16 crc10;
    guint16 extra_16bits;
    guint8 extra_8bits[2];

    crc10 = update_crc10_by_bytes_tvb(0, tvb, offset + 2, length);
    extra_16bits = tvb_get_ntohs(tvb, offset) & 0x3FF;
    extra_8bits[0] = extra_16bits >> 2;
    extra_8bits[1] = (extra_16bits << 6) & 0xFF;
    crc10 = update_crc10_by_bytes(crc10, extra_8bits, 2);
    return crc10;
}

static void add_payload_crc(tvbuff_t* tvb, packet_info* pinfo, proto_item* iuup_tree)
{
    proto_item *crc_item;
    int length = tvb_reported_length(tvb);
    guint16 crccheck = update_crc10_by_bytes_iuup(tvb, 2, length - 4);

    crc_item = proto_tree_add_item(iuup_tree,hf_iuup_payload_crc,tvb,2,2,ENC_BIG_ENDIAN);
    if (crccheck) {
        proto_item_append_text(crc_item, "%s", " [incorrect]");
        expert_add_info(pinfo, crc_item, &ei_iuup_payload_crc_bad);
    }
}

#define ACKNACK_MASK  0x0c
#define PROCEDURE_MASK  0x0f
#define FQC_MASK 0xc0
#define PDUTYPE_MASK 0xf0
static int dissect_iuup(tvbuff_t* tvb_in, packet_info* pinfo, proto_tree* tree, void* data _U_) {
    proto_item* pi;
    proto_item* iuup_item = NULL;
    proto_item* pdutype_item = NULL;
    proto_tree* iuup_tree = NULL;
    proto_item* proc_item = NULL;
    proto_item* ack_item = NULL;
    guint8 first_octet;
    guint8 second_octet;
    guint8 pdutype;
    guint phdr = 0;
    guint16  hdrcrc6;
    guint16  crccheck;
    tvbuff_t* tvb = tvb_in;

    col_set_str(pinfo->cinfo, COL_PROTOCOL, "IuUP");

    if (two_byte_pseudoheader) {
        int len = tvb_reported_length(tvb_in) - 2;

        phdr = tvb_get_ntohs(tvb,0);

        proto_tree_add_item(tree,hf_iuup_direction,tvb,0,2,ENC_BIG_ENDIAN);
        proto_tree_add_item(tree,hf_iuup_circuit_id,tvb,0,2,ENC_BIG_ENDIAN);

        phdr &= 0x7fff;

        pinfo->circuit_id = phdr;

        tvb = tvb_new_subset_length(tvb_in,2,len);
    }

    first_octet =  tvb_get_guint8(tvb,0);
    second_octet =  tvb_get_guint8(tvb,1);
    hdrcrc6 = tvb_get_guint8(tvb, 2) >> 2;
    crccheck = update_crc6_by_bytes(hdrcrc6, first_octet, second_octet);

    pdutype = ( first_octet & PDUTYPE_MASK ) >> 4;

    if (tree) {
        iuup_item = proto_tree_add_item(tree,proto_iuup,tvb,0,-1,ENC_NA);
        iuup_tree = proto_item_add_subtree(iuup_item,ett_iuup);

        pdutype_item = proto_tree_add_item(iuup_tree,hf_iuup_pdu_type,tvb,0,1,ENC_BIG_ENDIAN);
    }

    col_add_str(pinfo->cinfo, COL_INFO, val_to_str(pdutype, iuup_colinfo_pdu_types, "Unknown PDU Type(%u) "));

    switch(pdutype) {
        case PDUTYPE_DATA_WITH_CRC:
            col_append_fstr(pinfo->cinfo, COL_INFO,"FN: %x RFCI: %u", (guint)(first_octet & 0x0f) ,(guint)(second_octet & 0x3f));

            proto_tree_add_item(iuup_tree,hf_iuup_frame_number,tvb,0,1,ENC_BIG_ENDIAN);
            pi = proto_tree_add_item(iuup_tree,hf_iuup_fqc,tvb,1,1,ENC_BIG_ENDIAN);

            if (first_octet & FQC_MASK) {
                expert_add_info(pinfo, pi, &ei_iuup_error_response);
            }

            proto_tree_add_item(iuup_tree,hf_iuup_rfci,tvb,1,1,ENC_BIG_ENDIAN);
            add_hdr_crc(tvb, pinfo, iuup_tree, crccheck);
            add_payload_crc(tvb, pinfo, iuup_tree);
            dissect_iuup_payload(tvb,pinfo,iuup_tree,second_octet & 0x3f,4,pinfo->circuit_id);
            return tvb_captured_length(tvb);
        case PDUTYPE_DATA_NO_CRC:
            col_append_fstr(pinfo->cinfo, COL_INFO," RFCI %u", (guint)(second_octet & 0x3f));

            proto_tree_add_item(iuup_tree,hf_iuup_frame_number,tvb,0,1,ENC_BIG_ENDIAN);
            pi = proto_tree_add_item(iuup_tree,hf_iuup_fqc,tvb,1,1,ENC_BIG_ENDIAN);

            if (first_octet & FQC_MASK) {
                expert_add_info(pinfo, pi, &ei_iuup_error_response);
            }

            proto_tree_add_item(iuup_tree,hf_iuup_rfci,tvb,1,1,ENC_BIG_ENDIAN);
            add_hdr_crc(tvb, pinfo, iuup_tree, crccheck);
            dissect_iuup_payload(tvb,pinfo,iuup_tree,second_octet & 0x3f,3,pinfo->circuit_id);
            return tvb_captured_length(tvb);
        case PDUTYPE_DATA_CONTROL_PROC:
            if (tree) {
                ack_item = proto_tree_add_item(iuup_tree,hf_iuup_ack_nack,tvb,0,1,ENC_BIG_ENDIAN);
                proto_tree_add_item(iuup_tree,hf_iuup_frame_number_t14,tvb,0,1,ENC_BIG_ENDIAN);
                proto_tree_add_item(iuup_tree,hf_iuup_mode_version,tvb,1,1,ENC_BIG_ENDIAN);
                proc_item = proto_tree_add_item(iuup_tree,hf_iuup_procedure_indicator,tvb,1,1,ENC_BIG_ENDIAN);
                add_hdr_crc(tvb, pinfo, iuup_tree, crccheck);
            }

            col_append_str(pinfo->cinfo, COL_INFO,
                           val_to_str(first_octet & ACKNACK_MASK,
                                      iuup_colinfo_acknack_vals, "[action:%u] "));

            col_append_str(pinfo->cinfo, COL_INFO,
                           val_to_str(second_octet & PROCEDURE_MASK,
                                      iuup_colinfo_procedures, "[proc:%u] "));

            switch ( first_octet & ACKNACK_MASK ) {
                case ACKNACK_ACK:
                    switch(second_octet & PROCEDURE_MASK) {
                        case PROC_INIT:
                            proto_tree_add_item(iuup_tree,hf_iuup_spare_03,tvb,2,1,ENC_BIG_ENDIAN);
                            proto_tree_add_item(iuup_tree,hf_iuup_spare_ff,tvb,3,1,ENC_BIG_ENDIAN);
                            return tvb_captured_length(tvb);
                        case PROC_RATE:
                            dissect_iuup_ratectl(tvb,pinfo,iuup_tree);
                            return tvb_captured_length(tvb);
                        case PROC_TIME:
                        case PROC_ERROR:
                            break;
                        default:
                            expert_add_info(pinfo, proc_item, &ei_iuup_procedure_indicator);
                            return tvb_captured_length(tvb);
                    }
                    break;
                case ACKNACK_NACK:
                    pi = proto_tree_add_item(iuup_tree,hf_iuup_error_cause_val,tvb,4,1,ENC_BIG_ENDIAN);
                    expert_add_info(pinfo, pi, &ei_iuup_error_response);
                    return tvb_captured_length(tvb);
                case ACKNACK_RESERVED:
                    expert_add_info(pinfo, ack_item, &ei_iuup_ack_nack);
                    return tvb_captured_length(tvb);
                case ACKNACK_PROC:
                    break;
            }

            switch( second_octet & PROCEDURE_MASK ) {
                case PROC_INIT:
                    add_payload_crc(tvb, pinfo, iuup_tree);
                    dissect_iuup_init(tvb,pinfo,iuup_tree,pinfo->circuit_id);
                    return tvb_captured_length(tvb);
                case PROC_RATE:
                    add_payload_crc(tvb, pinfo, iuup_tree);
                    dissect_iuup_ratectl(tvb,pinfo,iuup_tree);
                    return tvb_captured_length(tvb);
                case PROC_TIME:
                {
                    proto_tree* time_tree;
                    guint ta;

                    ta = tvb_get_guint8(tvb,4);

                    pi = proto_tree_add_item(iuup_tree,hf_iuup_time_align,tvb,4,1,ENC_BIG_ENDIAN);
                    time_tree = proto_item_add_subtree(pi,ett_time);

                    if (ta >= 1 && ta <= 80) {
                        pi = proto_tree_add_uint(time_tree,hf_iuup_delay,tvb,4,1,ta * 500);
                        PROTO_ITEM_SET_GENERATED(pi);
                        pi = proto_tree_add_float(time_tree,hf_iuup_delta,tvb,4,1,((gfloat)((gint)(ta) * 500))/(gfloat)1000000.0);
                        PROTO_ITEM_SET_GENERATED(pi);
                    } else if (ta >= 129 && ta <= 208) {
                        pi = proto_tree_add_uint(time_tree,hf_iuup_advance,tvb,4,1,(ta-128) * 500);
                        PROTO_ITEM_SET_GENERATED(pi);
                        pi = proto_tree_add_float(time_tree,hf_iuup_delta,tvb,4,1,((gfloat)((gint)(-(((gint)ta)-128))) * 500)/(gfloat)1000000.0);
                        PROTO_ITEM_SET_GENERATED(pi);
                    } else {
                        expert_add_info(pinfo, pi, &ei_iuup_time_align);
                    }

                    proto_tree_add_item(iuup_tree,hf_iuup_spare_bytes,tvb,5,-1,ENC_NA);
                    return tvb_captured_length(tvb);
                }
                case PROC_ERROR:
                    col_append_str(pinfo->cinfo, COL_INFO, val_to_str(tvb_get_guint8(tvb,4) & 0x3f,iuup_error_causes,"Unknown (%u)"));

                    proto_tree_add_item(iuup_tree,hf_iuup_error_distance,tvb,4,1,ENC_BIG_ENDIAN);
                    pi = proto_tree_add_item(iuup_tree,hf_iuup_errorevt_cause_val,tvb,4,1,ENC_BIG_ENDIAN);
                    expert_add_info(pinfo, pi, &ei_iuup_error_response);
                    proto_tree_add_item(iuup_tree,hf_iuup_spare_bytes,tvb,5,-1,ENC_NA);
                    return tvb_captured_length(tvb);
                default: /* bad */
                    expert_add_info(pinfo, proc_item, &ei_iuup_procedure_indicator);
                    return tvb_captured_length(tvb);
            }
        default:
            expert_add_info(pinfo, pdutype_item, &ei_iuup_pdu_type);
            break;
    }
    return tvb_captured_length(tvb);
}


static gboolean dissect_iuup_heur(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_) {
    int len = tvb_captured_length(tvb);

    guint8 first_octet =  tvb_get_guint8(tvb,0);
    guint8 second_octet =  tvb_get_guint8(tvb,1);
    guint16 hdrcrc6 = tvb_get_guint8(tvb, 2) >> 2;

    if (update_crc6_by_bytes(hdrcrc6, first_octet, second_octet)) return FALSE;

    switch ( first_octet & 0xf0 ) {
        case 0x00: {
            if (len<7) return FALSE;
            if (update_crc10_by_bytes_iuup(tvb, 4, len-4) ) return FALSE;
            break;
        }
        case 0x10:
            /* a FALSE positive factory */
            if (len<5) return FALSE;
            break;
        case 0xe0:
            if (len<5) return FALSE;
            if( (second_octet & 0x0f) > 3) return FALSE;
            break;
        default:
            return FALSE;
    }

    dissect_iuup(tvb, pinfo, tree, data);
    return TRUE;
}


static int find_iuup(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_) {
    int len = tvb_captured_length(tvb);
    int offset = 0;

    while (len > 3) {
        if ( dissect_iuup_heur(tvb_new_subset_remaining(tvb,offset), pinfo, tree, data) )
            return tvb_captured_length(tvb);

        offset++;
        len--;
    }

    call_data_dissector(tvb, pinfo, tree);
    return tvb_captured_length(tvb);
}

static void init_iuup(void) {
    circuits = g_hash_table_new(g_direct_hash,g_direct_equal);
}

static void cleanup_iuup(void) {
    g_hash_table_destroy(circuits);
}


void proto_reg_handoff_iuup(void) {
    static gboolean iuup_prefs_initialized = FALSE;
    static dissector_handle_t iuup_handle;
    static guint saved_dynamic_payload_type = 0;

    if (!iuup_prefs_initialized) {
        iuup_handle = find_dissector("iuup");
        dissector_add_string("rtp_dyn_payload_type","VND.3GPP.IUFP", iuup_handle);
        iuup_prefs_initialized = TRUE;
    } else {
        if ( saved_dynamic_payload_type > 95 ) {
            dissector_delete_uint("rtp.pt", saved_dynamic_payload_type, iuup_handle);
        }
    }

    saved_dynamic_payload_type = global_dynamic_payload_type;

    if ( global_dynamic_payload_type > 95 ) {
        dissector_add_uint("rtp.pt", global_dynamic_payload_type, iuup_handle);
    }
}


#define HFS_RFCI(i) \
{ &hf_iuup_rfci_ratectl[i], { "RFCI " #i, "iuup.rfci." #i, FT_UINT8, BASE_DEC, VALS(iuup_rfci_indicator),0x80>>(i%8),NULL,HFILL}}, \
{ &hf_iuup_init_rfci[i], { "RFCI " #i, "iuup.rfci." #i, FT_UINT8, BASE_DEC, NULL,0x3f,NULL,HFILL}}, \
{ &hf_iuup_init_rfci_flow_len[i][0], { "RFCI " #i " Flow 0 Len", "iuup.rfci."#i".flow.0.len", FT_UINT16, BASE_DEC, NULL,0x0,NULL,HFILL}}, \
{ &hf_iuup_init_rfci_flow_len[i][1], { "RFCI " #i " Flow 1 Len", "iuup.rfci."#i".flow.1.len", FT_UINT16, BASE_DEC, NULL,0x0,NULL,HFILL}}, \
{ &hf_iuup_init_rfci_flow_len[i][2], { "RFCI " #i " Flow 2 Len", "iuup.rfci."#i".flow.2.len", FT_UINT16, BASE_DEC, NULL,0x0,NULL,HFILL}}, \
{ &hf_iuup_init_rfci_flow_len[i][3], { "RFCI " #i " Flow 3 Len", "iuup.rfci."#i".flow.3.len", FT_UINT16, BASE_DEC, NULL,0x0,NULL,HFILL}}, \
{ &hf_iuup_init_rfci_flow_len[i][4], { "RFCI " #i " Flow 4 Len", "iuup.rfci."#i".flow.4.len", FT_UINT16, BASE_DEC, NULL,0x0,NULL,HFILL}}, \
{ &hf_iuup_init_rfci_flow_len[i][5], { "RFCI " #i " Flow 5 Len", "iuup.rfci."#i".flow.5.len", FT_UINT16, BASE_DEC, NULL,0x0,NULL,HFILL}}, \
{ &hf_iuup_init_rfci_flow_len[i][6], { "RFCI " #i " Flow 6 Len", "iuup.rfci."#i".flow.6.len", FT_UINT16, BASE_DEC, NULL,0x0,NULL,HFILL}}, \
{ &hf_iuup_init_rfci_flow_len[i][7], { "RFCI " #i " Flow 7 Len", "iuup.rfci."#i".flow.7.len", FT_UINT16, BASE_DEC, NULL,0x0,NULL,HFILL}}, \
{ &hf_iuup_init_rfci_li[i], { "RFCI " #i " LI", "iuup.rfci."#i".li", FT_UINT8, BASE_HEX, VALS(iuup_init_rfci_li_vals),0x40,"Length Indicator",HFILL}}, \
{ &hf_iuup_init_rfci_lri[i], { "RFCI " #i " LRI", "iuup.rfci."#i".lri", FT_UINT8, BASE_HEX, VALS(iuup_init_lri_vals),0x80,"Last Record Indicator",HFILL}}, \
{ &hf_iuup_rfci_subflow[i][0], { "RFCI " #i " Flow 0", "iuup.rfci."#i".flow.0", FT_BYTES, BASE_NONE, NULL,0x0,NULL,HFILL}}, \
{ &hf_iuup_rfci_subflow[i][1], { "RFCI " #i " Flow 1", "iuup.rfci."#i".flow.1", FT_BYTES, BASE_NONE, NULL,0x0,NULL,HFILL}}, \
{ &hf_iuup_rfci_subflow[i][2], { "RFCI " #i " Flow 2", "iuup.rfci."#i".flow.2", FT_BYTES, BASE_NONE, NULL,0x0,NULL,HFILL}}, \
{ &hf_iuup_rfci_subflow[i][3], { "RFCI " #i " Flow 3", "iuup.rfci."#i".flow.3", FT_BYTES, BASE_NONE, NULL,0x0,NULL,HFILL}}, \
{ &hf_iuup_rfci_subflow[i][4], { "RFCI " #i " Flow 4", "iuup.rfci."#i".flow.4", FT_BYTES, BASE_NONE, NULL,0x0,NULL,HFILL}}, \
{ &hf_iuup_rfci_subflow[i][5], { "RFCI " #i " Flow 5", "iuup.rfci."#i".flow.5", FT_BYTES, BASE_NONE, NULL,0x0,NULL,HFILL}}, \
{ &hf_iuup_rfci_subflow[i][6], { "RFCI " #i " Flow 6", "iuup.rfci."#i".flow.6", FT_BYTES, BASE_NONE, NULL,0x0,NULL,HFILL}}, \
{ &hf_iuup_rfci_subflow[i][7], { "RFCI " #i " Flow 7", "iuup.rfci."#i".flow.7", FT_BYTES, BASE_NONE, NULL,0x0,NULL,HFILL}}, \
{ &hf_iuup_init_ipti[i], { "RFCI " #i " IPTI", "iuup.rfci."#i".ipti", FT_UINT8, BASE_HEX, NULL,i%2 ? 0x0F : 0xF0,NULL,HFILL}}



void proto_register_iuup(void) {
    static hf_register_info hf[] = {
        { &hf_iuup_direction, { "Frame Direction", "iuup.direction", FT_UINT16, BASE_DEC, NULL,0x8000,NULL,HFILL}},
        { &hf_iuup_circuit_id, { "Circuit ID", "iuup.circuit_id", FT_UINT16, BASE_DEC, NULL,0x7fff,NULL,HFILL}},
        { &hf_iuup_pdu_type, { "PDU Type", "iuup.pdu_type", FT_UINT8, BASE_DEC, VALS(iuup_pdu_types),0xf0,NULL,HFILL}},
        { &hf_iuup_frame_number, { "Frame Number", "iuup.framenum", FT_UINT8, BASE_DEC, NULL,0x0F,NULL,HFILL}},
        { &hf_iuup_fqc, { "FQC", "iuup.fqc", FT_UINT8, BASE_DEC, VALS(iuup_fqcs),0xc0,"Frame Quality Classification",HFILL}},
        { &hf_iuup_rfci, { "RFCI", "iuup.rfci", FT_UINT8, BASE_HEX, NULL, 0x3f, "RAB sub-Flow Combination Indicator",HFILL}},
        { &hf_iuup_hdr_crc, { "Header CRC", "iuup.header_crc", FT_UINT8, BASE_HEX, NULL,0xfc,NULL,HFILL}},
        { &hf_iuup_payload_crc, { "Payload CRC", "iuup.payload_crc", FT_UINT16, BASE_HEX, NULL,0x03FF,NULL,HFILL}},
        { &hf_iuup_ack_nack, { "Ack/Nack", "iuup.ack", FT_UINT8, BASE_DEC, VALS(iuup_acknack_vals),0x0c,NULL,HFILL}},
        { &hf_iuup_frame_number_t14, { "Frame Number", "iuup.framenum_t14", FT_UINT8, BASE_DEC, NULL,0x03,NULL,HFILL}},
        { &hf_iuup_mode_version, { "Mode Version", "iuup.mode", FT_UINT8, BASE_HEX, NULL,0xf0,NULL,HFILL}},
        { &hf_iuup_procedure_indicator, { "Procedure", "iuup.procedure", FT_UINT8, BASE_DEC, VALS(iuup_procedures),0x0f,NULL,HFILL}},
        { &hf_iuup_error_cause_val, { "Error Cause", "iuup.error_cause", FT_UINT8, BASE_DEC, VALS(iuup_error_causes),0xfc,NULL,HFILL}},
        { &hf_iuup_error_distance, { "Error DISTANCE", "iuup.error_distance", FT_UINT8, BASE_DEC, VALS(iuup_error_distances),0xc0,NULL,HFILL}},
        { &hf_iuup_errorevt_cause_val, { "Error Cause", "iuup.errorevt_cause", FT_UINT8, BASE_DEC, NULL,0x3f,NULL,HFILL}},
        { &hf_iuup_time_align, { "Time Align", "iuup.time_align", FT_UINT8, BASE_HEX, NULL,0x0,NULL,HFILL}},
        { &hf_iuup_data_pdu_type, { "RFCI Data Pdu Type", "iuup.data_pdu_type", FT_UINT8, BASE_HEX, VALS(iuup_payload_pdu_type),0xF0,NULL,HFILL}},

        { &hf_iuup_spare_03, { "Spare", "iuup.spare", FT_UINT8, BASE_HEX, NULL,0x03,NULL,HFILL}},
#if 0
        { &hf_iuup_spare_0f, { "Spare", "iuup.spare", FT_UINT8, BASE_HEX, NULL,0x0f,NULL,HFILL}},
#endif
#if 0
        { &hf_iuup_spare_c0, { "Spare", "iuup.spare", FT_UINT8, BASE_HEX, NULL,0xc0,NULL,HFILL}},
#endif
        { &hf_iuup_spare_e0, { "Spare", "iuup.spare", FT_UINT8, BASE_HEX, NULL,0xe0,NULL,HFILL}},
        { &hf_iuup_spare_ff, { "Spare", "iuup.spare", FT_UINT8, BASE_HEX, NULL,0xff,NULL,HFILL}},
        { &hf_iuup_spare_bytes, { "Spare", "iuup.spare_bytes", FT_BYTES, BASE_NONE, NULL,0x0,NULL,HFILL}},

        { &hf_iuup_delay, { "Delay", "iuup.delay", FT_UINT32, BASE_HEX, NULL,0x0,NULL,HFILL}},
        { &hf_iuup_advance, { "Advance", "iuup.advance", FT_UINT32, BASE_HEX, NULL,0x0,NULL,HFILL}},
        { &hf_iuup_delta, { "Delta Time", "iuup.delta", FT_FLOAT, BASE_NONE, NULL,0x0,NULL,HFILL}},

        { &hf_iuup_init_ti, { "TI", "iuup.ti", FT_UINT8, BASE_DEC, VALS(iuup_ti_vals),0x10,"Timing Information",HFILL}},
        { &hf_iuup_init_subflows_per_rfci, { "Subflows", "iuup.subflows", FT_UINT8, BASE_DEC, NULL,0x0e,"Number of Subflows",HFILL}},
        { &hf_iuup_init_chain_ind, { "Chain Indicator", "iuup.chain_ind", FT_UINT8, BASE_DEC, VALS(iuup_init_chain_ind_vals),0x01,NULL,HFILL}},
        { &hf_iuup_payload, { "Payload Data", "iuup.payload_data", FT_BYTES, BASE_NONE, NULL,0x00,NULL,HFILL}},


        { &hf_iuup_mode_versions, { "Iu UP Mode Versions Supported", "iuup.support_mode", FT_UINT16, BASE_HEX, NULL,0x0,NULL,HFILL}},

        { &hf_iuup_mode_versions_a[ 0], { "Version 16", "iuup.support_mode.version16", FT_UINT16, BASE_HEX, VALS(iuup_mode_version_support),0x8000,NULL,HFILL}},
        { &hf_iuup_mode_versions_a[ 1], { "Version 15", "iuup.support_mode.version15", FT_UINT16, BASE_HEX, VALS(iuup_mode_version_support),0x4000,NULL,HFILL}},
        { &hf_iuup_mode_versions_a[ 2], { "Version 14", "iuup.support_mode.version14", FT_UINT16, BASE_HEX, VALS(iuup_mode_version_support),0x2000,NULL,HFILL}},
        { &hf_iuup_mode_versions_a[ 3], { "Version 13", "iuup.support_mode.version13", FT_UINT16, BASE_HEX, VALS(iuup_mode_version_support),0x1000,NULL,HFILL}},
        { &hf_iuup_mode_versions_a[ 4], { "Version 12", "iuup.support_mode.version12", FT_UINT16, BASE_HEX, VALS(iuup_mode_version_support),0x0800,NULL,HFILL}},
        { &hf_iuup_mode_versions_a[ 5], { "Version 11", "iuup.support_mode.version11", FT_UINT16, BASE_HEX, VALS(iuup_mode_version_support),0x0400,NULL,HFILL}},
        { &hf_iuup_mode_versions_a[ 6], { "Version 10", "iuup.support_mode.version10", FT_UINT16, BASE_HEX, VALS(iuup_mode_version_support),0x0200,NULL,HFILL}},
        { &hf_iuup_mode_versions_a[ 7], { "Version  9", "iuup.support_mode.version9", FT_UINT16, BASE_HEX, VALS(iuup_mode_version_support),0x0100,NULL,HFILL}},
        { &hf_iuup_mode_versions_a[ 8], { "Version  8", "iuup.support_mode.version8", FT_UINT16, BASE_HEX, VALS(iuup_mode_version_support),0x0080,NULL,HFILL}},
        { &hf_iuup_mode_versions_a[ 9], { "Version  7", "iuup.support_mode.version7", FT_UINT16, BASE_HEX, VALS(iuup_mode_version_support),0x0040,NULL,HFILL}},
        { &hf_iuup_mode_versions_a[10], { "Version  6", "iuup.support_mode.version6", FT_UINT16, BASE_HEX, VALS(iuup_mode_version_support),0x0020,NULL,HFILL}},
        { &hf_iuup_mode_versions_a[11], { "Version  5", "iuup.support_mode.version5", FT_UINT16, BASE_HEX, VALS(iuup_mode_version_support),0x0010,NULL,HFILL}},
        { &hf_iuup_mode_versions_a[12], { "Version  4", "iuup.support_mode.version4", FT_UINT16, BASE_HEX, VALS(iuup_mode_version_support),0x0008,NULL,HFILL}},
        { &hf_iuup_mode_versions_a[13], { "Version  3", "iuup.support_mode.version3", FT_UINT16, BASE_HEX, VALS(iuup_mode_version_support),0x0004,NULL,HFILL}},
        { &hf_iuup_mode_versions_a[14], { "Version  2", "iuup.support_mode.version2", FT_UINT16, BASE_HEX, VALS(iuup_mode_version_support),0x0002,NULL,HFILL}},
        { &hf_iuup_mode_versions_a[15], { "Version  1", "iuup.support_mode.version1", FT_UINT16, BASE_HEX, VALS(iuup_mode_version_support),0x0001,NULL,HFILL}},

        { &hf_iuup_num_rfci_ind, { "Number of RFCI Indicators", "iuup.p", FT_UINT8, BASE_HEX, NULL,0x3f,NULL,HFILL}},
        { &hf_iuup_init_rfci_ind, { "RFCI Initialization", "iuup.rfci.init", FT_BYTES, BASE_NONE, NULL,0x0,NULL,HFILL}},

        HFS_RFCI(0),HFS_RFCI(1),HFS_RFCI(2),HFS_RFCI(3),HFS_RFCI(4),HFS_RFCI(5),HFS_RFCI(6),HFS_RFCI(7),
        HFS_RFCI(8),HFS_RFCI(9),HFS_RFCI(10),HFS_RFCI(11),HFS_RFCI(12),HFS_RFCI(13),HFS_RFCI(14),HFS_RFCI(15),
        HFS_RFCI(16),HFS_RFCI(17),HFS_RFCI(18),HFS_RFCI(19),HFS_RFCI(20),HFS_RFCI(21),HFS_RFCI(22),HFS_RFCI(23),
        HFS_RFCI(24),HFS_RFCI(25),HFS_RFCI(26),HFS_RFCI(27),HFS_RFCI(28),HFS_RFCI(29),HFS_RFCI(30),HFS_RFCI(31),
        HFS_RFCI(32),HFS_RFCI(33),HFS_RFCI(34),HFS_RFCI(35),HFS_RFCI(36),HFS_RFCI(37),HFS_RFCI(38),HFS_RFCI(39),
        HFS_RFCI(40),HFS_RFCI(41),HFS_RFCI(42),HFS_RFCI(43),HFS_RFCI(44),HFS_RFCI(45),HFS_RFCI(46),HFS_RFCI(47),
        HFS_RFCI(48),HFS_RFCI(49),HFS_RFCI(50),HFS_RFCI(51),HFS_RFCI(52),HFS_RFCI(53),HFS_RFCI(54),HFS_RFCI(55),
        HFS_RFCI(56),HFS_RFCI(57),HFS_RFCI(58),HFS_RFCI(59),HFS_RFCI(60),HFS_RFCI(61),HFS_RFCI(62),HFS_RFCI(63)

    };


    gint* ett[] = {
        &ett_iuup,
        &ett_rfci,
        &ett_ipti,
        &ett_support,
        &ett_time,
        &ett_rfciinds,
        &ett_payload,
        &ett_payload_subflows
    };

    static ei_register_info ei[] = {
        { &ei_iuup_hdr_crc_bad, { "iuup.hdr.crc.bad", PI_CHECKSUM, PI_ERROR, "Bad checksum", EXPFILL }},
        { &ei_iuup_payload_crc_bad, { "iuup.payload.crc.bad", PI_CHECKSUM, PI_ERROR, "Bad checksum", EXPFILL }},
        { &ei_iuup_payload_undecoded, { "iuup.payload.undecoded", PI_UNDECODED, PI_WARN, "Undecoded payload", EXPFILL }},
        { &ei_iuup_error_response, { "iuup.error_response", PI_RESPONSE_CODE, PI_ERROR, "Error response", EXPFILL }},
        { &ei_iuup_ack_nack, { "iuup.ack.malformed", PI_MALFORMED, PI_ERROR, "Malformed Ack/Nack", EXPFILL }},
        { &ei_iuup_time_align, { "iuup.time_align.malformed", PI_MALFORMED, PI_ERROR, "Malformed Time Align", EXPFILL }},
        { &ei_iuup_procedure_indicator, { "iuup.procedure.malformed", PI_MALFORMED, PI_ERROR, "Malformed Procedure", EXPFILL }},
        { &ei_iuup_pdu_type, { "iuup.pdu_type.malformed", PI_MALFORMED, PI_ERROR, "Malformed PDU Type", EXPFILL }},
    };

    module_t *iuup_module;
    expert_module_t* expert_iuup;

    proto_iuup = proto_register_protocol("IuUP", "IuUP", "iuup");
    proto_register_field_array(proto_iuup, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
    expert_iuup = expert_register_protocol(proto_iuup);
    expert_register_field_array(expert_iuup, ei, array_length(ei));
    register_dissector("iuup", dissect_iuup, proto_iuup);
    register_dissector("find_iuup", find_iuup, proto_iuup);

    register_init_routine(&init_iuup);
    register_cleanup_routine(&cleanup_iuup);

    iuup_module = prefs_register_protocol(proto_iuup, proto_reg_handoff_iuup);

    prefs_register_bool_preference(iuup_module, "dissect_payload",
                                   "Dissect IuUP Payload bits",
                                   "Whether IuUP Payload bits should be dissected",
                                   &dissect_fields);

    prefs_register_bool_preference(iuup_module, "two_byte_pseudoheader",
                                   "Two byte pseudoheader",
                                   "The payload contains a two byte pseudoheader indicating direction and circuit_id",
                                   &two_byte_pseudoheader);

    prefs_register_uint_preference(iuup_module, "dynamic.payload.type",
                                   "IuUP dynamic payload type",
                                   "The dynamic payload type which will be interpreted as IuUP",
                                   10,
                                   &global_dynamic_payload_type);
}

/*
 * Editor modelines
 *
 * Local Variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * ex: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
