/* capture_errs.h
 * Declarations of routines to return error and warning messages for
 * packet capture
 *
 * $Id: capture_errs.h 32803 2010-05-14 02:47:13Z guy $
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

#ifndef __CAPTURE_ERRS_H__
#define __CAPTURE_ERRS_H__

#ifdef _WIN32
/* error message, if WinPcap couldn't be loaded */
/* will use g_strdup, don't forget to g_free the returned string! */
extern char *cant_load_winpcap_err(const char *app_name);
#endif /* _WIN32 */

#endif /* __CAPTURE_ERRS_H__ */
