/*
 * GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2022 Canonical Ltd.
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
 *
 * Author: Marco Trevisan <marco.trevisan@canonical.com>
 */

#include "portal-support-utils.h"

#include <glib.h>
#include <glib/gstdio.h>


void
cleanup_snapfiles (const gchar *path)
{
  GDir *dir = NULL;
  const gchar *entry;

  dir = g_dir_open (path, 0, NULL);
  if (dir == NULL)
    {
      /* Assume it’s a file. Ignore failure. */
      (void) g_remove (path);
      return;
    }

  while ((entry = g_dir_read_name (dir)) != NULL)
    {
      gchar *sub_path = g_build_filename (path, entry, NULL);
      cleanup_snapfiles (sub_path);
      g_free (sub_path);
    }

  g_dir_close (dir);

  g_rmdir (path);
}

void
create_fake_snapctl (const char *path,
                     const char *supported_op)
{
  GError *error = NULL;
  char *snapctl_content;
  char *snapctl;

  snapctl = g_build_filename (path, "snapctl", NULL);
  snapctl_content = g_strdup_printf ("#!/bin/sh\n" \
                                     "[ \"$1\" != 'is-connected' ] && exit 2\n"
                                     "[ -z \"$2\" ] && exit 3\n"
                                     "[ -n \"$3\" ] && exit 4\n"
                                     "case \"$2\" in\n"
                                     "  %s) exit 0;;\n"
                                     "  *) exit 1;;\n"
                                     "esac\n",
                                     supported_op ? supported_op : "<invalid>");

  g_file_set_contents (snapctl, snapctl_content, -1, &error);
  g_assert_no_error (error);
  g_assert_cmpint (g_chmod (snapctl, 0500), ==, 0);

  g_test_message ("Created snapctl in %s", snapctl);

  g_clear_error (&error);
  g_free (snapctl_content);
  g_free (snapctl);
}

void
create_fake_snap_yaml (const char *snap_path,
                       gboolean is_classic)
{
  char *meta_path;
  char *yaml_path;
  char *yaml_contents;

  g_assert_nonnull (snap_path);

  yaml_contents = g_strconcat ("name: glib-test-portal-support\n"
                               "title: GLib Portal Support Test\n"
                               "version: 2.76\n"
                               "summary: Test it works\n",
                               is_classic ?
                                "confinement: classic\n" : NULL, NULL);

  meta_path = g_build_filename (snap_path, "meta", NULL);
  g_assert_cmpint (g_mkdir_with_parents (meta_path, 0700), ==, 0);

  yaml_path = g_build_filename (meta_path, "snap.yaml", NULL);
  g_file_set_contents (yaml_path, yaml_contents, -1, NULL);

  g_test_message ("Created snap.yaml in %s", yaml_path);

  g_free (meta_path);
  g_free (yaml_path);
  g_free (yaml_contents);
}

void
create_fake_flatpak_info_from_key_file (const char *root_path,
                                        GKeyFile   *key_file)
{
  GError *error = NULL;
  char *key_file_path;

  g_assert_nonnull (root_path);

  key_file_path = g_build_filename (root_path, ".flatpak-info", NULL);
  g_test_message ("Creating .flatpak-info in %s", key_file_path);
  g_key_file_save_to_file (key_file, key_file_path, &error);
  g_assert_no_error (error);

  g_free (key_file_path);
}

void
create_fake_flatpak_info (const char  *root_path,
                          const GStrv shared_context,
                          const char  *dconf_dbus_policy)
{
  GKeyFile *key_file;

  key_file = g_key_file_new ();

  /* File format is defined at:
   *  https://docs.flatpak.org/en/latest/flatpak-command-reference.html
   */
  g_key_file_set_string (key_file, "Application", "name",
                         "org.gnome.GLib.Test.Flatpak");
  g_key_file_set_string (key_file, "Application", "runtime",
                         "org.gnome.Platform/x86_64/44");
  g_key_file_set_string (key_file, "Application", "sdk",
                         "org.gnome.Sdk/x86_64/44");

  if (shared_context)
    {
      g_key_file_set_string_list (key_file, "Context", "shared",
                                  (const char * const *) shared_context,
                                  g_strv_length (shared_context));
    }

  if (dconf_dbus_policy)
    {
      g_key_file_set_string (key_file, "Session Bus Policy", "ca.desrt.dconf",
                             dconf_dbus_policy);
    }

  create_fake_flatpak_info_from_key_file (root_path, key_file);

  g_key_file_free (key_file);
}
