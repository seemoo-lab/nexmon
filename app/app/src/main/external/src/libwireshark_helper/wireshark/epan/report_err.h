/* report_err.h
 * Declarations of routines for code that can run in GUI and command-line
 * environments to use to report errors to the user (e.g., I/O errors, or
 * problems with preference settings).
 *
 * The application using libwireshark will register error-reporting
 * routines, and the routines declared here will call the registered
 * routines.  That way, these routines can be called by code that
 * doesn't itself know whether to pop up a dialog or print something
 * to the standard error.
 *
 * $Id: report_err.h 27456 2009-02-15 21:47:57Z guy $
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

#ifndef __REPORT_ERR_H__
#define __REPORT_ERR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 *  Initialize the report err routines
 */
extern void init_report_err(
	void (*report_failure)(const char *, va_list),
	void (*report_open_failure)(const char *, int, gboolean),
	void (*report_read_failure)(const char *, int),
	void (*report_write_failure)(const char *, int));

/*
 * Report a general error.
 */
extern void report_failure(const char *msg_format, ...);

/*
 * Report an error when trying to open a file.
 * "err" is assumed to be an error code from Wiretap; positive values are
 * UNIX-style errnos, so this can be used for open failures not from
 * Wiretap as long as the failure code is just an errno.
 */
extern void report_open_failure(const char *filename, int err,
    gboolean for_writing);

/*
 * Report an error when trying to read a file.
 * "err" is assumed to be a UNIX-style errno.
 */
extern void report_read_failure(const char *filename, int err);

/*
 * Report an error when trying to write a file.
 * "err" is assumed to be a UNIX-style errno.
 */
extern void report_write_failure(const char *filename, int err);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __REPORT_ERR_H__ */
