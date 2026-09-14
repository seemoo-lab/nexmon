/*
 * Copyright © 2010, 2011, 2012 Codethink Limited
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
 * Authors: Ryan Lortie <desrt@desrt.ca>
 */

#include "giotypes.h"

typedef struct _GApplicationImpl GApplicationImpl;

typedef struct
{
  gchar *name;

  GVariantType *parameter_type;
  gboolean      enabled;
  GVariant     *state;
} RemoteActionInfo;

void                    g_application_impl_destroy                      (GApplicationImpl   *impl);

GApplicationImpl *      g_application_impl_register                     (GApplication        *application,
                                                                         const gchar         *appid,
                                                                         GApplicationFlags    flags,
                                                                         GActionGroup        *exported_actions,
                                                                         GRemoteActionGroup **remote_actions,
                                                                         GCancellable        *cancellable,
                                                                         GError             **error);

void                    g_application_impl_activate                     (GApplicationImpl   *impl,
                                                                         GVariant           *platform_data);

void                    g_application_impl_open                         (GApplicationImpl   *impl,
                                                                         GFile             **files,
                                                                         gint                n_files,
                                                                         const gchar        *hint,
                                                                         GVariant           *platform_data);

int                     g_application_impl_command_line                 (GApplicationImpl   *impl,
                                                                         const gchar *const *arguments,
                                                                         GVariant           *platform_data);

void                    g_application_impl_flush                        (GApplicationImpl   *impl);

GDBusConnection *       g_application_impl_get_dbus_connection          (GApplicationImpl   *impl);

const gchar *           g_application_impl_get_dbus_object_path         (GApplicationImpl   *impl);

void                    g_application_impl_set_busy_state               (GApplicationImpl   *impl,
                                                                         gboolean            busy);
