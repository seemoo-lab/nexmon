/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: typelib validation, auxiliary functions
 * related to the binary typelib format
 *
 * Copyright (C) 2011 Colin Walters
 * Copyright (C) 2020 Gisle Vanem
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "gdump.c"

int
main (int    argc,
      char **argv)
{
  int i;
  GModule *self;

  self = g_module_open (NULL, 0);

  for (i = 1; i < argc; i++)
    {
      GError *error = NULL;
      GType type;

      type = invoke_get_type (self, argv[i], &error);
      if (!type)
        {
          g_printerr ("%s\n", error->message);
          g_clear_error (&error);
        }
      else
        dump_type (type, argv[i], stdout);
    }

  return 0;
}
