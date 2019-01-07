/* packet-ssl.h
 *
 * $Id: packet-ssl.h 25721 2008-07-12 14:42:14Z jake $
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
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

#ifndef __PACKET_SSL_H__
#define __PACKET_SSL_H__

extern void ssl_dissector_add(guint port, const gchar *protocol, gboolean tcp);
extern void ssl_dissector_delete(guint port, const gchar *protocol, gboolean tcp);

extern void ssl_set_master_secret(guint32 frame_num, address *addr_srv, address *addr_cli, 
                                  port_type ptype, guint32 port_srv, guint32 port_cli,
                                  guint32 version, gint cipher, const guchar *_master_secret,
                                  const guchar *_client_random, const guchar *_server_random,
                                  guint32 client_seq, guint32 server_seq);

#endif  /* __PACKET_SSL_H__ */
