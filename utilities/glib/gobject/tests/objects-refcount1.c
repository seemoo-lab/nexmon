#include <glib.h>
#include <glib-object.h>

#ifdef G_OS_UNIX
#include <unistd.h>
#endif

#define G_TYPE_TEST                (my_test_get_type ())
#define MY_TEST(test)              (G_TYPE_CHECK_INSTANCE_CAST ((test), G_TYPE_TEST, GTest))
#define MY_IS_TEST(test)           (G_TYPE_CHECK_INSTANCE_TYPE ((test), G_TYPE_TEST))
#define MY_TEST_CLASS(tclass)      (G_TYPE_CHECK_CLASS_CAST ((tclass), G_TYPE_TEST, GTestClass))
#define MY_IS_TEST_CLASS(tclass)   (G_TYPE_CHECK_CLASS_TYPE ((tclass), G_TYPE_TEST))
#define MY_TEST_GET_CLASS(test)    (G_TYPE_INSTANCE_GET_CLASS ((test), G_TYPE_TEST, GTestClass))

typedef struct _GTest GTest;
typedef struct _GTestClass GTestClass;

#if G_GNUC_CHECK_VERSION (4, 0)
/* Increase the alignment of GTest to check whether
 * G_TYPE_CHECK_INSTANCE_CAST() would trigger a "-Wcast-align=strict" warning.
 * That would happen, when trying to cast a "GObject*" to "GTest*", if latter
 * has larger alignment.
 *
 * Note that merely adding a int64 field to GTest does not increase the
 * alignment above 4 bytes on i386, hence use the __attribute__((__aligned__())).
 */
#define _GTest_increase_alignment __attribute__((__aligned__(__alignof(gint64))))
#else
#define _GTest_increase_alignment
#endif

struct _GTest
{
  GObject object;

  /* See _GTest_increase_alignment. */
  long double increase_alignment2;
} _GTest_increase_alignment;

struct _GTestClass
{
  GObjectClass parent_class;
};

static GType my_test_get_type (void);
static gint stopping;  /* (atomic) */

static void my_test_class_init (GTestClass * klass,
                                void       * class_data);
static void my_test_init (GTest * test,
                          void  * class_data);
static void my_test_dispose (GObject * object);

static GObjectClass *parent_class = NULL;

static GType
my_test_get_type (void)
{
  static GType test_type = 0;

  if (!test_type) {
    const GTypeInfo test_info = {
      sizeof (GTestClass),
      NULL,
      NULL,
      (GClassInitFunc) my_test_class_init,
      NULL,
      NULL,
      sizeof (GTest),
      0,
      (GInstanceInitFunc) my_test_init,
      NULL
    };

    test_type = g_type_register_static (G_TYPE_OBJECT, "GTest",
        &test_info, 0);
  }
  return test_type;
}

static void
my_test_class_init (GTestClass * klass,
                    G_GNUC_UNUSED void * class_data)
{
  GObjectClass *gobject_class;

  gobject_class = (GObjectClass *) klass;
  parent_class = g_type_class_ref (G_TYPE_OBJECT);

  gobject_class->dispose = my_test_dispose;
}

static void
my_test_init (GTest * test,
              G_GNUC_UNUSED void * class_data)
{
  g_test_message ("init %p\n", test);
}

static void
my_test_dispose (GObject * object)
{
  GTest *test;

  test = MY_TEST (object);

  g_test_message ("dispose %p!\n", test);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
my_test_do_refcount (GTest * test)
{
  g_object_ref (test);
  g_object_unref (test);
}

static gpointer
run_thread (GTest * test)
{
  gint i = 1;

  while (!g_atomic_int_get (&stopping)) {
    my_test_do_refcount (test);
    if ((i++ % 10000) == 0) {
        g_thread_yield (); /* force context switch */
    }
  }

  return NULL;
}

static void
test_refcount_object_basics (void)
{
  guint i;
  GTest *test1, *test2;
  GArray *test_threads;
  const guint n_threads = 5;

  test1 = g_object_new (G_TYPE_TEST, NULL);
  test2 = g_object_new (G_TYPE_TEST, NULL);

  test_threads = g_array_new (FALSE, FALSE, sizeof (GThread *));

  g_atomic_int_set (&stopping, 0);

  for (i = 0; i < n_threads; i++) {
    GThread *thread;

    thread = g_thread_new (NULL, (GThreadFunc) run_thread, test1);
    g_array_append_val (test_threads, thread);

    thread = g_thread_new (NULL, (GThreadFunc) run_thread, test2);
    g_array_append_val (test_threads, thread);
  }

  g_usleep (5000000);
  g_atomic_int_set (&stopping, 1);

  /* join all threads */
  for (i = 0; i < 2 * n_threads; i++) {
    GThread *thread;

    thread = g_array_index (test_threads, GThread *, i);
    g_thread_join (thread);
  }

  g_object_unref (test1);
  g_object_unref (test2);
  g_array_unref (test_threads);
}

int
main (int argc, gchar *argv[])
{
  g_log_set_always_fatal (G_LOG_LEVEL_WARNING |
                          G_LOG_LEVEL_CRITICAL |
                          g_log_set_always_fatal (G_LOG_FATAL_MASK));

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/gobject/refcount/object-basics", test_refcount_object_basics);

  return g_test_run ();
}
