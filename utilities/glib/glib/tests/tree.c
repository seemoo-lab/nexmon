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
#undef G_LOG_DOMAIN

/* We are testing some deprecated APIs here */
#ifndef GLIB_DISABLE_DEPRECATION_WARNINGS
#define GLIB_DISABLE_DEPRECATION_WARNINGS
#endif

#include <stdio.h>
#include <string.h>
#include "glib.h"


static gint
my_compare (gconstpointer a,
            gconstpointer b)
{
  const char *cha = a;
  const char *chb = b;

  return *cha - *chb;
}

static gint
my_compare_with_data (gconstpointer a,
                      gconstpointer b,
                      gpointer      user_data)
{
  const char *cha = a;
  const char *chb = b;

  /* just check that we got the right data */
  g_assert_cmpint (GPOINTER_TO_INT (user_data), ==, 123);

  return *cha - *chb;
}

static gint
my_search (gconstpointer a,
           gconstpointer b)
{
  return my_compare (b, a);
}

static gpointer destroyed_key = NULL;
static gpointer destroyed_value = NULL;
static guint destroyed_key_count = 0;
static guint destroyed_value_count = 0;

static void
my_key_destroy (gpointer key)
{
  destroyed_key = key;
  destroyed_key_count++;
}

static void
my_value_destroy (gpointer value)
{
  destroyed_value = value;
  destroyed_value_count++;
}

static gint
my_traverse (gpointer key,
             gpointer value,
             gpointer data)
{
  char *ch = key;

  g_assert_cmpint ((*ch), >, 0);

  if (*ch == 'd')
    return TRUE;

  return FALSE;
}

char chars[] =
  "0123456789"
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  "abcdefghijklmnopqrstuvwxyz";

char chars2[] =
  "0123456789"
  "abcdefghijklmnopqrstuvwxyz";

static gint
check_order (gpointer key,
             gpointer value,
             gpointer data)
{
  char **p = data;
  char *ch = key;
 
  g_assert_cmpint (**p, ==, *ch);

  (*p)++;

  return FALSE;
}

static void
test_tree_search (void)
{
  gint i;
  GTree *tree;
  gboolean removed;
  gchar c;
  gchar *p, *d;

  tree = g_tree_new_with_data (my_compare_with_data, GINT_TO_POINTER(123));

  for (i = 0; chars[i]; i++)
    g_tree_insert (tree, &chars[i], &chars[i]);

  g_tree_foreach (tree, my_traverse, NULL);

  g_assert_cmpint (g_tree_nnodes (tree), ==, strlen (chars));
  g_assert_cmpint (g_tree_height (tree), ==, 6);
 
  p = chars;
  g_tree_foreach (tree, check_order, &p);

  for (i = 0; i < 26; i++)
    {
      removed = g_tree_remove (tree, &chars[i + 10]);
      g_assert_true (removed);
    }

  c = '\0';
  removed = g_tree_remove (tree, &c);
  g_assert_false (removed);

  g_tree_foreach (tree, my_traverse, NULL);

  g_assert_cmpint (g_tree_nnodes (tree), ==, strlen (chars2));
  g_assert_cmpint (g_tree_height (tree), ==, 6);

  p = chars2;
  g_tree_foreach (tree, check_order, &p);

  for (i = 25; i >= 0; i--)
    g_tree_insert (tree, &chars[i + 10], &chars[i + 10]);

  p = chars;
  g_tree_foreach (tree, check_order, &p);

  c = '0';
  p = g_tree_lookup (tree, &c);
  g_assert_true (p && *p == c);
  g_assert_true (g_tree_lookup_extended (tree, &c, (gpointer *)&d, (gpointer *)&p));
  g_assert_true (c == *d && c == *p);

  c = 'A';
  p = g_tree_lookup (tree, &c);
  g_assert_true (p && *p == c);

  c = 'a';
  p = g_tree_lookup (tree, &c);
  g_assert_true (p && *p == c);

  c = 'z';
  p = g_tree_lookup (tree, &c);
  g_assert_true (p && *p == c);

  c = '!';
  p = g_tree_lookup (tree, &c);
  g_assert_null (p);

  c = '=';
  p = g_tree_lookup (tree, &c);
  g_assert_null (p);

  c = '|';
  p = g_tree_lookup (tree, &c);
  g_assert_null (p);

  c = '0';
  p = g_tree_search (tree, my_search, &c);
  g_assert_true (p && *p == c);

  c = 'A';
  p = g_tree_search (tree, my_search, &c);
  g_assert_true (p && *p == c);

  c = 'a';
  p = g_tree_search (tree, my_search, &c);
  g_assert_true (p &&*p == c);

  c = 'z';
  p = g_tree_search (tree, my_search, &c);
  g_assert_true (p && *p == c);

  c = '!';
  p = g_tree_search (tree, my_search, &c);
  g_assert_null (p);

  c = '=';
  p = g_tree_search (tree, my_search, &c);
  g_assert_null (p);

  c = '|';
  p = g_tree_search (tree, my_search, &c);
  g_assert_null (p);

  g_tree_destroy (tree);
}

static void
test_tree_remove (void)
{
  GTree *tree;
  char c, d, e, f;
  gint i;
  gboolean removed;
  GTreeNode *node;
  gchar *remove;

  tree = g_tree_new_full ((GCompareDataFunc)my_compare, NULL,
                          my_key_destroy,
                          my_value_destroy);

  for (i = 0; chars[i]; i++)
    g_tree_insert (tree, &chars[i], &chars[i]);

  c = '0';
  g_tree_insert (tree, &c, &c);
  g_assert_true (destroyed_key == &c);
  g_assert_true (destroyed_value == &chars[0]);
  destroyed_key = NULL;
  destroyed_value = NULL;

  d = '1';
  g_tree_replace (tree, &d, &d);
  g_assert_true (destroyed_key == &chars[1]);
  g_assert_true (destroyed_value == &chars[1]);
  destroyed_key = NULL;
  destroyed_value = NULL;

  e = '\xff';
  node = g_tree_insert_node (tree, &e, &e);
  g_assert_nonnull (node);
  g_assert_null (destroyed_key);
  g_assert_null (destroyed_value);

  c = '2';
  removed = g_tree_remove (tree, &c);
  g_assert_true (removed);
  g_assert_true (destroyed_key == &chars[2]);
  g_assert_true (destroyed_value == &chars[2]);
  destroyed_key = NULL;
  destroyed_value = NULL;

  c = '3';
  removed = g_tree_steal (tree, &c);
  g_assert_true (removed);
  g_assert_null (destroyed_key);
  g_assert_null (destroyed_value);

  f = '4';
  node = g_tree_replace_node (tree, &f, &f);
  g_assert_nonnull (node);
  g_assert_true (destroyed_key == &chars[4]);
  g_assert_true (destroyed_value == &chars[4]);
  destroyed_key = NULL;
  destroyed_value = NULL;

  remove = "omkjigfedba";
  for (i = 0; remove[i]; i++)
    {
      removed = g_tree_remove (tree, &remove[i]);
      g_assert_true (removed);
    }

  g_tree_destroy (tree);
}

static void
test_tree_remove_all (void)
{
  GTree *tree;
  gsize i;

  tree = g_tree_new_full ((GCompareDataFunc)my_compare, NULL,
                          my_key_destroy,
                          my_value_destroy);

  for (i = 0; chars[i]; i++)
    g_tree_insert (tree, &chars[i], &chars[i]);

  destroyed_key_count = 0;
  destroyed_value_count = 0;

  g_tree_remove_all (tree);

  g_assert_cmpuint (destroyed_key_count, ==, strlen (chars));
  g_assert_cmpuint (destroyed_value_count, ==, strlen (chars));
  g_assert_cmpint (g_tree_height (tree), ==, 0);
  g_assert_cmpint (g_tree_nnodes (tree), ==, 0);

  g_tree_unref (tree);
}

static void
test_tree_destroy (void)
{
  GTree *tree;
  gint i;

  tree = g_tree_new (my_compare);

  for (i = 0; chars[i]; i++)
    g_tree_insert (tree, &chars[i], &chars[i]);

  g_assert_cmpint (g_tree_nnodes (tree), ==, strlen (chars));

  g_tree_ref (tree);
  g_tree_destroy (tree);

  g_assert_cmpint (g_tree_nnodes (tree), ==, 0);

  g_tree_unref (tree);
}

typedef struct {
  GString *s;
  gint count;
} CallbackData;

static gboolean
traverse_func (gpointer key, gpointer value, gpointer data)
{
  CallbackData *d = data;

  gchar *c = value;
  g_string_append_c (d->s, *c);

  d->count--;

  if (d->count == 0)
    return TRUE;

  return FALSE;
}

typedef struct {
  GTraverseType  traverse;
  gint           limit;
  const gchar   *expected;
} TraverseData;

static void
test_tree_traverse (void)
{
  GTree *tree;
  gsize i;
  TraverseData orders[] = {
    { G_IN_ORDER,   -1, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz" },
    { G_IN_ORDER,    1, "0" },
    { G_IN_ORDER,    2, "01" },
    { G_IN_ORDER,    3, "012" },
    { G_IN_ORDER,    4, "0123" },
    { G_IN_ORDER,    5, "01234" },
    { G_IN_ORDER,    6, "012345" },
    { G_IN_ORDER,    7, "0123456" },
    { G_IN_ORDER,    8, "01234567" },
    { G_IN_ORDER,    9, "012345678" },
    { G_IN_ORDER,   10, "0123456789" },
    { G_IN_ORDER,   11, "0123456789A" },
    { G_IN_ORDER,   12, "0123456789AB" },
    { G_IN_ORDER,   13, "0123456789ABC" },
    { G_IN_ORDER,   14, "0123456789ABCD" },

    { G_PRE_ORDER,  -1, "VF73102546B98ADCENJHGILKMRPOQTSUldZXWYbachfegjiktpnmorqsxvuwyz" },
    { G_PRE_ORDER,   1, "V" },
    { G_PRE_ORDER,   2, "VF" },
    { G_PRE_ORDER,   3, "VF7" },
    { G_PRE_ORDER,   4, "VF73" },
    { G_PRE_ORDER,   5, "VF731" },
    { G_PRE_ORDER,   6, "VF7310" },
    { G_PRE_ORDER,   7, "VF73102" },
    { G_PRE_ORDER,   8, "VF731025" },
    { G_PRE_ORDER,   9, "VF7310254" },
    { G_PRE_ORDER,  10, "VF73102546" },
    { G_PRE_ORDER,  11, "VF73102546B" },
    { G_PRE_ORDER,  12, "VF73102546B9" },
    { G_PRE_ORDER,  13, "VF73102546B98" },
    { G_PRE_ORDER,  14, "VF73102546B98A" },

    { G_POST_ORDER, -1, "02146538A9CEDB7GIHKMLJOQPSUTRNFWYXacbZegfikjhdmonqsrpuwvzyxtlV" },
    { G_POST_ORDER,  1, "0" },
    { G_POST_ORDER,  2, "02" },
    { G_POST_ORDER,  3, "021" },
    { G_POST_ORDER,  4, "0214" },
    { G_POST_ORDER,  5, "02146" },
    { G_POST_ORDER,  6, "021465" },
    { G_POST_ORDER,  7, "0214653" },
    { G_POST_ORDER,  8, "02146538" },
    { G_POST_ORDER,  9, "02146538A" },
    { G_POST_ORDER, 10, "02146538A9" },
    { G_POST_ORDER, 11, "02146538A9C" },
    { G_POST_ORDER, 12, "02146538A9CE" },
    { G_POST_ORDER, 13, "02146538A9CED" },
    { G_POST_ORDER, 14, "02146538A9CEDB" }
  };
  CallbackData data;

  tree = g_tree_new (my_compare);

  for (i = 0; chars[i]; i++)
    g_tree_insert (tree, &chars[i], &chars[i]);

  data.s = g_string_new ("");
  for (i = 0; i < G_N_ELEMENTS (orders); i++)
    {
      g_string_set_size (data.s, 0);
      data.count = orders[i].limit;
      g_tree_traverse (tree, traverse_func, orders[i].traverse, &data);
      g_assert_cmpstr (data.s->str, ==, orders[i].expected);
    }

  g_tree_unref (tree);
  g_string_free (data.s, TRUE); 
}

static void
test_tree_insert (void)
{
  GTree *tree;
  gchar *p;
  size_t i;
  gchar *scrambled;

  tree = g_tree_new (my_compare);

  for (i = 0; chars[i]; i++)
    g_tree_insert (tree, &chars[i], &chars[i]);
  p = chars;
  g_tree_foreach (tree, check_order, &p);

  g_tree_unref (tree);
  tree = g_tree_new (my_compare);

  for (i = strlen (chars); i > 0; i--)
    g_tree_insert (tree, &chars[i - 1], &chars[i - 1]);
  p = chars;
  g_tree_foreach (tree, check_order, &p);

  g_tree_unref (tree);
  tree = g_tree_new (my_compare);

  scrambled = g_strdup (chars);

  for (i = 0; i < 30; i++)
    {
      gchar tmp;
      gint a, b;

      a = g_random_int_range (0, strlen (scrambled));
      b = g_random_int_range (0, strlen (scrambled));
      tmp = scrambled[a];
      scrambled[a] = scrambled[b];
      scrambled[b] = tmp;
    }

  for (i = 0; scrambled[i]; i++)
    g_tree_insert (tree, &scrambled[i], &scrambled[i]);
  p = chars;
  g_tree_foreach (tree, check_order, &p);

  g_free (scrambled);
  g_tree_unref (tree);
}

static void
binary_tree_bound (GTree *tree,
                   char   c,
                   char   expected,
                   int    lower)
{
  GTreeNode *node;

  if (lower)
    node = g_tree_lower_bound (tree, &c);
  else
    node = g_tree_upper_bound (tree, &c);

  if (g_test_verbose ())
    g_test_message ("%c %s: ", c, lower ? "lower" : "upper");

  if (!node)
    {
      if (!g_tree_nnodes (tree))
        {
          if (g_test_verbose ())
            g_test_message ("empty tree");
        }
      else
        {
          GTreeNode *last = g_tree_node_last (tree);

          g_assert_nonnull (last);
          if (g_test_verbose ())
            g_test_message ("past end last %c",
                            *(char *) g_tree_node_key (last));
        }
      g_assert_cmpint (expected, ==, '\x00');
    }
  else
    {
      GTreeNode *begin = g_tree_node_first (tree);
      GTreeNode *last = g_tree_node_last (tree);
      GTreeNode *prev = g_tree_node_previous (node);
      GTreeNode *next = g_tree_node_next (node);

      g_assert_cmpint (expected, !=, '\x00');
      g_assert_cmpint (expected, ==, *(char *) g_tree_node_key (node));

      if (g_test_verbose ())
        g_test_message ("%c", *(char *) g_tree_node_key (node));

      if (node != begin)
        {
          g_assert_nonnull (prev);
          if (g_test_verbose ())
            g_test_message (" prev %c", *(char *) g_tree_node_key (prev));
        }
      else
        {
          g_assert_null (prev);
          if (g_test_verbose ())
            g_test_message (" no prev, it's the first one");
        }

      if (node != last)
        {
          g_assert_nonnull (next);
          if (g_test_verbose ())
            g_test_message (" next %c", *(char *) g_tree_node_key (next));
        }
      else
        {
          g_assert_null (next);
          if (g_test_verbose ())
            g_test_message (" no next, it's the last one");
        }
    }

  if (g_test_verbose ())
    g_test_message ("\n");
}

static void
binary_tree_bounds (GTree *tree,
                    char   c,
                    int    mode)
{
  char expectedl, expectedu;
  char first = mode == 0 ? '0' : mode == 1 ? 'A' : 'z';

  g_assert_true (mode >= 0 && mode <= 3);

  if (c < first)
    expectedl = first;
  else if (c > 'z')
    expectedl = '\x00';
  else
    expectedl = c;

  if (c < first)
    expectedu = first;
  else if (c >= 'z')
    expectedu = '\x00';
  else
    expectedu = c == '9' ? 'A' : c == 'Z' ? 'a' : c + 1;

  if (mode == 3)
    {
      expectedl = '\x00';
      expectedu = '\x00';
    }

  binary_tree_bound (tree, c, expectedl, 1);
  binary_tree_bound (tree, c, expectedu, 0);
}

static void
binary_tree_bounds_test (GTree *tree,
                         int    mode)
{
  binary_tree_bounds (tree, 'a', mode);
  binary_tree_bounds (tree, 'A', mode);
  binary_tree_bounds (tree, 'z', mode);
  binary_tree_bounds (tree, 'Z', mode);
  binary_tree_bounds (tree, 'Y', mode);
  binary_tree_bounds (tree, '0', mode);
  binary_tree_bounds (tree, '9', mode);
  binary_tree_bounds (tree, '0' - 1, mode);
  binary_tree_bounds (tree, 'z' + 1, mode);
  binary_tree_bounds (tree, '0' - 2, mode);
  binary_tree_bounds (tree, 'z' + 2, mode);
}

static void
test_tree_bounds (void)
{
  GQueue queue = G_QUEUE_INIT;
  GTree *tree;
  char chars[62];
  guint i, j;

  tree = g_tree_new (my_compare);

  i = 0;
  for (j = 0; j < 10; j++, i++)
    {
      chars[i] = '0' + j;
      g_queue_push_tail (&queue, &chars[i]);
    }

  for (j = 0; j < 26; j++, i++)
    {
      chars[i] = 'A' + j;
      g_queue_push_tail (&queue, &chars[i]);
    }

  for (j = 0; j < 26; j++, i++)
    {
      chars[i] = 'a' + j;
      g_queue_push_tail (&queue, &chars[i]);
    }

  if (g_test_verbose ())
    g_test_message ("tree insert: ");

  while (!g_queue_is_empty (&queue))
    {
      gint32 which = g_random_int_range (0, g_queue_get_length (&queue));
      gpointer elem = g_queue_pop_nth (&queue, which);
      GTreeNode *node;

      if (g_test_verbose ())
        g_test_message ("%c ", *(char *) elem);

      node = g_tree_insert_node (tree, elem, elem);
      g_assert_nonnull (node);
      g_assert_true (g_tree_node_key (node) == elem);
      g_assert_true (g_tree_node_value (node) == elem);
    }

  if (g_test_verbose ())
    g_test_message ("\n");

  g_assert_cmpint (g_tree_nnodes (tree), ==, 10 + 26 + 26);
  g_assert_cmpint (g_tree_height (tree), >=, 6);
  g_assert_cmpint (g_tree_height (tree), <=, 8);

  if (g_test_verbose ())
    {
      g_test_message ("tree: ");
      g_tree_foreach (tree, my_traverse, NULL);
      g_test_message ("\n");
    }

  binary_tree_bounds_test (tree, 0);

  for (i = 0; i < 10; i++)
    g_tree_remove (tree, &chars[i]);

  g_assert_cmpint (g_tree_nnodes (tree), ==, 26 + 26);
  g_assert_cmpint (g_tree_height (tree), >=, 6);
  g_assert_cmpint (g_tree_height (tree), <=, 8);

  if (g_test_verbose ())
    {
      g_test_message ("tree: ");
      g_tree_foreach (tree, my_traverse, NULL);
      g_test_message ("\n");
    }

  binary_tree_bounds_test (tree, 1);

  for (i = 10; i < 10 + 26 + 26 - 1; i++)
    g_tree_remove (tree, &chars[i]);

  if (g_test_verbose ())
    {
      g_test_message ("tree: ");
      g_tree_foreach (tree, my_traverse, NULL);
      g_test_message ("\n");
    }

  binary_tree_bounds_test (tree, 2);

  g_tree_remove (tree, &chars[10 + 26 + 26 - 1]);

  if (g_test_verbose ())
    g_test_message ("empty tree\n");

  binary_tree_bounds_test (tree, 3);

  g_tree_unref (tree);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/tree/search", test_tree_search);
  g_test_add_func ("/tree/remove", test_tree_remove);
  g_test_add_func ("/tree/destroy", test_tree_destroy);
  g_test_add_func ("/tree/traverse", test_tree_traverse);
  g_test_add_func ("/tree/insert", test_tree_insert);
  g_test_add_func ("/tree/bounds", test_tree_bounds);
  g_test_add_func ("/tree/remove-all", test_tree_remove_all);

  return g_test_run ();
}
