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

enum {
  PROP_0,
  PROP_DUMMY
};

typedef struct _GTest GTest;
typedef struct _GTestClass GTestClass;

struct _GTest
{
  GObject object;

  gint dummy;
};

struct _GTestClass
{
  GObjectClass parent_class;
};

static GType my_test_get_type (void);

static void my_test_class_init (GTestClass * klass);
static void my_test_init (GTest * test);
static void my_test_dispose (GObject * object);
static void my_test_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec);
static void my_test_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec);

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
my_test_class_init (GTestClass * klass)
{
  GObjectClass *gobject_class;

  gobject_class = (GObjectClass *) klass;

  parent_class = g_type_class_ref (G_TYPE_OBJECT);

  gobject_class->dispose = my_test_dispose;
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
my_test_get_property (GObject *object,
                      guint prop_id,
                      GValue *value,
                      GParamSpec *pspec)
{
  GTest *test;

  test = MY_TEST (object);

  switch (prop_id)
    {
    case PROP_DUMMY:
      g_value_set_int (value, test->dummy);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
my_test_set_property (GObject *object,
                      guint prop_id,
                      const GValue *value,
                      GParamSpec *pspec)
{
  GTest *test;

  test = MY_TEST (object);

  switch (prop_id)
    {
    case PROP_DUMMY:
      test->dummy = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static gint count = 0;

static void
dummy_notify (GObject    *object,
              GParamSpec *pspec)
{
  count++;
  if (count % 10000 == 0)
    g_test_message (".");
}

static void
my_test_do_property (GTest * test)
{
  gint dummy;

  g_object_get (test, "dummy", &dummy, NULL);
  g_object_set (test, "dummy", dummy + 1, NULL);
}

static void
test_refcount_properties_2 (void)
{
  gint i;
  GTest *test;

  test = g_object_new (G_TYPE_TEST, NULL);

  g_signal_connect (test, "notify::dummy", G_CALLBACK (dummy_notify), NULL);
  g_assert_cmpint (count, ==, test->dummy);

  for (i = 0; i < 1000000; i++)
    {
      my_test_do_property (test);
    }
  g_assert_cmpint (count, ==, test->dummy);

  g_object_unref (test);
}

int
main (int argc, gchar *argv[])
{
  g_log_set_always_fatal (G_LOG_LEVEL_WARNING |
                          G_LOG_LEVEL_CRITICAL |
                          g_log_set_always_fatal (G_LOG_FATAL_MASK));

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/gobject/refcount/properties-2", test_refcount_properties_2);

  return g_test_run ();
}
