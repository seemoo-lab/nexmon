/*
 * Copyright © 2022 Endless OS Foundation, LLC
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
 * Author: Philip Withnall <pwithnall@endlessos.org>
 */

#include <gio/gio.h>
#include <locale.h>


static void
test_basic_properties (void)
{
  GApplicationCommandLine *cl = NULL;
  const gchar * const arguments[] = { "arg1", "arg2", "arg3", NULL };
  GVariantBuilder options_builder = G_VARIANT_BUILDER_INIT (G_VARIANT_TYPE_VARDICT);
  GVariantBuilder platform_data_builder = G_VARIANT_BUILDER_INIT (G_VARIANT_TYPE_VARDICT);
  gchar **argv = NULL;
  int argc = 0;
  GVariantDict *options_dict;
  GVariant *platform_data;
  GVariantDict *platform_data_dict = NULL;
  gboolean is_remote;

  /* Basic construction. */
  g_variant_builder_add (&options_builder, "{sv}", "option1", g_variant_new_string ("value1"));
  g_variant_builder_add (&options_builder, "{sv}", "option2", g_variant_new_string ("value2"));

  g_variant_builder_add (&platform_data_builder, "{sv}", "data1", g_variant_new_string ("data-value1"));
  g_variant_builder_add (&platform_data_builder, "{sv}", "data2", g_variant_new_string ("data-value2"));

  cl = g_object_new (G_TYPE_APPLICATION_COMMAND_LINE,
                     "arguments", g_variant_new_bytestring_array (arguments, -1),
                     "options", g_variant_builder_end (&options_builder),
                     "platform-data", g_variant_builder_end (&platform_data_builder),
                     NULL);
  g_assert_nonnull (cl);

  /* Check the getters. */
  argv = g_application_command_line_get_arguments (cl, &argc);
  g_assert_cmpint (argc, ==, 3);
  g_assert_cmpstrv (argv, arguments);
  g_clear_pointer (&argv, g_strfreev);

  options_dict = g_application_command_line_get_options_dict (cl);
  g_assert_nonnull (options_dict);
  g_assert_true (g_variant_dict_contains (options_dict, "option1"));
  g_assert_true (g_variant_dict_contains (options_dict, "option2"));

  g_assert_false (g_application_command_line_get_is_remote (cl));

  platform_data = g_application_command_line_get_platform_data (cl);
  g_assert_nonnull (platform_data);
  platform_data_dict = g_variant_dict_new (platform_data);
  g_assert_true (g_variant_dict_contains (platform_data_dict, "data1"));
  g_assert_true (g_variant_dict_contains (platform_data_dict, "data2"));
  g_variant_dict_unref (platform_data_dict);
  g_variant_unref (platform_data);

  /* And g_object_get(). */
  g_object_get (cl, "is-remote", &is_remote, NULL);
  g_assert_false (is_remote);

  /* exit status */
  g_assert_cmpint (g_application_command_line_get_exit_status (cl), ==, 0);
  g_application_command_line_set_exit_status (cl, 1);
  g_assert_cmpint (g_application_command_line_get_exit_status (cl), ==, 1);

  g_application_command_line_done (cl);
  g_application_command_line_set_exit_status (cl, 2);
  g_assert_cmpint (g_application_command_line_get_exit_status (cl), ==, 1);

  g_clear_object (&cl);
}

int
main (int   argc,
      char *argv[])
{
  setlocale (LC_ALL, "");
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/application-command-line/basic-properties", test_basic_properties);

  return g_test_run ();
}
