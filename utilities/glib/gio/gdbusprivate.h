/* GDBus - GLib D-Bus Library
 *
 * Copyright (C) 2008-2010 Red Hat, Inc.
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
 * Author: David Zeuthen <davidz@redhat.com>
 */

#ifndef __G_DBUS_PRIVATE_H__
#define __G_DBUS_PRIVATE_H__

#include <gio/giotypes.h>

G_BEGIN_DECLS

/* Bus name, interface and object path of the message bus itself */
#define DBUS_SERVICE_DBUS "org.freedesktop.DBus"
#define DBUS_INTERFACE_DBUS DBUS_SERVICE_DBUS
#define DBUS_PATH_DBUS "/org/freedesktop/DBus"

/* Reserved by the specification for locally-generated messages */
#define DBUS_INTERFACE_LOCAL "org.freedesktop.DBus.Local"
#define DBUS_PATH_LOCAL "/org/freedesktop/DBus/Local"

/* Other well-known D-Bus interfaces from the specification */
#define DBUS_INTERFACE_INTROSPECTABLE "org.freedesktop.DBus.Introspectable"
#define DBUS_INTERFACE_OBJECT_MANAGER "org.freedesktop.DBus.ObjectManager"
#define DBUS_INTERFACE_PEER "org.freedesktop.DBus.Peer"
#define DBUS_INTERFACE_PROPERTIES "org.freedesktop.DBus.Properties"

/* Frequently-used D-Bus error names */
#define DBUS_ERROR_FAILED "org.freedesktop.DBus.Error.Failed"
#define DBUS_ERROR_INVALID_ARGS "org.freedesktop.DBus.Error.InvalidArgs"
#define DBUS_ERROR_NAME_HAS_NO_OWNER "org.freedesktop.DBus.Error.NameHasNoOwner"
#define DBUS_ERROR_UNKNOWN_METHOD "org.freedesktop.DBus.Error.UnknownMethod"

/* Owner flags */
#define DBUS_NAME_FLAG_ALLOW_REPLACEMENT 0x1 /**< Allow another service to become the primary owner if requested */
#define DBUS_NAME_FLAG_REPLACE_EXISTING  0x2 /**< Request to replace the current primary owner */
#define DBUS_NAME_FLAG_DO_NOT_QUEUE      0x4 /**< If we can not become the primary owner do not place us in the queue */

/* Replies to request for a name */
#define DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER  1 /**< Service has become the primary owner of the requested name */
#define DBUS_REQUEST_NAME_REPLY_IN_QUEUE       2 /**< Service could not become the primary owner and has been placed in the queue */
#define DBUS_REQUEST_NAME_REPLY_EXISTS         3 /**< Service is already in the queue */
#define DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER  4 /**< Service is already the primary owner */

/* Replies to releasing a name */
#define DBUS_RELEASE_NAME_REPLY_RELEASED        1 /**< Service was released from the given name */
#define DBUS_RELEASE_NAME_REPLY_NON_EXISTENT    2 /**< The given name does not exist on the bus */
#define DBUS_RELEASE_NAME_REPLY_NOT_OWNER       3 /**< Service is not an owner of the given name */

/* Replies to service starts */
#define DBUS_START_REPLY_SUCCESS         1 /**< Service was auto started */
#define DBUS_START_REPLY_ALREADY_RUNNING 2 /**< Service was already running */

/* ---------------------------------------------------------------------------------------------------- */

typedef struct GDBusWorker GDBusWorker;

typedef void (*GDBusWorkerMessageReceivedCallback) (GDBusWorker   *worker,
                                                    GDBusMessage  *message,
                                                    gpointer       user_data);

typedef GDBusMessage *(*GDBusWorkerMessageAboutToBeSentCallback) (GDBusWorker   *worker,
                                                                  GDBusMessage  *message,
                                                                  gpointer       user_data);

typedef void (*GDBusWorkerDisconnectedCallback)    (GDBusWorker   *worker,
                                                    gboolean       remote_peer_vanished,
                                                    GError        *error,
                                                    gpointer       user_data);

/* This function may be called from any thread - callbacks will be in the shared private message thread
 * and must not block.
 */
GDBusWorker *_g_dbus_worker_new          (GIOStream                          *stream,
                                          GDBusCapabilityFlags                capabilities,
                                          gboolean                            initially_frozen,
                                          GDBusWorkerMessageReceivedCallback  message_received_callback,
                                          GDBusWorkerMessageAboutToBeSentCallback message_about_to_be_sent_callback,
                                          GDBusWorkerDisconnectedCallback     disconnected_callback,
                                          gpointer                            user_data);

/* can be called from any thread - steals blob */
void         _g_dbus_worker_send_message (GDBusWorker    *worker,
                                          GDBusMessage   *message,
                                          gchar          *blob,
                                          gsize           blob_len);

/* can be called from any thread */
void         _g_dbus_worker_stop         (GDBusWorker    *worker);

/* can be called from any thread */
void         _g_dbus_worker_unfreeze     (GDBusWorker    *worker);

/* can be called from any thread (except the worker thread) */
gboolean     _g_dbus_worker_flush_sync   (GDBusWorker    *worker,
                                          GCancellable   *cancellable,
                                          GError        **error);

/* can be called from any thread */
void         _g_dbus_worker_close        (GDBusWorker         *worker,
                                          GTask               *task);

/* ---------------------------------------------------------------------------------------------------- */

void _g_dbus_initialize (void);
gboolean _g_dbus_debug_authentication (void);
gboolean _g_dbus_debug_transport (void);
gboolean _g_dbus_debug_message (void);
gboolean _g_dbus_debug_payload (void);
gboolean _g_dbus_debug_call    (void);
gboolean _g_dbus_debug_signal  (void);
gboolean _g_dbus_debug_incoming (void);
gboolean _g_dbus_debug_return (void);
gboolean _g_dbus_debug_emission (void);
gboolean _g_dbus_debug_address (void);
gboolean _g_dbus_debug_proxy (void);

void     _g_dbus_debug_print_lock (void);
void     _g_dbus_debug_print_unlock (void);

gboolean _g_dbus_address_parse_entry (const gchar  *address_entry,
                                      gchar       **out_transport_name,
                                      GHashTable  **out_key_value_pairs,
                                      GError      **error);

GVariantType * _g_dbus_compute_complete_signature (GDBusArgInfo **args);

gchar *_g_dbus_hexdump (const gchar *data, gsize len, guint indent);

/* ---------------------------------------------------------------------------------------------------- */

#ifdef G_OS_WIN32
gchar *_g_dbus_win32_get_user_sid (void);

#define _GDBUS_ARG_WIN32_RUN_SESSION_BUS "_win32_run_session_bus"
/* The g_win32_run_session_bus is exported from libgio dll on win32,
 * but still is NOT part of API/ABI since it is declared in private header
 * and used only by tool built from same sources.
 * Initially this function was introduces for usage with rundll,
 * so the signature is kept rundll-compatible, though parameters aren't used.
 */
_GIO_EXTERN void __stdcall
g_win32_run_session_bus (void* hwnd, void* hinst, const char* cmdline, int cmdshow);
gchar *_g_dbus_win32_get_session_address_dbus_launch (GError **error);
#endif

gchar *_g_dbus_get_machine_id (GError **error);

gchar *_g_dbus_enum_to_string (GType enum_type, gint value);

/* ---------------------------------------------------------------------------------------------------- */

GDBusMethodInvocation *_g_dbus_method_invocation_new (const gchar             *sender,
                                                      const gchar             *object_path,
                                                      const gchar             *interface_name,
                                                      const gchar             *method_name,
                                                      const GDBusMethodInfo   *method_info,
                                                      const GDBusPropertyInfo *property_info,
                                                      GDBusConnection         *connection,
                                                      GDBusMessage            *message,
                                                      GVariant                *parameters,
                                                      gpointer                 user_data);

/* ---------------------------------------------------------------------------------------------------- */

gboolean _g_signal_accumulator_false_handled (GSignalInvocationHint *ihint,
                                              GValue                *return_accu,
                                              const GValue          *handler_return,
                                              gpointer               dummy);

gboolean _g_dbus_object_skeleton_has_authorize_method_handlers (GDBusObjectSkeleton *object);

void _g_dbus_object_proxy_add_interface (GDBusObjectProxy *proxy,
                                         GDBusProxy       *interface_proxy);
void _g_dbus_object_proxy_remove_interface (GDBusObjectProxy *proxy,
                                            const gchar      *interface_name);

gchar *_g_dbus_hexencode (const gchar *str,
                          gsize        str_len);

/* Implemented in gdbusconnection.c */
GDBusConnection *_g_bus_get_singleton_if_exists (GBusType bus_type);
void             _g_bus_forget_singleton        (GBusType bus_type);

G_END_DECLS

#endif /* __G_DBUS_PRIVATE_H__ */
