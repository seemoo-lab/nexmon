/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: Repository
 *
 * Copyright (C) 2005 Matthias Clasen
 * Copyright (C) 2008,2009 Red Hat, Inc.
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

#include <glib-object.h>
#include <gmodule.h>

#include <girepository/gi-visibility.h>

#define __GIREPOSITORY_H_INSIDE__

#include <girepository/giarginfo.h>
#include <girepository/gibaseinfo.h>
#include <girepository/gicallableinfo.h>
#include <girepository/gicallbackinfo.h>
#include <girepository/giconstantinfo.h>
#include <girepository/gienuminfo.h>
#include <girepository/gifieldinfo.h>
#include <girepository/giflagsinfo.h>
#include <girepository/gifunctioninfo.h>
#include <girepository/giinterfaceinfo.h>
#include <girepository/giobjectinfo.h>
#include <girepository/gipropertyinfo.h>
#include <girepository/giregisteredtypeinfo.h>
#include <girepository/gisignalinfo.h>
#include <girepository/gistructinfo.h>
#include <girepository/gitypeinfo.h>
#include <girepository/gitypelib.h>
#include <girepository/gitypes.h>
#include <girepository/giunioninfo.h>
#include <girepository/giunresolvedinfo.h>
#include <girepository/givalueinfo.h>
#include <girepository/givfuncinfo.h>

G_BEGIN_DECLS

#define GI_TYPE_REPOSITORY (gi_repository_get_type ())
GI_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (GIRepository, gi_repository, GI, REPOSITORY, GObject)

/**
 * GIRepositoryLoadFlags:
 * @GI_REPOSITORY_LOAD_FLAG_NONE: No flags set.
 * @GI_REPOSITORY_LOAD_FLAG_LAZY: Lazily load the typelib.
 *
 * Flags that control how a typelib is loaded.
 *
 * Since: 2.80
 */
typedef enum
{
  GI_REPOSITORY_LOAD_FLAG_NONE = 0,
  GI_REPOSITORY_LOAD_FLAG_LAZY = 1 << 0
} G_GNUC_FLAG_ENUM GIRepositoryLoadFlags;

/* Repository */

GI_AVAILABLE_IN_ALL
GIRepository *gi_repository_new (void);

GI_AVAILABLE_IN_ALL
void          gi_repository_prepend_search_path (GIRepository *repository,
                                                 const char   *directory);

GI_AVAILABLE_IN_ALL
void          gi_repository_prepend_library_path (GIRepository *repository,
                                                  const char   *directory);

GI_AVAILABLE_IN_ALL
const char * const * gi_repository_get_search_path (GIRepository *repository,
                                                    size_t       *n_paths_out);

GI_AVAILABLE_IN_ALL
const char * const *gi_repository_get_library_path (GIRepository *repository,
                                                    size_t       *n_paths_out);

GI_AVAILABLE_IN_ALL
const char *  gi_repository_load_typelib  (GIRepository           *repository,
                                           GITypelib              *typelib,
                                           GIRepositoryLoadFlags   flags,
                                           GError                **error);

GI_AVAILABLE_IN_ALL
gboolean      gi_repository_is_registered (GIRepository *repository,
                                           const char   *namespace_,
                                           const char   *version);

GI_AVAILABLE_IN_ALL
GIBaseInfo *  gi_repository_find_by_name  (GIRepository *repository,
                                           const char   *namespace_,
                                           const char   *name);

GI_AVAILABLE_IN_ALL
char       ** gi_repository_enumerate_versions (GIRepository *repository,
                                                const char   *namespace_,
                                                size_t       *n_versions_out);

GI_AVAILABLE_IN_ALL
GITypelib *    gi_repository_require       (GIRepository           *repository,
                                            const char             *namespace_,
                                            const char             *version,
                                            GIRepositoryLoadFlags   flags,
                                            GError                **error);

GI_AVAILABLE_IN_ALL
GITypelib *    gi_repository_require_private (GIRepository           *repository,
                                              const char             *typelib_dir,
                                              const char             *namespace_,
                                              const char             *version,
                                              GIRepositoryLoadFlags   flags,
                                              GError                **error);

GI_AVAILABLE_IN_ALL
char       ** gi_repository_get_immediate_dependencies (GIRepository *repository,
                                                        const char   *namespace_,
                                                        size_t       *n_dependencies_out);

GI_AVAILABLE_IN_ALL
char       ** gi_repository_get_dependencies (GIRepository *repository,
                                              const char   *namespace_,
                                              size_t       *n_dependencies_out);

GI_AVAILABLE_IN_ALL
char       ** gi_repository_get_loaded_namespaces (GIRepository *repository,
                                                   size_t       *n_namespaces_out);

GI_AVAILABLE_IN_ALL
GIBaseInfo *  gi_repository_find_by_gtype (GIRepository *repository,
                                           GType         gtype);

GI_AVAILABLE_IN_ALL
void          gi_repository_get_object_gtype_interfaces (GIRepository      *repository,
                                                         GType              gtype,
                                                         size_t            *n_interfaces_out,
                                                         GIInterfaceInfo ***interfaces_out);

GI_AVAILABLE_IN_ALL
unsigned int  gi_repository_get_n_infos   (GIRepository *repository,
                                           const char   *namespace_);

GI_AVAILABLE_IN_ALL
GIBaseInfo *  gi_repository_get_info      (GIRepository *repository,
                                           const char   *namespace_,
                                           unsigned int  idx);

GI_AVAILABLE_IN_ALL
GIEnumInfo *  gi_repository_find_by_error_domain (GIRepository *repository,
                                                  GQuark        domain);

GI_AVAILABLE_IN_ALL
const char  * gi_repository_get_typelib_path   (GIRepository *repository,
                                                const char   *namespace_);
GI_AVAILABLE_IN_ALL
const char * const *gi_repository_get_shared_libraries (GIRepository *repository,
                                                        const char   *namespace_,
                                                        size_t       *out_n_elements);
GI_AVAILABLE_IN_ALL
const char  * gi_repository_get_c_prefix (GIRepository *repository,
                                          const char   *namespace_);
GI_AVAILABLE_IN_ALL
const char  * gi_repository_get_version (GIRepository *repository,
                                         const char   *namespace_);


GI_AVAILABLE_IN_ALL
GOptionGroup * gi_repository_get_option_group (void);


GI_AVAILABLE_IN_ALL
gboolean       gi_repository_dump  (const char  *input_filename,
                                    const char  *output_filename,
                                    GError     **error);

/**
 * GIRepositoryError:
 * @GI_REPOSITORY_ERROR_TYPELIB_NOT_FOUND: the typelib could not be found.
 * @GI_REPOSITORY_ERROR_NAMESPACE_MISMATCH: the namespace does not match the
 *   requested namespace.
 * @GI_REPOSITORY_ERROR_NAMESPACE_VERSION_CONFLICT: the version of the
 *   typelib does not match the requested version.
 * @GI_REPOSITORY_ERROR_LIBRARY_NOT_FOUND: the library used by the typelib
 *   could not be found.
 *
 * An error code used with `GI_REPOSITORY_ERROR` in a [type@GLib.Error]
 * returned from a [class@GIRepository.Repository] routine.
 *
 * Since: 2.80
 */
typedef enum
{
  GI_REPOSITORY_ERROR_TYPELIB_NOT_FOUND,
  GI_REPOSITORY_ERROR_NAMESPACE_MISMATCH,
  GI_REPOSITORY_ERROR_NAMESPACE_VERSION_CONFLICT,
  GI_REPOSITORY_ERROR_LIBRARY_NOT_FOUND
} GIRepositoryError;

/**
 * GI_REPOSITORY_ERROR:
 *
 * Error domain for [class@GIRepository.Repository].
 *
 * Errors in this domain will be from the [enum@GIRepository.Error] enumeration.
 * See [type@GLib.Error] for more information on error domains.
 *
 * Since: 2.80
 */
#define GI_REPOSITORY_ERROR (gi_repository_error_quark ())

GI_AVAILABLE_IN_ALL
GQuark gi_repository_error_quark (void);


/* Global utility functions */

GI_AVAILABLE_IN_ALL
void gi_cclosure_marshal_generic (GClosure       *closure,
                                  GValue         *return_gvalue,
                                  unsigned int    n_param_values,
                                  const GValue   *param_values,
                                  void           *invocation_hint,
                                  void           *marshal_data);

GI_AVAILABLE_IN_2_86
GIRepository *gi_repository_dup_default (void);

#include <girepository/girepository-autocleanups.h>

G_END_DECLS
