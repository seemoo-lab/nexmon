/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* GObject introspection: Typelib compiler
 *
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later

 * Copyright (C) 2005 Matthias Clasen
 * Copyright (C) 2024 GNOME Foundation
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

#include <errno.h>
#include <locale.h>
#include <string.h>

#include <gio/gio.h>
#include <girepository.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>

#ifdef G_OS_WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include "girmodule-private.h"
#include "girnode-private.h"
#include "girparser-private.h"

static gchar **includedirs = NULL;
static gchar **input = NULL;
static gchar *output = NULL;
static gchar **shlibs = NULL;
static gboolean debug = FALSE;
static gboolean verbose = FALSE;
static gboolean show_version = FALSE;

static gboolean
write_out_typelib (gchar     *prefix,
                   GITypelib *typelib)
{
  FILE *file, *file_owned = NULL;
  gsize written;
  GFile *file_obj;
  gchar *filename;
  GFile *tmp_file_obj;
  gchar *tmp_filename;
  GError *error = NULL;
  gboolean success = FALSE;

  if (output == NULL)
    {
      file = stdout;
      file_obj = NULL;
      filename = NULL;
      tmp_filename = NULL;
      tmp_file_obj = NULL;
#ifdef G_OS_WIN32
      setmode (fileno (file), _O_BINARY);
#endif
    }
  else
    {
      if (prefix)
        {
          filename = g_strdup_printf ("%s-%s", prefix, output);
        }
      else
        {
          filename = g_strdup (output);
        }
      file_obj = g_file_new_for_path (filename);
      tmp_filename = g_strdup_printf ("%s.tmp", filename);
      tmp_file_obj = g_file_new_for_path (tmp_filename);
      file = file_owned = g_fopen (tmp_filename, "wbe");

      if (file == NULL)
        {
          char *message = g_strdup_printf (_("Failed to open ‘%s’: %s"), tmp_filename, g_strerror (errno));
          g_fprintf (stderr, "%s\n", message);
          g_free (message);
          goto out;
        }
    }

  written = fwrite (typelib->data, 1, typelib->len, file);
  if (written < typelib->len)
    {
      char *message = g_strdup_printf (_("Error: Could not write the whole output: %s"), g_strerror (errno));
      g_fprintf (stderr, "%s\n", message);
      g_free (message);
      goto out;
    }

  if (file_owned != NULL)
    fclose (g_steal_pointer (&file_owned));
  if (tmp_filename != NULL)
    {
      if (!g_file_move (tmp_file_obj, file_obj, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, &error))
        {
          char *message = g_strdup_printf (_("Error: Failed to rename ‘%s’ to ‘%s’: %s"),
                                           tmp_filename, filename,
                                           error->message);
          g_fprintf (stderr, "%s\n", message);
          g_free (message);
          g_clear_error (&error);
          goto out;
        }
    }
  success = TRUE;
out:
  g_clear_object (&file_obj);
  g_clear_object (&tmp_file_obj);
  g_free (filename);
  g_free (tmp_filename);

  return success;
}

static GLogLevelFlags logged_levels;

static void
log_handler (const gchar   *log_domain,
             GLogLevelFlags log_level,
             const gchar   *message,
             gpointer       user_data)
{
  if (log_level & logged_levels)
    g_log_default_handler (log_domain, log_level, message, user_data);
}

static GOptionEntry options[] = {
  { "includedir", 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &includedirs, N_("Include directories in GIR search path"), N_("DIRECTORY") },
  { "output", 'o', 0, G_OPTION_ARG_FILENAME, &output, N_("Output file"), N_("FILE") },
  { "shared-library", 'l', 0, G_OPTION_ARG_FILENAME_ARRAY, &shlibs, N_("Shared library"), N_("FILE") },
  { "debug", 0, 0, G_OPTION_ARG_NONE, &debug, N_("Show debug messages"), NULL },
  { "verbose", 0, 0, G_OPTION_ARG_NONE, &verbose, N_("Show verbose messages"), NULL },
  { "version", 0, 0, G_OPTION_ARG_NONE, &show_version, N_("Show program’s version number and exit"), NULL },
  { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &input, NULL, NULL },
  G_OPTION_ENTRY_NULL
};

int
main (int argc, char **argv)
{
  GOptionContext *context;
  GError *error = NULL;
  GIIrParser *parser;
  GIIrModule *module;

  setlocale (LC_ALL, "");

  /* Translators: commandline placeholder */
  context = g_option_context_new (_("FILE"));
  g_option_context_add_main_entries (context, options, NULL);
  g_option_context_parse (context, &argc, &argv, &error);
  g_option_context_free (context);

  if (error)
    {
      char *message = g_strdup_printf (_("Error parsing arguments: %s"), error->message);
      g_fprintf (stderr, "%s\n", message);
      g_free (message);

      g_error_free (error);

      return 1;
    }

  logged_levels = G_LOG_LEVEL_MASK & ~(G_LOG_LEVEL_MESSAGE | G_LOG_LEVEL_DEBUG);
  if (debug)
    logged_levels = logged_levels | G_LOG_LEVEL_DEBUG;
  if (verbose)
    logged_levels = logged_levels | G_LOG_LEVEL_MESSAGE;
  g_log_set_always_fatal (G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL);

  g_log_set_default_handler (log_handler, NULL);

  if (show_version)
    {
      g_printf ("gi-compile-repository %u.%u.%u\n",
                GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION);
      return 0;
    }

  if (!input || g_strv_length (input) != 1)
    {
      g_fprintf (stderr, "%s\n", _("Please specify exactly one input file"));

      return 1;
    }

  g_debug ("[parsing] start, %d includes",
           includedirs ? g_strv_length (includedirs) : 0);

  parser = gi_ir_parser_new ();
  gi_ir_parser_set_debug (parser, logged_levels);

  gi_ir_parser_set_includes (parser, (const char *const *) includedirs);

  module = gi_ir_parser_parse_file (parser, input[0], &error);
  if (module == NULL)
    {
      char *message = g_strdup_printf (_("Error parsing file ‘%s’: %s"), input[0], error->message);
      g_fprintf (stderr, "%s\n", message);
      g_free (message);
      gi_ir_parser_free (parser);
      g_error_free (error);

      return 1;
    }

  g_debug ("[parsing] done");

  g_debug ("[building] start");

  {
    GITypelib *typelib = NULL;
    int write_successful;

    if (shlibs)
      {
        if (module->shared_library)
          g_free (module->shared_library);
        module->shared_library = g_strjoinv (",", shlibs);
      }

    g_debug ("[building] module %s", module->name);

    typelib = gi_ir_module_build_typelib (module);
    if (typelib == NULL)
      g_error (_("Failed to build typelib for module ‘%s’"), module->name);
    if (!gi_typelib_validate (typelib, &error))
      g_error (_("Invalid typelib for module ‘%s’: %s"),
               module->name, error->message);

    write_successful = write_out_typelib (NULL, typelib);
    g_clear_pointer (&typelib, gi_typelib_unref);

    if (!write_successful)
      {
        gi_ir_parser_free (parser);
        return 1;
      }
  }

  g_debug ("[building] done");

  gi_ir_parser_free (parser);

  return 0;
}
