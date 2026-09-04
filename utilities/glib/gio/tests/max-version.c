/*
 * Copyright 2022 Collabora Ltd.
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
 * Public License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef GLIB_VERSION_MAX_ALLOWED
/* This is the oldest version macro available */
#define GLIB_VERSION_MIN_REQUIRED GLIB_VERSION_2_26
#define GLIB_VERSION_MAX_ALLOWED GLIB_VERSION_2_26
#endif

#include <glib.h>

/* All the headers that can validly be included in third-party code */
#include <gio/gio.h>
#include <gio/gnetworking.h>

#define G_SETTINGS_ENABLE_BACKEND
#include <gio/gsettingsbackend.h>

#ifdef G_OS_UNIX
#include <gio/gdesktopappinfo.h>
#include <gio/gfiledescriptorbased.h>
#include <gio/gunixconnection.h>
#include <gio/gunixcredentialsmessage.h>
#include <gio/gunixfdlist.h>
#include <gio/gunixfdmessage.h>
#include <gio/gunixinputstream.h>
#include <gio/gunixmounts.h>
#include <gio/gunixoutputstream.h>
#include <gio/gunixsocketaddress.h>
#endif

#ifdef G_OS_WIN32
#include <gio/gwin32inputstream.h>
#include <gio/gwin32outputstream.h>
#include <gio/gwin32registrykey.h>
#endif

#ifdef HAVE_COCOA
#include <gio/gosxappinfo.h>
#endif

static void
nothing (void)
{
  /* This doesn't really do anything: the real "test" is at compile time.
   * Just make sure the GIO library gets linked. */
  g_debug ("Loaded %s from GIO library", g_type_name (G_TYPE_CANCELLABLE));
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/max-version/tested-at-compile-time", nothing);
  return g_test_run ();
}
