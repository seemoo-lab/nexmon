/* packet-reload-framing.c
 * Routines for REsource LOcation And Discovery (RELOAD) Framing
 * Author: Stephane Bryant <sbryant@glycon.org>
 * Copyright 2010 Stonyfish Inc.
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
 *
 * Please refer to the following specs for protocol detail:
 * - draft-ietf-p2psip-base-15
 */

#include "config.h"

#include <epan/packet.h>
#include <epan/expert.h>
#include <epan/tap.h>
#include <epan/exported_pdu.h>
#include "packet-tcp.h"

void proto_register_reload_framing(void);
void proto_reg_handoff_reload_framing(void);

/* Initialize the protocol and registered fields */
static int proto_reload_framing = -1;

static int hf_reload_framing_type = -1;
static int hf_reload_framing_sequence = -1;
static int hf_reload_framing_ack_sequence = -1;
static int hf_reload_framing_message = -1;
static int hf_reload_framing_message_length = -1;
static int hf_reload_framing_message_data = -1;
static int hf_reload_framing_received = -1;
static int hf_reload_framing_parsed_received = -1;
static int hf_reload_framing_duplicate = -1;
static int hf_reload_framing_response_in = -1;
static int hf_reload_framing_response_to = -1;
static int hf_reload_framing_time = -1;

static dissector_handle_t reload_handle;

static gint exported_pdu_tap = -1;

/* Structure containing transaction specific information */
typedef struct _reload_frame_t {
  guint32  data_frame;
  guint32  ack_frame;
  nstime_t req_time;
} reload_frame_t;

/* Structure containing conversation specific information */
typedef struct _reload_frame_conv_info_t {
  wmem_tree_t *transaction_pdus;
} reload_conv_info_t;


/* RELOAD Message classes = (message_code & 0x1) (response = request +1) */
#define DATA            128
#define ACK             129


/* Initialize the subtree pointers */
static gint ett_reload_framing = -1;
static gint ett_reload_framing_message = -1;
static gint ett_reload_framing_received = -1;

static expert_field ei_reload_no_dissector = EI_INIT;

#define UDP_PORT_RELOAD                 6084
#define TCP_PORT_RELOAD                 6084

#define MIN_HDR_LENGTH                             9
#define MIN_RELOADDATA_HDR_LENGTH                  38

#define RELOAD_TOKEN                    0xd2454c4f

static const value_string types[] = {
  {DATA, "DATA"},
  {ACK,  "ACK"},
  {0x00, NULL}
};

static guint
get_reload_framing_message_length(packet_info *pinfo _U_, tvbuff_t *tvb,
                                  int offset, void *data _U_)
{
  /* Get the type */
  guint32 length = 9;


  if (tvb_get_guint8(tvb, offset) == DATA) {
    length = 1 + 4 + 3 + tvb_get_ntoh24(tvb, 1 + 4);
  }

  return length;
}


static int
dissect_reload_framing_message(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, gboolean from_dtls)
{
  proto_item         *ti;
  proto_tree         *reload_framing_tree;
  guint32             relo_token;
  guint32             message_length = 0;
  wmem_tree_key_t     transaction_id_key[4];
  guint32            *key_save, len_save;
  guint32             sequence;
  guint               effective_length;
  guint16             offset;
  conversation_t     *conversation;
  reload_conv_info_t *reload_framing_info = NULL;
  reload_frame_t *    reload_frame;
  guint8              type;

  offset = 0;
  effective_length = tvb_captured_length(tvb);

  /* First, make sure we have enough data to do the check. */
  if (effective_length < MIN_HDR_LENGTH)
    return 0;

  conversation = find_conversation(pinfo->num, &pinfo->src, &pinfo->dst,
                                   pinfo->ptype, pinfo->srcport, pinfo->destport, 0);
  if (conversation)
    reload_framing_info = (reload_conv_info_t *)conversation_get_proto_data(conversation, proto_reload_framing);

  /* Get the type
   * http://tools.ietf.org/html/draft-ietf-p2psip-base-12
   * 5.6.2.  Framing Header
   */
  type = tvb_get_guint8(tvb, 0);

  switch(type) {
  case DATA:
    /* in the data type, check the reload token to be sure this
    *  is a reLoad packet
    */
    if (effective_length < 12)  /* [type + seq + length + token] */
      return 0;

    relo_token = tvb_get_ntohl(tvb,1 + 4 + 3);
    if (relo_token != RELOAD_TOKEN) {
      return 0;
    }
    message_length = tvb_get_ntoh24(tvb, 1 + 4);
    if (message_length < MIN_RELOADDATA_HDR_LENGTH) {
      return 0;
    }
    break;
  case ACK:
    /* Require previous ACK (i.e., reload_framing_info attached to conversation). */
    if (effective_length < 9 || ! reload_framing_info) {
      return 0;
    }
    break;
  default:
    return 0;
  }

  if (from_dtls && have_tap_listener(exported_pdu_tap)) {
    exp_pdu_data_t *exp_pdu_data = export_pdu_create_common_tags(pinfo, "reload-framing", EXP_PDU_TAG_PROTO_NAME);

    exp_pdu_data->tvb_captured_length = effective_length;
    exp_pdu_data->tvb_reported_length = tvb_reported_length(tvb);
    exp_pdu_data->pdu_tvb = tvb;

    tap_queue_packet(exported_pdu_tap, pinfo, exp_pdu_data);
  }

  /* The message seems to be a valid RELOAD framing message! */

  col_set_str(pinfo->cinfo, COL_PROTOCOL, "RELOAD Frame");
  col_clear(pinfo->cinfo, COL_INFO);

  /* Create the transaction key which may be used to track the conversation */

  sequence = tvb_get_ntohl(tvb, 1);
  transaction_id_key[0].length = 1;
  transaction_id_key[0].key = &sequence; /* sequence number */

  /* When the wmem_tree_* functions iterate through the keys, they
   * perform pointer arithmetic with guint32s, so we have to divide
   * our length fields by that to make things work, but we still want
   * to g_malloc and memcpy the entire amounts, since those both operate
   * in raw bytes. */
  if (type==DATA) {
    transaction_id_key[1].length = 1;
    transaction_id_key[1].key    = &pinfo->srcport;
    transaction_id_key[2].length = (pinfo->src.len) / (guint)sizeof(guint32);
    transaction_id_key[2].key    = (guint32 *)g_malloc(pinfo->src.len);
    memcpy(transaction_id_key[2].key, pinfo->src.data, pinfo->src.len);
  }
  else {
    transaction_id_key[1].length = 1;
    transaction_id_key[1].key    = &pinfo->destport;
    transaction_id_key[2].length = (pinfo->dst.len) / (guint)sizeof(guint32);
    transaction_id_key[2].key    = (guint32 *)g_malloc(pinfo->dst.len);
    memcpy(transaction_id_key[2].key, pinfo->dst.data, pinfo->dst.len);
  }
  transaction_id_key[3].length=0;
  transaction_id_key[3].key=NULL;
  /* The tree functions are destructive to this part of the key, so save the
   * proper values here and restore them after each call. */
  key_save = transaction_id_key[2].key;
  len_save = transaction_id_key[2].length;

  if (!conversation) {
    conversation = conversation_new(pinfo->num, &pinfo->src, &pinfo->dst,
                                    pinfo->ptype, pinfo->srcport, pinfo->destport, 0);
  }

  /*
   * Do we already have a state structure for this conv
   */
  if (!reload_framing_info) {
    /* No.  Attach that information to the conversation, and add
     * it to the list of information structures.
     */
    reload_framing_info = wmem_new(wmem_file_scope(), reload_conv_info_t);
    reload_framing_info->transaction_pdus = wmem_tree_new(wmem_file_scope());
    conversation_add_proto_data(conversation, proto_reload_framing, reload_framing_info);
  }

  if (!pinfo->fd->flags.visited) {
    if ((reload_frame = (reload_frame_t *)
           wmem_tree_lookup32_array(reload_framing_info->transaction_pdus, transaction_id_key)) == NULL) {
      transaction_id_key[2].key    = key_save;
      transaction_id_key[2].length = len_save;
      reload_frame = wmem_new(wmem_file_scope(), reload_frame_t);
      reload_frame->data_frame = 0;
      reload_frame->ack_frame  = 0;
      reload_frame->req_time   = pinfo->abs_ts;
      wmem_tree_insert32_array(reload_framing_info->transaction_pdus, transaction_id_key, (void *)reload_frame);
    }
    transaction_id_key[2].key    = key_save;
    transaction_id_key[2].length = len_save;

    /* check whether the message is a request or a response */

    if (type == DATA) {
      /* This is a data */
      if (reload_frame->data_frame == 0) {
        reload_frame->data_frame = pinfo->num;
      }
    }
    else {
      /* This is a catch-all for all non-request messages */
      if (reload_frame->ack_frame == 0) {
        reload_frame->ack_frame = pinfo->num;
      }
    }
  }
  else {
    reload_frame=(reload_frame_t *)wmem_tree_lookup32_array(reload_framing_info->transaction_pdus, transaction_id_key);
    transaction_id_key[2].key    = key_save;
    transaction_id_key[2].length = len_save;
  }
  g_free(transaction_id_key[2].key);

  if (!reload_frame) {
    /* create a "fake" pana_trans structure */
    reload_frame = wmem_new(wmem_packet_scope(), reload_frame_t);
    reload_frame->data_frame = (type==DATA) ? pinfo->num : 0;
    reload_frame->ack_frame  = (type!=DATA) ? pinfo->num : 0;
    reload_frame->req_time   = pinfo->abs_ts;
  }

  ti = proto_tree_add_item(tree, proto_reload_framing, tvb, 0, -1, ENC_NA);

  reload_framing_tree = proto_item_add_subtree(ti, ett_reload_framing);

  col_add_fstr(pinfo->cinfo, COL_INFO, "%s", val_to_str_const(type, types, "Unknown"));
  proto_item_append_text(ti, ": %s", val_to_str_const(type, types, "Unknown"));

  /* Retransmission control */
  if (type == DATA) {
    if (reload_frame->data_frame != pinfo->num) {
      proto_item *it;
      it = proto_tree_add_uint(reload_framing_tree, hf_reload_framing_duplicate, tvb, 0, 0, reload_frame->data_frame);
      PROTO_ITEM_SET_GENERATED(it);
    }
    if (reload_frame->ack_frame) {
      proto_item *it;
      it = proto_tree_add_uint(reload_framing_tree, hf_reload_framing_response_in, tvb, 0, 0, reload_frame->ack_frame);
      PROTO_ITEM_SET_GENERATED(it);
    }
  }
  else {
    /* This is a response */
    if (reload_frame->ack_frame != pinfo->num) {
      proto_item *it;
      it = proto_tree_add_uint(reload_framing_tree, hf_reload_framing_duplicate, tvb, 0, 0, reload_frame->ack_frame);
      PROTO_ITEM_SET_GENERATED(it);
    }

    if (reload_frame->data_frame) {
      proto_item *it;
      nstime_t    ns;

      it = proto_tree_add_uint(reload_framing_tree, hf_reload_framing_response_to, tvb, 0, 0, reload_frame->data_frame);
      PROTO_ITEM_SET_GENERATED(it);

      nstime_delta(&ns, &pinfo->abs_ts, &reload_frame->req_time);
      it = proto_tree_add_time(reload_framing_tree, hf_reload_framing_time, tvb, 0, 0, &ns);
      PROTO_ITEM_SET_GENERATED(it);
    }
  }

  /*
   * Message dissection
   */
  proto_tree_add_item(reload_framing_tree, hf_reload_framing_type, tvb, offset , 1, ENC_BIG_ENDIAN);
  offset += 1;
  switch (type) {

  case DATA:
  {
    tvbuff_t   *next_tvb;
    proto_item *ti_message;
    proto_tree *message_tree;

    proto_tree_add_item(reload_framing_tree, hf_reload_framing_sequence, tvb, offset , 4, ENC_BIG_ENDIAN);
    offset += 4;
    ti_message = proto_tree_add_item(reload_framing_tree, hf_reload_framing_message, tvb, offset, 3+message_length, ENC_NA);
    proto_item_append_text(ti_message, " (opaque<%d>)", message_length);
    message_tree =  proto_item_add_subtree(ti_message, ett_reload_framing_message);
    proto_tree_add_item(message_tree, hf_reload_framing_message_length, tvb, offset, 3, ENC_BIG_ENDIAN);
    offset += 3;
    proto_tree_add_item(message_tree, hf_reload_framing_message_data, tvb, offset, message_length, ENC_NA);
    next_tvb = tvb_new_subset(tvb, offset, effective_length - offset, message_length);
    if (reload_handle == NULL) {
      expert_add_info(pinfo, ti, &ei_reload_no_dissector);
      return tvb_captured_length(tvb);
    }
    call_dissector_only(reload_handle, next_tvb, pinfo, tree, NULL);
  }
  break;

  case ACK:
  {
    proto_item *ti_received;

    proto_tree_add_uint(reload_framing_tree, hf_reload_framing_ack_sequence, tvb, offset , 4, sequence);
    offset += 4;

    ti_received = proto_tree_add_item(reload_framing_tree, hf_reload_framing_received, tvb, offset , 4, ENC_BIG_ENDIAN);
    {
      guint32     received;
      int         last_received      = -1;
      unsigned int         indx      = 0;
      proto_tree *received_tree;
      proto_item *ti_parsed_received = NULL;

      received = tvb_get_ntohl(tvb, offset);
      while ((indx<32) && (received<<indx) != 0) {
        if (received &(1U<<(31-indx))) {
          if (indx==0) {
            received_tree = proto_item_add_subtree(ti_received, ett_reload_framing_received);
            ti_parsed_received = proto_tree_add_item(received_tree, hf_reload_framing_parsed_received, tvb, offset, 4, ENC_NA);
            proto_item_append_text(ti_parsed_received, "[%u", (sequence -32+indx));
            last_received = indx;
          }
          else {
            if (received &(1U<<(31-indx+1))) {
              indx++;
              /* the previous one is also acked: in the middle of a range: skip */
              continue;
            }
            else {
              /* 1st acked in a serie */
              if (last_received<0) {
                /* 1st acked ever */
                received_tree = proto_item_add_subtree(ti_received, ett_reload_framing_received);
                ti_parsed_received = proto_tree_add_item(received_tree, hf_reload_framing_parsed_received, tvb, offset, 4, ENC_NA);
                proto_item_append_text(ti_parsed_received, "[%u",(sequence-32+indx));
              }
              else {
                proto_item_append_text(ti_parsed_received, ",%u",(sequence-32+indx));
              }
              last_received = indx;

            }
          }
        }
        else if (indx>0) {
          if ((indx>1) && (received &(1U<<(31-indx+1))) && (received &(1U<<(31-indx+2)))) {
            /* end of a series */
            if ((indx>2) && (received &(1U<<(31-indx+3)))) {
              proto_item_append_text(ti_parsed_received,"-%u",(sequence-32+indx-1));
            }
            else {
              /* just a pair */
              proto_item_append_text(ti_received, ",%u", (sequence-32+indx-1));
            }
          }
          else {
            indx++;
            continue;
          }
        }
        indx++;
      }
      if (last_received>=0) {
        if ((indx>1) && (received &(1U<<(31-indx+1)))  && (received &(1U<<(31-indx+2)))) {
          /* end of a series */
          if ((indx>2) && (received &(1U<<(31-indx+3)))) {
            proto_item_append_text(ti_parsed_received,"-%u",(sequence-32+indx-1));
          }
          else {
            /* just a pair */
            proto_item_append_text(ti_parsed_received, ",%u", (sequence-32+indx-1));
          }
        }
        proto_item_append_text(ti_parsed_received, "]");
        PROTO_ITEM_SET_GENERATED(ti_parsed_received);
      }
    }
  }
  break;

  default:
    DISSECTOR_ASSERT_NOT_REACHED();
  }

  return tvb_captured_length(tvb);
}

static int
dissect_reload_framing(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
  return dissect_reload_framing_message(tvb, pinfo, tree, FALSE);
}

static int
dissect_reload_framing_tcp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data)
{
  /* XXX: Check if we have a valid RELOAD Frame Type ? */
  tcp_dissect_pdus(tvb, pinfo, tree, TRUE, MIN_HDR_LENGTH,
                   get_reload_framing_message_length, dissect_reload_framing, data);
  return tvb_captured_length(tvb);
}

/* ToDo: If a TCP connection is identified heuristically as reload-framing, then
 *        the code should be such that reload-framing PDUs can be re-assembled (as is
 *        done for a TCP connection identified as reload-framing because of
 *        the TCP port used).
 */
static gboolean
dissect_reload_framing_heur(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
  if (dissect_reload_framing_message(tvb, pinfo, tree, FALSE) == 0) {
    /*
     * It wasn't a valid RELOAD message, and wasn't
     * dissected as such.
     */
    return FALSE;
  }
  return TRUE;
}

static gboolean
dissect_reload_framing_heur_dtls(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
  if (dissect_reload_framing_message(tvb, pinfo, tree, TRUE) == 0) {
    /*
     * It wasn't a valid RELOAD message, and wasn't
     * dissected as such.
     */
    return FALSE;
  }
  return TRUE;
}

void
proto_register_reload_framing(void)
{

  static hf_register_info hf[] = {
    { &hf_reload_framing_type,
      { "type (FramedMessageType)", "reload_framing.type", FT_UINT8,
        BASE_DEC, VALS(types),  0x0,  NULL, HFILL
      }
    },
    { &hf_reload_framing_sequence,
      { "sequence (uint32)", "reload_framing.sequence", FT_UINT32,
        BASE_DEC, NULL, 0x0,  NULL, HFILL
      }
    },
    { &hf_reload_framing_ack_sequence,
      { "ack_sequence (uint32)", "reload_framing.ack_sequence", FT_UINT32,
        BASE_DEC, NULL, 0x0,  NULL, HFILL
      }
    },
    { &hf_reload_framing_message,
      { "message", "reload_framing.message", FT_NONE,
        BASE_NONE, NULL, 0x0,  NULL, HFILL
      }
    },
    { &hf_reload_framing_message_length,
      { "length (uint24)", "reload_framing.message.length", FT_UINT32,
        BASE_DEC, NULL, 0x0,  NULL, HFILL
      }
    },
    { &hf_reload_framing_message_data,
      { "data", "reload_framing.message.data", FT_BYTES,
        BASE_NONE, NULL, 0x0,  NULL, HFILL
      }
    },
    { &hf_reload_framing_received,
      { "received (uint32)", "reload_framing.received", FT_UINT32,
        BASE_HEX, NULL, 0x0,  NULL, HFILL
      }
    },
    { &hf_reload_framing_parsed_received,
      { "Acked Frames:",  "reload_framing.parsed_received", FT_NONE,
        BASE_NONE, NULL, 0x0, NULL, HFILL
      }
    },
    { &hf_reload_framing_response_in,
      { "Response In",  "reload_framing.response-in", FT_FRAMENUM,
        BASE_NONE, NULL, 0x0, "The response to this RELOAD Request is in this frame", HFILL
      }
    },
    { &hf_reload_framing_response_to,
      { "Request In", "reload_framing.response-to", FT_FRAMENUM,
        BASE_NONE, NULL, 0x0, "This is a response to the RELOAD Request in this frame", HFILL
      }
    },
    { &hf_reload_framing_time,
      { "Time", "reload_framing.time", FT_RELATIVE_TIME,
        BASE_NONE, NULL, 0x0, "The time between the Request and the Response", HFILL
      }
    },
    { &hf_reload_framing_duplicate,
      { "Duplicated original message in", "reload_framing.duplicate", FT_FRAMENUM,
        BASE_NONE, NULL, 0x0, "This is a duplicate of RELOAD message in this frame", HFILL
      }
    },
  };

  /* Setup protocol subtree array */
  static gint *ett[] = {
    &ett_reload_framing,
    &ett_reload_framing_message,
    &ett_reload_framing_received,
  };

  static ei_register_info ei[] = {
     { &ei_reload_no_dissector, { "reload_framing.no_dissector", PI_PROTOCOL, PI_WARN, "Can not find reload dissector", EXPFILL }},
  };

  expert_module_t* expert_reload_framing;

  /* Register the protocol name and description */
  proto_reload_framing = proto_register_protocol("REsource LOcation And Discovery Framing", "RELOAD FRAMING", "reload-framing");

  /* Required function calls to register the header fields and subtrees used */
  proto_register_field_array(proto_reload_framing, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));
  expert_reload_framing = expert_register_protocol(proto_reload_framing);
  expert_register_field_array(expert_reload_framing, ei, array_length(ei));

  register_dissector("reload-framing", dissect_reload_framing, proto_reload_framing);

}

void
proto_reg_handoff_reload_framing(void)
{

  dissector_handle_t reload_framing_tcp_handle;
  dissector_handle_t reload_framing_udp_handle;

  reload_framing_tcp_handle = create_dissector_handle(dissect_reload_framing_tcp, proto_reload_framing);
  reload_framing_udp_handle = create_dissector_handle(dissect_reload_framing, proto_reload_framing);

  reload_handle = find_dissector_add_dependency("reload", proto_reload_framing);

  dissector_add_uint("tcp.port", TCP_PORT_RELOAD, reload_framing_tcp_handle);
  dissector_add_uint("udp.port", UDP_PORT_RELOAD, reload_framing_udp_handle);

  heur_dissector_add("udp",  dissect_reload_framing_heur, "RELOAD Framing over UDP", "reload_framing_udp", proto_reload_framing, HEURISTIC_ENABLE);
  heur_dissector_add("tcp",  dissect_reload_framing_heur, "RELOAD Framing over TCP", "reload_framing_tcp", proto_reload_framing, HEURISTIC_ENABLE);
  heur_dissector_add("dtls", dissect_reload_framing_heur_dtls, "RELOAD Framing over DTLS", "reload_framing_dtls", proto_reload_framing, HEURISTIC_ENABLE);

  exported_pdu_tap = find_tap_id(EXPORT_PDU_TAP_NAME_LAYER_7);
}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 2
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=2 tabstop=8 expandtab:
 * :indentSize=2:tabSize=8:noTabs=true:
 */
