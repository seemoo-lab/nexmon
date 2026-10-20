/* GLib testing framework examples and tests
 *
 * Copyright © 2026 Collabora Ltd.
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
 * Authors: Jakub Adam <jakub.adam@collabora.com>
 */

#include <gio/gio.h>
#include <gio/giptosmessage.h>
#include <gio/gnetworking.h>

/* See the g_test_skip() calls below for platform-specific reasons why this test
 * code sometimes needs to be skipped. */
#if ! (defined(G_OS_WIN32) || defined(__APPLE__) || defined(__GNU__) || defined(_AIX) || defined(__sun__))

static GSocketControlMessage *
send_recv_control_message (GSocketFamily family, GSocketControlMessage *msg)
{
  GSocket *wsock;
  GSocket *rsock;
  GInetAddress *addr;
  GSocketAddress *sockaddr;
  GOutputVector ov;
  GInputVector iv;
  GSocketControlMessage *result;
  GSocketControlMessage **mv;
  gint num_msg;
  GError *error = NULL;
  gchar recvbuf[20];
  gssize sent;

  const gchar MESSAGE[] = "TOSMESSAGE";

  wsock = g_socket_new (family, G_SOCKET_TYPE_DATAGRAM, G_SOCKET_PROTOCOL_UDP, &error);
  g_assert_no_error (error);

  rsock = g_socket_new (family, G_SOCKET_TYPE_DATAGRAM, G_SOCKET_PROTOCOL_UDP, &error);
  g_assert_no_error (error);

  addr = g_inet_address_new_loopback (family);
  sockaddr = g_inet_socket_address_new (addr, 0);

  g_socket_bind (wsock, sockaddr, TRUE, &error);
  g_assert_no_error (error);

  g_socket_bind (rsock, sockaddr, TRUE, &error);
  g_assert_no_error (error);

  g_clear_object (&sockaddr);
  g_clear_object (&addr);

  sockaddr = g_socket_get_local_address (rsock, &error);
  g_assert_no_error (error);

  if (family == G_SOCKET_FAMILY_IPV4)
    {
      g_socket_set_option (rsock, IPPROTO_IP, IP_RECVTOS, 1, &error);
    }
  else
    {
      g_socket_set_option (rsock, IPPROTO_IPV6, IPV6_RECVTCLASS, 1, &error);
    }
  g_assert_no_error (error);

  ov.buffer = MESSAGE;
  ov.size = strlen (MESSAGE) + 1;

  sent = g_socket_send_message (wsock, sockaddr, &ov, 1, &msg, 1, 0, NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (sent, ==, ov.size);

  iv.buffer = recvbuf;
  iv.size = sizeof (recvbuf);
  g_socket_receive_message (rsock, NULL, &iv, 1, &mv, &num_msg, NULL, NULL, &error);
  g_assert_no_error (error);

  g_assert_cmpstr (MESSAGE, ==, recvbuf);
  g_assert_cmpint (num_msg, ==, 1);

  result = mv[0];
  g_clear_pointer (&mv, g_free);

  g_clear_object (&sockaddr);
  g_clear_object (&wsock);
  g_clear_object (&rsock);

  return g_steal_pointer (&result);
}

const guint8 DSCP = 0x25;
const GEcnCodePoint ECN = G_ECN_ECT_0;

#endif

static void
test_ip_tos (void)
{
#if defined(G_OS_WIN32)
  g_test_skip ("GSocketControlMessage not supported on Windows.");
#elif defined(__APPLE__)
  g_test_skip ("IP_TOS not supported on macOS.");
#elif defined(__GNU__)
  g_test_skip ("IP_RECVTOS not supported on Hurd");
#elif defined(_AIX)
  g_test_skip ("IP_RECVTOS not supported on AIX");
#elif defined(__sun__)
  g_test_skip ("IP_RECVTOS not supported on Solaris");
#else
  GIPTosMessage *smsg;
  GIPTosMessage *rmsg;

  smsg = G_IP_TOS_MESSAGE (g_ip_tos_message_new (DSCP, ECN));
  rmsg = G_IP_TOS_MESSAGE (send_recv_control_message (G_SOCKET_FAMILY_IPV4, G_SOCKET_CONTROL_MESSAGE (smsg)));

  g_assert_cmpint (g_ip_tos_message_get_dscp (rmsg), ==, g_ip_tos_message_get_dscp (smsg));
  g_assert_cmpint (g_ip_tos_message_get_ecn (rmsg), ==, g_ip_tos_message_get_ecn (smsg));

  g_clear_object (&smsg);
  g_clear_object (&rmsg);
#endif
}

static void
test_ipv6_tclass (void)
{
#if defined(G_OS_WIN32)
  g_test_skip ("GSocketControlMessage not supported on Windows.");
#elif defined(__APPLE__)
  g_test_skip ("IPV6_TCLASS not supported on macOS.");
#elif defined(__GNU__)
  g_test_skip ("IPV6_RECVTCLASS not supported on Hurd");
#elif defined(_AIX)
  g_test_skip ("IPV6_RECVTCLASS not supported on AIX");
#elif defined(__sun__)
  g_test_skip ("IPV6_RECVTCLASS not supported on Solaris");
#else
  GIPv6TclassMessage *smsg;
  GIPv6TclassMessage *rmsg;

  smsg = G_IPV6_TCLASS_MESSAGE (g_ipv6_tclass_message_new (DSCP, ECN));
  rmsg = G_IPV6_TCLASS_MESSAGE (send_recv_control_message (G_SOCKET_FAMILY_IPV6, G_SOCKET_CONTROL_MESSAGE (smsg)));

  g_assert_cmpint (g_ipv6_tclass_message_get_dscp (rmsg), ==, g_ipv6_tclass_message_get_dscp (smsg));
  g_assert_cmpint (g_ipv6_tclass_message_get_ecn (rmsg), ==, g_ipv6_tclass_message_get_ecn (smsg));

  g_clear_object (&smsg);
  g_clear_object (&rmsg);
#endif
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/iptosmessage/iptos", test_ip_tos);
  g_test_add_func ("/iptosmessage/ipv6tclass", test_ipv6_tclass);

  return g_test_run ();
}
