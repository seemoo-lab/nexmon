/*
 * Copyright © 2010, 2011, 2013, 2014 Codethink Limited
 * Copyright © 2010, 2011, 2012, 2013, 2015 Red Hat, Inc.
 * Copyright © 2012 Pavel Vasin
 * Copyright © 2022 Endless OS Foundation, LLC
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
 * Authors: Ryan Lortie <desrt@desrt.ca>
 */

#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>

#include "gdbus-sessionbus.h"

typedef struct
{
  GVariant *params;
  gboolean did_run;
} Activation;

static void
activate (GAction  *action,
          GVariant *parameter,
          gpointer  user_data)
{
  Activation *activation = user_data;

  if (parameter)
    activation->params = g_variant_ref (parameter);
  else
    activation->params = NULL;
  activation->did_run = TRUE;
}

static void
test_basic (void)
{
  Activation a = { 0, };
  GSimpleAction *action;
  gchar *name;
  GVariantType *parameter_type;
  gboolean enabled;
  GVariantType *state_type;
  GVariant *state;

  action = g_simple_action_new ("foo", NULL);
  g_assert_true (g_action_get_enabled (G_ACTION (action)));
  g_assert_null (g_action_get_parameter_type (G_ACTION (action)));
  g_assert_null (g_action_get_state_type (G_ACTION (action)));
  g_assert_null (g_action_get_state_hint (G_ACTION (action)));
  g_assert_null (g_action_get_state (G_ACTION (action)));
  g_object_get (action,
                "name", &name,
                "parameter-type", &parameter_type,
                "enabled", &enabled,
                "state-type", &state_type,
                "state", &state,
                 NULL);
  g_assert_cmpstr (name, ==, "foo");
  g_assert_null (parameter_type);
  g_assert_true (enabled);
  g_assert_null (state_type);
  g_assert_null (state);
  g_free (name);

  g_signal_connect (action, "activate", G_CALLBACK (activate), &a);
  g_assert_false (a.did_run);
  g_action_activate (G_ACTION (action), NULL);
  g_assert_true (a.did_run);
  a.did_run = FALSE;

  g_simple_action_set_enabled (action, FALSE);
  g_action_activate (G_ACTION (action), NULL);
  g_assert_false (a.did_run);

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*g_variant_is_of_type*failed*");
      g_action_activate (G_ACTION (action), g_variant_new_string ("xxx"));
      g_test_assert_expected_messages ();
    }

  g_object_unref (action);
  g_assert_false (a.did_run);

  action = g_simple_action_new ("foo", G_VARIANT_TYPE_STRING);
  g_assert_true (g_action_get_enabled (G_ACTION (action)));
  g_assert_true (g_variant_type_equal (g_action_get_parameter_type (G_ACTION (action)), G_VARIANT_TYPE_STRING));
  g_assert_null (g_action_get_state_type (G_ACTION (action)));
  g_assert_null (g_action_get_state_hint (G_ACTION (action)));
  g_assert_null (g_action_get_state (G_ACTION (action)));

  g_signal_connect (action, "activate", G_CALLBACK (activate), &a);
  g_assert_false (a.did_run);
  g_action_activate (G_ACTION (action), g_variant_new_string ("Hello world"));
  g_assert_true (a.did_run);
  g_assert_cmpstr (g_variant_get_string (a.params, NULL), ==, "Hello world");
  g_variant_unref (a.params);
  a.did_run = FALSE;

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*failed*");
      g_action_activate (G_ACTION (action), NULL);
      g_test_assert_expected_messages ();
    }

  g_object_unref (action);
  g_assert_false (a.did_run);
}

static void
test_name (void)
{
  g_assert_false (g_action_name_is_valid (""));
  g_assert_false (g_action_name_is_valid ("("));
  g_assert_false (g_action_name_is_valid ("%abc"));
  g_assert_false (g_action_name_is_valid ("$x1"));
  g_assert_true (g_action_name_is_valid ("abc.def"));
  g_assert_true (g_action_name_is_valid ("ABC-DEF"));
}

static gboolean
strv_strv_cmp (const gchar * const *a,
               const gchar * const *b)
{
  guint n;

  for (n = 0; a[n] != NULL; n++)
    {
       if (!g_strv_contains (b, a[n]))
         return FALSE;
    }

  for (n = 0; b[n] != NULL; n++)
    {
       if (!g_strv_contains (a, b[n]))
         return FALSE;
    }

  return TRUE;
}

static gboolean
strv_set_equal (const gchar * const *strv, ...)
{
  guint count;
  va_list list;
  const gchar *str;
  gboolean res;

  res = TRUE;
  count = 0;
  va_start (list, strv);
  while (1)
    {
      str = va_arg (list, const gchar *);
      if (str == NULL)
        break;
      if (!g_strv_contains (strv, str))
        {
          res = FALSE;
          break;
        }
      count++;
    }
  va_end (list);

  if (res)
    res = g_strv_length ((gchar**)strv) == count;

  return res;
}

static void
ensure_state (GActionGroup *group,
              const gchar  *action_name,
              const gchar  *expected)
{
  GVariant *value;
  gchar *printed;

  value = g_action_group_get_action_state (group, action_name);
  printed = g_variant_print (value, TRUE);
  g_variant_unref (value);

  g_assert_cmpstr (printed, ==, expected);
  g_free (printed);
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void
test_simple_group (void)
{
  GSimpleActionGroup *group;
  Activation a = { 0, };
  GSimpleAction *simple;
  GAction *action;
  gchar **actions;
  GVariant *state;

  simple = g_simple_action_new ("foo", NULL);
  g_signal_connect (simple, "activate", G_CALLBACK (activate), &a);
  g_assert_false (a.did_run);
  g_action_activate (G_ACTION (simple), NULL);
  g_assert_true (a.did_run);
  a.did_run = FALSE;

  group = g_simple_action_group_new ();
  g_simple_action_group_insert (group, G_ACTION (simple));
  g_object_unref (simple);

  g_assert_false (a.did_run);
  g_action_group_activate_action (G_ACTION_GROUP (group), "foo", NULL);
  g_assert_true (a.did_run);

  simple = g_simple_action_new_stateful ("bar", G_VARIANT_TYPE_STRING, g_variant_new_string ("hihi"));
  g_simple_action_group_insert (group, G_ACTION (simple));
  g_object_unref (simple);

  g_assert_true (g_action_group_has_action (G_ACTION_GROUP (group), "foo"));
  g_assert_true (g_action_group_has_action (G_ACTION_GROUP (group), "bar"));
  g_assert_false (g_action_group_has_action (G_ACTION_GROUP (group), "baz"));
  actions = g_action_group_list_actions (G_ACTION_GROUP (group));
  g_assert_cmpint (g_strv_length (actions), ==, 2);
  g_assert_true (strv_set_equal ((const gchar * const *) actions, "foo", "bar", NULL));
  g_strfreev (actions);
  g_assert_true (g_action_group_get_action_enabled (G_ACTION_GROUP (group), "foo"));
  g_assert_true (g_action_group_get_action_enabled (G_ACTION_GROUP (group), "bar"));
  g_assert_null (g_action_group_get_action_parameter_type (G_ACTION_GROUP (group), "foo"));
  g_assert_true (g_variant_type_equal (g_action_group_get_action_parameter_type (G_ACTION_GROUP (group), "bar"), G_VARIANT_TYPE_STRING));
  g_assert_null (g_action_group_get_action_state_type (G_ACTION_GROUP (group), "foo"));
  g_assert_true (g_variant_type_equal (g_action_group_get_action_state_type (G_ACTION_GROUP (group), "bar"), G_VARIANT_TYPE_STRING));
  g_assert_null (g_action_group_get_action_state_hint (G_ACTION_GROUP (group), "foo"));
  g_assert_null (g_action_group_get_action_state_hint (G_ACTION_GROUP (group), "bar"));
  g_assert_null (g_action_group_get_action_state (G_ACTION_GROUP (group), "foo"));
  state = g_action_group_get_action_state (G_ACTION_GROUP (group), "bar");
  g_assert_true (g_variant_type_equal (g_variant_get_type (state), G_VARIANT_TYPE_STRING));
  g_assert_cmpstr (g_variant_get_string (state, NULL), ==, "hihi");
  g_variant_unref (state);

  g_action_group_change_action_state (G_ACTION_GROUP (group), "bar", g_variant_new_string ("boo"));
  state = g_action_group_get_action_state (G_ACTION_GROUP (group), "bar");
  g_assert_cmpstr (g_variant_get_string (state, NULL), ==, "boo");
  g_variant_unref (state);

  action = g_simple_action_group_lookup (group, "bar");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);
  g_assert_false (g_action_group_get_action_enabled (G_ACTION_GROUP (group), "bar"));

  g_simple_action_group_remove (group, "bar");
  action = g_simple_action_group_lookup (group, "foo");
  g_assert_cmpstr (g_action_get_name (action), ==, "foo");
  action = g_simple_action_group_lookup (group, "bar");
  g_assert_null (action);

  simple = g_simple_action_new ("foo", NULL);
  g_simple_action_group_insert (group, G_ACTION (simple));
  g_object_unref (simple);

  a.did_run = FALSE;
  g_object_unref (group);
  g_assert_false (a.did_run);
}

G_GNUC_END_IGNORE_DEPRECATIONS

static void
test_stateful (void)
{
  GSimpleAction *action;
  GVariant *state;

  action = g_simple_action_new_stateful ("foo", NULL, g_variant_new_string ("hihi"));
  g_assert_true (g_action_get_enabled (G_ACTION (action)));
  g_assert_null (g_action_get_parameter_type (G_ACTION (action)));
  g_assert_null (g_action_get_state_hint (G_ACTION (action)));
  g_assert_true (g_variant_type_equal (g_action_get_state_type (G_ACTION (action)),
                                       G_VARIANT_TYPE_STRING));
  state = g_action_get_state (G_ACTION (action));
  g_assert_cmpstr (g_variant_get_string (state, NULL), ==, "hihi");
  g_variant_unref (state);

  if (g_test_undefined ())
    {
      GVariant *new_state = g_variant_ref_sink (g_variant_new_int32 (123));
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*g_variant_is_of_type*failed*");
      g_simple_action_set_state (action, new_state);
      g_test_assert_expected_messages ();
      g_variant_unref (new_state);
    }

  g_simple_action_set_state (action, g_variant_new_string ("hello"));
  state = g_action_get_state (G_ACTION (action));
  g_assert_cmpstr (g_variant_get_string (state, NULL), ==, "hello");
  g_variant_unref (state);

  g_object_unref (action);

  action = g_simple_action_new ("foo", NULL);

  if (g_test_undefined ())
    {
      GVariant *new_state = g_variant_ref_sink (g_variant_new_int32 (123));
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*failed*");
      g_simple_action_set_state (action, new_state);
      g_test_assert_expected_messages ();
      g_variant_unref (new_state);
    }

  g_object_unref (action);
}

static void
test_default_activate (void)
{
  GSimpleAction *action;
  GVariant *state;

  /* Test changing state via activation with parameter */
  action = g_simple_action_new_stateful ("foo", G_VARIANT_TYPE_STRING, g_variant_new_string ("hihi"));
  g_action_activate (G_ACTION (action), g_variant_new_string ("bye"));
  state = g_action_get_state (G_ACTION (action));
  g_assert_cmpstr (g_variant_get_string (state, NULL), ==, "bye");
  g_variant_unref (state);
  g_object_unref (action);

  /* Test toggling a boolean action via activation with no parameter */
  action = g_simple_action_new_stateful ("foo", NULL, g_variant_new_boolean (FALSE));
  g_action_activate (G_ACTION (action), NULL);
  state = g_action_get_state (G_ACTION (action));
  g_assert_true (g_variant_get_boolean (state));
  g_variant_unref (state);
  /* and back again */
  g_action_activate (G_ACTION (action), NULL);
  state = g_action_get_state (G_ACTION (action));
  g_assert_false (g_variant_get_boolean (state));
  g_variant_unref (state);
  g_object_unref (action);
}

static gboolean foo_activated = FALSE;
static gboolean bar_activated = FALSE;

static void
activate_foo (GSimpleAction *simple,
              GVariant      *parameter,
              gpointer       user_data)
{
  g_assert_true (user_data == GINT_TO_POINTER (123));
  g_assert_null (parameter);
  foo_activated = TRUE;
}

static void
activate_bar (GSimpleAction *simple,
              GVariant      *parameter,
              gpointer       user_data)
{
  g_assert_true (user_data == GINT_TO_POINTER (123));
  g_assert_cmpstr (g_variant_get_string (parameter, NULL), ==, "param");
  bar_activated = TRUE;
}

static void
change_volume_state (GSimpleAction *action,
                     GVariant      *value,
                     gpointer       user_data)
{
  gint requested;

  requested = g_variant_get_int32 (value);

  /* Volume only goes from 0 to 10 */
  if (0 <= requested && requested <= 10)
    g_simple_action_set_state (action, value);
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void
test_entries (void)
{
  const GActionEntry entries[] = {
    { "foo",    activate_foo, NULL, NULL,    NULL,                { 0 } },
    { "bar",    activate_bar, "s",  NULL,    NULL,                { 0 } },
    { "toggle", NULL,         NULL, "false", NULL,                { 0 } },
    { "volume", NULL,         NULL, "0",     change_volume_state, { 0 } },
  };
  const GActionEntry entries2[] = {
    { "foo",    activate_foo, NULL, NULL,    NULL,                { 0 } },
    { "bar",    activate_bar, "s",  NULL,    NULL,                { 0 } },
    { NULL,     NULL,         NULL, NULL,    NULL,                { 0 } },
  };
  GSimpleActionGroup *actions;
  GVariant *state;
  GStrv names;

  actions = g_simple_action_group_new ();
  g_simple_action_group_add_entries (actions, entries,
                                     G_N_ELEMENTS (entries),
                                     GINT_TO_POINTER (123));

  g_assert_false (foo_activated);
  g_action_group_activate_action (G_ACTION_GROUP (actions), "foo", NULL);
  g_assert_true (foo_activated);
  foo_activated = FALSE;

  g_assert_false (bar_activated);
  g_action_group_activate_action (G_ACTION_GROUP (actions), "bar",
                                  g_variant_new_string ("param"));
  g_assert_true (bar_activated);
  g_assert_false (foo_activated);

  if (g_test_undefined ())
    {
      const GActionEntry bad_type = {
        "bad-type", NULL, "ss", NULL, NULL, { 0 }
      };
      const GActionEntry bad_state = {
        "bad-state", NULL, NULL, "flse", NULL, { 0 }
      };

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*not a valid GVariant type string*");
      g_simple_action_group_add_entries (actions, &bad_type, 1, NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*could not parse*");
      g_simple_action_group_add_entries (actions, &bad_state, 1, NULL);
      g_test_assert_expected_messages ();
    }

  state = g_action_group_get_action_state (G_ACTION_GROUP (actions), "volume");
  g_assert_cmpint (g_variant_get_int32 (state), ==, 0);
  g_variant_unref (state);

  /* should change */
  g_action_group_change_action_state (G_ACTION_GROUP (actions), "volume",
                                      g_variant_new_int32 (7));
  state = g_action_group_get_action_state (G_ACTION_GROUP (actions), "volume");
  g_assert_cmpint (g_variant_get_int32 (state), ==, 7);
  g_variant_unref (state);

  /* should not change */
  g_action_group_change_action_state (G_ACTION_GROUP (actions), "volume",
                                      g_variant_new_int32 (11));
  state = g_action_group_get_action_state (G_ACTION_GROUP (actions), "volume");
  g_assert_cmpint (g_variant_get_int32 (state), ==, 7);
  g_variant_unref (state);

  names = g_action_group_list_actions (G_ACTION_GROUP (actions));
  g_assert_cmpuint (g_strv_length (names), ==, G_N_ELEMENTS (entries));
  g_strfreev (names);

  g_action_map_remove_action_entries (G_ACTION_MAP (actions), entries, G_N_ELEMENTS (entries));
  names = g_action_group_list_actions (G_ACTION_GROUP (actions));
  g_assert_cmpuint (g_strv_length (names), ==, 0);
  g_strfreev (names);

  /* Check addition and removal of %NULL terminated array */
  g_action_map_add_action_entries (G_ACTION_MAP (actions), entries2, -1, NULL);
  names = g_action_group_list_actions (G_ACTION_GROUP (actions));
  g_assert_cmpuint (g_strv_length (names), ==, G_N_ELEMENTS (entries2) - 1);
  g_strfreev (names);
  g_action_map_remove_action_entries (G_ACTION_MAP (actions), entries2, -1);
  names = g_action_group_list_actions (G_ACTION_GROUP (actions));
  g_assert_cmpuint (g_strv_length (names), ==, 0);
  g_strfreev (names);

  g_object_unref (actions);
}

G_GNUC_END_IGNORE_DEPRECATIONS

static void
test_parse_detailed (void)
{
  struct {
    const gchar *detailed;
    const gchar *expected_name;
    const gchar *expected_target;
    const gchar *expected_error;
    const gchar *detailed_roundtrip;
  } testcases[] = {
    { "abc",              "abc",    NULL,       NULL,             "abc" },
    { " abc",             NULL,     NULL,       "invalid format", NULL },
    { " abc",             NULL,     NULL,       "invalid format", NULL },
    { "abc:",             NULL,     NULL,       "invalid format", NULL },
    { ":abc",             NULL,     NULL,       "invalid format", NULL },
    { "abc(",             NULL,     NULL,       "invalid format", NULL },
    { "abc)",             NULL,     NULL,       "invalid format", NULL },
    { "(abc",             NULL,     NULL,       "invalid format", NULL },
    { ")abc",             NULL,     NULL,       "invalid format", NULL },
    { "abc::xyz",         "abc",    "'xyz'",    NULL,             "abc::xyz" },
    { "abc('xyz')",       "abc",    "'xyz'",    NULL,             "abc::xyz" },
    { "abc(42)",          "abc",    "42",       NULL,             "abc(42)" },
    { "abc(int32 42)",    "abc",    "42",       NULL,             "abc(42)" },
    { "abc(@i 42)",       "abc",    "42",       NULL,             "abc(42)" },
    { "abc (42)",         NULL,     NULL,       "invalid format", NULL },
    { "abc(42abc)",       NULL,     NULL,       "invalid character in number", NULL },
    { "abc(42, 4)",       "abc",    "(42, 4)",  "expected end of input", NULL },
    { "abc(42,)",         "abc",    "(42,)",    "expected end of input", NULL }
  };
  gsize i;

  for (i = 0; i < G_N_ELEMENTS (testcases); i++)
    {
      GError *error = NULL;
      GVariant *target;
      gboolean success;
      gchar *name;

      success = g_action_parse_detailed_name (testcases[i].detailed, &name, &target, &error);
      g_assert_true (success == (error == NULL));
      if (success && testcases[i].expected_error)
        g_error ("Unexpected success on '%s'.  Expected error containing '%s'",
                 testcases[i].detailed, testcases[i].expected_error);

      if (!success && !testcases[i].expected_error)
        g_error ("Unexpected failure on '%s': %s", testcases[i].detailed, error->message);

      if (!success)
        {
          if (!strstr (error->message, testcases[i].expected_error))
            g_error ("Failure message '%s' for string '%s' did not contained expected substring '%s'",
                     error->message, testcases[i].detailed, testcases[i].expected_error);

          g_error_free (error);
          continue;
        }

      g_assert_cmpstr (name, ==, testcases[i].expected_name);
      g_assert_true ((target == NULL) == (testcases[i].expected_target == NULL));

      if (success)
        {
          gchar *detailed;

          detailed = g_action_print_detailed_name (name, target);
          g_assert_cmpstr (detailed, ==, testcases[i].detailed_roundtrip);
          g_free (detailed);
        }

      if (target)
        {
          GVariant *expected;

          expected = g_variant_parse (NULL, testcases[i].expected_target, NULL, NULL, NULL);
          g_assert_true (expected);

          g_assert_cmpvariant (expected, target);
          g_variant_unref (expected);
          g_variant_unref (target);
        }

      g_free (name);
    }
}

GHashTable *activation_counts;

static void
count_activation (const gchar *action)
{
  gint count;

  if (activation_counts == NULL)
    activation_counts = g_hash_table_new (g_str_hash, g_str_equal);
  count = GPOINTER_TO_INT (g_hash_table_lookup (activation_counts, action));
  count++;
  g_hash_table_insert (activation_counts, (gpointer)action, GINT_TO_POINTER (count));

  g_main_context_wakeup (NULL);
}

static gint
activation_count (const gchar *action)
{
  if (activation_counts == NULL)
    return 0;

  return GPOINTER_TO_INT (g_hash_table_lookup (activation_counts, action));
}

static void
activate_action (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  count_activation (g_action_get_name (G_ACTION (action)));
}

static void
activate_toggle (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  GVariant *old_state, *new_state;

  count_activation (g_action_get_name (G_ACTION (action)));

  old_state = g_action_get_state (G_ACTION (action));
  new_state = g_variant_new_boolean (!g_variant_get_boolean (old_state));
  g_simple_action_set_state (action, new_state);
  g_variant_unref (old_state);
}

static void
activate_radio (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  GVariant *new_state;

  count_activation (g_action_get_name (G_ACTION (action)));

  new_state = g_variant_new_string (g_variant_get_string (parameter, NULL));
  g_simple_action_set_state (action, new_state);
}

static gboolean
compare_action_groups (GActionGroup *a, GActionGroup *b)
{
  gchar **alist;
  gchar **blist;
  gint i;
  gboolean equal;
  gboolean ares, bres;
  gboolean aenabled, benabled;
  const GVariantType *aparameter_type, *bparameter_type;
  const GVariantType *astate_type, *bstate_type;
  GVariant *astate_hint, *bstate_hint;
  GVariant *astate, *bstate;

  alist = g_action_group_list_actions (a);
  blist = g_action_group_list_actions (b);
  equal = strv_strv_cmp ((const gchar * const *) alist, (const gchar * const *) blist);

  for (i = 0; equal && alist[i]; i++)
    {
      ares = g_action_group_query_action (a, alist[i], &aenabled, &aparameter_type, &astate_type, &astate_hint, &astate);
      bres = g_action_group_query_action (b, alist[i], &benabled, &bparameter_type, &bstate_type, &bstate_hint, &bstate);

      if (ares && bres)
        {
          equal = equal && (aenabled == benabled);
          equal = equal && ((!aparameter_type && !bparameter_type) || g_variant_type_equal (aparameter_type, bparameter_type));
          equal = equal && ((!astate_type && !bstate_type) || g_variant_type_equal (astate_type, bstate_type));
          equal = equal && ((!astate_hint && !bstate_hint) || g_variant_equal (astate_hint, bstate_hint));
          equal = equal && ((!astate && !bstate) || g_variant_equal (astate, bstate));

          if (astate_hint)
            g_variant_unref (astate_hint);
          if (bstate_hint)
            g_variant_unref (bstate_hint);
          if (astate)
            g_variant_unref (astate);
          if (bstate)
            g_variant_unref (bstate);
        }
      else
        equal = FALSE;
    }

  g_strfreev (alist);
  g_strfreev (blist);

  return equal;
}

static gboolean
timeout_cb (gpointer user_data)
{
  gboolean *timed_out = user_data;

  g_assert_false (*timed_out);
  *timed_out = TRUE;
  g_main_context_wakeup (NULL);

  return G_SOURCE_REMOVE;
}

static GActionEntry exported_entries[] = {
  { "undo",  activate_action, NULL, NULL,      NULL, { 0 } },
  { "redo",  activate_action, NULL, NULL,      NULL, { 0 } },
  { "cut",   activate_action, NULL, NULL,      NULL, { 0 } },
  { "copy",  activate_action, NULL, NULL,      NULL, { 0 } },
  { "paste", activate_action, NULL, NULL,      NULL, { 0 } },
  { "bold",  activate_toggle, NULL, "true",    NULL, { 0 } },
  { "lang",  activate_radio,  "s",  "'latin'", NULL, { 0 } },
};

static void
async_result_cb (GObject      *source,
                 GAsyncResult *res,
                 gpointer      user_data)
{
  GAsyncResult **result_out = user_data;

  g_assert_null (*result_out);
  *result_out = g_object_ref (res);

  g_main_context_wakeup (NULL);
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void
action_added_removed_cb (GActionGroup *action_group,
                         char         *action_name,
                         gpointer      user_data)
{
  guint *counter = user_data;

  *counter = *counter + 1;
  g_main_context_wakeup (NULL);
}

static void
action_enabled_changed_cb (GActionGroup *action_group,
                           char         *action_name,
                           gboolean      enabled,
                           gpointer      user_data)
{
  guint *counter = user_data;

  *counter = *counter + 1;
  g_main_context_wakeup (NULL);
}

static void
action_state_changed_cb (GActionGroup *action_group,
                         char         *action_name,
                         GVariant     *value,
                         gpointer      user_data)
{
  guint *counter = user_data;

  *counter = *counter + 1;
  g_main_context_wakeup (NULL);
}

static void
test_dbus_export (void)
{
  GDBusConnection *bus;
  GSimpleActionGroup *group;
  GDBusActionGroup *proxy;
  GSimpleAction *action;
  GError *error = NULL;
  GVariant *v;
  guint id;
  gchar **actions;
  guint n_actions_added = 0, n_actions_enabled_changed = 0, n_actions_removed = 0, n_actions_state_changed = 0;
  gulong added_signal_id, enabled_changed_signal_id, removed_signal_id, state_changed_signal_id;
  gboolean enabled;
  gchar *param;
  GVariantIter *iter;
  GAsyncResult *async_result = NULL;

  session_bus_up ();
  bus = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);

  group = g_simple_action_group_new ();
  g_simple_action_group_add_entries (group,
                                     exported_entries,
                                     G_N_ELEMENTS (exported_entries),
                                     NULL);

  id = g_dbus_connection_export_action_group (bus, "/", G_ACTION_GROUP (group), &error);
  g_assert_no_error (error);

  proxy = g_dbus_action_group_get (bus, g_dbus_connection_get_unique_name (bus), "/");
  added_signal_id = g_signal_connect (proxy, "action-added", G_CALLBACK (action_added_removed_cb), &n_actions_added);
  enabled_changed_signal_id = g_signal_connect (proxy, "action-enabled-changed", G_CALLBACK (action_enabled_changed_cb), &n_actions_enabled_changed);
  removed_signal_id = g_signal_connect (proxy, "action-removed", G_CALLBACK (action_added_removed_cb), &n_actions_removed);
  state_changed_signal_id = g_signal_connect (proxy, "action-state-changed", G_CALLBACK (action_state_changed_cb), &n_actions_state_changed);

  actions = g_action_group_list_actions (G_ACTION_GROUP (proxy));
  g_assert_cmpint (g_strv_length (actions), ==, 0);
  g_strfreev (actions);

  /* Actions are queried from the bus asynchronously after the first
   * list_actions() call. Wait for the expected signals then check again. */
  while (n_actions_added < G_N_ELEMENTS (exported_entries))
    g_main_context_iteration (NULL, TRUE);

  actions = g_action_group_list_actions (G_ACTION_GROUP (proxy));
  g_assert_cmpint (g_strv_length (actions), ==, G_N_ELEMENTS (exported_entries));
  g_strfreev (actions);

  /* check that calling "List" works too */
  g_dbus_connection_call (bus,
                          g_dbus_connection_get_unique_name (bus),
                          "/",
                          "org.gtk.Actions",
                          "List",
                          NULL,
                          NULL,
                          0,
                          G_MAXINT,
                          NULL,
                          async_result_cb,
                          &async_result);

  while (async_result == NULL)
    g_main_context_iteration (NULL, TRUE);

  v = g_dbus_connection_call_finish (bus, async_result, &error);
  g_assert_no_error (error);
  g_assert_nonnull (v);
  g_variant_get (v, "(^a&s)", &actions);
  g_assert_cmpuint (g_strv_length (actions), ==, G_N_ELEMENTS (exported_entries));
  g_free (actions);
  g_variant_unref (v);
  g_clear_object (&async_result);

  /* check that calling "Describe" works */
  g_dbus_connection_call (bus,
                          g_dbus_connection_get_unique_name (bus),
                          "/",
                          "org.gtk.Actions",
                          "Describe",
                          g_variant_new ("(s)", "copy"),
                          NULL,
                          0,
                          G_MAXINT,
                          NULL,
                          async_result_cb,
                          &async_result);

  while (async_result == NULL)
    g_main_context_iteration (NULL, TRUE);

  v = g_dbus_connection_call_finish (bus, async_result, &error);
  g_assert_no_error (error);
  g_assert_nonnull (v);
  /* FIXME: there's an extra level of tuplelization in here */
  g_variant_get (v, "((bgav))", &enabled, &param, &iter);
  g_assert_true (enabled);
  g_assert_cmpstr (param, ==, "");
  g_assert_cmpint (g_variant_iter_n_children (iter), ==, 0);
  g_free (param);
  g_variant_iter_free (iter);
  g_variant_unref (v);
  g_clear_object (&async_result);

  /* check that activating a parameterless action over D-Bus works */
  g_assert_cmpint (activation_count ("undo"), ==, 0);

  g_dbus_connection_call (bus,
                          g_dbus_connection_get_unique_name (bus),
                          "/",
                          "org.gtk.Actions",
                          "Activate",
                          g_variant_new ("(sava{sv})", "undo", NULL, NULL),
                          NULL,
                          0,
                          G_MAXINT,
                          NULL,
                          async_result_cb,
                          &async_result);

  while (async_result == NULL)
    g_main_context_iteration (NULL, TRUE);

  v = g_dbus_connection_call_finish (bus, async_result, &error);
  g_assert_no_error (error);
  g_assert_nonnull (v);
  g_assert_true (g_variant_is_of_type (v, G_VARIANT_TYPE_UNIT));
  g_variant_unref (v);
  g_clear_object (&async_result);

  g_assert_cmpint (activation_count ("undo"), ==, 1);

  /* check that activating a parameterful action over D-Bus works */
  g_assert_cmpint (activation_count ("lang"), ==, 0);
  ensure_state (G_ACTION_GROUP (group), "lang", "'latin'");

  g_dbus_connection_call (bus,
                          g_dbus_connection_get_unique_name (bus),
                          "/",
                          "org.gtk.Actions",
                          "Activate",
                          g_variant_new ("(s@ava{sv})", "lang", g_variant_new_parsed ("[<'spanish'>]"), NULL),
                          NULL,
                          0,
                          G_MAXINT,
                          NULL,
                          async_result_cb,
                          &async_result);

  while (async_result == NULL)
    g_main_context_iteration (NULL, TRUE);

  v = g_dbus_connection_call_finish (bus, async_result, &error);
  g_assert_no_error (error);
  g_assert_nonnull (v);
  g_assert_true (g_variant_is_of_type (v, G_VARIANT_TYPE_UNIT));
  g_variant_unref (v);
  g_clear_object (&async_result);

  g_assert_cmpint (activation_count ("lang"), ==, 1);
  ensure_state (G_ACTION_GROUP (group), "lang", "'spanish'");

  /* check that various error conditions are rejected */
  struct
    {
      const gchar *action_name;
      GVariant *parameter;  /* (owned floating) (nullable) */
    }
  activate_error_conditions[] =
    {
      { "nope", NULL },  /* non-existent action */
      { "lang", g_variant_new_parsed ("[<@u 4>]") },  /* wrong parameter type */
      { "lang", NULL },  /* parameter missing */
      { "undo", g_variant_new_parsed ("[<'silly'>]") },  /* extraneous parameter */
    };

  for (gsize i = 0; i < G_N_ELEMENTS (activate_error_conditions); i++)
    {
      GVariant *parameter = g_steal_pointer (&activate_error_conditions[i].parameter);
      const gchar *type_string = (parameter != NULL) ? "(s@ava{sv})" : "(sava{sv})";

      g_dbus_connection_call (bus,
                              g_dbus_connection_get_unique_name (bus),
                              "/",
                              "org.gtk.Actions",
                              "Activate",
                              g_variant_new (type_string,
                                             activate_error_conditions[i].action_name,
                                             g_steal_pointer (&parameter),
                                             NULL),
                              NULL,
                              0,
                              G_MAXINT,
                              NULL,
                              async_result_cb,
                              &async_result);

      while (async_result == NULL)
        g_main_context_iteration (NULL, TRUE);

      v = g_dbus_connection_call_finish (bus, async_result, &error);
      g_assert_error (error, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS);
      g_assert_null (v);
      g_clear_error (&error);
      g_clear_object (&async_result);
    }

  /* check that setting an action’s state over D-Bus works */
  g_assert_cmpint (activation_count ("lang"), ==, 1);
  ensure_state (G_ACTION_GROUP (group), "lang", "'spanish'");

  g_dbus_connection_call (bus,
                          g_dbus_connection_get_unique_name (bus),
                          "/",
                          "org.gtk.Actions",
                          "SetState",
                          g_variant_new ("(sva{sv})", "lang", g_variant_new_string ("portuguese"), NULL),
                          NULL,
                          0,
                          G_MAXINT,
                          NULL,
                          async_result_cb,
                          &async_result);

  while (async_result == NULL)
    g_main_context_iteration (NULL, TRUE);

  v = g_dbus_connection_call_finish (bus, async_result, &error);
  g_assert_no_error (error);
  g_assert_nonnull (v);
  g_assert_true (g_variant_is_of_type (v, G_VARIANT_TYPE_UNIT));
  g_variant_unref (v);
  g_clear_object (&async_result);

  g_assert_cmpint (activation_count ("lang"), ==, 1);
  ensure_state (G_ACTION_GROUP (group), "lang", "'portuguese'");

  /* check that various error conditions are rejected */
  struct
    {
      const gchar *action_name;
      GVariant *state;  /* (owned floating) (not nullable) */
    }
  set_state_error_conditions[] =
    {
      { "nope", g_variant_new_string ("hello") },  /* non-existent action */
      { "undo", g_variant_new_string ("not stateful") },  /* not a stateful action */
      { "lang", g_variant_new_uint32 (3) },  /* wrong state type */
    };

  for (gsize i = 0; i < G_N_ELEMENTS (set_state_error_conditions); i++)
    {
      g_dbus_connection_call (bus,
                              g_dbus_connection_get_unique_name (bus),
                              "/",
                              "org.gtk.Actions",
                              "SetState",
                              g_variant_new ("(s@va{sv})",
                                             set_state_error_conditions[i].action_name,
                                             g_variant_new_variant (g_steal_pointer (&set_state_error_conditions[i].state)),
                                             NULL),
                              NULL,
                              0,
                              G_MAXINT,
                              NULL,
                              async_result_cb,
                              &async_result);

      while (async_result == NULL)
        g_main_context_iteration (NULL, TRUE);

      v = g_dbus_connection_call_finish (bus, async_result, &error);
      g_assert_error (error, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS);
      g_assert_null (v);
      g_clear_error (&error);
      g_clear_object (&async_result);
    }

  /* test that the initial transfer works */
  g_assert_true (G_IS_DBUS_ACTION_GROUP (proxy));
  while (!compare_action_groups (G_ACTION_GROUP (group), G_ACTION_GROUP (proxy)))
    g_main_context_iteration (NULL, TRUE);
  n_actions_state_changed = 0;

  /* test that various changes get propagated from group to proxy */
  n_actions_added = 0;
  action = g_simple_action_new_stateful ("italic", NULL, g_variant_new_boolean (FALSE));
  g_simple_action_group_insert (group, G_ACTION (action));
  g_object_unref (action);

  while (n_actions_added == 0)
    g_main_context_iteration (NULL, TRUE);

  g_assert_true (compare_action_groups (G_ACTION_GROUP (group), G_ACTION_GROUP (proxy)));

  action = G_SIMPLE_ACTION (g_simple_action_group_lookup (group, "cut"));
  g_simple_action_set_enabled (action, FALSE);

  while (n_actions_enabled_changed == 0)
    g_main_context_iteration (NULL, TRUE);

  g_assert_true (compare_action_groups (G_ACTION_GROUP (group), G_ACTION_GROUP (proxy)));

  action = G_SIMPLE_ACTION (g_simple_action_group_lookup (group, "bold"));
  g_simple_action_set_state (action, g_variant_new_boolean (FALSE));

  while (n_actions_state_changed == 0)
    g_main_context_iteration (NULL, TRUE);

  g_assert_true (compare_action_groups (G_ACTION_GROUP (group), G_ACTION_GROUP (proxy)));

  g_simple_action_group_remove (group, "italic");

  while (n_actions_removed == 0)
    g_main_context_iteration (NULL, TRUE);

  g_assert_true (compare_action_groups (G_ACTION_GROUP (group), G_ACTION_GROUP (proxy)));

  /* test that activations and state changes propagate the other way */
  n_actions_state_changed = 0;
  g_assert_cmpint (activation_count ("copy"), ==, 0);
  g_action_group_activate_action (G_ACTION_GROUP (proxy), "copy", NULL);

  while (activation_count ("copy") == 0)
    g_main_context_iteration (NULL, TRUE);

  g_assert_cmpint (activation_count ("copy"), ==, 1);
  g_assert_true (compare_action_groups (G_ACTION_GROUP (group), G_ACTION_GROUP (proxy)));

  n_actions_state_changed = 0;
  g_assert_cmpint (activation_count ("bold"), ==, 0);
  g_action_group_activate_action (G_ACTION_GROUP (proxy), "bold", NULL);

  while (n_actions_state_changed == 0)
    g_main_context_iteration (NULL, TRUE);

  g_assert_cmpint (activation_count ("bold"), ==, 1);
  g_assert_true (compare_action_groups (G_ACTION_GROUP (group), G_ACTION_GROUP (proxy)));
  v = g_action_group_get_action_state (G_ACTION_GROUP (group), "bold");
  g_assert_true (g_variant_get_boolean (v));
  g_variant_unref (v);

  n_actions_state_changed = 0;
  g_action_group_change_action_state (G_ACTION_GROUP (proxy), "bold", g_variant_new_boolean (FALSE));

  while (n_actions_state_changed == 0)
    g_main_context_iteration (NULL, TRUE);

  g_assert_cmpint (activation_count ("bold"), ==, 1);
  g_assert_true (compare_action_groups (G_ACTION_GROUP (group), G_ACTION_GROUP (proxy)));
  v = g_action_group_get_action_state (G_ACTION_GROUP (group), "bold");
  g_assert_false (g_variant_get_boolean (v));
  g_variant_unref (v);

  g_dbus_connection_unexport_action_group (bus, id);

  g_signal_handler_disconnect (proxy, added_signal_id);
  g_signal_handler_disconnect (proxy, enabled_changed_signal_id);
  g_signal_handler_disconnect (proxy, removed_signal_id);
  g_signal_handler_disconnect (proxy, state_changed_signal_id);
  g_object_unref (proxy);
  g_object_unref (group);
  g_object_unref (bus);

  session_bus_down ();
}

static void
test_dbus_export_error_handling (void)
{
  GDBusConnection *bus = NULL;
  GSimpleActionGroup *group = NULL;
  GError *local_error = NULL;
  guint id1, id2;

  g_test_summary ("Test that error handling of action group export failure works");
  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/3366");

  session_bus_up ();
  bus = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);

  group = g_simple_action_group_new ();
  g_simple_action_group_add_entries (group,
                                     exported_entries,
                                     G_N_ELEMENTS (exported_entries),
                                     NULL);

  id1 = g_dbus_connection_export_action_group (bus, "/", G_ACTION_GROUP (group), &local_error);
  g_assert_no_error (local_error);
  g_assert_cmpuint (id1, !=, 0);

  /* Trigger a failure by trying to export on a path which is already in use */
  id2 = g_dbus_connection_export_action_group (bus, "/", G_ACTION_GROUP (group), &local_error);
  g_assert_error (local_error, G_IO_ERROR, G_IO_ERROR_EXISTS);
  g_assert_cmpuint (id2, ==, 0);
  g_clear_error (&local_error);

  g_dbus_connection_unexport_action_group (bus, id1);

  while (g_main_context_iteration (NULL, FALSE));

  g_object_unref (group);
  g_object_unref (bus);

  session_bus_down ();
}

static gpointer
do_export (gpointer data)
{
  GActionGroup *group = data;
  GMainContext *ctx;
  gint i;
  GError *error = NULL;
  guint id;
  GDBusConnection *bus;
  GAction *action;
  gchar *path;

  ctx = g_main_context_new ();

  g_main_context_push_thread_default (ctx);

  bus = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);
  path = g_strdup_printf("/%p", data);

  for (i = 0; i < 10000; i++)
    {
      id = g_dbus_connection_export_action_group (bus, path, G_ACTION_GROUP (group), &error);
      g_assert_no_error (error);

      action = g_simple_action_group_lookup (G_SIMPLE_ACTION_GROUP (group), "a");
      g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
                                   !g_action_get_enabled (action));

      g_dbus_connection_unexport_action_group (bus, id);

      while (g_main_context_iteration (ctx, FALSE));
    }

  g_free (path);
  g_object_unref (bus);

  g_main_context_pop_thread_default (ctx);

  g_main_context_unref (ctx);

  return NULL;
}

static void
test_dbus_threaded (void)
{
  GSimpleActionGroup *group[10];
  GThread *export[10];
  static GActionEntry entries[] = {
    { "a",  activate_action, NULL, NULL, NULL, { 0 } },
    { "b",  activate_action, NULL, NULL, NULL, { 0 } },
  };
  gint i;

  session_bus_up ();

  for (i = 0; i < 10; i++)
    {
      group[i] = g_simple_action_group_new ();
      g_simple_action_group_add_entries (group[i], entries, G_N_ELEMENTS (entries), NULL);
      export[i] = g_thread_new ("export", do_export, group[i]);
    }

  for (i = 0; i < 10; i++)
    g_thread_join (export[i]);

  for (i = 0; i < 10; i++)
    g_object_unref (group[i]);

  session_bus_down ();
}

G_GNUC_END_IGNORE_DEPRECATIONS

static void
test_bug679509 (void)
{
  GDBusConnection *bus;
  GDBusActionGroup *proxy;
  gboolean timed_out = FALSE;

  session_bus_up ();
  bus = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);

  proxy = g_dbus_action_group_get (bus, g_dbus_connection_get_unique_name (bus), "/");
  g_strfreev (g_action_group_list_actions (G_ACTION_GROUP (proxy)));
  g_object_unref (proxy);

  g_timeout_add (100, timeout_cb, &timed_out);
  while (!timed_out)
    g_main_context_iteration (NULL, TRUE);

  g_object_unref (bus);

  session_bus_down ();
}

static gchar *state_change_log;

static void
state_changed (GActionGroup *group,
               const gchar  *action_name,
               GVariant     *value,
               gpointer      user_data)
{
  GString *string;

  g_assert_false (state_change_log);

  string = g_string_new (action_name);
  g_string_append_c (string, ':');
  g_variant_print_string (value, string, TRUE);
  state_change_log = g_string_free (string, FALSE);
}

static void
verify_changed (const gchar *log_entry)
{
  g_assert_cmpstr (state_change_log, ==, log_entry);
  g_clear_pointer (&state_change_log, g_free);
}

static void
test_property_actions (void)
{
  GSimpleActionGroup *group;
  GPropertyAction *action;
  GSocketClient *client;
  GApplication *app;
  gchar *name;
  GVariantType *ptype, *stype;
  gboolean enabled;
  GVariant *state;

  group = g_simple_action_group_new ();
  g_signal_connect (group, "action-state-changed", G_CALLBACK (state_changed), NULL);

  client = g_socket_client_new ();
  app = g_application_new ("org.gtk.test", 0);

  /* string... */
  action = g_property_action_new ("app-id", app, "application-id");
  g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (action));
  g_object_unref (action);

  /* uint... */
  action = g_property_action_new ("keepalive", app, "inactivity-timeout");
  g_object_get (action, "name", &name, "parameter-type", &ptype, "enabled", &enabled, "state-type", &stype, "state", &state, NULL);
  g_assert_cmpstr (name, ==, "keepalive");
  g_assert_true (enabled);
  g_free (name);
  g_variant_type_free (ptype);
  g_variant_type_free (stype);
  g_variant_unref (state);

  g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (action));
  g_object_unref (action);

  /* bool... */
  action = g_property_action_new ("tls", client, "tls");
  g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (action));
  g_object_unref (action);

  /* inverted */
  action = g_object_new (G_TYPE_PROPERTY_ACTION,
                         "name", "disable-proxy",
                         "object", client,
                         "property-name", "enable-proxy",
                         "invert-boolean", TRUE,
                         NULL);
  g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (action));
  g_object_unref (action);

  /* enum... */
  action = g_property_action_new ("type", client, "type");
  g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (action));
  g_object_unref (action);

  /* the objects should be held alive by the actions... */
  g_object_unref (client);
  g_object_unref (app);

  ensure_state (G_ACTION_GROUP (group), "app-id", "'org.gtk.test'");
  ensure_state (G_ACTION_GROUP (group), "keepalive", "uint32 0");
  ensure_state (G_ACTION_GROUP (group), "tls", "false");
  ensure_state (G_ACTION_GROUP (group), "disable-proxy", "false");
  ensure_state (G_ACTION_GROUP (group), "type", "'stream'");

  verify_changed (NULL);

  /* some string tests... */
  g_action_group_change_action_state (G_ACTION_GROUP (group), "app-id", g_variant_new ("s", "org.gtk.test2"));
  verify_changed ("app-id:'org.gtk.test2'");
  g_assert_cmpstr (g_application_get_application_id (app), ==, "org.gtk.test2");
  ensure_state (G_ACTION_GROUP (group), "app-id", "'org.gtk.test2'");

  g_action_group_activate_action (G_ACTION_GROUP (group), "app-id", g_variant_new ("s", "org.gtk.test3"));
  verify_changed ("app-id:'org.gtk.test3'");
  g_assert_cmpstr (g_application_get_application_id (app), ==, "org.gtk.test3");
  ensure_state (G_ACTION_GROUP (group), "app-id", "'org.gtk.test3'");

  g_application_set_application_id (app, "org.gtk.test");
  verify_changed ("app-id:'org.gtk.test'");
  ensure_state (G_ACTION_GROUP (group), "app-id", "'org.gtk.test'");

  /* uint tests */
  g_action_group_change_action_state (G_ACTION_GROUP (group), "keepalive", g_variant_new ("u", 1234));
  verify_changed ("keepalive:uint32 1234");
  g_assert_cmpuint (g_application_get_inactivity_timeout (app), ==, 1234);
  ensure_state (G_ACTION_GROUP (group), "keepalive", "uint32 1234");

  g_action_group_activate_action (G_ACTION_GROUP (group), "keepalive", g_variant_new ("u", 5678));
  verify_changed ("keepalive:uint32 5678");
  g_assert_cmpuint (g_application_get_inactivity_timeout (app), ==, 5678);
  ensure_state (G_ACTION_GROUP (group), "keepalive", "uint32 5678");

  g_application_set_inactivity_timeout (app, 0);
  verify_changed ("keepalive:uint32 0");
  ensure_state (G_ACTION_GROUP (group), "keepalive", "uint32 0");

  /* bool tests */
  g_action_group_change_action_state (G_ACTION_GROUP (group), "tls", g_variant_new ("b", TRUE));
  verify_changed ("tls:true");
  g_assert_true (g_socket_client_get_tls (client));
  ensure_state (G_ACTION_GROUP (group), "tls", "true");

  g_action_group_change_action_state (G_ACTION_GROUP (group), "disable-proxy", g_variant_new ("b", TRUE));
  verify_changed ("disable-proxy:true");
  ensure_state (G_ACTION_GROUP (group), "disable-proxy", "true");
  g_assert_false (g_socket_client_get_enable_proxy (client));

  /* test toggle true->false */
  g_action_group_activate_action (G_ACTION_GROUP (group), "tls", NULL);
  verify_changed ("tls:false");
  g_assert_false (g_socket_client_get_tls (client));
  ensure_state (G_ACTION_GROUP (group), "tls", "false");

  /* and now back false->true */
  g_action_group_activate_action (G_ACTION_GROUP (group), "tls", NULL);
  verify_changed ("tls:true");
  g_assert_true (g_socket_client_get_tls (client));
  ensure_state (G_ACTION_GROUP (group), "tls", "true");

  g_socket_client_set_tls (client, FALSE);
  verify_changed ("tls:false");
  ensure_state (G_ACTION_GROUP (group), "tls", "false");

  /* now do the same for the inverted action */
  g_action_group_activate_action (G_ACTION_GROUP (group), "disable-proxy", NULL);
  verify_changed ("disable-proxy:false");
  g_assert_true (g_socket_client_get_enable_proxy (client));
  ensure_state (G_ACTION_GROUP (group), "disable-proxy", "false");

  g_action_group_activate_action (G_ACTION_GROUP (group), "disable-proxy", NULL);
  verify_changed ("disable-proxy:true");
  g_assert_false (g_socket_client_get_enable_proxy (client));
  ensure_state (G_ACTION_GROUP (group), "disable-proxy", "true");

  g_socket_client_set_enable_proxy (client, TRUE);
  verify_changed ("disable-proxy:false");
  ensure_state (G_ACTION_GROUP (group), "disable-proxy", "false");

  /* enum tests */
  g_action_group_change_action_state (G_ACTION_GROUP (group), "type", g_variant_new ("s", "datagram"));
  verify_changed ("type:'datagram'");
  g_assert_cmpint (g_socket_client_get_socket_type (client), ==, G_SOCKET_TYPE_DATAGRAM);
  ensure_state (G_ACTION_GROUP (group), "type", "'datagram'");

  g_action_group_activate_action (G_ACTION_GROUP (group), "type", g_variant_new ("s", "stream"));
  verify_changed ("type:'stream'");
  g_assert_cmpint (g_socket_client_get_socket_type (client), ==, G_SOCKET_TYPE_STREAM);
  ensure_state (G_ACTION_GROUP (group), "type", "'stream'");

  g_socket_client_set_socket_type (client, G_SOCKET_TYPE_SEQPACKET);
  verify_changed ("type:'seqpacket'");
  ensure_state (G_ACTION_GROUP (group), "type", "'seqpacket'");

  /* Check some error cases... */
  g_test_expect_message ("GLib-GIO", G_LOG_LEVEL_CRITICAL, "*non-existent*");
  action = g_property_action_new ("foo", app, "xyz");
  g_test_assert_expected_messages ();
  g_object_unref (action);

  g_test_expect_message ("GLib-GIO", G_LOG_LEVEL_CRITICAL, "*writable*");
  action = g_property_action_new ("foo", app, "is-registered");
  g_test_assert_expected_messages ();
  g_object_unref (action);

  g_test_expect_message ("GLib-GIO", G_LOG_LEVEL_CRITICAL, "*type 'GSocketAddress'*");
  action = g_property_action_new ("foo", client, "local-address");
  g_test_assert_expected_messages ();
  g_object_unref (action);

  g_object_unref (group);
}

static void
test_property_actions_no_properties (void)
{
  GPropertyAction *action;

  g_test_expect_message ("GLib-GIO", G_LOG_LEVEL_CRITICAL, "*Attempted to use an empty property name for GPropertyAction*");
  action = (GPropertyAction*) g_object_new_with_properties (G_TYPE_PROPERTY_ACTION, 0, NULL, NULL);

  g_test_assert_expected_messages ();
  g_assert_true (G_IS_PROPERTY_ACTION (action));

  g_object_unref (action);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/actions/basic", test_basic);
  g_test_add_func ("/actions/name", test_name);
  g_test_add_func ("/actions/simplegroup", test_simple_group);
  g_test_add_func ("/actions/stateful", test_stateful);
  g_test_add_func ("/actions/default-activate", test_default_activate);
  g_test_add_func ("/actions/entries", test_entries);
  g_test_add_func ("/actions/parse-detailed", test_parse_detailed);
  g_test_add_func ("/actions/dbus/export", test_dbus_export);
  g_test_add_func ("/actions/dbus/export/error-handling", test_dbus_export_error_handling);
  g_test_add_func ("/actions/dbus/threaded", test_dbus_threaded);
  g_test_add_func ("/actions/dbus/bug679509", test_bug679509);
  g_test_add_func ("/actions/property", test_property_actions);
  g_test_add_func ("/actions/no-properties", test_property_actions_no_properties);

  return g_test_run ();
}
