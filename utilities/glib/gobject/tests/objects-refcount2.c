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

struct _GTest
{
  GObject object;
};

struct _GTestClass
{
  GObjectClass parent_class;
};

static GType my_test_get_type (void);

static void my_test_class_init (GTestClass * klass);
static void my_test_init (GTest * test);
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
my_test_class_init (GTestClass * klass)
{
  GObjectClass *gobject_class;

  gobject_class = (GObjectClass *) klass;

  parent_class = g_type_class_ref (G_TYPE_OBJECT);

  gobject_class->dispose = my_test_dispose;
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
my_test_do_refcount (GTest * test)
{
  static guint i = 1;

  if (i++ % 100000 == 0)
    g_test_message (".");

  g_object_ref (test);
  g_object_unref (test);
}

static void
test_refcount_object_advanced (void)
{
  gint i;
  GTest *test;

  test = g_object_new (G_TYPE_TEST, NULL);

  for (i = 0; i < 100000000; i++)
    {
      my_test_do_refcount (test);
    }

  g_object_unref (test);
}

int
main (int argc, char **argv)
{
  g_log_set_always_fatal (G_LOG_LEVEL_WARNING |
                          G_LOG_LEVEL_CRITICAL |
                          g_log_set_always_fatal (G_LOG_FATAL_MASK));

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/gobject/refcount/object-advanced", test_refcount_object_advanced);

  return g_test_run ();
}
