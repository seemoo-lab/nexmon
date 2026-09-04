/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

#define GLIB_VERSION_MIN_REQUIRED       GLIB_VERSION_2_30

#include <glib.h>
#include <glib-object.h>
#include "gobject/gvaluecollector.h"

static void
test_enum_transformation (void)
{
  GType type;
  GValue orig = G_VALUE_INIT;
  GValue xform = G_VALUE_INIT;
  GEnumValue values[] = { {0,"0","0"}, {1,"1","1"}};

 type = g_enum_register_static ("TestEnum", values);

 g_value_init (&orig, type);
 g_value_set_enum (&orig, 1);

 memset (&xform, 0, sizeof (GValue));
 g_value_init (&xform, G_TYPE_CHAR);
 g_value_transform (&orig, &xform);
 g_assert_cmpint (g_value_get_char (&xform), ==, 1);
 g_assert_cmpint (g_value_get_schar (&xform), ==, 1);

 memset (&xform, 0, sizeof (GValue));
 g_value_init (&xform, G_TYPE_UCHAR);
 g_value_transform (&orig, &xform);
 g_assert_cmpint (g_value_get_uchar (&xform), ==, 1);

 memset (&xform, 0, sizeof (GValue));
 g_value_init (&xform, G_TYPE_INT);
 g_value_transform (&orig, &xform);
 g_assert_cmpint (g_value_get_int (&xform), ==, 1);

 memset (&xform, 0, sizeof (GValue));
 g_value_init (&xform, G_TYPE_UINT);
 g_value_transform (&orig, &xform);
 g_assert_cmpint (g_value_get_uint (&xform), ==, 1);

 memset (&xform, 0, sizeof (GValue));
 g_value_init (&xform, G_TYPE_LONG);
 g_value_transform (&orig, &xform);
 g_assert_cmpint (g_value_get_long (&xform), ==, 1);

 memset (&xform, 0, sizeof (GValue));
 g_value_init (&xform, G_TYPE_ULONG);
 g_value_transform (&orig, &xform);
 g_assert_cmpuint (g_value_get_ulong (&xform), ==, 1);

 memset (&xform, 0, sizeof (GValue));
 g_value_init (&xform, G_TYPE_INT64);
 g_value_transform (&orig, &xform);
 g_assert_cmpint (g_value_get_int64 (&xform), ==, 1);

 memset (&xform, 0, sizeof (GValue));
 g_value_init (&xform, G_TYPE_UINT64);
 g_value_transform (&orig, &xform);
 g_assert_cmpuint (g_value_get_uint64 (&xform), ==, 1);
}


static void
test_gtype_value (void)
{
  GType type;
  GValue value = G_VALUE_INIT;
  GValue copy = G_VALUE_INIT;

  g_value_init (&value, G_TYPE_GTYPE);

  g_value_set_gtype (&value, G_TYPE_BOXED);
  type = g_value_get_gtype (&value);
  g_assert_true (type == G_TYPE_BOXED);

  g_value_init (&copy, G_TYPE_GTYPE);
  g_value_copy (&value, &copy);
  type = g_value_get_gtype (&copy);
  g_assert_true (type == G_TYPE_BOXED);
}

static gchar *
collect (GValue *value, ...)
{
  gchar *error;
  va_list var_args;

  error = NULL;

  va_start (var_args, value);
  G_VALUE_COLLECT (value, var_args, 0, &error);
  va_end (var_args);

  return error;
}

static gchar *
lcopy (GValue *value, ...)
{
  gchar *error;
  va_list var_args;

  error = NULL;

  va_start (var_args, value);
  G_VALUE_LCOPY (value, var_args, 0, &error);
  va_end (var_args);

  return error;
}

static void
test_collection (void)
{
  GValue value = G_VALUE_INIT;
  gchar *error;

  g_value_init (&value, G_TYPE_CHAR);
  error = collect (&value, 'c');
  g_assert_null (error);
  g_assert_cmpint (g_value_get_char (&value), ==, 'c');

  g_value_unset (&value);
  g_value_init (&value, G_TYPE_UCHAR);
  error = collect (&value, 129);
  g_assert_null (error);
  g_assert_cmpint (g_value_get_uchar (&value), ==, 129);

  g_value_unset (&value);
  g_value_init (&value, G_TYPE_BOOLEAN);
  error = collect (&value, TRUE);
  g_assert_null (error);
  g_assert_cmpint (g_value_get_boolean (&value), ==, TRUE);

  g_value_unset (&value);
  g_value_init (&value, G_TYPE_INT);
  error = collect (&value, G_MAXINT);
  g_assert_null (error);
  g_assert_cmpint (g_value_get_int (&value), ==, G_MAXINT);

  g_value_unset (&value);
  g_value_init (&value, G_TYPE_UINT);
  error = collect (&value, G_MAXUINT);
  g_assert_null (error);
  g_assert_cmpuint (g_value_get_uint (&value), ==, G_MAXUINT);

  g_value_unset (&value);
  g_value_init (&value, G_TYPE_LONG);
  error = collect (&value, G_MAXLONG);
  g_assert_null (error);
  g_assert_cmpint (g_value_get_long (&value), ==, G_MAXLONG);

  g_value_unset (&value);
  g_value_init (&value, G_TYPE_ULONG);
  error = collect (&value, G_MAXULONG);
  g_assert_null (error);
  g_assert_cmpuint (g_value_get_ulong (&value), ==, G_MAXULONG);

  g_value_unset (&value);
  g_value_init (&value, G_TYPE_INT64);
  error = collect (&value, G_MAXINT64);
  g_assert_null (error);
  g_assert_cmpint (g_value_get_int64 (&value), ==, G_MAXINT64);

  g_value_unset (&value);
  g_value_init (&value, G_TYPE_UINT64);
  error = collect (&value, G_MAXUINT64);
  g_assert_null (error);
  g_assert_cmpuint (g_value_get_uint64 (&value), ==, G_MAXUINT64);

  g_value_unset (&value);
  g_value_init (&value, G_TYPE_FLOAT);
  error = collect (&value, G_MAXFLOAT);
  g_assert_null (error);
  g_assert_cmpfloat (g_value_get_float (&value), ==, G_MAXFLOAT);

  g_value_unset (&value);
  g_value_init (&value, G_TYPE_DOUBLE);
  error = collect (&value, G_MAXDOUBLE);
  g_assert_null (error);
  g_assert_cmpfloat (g_value_get_double (&value), ==, G_MAXDOUBLE);

  g_value_unset (&value);
  g_value_init (&value, G_TYPE_STRING);
  error = collect (&value, "string ?");
  g_assert_null (error);
  g_assert_cmpstr (g_value_get_string (&value), ==, "string ?");

  g_value_unset (&value);
  g_value_init (&value, G_TYPE_GTYPE);
  error = collect (&value, G_TYPE_BOXED);
  g_assert_null (error);
  g_assert_true (g_value_get_gtype (&value) == G_TYPE_BOXED);

  g_value_unset (&value);
  g_value_init (&value, G_TYPE_VARIANT);
  error = collect (&value, g_variant_new_uint32 (42));
  g_assert_null (error);
  g_assert_true (g_variant_is_of_type (g_value_get_variant (&value),
                                       G_VARIANT_TYPE ("u")));
  g_assert_cmpuint (g_variant_get_uint32 (g_value_get_variant (&value)), ==, 42);

  g_value_unset (&value);
}

static void
test_copying (void)
{
  GValue value = G_VALUE_INIT;
  gchar *error;

  {
    gchar c = 0;

    g_value_init (&value, G_TYPE_CHAR);
    g_value_set_char (&value, 'c');
    error = lcopy (&value, &c);
    g_assert_null (error);
    g_assert_cmpint (c, ==, 'c');
  }

  {
    guchar c = 0;

    g_value_unset (&value);
    g_value_init (&value, G_TYPE_UCHAR);
    g_value_set_uchar (&value, 129);
    error = lcopy (&value, &c);
    g_assert_null (error);
    g_assert_cmpint (c, ==, 129);
  }

  {
    gint c = 0;

    g_value_unset (&value);
    g_value_init (&value, G_TYPE_INT);
    g_value_set_int (&value, G_MAXINT);
    error = lcopy (&value, &c);
    g_assert_null (error);
    g_assert_cmpint (c, ==, G_MAXINT);
  }

  {
    guint c = 0;

    g_value_unset (&value);
    g_value_init (&value, G_TYPE_UINT);
    g_value_set_uint (&value, G_MAXUINT);
    error = lcopy (&value, &c);
    g_assert_null (error);
    g_assert_cmpuint (c, ==, G_MAXUINT);
  }

  {
    glong c = 0;

    g_value_unset (&value);
    g_value_init (&value, G_TYPE_LONG);
    g_value_set_long (&value, G_MAXLONG);
    error = lcopy (&value, &c);
    g_assert_null (error);
    g_assert (c == G_MAXLONG);
  }

  {
    gulong c = 0;

    g_value_unset (&value);
    g_value_init (&value, G_TYPE_ULONG);
    g_value_set_ulong (&value, G_MAXULONG);
    error = lcopy (&value, &c);
    g_assert_null (error);
    g_assert (c == G_MAXULONG);
  }

  {
    gint64 c = 0;

    g_value_unset (&value);
    g_value_init (&value, G_TYPE_INT64);
    g_value_set_int64 (&value, G_MAXINT64);
    error = lcopy (&value, &c);
    g_assert_null (error);
    g_assert (c == G_MAXINT64);
  }

  {
    guint64 c = 0;

    g_value_unset (&value);
    g_value_init (&value, G_TYPE_UINT64);
    g_value_set_uint64 (&value, G_MAXUINT64);
    error = lcopy (&value, &c);
    g_assert_null (error);
    g_assert (c == G_MAXUINT64);
  }

  {
    gfloat c = 0;

    g_value_unset (&value);
    g_value_init (&value, G_TYPE_FLOAT);
    g_value_set_float (&value, G_MAXFLOAT);
    error = lcopy (&value, &c);
    g_assert_null (error);
    g_assert (c == G_MAXFLOAT);
  }

  {
    gdouble c = 0;

    g_value_unset (&value);
    g_value_init (&value, G_TYPE_DOUBLE);
    g_value_set_double (&value, G_MAXDOUBLE);
    error = lcopy (&value, &c);
    g_assert_null (error);
    g_assert (c == G_MAXDOUBLE);
  }

  {
    gchar *c = NULL;

    g_value_unset (&value);
    g_value_init (&value, G_TYPE_STRING);
    g_value_set_string (&value, "string ?");
    error = lcopy (&value, &c);
    g_assert_null (error);
    g_assert_cmpstr (c, ==, "string ?");
    g_free (c);
  }

  {
    GType c = G_TYPE_NONE;

    g_value_unset (&value);
    g_value_init (&value, G_TYPE_GTYPE);
    g_value_set_gtype (&value, G_TYPE_BOXED);
    error = lcopy (&value, &c);
    g_assert_null (error);
    g_assert_true (c == G_TYPE_BOXED);
  }

  {
    GVariant *c = NULL;

    g_value_unset (&value);
    g_value_init (&value, G_TYPE_VARIANT);
    g_value_set_variant (&value, g_variant_new_uint32 (42));
    error = lcopy (&value, &c);
    g_assert_null (error);
    g_assert_nonnull (c);
    g_assert (g_variant_is_of_type (c, G_VARIANT_TYPE ("u")));
    g_assert_cmpuint (g_variant_get_uint32 (c), ==, 42);
    g_variant_unref (c);
    g_value_unset (&value);
  }
}

static void
test_value_basic (void)
{
  GValue value = G_VALUE_INIT;

  g_assert_false (G_IS_VALUE (&value));
  g_assert_false (G_VALUE_HOLDS_INT (&value));
  g_value_unset (&value);
  g_assert_false (G_IS_VALUE (&value));
  g_assert_false (G_VALUE_HOLDS_INT (&value));

  g_value_init (&value, G_TYPE_INT);
  g_assert_true (G_IS_VALUE (&value));
  g_assert_true (G_VALUE_HOLDS_INT (&value));
  g_assert_false (G_VALUE_HOLDS_UINT (&value));
  g_assert_cmpint (g_value_get_int (&value), ==, 0);

  g_value_set_int (&value, 10);
  g_assert_cmpint (g_value_get_int (&value), ==, 10);

  g_value_reset (&value);
  g_assert_true (G_IS_VALUE (&value));
  g_assert_true (G_VALUE_HOLDS_INT (&value));
  g_assert_cmpint (g_value_get_int (&value), ==, 0);

  g_value_unset (&value);
  g_assert_false (G_IS_VALUE (&value));
  g_assert_false (G_VALUE_HOLDS_INT (&value));
}

static void
test_value_string (void)
{
  const gchar *static1 = "static1";
  const gchar *static2 = "static2";
  const gchar *storedstr;
  const gchar *copystr;
  gchar *str1, *str2, *stolen_str;
  GValue value = G_VALUE_INIT;
  GValue copy = G_VALUE_INIT;

  g_test_summary ("Test that G_TYPE_STRING GValue copy properly");

  /*
   * Regular strings (ownership not passed)
   */

  /* Create a regular string gvalue and make sure it copies the provided string */
  g_value_init (&value, G_TYPE_STRING);
  g_assert_true (G_VALUE_HOLDS_STRING (&value));

  /* The string contents should be empty at this point */
  storedstr = g_value_get_string (&value);
  g_assert_true (storedstr == NULL);

  g_value_set_string (&value, static1);
  /* The contents should be a copy of the same string */
  storedstr = g_value_get_string (&value);
  g_assert_true (storedstr != static1);
  g_assert_cmpstr (storedstr, ==, static1);
  /* Check g_value_dup_string() provides a copy */
  str1 = g_value_dup_string (&value);
  g_assert_true (storedstr != str1);
  g_assert_cmpstr (str1, ==, static1);
  g_free (str1);

  /* Copying a regular string gvalue should copy the contents */
  g_value_init (&copy, G_TYPE_STRING);
  g_value_copy (&value, &copy);
  copystr = g_value_get_string (&copy);
  g_assert_true (copystr != storedstr);
  g_assert_cmpstr (copystr, ==, static1);
  g_value_unset (&copy);

  /* Setting a new string should change the contents */
  g_value_set_string (&value, static2);
  /* The contents should be a copy of that *new* string */
  storedstr = g_value_get_string (&value);
  g_assert_true (storedstr != static2);
  g_assert_cmpstr (storedstr, ==, static2);

  /* Setting a static string over that should also change it (test for
   * coverage and valgrind) */
  g_value_set_static_string (&value, static1);
  storedstr = g_value_get_string (&value);
  g_assert_true (storedstr != static2);
  g_assert_cmpstr (storedstr, ==, static1);

  /* Giving a string directly (ownership passed) should replace the content */
  str2 = g_strdup (static2);
  g_value_take_string (&value, str2);
  storedstr = g_value_get_string (&value);
  g_assert_true (storedstr != static2);
  g_assert_cmpstr (storedstr, ==, str2);

  g_value_unset (&value);

  /*
   * Regular strings (ownership passed)
   */

  g_value_init (&value, G_TYPE_STRING);
  g_assert_true (G_VALUE_HOLDS_STRING (&value));
  str1 = g_strdup (static1);
  g_value_take_string (&value, str1);
  /* The contents should be the string we provided */
  storedstr = g_value_get_string (&value);
  g_assert_true (storedstr == str1);
  /* But g_value_dup_string() should provide a copy */
  str2 = g_value_dup_string (&value);
  g_assert_true (storedstr != str2);
  g_assert_cmpstr (str2, ==, static1);
  g_free (str2);

  /* Copying a regular string gvalue (even with ownership passed) should copy
   * the contents */
  g_value_init (&copy, G_TYPE_STRING);
  g_value_copy (&value, &copy);
  copystr = g_value_get_string (&copy);
  g_assert_true (copystr != storedstr);
  g_assert_cmpstr (copystr, ==, static1);
  g_value_unset (&copy);

  /* Setting a new regular string should change the contents */
  g_value_set_string (&value, static2);
  /* The contents should be a copy of that *new* string */
  storedstr = g_value_get_string (&value);
  g_assert_true (storedstr != static2);
  g_assert_cmpstr (storedstr, ==, static2);

  /* Now check stealing the ownership of the contents */
  stolen_str = g_value_steal_string (&value);
  g_assert_null (g_value_get_string (&value));
  g_value_unset (&value);
  g_assert_cmpstr (stolen_str, ==, static2);
  g_free (stolen_str);

  /*
   * Static strings
   */
  g_value_init (&value, G_TYPE_STRING);
  g_assert_true (G_VALUE_HOLDS_STRING (&value));
  g_value_set_static_string (&value, static1);
  /* The contents should be the string we provided */
  storedstr = g_value_get_string (&value);
  g_assert_true (storedstr == static1);
  /* But g_value_dup_string() should provide a copy */
  str2 = g_value_dup_string (&value);
  g_assert_true (storedstr != str2);
  g_assert_cmpstr (str2, ==, static1);
  g_free (str2);

  /* Copying a static string gvalue should *actually* copy the contents */
  g_value_init (&copy, G_TYPE_STRING);
  g_value_copy (&value, &copy);
  copystr = g_value_get_string (&copy);
  g_assert_true (copystr != static1);
  g_value_unset (&copy);

  /* Setting a new string should change the contents */
  g_value_set_static_string (&value, static2);
  /* The contents should be a copy of that *new* string */
  storedstr = g_value_get_string (&value);
  g_assert_true (storedstr != static1);
  g_assert_cmpstr (storedstr, ==, static2);

  /* Check if g_value_steal_string() can handle GValue
   * with a static string */
  stolen_str = g_value_steal_string (&value);
  g_assert_true (stolen_str != static2);
  g_assert_cmpstr (stolen_str, ==, static2);
  g_assert_null (g_value_get_string (&value));
  g_free (stolen_str);

  g_value_unset (&value);

  /*
   * Interned/Canonical strings
   */
  static1 = g_intern_static_string (static1);
  g_value_init (&value, G_TYPE_STRING);
  g_assert_true (G_VALUE_HOLDS_STRING (&value));
  g_value_set_interned_string (&value, static1);
  g_assert_true (G_VALUE_IS_INTERNED_STRING (&value));
  /* The contents should be the string we provided */
  storedstr = g_value_get_string (&value);
  g_assert_true (storedstr == static1);
  /* But g_value_dup_string() should provide a copy */
  str2 = g_value_dup_string (&value);
  g_assert_true (storedstr != str2);
  g_assert_cmpstr (str2, ==, static1);
  g_free (str2);

  /* Copying an interned string gvalue should *not* copy the contents
   * and should still be an interned string */
  g_value_init (&copy, G_TYPE_STRING);
  g_value_copy (&value, &copy);
  g_assert_true (G_VALUE_IS_INTERNED_STRING (&copy));
  copystr = g_value_get_string (&copy);
  g_assert_true (copystr == static1);
  g_value_unset (&copy);

  /* Setting a new interned string should change the contents */
  static2 = g_intern_static_string (static2);
  g_value_set_interned_string (&value, static2);
  g_assert_true (G_VALUE_IS_INTERNED_STRING (&value));
  /* The contents should be the interned string */
  storedstr = g_value_get_string (&value);
  g_assert_cmpstr (storedstr, ==, static2);

  /* Setting a new regular string should change the contents */
  g_value_set_string (&value, static2);
  g_assert_false (G_VALUE_IS_INTERNED_STRING (&value));
  /* The contents should be a copy of that *new* string */
  storedstr = g_value_get_string (&value);
  g_assert_true (storedstr != static2);
  g_assert_cmpstr (storedstr, ==, static2);

  /* Check if g_value_steal_string() can handle GValue
   * with an interned string */
  stolen_str = g_value_steal_string (&value);
  g_assert_true (stolen_str != static2);
  g_assert_cmpstr (stolen_str, ==, static2);
  g_assert_null (g_value_get_string (&value));
  g_free (stolen_str);

  g_value_unset (&value);
}

static gint
cmpint (gconstpointer a, gconstpointer b)
{
  const GValue *aa = a;
  const GValue *bb = b;

  return g_value_get_int (aa) - g_value_get_int (bb);
}

static void
test_valuearray_basic (void)
{
  GValueArray *a;
  GValueArray *a2;
  GValue v = G_VALUE_INIT;
  GValue *p;
  guint i;

  a = g_value_array_new (20);

  g_value_init (&v, G_TYPE_INT);
  for (int j = 0; j < 100; j++)
    {
      g_value_set_int (&v, j);
      g_value_array_append (a, &v);
    }

  g_assert_cmpint (a->n_values, ==, 100);
  p = g_value_array_get_nth (a, 5);
  g_assert_cmpint (g_value_get_int (p), ==, 5);

  for (i = 20; i < 100; i+= 5)
    g_value_array_remove (a, 100 - i);

  for (int j = 100; j < 150; j++)
    {
      g_value_set_int (&v, j);
      g_value_array_prepend (a, &v);
    }

  g_value_array_sort (a, cmpint);
  for (i = 0; i < a->n_values - 1; i++)
    g_assert_cmpint (g_value_get_int (&a->values[i]), <=, g_value_get_int (&a->values[i+1]));

  a2 = g_value_array_copy (a);
  for (i = 0; i < a->n_values; i++)
    g_assert_cmpint (g_value_get_int (&a->values[i]), ==, g_value_get_int (&a2->values[i]));

  g_value_array_free (a);
  g_value_array_free (a2);
}

static gint
cmpint_with_data (gconstpointer a,
                  gconstpointer b,
                  gpointer      user_data)
{
  const GValue *aa = a;
  const GValue *bb = b;

  g_assert_cmpuint (GPOINTER_TO_UINT (user_data), ==, 123);

  return g_value_get_int (aa) - g_value_get_int (bb);
}

static void
test_value_array_sort_with_data (void)
{
  GValueArray *a, *a2;
  GValue v = G_VALUE_INIT;

  a = g_value_array_new (20);

  /* Try sorting an empty array. */
  a2 = g_value_array_sort_with_data (a, cmpint_with_data, GUINT_TO_POINTER (456));
  g_assert_cmpuint (a->n_values, ==, 0);
  g_assert_true (a2 == a);

  /* Add some values and try sorting them. */
  g_value_init (&v, G_TYPE_INT);
  for (int i = 0; i < 100; i++)
    {
      g_value_set_int (&v, 100 - i);
      g_value_array_append (a, &v);
    }

  g_assert_cmpint (a->n_values, ==, 100);

  a2 = g_value_array_sort_with_data (a, cmpint_with_data, GUINT_TO_POINTER (123));

  for (unsigned int i = 0; i < a->n_values - 1; i++)
    g_assert_cmpint (g_value_get_int (&a->values[i]), <=, g_value_get_int (&a->values[i+1]));

  g_assert_true (a2 == a);

  g_value_array_free (a);
}

/* We create some dummy objects with this relationship:
 *
 *               GObject           TestInterface
 *              /       \         /  /
 *     TestObjectA     TestObjectB  /
 *      /       \                  /
 * TestObjectA1 TestObjectA2-------   
 *
 * ie: TestObjectA1 and TestObjectA2 are subclasses of TestObjectA
 * and TestObjectB is related to neither. TestObjectA2 and TestObjectB
 * implement TestInterface
 */

typedef GTypeInterface TestInterfaceInterface;
static GType test_interface_get_type (void);
G_DEFINE_INTERFACE (TestInterface, test_interface, G_TYPE_OBJECT)
static void test_interface_default_init (TestInterfaceInterface *iface) { }

static GType test_object_a_get_type (void);
typedef GObject TestObjectA; typedef GObjectClass TestObjectAClass;
G_DEFINE_TYPE (TestObjectA, test_object_a, G_TYPE_OBJECT)
static void test_object_a_class_init (TestObjectAClass *class) { }
static void test_object_a_init (TestObjectA *a) { }

static GType test_object_b_get_type (void);
typedef GObject TestObjectB; typedef GObjectClass TestObjectBClass;
static void test_object_b_iface_init (TestInterfaceInterface *iface) { }
G_DEFINE_TYPE_WITH_CODE (TestObjectB, test_object_b, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (test_interface_get_type (), test_object_b_iface_init))
static void test_object_b_class_init (TestObjectBClass *class) { }
static void test_object_b_init (TestObjectB *b) { }

static GType test_object_a1_get_type (void);
typedef GObject TestObjectA1; typedef GObjectClass TestObjectA1Class;
G_DEFINE_TYPE (TestObjectA1, test_object_a1, test_object_a_get_type ())
static void test_object_a1_class_init (TestObjectA1Class *class) { }
static void test_object_a1_init (TestObjectA1 *c) { }

static GType test_object_a2_get_type (void);
typedef GObject TestObjectA2; typedef GObjectClass TestObjectA2Class;
static void test_object_a2_iface_init (TestInterfaceInterface *iface) { }
G_DEFINE_TYPE_WITH_CODE (TestObjectA2, test_object_a2, test_object_a_get_type (),
                         G_IMPLEMENT_INTERFACE (test_interface_get_type (), test_object_a2_iface_init))
static void test_object_a2_class_init (TestObjectA2Class *class) { }
static void test_object_a2_init (TestObjectA2 *b) { }

static void
test_value_transform_object (void)
{
  GValue src = G_VALUE_INIT;
  GValue dest = G_VALUE_INIT;
  GObject *object;
  guint i, s, d;
  GType types[] = {
    G_TYPE_OBJECT,
    test_interface_get_type (),
    test_object_a_get_type (),
    test_object_b_get_type (),
    test_object_a1_get_type (),
    test_object_a2_get_type ()
  };

  for (i = 0; i < G_N_ELEMENTS (types); i++)
    {
      if (!G_TYPE_IS_CLASSED (types[i]))
        continue;

      object = g_object_new (types[i], NULL);

      for (s = 0; s < G_N_ELEMENTS (types); s++)
        {
          if (!G_TYPE_CHECK_INSTANCE_TYPE (object, types[s]))
            continue;

          g_value_init (&src, types[s]);
          g_value_set_object (&src, object);
          g_value_set_object (&src, g_value_get_object (&src));

          for (d = 0; d < G_N_ELEMENTS (types); d++)
            {
              g_test_message ("Next: %s object in GValue of %s to GValue of %s", g_type_name (types[i]), g_type_name (types[s]), g_type_name (types[d]));
              g_assert_true (g_value_type_transformable (types[s], types[d]));
              g_value_init (&dest, types[d]);
              g_assert_true (g_value_transform (&src, &dest));
              g_assert_cmpint (g_value_get_object (&dest) != NULL, ==, G_TYPE_CHECK_INSTANCE_TYPE (object, types[d]));
              g_value_unset (&dest);
            }
          g_value_unset (&src);
        }

      g_object_unref (object);
    }
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/value/basic", test_value_basic);
  g_test_add_func ("/value/array/basic", test_valuearray_basic);
  g_test_add_func ("/value/array/sort-with-data", test_value_array_sort_with_data);
  g_test_add_func ("/value/collection", test_collection);
  g_test_add_func ("/value/copying", test_copying);
  g_test_add_func ("/value/enum-transformation", test_enum_transformation);
  g_test_add_func ("/value/gtype", test_gtype_value);
  g_test_add_func ("/value/string", test_value_string);
  g_test_add_func ("/value/transform-object", test_value_transform_object);

  return g_test_run ();
}
