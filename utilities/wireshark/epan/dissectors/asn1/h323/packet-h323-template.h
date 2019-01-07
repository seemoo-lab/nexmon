/* packet-h323.h
 * Routines for H.235 packet dissection
 * 2007  Tomas Kukosa
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

#ifndef PACKET_H323_H
#define PACKET_H323_H

/* Generic Extensible Framework */

#define GEF_CTX_SIGNATURE 0x47454658  /* "GEFX" */

typedef struct _gef_ctx_t {
  guint32 signature;
  struct _gef_ctx_t *parent;
  /*
    H323-MESSAGES
      FeatureDescriptor/<id>
        <id>
      GenericData/<id>
        <id>
    MULTIMEDIA-SYSTEM-CONTROL
      GenericInformation/<id>[-<subid>]
        <id>
      GenericMessage/<id>[-<subid>]
        <id>
      GenericCapability/<id>
        collapsing/<id>
        nonCollapsing/<id>
        nonCollapsingRaw
      EncryptionSync
        <id>
  */
  const gchar *type;
  const gchar *id;
  const gchar *subid;
  const gchar *key;
} gef_ctx_t;

extern gef_ctx_t* gef_ctx_alloc(gef_ctx_t *parent, const gchar *type);
extern gboolean gef_ctx_check_signature(gef_ctx_t *gefx);
extern gef_ctx_t* gef_ctx_get(void *ptr);
extern void gef_ctx_update_key(gef_ctx_t *gefx);

#endif  /* PACKET_H323_H */

