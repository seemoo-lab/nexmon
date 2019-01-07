/* export_object_dicom.c
 * Routines for tracking & saving objects found in DICOM streams
 * See also: export_object.c / export_object.h for common code
 * Copyright 2008, David Aggeler <david_aggeler@hispeed.ch>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

#include "config.h"

#include <glib.h>

#include <epan/dissectors/packet-dcm.h>

#include <epan/packet.h>
#include <epan/tap.h>

#include "export_object.h"

gboolean
eo_dicom_packet(void *tapdata, packet_info *pinfo, epan_dissect_t *edt _U_,
                const void *data)
{
    export_object_list_t *object_list = (export_object_list_t *)tapdata;
    const dicom_eo_t *eo_info = (const dicom_eo_t *)data;
    export_object_entry_t *entry;

    if (eo_info) { /* We have data waiting for us */
        /*
           Don't copy any data. dcm_export_create_object() is already g_malloc() the items
           Still, the values will be freed when the export Object window is closed.
           Therefore, strings and buffers must be copied
        */
        entry = (export_object_entry_t *)g_malloc(sizeof(export_object_entry_t));

        entry->pkt_num = pinfo->num;
        entry->hostname = eo_info->hostname;
        entry->content_type = eo_info->content_type;
        entry->filename = g_strdup(g_path_get_basename(eo_info->filename));
        entry->payload_len  = eo_info->payload_len;
        entry->payload_data = eo_info->payload_data;

        object_list_add_entry(object_list, entry);

        return TRUE; /* State changed - window should be redrawn */
    } else {
        return FALSE; /* State unchanged - no window updates needed */
    }
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
