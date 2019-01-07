/* packet-imf.h
 * Routines for Internet Message Format (IMF) packet disassembly
 *
 * $Id: packet-imf.h 22310 2007-07-14 09:15:02Z gal $
 *
 * Copyright (c) 2007 by Graeme Lunt
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1999 Gerald Combs
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

/* Find the end of the next IMF field in the tvb. 
 * This is not necessarily the first \r\n as there may be continuation lines.
 * 
 * If we have found the last field (terminated by \r\n\r\n) we indicate this in last_field .
 */

int imf_find_field_end(tvbuff_t *tvb, int offset, gint max_length, gboolean *last_field);

