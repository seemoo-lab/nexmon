/* packet-bthci_acl.h
 *
 * $Id: packet-bthci_acl.h 18219 2006-05-26 08:32:17Z sahlberg $
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

#ifndef __PACKET_BTHCI_ACL_H__
#define __PACKET_BTHCI_ACL_H__

typedef struct _bthci_acl_data_t {
	guint16 chandle;  /* only low 12 bits used */
} bthci_acl_data_t;

#endif
