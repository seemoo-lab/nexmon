/* GObject - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 2005 Red Hat, Inc.
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
 */

#include <glib-object.h>

/* This test tests weak and toggle references */

static GObject *global_object;

static gboolean object_destroyed;
static gboolean weak_ref1_notified;
static gboolean weak_ref2_notified;
static gboolean toggle_ref1_weakened;
static gboolean toggle_ref1_strengthened;
static gboolean toggle_ref2_weakened;
static gboolean toggle_ref2_strengthened;
static gboolean toggle_ref3_weakened;
static gboolean toggle_ref3_strengthened;

/* TestObject, a parent class for TestObject */
static GType test_object_get_type (void);
#define TEST_TYPE_OBJECT          (test_object_get_type ())
typedef struct _TestObject        TestObject;
typedef struct _TestObjectClass   TestObjectClass;

struct _TestObject
{
  GObject parent_instance;
};
struct _TestObjectClass
{
  GObjectClass parent_class;
};

G_DEFINE_TYPE (TestObject, test_object, G_TYPE_OBJECT)

static void
test_object_finalize (GObject *object)
{
  object_destroyed = TRUE;

  G_OBJECT_CLASS (test_object_parent_class)->finalize (object);
}

static void
test_object_class_init (TestObjectClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = test_object_finalize;
}

static void
test_object_init (TestObject *test_object)
{
}

static void
clear_flags (void)
{
  object_destroyed = FALSE;
  weak_ref1_notified = FALSE;
  weak_ref2_notified = FALSE;
  toggle_ref1_weakened = FALSE;
  toggle_ref1_strengthened = FALSE;
  toggle_ref2_weakened = FALSE;
  toggle_ref2_strengthened = FALSE;
  toggle_ref3_weakened = FALSE;
  toggle_ref3_strengthened = FALSE;
}

static void
weak_ref1 (gpointer data,
           GObject *object)
{
  g_assert_true (object == global_object);
  g_assert_cmpint (GPOINTER_TO_INT (data), ==, 42);

  weak_ref1_notified = TRUE;
}

static void
weak_ref2 (gpointer data,
           GObject *object)
{
  g_assert_true (object == global_object);
  g_assert_cmpint (GPOINTER_TO_INT (data), ==, 24);

  weak_ref2_notified = TRUE;
}

static void
weak_ref3 (gpointer data,
           GObject *object)
{
  GWeakRef *weak_ref = data;

  g_assert_null (g_weak_ref_get (weak_ref));

  weak_ref2_notified = TRUE;
}

static void
toggle_ref1 (gpointer data,
             GObject *object,
             gboolean is_last_ref)
{
  g_assert_true (object == global_object);
  g_assert_cmpint (GPOINTER_TO_INT (data), ==, 42);

  if (is_last_ref)
    toggle_ref1_weakened = TRUE;
  else
    toggle_ref1_strengthened = TRUE;
}

static void
toggle_ref2 (gpointer data,
             GObject *object,
             gboolean is_last_ref)
{
  g_assert_true (object == global_object);
  g_assert_cmpint (GPOINTER_TO_INT (data), ==, 24);

  if (is_last_ref)
    toggle_ref2_weakened = TRUE;
  else
    toggle_ref2_strengthened = TRUE;
}

static void
toggle_ref3 (gpointer data,
             GObject *object,
             gboolean is_last_ref)
{
  g_assert_true (object == global_object);
  g_assert_cmpint (GPOINTER_TO_INT (data), ==, 34);

  if (is_last_ref)
    {
      toggle_ref3_weakened = TRUE;
      g_object_remove_toggle_ref (object, toggle_ref3, GUINT_TO_POINTER (34));
    }
  else
    toggle_ref3_strengthened = TRUE;
}

static void
test_references (void)
{
  GObject *object;
  GWeakRef weak_ref;

  /* Test basic weak reference operation */
  global_object = object = g_object_new (TEST_TYPE_OBJECT, NULL);

  g_object_weak_ref (object, weak_ref1, GUINT_TO_POINTER (42));

  clear_flags ();
  g_object_unref (object);
  g_assert_true (weak_ref1_notified);
  g_assert_true (object_destroyed);

  /* Test two weak references at once
   */
  global_object = object = g_object_new (TEST_TYPE_OBJECT, NULL);

  g_object_weak_ref (object, weak_ref1, GUINT_TO_POINTER (42));
  g_object_weak_ref (object, weak_ref2, GUINT_TO_POINTER (24));

  clear_flags ();
  g_object_unref (object);
  g_assert_true (weak_ref1_notified);
  g_assert_true (weak_ref2_notified);
  g_assert_true (object_destroyed);

  /* Test remove weak references */
  global_object = object = g_object_new (TEST_TYPE_OBJECT, NULL);

  g_object_weak_ref (object, weak_ref1, GUINT_TO_POINTER (42));
  g_object_weak_ref (object, weak_ref2, GUINT_TO_POINTER (24));
  g_object_weak_unref (object, weak_ref1, GUINT_TO_POINTER (42));

  clear_flags ();
  g_object_unref (object);
  g_assert_false (weak_ref1_notified);
  g_assert_true (weak_ref2_notified);
  g_assert_true (object_destroyed);

  /* Test that within a GWeakNotify the GWeakRef is NULL already. */
  weak_ref2_notified = FALSE;
  object = g_object_new (G_TYPE_OBJECT, NULL);
  g_weak_ref_init (&weak_ref, object);
  g_assert_true (object == g_weak_ref_get (&weak_ref));
  g_object_weak_ref (object, weak_ref3, &weak_ref);
  g_object_unref (object);
  g_object_unref (object);
  g_assert_true (weak_ref2_notified);
  g_weak_ref_clear (&weak_ref);

  /* Test basic toggle reference operation */
  global_object = object = g_object_new (TEST_TYPE_OBJECT, NULL);

  g_object_add_toggle_ref (object, toggle_ref1, GUINT_TO_POINTER (42));

  clear_flags ();
  g_object_unref (object);
  g_assert_true (toggle_ref1_weakened);
  g_assert_false (toggle_ref1_strengthened);
  g_assert_false (object_destroyed);

  clear_flags ();
  g_object_ref (object);
  g_assert_false (toggle_ref1_weakened);
  g_assert_true (toggle_ref1_strengthened);
  g_assert_false (object_destroyed);

  g_object_unref (object);

  clear_flags ();
  g_object_remove_toggle_ref (object, toggle_ref1, GUINT_TO_POINTER (42));
  g_assert_false (toggle_ref1_weakened);
  g_assert_false (toggle_ref1_strengthened);
  g_assert_true (object_destroyed);

  global_object = object = g_object_new (TEST_TYPE_OBJECT, NULL);

  /* Test two toggle references at once */
  g_object_add_toggle_ref (object, toggle_ref1, GUINT_TO_POINTER (42));
  g_object_add_toggle_ref (object, toggle_ref2, GUINT_TO_POINTER (24));

  clear_flags ();
  g_object_unref (object);
  g_assert_false (toggle_ref1_weakened);
  g_assert_false (toggle_ref1_strengthened);
  g_assert_false (toggle_ref2_weakened);
  g_assert_false (toggle_ref2_strengthened);
  g_assert_false (object_destroyed);

  clear_flags ();
  g_object_remove_toggle_ref (object, toggle_ref1, GUINT_TO_POINTER (42));
  g_assert_false (toggle_ref1_weakened);
  g_assert_false (toggle_ref1_strengthened);
  g_assert_true (toggle_ref2_weakened);
  g_assert_false (toggle_ref2_strengthened);
  g_assert_false (object_destroyed);

  clear_flags ();
  /* Check that removing a toggle ref with %NULL data works fine. */
  g_object_remove_toggle_ref (object, toggle_ref2, NULL);
  g_assert_false (toggle_ref1_weakened);
  g_assert_false (toggle_ref1_strengthened);
  g_assert_false (toggle_ref2_weakened);
  g_assert_false (toggle_ref2_strengthened);
  g_assert_true (object_destroyed);

  /* Test a toggle reference that removes itself */
  global_object = object = g_object_new (TEST_TYPE_OBJECT, NULL);

  g_object_add_toggle_ref (object, toggle_ref3, GUINT_TO_POINTER (34));

  clear_flags ();
  g_object_unref (object);
  g_assert_true (toggle_ref3_weakened);
  g_assert_false (toggle_ref3_strengthened);
  g_assert_true (object_destroyed);
}

/*****************************************************************************/

static guint weak_ref4_notified;
static guint weak_ref4_finalizing_notified;
static guint weak_ref4_finalizing_notified_skipped;
static gint weak_ref4_finalizing_resurrected = -1;
static guint weak_ref4_indexes_n;
static const guint *weak_ref4_indexes;

static void
weak_ref4_finalizing (gpointer data,
                      GObject *object)
{
  static gint last = -1;
  gint idx;

  g_assert_true (weak_ref4_finalizing_resurrected == FALSE || weak_ref4_finalizing_resurrected == 4);

  idx = (gint) GPOINTER_TO_UINT (data);
  g_assert_cmpint (last, <, idx);
  last = idx;

  weak_ref4_finalizing_notified++;
}

static void
weak_ref4 (gpointer data,
           GObject *object)
{
  static gint last = -1;
  guint idx;

  g_assert_cmpint (weak_ref4_finalizing_notified, ==, 0);

  idx = GPOINTER_TO_UINT (data);
  g_assert_cmpint (last, <, (gint) idx);
  last = (gint) idx;

  weak_ref4_notified++;

  if (weak_ref4_finalizing_resurrected == -1)
    {
      if (g_random_boolean ())
        {
          /* Resurrect the object. */
          weak_ref4_finalizing_resurrected = TRUE;
          g_object_ref (object);
        }
      else
        weak_ref4_finalizing_resurrected = FALSE;
    }

  g_object_weak_ref (object, weak_ref4_finalizing, GUINT_TO_POINTER (idx));

  if (weak_ref4_indexes_n > 0 && (g_random_int () % 4 == 0))
    {

      while (weak_ref4_indexes_n > 0 && weak_ref4_indexes[weak_ref4_indexes_n - 1] <= idx)
        {
          /* already invoked. Skip. */
          weak_ref4_indexes_n--;
        }

      /* From the callback, we unregister another callback that we know is
       * still registered. */
      if (weak_ref4_indexes_n > 0)
        {
          guint new_idx = weak_ref4_indexes[weak_ref4_indexes_n - 1];

          weak_ref4_indexes_n--;
          g_object_weak_unref (object, weak_ref4, GUINT_TO_POINTER (new_idx));

          /* Behave as if we got this callback invoked still, so that the remainder matches up. */
          weak_ref4_finalizing_notified_skipped++;
          weak_ref4_notified++;
        }
    }
}

static void
test_references_many (void)
{
  GObject *object;
  guint *indexes;
  guint i;
  guint n;
  guint m;

  /* Test subscribing a (random) number of weak references. */
  object = g_object_new (TEST_TYPE_OBJECT, NULL);
  n = g_random_int () % 1000;
  indexes = g_new (guint, MAX (n, 1));
  for (i = 0; i < n; i++)
    {
      g_object_weak_ref (object, weak_ref4, GUINT_TO_POINTER (i));
      indexes[i] = i;
    }
  for (i = 0; i < n; i++)
    {
      guint tmp, j;

      j = (guint) g_random_int_range ((gint) i, (gint) n);
      tmp = indexes[i];
      indexes[i] = indexes[j];
      indexes[j] = tmp;
    }
  m = g_random_int () % (n + 1u);
  for (i = 0; i < m; i++)
    g_object_weak_unref (object, weak_ref4, GUINT_TO_POINTER (indexes[i]));
  weak_ref4_indexes_n = n - m;
  weak_ref4_indexes = &indexes[m];
  g_object_unref (object);
  g_assert_cmpint (weak_ref4_notified, ==, n - m);
  if (n - m == 0)
    {
      g_assert_cmpint (weak_ref4_finalizing_resurrected, ==, -1);
      g_assert_cmpint (weak_ref4_finalizing_notified, ==, 0);
      g_assert_cmpint (weak_ref4_finalizing_notified_skipped, ==, 0);
    }
  else if (weak_ref4_finalizing_resurrected == TRUE)
    {
      weak_ref4_finalizing_resurrected = 4;
      g_assert_cmpint (weak_ref4_finalizing_notified, ==, 0);
      g_object_unref (object);
      g_assert_cmpint (weak_ref4_notified, ==, n - m);
      g_assert_cmpint (weak_ref4_finalizing_notified + weak_ref4_finalizing_notified_skipped, ==, n - m);
    }
  else
    {
      g_assert_cmpint (weak_ref4_finalizing_resurrected, ==, FALSE);
      g_assert_cmpint (weak_ref4_finalizing_notified + weak_ref4_finalizing_notified_skipped, ==, n - m);
    }
  g_free (indexes);
}

/*****************************************************************************/

static void notify_two (gpointer data, GObject *former_object);

static void
notify_one (gpointer data, GObject *former_object)
{
  g_object_weak_unref (data, notify_two, former_object);
}

static void
notify_two (gpointer data, GObject *former_object)
{
  g_assert_not_reached ();
}

static void
test_references_two (void)
{
  GObject *obj;

  /* https://gitlab.gnome.org/GNOME/gtk/-/issues/5542#note_1688809 */

  obj = g_object_new (G_TYPE_OBJECT, NULL);

  g_object_weak_ref (obj, notify_one, obj);
  g_object_weak_ref (obj, notify_two, obj);

  g_object_unref (obj);
}

/*****************************************************************************/

#define RUN_DISPOSE_N_THREADS 5

GThread *run_dispose_threads[RUN_DISPOSE_N_THREADS];
GObject *run_dispose_obj;
gint run_dispose_thread_ready_count;
const guint RUN_DISPOSE_N_ITERATIONS = 5000;

gint run_dispose_weak_notify_pending;
gint run_dispose_weak_notify_pending_nested;

static void
_run_dispose_weak_notify_nested (gpointer data,
                                 GObject *where_the_object_was)
{
  g_assert_true (run_dispose_obj == where_the_object_was);
  g_assert_cmpint (g_atomic_int_add (&run_dispose_weak_notify_pending_nested, -1), >, 0);
}

static void
_run_dispose_weak_notify (gpointer data,
                          GObject *where_the_object_was)
{
  g_assert_true (run_dispose_obj == where_the_object_was);
  g_assert_cmpint (g_atomic_int_add (&run_dispose_weak_notify_pending, -1), >, 0);

  if (g_random_int () % 10 == 0)
    {
      g_object_run_dispose (run_dispose_obj);
    }

  if (g_random_int () % 10 == 0)
    {
      g_atomic_int_inc (&run_dispose_weak_notify_pending_nested);
      g_object_weak_ref (run_dispose_obj, _run_dispose_weak_notify_nested, NULL);
    }
}

static gpointer
_run_dispose_thread_fcn (gpointer thread_data)
{
  guint i;

  g_atomic_int_inc (&run_dispose_thread_ready_count);

  while (g_atomic_int_get (&run_dispose_thread_ready_count) != RUN_DISPOSE_N_THREADS)
    g_usleep (10);

  for (i = 0; i < RUN_DISPOSE_N_ITERATIONS; i++)
    {
      g_atomic_int_inc (&run_dispose_weak_notify_pending);
      g_object_weak_ref (run_dispose_obj, _run_dispose_weak_notify, NULL);
      if (g_random_int () % 10 == 0)
        g_object_run_dispose (run_dispose_obj);
    }

  g_object_run_dispose (run_dispose_obj);

  return NULL;
}

static void
test_references_run_dispose (void)
{
  GQuark quark_weak_notifies = g_quark_from_static_string ("GObject-weak-notifies");
  guint i;

  run_dispose_obj = g_object_new (G_TYPE_OBJECT, NULL);

  for (i = 0; i < RUN_DISPOSE_N_THREADS; i++)
    {
      run_dispose_threads[i] = g_thread_new ("run-dispose", _run_dispose_thread_fcn, GINT_TO_POINTER (i));
    }
  for (i = 0; i < RUN_DISPOSE_N_THREADS; i++)
    {
      g_thread_join (run_dispose_threads[i]);
    }

  g_assert_cmpint (g_atomic_int_get (&run_dispose_weak_notify_pending), ==, 0);

  if (g_atomic_int_get (&run_dispose_weak_notify_pending_nested) == 0)
    {
      g_assert_null (g_object_get_qdata (run_dispose_obj, quark_weak_notifies));
    }
  else
    {
      g_assert_nonnull (g_object_get_qdata (run_dispose_obj, quark_weak_notifies));
    }

  g_object_run_dispose (run_dispose_obj);

  g_assert_cmpint (g_atomic_int_get (&run_dispose_weak_notify_pending), ==, 0);
  g_assert_cmpint (g_atomic_int_get (&run_dispose_weak_notify_pending_nested), ==, 0);
  g_assert_null (g_object_get_qdata (run_dispose_obj, quark_weak_notifies));

  g_clear_object (&run_dispose_obj);
}

/*****************************************************************************/

int
main (int   argc,
      char *argv[])
{
  g_log_set_always_fatal (g_log_set_always_fatal (G_LOG_FATAL_MASK) |
                          G_LOG_LEVEL_WARNING |
                          G_LOG_LEVEL_CRITICAL);

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/gobject/references", test_references);
  g_test_add_func ("/gobject/references-many", test_references_many);
  g_test_add_func ("/gobject/references_two", test_references_two);
  g_test_add_func ("/gobject/references_run_dispose", test_references_run_dispose);

  return g_test_run ();
}
