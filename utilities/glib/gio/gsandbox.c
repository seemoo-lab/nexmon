/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2022 Canonical Ltd
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
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gsandbox.h"

#include <string.h>

#define SNAP_CONFINEMENT_PREFIX "confinement:"

static gboolean
is_flatpak (void)
{
  const char *flatpak_info = "/.flatpak-info";
  gboolean found;

#ifdef G_PORTAL_SUPPORT_TEST
        char *test_key_file =
          g_build_filename (g_get_user_runtime_dir (), flatpak_info, NULL);
        flatpak_info = test_key_file;
#endif

  found = g_file_test (flatpak_info, G_FILE_TEST_EXISTS);

#ifdef G_PORTAL_SUPPORT_TEST
  g_clear_pointer (&test_key_file, g_free);
#endif

  return found;
}

static gchar *
get_snap_confinement (const char  *snap_yaml,
                      GError     **error)
{
  char *confinement = NULL;
  char *yaml_contents;

  if (g_file_get_contents (snap_yaml, &yaml_contents, NULL, error))
    {
      const char *line = yaml_contents;

      do
        {
          if (g_str_has_prefix (line, SNAP_CONFINEMENT_PREFIX))
            break;

          line = strchr (line, '\n');
          if (line)
            line += 1;
        }
      while (line != NULL);

      if (line)
        {
          const char *start = line + strlen (SNAP_CONFINEMENT_PREFIX);
          const char *end = strchr (start, '\n');

          confinement =
            g_strstrip (end ? g_strndup (start, end-start) : g_strdup (start));
        }

      g_free (yaml_contents);
    }

  return g_steal_pointer (&confinement);
}

static gboolean
is_snap (void)
{
  GError *error = NULL;
  const gchar *snap_path;
  gchar *yaml_path;
  char *confinement;
  gboolean result;

  snap_path = g_getenv ("SNAP");
  if (snap_path == NULL)
    return FALSE;

  result = FALSE;
  yaml_path = g_build_filename (snap_path, "meta", "snap.yaml", NULL);
  confinement = get_snap_confinement (yaml_path, &error);
  g_free (yaml_path);

  /* Classic snaps are de-facto no sandboxed apps, so we can ignore them */
  if (!error && g_strcmp0 (confinement, "classic") != 0)
    result = TRUE;

  g_clear_error (&error);
  g_free (confinement);

  return result;
}

/*
 * glib_get_sandbox_type:
 *
 * Gets the type of sandbox this process is running inside.
 *
 * Checking for sandboxes may involve doing blocking I/O calls, but should not take
 * any significant time.
 *
 * The sandbox will not change over the lifetime of the process, so calling this
 * function once and reusing the result is valid.
 *
 * If this process is not sandboxed then @G_SANDBOX_TYPE_UNKNOWN will be returned.
 * This is because this function only detects known sandbox types in #GSandboxType.
 * It may be updated in the future if new sandboxes come into use.
 *
 * Returns: a #GSandboxType.
 */
GSandboxType
glib_get_sandbox_type (void)
{
  if (is_flatpak ())
    return G_SANDBOX_TYPE_FLATPAK;
  else if (is_snap ())
    return G_SANDBOX_TYPE_SNAP;
  else
    return G_SANDBOX_TYPE_UNKNOWN;
}
