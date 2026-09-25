/* GObject - GLib Type, Object, Parameter and Signal Library
 *
 * Copyright (C) 2015-2022 Christian Hergert <christian@hergert.me>
 * Copyright (C) 2015 Garrett Regier <garrettregier@gmail.com>
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
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <glib-object.h>

/* Copied from glib */
typedef struct _BindingSource
{
  GObject parent_instance;

  gint foo;
  gint bar;
  gdouble value;
  gboolean toggle;
} BindingSource;

typedef struct _BindingSourceClass
{
  GObjectClass parent_class;
} BindingSourceClass;

enum
{
  PROP_SOURCE_FOO = 1,
  PROP_SOURCE_BAR,
  PROP_SOURCE_VALUE,
  PROP_SOURCE_TOGGLE
};

static GType binding_source_get_type (void);
G_DEFINE_TYPE (BindingSource, binding_source, G_TYPE_OBJECT);

static void
binding_source_set_property (GObject      *gobject,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  BindingSource *source = (BindingSource *) gobject;

  switch (prop_id)
    {
    case PROP_SOURCE_FOO:
      source->foo = g_value_get_int (value);
      break;

    case PROP_SOURCE_BAR:
      source->bar = g_value_get_int (value);
      break;

    case PROP_SOURCE_VALUE:
      source->value = g_value_get_double (value);
      break;

    case PROP_SOURCE_TOGGLE:
      source->toggle = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
binding_source_get_property (GObject    *gobject,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  BindingSource *source = (BindingSource *) gobject;

  switch (prop_id)
    {
    case PROP_SOURCE_FOO:
      g_value_set_int (value, source->foo);
      break;

    case PROP_SOURCE_BAR:
      g_value_set_int (value, source->bar);
      break;

    case PROP_SOURCE_VALUE:
      g_value_set_double (value, source->value);
      break;

    case PROP_SOURCE_TOGGLE:
      g_value_set_boolean (value, source->toggle);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
binding_source_class_init (BindingSourceClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = binding_source_set_property;
  gobject_class->get_property = binding_source_get_property;

  g_object_class_install_property (gobject_class, PROP_SOURCE_FOO,
                                   g_param_spec_int ("foo", "Foo", "Foo",
                                                     -1, 100,
                                                     0,
                                                     G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_SOURCE_BAR,
                                   g_param_spec_int ("bar", "Bar", "Bar",
                                                     -1, 100,
                                                     0,
                                                     G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_SOURCE_VALUE,
                                   g_param_spec_double ("value", "Value", "Value",
                                                        -100.0, 200.0,
                                                        0.0,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_SOURCE_TOGGLE,
                                   g_param_spec_boolean ("toggle", "Toggle", "Toggle",
                                                         FALSE,
                                                         G_PARAM_READWRITE));
}

static void
binding_source_init (BindingSource *self)
{
}

typedef struct _BindingTarget
{
  GObject parent_instance;

  gint bar;
  gdouble value;
  gboolean toggle;
} BindingTarget;

typedef struct _BindingTargetClass
{
  GObjectClass parent_class;
} BindingTargetClass;

enum
{
  PROP_TARGET_BAR = 1,
  PROP_TARGET_VALUE,
  PROP_TARGET_TOGGLE
};

static GType binding_target_get_type (void);
G_DEFINE_TYPE (BindingTarget, binding_target, G_TYPE_OBJECT);

static void
binding_target_set_property (GObject      *gobject,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  BindingTarget *target = (BindingTarget *) gobject;

  switch (prop_id)
    {
    case PROP_TARGET_BAR:
      target->bar = g_value_get_int (value);
      break;

    case PROP_TARGET_VALUE:
      target->value = g_value_get_double (value);
      break;

    case PROP_TARGET_TOGGLE:
      target->toggle = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
binding_target_get_property (GObject    *gobject,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  BindingTarget *target = (BindingTarget *) gobject;

  switch (prop_id)
    {
    case PROP_TARGET_BAR:
      g_value_set_int (value, target->bar);
      break;

    case PROP_TARGET_VALUE:
      g_value_set_double (value, target->value);
      break;

    case PROP_TARGET_TOGGLE:
      g_value_set_boolean (value, target->toggle);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
binding_target_class_init (BindingTargetClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = binding_target_set_property;
  gobject_class->get_property = binding_target_get_property;

  g_object_class_install_property (gobject_class, PROP_TARGET_BAR,
                                   g_param_spec_int ("bar", "Bar", "Bar",
                                                     -1, 100,
                                                     0,
                                                     G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_TARGET_VALUE,
                                   g_param_spec_double ("value", "Value", "Value",
                                                        -100.0, 200.0,
                                                        0.0,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_TARGET_TOGGLE,
                                   g_param_spec_boolean ("toggle", "Toggle", "Toggle",
                                                         FALSE,
                                                         G_PARAM_READWRITE));
}

static void
binding_target_init (BindingTarget *self)
{
}

static gboolean
celsius_to_fahrenheit (GBinding     *binding,
                       const GValue *from_value,
                       GValue       *to_value,
                       gpointer      user_data G_GNUC_UNUSED)
{
  gdouble celsius, fahrenheit;

  g_assert_true (G_VALUE_HOLDS (from_value, G_TYPE_DOUBLE));
  g_assert_true (G_VALUE_HOLDS (to_value, G_TYPE_DOUBLE));

  celsius = g_value_get_double (from_value);
  fahrenheit = (9 * celsius / 5) + 32.0;

  if (g_test_verbose ())
    g_printerr ("Converting %.2fC to %.2fF\n", celsius, fahrenheit);

  g_value_set_double (to_value, fahrenheit);

  return TRUE;
}

static gboolean
fahrenheit_to_celsius (GBinding     *binding,
                       const GValue *from_value,
                       GValue       *to_value,
                       gpointer      user_data G_GNUC_UNUSED)
{
  gdouble celsius, fahrenheit;

  g_assert_true (G_VALUE_HOLDS (from_value, G_TYPE_DOUBLE));
  g_assert_true (G_VALUE_HOLDS (to_value, G_TYPE_DOUBLE));

  fahrenheit = g_value_get_double (from_value);
  celsius = 5 * (fahrenheit - 32.0) / 9;

  if (g_test_verbose ())
    g_printerr ("Converting %.2fF to %.2fC\n", fahrenheit, celsius);

  g_value_set_double (to_value, celsius);

  return TRUE;
}

static void
test_binding_group_invalid (void)
{
  GBindingGroup *group = g_binding_group_new ();
  BindingSource *source = g_object_new (binding_source_get_type (), NULL);
  BindingTarget *target = g_object_new (binding_target_get_type (), NULL);

  /* Invalid Target Property */
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*find_property*target_property*!=*NULL*");
  g_binding_group_bind (group, "value",
                        target, "does-not-exist",
                        G_BINDING_DEFAULT);
  g_test_assert_expected_messages ();

  g_binding_group_set_source (group, NULL);

  /* Invalid Source Property */
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*find_property*source_property*!=*NULL*");
  g_binding_group_set_source (group, source);
  g_binding_group_bind (group, "does-not-exist",
                        target, "value",
                        G_BINDING_DEFAULT);
  g_test_assert_expected_messages ();

  g_binding_group_set_source (group, NULL);

  /* Invalid Source */
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*find_property*->source_property*!=*NULL*");
  g_binding_group_bind (group, "does-not-exist",
                        target, "value",
                        G_BINDING_DEFAULT);
  g_binding_group_set_source (group, source);
  g_test_assert_expected_messages ();

  g_object_unref (target);
  g_object_unref (source);
  g_object_unref (group);
}

static void
test_binding_group_default (void)
{
  gsize i, j;
  GBindingGroup *group = g_binding_group_new ();
  BindingSource *source = g_object_new (binding_source_get_type (), NULL);
  BindingTarget *targets[5];
  BindingSource *readback;

  for (i = 0; i < G_N_ELEMENTS (targets); ++i)
    {
      targets[i] = g_object_new (binding_target_get_type (), NULL);
      g_binding_group_bind (group, "foo",
                            targets[i], "bar",
                            G_BINDING_DEFAULT);
    }

  g_assert_null (g_binding_group_dup_source (group));
  g_binding_group_set_source (group, source);
  readback = g_binding_group_dup_source (group);
  g_assert_true (readback == source);
  g_object_unref (readback);

  for (i = 0; i < 2; ++i)
    {
      g_object_set (source, "foo", 42, NULL);
      for (j = 0; j < G_N_ELEMENTS (targets); ++j)
        g_assert_cmpint (source->foo, ==, targets[j]->bar);

      g_object_set (targets[0], "bar", 47, NULL);
      g_assert_cmpint (source->foo, !=, targets[0]->bar);

      /* Check that we transition the source correctly */
      g_binding_group_set_source (group, NULL);
      g_assert_null (g_binding_group_dup_source (group));
      g_binding_group_set_source (group, source);
      readback = g_binding_group_dup_source (group);
      g_assert_true (readback == source);
      g_object_unref (readback);
    }

  g_object_unref (group);

  g_object_set (source, "foo", 0, NULL);
  for (i = 0; i < G_N_ELEMENTS (targets); ++i)
    g_assert_cmpint (source->foo, !=, targets[i]->bar);

  g_object_unref (source);
  for (i = 0; i < G_N_ELEMENTS (targets); ++i)
    g_object_unref (targets[i]);
}

static void
test_binding_group_bidirectional (void)
{
  gsize i, j;
  GBindingGroup *group = g_binding_group_new ();
  BindingSource *source = g_object_new (binding_source_get_type (), NULL);
  BindingTarget *targets[5];
  BindingSource *readback;

  for (i = 0; i < G_N_ELEMENTS (targets); ++i)
    {
      targets[i] = g_object_new (binding_target_get_type (), NULL);
      g_binding_group_bind (group, "value",
                            targets[i], "value",
                            G_BINDING_BIDIRECTIONAL);
    }

  g_assert_null (g_binding_group_dup_source (group));
  g_binding_group_set_source (group, source);
  readback = g_binding_group_dup_source (group);
  g_assert_true (readback == source);
  g_object_unref (readback);

  for (i = 0; i < 2; ++i)
    {
      g_object_set (source, "value", 42.0, NULL);
      for (j = 0; j < G_N_ELEMENTS (targets); ++j)
        g_assert_cmpfloat (source->value, ==, targets[j]->value);

      g_object_set (targets[0], "value", 47.0, NULL);
      g_assert_cmpfloat (source->value, ==, targets[0]->value);

      /* Check that we transition the source correctly */
      g_binding_group_set_source (group, NULL);
      g_assert_null (g_binding_group_dup_source (group));
      g_binding_group_set_source (group, source);
      readback = g_binding_group_dup_source (group);
      g_assert_true (readback == source);
      g_object_unref (readback);
    }

  g_object_unref (group);

  g_object_set (targets[0], "value", 0.0, NULL);
  g_assert_cmpfloat (source->value, !=, targets[0]->value);

  g_object_unref (source);
  for (i = 0; i < G_N_ELEMENTS (targets); ++i)
    g_object_unref (targets[i]);
}

static void
transform_destroy_notify (gpointer data)
{
  gboolean *transform_destroy_called = data;

  *transform_destroy_called = TRUE;
}

static void
test_binding_group_transform (void)
{
  gboolean transform_destroy_called = FALSE;
  GBindingGroup *group = g_binding_group_new ();
  BindingSource *source = g_object_new (binding_source_get_type (), NULL);
  BindingTarget *target = g_object_new (binding_target_get_type (), NULL);

  g_binding_group_set_source (group, source);
  g_binding_group_bind_full (group, "value",
                             target, "value",
                             G_BINDING_BIDIRECTIONAL,
                             celsius_to_fahrenheit,
                             fahrenheit_to_celsius,
                             &transform_destroy_called,
                             transform_destroy_notify);

  g_object_set (source, "value", 24.0, NULL);
  g_assert_cmpfloat (target->value, ==, ((9 * 24.0 / 5) + 32.0));

  g_object_set (target, "value", 69.0, NULL);
  g_assert_cmpfloat (source->value, ==, (5 * (69.0 - 32.0) / 9));

  /* The GDestroyNotify should only be called when the
   * set is freed, not when the various GBindings are freed
   */
  g_binding_group_set_source (group, NULL);
  g_assert_false (transform_destroy_called);

  g_object_unref (group);
  g_assert_true (transform_destroy_called);

  g_object_unref (source);
  g_object_unref (target);
}

static void
test_binding_group_transform_closures (void)
{
  gboolean transform_destroy_called_1 = FALSE;
  gboolean transform_destroy_called_2 = FALSE;
  GBindingGroup *group = g_binding_group_new ();
  BindingSource *source = g_object_new (binding_source_get_type (), NULL);
  BindingTarget *target = g_object_new (binding_target_get_type (), NULL);
  GClosure *c2f_closure, *f2c_closure;

  c2f_closure = g_cclosure_new (G_CALLBACK (celsius_to_fahrenheit),
                                &transform_destroy_called_1,
                                (GClosureNotify) transform_destroy_notify);
  f2c_closure = g_cclosure_new (G_CALLBACK (fahrenheit_to_celsius),
                                &transform_destroy_called_2,
                                (GClosureNotify) transform_destroy_notify);

  g_binding_group_set_source (group, source);
  g_binding_group_bind_with_closures (group, "value",
                                      target, "value",
                                      G_BINDING_BIDIRECTIONAL,
                                      c2f_closure,
                                      f2c_closure);

  g_object_set (source, "value", 24.0, NULL);
  g_assert_cmpfloat (target->value, ==, ((9 * 24.0 / 5) + 32.0));

  g_object_set (target, "value", 69.0, NULL);
  g_assert_cmpfloat (source->value, ==, (5 * (69.0 - 32.0) / 9));

  /* The GClsoureNotify should only be called when the
   * set is freed, not when the various GBindings are freed
   */
  g_binding_group_set_source (group, NULL);
  g_assert_false (transform_destroy_called_1);
  g_assert_false (transform_destroy_called_2);

  g_object_unref (group);
  g_assert_true (transform_destroy_called_1);
  g_assert_true (transform_destroy_called_2);

  g_object_unref (source);
  g_object_unref (target);
}

static void
test_binding_group_same_object (void)
{
  gsize i;
  GBindingGroup *group = g_binding_group_new ();
  BindingSource *source = g_object_new (binding_source_get_type (),
                                        "foo", 100,
                                        "bar", 50,
                                        NULL);

  g_binding_group_set_source (group, source);
  g_binding_group_bind (group, "foo",
                        source, "bar",
                        G_BINDING_BIDIRECTIONAL);

  for (i = 0; i < 2; ++i)
    {
      g_object_set (source, "foo", 10, NULL);
      g_assert_cmpint (source->foo, ==, 10);
      g_assert_cmpint (source->bar, ==, 10);

      g_object_set (source, "bar", 30, NULL);
      g_assert_cmpint (source->foo, ==, 30);
      g_assert_cmpint (source->bar, ==, 30);

      /* Check that it is possible both when initially
       * adding the binding and when changing the source
       */
      g_binding_group_set_source (group, NULL);
      g_binding_group_set_source (group, source);
    }

  g_object_unref (source);
  g_object_unref (group);
}

static void
test_binding_group_weak_ref_source (void)
{
  GBindingGroup *group = g_binding_group_new ();
  BindingSource *source = g_object_new (binding_source_get_type (), NULL);
  BindingTarget *target = g_object_new (binding_target_get_type (), NULL);
  BindingSource *readback;

  g_binding_group_set_source (group, source);
  g_binding_group_bind (group, "value",
                        target, "value",
                        G_BINDING_BIDIRECTIONAL);

  g_object_add_weak_pointer (G_OBJECT (source), (gpointer)&source);
  readback = g_binding_group_dup_source (group);
  g_assert_true (readback == source);
  g_object_unref (readback);
  g_object_unref (source);
  g_assert_null (source);
  g_assert_null (g_binding_group_dup_source (group));

  /* Hopefully this would explode if the binding was still alive */
  g_object_set (target, "value", 0.0, NULL);

  g_object_unref (target);
  g_object_unref (group);
}

static void
test_binding_group_weak_ref_target (void)
{
  GBindingGroup *group = g_binding_group_new ();
  BindingSource *source = g_object_new (binding_source_get_type (), NULL);
  BindingTarget *target = g_object_new (binding_target_get_type (), NULL);

  g_binding_group_set_source (group, source);
  g_binding_group_bind (group, "value",
                        target, "value",
                        G_BINDING_BIDIRECTIONAL);

  g_object_set (source, "value", 47.0, NULL);
  g_assert_cmpfloat (target->value, ==, 47.0);

  g_object_add_weak_pointer (G_OBJECT (target), (gpointer)&target);
  g_object_unref (target);
  g_assert_null (target);

  /* Hopefully this would explode if the binding was still alive */
  g_object_set (source, "value", 0.0, NULL);

  g_object_unref (source);
  g_object_unref (group);
}

static void
test_binding_group_properties (void)
{
  GBindingGroup *group = g_binding_group_new ();
  BindingSource *source = g_object_new (binding_source_get_type (), NULL);
  BindingTarget *target = g_object_new (binding_target_get_type (), NULL);
  BindingSource *other;

  g_binding_group_set_source (group, source);
  g_binding_group_bind (group, "value",
                        target, "value",
                        G_BINDING_BIDIRECTIONAL);

  g_object_get (group, "source", &other, NULL);
  g_assert_true (other == source);
  g_object_unref (other);

  g_object_set (group, "source", NULL, NULL);
  g_object_get (group, "source", &other, NULL);
  g_assert_null (other);

  g_object_add_weak_pointer (G_OBJECT (target), (gpointer)&target);
  g_object_unref (target);
  g_assert_null (target);

  g_object_unref (source);
  g_object_unref (group);
}

static void
test_binding_group_weak_notify_no_bindings (void)
{
  GBindingGroup *group = g_binding_group_new ();
  BindingSource *source = g_object_new (binding_source_get_type (), NULL);

  g_binding_group_set_source (group, source);
  g_assert_finalize_object (source);
  g_assert_finalize_object (group);
}

static void
test_binding_group_empty_closures (void)
{
  GBindingGroup *group = g_binding_group_new ();
  BindingSource *source = g_object_new (binding_source_get_type (), NULL);
  BindingTarget *target = g_object_new (binding_target_get_type (), NULL);

  g_binding_group_bind_full (group, "value", target, "value", 0,
                             NULL, NULL, NULL, NULL);

  g_assert_finalize_object (group);
  g_assert_finalize_object (target);
  g_assert_finalize_object (source);
}

gint
main (gint   argc,
      gchar *argv[])
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/GObject/BindingGroup/invalid", test_binding_group_invalid);
  g_test_add_func ("/GObject/BindingGroup/default", test_binding_group_default);
  g_test_add_func ("/GObject/BindingGroup/bidirectional", test_binding_group_bidirectional);
  g_test_add_func ("/GObject/BindingGroup/transform", test_binding_group_transform);
  g_test_add_func ("/GObject/BindingGroup/transform-closures", test_binding_group_transform_closures);
  g_test_add_func ("/GObject/BindingGroup/same-object", test_binding_group_same_object);
  g_test_add_func ("/GObject/BindingGroup/weak-ref-source", test_binding_group_weak_ref_source);
  g_test_add_func ("/GObject/BindingGroup/weak-ref-target", test_binding_group_weak_ref_target);
  g_test_add_func ("/GObject/BindingGroup/properties", test_binding_group_properties);
  g_test_add_func ("/GObject/BindingGroup/weak-notify-no-bindings", test_binding_group_weak_notify_no_bindings);
  g_test_add_func ("/GObject/BindingGroup/empty-closures", test_binding_group_empty_closures);
  return g_test_run ();
}
