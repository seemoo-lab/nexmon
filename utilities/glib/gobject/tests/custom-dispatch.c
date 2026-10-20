/* custom-dispatch.c: Test GObjectClass.dispatch_properties_changed
 * Copyright (C) 2022 Red Hat, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-old-glib-tests
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
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

typedef struct {
  GObject parent_instance;
  int foo;
} TestObject;

typedef struct {
  GObjectClass parent_class;
} TestObjectClass;

typedef enum {
  PROP_FOO = 1,
  N_PROPERTIES,
} TestObjectProperty;

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

      g_assert (properties[PROP_FOO] != NULL);
      g_object_notify_by_pspec (G_OBJECT (obj), properties[PROP_FOO]);
    }
}

static void
test_object_set_property (GObject *gobject,
                          guint prop_id,
                          const GValue *value,
                          GParamSpec *pspec)
{
  TestObject *tobj = (TestObject *) gobject;

  switch ((TestObjectProperty)prop_id)
    {
    case PROP_FOO:
      test_object_set_foo (tobj, g_value_get_int (value));
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

  switch ((TestObjectProperty)prop_id)
    {
    case PROP_FOO:
      g_value_set_int (value, tobj->foo);
      break;

    default:
      g_assert_not_reached ();
    }
}

static int dispatch_properties_called;

static void
test_object_dispatch_properties_changed (GObject     *object,
                                         guint        n_pspecs,
                                         GParamSpec **pspecs)
{
  dispatch_properties_called++;

  G_OBJECT_CLASS (test_object_parent_class)->dispatch_properties_changed (object, n_pspecs, pspecs);
}


static void
test_object_class_init (TestObjectClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  properties[PROP_FOO] = g_param_spec_int ("foo", "Foo", "Foo",
                                           -1, G_MAXINT,
                                           0,
                                           G_PARAM_READWRITE |
                                           G_PARAM_STATIC_STRINGS |
                                           G_PARAM_EXPLICIT_NOTIFY);

  gobject_class->set_property = test_object_set_property;
  gobject_class->get_property = test_object_get_property;
  gobject_class->dispatch_properties_changed = test_object_dispatch_properties_changed;

  g_object_class_install_properties (gobject_class, N_PROPERTIES, properties);
}

static void
test_object_init (TestObject *self)
{
  self->foo = 42;
}

static gboolean
object_has_notify_signal_handlers (gpointer instance)
{
  guint signal_id = g_signal_lookup ("notify", G_TYPE_OBJECT);

  return g_signal_handler_find (instance, G_SIGNAL_MATCH_ID, signal_id, 0, NULL, NULL, NULL) != 0;
}

static void
test_custom_dispatch_init (void)
{
  TestObject *obj;

  g_test_summary ("Test that custom dispatch_properties_changed is called "
                  "on initialization");

  dispatch_properties_called = 0;
  obj = g_object_new (test_object_get_type (), "foo", 5, NULL);

  g_assert_false (object_has_notify_signal_handlers (obj));

  g_assert_cmpint (dispatch_properties_called, ==, 1);
  g_object_set (obj, "foo", 11, NULL);
  g_assert_cmpint (dispatch_properties_called, ==, 2);

  g_object_unref (obj);
}

/* This instance init behavior is the thing we are testing:
 *
 * 1. Don't connect any notify handlers
 * 2. Change the the foo property
 * 3. Verify that our custom dispatch_properties_changed is called
 */
static void
test_custom_dispatch_set (void)
{
  TestObject *obj;

  g_test_summary ("Test that custom dispatch_properties_changed is called regardless of connected notify handlers");

  dispatch_properties_called = 0;
  obj = g_object_new (test_object_get_type (), NULL);

  g_assert_false (object_has_notify_signal_handlers (obj));

  g_assert_cmpint (dispatch_properties_called, ==, 0);
  g_object_set (obj, "foo", 11, NULL);
  g_assert_cmpint (dispatch_properties_called, ==, 1);
  g_object_set (obj, "foo", 11, NULL);
  g_assert_cmpint (dispatch_properties_called, ==, 1);

  g_object_unref (obj);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/properties/custom-dispatch/init", test_custom_dispatch_init);
  g_test_add_func ("/properties/custom-dispatch/set", test_custom_dispatch_set);

  return g_test_run ();
}
