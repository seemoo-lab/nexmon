/*
 * Copyright 2020 (C) Ruslan N. Marchenko <me@ruff.mobi>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include <gio/gio.h>

#include "gtesttlsbackend.h"

static void
get_tls_channel_binding (void)
{
  GTlsBackend *backend;
  gchar *not_null = "NOT_NULL";
  GTlsConnection *tls = NULL;
  GError *error = NULL;

  backend = g_tls_backend_get_default ();
  g_assert_nonnull (backend);

  /* check unimplemented GTlsConnection API sanity */
  tls = G_TLS_CONNECTION (g_object_new (
          g_tls_backend_get_client_connection_type (backend), NULL));
  g_assert_nonnull (tls);

  g_assert_false (g_tls_connection_get_channel_binding_data (tls, 
          G_TLS_CHANNEL_BINDING_TLS_UNIQUE, NULL, NULL));

  g_assert_false (g_tls_connection_get_channel_binding_data (tls, 
          G_TLS_CHANNEL_BINDING_TLS_UNIQUE, NULL, &error));
  g_assert_error (error, G_TLS_CHANNEL_BINDING_ERROR,
                         G_TLS_CHANNEL_BINDING_ERROR_NOT_IMPLEMENTED);
  g_clear_error (&error);

  if (g_test_subprocess ())
    g_assert_false (g_tls_connection_get_channel_binding_data (tls, 
            G_TLS_CHANNEL_BINDING_TLS_UNIQUE, NULL, (GError **)&not_null));

  g_object_unref (tls);
  g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*GLib-GIO-CRITICAL*");
}

static void
get_dtls_channel_binding (void)
{
  GTlsBackend *backend;
  gchar *not_null = "NOT_NULL";
  GDtlsConnection *dtls = NULL;
  GError *error = NULL;

  backend = g_tls_backend_get_default ();
  g_assert_nonnull (backend);

  /* repeat for the dtls now */
  dtls = G_DTLS_CONNECTION (g_object_new (
          g_tls_backend_get_dtls_client_connection_type (backend), NULL));
  g_assert_nonnull (dtls);

  g_assert_false (g_dtls_connection_get_channel_binding_data (dtls, 
          G_TLS_CHANNEL_BINDING_TLS_UNIQUE, NULL, NULL));

  g_assert_false (g_dtls_connection_get_channel_binding_data (dtls, 
          G_TLS_CHANNEL_BINDING_TLS_UNIQUE, NULL, &error));
  g_assert_error (error, G_TLS_CHANNEL_BINDING_ERROR,
                         G_TLS_CHANNEL_BINDING_ERROR_NOT_IMPLEMENTED);
  g_clear_error (&error);

  if (g_test_subprocess ())
    g_assert_false (g_dtls_connection_get_channel_binding_data (dtls, 
            G_TLS_CHANNEL_BINDING_TLS_UNIQUE, NULL, (GError **)&not_null));

  g_object_unref (dtls);
  g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*GLib-GIO-CRITICAL*");
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  _g_test_tls_backend_get_type ();

  g_test_add_func ("/tls-connection/get-tls-channel-binding", get_tls_channel_binding);
  g_test_add_func ("/tls-connection/get-dtls-channel-binding", get_dtls_channel_binding);

  return g_test_run ();
}
