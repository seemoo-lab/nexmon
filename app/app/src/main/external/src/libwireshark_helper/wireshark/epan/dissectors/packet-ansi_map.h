/* Do not modify this file.                                                   */
/* It is created automatically by the ASN.1 to Wireshark dissector compiler   */
/* packet-ansi_map.h                                                          */
/* ../../tools/asn2wrs.py -b -p ansi_map -c ./ansi_map.cnf -s ./packet-ansi_map-template -D . ansi_map.asn */

/* Input file: packet-ansi_map-template.h */

#line 1 "../../asn1/ansi_map/packet-ansi_map-template.h"
/* packet-ansi_map.h
 * Routines for ansi_map packet dissection
 * Copyright 2005, Anders Broman <anders.broman@ericsson.com>
 *
 * $Id: packet-ansi_map.h 36569 2011-04-11 23:07:53Z gerald $
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

#ifndef PACKET_ANSI_MAP_H
#define PACKET_ANSI_MAP_H

#define	ANSI_MAP_MAX_NUM_MESSAGE_TYPES	256

typedef struct _ansi_map_tap_rec_t {
    guint8		message_type;
    guint16		size;
} ansi_map_tap_rec_t;


/*
 * the following allows TAP code access to the messages
 * without having to duplicate it. With MSVC and a
 * libwireshark.dll, we need a special declaration.
 */
WS_VAR_IMPORT const value_string ansi_map_opr_code_strings[];



/*#include "packet-ansi_map-exp.h" */

#endif  /* PACKET_ansi_map_H */


