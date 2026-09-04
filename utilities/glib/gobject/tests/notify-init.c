/* GLib testing framework examples and tests
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

#include <stdlib.h>
#include <gstdio.h>
#include <glib-object.h>

typedef struct {
  GObject parent_instance;
  gint foo;
  gboolean bar;
  gchar *baz;
  gchar *quux;
} TestObject;

typedef struct {
  GObjectClass parent_class;
} TestObjectClass;

typedef enum {
  PROP_FOO = 1,
  PROP_BAR,
  PROP_BAZ,
  PROP_QUUX,
  N_PROPERTIES
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
  TestObject *self = (TestObject *) gobject;

  g_free (self->baz);
  g_free (self->quux);

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

  switch ((TestObjectProperty)prop_id)
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
  g_assert_true (prop_id < N_PROPERTIES && pspec == properties[prop_id]);

  switch ((TestObjectProperty)prop_id)
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
                                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  properties[PROP_BAR] = g_param_spec_boolean ("bar", "Bar", "Bar",
                                               FALSE,
                                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  properties[PROP_BAZ] = g_param_spec_string ("baz", "Baz", "Baz",
                                              NULL,
                                              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  properties[PROP_QUUX] = g_param_spec_string ("quux", "quux", "quux",
                                               NULL,
                                               G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  gobject_class->set_property = test_object_set_property;
  gobject_class->get_property = test_object_get_property;
  gobject_class->finalize = test_object_finalize;

  g_object_class_install_properties (gobject_class, N_PROPERTIES, properties);
}

static void
quux_changed (TestObject *self,
              GParamSpec *pspec,
              gpointer    data)
{
  g_assert (self->baz != NULL);
}

static void
test_object_init (TestObject *self)
{
  /* This instance init behavior is the thing we are testing:
   *
   * 1. Connect to notify::quux
   * 2. Change the the quux property
   * 3. Continue to set up things that the quux_changed handler
   *   relies on
   *
   * The expected behavior is that:
   *
   * - The quux_changed handler *is* called
   * - It is only called after the object is fully constructed
   */
  g_signal_connect (self, "notify::quux", G_CALLBACK (quux_changed), NULL);

  test_object_set_quux (self, "quux");

  self->foo = 42;
  self->bar = TRUE;
  self->baz = g_strdup ("Hello");
}

static void
test_notify_in_init (void)
{
  TestObject *obj;

  g_test_summary ("Test that emitting notify with a handler already connected in test_object_init() works");
  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/2665");

  obj = g_object_new (test_object_get_type (), NULL);

  g_object_unref (obj);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/properties/notify-in-init", test_notify_in_init);

  return g_test_run ();
}
