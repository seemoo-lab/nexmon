/* aftypes.h
 * AF_ values on various OSes; they're used in some network protocols, as
 * well as in BSD DLT_NULL and DLT_LOOP headers.
 *
 * $Id: aftypes.h 31163 2009-12-03 17:31:03Z stig $
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 *
 * This file created and by Mike Hall <mlh@io.com>
 * Copyright 1998
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

#ifndef __AFTYPES_H__
#define __AFTYPES_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* BSD AF_ values. */
#define BSD_AF_INET		2
#define BSD_AF_ISO		7
#define BSD_AF_APPLETALK	16
#define BSD_AF_IPX		23
#define BSD_AF_INET6_BSD	24	/* OpenBSD (and probably NetBSD), BSD/OS */
#define BSD_AF_INET6_FREEBSD	28
#define BSD_AF_INET6_DARWIN	30

/* Linux AF_ values. */
#define LINUX_AF_INET		2
#define LINUX_AF_INET6		10

/* Solaris AF_ values. */
#define SOLARIS_AF_INET		2
#define SOLARIS_AF_INET6       	26

/* Winsock AF_ values. */
#define WINSOCK_AF_INET		2
#define WINSOCK_AF_IPX		6
#define WINSOCK_AF_INET6	23

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* aftypes.h */
