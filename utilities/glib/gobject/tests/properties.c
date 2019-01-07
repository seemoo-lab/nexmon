#include <stdlib.h>
#include <gstdio.h>
#include <glib-object.h>

typedef struct _TestObject {
  GObject parent_instance;
  gint foo;
  gboolean bar;
  gchar *baz;
  gchar *quux;
} TestObject;

typedef struct _TestObjectClass {
  GObjectClass parent_class;
} TestObjectClass;

enum { PROP_0, PROP_FOO, PROP_BAR, PROP_BAZ, PROP_QUUX, N_PROPERTIES };

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

static GType test_object_get_type (void);
G_DEFINE_TYPE (TestObject, test_object, G_TYPE_OBJECT);

static void
test_object_set_foo (TestObject *obj,
                     gint        foo)
{
  if (obj->foo != foo)
    {
      obj->foo = foo;

      g_assert (properties[PROP_FOO] != NULL);
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

      g_assert (properties[PROP_BAR] != NULL);
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

      g_assert (properties[PROP_BAZ] != NULL);
      g_object_notify_by_pspec (G_OBJECT (obj), properties[PROP_BAZ]);
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

      g_assert (properties[PROP_QUUX] != NULL);
      g_object_notify_by_pspec (G_OBJECT (obj), properties[PROP_QUUX]);
    }
}

static void
test_object_finalize (GObject *gobject)
{
  g_free (((TestObject *) gobject)->baz);

  /* When the ref_count of an object is zero it is still
   * possible to notify the property, but it should do
   * nothing and silenty quit (bug #705570)
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
  g_assert_cmpint (prop_id, !=, N_PROPERTIES);
  g_assert (pspec == properties[prop_id]);

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
  g_assert_cmpint (prop_id, !=, N_PROPERTIES);
  g_assert (pspec == properties[prop_id]);

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
                                           G_PARAM_READWRITE);
  properties[PROP_BAR] = g_param_spec_boolean ("bar", "Bar", "Bar",
                                               FALSE,
                                               G_PARAM_READWRITE);
  properties[PROP_BAZ] = g_param_spec_string ("baz", "Baz", "Baz",
                                              NULL,
                                              G_PARAM_READWRITE);
  properties[PROP_QUUX] = g_param_spec_string ("quux", "quux", "quux",
                                               NULL,
                                               G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  gobject_class->set_property = test_object_set_property;
  gobject_class->get_property = test_object_get_property;
  gobject_class->finalize = test_object_finalize;

  g_object_class_install_properties (gobject_class, N_PROPERTIES, properties);
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

  g_assert (properties[PROP_FOO] != NULL);

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (obj), "foo");
  g_assert (properties[PROP_FOO] == pspec);

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
           TestNotifyClosure *clos)
{
  g_assert (clos->pspec == pspec);
  g_assert_cmpstr (clos->name, ==, pspec->name);
  clos->fired = TRUE;
}

static void
properties_notify (void)
{
  TestObject *obj = g_object_new (test_object_get_type (), NULL);
  TestNotifyClosure clos;

  g_assert (properties[PROP_FOO] != NULL);
  g_assert (properties[PROP_QUUX] != NULL);
  g_signal_connect (obj, "notify", G_CALLBACK (on_notify), &clos);

  clos.name = "foo";
  clos.pspec = properties[PROP_FOO];

  clos.fired = FALSE;
  g_object_set (obj, "foo", 47, NULL);
  g_assert (clos.fired);

  clos.name = "baz";
  clos.pspec = properties[PROP_BAZ];

  clos.fired = FALSE;
  g_object_set (obj, "baz", "something new", NULL);
  g_assert (clos.fired);

  /* baz lacks explicit notify, so we will see this twice */
  clos.fired = FALSE;
  g_object_set (obj, "baz", "something new", NULL);
  g_assert (clos.fired);

  /* quux on the other hand, ... */
  clos.name = "quux";
  clos.pspec = properties[PROP_QUUX];

  clos.fired = FALSE;
  g_object_set (obj, "quux", "something new", NULL);
  g_assert (clos.fired);

  /* no change; no notify */
  clos.fired = FALSE;
  g_object_set (obj, "quux", "something new", NULL);
  g_assert (!clos.fired);


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
  g_assert (n->pspec[n->pos] == pspec);
  n->pos++;
}

static void
properties_notify_queue (void)
{
  TestObject *obj = g_object_new (test_object_get_type (), NULL);
  Notifys n;

  g_assert (properties[PROP_FOO] != NULL);

  n.pspec[0] = properties[PROP_BAZ];
  n.pspec[1] = properties[PROP_BAR];
  n.pspec[2] = properties[PROP_FOO];
  n.pos = 0;

  g_signal_connect (obj, "notify", G_CALLBACK (on_notify2), &n);

  g_object_freeze_notify (G_OBJECT (obj));
  g_object_set (obj, "foo", 47, NULL);
  g_object_set (obj, "bar", TRUE, "foo", 42, "baz", "abc", NULL);
  g_object_thaw_notify (G_OBJECT (obj));
  g_assert (n.pos == 3);

  g_object_unref (obj);
}

static void
properties_construct (void)
{
  TestObject *obj;
  gint val;
  gboolean b;
  gchar *s;

  g_test_bug ("630357");

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
  g_assert (val == 18);
  g_object_get (obj, "bar", &b, NULL);
  g_assert (!b);
  g_object_get (obj, "baz", &s, NULL);
  g_assert_cmpstr (s, ==, "boo");
  g_free (s);

  g_object_unref (obj);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_bug_base ("http://bugzilla.gnome.org/");

  g_test_add_func ("/properties/install", properties_install);
  g_test_add_func ("/properties/notify", properties_notify);
  g_test_add_func ("/properties/notify-queue", properties_notify_queue);
  g_test_add_func ("/properties/construct", properties_construct);

  return g_test_run ();
}
