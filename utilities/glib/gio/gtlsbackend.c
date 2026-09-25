/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2010 Red Hat, Inc
 * Copyright © 2015 Collabora, Ltd.
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

#include "config.h"
#include "glib.h"

#include "gtlsbackend.h"
#include "gtlsdatabase.h"
#include "gdummytlsbackend.h"
#include "gioenumtypes.h"
#include "giomodule-priv.h"

/**
 * GTlsBackend:
 *
 * TLS (Transport Layer Security, aka SSL) and DTLS backend. This is an
 * internal type used to coordinate the different classes implemented
 * by a TLS backend.
 *
 * Since: 2.28
 */

G_DEFINE_INTERFACE (GTlsBackend, g_tls_backend, G_TYPE_OBJECT)

static GTlsDatabase *default_database;
G_LOCK_DEFINE_STATIC (default_database_lock);

static void
g_tls_backend_default_init (GTlsBackendInterface *iface)
{
}

static GTlsBackend *tls_backend_default_singleton = NULL;  /* (owned) (atomic) */

/**
 * g_tls_backend_get_default:
 *
 * Gets the default #GTlsBackend for the system.
 *
 * Returns: (not nullable) (transfer none): a #GTlsBackend, which will be a
 *     dummy object if no TLS backend is available
 *
 * Since: 2.28
 */
GTlsBackend *
g_tls_backend_get_default (void)
{
  if (g_once_init_enter_pointer (&tls_backend_default_singleton))
    {
      GTlsBackend *singleton;

      singleton = _g_io_module_get_default (G_TLS_BACKEND_EXTENSION_POINT_NAME,
                                            "GIO_USE_TLS",
                                            NULL);

      g_once_init_leave_pointer (&tls_backend_default_singleton, singleton);
    }

  return tls_backend_default_singleton;
}

/**
 * g_tls_backend_supports_tls:
 * @backend: the #GTlsBackend
 *
 * Checks if TLS is supported; if this returns %FALSE for the default
 * #GTlsBackend, it means no "real" TLS backend is available.
 *
 * Returns: whether or not TLS is supported
 *
 * Since: 2.28
 */
gboolean
g_tls_backend_supports_tls (GTlsBackend *backend)
{
  if (G_TLS_BACKEND_GET_INTERFACE (backend)->supports_tls)
    return G_TLS_BACKEND_GET_INTERFACE (backend)->supports_tls (backend);
  else if (G_IS_DUMMY_TLS_BACKEND (backend))
    return FALSE;
  else
    return TRUE;
}

/**
 * g_tls_backend_supports_dtls:
 * @backend: the #GTlsBackend
 *
 * Checks if DTLS is supported. DTLS support may not be available even if TLS
 * support is available, and vice-versa.
 *
 * Returns: whether DTLS is supported
 *
 * Since: 2.48
 */
gboolean
g_tls_backend_supports_dtls (GTlsBackend *backend)
{
  if (G_TLS_BACKEND_GET_INTERFACE (backend)->supports_dtls)
    return G_TLS_BACKEND_GET_INTERFACE (backend)->supports_dtls (backend);

  return FALSE;
}

/**
 * g_tls_backend_get_default_database:
 * @backend: the #GTlsBackend
 *
 * Gets the default #GTlsDatabase used to verify TLS connections.
 *
 * Returns: (transfer full): the default database, which should be
 *               unreffed when done.
 *
 * Since: 2.30
 */
GTlsDatabase *
g_tls_backend_get_default_database (GTlsBackend *backend)
{
  GTlsDatabase *db;

  g_return_val_if_fail (G_IS_TLS_BACKEND (backend), NULL);

  /* This method was added later, so accept the (remote) possibility it can be NULL */
  if (!G_TLS_BACKEND_GET_INTERFACE (backend)->get_default_database)
    return NULL;

  G_LOCK (default_database_lock);

  if (!default_database)
    default_database = G_TLS_BACKEND_GET_INTERFACE (backend)->get_default_database (backend);
  db = default_database ? g_object_ref (default_database) : NULL;
  G_UNLOCK (default_database_lock);

  return db;
}

/**
 * g_tls_backend_set_default_database:
 * @backend: the #GTlsBackend
 * @database: (nullable): the #GTlsDatabase
 *
 * Set the default #GTlsDatabase used to verify TLS connections
 *
 * Any subsequent call to g_tls_backend_get_default_database() will return
 * the database set in this call.  Existing databases and connections are not
 * modified.
 *
 * Setting a %NULL default database will reset to using the system default
 * database as if g_tls_backend_set_default_database() had never been called.
 *
 * Since: 2.60
 */
void
g_tls_backend_set_default_database (GTlsBackend  *backend,
                                    GTlsDatabase *database)
{
  g_return_if_fail (G_IS_TLS_BACKEND (backend));
  g_return_if_fail (database == NULL || G_IS_TLS_DATABASE (database));

  G_LOCK (default_database_lock);
  g_set_object (&default_database, database);
  G_UNLOCK (default_database_lock);
}

/**
 * g_tls_backend_get_certificate_type:
 * @backend: the #GTlsBackend
 *
 * Gets the #GType of @backend's #GTlsCertificate implementation.
 *
 * Returns: the #GType of @backend's #GTlsCertificate
 *   implementation.
 *
 * Since: 2.28
 */
GType
g_tls_backend_get_certificate_type (GTlsBackend *backend)
{
  return G_TLS_BACKEND_GET_INTERFACE (backend)->get_certificate_type ();
}

/**
 * g_tls_backend_get_client_connection_type:
 * @backend: the #GTlsBackend
 *
 * Gets the #GType of @backend's #GTlsClientConnection implementation.
 *
 * Returns: the #GType of @backend's #GTlsClientConnection
 *   implementation.
 *
 * Since: 2.28
 */
GType
g_tls_backend_get_client_connection_type (GTlsBackend *backend)
{
  return G_TLS_BACKEND_GET_INTERFACE (backend)->get_client_connection_type ();
}

/**
 * g_tls_backend_get_server_connection_type:
 * @backend: the #GTlsBackend
 *
 * Gets the #GType of @backend's #GTlsServerConnection implementation.
 *
 * Returns: the #GType of @backend's #GTlsServerConnection
 *   implementation.
 *
 * Since: 2.28
 */
GType
g_tls_backend_get_server_connection_type (GTlsBackend *backend)
{
  return G_TLS_BACKEND_GET_INTERFACE (backend)->get_server_connection_type ();
}

/**
 * g_tls_backend_get_dtls_client_connection_type:
 * @backend: the #GTlsBackend
 *
 * Gets the #GType of @backend’s #GDtlsClientConnection implementation.
 *
 * Returns: the #GType of @backend’s #GDtlsClientConnection
 *   implementation, or %G_TYPE_INVALID if this backend doesn’t support DTLS.
 *
 * Since: 2.48
 */
GType
g_tls_backend_get_dtls_client_connection_type (GTlsBackend *backend)
{
  GTlsBackendInterface *iface;

  g_return_val_if_fail (G_IS_TLS_BACKEND (backend), G_TYPE_INVALID);

  iface = G_TLS_BACKEND_GET_INTERFACE (backend);
  if (iface->get_dtls_client_connection_type == NULL)
    return G_TYPE_INVALID;

  return iface->get_dtls_client_connection_type ();
}

/**
 * g_tls_backend_get_dtls_server_connection_type:
 * @backend: the #GTlsBackend
 *
 * Gets the #GType of @backend’s #GDtlsServerConnection implementation.
 *
 * Returns: the #GType of @backend’s #GDtlsServerConnection
 *   implementation, or %G_TYPE_INVALID if this backend doesn’t support DTLS.
 *
 * Since: 2.48
 */
GType
g_tls_backend_get_dtls_server_connection_type (GTlsBackend *backend)
{
  GTlsBackendInterface *iface;

  g_return_val_if_fail (G_IS_TLS_BACKEND (backend), G_TYPE_INVALID);

  iface = G_TLS_BACKEND_GET_INTERFACE (backend);
  if (iface->get_dtls_server_connection_type == NULL)
    return G_TYPE_INVALID;

  return iface->get_dtls_server_connection_type ();
}

/**
 * g_tls_backend_get_file_database_type:
 * @backend: the #GTlsBackend
 *
 * Gets the #GType of @backend's #GTlsFileDatabase implementation.
 *
 * Returns: the #GType of backend's #GTlsFileDatabase implementation.
 *
 * Since: 2.30
 */
GType
g_tls_backend_get_file_database_type (GTlsBackend *backend)
{
  g_return_val_if_fail (G_IS_TLS_BACKEND (backend), 0);

  /* This method was added later, so accept the (remote) possibility it can be NULL */
  if (!G_TLS_BACKEND_GET_INTERFACE (backend)->get_file_database_type)
    return 0;

  return G_TLS_BACKEND_GET_INTERFACE (backend)->get_file_database_type ();
}
