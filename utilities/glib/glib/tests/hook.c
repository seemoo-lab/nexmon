/* Unit tests for hook lists
 * Copyright (C) 2011 Red Hat, Inc.
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
 *
 * Author: Matthias Clasen
 */

#include "glib.h"

static void
hook_func (gpointer data)
{
}

static void
hook_destroy (gpointer data)
{
}

static gboolean
hook_find_false (GHook *hook, gpointer data)
{
  return FALSE;
}

static gboolean
hook_find_true (GHook *hook, gpointer data)
{
  return TRUE;
}

static void
hook_marshaller (GHook *hook, gpointer marshal_data)
{
}

static gboolean
hook_marshaller_check (GHook *hook, gpointer marshal_data)
{
  return TRUE;
}

static gint
hook_compare (GHook *new_hook, GHook *sibling)
{
  return 1;
}

static void
test_hook_corner_cases (void)
{
  GHookList *hl;
  GHook *hook;

  /* Check if hl->finalize_hook is NULL */
  hl = g_new (GHookList, 1);
  g_hook_list_init (hl, sizeof (GHook));
  hl->finalize_hook = NULL;
  hl->is_setup = FALSE;
  g_hook_list_clear (hl);
  g_free (hl);

  /* Check if hook->destroy is NULL */
  hl = g_new (GHookList, 1);
  g_hook_list_init (hl, sizeof (GHook));

  hook = g_hook_alloc (hl);
  g_assert_nonnull (hook);
  hook->data = GINT_TO_POINTER (1);
  hook->func = hook_func;
  hook->flags = G_HOOK_FLAG_ACTIVE;
  hook->destroy = NULL;
  g_hook_append (hl, hook);

  g_assert_false (g_hook_destroy (hl, 10));

  g_hook_list_clear (hl);
  g_free (hl);
}

static void
test_hook_basics (void)
{
  GHookList *hl;
  GHook *hook;
  gulong id;
  GHook *h;

  hl = g_new (GHookList, 1);
  g_hook_list_init (hl, sizeof (GHook));
  g_assert_nonnull (hl);
  g_assert_cmpint (hl->seq_id, ==, 1);
  g_assert_cmpint (hl->hook_size, ==, sizeof (GHook));
  g_assert_true (hl->is_setup);
  g_assert_null (hl->hooks);
  g_assert_null (hl->dummy3);
  g_assert_nonnull (hl->finalize_hook);
  g_assert_null (hl->dummy[0]);
  g_assert_null (hl->dummy[1]);

  hook = g_hook_alloc (hl);
  g_assert_null (hook->data);
  g_assert_null (hook->next);
  g_assert_null (hook->prev);
  g_assert_cmpint (hook->flags, ==, G_HOOK_FLAG_ACTIVE);
  g_assert_cmpint (hook->ref_count, ==, 0);
  g_assert_cmpint (hook->hook_id, ==, 0);
  g_assert_null (hook->func);
  g_assert_null (hook->destroy);

  hook->data = GINT_TO_POINTER(1);
  hook->func = hook_func;
  hook->flags = G_HOOK_FLAG_ACTIVE;
  hook->destroy = hook_destroy;
  g_hook_append (hl, hook);
  id = hook->hook_id;

  h = g_hook_get (hl, id);
  g_assert_cmpmem (h, sizeof (GHook), hook, sizeof (GHook));

  g_assert_cmpint (g_hook_compare_ids (h, hook), ==, 0);

  h = hook = g_hook_alloc (hl);
  hook->data = GINT_TO_POINTER(2);
  hook->func = hook_func;
  hook->flags = G_HOOK_FLAG_ACTIVE;
  hook->destroy = hook_destroy;
  g_hook_prepend (hl, hook);

  g_hook_destroy (hl, id);

  hook = g_hook_alloc (hl);
  hook->data = GINT_TO_POINTER(3);
  hook->func = hook_func;
  hook->flags = G_HOOK_FLAG_ACTIVE;
  hook->destroy = hook_destroy;
  g_hook_insert_sorted (hl, hook, g_hook_compare_ids);

  g_assert_cmpint (g_hook_compare_ids (h, hook), ==, -1);

  hook = g_hook_alloc (hl);
  hook->data = GINT_TO_POINTER(4);
  hook->func = hook_func;
  hook->flags = G_HOOK_FLAG_ACTIVE;
  hook->destroy = hook_destroy;
  g_hook_insert_sorted (hl, hook, hook_compare);

  hook = g_hook_alloc (hl);
  hook->data = GINT_TO_POINTER(5);
  hook->func = hook_func;
  hook->flags = G_HOOK_FLAG_ACTIVE;
  hook->destroy = hook_destroy;
  g_hook_insert_before (hl, h, hook);

  hook = g_hook_alloc (hl);
  hook->data = GINT_TO_POINTER (6);
  hook->func = hook_func;
  hook->flags = G_HOOK_FLAG_ACTIVE;
  hook->destroy = hook_destroy;
  g_hook_insert_before (hl, NULL, hook);

  /* Hook list is built, let's dig into it now */
  g_hook_list_invoke (hl, TRUE);
  g_hook_list_invoke_check (hl, TRUE);

  g_assert_null (g_hook_find (hl, FALSE, hook_find_false, NULL));
  g_assert_nonnull (g_hook_find (hl, TRUE, hook_find_true, NULL));

  g_assert_null (g_hook_find_data (hl, TRUE, &id));
  g_assert_nonnull (g_hook_find_data (hl, TRUE, GINT_TO_POINTER(2)));
  g_assert_null (g_hook_find_data (hl, FALSE, &id));

  g_assert_nonnull (g_hook_find_func (hl, TRUE, hook_func));
  g_assert_nonnull (g_hook_find_func (hl, FALSE, hook_func));
  g_assert_null (g_hook_find_func (hl, FALSE, hook_destroy));

  g_assert_nonnull (g_hook_find_func_data (hl, TRUE, hook_func, GINT_TO_POINTER(2)));
  g_assert_null (g_hook_find_func_data (hl, FALSE, hook_func, GINT_TO_POINTER(20)));
  g_assert_null (g_hook_find_func_data (hl, FALSE, hook_destroy, GINT_TO_POINTER(20)));

  g_hook_list_marshal (hl, TRUE, hook_marshaller, NULL);
  g_hook_list_marshal (hl, TRUE, hook_marshaller, GINT_TO_POINTER(2));
  g_hook_list_marshal (hl, FALSE, hook_marshaller, NULL);

  g_hook_list_marshal_check (hl, TRUE, hook_marshaller_check, NULL);
  g_hook_list_marshal_check (hl, TRUE, hook_marshaller_check, GINT_TO_POINTER(2));
  g_hook_list_marshal_check (hl, FALSE, hook_marshaller_check, NULL);

  g_hook_list_clear (hl);
  g_free (hl);
}

int main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/hook/basics", test_hook_basics);
  g_test_add_func ("/hook/corner-cases", test_hook_corner_cases);

  return g_test_run ();
}
