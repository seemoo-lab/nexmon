/*
 * Copyright © 2013 Lars Uebernickel
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
 * Authors: Lars Uebernickel <lars@uebernic.de>
 */

#include "gnotificationbackend.h"

#include "gnotification.h"
#include "gapplication.h"
#include "gactiongroup.h"
#include "giomodule-priv.h"

G_DEFINE_TYPE (GNotificationBackend, g_notification_backend, G_TYPE_OBJECT)

typedef enum
{
  PROP_APPLICATION = 1,
} GNotificationBackendProperty;

static GParamSpec *props[PROP_APPLICATION + 1];

/**
 * g_notification_backend_dup_application:
 *
 * Returns: (transfer full) (nullable):
 *  The application object this notification backend handles notifications for.
 */
GApplication *
g_notification_backend_dup_application (GNotificationBackend *self)
{
  g_return_val_if_fail (G_IS_NOTIFICATION_BACKEND (self), NULL);

  return (GApplication *) g_weak_ref_get (&self->application);
}

static void
g_notification_backend_get_property (GObject *obj,
                                     guint prop_id,
                                     GValue *value,
                                     GParamSpec *pspec)
{
  GNotificationBackend *self = G_NOTIFICATION_BACKEND (obj);

  switch ((GNotificationBackendProperty) prop_id)
    {
    case PROP_APPLICATION:
      {
        g_value_take_object (value, g_notification_backend_dup_application (self));
        break;
      }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
    }
}

static void
g_notification_backend_set_property (GObject *obj,
                                     guint prop_id,
                                     const GValue *value,
                                     GParamSpec *pspec)
{
  GNotificationBackend *self = G_NOTIFICATION_BACKEND (obj);

  switch ((GNotificationBackendProperty) prop_id)
    {
    case PROP_APPLICATION:
      {
        g_weak_ref_init (&self->application, g_value_get_object (value));
        break;
      }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
    }
}

static void
g_notification_backend_dispose (GObject *obj)
{
  GNotificationBackend *backend = G_NOTIFICATION_BACKEND (obj);

  g_weak_ref_clear (&backend->application);
  g_clear_object (&backend->dbus_connection);

  G_OBJECT_CLASS (g_notification_backend_parent_class)->dispose (obj);
}

static void
g_notification_backend_class_init (GNotificationBackendClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->dispose = g_notification_backend_dispose;
  object_class->get_property = g_notification_backend_get_property;
  object_class->set_property = g_notification_backend_set_property;

  /**
   * GNotificationBackend:application:
   *
   * The application object this notification backend handles
   * notifications for.
   *
   * Since: 2.90
   */
  props[PROP_APPLICATION] =
    g_param_spec_object ("application", NULL, NULL,
                         G_TYPE_APPLICATION,
                         G_PARAM_STATIC_STRINGS |
                         G_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (props), props);
}

static void
g_notification_backend_init (GNotificationBackend *backend)
{
}

GNotificationBackend *
g_notification_backend_new_default (GApplication *application)
{
  GType backend_type;
  GNotificationBackend *backend;

  g_return_val_if_fail (G_IS_APPLICATION (application), NULL);

  backend_type = _g_io_module_get_default_type (G_NOTIFICATION_BACKEND_EXTENSION_POINT_NAME,
                                                "GNOTIFICATION_BACKEND",
                                                G_STRUCT_OFFSET (GNotificationBackendClass, is_supported));

  backend = g_object_new (backend_type, "application", application, NULL);

  backend->dbus_connection = g_application_get_dbus_connection (application);
  if (backend->dbus_connection)
    g_object_ref (backend->dbus_connection);

  return backend;
}

void
g_notification_backend_send_notification (GNotificationBackend *backend,
                                          const gchar          *id,
                                          GNotification        *notification)
{
  g_return_if_fail (G_IS_NOTIFICATION_BACKEND (backend));
  g_return_if_fail (G_IS_NOTIFICATION (notification));

  G_NOTIFICATION_BACKEND_GET_CLASS (backend)->send_notification (backend, id, notification);
}

void
g_notification_backend_withdraw_notification (GNotificationBackend *backend,
                                              const gchar          *id)
{
  g_return_if_fail (G_IS_NOTIFICATION_BACKEND (backend));
  g_return_if_fail (id != NULL);

  G_NOTIFICATION_BACKEND_GET_CLASS (backend)->withdraw_notification (backend, id);
}
