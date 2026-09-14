/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: A parser for the XML GIR format
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

G_BEGIN_DECLS

#include "girmodule-private.h"

typedef struct _GIIrParser GIIrParser;

GIIrParser *gi_ir_parser_new          (void);
void        gi_ir_parser_free         (GIIrParser         *parser);
void        gi_ir_parser_set_debug    (GIIrParser          *parser,
                                       GLogLevelFlags       logged_levels);
void        gi_ir_parser_set_includes (GIIrParser         *parser,
                                       const char  *const *includes);

GIIrModule *gi_ir_parser_parse_string (GIIrParser   *parser,
                                       const char   *namespace,
                                       const char   *filename,
                                       const char   *buffer,
                                       gssize        length,
                                       GError      **error);
GIIrModule *gi_ir_parser_parse_file   (GIIrParser   *parser,
                                       const char   *filename,
                                       GError      **error);

G_END_DECLS
