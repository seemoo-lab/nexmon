/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2018 Igalia S.L.
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

#include "mock-resolver.h"

struct _MockResolver
{
  GResolver parent_instance;
  guint ipv4_delay_ms;
  guint ipv6_delay_ms;
  GList *ipv4_results;
  GList *ipv6_results;
  GError *ipv4_error;
  GError *ipv6_error;
};

G_DEFINE_TYPE (MockResolver, mock_resolver, G_TYPE_RESOLVER)

MockResolver *
mock_resolver_new (void)
{
  return g_object_new (MOCK_TYPE_RESOLVER, NULL);
}

void
mock_resolver_set_ipv4_delay_ms (MockResolver *self, guint delay_ms)
{
  self->ipv4_delay_ms = delay_ms;
}

static gpointer
copy_object (gconstpointer obj, gpointer user_data)
{
  return g_object_ref (G_OBJECT (obj));
}

void
mock_resolver_set_ipv4_results (MockResolver *self, GList *results)
{
  if (self->ipv4_results)
    g_list_free_full (self->ipv4_results, g_object_unref);
  self->ipv4_results = g_list_copy_deep (results, copy_object, NULL);
}

void
mock_resolver_set_ipv4_error (MockResolver *self, GError *error)
{
  g_clear_error (&self->ipv4_error);
  if (error)
    self->ipv4_error = g_error_copy (error);
}

void
mock_resolver_set_ipv6_delay_ms (MockResolver *self, guint delay_ms)
{
  self->ipv6_delay_ms = delay_ms;
}

void
mock_resolver_set_ipv6_results (MockResolver *self, GList *results)
{
  if (self->ipv6_results)
    g_list_free_full (self->ipv6_results, g_object_unref);
  self->ipv6_results = g_list_copy_deep (results, copy_object, NULL);
}

void
mock_resolver_set_ipv6_error (MockResolver *self, GError *error)
{
  g_clear_error (&self->ipv6_error);
  if (error)
    self->ipv6_error = g_error_copy (error);
}

static gboolean lookup_by_name_cb (gpointer user_data);

/* Core of the implementation of `lookup_by_name()` in the mock resolver.
 *
 * It creates a #GSource which will become ready with the resolver results. It
 * will become ready either after a timeout, or as an idle callback. This
 * simulates doing some actual network-based resolution work.
 *
 * A previous implementation of this did the work in a thread, but that made it
 * hard to synchronise the timeouts with the #GResolver failure timeouts in the
 * calling thread, as spawning a worker thread could be subject to non-trivial
 * delays. */
static void
do_lookup_by_name (MockResolver             *self,
                   GTask                    *task,
                   GResolverNameLookupFlags  flags)
{
  GSource *source = NULL;

  g_task_set_task_data (task, GINT_TO_POINTER (flags), NULL);

  if (flags == G_RESOLVER_NAME_LOOKUP_FLAGS_IPV4_ONLY)
    source = g_timeout_source_new (self->ipv4_delay_ms);
  else if (flags == G_RESOLVER_NAME_LOOKUP_FLAGS_IPV6_ONLY)
    source = g_timeout_source_new (self->ipv6_delay_ms);
  else if (flags == G_RESOLVER_NAME_LOOKUP_FLAGS_DEFAULT)
    source = g_idle_source_new ();
  else
    g_assert_not_reached ();

  g_source_set_callback (source, lookup_by_name_cb, g_object_ref (task), g_object_unref);
  g_source_attach (source, g_main_context_get_thread_default ());
  g_source_unref (source);
}

static gboolean
lookup_by_name_cb (gpointer user_data)
{
  GTask *task = G_TASK (user_data);
  MockResolver *self = g_task_get_source_object (task);
  GResolverNameLookupFlags flags = GPOINTER_TO_INT (g_task_get_task_data (task));

  if (flags == G_RESOLVER_NAME_LOOKUP_FLAGS_IPV4_ONLY)
    {
      if (self->ipv4_error)
        g_task_return_error (task, g_error_copy (self->ipv4_error));
      else
        g_task_return_pointer (task, g_list_copy_deep (self->ipv4_results, copy_object, NULL), NULL);
    }
  else if (flags == G_RESOLVER_NAME_LOOKUP_FLAGS_IPV6_ONLY)
    {
      if (self->ipv6_error)
        g_task_return_error (task, g_error_copy (self->ipv6_error));
      else
        g_task_return_pointer (task, g_list_copy_deep (self->ipv6_results, copy_object, NULL), NULL);
    }
  else if (flags == G_RESOLVER_NAME_LOOKUP_FLAGS_DEFAULT)
    {
      /* This is only the minimal implementation needed for some tests */
      g_assert (self->ipv4_error == NULL && self->ipv6_error == NULL && self->ipv6_results == NULL);
      g_task_return_pointer (task, g_list_copy_deep (self->ipv4_results, copy_object, NULL), NULL);
    }
  else
    g_assert_not_reached ();

  return G_SOURCE_REMOVE;
}

static void
lookup_by_name_with_flags_async (GResolver                *resolver,
                                 const gchar              *hostname,
                                 GResolverNameLookupFlags  flags,
                                 GCancellable             *cancellable,
                                 GAsyncReadyCallback       callback,
                                 gpointer                  user_data)
{
  MockResolver *self = MOCK_RESOLVER (resolver);
  GTask *task = NULL;

  task = g_task_new (resolver, cancellable, callback, user_data);
  g_task_set_source_tag (task, lookup_by_name_with_flags_async);

  do_lookup_by_name (self, task, flags);

  g_object_unref (task);
}

static void
async_result_cb (GObject      *source_object,
                 GAsyncResult *result,
                 gpointer      user_data)
{
  GAsyncResult **result_out = user_data;

  g_assert (*result_out == NULL);
  *result_out = g_object_ref (result);

  g_main_context_wakeup (g_main_context_get_thread_default ());
}

static GList *
lookup_by_name (GResolver     *resolver,
                const gchar   *hostname,
                GCancellable  *cancellable,
                GError       **error)
{
  MockResolver *self = MOCK_RESOLVER (resolver);
  GMainContext *context = NULL;
  GList *result = NULL;
  GAsyncResult *async_result = NULL;
  GTask *task = NULL;

  context = g_main_context_new ();
  g_main_context_push_thread_default (context);

  task = g_task_new (resolver, cancellable, async_result_cb, &async_result);
  g_task_set_source_tag (task, lookup_by_name);

  /* Set up the resolution job. */
  do_lookup_by_name (self, task, G_RESOLVER_NAME_LOOKUP_FLAGS_DEFAULT);

  /* Wait for it to complete synchronously. */
  while (async_result == NULL)
    g_main_context_iteration (context, TRUE);

  result = g_task_propagate_pointer (G_TASK (async_result), error);
  g_object_unref (async_result);

  g_assert_finalize_object (task);

  g_main_context_pop_thread_default (context);
  g_main_context_unref (context);

  return g_steal_pointer (&result);
}

static GList *
lookup_by_name_with_flags_finish (GResolver     *resolver,
                                  GAsyncResult  *result,
                                  GError       **error)
{
  return g_task_propagate_pointer (G_TASK (result), error);
}

static void
mock_resolver_finalize (GObject *object)
{
  MockResolver *self = (MockResolver*)object;

  g_clear_error (&self->ipv4_error);
  g_clear_error (&self->ipv6_error);
  if (self->ipv6_results)
    g_list_free_full (self->ipv6_results, g_object_unref);
  if (self->ipv4_results)
    g_list_free_full (self->ipv4_results, g_object_unref);

  G_OBJECT_CLASS (mock_resolver_parent_class)->finalize (object);
}

static void
mock_resolver_class_init (MockResolverClass *klass)
{
  GResolverClass *resolver_class = G_RESOLVER_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  resolver_class->lookup_by_name_with_flags_async  = lookup_by_name_with_flags_async;
  resolver_class->lookup_by_name_with_flags_finish = lookup_by_name_with_flags_finish;
  resolver_class->lookup_by_name = lookup_by_name;
  object_class->finalize = mock_resolver_finalize;
}

static void
mock_resolver_init (MockResolver *self)
{
}
