/*
 * Copyright 2015 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Matthias Clasen <mclasen@redhat.com>
 */

#include "config.h"

#include <gio/gio.h>
#include <gi18n.h>
#include <stdio.h>
#if 0
#include <locale.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#endif

#include "gio-tool.h"

static gboolean no_target_directory = FALSE;
static gboolean progress = FALSE;
static gboolean interactive = FALSE;
static gboolean preserve = FALSE;
static gboolean backup = FALSE;
static gboolean no_dereference = FALSE;
static gboolean default_permissions = FALSE;
static gboolean default_modified_time = FALSE;

static const GOptionEntry entries[] = {
  { "no-target-directory", 'T', 0, G_OPTION_ARG_NONE, &no_target_directory, N_("No target directory"), NULL },
  { "progress", 'p', 0, G_OPTION_ARG_NONE, &progress, N_("Show progress"), NULL },
  { "interactive", 'i', 0, G_OPTION_ARG_NONE, &interactive, N_("Prompt before overwrite"), NULL },
  { "preserve", 'p', 0, G_OPTION_ARG_NONE, &preserve, N_("Preserve all attributes"), NULL },
  { "backup", 'b', 0, G_OPTION_ARG_NONE, &backup, N_("Backup existing destination files"), NULL },
  { "no-dereference", 'P', 0, G_OPTION_ARG_NONE, &no_dereference, N_("Never follow symbolic links"), NULL },
  { "default-permissions", 0, 0, G_OPTION_ARG_NONE, &default_permissions, N_("Use default permissions for the destination"), NULL },
  { "default-modified-time", 0, 0, G_OPTION_ARG_NONE, &default_modified_time, N_("Use default file modification timestamps for the destination"), NULL },
  G_OPTION_ENTRY_NULL
};

static gint64 start_time;
static gint64 previous_time;
static goffset previous_num_bytes;

static void
show_progress (goffset current_num_bytes,
               goffset total_num_bytes,
               gpointer user_data)
{
  gint64 tv;
  char *current_size, *total_size, *current_rate, *average_rate;
  goffset bytes_since_last;
  gint64 time_since_last;

  tv = g_get_monotonic_time ();
  if (tv - previous_time < (G_USEC_PER_SEC / 5) &&
      current_num_bytes != total_num_bytes)
    return;

  current_size = g_format_size (current_num_bytes);
  total_size = g_format_size (total_num_bytes);

  average_rate = g_format_size (current_num_bytes * G_USEC_PER_SEC /
                                MAX ((tv - start_time), 1));

  bytes_since_last = current_num_bytes - previous_num_bytes;
  time_since_last = tv - previous_time;
  current_rate = g_format_size ((bytes_since_last * G_USEC_PER_SEC) /
                                MAX (time_since_last, 1));

  g_print ("\r\033[K");

  if (current_num_bytes == total_num_bytes)
    {
      g_print (_("Copied %s (average: %s/s)"),
               current_size, average_rate);
    }
  else
    {
      g_print (_("Copied %s out of %s (%s/s; average: %s/s)"),
               current_size, total_size, current_rate, average_rate);
    }

  previous_time = tv;
  previous_num_bytes = current_num_bytes;

  g_free (current_size);
  g_free (total_size);
  g_free (current_rate);
  g_free (average_rate);
}

int
handle_copy (int argc, char *argv[], gboolean do_help)
{
  GOptionContext *context;
  GError *error = NULL;
  char *param;
  GFile *source, *dest, *target;
  gboolean dest_is_dir;
  char *basename;
  char *uri;
  int i;
  GFileCopyFlags flags;
  int retval = 0;

  g_set_prgname ("gio copy");

  /* Translators: commandline placeholder */
  param = g_strdup_printf ("%s… %s", _("SOURCE"), _("DESTINATION"));
  context = g_option_context_new (param);
  g_free (param);
  g_option_context_set_help_enabled (context, FALSE);
  g_option_context_set_summary (context,
      _("Copy one or more files from SOURCE to DESTINATION."));
  g_option_context_set_description (context,
      _("gio copy is similar to the traditional cp utility, but using GIO\n"
        "locations instead of local files: for example, you can use something\n"
        "like smb://server/resource/file.txt as location."));
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);

  if (do_help)
    {
      show_help (context, NULL);
      g_option_context_free (context);
      return 0;
    }

  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      show_help (context, error->message);
      g_error_free (error);
      g_option_context_free (context);
      return 1;
    }

  if (argc < 3)
    {
      show_help (context, NULL);
      g_option_context_free (context);
      return 1;
    }

  dest = g_file_new_for_commandline_arg (argv[argc - 1]);

  if (no_target_directory && argc > 3)
    {
      show_help (context, NULL);
      g_object_unref (dest);
      g_option_context_free (context);
      return 1;
    }

  dest_is_dir = file_is_dir (dest);
  if (!dest_is_dir && argc > 3)
    {
      char *message;

      message = g_strdup_printf (_("Destination %s is not a directory"), argv[argc - 1]);
      show_help (context, message);
      g_free (message);
      g_object_unref (dest);
      g_option_context_free (context);
      return 1;
    }

  g_option_context_free (context);

  for (i = 1; i < argc - 1; i++)
    {
      source = g_file_new_for_commandline_arg (argv[i]);
      if (dest_is_dir && !no_target_directory)
        {
          basename = g_file_get_basename (source);
          target = g_file_get_child (dest, basename);
          g_free (basename);
        }
      else
        target = g_object_ref (dest);

      flags = 0;
      if (backup)
        flags |= G_FILE_COPY_BACKUP;
      if (!interactive)
        flags |= G_FILE_COPY_OVERWRITE;
      if (no_dereference)
        flags |= G_FILE_COPY_NOFOLLOW_SYMLINKS;
      if (preserve)
        flags |= G_FILE_COPY_ALL_METADATA;
      if (default_permissions)
        flags |= G_FILE_COPY_TARGET_DEFAULT_PERMS;
      if (default_modified_time)
        flags |= G_FILE_COPY_TARGET_DEFAULT_MODIFIED_TIME;

      error = NULL;
      start_time = g_get_monotonic_time ();
      previous_num_bytes = 0;

      if (!g_file_copy (source, target, flags, NULL, progress ? show_progress : NULL, NULL, &error))
        {
          if (interactive && g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS))
            {
              char line[16];

              g_error_free (error);
              error = NULL;

              uri = g_file_get_uri (target);
              g_print (_("%s: overwrite “%s”? "), argv[0], uri);
              g_free (uri);

              if (fgets (line, sizeof (line), stdin) &&
                  (line[0] == 'y' || line[0] == 'Y'))
                {
                  flags |= G_FILE_COPY_OVERWRITE;
                  start_time = g_get_monotonic_time ();
                  previous_num_bytes = 0;
                  if (!g_file_copy (source, target, flags, NULL, progress ? show_progress : NULL, NULL, &error))
                    goto copy_failed;
                }
            }
          else
            {
            copy_failed:
              print_file_error (source, error->message);
              g_error_free (error);
              retval = 1;
            }
        }

     if (progress && retval == 0)
        g_print ("\n");

      g_object_unref (source);
      g_object_unref (target);
    }

  g_object_unref (dest);

  return retval;
}
