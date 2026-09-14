/*
 * Copyright 2022 Simon McVittie
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <glib-object.h>
#include <glib.h>

typedef struct
{
  GObject parent;
  int normal;
  int normal_construct;
  int deprecated;
  int deprecated_construct;
} MyObject;

typedef struct
{
  GObjectClass parent;
} MyObjectClass;

typedef enum
{
  PROP_0,
  PROP_NORMAL,
  PROP_NORMAL_CONSTRUCT,
  PROP_DEPRECATED,
  PROP_DEPRECATED_CONSTRUCT,
  N_PROPS
} Property;

static GParamSpec *props[N_PROPS] = { NULL };

static GType my_object_get_type (void);

G_DEFINE_TYPE (MyObject, my_object, G_TYPE_OBJECT);

static void
my_object_init (MyObject *self)
{
}

static void
my_object_set_property (GObject *object,
                        guint prop_id,
                        const GValue *value,
                        GParamSpec *param_spec)
{
  MyObject *self = (MyObject *) object;

  switch ((Property) prop_id)
    {
    case PROP_NORMAL:
      self->normal = g_value_get_int (value);
      break;

    case PROP_NORMAL_CONSTRUCT:
      self->normal_construct = g_value_get_int (value);
      break;

    case PROP_DEPRECATED:
      self->deprecated = g_value_get_int (value);
      break;

    case PROP_DEPRECATED_CONSTRUCT:
      self->deprecated_construct = g_value_get_int (value);
      break;

    case PROP_0:
    case N_PROPS:
    default:
      g_assert_not_reached ();
    }
}

static void
my_object_get_property (GObject *object,
                        guint prop_id,
                        GValue *value,
                        GParamSpec *param_spec)
{
  MyObject *self = (MyObject *) object;

  switch ((Property) prop_id)
    {
    case PROP_NORMAL:
      g_value_set_int (value, self->normal);
      break;

    case PROP_NORMAL_CONSTRUCT:
      g_value_set_int (value, self->normal_construct);
      break;

    case PROP_DEPRECATED:
      g_value_set_int (value, self->deprecated);
      break;

    case PROP_DEPRECATED_CONSTRUCT:
      g_value_set_int (value, self->deprecated_construct);
      break;

    case PROP_0:
    case N_PROPS:
    default:
      g_assert_not_reached ();
    }
}

static void
my_object_class_init (MyObjectClass *cls)
{
  GObjectClass *object_class = G_OBJECT_CLASS (cls);

  props[PROP_NORMAL] = g_param_spec_int ("normal", NULL, NULL,
                                         G_MININT, G_MAXINT, -1,
                                         (G_PARAM_READWRITE |
                                          G_PARAM_STATIC_STRINGS));
  props[PROP_NORMAL_CONSTRUCT] = g_param_spec_int ("normal-construct", NULL, NULL,
                                                   G_MININT, G_MAXINT, -1,
                                                   (G_PARAM_READWRITE |
                                                    G_PARAM_STATIC_STRINGS |
                                                    G_PARAM_CONSTRUCT));
  props[PROP_DEPRECATED] = g_param_spec_int ("deprecated", NULL, NULL,
                                             G_MININT, G_MAXINT, -1,
                                             (G_PARAM_READWRITE |
                                              G_PARAM_STATIC_STRINGS |
                                              G_PARAM_DEPRECATED));
  props[PROP_DEPRECATED_CONSTRUCT] = g_param_spec_int ("deprecated-construct", NULL, NULL,
                                                       G_MININT, G_MAXINT, -1,
                                                       (G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_DEPRECATED));
  object_class->get_property = my_object_get_property;
  object_class->set_property = my_object_set_property;
  g_object_class_install_properties (object_class, N_PROPS, props);
}

static void
test_construct (void)
{
  if (g_test_subprocess ())
    {
      MyObject *o;

      /* Don't crash on deprecation warnings, so we can see all of them */
      g_log_set_always_fatal (G_LOG_FATAL_MASK);
      g_log_set_fatal_mask ("GLib-GObject", G_LOG_FATAL_MASK);

      o = g_object_new (my_object_get_type (),
                        "normal", 1,
                        "normal-construct", 2,
                        "deprecated", 3,
                        "deprecated-construct", 4,
                        NULL);
      g_printerr ("Constructed object");
      g_assert_cmpint (o->normal, ==, 1);
      g_assert_cmpint (o->normal_construct, ==, 2);
      g_assert_cmpint (o->deprecated, ==, 3);
      g_assert_cmpint (o->deprecated_construct, ==, 4);
      g_clear_object (&o);
      return;
    }

  g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_stderr ("*The property MyObject:deprecated-construct is deprecated*");
  g_test_trap_assert_stderr ("*The property MyObject:deprecated is deprecated*");
  g_test_trap_assert_stderr_unmatched ("*The property MyObject:normal*");
  g_test_trap_assert_passed ();
}

static void
test_def_construct (void)
{
  if (g_test_subprocess ())
    {
      MyObject *o;

      /* Don't crash on deprecation warnings, so we can see all of them */
      g_log_set_always_fatal (G_LOG_FATAL_MASK);
      g_log_set_fatal_mask ("GLib-GObject", G_LOG_FATAL_MASK);

      o = g_object_new (my_object_get_type (),
                        NULL);
      g_printerr ("Constructed object");
      g_assert_cmpint (o->normal, ==, 0);
      g_assert_cmpint (o->normal_construct, ==, -1);
      g_assert_cmpint (o->deprecated, ==, 0);
      g_assert_cmpint (o->deprecated_construct, ==, -1);
      g_clear_object (&o);
      return;
    }

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/2748");
  g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_stderr_unmatched ("*The property MyObject:deprecated*");
  g_test_trap_assert_stderr_unmatched ("*The property MyObject:normal*");
  g_test_trap_assert_passed ();
}

static void
test_set (void)
{
  if (g_test_subprocess ())
    {
      MyObject *o;

      /* Don't crash on deprecation warnings, so we can see all of them */
      g_log_set_always_fatal (G_LOG_FATAL_MASK);
      g_log_set_fatal_mask ("GLib-GObject", G_LOG_FATAL_MASK);

      o = g_object_new (my_object_get_type (),
                        NULL);
      g_printerr ("Constructed object");
      g_assert_cmpint (o->normal, ==, 0);
      g_assert_cmpint (o->normal_construct, ==, -1);
      g_assert_cmpint (o->deprecated, ==, 0);
      g_assert_cmpint (o->deprecated_construct, ==, -1);

      g_object_set (o,
                    "normal", 1,
                    "normal-construct", 2,
                    "deprecated", 3,
                    "deprecated-construct", 4,
                    NULL);
      g_printerr ("Set properties");
      g_assert_cmpint (o->normal, ==, 1);
      g_assert_cmpint (o->normal_construct, ==, 2);
      g_assert_cmpint (o->deprecated, ==, 3);
      g_assert_cmpint (o->deprecated_construct, ==, 4);

      g_clear_object (&o);
      return;
    }

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/2748");
  g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_stderr ("*The property MyObject:deprecated-construct is deprecated*");
  g_test_trap_assert_stderr ("*The property MyObject:deprecated is deprecated*");
  g_test_trap_assert_stderr_unmatched ("*The property MyObject:normal*");
  g_test_trap_assert_passed ();
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_setenv ("G_ENABLE_DIAGNOSTIC", "1", TRUE);

  g_test_set_nonfatal_assertions ();
  g_test_add_func ("/deprecated-properties/construct", test_construct);
  g_test_add_func ("/deprecated-properties/default-construct", test_def_construct);
  g_test_add_func ("/deprecated-properties/set", test_set);
  return g_test_run ();
}
