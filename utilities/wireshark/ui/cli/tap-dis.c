/* tap-dis.c
 * DIS TAP for tshark
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*
 * This TAP provides statistics for DIS radio streams.
 */

#include "config.h"

#include <locale.h>

#include <glib.h>

#include <epan/addr_resolv.h>
#include <epan/packet_info.h>
#include <epan/stat_tap_ui.h>

#include "ui/tap-dis-common.h"

void register_tap_listener_disstreams(void);
static void disstreams_stat_draw_cb(disstream_tapinfo_t *tapinfo);

static disstream_tapinfo_t the_tapinfo_struct = {
    NULL, NULL, 0, 0,
    DISSTREAM_TAP_ANALYSE, NULL, NULL,
    disstreams_stat_draw_cb, NULL, false
};

static void
disstreams_stat_draw_cb(disstream_tapinfo_t *tapinfo _U_)
{
    GList *list;
    char *savelocale;

    printf("========================= DIS Streams ========================\n");
    printf("%11s %11s %15s %5s %15s %5s %7s %16s %8s %8s %8s %12s %12s %12s %s\n",
        "Start", "End", "Src IP addr", "Port", "Dest IP addr", "Port",
        "Radio", "Entity", "Signal", "Tx", "Lost", "MaxDelta", "MeanJit",
        "MaxJit", "Problems?");

    savelocale = g_strdup(setlocale(LC_NUMERIC, NULL));
    setlocale(LC_NUMERIC, "C");

    list = g_list_first(the_tapinfo_struct.strinfo_list);
    while (list) {
        disstream_info_t *stream_info = (disstream_info_t *)list->data;
        char *src_addr_str;
        char *dst_addr_str;

        src_addr_str = address_to_display(NULL, &stream_info->id.src_addr);
        dst_addr_str = address_to_display(NULL, &stream_info->id.dst_addr);

        printf("%11.6f %11.6f %15s %5u %15s %5u 0x%04x %4u/%4u/%4u %8u %8u %8u %12.3f %12.3f %12.3f %s\n",
            nstime_to_sec(&stream_info->start_rel_time),
            nstime_to_sec(&stream_info->stop_rel_time),
            src_addr_str,
            stream_info->id.src_port,
            dst_addr_str,
            stream_info->id.dst_port,
            stream_info->id.radio_id,
            stream_info->id.entity_id_site,
            stream_info->id.entity_id_appl,
            stream_info->id.entity_id_entity,
            stream_info->signal_packet_count,
            stream_info->transmitter_packet_count,
            stream_info->estimated_lost_packets,
            stream_info->max_delta_ms,
            stream_info->mean_jitter_ms,
            stream_info->max_jitter_ms,
            stream_info->problem ? "X" : "");

        wmem_free(NULL, src_addr_str);
        wmem_free(NULL, dst_addr_str);

        list = g_list_next(list);
    }

    printf("============================================================\n\n");
    setlocale(LC_NUMERIC, savelocale);
    g_free(savelocale);
}

static bool
disstreams_stat_init(const char *opt_arg _U_, void *userdata _U_)
{
    register_tap_listener_disstream(&the_tapinfo_struct, NULL, NULL);
    return true;
}

static stat_tap_ui disstreams_stat_ui = {
    REGISTER_STAT_GROUP_GENERIC,
    NULL,
    "dis,streams",
    disstreams_stat_init,
    0,
    NULL
};

void
register_tap_listener_disstreams(void)
{
    register_stat_tap_ui(&disstreams_stat_ui, NULL);
}