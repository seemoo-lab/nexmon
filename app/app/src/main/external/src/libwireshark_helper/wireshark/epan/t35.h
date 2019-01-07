/* t35.h
 * T.35 and H.221 tables
 * 2003  Tomas Kukosa
 *
 * $Id: t35.h 29502 2009-08-21 20:51:13Z krj $
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

#ifndef __T35_H__
#define __T35_H__ 

#include <epan/value_string.h>

extern const value_string T35CountryCode_vals[];
extern const value_string T35Extension_vals[];
extern const value_string H221ManufacturerCode_vals[];

#endif  /* __T35_H__ */
