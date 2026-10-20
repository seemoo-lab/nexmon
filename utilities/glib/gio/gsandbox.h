/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2022 Canonical Ltd
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

#ifndef __G_SANDBOX_H__
#define __G_SANDBOX_H__

#include <gio.h>

G_BEGIN_DECLS

/*
 * GSandboxType:
 * @G_SANDBOX_TYPE_UNKNOWN: process is running inside an unknown or no sandbox.
 * @G_SANDBOX_TYPE_FLATPAK: process is running inside a flatpak sandbox.
 * @G_SANDBOX_TYPE_SNAP: process is running inside a snap sandbox.
 *
 * The type of sandbox that processes can be running inside.
 */
typedef enum
{
  G_SANDBOX_TYPE_UNKNOWN,
  G_SANDBOX_TYPE_FLATPAK,
  G_SANDBOX_TYPE_SNAP
} GSandboxType;

GSandboxType glib_get_sandbox_type (void);

G_END_DECLS

#endif
