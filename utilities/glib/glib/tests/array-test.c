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

#undef G_DISABLE_ASSERT

#include "glib.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Test data to be passed to any function which calls g_array_new(), providing
 * the parameters for that call. Most #GArray tests should be repeated for all
 * possible values of #ArrayTestData. */
typedef struct
{
  gboolean zero_terminated;
  gboolean clear_;
} ArrayTestData;

/* Assert that @garray contains @n_expected_elements as given in @expected_data.
 * @garray must contain #gint elements. */
static void
assert_int_array_equal (GArray     *garray,
                        const gint *expected_data,
                        gsize       n_expected_elements)
{
  gsize i;

  g_assert_cmpuint (garray->len, ==, n_expected_elements);
  for (i = 0; i < garray->len; i++)
    g_assert_cmpint (g_array_index (garray, gint, i), ==, expected_data[i]);
}

/* Iff config->zero_terminated is %TRUE, assert that the final element of
 * @garray is zero. @garray must contain #gint elements. */
static void
assert_int_array_zero_terminated (const ArrayTestData *config,
                                  GArray              *garray)
{
  if (config->zero_terminated)
    {
      gint *data = (gint *) garray->data;
      g_assert_cmpint (data[garray->len], ==, 0);
    }
}

static void
sum_up (gpointer data,
	gpointer user_data)
{
  gint *sum = (gint *)user_data;

  *sum += GPOINTER_TO_INT (data);
}

/* Check that expanding an array with g_array_set_size() clears the new elements
 * if @clear_ was specified during construction. */
static void
array_set_size (gconstpointer test_data)
{
  const ArrayTestData *config = test_data;
  GArray *garray;
  gsize i;

  garray = g_array_new (config->zero_terminated, config->clear_, sizeof (gint));
  g_assert_cmpuint (garray->len, ==, 0);
  assert_int_array_zero_terminated (config, garray);

  g_array_set_size (garray, 5);
  g_assert_cmpuint (garray->len, ==, 5);
  assert_int_array_zero_terminated (config, garray);

  if (config->clear_)
    for (i = 0; i < 5; i++)
      g_assert_cmpint (g_array_index (garray, gint, i), ==, 0);

  g_array_unref (garray);
}

/* Check that unallocated zero terminated arrays can be set to size 0. */
static void
array_set_size_zero_terminated_null (void)
{
  GArray *garray;

  garray = g_array_new_take_zero_terminated(NULL, FALSE, sizeof (gchar));

  g_array_set_size (garray, 0);
  g_assert_cmpuint (garray->len, ==, 0);

  g_array_free (garray, TRUE);
}

/* As with array_set_size(), but with a sized array. */
static void
array_set_size_sized (gconstpointer test_data)
{
  const ArrayTestData *config = test_data;
  GArray *garray;
  gsize i;

  garray = g_array_sized_new (config->zero_terminated, config->clear_, sizeof (gint), 10);
  g_assert_cmpuint (garray->len, ==, 0);
  assert_int_array_zero_terminated (config, garray);

  g_array_set_size (garray, 5);
  g_assert_cmpuint (garray->len, ==, 5);
  assert_int_array_zero_terminated (config, garray);

  if (config->clear_)
    for (i = 0; i < 5; i++)
      g_assert_cmpint (g_array_index (garray, gint, i), ==, 0);

  g_array_unref (garray);
}

/* Check that a zero-terminated array does actually have a zero terminator. */
static void
array_new_zero_terminated (void)
{
  GArray *garray;
  gchar *out_str = NULL;

  garray = g_array_new (TRUE, FALSE, sizeof (gchar));
  g_assert_cmpuint (garray->len, ==, 0);

  g_array_append_vals (garray, "hello", (guint) strlen ("hello"));
  g_assert_cmpuint (garray->len, ==, 5);
  g_assert_cmpstr (garray->data, ==, "hello");

  out_str = g_array_free (garray, FALSE);
  g_assert_cmpstr (out_str, ==, "hello");
  g_free (out_str);
}

static void
array_new_take (void)
{
  const size_t array_size = 10000;
  GArray *garray;
  gpointer *data;
  gpointer *old_data_copy;
  gsize len;

  garray = g_array_new (FALSE, FALSE, sizeof (size_t));
  for (size_t i = 0; i < array_size; i++)
    g_array_append_val (garray, i);

  data = g_array_steal (garray, &len);
  g_assert_cmpuint (array_size, ==, len);
  g_assert_nonnull (data);
  g_clear_pointer (&garray, g_array_unref);

  old_data_copy = g_memdup2 (data, len * sizeof (size_t));
  garray = g_array_new_take (g_steal_pointer (&data), len, FALSE, sizeof (size_t));
  g_assert_cmpuint (garray->len, ==, array_size);

  g_assert_cmpuint (g_array_index (garray, size_t, 0), ==, 0);
  g_assert_cmpuint (g_array_index (garray, size_t, 10), ==, 10);

  g_assert_cmpmem (old_data_copy, array_size * sizeof (size_t),
                   garray->data, array_size * sizeof (size_t));

  size_t val = 55;
  g_array_append_val (garray, val);
  val = 33;
  g_array_prepend_val (garray, val);

  g_assert_cmpuint (garray->len, ==, array_size + 2);
  g_assert_cmpuint (g_array_index (garray, size_t, 0), ==, 33);
  g_assert_cmpuint (g_array_index (garray, size_t, garray->len - 1), ==, 55);

  g_array_remove_index (garray, 0);
  g_assert_cmpuint (garray->len, ==, array_size + 1);
  g_array_remove_index (garray, garray->len - 1);
  g_assert_cmpuint (garray->len, ==, array_size);

  g_assert_cmpmem (old_data_copy, array_size * sizeof (size_t),
                   garray->data, array_size * sizeof (size_t));

  g_array_unref (garray);
  g_free (old_data_copy);
}

static void
array_new_take_empty (void)
{
  GArray *garray;
  size_t empty_array[] = {0};

  garray = g_array_new_take (
    g_memdup2 (&empty_array, sizeof (size_t)), 0, FALSE, sizeof (size_t));
  g_assert_cmpuint (garray->len, ==, 0);

  g_clear_pointer (&garray, g_array_unref);

  garray = g_array_new_take (NULL, 0, FALSE, sizeof (size_t));
  g_assert_cmpuint (garray->len, ==, 0);

  g_clear_pointer (&garray, g_array_unref);
}

static void
array_new_take_zero_terminated (void)
{
  size_t array_size = 10000;
  GArray *garray;
  gpointer *data;
  gpointer *old_data_copy;
  gsize len;

  garray = g_array_new (TRUE, FALSE, sizeof (size_t));
  for (size_t i = 1; i <= array_size; i++)
    g_array_append_val (garray, i);

  data = g_array_steal (garray, &len);
  g_assert_cmpuint (array_size, ==, len);
  g_assert_nonnull (data);
  g_clear_pointer (&garray, g_array_unref);

  old_data_copy = g_memdup2 (data, len * sizeof (size_t));
  garray = g_array_new_take_zero_terminated (
    g_steal_pointer (&data), FALSE, sizeof (size_t));
  g_assert_cmpuint (garray->len, ==, array_size);
  g_assert_cmpuint (g_array_index (garray, size_t, garray->len), ==, 0);

  g_assert_cmpuint (g_array_index (garray, size_t, 0), ==, 1);
  g_assert_cmpuint (g_array_index (garray, size_t, 10), ==, 11);

  g_assert_cmpmem (old_data_copy, array_size * sizeof (size_t),
                   garray->data, array_size * sizeof (size_t));

  size_t val = 55;
  g_array_append_val (garray, val);
  val = 33;
  g_array_prepend_val (garray, val);

  g_assert_cmpuint (garray->len, ==, array_size + 2);
  g_assert_cmpuint (g_array_index (garray, size_t, 0), ==, 33);
  g_assert_cmpuint (g_array_index (garray, size_t, garray->len - 1), ==, 55);

  g_array_remove_index (garray, 0);
  g_assert_cmpuint (garray->len, ==, array_size + 1);
  g_array_remove_index (garray, garray->len - 1);
  g_assert_cmpuint (garray->len, ==, array_size);
  g_assert_cmpuint (g_array_index (garray, size_t, garray->len), ==, 0);

  g_assert_cmpmem (old_data_copy, array_size * sizeof (size_t),
                   garray->data, array_size * sizeof (size_t));

  g_clear_pointer (&garray, g_array_unref);
  g_clear_pointer (&old_data_copy, g_free);

  array_size = G_MAXUINT8;
  garray = g_array_new (TRUE, FALSE, sizeof (guint8));
  for (guint8 i = 1; i < array_size; i++)
    g_array_append_val (garray, i);

  guint8 byte_val = G_MAXUINT8 / 2;
  g_array_append_val (garray, byte_val);

  data = g_array_steal (garray, &len);
  g_assert_cmpuint (array_size, ==, len);
  g_assert_nonnull (data);
  g_clear_pointer (&garray, g_array_unref);

  old_data_copy = g_memdup2 (data, len * sizeof (guint8));
  garray = g_array_new_take_zero_terminated (
    g_steal_pointer (&data), FALSE, sizeof (guint8));
  g_assert_cmpuint (garray->len, ==, array_size);
  g_assert_cmpuint (g_array_index (garray, guint8, garray->len), ==, 0);

  g_assert_cmpuint (g_array_index (garray, guint8, 0), ==, 1);
  g_assert_cmpuint (g_array_index (garray, guint8, 10), ==, 11);

  g_assert_cmpmem (old_data_copy, array_size * sizeof (guint8),
                   garray->data, array_size * sizeof (guint8));

  byte_val = 55;
  g_array_append_val (garray, byte_val);
  byte_val = 33;
  g_array_prepend_val (garray, byte_val);

  g_assert_cmpuint (garray->len, ==, array_size + 2);
  g_assert_cmpuint (g_array_index (garray, guint8, 0), ==, 33);
  g_assert_cmpuint (g_array_index (garray, guint8, garray->len - 1), ==, 55);

  g_array_remove_index (garray, 0);
  g_assert_cmpuint (garray->len, ==, array_size + 1);
  g_array_remove_index (garray, garray->len - 1);
  g_assert_cmpuint (garray->len, ==, array_size);
  g_assert_cmpuint (g_array_index (garray, guint8, garray->len), ==, 0);

  g_assert_cmpmem (old_data_copy, array_size * sizeof (guint8),
                   garray->data, array_size * sizeof (guint8));

  g_clear_pointer (&garray, g_array_unref);
  g_clear_pointer (&old_data_copy, g_free);
}

/* Check that zero terminated arrays with zero size elements are not allowed. */
static void
array_new_take_zero_terminated_zero_size (void)
{
  gpointer str = g_strdup ("not null");

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion 'element_size > 0 && element_size <= G_MAXUINT' failed");
  g_assert_null (
      g_array_new_take_zero_terminated (str, FALSE, 0));
  g_test_assert_expected_messages ();

  g_free (str);
}

/* Check that a non-existing array becomes a zero-terminated one when requested. */
static void
array_new_take_zero_terminated_null (void)
{
  GArray *garray;
  gchar *out_str = NULL;
  gsize len;

  garray = g_array_new_take_zero_terminated (NULL, FALSE, sizeof (gchar));
  g_assert_cmpuint (garray->len, ==, 0);

  out_str = g_array_steal (garray, &len);
  g_assert_cmpstr (out_str, ==, NULL);
  g_assert_cmpuint (len, ==, 0);

  g_free (out_str);
  g_array_free (garray, TRUE);
}

static void
array_new_take_overflow (void)
{
#if SIZE_WIDTH <= UINT_WIDTH
  g_test_skip ("Overflow test requires SIZE_WIDTH > UINT_WIDTH.");
#else
  if (!g_test_undefined ())
      return;

  /* Check for overflow should happen before data is accessed. */
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion 'len <= G_MAXUINT' failed");
  g_assert_null (
    g_array_new_take (
      (gpointer) (int []) { 0 }, (gsize) G_MAXUINT + 1, FALSE, sizeof (int)));
  g_test_assert_expected_messages ();

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion 'element_size > 0 && element_size <= G_MAXUINT' failed");
  g_assert_null (
    g_array_new_take (NULL, 0, FALSE, (gsize) G_MAXUINT + 1));
  g_test_assert_expected_messages ();
#endif
}

/* Check that arrays with zero size elements are not allowed. */
static void
array_new_take_zero_size (void)
{
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion 'element_size > 0 && element_size <= G_MAXUINT' failed");
  g_assert_null (
      g_array_new_take (NULL, 0, FALSE, 0));
  g_test_assert_expected_messages ();
}

/* Check g_array_steal() function */
static void
array_steal (void)
{
  const guint array_size = 10000;
  GArray *garray;
  gint *adata;
  guint i;
  gsize len, past_len;

  garray = g_array_new (FALSE, FALSE, sizeof (gint));
  adata = (gint *) g_array_steal (garray, NULL);
  g_assert_null (adata);

  adata = (gint *) g_array_steal (garray, &len);
  g_assert_null (adata);
  g_assert_cmpint (len, ==, 0);

  for (i = 0; i < array_size; i++)
    g_array_append_val (garray, i);

  for (i = 0; i < array_size; i++)
    g_assert_cmpint (g_array_index (garray, gint, i), ==, i);


  past_len = garray->len;
  adata = (gint *) g_array_steal (garray, &len);
  for (i = 0; i < array_size; i++)
    g_assert_cmpint (adata[i], ==, i);

  g_assert_cmpint (past_len, ==, len);
  g_assert_cmpint (garray->len, ==, 0);

  g_array_append_val (garray, i);

  g_assert_cmpint (adata[0], ==, 0);
  g_assert_cmpint (g_array_index (garray, gint, 0), ==, array_size);
  g_assert_cmpint (garray->len, ==, 1);

  g_array_remove_index (garray, 0);

  for (i = 0; i < array_size; i++)
    g_array_append_val (garray, i);

  g_assert_cmpint (garray->len, ==, array_size);
  g_assert_cmpmem (adata, array_size * sizeof (gint),
                   garray->data, array_size * sizeof (gint));
  g_free (adata);
  g_array_free (garray, TRUE);
}

/* Check that g_array_append_val() works correctly for various #GArray
 * configurations. */
static void
array_append_val (gconstpointer test_data)
{
  const ArrayTestData *config = test_data;
  GArray *garray;
  gint i;
  gint *segment;

  garray = g_array_new (config->zero_terminated, config->clear_, sizeof (gint));
  for (i = 0; i < 10000; i++)
    g_array_append_val (garray, i);
  assert_int_array_zero_terminated (config, garray);

  for (i = 0; i < 10000; i++)
    g_assert_cmpint (g_array_index (garray, gint, i), ==, i);

  segment = (gint*)g_array_free (garray, FALSE);
  for (i = 0; i < 10000; i++)
    g_assert_cmpint (segment[i], ==, i);
  if (config->zero_terminated)
    g_assert_cmpint (segment[10000], ==, 0);

  g_free (segment);
}

/* Check that g_array_prepend_val() works correctly for various #GArray
 * configurations. */
static void
array_prepend_val (gconstpointer test_data)
{
  const ArrayTestData *config = test_data;
  GArray *garray;
  gint i;

  garray = g_array_new (config->zero_terminated, config->clear_, sizeof (gint));
  for (i = 0; i < 100; i++)
    g_array_prepend_val (garray, i);
  assert_int_array_zero_terminated (config, garray);

  for (i = 0; i < 100; i++)
    g_assert_cmpint (g_array_index (garray, gint, i), ==, (100 - i - 1));

  g_array_free (garray, TRUE);
}

/* Test that g_array_prepend_vals() works correctly with various array
 * configurations. */
static void
array_prepend_vals (gconstpointer test_data)
{
  const ArrayTestData *config = test_data;
  GArray *garray, *garray_out;
  const gint vals[] = { 0, 1, 2, 3, 4 };
  const gint expected_vals1[] = { 0, 1 };
  const gint expected_vals2[] = { 2, 0, 1 };
  const gint expected_vals3[] = { 3, 4, 2, 0, 1 };

  /* Set up an array. */
  garray = g_array_new (config->zero_terminated, config->clear_, sizeof (gint));
  assert_int_array_zero_terminated (config, garray);

  /* Prepend several values to an empty array. */
  garray_out = g_array_prepend_vals (garray, vals, 2);
  g_assert_true (garray == garray_out);
  assert_int_array_equal (garray, expected_vals1, G_N_ELEMENTS (expected_vals1));
  assert_int_array_zero_terminated (config, garray);

  /* Prepend a single value. */
  garray_out = g_array_prepend_vals (garray, vals + 2, 1);
  g_assert_true (garray == garray_out);
  assert_int_array_equal (garray, expected_vals2, G_N_ELEMENTS (expected_vals2));
  assert_int_array_zero_terminated (config, garray);

  /* Prepend several values to a non-empty array. */
  garray_out = g_array_prepend_vals (garray, vals + 3, 2);
  g_assert_true (garray == garray_out);
  assert_int_array_equal (garray, expected_vals3, G_N_ELEMENTS (expected_vals3));
  assert_int_array_zero_terminated (config, garray);

  /* Prepend no values. */
  garray_out = g_array_prepend_vals (garray, vals, 0);
  g_assert_true (garray == garray_out);
  assert_int_array_equal (garray, expected_vals3, G_N_ELEMENTS (expected_vals3));
  assert_int_array_zero_terminated (config, garray);

  /* Prepend no values with %NULL data. */
  garray_out = g_array_prepend_vals (garray, NULL, 0);
  g_assert_true (garray == garray_out);
  assert_int_array_equal (garray, expected_vals3, G_N_ELEMENTS (expected_vals3));
  assert_int_array_zero_terminated (config, garray);

  g_array_free (garray, TRUE);
}

/* Test that g_array_insert_vals() works correctly with various array
 * configurations. */
static void
array_insert_vals (gconstpointer test_data)
{
  const ArrayTestData *config = test_data;
  GArray *garray, *garray_out;
  gsize i;
  const gint vals[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
  const gint expected_vals1[] = { 0, 1 };
  const gint expected_vals2[] = { 0, 2, 3, 1 };
  const gint expected_vals3[] = { 0, 2, 3, 1, 4 };
  const gint expected_vals4[] = { 5, 0, 2, 3, 1, 4 };
  const gint expected_vals5[] = { 5, 0, 2, 3, 1, 4, 0, 0, 0, 0, 6, 7 };

  /* Set up an array. */
  garray = g_array_new (config->zero_terminated, config->clear_, sizeof (gint));
  assert_int_array_zero_terminated (config, garray);

  /* Insert several values at the beginning. */
  garray_out = g_array_insert_vals (garray, 0, vals, 2);
  g_assert_true (garray == garray_out);
  assert_int_array_equal (garray, expected_vals1, G_N_ELEMENTS (expected_vals1));
  assert_int_array_zero_terminated (config, garray);

  /* Insert some more part-way through. */
  garray_out = g_array_insert_vals (garray, 1, vals + 2, 2);
  g_assert_true (garray == garray_out);
  assert_int_array_equal (garray, expected_vals2, G_N_ELEMENTS (expected_vals2));
  assert_int_array_zero_terminated (config, garray);

  /* And at the end. */
  garray_out = g_array_insert_vals (garray, garray->len, vals + 4, 1);
  g_assert_true (garray == garray_out);
  assert_int_array_equal (garray, expected_vals3, G_N_ELEMENTS (expected_vals3));
  assert_int_array_zero_terminated (config, garray);

  /* Then back at the beginning again. */
  garray_out = g_array_insert_vals (garray, 0, vals + 5, 1);
  g_assert_true (garray == garray_out);
  assert_int_array_equal (garray, expected_vals4, G_N_ELEMENTS (expected_vals4));
  assert_int_array_zero_terminated (config, garray);

  /* Insert zero elements. */
  garray_out = g_array_insert_vals (garray, 0, vals, 0);
  g_assert_true (garray == garray_out);
  assert_int_array_equal (garray, expected_vals4, G_N_ELEMENTS (expected_vals4));
  assert_int_array_zero_terminated (config, garray);

  /* Insert zero elements with a %NULL pointer. */
  garray_out = g_array_insert_vals (garray, 0, NULL, 0);
  g_assert_true (garray == garray_out);
  assert_int_array_equal (garray, expected_vals4, G_N_ELEMENTS (expected_vals4));
  assert_int_array_zero_terminated (config, garray);

  /* Insert some elements off the end of the array. The behaviour here depends
   * on whether the array clears entries. */
  garray_out = g_array_insert_vals (garray, garray->len + 4, vals + 6, 2);
  g_assert_true (garray == garray_out);

  g_assert_cmpuint (garray->len, ==, G_N_ELEMENTS (expected_vals5));
  for (i = 0; i < G_N_ELEMENTS (expected_vals5); i++)
    {
      if (config->clear_ || i < 6 || i > 9)
        g_assert_cmpint (g_array_index (garray, gint, i), ==, expected_vals5[i]);
    }

  assert_int_array_zero_terminated (config, garray);

  g_array_free (garray, TRUE);
}

/* Check that g_array_remove_index() works correctly for various #GArray
 * configurations. */
static void
array_remove_index (gconstpointer test_data)
{
  const ArrayTestData *config = test_data;
  GArray *garray;
  guint i;
  gint prev, cur;

  garray = g_array_new (config->zero_terminated, config->clear_, sizeof (gint));
  for (i = 0; i < 100; i++)
    g_array_append_val (garray, i);
  assert_int_array_zero_terminated (config, garray);

  g_assert_cmpint (garray->len, ==, 100);

  g_array_remove_index (garray, 1);
  g_array_remove_index (garray, 3);
  g_array_remove_index (garray, 21);
  g_array_remove_index (garray, 57);

  g_assert_cmpint (garray->len, ==, 96);
  assert_int_array_zero_terminated (config, garray);

  prev = -1;
  for (i = 0; i < garray->len; i++)
    {
      cur = g_array_index (garray, gint, i);
      g_assert (cur != 1 &&  cur != 4 && cur != 23 && cur != 60);
      g_assert_cmpint (prev, <, cur);
      prev = cur;
    }

  g_array_free (garray, TRUE);
}

/* Check that g_array_remove_index_fast() works correctly for various #GArray
 * configurations. */
static void
array_remove_index_fast (gconstpointer test_data)
{
  const ArrayTestData *config = test_data;
  GArray *garray;
  guint i;
  gint prev, cur;

  garray = g_array_new (config->zero_terminated, config->clear_, sizeof (gint));
  for (i = 0; i < 100; i++)
    g_array_append_val (garray, i);

  g_assert_cmpint (garray->len, ==, 100);
  assert_int_array_zero_terminated (config, garray);

  g_array_remove_index_fast (garray, 1);
  g_array_remove_index_fast (garray, 3);
  g_array_remove_index_fast (garray, 21);
  g_array_remove_index_fast (garray, 57);

  g_assert_cmpint (garray->len, ==, 96);
  assert_int_array_zero_terminated (config, garray);

  prev = -1;
  for (i = 0; i < garray->len; i++)
    {
      cur = g_array_index (garray, gint, i);
      g_assert (cur != 1 &&  cur != 3 && cur != 21 && cur != 57);
      if (cur < 96)
        {
          g_assert_cmpint (prev, <, cur);
          prev = cur;
        }
    }

  g_array_free (garray, TRUE);
}

/* Check that g_array_remove_range() works correctly for various #GArray
 * configurations. */
static void
array_remove_range (gconstpointer test_data)
{
  const ArrayTestData *config = test_data;
  GArray *garray;
  guint i;
  gint prev, cur;

  garray = g_array_new (config->zero_terminated, config->clear_, sizeof (gint));
  for (i = 0; i < 100; i++)
    g_array_append_val (garray, i);

  g_assert_cmpint (garray->len, ==, 100);
  assert_int_array_zero_terminated (config, garray);

  g_array_remove_range (garray, 31, 4);

  g_assert_cmpint (garray->len, ==, 96);
  assert_int_array_zero_terminated (config, garray);

  prev = -1;
  for (i = 0; i < garray->len; i++)
    {
      cur = g_array_index (garray, gint, i);
      g_assert (cur < 31 || cur > 34);
      g_assert_cmpint (prev, <, cur);
      prev = cur;
    }

  /* Ensure the entire array can be cleared, even when empty. */
  g_array_remove_range (garray, 0, garray->len);

  g_assert_cmpint (garray->len, ==, 0);
  assert_int_array_zero_terminated (config, garray);

  g_array_remove_range (garray, 0, garray->len);

  g_assert_cmpint (garray->len, ==, 0);
  assert_int_array_zero_terminated (config, garray);

  g_array_free (garray, TRUE);
}

/* Check that g_array_remove_range() works with a zero terminated array
 * without any data. */
static void
array_remove_range_zero_terminated_null (void)
{
  GArray *garray;

  garray = g_array_new_take_zero_terminated(NULL, FALSE, sizeof (gchar));

  g_array_remove_range (garray, 0, 0);
  g_assert_cmpuint (garray->len, ==, 0);

  g_array_free (garray, TRUE);
}

static void
array_ref_count (void)
{
  GArray *garray;
  GArray *garray2;
  gint i;

  garray = g_array_new (FALSE, FALSE, sizeof (gint));
  g_assert_cmpint (g_array_get_element_size (garray), ==, sizeof (gint));
  for (i = 0; i < 100; i++)
    g_array_prepend_val (garray, i);

  /* check we can ref, unref and still access the array */
  garray2 = g_array_ref (garray);
  g_assert (garray == garray2);
  g_array_unref (garray2);
  for (i = 0; i < 100; i++)
    g_assert_cmpint (g_array_index (garray, gint, i), ==, (100 - i - 1));

  /* garray2 should be an empty valid GArray wrapper */
  garray2 = g_array_ref (garray);
  g_array_free (garray, TRUE);

  g_assert_cmpint (garray2->len, ==, 0);
  g_array_unref (garray2);
}

static int
int_compare (gconstpointer p1, gconstpointer p2)
{
  const gint *i1 = p1;
  const gint *i2 = p2;

  return *i1 - *i2;
}

static void
array_copy (gconstpointer test_data)
{
  GArray *array, *array_copy;
  gsize i;
  const ArrayTestData *config = test_data;
  const gsize array_size = 100;

  /* Testing degenerated cases */
  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      array = g_array_copy (NULL);
      g_test_assert_expected_messages ();

      g_assert_null (array);
    }

  /* Testing simple copy */
  array = g_array_new (config->zero_terminated, config->clear_, sizeof (gint));

  for (i = 0; i < array_size; i++)
    g_array_append_val (array, i);

  array_copy = g_array_copy (array);

  /* Check internal data */
  for (i = 0; i < array_size; i++)
    g_assert_cmpuint (g_array_index (array, gint, i), ==,
                      g_array_index (array_copy, gint, i));

  /* Check internal parameters ('zero_terminated' flag) */
  if (config->zero_terminated)
    {
      const gint *data = (const gint *) array_copy->data;
      g_assert_cmpint (data[array_copy->len], ==, 0);
    }

  /* Check internal parameters ('clear' flag) */
  if (config->clear_)
    {
      guint old_length = array_copy->len;
      g_array_set_size (array_copy, old_length + 5);
      for (i = old_length; i < old_length + 5; i++)
        g_assert_cmpint (g_array_index (array_copy, gint, i), ==, 0);
    }

  /* Clean-up */
  g_array_unref (array);
  g_array_unref (array_copy);
}

static int
int_compare_data (gconstpointer p1, gconstpointer p2, gpointer data)
{
  const gint *i1 = p1;
  const gint *i2 = p2;

  return *i1 - *i2;
}

/* Check that g_array_sort() works correctly for various #GArray
 * configurations. */
static void
array_sort (gconstpointer test_data)
{
  const ArrayTestData *config = test_data;
  GArray *garray;
  guint i;
  gint prev, cur;

  garray = g_array_new (config->zero_terminated, config->clear_, sizeof (gint));

  /* Sort empty array */
  g_array_sort (garray, int_compare);

  for (i = 0; i < 10000; i++)
    {
      cur = g_random_int_range (0, 10000);
      g_array_append_val (garray, cur);
    }
  assert_int_array_zero_terminated (config, garray);

  g_array_sort (garray, int_compare);
  assert_int_array_zero_terminated (config, garray);

  prev = -1;
  for (i = 0; i < garray->len; i++)
    {
      cur = g_array_index (garray, gint, i);
      g_assert_cmpint (prev, <=, cur);
      prev = cur;
    }

  g_array_free (garray, TRUE);
}

/* Check that g_array_sort_with_data() works correctly for various #GArray
 * configurations. */
static void
array_sort_with_data (gconstpointer test_data)
{
  const ArrayTestData *config = test_data;
  GArray *garray;
  guint i;
  gint prev, cur;

  garray = g_array_new (config->zero_terminated, config->clear_, sizeof (gint));

  /* Sort empty array */
  g_array_sort_with_data (garray, int_compare_data, NULL);

  for (i = 0; i < 10000; i++)
    {
      cur = g_random_int_range (0, 10000);
      g_array_append_val (garray, cur);
    }
  assert_int_array_zero_terminated (config, garray);

  g_array_sort_with_data (garray, int_compare_data, NULL);
  assert_int_array_zero_terminated (config, garray);

  prev = -1;
  for (i = 0; i < garray->len; i++)
    {
      cur = g_array_index (garray, gint, i);
      g_assert_cmpint (prev, <=, cur);
      prev = cur;
    }

  g_array_free (garray, TRUE);
}

static gint num_clear_func_invocations = 0;

static void
my_clear_func (gpointer data)
{
  num_clear_func_invocations += 1;
}

static void
array_clear_func (void)
{
  GArray *garray;
  gint i;
  gint cur;

  garray = g_array_new (FALSE, FALSE, sizeof (gint));
  g_array_set_clear_func (garray, my_clear_func);

  for (i = 0; i < 10; i++)
    {
      cur = g_random_int_range (0, 100);
      g_array_append_val (garray, cur);
    }

  g_array_remove_index (garray, 9);
  g_assert_cmpint (num_clear_func_invocations, ==, 1);

  g_array_remove_range (garray, 5, 3);
  g_assert_cmpint (num_clear_func_invocations, ==, 4);

  g_array_remove_index_fast (garray, 4);
  g_assert_cmpint (num_clear_func_invocations, ==, 5);

  g_array_free (garray, TRUE);
  g_assert_cmpint (num_clear_func_invocations, ==, 10);
}

/* Defining a comparison function for testing g_array_binary_search() */
static gint
cmpint (gconstpointer a, gconstpointer b)
{
  const gint *_a = a;
  const gint *_b = b;

  return *_a - *_b;
}

/* Testing g_array_binary_search() function */
static void
test_array_binary_search (void)
{
  GArray *garray;
  guint i, matched_index;

  if (g_test_undefined ())
    {
      /* Testing degenerated cases */
      garray = g_array_sized_new (FALSE, FALSE, sizeof (guint), 0);
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      i = 1;
      g_assert_false (g_array_binary_search (NULL, &i, cmpint, NULL));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      i = 1;
      g_assert_false (g_array_binary_search (garray, &i, NULL, NULL));
      g_test_assert_expected_messages ();
      g_array_free (garray, TRUE);
    }

  /* Testing array of size 0 */
  garray = g_array_sized_new (FALSE, FALSE, sizeof (guint), 0);

  i = 1;
  g_assert_false (g_array_binary_search (garray, &i, cmpint, NULL));

  g_array_free (garray, TRUE);

  /* Testing array of size 1 */
  garray = g_array_sized_new (FALSE, FALSE, sizeof (guint), 1);
  i = 1;
  g_array_append_val (garray, i);

  g_assert_true (g_array_binary_search (garray, &i, cmpint, NULL));

  i = 0;
  g_assert_false (g_array_binary_search (garray, &i, cmpint, NULL));

  i = 2;
  g_assert_false (g_array_binary_search (garray, &i, cmpint, NULL));

  g_array_free (garray, TRUE);

  /* Testing array of size 2 */
  garray = g_array_sized_new (FALSE, FALSE, sizeof (guint), 2);
  for (i = 1; i < 3; i++)
    g_array_append_val (garray, i);

  for (i = 1; i < 3; i++)
    g_assert_true (g_array_binary_search (garray, &i, cmpint, NULL));

  i = 0;
  g_assert_false (g_array_binary_search (garray, &i, cmpint, NULL));

  i = 4;
  g_assert_false (g_array_binary_search (garray, &i, cmpint, NULL));

  g_array_free (garray, TRUE);

  /* Testing array of size 3 */
  garray = g_array_sized_new (FALSE, FALSE, sizeof (guint), 3);
  for (i = 1; i < 4; i++)
    g_array_append_val (garray, i);

  for (i = 1; i < 4; i++)
    g_assert_true (g_array_binary_search (garray, &i, cmpint, NULL));

  i = 0;
  g_assert_false (g_array_binary_search (garray, &i, cmpint, NULL));

  i = 5;
  g_assert_false (g_array_binary_search (garray, &i, cmpint, NULL));

  g_array_free (garray, TRUE);

  /* Testing array of size 10000 */
  garray = g_array_sized_new (FALSE, FALSE, sizeof (guint), 10000);

  for (i = 1; i < 10001; i++)
    g_array_append_val (garray, i);

  for (i = 1; i < 10001; i++)
    g_assert_true (g_array_binary_search (garray, &i, cmpint, NULL));

  for (i = 1; i < 10001; i++)
    {
      g_assert_true (g_array_binary_search (garray, &i, cmpint, &matched_index));
      g_assert_cmpint (i, ==, matched_index + 1);
    }

  /* Testing negative result */
  i = 0;
  g_assert_false (g_array_binary_search (garray, &i, cmpint, NULL));
  g_assert_false (g_array_binary_search (garray, &i, cmpint, &matched_index));

  i = 10002;
  g_assert_false (g_array_binary_search (garray, &i, cmpint, NULL));
  g_assert_false (g_array_binary_search (garray, &i, cmpint, &matched_index));

  g_array_free (garray, TRUE);

  /* Test for a not-found element in the middle of the array. */
  garray = g_array_sized_new (FALSE, FALSE, sizeof (guint), 3);
  for (i = 1; i < 10; i += 2)
    g_array_append_val (garray, i);

  i = 0;
  g_assert_false (g_array_binary_search (garray, &i, cmpint, NULL));

  i = 2;
  g_assert_false (g_array_binary_search (garray, &i, cmpint, NULL));

  i = 10;
  g_assert_false (g_array_binary_search (garray, &i, cmpint, NULL));

  g_array_free (garray, TRUE);

  /* Test with an array sorted in descending order (illegal input). */
  garray = g_array_sized_new (FALSE, FALSE, sizeof (guint), 10000);

  for (i = 100; i > 0; i--)
    g_array_append_val (garray, i);

  i = 100;
  g_assert_false (g_array_binary_search (garray, &i, cmpint, NULL));

  i = 1;
  g_assert_false (g_array_binary_search (garray, &i, cmpint, NULL));

  g_array_free (garray, TRUE);
}

static void
test_array_copy_sized (void)
{
  GArray *array1 = NULL, *array2 = NULL, *array3 = NULL;
  int val = 5;

  g_test_summary ("Test that copying a newly-allocated sized array works.");

  array1 = g_array_sized_new (FALSE, FALSE, sizeof (int), 1);
  array2 = g_array_copy (array1);

  g_assert_cmpuint (array2->len, ==, array1->len);

  g_array_append_val (array1, val);
  array3 = g_array_copy (array1);

  g_assert_cmpuint (array3->len, ==, array1->len);
  g_assert_cmpuint (g_array_index (array3, int, 0), ==, g_array_index (array1, int, 0));
  g_assert_cmpuint (array3->len, ==, 1);
  g_assert_cmpuint (g_array_index (array3, int, 0), ==, val);

  g_array_unref (array3);
  g_array_unref (array2);
  g_array_unref (array1);
}

/* Check that copying does not exponentially grow array size. */
static void
test_array_copy_zero_terminated (void)
{
  GArray *array;

  array = g_array_new_take_zero_terminated (NULL, FALSE, 1);

  for (gint i = 0; i < 32; i++)
    {
      GArray *next;

      next = g_array_copy (array);
      g_array_unref (array);
      array = next;
    }

  g_array_unref (array);
}

static void
array_overflow_append_vals (void)
{
  if (!g_test_undefined ())
      return;

  if (g_test_subprocess ())
    {
      GArray *array = g_array_new (TRUE, FALSE, 1);
      /* Check for overflow should happen before data is accessed. */
      g_array_append_vals (array, NULL, G_MAXUINT);
    }
  else
    {
      g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
      g_test_trap_assert_failed ();
      g_test_trap_assert_stderr ("*adding 4294967295 to array would overflow*");
    }
}

static void
array_overflow_set_size (void)
{
  if (!g_test_undefined ())
      return;

  if (g_test_subprocess ())
    {
      GArray *array = g_array_new (TRUE, FALSE, 1);
      g_array_set_size (array, G_MAXUINT);
    }
  else
    {
      g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
      g_test_trap_assert_failed ();
      g_test_trap_assert_stderr ("*adding 4294967295 to array would overflow*");
    }
}

static void
assert_ptr_array_null_terminated (GPtrArray *array, gboolean null_terminated)
{
  g_assert_cmpint (null_terminated, ==, g_ptr_array_is_null_terminated (array));
  if (array->pdata)
    {
      if (null_terminated)
        g_assert_null (array->pdata[array->len]);
    }
  else
    g_assert_cmpint (array->len, ==, 0);
}

/* Check g_ptr_array_steal() function */
static void
pointer_array_steal (void)
{
  const guint array_size = 10000;
  GPtrArray *gparray;
  gpointer *pdata;
  guint i;
  gsize len, past_len;

  gparray = g_ptr_array_new ();
  pdata = g_ptr_array_steal (gparray, NULL);
  g_assert_null (pdata);

  pdata = g_ptr_array_steal (gparray, &len);
  g_assert_null (pdata);
  g_assert_cmpint (len, ==, 0);

  for (i = 0; i < array_size; i++)
    g_ptr_array_add (gparray, GINT_TO_POINTER (i));

  past_len = gparray->len;
  pdata = g_ptr_array_steal (gparray, &len);
  g_assert_cmpint (gparray->len, ==, 0);
  g_assert_cmpint (past_len, ==, len);
  g_ptr_array_add (gparray, GINT_TO_POINTER (10));

  g_assert_cmpint ((gsize) pdata[0], ==, (gsize) GINT_TO_POINTER (0));
  g_assert_cmpint ((gsize) g_ptr_array_index (gparray, 0), ==,
                   (gsize) GINT_TO_POINTER (10));
  g_assert_cmpint (gparray->len, ==, 1);

  g_ptr_array_remove_index (gparray, 0);

  for (i = 0; i < array_size; i++)
    g_ptr_array_add (gparray, GINT_TO_POINTER (i));
  g_assert_cmpmem (pdata, array_size * sizeof (gpointer),
                   gparray->pdata, array_size * sizeof (gpointer));
  g_free (pdata);

  g_ptr_array_free (gparray, TRUE);

  gparray = g_ptr_array_new_null_terminated (0, NULL, TRUE);
  pdata = g_ptr_array_steal (gparray, NULL);
  g_assert_null (pdata);
  g_ptr_array_unref (gparray);
}

static void
pointer_array_free_null_terminated (void)
{
  GPtrArray *parray = NULL;
  gpointer *segment;

  g_test_summary ("Check that g_ptr_array_free() on an empty array returns a NULL-terminated empty array");

  parray = g_ptr_array_new_null_terminated (0, NULL, TRUE);
  g_assert_nonnull (parray);
  assert_ptr_array_null_terminated (parray, TRUE);

  segment = g_ptr_array_free (parray, FALSE);
  g_assert_nonnull (segment);
  g_assert_null (segment[0]);

  g_free (segment);
}

static void
pointer_array_add (void)
{
  GPtrArray *gparray;
  gint i;
  gint sum = 0;
  gpointer *segment;

  gparray = g_ptr_array_sized_new (1000);

  for (i = 0; i < 10000; i++)
    g_ptr_array_add (gparray, GINT_TO_POINTER (i));

  for (i = 0; i < 10000; i++)
    g_assert (g_ptr_array_index (gparray, i) == GINT_TO_POINTER (i));
  
  g_ptr_array_foreach (gparray, sum_up, &sum);
  g_assert (sum == 49995000);

  segment = g_ptr_array_free (gparray, FALSE);
  for (i = 0; i < 10000; i++)
    g_assert (segment[i] == GINT_TO_POINTER (i));
  g_free (segment);
}

static void
pointer_array_insert (void)
{
  GPtrArray *gparray;
  gint i;
  gint sum = 0;
  gint index;

  gparray = g_ptr_array_sized_new (1000);

  for (i = 0; i < 10000; i++)
    {
      index = g_random_int_range (-1, i + 1);
      g_ptr_array_insert (gparray, index, GINT_TO_POINTER (i));
    }

  g_ptr_array_foreach (gparray, sum_up, &sum);
  g_assert (sum == 49995000);

  g_ptr_array_free (gparray, TRUE);
}

static void
pointer_array_new_take (void)
{
  const size_t array_size = 10000;
  GPtrArray *gparray;
  gpointer *pdata;
  gpointer *old_pdata_copy;
  gsize len;

  gparray = g_ptr_array_new ();
  for (size_t i = 0; i < array_size; i++)
    g_ptr_array_add (gparray, GUINT_TO_POINTER (i));

  pdata = g_ptr_array_steal (gparray, &len);
  g_assert_cmpuint (array_size, ==, len);
  g_assert_nonnull (pdata);
  g_clear_pointer (&gparray, g_ptr_array_unref);

  old_pdata_copy = g_memdup2 (pdata, len * sizeof (gpointer));
  gparray = g_ptr_array_new_take (g_steal_pointer (&pdata), len, NULL);
  g_assert_false (g_ptr_array_is_null_terminated (gparray));
  g_assert_cmpuint (gparray->len, ==, array_size);

  g_assert_cmpuint (GPOINTER_TO_UINT (g_ptr_array_index (gparray, 0)), ==, 0);
  g_assert_cmpuint (GPOINTER_TO_UINT (g_ptr_array_index (gparray, 10)), ==, 10);

  g_assert_cmpmem (old_pdata_copy, array_size * sizeof (gpointer),
                   gparray->pdata, array_size * sizeof (gpointer));

  g_ptr_array_add (gparray, GUINT_TO_POINTER (55));
  g_ptr_array_insert (gparray, 0, GUINT_TO_POINTER (33));

  g_assert_cmpuint (gparray->len, ==, array_size + 2);
  g_assert_cmpuint (GPOINTER_TO_UINT (g_ptr_array_index (gparray, 0)), ==, 33);
  g_assert_cmpuint (
    GPOINTER_TO_UINT (g_ptr_array_index (gparray, gparray->len - 1)), ==, 55);

  g_ptr_array_remove_index (gparray, 0);
  g_assert_cmpuint (gparray->len, ==, array_size + 1);
  g_ptr_array_remove_index (gparray, gparray->len - 1);
  g_assert_cmpuint (gparray->len, ==, array_size);

  g_assert_cmpmem (old_pdata_copy, array_size * sizeof (gpointer),
                   gparray->pdata, array_size * sizeof (gpointer));

  g_ptr_array_unref (gparray);
  g_free (old_pdata_copy);
}

static void
pointer_array_new_take_empty (void)
{
  GPtrArray *gparray;
  gpointer empty_array[] = {0};

  gparray = g_ptr_array_new_take (
    g_memdup2 (&empty_array, sizeof (gpointer)), 0, NULL);
  g_assert_false (g_ptr_array_is_null_terminated (gparray));
  g_assert_cmpuint (gparray->len, ==, 0);

  g_clear_pointer (&gparray, g_ptr_array_unref);

  gparray = g_ptr_array_new_take (NULL, 0, NULL);
  g_assert_false (g_ptr_array_is_null_terminated (gparray));
  g_assert_cmpuint (gparray->len, ==, 0);

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*data*!=*NULL*||*len*==*0*");
  g_assert_null (g_ptr_array_new_take (NULL, 10, NULL));
  g_test_assert_expected_messages ();

  g_clear_pointer (&gparray, g_ptr_array_unref);
}

static void
pointer_array_new_take_overflow (void)
{
#if SIZE_WIDTH <= UINT_WIDTH
  g_test_skip ("Overflow test requires SIZE_WIDTH > UINT_WIDTH.");
#else
  if (!g_test_undefined ())
      return;

  /* Check for overflow should happen before data is accessed. */
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion 'len <= G_MAXUINT' failed");
  g_assert_null (g_ptr_array_new_take (
    (gpointer []) { NULL }, (gsize) G_MAXUINT + 1, NULL));
  g_test_assert_expected_messages ();
#endif
}

static void
pointer_array_new_take_with_free_func (void)
{
  const size_t array_size = 10000;
  GPtrArray *gparray;
  gpointer *pdata;
  gpointer *old_pdata_copy;
  gsize len;

  gparray = g_ptr_array_new_with_free_func (g_free);
  for (size_t i = 0; i < array_size; i++)
    g_ptr_array_add (gparray, g_strdup_printf ("%" G_GSIZE_FORMAT, i));

  pdata = g_ptr_array_steal (gparray, &len);
  g_assert_cmpuint (array_size, ==, len);
  g_assert_nonnull (pdata);
  g_clear_pointer (&gparray, g_ptr_array_unref);

  old_pdata_copy = g_memdup2 (pdata, len * sizeof (gpointer));
  gparray = g_ptr_array_new_take (g_steal_pointer (&pdata), len, g_free);
  g_assert_false (g_ptr_array_is_null_terminated (gparray));
  g_assert_cmpuint (gparray->len, ==, array_size);

  g_assert_cmpstr ((const char *) g_ptr_array_index (gparray, 0), ==, "0");
  g_assert_cmpstr ((const char *) g_ptr_array_index (gparray, 101), ==, "101");

  g_assert_cmpmem (old_pdata_copy, array_size * sizeof (gpointer),
                   gparray->pdata, array_size * sizeof (gpointer));

  g_ptr_array_add (gparray, g_strdup_printf ("%d", 55));
  g_ptr_array_insert (gparray, 0, g_strdup_printf ("%d", 33));

  g_assert_cmpuint (gparray->len, ==, array_size + 2);
  g_assert_cmpstr ((const char *) g_ptr_array_index (gparray, 0), ==, "33");
  g_assert_cmpstr (
    (const char *) g_ptr_array_index (gparray, gparray->len - 1), ==, "55");

  g_ptr_array_remove_index (gparray, 0);
  g_assert_cmpuint (gparray->len, ==, array_size + 1);
  g_ptr_array_remove_index (gparray, gparray->len - 1);
  g_assert_cmpuint (gparray->len, ==, array_size);

  g_assert_cmpmem (old_pdata_copy, array_size * sizeof (gpointer),
                   gparray->pdata, array_size * sizeof (gpointer));

  g_ptr_array_unref (gparray);
  g_free (old_pdata_copy);
}

static void
pointer_array_new_take_null_terminated (void)
{
  const size_t array_size = 10000;
  GPtrArray *gparray;
  gpointer *pdata;
  gpointer *old_pdata_copy;
  gsize len;

  gparray = g_ptr_array_new_null_terminated (array_size, NULL, TRUE);
  g_assert_true (g_ptr_array_is_null_terminated (gparray));

  for (size_t i = 0; i < array_size; i++)
    g_ptr_array_add (gparray, GUINT_TO_POINTER (i + 1));

  assert_ptr_array_null_terminated (gparray, TRUE);
  pdata = g_ptr_array_steal (gparray, &len);
  g_assert_cmpuint (array_size, ==, len);
  g_assert_nonnull (pdata);
  g_clear_pointer (&gparray, g_ptr_array_unref);

  old_pdata_copy = g_memdup2 (pdata, len * sizeof (gpointer));
  gparray = g_ptr_array_new_take_null_terminated (g_steal_pointer (&pdata), NULL);
  g_assert_true (g_ptr_array_is_null_terminated (gparray));
  assert_ptr_array_null_terminated (gparray, TRUE);
  g_assert_cmpuint (gparray->len, ==, array_size);

  g_assert_cmpuint (GPOINTER_TO_UINT (g_ptr_array_index (gparray, 0)), ==, 1);
  g_assert_cmpuint (GPOINTER_TO_UINT (g_ptr_array_index (gparray, 10)), ==, 11);

  g_assert_cmpmem (old_pdata_copy, array_size * sizeof (gpointer),
                   gparray->pdata, array_size * sizeof (gpointer));

  g_ptr_array_add (gparray, GUINT_TO_POINTER (55));
  assert_ptr_array_null_terminated (gparray, TRUE);

  g_ptr_array_insert (gparray, 0, GUINT_TO_POINTER (33));
  assert_ptr_array_null_terminated (gparray, TRUE);

  g_assert_cmpuint (gparray->len, ==, array_size + 2);
  g_assert_cmpuint (GPOINTER_TO_UINT (g_ptr_array_index (gparray, 0)), ==, 33);
  g_assert_cmpuint (
    GPOINTER_TO_UINT (g_ptr_array_index (gparray, gparray->len - 1)), ==, 55);

  g_ptr_array_remove_index (gparray, 0);
  g_assert_cmpuint (gparray->len, ==, array_size + 1);
  assert_ptr_array_null_terminated (gparray, TRUE);

  g_ptr_array_remove_index (gparray, gparray->len - 1);
  g_assert_cmpuint (gparray->len, ==, array_size);
  assert_ptr_array_null_terminated (gparray, TRUE);

  g_assert_cmpmem (old_pdata_copy, array_size * sizeof (gpointer),
                   gparray->pdata, array_size * sizeof (gpointer));

  g_ptr_array_unref (gparray);
  g_free (old_pdata_copy);
}

static void
pointer_array_new_take_null_terminated_empty (void)
{
  GPtrArray *gparray;
  const gpointer *data = (gpointer []) { NULL };

  gparray = g_ptr_array_new_take_null_terminated (
    g_memdup2 (data, sizeof (gpointer)), NULL);
  g_assert_true (g_ptr_array_is_null_terminated (gparray));
  assert_ptr_array_null_terminated (gparray, TRUE);
  g_assert_cmpuint (gparray->len, ==, 0);

  g_clear_pointer (&gparray, g_ptr_array_unref);

  gparray = g_ptr_array_new_take_null_terminated (NULL, NULL);
  g_assert_true (g_ptr_array_is_null_terminated (gparray));
  assert_ptr_array_null_terminated (gparray, TRUE);
  g_assert_cmpuint (gparray->len, ==, 0);

  g_clear_pointer (&gparray, g_ptr_array_unref);
}

static void
pointer_array_new_take_null_terminated_with_free_func (void)
{
  const size_t array_size = 10000;
  GPtrArray *gparray;
  gpointer *pdata;
  gpointer *old_pdata_copy;
  gsize len;

  gparray = g_ptr_array_new_null_terminated (array_size, g_free, TRUE);
  g_assert_true (g_ptr_array_is_null_terminated (gparray));

  for (size_t i = 0; i < array_size; i++)
    g_ptr_array_add (gparray, g_strdup_printf ("%" G_GSIZE_FORMAT, i));

  assert_ptr_array_null_terminated (gparray, TRUE);

  pdata = g_ptr_array_steal (gparray, &len);
  g_assert_cmpuint (array_size, ==, len);
  g_assert_nonnull (pdata);
  g_clear_pointer (&gparray, g_ptr_array_unref);

  old_pdata_copy = g_memdup2 (pdata, len * sizeof (gpointer));
  gparray = g_ptr_array_new_take_null_terminated (g_steal_pointer (&pdata), g_free);
  g_assert_true (g_ptr_array_is_null_terminated (gparray));
  assert_ptr_array_null_terminated (gparray, TRUE);
  g_assert_cmpuint (gparray->len, ==, array_size);

  g_assert_cmpstr ((const char *) g_ptr_array_index (gparray, 0), ==, "0");
  g_assert_cmpstr ((const char *) g_ptr_array_index (gparray, 101), ==, "101");

  g_assert_cmpmem (old_pdata_copy, array_size * sizeof (gpointer),
                   gparray->pdata, array_size * sizeof (gpointer));

  g_ptr_array_add (gparray, g_strdup_printf ("%d", 55));
  assert_ptr_array_null_terminated (gparray, TRUE);

  g_ptr_array_insert (gparray, 0, g_strdup_printf ("%d", 33));
  assert_ptr_array_null_terminated (gparray, TRUE);

  g_assert_cmpuint (gparray->len, ==, array_size + 2);
  g_assert_cmpstr ((const char *) g_ptr_array_index (gparray, 0), ==, "33");
  g_assert_cmpstr (
    (const char *) g_ptr_array_index (gparray, gparray->len - 1), ==, "55");

  g_ptr_array_remove_index (gparray, 0);
  g_assert_cmpuint (gparray->len, ==, array_size + 1);
  assert_ptr_array_null_terminated (gparray, TRUE);

  g_ptr_array_remove_index (gparray, gparray->len - 1);
  g_assert_cmpuint (gparray->len, ==, array_size);
  assert_ptr_array_null_terminated (gparray, TRUE);

  g_assert_cmpmem (old_pdata_copy, array_size * sizeof (gpointer),
                   gparray->pdata, array_size * sizeof (gpointer));

  g_ptr_array_unref (gparray);
  g_free (old_pdata_copy);
}

static void
pointer_array_new_take_null_terminated_from_gstrv (void)
{
  GPtrArray *gparray;
  char *joined;

  gparray = g_ptr_array_new_take_null_terminated (
    (gpointer) g_strsplit ("A.dot.separated.string", ".", -1), g_free);

  g_assert_cmpstr (
    (const char *) g_ptr_array_index (gparray, 0), ==, "A");
  g_assert_cmpstr (
    (const char *) g_ptr_array_index (gparray, 1), ==, "dot");
  g_assert_cmpstr (
    (const char *) g_ptr_array_index (gparray, 2), ==, "separated");
  g_assert_cmpstr (
    (const char *) g_ptr_array_index (gparray, 3), ==, "string");

  g_assert_null (g_ptr_array_index (gparray, 4));

  joined = g_strjoinv (".", (char **) gparray->pdata);
  g_assert_cmpstr (joined, ==, "A.dot.separated.string");

  g_ptr_array_unref (gparray);
  g_free (joined);
}

static void
pointer_array_new_from_array (void)
{
  const size_t array_size = 10000;
  GPtrArray *source_array;
  GPtrArray *gparray;
  gpointer *old_pdata_copy;

  source_array = g_ptr_array_new ();
  for (size_t i = 0; i < array_size; i++)
    g_ptr_array_add (source_array, GUINT_TO_POINTER (i));

  g_assert_cmpuint (array_size, ==, source_array->len);
  g_assert_nonnull (source_array->pdata);

  gparray = g_ptr_array_new_from_array (source_array->pdata, source_array->len,
                                        NULL, NULL, NULL);

  old_pdata_copy =
    g_memdup2 (source_array->pdata, source_array->len * sizeof (gpointer));
  g_assert_nonnull (old_pdata_copy);
  g_clear_pointer (&source_array, g_ptr_array_unref);

  g_assert_false (g_ptr_array_is_null_terminated (gparray));
  g_assert_cmpuint (gparray->len, ==, array_size);

  g_assert_cmpuint (GPOINTER_TO_UINT (g_ptr_array_index (gparray, 0)), ==, 0);
  g_assert_cmpuint (GPOINTER_TO_UINT (g_ptr_array_index (gparray, 10)), ==, 10);

  g_assert_cmpmem (old_pdata_copy, array_size * sizeof (gpointer),
                   gparray->pdata, array_size * sizeof (gpointer));

  g_ptr_array_add (gparray, GUINT_TO_POINTER (55));
  g_ptr_array_insert (gparray, 0, GUINT_TO_POINTER (33));

  g_assert_cmpuint (gparray->len, ==, array_size + 2);
  g_assert_cmpuint (GPOINTER_TO_UINT (g_ptr_array_index (gparray, 0)), ==, 33);
  g_assert_cmpuint (
    GPOINTER_TO_UINT (g_ptr_array_index (gparray, gparray->len - 1)), ==, 55);

  g_ptr_array_remove_index (gparray, 0);
  g_assert_cmpuint (gparray->len, ==, array_size + 1);
  g_ptr_array_remove_index (gparray, gparray->len - 1);
  g_assert_cmpuint (gparray->len, ==, array_size);

  g_assert_cmpmem (old_pdata_copy, array_size * sizeof (gpointer),
                   gparray->pdata, array_size * sizeof (gpointer));

  g_ptr_array_unref (gparray);
  g_free (old_pdata_copy);
}

static void
pointer_array_new_from_array_empty (void)
{
  GPtrArray *gparray;
  gpointer empty_array[] = {0};

  gparray = g_ptr_array_new_from_array (empty_array, 0, NULL, NULL, NULL);
  g_assert_false (g_ptr_array_is_null_terminated (gparray));
  g_assert_cmpuint (gparray->len, ==, 0);

  g_clear_pointer (&gparray, g_ptr_array_unref);

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*data*!=*NULL*||*len*==*0*");
  g_assert_null (g_ptr_array_new_from_array (NULL, 10, NULL, NULL, NULL));
  g_test_assert_expected_messages ();
}

static void
pointer_array_new_from_array_overflow (void)
{
#if SIZE_WIDTH <= UINT_WIDTH
  g_test_skip ("Overflow test requires SIZE_WIDTH > UINT_WIDTH.");
#else
  if (!g_test_undefined ())
      return;

  /* Check for overflow should happen before data is accessed. */
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion 'len <= G_MAXUINT' failed");
  g_assert_null (g_ptr_array_new_from_array (
    (gpointer []) { NULL }, (gsize) G_MAXUINT + 1, NULL, NULL, NULL));
  g_test_assert_expected_messages ();
#endif
}

static void
pointer_array_new_from_array_with_copy_and_free_func (void)
{
  const size_t array_size = 10000;
  GPtrArray *source_array;
  GPtrArray *gparray;
  gpointer *old_pdata_copy;

  source_array = g_ptr_array_new_with_free_func (g_free);
  for (size_t i = 0; i < array_size; i++)
    g_ptr_array_add (source_array, g_strdup_printf ("%" G_GSIZE_FORMAT, i));

  g_assert_cmpuint (array_size, ==, source_array->len);
  g_assert_nonnull (source_array->pdata);

  gparray = g_ptr_array_new_from_array (source_array->pdata, source_array->len,
                                        (GCopyFunc) g_strdup, NULL, g_free);

  old_pdata_copy =
    g_memdup2 (source_array->pdata, source_array->len * sizeof (gpointer));
  g_assert_nonnull (old_pdata_copy);

  for (size_t i = 0; i < gparray->len; i++)
    {
      g_assert_cmpstr ((const char *) g_ptr_array_index (gparray, i), ==,
                       (const char *) old_pdata_copy[i]);
    }

  g_clear_pointer (&source_array, g_ptr_array_unref);

  g_assert_cmpstr ((const char *) g_ptr_array_index (gparray, 0), ==, "0");
  g_assert_cmpstr ((const char *) g_ptr_array_index (gparray, 101), ==, "101");

  g_ptr_array_add (gparray, g_strdup_printf ("%d", 55));
  g_ptr_array_insert (gparray, 0, g_strdup_printf ("%d", 33));

  g_assert_cmpuint (gparray->len, ==, array_size + 2);
  g_assert_cmpstr ((const char *) g_ptr_array_index (gparray, 0), ==, "33");
  g_assert_cmpstr (
    (const char *) g_ptr_array_index (gparray, gparray->len - 1), ==, "55");

  g_ptr_array_remove_index (gparray, 0);
  g_assert_cmpuint (gparray->len, ==, array_size + 1);
  g_ptr_array_remove_index (gparray, gparray->len - 1);
  g_assert_cmpuint (gparray->len, ==, array_size);

  g_ptr_array_unref (gparray);
  g_free (old_pdata_copy);
}

static void
pointer_array_new_from_null_terminated_array (void)
{
  const size_t array_size = 10000;
  GPtrArray *source_array;
  GPtrArray *gparray;
  gpointer *old_pdata_copy;

  source_array = g_ptr_array_new_null_terminated (array_size, NULL, TRUE);
  g_assert_true (g_ptr_array_is_null_terminated (source_array));

  for (size_t i = 0; i < array_size; i++)
    g_ptr_array_add (source_array, GUINT_TO_POINTER (i + 1));

  g_assert_cmpuint (array_size, ==, source_array->len);
  g_assert_nonnull (source_array->pdata);

  old_pdata_copy =
    g_memdup2 (source_array->pdata, source_array->len * sizeof (gpointer));
  g_assert_nonnull (old_pdata_copy);

  gparray = g_ptr_array_new_from_null_terminated_array (source_array->pdata,
                                                        NULL, NULL, NULL);
  g_assert_true (g_ptr_array_is_null_terminated (source_array));
  assert_ptr_array_null_terminated (gparray, TRUE);

  g_clear_pointer (&source_array, g_ptr_array_unref);

  g_assert_true (g_ptr_array_is_null_terminated (gparray));
  g_assert_cmpuint (gparray->len, ==, array_size);

  g_assert_cmpuint (GPOINTER_TO_UINT (g_ptr_array_index (gparray, 0)), ==, 1);
  g_assert_cmpuint (GPOINTER_TO_UINT (g_ptr_array_index (gparray, 10)), ==, 11);

  g_assert_cmpmem (old_pdata_copy, array_size * sizeof (gpointer),
                   gparray->pdata, array_size * sizeof (gpointer));

  g_ptr_array_add (gparray, GUINT_TO_POINTER (55));
  assert_ptr_array_null_terminated (gparray, TRUE);

  g_ptr_array_insert (gparray, 0, GUINT_TO_POINTER (33));
  assert_ptr_array_null_terminated (gparray, TRUE);

  g_assert_cmpuint (gparray->len, ==, array_size + 2);
  g_assert_cmpuint (GPOINTER_TO_UINT (g_ptr_array_index (gparray, 0)), ==, 33);
  g_assert_cmpuint (
    GPOINTER_TO_UINT (g_ptr_array_index (gparray, gparray->len - 1)), ==, 55);

  g_ptr_array_remove_index (gparray, 0);
  g_assert_cmpuint (gparray->len, ==, array_size + 1);
  assert_ptr_array_null_terminated (gparray, TRUE);

  g_ptr_array_remove_index (gparray, gparray->len - 1);
  g_assert_cmpuint (gparray->len, ==, array_size);
  assert_ptr_array_null_terminated (gparray, TRUE);

  g_assert_cmpmem (old_pdata_copy, array_size * sizeof (gpointer),
                   gparray->pdata, array_size * sizeof (gpointer));

  g_ptr_array_unref (gparray);
  g_free (old_pdata_copy);
}

static void
pointer_array_new_from_null_terminated_array_empty (void)
{
  GPtrArray *gparray;

  gparray = g_ptr_array_new_from_null_terminated_array (
    (gpointer []) { NULL }, NULL, NULL, NULL);
  g_assert_true (g_ptr_array_is_null_terminated (gparray));
  assert_ptr_array_null_terminated (gparray, TRUE);
  g_assert_cmpuint (gparray->len, ==, 0);

  g_clear_pointer (&gparray, g_ptr_array_unref);

  gparray = g_ptr_array_new_from_null_terminated_array (
    NULL, NULL, NULL, NULL);
  g_assert_true (g_ptr_array_is_null_terminated (gparray));
  assert_ptr_array_null_terminated (gparray, TRUE);
  g_assert_cmpuint (gparray->len, ==, 0);

  g_clear_pointer (&gparray, g_ptr_array_unref);
}

static void
pointer_array_new_from_null_terminated_array_with_copy_and_free_func (void)
{
  const size_t array_size = 10000;
  GPtrArray *source_array;
  GPtrArray *gparray;
  GStrv old_pdata_copy;

  source_array = g_ptr_array_new_null_terminated (array_size, g_free, TRUE);
  g_assert_true (g_ptr_array_is_null_terminated (source_array));

  for (size_t i = 0; i < array_size; i++)
    g_ptr_array_add (source_array, g_strdup_printf ("%" G_GSIZE_FORMAT, i));

  g_assert_cmpuint (array_size, ==, source_array->len);
  g_assert_nonnull (source_array->pdata);

  old_pdata_copy = g_strdupv ((char **) source_array->pdata);
  g_assert_cmpuint (g_strv_length (old_pdata_copy), ==, array_size);
  g_assert_nonnull (old_pdata_copy);
  g_clear_pointer (&source_array, g_ptr_array_unref);

  gparray = g_ptr_array_new_from_null_terminated_array (
    (gpointer* ) old_pdata_copy, (GCopyFunc) g_strdup, NULL, g_free);
  g_assert_true (g_ptr_array_is_null_terminated (gparray));
  assert_ptr_array_null_terminated (gparray, TRUE);

  for (size_t i = 0; i < gparray->len; i++)
    {
      g_assert_cmpstr ((const char *) g_ptr_array_index (gparray, i), ==,
                       (const char *) old_pdata_copy[i]);
    }

  g_assert_cmpstr ((const char *) g_ptr_array_index (gparray, 0), ==, "0");
  g_assert_cmpstr ((const char *) g_ptr_array_index (gparray, 101), ==, "101");

  g_ptr_array_add (gparray, g_strdup_printf ("%d", 55));
  assert_ptr_array_null_terminated (gparray, TRUE);

  g_ptr_array_insert (gparray, 0, g_strdup_printf ("%d", 33));
  assert_ptr_array_null_terminated (gparray, TRUE);

  g_assert_cmpuint (gparray->len, ==, array_size + 2);
  g_assert_cmpstr ((const char *) g_ptr_array_index (gparray, 0), ==, "33");
  g_assert_cmpstr (
    (const char *) g_ptr_array_index (gparray, gparray->len - 1), ==, "55");

  g_ptr_array_remove_index (gparray, 0);
  assert_ptr_array_null_terminated (gparray, TRUE);
  g_assert_cmpuint (gparray->len, ==, array_size + 1);

  g_ptr_array_remove_index (gparray, gparray->len - 1);
  assert_ptr_array_null_terminated (gparray, TRUE);
  g_assert_cmpuint (gparray->len, ==, array_size);

  for (size_t i = 0; i < gparray->len; i++)
    {
      g_assert_cmpstr ((const char *) g_ptr_array_index (gparray, i), ==,
                       (const char *) old_pdata_copy[i]);
    }

  g_ptr_array_unref (gparray);
  g_strfreev (old_pdata_copy);
}

static void
pointer_array_new_from_null_terminated_array_from_gstrv (void)
{
  GPtrArray *gparray;
  GStrv strv;
  char *joined;

  strv = g_strsplit ("A.dot.separated.string", ".", -1);
  gparray = g_ptr_array_new_from_null_terminated_array (
    (gpointer) strv, NULL, NULL, NULL);

  g_assert_cmpstr (
    (const char *) g_ptr_array_index (gparray, 0), ==, "A");
  g_assert_true (g_ptr_array_index (gparray, 0) == strv[0]);
  g_assert_cmpstr (
    (const char *) g_ptr_array_index (gparray, 1), ==, "dot");
  g_assert_true (g_ptr_array_index (gparray, 1) == strv[1]);
  g_assert_cmpstr (
    (const char *) g_ptr_array_index (gparray, 2), ==, "separated");
  g_assert_true (g_ptr_array_index (gparray, 2) == strv[2]);
  g_assert_cmpstr (
    (const char *) g_ptr_array_index (gparray, 3), ==, "string");
  g_assert_true (g_ptr_array_index (gparray, 3) == strv[3]);

  g_assert_null (strv[4]);
  g_assert_null (g_ptr_array_index (gparray, 4));

  joined = g_strjoinv (".", (char **) gparray->pdata);
  g_assert_cmpstr (joined, ==, "A.dot.separated.string");

  g_ptr_array_unref (gparray);
  g_strfreev (strv);
  g_free (joined);
}

static void
pointer_array_ref_count (gconstpointer test_data)
{
  const gboolean null_terminated = GPOINTER_TO_INT (test_data);
  GPtrArray *gparray;
  GPtrArray *gparray2;
  gint i;
  gint sum = 0;

  if (null_terminated)
    gparray = g_ptr_array_new_null_terminated (0, NULL, null_terminated);
  else
    gparray = g_ptr_array_new ();

  assert_ptr_array_null_terminated (gparray, null_terminated);

  for (i = 0; i < 10000; i++)
    {
      g_ptr_array_add (gparray, GINT_TO_POINTER (i));
      assert_ptr_array_null_terminated (gparray, null_terminated);
    }

  /* check we can ref, unref and still access the array */
  gparray2 = g_ptr_array_ref (gparray);
  g_assert (gparray == gparray2);
  g_ptr_array_unref (gparray2);
  for (i = 0; i < 10000; i++)
    g_assert (g_ptr_array_index (gparray, i) == GINT_TO_POINTER (i));

  assert_ptr_array_null_terminated (gparray, null_terminated);

  g_ptr_array_foreach (gparray, sum_up, &sum);
  g_assert (sum == 49995000);

  /* gparray2 should be an empty valid GPtrArray wrapper */
  gparray2 = g_ptr_array_ref (gparray);
  g_ptr_array_free (gparray, TRUE);

  g_assert_cmpint (gparray2->len, ==, 0);
  assert_ptr_array_null_terminated (gparray, null_terminated);

  g_ptr_array_unref (gparray2);
}

static gint num_free_func_invocations = 0;

static void
my_free_func (gpointer data)
{
  num_free_func_invocations++;
  g_free (data);
}

static void
pointer_array_free_func (void)
{
  GPtrArray *gparray;
  GPtrArray *gparray2;
  gchar **strv;
  gchar *s;

  num_free_func_invocations = 0;
  gparray = g_ptr_array_new_with_free_func (my_free_func);
  g_ptr_array_unref (gparray);
  g_assert_cmpint (num_free_func_invocations, ==, 0);

  gparray = g_ptr_array_new_with_free_func (my_free_func);
  g_ptr_array_free (gparray, TRUE);
  g_assert_cmpint (num_free_func_invocations, ==, 0);

  num_free_func_invocations = 0;
  gparray = g_ptr_array_new_with_free_func (my_free_func);
  g_ptr_array_add (gparray, g_strdup ("foo"));
  g_ptr_array_add (gparray, g_strdup ("bar"));
  g_ptr_array_add (gparray, g_strdup ("baz"));
  g_ptr_array_remove_index (gparray, 0);
  g_assert_cmpint (num_free_func_invocations, ==, 1);
  g_ptr_array_remove_index_fast (gparray, 1);
  g_assert_cmpint (num_free_func_invocations, ==, 2);
  s = g_strdup ("frob");
  g_ptr_array_add (gparray, s);
  g_assert (g_ptr_array_remove (gparray, s));
  g_assert (!g_ptr_array_remove (gparray, "nuun"));
  g_assert (!g_ptr_array_remove_fast (gparray, "mlo"));
  g_assert_cmpint (num_free_func_invocations, ==, 3);
  s = g_strdup ("frob");
  g_ptr_array_add (gparray, s);
  g_ptr_array_set_size (gparray, 1);
  g_assert_cmpint (num_free_func_invocations, ==, 4);
  g_ptr_array_ref (gparray);
  g_ptr_array_unref (gparray);
  g_assert_cmpint (num_free_func_invocations, ==, 4);
  g_ptr_array_unref (gparray);
  g_assert_cmpint (num_free_func_invocations, ==, 5);

  num_free_func_invocations = 0;
  gparray = g_ptr_array_new_full (10, my_free_func);
  g_ptr_array_add (gparray, g_strdup ("foo"));
  g_ptr_array_add (gparray, g_strdup ("bar"));
  g_ptr_array_add (gparray, g_strdup ("baz"));
  g_ptr_array_set_size (gparray, 20);
  g_ptr_array_add (gparray, NULL);
  gparray2 = g_ptr_array_ref (gparray);
  strv = (gchar **) g_ptr_array_free (gparray, FALSE);
  g_assert_cmpint (num_free_func_invocations, ==, 0);
  g_strfreev (strv);
  g_ptr_array_unref (gparray2);
  g_assert_cmpint (num_free_func_invocations, ==, 0);

  num_free_func_invocations = 0;
  gparray = g_ptr_array_new_with_free_func (my_free_func);
  g_ptr_array_add (gparray, g_strdup ("foo"));
  g_ptr_array_add (gparray, g_strdup ("bar"));
  g_ptr_array_add (gparray, g_strdup ("baz"));
  g_ptr_array_remove_range (gparray, 1, 1);
  g_ptr_array_unref (gparray);
  g_assert_cmpint (num_free_func_invocations, ==, 3);

  num_free_func_invocations = 0;
  gparray = g_ptr_array_new_with_free_func (my_free_func);
  g_ptr_array_add (gparray, g_strdup ("foo"));
  g_ptr_array_add (gparray, g_strdup ("bar"));
  g_ptr_array_add (gparray, g_strdup ("baz"));
  g_ptr_array_free (gparray, TRUE);
  g_assert_cmpint (num_free_func_invocations, ==, 3);

  num_free_func_invocations = 0;
  gparray = g_ptr_array_new_with_free_func (my_free_func);
  g_ptr_array_add (gparray, "foo");
  g_ptr_array_add (gparray, "bar");
  g_ptr_array_add (gparray, "baz");
  g_ptr_array_set_free_func (gparray, NULL);
  g_ptr_array_free (gparray, TRUE);
  g_assert_cmpint (num_free_func_invocations, ==, 0);
}

static gpointer
ptr_array_copy_func (gconstpointer src, gpointer userdata)
{
  gsize *dst = g_malloc (sizeof (gsize));
  *dst = *((gsize *) src);
  return dst;
}

/* Test the g_ptr_array_copy() function */
static void
pointer_array_copy (gconstpointer test_data)
{
  const gboolean null_terminated = GPOINTER_TO_INT (test_data);
  GPtrArray *ptr_array, *ptr_array2;
  gsize i;
  const gsize array_size = 100;
  gsize *array_test = g_malloc (array_size * sizeof (gsize));

  g_test_summary ("Check all normal behaviour of stealing elements from one "
                  "array to append to another, covering different array sizes "
                  "and element copy functions");

  if (g_test_undefined ())
    {
      /* Testing degenerated cases */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      ptr_array = g_ptr_array_copy (NULL, NULL, NULL);
      g_test_assert_expected_messages ();
      g_assert_cmpuint ((gsize) ptr_array, ==, (gsize) NULL);
    }

  /* Initializing array_test */
  for (i = 0; i < array_size; i++)
    array_test[i] = i;

  /* Test copy an empty array */
  ptr_array = g_ptr_array_new_null_terminated (0, NULL, null_terminated);
  ptr_array2 = g_ptr_array_copy (ptr_array, NULL, NULL);

  g_assert_cmpuint (ptr_array2->len, ==, ptr_array->len);
  assert_ptr_array_null_terminated (ptr_array, null_terminated);
  assert_ptr_array_null_terminated (ptr_array2, null_terminated);

  g_ptr_array_unref (ptr_array);
  g_ptr_array_unref (ptr_array2);

  /* Test simple copy */
  ptr_array = g_ptr_array_new_null_terminated (array_size, NULL, null_terminated);

  for (i = 0; i < array_size; i++)
    g_ptr_array_add (ptr_array, &array_test[i]);

  ptr_array2 = g_ptr_array_copy (ptr_array, NULL, NULL);

  g_assert_cmpuint (ptr_array2->len, ==, ptr_array->len);
  for (i = 0; i < array_size; i++)
    g_assert_cmpuint (*((gsize *) g_ptr_array_index (ptr_array2, i)), ==, i);

  for (i = 0; i < array_size; i++)
    g_assert_cmpuint ((gsize) g_ptr_array_index (ptr_array, i), ==,
                      (gsize) g_ptr_array_index (ptr_array2, i));

  assert_ptr_array_null_terminated (ptr_array, null_terminated);
  assert_ptr_array_null_terminated (ptr_array2, null_terminated);

  g_ptr_array_free (ptr_array2, TRUE);

  /* Test copy through GCopyFunc */
  ptr_array2 = g_ptr_array_copy (ptr_array, ptr_array_copy_func, NULL);
  g_ptr_array_set_free_func (ptr_array2, g_free);

  g_assert_cmpuint (ptr_array2->len, ==, ptr_array->len);
  for (i = 0; i < array_size; i++)
    g_assert_cmpuint (*((gsize *) g_ptr_array_index (ptr_array2, i)), ==, i);

  for (i = 0; i < array_size; i++)
    g_assert_cmpuint ((gsize) g_ptr_array_index (ptr_array, i), !=,
                      (gsize) g_ptr_array_index (ptr_array2, i));

  assert_ptr_array_null_terminated (ptr_array, null_terminated);
  assert_ptr_array_null_terminated (ptr_array2, null_terminated);

  g_ptr_array_free (ptr_array2, TRUE);

  /* Final cleanup */
  g_ptr_array_free (ptr_array, TRUE);
  g_free (array_test);
}

/* Test the g_ptr_array_extend() function */
static void
pointer_array_extend (gconstpointer test_data)
{
  gboolean null_terminated = GPOINTER_TO_INT (test_data);
  GPtrArray *ptr_array, *ptr_array2;
  gsize i;
  const gsize array_size = 100;
  gsize *array_test = g_malloc (array_size * sizeof (gsize));

  if (g_test_undefined ())
    {
      /* Testing degenerated cases */
      ptr_array = g_ptr_array_sized_new (0);
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_ptr_array_extend (NULL, ptr_array, NULL, NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_ptr_array_extend (ptr_array, NULL, NULL, NULL);
      g_test_assert_expected_messages ();

      g_ptr_array_unref (ptr_array);
    }

  /* Initializing array_test */
  for (i = 0; i < array_size; i++)
    array_test[i] = i;

  /* Testing extend with array of size zero */
  ptr_array = g_ptr_array_new_null_terminated (0, NULL, null_terminated);
  ptr_array2 = g_ptr_array_new_null_terminated (0, NULL, null_terminated);

  g_ptr_array_extend (ptr_array, ptr_array2, NULL, NULL);

  g_assert_cmpuint (ptr_array->len, ==, 0);
  g_assert_cmpuint (ptr_array2->len, ==, 0);

  assert_ptr_array_null_terminated (ptr_array, null_terminated);
  assert_ptr_array_null_terminated (ptr_array2, null_terminated);

  g_ptr_array_unref (ptr_array);
  g_ptr_array_unref (ptr_array2);

  /* Testing extend an array of size zero */
  ptr_array = g_ptr_array_new_null_terminated (array_size, NULL, null_terminated);
  ptr_array2 = g_ptr_array_new_null_terminated (0, NULL, null_terminated);

  for (i = 0; i < array_size; i++)
    {
      g_ptr_array_add (ptr_array, &array_test[i]);
    }

  g_ptr_array_extend (ptr_array, ptr_array2, NULL, NULL);

  for (i = 0; i < array_size; i++)
    g_assert_cmpuint (*((gsize *) g_ptr_array_index (ptr_array, i)), ==, i);

  assert_ptr_array_null_terminated (ptr_array, null_terminated);
  assert_ptr_array_null_terminated (ptr_array2, null_terminated);

  g_ptr_array_unref (ptr_array);
  g_ptr_array_unref (ptr_array2);

  /* Testing extend an array of size zero */
  ptr_array = g_ptr_array_new_null_terminated (0, NULL, null_terminated);
  ptr_array2 = g_ptr_array_new_null_terminated (array_size, NULL, null_terminated);

  for (i = 0; i < array_size; i++)
    {
      g_ptr_array_add (ptr_array2, &array_test[i]);
    }

  g_ptr_array_extend (ptr_array, ptr_array2, NULL, NULL);

  for (i = 0; i < array_size; i++)
    g_assert_cmpuint (*((gsize *) g_ptr_array_index (ptr_array, i)), ==, i);

  assert_ptr_array_null_terminated (ptr_array, null_terminated);
  assert_ptr_array_null_terminated (ptr_array2, null_terminated);

  g_ptr_array_unref (ptr_array);
  g_ptr_array_unref (ptr_array2);

  /* Testing simple extend */
  ptr_array = g_ptr_array_new_null_terminated (array_size / 2, NULL, null_terminated);
  ptr_array2 = g_ptr_array_new_null_terminated (array_size / 2, NULL, null_terminated);

  for (i = 0; i < array_size / 2; i++)
    {
      g_ptr_array_add (ptr_array, &array_test[i]);
      g_ptr_array_add (ptr_array2, &array_test[i + (array_size / 2)]);
    }

  g_ptr_array_extend (ptr_array, ptr_array2, NULL, NULL);

  for (i = 0; i < array_size; i++)
    g_assert_cmpuint (*((gsize *) g_ptr_array_index (ptr_array, i)), ==, i);

  assert_ptr_array_null_terminated (ptr_array, null_terminated);
  assert_ptr_array_null_terminated (ptr_array2, null_terminated);

  g_ptr_array_unref (ptr_array);
  g_ptr_array_unref (ptr_array2);

  /* Testing extend with GCopyFunc */
  ptr_array = g_ptr_array_new_null_terminated (array_size / 2, NULL, null_terminated);
  ptr_array2 = g_ptr_array_new_null_terminated (array_size / 2, NULL, null_terminated);

  for (i = 0; i < array_size / 2; i++)
    {
      g_ptr_array_add (ptr_array, &array_test[i]);
      g_ptr_array_add (ptr_array2, &array_test[i + (array_size / 2)]);
    }

  g_ptr_array_extend (ptr_array, ptr_array2, ptr_array_copy_func, NULL);

  for (i = 0; i < array_size; i++)
    g_assert_cmpuint (*((gsize *) g_ptr_array_index (ptr_array, i)), ==, i);

  assert_ptr_array_null_terminated (ptr_array, null_terminated);
  assert_ptr_array_null_terminated (ptr_array2, null_terminated);

  /* Clean-up memory */
  for (i = array_size / 2; i < array_size; i++)
    g_free (g_ptr_array_index (ptr_array, i));

  g_ptr_array_unref (ptr_array);
  g_ptr_array_unref (ptr_array2);
  g_free (array_test);
}

/* Test the g_ptr_array_extend_and_steal() function */
static void
pointer_array_extend_and_steal (void)
{
  GPtrArray *ptr_array, *ptr_array2, *ptr_array3;
  gsize i;
  const gsize array_size = 100;
  guintptr *array_test = g_malloc (array_size * sizeof (guintptr));

  if (g_test_undefined ())
    {
      /* Testing degenerated cases */
      ptr_array = g_ptr_array_sized_new (0);
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_ptr_array_extend_and_steal (NULL, ptr_array);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_ptr_array_extend_and_steal (ptr_array, NULL);
      g_test_assert_expected_messages ();

      g_ptr_array_unref (ptr_array);
    }

  /* Initializing array_test */
  for (i = 0; i < array_size; i++)
    array_test[i] = i;

  /* Testing simple extend_and_steal() */
  ptr_array = g_ptr_array_sized_new (array_size / 2);
  ptr_array2 = g_ptr_array_sized_new (array_size / 2);

  for (i = 0; i < array_size / 2; i++)
    {
      g_ptr_array_add (ptr_array, &array_test[i]);
      g_ptr_array_add (ptr_array2, &array_test[i + (array_size / 2)]);
    }

  g_ptr_array_extend_and_steal (ptr_array, ptr_array2);

  for (i = 0; i < array_size; i++)
    g_assert_cmpuint (*((guintptr *) g_ptr_array_index (ptr_array, i)), ==, i);

  g_ptr_array_free (ptr_array, TRUE);

  /* Testing extend_and_steal() with a pending reference to stolen array */
  ptr_array = g_ptr_array_sized_new (array_size / 2);
  ptr_array2 = g_ptr_array_sized_new (array_size / 2);

  for (i = 0; i < array_size / 2; i++)
    {
      g_ptr_array_add (ptr_array, &array_test[i]);
      g_ptr_array_add (ptr_array2, &array_test[i + (array_size / 2)]);
    }

  ptr_array3 = g_ptr_array_ref (ptr_array2);

  g_ptr_array_extend_and_steal (ptr_array, ptr_array2);

  for (i = 0; i < array_size; i++)
    g_assert_cmpuint (*((guintptr *) g_ptr_array_index (ptr_array, i)), ==, i);

  g_assert_cmpuint (ptr_array3->len, ==, 0);
  g_assert_null (ptr_array3->pdata);

  g_ptr_array_add (ptr_array2, NULL);

  g_ptr_array_free (ptr_array, TRUE);
  g_ptr_array_free (ptr_array3, TRUE);

  /* Final memory clean-up */
  g_free (array_test);
}

static gint
ptr_compare_values (gconstpointer p1, gconstpointer p2)
{
  return GPOINTER_TO_INT (p1) - GPOINTER_TO_INT (p2);
}

static gint
ptr_compare (gconstpointer p1, gconstpointer p2)
{
  gpointer i1 = *(gpointer*)p1;
  gpointer i2 = *(gpointer*)p2;

  return ptr_compare_values (i1, i2);
}

static gint
ptr_compare_values_data (gconstpointer p1, gconstpointer p2, gpointer data)
{
  return GPOINTER_TO_INT (p1) - GPOINTER_TO_INT (p2);
}

static gint
ptr_compare_data (gconstpointer p1, gconstpointer p2, gpointer data)
{
  gpointer i1 = *(gpointer*)p1;
  gpointer i2 = *(gpointer*)p2;

  return ptr_compare_values_data (i1, i2, data);
}

static void
pointer_array_sort (void)
{
  GPtrArray *gparray;
  gint i;
  gint val;
  gint prev, cur;

  gparray = g_ptr_array_new ();

  /* Sort empty array */
  g_ptr_array_sort (gparray, ptr_compare);

  for (i = 0; i < 10000; i++)
    {
      val = g_random_int_range (0, 10000);
      g_ptr_array_add (gparray, GINT_TO_POINTER (val));
    }

  g_ptr_array_sort (gparray, ptr_compare);

  prev = -1;
  for (i = 0; i < 10000; i++)
    {
      cur = GPOINTER_TO_INT (g_ptr_array_index (gparray, i));
      g_assert_cmpint (prev, <=, cur);
      prev = cur;
    }

  g_ptr_array_free (gparray, TRUE);
}

/* Please keep pointer_array_sort_example() in sync with the doc-comment
 * of g_ptr_array_sort() */

typedef struct
{
  gchar *name;
  gint size;
} FileListEntry;

static void
file_list_entry_free (gpointer p)
{
  FileListEntry *entry = p;

  g_free (entry->name);
  g_free (entry);
}

static gint
sort_filelist (gconstpointer a, gconstpointer b)
{
   const FileListEntry *entry1 = *((FileListEntry **) a);
   const FileListEntry *entry2 = *((FileListEntry **) b);

   return g_ascii_strcasecmp (entry1->name, entry2->name);
}

static void
pointer_array_sort_example (void)
{
  GPtrArray *file_list = NULL;
  FileListEntry *entry;

  g_test_summary ("Check that the doc-comment for g_ptr_array_sort() is correct");

  file_list = g_ptr_array_new_with_free_func (file_list_entry_free);

  entry = g_new0 (FileListEntry, 1);
  entry->name = g_strdup ("README");
  entry->size = 42;
  g_ptr_array_add (file_list, g_steal_pointer (&entry));

  entry = g_new0 (FileListEntry, 1);
  entry->name = g_strdup ("empty");
  entry->size = 0;
  g_ptr_array_add (file_list, g_steal_pointer (&entry));

  entry = g_new0 (FileListEntry, 1);
  entry->name = g_strdup ("aardvark");
  entry->size = 23;
  g_ptr_array_add (file_list, g_steal_pointer (&entry));

  g_ptr_array_sort (file_list, sort_filelist);

  g_assert_cmpuint (file_list->len, ==, 3);
  entry = g_ptr_array_index (file_list, 0);
  g_assert_cmpstr (entry->name, ==, "aardvark");
  entry = g_ptr_array_index (file_list, 1);
  g_assert_cmpstr (entry->name, ==, "empty");
  entry = g_ptr_array_index (file_list, 2);
  g_assert_cmpstr (entry->name, ==, "README");

  g_ptr_array_unref (file_list);
}

/* Please keep pointer_array_sort_with_data_example() in sync with the
 * doc-comment of g_ptr_array_sort_with_data() */

typedef enum { SORT_NAME, SORT_SIZE } SortMode;

static gint
sort_filelist_how (gconstpointer a, gconstpointer b, gpointer user_data)
{
  gint order;
  const SortMode sort_mode = GPOINTER_TO_INT (user_data);
  const FileListEntry *entry1 = *((FileListEntry **) a);
  const FileListEntry *entry2 = *((FileListEntry **) b);

  switch (sort_mode)
    {
    case SORT_NAME:
      order = g_ascii_strcasecmp (entry1->name, entry2->name);
      break;
    case SORT_SIZE:
      order = entry1->size - entry2->size;
      break;
    default:
      order = 0;
      break;
    }
  return order;
}

static void
pointer_array_sort_with_data_example (void)
{
  GPtrArray *file_list = NULL;
  FileListEntry *entry;
  SortMode sort_mode;

  g_test_summary ("Check that the doc-comment for g_ptr_array_sort_with_data() is correct");

  file_list = g_ptr_array_new_with_free_func (file_list_entry_free);

  entry = g_new0 (FileListEntry, 1);
  entry->name = g_strdup ("README");
  entry->size = 42;
  g_ptr_array_add (file_list, g_steal_pointer (&entry));

  entry = g_new0 (FileListEntry, 1);
  entry->name = g_strdup ("empty");
  entry->size = 0;
  g_ptr_array_add (file_list, g_steal_pointer (&entry));

  entry = g_new0 (FileListEntry, 1);
  entry->name = g_strdup ("aardvark");
  entry->size = 23;
  g_ptr_array_add (file_list, g_steal_pointer (&entry));

  sort_mode = SORT_NAME;
  g_ptr_array_sort_with_data (file_list, sort_filelist_how, GINT_TO_POINTER (sort_mode));

  g_assert_cmpuint (file_list->len, ==, 3);
  entry = g_ptr_array_index (file_list, 0);
  g_assert_cmpstr (entry->name, ==, "aardvark");
  entry = g_ptr_array_index (file_list, 1);
  g_assert_cmpstr (entry->name, ==, "empty");
  entry = g_ptr_array_index (file_list, 2);
  g_assert_cmpstr (entry->name, ==, "README");

  sort_mode = SORT_SIZE;
  g_ptr_array_sort_with_data (file_list, sort_filelist_how, GINT_TO_POINTER (sort_mode));

  g_assert_cmpuint (file_list->len, ==, 3);
  entry = g_ptr_array_index (file_list, 0);
  g_assert_cmpstr (entry->name, ==, "empty");
  entry = g_ptr_array_index (file_list, 1);
  g_assert_cmpstr (entry->name, ==, "aardvark");
  entry = g_ptr_array_index (file_list, 2);
  g_assert_cmpstr (entry->name, ==, "README");

  g_ptr_array_unref (file_list);
}

static void
pointer_array_sort_with_data (void)
{
  GPtrArray *gparray;
  gint i;
  gint prev, cur;

  gparray = g_ptr_array_new ();

  /* Sort empty array */
  g_ptr_array_sort_with_data (gparray, ptr_compare_data, NULL);

  for (i = 0; i < 10000; i++)
    g_ptr_array_add (gparray, GINT_TO_POINTER (g_random_int_range (0, 10000)));

  g_ptr_array_sort_with_data (gparray, ptr_compare_data, NULL);

  prev = -1;
  for (i = 0; i < 10000; i++)
    {
      cur = GPOINTER_TO_INT (g_ptr_array_index (gparray, i));
      g_assert_cmpint (prev, <=, cur);
      prev = cur;
    }

  g_ptr_array_free (gparray, TRUE);
}

static void
pointer_array_sort_values (void)
{
  GPtrArray *gparray;
  gint i;
  gint val;
  gint prev, cur;

  gparray = g_ptr_array_new ();

  /* Sort empty array */
  g_ptr_array_sort_values (gparray, ptr_compare_values);

  for (i = 0; i < 10000; i++)
    {
      val = g_random_int_range (0, 10000);
      g_ptr_array_add (gparray, GINT_TO_POINTER (val));
    }

  g_ptr_array_sort_values (gparray, ptr_compare_values);

  prev = -1;
  for (i = 0; i < 10000; i++)
    {
      cur = GPOINTER_TO_INT (g_ptr_array_index (gparray, i));
      g_assert_cmpint (prev, <=, cur);
      prev = cur;
    }

  g_clear_pointer (&gparray, g_ptr_array_unref);

  gparray = g_ptr_array_new ();

  g_ptr_array_add (gparray, "dddd");
  g_ptr_array_add (gparray, "cccc");
  g_ptr_array_add (gparray, NULL);
  g_ptr_array_add (gparray, "bbbb");
  g_ptr_array_add (gparray, "aaaa");

  g_ptr_array_sort_values (gparray, (GCompareFunc) g_strcmp0);

  i = 0;
  g_assert_cmpstr (g_ptr_array_index (gparray, i++), ==, NULL);
  g_assert_cmpstr (g_ptr_array_index (gparray, i++), ==, "aaaa");
  g_assert_cmpstr (g_ptr_array_index (gparray, i++), ==, "bbbb");
  g_assert_cmpstr (g_ptr_array_index (gparray, i++), ==, "cccc");
  g_assert_cmpstr (g_ptr_array_index (gparray, i++), ==, "dddd");

  g_clear_pointer (&gparray, g_ptr_array_unref);
}

static gint
sort_filelist_values (gconstpointer a, gconstpointer b)
{
  const FileListEntry *entry1 = a;
  const FileListEntry *entry2 = b;

  return g_ascii_strcasecmp (entry1->name, entry2->name);
}

static void
pointer_array_sort_values_example (void)
{
  GPtrArray *file_list = NULL;
  FileListEntry *entry;

  file_list = g_ptr_array_new_with_free_func (file_list_entry_free);

  entry = g_new0 (FileListEntry, 1);
  entry->name = g_strdup ("README");
  entry->size = 42;
  g_ptr_array_add (file_list, g_steal_pointer (&entry));

  entry = g_new0 (FileListEntry, 1);
  entry->name = g_strdup ("empty");
  entry->size = 0;
  g_ptr_array_add (file_list, g_steal_pointer (&entry));

  entry = g_new0 (FileListEntry, 1);
  entry->name = g_strdup ("aardvark");
  entry->size = 23;
  g_ptr_array_add (file_list, g_steal_pointer (&entry));

  g_ptr_array_sort_values (file_list, sort_filelist_values);

  g_assert_cmpuint (file_list->len, ==, 3);
  entry = g_ptr_array_index (file_list, 0);
  g_assert_cmpstr (entry->name, ==, "aardvark");
  entry = g_ptr_array_index (file_list, 1);
  g_assert_cmpstr (entry->name, ==, "empty");
  entry = g_ptr_array_index (file_list, 2);
  g_assert_cmpstr (entry->name, ==, "README");

  g_ptr_array_unref (file_list);
}

static gint
sort_filelist_how_values (gconstpointer a, gconstpointer b, gpointer user_data)
{
  gint order;
  const SortMode sort_mode = GPOINTER_TO_INT (user_data);
  const FileListEntry *entry1 = a;
  const FileListEntry *entry2 = b;

  switch (sort_mode)
    {
    case SORT_NAME:
      order = g_ascii_strcasecmp (entry1->name, entry2->name);
      break;
    case SORT_SIZE:
      order = entry1->size - entry2->size;
      break;
    default:
      order = 0;
      break;
    }
  return order;
}

static void
pointer_array_sort_values_with_data_example (void)
{
  GPtrArray *file_list = NULL;
  FileListEntry *entry;
  SortMode sort_mode;

  file_list = g_ptr_array_new_with_free_func (file_list_entry_free);

  entry = g_new0 (FileListEntry, 1);
  entry->name = g_strdup ("README");
  entry->size = 42;
  g_ptr_array_add (file_list, g_steal_pointer (&entry));

  entry = g_new0 (FileListEntry, 1);
  entry->name = g_strdup ("empty");
  entry->size = 0;
  g_ptr_array_add (file_list, g_steal_pointer (&entry));

  entry = g_new0 (FileListEntry, 1);
  entry->name = g_strdup ("aardvark");
  entry->size = 23;
  g_ptr_array_add (file_list, g_steal_pointer (&entry));

  sort_mode = SORT_NAME;
  g_ptr_array_sort_values_with_data (file_list, sort_filelist_how_values,
                                     GINT_TO_POINTER (sort_mode));

  g_assert_cmpuint (file_list->len, ==, 3);
  entry = g_ptr_array_index (file_list, 0);
  g_assert_cmpstr (entry->name, ==, "aardvark");
  entry = g_ptr_array_index (file_list, 1);
  g_assert_cmpstr (entry->name, ==, "empty");
  entry = g_ptr_array_index (file_list, 2);
  g_assert_cmpstr (entry->name, ==, "README");

  sort_mode = SORT_SIZE;
  g_ptr_array_sort_values_with_data (file_list, sort_filelist_how_values,
                                     GINT_TO_POINTER (sort_mode));

  g_assert_cmpuint (file_list->len, ==, 3);
  entry = g_ptr_array_index (file_list, 0);
  g_assert_cmpstr (entry->name, ==, "empty");
  entry = g_ptr_array_index (file_list, 1);
  g_assert_cmpstr (entry->name, ==, "aardvark");
  entry = g_ptr_array_index (file_list, 2);
  g_assert_cmpstr (entry->name, ==, "README");

  g_ptr_array_unref (file_list);
}

static void
pointer_array_sort_values_with_data (void)
{
  GPtrArray *gparray;
  gint i;
  gint prev, cur;

  gparray = g_ptr_array_new ();

  /* Sort empty array */
  g_ptr_array_sort_values_with_data (gparray, ptr_compare_values_data, NULL);

  for (i = 0; i < 10000; i++)
    g_ptr_array_add (gparray, GINT_TO_POINTER (g_random_int_range (0, 10000)));

  g_ptr_array_sort_values_with_data (gparray, ptr_compare_values_data, NULL);

  prev = -1;
  for (i = 0; i < 10000; i++)
    {
      cur = GPOINTER_TO_INT (g_ptr_array_index (gparray, i));
      g_assert_cmpint (prev, <=, cur);
      prev = cur;
    }

  g_ptr_array_free (gparray, TRUE);
}

static void
pointer_array_find_empty (void)
{
  GPtrArray *array;
  guint idx;

  array = g_ptr_array_new ();

  g_assert_false (g_ptr_array_find (array, "some-value", NULL));  /* NULL index */
  g_assert_false (g_ptr_array_find (array, "some-value", &idx));  /* non-NULL index */
  g_assert_false (g_ptr_array_find_with_equal_func (array, "some-value", g_str_equal, NULL));  /* NULL index */
  g_assert_false (g_ptr_array_find_with_equal_func (array, "some-value", g_str_equal, &idx));  /* non-NULL index */

  g_ptr_array_free (array, TRUE);
}

static void
pointer_array_find_non_empty (void)
{
  GPtrArray *array;
  guint idx;
  const gchar *str_pointer = "static-string";

  array = g_ptr_array_new ();

  g_ptr_array_add (array, "some");
  g_ptr_array_add (array, "random");
  g_ptr_array_add (array, "values");
  g_ptr_array_add (array, "some");
  g_ptr_array_add (array, "duplicated");
  g_ptr_array_add (array, (gpointer) str_pointer);

  g_assert_true (g_ptr_array_find_with_equal_func (array, "random", g_str_equal, NULL));  /* NULL index */
  g_assert_true (g_ptr_array_find_with_equal_func (array, "random", g_str_equal, &idx));  /* non-NULL index */
  g_assert_cmpuint (idx, ==, 1);

  g_assert_true (g_ptr_array_find_with_equal_func (array, "some", g_str_equal, &idx));  /* duplicate element */
  g_assert_cmpuint (idx, ==, 0);

  g_assert_false (g_ptr_array_find_with_equal_func (array, "nope", g_str_equal, NULL));

  g_assert_true (g_ptr_array_find_with_equal_func (array, str_pointer, g_str_equal, &idx));
  g_assert_cmpuint (idx, ==, 5);
  idx = G_MAXUINT;
  g_assert_true (g_ptr_array_find_with_equal_func (array, str_pointer, NULL, &idx));  /* NULL equal func */
  g_assert_cmpuint (idx, ==, 5);
  idx = G_MAXUINT;
  g_assert_true (g_ptr_array_find (array, str_pointer, &idx));  /* NULL equal func */
  g_assert_cmpuint (idx, ==, 5);

  g_ptr_array_free (array, TRUE);
}

static void
pointer_array_remove_range (void)
{
  GPtrArray *parray = NULL;

  /* Try removing an empty range. */
  parray = g_ptr_array_new ();
  g_ptr_array_remove_range (parray, 0, 0);
  g_ptr_array_unref (parray);
}

static void
steal_destroy_notify (gpointer data)
{
  guint *counter = data;
  *counter = *counter + 1;
}

/* Test that g_ptr_array_steal_index() and g_ptr_array_steal_index_fast() can
 * remove elements from a pointer array without the #GDestroyNotify being called. */
static void
pointer_array_steal_index (gconstpointer test_data)
{
  const gboolean null_terminated = GPOINTER_TO_INT (test_data);
  guint i1 = 0, i2 = 0, i3 = 0, i4 = 0;
  gpointer out1, out2;
  GPtrArray *array;

  if (null_terminated)
    array = g_ptr_array_new_null_terminated (0, steal_destroy_notify, null_terminated);
  else
    array = g_ptr_array_new_with_free_func (steal_destroy_notify);

  assert_ptr_array_null_terminated (array, null_terminated);

  g_ptr_array_add (array, &i1);
  g_ptr_array_add (array, &i2);

  assert_ptr_array_null_terminated (array, null_terminated);

  g_ptr_array_add (array, &i3);
  g_ptr_array_add (array, &i4);

  g_assert_cmpuint (array->len, ==, 4);

  assert_ptr_array_null_terminated (array, null_terminated);

  /* Remove a single element. */
  out1 = g_ptr_array_steal_index (array, 0);
  g_assert_true (out1 == &i1);
  g_assert_cmpuint (i1, ==, 0);  /* should not have been destroyed */

  /* Following elements should have been moved down. */
  g_assert_cmpuint (array->len, ==, 3);
  g_assert_true (g_ptr_array_index (array, 0) == &i2);
  g_assert_true (g_ptr_array_index (array, 1) == &i3);
  g_assert_true (g_ptr_array_index (array, 2) == &i4);

  assert_ptr_array_null_terminated (array, null_terminated);

  /* Remove another element, quickly. */
  out2 = g_ptr_array_steal_index_fast (array, 0);
  g_assert_true (out2 == &i2);
  g_assert_cmpuint (i2, ==, 0);  /* should not have been destroyed */

  /* Last element should have been swapped in place. */
  g_assert_cmpuint (array->len, ==, 2);
  g_assert_true (g_ptr_array_index (array, 0) == &i4);
  g_assert_true (g_ptr_array_index (array, 1) == &i3);

  assert_ptr_array_null_terminated (array, null_terminated);

  /* Check that destroying the pointer array doesn’t affect the stolen elements. */
  g_ptr_array_unref (array);

  g_assert_cmpuint (i1, ==, 0);
  g_assert_cmpuint (i2, ==, 0);
  g_assert_cmpuint (i3, ==, 1);
  g_assert_cmpuint (i4, ==, 1);
}

static void
byte_array_new_take_overflow (void)
{
#if SIZE_WIDTH <= UINT_WIDTH
  g_test_skip ("Overflow test requires SIZE_WIDTH > UINT_WIDTH.");
#else
  GByteArray* arr;

  if (!g_test_undefined ())
      return;

  /* Check for overflow should happen before data is accessed. */
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                          "*assertion 'len <= G_MAXUINT' failed");
  arr = g_byte_array_new_take (NULL, (gsize)G_MAXUINT + 1);
  g_assert_null (arr);
  g_test_assert_expected_messages ();
#endif
}

static void
byte_array_steal (void)
{
  const guint array_size = 10000;
  GByteArray *gbarray;
  guint8 *bdata;
  guint i;
  gsize len, past_len;

  gbarray = g_byte_array_new ();
  bdata = g_byte_array_steal (gbarray, NULL);
  g_assert_cmpint ((gsize) bdata, ==, (gsize) gbarray->data);
  g_free (bdata);

  for (i = 0; i < array_size; i++)
    g_byte_array_append (gbarray, (guint8 *) "abcd", 4);

  past_len = gbarray->len;
  bdata = g_byte_array_steal (gbarray, &len);

  g_assert_cmpint (len, ==, past_len);
  g_assert_cmpint (gbarray->len, ==, 0);

  g_byte_array_append (gbarray, (guint8 *) "@", 1);

  g_assert_cmpint (bdata[0], ==, 'a');
  g_assert_cmpint (gbarray->data[0], ==, '@');
  g_assert_cmpint (gbarray->len, ==, 1);

  g_byte_array_remove_index (gbarray, 0);

  g_free (bdata);
  g_byte_array_free (gbarray, TRUE);
}

static void
byte_array_append (void)
{
  GByteArray *gbarray;
  gint i;
  guint8 *segment;

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion 'array' failed");
      g_assert_null (
          g_byte_array_append (NULL, (guint8 *) "abcd", 4));
      g_test_assert_expected_messages ();
    }

  gbarray = g_byte_array_sized_new (1000);
  for (i = 0; i < 10000; i++)
    g_byte_array_append (gbarray, (guint8*) "abcd", 4);

  for (i = 0; i < 10000; i++)
    {
      g_assert (gbarray->data[4*i] == 'a');
      g_assert (gbarray->data[4*i+1] == 'b');
      g_assert (gbarray->data[4*i+2] == 'c');
      g_assert (gbarray->data[4*i+3] == 'd');
    }

  segment = g_byte_array_free (gbarray, FALSE);

  for (i = 0; i < 10000; i++)
    {
      g_assert (segment[4*i] == 'a');
      g_assert (segment[4*i+1] == 'b');
      g_assert (segment[4*i+2] == 'c');
      g_assert (segment[4*i+3] == 'd');
    }

  g_free (segment);
}

static void
byte_array_prepend (void)
{
  GByteArray *gbarray;
  gint i;

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion 'array' failed");
      g_assert_null (
          g_byte_array_prepend (NULL, (guint8 *) "abcd", 4));
      g_test_assert_expected_messages ();
    }

  gbarray = g_byte_array_new ();
  g_byte_array_set_size (gbarray, 1000);

  for (i = 0; i < 10000; i++)
    g_byte_array_prepend (gbarray, (guint8*) "abcd", 4);

  for (i = 0; i < 10000; i++)
    {
      g_assert (gbarray->data[4*i] == 'a');
      g_assert (gbarray->data[4*i+1] == 'b');
      g_assert (gbarray->data[4*i+2] == 'c');
      g_assert (gbarray->data[4*i+3] == 'd');
    }

  g_byte_array_free (gbarray, TRUE);
}

static void
byte_array_set_size (void)
{
  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion 'array' failed");
      g_assert_null (
          g_byte_array_set_size (NULL, 1));
      g_test_assert_expected_messages ();
    }
}

static void
byte_array_ref_count (void)
{
  GByteArray *gbarray;
  GByteArray *gbarray2;
  gint i;

  gbarray = g_byte_array_new ();
  for (i = 0; i < 10000; i++)
    g_byte_array_append (gbarray, (guint8*) "abcd", 4);

  gbarray2 = g_byte_array_ref (gbarray);
  g_assert (gbarray2 == gbarray);
  g_byte_array_unref (gbarray2);
  for (i = 0; i < 10000; i++)
    {
      g_assert (gbarray->data[4*i] == 'a');
      g_assert (gbarray->data[4*i+1] == 'b');
      g_assert (gbarray->data[4*i+2] == 'c');
      g_assert (gbarray->data[4*i+3] == 'd');
    }

  gbarray2 = g_byte_array_ref (gbarray);
  g_assert (gbarray2 == gbarray);
  g_byte_array_free (gbarray, TRUE);
  g_assert_cmpint (gbarray2->len, ==, 0);
  g_byte_array_unref (gbarray2);
}

static void
byte_array_remove (void)
{
  GByteArray *gbarray;
  gint i;

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion 'array' failed");
      g_assert_null (
          g_byte_array_remove_index (NULL, 1));
      g_test_assert_expected_messages ();
    }

  gbarray = g_byte_array_new ();
  for (i = 0; i < 100; i++)
    g_byte_array_append (gbarray, (guint8*) "abcd", 4);

  g_assert_cmpint (gbarray->len, ==, 400);

  g_byte_array_remove_index (gbarray, 4);
  g_byte_array_remove_index (gbarray, 4);
  g_byte_array_remove_index (gbarray, 4);
  g_byte_array_remove_index (gbarray, 4);

  g_assert_cmpint (gbarray->len, ==, 396);

  for (i = 0; i < 99; i++)
    {
      g_assert (gbarray->data[4*i] == 'a');
      g_assert (gbarray->data[4*i+1] == 'b');
      g_assert (gbarray->data[4*i+2] == 'c');
      g_assert (gbarray->data[4*i+3] == 'd');
    }

  g_byte_array_free (gbarray, TRUE);
}

static void
byte_array_remove_fast (void)
{
  GByteArray *gbarray;
  gint i;

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion 'array' failed");
      g_assert_null (
          g_byte_array_remove_index_fast (NULL, 1));
      g_test_assert_expected_messages ();
    }

  gbarray = g_byte_array_new ();
  for (i = 0; i < 100; i++)
    g_byte_array_append (gbarray, (guint8*) "abcd", 4);

  g_assert_cmpint (gbarray->len, ==, 400);

  g_byte_array_remove_index_fast (gbarray, 4);
  g_byte_array_remove_index_fast (gbarray, 4);
  g_byte_array_remove_index_fast (gbarray, 4);
  g_byte_array_remove_index_fast (gbarray, 4);

  g_assert_cmpint (gbarray->len, ==, 396);

  for (i = 0; i < 99; i++)
    {
      g_assert (gbarray->data[4*i] == 'a');
      g_assert (gbarray->data[4*i+1] == 'b');
      g_assert (gbarray->data[4*i+2] == 'c');
      g_assert (gbarray->data[4*i+3] == 'd');
    }

  g_byte_array_free (gbarray, TRUE);
}

static void
byte_array_remove_range (void)
{
  GByteArray *gbarray;
  gint i;

  gbarray = g_byte_array_new ();
  for (i = 0; i < 100; i++)
    g_byte_array_append (gbarray, (guint8*) "abcd", 4);

  g_assert_cmpint (gbarray->len, ==, 400);

  g_byte_array_remove_range (gbarray, 12, 4);

  g_assert_cmpint (gbarray->len, ==, 396);

  for (i = 0; i < 99; i++)
    {
      g_assert (gbarray->data[4*i] == 'a');
      g_assert (gbarray->data[4*i+1] == 'b');
      g_assert (gbarray->data[4*i+2] == 'c');
      g_assert (gbarray->data[4*i+3] == 'd');
    }

  /* Ensure the entire array can be cleared, even when empty. */
  g_byte_array_remove_range (gbarray, 0, gbarray->len);
  g_byte_array_remove_range (gbarray, 0, gbarray->len);

  g_byte_array_free (gbarray, TRUE);
}

static int
byte_compare (gconstpointer p1, gconstpointer p2)
{
  const guint8 *i1 = p1;
  const guint8 *i2 = p2;

  return *i1 - *i2;
}

static int
byte_compare_data (gconstpointer p1, gconstpointer p2, gpointer data)
{
  const guint8 *i1 = p1;
  const guint8 *i2 = p2;

  return *i1 - *i2;
}

static void
byte_array_sort (void)
{
  GByteArray *gbarray;
  guint i;
  guint8 val;
  guint8 prev, cur;

  gbarray = g_byte_array_new ();
  for (i = 0; i < 100; i++)
    {
      val = 'a' + g_random_int_range (0, 26);
      g_byte_array_append (gbarray, (guint8*) &val, 1);
    }

  g_byte_array_sort (gbarray, byte_compare);

  prev = 'a';
  for (i = 0; i < gbarray->len; i++)
    {
      cur = gbarray->data[i];
      g_assert_cmpint (prev, <=, cur);
      prev = cur;
    }

  g_byte_array_free (gbarray, TRUE);
}

static void
byte_array_sort_with_data (void)
{
  GByteArray *gbarray;
  guint i;
  guint8 val;
  guint8 prev, cur;

  gbarray = g_byte_array_new ();
  for (i = 0; i < 100; i++)
    {
      val = 'a' + g_random_int_range (0, 26);
      g_byte_array_append (gbarray, (guint8*) &val, 1);
    }

  g_byte_array_sort_with_data (gbarray, byte_compare_data, NULL);

  prev = 'a';
  for (i = 0; i < gbarray->len; i++)
    {
      cur = gbarray->data[i];
      g_assert_cmpint (prev, <=, cur);
      prev = cur;
    }

  g_byte_array_free (gbarray, TRUE);
}

static void
byte_array_new_take (void)
{
  GByteArray *gbarray;
  guint8 *data;

  data = g_memdup2 ("woooweeewow", 11);
  gbarray = g_byte_array_new_take (data, 11);
  g_assert (gbarray->data == data);
  g_assert_cmpuint (gbarray->len, ==, 11);
  g_byte_array_free (gbarray, TRUE);
}

static void
byte_array_free_to_bytes (void)
{
  GByteArray *gbarray;
  gpointer memory;
  GBytes *bytes;
  gsize size;

  gbarray = g_byte_array_new ();
  g_byte_array_append (gbarray, (guint8 *)"woooweeewow", 11);
  memory = gbarray->data;

  bytes = g_byte_array_free_to_bytes (gbarray);
  g_assert (bytes != NULL);
  g_assert_cmpuint (g_bytes_get_size (bytes), ==, 11);
  g_assert (g_bytes_get_data (bytes, &size) == memory);
  g_assert_cmpuint (size, ==, 11);

  g_bytes_unref (bytes);
}

static void
add_array_test (const gchar         *test_path,
                const ArrayTestData *config,
                GTestDataFunc        test_func)
{
  gchar *test_name = NULL;

  test_name = g_strdup_printf ("%s/%s-%s",
                               test_path,
                               config->zero_terminated ? "zero-terminated" : "non-zero-terminated",
                               config->clear_ ? "clear" : "no-clear");
  g_test_add_data_func (test_name, config, test_func);
  g_free (test_name);
}

int
main (int argc, char *argv[])
{
  /* Test all possible combinations of g_array_new() parameters. */
  const ArrayTestData array_configurations[] =
    {
      { FALSE, FALSE },
      { FALSE, TRUE },
      { TRUE, FALSE },
      { TRUE, TRUE },
    };
  gsize i;

  g_test_init (&argc, &argv, NULL);

  /* array tests */
  g_test_add_func ("/array/new/zero-terminated", array_new_zero_terminated);
  g_test_add_func ("/array/new/take", array_new_take);
  g_test_add_func ("/array/new/take/empty", array_new_take_empty);
  g_test_add_func ("/array/new/take/overflow", array_new_take_overflow);
  g_test_add_func ("/array/new/take/zero-size", array_new_take_zero_size);
  g_test_add_func ("/array/new/take-zero-terminated", array_new_take_zero_terminated);
  g_test_add_func ("/array/new/take-zero-terminated/zero-size", array_new_take_zero_terminated_zero_size);
  g_test_add_func ("/array/new/take-zero-terminated/null", array_new_take_zero_terminated_null);
  g_test_add_func ("/array/ref-count", array_ref_count);
  g_test_add_func ("/array/steal", array_steal);
  g_test_add_func ("/array/clear-func", array_clear_func);
  g_test_add_func ("/array/binary-search", test_array_binary_search);
  g_test_add_func ("/array/copy/sized", test_array_copy_sized);
  g_test_add_func ("/array/copy/zero-terminated", test_array_copy_zero_terminated);
  g_test_add_func ("/array/overflow-append-vals", array_overflow_append_vals);
  g_test_add_func ("/array/overflow-set-size", array_overflow_set_size);
  g_test_add_func ("/array/remove-range/zero-terminated-null", array_remove_range_zero_terminated_null);
  g_test_add_func ("/array/set-size/zero-terminated-null", array_set_size_zero_terminated_null);

  for (i = 0; i < G_N_ELEMENTS (array_configurations); i++)
    {
      add_array_test ("/array/set-size", &array_configurations[i], array_set_size);
      add_array_test ("/array/set-size/sized", &array_configurations[i], array_set_size_sized);
      add_array_test ("/array/append-val", &array_configurations[i], array_append_val);
      add_array_test ("/array/prepend-val", &array_configurations[i], array_prepend_val);
      add_array_test ("/array/prepend-vals", &array_configurations[i], array_prepend_vals);
      add_array_test ("/array/insert-vals", &array_configurations[i], array_insert_vals);
      add_array_test ("/array/remove-index", &array_configurations[i], array_remove_index);
      add_array_test ("/array/remove-index-fast", &array_configurations[i], array_remove_index_fast);
      add_array_test ("/array/remove-range", &array_configurations[i], array_remove_range);
      add_array_test ("/array/copy", &array_configurations[i], array_copy);
      add_array_test ("/array/sort", &array_configurations[i], array_sort);
      add_array_test ("/array/sort-with-data", &array_configurations[i], array_sort_with_data);
    }

  /* pointer arrays */
  g_test_add_func ("/pointerarray/free/null-terminated", pointer_array_free_null_terminated);
  g_test_add_func ("/pointerarray/add", pointer_array_add);
  g_test_add_func ("/pointerarray/insert", pointer_array_insert);
  g_test_add_func ("/pointerarray/new-take", pointer_array_new_take);
  g_test_add_func ("/pointerarray/new-take/empty", pointer_array_new_take_empty);
  g_test_add_func ("/pointerarray/new-take/overflow", pointer_array_new_take_overflow);
  g_test_add_func ("/pointerarray/new-take/with-free-func", pointer_array_new_take_with_free_func);
  g_test_add_func ("/pointerarray/new-take-null-terminated", pointer_array_new_take_null_terminated);
  g_test_add_func ("/pointerarray/new-take-null-terminated/empty", pointer_array_new_take_null_terminated_empty);
  g_test_add_func ("/pointerarray/new-take-null-terminated/with-free-func", pointer_array_new_take_null_terminated_with_free_func);
  g_test_add_func ("/pointerarray/new-take-null-terminated/from-gstrv", pointer_array_new_take_null_terminated_from_gstrv);
  g_test_add_func ("/pointerarray/new-from-array", pointer_array_new_from_array);
  g_test_add_func ("/pointerarray/new-from-array/empty", pointer_array_new_from_array_empty);
  g_test_add_func ("/pointerarray/new-from-array/overflow", pointer_array_new_from_array_overflow);
  g_test_add_func ("/pointerarray/new-from-array/with-copy-and-free-func", pointer_array_new_from_array_with_copy_and_free_func);
  g_test_add_func ("/pointerarray/new-from-null-terminated-array", pointer_array_new_from_null_terminated_array);
  g_test_add_func ("/pointerarray/new-from-null-terminated-array/empty", pointer_array_new_from_null_terminated_array_empty);
  g_test_add_func ("/pointerarray/new-from-null-terminated-array/with-copy-and-free-func", pointer_array_new_from_null_terminated_array_with_copy_and_free_func);
  g_test_add_func ("/pointerarray/new-from-null-terminated-array/from-gstrv", pointer_array_new_from_null_terminated_array_from_gstrv);
  g_test_add_data_func ("/pointerarray/ref-count/not-null-terminated", GINT_TO_POINTER (0), pointer_array_ref_count);
  g_test_add_data_func ("/pointerarray/ref-count/null-terminated", GINT_TO_POINTER (1), pointer_array_ref_count);
  g_test_add_func ("/pointerarray/free-func", pointer_array_free_func);
  g_test_add_data_func ("/pointerarray/array_copy/not-null-terminated", GINT_TO_POINTER (0), pointer_array_copy);
  g_test_add_data_func ("/pointerarray/array_copy/null-terminated", GINT_TO_POINTER (1), pointer_array_copy);
  g_test_add_data_func ("/pointerarray/array_extend/not-null-terminated", GINT_TO_POINTER (0), pointer_array_extend);
  g_test_add_data_func ("/pointerarray/array_extend/null-terminated", GINT_TO_POINTER (1), pointer_array_extend);
  g_test_add_func ("/pointerarray/array_extend_and_steal", pointer_array_extend_and_steal);
  g_test_add_func ("/pointerarray/sort", pointer_array_sort);
  g_test_add_func ("/pointerarray/sort/example", pointer_array_sort_example);
  g_test_add_func ("/pointerarray/sort-with-data", pointer_array_sort_with_data);
  g_test_add_func ("/pointerarray/sort-with-data/example", pointer_array_sort_with_data_example);
  g_test_add_func ("/pointerarray/sort-values", pointer_array_sort_values);
  g_test_add_func ("/pointerarray/sort-values/example", pointer_array_sort_values_example);
  g_test_add_func ("/pointerarray/sort-values-with-data", pointer_array_sort_values_with_data);
  g_test_add_func ("/pointerarray/sort-values-with-data/example", pointer_array_sort_values_with_data_example);
  g_test_add_func ("/pointerarray/find/empty", pointer_array_find_empty);
  g_test_add_func ("/pointerarray/find/non-empty", pointer_array_find_non_empty);
  g_test_add_func ("/pointerarray/remove-range", pointer_array_remove_range);
  g_test_add_func ("/pointerarray/steal", pointer_array_steal);
  g_test_add_data_func ("/pointerarray/steal_index/not-null-terminated", GINT_TO_POINTER (0), pointer_array_steal_index);
  g_test_add_data_func ("/pointerarray/steal_index/null-terminated", GINT_TO_POINTER (1), pointer_array_steal_index);

  /* byte arrays */
  g_test_add_func ("/bytearray/steal", byte_array_steal);
  g_test_add_func ("/bytearray/append", byte_array_append);
  g_test_add_func ("/bytearray/prepend", byte_array_prepend);
  g_test_add_func ("/bytearray/remove", byte_array_remove);
  g_test_add_func ("/bytearray/remove-fast", byte_array_remove_fast);
  g_test_add_func ("/bytearray/remove-range", byte_array_remove_range);
  g_test_add_func ("/bytearray/ref-count", byte_array_ref_count);
  g_test_add_func ("/bytearray/set-size", byte_array_set_size);
  g_test_add_func ("/bytearray/sort", byte_array_sort);
  g_test_add_func ("/bytearray/sort-with-data", byte_array_sort_with_data);
  g_test_add_func ("/bytearray/new-take", byte_array_new_take);
  g_test_add_func ("/bytearray/new-take-overflow", byte_array_new_take_overflow);
  g_test_add_func ("/bytearray/free-to-bytes", byte_array_free_to_bytes);

  return g_test_run ();
}
