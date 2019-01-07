/* guid-utils.h
 * Definitions for GUID handling
 *
 * $Id: guid-utils.h 18939 2006-08-17 19:09:41Z ulfl $
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 *
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __GUID_UTILS_H__
#define __GUID_UTILS_H__

#define GUID_LEN	16

/* Note: this might be larger than GUID_LEN, so don't overlay data in packets
   with this. */
typedef struct _e_guid_t {
    guint32 data1;
    guint16 data2;
    guint16 data3;
    guint8  data4[8];
} e_guid_t;


extern void guids_init(void);

/* add a GUID */
extern void guids_add_guid(e_guid_t *guid, const gchar *name);

/* try to get registered name for this GUID */
extern const gchar *guids_get_guid_name(e_guid_t *guid);

/* resolve GUID to name (or if unknown to hex string) */
/* (if you need hex string only, use guid_to_str instead) */
extern const gchar* guids_resolve_guid_to_str(e_guid_t *guid);

/* add a UUID (dcerpc_init_uuid() will call this too) */
#define guids_add_uuid(uuid, name) guids_add_guid((e_guid_t *) (uuid), (name))

/* try to get registered name for this UUID */
#define guids_get_uuid_name(uuid) guids_get_guid_name((e_guid_t *) (uuid))

/* resolve UUID to name (or if unknown to hex string) */
/* (if you need hex string only, use guid_to_str instead) */
#define guids_resolve_uuid_to_str(uuid) guids_resolve_guid_to_str((e_guid_t *) (uuid))

#endif /* __GUID_UTILS_H__ */
