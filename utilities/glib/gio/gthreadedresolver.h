/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2008 Red Hat, Inc.
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

#ifndef __G_THREADED_RESOLVER_H__
#define __G_THREADED_RESOLVER_H__

#include <gio/gio.h>
#include <gio/gresolver.h>

G_BEGIN_DECLS

/**
 * GThreadedResolver:
 *
 * #GThreadedResolver is an implementation of #GResolver which calls the libc
 * lookup functions in threads to allow them to run asynchronously.
 *
 * Since: 2.20
 */
#define G_TYPE_THREADED_RESOLVER         (g_threaded_resolver_get_type ())

GIO_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (GThreadedResolver, g_threaded_resolver, G, THREADED_RESOLVER, GResolver)

G_END_DECLS

#endif /* __G_RESOLVER_H__ */
