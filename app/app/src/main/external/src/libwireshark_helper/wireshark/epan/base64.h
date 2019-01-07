/* base64.h
 * Base-64 conversion
 *
 * $Id: base64.h 29502 2009-08-21 20:51:13Z krj $
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
#ifndef __BASE64_H__
#define __BASE64_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <epan/tvbuff.h>
/* In-place decoding of a base64 string. */
size_t epan_base64_decode(char *s);

extern tvbuff_t* base64_to_tvb(tvbuff_t *parent, const char *base64);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BASE64_H__ */
