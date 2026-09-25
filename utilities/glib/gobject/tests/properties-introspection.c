/* properties-introspection.c: Test the properties introspection API
 *
 * SPDX-FileCopyrightText: 2023 Emmanuele Bassi
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

/* This test is isolated so we can control the initialization of
 * GObjectClass, and the global GParamSpecPool
 */

#include <stdlib.h>
#include <glib-object.h>

G_DECLARE_INTERFACE (MyTestable, my_testable, MY, TESTABLE, GObject)

struct _MyTestableInterface
{
  GTypeInterface g_iface;
};

G_DEFINE_INTERFACE (MyTestable, my_testable, G_TYPE_OBJECT)

static void
my_testable_default_init (MyTestableInterface *iface)
{
  g_object_interface_install_property (iface,
    g_param_spec_int ("check", NULL, NULL, -1, 10, 0, G_PARAM_READWRITE));
}

static void
properties_introspection (void)
{
  g_test_summary ("Verify that introspecting properties on an interface initializes the GParamSpecPool.");

  if (g_test_subprocess ())
    {
      gpointer klass = g_type_default_interface_ref (my_testable_get_type ());
      g_assert_nonnull (klass);

      GParamSpec *pspec = g_object_interface_find_property (klass, "check");
      g_assert_nonnull (pspec);

      g_type_default_interface_unref (klass);
      return;
    }

  g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stderr ("");
}

static gpointer
inspect_func (gpointer data)
{
  unsigned int *n_checks = data; /* (atomic) */

  gpointer klass = NULL;
  do
    {
      klass = g_type_default_interface_ref (my_testable_get_type ());
    }
  while (klass == NULL);

  GParamSpec *pspec = NULL;
  do
    {
      pspec = g_object_interface_find_property (klass, "check");
    }
  while (pspec == NULL);

  g_type_default_interface_unref (klass);

  g_atomic_int_inc (n_checks);

  return NULL;
}

#define N_THREADS 10

static void
properties_collision (void)
{
  GThread *threads[N_THREADS];
  unsigned int n_checks = 0; /* (atomic) */

  g_test_summary ("Verify that multiple threads create a single GParamSpecPool.");

  for (unsigned int i = 0; i < N_THREADS; i++)
    {
      char *t_name = g_strdup_printf ("inspect [%d]", i);
      threads[i] = g_thread_new (t_name, inspect_func, &n_checks);
      g_assert_nonnull (threads[i]);
      g_free (t_name);
    }

  while (g_atomic_int_get (&n_checks) != N_THREADS)
    g_usleep (50);

  for (unsigned int i = 0; i < N_THREADS; i++)
    g_thread_join (threads[i]);
}

#undef N_THREADS

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/properties/introspection", properties_introspection);
  g_test_add_func ("/properties/collision", properties_collision);

  return g_test_run ();
}
