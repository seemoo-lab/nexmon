/* packet-x509if.h
 * Routines for X.509 Information Framework packet dissection
 *  Ronnie Sahlberg 2004
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

#ifndef PACKET_X509IF_H
#define PACKET_X509IF_H

#include "packet-x509if-exp.h"

extern const char * x509if_get_last_dn(void);

extern gboolean x509if_register_fmt(int hf_index, const gchar *fmt);
extern const char * x509if_get_last_ava(void);

#endif  /* PACKET_X509IF_H */

