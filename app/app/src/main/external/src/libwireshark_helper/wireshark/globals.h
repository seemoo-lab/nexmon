/* globals.h
 * Global defines, etc.
 *
 * $Id: globals.h 35521 2011-01-13 17:39:54Z sfisher $
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

#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "file.h"
#include <epan/timestamp.h>

extern capture_file cfile;
#ifdef HAVE_LIBPCAP
/** @todo move this to the gtk dir */
extern gboolean     auto_scroll_live;
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GLOBALS_H__ */
