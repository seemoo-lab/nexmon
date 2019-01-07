/* console_io.h
 * Declarations of routines to print to the standard error, and, in
 * GUI programs on Windows, to create a console in which to display
 * the standard error.
 *
 * $Id: console_io.h 32711 2010-05-07 08:40:02Z guy $
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

#ifndef __CONSOLE_IO_H__
#define __CONSOLE_IO_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Print to the standard error.  On Windows, create a console for the
 * standard error to show up on, if necessary.
 * XXX - pop this up in a window of some sort on UNIX+X11 if the controlling
 * terminal isn't the standard error?
 */
extern void
vfprintf_stderr(const char *fmt, va_list ap);

extern void
fprintf_stderr(const char *fmt, ...)
    G_GNUC_PRINTF(1, 2);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CMDARG_ERR_H__ */
