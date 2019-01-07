/* privileges.h
 * Declarations of routines for handling privileges.
 *
 * $Id: privileges.h 35093 2010-12-01 18:25:18Z sfisher $
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 2006 Gerald Combs
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

#ifndef __PRIVILEGES_H__
#define __PRIVILEGES_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Called when the program starts, to enable security features and save
 * whatever credential information we'll need later.
 */
extern void init_process_policies(void);

/**
 * Was this program started with special privileges?  get_credential_info()
 * MUST be called before calling this.
 * @return TRUE if the program was started with special privileges,
 * FALSE otherwise.
 */
extern gboolean started_with_special_privs(void);

/**
 * Is this program running with special privileges? get_credential_info()
 * MUST be called before calling this.
 * @return TRUE if the program is running with special privileges,
 * FALSE otherwise.
 */
extern gboolean running_with_special_privs(void);

/**
 * Permanently relinquish special privileges. get_credential_info()
 * MUST be called before calling this.
 */
extern void relinquish_special_privs_perm(void);

/**
 * Get the current username.  String must be g_free()d after use.
 * @return A freshly g_alloc()ed string containing the username,
 * or "UNKNOWN" on failure.
 */
extern gchar *get_cur_username(void);

/**
 * Get the current group.  String must be g_free()d after use.
 * @return A freshly g_alloc()ed string containing the group,
 * or "UNKNOWN" on failure.
 */
extern gchar *get_cur_groupname(void);

#ifdef _WIN32
/**
 * Check to see if npf.sys is running.
 * @return TRUE if npf.sys is running, FALSE if it's not or if there was
 * an error checking its status.
 */
extern gboolean npf_sys_is_running();
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PRIVILEGES_H__ */
