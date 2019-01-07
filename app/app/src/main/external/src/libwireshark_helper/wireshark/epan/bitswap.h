/* bitswap.h
 * Macro to bitswap a byte by looking it up in a table
 *
 * $Id: bitswap.h 20485 2007-01-18 18:43:30Z guy $
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

#ifndef __BITSWAP_H__
#define __BITSWAP_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern guint8 swaptab[256];

#define BIT_SWAP(b)	(swaptab[b])

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* bitswap.h */
