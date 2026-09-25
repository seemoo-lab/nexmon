/* module-test.c - test program for GMODULE
 * Copyright (C) 1998 Tim Janik
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

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

#include <gmodule.h>
#include <glib/gstdio.h>

#ifdef _MSC_VER
# define MODULE_FILENAME_PREFIX ""
#else
# define MODULE_FILENAME_PREFIX "lib"
#endif

gchar *global_state = NULL;

G_MODULE_EXPORT void g_clash_func (void);

G_MODULE_EXPORT void
g_clash_func (void)
{
  global_state = "global clash";
}

typedef	void (*SimpleFunc) (void);
typedef	void (*GModuleFunc) (GModule *);

static gchar **gplugin_a_state;
static gchar **gplugin_b_state;

static void
compare (const gchar *desc, const gchar *expected, const gchar *found)
{
  if (!expected && !found)
    return;

  if (expected && found && strcmp (expected, found) == 0)
    return;

  g_error ("error: %s state should have been \"%s\", but is \"%s\"",
	   desc, expected ? expected : "NULL", found ? found : "NULL");
}

static void
test_states (const gchar *global, const gchar *gplugin_a, const gchar *gplugin_b)
{
  compare ("global", global, global_state);
  compare ("Plugin A", gplugin_a, *gplugin_a_state);
  compare ("Plugin B", gplugin_b, *gplugin_b_state);

  global_state = *gplugin_a_state = *gplugin_b_state = NULL;
}

static SimpleFunc plugin_clash_func = NULL;

static void
test_module_basics (void)
{
  if (!g_module_supported ())
    {
      g_test_skip ("dynamic modules not supported");
      return;
    }

  /* Run the actual test in a subprocess to avoid symbol table changes from
   * previous tests potentially affecting it. */
  if (g_test_subprocess ())
    {
      GModule *module_self, *module_a, *module_b;
      gchar *plugin_a, *plugin_b;
      SimpleFunc f_a, f_b, f_self;
      GModuleFunc gmod_f;
      GError *error = NULL;

      plugin_a = g_test_build_filename (G_TEST_BUILT, MODULE_FILENAME_PREFIX "moduletestplugin_a_" MODULE_TYPE, NULL);
      plugin_b = g_test_build_filename (G_TEST_BUILT, MODULE_FILENAME_PREFIX "moduletestplugin_b_" MODULE_TYPE, NULL);

      /* module handles */

      module_self = g_module_open_full (NULL, G_MODULE_BIND_LAZY, &error);
      g_assert_no_error (error);
      if (!module_self)
        g_error ("error: %s", g_module_error ());

      /* On Windows static compilation mode, glib API symbols are not
       * exported dynamically by definition. */
#if !defined(G_PLATFORM_WIN32) || !defined(GLIB_STATIC_COMPILATION)
      if (!g_module_symbol (module_self, "g_module_close", (gpointer *) &f_self))
        g_error ("error: %s", g_module_error ());
#endif

      module_a = g_module_open_full (plugin_a, G_MODULE_BIND_LAZY, &error);
      g_assert_no_error (error);
      if (!module_a)
        g_error ("error: %s", g_module_error ());

      module_b = g_module_open_full (plugin_b, G_MODULE_BIND_LAZY, &error);
      g_assert_no_error (error);
      if (!module_b)
        g_error ("error: %s", g_module_error ());

      /* get plugin state vars */

      if (!g_module_symbol (module_a, "gplugin_a_state",
                            (gpointer *) &gplugin_a_state))
        g_error ("error: %s", g_module_error ());

      if (!g_module_symbol (module_b, "gplugin_b_state",
                            (gpointer *) &gplugin_b_state))
        g_error ("error: %s", g_module_error ());
      test_states (NULL, NULL, "check-init");

      /* get plugin specific symbols and call them */

      if (!g_module_symbol (module_a, "gplugin_a_func", (gpointer *) &f_a))
        g_error ("error: %s", g_module_error ());
      test_states (NULL, NULL, NULL);

      if (!g_module_symbol (module_b, "gplugin_b_func", (gpointer *) &f_b))
        g_error ("error: %s", g_module_error ());
      test_states (NULL, NULL, NULL);

      f_a ();
      test_states (NULL, "Hello world", NULL);

      f_b ();
      test_states (NULL, NULL, "Hello world");

      /* get and call globally clashing functions */

      if (!g_module_symbol (module_self, "g_clash_func", (gpointer *) &f_self))
        g_error ("error: %s", g_module_error ());
      test_states (NULL, NULL, NULL);

      if (!g_module_symbol (module_a, "g_clash_func", (gpointer *) &f_a))
        g_error ("error: %s", g_module_error ());
      test_states (NULL, NULL, NULL);

      if (!g_module_symbol (module_b, "g_clash_func", (gpointer *) &f_b))
        g_error ("error: %s", g_module_error ());
      test_states (NULL, NULL, NULL);

      f_self ();
      test_states ("global clash", NULL, NULL);

      f_a ();
      test_states (NULL, "global clash", NULL);

      f_b ();
      test_states (NULL, NULL, "global clash");

      /* get and call clashing plugin functions  */

      if (!g_module_symbol (module_a, "gplugin_clash_func", (gpointer *) &f_a))
        g_error ("error: %s", g_module_error ());
      test_states (NULL, NULL, NULL);

      if (!g_module_symbol (module_b, "gplugin_clash_func", (gpointer *) &f_b))
        g_error ("error: %s", g_module_error ());
      test_states (NULL, NULL, NULL);

      plugin_clash_func = f_a;
      plugin_clash_func ();
      test_states (NULL, "plugin clash", NULL);

      plugin_clash_func = f_b;
      plugin_clash_func ();
      test_states (NULL, NULL, "plugin clash");

      /* call gmodule function from A  */

      if (!g_module_symbol (module_a, "gplugin_a_module_func", (gpointer *) &gmod_f))
        g_error ("error: %s", g_module_error ());
      test_states (NULL, NULL, NULL);

      gmod_f (module_b);
      test_states (NULL, NULL, "BOOH");

      gmod_f (module_a);
      test_states (NULL, "BOOH", NULL);

      /* unload plugins  */

      if (!g_module_close (module_a))
        g_error ("error: %s", g_module_error ());

      if (!g_module_close (module_b))
        g_error ("error: %s", g_module_error ());

      g_free (plugin_a);
      g_free (plugin_b);
      g_module_close (module_self);
    }
  else
    {
      g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
      g_test_trap_assert_passed ();
    }
}

static void
test_module_invalid_libtool_archive (void)
{
  int la_fd;
  gchar *la_filename = NULL;
  GModule *module = NULL;
  GError *local_error = NULL;

  g_test_summary ("Test that opening an invalid .la file fails");

  /* Create an empty temporary file ending in `.la` */
  la_fd = g_file_open_tmp ("gmodule-invalid-XXXXXX.la", &la_filename, &local_error);
  g_assert_no_error (local_error);
  g_assert_true (g_str_has_suffix (la_filename, ".la"));
  g_close (la_fd, NULL);

  /* Try loading it */
  module = g_module_open_full (la_filename, 0, &local_error);
  g_assert_error (local_error, G_MODULE_ERROR, G_MODULE_ERROR_FAILED);
  g_assert_null (module);
  g_clear_error (&local_error);

  (void) g_unlink (la_filename);

  g_free (la_filename);
}

static void
test_local_binding (void)
{
  g_test_summary ("Test that binding a library's symbols locally does not add them globally");

#if defined(G_PLATFORM_WIN32)
  g_test_skip ("G_MODULE_BIND_LOCAL is not supported on Windows.");
  return;
#elif defined(__CYGWIN__)
  g_test_skip ("G_MODULE_BIND_LOCAL is not supported on Cygwin.");
  return;
#endif

  /* Run the actual test in a subprocess to avoid symbol table changes from
   * previous tests potentially affecting it. */
  if (g_test_subprocess ())
    {
      gchar *plugin = NULL;
      GModule *module_plugin = NULL, *module_self = NULL;
      GError *error = NULL;

      gboolean found_symbol = FALSE;
      gpointer symbol = NULL;

      plugin = g_test_build_filename (G_TEST_BUILT, MODULE_FILENAME_PREFIX "moduletestplugin_a_" MODULE_TYPE, NULL);

      module_plugin = g_module_open_full (plugin, G_MODULE_BIND_LOCAL, &error);
      g_assert_no_error (error);
      g_assert_nonnull (module_plugin);

      module_self = g_module_open_full (NULL, 0, &error);
      g_assert_no_error (error);
      g_assert_nonnull (module_self);

      found_symbol = g_module_symbol (module_self, "gplugin_say_boo_func", &symbol);
      g_assert_false (found_symbol);
      g_assert_null (symbol);

      g_module_close (module_self);
      g_module_close (module_plugin);
      g_free (plugin);
    }
  else
    {
      g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
      g_test_trap_assert_passed ();
    }
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/module/basics", test_module_basics);
  g_test_add_func ("/module/invalid-libtool-archive", test_module_invalid_libtool_archive);
  g_test_add_func ("/module/local-binding", test_local_binding);

  return g_test_run ();
}
