/* GObject - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 2001, 2003 Red Hat, Inc.
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
 */

#include <glib-object.h>

#include "marshalers.h"
#include "testcommon.h"

/* What this test tests is the behavior of signal accumulators
 * Two accumulators are tested:
 *
 * 1: A custom accumulator that appends the returned strings
 * 2: The standard g_signal_accumulator_true_handled that stops
 *    emission on TRUE returns.
 */

/* TestObject, a parent class for TestObject */
#define TEST_TYPE_OBJECT          (test_object_get_type ())
typedef struct _TestObject        TestObject;
typedef struct _TestObjectClass   TestObjectClass;

struct _TestObject
{
  GObject parent_instance;
};
struct _TestObjectClass
{
  GObjectClass parent_class;

  gchar*   (*test_signal1) (TestObject *tobject,
                            gint        param);
  gboolean (*test_signal2) (TestObject *tobject,
                            gint        param);
  GVariant* (*test_signal3) (TestObject *tobject,
                             gboolean *weak_ptr);
};

static GType test_object_get_type (void);

static gboolean
test_signal1_accumulator (GSignalInvocationHint *ihint,
                          GValue                *return_accu,
                          const GValue          *handler_return,
                          gpointer               data)
{
  const gchar *accu_string = g_value_get_string (return_accu);
  const gchar *new_string = g_value_get_string (handler_return);
  gchar *result_string;

  if (accu_string)
    result_string = g_strconcat (accu_string, new_string, NULL);
  else if (new_string)
    result_string = g_strdup (new_string);
  else
    result_string = NULL;

  g_value_take_string (return_accu, result_string);

  return TRUE;
}

static gchar *
test_object_signal1_callback_before (TestObject *tobject,
                                     gint        param,
                                     gpointer    data)
{
  return g_strdup ("<before>");
}

static gchar *
test_object_real_signal1 (TestObject *tobject,
                          gint        param)
{
  return g_strdup ("<default>");
}

static gchar *
test_object_signal1_callback_after (TestObject *tobject,
                                    gint        param,
                                    gpointer    data)
{
  return g_strdup ("<after>");
}

static gboolean
test_object_signal2_callback_before (TestObject *tobject,
                                     gint        param)
{
  switch (param)
    {
    case 1: return TRUE;
    case 2: return FALSE;
    case 3: return FALSE;
    case 4: return FALSE;
    }

  g_assert_not_reached ();
  return FALSE;
}

static gboolean
test_object_real_signal2 (TestObject *tobject,
                          gint        param)
{
  switch (param)
    {
    case 1: g_assert_not_reached (); return FALSE;
    case 2: return TRUE;
    case 3: return FALSE;
    case 4: return FALSE;
    }

  g_assert_not_reached ();
  return FALSE;
}

static gboolean
test_object_signal2_callback_after (TestObject *tobject,
                                    gint        param)
{
  switch (param)
    {
    case 1: g_assert_not_reached (); return FALSE;
    case 2: g_assert_not_reached (); return FALSE;
    case 3: return TRUE;
    case 4: return FALSE;
    }

  g_assert_not_reached ();
  return FALSE;
}

static gboolean
test_signal3_accumulator (GSignalInvocationHint *ihint,
                          GValue                *return_accu,
                          const GValue          *handler_return,
                          gpointer               data)
{
  GVariant *variant;

  variant = g_value_get_variant (handler_return);
  g_assert_false (g_variant_is_floating (variant));

  g_value_set_variant (return_accu, variant);

  return variant == NULL;
}

/* To be notified when the variant is finalised, we construct
 * it from data with a custom GDestroyNotify.
 */

typedef struct {
  char *mem;
  gsize n;
  gboolean *weak_ptr;
} VariantData;

static void
free_data (VariantData *data)
{
  *(data->weak_ptr) = TRUE;
  g_free (data->mem);
  g_slice_free (VariantData, data);
}

static GVariant *
test_object_real_signal3 (TestObject *tobject,
                          gboolean *weak_ptr)
{
  GVariant *variant;
  VariantData *data;

  variant = g_variant_ref_sink (g_variant_new_uint32 (42));
  data = g_slice_new (VariantData);
  data->weak_ptr = weak_ptr;
  data->n = g_variant_get_size (variant);
  data->mem = g_malloc (data->n);
  g_variant_store (variant, data->mem);
  g_variant_unref (variant);

  variant = g_variant_new_from_data (G_VARIANT_TYPE ("u"),
                                     data->mem,
                                     data->n,
                                     TRUE,
                                     (GDestroyNotify) free_data,
                                     data);
  return g_variant_ref_sink (variant);
}

static void
test_object_class_init (TestObjectClass *class)
{
  class->test_signal1 = test_object_real_signal1;
  class->test_signal2 = test_object_real_signal2;
  class->test_signal3 = test_object_real_signal3;

  g_signal_new ("test-signal1",
                G_OBJECT_CLASS_TYPE (class),
                G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET (TestObjectClass, test_signal1),
                test_signal1_accumulator, NULL,
                test_STRING__INT,
                G_TYPE_STRING, 1, G_TYPE_INT);
  g_signal_new ("test-signal2",
                G_OBJECT_CLASS_TYPE (class),
                G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET (TestObjectClass, test_signal2),
                g_signal_accumulator_true_handled, NULL,
                test_BOOLEAN__INT,
                G_TYPE_BOOLEAN, 1, G_TYPE_INT);
  g_signal_new ("test-signal3",
                G_OBJECT_CLASS_TYPE (class),
                G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET (TestObjectClass, test_signal3),
                test_signal3_accumulator, NULL,
                test_VARIANT__POINTER,
                G_TYPE_VARIANT, 1, G_TYPE_POINTER);
}

static DEFINE_TYPE(TestObject, test_object,
                   test_object_class_init, NULL, NULL,
                   G_TYPE_OBJECT);

static void
test_accumulator (void)
{
  TestObject *object;
  gchar *string_result;
  gboolean bool_result;
  gboolean variant_finalised;
  GVariant *variant_result;

  object = g_object_new (TEST_TYPE_OBJECT, NULL);

  g_signal_connect (object, "test-signal1",
                    G_CALLBACK (test_object_signal1_callback_before), NULL);
  g_signal_connect_after (object, "test-signal1",
                          G_CALLBACK (test_object_signal1_callback_after), NULL);

  g_signal_emit_by_name (object, "test-signal1", 0, &string_result);
  g_assert_cmpstr (string_result, ==, "<before><default><after>");
  g_free (string_result);

  g_signal_connect (object, "test-signal2",
                    G_CALLBACK (test_object_signal2_callback_before), NULL);
  g_signal_connect_after (object, "test-signal2",
                          G_CALLBACK (test_object_signal2_callback_after), NULL);

  bool_result = FALSE;
  g_signal_emit_by_name (object, "test-signal2", 1, &bool_result);
  g_assert_true (bool_result);
  bool_result = FALSE;
  g_signal_emit_by_name (object, "test-signal2", 2, &bool_result);
  g_assert_true (bool_result);
  bool_result = FALSE;
  g_signal_emit_by_name (object, "test-signal2", 3, &bool_result);
  g_assert_true (bool_result);
  bool_result = TRUE;
  g_signal_emit_by_name (object, "test-signal2", 4, &bool_result);
  g_assert_false (bool_result);

  variant_finalised = FALSE;
  variant_result = NULL;
  g_signal_emit_by_name (object, "test-signal3", &variant_finalised, &variant_result);
  g_assert_nonnull (variant_result);
  g_assert_false (g_variant_is_floating (variant_result));

  /* Test that variant_result had refcount 1 */
  g_assert_false (variant_finalised);
  g_variant_unref (variant_result);
  g_assert_true (variant_finalised);

  g_object_unref (object);
}

int
main (int  argc,
      char *argv[])
{
  g_log_set_always_fatal (g_log_set_always_fatal (G_LOG_FATAL_MASK) |
                          G_LOG_LEVEL_WARNING |
                          G_LOG_LEVEL_CRITICAL);

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/gobject/accumulator", test_accumulator);

  return g_test_run ();
}
