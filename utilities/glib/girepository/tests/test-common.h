/*
 * Copyright 2024 Philip Chimento <philip.chimento@gmail.com>
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

#pragma once

#include "girepository.h"

typedef struct
{
  GIRepository *repository;
  char *gobject_typelib_dir;
} RepositoryFixture;

typedef struct
{
  const char *name;
  const char *version;
} TypelibLoadSpec;

static const TypelibLoadSpec typelib_load_spec_glib = { "GLib", "2.0" };
static const TypelibLoadSpec typelib_load_spec_gobject = { "GObject", "2.0" };
static const TypelibLoadSpec typelib_load_spec_gio = { "Gio", "2.0" };

void repository_init (int *argc,
                      char **argv[]);

void repository_setup (RepositoryFixture *fx,
                       const void *data);

void repository_teardown (RepositoryFixture *fx,
                          const void *unused);

#define ADD_REPOSITORY_TEST(testpath, ftest, load_spec) \
  g_test_add ((testpath), RepositoryFixture, load_spec, repository_setup, (ftest), repository_teardown)
