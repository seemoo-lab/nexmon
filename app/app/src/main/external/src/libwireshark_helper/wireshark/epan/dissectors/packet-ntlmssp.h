/* packet-ntlmssp.h
 * Declarations for NTLM Secure Service Provider
 * Copyright 2003, Tim Potter <tpot@samba.org>
 *
 * $Id: packet-ntlmssp.h 18196 2006-05-21 04:49:01Z sahlberg $
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __PACKET_NTLMSSP_H__
#define __PACKET_NTLMSSP_H__

/* Dissect a ntlmv2 response */

int
dissect_ntlmv2_response(tvbuff_t *tvb, proto_tree *ntlmssp_tree, int offset,
			int len);

/* the ntlmssp data passed to tap listeners */
typedef struct _ntlmssp_header_t {
	guint32		type;
	const char 	*domain_name;
	const char 	*acct_name;
	const char 	*host_name;
} ntlmssp_header_t;

#endif
