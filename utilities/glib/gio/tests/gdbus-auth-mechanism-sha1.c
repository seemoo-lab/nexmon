/* GLib testing framework examples and tests
 *
 * Copyright (C) 2026 Philip Withnall
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
 * Author: Philip Withnall <pwithnall@gnome.org>
 */

#include <locale.h>
#include <gio/gio.h>

#include <string.h>
#include <unistd.h>

#include "gdbus-tests.h"

#ifdef G_OS_UNIX
#include <gio/gunixconnection.h>
#include <gio/gnetworkingprivate.h>
#include <gio/gunixsocketaddress.h>
#include <gio/gunixfdlist.h>
#endif

#define GIO_COMPILATION 1
#include "gdbusauthmechanism.h"
#include "gdbusauthmechanismsha1.h"

/* Vfunc wrappers copied from gdbusauthmechanism.c as they are not public. */
static gboolean
dbus_auth_mechanism_is_supported (GDBusAuthMechanism *mechanism)
{
  return G_DBUS_AUTH_MECHANISM_GET_CLASS (mechanism)->is_supported (mechanism);
}

static GDBusAuthMechanismState
dbus_auth_mechanism_client_get_state (GDBusAuthMechanism *mechanism)
{
  return G_DBUS_AUTH_MECHANISM_GET_CLASS (mechanism)->client_get_state (mechanism);
}

static gchar *
dbus_auth_mechanism_client_initiate (GDBusAuthMechanism   *mechanism,
                                     GDBusConnectionFlags  conn_flags,
                                     size_t               *out_initial_response_len)
{
  return G_DBUS_AUTH_MECHANISM_GET_CLASS (mechanism)->client_initiate (mechanism,
                                                                       conn_flags,
                                                                       out_initial_response_len);
}

static void
dbus_auth_mechanism_client_data_receive (GDBusAuthMechanism *mechanism,
                                         const char         *data,
                                         size_t              data_len)
{
  G_DBUS_AUTH_MECHANISM_GET_CLASS (mechanism)->client_data_receive (mechanism, data, data_len);
}

static char *
dbus_auth_mechanism_client_get_reject_reason (GDBusAuthMechanism *mechanism)
{
  return G_DBUS_AUTH_MECHANISM_GET_CLASS (mechanism)->client_get_reject_reason (mechanism);
}

static void
dbus_auth_mechanism_client_shutdown (GDBusAuthMechanism *mechanism)
{
  G_DBUS_AUTH_MECHANISM_GET_CLASS (mechanism)->client_shutdown (mechanism);
}

static void
test_server_challenge_validation (void)
{
  const struct
    {
      const char *server_challenge;
      const char *expected_reject_reason_prefix;
    }
  vectors[] = {
    { "valid_context 123 456", "Problems looking up entry in keyring" },
    { "invalid/context 123 456", "Malformed cookie_context" },
    { "invalid.context 123 456", "Malformed cookie_context" },
    { " 123 456", "Malformed cookie_context" },
    { "😀 123 456", "Malformed cookie_context" },
    { "invalid\ncontext 123 456", "Malformed cookie_context" },
    { "invalid\rcontext 123 456", "Malformed cookie_context" },
    { "invalid\tcontext 123 456", "Malformed cookie_context" },
    { "invalid\\context 123 456", "Malformed cookie_context" },
    { "valid_context  456", "Malformed cookie_id" },
    { "valid_context 123notanumber 456", "Malformed cookie_id" },
    { "valid_context -1 456", "Malformed cookie_id" },
    { "valid_context 4294967296 456", "Malformed cookie_id" },
    { "valid_context 123  ", "Malformed data" },
    { "valid_context ", "Malformed data" },
  };
  GType mechanism_type;
  GDBusConnection *connection = NULL;

  g_test_summary ("Test that GDBusAuthMechanismSha1 rejects various malformed server data lines");

  /* Briefly connect to the actual bus to ensure the GDBusAuth mechanisms are
   * all registered. */
  session_bus_up ();

  connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);
  g_assert_nonnull (connection);
  g_clear_object (&connection);

  session_bus_down ();

  /* Check that we now have the type ID for GDBusAuthMechanismSha1 */
  mechanism_type = g_type_from_name ("GDBusAuthMechanismSha1");
  g_assert_cmpint (mechanism_type,  !=, 0);

  for (size_t i = 0; i < G_N_ELEMENTS (vectors); i++)
    {
      GDBusAuthMechanism *mechanism = NULL;
      char *data = NULL;
      size_t data_len = 0;
      char *reject_reason = NULL;

      mechanism = g_object_new (mechanism_type, NULL);

      if (!dbus_auth_mechanism_is_supported (mechanism))
        {
          g_test_skip ("Mechanism not supported");
          g_clear_object (&mechanism);
          return;
        }

      data = dbus_auth_mechanism_client_initiate (mechanism,
                                                  G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
                                                  &data_len);
      g_free (data);

      dbus_auth_mechanism_client_data_receive (mechanism, vectors[i].server_challenge, strlen (vectors[i].server_challenge));

      g_assert_cmpint (dbus_auth_mechanism_client_get_state (mechanism), ==, G_DBUS_AUTH_MECHANISM_STATE_REJECTED);

      reject_reason = dbus_auth_mechanism_client_get_reject_reason (mechanism);
      g_assert_true (g_str_has_prefix (reject_reason, vectors[i].expected_reject_reason_prefix));
      g_free (reject_reason);

      dbus_auth_mechanism_client_shutdown (mechanism);

      g_clear_object (&mechanism);
    }
}

int
main (int   argc,
      char *argv[])
{
  setlocale (LC_ALL, "C");

  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  g_test_dbus_unset ();

  g_test_add_func ("/gdbus/auth-mechanism-sha1/server-challenge-validation", test_server_challenge_validation);

  return g_test_run ();
}
