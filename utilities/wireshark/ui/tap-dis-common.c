/* tap-dis-common.c
 * DIS stream handler functions used by tshark and wireshark.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <math.h>
#include <string.h>

#include <glib.h>

#include <epan/dissectors/packet-dis.h>

#include "tap-dis-common.h"

#define DISSTREAM_TAP_NAME "dis"
#define DISSTREAM_CODEC_CLOCK_HZ 8000.0
#define DISSTREAM_TX_WRAP_MS 3600000.0
#define DISSTREAM_STOPPED_STATE 0

void
disstream_packet_free(disstream_packet_t *packet)
{
    if (!packet) {
        return;
    }

    g_free(packet->payload_data);
    g_free(packet);
}

void
disstream_id_copy(const disstream_id_t *src, disstream_id_t *dst)
{
    copy_address(&dst->src_addr, &src->src_addr);
    dst->src_port = src->src_port;
    copy_address(&dst->dst_addr, &src->dst_addr);
    dst->dst_port = src->dst_port;
    dst->radio_id = src->radio_id;
    dst->entity_id_site = src->entity_id_site;
    dst->entity_id_appl = src->entity_id_appl;
    dst->entity_id_entity = src->entity_id_entity;
}

void
disstream_id_copy_pinfo(const packet_info *pinfo, disstream_id_t *dst)
{
    copy_address(&dst->src_addr, &pinfo->src);
    dst->src_port = pinfo->srcport;
    copy_address(&dst->dst_addr, &pinfo->dst);
    dst->dst_port = pinfo->destport;
}

void
disstream_id_copy_pinfo_shallow(const packet_info *pinfo, disstream_id_t *dst)
{
    copy_address_shallow(&dst->src_addr, &pinfo->src);
    dst->src_port = pinfo->srcport;
    copy_address_shallow(&dst->dst_addr, &pinfo->dst);
    dst->dst_port = pinfo->destport;
}

void
disstream_id_free(disstream_id_t *id)
{
    free_address(&id->src_addr);
    free_address(&id->dst_addr);
    memset(id, 0, sizeof(*id));
}

unsigned
disstream_id_to_hash(const disstream_id_t *id)
{
    unsigned hash = 0;

    if (!id) {
        return 0;
    }

    hash ^= id->src_port | id->dst_port << 16;
    hash ^= id->radio_id;
    hash ^= id->entity_id_site << 16;
    hash ^= id->entity_id_appl;
    hash ^= id->entity_id_entity << 16;
    hash = add_address_to_hash(hash, &id->src_addr);
    hash = add_address_to_hash(hash, &id->dst_addr);

    return hash;
}

bool
disstream_id_equal(const disstream_id_t *id1, const disstream_id_t *id2)
{
    if (!id1 || !id2) {
        return false;
    }

    return addresses_equal(&id1->src_addr, &id2->src_addr)
        && id1->src_port == id2->src_port
        && addresses_equal(&id1->dst_addr, &id2->dst_addr)
        && id1->dst_port == id2->dst_port
        && id1->radio_id == id2->radio_id
        && id1->entity_id_site == id2->entity_id_site
        && id1->entity_id_appl == id2->entity_id_appl
        && id1->entity_id_entity == id2->entity_id_entity;
}

void
disstream_info_init(disstream_info_t *info)
{
    memset(info, 0, sizeof(*info));
    info->payload_type = DIS_PAYLOAD_TYPE_INVALID;
    info->first_timing_packet = true;
    info->signal_packets = g_ptr_array_new_with_free_func((GDestroyNotify)disstream_packet_free);
}

disstream_info_t *
disstream_info_malloc_and_init(void)
{
    disstream_info_t *info = g_new(disstream_info_t, 1);

    disstream_info_init(info);
    return info;
}

void
disstream_info_free_data(disstream_info_t *info)
{
    if (info->signal_packets) {
        g_ptr_array_free(info->signal_packets, true);
        info->signal_packets = NULL;
    }

    disstream_id_free(&info->id);
}

void
disstream_info_free_all(disstream_info_t *info)
{
    disstream_info_free_data(info);
    g_free(info);
}

static void
disstream_multihash_destroy_value(void *key _U_, void *value, void *user_data _U_)
{
    g_list_free((GList *)value);
}

static disstream_info_t *
disstream_info_multihash_lookup(GHashTable *multihash, const disstream_id_t *stream_id)
{
    GList *hlist;
    GList *iter;

    if (!multihash || !stream_id) {
        return NULL;
    }

    hlist = (GList *)g_hash_table_lookup(multihash, GINT_TO_POINTER(disstream_id_to_hash(stream_id)));
    if (!hlist) {
        return NULL;
    }

    iter = g_list_first(hlist);
    while (iter) {
        disstream_info_t *candidate = (disstream_info_t *)iter->data;

        if (disstream_id_equal(stream_id, &candidate->id)) {
            return candidate;
        }

        iter = g_list_next(iter);
    }

    return NULL;
}

static void
disstream_info_multihash_insert(GHashTable *multihash, disstream_info_t *stream_info)
{
    unsigned hash;
    GList *hlist;
    GList *iter;

    if (!multihash || !stream_info) {
        return;
    }

    hash = disstream_id_to_hash(&stream_info->id);
    hlist = (GList *)g_hash_table_lookup(multihash, GINT_TO_POINTER(hash));

    iter = g_list_first(hlist);
    while (iter) {
        disstream_info_t *candidate = (disstream_info_t *)iter->data;

        if (disstream_id_equal(&candidate->id, &stream_info->id)) {
            iter->data = stream_info;
            return;
        }

        iter = g_list_next(iter);
    }

    hlist = g_list_prepend(hlist, stream_info);
    g_hash_table_insert(multihash, GINT_TO_POINTER(hash), hlist);
}

void
disstream_reset(disstream_tapinfo_t *tapinfo)
{
    GList *list;

    if (!tapinfo) {
        return;
    }

    if (tapinfo->strinfo_hash) {
        g_hash_table_foreach(tapinfo->strinfo_hash, disstream_multihash_destroy_value, NULL);
        g_hash_table_destroy(tapinfo->strinfo_hash);
        tapinfo->strinfo_hash = NULL;
    }

    list = g_list_first(tapinfo->strinfo_list);
    while (list) {
        disstream_info_t *stream_info = (disstream_info_t *)list->data;

        disstream_info_free_all(stream_info);
        list = g_list_next(list);
    }

    g_list_free(tapinfo->strinfo_list);
    tapinfo->strinfo_list = NULL;
    tapinfo->nstreams = 0;
    tapinfo->npackets = 0;
}

void
disstream_reset_cb(void *arg)
{
    disstream_tapinfo_t *tapinfo = (disstream_tapinfo_t *)arg;

    if (tapinfo && tapinfo->tap_reset) {
        tapinfo->tap_reset(tapinfo);
    }

    disstream_reset(tapinfo);
}

static void
disstream_draw_cb(void *arg)
{
    disstream_tapinfo_t *tapinfo = (disstream_tapinfo_t *)arg;

    if (tapinfo && tapinfo->tap_draw) {
        tapinfo->tap_draw(tapinfo);
    }
}

void
remove_tap_listener_disstream(disstream_tapinfo_t *tapinfo)
{
    if (tapinfo && tapinfo->is_registered) {
        remove_tap_listener(tapinfo);
        tapinfo->is_registered = false;
    }
}

void
register_tap_listener_disstream(disstream_tapinfo_t *tapinfo, const char *fstring,
    disstream_tap_error_cb tap_error)
{
    GString *error_string;

    if (!tapinfo || tapinfo->is_registered) {
        return;
    }

    error_string = register_tap_listener(DISSTREAM_TAP_NAME, tapinfo, fstring, 0,
        disstream_reset_cb, disstream_packet_cb, disstream_draw_cb, NULL);
    if (error_string != NULL) {
        if (tap_error) {
            tap_error(error_string);
        }

        g_string_free(error_string, true);
        return;
    }

    tapinfo->is_registered = true;
}

static double
dis_tx_timestamp_to_ms(uint32_t tx_timestamp)
{
    return (((double)(tx_timestamp >> 1)) * DISSTREAM_TX_WRAP_MS) / 0x7fffffff;
}

static void
disstream_info_update_timing(disstream_info_t *stream_info, packet_info *pinfo,
    const dis_tap_info_t *dis_info, double *out_delta_ms, double *out_jitter_ms,
    uint32_t *out_lost_packets_added)
{
    double current_time_ms;
    double tx_ms;
    double nominal_ms;
    double nominal_delta_ms;
    double reference_delta_ms;
    double sample_time_ms;
    double sample_tx_ms;
    double jitter_diff_ms;
    double current_jitter_ms;
    double timestamp_spacing_error_ms;
    double arrival_delta_ms;
    uint32_t lost_packets;
    bool use_dis_timestamp_spacing;

    current_time_ms = nstime_to_msec(&pinfo->rel_ts);
    tx_ms = dis_tx_timestamp_to_ms(dis_info->info_tx_timestamp);

    if (out_delta_ms) {
        *out_delta_ms = 0.0;
    }
    if (out_jitter_ms) {
        *out_jitter_ms = 0.0;
    }
    if (out_lost_packets_added) {
        *out_lost_packets_added = 0;
    }

    if (stream_info->first_timing_packet) {
        stream_info->start_arrival_ms = current_time_ms;
        stream_info->prev_arrival_ms = current_time_ms;
        stream_info->first_tx_ms = tx_ms;
        stream_info->prev_tx_ms = tx_ms;
        stream_info->prev_nominal_ms = 0.0;
        stream_info->first_timing_packet = false;
        stream_info->timing_packet_count = 1;
        return;
    }

    if (tx_ms < stream_info->first_tx_ms) {
        nominal_ms = tx_ms + DISSTREAM_TX_WRAP_MS - stream_info->first_tx_ms;
    } else {
        nominal_ms = tx_ms - stream_info->first_tx_ms;
    }

    stream_info->timing_packet_count++;

    arrival_delta_ms = current_time_ms - stream_info->prev_arrival_ms;

    stream_info->max_delta_ms = MAX(stream_info->max_delta_ms, arrival_delta_ms);
    stream_info->mean_delta_ms += (arrival_delta_ms
        - stream_info->mean_delta_ms) / (stream_info->timing_packet_count - 1);

    sample_time_ms = 0.0;
    if (dis_info->info_sample_rate > 0 && dis_info->info_num_samples > 0) {
        sample_time_ms = ((double)dis_info->info_num_samples / (double)dis_info->info_sample_rate) * 1000.0;
    } else if (dis_info->info_payload_len > 0) {
        sample_time_ms = ((double)dis_info->info_payload_len / DISSTREAM_CODEC_CLOCK_HZ) * 1000.0;
    }

    nominal_delta_ms = nominal_ms - stream_info->prev_nominal_ms;
    use_dis_timestamp_spacing = nominal_delta_ms > 0.0;

    /*
     * DIS transit timestamps are unreliable depending on radio vendor.
     * Some radios do not advance the transmit timestamp in a way that
     * reflects packetization spacing. In that case, fall back to media timing
     * derived from sample count and sample rate, which matches the older DIS
     * voice tools logic more closely.
     */
    if (use_dis_timestamp_spacing && sample_time_ms > 0.0) {
        timestamp_spacing_error_ms = fabs(nominal_delta_ms - sample_time_ms);
        if (timestamp_spacing_error_ms > MAX(sample_time_ms * 0.5, 5.0)) {
            use_dis_timestamp_spacing = false;
        }
    }

    reference_delta_ms = use_dis_timestamp_spacing ? nominal_delta_ms : sample_time_ms;
    jitter_diff_ms = fabs(arrival_delta_ms - reference_delta_ms);
    current_jitter_ms = ((15.0 * stream_info->filtered_jitter_ms) + jitter_diff_ms) / 16.0;
    stream_info->filtered_jitter_ms = current_jitter_ms;

    stream_info->max_jitter_ms = MAX(stream_info->max_jitter_ms, current_jitter_ms);
    stream_info->mean_jitter_ms += (current_jitter_ms - stream_info->mean_jitter_ms)
        / (stream_info->timing_packet_count - 1);

    if (out_delta_ms) {
        *out_delta_ms = arrival_delta_ms;
    }
    if (out_jitter_ms) {
        *out_jitter_ms = current_jitter_ms;
    }

    sample_tx_ms = reference_delta_ms;
    if (sample_tx_ms <= 0.0) {
        sample_tx_ms = tx_ms - stream_info->prev_tx_ms;
        if (sample_tx_ms < 0.0) {
            sample_tx_ms += DISSTREAM_TX_WRAP_MS;
        }
    }

    if (sample_time_ms > 0.0) {
        stream_info->excess_codec_time_ms += sample_tx_ms - sample_time_ms;
        if (stream_info->excess_codec_time_ms > sample_time_ms) {
            lost_packets = (uint32_t)(stream_info->excess_codec_time_ms / sample_time_ms);
            stream_info->estimated_lost_packets += lost_packets;
            stream_info->excess_codec_time_ms -= lost_packets * sample_time_ms;
            if (out_lost_packets_added) {
                *out_lost_packets_added = lost_packets;
            }
            if (lost_packets > 0) {
                stream_info->problem = true;
            }
        }
    }

    stream_info->prev_arrival_ms = current_time_ms;
    stream_info->prev_tx_ms = tx_ms;
    stream_info->prev_nominal_ms = nominal_ms;
}

static disstream_info_t *
disstream_info_create(packet_info *pinfo, const dis_tap_info_t *dis_info,
    const disstream_id_t *stream_id)
{
    disstream_info_t *stream_info;

    stream_info = disstream_info_malloc_and_init();
    disstream_id_copy(stream_id, &stream_info->id);

    stream_info->payload_type = dis_info->info_payload_type;
    stream_info->payload_type_str = dis_info->info_payload_type_str;
    stream_info->radio_input_source = dis_info->info_radio_input_source;
    stream_info->transmit_state = dis_info->info_transmit_state;

    stream_info->first_packet_num = pinfo->fd->num;
    stream_info->last_packet_num = pinfo->fd->num;
    stream_info->first_signal_frame_num = pinfo->fd->num;
    stream_info->last_signal_frame_num = pinfo->fd->num;
    stream_info->start_rel_time = pinfo->rel_ts;
    stream_info->stop_rel_time = pinfo->rel_ts;

    return stream_info;
}

tap_packet_status
disstream_packet_cb(void *arg, packet_info *pinfo, epan_dissect_t *edt _U_,
    const void *arg2, tap_flags_t flags _U_)
{
    disstream_tapinfo_t *tapinfo = (disstream_tapinfo_t *)arg;
    const dis_tap_info_t *dis_info = (const dis_tap_info_t *)arg2;
    disstream_id_t stream_id;
    disstream_info_t *stream_info;
    bool redraw = false;
    double packet_delta_ms = 0.0;
    double packet_jitter_ms = 0.0;
    uint32_t packet_lost_added = 0;

    if (!tapinfo || !dis_info || !dis_info->info_valid_radio_pdu_tap) {
        return TAP_PACKET_DONT_REDRAW;
    }

    if (tapinfo->mode != DISSTREAM_TAP_ANALYSE) {
        return TAP_PACKET_DONT_REDRAW;
    }

    disstream_id_copy_pinfo_shallow(pinfo, &stream_id);
    stream_id.radio_id = dis_info->info_radio_id;
    stream_id.entity_id_site = dis_info->info_entity_id_site;
    stream_id.entity_id_appl = dis_info->info_entity_id_appl;
    stream_id.entity_id_entity = dis_info->info_entity_id_entity;

    stream_info = disstream_info_multihash_lookup(tapinfo->strinfo_hash, &stream_id);

    if (dis_info->info_pdu_type == DIS_TAP_PDU_TRANSMITTER) {
        if (stream_info) {
            stream_info->transmitter_packet_count++;
            stream_info->packet_count++;
            stream_info->last_packet_num = pinfo->fd->num;
            stream_info->stop_rel_time = pinfo->rel_ts;
            stream_info->transmit_state = dis_info->info_transmit_state;

            if (dis_info->info_transmit_state == DISSTREAM_STOPPED_STATE) {
                stream_info->transmission_stopped = true;
            }

            redraw = true;
        }

        tapinfo->npackets++;
        return redraw ? TAP_PACKET_REDRAW : TAP_PACKET_DONT_REDRAW;
    }

    if (dis_info->info_pdu_type != DIS_TAP_PDU_SIGNAL) {
        tapinfo->npackets++;
        return TAP_PACKET_DONT_REDRAW;
    }

    if (!stream_info || stream_info->transmission_stopped) {
        stream_info = disstream_info_create(pinfo, dis_info, &stream_id);

        tapinfo->strinfo_list = g_list_prepend(tapinfo->strinfo_list, stream_info);
        if (!tapinfo->strinfo_hash) {
            tapinfo->strinfo_hash = g_hash_table_new(g_direct_hash, g_direct_equal);
        }
        disstream_info_multihash_insert(tapinfo->strinfo_hash, stream_info);
        tapinfo->nstreams++;
        redraw = true;
    }

    stream_info->packet_count++;
    stream_info->signal_packet_count++;
    stream_info->total_payload_bytes += dis_info->info_payload_len;
    stream_info->last_packet_num = pinfo->fd->num;
    stream_info->last_signal_frame_num = pinfo->fd->num;
    stream_info->stop_rel_time = pinfo->rel_ts;
    stream_info->radio_input_source = dis_info->info_radio_input_source;
    stream_info->transmit_state = dis_info->info_transmit_state;

    if (stream_info->payload_type == DIS_PAYLOAD_TYPE_INVALID
        && dis_info->info_payload_type != DIS_PAYLOAD_TYPE_INVALID) {
        stream_info->payload_type = dis_info->info_payload_type;
        stream_info->payload_type_str = dis_info->info_payload_type_str;
    }

    if (!dis_info->info_all_data_present) {
        stream_info->problem = true;
    }

    disstream_info_update_timing(stream_info, pinfo, dis_info,
        &packet_delta_ms, &packet_jitter_ms, &packet_lost_added);

    if (dis_info->info_all_data_present && dis_info->info_payload_len > 0 && dis_info->info_data) {
        disstream_packet_t *packet = g_new0(disstream_packet_t, 1);

        packet->frame_num = pinfo->fd->num;
        packet->rel_time = pinfo->rel_ts;
        packet->payload_type = dis_info->info_payload_type;
        packet->payload_len = dis_info->info_payload_len;
        packet->payload_data = g_memdup2(
            dis_info->info_data + dis_info->info_payload_offset,
            dis_info->info_payload_len
        );
        packet->delta_ms = packet_delta_ms;
        packet->jitter_ms = packet_jitter_ms;
        packet->estimated_lost_added = packet_lost_added;
        packet->problem = (!dis_info->info_all_data_present) || packet_lost_added > 0;

        g_ptr_array_add(stream_info->signal_packets, packet);
    }

    tapinfo->npackets++;
    return redraw ? TAP_PACKET_REDRAW : TAP_PACKET_DONT_REDRAW;
}

/*
 * Editor modelines  -  https://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 expandtab:
 * :indentSize=8:tabSize=8:
 */
