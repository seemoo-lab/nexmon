/* packet-osi-options.h
 * Defines for OSI options part decode
 *
 * $Id: packet-osi-options.h 18196 2006-05-21 04:49:01Z sahlberg $
 * Ralf Schneider <Ralf.Schneider@t-online.de>
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

#ifndef _PACKET_OSI_OPTION_H
#define _PACKET_OSI_OPTIONS_H

/*
 * published API functions
 */
extern void dissect_osi_options( guchar, tvbuff_t *, int,
                                 proto_tree *);
extern void proto_register_osi_options(void);

#endif /* _PACKET_OSI_OPTIONS_H */
