/* packet-t30.h
 *
 * Routines for T38 dissection
 * 2003 Hans Viens
 * 2004 Alejandro Vaquero, add support to conversation
 *
 * $Id: packet-t30.h 21676 2007-05-04 07:10:15Z kukosa $
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

WS_VAR_IMPORT const value_string t30_facsimile_control_field_vals[];
WS_VAR_IMPORT const value_string t30_facsimile_control_field_vals_short[];
