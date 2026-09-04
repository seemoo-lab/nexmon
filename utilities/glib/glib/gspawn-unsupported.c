/* gspawn.c - Process launching implementation that simply return an error
 *
 * Copyright 2026 Nirbheek Chauhan
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"


#include "gspawn.h"
#include "gspawn-private.h"
#include "gtestutils.h"

G_DEFINE_QUARK (g-exec-error-quark, g_spawn_error)
G_DEFINE_QUARK (g-spawn-exit-error-quark, g_spawn_exit_error)

gboolean
g_spawn_sync_impl (const gchar           *working_directory,
                   gchar                **argv,
                   gchar                **envp,
                   GSpawnFlags            flags,
                   GSpawnChildSetupFunc   child_setup,
                   gpointer               user_data,
                   gchar                **standard_output,
                   gchar                **standard_error,
                   gint                  *wait_status,
                   GError               **error)
{
  g_set_error_literal (error, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED,
      "g_spawn_sync() is unavailable on this platform");
  return FALSE;
}

gboolean
g_spawn_async_with_pipes_and_fds_impl (const gchar           *working_directory,
                                       const gchar * const   *argv,
                                       const gchar * const   *envp,
                                       GSpawnFlags            flags,
                                       GSpawnChildSetupFunc   child_setup,
                                       gpointer               user_data,
                                       gint                   stdin_fd,
                                       gint                   stdout_fd,
                                       gint                   stderr_fd,
                                       const gint            *source_fds,
                                       const gint            *target_fds,
                                       gsize                  n_fds,
                                       GPid                  *child_pid_out,
                                       gint                  *stdin_pipe_out,
                                       gint                  *stdout_pipe_out,
                                       gint                  *stderr_pipe_out,
                                       GError               **error)
{
  g_set_error_literal (error, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED,
      "g_spawn_async_with_pipes_and_fds() is unavailable on this platform");
  return FALSE;
}

gboolean
g_spawn_check_wait_status_impl (gint     wait_status,
                                GError **error)
{
  g_set_error_literal (error, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED,
      "g_spawn_check_wait_status() is unavailable on this platform");
  return FALSE;
}

void
g_spawn_close_pid_impl (GPid pid)
{
  /* no-op */
}
