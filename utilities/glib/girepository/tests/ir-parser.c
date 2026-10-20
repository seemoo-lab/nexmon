/*
 * Copyright 2025 GNOME Foundation, Inc.
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
 * Authors:
 *  - Philip Withnall <pwithnall@gnome.org>
 */

#include "config.h"

#include "girepository.h"
#include "girparser-private.h"

static void
test_type_parsing (void)
{
  const char *buffer_template = "<?xml version='1.0'?>"
    "<repository version='1.2'"
    "   xmlns='http://www.gtk.org/introspection/core/1.0'"
    "   xmlns:c='http://www.gtk.org/introspection/c/1.0'>"
      "<package name='TestNamespace-1.0'/>"
      "<namespace name='TestNamespace' version='1.0'"
      "   c:identifier-prefixes='test'"
      "   c:symbol-prefixes='test'>"
        "<function name='dummy' c:identifier='dummy'>"
          "<return-value transfer-ownership='none'>"
            "<type name='%s'/>"
          "</return-value>"
          "<parameters>"
          "</parameters>"
        "</function>"
      "</namespace>"
    "</repository>";
  const struct
    {
      const char *type;
      gboolean expected_success;
    }
  vectors[] =
    {
      { "GLib.Error", TRUE },
      { "GLib.Error<IOError,FileError>", TRUE },
      { "GLib.Error<IOError", FALSE },
    };

  g_test_summary ("Test parsing different valid and invalid types");

  for (size_t i = 0; i < G_N_ELEMENTS (vectors); i++)
    {
      GIIrParser *parser = NULL;
      GIIrModule *module;
      GError *local_error = NULL;
      char *buffer = NULL;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
      buffer = g_strdup_printf (buffer_template, vectors[i].type);
#pragma GCC diagnostic pop

      parser = gi_ir_parser_new ();
      module = gi_ir_parser_parse_string (parser, "TestNamespace",
                                          "TestNamespace-1.0.gir",
                                          buffer, -1,
                                          &local_error);

      if (vectors[i].expected_success)
        {
          g_assert_no_error (local_error);
          g_assert_nonnull (module);
        }
      else
        {
          g_assert_error (local_error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT);
          g_assert_null (module);
        }

      g_clear_error (&local_error);
      g_clear_pointer (&parser, gi_ir_parser_free);
      g_free (buffer);
    }
}

int
main (int argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/ir-parser/type-parsing", test_type_parsing);

  return g_test_run ();
}
