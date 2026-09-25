#include <stdlib.h>
#include <gstdio.h>
#include <glib-object.h>

typedef struct _TestObject {
  GObject parent_instance;
  gint foo;
  gboolean bar;
  gchar *baz;
  GVariant *var;  /* (nullable) (owned) */
  gchar *quux;
} TestObject;

typedef struct _TestObjectClass {
  GObjectClass parent_class;
} TestObjectClass;

enum { PROP_0, PROP_FOO, PROP_BAR, PROP_BAZ, PROP_VAR, PROP_QUUX, N_PROPERTIES };

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

static GType test_object_get_type (void);
G_DEFINE_TYPE (TestObject, test_object, G_TYPE_OBJECT)

static void
test_object_set_foo (TestObject *obj,
                     gint        foo)
{
  if (obj->foo != foo)
    {
      obj->foo = foo;

      g_assert_nonnull (properties[PROP_FOO]);
      g_object_notify_by_pspec (G_OBJECT (obj), properties[PROP_FOO]);
    }
}

static void
test_object_set_bar (TestObject *obj,
                     gboolean    bar)
{
  bar = !!bar;

  if (obj->bar != bar)
    {
      obj->bar = bar;

      g_assert_nonnull (properties[PROP_BAR]);
      g_object_notify_by_pspec (G_OBJECT (obj), properties[PROP_BAR]);
    }
}

static void
test_object_set_baz (TestObject  *obj,
                     const gchar *baz)
{
  if (g_strcmp0 (obj->baz, baz) != 0)
    {
      g_free (obj->baz);
      obj->baz = g_strdup (baz);

      g_assert_nonnull (properties[PROP_BAZ]);
      g_object_notify_by_pspec (G_OBJECT (obj), properties[PROP_BAZ]);
    }
}

static void
test_object_set_var (TestObject *obj,
                     GVariant   *var)
{
  GVariant *new_var = NULL;

  if (var == NULL || obj->var == NULL ||
      !g_variant_equal (var, obj->var))
    {
      /* Note: We deliberately don’t sink @var here, to make sure that
       * properties_set_property_variant_floating() is testing that GObject
       * internally sinks variants. */
      new_var = g_variant_ref (var);
      g_clear_pointer (&obj->var, g_variant_unref);
      obj->var = g_steal_pointer (&new_var);

      g_assert_nonnull (properties[PROP_VAR]);
      g_object_notify_by_pspec (G_OBJECT (obj), properties[PROP_VAR]);
    }
}

static void
test_object_set_quux (TestObject  *obj,
                      const gchar *quux)
{
  if (g_strcmp0 (obj->quux, quux) != 0)
    {
      g_free (obj->quux);
      obj->quux = g_strdup (quux);

      g_assert_nonnull (properties[PROP_QUUX]);
      g_object_notify_by_pspec (G_OBJECT (obj), properties[PROP_QUUX]);
    }
}

static void
test_object_finalize (GObject *gobject)
{
  TestObject *self = (TestObject *) gobject;

  g_free (self->baz);
  g_clear_pointer (&self->var, g_variant_unref);
  g_free (self->quux);

  /* When the ref_count of an object is zero it is still
   * possible to notify the property, but it should do
   * nothing and silently quit (bug #705570)
   */
  g_object_notify (gobject, "foo");
  g_object_notify_by_pspec (gobject, properties[PROP_BAR]);

  G_OBJECT_CLASS (test_object_parent_class)->finalize (gobject);
}

static void
test_object_set_property (GObject *gobject,
                          guint prop_id,
                          const GValue *value,
                          GParamSpec *pspec)
{
  TestObject *tobj = (TestObject *) gobject;

  g_assert_cmpint (prop_id, !=, 0);
  g_assert_true (prop_id < N_PROPERTIES && pspec == properties[prop_id]);

  switch (prop_id)
    {
    case PROP_FOO:
      test_object_set_foo (tobj, g_value_get_int (value));
      break;

    case PROP_BAR:
      test_object_set_bar (tobj, g_value_get_boolean (value));
      break;

    case PROP_BAZ:
      test_object_set_baz (tobj, g_value_get_string (value));
      break;

    case PROP_VAR:
      test_object_set_var (tobj, g_value_get_variant (value));
      break;

    case PROP_QUUX:
      test_object_set_quux (tobj, g_value_get_string (value));
      break;

    default:
      g_assert_not_reached ();
    }
}

static void
test_object_get_property (GObject *gobject,
                          guint prop_id,
                          GValue *value,
                          GParamSpec *pspec)
{
  TestObject *tobj = (TestObject *) gobject;

  g_assert_cmpint (prop_id, !=, 0);
  g_assert_true (prop_id < N_PROPERTIES && pspec == properties[prop_id]);

  switch (prop_id)
    {
    case PROP_FOO:
      g_value_set_int (value, tobj->foo);
      break;

    case PROP_BAR:
      g_value_set_boolean (value, tobj->bar);
      break;

    case PROP_BAZ:
      g_value_set_string (value, tobj->baz);
      break;

    case PROP_VAR:
      g_value_set_variant (value, tobj->var);
      break;

    case PROP_QUUX:
      g_value_set_string (value, tobj->quux);
      break;

    default:
      g_assert_not_reached ();
    }
}

static void
test_object_class_init (TestObjectClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  properties[PROP_FOO] = g_param_spec_int ("foo", "Foo", "Foo",
                                           -1, G_MAXINT,
                                           0,
                                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  properties[PROP_BAR] = g_param_spec_boolean ("bar", "Bar", "Bar",
                                               FALSE,
                                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  properties[PROP_BAZ] = g_param_spec_string ("baz", "Baz", "Baz",
                                              NULL,
                                              G_PARAM_READWRITE);
  properties[PROP_VAR] = g_param_spec_variant ("var", "Var", "Var",
                                               G_VARIANT_TYPE_STRING, NULL,
                                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  gobject_class->set_property = test_object_set_property;
  gobject_class->get_property = test_object_get_property;
  gobject_class->finalize = test_object_finalize;

  g_object_class_install_properties (gobject_class, N_PROPERTIES - 1, properties);

  /* We intentionally install this property separately, to test
   * that that works, and that property lookup works regardless
   * how the property was installed.
   */
  properties[PROP_QUUX] = g_param_spec_string ("quux", "quux", "quux",
                                               NULL,
                                               G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_property (gobject_class, PROP_QUUX, properties[PROP_QUUX]);
}

static void
test_object_init (TestObject *self)
{
  self->foo = 42;
  self->bar = TRUE;
  self->baz = g_strdup ("Hello");
  self->quux = NULL;
}

static void
properties_install (void)
{
  TestObject *obj = g_object_new (test_object_get_type (), NULL);
  GParamSpec *pspec;
  char *name;

  g_assert_nonnull (properties[PROP_FOO]);

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (obj), "foo");
  g_assert_true (properties[PROP_FOO] == pspec);

  name = g_strdup ("bar");
  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (obj), name);
  g_assert_true (properties[PROP_BAR] == pspec);
  g_free (name);

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (obj), "baz");
  g_assert_true (properties[PROP_BAZ] == pspec);

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (obj), "var");
  g_assert_true (properties[PROP_VAR] == pspec);

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (obj), "quux");
  g_assert_true (properties[PROP_QUUX] == pspec);

  g_object_unref (obj);
}

typedef struct {
  GObject parent_instance;
  int value[16];
} ManyProps;

typedef GObjectClass ManyPropsClass;

static GParamSpec *props[16];

GType many_props_get_type (void);

G_DEFINE_TYPE(ManyProps, many_props, G_TYPE_OBJECT)

static void
many_props_init (ManyProps *self)
{
}

static void
get_prop (GObject    *object,
          guint       prop_id,
          GValue     *value,
          GParamSpec *pspec)
{
  ManyProps *mp = (ManyProps *) object;

  if (prop_id > 0 && prop_id < 13)
    g_value_set_int (value, mp->value[prop_id]);
  else
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static void
set_prop (GObject      *object,
          guint         prop_id,
          const GValue *value,
          GParamSpec   *pspec)
{
  ManyProps *mp = (ManyProps *) object;

  if (prop_id > 0 && prop_id < 13)
    mp->value[prop_id] = g_value_get_int (value);
  else
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static void
many_props_class_init (ManyPropsClass *class)
{
  G_OBJECT_CLASS (class)->get_property = get_prop;
  G_OBJECT_CLASS (class)->set_property = set_prop;

  props[1] = g_param_spec_int ("one", NULL, NULL,
                               0, G_MAXINT, 0,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  props[2] = g_param_spec_int ("two", NULL, NULL,
                               0, G_MAXINT, 0,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  props[3] = g_param_spec_int ("three", NULL, NULL,
                               0, G_MAXINT, 0,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  props[4] = g_param_spec_int ("four", NULL, NULL,
                               0, G_MAXINT, 0,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  props[5] = g_param_spec_int ("five", NULL, NULL,
                               0, G_MAXINT, 0,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  props[6] = g_param_spec_int ("six", NULL, NULL,
                               0, G_MAXINT, 0,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  props[7] = g_param_spec_int ("seven", NULL, NULL,
                               0, G_MAXINT, 0,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  props[8] = g_param_spec_int ("eight", NULL, NULL,
                               0, G_MAXINT, 0,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  props[9] = g_param_spec_int ("nine", NULL, NULL,
                               0, G_MAXINT, 0,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  props[10] = g_param_spec_int ("ten", NULL, NULL,
                               0, G_MAXINT, 0,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  props[11] = g_param_spec_int ("eleven", NULL, NULL,
                               0, G_MAXINT, 0,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  props[12] = g_param_spec_int ("twelve", NULL, NULL,
                               0, G_MAXINT, 0,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_properties (G_OBJECT_CLASS (class), 12, props);
}

static void
properties_install_many (void)
{
  ManyProps *obj = g_object_new (many_props_get_type (), NULL);
  GParamSpec *pspec;

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (obj), "one");
  g_assert_true (props[1] == pspec);

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (obj), "ten");
  g_assert_true (props[10] == pspec);

  g_object_unref (obj);
}

typedef struct {
  const gchar *name;
  GParamSpec *pspec;
  gboolean    fired;
} TestNotifyClosure;

static void
on_notify (GObject           *gobject,
           GParamSpec        *pspec,
           TestNotifyClosure *closure)
{
  g_assert_true (closure->pspec == pspec);
  g_assert_cmpstr (closure->name, ==, pspec->name);
  closure->fired = TRUE;
}

static void
properties_notify (void)
{
  TestObject *obj = g_object_new (test_object_get_type (), NULL);
  TestNotifyClosure closure;

  g_assert_nonnull (properties[PROP_FOO]);
  g_assert_nonnull (properties[PROP_QUUX]);
  g_signal_connect (obj, "notify", G_CALLBACK (on_notify), &closure);

  closure.name = "foo";
  closure.pspec = properties[PROP_FOO];

  closure.fired = FALSE;
  g_object_set (obj, "foo", 47, NULL);
  g_assert_true (closure.fired);

  closure.name = "baz";
  closure.pspec = properties[PROP_BAZ];

  closure.fired = FALSE;
  g_object_set (obj, "baz", "something new", NULL);
  g_assert_true (closure.fired);

  /* baz lacks explicit notify, so we will see this twice */
  closure.fired = FALSE;
  g_object_set (obj, "baz", "something new", NULL);
  g_assert_true (closure.fired);

  /* quux on the other hand, ... */
  closure.name = "quux";
  closure.pspec = properties[PROP_QUUX];

  closure.fired = FALSE;
  g_object_set (obj, "quux", "something new", NULL);
  g_assert_true (closure.fired);

  /* no change; no notify */
  closure.fired = FALSE;
  g_object_set (obj, "quux", "something new", NULL);
  g_assert_false (closure.fired);


  g_object_unref (obj);
}

typedef struct {
  GParamSpec *pspec[3];
  gint pos;
} Notifys;

static void
on_notify2 (GObject    *gobject,
            GParamSpec *pspec,
            Notifys    *n)
{
  g_assert_true (n->pspec[n->pos] == pspec);
  n->pos++;
}

static void
properties_notify_queue (void)
{
  TestObject *obj = g_object_new (test_object_get_type (), NULL);
  Notifys n;

  g_assert_nonnull (properties[PROP_FOO]);

  n.pspec[0] = properties[PROP_BAZ];
  n.pspec[1] = properties[PROP_BAR];
  n.pspec[2] = properties[PROP_FOO];
  n.pos = 0;

  g_signal_connect (obj, "notify", G_CALLBACK (on_notify2), &n);

  g_object_freeze_notify (G_OBJECT (obj));
  g_object_set (obj, "foo", 47, NULL);
  g_object_set (obj, "bar", TRUE, "foo", 42, "baz", "abc", NULL);
  g_object_thaw_notify (G_OBJECT (obj));
  g_assert_cmpint (n.pos, ==, 3);

  g_object_unref (obj);
}

static void
test_properties_notify_too_frozen (void)
{
  if (g_test_subprocess ())
    {
      TestObject *obj = g_object_new (test_object_get_type (), NULL);

      for (unsigned int i = 0; i < 1000000; i++)
        g_object_freeze_notify (G_OBJECT (obj));

      g_object_unref (obj);
    }

  g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*CRITICAL*called g_object_freeze_notify() too often*");
}

static void
properties_construct (void)
{
  TestObject *obj;
  gint val;
  gboolean b;
  gchar *s;

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=630357");

  /* more than 16 args triggers a realloc in g_object_new_valist() */
  obj = g_object_new (test_object_get_type (),
                      "foo", 1,
                      "foo", 2,
                      "foo", 3,
                      "foo", 4,
                      "foo", 5,
                      "bar", FALSE,
                      "foo", 6,
                      "foo", 7,
                      "foo", 8,
                      "foo", 9,
                      "foo", 10,
                      "baz", "boo",
                      "foo", 11,
                      "foo", 12,
                      "foo", 13,
                      "foo", 14,
                      "foo", 15,
                      "foo", 16,
                      "foo", 17,
                      "foo", 18,
                      NULL);

  g_object_get (obj, "foo", &val, NULL);
  g_assert_cmpint (val, ==, 18);
  g_object_get (obj, "bar", &b, NULL);
  g_assert_false (b);
  g_object_get (obj, "baz", &s, NULL);
  g_assert_cmpstr (s, ==, "boo");
  g_free (s);

  g_object_unref (obj);
}

static void
properties_testv_with_no_properties (void)
{
  TestObject *test_obj;
  const char *prop_names[4] = { "foo", "bar", "baz", "quux" };
  GValue values_out[4] = { G_VALUE_INIT };
  guint i;

  /* Test newv_with_properties && getv */
  test_obj = (TestObject *) g_object_new_with_properties (
      test_object_get_type (), 0, NULL, NULL);
  g_object_getv (G_OBJECT (test_obj), 4, prop_names, values_out);

  /* It should have init values */
  g_assert_cmpint (g_value_get_int (&values_out[0]), ==, 42);
  g_assert_true (g_value_get_boolean (&values_out[1]));
  g_assert_cmpstr (g_value_get_string (&values_out[2]), ==, "Hello");
  g_assert_cmpstr (g_value_get_string (&values_out[3]), ==, NULL);

  for (i = 0; i < 4; i++)
    g_value_unset (&values_out[i]);
  g_object_unref (test_obj);
}

static void
properties_testv_with_valid_properties (void)
{
  TestObject *test_obj;
  const char *prop_names[4] = { "foo", "bar", "baz", "quux" };

  GValue values_in[4] = { G_VALUE_INIT };
  GValue values_out[4] = { G_VALUE_INIT };
  guint i;

  g_value_init (&(values_in[0]), G_TYPE_INT);
  g_value_set_int (&(values_in[0]), 100);

  g_value_init (&(values_in[1]), G_TYPE_BOOLEAN);
  g_value_set_boolean (&(values_in[1]), TRUE);

  g_value_init (&(values_in[2]), G_TYPE_STRING);
  g_value_set_string (&(values_in[2]), "pigs");

  g_value_init (&(values_in[3]), G_TYPE_STRING);
  g_value_set_string (&(values_in[3]), "fly");

  /* Test newv_with_properties && getv */
  test_obj = (TestObject *) g_object_new_with_properties (
      test_object_get_type (), 4, prop_names, values_in);
  g_object_getv (G_OBJECT (test_obj), 4, prop_names, values_out);

  g_assert_cmpint (g_value_get_int (&values_out[0]), ==, 100);
  g_assert_true (g_value_get_boolean (&values_out[1]));
  g_assert_cmpstr (g_value_get_string (&values_out[2]), ==, "pigs");
  g_assert_cmpstr (g_value_get_string (&values_out[3]), ==, "fly");

  for (i = 0; i < G_N_ELEMENTS (values_out); i++)
    g_value_unset (&values_out[i]);

  /* Test newv2 && getv */
  g_value_set_string (&(values_in[2]), "Elmo knows");
  g_value_set_string (&(values_in[3]), "where you live");
  g_object_setv (G_OBJECT (test_obj), 4, prop_names, values_in);

  g_object_getv (G_OBJECT (test_obj), 4, prop_names, values_out);

  g_assert_cmpint (g_value_get_int (&values_out[0]), ==, 100);
  g_assert_true (g_value_get_boolean (&values_out[1]));

  g_assert_cmpstr (g_value_get_string (&values_out[2]), ==, "Elmo knows");
  g_assert_cmpstr (g_value_get_string (&values_out[3]), ==, "where you live");

  for (i = 0; i < G_N_ELEMENTS (values_in); i++)
    g_value_unset (&values_in[i]);
  for (i = 0; i < G_N_ELEMENTS (values_out); i++)
    g_value_unset (&values_out[i]);

  g_object_unref (test_obj);
}

static void
properties_testv_with_invalid_property_type (void)
{
  if (g_test_subprocess ())
    {
      TestObject *test_obj;
      const char *invalid_prop_names[1] = { "foo" };
      GValue values_in[1] = { G_VALUE_INIT };

      g_value_init (&(values_in[0]), G_TYPE_STRING);
      g_value_set_string (&(values_in[0]), "fly");

      test_obj = (TestObject *) g_object_new_with_properties (
          test_object_get_type (), 1, invalid_prop_names, values_in);
      /* should give a warning */

      g_object_unref (test_obj);
    }
  g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*CRITICAL*foo*gint*gchararray*");
}


static void
properties_testv_with_invalid_property_names (void)
{
  if (g_test_subprocess ())
    {
      TestObject *test_obj;
      const char *invalid_prop_names[4] = { "foo", "boo", "moo", "poo" };
      GValue values_in[4] = { G_VALUE_INIT };

      g_value_init (&(values_in[0]), G_TYPE_INT);
      g_value_set_int (&(values_in[0]), 100);

      g_value_init (&(values_in[1]), G_TYPE_BOOLEAN);
      g_value_set_boolean (&(values_in[1]), TRUE);

      g_value_init (&(values_in[2]), G_TYPE_STRING);
      g_value_set_string (&(values_in[2]), "pigs");

      g_value_init (&(values_in[3]), G_TYPE_STRING);
      g_value_set_string (&(values_in[3]), "fly");

      test_obj = (TestObject *) g_object_new_with_properties (
          test_object_get_type (), 4, invalid_prop_names, values_in);
      /* This call should give 3 Critical warnings. Actually, a critical warning
       * shouldn't make g_object_new_with_properties to fail when a bad named
       * property is given, because, it will just ignore that property. However,
       * for test purposes, it is considered that the test doesn't pass.
       */

      g_object_unref (test_obj);
    }

  g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*CRITICAL*g_object_new_is_valid_property*boo*");
}

static void
properties_testv_getv (void)
{
  TestObject *test_obj;
  const char *prop_names[4] = { "foo", "bar", "baz", "quux" };
  GValue values_out_initialized[4] = { G_VALUE_INIT };
  GValue values_out_uninitialized[4] = { G_VALUE_INIT };
  guint i;

  g_value_init (&(values_out_initialized[0]), G_TYPE_INT);
  g_value_init (&(values_out_initialized[1]), G_TYPE_BOOLEAN);
  g_value_init (&(values_out_initialized[2]), G_TYPE_STRING);
  g_value_init (&(values_out_initialized[3]), G_TYPE_STRING);

  test_obj = (TestObject *) g_object_new_with_properties (
      test_object_get_type (), 0, NULL, NULL);

  /* Test g_object_getv for an initialized values array */
  g_object_getv (G_OBJECT (test_obj), 4, prop_names, values_out_initialized);
  /* It should have init values */
  g_assert_cmpint (g_value_get_int (&values_out_initialized[0]), ==, 42);
  g_assert_true (g_value_get_boolean (&values_out_initialized[1]));
  g_assert_cmpstr (g_value_get_string (&values_out_initialized[2]), ==, "Hello");
  g_assert_cmpstr (g_value_get_string (&values_out_initialized[3]), ==, NULL);

  /* Test g_object_getv for an uninitialized values array */
  g_object_getv (G_OBJECT (test_obj), 4, prop_names, values_out_uninitialized);
  /* It should have init values */
  g_assert_cmpint (g_value_get_int (&values_out_uninitialized[0]), ==, 42);
  g_assert_true (g_value_get_boolean (&values_out_uninitialized[1]));
  g_assert_cmpstr (g_value_get_string (&values_out_uninitialized[2]), ==, "Hello");
  g_assert_cmpstr (g_value_get_string (&values_out_uninitialized[3]), ==, NULL);

  for (i = 0; i < 4; i++)
    {
      g_value_unset (&values_out_initialized[i]);
      g_value_unset (&values_out_uninitialized[i]);
    }
  g_object_unref (test_obj);
}

static void
properties_get_property (void)
{
  TestObject *test_obj;
  struct {
    const char *name;
    GType gtype;
    GValue value;
  } test_props[] = {
    { "foo", G_TYPE_INT, G_VALUE_INIT },
    { "bar", G_TYPE_INVALID, G_VALUE_INIT },
    { "bar", G_TYPE_STRING, G_VALUE_INIT },
  };
  gsize i;

  g_test_summary ("g_object_get_property() accepts uninitialized, "
                  "initialized, and transformable values");

  for (i = 0; i < G_N_ELEMENTS (test_props); i++)
    {
      if (test_props[i].gtype != G_TYPE_INVALID)
        g_value_init (&(test_props[i].value), test_props[i].gtype);
    }

  test_obj = (TestObject *) g_object_new_with_properties (test_object_get_type (), 0, NULL, NULL);

  g_test_message ("Test g_object_get_property with an initialized value");
  g_object_get_property (G_OBJECT (test_obj), test_props[0].name, &(test_props[0].value));
  g_assert_cmpint (g_value_get_int (&(test_props[0].value)), ==, 42);

  g_test_message ("Test g_object_get_property with an uninitialized value");
  g_object_get_property (G_OBJECT (test_obj), test_props[1].name, &(test_props[1].value));
  g_assert_true (g_value_get_boolean (&(test_props[1].value)));

  g_test_message ("Test g_object_get_property with a transformable value");
  g_object_get_property (G_OBJECT (test_obj), test_props[2].name, &(test_props[2].value));
  g_assert_true (G_VALUE_HOLDS_STRING (&(test_props[2].value)));
  g_assert_cmpstr (g_value_get_string (&(test_props[2].value)), ==, "TRUE");

  for (i = 0; i < G_N_ELEMENTS (test_props); i++)
    g_value_unset (&(test_props[i].value));

  g_object_unref (test_obj);
}

static void
properties_set_property_variant_floating (void)
{
  TestObject *test_obj = NULL;
  GVariant *owned_floating_variant = NULL;
  GVariant *floating_variant_ptr = NULL;
  GVariant *got_variant = NULL;

  g_test_summary ("Test that setting a property to a floating variant consumes the reference");

  test_obj = (TestObject *) g_object_new (test_object_get_type (), NULL);

  owned_floating_variant = floating_variant_ptr = g_variant_new_string ("this variant has only one floating ref");
  g_assert_true (g_variant_is_floating (floating_variant_ptr));

  g_object_set (test_obj, "var", g_steal_pointer (&owned_floating_variant), NULL);

  /* This assumes that the GObject implementation refs, rather than copies and destroys, the incoming variant */
  g_assert_false (g_variant_is_floating (floating_variant_ptr));

  g_object_get (test_obj, "var", &got_variant, NULL);
  g_assert_false (g_variant_is_floating (got_variant));
  g_assert_cmpvariant (got_variant, floating_variant_ptr);

  g_variant_unref (got_variant);
  g_object_unref (test_obj);
}

static void
properties_testv_notify_queue (void)
{
  TestObject *test_obj;
  const char *prop_names[3] = { "foo", "bar", "baz" };
  GValue values_in[3] = { G_VALUE_INIT };
  Notifys n;
  guint i;

  g_value_init (&(values_in[0]), G_TYPE_INT);
  g_value_set_int (&(values_in[0]), 100);

  g_value_init (&(values_in[1]), G_TYPE_BOOLEAN);
  g_value_set_boolean (&(values_in[1]), TRUE);

  g_value_init (&(values_in[2]), G_TYPE_STRING);
  g_value_set_string (&(values_in[2]), "");

  /* Test newv_with_properties && getv */
  test_obj = (TestObject *) g_object_new_with_properties (
      test_object_get_type (), 0, NULL, NULL);

  g_assert_nonnull (properties[PROP_FOO]);

  n.pspec[0] = properties[PROP_BAZ];
  n.pspec[1] = properties[PROP_BAR];
  n.pspec[2] = properties[PROP_FOO];
  n.pos = 0;

  g_signal_connect (test_obj, "notify", G_CALLBACK (on_notify2), &n);

  g_object_freeze_notify (G_OBJECT (test_obj));
  {
    g_object_setv (G_OBJECT (test_obj), 3, prop_names, values_in);

    /* Set "foo" to 70 */
    g_value_set_int (&(values_in[0]), 100);
    g_object_setv (G_OBJECT (test_obj), 1, prop_names, values_in);
  }
  g_object_thaw_notify (G_OBJECT (test_obj));
  g_assert_cmpint (n.pos, ==, 3);

  for (i = 0; i < 3; i++)
    g_value_unset (&values_in[i]);
  g_object_unref (test_obj);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/properties/install", properties_install);
  g_test_add_func ("/properties/install-many", properties_install_many);
  g_test_add_func ("/properties/notify", properties_notify);
  g_test_add_func ("/properties/notify-queue", properties_notify_queue);
  g_test_add_func ("/properties/notify/too-many-freezes", test_properties_notify_too_frozen);
  g_test_add_func ("/properties/construct", properties_construct);
  g_test_add_func ("/properties/get-property", properties_get_property);
  g_test_add_func ("/properties/set-property/variant/floating", properties_set_property_variant_floating);

  g_test_add_func ("/properties/testv_with_no_properties",
      properties_testv_with_no_properties);
  g_test_add_func ("/properties/testv_with_valid_properties",
      properties_testv_with_valid_properties);
  g_test_add_func ("/properties/testv_with_invalid_property_type",
      properties_testv_with_invalid_property_type);
  g_test_add_func ("/properties/testv_with_invalid_property_names",
      properties_testv_with_invalid_property_names);
  g_test_add_func ("/properties/testv_getv", properties_testv_getv);
  g_test_add_func ("/properties/testv_notify_queue",
      properties_testv_notify_queue);

  return g_test_run ();
}
