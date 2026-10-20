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
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "gio-tool.h"
#include "glib/glib-private.h"

void
print_error (const gchar *format, ...)
{
  gchar *message;
  va_list args;

  va_start (args, format);
  message = g_strdup_vprintf (format, args);
  va_end (args);

  g_printerr ("gio: %s\n", message);
  g_free (message);
}

void
print_file_error (GFile *file, const gchar *message)
{
  gchar *uri;

  uri = g_file_get_uri (file);
  print_error ("%s: %s", uri, message);
  g_free (uri);
}

void
show_help (GOptionContext *context, const char *message)
{
  char *help;

  if (message)
    g_printerr ("gio: %s\n\n", message);

  help = g_option_context_get_help (context, TRUE, NULL);
  g_printerr ("%s", help);
  g_free (help);
}

const char *
file_type_to_string (GFileType type)
{
  switch (type)
    {
    case G_FILE_TYPE_UNKNOWN:
      return "unknown";
    case G_FILE_TYPE_REGULAR:
      return "regular";
    case G_FILE_TYPE_DIRECTORY:
      return "directory";
    case G_FILE_TYPE_SYMBOLIC_LINK:
      return "symlink";
    case G_FILE_TYPE_SPECIAL:
      return "special";
    case G_FILE_TYPE_SHORTCUT:
      return "shortcut";
    case G_FILE_TYPE_MOUNTABLE:
      return "mountable";
    default:
      return "invalid type";
    }
}

const char *
attribute_type_to_string (GFileAttributeType type)
{
  switch (type)
    {
    case G_FILE_ATTRIBUTE_TYPE_INVALID:
      return "invalid";
    case G_FILE_ATTRIBUTE_TYPE_STRING:
      return "string";
    case G_FILE_ATTRIBUTE_TYPE_BYTE_STRING:
      return "bytestring";
    case G_FILE_ATTRIBUTE_TYPE_BOOLEAN:
      return "boolean";
    case G_FILE_ATTRIBUTE_TYPE_UINT32:
      return "uint32";
    case G_FILE_ATTRIBUTE_TYPE_INT32:
      return "int32";
    case G_FILE_ATTRIBUTE_TYPE_UINT64:
      return "uint64";
    case G_FILE_ATTRIBUTE_TYPE_INT64:
      return "int64";
    case G_FILE_ATTRIBUTE_TYPE_OBJECT:
      return "object";
    default:
      return "unknown type";
    }
}

GFileAttributeType
attribute_type_from_string (const char *str)
{
  if (strcmp (str, "string") == 0)
    return G_FILE_ATTRIBUTE_TYPE_STRING;
  if (strcmp (str, "stringv") == 0)
    return G_FILE_ATTRIBUTE_TYPE_STRINGV;
  if (strcmp (str, "bytestring") == 0)
    return G_FILE_ATTRIBUTE_TYPE_BYTE_STRING;
  if (strcmp (str, "boolean") == 0)
    return G_FILE_ATTRIBUTE_TYPE_BOOLEAN;
  if (strcmp (str, "uint32") == 0)
    return G_FILE_ATTRIBUTE_TYPE_UINT32;
  if (strcmp (str, "int32") == 0)
    return G_FILE_ATTRIBUTE_TYPE_INT32;
  if (strcmp (str, "uint64") == 0)
    return G_FILE_ATTRIBUTE_TYPE_UINT64;
  if (strcmp (str, "int64") == 0)
    return G_FILE_ATTRIBUTE_TYPE_INT64;
  if (strcmp (str, "object") == 0)
    return G_FILE_ATTRIBUTE_TYPE_OBJECT;
  if (strcmp (str, "unset") == 0)
    return G_FILE_ATTRIBUTE_TYPE_INVALID;
  return -1;
}

char *
attribute_flags_to_string (GFileAttributeInfoFlags flags)
{
  GString *s;
  gsize i;
  gboolean first;
  struct {
    guint32 mask;
    char *descr;
  } flag_descr[] = {
    {
      G_FILE_ATTRIBUTE_INFO_COPY_WITH_FILE,
      N_("Copy with file")
    },
    {
      G_FILE_ATTRIBUTE_INFO_COPY_WHEN_MOVED,
      N_("Keep with file when moved")
    }
  };

  first = TRUE;

  s = g_string_new ("");
  for (i = 0; i < G_N_ELEMENTS (flag_descr); i++)
    {
      if (flags & flag_descr[i].mask)
        {
          if (!first)
            g_string_append (s, ", ");
          g_string_append (s, gettext (flag_descr[i].descr));
          first = FALSE;
        }
    }

  return g_string_free (s, FALSE);
}

gboolean
file_is_dir (GFile *file)
{
  GFileInfo *info;
  gboolean res;

  info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_TYPE, 0, NULL, NULL);
  res = info && g_file_info_get_file_type (info) == G_FILE_TYPE_DIRECTORY;
  if (info)
    g_object_unref (info);
  return res;
}


static int
handle_version (int argc, char *argv[], gboolean do_help)
{
  if (do_help || argc > 1)
    {
      if (!do_help)
        g_printerr ("gio: %s\n\n", _("“version” takes no arguments"));

      g_printerr ("%s\n", _("Usage:"));
      g_printerr ("  gio version\n");
      g_printerr ("\n");
      g_printerr ("%s\n", _("Print version information and exit."));

      return do_help ? 0 : 2;
    }

  g_print ("%d.%d.%d\n", glib_major_version, glib_minor_version, glib_micro_version);

  return 0;
}

typedef int (*HandleSubcommand) (int argc, char *argv[], gboolean do_help);

static const struct
{
  const char *name;  /* as used on the command line */
  HandleSubcommand handle_func;  /* (nullable) for "help" only */
  const char *description;  /* translatable */
} gio_subcommands[] = {
  { "help", NULL, N_("Print help") },
  { "version", handle_version, N_("Print version") },
  { "cat", handle_cat, N_("Concatenate files to standard output") },
  { "copy", handle_copy, N_("Copy one or more files") },
  { "info", handle_info, N_("Show information about locations") },
  { "launch", handle_launch, N_("Launch an application from a desktop file") },
  { "list", handle_list, N_("List the contents of locations") },
  { "mime", handle_mime, N_("Get or set the handler for a mimetype") },
  { "mkdir", handle_mkdir, N_("Create directories") },
  { "monitor", handle_monitor, N_("Monitor files and directories for changes") },
  { "mount", handle_mount, N_("Mount or unmount the locations") },
  { "move", handle_move, N_("Move one or more files") },
  { "open", handle_open, N_("Open files with the default application") },
  { "rename", handle_rename, N_("Rename a file") },
  { "remove", handle_remove, N_("Delete one or more files") },
  { "save", handle_save, N_("Read from standard input and save") },
  { "set", handle_set, N_("Set a file attribute") },
  { "trash", handle_trash, N_("Move files or directories to the trash") },
  { "tree", handle_tree, N_("Lists the contents of locations in a tree") },
};

static void
usage (gboolean is_error)
{
  GString *out = NULL;
  size_t name_width = 0;

  out = g_string_new ("");
  g_string_append_printf (out, "%s\n", _("Usage:"));
  g_string_append_printf (out, "  gio %s %s\n", _("COMMAND"), _("[ARGS…]"));
  g_string_append_c (out, '\n');
  g_string_append_printf (out, "%s\n", _("Commands:"));

  /* Work out the maximum name length for column alignment. */
  for (size_t i = 0; i < G_N_ELEMENTS (gio_subcommands); i++)
    name_width = MAX (name_width, strlen (gio_subcommands[i].name));

  for (size_t i = 0; i < G_N_ELEMENTS (gio_subcommands); i++)
    {
      g_string_append_printf (out, "  %-*s  %s\n",
                              (int) name_width, gio_subcommands[i].name,
                              _(gio_subcommands[i].description));
    }

  g_string_append_c (out, '\n');
  g_string_append_printf (out, _("Use %s to get detailed help.\n"), "“gio help COMMAND”");

  if (is_error)
    g_printerr ("%s", out->str);
  else
    g_print ("%s", out->str);

  g_string_free (out, TRUE);
}

int
main (int argc, char **argv)
{
  const char *command;
  gboolean do_help;

#ifdef G_OS_WIN32
  gchar *localedir;
#endif

  setlocale (LC_ALL, GLIB_DEFAULT_LOCALE);
  textdomain (GETTEXT_PACKAGE);

#ifdef G_OS_WIN32
  localedir = _glib_get_locale_dir ();
  bindtextdomain (GETTEXT_PACKAGE, localedir);
  g_free (localedir);
#else
  bindtextdomain (GETTEXT_PACKAGE, GLIB_LOCALE_DIR);
#endif

#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif

  if (argc < 2)
    {
      usage (TRUE);
      return 1;
    }

  command = argv[1];
  argc -= 1;
  argv += 1;

  do_help = FALSE;
  if (g_str_equal (command, "help"))
    {
      if (argc == 1)
        {
          usage (FALSE);
          return 0;
        }
      else
        {
          command = argv[1];
          do_help = TRUE;
        }
    }
  else if (g_str_equal (command, "--help"))
    {
      usage (FALSE);
      return 0;
    }
  else if (g_str_equal (command, "--version"))
    command = "version";

  /* Work out which subcommand it is. */
  for (size_t i = 0; i < G_N_ELEMENTS (gio_subcommands); i++)
    {
      if (g_str_equal (command, gio_subcommands[i].name))
        {
          if (g_str_equal (command, "help"))
            {
              usage (FALSE);
              return 0;
            }

          g_assert (gio_subcommands[i].handle_func != NULL);
          return gio_subcommands[i].handle_func (argc, argv, do_help);
        }
    }

  /* Unknown subcommand. */
  usage (TRUE);

  return 1;
}
