/* GLib testing framework examples and tests
 *
 * Copyright © 2011, 2014, 2024 Red Hat, Inc.
 * Copyright © 2022 Peter Bloomfield
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

#include <glib.h>
#include <stdlib.h>

#include "glib/glib-private.h"

static void
test_quark_basic (void)
{
  GQuark quark;
  const gchar *orig = "blargh";
  gchar *copy;
  const gchar *str;

  quark = g_quark_try_string ("no-such-quark");
  g_assert_cmpuint (quark, ==, 0);

  copy = g_strdup (orig);
  quark = g_quark_from_static_string (orig);
  g_assert_cmpuint (quark, !=, 0);
  g_assert_cmpuint (g_quark_from_string (orig), ==, quark);
  g_assert_cmpuint (g_quark_from_string (copy), ==, quark);
  g_assert_cmpuint (g_quark_try_string (orig), ==, quark);

  str = g_quark_to_string (quark);
  g_assert_cmpstr (str, ==, orig);

  g_free (copy);
}

static void
test_quark_string (void)
{
  const gchar *orig = "string1";
  gchar *copy;
  const gchar *str1;
  const gchar *str2;

  copy = g_strdup (orig);

  str1 = g_intern_static_string (orig);
  str2 = g_intern_string (copy);
  g_assert_cmpstr (str1, ==, str2);
  g_assert_true (str1 == str2);  /* also compare pointers */
  g_assert_cmpstr (str1, ==, orig);
  g_assert_true (str1 == orig);

  g_free (copy);
}

static void
test_dataset_basic (void)
{
  gpointer location = (gpointer)test_dataset_basic;
  gpointer other = (gpointer)test_quark_basic;
  gpointer data = "test1";
  gpointer ret;

  g_dataset_set_data (location, "test1", data);

  ret = g_dataset_get_data (location, "test1");
  g_assert_true (ret == data);

  ret = g_dataset_get_data (location, "test2");
  g_assert_null (ret);

  ret = g_dataset_get_data (other, "test1");
  g_assert_null (ret);

  g_dataset_set_data (location, "test1", "new-value");
  ret = g_dataset_get_data (location, "test1");
  g_assert_true (ret != data);

  g_dataset_remove_data (location, "test1");
  ret = g_dataset_get_data (location, "test1");
  g_assert_null (ret);

  ret = g_dataset_get_data (location, NULL);
  g_assert_null (ret);
}

static guint destroy_count;

static void
notify (gpointer data)
{
  destroy_count++;
}

static void
test_dataset_full (void)
{
  gpointer location = (gpointer)test_dataset_full;

  g_dataset_set_data_full (location, "test1", "test1", notify);

  destroy_count = 0;
  g_dataset_set_data (location, "test1", NULL);
  g_assert_cmpuint (destroy_count, ==, 1);

  g_dataset_set_data_full (location, "test1", "test1", notify);

  destroy_count = 0;
  g_dataset_remove_data (location, "test1");
  g_assert_cmpuint (destroy_count, ==, 1);

  g_dataset_set_data_full (location, "test1", "test1", notify);

  destroy_count = 0;
  g_dataset_remove_no_notify (location, "test1");
  g_assert_cmpuint (destroy_count, ==, 0);
}

static void
foreach (GQuark   id,
         gpointer data,
         gpointer user_data)
{
  guint *counter = user_data;

  *counter += 1;
}

static void
test_dataset_foreach (void)
{
  gpointer location = (gpointer)test_dataset_foreach;
  guint my_count;

  my_count = 0;
  g_dataset_set_data_full (location, "test1", "test1", notify);
  g_dataset_set_data_full (location, "test2", "test2", notify);
  g_dataset_set_data_full (location, "test3", "test3", notify);
  g_dataset_foreach (location, foreach, &my_count);
  g_assert_cmpuint (my_count, ==, 3);

  g_dataset_destroy (location);
}

static void
test_dataset_destroy (void)
{
  gpointer location = (gpointer)test_dataset_destroy;

  destroy_count = 0;
  g_dataset_set_data_full (location, "test1", "test1", notify);
  g_dataset_set_data_full (location, "test2", "test2", notify);
  g_dataset_set_data_full (location, "test3", "test3", notify);
  g_dataset_destroy (location);
  g_assert_cmpuint (destroy_count, ==, 3);
}

static void
test_dataset_id (void)
{
  gpointer location = (gpointer)test_dataset_id;
  gpointer other = (gpointer)test_quark_basic;
  gpointer data = "test1";
  gpointer ret;
  GQuark quark;

  quark = g_quark_from_string ("test1");

  g_dataset_id_set_data (location, quark, data);

  ret = g_dataset_id_get_data (location, quark);
  g_assert_true (ret == data);

  ret = g_dataset_id_get_data (location, g_quark_from_string ("test2"));
  g_assert_null (ret);

  ret = g_dataset_id_get_data (other, quark);
  g_assert_null (ret);

  g_dataset_id_set_data (location, quark, "new-value");
  ret = g_dataset_id_get_data (location, quark);
  g_assert_true (ret != data);

  g_dataset_id_remove_data (location, quark);
  ret = g_dataset_id_get_data (location, quark);
  g_assert_null (ret);

  ret = g_dataset_id_get_data (location, 0);
  g_assert_null (ret);
}

static GData *global_list;

static void
free_one (gpointer data)
{
  /* recurse */
  g_datalist_clear (&global_list);
}

static void
test_datalist_clear (void)
{
  g_datalist_init (&global_list);
  g_datalist_set_data_full (&global_list, "one", GINT_TO_POINTER (1), free_one);
  g_datalist_set_data_full (&global_list, "two", GINT_TO_POINTER (2), NULL);
  g_datalist_clear (&global_list);
  g_assert_null (global_list);
}

static void
test_datalist_basic (gconstpointer test_data)
{
  const gboolean HAS_MANY = GPOINTER_TO_INT (test_data);
  const GQuark BOGUS_QUARK = 1000000000u;
  GData *list = NULL;
  gpointer data;
  gpointer ret;
  guint i;

  g_datalist_init (&list);
  data = "one";

  if (HAS_MANY)
    {
      /* Add many entries. This ensures we cover the code path where GData uses
       * a lookup hash table. */
      for (i = 0; i < 200; i++)
        {
          /* pick a bogus GQuark. They work fine here. */
          g_datalist_id_set_data (&list, BOGUS_QUARK + 1u + i, data);
        }
    }

  g_datalist_set_data (&list, "one", data);
  ret = g_datalist_get_data (&list, "one");
  g_assert_true (ret == data);

  ret = g_datalist_get_data (&list, "two");
  g_assert_null (ret);

  ret = g_datalist_get_data (&list, NULL);
  g_assert_null (ret);

  /* It is not enforced that GQuark are valid. They are just numbers and work
   * too. */
  g_datalist_id_set_data (&list, BOGUS_QUARK, data);
  ret = g_datalist_id_get_data (&list, BOGUS_QUARK);
  g_assert_true (ret == data);

  /* Ensure that we don't find the BOGUS_QUARK when looking up by NULL. */
  ret = g_datalist_get_data (&list, NULL);
  g_assert_null (ret);

  g_datalist_clear (&list);
}

static void
test_datalist_id (void)
{
  GData *list = NULL;
  gpointer data;
  gpointer ret;

  g_datalist_init (&list);
  data = "one";
  g_datalist_id_set_data (&list, g_quark_from_string ("one"), data);
  ret = g_datalist_id_get_data (&list, g_quark_from_string ("one"));
  g_assert_true (ret == data);

  ret = g_datalist_id_get_data (&list, g_quark_from_string ("two"));
  g_assert_null (ret);

  ret = g_datalist_id_get_data (&list, 0);
  g_assert_null (ret);

  g_datalist_clear (&list);
}

static void
test_datalist_id_remove_multiple (void)
{
  /* Test that g_datalist_id_remove_multiple() removes all the keys it
   * is given. */
  GData *list = NULL;
  GQuark one = g_quark_from_static_string ("one");
  GQuark two = g_quark_from_static_string ("two");
  GQuark three = g_quark_from_static_string ("three");
  GQuark keys[] = {
    one,
    two,
    three,
  };

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/issues/2672");

  g_datalist_init (&list);
  g_datalist_id_set_data (&list, one, GINT_TO_POINTER (1));
  g_datalist_id_set_data (&list, two, GINT_TO_POINTER (2));
  g_datalist_id_set_data (&list, three, GINT_TO_POINTER (3));

  destroy_count = 0;
  g_datalist_foreach (&list, (GDataForeachFunc) notify, NULL);
  g_assert_cmpuint (destroy_count, ==, 3);

  g_datalist_id_remove_multiple (&list, keys, G_N_ELEMENTS (keys));

  destroy_count = 0;
  g_datalist_foreach (&list, (GDataForeachFunc) notify, NULL);
  g_assert_cmpuint (destroy_count, ==, 0);
}

static void
test_datalist_id_remove_multiple_resize (void)
{
  GQuark *quarks;
  GQuark *quarks2;
  gboolean *has;
  const guint N = 1000;
  const guint PRIME = 1048583u;
  guint i;
  char sbuf[100];
  GData *list = NULL;
  guint i_run;

  quarks = g_new (GQuark, N);
  quarks2 = g_new (GQuark, N);
  has = g_new0 (gboolean, N);

  for (i = 0; i < N; i++)
    {
      g_snprintf (sbuf, sizeof (sbuf), "%d", i);
      quarks[i] = g_quark_from_string (sbuf);
    }

  for (i = 0; i < N; i++)
    {
      g_datalist_id_set_data (&list, quarks[i], (gpointer) g_quark_to_string (quarks[i]));
      has[i] = TRUE;
    }

  /* Now we perform a list of random operations (remove/add quarks). */
  for (i_run = 0; TRUE; i_run++)
    {
      int MODE = ((guint) g_test_rand_int ()) % 6;
      guint n;
      guint j;

      n = ((guint) g_test_rand_int ()) % (N + 1);
      j = ((guint) g_test_rand_int ()) % N;

      if (i_run > 20)
        {
          /* After a few runs, we only remove elements, until the list
           * is empty. */
          if (!list)
            break;
          MODE = 0;
          if (i_run > 30)
            n = N;
        }

      switch (MODE)
        {
        case 0:
        case 1:
        case 2:
          /* Mode: add or remove a number of random quarks. */
          for (i = 0; i < n; i++)
            {
              j = (j + PRIME) % N;
              if (MODE == 0)
                {
                  g_datalist_id_remove_data (&list, quarks[j]);
                  has[j] = FALSE;
                }
              else
                {
                  g_datalist_id_set_data (&list, quarks[j], (gpointer) g_quark_to_string (quarks[j]));
                  has[j] = TRUE;
                }
            }
          break;
        case 3:
          /* Mode: remove a list of (random) quarks. */
          for (i = 0; i < n; i++)
            {
              j = (j + PRIME) % N;
              quarks2[i] = quarks[j];
              has[j] = FALSE;
            }

          g_datalist_id_remove_multiple (&list, quarks2, n);
          break;
        case 4:
          /* Mode: lookup string via g_datalist_get_data()*/
          for (i = 0; i < n; i++)
            {
              const char *data;
              const char *data2;
              const char *key;

              j = (j + PRIME) % N;

              key = g_quark_to_string (quarks[j]);

              data = g_datalist_id_get_data (&list, quarks[j]);
              data2 = g_datalist_get_data (&list, key);
              g_assert_true (data == data2);
              if (data)
                g_assert_true (data == key);
              g_assert_true ((!!data) == has[j]);
            }
          break;
        case 5:
          /* Fill/empty the list completely. */
          switch (((guint) g_test_rand_int ()) % 5)
            {
            case 0:
              g_datalist_clear (&list);
              for (i = 0; i < N; i++)
                has[i] = FALSE;
              break;
            case 1:
              for (i = 0; i < N; i++)
                {
                  j = (j + PRIME) % N;
                  g_datalist_id_set_data (&list, quarks[j], (gpointer) g_quark_to_string (quarks[j]));
                  has[i] = TRUE;
                }
              break;
            default:
              /* Most of the time we do nothing. The case where we fill/empty
               * the list entirely is less interesting. */
              break;
            }
        }
    }

  g_free (quarks);
  g_free (quarks2);
  g_free (has);
}

static void
destroy_func (gpointer data)
{
  destroy_count++;
  g_assert_cmpuint (GPOINTER_TO_UINT (data), ==, destroy_count);
}

static void
test_datalist_id_remove_multiple_destroy_order (void)
{
  /* Test that destroy-funcs are called in the order that the keys are
   * specified, not the order that they are found in the datalist. */
  GData *list = NULL;
  GQuark one = g_quark_from_static_string ("one");
  GQuark two = g_quark_from_static_string ("two");
  GQuark three = g_quark_from_static_string ("three");
  GQuark keys[] = {
    one,
    two,
    three,
  };

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/issues/2672");

  g_datalist_init (&list);

  g_datalist_id_set_data_full (&list, two, GUINT_TO_POINTER (2), destroy_func);
  g_datalist_id_set_data_full (&list, three, GUINT_TO_POINTER (3), destroy_func);
  g_datalist_id_set_data_full (&list, one, GUINT_TO_POINTER (1), destroy_func);

  destroy_count = 0;
  g_datalist_id_remove_multiple (&list, keys, G_N_ELEMENTS (keys));
  /* This verifies that destroy_func() was called three times: */
  g_assert_cmpuint (destroy_count, ==, 3);
}

static gpointer
_update_atomic_cb (gpointer *data,
                   GDestroyNotify *destroy_notify,
                   gpointer user_data)
{
  const char *op = user_data;
  char *data_entry = *data;

  g_assert_nonnull (op);

  if (strcmp (op, "create") == 0)
    {
      g_assert_cmpstr (data_entry, ==, NULL);
      g_assert_null (*destroy_notify);

      *data = g_strdup ("hello");
      *destroy_notify = g_free;
    }
  else if (strcmp (op, "remove") == 0)
    {
      g_assert_cmpstr (data_entry, ==, "hello");
      g_assert_true (*destroy_notify == g_free);

      g_free (data_entry);
      *data = NULL;
    }
  else
    g_assert_not_reached ();

  return "result";
}

static void
test_datalist_update_atomic (void)
{
  GQuark one = g_quark_from_static_string ("one");
  GData *list = NULL;
  const char *result;

  result = _g_datalist_id_update_atomic (&list, one, _update_atomic_cb, "create");
  g_assert_cmpstr (result, ==, "result");
  g_assert_cmpstr ((const char *) g_datalist_id_get_data (&list, one), ==, "hello");

  g_datalist_id_set_data_full (&list, one, g_strdup ("hello"), g_free);

  result = _g_datalist_id_update_atomic (&list, one, _update_atomic_cb, "remove");
  g_assert_cmpstr (result, ==, "result");

  g_assert_null (list);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/quark/basic", test_quark_basic);
  g_test_add_func ("/quark/string", test_quark_string);
  g_test_add_func ("/dataset/basic", test_dataset_basic);
  g_test_add_func ("/dataset/id", test_dataset_id);
  g_test_add_func ("/dataset/full", test_dataset_full);
  g_test_add_func ("/dataset/foreach", test_dataset_foreach);
  g_test_add_func ("/dataset/destroy", test_dataset_destroy);
  g_test_add_data_func ("/datalist/basic/few", GINT_TO_POINTER (FALSE), test_datalist_basic);
  g_test_add_data_func ("/datalist/basic/many", GINT_TO_POINTER (TRUE), test_datalist_basic);
  g_test_add_func ("/datalist/id", test_datalist_id);
  g_test_add_func ("/datalist/recursive-clear", test_datalist_clear);
  g_test_add_func ("/datalist/id-remove-multiple", test_datalist_id_remove_multiple);
  g_test_add_func ("/datalist/id-remove-multiple/resize", test_datalist_id_remove_multiple_resize);
  g_test_add_func ("/datalist/id-remove-multiple-destroy-order",
                   test_datalist_id_remove_multiple_destroy_order);
  g_test_add_func ("/datalist/update-atomic", test_datalist_update_atomic);

  return g_test_run ();
}
