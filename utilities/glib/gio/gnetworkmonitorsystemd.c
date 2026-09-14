/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2026 Sorah Fukumori
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

#include <string.h>

#include "gnetworkmonitorsystemd.h"
#include "gcancellable.h"
#include "gioerror.h"
#include "ginitable.h"
#include "giomodule-priv.h"
#include "gnetworkmonitor.h"
#include "gdbusproxy.h"

#define G_NETWORK_MONITOR_SYSTEMD_GET_INITABLE_IFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE ((o), G_TYPE_INITABLE, GInitable))

static void g_network_monitor_systemd_iface_init (GNetworkMonitorInterface *iface);
static void g_network_monitor_systemd_initable_iface_init (GInitableIface *iface);

typedef enum
{
  PROP_NETWORK_AVAILABLE = 1,
  PROP_NETWORK_METERED,
  PROP_CONNECTIVITY
} GNetworkMonitorSystemdProperty;

/* Network monitor backend for systemd-networkd, via its
 * org.freedesktop.network1.Manager D-Bus interface. Ranks below the
 * NetworkManager backend, which reports richer system-wide state.
 *
 * This backend initializes itself asynchronously, to avoid blocking D-Bus
 * round trips during initable_init(). Until the connection to
 * systemd-networkd is established, network-available is FALSE. If networkd
 * is unavailable or not authoritative, it falls back to the netlink backend.
 * can_reach() is inherited from the netlink parent and works in every
 * state. */
struct _GNetworkMonitorSystemd
{
  GNetworkMonitorNetlink parent_instance;

  GCancellable *cancellable;  /* (owned) */

  /* At most one of proxy and fallback is set; neither while the async proxy
   * setup is still pending. */
  GDBusProxy *proxy;  /* (owned) (nullable) */
  unsigned long signal_id;

  /* Non-NULL only in fallback mode. The netlink backend disables its
   * availability tracking when subclassed, so a standalone instance is needed
   * to activate its full functionality. */
  GNetworkMonitorNetlink *fallback;  /* (owned) (nullable) */
  unsigned long fallback_network_changed_id;

  GNetworkConnectivity connectivity;
  gboolean network_available;
};

G_DEFINE_TYPE_WITH_CODE (GNetworkMonitorSystemd, g_network_monitor_systemd, G_TYPE_NETWORK_MONITOR_NETLINK,
                         G_IMPLEMENT_INTERFACE (G_TYPE_NETWORK_MONITOR,
                                                g_network_monitor_systemd_iface_init)
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE,
                                                g_network_monitor_systemd_initable_iface_init)
                         _g_io_modules_ensure_extension_points_registered ();
                         g_io_extension_point_implement (G_NETWORK_MONITOR_EXTENSION_POINT_NAME,
                                                         g_define_type_id,
                                                         "systemd",
                                                         25))

static void
g_network_monitor_systemd_init (GNetworkMonitorSystemd *self)
{
  self->connectivity = G_NETWORK_CONNECTIVITY_LOCAL;
  self->network_available = FALSE;
}

static void
g_network_monitor_systemd_get_property (GObject    *object,
                                        guint       prop_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  GNetworkMonitorSystemd *self = G_NETWORK_MONITOR_SYSTEMD (object);

  switch ((GNetworkMonitorSystemdProperty) prop_id)
    {
    case PROP_NETWORK_AVAILABLE:
      g_value_set_boolean (value, self->network_available);
      break;

    case PROP_NETWORK_METERED:
      /* systemd-networkd does not expose metered state. */
      g_value_set_boolean (value, FALSE);
      break;

    case PROP_CONNECTIVITY:
      g_value_set_enum (value, self->connectivity);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/* "routable" means at least one managed link has a global address, i.e. the host
 * can reach off-link destinations. OperationalState is used rather than
 * OnlineState because OnlineState is shaped by RequiredForOnline= policy and
 * treats link-local-only links as online. */
static gboolean
operational_state_is_available (GDBusProxy *proxy)
{
  GVariant *v;
  const char *state;
  gboolean available;

  v = g_dbus_proxy_get_cached_property (proxy, "OperationalState");
  if (v == NULL)
    return FALSE;

  state = g_variant_get_string (v, NULL);
  available = (g_strcmp0 (state, "routable") == 0);
  g_variant_unref (v);

  return available;
}

static gboolean
set_availability (GNetworkMonitorSystemd *self,
                  gboolean                new_available,
                  GNetworkConnectivity    new_connectivity)
{
  gboolean changed = FALSE;

  g_object_freeze_notify (G_OBJECT (self));
  if (new_available != self->network_available)
    {
      self->network_available = new_available;
      g_object_notify (G_OBJECT (self), "network-available");
      changed = TRUE;
    }
  if (new_connectivity != self->connectivity)
    {
      self->connectivity = new_connectivity;
      g_object_notify (G_OBJECT (self), "connectivity");
      changed = TRUE;
    }
  g_object_thaw_notify (G_OBJECT (self));

  return changed;
}

static void
sync_properties (GNetworkMonitorSystemd *self)
{
  gboolean new_available;
  GNetworkConnectivity new_connectivity;

  new_available = operational_state_is_available (self->proxy);
  /* networkd performs no internet-reachability or captive-portal check, so we
   * can only distinguish "routable" (FULL) from "not routable" (LOCAL). */
  new_connectivity = new_available ? G_NETWORK_CONNECTIVITY_FULL
                                   : G_NETWORK_CONNECTIVITY_LOCAL;

  if (set_availability (self, new_available, new_connectivity))
    g_signal_emit_by_name (self, "network-changed", new_available);
}

static void
proxy_properties_changed_cb (GDBusProxy             *proxy,
                             GVariant               *changed_properties,
                             GStrv                   invalidated_properties,
                             GNetworkMonitorSystemd *self)
{
  sync_properties (self);
}

/* networkd may be running without being the connectivity authority (e.g. it
 * manages no links). It then reports OnlineState "unknown". Only act as the
 * backend when we can confirm it is managing the network; otherwise decline so
 * a lower-priority backend (plain netlink) is used instead. */
static gboolean
networkd_is_authoritative (GDBusProxy *proxy)
{
  GVariant *v;
  const char *state;
  gboolean authoritative;

  v = g_dbus_proxy_get_cached_property (proxy, "OnlineState");
  if (v == NULL)
    return FALSE; /* networkd too old (pre-v249) to tell us; let netlink handle it. */

  state = g_variant_get_string (v, NULL);
  authoritative = g_strcmp0 (state, "unknown") != 0 && state[0] != '\0';
  g_variant_unref (v);

  return authoritative;
}

/* The netlink monitor emits network-changed on any default route change, not
 * only when availability flips; force_network_changed preserves that. */
static void
sync_properties_from_fallback (GNetworkMonitorSystemd *self,
                               gboolean                force_network_changed)
{
  gboolean available;
  GNetworkConnectivity connectivity;
  gboolean changed;

  g_object_get (self->fallback,
                "network-available", &available,
                "connectivity", &connectivity,
                NULL);

  changed = set_availability (self, available, connectivity);
  if (changed || force_network_changed)
    g_signal_emit_by_name (self, "network-changed", available);
}

static void
fallback_network_changed_cb (GNetworkMonitor        *fallback,
                             gboolean                network_available,
                             GNetworkMonitorSystemd *self)
{
  sync_properties_from_fallback (self, TRUE);
}

static void
start_fallback (GNetworkMonitorSystemd *self)
{
  GError *error = NULL;

  self->fallback = g_initable_new (G_TYPE_NETWORK_MONITOR_NETLINK,
                                   self->cancellable, &error, NULL);
  if (self->fallback == NULL)
    {
      g_debug ("GNetworkMonitorSystemd: could not create the netlink fallback monitor: %s",
               error->message);
      g_error_free (error);
      return;
    }

  self->fallback_network_changed_id =
      g_signal_connect (self->fallback, "network-changed",
                        G_CALLBACK (fallback_network_changed_cb), self);
  sync_properties_from_fallback (self, FALSE);
}

static void
proxy_ready_cb (GObject      *source_object,
                GAsyncResult *result,
                gpointer      user_data)
{
  GNetworkMonitorSystemd *self = user_data;
  GDBusProxy *proxy;
  GError *error = NULL;
  char *name_owner = NULL;

  proxy = g_dbus_proxy_new_for_bus_finish (result, &error);

  if (proxy == NULL)
    {
      /* Cancelled means the monitor is being disposed; leave it alone. */
      if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        {
          g_debug ("GNetworkMonitorSystemd: could not create the D-Bus proxy: %s",
                   error->message);
          start_fallback (self);
        }
      g_error_free (error);
      g_object_unref (self);
      return;
    }

  name_owner = g_dbus_proxy_get_name_owner (proxy);

  if (name_owner == NULL)
    {
      g_debug ("GNetworkMonitorSystemd: systemd-networkd is not running");
      g_object_unref (proxy);
      start_fallback (self);
    }
  else if (!networkd_is_authoritative (proxy))
    {
      g_debug ("GNetworkMonitorSystemd: systemd-networkd is not managing the network");
      g_object_unref (proxy);
      start_fallback (self);
    }
  else
    {
      self->signal_id = g_signal_connect (G_OBJECT (proxy), "g-properties-changed",
                                          G_CALLBACK (proxy_properties_changed_cb), self);
      self->proxy = g_steal_pointer (&proxy);
      sync_properties (self);
    }

  g_free (name_owner);
  g_object_unref (self);
}

static gboolean
g_network_monitor_systemd_initable_init (GInitable     *initable,
                                         GCancellable  *cancellable,
                                         GError       **error)
{
  GNetworkMonitorSystemd *self = G_NETWORK_MONITOR_SYSTEMD (initable);
  GInitableIface *parent_iface;

  /* Set up the netlink parent, which provides the route lookups used by
   * can_reach(). */
  parent_iface = g_type_interface_peek_parent (G_NETWORK_MONITOR_SYSTEMD_GET_INITABLE_IFACE (initable));
  if (!parent_iface->init (initable, cancellable, error))
    return FALSE;

  /* Whether networkd is usable is determined asynchronously in
   * proxy_ready_cb(), which holds a reference on the monitor. */
  self->cancellable = g_cancellable_new ();
  g_dbus_proxy_new_for_bus (G_BUS_TYPE_SYSTEM,
                            G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                            NULL,
                            "org.freedesktop.network1",
                            "/org/freedesktop/network1",
                            "org.freedesktop.network1.Manager",
                            self->cancellable,
                            proxy_ready_cb,
                            g_object_ref (self));

  return TRUE;
}

static void
g_network_monitor_systemd_dispose (GObject *object)
{
  GNetworkMonitorSystemd *self = G_NETWORK_MONITOR_SYSTEMD (object);

  g_cancellable_cancel (self->cancellable);
  g_clear_object (&self->cancellable);

  g_clear_signal_handler (&self->signal_id, self->proxy);
  g_clear_object (&self->proxy);

  g_clear_signal_handler (&self->fallback_network_changed_id, self->fallback);
  g_clear_object (&self->fallback);

  G_OBJECT_CLASS (g_network_monitor_systemd_parent_class)->dispose (object);
}

static void
g_network_monitor_systemd_class_init (GNetworkMonitorSystemdClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  gobject_class->dispose = g_network_monitor_systemd_dispose;
  gobject_class->get_property = g_network_monitor_systemd_get_property;

  g_object_class_override_property (gobject_class, PROP_NETWORK_AVAILABLE, "network-available");
  g_object_class_override_property (gobject_class, PROP_NETWORK_METERED, "network-metered");
  g_object_class_override_property (gobject_class, PROP_CONNECTIVITY, "connectivity");
}

static void
g_network_monitor_systemd_iface_init (GNetworkMonitorInterface *monitor_iface)
{
  /* can_reach() is inherited from the netlink backend. */
}

static void
g_network_monitor_systemd_initable_iface_init (GInitableIface *iface)
{
  iface->init = g_network_monitor_systemd_initable_init;
}
