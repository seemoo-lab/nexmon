/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: Parsed IDL
 *
 * Copyright (C) 2005 Matthias Clasen
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
#include "gitypelib-internal.h"

G_BEGIN_DECLS

typedef struct _GIIrTypelibBuild GIIrTypelibBuild;
typedef struct _GIIrModule GIIrModule;

struct _GIIrTypelibBuild {
  GIIrModule  *module;
  GHashTable  *strings;
  GHashTable  *types;
  GList       *nodes_with_attributes;
  uint32_t     n_attributes;
  uint8_t     *data;
  GList       *stack;
};

struct _GIIrModule
{
  char  *name;
  char  *version;
  char  *shared_library;
  char  *c_prefix;
  GPtrArray *dependencies; /* (owned) */
  GList *entries;

  /* All modules that are included directly or indirectly */
  GList *include_modules;

  /* Aliases defined in the module or in included modules */
  GHashTable *aliases;

  /* Structures with the 'pointer' flag (typedef struct _X *X)
   * in the module or in included modules
   */
  GHashTable *pointer_structures;
  /* Same as 'pointer' structures, but with the deprecated
   * 'disguised' flag
   */
  GHashTable *disguised_structures;
};

GIIrModule *gi_ir_module_new            (const char  *name,
                                         const char  *nsversion,
                                         const char  *module_filename,
                                         const char  *c_prefix);
void       gi_ir_module_free            (GIIrModule  *module);

void       gi_ir_module_add_include_module (GIIrModule  *module,
                                            GIIrModule  *include_module);

GITypelib * gi_ir_module_build_typelib (GIIrModule  *module);

void       gi_ir_module_fatal (GIIrTypelibBuild  *build,
                               unsigned int       line,
                               const char *msg,
                               ...) G_GNUC_PRINTF (3, 4) G_GNUC_NORETURN;

void gi_ir_node_init_stats (void);
void gi_ir_node_dump_stats (void);

G_END_DECLS
