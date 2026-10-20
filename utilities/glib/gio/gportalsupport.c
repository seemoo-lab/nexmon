/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2016 Red Hat, Inc.
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

#include "glib-private.h"
#include "gportalsupport.h"
#include "gsandbox.h"

static GSandboxType sandbox_type = G_SANDBOX_TYPE_UNKNOWN;
static gboolean use_portal;
static gboolean network_available;
static gboolean dconf_access;

#ifdef G_PORTAL_SUPPORT_TEST
static const char *snapctl = "snapctl";
#else
static const char *snapctl = "/usr/bin/snapctl";
#endif

static gboolean
snap_plug_is_connected (const gchar *plug_name)
{
  gint wait_status;
  const gchar *argv[] = { snapctl, "is-connected", plug_name, NULL };

  /* Bail out if our process is privileged - we don't want to pass those
   * privileges to snapctl. It could be overridden and this would
   * allow arbitrary code execution.
   */
  if (GLIB_PRIVATE_CALL (g_check_setuid) ())
    return FALSE;

  if (!g_spawn_sync (NULL, (gchar **) argv, NULL,
#ifdef G_PORTAL_SUPPORT_TEST
                     G_SPAWN_SEARCH_PATH |
#endif
                         G_SPAWN_STDOUT_TO_DEV_NULL |
                         G_SPAWN_STDERR_TO_DEV_NULL,
                     NULL, NULL, NULL, NULL, &wait_status,
                     NULL))
    return FALSE;

  return g_spawn_check_wait_status (wait_status, NULL);
}

static void
sandbox_info_read (void)
{
  static gsize sandbox_info_is_read = 0;

  /* Sandbox type and Flatpak info is static, so only read once */
  if (!g_once_init_enter (&sandbox_info_is_read))
    return;

  sandbox_type = glib_get_sandbox_type ();

  switch (sandbox_type)
    {
    case G_SANDBOX_TYPE_FLATPAK:
      {
        GKeyFile *keyfile;
        const char *keyfile_path = "/.flatpak-info";

        use_portal = TRUE;
        network_available = FALSE;
        dconf_access = FALSE;

        keyfile = g_key_file_new ();

#ifdef G_PORTAL_SUPPORT_TEST
        char *test_key_file =
          g_build_filename (g_get_user_runtime_dir (), keyfile_path, NULL);
        keyfile_path = test_key_file;
#endif

        if (g_key_file_load_from_file (keyfile, keyfile_path, G_KEY_FILE_NONE, NULL))
          {
            char **shared = NULL;
            char *dconf_policy = NULL;

            shared = g_key_file_get_string_list (keyfile, "Context", "shared", NULL, NULL);
            if (shared)
              {
                network_available = g_strv_contains ((const char *const *) shared, "network");
                g_strfreev (shared);
              }

            dconf_policy = g_key_file_get_string (keyfile, "Session Bus Policy", "ca.desrt.dconf", NULL);
            if (dconf_policy)
              {
                if (strcmp (dconf_policy, "talk") == 0)
                  dconf_access = TRUE;
                g_free (dconf_policy);
              }
          }

#ifdef G_PORTAL_SUPPORT_TEST
        g_clear_pointer (&test_key_file, g_free);
#endif

        g_key_file_unref (keyfile);
      }
      break;
    case G_SANDBOX_TYPE_SNAP:
      break;
    case G_SANDBOX_TYPE_UNKNOWN:
      {
        const char *var;

        var = g_getenv ("GIO_USE_PORTALS");
        if (var && var[0] == '1')
          use_portal = TRUE;
        network_available = TRUE;
        dconf_access = TRUE;
      }
      break;
    }

  g_once_init_leave (&sandbox_info_is_read, 1);
}

gboolean
glib_should_use_portal (void)
{
  sandbox_info_read ();

  if (sandbox_type == G_SANDBOX_TYPE_SNAP)
    return snap_plug_is_connected ("desktop");

  return use_portal;
}

gboolean
glib_network_available_in_sandbox (void)
{
  sandbox_info_read ();

  if (sandbox_type == G_SANDBOX_TYPE_SNAP)
    {
      /* FIXME: This is inefficient doing multiple calls to check connections.
       * See https://github.com/snapcore/snapd/pull/12301 for a proposed
       * improvement to snapd for this.
       */
      return snap_plug_is_connected ("desktop") ||
        snap_plug_is_connected ("network-status");
    }

  return network_available;
}

gboolean
glib_has_dconf_access_in_sandbox (void)
{
  sandbox_info_read ();

  if (sandbox_type == G_SANDBOX_TYPE_SNAP)
    return snap_plug_is_connected ("gsettings");

  return dconf_access;
}
