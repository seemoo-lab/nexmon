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

#if defined(G_OS_UNIX) && !defined(__APPLE__)
#include <gio/gdesktopappinfo.h>
#endif

#include <gi18n.h>

#include "gio-tool.h"

static int n_outstanding = 0;
static gboolean success = TRUE;

static const GOptionEntry entries[] = {
  G_OPTION_ENTRY_NULL
};

static void
launch_default_for_uri_cb (GObject *source_object,
                           GAsyncResult *res,
                           gpointer user_data)
{
  GError *error = NULL;
  gchar *uri = user_data;

  if (!g_app_info_launch_default_for_uri_finish (res, &error))
    {
       print_error ("%s: %s", uri, error->message);
       g_clear_error (&error);
       success = FALSE;
    }

  n_outstanding--;

  g_free (uri);
}

int
handle_open (int argc, char *argv[], gboolean do_help)
{
  GOptionContext *context;
  gchar *param;
  GError *error = NULL;
  int i;

  g_set_prgname ("gio open");

  /* Translators: commandline placeholder */
  param = g_strdup_printf ("%s…", _("LOCATION"));
  context = g_option_context_new (param);
  g_free (param);
  g_option_context_set_help_enabled (context, FALSE);
  g_option_context_set_summary (context,
      _("Open files with the default application that\n"
        "is registered to handle files of this type."));
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

  if (argc < 2)
    {
      show_help (context, _("No locations given"));
      g_option_context_free (context);
      return 1;
    }

  g_option_context_free (context);

  for (i = 1; i < argc; i++)
    {
      char *uri = NULL;
      char *uri_scheme;

      /* Workaround to handle non-URI locations. We still use the original
       * location for other cases, because GFile might modify the URI in ways
       * we don't want. See:
       * https://bugzilla.gnome.org/show_bug.cgi?id=779182 */
      uri_scheme = g_uri_parse_scheme (argv[i]);
      if (!uri_scheme || uri_scheme[0] == '\0')
        {
          GFile *file;

          file = g_file_new_for_commandline_arg (argv[i]);
          uri = g_file_get_uri (file);
          g_object_unref (file);
        }
      else
        uri = g_strdup (argv[i]);
      g_free (uri_scheme);

      g_app_info_launch_default_for_uri_async (uri,
                                               NULL,
                                               NULL,
                                               launch_default_for_uri_cb,
                                               g_strdup (uri));

      n_outstanding++;

      g_free (uri);
    }

  while (n_outstanding > 0)
    g_main_context_iteration (NULL, TRUE);

  return success ? 0 : 2;
}
