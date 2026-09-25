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
 * GIPTosMessage:
 *
 * Contains the type of service (ToS) byte of an IPv4 header.
 * 
 * This consists of the DSCP field as per
 * [RFC 2474](https://www.rfc-editor.org/rfc/rfc2474#section-3),
 * and the ECN field as per
 * [RFC 3168](https://www.rfc-editor.org/rfc/rfc3168#section-5).
 * 
 * It may be received using [method@Gio.Socket.receive_message] over UDP sockets
 * (i.e. sockets in the `G_SOCKET_FAMILY_IPV4` family with
 * `G_SOCKET_TYPE_DATAGRAM` type). The message is not meant for sending. To set
 * ToS field to be used in datagrams sent on a [class@Gio.Socket] use:
 * ```c
 * g_socket_set_option (socket, IPPROTO_IP, IP_TOS, <ToS value>, &error);
 * ```
 *
 * Since: 2.88
 */

#include "giptosmessage.h"

#include <gio/gnetworking.h>

struct _GIPTosMessage
{
  GSocketControlMessage parent_instance;

  /*< private >*/

  guint8 tos;
};

G_DEFINE_TYPE (GIPTosMessage, g_ip_tos_message, G_TYPE_SOCKET_CONTROL_MESSAGE);

static void
g_ip_tos_message_init (GIPTosMessage *message)
{
}

static gsize
g_ip_tos_message_get_size (GSocketControlMessage *message)
{
  return sizeof (guint8);
}

static int
g_ip_tos_message_get_level (GSocketControlMessage *message)
{
  return IPPROTO_IP;
}

static int
g_ip_tos_message_get_msg_type (GSocketControlMessage *message)
{
  return IP_TOS;
}

static void
g_ip_tos_message_serialize (GSocketControlMessage *message, gpointer data)
{
  *(guint8 *) data = G_IP_TOS_MESSAGE (message)->tos;
}

static GSocketControlMessage *
g_ip_tos_message_deserialize (gint level, gint type, gsize size, gpointer data)
{
  GIPTosMessage *message;

  if (level != IPPROTO_IP || type != IP_TOS)
    return NULL;

  if (size != sizeof (guint8))
    return NULL;

  message = g_object_new (G_TYPE_IP_TOS_MESSAGE, NULL);
  message->tos = *(guint8 *) data;

  return G_SOCKET_CONTROL_MESSAGE (message);
}

static void
g_ip_tos_message_class_init (GIPTosMessageClass *klass)
{
  GSocketControlMessageClass *scm_class = G_SOCKET_CONTROL_MESSAGE_CLASS (klass);

  scm_class->get_size = g_ip_tos_message_get_size;
  scm_class->get_level = g_ip_tos_message_get_level;
  scm_class->get_type = g_ip_tos_message_get_msg_type;
  scm_class->serialize = g_ip_tos_message_serialize;
  scm_class->deserialize = g_ip_tos_message_deserialize;
}

/**
 * g_ip_tos_message_new:
 * @dscp: the DSCP value of the message
 * @ecn: the ECN value of the message
 *
 * Creates a new type-of-service message with given DSCP and ECN values.
 *
 * Returns: (transfer full): a new type-of-service message
 * Since: 2.88
 */
GSocketControlMessage *
g_ip_tos_message_new (guint8 dscp, GEcnCodePoint ecn)
{
  GIPTosMessage *msg = g_object_new (G_TYPE_IP_TOS_MESSAGE, NULL);

  msg->tos = (dscp << 2) | ecn;

  return (GSocketControlMessage *) msg;
}

/**
 * g_ip_tos_message_get_dscp:
 * @message: a type-of-service message
 *
 * Gets the differentiated services code point stored in @message.
 *
 * Returns: A DSCP value as described in [RFC 2474](https://www.rfc-editor.org/rfc/rfc2474.html#section-3).
 *
 * Since: 2.88
 */
guint8
g_ip_tos_message_get_dscp (GIPTosMessage *message)
{
  return message->tos >> 2;
}

/**
 * g_ip_tos_message_get_ecn:
 * @message: a type-of-service message
 *
 * Gets the Explicit Congestion Notification code point stored in @message.
 *
 * Returns: An ECN value as described in [RFC 3168](https://www.rfc-editor.org/rfc/rfc3168#section-5).
 *
 * Since: 2.88
 */
GEcnCodePoint
g_ip_tos_message_get_ecn (GIPTosMessage *message)
{
  return message->tos & 0x3;
}
