/* Copyright (C) 2008 Luc Pionchon
 * Copyright (C) 2012 David King
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
 */

#include <glib.h>

static void foo_parser_start_element (GMarkupParseContext  *context,
                                      const gchar          *element_name,
                                      const gchar         **attribute_names,
                                      const gchar         **attribute_values,
                                      gpointer              user_data,
                                      GError              **error);
static void foo_parser_end_element   (GMarkupParseContext  *context,
                                      const gchar          *element_name,
                                      gpointer              user_data,
                                      GError              **error);
static void foo_parser_characters    (GMarkupParseContext  *context,
                                      const gchar          *text,
                                      gsize                 text_len,
                                      gpointer              user_data,
                                      GError              **error);
static void foo_parser_passthrough   (GMarkupParseContext  *context,
                                      const gchar          *passthrough_text,
                                      gsize                 text_len,
                                      gpointer              user_data,
                                      GError              **error);
static void foo_parser_error         (GMarkupParseContext  *context,
                                      GError               *error,
                                      gpointer              user_data);

/*
 * Parser
 */
static const GMarkupParser foo_xml_parser = {
  foo_parser_start_element,
  foo_parser_end_element,
  foo_parser_characters,
  foo_parser_passthrough,
  foo_parser_error
};

/*
 * Called for opening tags like <foo bar="baz">
 */
static void
foo_parser_start_element (GMarkupParseContext *context,
                          const gchar         *element_name,
                          const gchar        **attribute_names,
                          const gchar        **attribute_values,
                          gpointer             user_data,
                          GError             **error)
{
  g_print ("element: <%s>\n", element_name);

  for (gsize i = 0; attribute_names[i]; i++)
    {
      g_print ("attribute: %s = \"%s\"\n", attribute_names[i],
                                           attribute_values[i]);
    }
}

/*
 * Called for closing tags like </foo>
 */
static void
foo_parser_end_element (GMarkupParseContext *context,
                        const gchar         *element_name,
                        gpointer             user_data,
                        GError             **error)
{
  g_print ("element: </%s>\n", element_name);
}

/*
 * Called for character data. Text is not nul-terminated
 */
static void
foo_parser_characters (GMarkupParseContext *context,
                       const gchar         *text,
                       gsize                text_len,
                       gpointer             user_data,
                       GError             **error)
{
  g_print ("text: [%s]\n", text);
}

/*
 * Called for strings that should be re-saved verbatim in this same
 * position, but are not otherwise interpretable. At the moment this
 * includes comments and processing instructions. Text is not
 * nul-terminated.
 */
static void
foo_parser_passthrough (GMarkupParseContext  *context,
                        const gchar          *passthrough_text,
                        gsize                 text_len,
                        gpointer              user_data,
                        GError              **error)
{
  g_print ("passthrough: %s\n", passthrough_text);
}

/*
 * Called when any parsing method encounters an error. The GError should not be
 * freed.
 */
static void
foo_parser_error (GMarkupParseContext *context,
                  GError              *error,
                  gpointer             user_data)
{
  g_printerr ("ERROR: %s\n", error->message);
}

int
main (void)
{
  GMarkupParseContext *context;
  gboolean success = FALSE;
  glong len;

  /*
   * Example XML for the parser.
   */
  const gchar foo_xml_example[] =
      "<foo bar='baz' bir='boz'>"
      "   <bar>bar text 1</bar> "
      "   <bar>bar text 2</bar> "
      "   foo text              "
      "<!-- nothing -->         "
      "</foo>                   ";

  len = g_utf8_strlen (foo_xml_example, -1);
  g_print ("Parsing: %s\n", foo_xml_example);
  g_print ("(%ld UTF-8 characters)\n", len);

  context = g_markup_parse_context_new (&foo_xml_parser, G_MARKUP_DEFAULT_FLAGS, NULL, NULL);

  success = g_markup_parse_context_parse (context, foo_xml_example, len, NULL);

  g_markup_parse_context_free (context);

  if (success)
    {
      g_print ("DONE\n");
      return 0;
    }
  else
    {
      g_printerr ("ERROR\n");
      return 1;
    }
}
