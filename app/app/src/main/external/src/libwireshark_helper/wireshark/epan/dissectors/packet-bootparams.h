/* packet-bootparams.h */
/* $Id: packet-bootparams.h 18787 2006-07-22 22:15:15Z sahlberg $
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

#ifndef PACKET_BOOTPARAMS_H
#define PACKET_BOOTPARAMS_H

#define BOOTPARAMSPROC_NULL 0
#define BOOTPARAMSPROC_WHOAMI 1
#define BOOTPARAMSPROC_GETFILE 2

#define BOOTPARAMS_PROGRAM 100026

#endif
