/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2025 Collabora Ltd.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * See the included COPYING file for more information.
 *
 * Authors: Jakub Adam <jakub.adam@collabora.com>
 */

/**
 * GIPv6TclassMessage:
 *
 * Contains the Traffic Class byte of an IPv6 header.
 *
 * This consists of the DSCP field as per
 * [RFC 2474](https://www.rfc-editor.org/rfc/rfc2474#section-3),
 * and the ECN field as per
 * [RFC 3168](https://www.rfc-editor.org/rfc/rfc3168#section-5).
 *
 * It may be received using [method@Gio.Socket.receive_message] over UDP sockets
 * (i.e. sockets in the `G_SOCKET_FAMILY_IPV6` family with
 * `G_SOCKET_TYPE_DATAGRAM` type). The message is not meant for sending. To set
 * Traffic Class field to be used in datagrams sent on a [class@Gio.Socket] use:
 * ```c
 * g_socket_set_option (socket, IPPROTO_IPV6, IPV6_TCLASS, <TC value>, &error);
 * ```
 *
 * Since: 2.88
 */

#include "gipv6tclassmessage.h"

#include <gio/gnetworking.h>

struct _GIPv6TclassMessage
{
  GSocketControlMessage parent_instance;

  /*< private >*/

  guint8 tclass;
};

G_DEFINE_TYPE (GIPv6TclassMessage, g_ipv6_tclass_message, G_TYPE_SOCKET_CONTROL_MESSAGE);

static void
g_ipv6_tclass_message_init (GIPv6TclassMessage *message)
{
}

static gsize
g_ipv6_tclass_message_get_size (GSocketControlMessage *message)
{
  return sizeof (int);
}

static int
g_ipv6_tclass_message_get_level (GSocketControlMessage *message)
{
  return IPPROTO_IPV6;
}

static int
g_ipv6_tclass_message_get_msg_type (GSocketControlMessage *message)
{
  return IPV6_TCLASS;
}

static void
g_ipv6_tclass_message_serialize (GSocketControlMessage *message, gpointer data)
{
  *(int *) data = G_IPV6_TCLASS_MESSAGE (message)->tclass;
}

static GSocketControlMessage *
g_ipv6_tclass_message_deserialize (gint level, gint type, gsize size, gpointer data)
{
  GIPv6TclassMessage *message;

  if (level != IPPROTO_IPV6 || type != IPV6_TCLASS)
    return NULL;

  if (size != sizeof (int))
    return NULL;

  message = g_object_new (G_TYPE_IPV6_TCLASS_MESSAGE, NULL);
  message->tclass = *(int *) data;

  return G_SOCKET_CONTROL_MESSAGE (message);
}

static void
g_ipv6_tclass_message_class_init (GIPv6TclassMessageClass *klass)
{
  GSocketControlMessageClass *scm_class = G_SOCKET_CONTROL_MESSAGE_CLASS (klass);

  scm_class->get_size = g_ipv6_tclass_message_get_size;
  scm_class->get_level = g_ipv6_tclass_message_get_level;
  scm_class->get_type = g_ipv6_tclass_message_get_msg_type;
  scm_class->serialize = g_ipv6_tclass_message_serialize;
  scm_class->deserialize = g_ipv6_tclass_message_deserialize;
}

/**
 * g_ipv6_tclass_message_new:
 * @dscp: the DSCP value of the message
 * @ecn: the ECN value of the message
 *
 * Creates a new traffic class message with given DSCP and ECN values.
 *
 * Returns: (transfer full): a new traffic class message
 * Since: 2.88
 */
GSocketControlMessage *
g_ipv6_tclass_message_new (guint8 dscp, GEcnCodePoint ecn)
{
  GIPv6TclassMessage *msg = g_object_new (G_TYPE_IPV6_TCLASS_MESSAGE, NULL);

  msg->tclass = (dscp << 2) | ecn;

  return (GSocketControlMessage *) msg;
}

/**
 * g_ipv6_tclass_message_get_dscp:
 * @message: a traffic class message
 *
 * Gets the differentiated services code point stored in @message.
 *
 * Returns: A DSCP value as described in [RFC 2474](https://www.rfc-editor.org/rfc/rfc2474.html#section-3).
 *
 * Since: 2.88
 */
guint8
g_ipv6_tclass_message_get_dscp (GIPv6TclassMessage *message)
{
  return message->tclass >> 2;
}

/**
 * g_ipv6_tclass_message_get_ecn:
 * @message: a traffic class message
 *
 * Gets the Explicit Congestion Notification code point stored in @message.
 *
 * Returns: An ECN value as described in [RFC 3168](https://www.rfc-editor.org/rfc/rfc3168#section-5).
 *
 * Since: 2.88
 */
GEcnCodePoint
g_ipv6_tclass_message_get_ecn (GIPv6TclassMessage *message)
{
  return message->tclass & 0x3;
}
