/* packet-rquota.h
 *
 * $Id: packet-rquota.h 34270 2010-09-28 05:50:54Z sahlberg $
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

#ifndef PACKET_RQUOTA_H
#define PACKET_RQUOTA_H

#define RQUOTAPROC_NULL 		0
#define RQUOTAPROC_GETQUOTA		1
#define RQUOTAPROC_GETACTIVEQUOTA	2
#define RQUOTAPROC_SETQUOTA		3
#define RQUOTAPROC_SETACTIVEQUOTA	4

#define RQUOTA_PROGRAM 100011

#endif
