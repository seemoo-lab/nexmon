/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2006-2007 Red Hat, Inc.
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
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#include "config.h"
#include "gcontenttypeprivate.h"
#include "gfile.h"



/**
 * g_content_type_set_mime_dirs:
 * @dirs: (array zero-terminated=1) (nullable): %NULL-terminated list of
 *    directories to load MIME data from, including any `mime/` subdirectory,
 *    and with the first directory to try listed first
 *
 * Set the list of directories used by GIO to load the MIME database.
 * If @dirs is %NULL, the directories used are the default:
 *
 *  - the `mime` subdirectory of the directory in `$XDG_DATA_HOME`
 *  - the `mime` subdirectory of every directory in `$XDG_DATA_DIRS`
 *
 * This function is intended to be used when writing tests that depend on
 * information stored in the MIME database, in order to control the data.
 *
 * Typically, in case your tests use %G_TEST_OPTION_ISOLATE_DIRS, but they
 * depend on the system’s MIME database, you should call this function
 * with @dirs set to %NULL before calling g_test_init(), for instance:
 *
 * |[<!-- language="C" -->
 *   // Load MIME data from the system
 *   g_content_type_set_mime_dirs (NULL);
 *   // Isolate the environment
 *   g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);
 *
 *   …
 *
 *   return g_test_run ();
 * ]|
 *
 * Since: 2.60
 */
void
g_content_type_set_mime_dirs (const gchar * const *dirs)
{
  g_content_type_set_mime_dirs_impl (dirs);
}

/**
 * g_content_type_get_mime_dirs:
 *
 * Get the list of directories which MIME data is loaded from. See
 * g_content_type_set_mime_dirs() for details.
 *
 * Returns: (transfer none) (array zero-terminated=1): %NULL-terminated list of
 *    directories to load MIME data from, including any `mime/` subdirectory,
 *    and with the first directory to try listed first
 * Since: 2.60
 */
const gchar * const *
g_content_type_get_mime_dirs (void)
{
  return g_content_type_get_mime_dirs_impl ();
}

/**
 * g_content_type_equals:
 * @type1: a content type string
 * @type2: a content type string
 *
 * Compares two content types for equality.
 *
 * Returns: %TRUE if the two strings are identical or equivalent,
 *     %FALSE otherwise.
 */
gboolean
g_content_type_equals (const gchar *type1,
                       const gchar *type2)
{
  g_return_val_if_fail (type1 != NULL, FALSE);
  g_return_val_if_fail (type2 != NULL, FALSE);

  return g_content_type_equals_impl (type1, type2);
}

/**
 * g_content_type_is_a:
 * @type: a content type string
 * @supertype: a content type string
 *
 * Determines if @type is a subset of @supertype.
 *
 * Returns: %TRUE if @type is a kind of @supertype,
 *     %FALSE otherwise.
 */
gboolean
g_content_type_is_a (const gchar *type,
                     const gchar *supertype)
{
  g_return_val_if_fail (type != NULL, FALSE);
  g_return_val_if_fail (supertype != NULL, FALSE);

  return g_content_type_is_a_impl (type, supertype);
}

/**
 * g_content_type_is_mime_type:
 * @type: a content type string
 * @mime_type: a mime type string
 *
 * Determines if @type is a subset of @mime_type.
 * Convenience wrapper around g_content_type_is_a().
 *
 * Returns: %TRUE if @type is a kind of @mime_type,
 *     %FALSE otherwise.
 *
 * Since: 2.52
 */
gboolean
g_content_type_is_mime_type (const gchar *type,
                             const gchar *mime_type)
{
  g_return_val_if_fail (type != NULL, FALSE);
  g_return_val_if_fail (mime_type != NULL, FALSE);

  return g_content_type_is_mime_type_impl (type, mime_type);
}

/**
 * g_content_type_is_unknown:
 * @type: a content type string
 *
 * Checks if the content type is the generic "unknown" type.
 * On UNIX this is the "application/octet-stream" mimetype,
 * while on win32 it is "*" and on OSX it is a dynamic type
 * or octet-stream.
 *
 * Returns: %TRUE if the type is the unknown type.
 */
gboolean
g_content_type_is_unknown (const gchar *type)
{
  g_return_val_if_fail (type != NULL, FALSE);

  return g_content_type_is_unknown_impl (type);
}

/**
 * g_content_type_get_description:
 * @type: a content type string
 *
 * Gets the human readable description of the content type.
 *
 * Returns: a short description of the content type @type. Free the
 *     returned string with g_free()
 */
gchar *
g_content_type_get_description (const gchar *type)
{
  g_return_val_if_fail (type != NULL, NULL);

  return g_content_type_get_description_impl (type);
}

/**
 * g_content_type_get_mime_type:
 * @type: a content type string
 *
 * Gets the mime type for the content type, if one is registered.
 *
 * Returns: (nullable) (transfer full): the registered mime type for the
 *     given @type, or %NULL if unknown; free with g_free().
 */
char *
g_content_type_get_mime_type (const char *type)
{
  g_return_val_if_fail (type != NULL, NULL);

  return g_content_type_get_mime_type_impl (type);
}

/**
 * g_content_type_get_icon:
 * @type: a content type string
 *
 * Gets the icon for a content type.
 *
 * Returns: (transfer full): #GIcon corresponding to the content type. Free the returned
 *     object with g_object_unref()
 */
GIcon *
g_content_type_get_icon (const gchar *type)
{
  g_return_val_if_fail (type != NULL, NULL);

  return g_content_type_get_icon_impl (type);
}

/**
 * g_content_type_get_symbolic_icon:
 * @type: a content type string
 *
 * Gets the symbolic icon for a content type.
 *
 * Returns: (transfer full): symbolic #GIcon corresponding to the content type.
 *     Free the returned object with g_object_unref()
 *
 * Since: 2.34
 */
GIcon *
g_content_type_get_symbolic_icon (const gchar *type)
{
  g_return_val_if_fail (type != NULL, NULL);

  return g_content_type_get_symbolic_icon_impl (type);
}

/**
 * g_content_type_get_generic_icon_name:
 * @type: a content type string
 *
 * Gets the generic icon name for a content type.
 *
 * See the
 * [shared-mime-info](http://www.freedesktop.org/wiki/Specifications/shared-mime-info-spec)
 * specification for more on the generic icon name.
 *
 * Returns: (nullable): the registered generic icon name for the given @type,
 *     or %NULL if unknown. Free with g_free()
 *
 * Since: 2.34
 */
gchar *
g_content_type_get_generic_icon_name (const gchar *type)
{
  g_return_val_if_fail (type != NULL, NULL);

  return g_content_type_get_generic_icon_name_impl (type);
}

/**
 * g_content_type_can_be_executable:
 * @type: a content type string
 *
 * Checks if a content type can be executable. Note that for instance
 * things like text files can be executables (i.e. scripts and batch files).
 *
 * Returns: %TRUE if the file type corresponds to a type that
 *     can be executable, %FALSE otherwise.
 */
gboolean
g_content_type_can_be_executable (const gchar *type)
{
  g_return_val_if_fail (type != NULL, FALSE);

  return g_content_type_can_be_executable_impl (type);
}

/**
 * g_content_type_from_mime_type:
 * @mime_type: a mime type string
 *
 * Tries to find a content type based on the mime type name.
 *
 * Returns: (nullable): Newly allocated string with content type or
 *     %NULL. Free with g_free()
 *
 * Since: 2.18
 **/
gchar *
g_content_type_from_mime_type (const gchar *mime_type)
{
  g_return_val_if_fail (mime_type != NULL, NULL);

  return g_content_type_from_mime_type_impl (mime_type);
}

/**
 * g_content_type_guess:
 * @filename: (nullable) (type filename): a path, or %NULL
 * @data: (nullable) (array length=data_size): a stream of data, or %NULL
 * @data_size: the size of @data
 * @result_uncertain: (out) (optional): return location for the certainty
 *     of the result, or %NULL
 *
 * Guesses the content type based on example data. If the function is
 * uncertain, @result_uncertain will be set to %TRUE. Either @filename
 * or @data may be %NULL, in which case the guess will be based solely
 * on the other argument.
 *
 * Returns: a string indicating a guessed content type for the
 *     given data. Free with g_free()
 */
gchar *
g_content_type_guess (const gchar  *filename,
                      const guchar *data,
                      gsize         data_size,
                      gboolean     *result_uncertain)
{
  return g_content_type_guess_impl (filename, data, data_size, result_uncertain);
}

/**
 * g_content_types_get_registered:
 *
 * Gets a list of strings containing all the registered content types
 * known to the system. The list and its data should be freed using
 * `g_list_free_full (list, g_free)`.
 *
 * Returns: (element-type utf8) (transfer full): list of the registered
 *     content types
 */
GList *
g_content_types_get_registered (void)
{
  return g_content_types_get_registered_impl ();
}

/**
 * g_content_type_guess_for_tree:
 * @root: the root of the tree to guess a type for
 *
 * Tries to guess the type of the tree with root @root, by
 * looking at the files it contains. The result is an array
 * of content types, with the best guess coming first.
 *
 * The types returned all have the form x-content/foo, e.g.
 * x-content/audio-cdda (for audio CDs) or x-content/image-dcf
 * (for a camera memory card). See the
 * [shared-mime-info](http://www.freedesktop.org/wiki/Specifications/shared-mime-info-spec)
 * specification for more on x-content types.
 *
 * This function is useful in the implementation of
 * g_mount_guess_content_type().
 *
 * Returns: (transfer full) (array zero-terminated=1): an %NULL-terminated
 *     array of zero or more content types. Free with g_strfreev()
 *
 * Since: 2.18
 */
gchar **
g_content_type_guess_for_tree (GFile *root)
{
  g_return_val_if_fail (G_IS_FILE (root), NULL);

  return g_content_type_guess_for_tree_impl (root);
}
