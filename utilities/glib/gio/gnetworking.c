/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2011 Red Hat, Inc.
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

#include "gnetworking.h"
#include "gnetworkingprivate.h"

/**
 * g_networking_init:
 *
 * Initializes the platform networking libraries (eg, on Windows, this
 * calls WSAStartup()). GLib will call this itself if it is needed, so
 * you only need to call it if you directly call system networking
 * functions (without calling any GLib networking functions first).
 *
 * Since: 2.36
 */
void
g_networking_init (void)
{
#ifdef G_OS_WIN32
  static gsize inited = 0;

  if (g_once_init_enter (&inited))
    {
      WSADATA wsadata;

      if (WSAStartup (MAKEWORD (2, 0), &wsadata) != 0)
        g_error ("Windows Sockets could not be initialized");

      g_once_init_leave (&inited, 1);
    }
#endif
}

gboolean
g_getservbyname_ntohs (const char *name, const char *proto, guint16 *out_port)
{
  struct servent *result;

#ifdef HAVE_GETSERVBYNAME_R
  struct servent result_buf;
  char buf[2048];
  int r;

  r = getservbyname_r (name, proto, &result_buf, buf, sizeof (buf), &result);
  if (r != 0 || result != &result_buf)
    result = NULL;
#else
  result = getservbyname (name, proto);
#endif

  if (!result)
    return FALSE;
  *out_port = g_ntohs (result->s_port);
  return TRUE;
}
