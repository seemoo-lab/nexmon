/*
 * GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2022 Canonical Ltd.
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
 * Author: Marco Trevisan <marco.trevisan@canonical.com>
 */

#include <glib.h>

void cleanup_snapfiles (const gchar *path);

void create_fake_snap_yaml (const char *snap_path,
                            gboolean is_classic);

void create_fake_snapctl (const char *path,
                          const char *supported_op);

void create_fake_flatpak_info (const char  *root_path,
                               const GStrv shared_context,
                               const char  *dconf_dbus_policy);

void create_fake_flatpak_info_from_key_file (const char *root_path,
                                             GKeyFile   *key_file);
