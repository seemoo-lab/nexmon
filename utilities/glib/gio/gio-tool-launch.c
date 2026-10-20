/*
 * Copyright 2020 Frederic Martinsons
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
 * Author: Frederic Martinsons <frederic.martinsons@sigfox.com>
 */

#include "config.h"

#include <gio/gio.h>

#if defined(G_OS_UNIX) && !defined(__APPLE__)
#include <gio/gdesktopappinfo.h>
#endif

#include <gi18n.h>

#include "gio-tool.h"

static const GOptionEntry entries[] = {
  G_OPTION_ENTRY_NULL
};

int
handle_launch (int argc, char *argv[], gboolean do_help)
{
  GOptionContext *context;
  GError *error = NULL;
#if defined(G_OS_UNIX) && !defined(__APPLE__)
  int i;
  GAppInfo *app = NULL;
  GAppLaunchContext *app_context = NULL;
  GKeyFile *keyfile = NULL;
  GList *args = NULL;
  char *desktop_file = NULL;
#endif
  int retval;

  g_set_prgname ("gio launch");

  /* Translators: commandline placeholder */
  context = g_option_context_new (_("DESKTOP-FILE [FILE-ARG …]"));
  g_option_context_set_help_enabled (context, FALSE);
  g_option_context_set_summary (context,
        _("Launch an application from a desktop file, passing optional filename arguments to it."));
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
      show_help (context, _("No desktop file given"));
      g_option_context_free (context);
      return 1;
    }

  g_option_context_free (context);

#if !defined(G_OS_UNIX) || defined(__APPLE__)
  print_error (_("The launch command is not currently supported on this platform"));
  retval = 1;
#else
  retval = 0;
  desktop_file = g_canonicalize_filename (argv[1], NULL);

  /* Use g_key_file_load_from_file() to give better user feedback (missing vs.
   * malformed file), then load it with g_desktop_app_info_new_from_filename()
   * to set the constructor-only filename property required for expanding %k.
   */
  keyfile = g_key_file_new ();
  if (!g_key_file_load_from_file (keyfile, desktop_file, G_KEY_FILE_NONE, &error))
    {
      print_error (_("Unable to load ‘%s’: %s"), desktop_file, error->message);
      g_clear_error (&error);
      retval = 1;
    }
  else
    {
      app = (GAppInfo *)g_desktop_app_info_new_from_filename (desktop_file);
      if (!app)
        {
          print_error (_("Unable to load application information for ‘%s’"), desktop_file);
          retval = 1;
        }
      else
        {
          for (i = 2; i < argc; i++)
            {
              args = g_list_append (args, g_file_new_for_commandline_arg (argv[i]));
            }
          app_context = g_app_launch_context_new ();
          if (!g_app_info_launch (app, args, app_context, &error))
            {
              print_error (_("Unable to launch application ‘%s’: %s"), desktop_file, error->message);
              g_clear_error (&error);
              retval = 1;
            }
          g_list_free_full (args, g_object_unref);
          g_clear_object (&app_context);
        }
      g_clear_object (&app);
    }
  g_key_file_free (keyfile);
  g_free (desktop_file);
#endif
  return retval;
}
