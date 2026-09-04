#include <glib.h>
#include <glib-object.h>

#define G_TYPE_TEST                (my_test_get_type ())
#define MY_TEST(test)              (G_TYPE_CHECK_INSTANCE_CAST ((test), G_TYPE_TEST, GTest))
#define MY_IS_TEST(test)           (G_TYPE_CHECK_INSTANCE_TYPE ((test), G_TYPE_TEST))
#define MY_TEST_CLASS(tclass)      (G_TYPE_CHECK_CLASS_CAST ((tclass), G_TYPE_TEST, GTestClass))
#define MY_IS_TEST_CLASS(tclass)   (G_TYPE_CHECK_CLASS_TYPE ((tclass), G_TYPE_TEST))
#define MY_TEST_GET_CLASS(test)    (G_TYPE_INSTANCE_GET_CLASS ((test), G_TYPE_TEST, GTestClass))

enum {
  PROP_0,
  PROP_DUMMY
};

typedef struct _GTest GTest;
typedef struct _GTestClass GTestClass;

struct _GTest
{
  GObject object;
  unsigned int id;
  gint dummy;

  gint count;
  gint setcount;
};

struct _GTestClass
{
  GObjectClass parent_class;
};

static GType my_test_get_type (void);
G_DEFINE_TYPE (GTest, my_test, G_TYPE_OBJECT)

static gint stopping;  /* (atomic) */

static void my_test_get_property (GObject    *object,
				  guint       prop_id,
				  GValue     *value,
				  GParamSpec *pspec);
static void my_test_set_property (GObject      *object,
				  guint         prop_id,
				  const GValue *value,
				  GParamSpec   *pspec);

static void
my_test_class_init (GTestClass * klass)
{
  GObjectClass *gobject_class;

  gobject_class = (GObjectClass *) klass;

  gobject_class->get_property = my_test_get_property;
  gobject_class->set_property = my_test_set_property;

  g_object_class_install_property (gobject_class,
				   PROP_DUMMY,
				   g_param_spec_int ("dummy",
						     NULL,
						     NULL,
						     0, G_MAXINT, 0,
						     G_PARAM_READWRITE));
}

static void
my_test_init (GTest * test)
{
  static guint static_id = 1;
  test->id = static_id++;
}

static void
my_test_get_property (GObject    *object,
		      guint       prop_id,
		      GValue     *value,
		      GParamSpec *pspec)
{
  GTest *test;

  test = MY_TEST (object);

  switch (prop_id)
    {
    case PROP_DUMMY:
      g_value_set_int (value, g_atomic_int_get (&test->dummy));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
my_test_set_property (GObject      *object,
		      guint         prop_id,
		      const GValue *value,
		      GParamSpec   *pspec)
{
  GTest *test;

  test = MY_TEST (object);

  switch (prop_id)
    {
    case PROP_DUMMY:
      g_atomic_int_set (&test->dummy, g_value_get_int (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
dummy_notify (GObject    *object,
	      GParamSpec *pspec)
{
  GTest *test;

  test = MY_TEST (object);

  g_atomic_int_inc (&test->count);
}

static void
my_test_do_property (GTest * test)
{
  gint dummy;

  g_atomic_int_inc (&test->setcount);

  g_object_get (test, "dummy", &dummy, NULL);
  g_object_set (test, "dummy", dummy + 1, NULL);
}

static gpointer
run_thread (GTest * test)
{
  gint i = 1;

  while (!g_atomic_int_get (&stopping)) {
    my_test_do_property (test);
    if ((i++ % 10000) == 0)
      {
        g_thread_yield(); /* force context switch */
      }
  }

  return NULL;
}

static void
test_refcount_properties_3 (void)
{
  gint i;
  GTest *test;
  GArray *test_threads;
  const gint n_threads = 5;

  test = g_object_new (G_TYPE_TEST, NULL);

  g_assert_cmpint (test->count, ==, test->dummy);
  g_signal_connect (test, "notify::dummy", G_CALLBACK (dummy_notify), NULL);

  test_threads = g_array_new (FALSE, FALSE, sizeof (GThread *));

  g_atomic_int_set (&stopping, 0);

  for (i = 0; i < n_threads; i++) {
    GThread *thread;

    thread = g_thread_new (NULL, (GThreadFunc) run_thread, test);
    g_array_append_val (test_threads, thread);
  }
  g_usleep (30000000);

  g_atomic_int_set (&stopping, 1);
  g_test_message ("\nstopping\n");

  /* join all threads */
  for (i = 0; i < n_threads; i++) {
    GThread *thread;

    thread = g_array_index (test_threads, GThread *, i);
    g_thread_join (thread);
  }

  g_test_message ("stopped\n");
  g_test_message ("%d %d\n", test->setcount, test->count);

  g_array_free (test_threads, TRUE);
  g_object_unref (test);
}

int
main (int argc, gchar *argv[])
{
  g_log_set_always_fatal (G_LOG_LEVEL_WARNING |
                          G_LOG_LEVEL_CRITICAL |
                          g_log_set_always_fatal (G_LOG_FATAL_MASK));

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/gobject/refcount/properties-3", test_refcount_properties_3);

  return g_test_run ();
}
