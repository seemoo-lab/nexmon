/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: Repository implementation
 *
 * Copyright (C) 2005 Matthias Clasen
 * Copyright (C) 2008 Colin Walters <walters@verbum.org>
 * Copyright (C) 2008 Red Hat, Inc.
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

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <glib.h>
#include <glib-private.h>
#include <glib/gprintf.h>
#include <gmodule.h>
#include "gibaseinfo-private.h"
#include "girepository.h"
#include "gitypelib-internal.h"
#include "girepository-private.h"

/**
 * GIRepository:
 *
 * `GIRepository` is used to manage repositories of namespaces. Namespaces
 * are represented on disk by type libraries (`.typelib` files).
 *
 * The individual pieces of API within a type library are represented by
 * subclasses of [class@GIRepository.BaseInfo]. These can be found using
 * methods like [method@GIRepository.Repository.find_by_name] or
 * [method@GIRepository.Repository.get_info].
 *
 * You are responsible for ensuring that the lifetime of the
 * [class@GIRepository.Repository] exceeds that of the lifetime of any of its
 * [class@GIRepository.BaseInfo]s. This cannot be guaranteed by using internal
 * references within libgirepository as that would affect performance.
 *
 * ### Discovery of type libraries
 *
 * `GIRepository` will typically look for a `girepository-1.0` directory
 * under the library directory used when compiling gobject-introspection. On a
 * standard Linux system this will end up being `/usr/lib/girepository-1.0`.
 *
 * It is possible to control the search paths programmatically, using
 * [method@GIRepository.Repository.prepend_search_path]. It is also possible to
 * modify the search paths by using the `GI_TYPELIB_PATH` environment variable.
 * The environment variable takes precedence over the default search path
 * and the [method@GIRepository.Repository.prepend_search_path] calls.
 *
 * ### Namespace ordering
 *
 * In situations where namespaces may be searched in order, or returned in a
 * list, the namespaces will be returned in alphabetical order, with all fully
 * loaded namespaces being returned before any lazily loaded ones (those loaded
 * with `GI_REPOSITORY_LOAD_FLAG_LAZY`). This allows for deterministic and
 * reproducible results.
 *
 * Similarly, if a symbol (such as a `GType` or error domain) is being searched
 * for in the set of loaded namespaces, the namespaces will be searched in that
 * order. In particular, this means that a symbol which exists in two namespaces
 * will always be returned from the alphabetically-higher namespace. This should
 * only happen in the case of `Gio` and `GioUnix`/`GioWin32`, which all refer to
 * the same `.so` file and expose overlapping sets of symbols. Symbols should
 * always end up being resolved to `GioUnix` or `GioWin32` if they are platform
 * dependent, rather than `Gio` itself.
 *
 * Since: 2.80
 */

/* The namespace and version corresponding to libgirepository itself, so
 * that we can refuse to load typelibs corresponding to the older,
 * incompatible version of this same library in gobject-introspection. */
#define GIREPOSITORY_TYPELIB_NAME "GIRepository"
#define GIREPOSITORY_TYPELIB_VERSION "3.0"
#define GIREPOSITORY_TYPELIB_FILENAME \
  GIREPOSITORY_TYPELIB_NAME "-" GIREPOSITORY_TYPELIB_VERSION ".typelib"

typedef struct {
  size_t n_interfaces;
  GIBaseInfo *interfaces[];
} GTypeInterfaceCache;

static void
gtype_interface_cache_free (gpointer data)
{
  GTypeInterfaceCache *cache = data;

  for (size_t i = 0; i < cache->n_interfaces; i++)
    gi_base_info_unref ((GIBaseInfo*) cache->interfaces[i]);
  g_free (cache);
}

struct _GIRepository
{
  GObject parent;

  GPtrArray *typelib_search_path;  /* (element-type filename) (owned) */
  GPtrArray *library_paths;  /* (element-type filename) (owned) */

  /* Certain operations require iterating over the typelibs and the iteration
   * order may affect the results. So keep an ordered list of the typelibs,
   * alongside the hash table which keep the canonical strong reference to them. */
  GHashTable *typelibs; /* (string) namespace -> GITypelib */
  GPtrArray *ordered_typelibs;  /* (element-type unowned GITypelib) (owned) (not nullable) */
  GHashTable *lazy_typelibs; /* (string) namespace-version -> GITypelib */
  GPtrArray *ordered_lazy_typelibs;  /* (element-type unowned GITypelib) (owned) (not nullable) */

  GHashTable *info_by_gtype; /* GType -> GIBaseInfo */
  GHashTable *info_by_error_domain; /* GQuark -> GIBaseInfo */
  GHashTable *interfaces_for_gtype; /* GType -> GTypeInterfaceCache */
  GHashTable *unknown_gtypes; /* hashset of GType */

  char **cached_shared_libraries;  /* (owned) (nullable) (array zero-terminated=1) */
  size_t cached_n_shared_libraries;  /* length of @cached_shared_libraries, not including NULL terminator */
};

G_DEFINE_TYPE (GIRepository, gi_repository, G_TYPE_OBJECT);

static GITypelib *
require_internal (GIRepository           *repository,
                  const char             *namespace,
                  const char             *version,
                  GIRepositoryLoadFlags   flags,
                  const char * const     *search_paths,
                  size_t                  n_search_paths,
                  GError                **error);

#ifdef G_PLATFORM_WIN32
#include <windows.h>

static HMODULE girepository_dll = NULL;

BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);

BOOL WINAPI
DllMain (HINSTANCE hinstDLL,
         DWORD     fdwReason,
         LPVOID    lpvReserved)
{
  if (fdwReason == DLL_PROCESS_ATTACH)
      girepository_dll = hinstDLL;

  return TRUE;
}

#endif /* G_PLATFORM_WIN32 */

#ifdef __APPLE__
#include <mach-o/dyld.h>

/* This function returns the file path of the loaded libgirepository1.0.dylib.
 * It iterates over all the loaded images to find the one with the
 * gi_repository_init symbol and returns its file path.
  */
static const char *
gi_repository_get_library_path_macos (void)
{
  /*
   * Relevant documentation:
   * https://developer.apple.com/library/archive/documentation/DeveloperTools/Conceptual/MachOTopics/0-Introduction/introduction.html
   * https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man3/dyld.3.html
   * https://opensource.apple.com/source/xnu/xnu-2050.18.24/EXTERNAL_HEADERS/mach-o/loader.h
  */
  const void *ptr = gi_repository_init;
  const struct mach_header *header;
  intptr_t offset;
  uint32_t i, count;

  /* Iterate over all the loaded images */
  count = _dyld_image_count ();
  for (i = 0; i < count; i++)
    {
      header = _dyld_get_image_header (i);
      offset = _dyld_get_image_vmaddr_slide (i);

      /* Locate the first `load` command */
      struct load_command *cmd = (struct load_command *) ((char *) header + sizeof (struct mach_header));
      if (header->magic == MH_MAGIC_64)
        cmd = (struct load_command *) ((char *) header + sizeof (struct mach_header_64));

      /* Find the first `segment` command iterating over all the `load` commands.
       * Then, check if the gi_repository_init symbol is in this image by checking
       * if the pointer is in the segment's memory address range.
       */
      uint32_t j = 0;
      while (j < header->ncmds)
        {
          if (cmd->cmd == LC_SEGMENT)
            {
              struct segment_command *seg = (struct segment_command *) cmd;
              if (((intptr_t) ptr >= ((intptr_t) seg->vmaddr + offset)) && ((intptr_t) ptr < ((intptr_t) seg->vmaddr + offset + (intptr_t) seg->vmsize)))
                return _dyld_get_image_name (i);
           }
          if (cmd->cmd == LC_SEGMENT_64)
            {
              struct segment_command_64 *seg = (struct segment_command_64 *) cmd;
              if (((intptr_t) ptr >= ((intptr_t) seg->vmaddr + offset)) && ((intptr_t) ptr < ((intptr_t) seg->vmaddr + offset + (intptr_t) seg->vmsize)))
                return _dyld_get_image_name (i);
            }
          /* Jump to the next command */
          j++;
          cmd = (struct load_command *) ((char *) cmd + cmd->cmdsize);
        }
    }
  return NULL;
}
#endif /* __APPLE__ */

/*
 * gi_repository_get_libdir:
 *
 * Returns the directory where the typelib files are installed.
 *
 * In platforms without relocation support, this functions returns the
 * `GOBJECT_INTROSPECTION_LIBDIR` directory defined at build time .
 *
 * On Windows and macOS this function returns the directory
 * relative to the installation directory detected at runtime.
 *
 * On macOS, if the library is installed in
 * `/Applications/MyApp.app/Contents/Home/lib/libgirepository-1.0.dylib`, it returns
 * `/Applications/MyApp.app/Contents/Home/lib/girepository-1.0`
 *
 * On Windows, if the application is installed in
 * `C:/Program Files/MyApp/bin/MyApp.exe`, it returns
 * `C:/Program Files/MyApp/lib/girepository-1.0`
*/
static const gchar *
gi_repository_get_libdir (void)
{
  static gchar *static_libdir;

  if (g_once_init_enter_pointer (&static_libdir))
    {
      gchar *libdir;
#if defined(G_PLATFORM_WIN32)
      const char *toplevel = g_win32_get_package_installation_directory_of_module (girepository_dll);
      libdir = g_build_filename (toplevel, GOBJECT_INTROSPECTION_RELATIVE_LIBDIR, NULL);
      g_ignore_leak (libdir);
#elif defined(__APPLE__)
      const char *libpath = gi_repository_get_library_path_macos ();
      if (libpath != NULL)
        {
          libdir = g_path_get_dirname (libpath);
          g_ignore_leak (libdir);
        } else {
          libdir = GOBJECT_INTROSPECTION_LIBDIR;
        }
#else /* !G_PLATFORM_WIN32 && !__APPLE__ */
        libdir = GOBJECT_INTROSPECTION_LIBDIR;
#endif
      g_once_init_leave_pointer (&static_libdir, libdir);
    }
  return static_libdir;
}

static void
gi_repository_init (GIRepository *repository)
{
  /* typelib search path */
    {
      const char *libdir;
      char *typelib_dir;
      const char *type_lib_path_env;

      /* This variable is intended to take precedence over both:
       *   - the default search path;
       *   - all gi_repository_prepend_search_path() calls.
       */
      type_lib_path_env = g_getenv ("GI_TYPELIB_PATH");

      if (type_lib_path_env)
        {
          char **custom_dirs;

          custom_dirs = g_strsplit (type_lib_path_env, G_SEARCHPATH_SEPARATOR_S, 0);
          repository->typelib_search_path =
            g_ptr_array_new_take_null_terminated ((gpointer) g_steal_pointer (&custom_dirs), g_free);
        }
      else
        {
          repository->typelib_search_path = g_ptr_array_new_null_terminated (1, g_free, TRUE);
        }

      libdir = gi_repository_get_libdir ();

      typelib_dir = g_build_filename (libdir, "girepository-1.0", NULL);

      g_ptr_array_add (repository->typelib_search_path, g_steal_pointer (&typelib_dir));
    }

  repository->library_paths = g_ptr_array_new_null_terminated (1, g_free, TRUE);

  repository->typelibs
    = g_hash_table_new_full (g_str_hash, g_str_equal,
                             (GDestroyNotify) g_free,
                             (GDestroyNotify) gi_typelib_unref);
  repository->ordered_typelibs = g_ptr_array_new_with_free_func (NULL);
  repository->lazy_typelibs
    = g_hash_table_new_full (g_str_hash, g_str_equal,
                             (GDestroyNotify) g_free,
                             (GDestroyNotify) gi_typelib_unref);
  repository->ordered_lazy_typelibs = g_ptr_array_new_with_free_func (NULL);

  repository->info_by_gtype
    = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                             (GDestroyNotify) NULL,
                             (GDestroyNotify) gi_base_info_unref);
  repository->info_by_error_domain
    = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                             (GDestroyNotify) NULL,
                             (GDestroyNotify) gi_base_info_unref);
  repository->interfaces_for_gtype
    = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                             (GDestroyNotify) NULL,
                             (GDestroyNotify) gtype_interface_cache_free);
  repository->unknown_gtypes = g_hash_table_new (NULL, NULL);
}

static void
gi_repository_finalize (GObject *object)
{
  GIRepository *repository = GI_REPOSITORY (object);

  g_hash_table_destroy (repository->typelibs);
  g_ptr_array_unref (repository->ordered_typelibs);
  g_hash_table_destroy (repository->lazy_typelibs);
  g_ptr_array_unref (repository->ordered_lazy_typelibs);

  g_hash_table_destroy (repository->info_by_gtype);
  g_hash_table_destroy (repository->info_by_error_domain);
  g_hash_table_destroy (repository->interfaces_for_gtype);
  g_hash_table_destroy (repository->unknown_gtypes);

  g_clear_pointer (&repository->cached_shared_libraries, g_strfreev);

  g_clear_pointer (&repository->library_paths, g_ptr_array_unref);
  g_clear_pointer (&repository->typelib_search_path, g_ptr_array_unref);

  (* G_OBJECT_CLASS (gi_repository_parent_class)->finalize) (G_OBJECT (repository));
}

static void
gi_repository_class_init (GIRepositoryClass *class)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (class);

  gobject_class->finalize = gi_repository_finalize;
}

/**
 * gi_repository_prepend_search_path:
 * @repository: A #GIRepository
 * @directory: (type filename): directory name to prepend to the typelib
 *   search path
 *
 * Prepends @directory to the typelib search path.
 *
 * See also: gi_repository_get_search_path().
 *
 * Since: 2.80
 */
void
gi_repository_prepend_search_path (GIRepository *repository,
                                   const char   *directory)
{
  g_return_if_fail (GI_IS_REPOSITORY (repository));

  g_ptr_array_insert (repository->typelib_search_path, 0, g_strdup (directory));
}

/**
 * gi_repository_get_search_path:
 * @repository: A #GIRepository
 * @n_paths_out: (optional) (out): The number of search paths returned.
 *
 * Returns the current search path [class@GIRepository.Repository] will use when
 * loading typelib files.
 *
 * The list is internal to [class@GIRepository.Repository] and should not be
 * freed, nor should its string elements.
 *
 * The list is guaranteed to be `NULL` terminated. The `NULL` terminator is not
 * counted in @n_paths_out.
 *
 * Returns: (element-type filename) (transfer none) (array length=n_paths_out): list of search paths, most
 *   important first
 * Since: 2.80
 */
const char * const *
gi_repository_get_search_path (GIRepository *repository,
                               size_t       *n_paths_out)
{
  g_return_val_if_fail (GI_IS_REPOSITORY (repository), NULL);

  if G_UNLIKELY (!repository->typelib_search_path ||
                 !repository->typelib_search_path->pdata)
    {
      static const char * const empty_search_path[] = {NULL};

      if (n_paths_out)
        *n_paths_out = 0;

      return empty_search_path;
    }

  if (n_paths_out)
    *n_paths_out = repository->typelib_search_path->len;

  return (const char * const *) repository->typelib_search_path->pdata;
}

/**
 * gi_repository_prepend_library_path:
 * @repository: A #GIRepository
 * @directory: (type filename): a single directory to scan for shared libraries
 *
 * Prepends @directory to the search path that is used to
 * search shared libraries referenced by imported namespaces.
 *
 * Multiple calls to this function all contribute to the final
 * list of paths.
 *
 * The list of paths is unique to @repository. When a typelib is loaded by the
 * repository, the list of paths from the @repository at that instant is used
 * by the typelib for loading its modules.
 *
 * If the library is not found in the directories configured
 * in this way, loading will fall back to the system library
 * path (i.e. `LD_LIBRARY_PATH` and `DT_RPATH` in ELF systems).
 * See the documentation of your dynamic linker for full details.
 *
 * Since: 2.80
 */
void
gi_repository_prepend_library_path (GIRepository *repository,
                                    const char   *directory)
{
  g_return_if_fail (GI_IS_REPOSITORY (repository));

  g_ptr_array_insert (repository->library_paths, 0, g_strdup (directory));
}

/**
 * gi_repository_get_library_path:
 * @repository: A #GIRepository
 * @n_paths_out: (optional) (out): The number of library paths returned.
 *
 * Returns the current search path [class@GIRepository.Repository] will use when
 * loading shared libraries referenced by imported namespaces.
 *
 * The list is internal to [class@GIRepository.Repository] and should not be
 * freed, nor should its string elements.
 *
 * The list is guaranteed to be `NULL` terminated. The `NULL` terminator is not
 * counted in @n_paths_out.
 *
 * Returns: (element-type filename) (transfer none) (array length=n_paths_out): list of search paths, most
 *   important first
 * Since: 2.80
 */
const char * const *
gi_repository_get_library_path (GIRepository *repository,
                                size_t       *n_paths_out)
{
  g_return_val_if_fail (GI_IS_REPOSITORY (repository), NULL);

  if G_UNLIKELY (!repository->library_paths || !repository->library_paths->pdata)
    {
      static const char * const empty_search_path[] = {NULL};

      if (n_paths_out)
        *n_paths_out = 0;

      return empty_search_path;
    }

  if (n_paths_out)
    *n_paths_out = repository->library_paths->len;

  return (const char * const *) repository->library_paths->pdata;
}

static char *
build_typelib_key (const char *name, const char *source)
{
  GString *str = g_string_new (name);
  g_string_append_c (str, '\0');
  g_string_append (str, source);
  return g_string_free (str, FALSE);
}

/* Note: Returns %NULL (not an empty %NULL-terminated array) if there are no
 * dependencies. */
static char **
get_typelib_dependencies (GITypelib *typelib)
{
  Header *header;
  const char *dependencies_glob;

  header = (Header *)typelib->data;

  if (header->dependencies == 0)
    return NULL;

  dependencies_glob = gi_typelib_get_string (typelib, header->dependencies);
  return g_strsplit (dependencies_glob, "|", 0);
}

static GITypelib *
check_version_conflict (GITypelib *typelib,
                        const char  *namespace,
                        const char  *expected_version,
                        char       **version_conflict)
{
  Header *header;
  const char *loaded_version;

  if (expected_version == NULL)
    {
      if (version_conflict)
        *version_conflict = NULL;
      return typelib;
    }

  header = (Header*)typelib->data;
  loaded_version = gi_typelib_get_string (typelib, header->nsversion);
  g_assert (loaded_version != NULL);

  if (strcmp (expected_version, loaded_version) != 0)
    {
      if (version_conflict)
        *version_conflict = (char*)loaded_version;
      return NULL;
    }
  if (version_conflict)
    *version_conflict = NULL;
  return typelib;
}

static GITypelib *
get_registered_status (GIRepository *repository,
                       const char   *namespace,
                       const char   *version,
                       gboolean      allow_lazy,
                       gboolean     *lazy_status,
                       char        **version_conflict)
{
  GITypelib *typelib;

  if (lazy_status)
    *lazy_status = FALSE;
  typelib = g_hash_table_lookup (repository->typelibs, namespace);
  if (typelib)
    return check_version_conflict (typelib, namespace, version, version_conflict);
  typelib = g_hash_table_lookup (repository->lazy_typelibs, namespace);
  if (!typelib)
    return NULL;
  if (lazy_status)
    *lazy_status = TRUE;
  if (!allow_lazy)
    return NULL;
  return check_version_conflict (typelib, namespace, version, version_conflict);
}

static GITypelib *
get_registered (GIRepository *repository,
                const char   *namespace,
                const char   *version)
{
  return get_registered_status (repository, namespace, version, TRUE, NULL, NULL);
}

static gboolean
load_dependencies_recurse (GIRepository *repository,
                           GITypelib     *typelib,
                           GError      **error)
{
  char **dependencies;

  dependencies = get_typelib_dependencies (typelib);

  if (dependencies != NULL)
    {
      int i;

      const char * const *search_path =
        (const char * const *) repository->typelib_search_path->pdata;
      gsize search_path_len = repository->typelib_search_path->len;

      for (i = 0; dependencies[i]; i++)
        {
          char *dependency = dependencies[i];
          const char *last_dash;
          char *dependency_namespace;
          const char *dependency_version;

          last_dash = strrchr (dependency, '-');
          g_assert (last_dash != NULL);  /* get_typelib_dependencies() guarantees this */
          dependency_namespace = g_strndup (dependency, (size_t) (last_dash - dependency));
          dependency_version = last_dash+1;

          if (!require_internal (repository, dependency_namespace, dependency_version,
                                 0, search_path, search_path_len,
                                 error))
            {
              g_free (dependency_namespace);
              g_strfreev (dependencies);
              return FALSE;
            }
          g_free (dependency_namespace);
        }
      g_strfreev (dependencies);
    }
  return TRUE;
}

/* Sort typelibs by namespace. The main requirement here is just to make iteration
 * deterministic, otherwise results can change as a lot of the code here would
 * just iterate over a `GHashTable`.
 *
 * A sub-requirement of this is that namespaces are sorted such that if a GType
 * or symbol is found in multiple namespaces where one is a prefix of the other,
 * the longest namespace wins. In practice, this only happens in
 * Gio/GioUnix/GioWin32, as all three of those namespaces refer to the same
 * `.so` file and overlapping sets of the same symbols, but we want the platform
 * specific namespace to be returned in preference to anything else (even though
 * either namespace is valid).
 * See https://gitlab.gnome.org/GNOME/glib/-/issues/3303 */
static int
sort_typelibs_cb (const void *a,
                  const void *b)
{
  GITypelib *typelib_a = *(GITypelib **) a;
  GITypelib *typelib_b = *(GITypelib **) b;

  return strcmp (gi_typelib_get_namespace (typelib_a),
                 gi_typelib_get_namespace (typelib_b));
}

static const char *
register_internal (GIRepository *repository,
                   const char   *source,
                   gboolean      lazy,
                   GITypelib    *typelib,
                   GError      **error)
{
  Header *header;
  const char *namespace;

  g_return_val_if_fail (typelib != NULL, NULL);

  header = (Header *)typelib->data;

  g_return_val_if_fail (header != NULL, NULL);

  namespace = gi_typelib_get_string (typelib, header->namespace);

  if (lazy)
    {
      g_assert (!g_hash_table_lookup (repository->lazy_typelibs,
                                      namespace));
      g_hash_table_insert (repository->lazy_typelibs,
                           build_typelib_key (namespace, source), gi_typelib_ref (typelib));
      g_ptr_array_add (repository->ordered_lazy_typelibs, typelib);
      g_ptr_array_sort (repository->ordered_lazy_typelibs, sort_typelibs_cb);
    }
  else
    {
      gpointer value;
      char *key;

      /* First, try loading all the dependencies */
      if (!load_dependencies_recurse (repository, typelib, error))
        return NULL;

      /* Check if we are transitioning from lazily loaded state */
      if (g_hash_table_lookup_extended (repository->lazy_typelibs,
                                        namespace,
                                        (gpointer)&key, &value))
        {
          g_hash_table_remove (repository->lazy_typelibs, key);
          g_ptr_array_remove (repository->ordered_lazy_typelibs, typelib);
        }
      else
        {
          key = build_typelib_key (namespace, source);
        }

      g_hash_table_insert (repository->typelibs,
                           g_steal_pointer (&key),
                           gi_typelib_ref (typelib));
      g_ptr_array_add (repository->ordered_typelibs, typelib);
      g_ptr_array_sort (repository->ordered_typelibs, sort_typelibs_cb);
    }

  /* These types might be resolved now, clear the cache */
  g_hash_table_remove_all (repository->unknown_gtypes);

  /* Success */
  g_assert (namespace != NULL);

  return namespace;
}

/**
 * gi_repository_get_immediate_dependencies:
 * @repository: A #GIRepository
 * @namespace_: Namespace of interest
 * @n_dependencies_out: (optional) (out): Return location for the number of
 *   dependencies
 *
 * Return an array of the immediate versioned dependencies for @namespace_.
 * Returned strings are of the form `namespace-version`.
 *
 * Note: @namespace_ must have already been loaded using a function
 * such as [method@GIRepository.Repository.require] before calling this
 * function.
 *
 * To get the transitive closure of dependencies for @namespace_, use
 * [method@GIRepository.Repository.get_dependencies].
 *
 * The list is guaranteed to be `NULL` terminated. The `NULL` terminator is not
 * counted in @n_dependencies_out.
 *
 * Returns: (transfer full) (array length=n_dependencies_out): String array of
 *   immediate versioned dependencies
 * Since: 2.80
 */
char **
gi_repository_get_immediate_dependencies (GIRepository *repository,
                                          const char   *namespace,
                                          size_t       *n_dependencies_out)
{
  GITypelib *typelib;
  char **deps;

  g_return_val_if_fail (GI_IS_REPOSITORY (repository), NULL);
  g_return_val_if_fail (namespace != NULL, NULL);

  typelib = get_registered (repository, namespace, NULL);
  g_return_val_if_fail (typelib != NULL, NULL);

  /* Ensure we always return a non-%NULL vector. */
  deps = get_typelib_dependencies (typelib);
  if (deps == NULL)
      deps = g_strsplit ("", "|", 0);

  if (n_dependencies_out != NULL)
    *n_dependencies_out = g_strv_length (deps);

  return deps;
}

/* Load the transitive closure of dependency namespace-version strings for the
 * given @typelib. @repository must be non-%NULL. @transitive_dependencies must
 * be a pre-existing GHashTable<owned utf8, owned utf8> set for storing the
 * dependencies. */
static void
get_typelib_dependencies_transitive (GIRepository *repository,
                                     GITypelib    *typelib,
                                     GHashTable   *transitive_dependencies)
{
  char **immediate_dependencies;

  immediate_dependencies = get_typelib_dependencies (typelib);

  for (size_t i = 0; immediate_dependencies != NULL && immediate_dependencies[i]; i++)
    {
      char *dependency;
      const char *last_dash;
      char *dependency_namespace;

      dependency = immediate_dependencies[i];

      /* Steal from the strv. */
      g_hash_table_add (transitive_dependencies, dependency);
      immediate_dependencies[i] = NULL;

      /* Recurse for this namespace. */
      last_dash = strrchr (dependency, '-');
      g_assert (last_dash != NULL);  /* get_typelib_dependencies() guarantees this */
      dependency_namespace = g_strndup (dependency, (size_t) (last_dash - dependency));

      typelib = get_registered (repository, dependency_namespace, NULL);
      g_return_if_fail (typelib != NULL);
      get_typelib_dependencies_transitive (repository, typelib,
                                           transitive_dependencies);

      g_free (dependency_namespace);
    }

  g_free (immediate_dependencies);
}

/**
 * gi_repository_get_dependencies:
 * @repository: A #GIRepository
 * @namespace_: Namespace of interest
 * @n_dependencies_out: (optional) (out): Return location for the number of
 *   dependencies
 *
 * Retrieves all (transitive) versioned dependencies for
 * @namespace_.
 *
 * The returned strings are of the form `namespace-version`.
 *
 * Note: @namespace_ must have already been loaded using a function
 * such as [method@GIRepository.Repository.require] before calling this
 * function.
 *
 * To get only the immediate dependencies for @namespace_, use
 * [method@GIRepository.Repository.get_immediate_dependencies].
 *
 * The list is guaranteed to be `NULL` terminated. The `NULL` terminator is not
 * counted in @n_dependencies_out.
 *
 * Returns: (transfer full) (array length=n_dependencies_out): String array of
 *   all versioned dependencies
 * Since: 2.80
 */
char **
gi_repository_get_dependencies (GIRepository *repository,
                                const char   *namespace,
                                size_t       *n_dependencies_out)
{
  GITypelib *typelib;
  GHashTable *transitive_dependencies;  /* set of owned utf8 */
  GHashTableIter iter;
  char *dependency;
  GPtrArray *out;  /* owned utf8 elements */

  g_return_val_if_fail (GI_IS_REPOSITORY (repository), NULL);
  g_return_val_if_fail (namespace != NULL, NULL);

  typelib = get_registered (repository, namespace, NULL);
  g_return_val_if_fail (typelib != NULL, NULL);

  /* Load the dependencies. */
  transitive_dependencies = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                   g_free, NULL);
  get_typelib_dependencies_transitive (repository, typelib,
                                       transitive_dependencies);

  /* Convert to a string array. */
  out = g_ptr_array_new_null_terminated (g_hash_table_size (transitive_dependencies),
                                         g_free, TRUE);
  g_hash_table_iter_init (&iter, transitive_dependencies);

  while (g_hash_table_iter_next (&iter, (gpointer) &dependency, NULL))
    {
      g_ptr_array_add (out, dependency);
      g_hash_table_iter_steal (&iter);
    }

  g_hash_table_unref (transitive_dependencies);

  if (n_dependencies_out != NULL)
    *n_dependencies_out = out->len;

  return (char **) g_ptr_array_free (out, FALSE);
}

/**
 * gi_repository_load_typelib:
 * @repository: A #GIRepository
 * @typelib: (transfer none): the typelib to load
 * @flags: flags affecting the loading operation
 * @error: return location for a [type@GLib.Error], or `NULL`
 *
 * Load the given @typelib into the repository.
 *
 * Returns: namespace of the loaded typelib
 * Since: 2.80
 */
const char *
gi_repository_load_typelib (GIRepository           *repository,
                            GITypelib              *typelib,
                            GIRepositoryLoadFlags   flags,
                            GError                **error)
{
  Header *header;
  const char *namespace;
  const char *nsversion;
  gboolean allow_lazy = flags & GI_REPOSITORY_LOAD_FLAG_LAZY;
  gboolean is_lazy;
  char *version_conflict;

  g_return_val_if_fail (GI_IS_REPOSITORY (repository), NULL);

  header = (Header *) typelib->data;
  namespace = gi_typelib_get_string (typelib, header->namespace);
  nsversion = gi_typelib_get_string (typelib, header->nsversion);

  if (get_registered_status (repository, namespace, nsversion, allow_lazy,
                             &is_lazy, &version_conflict))
    {
      if (version_conflict != NULL)
        {
          g_set_error (error, GI_REPOSITORY_ERROR,
                       GI_REPOSITORY_ERROR_NAMESPACE_VERSION_CONFLICT,
                       "Attempting to load namespace '%s', version '%s', but '%s' is already loaded",
                       namespace, nsversion, version_conflict);
          return NULL;
        }
      return namespace;
    }
  return register_internal (repository, "<builtin>",
                            allow_lazy, typelib, error);
}

/**
 * gi_repository_is_registered:
 * @repository: A #GIRepository
 * @namespace_: Namespace of interest
 * @version: (nullable): Required version, may be `NULL` for latest
 *
 * Check whether a particular namespace (and optionally, a specific
 * version thereof) is currently loaded.
 *
 * This function is likely to only be useful in unusual circumstances; in order
 * to act upon metadata in the namespace, you should call
 * [method@GIRepository.Repository.require] instead which will ensure the
 * namespace is loaded, and return as quickly as this function will if it has
 * already been loaded.
 *
 * Returns: `TRUE` if namespace-version is loaded, `FALSE` otherwise
 * Since: 2.80
 */
gboolean
gi_repository_is_registered (GIRepository *repository,
                             const char   *namespace,
                             const char   *version)
{
  g_return_val_if_fail (GI_IS_REPOSITORY (repository), FALSE);

  return get_registered (repository, namespace, version) != NULL;
}

/**
 * gi_repository_new:
 *
 * Create a new [class@GIRepository.Repository].
 *
 * Returns: (transfer full): a new [class@GIRepository.Repository]
 * Since: 2.80
 */
GIRepository *
gi_repository_new (void)
{
  return g_object_new (GI_TYPE_REPOSITORY, NULL);
}

/**
 * gi_repository_get_n_infos:
 * @repository: A #GIRepository
 * @namespace_: Namespace to inspect
 *
 * This function returns the number of metadata entries in
 * given namespace @namespace_.
 *
 * The namespace must have already been loaded before calling this function.
 *
 * Returns: number of metadata entries
 * Since: 2.80
 */
unsigned int
gi_repository_get_n_infos (GIRepository *repository,
                           const char   *namespace)
{
  GITypelib *typelib;
  unsigned int n_interfaces = 0;

  g_return_val_if_fail (GI_IS_REPOSITORY (repository), 0);
  g_return_val_if_fail (namespace != NULL, 0);

  typelib = get_registered (repository, namespace, NULL);

  g_return_val_if_fail (typelib != NULL, 0);

  n_interfaces = ((Header *)typelib->data)->n_local_entries;

  return n_interfaces;
}

/**
 * gi_repository_get_info:
 * @repository: A #GIRepository
 * @namespace_: Namespace to inspect
 * @idx: 0-based offset into namespace metadata for entry
 *
 * This function returns a particular metadata entry in the
 * given namespace @namespace_.
 *
 * The namespace must have already been loaded before calling this function.
 * See [method@GIRepository.Repository.get_n_infos] to find the maximum number
 * of entries. It is an error to pass an invalid @idx to this function.
 *
 * Returns: (transfer full) (not nullable): [class@GIRepository.BaseInfo]
 *   containing metadata
 * Since: 2.80
 */
GIBaseInfo *
gi_repository_get_info (GIRepository *repository,
                        const char   *namespace,
                        unsigned int  idx)
{
  GITypelib *typelib;
  DirEntry *entry;

  g_return_val_if_fail (GI_IS_REPOSITORY (repository), NULL);
  g_return_val_if_fail (namespace != NULL, NULL);
  g_return_val_if_fail (idx < G_MAXUINT16, NULL);

  typelib = get_registered (repository, namespace, NULL);

  g_return_val_if_fail (typelib != NULL, NULL);

  entry = gi_typelib_get_dir_entry (typelib, idx + 1);
  g_return_val_if_fail (entry != NULL, NULL);

  return gi_info_new_full (gi_typelib_blob_type_to_info_type (entry->blob_type),
                           repository,
                           NULL, typelib, entry->offset);
}

static DirEntry *
find_by_gtype (GPtrArray   *ordered_table,
               const char  *gtype_name,
               gboolean     check_prefix,
               GITypelib  **out_result_typelib)
{
  /* Search in reverse order as the longest namespaces will be listed last, and
   * those are the ones we want to search first. */
  for (guint i = ordered_table->len; i > 0; i--)
    {
      GITypelib *typelib = g_ptr_array_index (ordered_table, i - 1);
      DirEntry *ret;

      if (check_prefix)
        {
          if (!gi_typelib_matches_gtype_name_prefix (typelib, gtype_name))
            continue;
        }

      ret = gi_typelib_get_dir_entry_by_gtype_name (typelib, gtype_name);
      if (ret)
        {
          *out_result_typelib = typelib;
          return ret;
        }
    }

  return NULL;
}

/**
 * gi_repository_find_by_gtype:
 * @repository: A #GIRepository
 * @gtype: [type@GObject.Type] to search for
 *
 * Searches all loaded namespaces for a particular [type@GObject.Type].
 *
 * Note that in order to locate the metadata, the namespace corresponding to
 * the type must first have been loaded.  There is currently no
 * mechanism for determining the namespace which corresponds to an
 * arbitrary [type@GObject.Type] — thus, this function will operate most
 * reliably when you know the [type@GObject.Type] is from a loaded namespace.
 *
 * Returns: (transfer full) (nullable): [class@GIRepository.BaseInfo]
 *   representing metadata about @type, or `NULL` if none found
 * Since: 2.80
 */
GIBaseInfo *
gi_repository_find_by_gtype (GIRepository *repository,
                             GType         gtype)
{
  const char *gtype_name;
  GITypelib *result_typelib = NULL;
  GIBaseInfo *cached;
  DirEntry *entry;

  g_return_val_if_fail (GI_IS_REPOSITORY (repository), NULL);
  g_return_val_if_fail (gtype != G_TYPE_INVALID, NULL);

  cached = g_hash_table_lookup (repository->info_by_gtype,
                                (gpointer)gtype);

  if (cached != NULL)
    return gi_base_info_ref (cached);

  if (g_hash_table_contains (repository->unknown_gtypes, (gpointer)gtype))
    return NULL;

  gtype_name = g_type_name (gtype);

  /* Inside each typelib, we include the "C prefix" which acts as
   * a namespace mechanism.  For GtkTreeView, the C prefix is Gtk.
   * Given the assumption that GTypes for a library also use the
   * C prefix, we know we can skip examining a typelib if our
   * target type does not have this typelib's C prefix. Use this
   * assumption as our first attempt at locating the DirEntry.
   */
  entry = find_by_gtype (repository->ordered_typelibs, gtype_name, TRUE, &result_typelib);
  if (entry == NULL)
    entry = find_by_gtype (repository->ordered_lazy_typelibs, gtype_name, TRUE, &result_typelib);

  /* Not every class library necessarily specifies a correct c_prefix,
   * so take a second pass. This time we will try a global lookup,
   * ignoring prefixes.
   * See http://bugzilla.gnome.org/show_bug.cgi?id=564016
   */
  if (entry == NULL)
    entry = find_by_gtype (repository->ordered_typelibs, gtype_name, FALSE, &result_typelib);
  if (entry == NULL)
    entry = find_by_gtype (repository->ordered_lazy_typelibs, gtype_name, FALSE, &result_typelib);

  if (entry != NULL)
    {
      cached = gi_info_new_full (gi_typelib_blob_type_to_info_type (entry->blob_type),
                                 repository,
                                 NULL, result_typelib, entry->offset);

      g_hash_table_insert (repository->info_by_gtype,
                           (gpointer) gtype,
                           gi_base_info_ref (cached));
      return cached;
    }
  else
    {
      g_hash_table_add (repository->unknown_gtypes, (gpointer) gtype);
      return NULL;
    }
}

/**
 * gi_repository_find_by_name:
 * @repository: A #GIRepository
 * @namespace_: Namespace which will be searched
 * @name: Entry name to find
 *
 * Searches for a particular entry in a namespace.
 *
 * Before calling this function for a particular namespace, you must call
 * [method@GIRepository.Repository.require] to load the namespace, or otherwise
 * ensure the namespace has already been loaded.
 *
 * Returns: (transfer full) (nullable): [class@GIRepository.BaseInfo]
 *   representing metadata about @name, or `NULL` if none found
 * Since: 2.80
 */
GIBaseInfo *
gi_repository_find_by_name (GIRepository *repository,
                            const char   *namespace,
                            const char   *name)
{
  GITypelib *typelib;
  DirEntry *entry;

  g_return_val_if_fail (GI_IS_REPOSITORY (repository), NULL);
  g_return_val_if_fail (namespace != NULL, NULL);

  typelib = get_registered (repository, namespace, NULL);
  g_return_val_if_fail (typelib != NULL, NULL);

  entry = gi_typelib_get_dir_entry_by_name (typelib, name);
  if (entry == NULL)
    return NULL;
  return gi_info_new_full (gi_typelib_blob_type_to_info_type (entry->blob_type),
                           repository,
                           NULL, typelib, entry->offset);
}

static DirEntry *
find_by_error_domain (GPtrArray  *ordered_typelibs,
                      GQuark      target_domain,
                      GITypelib **out_typelib)
{
  /* Search in reverse order as the longest namespaces will be listed last, and
   * those are the ones we want to search first. */
  for (guint i = ordered_typelibs->len; i > 0; i--)
    {
      GITypelib *typelib = g_ptr_array_index (ordered_typelibs, i - 1);
      DirEntry *entry;

      entry = gi_typelib_get_dir_entry_by_error_domain (typelib, target_domain);
      if (entry != NULL)
        {
          *out_typelib = typelib;
          return entry;
        }
    }

  return NULL;
}

/**
 * gi_repository_find_by_error_domain:
 * @repository: A #GIRepository
 * @domain: a [type@GLib.Error] domain
 *
 * Searches for the enum type corresponding to the given [type@GLib.Error]
 * domain.
 *
 * Before calling this function for a particular namespace, you must call
 * [method@GIRepository.Repository.require] to load the namespace, or otherwise
 * ensure the namespace has already been loaded.
 *
 * Returns: (transfer full) (nullable): [class@GIRepository.EnumInfo]
 *   representing metadata about @domain’s enum type, or `NULL` if none found
 * Since: 2.80
 */
GIEnumInfo *
gi_repository_find_by_error_domain (GIRepository *repository,
                                    GQuark        domain)
{
  GIEnumInfo *cached;
  DirEntry *result = NULL;
  GITypelib *result_typelib = NULL;

  g_return_val_if_fail (GI_IS_REPOSITORY (repository), NULL);

  cached = g_hash_table_lookup (repository->info_by_error_domain,
                                GUINT_TO_POINTER (domain));

  if (cached != NULL)
    return (GIEnumInfo *) gi_base_info_ref ((GIBaseInfo *)cached);

  result = find_by_error_domain (repository->ordered_typelibs, domain, &result_typelib);
  if (result == NULL)
    result = find_by_error_domain (repository->ordered_lazy_typelibs, domain, &result_typelib);

  if (result != NULL)
    {
      cached = (GIEnumInfo *) gi_info_new_full (gi_typelib_blob_type_to_info_type (result->blob_type),
                                                repository,
                                                NULL, result_typelib, result->offset);

      g_hash_table_insert (repository->info_by_error_domain,
                           GUINT_TO_POINTER (domain),
                           gi_base_info_ref ((GIBaseInfo *) cached));
      return cached;
    }
  return NULL;
}

/**
 * gi_repository_get_object_gtype_interfaces:
 * @repository: a #GIRepository
 * @gtype: a [type@GObject.Type] whose fundamental type is `G_TYPE_OBJECT`
 * @n_interfaces_out: (out): Number of interfaces
 * @interfaces_out: (out) (transfer none) (array length=n_interfaces_out): Interfaces for @gtype
 *
 * Look up the implemented interfaces for @gtype.
 *
 * This function cannot fail per se; but for a totally ‘unknown’
 * [type@GObject.Type], it may return 0 implemented interfaces.
 *
 * The semantics of this function are designed for a dynamic binding,
 * where in certain cases (such as a function which returns an
 * interface which may have ‘hidden’ implementation classes), not all
 * data may be statically known, and will have to be determined from
 * the [type@GObject.Type] of the object.  An example is
 * [func@Gio.File.new_for_path] returning a concrete class of
 * `GLocalFile`, which is a [type@GObject.Type] we see at runtime, but
 * not statically.
 *
 * Since: 2.80
 */
void
gi_repository_get_object_gtype_interfaces (GIRepository      *repository,
                                           GType              gtype,
                                           size_t            *n_interfaces_out,
                                           GIInterfaceInfo ***interfaces_out)
{
  GTypeInterfaceCache *cache;

  g_return_if_fail (GI_IS_REPOSITORY (repository));
  g_return_if_fail (g_type_fundamental (gtype) == G_TYPE_OBJECT);

  cache = g_hash_table_lookup (repository->interfaces_for_gtype,
                               (void *) gtype);
  if (cache == NULL)
    {
      GType *interfaces;
      unsigned int i;
      unsigned int n_interfaces;
      GList *interface_infos = NULL, *iter;

      interfaces = g_type_interfaces (gtype, &n_interfaces);
      for (i = 0; i < n_interfaces; i++)
        {
          GIBaseInfo *base_info;

          base_info = gi_repository_find_by_gtype (repository, interfaces[i]);
          if (base_info == NULL)
            continue;

          if (gi_base_info_get_info_type (base_info) != GI_INFO_TYPE_INTERFACE)
            {
              /* FIXME - could this really happen? */
              gi_base_info_unref (base_info);
              continue;
            }

          if (!g_list_find (interface_infos, base_info))
            interface_infos = g_list_prepend (interface_infos, base_info);
        }

      cache = g_malloc (sizeof (GTypeInterfaceCache)
                        + sizeof (GIBaseInfo*) * g_list_length (interface_infos));
      cache->n_interfaces = g_list_length (interface_infos);
      for (iter = interface_infos, i = 0; iter; iter = iter->next, i++)
        cache->interfaces[i] = iter->data;
      g_list_free (interface_infos);

      g_hash_table_insert (repository->interfaces_for_gtype, (gpointer) gtype,
                           cache);

      g_free (interfaces);
    }

  *n_interfaces_out = cache->n_interfaces;
  *interfaces_out = (GIInterfaceInfo**)&cache->interfaces[0];
}

static void
collect_namespaces (GPtrArray  *ordered_typelibs,
                    char      **names,
                    size_t     *inout_i)
{
  for (guint j = 0; j < ordered_typelibs->len; j++)
    {
      GITypelib *typelib = g_ptr_array_index (ordered_typelibs, j);
      const char *namespace = gi_typelib_get_namespace (typelib);
      names[(*inout_i)++] = g_strdup (namespace);
    }
}

/**
 * gi_repository_get_loaded_namespaces:
 * @repository: A #GIRepository
 * @n_namespaces_out: (optional) (out): Return location for the number of
 *   namespaces
 *
 * Return the list of currently loaded namespaces.
 *
 * The list is guaranteed to be `NULL` terminated. The `NULL` terminator is not
 * counted in @n_namespaces_out.
 *
 * Returns: (element-type utf8) (transfer full) (array length=n_namespaces_out):
 *   list of namespaces
 * Since: 2.80
 */
char **
gi_repository_get_loaded_namespaces (GIRepository *repository,
                                     size_t       *n_namespaces_out)
{
  char **names;
  size_t i;
  size_t n_typelibs;

  g_return_val_if_fail (GI_IS_REPOSITORY (repository), NULL);

  n_typelibs = repository->ordered_typelibs->len + repository->ordered_lazy_typelibs->len;
  names = g_malloc0 (sizeof (char *) * (n_typelibs + 1));
  i = 0;

  collect_namespaces (repository->ordered_typelibs, names, &i);
  collect_namespaces (repository->ordered_lazy_typelibs, names, &i);

  if (n_namespaces_out != NULL)
    *n_namespaces_out = i;

  return names;
}

/**
 * gi_repository_get_version:
 * @repository: A #GIRepository
 * @namespace_: Namespace to inspect
 *
 * This function returns the loaded version associated with the given
 * namespace @namespace_.
 *
 * Note: The namespace must have already been loaded using a function
 * such as [method@GIRepository.Repository.require] before calling this
 * function.
 *
 * Returns: Loaded version
 * Since: 2.80
 */
const char *
gi_repository_get_version (GIRepository *repository,
                           const char   *namespace)
{
  GITypelib *typelib;
  Header *header;

  g_return_val_if_fail (GI_IS_REPOSITORY (repository), NULL);
  g_return_val_if_fail (namespace != NULL, NULL);

  typelib = get_registered (repository, namespace, NULL);

  g_return_val_if_fail (typelib != NULL, NULL);

  header = (Header *) typelib->data;
  return gi_typelib_get_string (typelib, header->nsversion);
}

/**
 * gi_repository_get_shared_libraries:
 * @repository: A #GIRepository
 * @namespace_: Namespace to inspect
 * @out_n_elements: (out) (optional): Return location for the number of elements
 *   in the returned array
 *
 * This function returns an array of paths to the
 * shared C libraries associated with the given namespace @namespace_.
 *
 * There may be no shared library path associated, in which case this
 * function will return `NULL`.
 *
 * Note: The namespace must have already been loaded using a function
 * such as [method@GIRepository.Repository.require] before calling this
 * function.
 *
 * The list is internal to [class@GIRepository.Repository] and should not be
 * freed, nor should its string elements.
 *
 * The list is guaranteed to be `NULL` terminated. The `NULL` terminator is not
 * counted in @out_n_elements.
 *
 * Returns: (nullable) (array length=out_n_elements) (transfer none): Array of
 *   paths to shared libraries, or `NULL` if none are associated
 * Since: 2.80
 */
const char * const *
gi_repository_get_shared_libraries (GIRepository *repository,
                                    const char   *namespace,
                                    size_t       *out_n_elements)
{
  GITypelib *typelib;
  Header *header;

  g_return_val_if_fail (GI_IS_REPOSITORY (repository), NULL);
  g_return_val_if_fail (namespace != NULL, NULL);

  typelib = get_registered (repository, namespace, NULL);

  g_return_val_if_fail (typelib != NULL, NULL);

  header = (Header *) typelib->data;
  if (!header->shared_library)
    {
      if (out_n_elements != NULL)
        *out_n_elements = 0;
      return NULL;
    }

  /* Populate the cache. */
  if (repository->cached_shared_libraries == NULL)
    {
      const char *comma_separated = gi_typelib_get_string (typelib, header->shared_library);

      if (comma_separated != NULL && *comma_separated != '\0')
        {
          repository->cached_shared_libraries = g_strsplit (comma_separated, ",", -1);
          repository->cached_n_shared_libraries = g_strv_length (repository->cached_shared_libraries);
        }
    }

  if (out_n_elements != NULL)
    *out_n_elements = repository->cached_n_shared_libraries;

  return (const char * const *) repository->cached_shared_libraries;
}

/**
 * gi_repository_get_c_prefix:
 * @repository: A #GIRepository
 * @namespace_: Namespace to inspect
 *
 * This function returns the ‘C prefix’, or the C level namespace
 * associated with the given introspection namespace.
 *
 * Each C symbol starts with this prefix, as well each [type@GObject.Type] in
 * the library.
 *
 * Note: The namespace must have already been loaded using a function
 * such as [method@GIRepository.Repository.require] before calling this
 * function.
 *
 * Returns: (nullable): C namespace prefix, or `NULL` if none associated
 * Since: 2.80
 */
const char *
gi_repository_get_c_prefix (GIRepository *repository,
                            const char   *namespace_)
{
  GITypelib *typelib;
  Header *header;

  g_return_val_if_fail (GI_IS_REPOSITORY (repository), NULL);
  g_return_val_if_fail (namespace_ != NULL, NULL);

  typelib = get_registered (repository, namespace_, NULL);

  g_return_val_if_fail (typelib != NULL, NULL);

  header = (Header *) typelib->data;
  if (header->c_prefix)
    return gi_typelib_get_string (typelib, header->c_prefix);
  else
    return NULL;
}

/**
 * gi_repository_get_typelib_path:
 * @repository: A #GIRepository
 * @namespace_: GI namespace to use, e.g. `Gtk`
 *
 * If namespace @namespace_ is loaded, return the full path to the
 * .typelib file it was loaded from.
 *
 * If the typelib for namespace @namespace_ was included in a shared library,
 * return the special string `<builtin>`.
 *
 * Returns: (type filename) (nullable): Filesystem path (or `<builtin>`) if
 *   successful, `NULL` if namespace is not loaded
 * Since: 2.80
 */
const char *
gi_repository_get_typelib_path (GIRepository *repository,
                                const char   *namespace)
{
  gpointer orig_key, value;

  g_return_val_if_fail (GI_IS_REPOSITORY (repository), NULL);

  if (!g_hash_table_lookup_extended (repository->typelibs, namespace,
                                     &orig_key, &value))
    {
      if (!g_hash_table_lookup_extended (repository->lazy_typelibs, namespace,
                                         &orig_key, &value))

        return NULL;
    }
  return ((char*)orig_key) + strlen ((char *) orig_key) + 1;
}

/* This simple search function looks for a specified namespace-version;
   it's faster than the full directory listing required for latest version. */
static GMappedFile *
find_namespace_version (const char          *namespace,
                        const char          *version,
                        const char * const  *search_paths,
                        size_t               n_search_paths,
                        char               **path_ret)
{
  GError *error = NULL;
  GMappedFile *mfile = NULL;
  char *fname;

  if (g_str_equal (namespace, GIREPOSITORY_TYPELIB_NAME) &&
      !g_str_equal (version, GIREPOSITORY_TYPELIB_VERSION))
    {
      g_debug ("Ignoring %s-%s.typelib because this libgirepository "
               "corresponds to %s-%s",
               namespace, version,
               namespace, GIREPOSITORY_TYPELIB_VERSION);
      return NULL;
    }

  fname = g_strdup_printf ("%s-%s.typelib", namespace, version);

  for (size_t i = 0; i < n_search_paths; ++i)
    {
      char *path = g_build_filename (search_paths[i], fname, NULL);

      mfile = g_mapped_file_new (path, FALSE, &error);
      if (error)
        {
          g_free (path);
          g_clear_error (&error);
          continue;
        }
      *path_ret = path;
      break;
    }
  g_free (fname);
  return mfile;
}

static gboolean
parse_version (const char *version,
               int *major,
               int *minor)
{
  const char *dot;
  char *end;

  *major = strtol (version, &end, 10);
  dot = strchr (version, '.');
  if (dot == NULL)
    {
      *minor = 0;
      return TRUE;
    }
  if (dot != end)
    return FALSE;
  *minor = strtol (dot+1, &end, 10);
  if (end != (version + strlen (version)))
    return FALSE;
  return TRUE;
}

static int
compare_version (const char *v1,
                 const char *v2)
{
  gboolean success;
  int v1_major, v1_minor;
  int v2_major, v2_minor;

  success = parse_version (v1, &v1_major, &v1_minor);
  g_assert (success);

  success = parse_version (v2, &v2_major, &v2_minor);
  g_assert (success);

  /* Avoid a compiler warning about `success` being unused with G_DISABLE_ASSERT */
  (void) success;

  if (v1_major > v2_major)
    return 1;
  else if (v2_major > v1_major)
    return -1;
  else if (v1_minor > v2_minor)
    return 1;
  else if (v2_minor > v1_minor)
    return -1;
  return 0;
}

struct NamespaceVersionCandidadate
{
  GMappedFile *mfile;
  int path_index;
  char *path;
  char *version;
};

static int
compare_candidate_reverse (struct NamespaceVersionCandidadate *c1,
                           struct NamespaceVersionCandidadate *c2)
{
  int result = compare_version (c1->version, c2->version);
  /* First, check the version */
  if (result > 0)
    return -1;
  else if (result < 0)
    return 1;
  else
    {
      /* Now check the path index, which says how early in the search path
       * we found it.  This ensures that of equal version targets, we
       * pick the earlier one.
       */
      if (c1->path_index == c2->path_index)
        return 0;
      else if (c1->path_index > c2->path_index)
        return 1;
      else
        return -1;
    }
}

static void
free_candidate (struct NamespaceVersionCandidadate *candidate)
{
  g_mapped_file_unref (candidate->mfile);
  g_free (candidate->path);
  g_free (candidate->version);
  g_slice_free (struct NamespaceVersionCandidadate, candidate);
}

static GSList *
enumerate_namespace_versions (const char         *namespace,
                              const char * const *search_paths,
                              size_t              n_search_paths)
{
  GSList *candidates = NULL;
  GHashTable *found_versions = g_hash_table_new (g_str_hash, g_str_equal);
  char *namespace_dash;
  char *namespace_typelib;
  GError *error = NULL;
  int index;

  namespace_dash = g_strdup_printf ("%s-", namespace);
  namespace_typelib = g_strdup_printf ("%s.typelib", namespace);

  index = 0;
  for (size_t i = 0; i < n_search_paths; ++i)
    {
      GDir *dir;
      const char *dirname;
      const char *entry;

      dirname = search_paths[i];
      dir = g_dir_open (dirname, 0, NULL);
      if (dir == NULL)
        continue;
      while ((entry = g_dir_read_name (dir)) != NULL)
        {
          GMappedFile *mfile;
          char *path, *version;
          struct NamespaceVersionCandidadate *candidate;

          if (!g_str_has_suffix (entry, ".typelib"))
            continue;

          if (g_str_has_prefix (entry, namespace_dash))
            {
              const char *last_dash;
              const char *name_end;
              int major, minor;

              if (g_str_equal (namespace, GIREPOSITORY_TYPELIB_NAME) &&
                  !g_str_equal (entry, GIREPOSITORY_TYPELIB_FILENAME))
                {
                  g_debug ("Ignoring %s because this libgirepository "
                           "corresponds to %s",
                           entry, GIREPOSITORY_TYPELIB_FILENAME);
                  continue;
                }

              name_end = strrchr (entry, '.');
              last_dash = strrchr (entry, '-');

              /* These are guaranteed by the suffix and prefix checks above: */
              g_assert (name_end != NULL);
              g_assert (last_dash != NULL);

              version = g_strndup (last_dash + 1, (size_t) (name_end - (last_dash + 1u)));
              if (!parse_version (version, &major, &minor))
                {
                  g_free (version);
                  continue;
                }
            }
          else
            continue;

          if (g_hash_table_lookup (found_versions, version) != NULL)
            {
              g_free (version);
              continue;
            }

          path = g_build_filename (dirname, entry, NULL);
          mfile = g_mapped_file_new (path, FALSE, &error);
          if (mfile == NULL)
            {
              g_free (path);
              g_free (version);
              g_clear_error (&error);
              continue;
            }
          candidate = g_slice_new0 (struct NamespaceVersionCandidadate);
          candidate->mfile = mfile;
          candidate->path_index = index;
          candidate->path = path;
          candidate->version = version;
          candidates = g_slist_prepend (candidates, candidate);
          g_hash_table_add (found_versions, version);
        }
      g_dir_close (dir);
      index++;
    }

  g_free (namespace_dash);
  g_free (namespace_typelib);
  g_hash_table_destroy (found_versions);

  return candidates;
}

static GMappedFile *
find_namespace_latest (const char          *namespace,
                       const char * const  *search_paths,
                       size_t               n_search_paths,
                       char               **version_ret,
                       char               **path_ret)
{
  GSList *candidates;
  GMappedFile *result = NULL;

  *version_ret = NULL;
  *path_ret = NULL;

  candidates = enumerate_namespace_versions (namespace, search_paths, n_search_paths);

  if (candidates != NULL)
    {
      struct NamespaceVersionCandidadate *elected;
      candidates = g_slist_sort (candidates, (GCompareFunc) compare_candidate_reverse);

      elected = (struct NamespaceVersionCandidadate *) candidates->data;
      /* Remove the elected one so we don't try to free its contents */
      candidates = g_slist_delete_link (candidates, candidates);

      result = elected->mfile;
      *path_ret = elected->path;
      *version_ret = elected->version;
      g_slice_free (struct NamespaceVersionCandidadate, elected); /* just free the container */
      g_slist_foreach (candidates, (GFunc) (void *) free_candidate, NULL);
      g_slist_free (candidates);
    }
  return result;
}

/**
 * gi_repository_enumerate_versions:
 * @repository: A #GIRepository
 * @namespace_: GI namespace, e.g. `Gtk`
 * @n_versions_out: (optional) (out): The number of versions returned.
 *
 * Obtain an unordered list of versions (either currently loaded or
 * available) for @namespace_ in this @repository.
 *
 * The list is guaranteed to be `NULL` terminated. The `NULL` terminator is not
 * counted in @n_versions_out.
 *
 * Returns: (element-type utf8) (transfer full) (array length=n_versions_out): the array of versions.
 * Since: 2.80
 */
char **
gi_repository_enumerate_versions (GIRepository *repository,
                                  const char   *namespace_,
                                  size_t       *n_versions_out)
{
  GPtrArray *versions;
  GSList *candidates, *link;
  const char *loaded_version;
  char **ret;

  g_return_val_if_fail (GI_IS_REPOSITORY (repository), NULL);

  candidates = enumerate_namespace_versions (namespace_,
                                             (const char * const *) repository->typelib_search_path->pdata,
                                             repository->typelib_search_path->len);

  if (!candidates)
    {
      if (n_versions_out)
        *n_versions_out = 0;
      return g_strdupv ((char *[]) {NULL});
    }

  versions = g_ptr_array_new_null_terminated (1, g_free, TRUE);
  for (link = candidates; link; link = link->next)
    {
      struct NamespaceVersionCandidadate *candidate = link->data;
      g_ptr_array_add (versions, g_steal_pointer (&candidate->version));
      free_candidate (candidate);
    }
  g_slist_free (candidates);

  /* The currently loaded version of a namespace is also part of the
   * available versions, as it could have been loaded using
   * require_private().
   */
  if (gi_repository_is_registered (repository, namespace_, NULL))
    {
      loaded_version = gi_repository_get_version (repository, namespace_);
      if (loaded_version &&
          !g_ptr_array_find_with_equal_func (versions, loaded_version, g_str_equal, NULL))
        g_ptr_array_add (versions, g_strdup (loaded_version));
    }

  ret = (char **) g_ptr_array_steal (versions, n_versions_out);
  g_ptr_array_unref (g_steal_pointer (&versions));

  return ret;
}

static GITypelib *
require_internal (GIRepository           *repository,
                  const char             *namespace,
                  const char             *version,
                  GIRepositoryLoadFlags   flags,
                  const char * const     *search_paths,
                  size_t                  n_search_paths,
                  GError                **error)
{
  GMappedFile *mfile;
  GITypelib *ret = NULL;
  Header *header;
  GITypelib *typelib = NULL;
  GITypelib *typelib_owned = NULL;
  const char *typelib_namespace, *typelib_version;
  gboolean allow_lazy = (flags & GI_REPOSITORY_LOAD_FLAG_LAZY) > 0;
  gboolean is_lazy;
  char *version_conflict = NULL;
  char *path = NULL;
  char *tmp_version = NULL;

  g_return_val_if_fail (GI_IS_REPOSITORY (repository), NULL);
  g_return_val_if_fail (namespace != NULL, NULL);

  typelib = get_registered_status (repository, namespace, version, allow_lazy,
                                   &is_lazy, &version_conflict);
  if (typelib)
    return typelib;

  if (version_conflict != NULL)
    {
      g_set_error (error, GI_REPOSITORY_ERROR,
                   GI_REPOSITORY_ERROR_NAMESPACE_VERSION_CONFLICT,
                   "Requiring namespace '%s' version '%s', but '%s' is already loaded",
                   namespace, version, version_conflict);
      return NULL;
    }

  if (version != NULL)
    {
      mfile = find_namespace_version (namespace, version, search_paths,
                                      n_search_paths, &path);
      tmp_version = g_strdup (version);
    }
  else
    {
      mfile = find_namespace_latest (namespace, search_paths, n_search_paths,
                                     &tmp_version, &path);
    }

  if (mfile == NULL)
    {
      if (version != NULL)
        g_set_error (error, GI_REPOSITORY_ERROR,
                     GI_REPOSITORY_ERROR_TYPELIB_NOT_FOUND,
                     "Typelib file for namespace '%s', version '%s' not found",
                     namespace, version);
      else
        g_set_error (error, GI_REPOSITORY_ERROR,
                     GI_REPOSITORY_ERROR_TYPELIB_NOT_FOUND,
                     "Typelib file for namespace '%s' (any version) not found",
                     namespace);
      goto out;
    }

  {
    GError *temp_error = NULL;
    GBytes *bytes = NULL;

    bytes = g_mapped_file_get_bytes (mfile);
    typelib_owned = typelib = gi_typelib_new_from_bytes (bytes, &temp_error);
    g_bytes_unref (bytes);
    g_clear_pointer (&mfile, g_mapped_file_unref);

    if (!typelib)
      {
        g_set_error (error, GI_REPOSITORY_ERROR,
                     GI_REPOSITORY_ERROR_TYPELIB_NOT_FOUND,
                     "Failed to load typelib file '%s' for namespace '%s': %s",
                     path, namespace, temp_error->message);
        g_clear_error (&temp_error);
        goto out;
      }

    typelib->library_paths = (repository->library_paths != NULL) ? g_ptr_array_ref (repository->library_paths) : NULL;
  }
  header = (Header *) typelib->data;
  typelib_namespace = gi_typelib_get_string (typelib, header->namespace);
  typelib_version = gi_typelib_get_string (typelib, header->nsversion);

  if (strcmp (typelib_namespace, namespace) != 0)
    {
      g_set_error (error, GI_REPOSITORY_ERROR,
                   GI_REPOSITORY_ERROR_NAMESPACE_MISMATCH,
                   "Typelib file %s for namespace '%s' contains "
                   "namespace '%s' which doesn't match the file name",
                   path, namespace, typelib_namespace);
      goto out;
    }
  if (version != NULL && strcmp (typelib_version, version) != 0)
    {
      g_set_error (error, GI_REPOSITORY_ERROR,
                   GI_REPOSITORY_ERROR_NAMESPACE_MISMATCH,
                   "Typelib file %s for namespace '%s' contains "
                   "version '%s' which doesn't match the expected version '%s'",
                   path, namespace, typelib_version, version);
      goto out;
    }

  if (!register_internal (repository, path, allow_lazy,
                          typelib, error))
    goto out;
  ret = typelib;
 out:
  g_clear_pointer (&typelib_owned, gi_typelib_unref);
  g_free (tmp_version);
  g_free (path);
  return ret;
}

static GITypelib *
require_internal_with_platform_data (GIRepository           *repository,
                                     const char             *namespace,
                                     const char             *version,
                                     GIRepositoryLoadFlags   flags,
                                     const char * const     *search_paths,
                                     size_t                  search_paths_len,
                                     GError                **error)
{
  GITypelib *typelib;

  typelib = require_internal (repository, namespace, version, flags,
                              search_paths, search_paths_len,
                              error);
  if (!typelib)
    return NULL;

#if defined (G_OS_UNIX) || defined (G_OS_WIN32)
  /* Backward compatibility hack: if we're loading GLib/Gio-2.0, we automatically
   * load the platform specific introspection data that used to exist inside
   * GLib/Gio-2.0
   */
  if ((g_str_equal (namespace, "GLib") || g_str_equal (namespace, "Gio")) &&
      (!version || g_str_equal (version, "2.0")))
    {
      GError *local_error = NULL;
      char *platform_namespace = NULL;

#  if defined (G_OS_UNIX)
      platform_namespace = g_strconcat (namespace, "Unix", NULL);
#  elif defined (G_OS_WIN32)
      platform_namespace = g_strconcat (namespace, "Win32", NULL);
#  endif /* defined (G_OS_LINUX) */

      if (!require_internal (repository, platform_namespace, version, flags,
                             search_paths, search_paths_len,
                             &local_error))
        {
          g_critical ("Unable to load platform-specific GIO introspection data: %s",
                      local_error->message);
          g_error_free (local_error);
        }

      g_clear_pointer (&platform_namespace, g_free);
    }
#endif /* defined(G_OS_UNIX) || defined(G_OS_WIN32) */

  return typelib;
}

/**
 * gi_repository_require:
 * @repository: A #GIRepository
 * @namespace_: GI namespace to use, e.g. `Gtk`
 * @version: (nullable): Version of namespace, may be `NULL` for latest
 * @flags: Set of [flags@GIRepository.RepositoryLoadFlags], may be 0
 * @error: a [type@GLib.Error].
 *
 * Force the namespace @namespace_ to be loaded if it isn’t already.
 *
 * If @namespace_ is not loaded, this function will search for a
 * `.typelib` file using the repository search path.  In addition, a
 * version @version of namespace may be specified.  If @version is
 * not specified, the latest will be used.
 *
 * Returns: (transfer none): a pointer to the [type@GIRepository.Typelib] if
 *   successful, `NULL` otherwise
 * Since: 2.80
 */
GITypelib *
gi_repository_require (GIRepository           *repository,
                       const char             *namespace,
                       const char             *version,
                       GIRepositoryLoadFlags   flags,
                       GError                **error)
{
  const char * const *search_paths;
  size_t search_paths_len;

  g_return_val_if_fail (GI_IS_REPOSITORY (repository), NULL);
  g_return_val_if_fail (namespace != NULL, NULL);

  search_paths = (const char * const *) repository->typelib_search_path->pdata;
  search_paths_len = repository->typelib_search_path->len;

  return require_internal_with_platform_data (repository, namespace, version, flags,
                                              search_paths, search_paths_len,
                                              error);
}

/**
 * gi_repository_require_private:
 * @repository: A #GIRepository
 * @typelib_dir: (type filename): Private directory where to find the requested
 *   typelib
 * @namespace_: GI namespace to use, e.g. `Gtk`
 * @version: (nullable): Version of namespace, may be `NULL` for latest
 * @flags: Set of [flags@GIRepository.RepositoryLoadFlags], may be 0
 * @error: a [type@GLib.Error].
 *
 * Force the namespace @namespace_ to be loaded if it isn’t already.
 *
 * If @namespace_ is not loaded, this function will search for a
 * `.typelib` file within the private directory only. In addition, a
 * version @version of namespace should be specified.  If @version is
 * not specified, the latest will be used.
 *
 * Returns: (transfer none): a pointer to the [type@GIRepository.Typelib] if
 *   successful, `NULL` otherwise
 * Since: 2.80
 */
GITypelib *
gi_repository_require_private (GIRepository           *repository,
                               const char             *typelib_dir,
                               const char             *namespace,
                               const char             *version,
                               GIRepositoryLoadFlags   flags,
                               GError                **error)
{
  const char * const search_path[] = { typelib_dir, NULL };

  g_return_val_if_fail (GI_IS_REPOSITORY (repository), NULL);
  g_return_val_if_fail (namespace != NULL, NULL);

  return require_internal_with_platform_data (repository, namespace, version, flags,
                                              search_path, 1,
                                              error);
}

static gboolean
gi_repository_introspect_cb (const char *option_name,
                             const char *value,
                             gpointer data,
                             GError **error)
{
  GError *tmp_error = NULL;
  char **args;

  args = g_strsplit (value, ",", 2);

  if (!gi_repository_dump (args[0], args[1], &tmp_error))
    {
      g_error ("Failed to extract GType data: %s",
               tmp_error->message);
      exit (1);
    }
  exit (0);
}

static const GOptionEntry introspection_args[] = {
  { "introspect-dump", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_CALLBACK,
    gi_repository_introspect_cb, "Dump introspection information",
    "infile.txt,outfile.xml" },
  G_OPTION_ENTRY_NULL
};

/**
 * gi_repository_get_option_group:
 *
 * Obtain the option group for girepository.
 *
 * It’s used by the dumper and for programs that want to provide introspection
 * information
 *
 * Returns: (transfer full): the option group
 * Since: 2.80
 */
GOptionGroup *
gi_repository_get_option_group (void)
{
  GOptionGroup *group;
  group = g_option_group_new ("girepository", "Introspection Options", "Show Introspection Options", NULL, NULL);

  g_option_group_add_entries (group, introspection_args);
  return group;
}

GQuark
gi_repository_error_quark (void)
{
  static GQuark quark = 0;
  if (quark == 0)
    quark = g_quark_from_static_string ("g-irepository-error-quark");
  return quark;
}

/**
 * gi_type_tag_to_string:
 * @type: the type_tag
 *
 * Obtain a string representation of @type
 *
 * Returns: the string
 * Since: 2.80
 */
const char *
gi_type_tag_to_string (GITypeTag type)
{
  switch (type)
    {
    case GI_TYPE_TAG_VOID:
      return "void";
    case GI_TYPE_TAG_BOOLEAN:
      return "gboolean";
    case GI_TYPE_TAG_INT8:
      return "gint8";
    case GI_TYPE_TAG_UINT8:
      return "guint8";
    case GI_TYPE_TAG_INT16:
      return "gint16";
    case GI_TYPE_TAG_UINT16:
      return "guint16";
    case GI_TYPE_TAG_INT32:
      return "gint32";
    case GI_TYPE_TAG_UINT32:
      return "guint32";
    case GI_TYPE_TAG_INT64:
      return "gint64";
    case GI_TYPE_TAG_UINT64:
      return "guint64";
    case GI_TYPE_TAG_FLOAT:
      return "gfloat";
    case GI_TYPE_TAG_DOUBLE:
      return "gdouble";
    case GI_TYPE_TAG_UNICHAR:
      return "gunichar";
    case GI_TYPE_TAG_GTYPE:
      return "GType";
    case GI_TYPE_TAG_UTF8:
      return "utf8";
    case GI_TYPE_TAG_FILENAME:
      return "filename";
    case GI_TYPE_TAG_ARRAY:
      return "array";
    case GI_TYPE_TAG_INTERFACE:
      return "interface";
    case GI_TYPE_TAG_GLIST:
      return "glist";
    case GI_TYPE_TAG_GSLIST:
      return "gslist";
    case GI_TYPE_TAG_GHASH:
      return "ghash";
    case GI_TYPE_TAG_ERROR:
      return "error";
    default:
      return "unknown";
    }
}

/**
 * gi_info_type_to_string:
 * @type: the info type
 *
 * Obtain a string representation of @type
 *
 * Returns: the string
 * Since: 2.80
 */
const char *
gi_info_type_to_string (GIInfoType type)
{
  switch (type)
    {
    case GI_INFO_TYPE_INVALID:
      return "invalid";
    case GI_INFO_TYPE_FUNCTION:
      return "function";
    case GI_INFO_TYPE_CALLBACK:
      return "callback";
    case GI_INFO_TYPE_STRUCT:
      return "struct";
    case GI_INFO_TYPE_ENUM:
      return "enum";
    case GI_INFO_TYPE_FLAGS:
      return "flags";
    case GI_INFO_TYPE_OBJECT:
      return "object";
    case GI_INFO_TYPE_INTERFACE:
      return "interface";
    case GI_INFO_TYPE_CONSTANT:
      return "constant";
    case GI_INFO_TYPE_UNION:
      return "union";
    case GI_INFO_TYPE_VALUE:
      return "value";
    case GI_INFO_TYPE_SIGNAL:
      return "signal";
    case GI_INFO_TYPE_VFUNC:
      return "vfunc";
    case GI_INFO_TYPE_PROPERTY:
      return "property";
    case GI_INFO_TYPE_FIELD:
      return "field";
    case GI_INFO_TYPE_ARG:
      return "arg";
    case GI_INFO_TYPE_TYPE:
      return "type";
    case GI_INFO_TYPE_UNRESOLVED:
      return "unresolved";
    default:
      return "unknown";
  }
}

GIInfoType
gi_typelib_blob_type_to_info_type (GITypelibBlobType blob_type)
{
  switch (blob_type)
    {
    case BLOB_TYPE_BOXED:
      /* `BLOB_TYPE_BOXED` now always refers to a `StructBlob`, and
       * `GIRegisteredTypeInfo` (the parent type of `GIStructInfo`) has a method
       * for distinguishing whether the struct is a boxed type. So presenting
       * `BLOB_TYPE_BOXED` as its own `GIBaseInfo` subclass is not helpful.
       * See commit e28078c70cbf4a57c7dbd39626f43f9bd2674145 and
       * https://gitlab.gnome.org/GNOME/glib/-/issues/3245. */
      return GI_INFO_TYPE_STRUCT;
    default:
      return (GIInfoType) blob_type;
    }
}

/**
 * gi_repository_dup_default:
 *
 * Gets the singleton process-global default `GIRepository`.
 *
 * The singleton is needed for situations where you must coordinate between
 * bindings and libraries which also need to interact with introspection which
 * could affect the bindings. For example, a Python application using a
 * GObject-based library through `GIRepository` to load plugins also written in
 * Python.
 *
 * Returns: (transfer full): the global singleton repository
 *
 * Since: 2.86
 */
GIRepository *
gi_repository_dup_default (void)
{
  static GIRepository *instance;

  if (g_once_init_enter (&instance))
    {
      GIRepository *repository = gi_repository_new ();
      g_object_add_weak_pointer (G_OBJECT (repository), (gpointer *)&instance);
      g_once_init_leave (&instance, repository);
    }

  return g_object_ref (instance);
}
