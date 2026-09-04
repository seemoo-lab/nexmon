/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: Parsed GIR
 *
 * Copyright 2023 GNOME Foundation Inc.
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

#pragma once

#include <glib.h>
#include <glib-object.h>

#include "girepository-private.h"
#include "gitypes.h"

G_BEGIN_DECLS

struct _GIBaseInfoClass
{
  GTypeClass parent_class;

  GIInfoType info_type;

  void (* finalize) (GIBaseInfo *info);
};

void            gi_base_info_init_types              (void);

GType           gi_base_info_type_register_static    (const char     *type_name,
                                                      size_t          instance_size,
                                                      GClassInitFunc  class_init,
                                                      GType           parent_type,
                                                      GTypeFlags      type_flags);

GIInfoType             gi_base_info_get_info_type    (GIBaseInfo   *info);

GIBaseInfo *           gi_base_info_new              (GIInfoType  type,
                                                      GIBaseInfo *container,
                                                      GITypelib  *typelib,
                                                      size_t      offset);

G_END_DECLS
