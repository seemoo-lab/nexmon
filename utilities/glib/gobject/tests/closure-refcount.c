/* Copyright (C) 2005 Imendio AB
 *
 * SPDX-License-Identifier: LicenseRef-old-glib-tests
 *
 * This software is provided "as is"; redistribution and modification
 * is permitted, provided that the following disclaimer is retained.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */
#include <glib-object.h>

#ifdef G_OS_UNIX
#include <unistd.h>
#endif

#define TEST_POINTER1   ((gpointer) 47)
#define TEST_POINTER2   ((gpointer) 49)
#define TEST_INT1       (-77)
#define TEST_INT2       (78)

/* --- GTest class --- */
typedef struct {
  GObject object;
  gint value;
  gpointer test_pointer1;
  gpointer test_pointer2;
} GTest;
typedef struct {
  GObjectClass parent_class;
  void (*test_signal1) (GTest * test, gint an_int);
  void (*test_signal2) (GTest * test, gint an_int);
} GTestClass;

#define G_TYPE_TEST                (my_test_get_type ())
#define MY_TEST(test)              (G_TYPE_CHECK_INSTANCE_CAST ((test), G_TYPE_TEST, GTest))
#define MY_IS_TEST(test)           (G_TYPE_CHECK_INSTANCE_TYPE ((test), G_TYPE_TEST))
#define MY_TEST_CLASS(tclass)      (G_TYPE_CHECK_CLASS_CAST ((tclass), G_TYPE_TEST, GTestClass))
#define MY_IS_TEST_CLASS(tclass)   (G_TYPE_CHECK_CLASS_TYPE ((tclass), G_TYPE_TEST))
#define MY_TEST_GET_CLASS(test)    (G_TYPE_INSTANCE_GET_CLASS ((test), G_TYPE_TEST, GTestClass))

static GType my_test_get_type (void);
G_DEFINE_TYPE (GTest, my_test, G_TYPE_OBJECT)

/* Test state */
typedef struct
{
  GClosure *closure;  /* (unowned) */
  gboolean stopping;
  gboolean seen_signal_handler;
  gboolean seen_cleanup;
  gboolean seen_test_int1;
  gboolean seen_test_int2;
  gboolean seen_thread1;
  gboolean seen_thread2;
} TestClosureRefcountData;

/* --- functions --- */
static void
my_test_init (GTest * test)
{
  g_test_message ("Init %p", test);

  test->value = 0;
  test->test_pointer1 = TEST_POINTER1;
  test->test_pointer2 = TEST_POINTER2;
}

typedef enum
{
  PROP_TEST_PROP = 1,
} MyTestProperty;

typedef enum
{
  SIGNAL_TEST_SIGNAL1,
  SIGNAL_TEST_SIGNAL2,
} MyTestSignal;

static guint signals[SIGNAL_TEST_SIGNAL2 + 1] = { 0, };

static void
my_test_set_property (GObject      *object,
                     guint         prop_id,
                     const GValue *value,
                     GParamSpec   *pspec)
{
  GTest *test = MY_TEST (object);
  switch (prop_id)
    {
    case PROP_TEST_PROP:
      test->value = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
my_test_get_property (GObject    *object,
                     guint       prop_id,
                     GValue     *value,
                     GParamSpec *pspec)
{
  GTest *test = MY_TEST (object);
  switch (prop_id)
    {
    case PROP_TEST_PROP:
      g_value_set_int (value, test->value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
my_test_test_signal2 (GTest *test,
                      gint   an_int)
{
}

static void
my_test_emit_test_signal1 (GTest *test,
                           gint   vint)
{
  g_signal_emit (G_OBJECT (test), signals[SIGNAL_TEST_SIGNAL1], 0, vint);
}

static void
my_test_emit_test_signal2 (GTest *test,
                           gint   vint)
{
  g_signal_emit (G_OBJECT (test), signals[SIGNAL_TEST_SIGNAL2], 0, vint);
}

static void
my_test_class_init (GTestClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = my_test_set_property;
  gobject_class->get_property = my_test_get_property;

  signals[SIGNAL_TEST_SIGNAL1] =
      g_signal_new ("test-signal1", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (GTestClass, test_signal1), NULL, NULL,
                    g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT);
  signals[SIGNAL_TEST_SIGNAL2] =
      g_signal_new ("test-signal2", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (GTestClass, test_signal2), NULL, NULL,
                    g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT);

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_TEST_PROP,
                                   g_param_spec_int ("test-prop", "Test Prop", "Test property",
                                                     0, 1, 0, G_PARAM_READWRITE));
  klass->test_signal2 = my_test_test_signal2;
}

static void
test_closure (GClosure *closure)
{
  /* try to produce high contention in closure->ref_count */
  guint i = 0, n = g_random_int () % 199;
  for (i = 0; i < n; i++)
    g_closure_ref (closure);
  g_closure_sink (closure); /* NOP */
  for (i = 0; i < n; i++)
    g_closure_unref (closure);
}

static gpointer
thread1_main (gpointer user_data)
{
  TestClosureRefcountData *data = user_data;
  guint i = 0;

  for (i = 1; !g_atomic_int_get (&data->stopping); i++)
    {
      test_closure (data->closure);
      if (i % 10000 == 0)
        {
          g_test_message ("Yielding from thread1");
          g_thread_yield (); /* force context switch */
          g_atomic_int_set (&data->seen_thread1, TRUE);
        }
    }
  return NULL;
}

static gpointer
thread2_main (gpointer user_data)
{
  TestClosureRefcountData *data = user_data;
  guint i = 0;

  for (i = 1; !g_atomic_int_get (&data->stopping); i++)
    {
      test_closure (data->closure);
      if (i % 10000 == 0)
        {
          g_test_message ("Yielding from thread2");
          g_thread_yield (); /* force context switch */
          g_atomic_int_set (&data->seen_thread2, TRUE);
        }
    }
  return NULL;
}

static void
test_signal_handler (GTest   *test,
                     gint     vint,
                     gpointer user_data)
{
  TestClosureRefcountData *data = user_data;

  g_assert_true (test->test_pointer1 == TEST_POINTER1);

  data->seen_signal_handler = TRUE;
  data->seen_test_int1 |= vint == TEST_INT1;
  data->seen_test_int2 |= vint == TEST_INT2;
}

static void
destroy_data (gpointer  user_data,
              GClosure *closure)
{
  TestClosureRefcountData *data = user_data;

  data->seen_cleanup = TRUE;
  g_assert_true (data->closure == closure);
  g_assert_cmpint (closure->ref_count, ==, 0);
}

static void
test_emissions (GTest *test)
{
  my_test_emit_test_signal1 (test, TEST_INT1);
  my_test_emit_test_signal2 (test, TEST_INT2);
}

/* Test that closure refcounting works even when high contested between three
 * threads (the main thread, thread1 and thread2). Both child threads are
 * contesting refs/unrefs, while the main thread periodically emits signals
 * which also do refs/unrefs on closures. */
static void
test_closure_refcount (void)
{
  GThread *thread1, *thread2;
  TestClosureRefcountData test_data = { 0, };
  GClosure *closure;
  GTest *object;
  guint i, n_iterations;

  object = g_object_new (G_TYPE_TEST, NULL);
  closure = g_cclosure_new (G_CALLBACK (test_signal_handler), &test_data, destroy_data);

  g_signal_connect_closure (object, "test-signal1", closure, FALSE);
  g_signal_connect_closure (object, "test-signal2", closure, FALSE);

  test_data.stopping = FALSE;
  test_data.closure = closure;

  thread1 = g_thread_new ("thread1", thread1_main, &test_data);
  thread2 = g_thread_new ("thread2", thread2_main, &test_data);

  /* The 16-bit compare-and-swap operations currently used for closure
   * refcounts are really slow on some ARM CPUs, notably Cortex-A57.
   * Reduce the number of iterations so that the test completes in a
   * finite time, but don't reduce it so much that the main thread
   * starves the other threads and causes a test failure.
   *
   * https://gitlab.gnome.org/GNOME/glib/issues/1316
   * aka https://bugs.debian.org/880883 */
#if defined(__aarch64__) || defined(__arm__)
  n_iterations = 100000;
#else
  n_iterations = 1000000;
#endif

  /* Run the test for a reasonably high number of iterations, and ensure we
   * don’t terminate until at least 10000 iterations have completed in both
   * thread1 and thread2. Even though @n_iterations is high, we can’t guarantee
   * that the scheduler allocates time fairly (or at all!) to thread1 or
   * thread2. */
  for (i = 1;
       i < n_iterations ||
       !g_atomic_int_get (&test_data.seen_thread1) ||
       !g_atomic_int_get (&test_data.seen_thread2);
       i++)
    {
      test_emissions (object);
      if (i % 10000 == 0)
        {
          g_test_message ("Yielding from main thread");
          g_thread_yield (); /* force context switch */
        }
    }

  g_atomic_int_set (&test_data.stopping, TRUE);
  g_test_message ("Stopping");

  /* wait for thread shutdown */
  g_thread_join (thread1);
  g_thread_join (thread2);

  /* finalize object, destroy signals, run cleanup code */
  g_object_unref (object);

  g_test_message ("Stopped");

  g_assert_true (g_atomic_int_get (&test_data.seen_thread1));
  g_assert_true (g_atomic_int_get (&test_data.seen_thread2));
  g_assert_true (test_data.seen_test_int1);
  g_assert_true (test_data.seen_test_int2);
  g_assert_true (test_data.seen_signal_handler);
  g_assert_true (test_data.seen_cleanup);
}

int
main (int argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/closure/refcount", test_closure_refcount);

  return g_test_run ();
}
