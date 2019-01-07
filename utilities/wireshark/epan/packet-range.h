/* packet-range.h
 * Packet range routines (save, print, ...)
 *
 * Dick Gooris <gooris@lucent.com>
 * Ulf Lamping <ulf.lamping@web.de>
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

#ifndef __PACKET_RANGE_H__
#define __PACKET_RANGE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <glib.h>

#include <epan/range.h>

#include "cfile.h"


extern guint32  curr_selected_frame;

typedef enum {
    range_process_all,
    range_process_selected,
    range_process_marked,
    range_process_marked_range,
    range_process_user_range
} packet_range_e;

typedef struct packet_range_tag {
    /* values coming from the UI */
    packet_range_e  process;            /* which range to process */
    gboolean        process_filtered;   /* captured or filtered packets */
    gboolean        remove_ignored;     /* remove ignored packets */
    gboolean        include_dependents;	/* True if packets which are dependents of others should be processed */

    /* user specified range(s) and, if null, error status */
    range_t         *user_range;
    convert_ret_t   user_range_status;

    /* calculated values */
    guint32  selected_packet;       /* the currently selected packet */

    /* current packet counts (captured) */
    capture_file *cf;                     /* Associated capture file. */
    guint32       mark_range_cnt;         /* packets in marked range */
    guint32       user_range_cnt;         /* packets in user specified range */
    guint32       ignored_cnt;            /* packets ignored */
    guint32       ignored_marked_cnt;     /* packets ignored and marked */
    guint32       ignored_mark_range_cnt; /* packets ignored in marked range */
    guint32       ignored_user_range_cnt; /* packets ignored in user specified range */

    /* current packet counts (displayed) */
    guint32  displayed_cnt;
    guint32  displayed_plus_dependents_cnt;
    guint32  displayed_marked_cnt;
    guint32  displayed_mark_range_cnt;
    guint32  displayed_user_range_cnt;
    guint32  displayed_ignored_cnt;
    guint32  displayed_ignored_marked_cnt;
    guint32  displayed_ignored_mark_range_cnt;
    guint32  displayed_ignored_user_range_cnt;

    /* "enumeration" values */
    gboolean marked_range_active;   /* marked range is currently processed */
    guint32  marked_range_left;     /* marked range packets left to do */
    gboolean selected_done;         /* selected packet already processed */
} packet_range_t;

typedef enum {
    range_process_this,             /* process this packet */
    range_process_next,             /* skip this packet, process next */
    range_processing_finished       /* stop processing, required packets done */
} range_process_e;

/* init the range structure */
WS_DLL_PUBLIC void packet_range_init(packet_range_t *range, capture_file *cf);

/* check whether the packet range is OK */
WS_DLL_PUBLIC convert_ret_t packet_range_check(packet_range_t *range);

/* init the processing run */
WS_DLL_PUBLIC void packet_range_process_init(packet_range_t *range);

/* do we have to process all packets? */
WS_DLL_PUBLIC gboolean packet_range_process_all(packet_range_t *range);

/* do we have to process this packet? */
WS_DLL_PUBLIC range_process_e packet_range_process_packet(packet_range_t *range, frame_data *fdata);

/* convert user given string to the internal user specified range representation */
WS_DLL_PUBLIC void packet_range_convert_str(packet_range_t *range, const gchar *es);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PACKET_RANGE_H__ */
