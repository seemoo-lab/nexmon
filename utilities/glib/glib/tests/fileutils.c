/* Unit tests for gfileutils
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * SPDX-License-Identifier: LicenseRef-old-glib-tests
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */

#include "config.h"
#include <string.h>
#include <errno.h>

/* We are testing some deprecated APIs here */
#ifndef GLIB_DISABLE_DEPRECATION_WARNINGS
#define GLIB_DISABLE_DEPRECATION_WARNINGS
#endif

#include <glib.h>

/* Test our stdio wrappers here; this disables redefining (e.g.) g_open() to open() */
#define G_STDIO_WRAP_ON_UNIX
#include <glib/gstdio.h>
#include "glib-private.h"

#ifdef G_OS_UNIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <utime.h>

#define G_TEST_DIR_MODE 0555
#endif
#include <fcntl.h>
#ifdef G_OS_WIN32
#include <windows.h>
#include <sys/utime.h>
#include <io.h>
#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & _S_IFMT) == _S_IFDIR)
#endif
#ifndef F_OK
#define F_OK 0
#endif

#define G_TEST_DIR_MODE (S_IWRITE | S_IREAD)
#endif

#include "testutils.h"

#define S G_DIR_SEPARATOR_S

static void
check_string (gchar *str, const gchar *expected)
{
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, expected);
  g_free (str);
}

static void
test_paths (void)
{
  struct
  {
    const char *filename;
    const char *expected_dirname;
    const char *expected_basename;
  } dirname_checks[] = {
    { "/", "/", G_DIR_SEPARATOR_S },
    { "////", "/", G_DIR_SEPARATOR_S },
    { ".////", ".", "." },
    { "../", "..", ".." },
    { "..////", "..", ".." },
    { "a/b", "a", "b" },
    { "a/b/", "a/b", "b" },
    { "c///", "c", "c" },
    { "OSTree-1.0.gir", ".", "OSTree-1.0.gir" },
    { "/some/multi/part/path", "/some/multi/part", "path" },
#ifdef G_OS_WIN32
    { "\\", "\\", "\\" },
    { ".\\\\\\\\", ".", "." },
    { "..\\", "..", ".." },
    { "..\\\\\\\\", "..", ".." },
    { "a\\b", "a", "b" },
    { "a\\b/", "a\\b", "b" },
    { "a/b\\", "a/b", "b" },
    { "c\\\\/", "c", "c" },
    { "//\\", "/", G_DIR_SEPARATOR_S },
#endif
#ifdef G_WITH_CYGWIN
    { "//server/share///x", "//server/share", "x" },
#endif
    { ".", ".", "." },
    { "..", ".", ".." },
    { "", ".", "." },  /* note this is different from what the `basename` command does */
    { G_DIR_SEPARATOR_S "foo" G_DIR_SEPARATOR_S "dir" G_DIR_SEPARATOR_S, G_DIR_SEPARATOR_S "foo" G_DIR_SEPARATOR_S "dir", "dir" },
    { G_DIR_SEPARATOR_S "foo" G_DIR_SEPARATOR_S "file", G_DIR_SEPARATOR_S "foo", "file" },
#ifdef G_OS_WIN32
    { "/foo/dir/", "/foo/dir", "dir" },
    { "/foo/file", "/foo", "file" },
#endif
  };
  const guint n_dirname_checks = G_N_ELEMENTS (dirname_checks);
  struct
  {
    gchar *filename;
    gchar *without_root;
  } skip_root_checks[] = {
    { "/", "" },
    { "//", "" },
    { "/foo", "foo" },
    { "//foo", "foo" },
    { "a/b", NULL },
#ifdef G_OS_WIN32
    { "\\", "" },
    { "\\foo", "foo" },
    { "\\\\server\\foo", "" },
    { "\\\\server\\foo\\bar", "bar" },
    { "a\\b", NULL },
#endif
#ifdef G_WITH_CYGWIN
    { "//server/share///x", "//x" },
#endif
    { ".", NULL },
    { "", NULL },
  };
  const guint n_skip_root_checks = G_N_ELEMENTS (skip_root_checks);
  struct
  {
    gchar *cwd;
    gchar *relative_path;
    gchar *canonical_path;
  } canonicalize_filename_checks[] = {
#ifndef G_OS_WIN32
    { "/etc", "../usr/share", "/usr/share" },
    { "/", "/foo/bar", "/foo/bar" },
    { "/usr/bin", "../../foo/bar", "/foo/bar" },
    { "/", "../../foo/bar", "/foo/bar" },
    { "/double//dash", "../../foo/bar", "/foo/bar" },
    { "/usr/share/foo", ".././././bar", "/usr/share/bar" },
    { "/foo/bar", "../bar/./.././bar", "/foo/bar" },
    { "/test///dir", "../../././foo/bar", "/foo/bar" },
    { "/test///dir", "../../././/foo///bar", "/foo/bar" },
    { "/etc", "///triple/slash", "/triple/slash" },
    { "/etc", "//double/slash", "//double/slash" },
    { "///triple/slash", ".", "/triple/slash" },
    { "//double/slash", ".", "//double/slash" },
    { "/cwd/../with/./complexities/", "./hello", "/with/complexities/hello" },
    { "/", ".dot-dir", "/.dot-dir" },
    { "/cwd", "..", "/" },
    { "/etc", "hello/..", "/etc" },
    { "/etc", "hello/../", "/etc" },
    { "/", "..", "/" },
    { "/", "../", "/" },
    { "/", "/..", "/" },
    { "/", "/../", "/" },
    { "/", ".", "/" },
    { "/", "./", "/" },
    { "/", "/.", "/" },
    { "/", "/./", "/" },
    { "/", "///usr/../usr", "/usr" },
#else
    { "/etc", "../usr/share", "\\usr\\share" },
    { "/", "/foo/bar", "\\foo\\bar" },
    { "/usr/bin", "../../foo/bar", "\\foo\\bar" },
    { "/", "../../foo/bar", "\\foo\\bar" },
    { "/double//dash", "../../foo/bar", "\\foo\\bar" },
    { "/usr/share/foo", ".././././bar", "\\usr\\share\\bar" },
    { "/foo/bar", "../bar/./.././bar", "\\foo\\bar" },
    { "/test///dir", "../../././foo/bar", "\\foo\\bar" },
    { "/test///dir", "../../././/foo///bar", "\\foo\\bar" },
    { "/etc", "///triple/slash", "\\triple\\slash" },
    { "/etc", "//double/slash", "//double/slash" },
    { "///triple/slash", ".", "\\triple\\slash" },
    { "//double/slash", ".", "//double/slash\\" },
    { "/cwd/../with/./complexities/", "./hello", "\\with\\complexities\\hello" },
    { "/", ".dot-dir", "\\.dot-dir" },
    { "/cwd", "..", "\\" },
    { "/etc", "hello/..", "\\etc" },
    { "/etc", "hello/../", "\\etc" },
    { "/", "..", "\\" },
    { "/", "../", "\\" },
    { "/", "/..", "\\" },
    { "/", "/../", "\\" },
    { "/", ".", "\\" },
    { "/", "./", "\\" },
    { "/", "/.", "\\" },
    { "/", "/./", "\\" },
    { "/", "///usr/../usr", "\\usr" },

    { "\\etc", "..\\usr\\share", "\\usr\\share" },
    { "\\", "\\foo\\bar", "\\foo\\bar" },
    { "\\usr\\bin", "..\\..\\foo\\bar", "\\foo\\bar" },
    { "\\", "..\\..\\foo\\bar", "\\foo\\bar" },
    { "\\double\\\\dash", "..\\..\\foo\\bar", "\\foo\\bar" },
    { "\\usr\\share\\foo", "..\\.\\.\\.\\bar", "\\usr\\share\\bar" },
    { "\\foo\\bar", "..\\bar\\.\\..\\.\\bar", "\\foo\\bar" },
    { "\\test\\\\\\dir", "..\\..\\.\\.\\foo\\bar", "\\foo\\bar" },
    { "\\test\\\\\\dir", "..\\..\\.\\.\\\\foo\\\\\\bar", "\\foo\\bar" },
    { "\\etc", "\\\\\\triple\\slash", "\\triple\\slash" },
    { "\\etc", "\\\\double\\slash", "\\\\double\\slash" },
    { "\\\\\\triple\\slash", ".", "\\triple\\slash" },
    { "\\\\double\\slash", ".", "\\\\double\\slash\\" },
    { "\\cwd\\..\\with\\.\\complexities\\", ".\\hello", "\\with\\complexities\\hello" },
    { "\\", ".dot-dir", "\\.dot-dir" },
    { "\\cwd", "..", "\\" },
    { "\\etc", "hello\\..", "\\etc" },
    { "\\etc", "hello\\..\\", "\\etc" },
    { "\\", "..", "\\" },
    { "\\", "..\\", "\\" },
    { "\\", "\\..", "\\" },
    { "\\", "\\..\\", "\\" },
    { "\\", ".", "\\" },
    { "\\", ".\\", "\\" },
    { "\\", "\\.", "\\" },
    { "\\", "\\.\\", "\\" },
    { "\\", "\\\\\\usr\\..\\usr", "\\usr" },
#endif
  };
  const guint n_canonicalize_filename_checks = G_N_ELEMENTS (canonicalize_filename_checks);
  guint i;

  for (i = 0; i < n_dirname_checks; i++)
    {
      gchar *dirname = NULL, *basename = NULL;

      dirname = g_path_get_dirname (dirname_checks[i].filename);
      g_assert_cmpstr (dirname, ==, dirname_checks[i].expected_dirname);
      g_free (dirname);

      basename = g_path_get_basename (dirname_checks[i].filename);
      g_assert_cmpstr (basename, ==, dirname_checks[i].expected_basename);
      g_free (basename);
    }

  for (i = 0; i < n_skip_root_checks; i++)
    {
      const gchar *skipped = g_path_skip_root (skip_root_checks[i].filename);
      if ((skipped && !skip_root_checks[i].without_root) ||
          (!skipped && skip_root_checks[i].without_root) ||
          ((skipped && skip_root_checks[i].without_root) &&
           strcmp (skipped, skip_root_checks[i].without_root)))
        {
          g_error ("failed for \"%s\"==\"%s\" (returned: \"%s\")",
                   skip_root_checks[i].filename,
                   (skip_root_checks[i].without_root ? skip_root_checks[i].without_root : "<NULL>"),
                   (skipped ? skipped : "<NULL>"));
        }
    }

  for (i = 0; i < n_canonicalize_filename_checks; i++)
    {
      gchar *canonical_path =
          g_canonicalize_filename (canonicalize_filename_checks[i].relative_path,
                                   canonicalize_filename_checks[i].cwd);
      g_assert_cmpstr (canonical_path, ==,
                       canonicalize_filename_checks[i].canonical_path);
      g_free (canonical_path);
    }

  {
    const gchar *relative_path = "./";
    gchar *canonical_path = g_canonicalize_filename (relative_path, NULL);
    gchar *cwd = g_get_current_dir ();
    g_assert_cmpstr (canonical_path, ==, cwd);
    g_free (cwd);
    g_free (canonical_path);
  }
}

static void
test_build_path (void)
{
  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_null (g_build_path (NULL, "x", "y", NULL));
      g_test_assert_expected_messages ();
    }

/*  check_string (g_build_path ("", NULL), "");*/
  check_string (g_build_path ("", "", NULL), "");
  check_string (g_build_path ("", "x", NULL), "x");
  check_string (g_build_path ("", "x", "y",  NULL), "xy");
  check_string (g_build_path ("", "x", "y", "z", NULL), "xyz");

/*  check_string (g_build_path (":", NULL), "");*/
  check_string (g_build_path (":", ":", NULL), ":");
  check_string (g_build_path (":", ":x", NULL), ":x");
  check_string (g_build_path (":", "x:", NULL), "x:");
  check_string (g_build_path (":", "", "x", NULL), "x");
  check_string (g_build_path (":", "", ":x", NULL), ":x");
  check_string (g_build_path (":", ":", "x", NULL), ":x");
  check_string (g_build_path (":", "::", "x", NULL), "::x");
  check_string (g_build_path (":", "x", "", NULL), "x");
  check_string (g_build_path (":", "x:", "", NULL), "x:");
  check_string (g_build_path (":", "x", ":", NULL), "x:");
  check_string (g_build_path (":", "x", "::", NULL), "x::");
  check_string (g_build_path (":", "x", "y",  NULL), "x:y");
  check_string (g_build_path (":", ":x", "y", NULL), ":x:y");
  check_string (g_build_path (":", "x", "y:", NULL), "x:y:");
  check_string (g_build_path (":", ":x:", ":y:", NULL), ":x:y:");
  check_string (g_build_path (":", ":x::", "::y:", NULL), ":x:y:");
  check_string (g_build_path (":", "x", "","y",  NULL), "x:y");
  check_string (g_build_path (":", "x", ":", "y",  NULL), "x:y");
  check_string (g_build_path (":", "x", "::", "y",  NULL), "x:y");
  check_string (g_build_path (":", "x", "y", "z", NULL), "x:y:z");
  check_string (g_build_path (":", ":x:", ":y:", ":z:", NULL), ":x:y:z:");
  check_string (g_build_path (":", "::x::", "::y::", "::z::", NULL), "::x:y:z::");

/*  check_string (g_build_path ("::", NULL), "");*/
  check_string (g_build_path ("::", "::", NULL), "::");
  check_string (g_build_path ("::", ":::", NULL), ":::");
  check_string (g_build_path ("::", "::x", NULL), "::x");
  check_string (g_build_path ("::", "x::", NULL), "x::");
  check_string (g_build_path ("::", "", "x", NULL), "x");
  check_string (g_build_path ("::", "", "::x", NULL), "::x");
  check_string (g_build_path ("::", "::", "x", NULL), "::x");
  check_string (g_build_path ("::", "::::", "x", NULL), "::::x");
  check_string (g_build_path ("::", "x", "", NULL), "x");
  check_string (g_build_path ("::", "x::", "", NULL), "x::");
  check_string (g_build_path ("::", "x", "::", NULL), "x::");

  /* This following is weird, but keeps the definition simple */
  check_string (g_build_path ("::", "x", ":::", NULL), "x:::::");
  check_string (g_build_path ("::", "x", "::::", NULL), "x::::");
  check_string (g_build_path ("::", "x", "y",  NULL), "x::y");
  check_string (g_build_path ("::", "::x", "y", NULL), "::x::y");
  check_string (g_build_path ("::", "x", "y::", NULL), "x::y::");
  check_string (g_build_path ("::", "::x::", "::y::", NULL), "::x::y::");
  check_string (g_build_path ("::", "::x:::", ":::y::", NULL), "::x::::y::");
  check_string (g_build_path ("::", "::x::::", "::::y::", NULL), "::x::y::");
  check_string (g_build_path ("::", "x", "", "y",  NULL), "x::y");
  check_string (g_build_path ("::", "x", "::", "y",  NULL), "x::y");
  check_string (g_build_path ("::", "x", "::::", "y",  NULL), "x::y");
  check_string (g_build_path ("::", "x", "y", "z", NULL), "x::y::z");
  check_string (g_build_path ("::", "::x::", "::y::", "::z::", NULL), "::x::y::z::");
  check_string (g_build_path ("::", ":::x:::", ":::y:::", ":::z:::", NULL), ":::x::::y::::z:::");
  check_string (g_build_path ("::", "::::x::::", "::::y::::", "::::z::::", NULL), "::::x::y::z::::");
}

static void
test_build_pathv (void)
{
  gchar *args[10];

  g_assert_null (g_build_pathv ("", NULL));
  args[0] = NULL;
  check_string (g_build_pathv ("", args), "");
  args[0] = ""; args[1] = NULL;
  check_string (g_build_pathv ("", args), "");
  args[0] = "x"; args[1] = NULL;
  check_string (g_build_pathv ("", args), "x");
  args[0] = "x"; args[1] = "y"; args[2] = NULL;
  check_string (g_build_pathv ("", args), "xy");
  args[0] = "x"; args[1] = "y"; args[2] = "z", args[3] = NULL;
  check_string (g_build_pathv ("", args), "xyz");

  args[0] = NULL;
  check_string (g_build_pathv (":", args), "");
  args[0] = ":"; args[1] = NULL;
  check_string (g_build_pathv (":", args), ":");
  args[0] = ":x"; args[1] = NULL;
  check_string (g_build_pathv (":", args), ":x");
  args[0] = "x:"; args[1] = NULL;
  check_string (g_build_pathv (":", args), "x:");
  args[0] = ""; args[1] = "x"; args[2] = NULL;
  check_string (g_build_pathv (":", args), "x");
  args[0] = ""; args[1] = ":x"; args[2] = NULL;
  check_string (g_build_pathv (":", args), ":x");
  args[0] = ":"; args[1] = "x"; args[2] = NULL;
  check_string (g_build_pathv (":", args), ":x");
  args[0] = "::"; args[1] = "x"; args[2] = NULL;
  check_string (g_build_pathv (":", args), "::x");
  args[0] = "x"; args[1] = ""; args[2] = NULL;
  check_string (g_build_pathv (":", args), "x");
  args[0] = "x:"; args[1] = ""; args[2] = NULL;
  check_string (g_build_pathv (":", args), "x:");
  args[0] = "x"; args[1] = ":"; args[2] = NULL;
  check_string (g_build_pathv (":", args), "x:");
  args[0] = "x"; args[1] = "::"; args[2] = NULL;
  check_string (g_build_pathv (":", args), "x::");
  args[0] = "x"; args[1] = "y"; args[2] = NULL;
  check_string (g_build_pathv (":", args), "x:y");
  args[0] = ":x"; args[1] = "y"; args[2] = NULL;
  check_string (g_build_pathv (":", args), ":x:y");
  args[0] = "x"; args[1] = "y:"; args[2] = NULL;
  check_string (g_build_pathv (":", args), "x:y:");
  args[0] = ":x:"; args[1] = ":y:"; args[2] = NULL;
  check_string (g_build_pathv (":", args), ":x:y:");
  args[0] = ":x::"; args[1] = "::y:"; args[2] = NULL;
  check_string (g_build_pathv (":", args), ":x:y:");
  args[0] = "x"; args[1] = ""; args[2] = "y"; args[3] = NULL;
  check_string (g_build_pathv (":", args), "x:y");
  args[0] = "x"; args[1] = ":"; args[2] = "y"; args[3] = NULL;
  check_string (g_build_pathv (":", args), "x:y");
  args[0] = "x"; args[1] = "::"; args[2] = "y"; args[3] = NULL;
  check_string (g_build_pathv (":", args), "x:y");
  args[0] = "x"; args[1] = "y"; args[2] = "z"; args[3] = NULL;
  check_string (g_build_pathv (":", args), "x:y:z");
  args[0] = ":x:"; args[1] = ":y:"; args[2] = ":z:"; args[3] = NULL;
  check_string (g_build_pathv (":", args), ":x:y:z:");
  args[0] = "::x::"; args[1] = "::y::"; args[2] = "::z::"; args[3] = NULL;
  check_string (g_build_pathv (":", args), "::x:y:z::");

  args[0] = NULL;
  check_string (g_build_pathv ("::", args), "");
  args[0] = "::"; args[1] = NULL;
  check_string (g_build_pathv ("::", args), "::");
  args[0] = ":::"; args[1] = NULL;
  check_string (g_build_pathv ("::", args), ":::");
  args[0] = "::x"; args[1] = NULL;
  check_string (g_build_pathv ("::", args), "::x");
  args[0] = "x::"; args[1] = NULL;
  check_string (g_build_pathv ("::", args), "x::");
  args[0] = ""; args[1] = "x"; args[2] = NULL;
  check_string (g_build_pathv ("::", args), "x");
  args[0] = ""; args[1] = "::x"; args[2] = NULL;
  check_string (g_build_pathv ("::", args), "::x");
  args[0] = "::"; args[1] = "x"; args[2] = NULL;
  check_string (g_build_pathv ("::", args), "::x");
  args[0] = "::::"; args[1] = "x"; args[2] = NULL;
  check_string (g_build_pathv ("::", args), "::::x");
  args[0] = "x"; args[1] = ""; args[2] = NULL;
  check_string (g_build_pathv ("::", args), "x");
  args[0] = "x::"; args[1] = ""; args[2] = NULL;
  check_string (g_build_pathv ("::", args), "x::");
  args[0] = "x"; args[1] = "::"; args[2] = NULL;
  check_string (g_build_pathv ("::", args), "x::");
  /* This following is weird, but keeps the definition simple */
  args[0] = "x"; args[1] = ":::"; args[2] = NULL;
  check_string (g_build_pathv ("::", args), "x:::::");
  args[0] = "x"; args[1] = "::::"; args[2] = NULL;
  check_string (g_build_pathv ("::", args), "x::::");
  args[0] = "x"; args[1] = "y"; args[2] = NULL;
  check_string (g_build_pathv ("::", args), "x::y");
  args[0] = "::x"; args[1] = "y"; args[2] = NULL;
  check_string (g_build_pathv ("::", args), "::x::y");
  args[0] = "x"; args[1] = "y::"; args[2] = NULL;
  check_string (g_build_pathv ("::", args), "x::y::");
  args[0] = "::x::"; args[1] = "::y::"; args[2] = NULL;
  check_string (g_build_pathv ("::", args), "::x::y::");
  args[0] = "::x:::"; args[1] = ":::y::"; args[2] = NULL;
  check_string (g_build_pathv ("::", args), "::x::::y::");
  args[0] = "::x::::"; args[1] = "::::y::"; args[2] = NULL;
  check_string (g_build_pathv ("::", args), "::x::y::");
  args[0] = "x"; args[1] = ""; args[2] = "y"; args[3] = NULL;
  check_string (g_build_pathv ("::", args), "x::y");
  args[0] = "x"; args[1] = "::"; args[2] = "y"; args[3] = NULL;
  check_string (g_build_pathv ("::", args), "x::y");
  args[0] = "x"; args[1] = "::::"; args[2] = "y"; args[3] = NULL;
  check_string (g_build_pathv ("::", args), "x::y");
  args[0] = "x"; args[1] = "y"; args[2] = "z"; args[3] = NULL;
  check_string (g_build_pathv ("::", args), "x::y::z");
  args[0] = "::x::"; args[1] = "::y::"; args[2] = "::z::"; args[3] = NULL;
  check_string (g_build_pathv ("::", args), "::x::y::z::");
  args[0] = ":::x:::"; args[1] = ":::y:::"; args[2] = ":::z:::"; args[3] = NULL;
  check_string (g_build_pathv ("::", args), ":::x::::y::::z:::");
  args[0] = "::::x::::"; args[1] = "::::y::::"; args[2] = "::::z::::"; args[3] = NULL;
  check_string (g_build_pathv ("::", args), "::::x::y::z::::");
}

static void
test_build_filename (void)
{
  check_string (g_build_filename (S, NULL), S);
  check_string (g_build_filename (S"x", NULL), S"x");
  check_string (g_build_filename ("x"S, NULL), "x"S);
  check_string (g_build_filename ("", "x", NULL), "x");
  check_string (g_build_filename ("", S"x", NULL), S"x");
  check_string (g_build_filename (S, "x", NULL), S"x");
  check_string (g_build_filename (S S, "x", NULL), S S"x");
  check_string (g_build_filename ("x", "", NULL), "x");
  check_string (g_build_filename ("x"S, "", NULL), "x"S);
  check_string (g_build_filename ("x", S, NULL), "x"S);
  check_string (g_build_filename ("x", S S, NULL), "x"S S);
  check_string (g_build_filename ("x", "y",  NULL), "x"S"y");
  check_string (g_build_filename (S"x", "y", NULL), S"x"S"y");
  check_string (g_build_filename ("x", "y"S, NULL), "x"S"y"S);
  check_string (g_build_filename (S"x"S, S"y"S, NULL), S"x"S"y"S);
  check_string (g_build_filename (S"x"S S, S S"y"S, NULL), S"x"S"y"S);
  check_string (g_build_filename ("x", "", "y",  NULL), "x"S"y");
  check_string (g_build_filename ("x", S, "y",  NULL), "x"S"y");
  check_string (g_build_filename ("x", S S, "y",  NULL), "x"S"y");
  check_string (g_build_filename ("x", "y", "z", NULL), "x"S"y"S"z");
  check_string (g_build_filename (S"x"S, S"y"S, S"z"S, NULL), S"x"S"y"S"z"S);
  check_string (g_build_filename (S S"x"S S, S S"y"S S, S S"z"S S, NULL), S S"x"S"y"S"z"S S);

#ifdef G_OS_WIN32

  /* Test also using the slash as file name separator */
#define Z "/"
  /* check_string (g_build_filename (NULL), ""); */
  check_string (g_build_filename (Z, NULL), Z);
  check_string (g_build_filename (Z"x", NULL), Z"x");
  check_string (g_build_filename ("x"Z, NULL), "x"Z);
  check_string (g_build_filename ("", Z"x", NULL), Z"x");
  check_string (g_build_filename ("", Z"x", NULL), Z"x");
  check_string (g_build_filename (Z, "x", NULL), Z"x");
  check_string (g_build_filename (Z Z, "x", NULL), Z Z"x");
  check_string (g_build_filename (Z S, "x", NULL), Z S"x");
  check_string (g_build_filename ("x"Z, "", NULL), "x"Z);
  check_string (g_build_filename ("x"S"y", "z"Z"a", NULL), "x"S"y"S"z"Z"a");
  check_string (g_build_filename ("x", Z, NULL), "x"Z);
  check_string (g_build_filename ("x", Z Z, NULL), "x"Z Z);
  check_string (g_build_filename ("x", S Z, NULL), "x"S Z);
  check_string (g_build_filename (Z"x", "y", NULL), Z"x"Z"y");
  check_string (g_build_filename ("x", "y"Z, NULL), "x"Z"y"Z);
  check_string (g_build_filename (Z"x"Z, Z"y"Z, NULL), Z"x"Z"y"Z);
  check_string (g_build_filename (Z"x"Z Z, Z Z"y"Z, NULL), Z"x"Z"y"Z);
  check_string (g_build_filename ("x", Z, "y",  NULL), "x"Z"y");
  check_string (g_build_filename ("x", Z Z, "y",  NULL), "x"Z"y");
  check_string (g_build_filename ("x", Z S, "y",  NULL), "x"S"y");
  check_string (g_build_filename ("x", S Z, "y",  NULL), "x"Z"y");
  check_string (g_build_filename ("x", Z "y", "z", NULL), "x"Z"y"Z"z");
  check_string (g_build_filename ("x", S "y", "z", NULL), "x"S"y"S"z");
  check_string (g_build_filename ("x", S "y", "z", Z, "a", "b", NULL), "x"S"y"S"z"Z"a"Z"b");
  check_string (g_build_filename (Z"x"Z, Z"y"Z, Z"z"Z, NULL), Z"x"Z"y"Z"z"Z);
  check_string (g_build_filename (Z Z"x"Z Z, Z Z"y"Z Z, Z Z"z"Z Z, NULL), Z Z"x"Z"y"Z"z"Z Z);

#undef Z

#endif /* G_OS_WIN32 */

}

static void
test_build_filenamev (void)
{
  gchar *args[10];

  args[0] = NULL;
  check_string (g_build_filenamev (args), "");
  args[0] = S; args[1] = NULL;
  check_string (g_build_filenamev (args), S);
  args[0] = S"x"; args[1] = NULL;
  check_string (g_build_filenamev (args), S"x");
  args[0] = "x"S; args[1] = NULL;
  check_string (g_build_filenamev (args), "x"S);
  args[0] = ""; args[1] = "x"; args[2] = NULL;
  check_string (g_build_filenamev (args), "x");
  args[0] = ""; args[1] = S"x"; args[2] = NULL;
  check_string (g_build_filenamev (args), S"x");
  args[0] = S; args[1] = "x"; args[2] = NULL;
  check_string (g_build_filenamev (args), S"x");
  args[0] = S S; args[1] = "x"; args[2] = NULL;
  check_string (g_build_filenamev (args), S S"x");
  args[0] = "x"; args[1] = ""; args[2] = NULL;
  check_string (g_build_filenamev (args), "x");
  args[0] = "x"S; args[1] = ""; args[2] = NULL;
  check_string (g_build_filenamev (args), "x"S);
  args[0] = "x"; args[1] = S; args[2] = NULL;
  check_string (g_build_filenamev (args), "x"S);
  args[0] = "x"; args[1] = S S; args[2] = NULL;
  check_string (g_build_filenamev (args), "x"S S);
  args[0] = "x"; args[1] = "y"; args[2] = NULL;
  check_string (g_build_filenamev (args), "x"S"y");
  args[0] = S"x"; args[1] = "y"; args[2] = NULL;
  check_string (g_build_filenamev (args), S"x"S"y");
  args[0] = "x"; args[1] = "y"S; args[2] = NULL;
  check_string (g_build_filenamev (args), "x"S"y"S);
  args[0] = S"x"S; args[1] = S"y"S; args[2] = NULL;
  check_string (g_build_filenamev (args), S"x"S"y"S);
  args[0] = S"x"S S; args[1] = S S"y"S; args[2] = NULL;
  check_string (g_build_filenamev (args), S"x"S"y"S);
  args[0] = "x"; args[1] = ""; args[2] = "y"; args[3] = NULL;
  check_string (g_build_filenamev (args), "x"S"y");
  args[0] = "x"; args[1] = S; args[2] = "y"; args[3] = NULL;
  check_string (g_build_filenamev (args), "x"S"y");
  args[0] = "x"; args[1] = S S; args[2] = "y"; args[3] = NULL;
  check_string (g_build_filenamev (args), "x"S"y");
  args[0] = "x"; args[1] = "y"; args[2] = "z"; args[3] = NULL;
  check_string (g_build_filenamev (args), "x"S"y"S"z");
  args[0] = S"x"S; args[1] = S"y"S; args[2] = S"z"S; args[3] = NULL;
  check_string (g_build_filenamev (args), S"x"S"y"S"z"S);
  args[0] = S S"x"S S; args[1] = S S"y"S S; args[2] = S S"z"S S; args[3] = NULL;
  check_string (g_build_filenamev (args), S S"x"S"y"S"z"S S);

#ifdef G_OS_WIN32

  /* Test also using the slash as file name separator */
#define Z "/"
  args[0] = NULL;
  check_string (g_build_filenamev (args), "");
  args[0] = Z; args[1] = NULL;
  check_string (g_build_filenamev (args), Z);
  args[0] = Z"x"; args[1] = NULL;
  check_string (g_build_filenamev (args), Z"x");
  args[0] = "x"Z; args[1] = NULL;
  check_string (g_build_filenamev (args), "x"Z);
  args[0] = ""; args[1] = Z"x"; args[2] = NULL;
  check_string (g_build_filenamev (args), Z"x");
  args[0] = ""; args[1] = Z"x"; args[2] = NULL;
  check_string (g_build_filenamev (args), Z"x");
  args[0] = Z; args[1] = "x"; args[2] = NULL;
  check_string (g_build_filenamev (args), Z"x");
  args[0] = Z Z; args[1] = "x"; args[2] = NULL;
  check_string (g_build_filenamev (args), Z Z"x");
  args[0] = Z S; args[1] = "x"; args[2] = NULL;
  check_string (g_build_filenamev (args), Z S"x");
  args[0] = "x"Z; args[1] = ""; args[2] = NULL;
  check_string (g_build_filenamev (args), "x"Z);
  args[0] = "x"S"y"; args[1] = "z"Z"a"; args[2] = NULL;
  check_string (g_build_filenamev (args), "x"S"y"S"z"Z"a");
  args[0] = "x"; args[1] = Z; args[2] = NULL;
  check_string (g_build_filenamev (args), "x"Z);
  args[0] = "x"; args[1] = Z Z; args[2] = NULL;
  check_string (g_build_filenamev (args), "x"Z Z);
  args[0] = "x"; args[1] = S Z; args[2] = NULL;
  check_string (g_build_filenamev (args), "x"S Z);
  args[0] = Z"x"; args[1] = "y"; args[2] = NULL;
  check_string (g_build_filenamev (args), Z"x"Z"y");
  args[0] = "x"; args[1] = "y"Z; args[2] = NULL;
  check_string (g_build_filenamev (args), "x"Z"y"Z);
  args[0] = Z"x"Z; args[1] = Z"y"Z; args[2] = NULL;
  check_string (g_build_filenamev (args), Z"x"Z"y"Z);
  args[0] = Z"x"Z Z; args[1] = Z Z"y"Z; args[2] = NULL;
  check_string (g_build_filenamev (args), Z"x"Z"y"Z);
  args[0] = "x"; args[1] = Z; args[2] = "y", args[3] = NULL;
  check_string (g_build_filenamev (args), "x"Z"y");
  args[0] = "x"; args[1] = Z Z; args[2] = "y", args[3] = NULL;
  check_string (g_build_filenamev (args), "x"Z"y");
  args[0] = "x"; args[1] = Z S; args[2] = "y", args[3] = NULL;
  check_string (g_build_filenamev (args), "x"S"y");
  args[0] = "x"; args[1] = S Z; args[2] = "y", args[3] = NULL;
  check_string (g_build_filenamev (args), "x"Z"y");
  args[0] = "x"; args[1] = Z "y"; args[2] = "z", args[3] = NULL;
  check_string (g_build_filenamev (args), "x"Z"y"Z"z");
  args[0] = "x"; args[1] = S "y"; args[2] = "z", args[3] = NULL;
  check_string (g_build_filenamev (args), "x"S"y"S"z");
  args[0] = "x"; args[1] = S "y"; args[2] = "z", args[3] = Z;
  args[4] = "a"; args[5] = "b"; args[6] = NULL;
  check_string (g_build_filenamev (args), "x"S"y"S"z"Z"a"Z"b");
  args[0] = Z"x"Z; args[1] = Z"y"Z; args[2] = Z"z"Z, args[3] = NULL;
  check_string (g_build_filenamev (args), Z"x"Z"y"Z"z"Z);
  args[0] = Z Z"x"Z Z; args[1] = Z Z"y"Z Z; args[2] = Z Z"z"Z Z, args[3] = NULL;
  check_string (g_build_filenamev (args), Z Z"x"Z"y"Z"z"Z Z);

#undef Z

#endif /* G_OS_WIN32 */
}

#undef S

static void
test_mkdir_with_parents_1 (const gchar *base)
{
  char *p0 = g_build_filename (g_get_tmp_dir (), base, "fum", NULL);
  char *p1 = g_build_filename (p0, "tem", NULL);
  char *p2 = g_build_filename (p1, "zap", NULL);
  FILE *f;

  g_remove (p2);
  g_remove (p1);
  g_remove (p0);

  if (g_file_test (p0, G_FILE_TEST_EXISTS))
    g_error ("failed, %s exists, cannot test g_mkdir_with_parents", p0);

  if (g_file_test (p1, G_FILE_TEST_EXISTS))
    g_error ("failed, %s exists, cannot test g_mkdir_with_parents", p1);

  if (g_file_test (p2, G_FILE_TEST_EXISTS))
    g_error ("failed, %s exists, cannot test g_mkdir_with_parents", p2);

  if (g_mkdir_with_parents (p2, 0777) == -1)
    {
      int errsv = errno;
      g_error ("failed, g_mkdir_with_parents(%s) failed: %s", p2, g_strerror (errsv));
    }

  if (!g_file_test (p2, G_FILE_TEST_IS_DIR))
    g_error ("failed, g_mkdir_with_parents(%s) succeeded, but %s is not a directory", p2, p2);

  if (!g_file_test (p1, G_FILE_TEST_IS_DIR))
    g_error ("failed, g_mkdir_with_parents(%s) succeeded, but %s is not a directory", p2, p1);

  if (!g_file_test (p0, G_FILE_TEST_IS_DIR))
    g_error ("failed, g_mkdir_with_parents(%s) succeeded, but %s is not a directory", p2, p0);

  g_rmdir (p2);
  if (g_file_test (p2, G_FILE_TEST_EXISTS))
    g_error ("failed, did g_rmdir(%s), but %s is still there", p2, p2);

  g_rmdir (p1);
  if (g_file_test (p1, G_FILE_TEST_EXISTS))
    g_error ("failed, did g_rmdir(%s), but %s is still there", p1, p1);

  f = g_fopen (p1, "we");
  if (f == NULL)
    g_error ("failed, couldn't create file %s", p1);
  fclose (f);

  if (g_mkdir_with_parents (p1, 0666) == 0)
    g_error ("failed, g_mkdir_with_parents(%s) succeeded, even if %s is a file", p1, p1);

  if (g_mkdir_with_parents (p2, 0666) == 0)
    g_error("failed, g_mkdir_with_parents(%s) succeeded, even if %s is a file", p2, p1);

  g_remove (p2);
  g_remove (p1);
  g_remove (p0);

  g_free (p2);
  g_free (p1);
  g_free (p0);
}

/*
 * check_cap_dac_override:
 * @tmpdir: (nullable): A temporary directory in which we can create
 *  and delete files. If %NULL, use the g_get_tmp_dir(), safely.
 *
 * Check whether the current process can bypass DAC permissions.
 *
 * Traditionally, "privileged" processes (those with effective uid 0)
 * could do this (and bypass many other checks), and "unprivileged"
 * processes could not.
 *
 * In Linux, the special powers of euid 0 are divided into many
 * capabilities: see `capabilities(7)`. The one we are interested in
 * here is `CAP_DAC_OVERRIDE`.
 *
 * We do this generically instead of actually looking at the capability
 * bits, so that the right thing will happen on non-Linux Unix
 * implementations, in particular if they have something equivalent to
 * but not identical to Linux permissions.
 *
 * Returns: %TRUE if we have Linux `CAP_DAC_OVERRIDE` or equivalent
 *  privileges
 */
static gboolean
check_cap_dac_override (const char *tmpdir)
{
#ifdef G_OS_UNIX
  gchar *safe_tmpdir = NULL;
  gchar *dac_denies_write;
  gchar *inside;
  gboolean have_cap;

  if (tmpdir == NULL)
    {
      /* It's unsafe to write predictable filenames into g_get_tmp_dir(),
       * because it's usually a shared directory that can be subject to
       * symlink attacks, so use a subdirectory for this check. */
      GError *error = NULL;

      safe_tmpdir = g_dir_make_tmp (NULL, &error);
      g_assert_no_error (error);
      g_clear_error (&error);

      if (safe_tmpdir == NULL)
        return FALSE;

      tmpdir = safe_tmpdir;
    }

  dac_denies_write = g_build_filename (tmpdir, "dac-denies-write", NULL);
  inside = g_build_filename (dac_denies_write, "inside", NULL);

  g_assert_no_errno (mkdir (dac_denies_write, S_IRWXU));
  g_assert_no_errno (g_chmod (dac_denies_write, 0));

  if (mkdir (inside, S_IRWXU) == 0)
    {
      g_test_message ("Looks like we have CAP_DAC_OVERRIDE or equivalent");
      g_assert_no_errno (rmdir (inside));
      have_cap = TRUE;
    }
  else
    {
      int saved_errno = errno;

      g_test_message ("We do not have CAP_DAC_OVERRIDE or equivalent");
      g_assert_cmpint (saved_errno, ==, EACCES);
      have_cap = FALSE;
    }

  g_assert_no_errno (g_chmod (dac_denies_write, S_IRWXU));
  g_assert_no_errno (rmdir (dac_denies_write));

  if (safe_tmpdir != NULL)
    g_assert_no_errno (rmdir (safe_tmpdir));

  g_free (dac_denies_write);
  g_free (inside);
  g_free (safe_tmpdir);
  return have_cap;
#else
  return FALSE;
#endif
}

static void
test_mkdir_with_parents (void)
{
  gchar *cwd, *new_path;
#ifndef G_OS_WIN32
  gboolean can_override_dac = check_cap_dac_override (NULL);
#endif

  g_test_message ("Checking g_mkdir_with_parents() in subdir ./hum/");
  test_mkdir_with_parents_1 ("hum");
  g_remove ("hum");

  g_test_message ("Checking g_mkdir_with_parents() in subdir ./hii///haa/hee/");
  test_mkdir_with_parents_1 ("./hii///haa/hee///");
  g_remove ("hii/haa/hee");
  g_remove ("hii/haa");
  g_remove ("hii");

  cwd = g_get_current_dir ();
  new_path = g_build_filename (cwd, "new", NULL);
  g_assert_cmpint (g_mkdir_with_parents (new_path, 0), ==, 0);
  g_assert_cmpint (g_rmdir (new_path), ==, 0);
  g_free (new_path);
  g_free (cwd);

  g_assert_cmpint (g_mkdir_with_parents ("./test", 0), ==, 0);
  g_assert_cmpint (g_mkdir_with_parents ("./test", 0), ==, 0);
  g_remove ("./test");

#ifndef G_OS_WIN32
  if (can_override_dac)
    {
      g_assert_cmpint (g_mkdir_with_parents ("/usr/b/c", 0), ==, 0);
      g_remove ("/usr/b/c");
      g_remove ("/usr/b");
    }
  else
    {
      g_assert_cmpint (g_mkdir_with_parents ("/usr/b/c", 0), ==, -1);
      /* EPERM or EROFS may be returned if the filesystem as a whole is read-only */
      if (errno != EPERM && errno != EROFS)
        g_assert_cmpint (errno, ==, EACCES);
    }

#endif

  g_assert_cmpint (g_mkdir_with_parents (NULL, 0), ==, -1);
  g_assert_cmpint (errno, ==, EINVAL);
}

/* Reproducer for https://gitlab.gnome.org/GNOME/glib/issues/1852 */
static void
test_mkdir_with_parents_permission (void)
{
#ifdef G_OS_UNIX
  gchar *tmpdir;
  gchar *subdir;
  gchar *subdir2;
  gchar *subdir3;
  GError *error = NULL;
  int result;
  int saved_errno;
  gboolean have_cap_dac_override;

  tmpdir = g_dir_make_tmp ("test-fileutils.XXXXXX", &error);
  g_assert_no_error (error);
  g_assert_nonnull (tmpdir);

  have_cap_dac_override = check_cap_dac_override (tmpdir);

  subdir = g_build_filename (tmpdir, "sub", NULL);
  subdir2 = g_build_filename (subdir, "sub2", NULL);
  subdir3 = g_build_filename (subdir2, "sub3", NULL);
  g_assert_no_errno (g_mkdir (subdir, 0700));
  g_assert_no_errno (g_chmod (subdir, 0));

  if (have_cap_dac_override)
    {
      g_test_skip ("have CAP_DAC_OVERRIDE or equivalent, cannot test");
    }
  else
    {
      result = g_mkdir_with_parents (subdir2, 0700);
      saved_errno = errno;
      g_assert_cmpint (result, ==, -1);
      g_assert_cmpint (saved_errno, ==, EACCES);

      result = g_mkdir_with_parents (subdir3, 0700);
      saved_errno = errno;
      g_assert_cmpint (result, ==, -1);
      g_assert_cmpint (saved_errno, ==, EACCES);

      g_assert_no_errno (g_chmod (subdir, 0700));
    }

  g_assert_no_errno (g_remove (subdir));
  g_assert_no_errno (g_remove (tmpdir));
  g_free (subdir3);
  g_free (subdir2);
  g_free (subdir);
  g_free (tmpdir);
#else
  g_test_skip ("cannot test without Unix-style permissions");
#endif
}

static void
test_format_size_for_display (void)
{
#ifdef G_OS_WIN32
  SetThreadLocale (MAKELCID (MAKELANGID (LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT));
#endif
  /* nobody called setlocale(), so we should get "C" behaviour... */
  check_string (g_format_size_for_display (0), "0 bytes");
  check_string (g_format_size_for_display (1), "1 byte");
  check_string (g_format_size_for_display (2), "2 bytes");
  check_string (g_format_size_for_display (1024), "1.0 KB");
  check_string (g_format_size_for_display (1024 * 1024), "1.0 MB");
  check_string (g_format_size_for_display (1024 * 1024 * 1024), "1.0 GB");
  check_string (g_format_size_for_display (1024ULL * 1024 * 1024 * 1024), "1.0 TB");
  check_string (g_format_size_for_display (1024ULL * 1024 * 1024 * 1024 * 1024), "1.0 PB");
  check_string (g_format_size_for_display (1024ULL * 1024 * 1024 * 1024 * 1024 * 1024), "1.0 EB");

  check_string (g_format_size (0), "0 bytes");
  check_string (g_format_size (1), "1 byte");
  check_string (g_format_size (2), "2 bytes");
  /* '\302\240' is a no-break space, to keep quantity and unit symbol together at line breaks*/
  check_string (g_format_size (1000ULL), "1.0\302\240kB");
  check_string (g_format_size (1000ULL * 1000), "1.0\302\240MB");
  check_string (g_format_size (1000ULL * 1000 * 1000), "1.0\302\240GB");
  check_string (g_format_size (1000ULL * 1000 * 1000 * 1000), "1.0\302\240TB");
  check_string (g_format_size (1000ULL * 1000 * 1000 * 1000 * 1000), "1.0\302\240PB");
  check_string (g_format_size (1000ULL * 1000 * 1000 * 1000 * 1000 * 1000), "1.0\302\240EB");

  check_string (g_format_size_full (0, G_FORMAT_SIZE_IEC_UNITS), "0 bytes");
  check_string (g_format_size_full (0, G_FORMAT_SIZE_IEC_UNITS | G_FORMAT_SIZE_ONLY_VALUE), "0");
  check_string (g_format_size_full (0, G_FORMAT_SIZE_IEC_UNITS | G_FORMAT_SIZE_ONLY_UNIT), "bytes");
  check_string (g_format_size_full (1, G_FORMAT_SIZE_IEC_UNITS), "1 byte");
  check_string (g_format_size_full (1, G_FORMAT_SIZE_IEC_UNITS | G_FORMAT_SIZE_ONLY_VALUE), "1");
  check_string (g_format_size_full (1, G_FORMAT_SIZE_IEC_UNITS | G_FORMAT_SIZE_ONLY_UNIT), "byte");
  check_string (g_format_size_full (2, G_FORMAT_SIZE_IEC_UNITS), "2 bytes");
  check_string (g_format_size_full (2, G_FORMAT_SIZE_IEC_UNITS | G_FORMAT_SIZE_ONLY_VALUE), "2");
  check_string (g_format_size_full (2, G_FORMAT_SIZE_IEC_UNITS | G_FORMAT_SIZE_ONLY_UNIT), "bytes");

  check_string (g_format_size_full (2048ULL, G_FORMAT_SIZE_IEC_UNITS), "2.0\302\240KiB");
  check_string (g_format_size_full (2048ULL * 1024, G_FORMAT_SIZE_IEC_UNITS), "2.0\302\240MiB");
  check_string (g_format_size_full (2048ULL * 1024 * 1024, G_FORMAT_SIZE_IEC_UNITS), "2.0\302\240GiB");
  check_string (g_format_size_full (2048ULL * 1024 * 1024 * 1024, G_FORMAT_SIZE_IEC_UNITS), "2.0\302\240TiB");
  check_string (g_format_size_full (2048ULL * 1024 * 1024 * 1024 * 1024, G_FORMAT_SIZE_IEC_UNITS), "2.0\302\240PiB");
  check_string (g_format_size_full (2048ULL * 1024 * 1024 * 1024 * 1024 * 1024, G_FORMAT_SIZE_IEC_UNITS), "2.0\302\240EiB");

  check_string (g_format_size_full (238472938, G_FORMAT_SIZE_IEC_UNITS), "227.4\302\240MiB");
  check_string (g_format_size_full (238472938, G_FORMAT_SIZE_DEFAULT), "238.5\302\240MB");
  check_string (g_format_size_full (238472938, G_FORMAT_SIZE_LONG_FORMAT), "238.5\302\240MB (238472938 bytes)");
  check_string (g_format_size_full (238472938, G_FORMAT_SIZE_IEC_UNITS | G_FORMAT_SIZE_ONLY_VALUE), "227.4");
  check_string (g_format_size_full (238472938, G_FORMAT_SIZE_IEC_UNITS | G_FORMAT_SIZE_ONLY_UNIT), "MiB");


  check_string (g_format_size_full (0, G_FORMAT_SIZE_BITS), "0 bits");
  check_string (g_format_size_full (0, G_FORMAT_SIZE_BITS | G_FORMAT_SIZE_ONLY_VALUE), "0");
  check_string (g_format_size_full (0, G_FORMAT_SIZE_BITS | G_FORMAT_SIZE_ONLY_UNIT), "bits");
  check_string (g_format_size_full (1, G_FORMAT_SIZE_BITS), "1 bit");
  check_string (g_format_size_full (1, G_FORMAT_SIZE_BITS | G_FORMAT_SIZE_ONLY_VALUE), "1");
  check_string (g_format_size_full (1, G_FORMAT_SIZE_BITS | G_FORMAT_SIZE_ONLY_UNIT), "bit");
  check_string (g_format_size_full (2, G_FORMAT_SIZE_BITS), "2 bits");
  check_string (g_format_size_full (2, G_FORMAT_SIZE_BITS | G_FORMAT_SIZE_ONLY_VALUE), "2");
  check_string (g_format_size_full (2, G_FORMAT_SIZE_BITS | G_FORMAT_SIZE_ONLY_UNIT), "bits");

  check_string (g_format_size_full (2000ULL, G_FORMAT_SIZE_BITS), "2.0\302\240kbit");
  check_string (g_format_size_full (2000ULL * 1000, G_FORMAT_SIZE_BITS), "2.0\302\240Mbit");
  check_string (g_format_size_full (2000ULL * 1000 * 1000, G_FORMAT_SIZE_BITS), "2.0\302\240Gbit");
  check_string (g_format_size_full (2000ULL * 1000 * 1000 * 1000, G_FORMAT_SIZE_BITS), "2.0\302\240Tbit");
  check_string (g_format_size_full (2000ULL * 1000 * 1000 * 1000 * 1000, G_FORMAT_SIZE_BITS), "2.0\302\240Pbit");
  check_string (g_format_size_full (2000ULL * 1000 * 1000 * 1000 * 1000 * 1000, G_FORMAT_SIZE_BITS), "2.0\302\240Ebit");

  check_string (g_format_size_full (238472938, G_FORMAT_SIZE_BITS), "238.5\302\240Mbit");
  check_string (g_format_size_full (238472938, G_FORMAT_SIZE_BITS | G_FORMAT_SIZE_LONG_FORMAT), "238.5\302\240Mbit (238472938 bits)");
  check_string (g_format_size_full (238472938, G_FORMAT_SIZE_BITS | G_FORMAT_SIZE_ONLY_VALUE), "238.5");
  check_string (g_format_size_full (238472938, G_FORMAT_SIZE_BITS | G_FORMAT_SIZE_ONLY_UNIT), "Mbit");


  check_string (g_format_size_full (0, G_FORMAT_SIZE_BITS | G_FORMAT_SIZE_IEC_UNITS), "0 bits");
  check_string (g_format_size_full (0, G_FORMAT_SIZE_BITS | G_FORMAT_SIZE_IEC_UNITS | G_FORMAT_SIZE_ONLY_VALUE), "0");
  check_string (g_format_size_full (0, G_FORMAT_SIZE_BITS | G_FORMAT_SIZE_IEC_UNITS | G_FORMAT_SIZE_ONLY_UNIT), "bits");
  check_string (g_format_size_full (1, G_FORMAT_SIZE_BITS | G_FORMAT_SIZE_IEC_UNITS), "1 bit");
  check_string (g_format_size_full (1, G_FORMAT_SIZE_BITS | G_FORMAT_SIZE_IEC_UNITS | G_FORMAT_SIZE_ONLY_VALUE), "1");
  check_string (g_format_size_full (1, G_FORMAT_SIZE_BITS | G_FORMAT_SIZE_IEC_UNITS | G_FORMAT_SIZE_ONLY_UNIT), "bit");
  check_string (g_format_size_full (2, G_FORMAT_SIZE_BITS | G_FORMAT_SIZE_IEC_UNITS), "2 bits");
  check_string (g_format_size_full (2, G_FORMAT_SIZE_BITS | G_FORMAT_SIZE_IEC_UNITS | G_FORMAT_SIZE_ONLY_VALUE), "2");
  check_string (g_format_size_full (2, G_FORMAT_SIZE_BITS | G_FORMAT_SIZE_IEC_UNITS | G_FORMAT_SIZE_ONLY_UNIT), "bits");

  check_string (g_format_size_full (2048ULL, G_FORMAT_SIZE_BITS | G_FORMAT_SIZE_IEC_UNITS), "2.0\302\240Kibit");
  check_string (g_format_size_full (2048ULL * 1024, G_FORMAT_SIZE_BITS | G_FORMAT_SIZE_IEC_UNITS), "2.0\302\240Mibit");
  check_string (g_format_size_full (2048ULL * 1024 * 1024, G_FORMAT_SIZE_BITS | G_FORMAT_SIZE_IEC_UNITS), "2.0\302\240Gibit");
  check_string (g_format_size_full (2048ULL * 1024 * 1024 * 1024, G_FORMAT_SIZE_BITS | G_FORMAT_SIZE_IEC_UNITS), "2.0\302\240Tibit");
  check_string (g_format_size_full (2048ULL * 1024 * 1024 * 1024 * 1024, G_FORMAT_SIZE_BITS | G_FORMAT_SIZE_IEC_UNITS), "2.0\302\240Pibit");
  check_string (g_format_size_full (2048ULL * 1024 * 1024 * 1024 * 1024 * 1024, G_FORMAT_SIZE_BITS | G_FORMAT_SIZE_IEC_UNITS), "2.0\302\240Eibit");

  check_string (g_format_size_full (238472938, G_FORMAT_SIZE_BITS | G_FORMAT_SIZE_IEC_UNITS), "227.4\302\240Mibit");
  check_string (g_format_size_full (238472938, G_FORMAT_SIZE_BITS | G_FORMAT_SIZE_IEC_UNITS | G_FORMAT_SIZE_LONG_FORMAT), "227.4\302\240Mibit (238472938 bits)");
  check_string (g_format_size_full (238472938, G_FORMAT_SIZE_BITS | G_FORMAT_SIZE_IEC_UNITS | G_FORMAT_SIZE_ONLY_VALUE), "227.4");
  check_string (g_format_size_full (238472938, G_FORMAT_SIZE_BITS | G_FORMAT_SIZE_IEC_UNITS | G_FORMAT_SIZE_ONLY_UNIT), "Mibit");
}

static void
test_file_errors (void)
{
  g_assert_cmpint (g_file_error_from_errno (-1), ==, G_FILE_ERROR_FAILED);

#ifdef EEXIST
  g_assert_cmpint (g_file_error_from_errno (EEXIST), ==, G_FILE_ERROR_EXIST);
#endif
#ifdef EISDIR
  g_assert_cmpint (g_file_error_from_errno (EISDIR), ==, G_FILE_ERROR_ISDIR);
#endif
#ifdef EACCES
  g_assert_cmpint (g_file_error_from_errno (EACCES), ==, G_FILE_ERROR_ACCES);
#endif
#ifdef ENAMETOOLONG
  g_assert_cmpint (g_file_error_from_errno (ENAMETOOLONG), ==, G_FILE_ERROR_NAMETOOLONG);
#endif
#ifdef ENOENT
  g_assert_cmpint (g_file_error_from_errno (ENOENT), ==, G_FILE_ERROR_NOENT);
#endif
#ifdef ENOTDIR
  g_assert_cmpint (g_file_error_from_errno (ENOTDIR), ==, G_FILE_ERROR_NOTDIR);
#endif
#ifdef ENXIO
  g_assert_cmpint (g_file_error_from_errno (ENXIO), ==, G_FILE_ERROR_NXIO);
#endif
#ifdef ENODEV
  g_assert_cmpint (g_file_error_from_errno (ENODEV), ==, G_FILE_ERROR_NODEV);
#endif
#ifdef EROFS
  g_assert_cmpint (g_file_error_from_errno (EROFS), ==, G_FILE_ERROR_ROFS);
#endif
#ifdef ETXTBSY
  g_assert_cmpint (g_file_error_from_errno (ETXTBSY), ==, G_FILE_ERROR_TXTBSY);
#endif
#ifdef EFAULT
  g_assert_cmpint (g_file_error_from_errno (EFAULT), ==, G_FILE_ERROR_FAULT);
#endif
#ifdef ELOOP
  g_assert_cmpint (g_file_error_from_errno (ELOOP), ==, G_FILE_ERROR_LOOP);
#endif
#ifdef ENOSPC
  g_assert_cmpint (g_file_error_from_errno (ENOSPC), ==, G_FILE_ERROR_NOSPC);
#endif
#ifdef ENOMEM
  g_assert_cmpint (g_file_error_from_errno (ENOMEM), ==, G_FILE_ERROR_NOMEM);
#endif
#ifdef EMFILE
  g_assert_cmpint (g_file_error_from_errno (EMFILE), ==, G_FILE_ERROR_MFILE);
#endif
#ifdef ENFILE
  g_assert_cmpint (g_file_error_from_errno (ENFILE), ==, G_FILE_ERROR_NFILE);
#endif
#ifdef EBADF
  g_assert_cmpint (g_file_error_from_errno (EBADF), ==, G_FILE_ERROR_BADF);
#endif
#ifdef EINVAL
  g_assert_cmpint (g_file_error_from_errno (EINVAL), ==, G_FILE_ERROR_INVAL);
#endif
#ifdef EPIPE
  g_assert_cmpint (g_file_error_from_errno (EPIPE), ==, G_FILE_ERROR_PIPE);
#endif
#ifdef EAGAIN
  g_assert_cmpint (g_file_error_from_errno (EAGAIN), ==, G_FILE_ERROR_AGAIN);
#endif
#ifdef EINTR
  g_assert_cmpint (g_file_error_from_errno (EINTR), ==, G_FILE_ERROR_INTR);
#endif
#ifdef EIO
  g_assert_cmpint (g_file_error_from_errno (EIO), ==, G_FILE_ERROR_IO);
#endif
#ifdef EPERM
  g_assert_cmpint (g_file_error_from_errno (EPERM), ==, G_FILE_ERROR_PERM);
#endif
#ifdef ENOSYS
  g_assert_cmpint (g_file_error_from_errno (ENOSYS), ==, G_FILE_ERROR_NOSYS);
#endif
}

static void
test_basename (void)
{
  const gchar *path = "/path/to/a/file/deep/down.sh";
  const gchar *b;

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_null (g_basename (NULL));
      g_test_assert_expected_messages ();
    }

  b = g_basename (path);

  g_assert_cmpstr (b, ==, "down.sh");
}

static void
test_get_basename (void)
{
  gchar *b;

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_null (g_path_get_basename (NULL));
      g_test_assert_expected_messages ();
    }

  b = g_path_get_basename ("");
  g_assert_cmpstr (b, ==, ".");
  g_free (b);

  b = g_path_get_basename ("///");
  g_assert_cmpstr (b, ==, G_DIR_SEPARATOR_S);
  g_free (b);

  b = g_path_get_basename ("/a/b/c/d");
  g_assert_cmpstr (b, ==, "d");
  g_free (b);
}

static void
test_dirname (void)
{
  gsize i;
  struct {
    const gchar *filename;
    const gchar *dirname;
  } dirname_checks[] = {
    { "/", "/" },
    { "////", "/" },
    { ".////", "." },
    { ".", "." },
    { "..", "." },
    { "../", ".." },
    { "..////", ".." },
    { "", "." },
    { "a/b", "a" },
    { "a/b/", "a/b" },
    { "c///", "c" },
    { "/a/b", "/a" },
    { "/a/b/", "/a/b" },
#ifdef G_OS_WIN32
    { "\\", "\\" },
    { ".\\\\\\\\", "." },
    { ".\\/\\/", "." },
    { ".", "." },
    { "..", "." },
    { "..\\", ".." },
    { "..\\\\\\\\", ".." },
    { "..\\//\\", ".." },
    { "", "." },
    { "a\\b", "a" },
    { "a\\b\\", "a\\b" },
    { "\\a\\b", "\\a" },
    { "\\a\\b\\", "\\a\\b" },
    { "c\\\\\\", "c" },
    { "c/\\\\", "c" },
    { "a:", "a:." },
    { "a:foo", "a:." },
    { "a:foo\\bar", "a:foo" },
    { "a:/foo", "a:/" },
    { "a:/foo/bar", "a:/foo" },
    { "a:/", "a:/" },
    { "a://", "a:/" },
    { "a:\\foo", "a:\\" },
    { "a:\\", "a:\\" },
    { "a:\\\\", "a:\\" },
    { "a:\\/", "a:\\" },
#endif
  };

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_null (g_path_get_dirname (NULL));
      g_test_assert_expected_messages ();
    }

  for (i = 0; i < G_N_ELEMENTS (dirname_checks); i++)
    {
      gchar *dirname;

      dirname = g_path_get_dirname (dirname_checks[i].filename);
      g_assert_cmpstr (dirname, ==, dirname_checks[i].dirname);
      g_free (dirname);
    }
}

static void
test_dir_make_tmp (void)
{
  gchar *name;
  GError *error = NULL;
  gint ret;

  name = g_dir_make_tmp ("testXXXXXXtest", &error);
  g_assert_no_error (error);
  g_assert_true (g_file_test (name, G_FILE_TEST_IS_DIR));
  g_assert_true (g_str_has_prefix (name, g_getenv ("G_TEST_TMPDIR")));
  ret = g_rmdir (name);
  g_assert_cmpint (ret, ==, 0);
  g_free (name);

  name = g_dir_make_tmp (NULL, &error);
  g_assert_no_error (error);
  g_assert_true (g_file_test (name, G_FILE_TEST_IS_DIR));
  g_assert_true (g_str_has_prefix (name, g_getenv ("G_TEST_TMPDIR")));
  ret = g_rmdir (name);
  g_assert_cmpint (ret, ==, 0);
  g_free (name);

  name = g_dir_make_tmp ("test/XXXXXX", &error);
  g_assert_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED);
  g_clear_error (&error);
  g_assert_null (name);

  name = g_dir_make_tmp ("XXXXxX", &error);
  g_assert_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED);
  g_clear_error (&error);
  g_assert_null (name);
}

static void
test_file_open_tmp (void)
{
  gchar *name = NULL;
  GError *error = NULL;
  gint fd;

  fd = g_file_open_tmp ("testXXXXXXtest", &name, &error);
  g_assert_cmpint (fd, !=, -1);
  g_assert_no_error (error);
  g_assert_nonnull (name);
  g_assert_true (g_str_has_prefix (name, g_getenv ("G_TEST_TMPDIR")));
  unlink (name);
  g_free (name);
  close (fd);

  fd = g_file_open_tmp (NULL, &name, &error);
  g_assert_cmpint (fd, !=, -1);
  g_assert_no_error (error);
  g_assert_nonnull (name);
  g_assert_true (g_str_has_prefix (name, g_getenv ("G_TEST_TMPDIR")));
  g_unlink (name);
  g_free (name);
  close (fd);

  name = NULL;
  fd = g_file_open_tmp ("test/XXXXXX", &name, &error);
  g_assert_cmpint (fd, ==, -1);
  g_assert_null (name);
  g_assert_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED);
  g_clear_error (&error);

  fd = g_file_open_tmp ("XXXXxX", &name, &error);
  g_assert_cmpint (fd, ==, -1);
  g_assert_null (name);
  g_assert_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED);
  g_clear_error (&error);

  error = NULL;
  name = NULL;
  fd = g_file_open_tmp ("zap" G_DIR_SEPARATOR_S "barXXXXXX", &name, &error);
  g_assert_cmpint (fd, ==, -1);

  g_clear_error (&error);
  g_free (name);

#ifdef G_OS_WIN32
  name = NULL;
  fd = g_file_open_tmp ("zap/barXXXXXX", &name, &error);
  g_assert_cmpint (fd, ==, -1);

  g_clear_error (&error);
  g_free (name);
#endif

  name = NULL;
  fd = g_file_open_tmp ("zapXXXXXX", &name, &error);
  g_assert_cmpint (fd, !=, -1);

  close (fd);
  g_clear_error (&error);
  remove (name);
  g_free (name);

  name = NULL;
  fd = g_file_open_tmp (NULL, &name, &error);
  g_assert_cmpint (fd, !=, -1);

  close (fd);
  g_clear_error (&error);
  remove (name);
  g_free (name);
}

static void
test_mkstemp (void)
{
  gint fd;
  gint result;
  gchar *name;
  char chars[62];
  char template[32];
  const char hello[] = "Hello, World";
  const gsize hellolen = sizeof (hello) - 1;

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_cmpint (g_mkstemp (NULL), ==, -1);
      g_test_assert_expected_messages ();
    }

  /* Expect to fail if no 'XXXXXX' is given */
  name = g_strdup ("test");
  g_assert_cmpint (g_mkstemp (name), ==, -1);
  g_free (name);

  /* Test normal case */
  name = g_build_filename (g_get_tmp_dir (), "testXXXXXXtest", NULL),
  fd = g_mkstemp (name);
  g_assert_cmpint (fd, !=, -1);
  g_assert_null (strstr (name, "XXXXXX"));
  unlink (name);
  close (fd);
  g_free (name);

  /* g_mkstemp() must not work if template doesn't contain XXXXXX */
  strcpy (template, "foobar");
  g_assert_cmpint (g_mkstemp (template), ==, -1);

  /* g_mkstemp() must not work if template doesn't contain six X */
  strcpy (template, "foobarXXX");
  g_assert_cmpint (g_mkstemp (template), ==, -1);

  name = g_build_filename (g_get_tmp_dir (), "fooXXXXXX", NULL);
  fd = g_mkstemp (name);
  g_assert_cmpint (fd, !=, -1);
  result = write (fd, hello, hellolen);
  g_assert_cmpint (result, !=, -1);
  g_assert_cmpint (result, ==, hellolen);

  lseek (fd, 0, 0);
  result = read (fd, chars, sizeof (chars));
  g_assert_cmpint (result, !=, -1);
  g_assert_cmpint (result, ==, hellolen);

  chars[result] = '\0';
  g_assert_cmpstr (chars, ==, hello);

  close (fd);
  remove (name);
  g_free (name);

  /* Check that it works for "fooXXXXXX.pdf" */
  name = g_build_filename (g_get_tmp_dir (), "fooXXXXXX.pdf", NULL);
  fd = g_mkstemp (name);
  g_assert_cmpint (fd, !=, -1);
  close (fd);
  remove (name);
  g_free (name);
}

static void
test_mkdtemp (void)
{
  gint fd;
  gchar *ret;
  gchar *name, *name2;
  char template[32];

  name = g_build_filename (g_get_tmp_dir (), "testXXXXXXtest", NULL),
  ret = g_mkdtemp (name);
  g_assert_true (ret == name);
  g_assert_null (strstr (name, "XXXXXX"));
  g_rmdir (name);
  g_free (name);

  name = g_strdup ("testYYYYYYtest"),
  ret = g_mkdtemp (name);
  g_assert_null (ret);
  g_free (name);

  strcpy (template, "foodir");
  g_assert_null (g_mkdtemp (template));

  strcpy (template, "foodir");
  g_assert_null (g_mkdtemp (template));

  name = g_build_filename (g_get_tmp_dir (), "fooXXXXXX", NULL);
  ret = g_mkdtemp (name);
  g_assert_nonnull (ret);
  g_assert_true (ret == name);
  g_assert_false (g_file_test (name, G_FILE_TEST_IS_REGULAR));
  g_assert_true (g_file_test (name, G_FILE_TEST_IS_DIR));

  name2 = g_build_filename (name, "abc", NULL);
  fd = g_open (name2, O_WRONLY | O_CREAT, 0600);
  g_assert_cmpint (fd, !=, -1);
  close (fd);
  g_assert_true (g_file_test (name2, G_FILE_TEST_IS_REGULAR));
  g_assert_cmpint (g_unlink (name2), !=, -1);
  g_free (name2);

  g_assert_cmpint (g_rmdir (name), !=, -1);
  g_free (name);

  name = g_build_filename (g_get_tmp_dir (), "fooXXXXXX.dir", NULL);
  g_assert_nonnull (g_mkdtemp (name));
  g_assert_true (g_file_test (name, G_FILE_TEST_IS_DIR));
  g_rmdir (name);
  g_free (name);
}

static void
test_get_contents (void)
{
  FILE *f;
  gsize len;
  gchar *contents;
  GError *error = NULL;
  const gchar *text = "abcdefghijklmnopqrstuvwxyz";
  char *filename = g_build_filename (g_get_tmp_dir (), "file-test-get-contents", NULL);
  gsize bytes_written;

  f = g_fopen (filename, "we");
  bytes_written = fwrite (text, 1, strlen (text), f);
  g_assert_cmpint (bytes_written, ==, strlen (text));
  fclose (f);

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_file_get_contents (NULL, &contents, &len, &error));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_file_get_contents (filename, NULL, &len, &error));
      g_test_assert_expected_messages ();
    }

  g_assert_true (g_file_test (filename, G_FILE_TEST_IS_REGULAR));

  g_assert_true (g_file_get_contents (filename, &contents, &len, &error));
  g_assert_cmpstr (text, ==, contents);
  g_assert_no_error (error);

  g_free (contents);
  g_remove (filename);
  g_free (filename);
}

static gboolean
resize_file (const gchar *filename,
             gint64       size)
{
  int fd;
  int retval;

  fd = g_open (filename, O_CREAT | O_RDWR | O_TRUNC, 0666);
  g_assert_cmpint (fd, >=, 0);

#ifdef G_OS_WIN32
  retval = _chsize_s (fd, size);
#elif HAVE_FTRUNCATE64
  retval = ftruncate64 (fd, size);
#else
  errno = ENOSYS;
  retval = -1;
#endif
  if (retval != 0)
    {
      g_test_message ("Error trying to resize file (%s)", strerror (errno));
      close (fd);
      return FALSE;
    }

  close (fd);
  return TRUE;
}

static gboolean
is_error_in_list (GFileError       error_code,
                  const GFileError ok_list[],
                  size_t           ok_count)
{
  for (size_t i = 0; i < ok_count; i++)
    {
      if (ok_list[i] == error_code)
        return TRUE;
    }
  return FALSE;
}

static void
get_largefile_check_len (const gchar      *filename,
                         gint64            large_len,
                         const GFileError  ok_list[],
                         size_t            ok_count)
{
  gboolean get_ok;
  gsize len;
  gchar *contents;
  GError *error = NULL;

  get_ok = g_file_get_contents (filename, &contents, &len, &error);
  if (get_ok)
    {
      g_assert_cmpint ((gint64) len, ==, large_len);
      g_free (contents);
    }
  else
    {
      g_assert_cmpint (error->domain, ==, G_FILE_ERROR);
      if (is_error_in_list ((GFileError)error->code, ok_list, ok_count))
        {
          g_test_message ("Error reading file of size 0x%" G_GINT64_MODIFIER "x, but with acceptable error type (%s)", large_len, error->message);
        }
      else
        {
          /* fail for other errors */
          g_assert_no_error (error);
        }
      g_clear_error (&error);
    }
}

static void
test_get_contents_largefile (void)
{
  if (!g_test_slow ())
    {
      g_test_skip ("Skipping slow largefile test");
      return;
    }

  const gchar *filename = "file-test-get-contents-large";
  gint64 large_len;

  /* error OK if couldn't allocate large buffer, or if file is too large */
  const GFileError too_large_errors[] = { G_FILE_ERROR_NOMEM, G_FILE_ERROR_FAILED };
  /* error OK if couldn't allocate large buffer */
  const GFileError nomem_errors[] = { G_FILE_ERROR_NOMEM };

  /* OK to fail to read this, but don't silently under-read */
  large_len = (G_GINT64_CONSTANT (1) << 32) + 16;
  if (!resize_file (filename, large_len))
    goto failed_resize;
  get_largefile_check_len (filename, large_len, too_large_errors, G_N_ELEMENTS (too_large_errors));

  /* OK to fail to read this size, but don't silently under-read */
  large_len = (G_GINT64_CONSTANT (1) << 32) - 1;
  if (!resize_file (filename, large_len))
    goto failed_resize;
  get_largefile_check_len (filename, large_len, too_large_errors, G_N_ELEMENTS (too_large_errors));

  /* OK to fail memory allocation, but don't otherwise fail this size */
  large_len = (G_GINT64_CONSTANT (1) << 31) - 1;
  if (!resize_file (filename, large_len))
    goto failed_resize;
  get_largefile_check_len (filename, large_len, nomem_errors, G_N_ELEMENTS (nomem_errors));

  g_remove (filename);
  return;

failed_resize:
  g_test_incomplete ("Failed to resize large file, unable to complete large file tests.");
  g_remove (filename);
}

static void
test_file_test (void)
{
  GError *error = NULL;
  gboolean result;
  gchar *name;
  gint fd;

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      result = g_file_test (NULL, G_FILE_TEST_EXISTS);
      g_assert_false (result);
      g_test_assert_expected_messages ();
    }

  fd = g_file_open_tmp (NULL, &name, &error);
  g_assert_no_error (error);
  g_assert_cmpint (write (fd, "a", 1), ==, 1);
  g_assert_cmpint (g_fsync (fd), ==, 0);
  close (fd);

#ifndef G_OS_WIN32
  result = g_file_test (name, G_FILE_TEST_IS_SYMLINK);
  g_assert_false (result);

  g_assert_no_errno (symlink (name, "symlink"));
  result = g_file_test ("symlink", G_FILE_TEST_IS_SYMLINK);
  g_assert_true (result);
  unlink ("symlink");
#endif

  /* Cleaning */
  g_remove (name);
  g_free (name);
}

static void
test_set_contents (void)
{
  GError *error = NULL;
  gint fd;
  gchar *name;
  gchar *buf;
  gsize len;
  gboolean ret;

  fd = g_file_open_tmp (NULL, &name, &error);
  g_assert_no_error (error);
  g_assert_cmpint (write (fd, "a", 1), ==, 1);
  g_assert_cmpint (g_fsync (fd), ==, 0);
  close (fd);

  ret = g_file_get_contents (name, &buf, &len, &error);
  g_assert_true (ret);
  g_assert_no_error (error);
  g_assert_cmpstr (buf, ==, "a");
  g_free (buf);

  ret = g_file_set_contents (name, "b", 1, &error);
  g_assert_true (ret);
  g_assert_no_error (error);

  ret = g_file_get_contents (name, &buf, &len, &error);
  g_assert_true (ret);
  g_assert_no_error (error);
  g_assert_cmpstr (buf, ==, "b");
  g_free (buf);

  g_remove (name);
  g_free (name);
}

static void
test_set_contents_full (void)
{
  GFileSetContentsFlags flags_mask =
      G_FILE_SET_CONTENTS_ONLY_EXISTING |
      G_FILE_SET_CONTENTS_DURABLE |
      G_FILE_SET_CONTENTS_CONSISTENT;
  gint flags;
  const struct
    {
      enum
        {
          EXISTING_FILE_NONE,
          EXISTING_FILE_REGULAR,
#ifndef G_OS_WIN32
          EXISTING_FILE_SYMLINK,
#endif
          EXISTING_FILE_DIRECTORY,
        }
      existing_file;
      /* only relevant if @existing_file is
       * %EXISTING_FILE_NONE or %EXISTING_FILE_REGULAR */
      int mode;
      gboolean use_strlen;

      gboolean expected_success;
      gint expected_error;
      gboolean use_non_ascii_filename;
    }
  tests[] =
    {
      { EXISTING_FILE_NONE, 0644, FALSE, TRUE, 0, FALSE },
      { EXISTING_FILE_NONE, 0644, TRUE, TRUE, 0, FALSE },
      { EXISTING_FILE_NONE, 0600, FALSE, TRUE, 0, FALSE },
      // Assume umask is 022, ensures that we preserve perms with, eg. 077
      { EXISTING_FILE_REGULAR, 0666, FALSE, TRUE, 0, FALSE },
      { EXISTING_FILE_REGULAR, 0644, FALSE, TRUE, 0, FALSE },
      { EXISTING_FILE_REGULAR, 0666, FALSE, TRUE, 0, TRUE },
      { EXISTING_FILE_REGULAR, 0644, FALSE, TRUE, 0, TRUE },
#ifndef G_OS_WIN32
      { EXISTING_FILE_SYMLINK, 0644, FALSE, TRUE, 0, FALSE },
      { EXISTING_FILE_DIRECTORY, 0644, FALSE, FALSE, G_FILE_ERROR_ISDIR, FALSE },
      { EXISTING_FILE_SYMLINK, 0644, FALSE, TRUE, 0, TRUE },
      { EXISTING_FILE_DIRECTORY, 0644, FALSE, FALSE, G_FILE_ERROR_ISDIR, TRUE },
#else
      /* on win32, _wopen returns EACCES if path is a directory */
      { EXISTING_FILE_DIRECTORY, 0644, FALSE, FALSE, G_FILE_ERROR_ACCES, FALSE },
      { EXISTING_FILE_DIRECTORY, 0644, FALSE, FALSE, G_FILE_ERROR_ACCES, TRUE },
#endif
    };
  gsize i;

  g_test_summary ("Test g_file_set_contents_full() with various flags");

  for (flags = 0; flags < (gint) flags_mask; flags++)
    {
      for (i = 0; i < G_N_ELEMENTS (tests); i++)
        {
          GError *error = NULL;
          gchar *file_name = NULL, *link_name = NULL, *dir_name = NULL;
          const gchar *set_contents_name;
          const gchar *tmpl = NULL;
          gchar *buf = NULL;
          gsize len;
          gboolean ret;
          GStatBuf statbuf;
          const gchar *original_contents = "a string which is longer than what will be overwritten on it";
          size_t original_contents_len = strlen (original_contents);

          g_test_message ("Flags %d and test %" G_GSIZE_FORMAT, flags, i);

          switch (tests[i].existing_file)
            {
            case EXISTING_FILE_REGULAR:
#ifndef G_OS_WIN32
            case EXISTING_FILE_SYMLINK:
#endif
              {
                gint fd;

                if (tests[i].use_non_ascii_filename)
                  {
                    tmpl = "ñön-äşçïï-XXXXXX";  
                  }
                fd = g_file_open_tmp (tmpl, &file_name, &error);
                g_assert_no_error (error);
                g_assert_cmpint (write (fd, original_contents, original_contents_len), ==, original_contents_len);
                g_assert_no_errno (g_fsync (fd));
                close (fd);

                g_assert_no_errno (g_chmod (file_name, tests[i].mode));

#ifndef G_OS_WIN32
                /* Pass an existing symlink to g_file_set_contents_full() to see
                 * what it does. */
                if (tests[i].existing_file == EXISTING_FILE_SYMLINK)
                  {
                    link_name = g_strconcat (file_name, ".link", NULL);
                    g_assert_no_errno (symlink (file_name, link_name));

                    set_contents_name = link_name;
                  }
                else
#endif  /* !G_OS_WIN32 */
                  {
                    set_contents_name = file_name;
                  }
                break;
              }
            case EXISTING_FILE_DIRECTORY:
              {
                if (tests[i].use_non_ascii_filename)
                  {
                    tmpl = "glib-fileutils-set-contents-full-non-äşçïï-XXXXXX";  
                  }
                else 
                  {
                    tmpl = "glib-fileutils-set-contents-full-XXXXXX";
                  }
                dir_name = g_dir_make_tmp (tmpl, &error);
                g_assert_no_error (error);

                set_contents_name = dir_name;
                break;
              }
            case EXISTING_FILE_NONE:
              {
                file_name = g_build_filename (g_get_tmp_dir (), "glib-file-set-contents-full-test", NULL);
                g_remove (file_name);
                g_assert_false (g_file_test (file_name, G_FILE_TEST_EXISTS));

                set_contents_name = file_name;
                break;
              }
            default:
              {
                g_assert_not_reached ();
              }
            }

          /* Set the file contents */
          ret = g_file_set_contents_full (set_contents_name, "b",
                                          tests[i].use_strlen ? -1 : 1,
                                          flags, tests[i].mode, &error);

          if (!tests[i].expected_success)
            {
              g_assert_error (error, G_FILE_ERROR, tests[i].expected_error);
              g_assert_false (ret);
              g_clear_error (&error);
            }
          else
            {
              g_assert_no_error (error);
              g_assert_true (ret);

              /* Check the contents and mode were set correctly. The mode isn’t
               * changed on existing files. */
              ret = g_file_get_contents (set_contents_name, &buf, &len, &error);
              g_assert_no_error (error);
              g_assert_true (ret);
              g_assert_cmpstr (buf, ==, "b");
              g_assert_cmpuint (len, ==, 1);
              g_free (buf);

              g_assert_no_errno (g_lstat (set_contents_name, &statbuf));

              if (tests[i].existing_file & (EXISTING_FILE_NONE | EXISTING_FILE_REGULAR))
                {
                  int mode = statbuf.st_mode & ~S_IFMT;
                  int new_mode = tests[i].mode;
#ifdef G_OS_WIN32
                  /* on windows, group and others perms handling is different */
                  /* only check the rwx user permissions */
                  mode &= (_S_IREAD|_S_IWRITE|_S_IEXEC);
                  new_mode &= (_S_IREAD|_S_IWRITE|_S_IEXEC);
#endif
                  g_assert_cmpint (mode, ==, new_mode);
                }

#ifndef G_OS_WIN32
              if (tests[i].existing_file == EXISTING_FILE_SYMLINK)
                {
                  gchar *target_contents = NULL;

                  /* If the @set_contents_name was a symlink, it should now be a
                   * regular file, and the file it pointed to should not have
                   * changed. */
                  g_assert_cmpint (statbuf.st_mode & S_IFMT, ==, S_IFREG);

                  g_file_get_contents (file_name, &target_contents, NULL, &error);
                  g_assert_no_error (error);
                  g_assert_cmpstr (target_contents, ==, original_contents);

                  g_free (target_contents);
                }
#endif  /* !G_OS_WIN32 */
            }

          if (dir_name != NULL)
            g_rmdir (dir_name);
          if (link_name != NULL)
            g_remove (link_name);
          if (file_name != NULL)
            g_remove (file_name);

          g_free (dir_name);
          g_free (link_name);
          g_free (file_name);
        }
    }
}

static void
test_set_contents_full_read_only_file (void)
{
  gint fd;
  GError *error = NULL;
  gchar *file_name = NULL;
  gboolean ret;
  gboolean can_override_dac = check_cap_dac_override (NULL);

  g_test_summary ("Test g_file_set_contents_full() on a read-only file");

  /* Can’t test this with different #GFileSetContentsFlags as they all have
   * different behaviours wrt replacing the file while noticing/ignoring the
   * existing file permissions. */
  fd = g_file_open_tmp (NULL, &file_name, &error);
  g_assert_no_error (error);
  g_assert_cmpint (write (fd, "a", 1), ==, 1);
  g_assert_no_errno (g_fsync (fd));
  close (fd);
  g_assert_no_errno (g_chmod (file_name, 0400)); /* S_IREAD */

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      ret = g_file_set_contents_full (NULL, "b", 1,
                                      G_FILE_SET_CONTENTS_NONE, 0644, &error);
      g_assert_false (ret);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      ret = g_file_set_contents_full (file_name, NULL, 1,
                                      G_FILE_SET_CONTENTS_NONE, 0644, &error);
      g_assert_false (ret);
      g_test_assert_expected_messages ();
    }

  /* Set the file contents */
  ret = g_file_set_contents_full (file_name, "b", 1, G_FILE_SET_CONTENTS_NONE, 0644, &error);

  if (can_override_dac)
    {
      g_assert_no_error (error);
      g_assert_true (ret);
    }
  else
    {
      g_assert_error (error, G_FILE_ERROR, G_FILE_ERROR_ACCES);
      g_assert_false (ret);
    }

  g_clear_error (&error);

  g_remove (file_name);

  g_free (file_name);
}

static void
test_set_contents_full_read_only_directory (void)
{
#ifndef G_OS_WIN32
/* windows mostly ignores read-only flagged directories, chmod doesn't work */
  GFileSetContentsFlags flags_mask =
      G_FILE_SET_CONTENTS_ONLY_EXISTING |
      G_FILE_SET_CONTENTS_DURABLE |
      G_FILE_SET_CONTENTS_CONSISTENT;
  gint flags;

  g_test_summary ("Test g_file_set_contents_full() on a file in a read-only directory");

  for (flags = 0; flags < (gint) flags_mask; flags++)
    {
      gint fd;
      GError *error = NULL;
      gchar *dir_name = NULL;
      gchar *file_name = NULL;
      gboolean ret;
      gboolean can_override_dac;

      g_test_message ("Flags %d", flags);

      dir_name = g_dir_make_tmp ("glib-file-set-contents-full-rodir-XXXXXX", &error);
      g_assert_no_error (error);
      can_override_dac = check_cap_dac_override (dir_name);

      file_name = g_build_filename (dir_name, "file", NULL);
      fd = g_open (file_name, O_CREAT | O_RDWR, 0644);
      g_assert_cmpint (fd, >=, 0);
      g_assert_cmpint (write (fd, "a", 1), ==, 1);
      g_assert_no_errno (g_fsync (fd));
      close (fd);

      g_assert_no_errno (g_chmod (dir_name, 0));

      /* Set the file contents */
      ret = g_file_set_contents_full (file_name, "b", 1, flags, 0644, &error);

      if (can_override_dac)
        {
          g_assert_no_error (error);
          g_assert_true (ret);
        }
      else
        {
          g_assert_error (error, G_FILE_ERROR, G_FILE_ERROR_ACCES);
          g_assert_false (ret);
        }

      g_clear_error (&error);
      g_remove (file_name);
      g_unlink (dir_name);

      g_free (file_name);
      g_free (dir_name);
    }
#else
  g_test_skip ("Windows doesn’t support read-only directories in the same way as Unix");
#endif
}

static void
test_read_link (void)
{
#ifdef HAVE_READLINK
#ifdef G_OS_UNIX
  int ret;
  FILE *file;
  gchar *cwd;
  gchar *data;
  gchar *newpath;
  gchar *badpath;
  gchar *path;
  GError *error = NULL;
  const gchar *oldpath;
  const gchar *filename = "file-test-data";
  const gchar *link1 = "file-test-link1";
  const gchar *link2 = "file-test-link2";
  const gchar *link3 = "file-test-link3";

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      newpath = g_file_read_link (NULL, &error);
      g_assert_null (newpath);
      g_test_assert_expected_messages ();
    }

  cwd = g_get_current_dir ();

  oldpath = g_test_get_filename (G_TEST_DIST, "4096-random-bytes", NULL);
  newpath = g_build_filename (cwd, "page-of-junk", NULL);
  badpath = g_build_filename (cwd, "4097-random-bytes", NULL);
  remove (newpath);
  ret = symlink (oldpath, newpath);
  g_assert_cmpint (ret, ==, 0);
  path = g_file_read_link (newpath, &error);
  g_assert_no_error (error);
  g_assert_cmpstr (path, ==, oldpath);
  g_free (path);

  remove (newpath);
  ret = symlink (badpath, newpath);
  g_assert_cmpint (ret, ==, 0);
  path = g_file_read_link (newpath, &error);
  g_assert_no_error (error);
  g_assert_cmpstr (path, ==, badpath);
  g_free (path);

  path = g_file_read_link (oldpath, &error);
  g_assert_error (error, G_FILE_ERROR, G_FILE_ERROR_INVAL);
  g_assert_null (path);
  g_error_free (error);

  remove (newpath);
  g_free (cwd);
  g_free (newpath);
  g_free (badpath);

  file = g_fopen (filename, "we");
  g_assert_nonnull (file);
  fclose (file);

  g_assert_no_errno (symlink (filename, link1));
  g_assert_no_errno (symlink (link1, link2));

  error = NULL;
  data = g_file_read_link (link1, &error);
  g_assert_nonnull (data);
  g_assert_cmpstr (data, ==, filename);
  g_assert_no_error (error);
  g_free (data);

  error = NULL;
  data = g_file_read_link (link2, &error);
  g_assert_nonnull (data);
  g_assert_cmpstr (data, ==, link1);
  g_assert_no_error (error);
  g_free (data);

  error = NULL;
  data = g_file_read_link (link3, &error);
  g_assert_null (data);
  g_assert_error (error, G_FILE_ERROR, G_FILE_ERROR_NOENT);
  g_error_free (error);

  error = NULL;
  data = g_file_read_link (filename, &error);
  g_assert_null (data);
  g_assert_error (error, G_FILE_ERROR, G_FILE_ERROR_INVAL);
  g_error_free (error);

  remove (filename);
  remove (link1);
  remove (link2);
#endif
#else
  g_test_skip ("Symbolic links not supported");
#endif
}

static void
test_stdio_wrappers (void)
{
  GStatBuf buf;
  gchar *cwd, *path;
  gint ret;
  struct utimbuf ut;
  GError *error = NULL;
  GStatBuf path_statbuf, cwd_statbuf;
  time_t now;
#ifdef G_OS_UNIX
  gboolean have_cap_dac_override;
#endif

  g_remove ("mkdir-test/test-create");
  ret = g_rmdir ("mkdir-test");
  g_assert_true (ret == 0 || errno == ENOENT);

  ret = g_stat ("mkdir-test", &buf);
  g_assert_cmpint (ret, ==, -1);
  ret = g_mkdir ("mkdir-test", 0666);
  g_assert_cmpint (ret, ==, 0);
  ret = g_stat ("mkdir-test", &buf);
  g_assert_cmpint (ret, ==, 0);
  g_assert_cmpint (S_ISDIR (buf.st_mode), !=, 0);

  cwd = g_get_current_dir ();
  path = g_build_filename (cwd, "mkdir-test", NULL);
#ifdef G_OS_UNIX
  have_cap_dac_override = check_cap_dac_override (cwd);
#endif
  g_free (cwd);

  /* 0666 on directories means nothing to Windows, it only obeys ACLs.
   * It doesn't necessarily mean anything on Unix either: if we have
   * Linux CAP_DAC_OVERRIDE or equivalent (in particular if we're root),
   * then we ignore filesystem permissions. */
#ifdef G_OS_UNIX
  if (have_cap_dac_override)
    {
      g_test_message ("Cannot test g_chdir() failing with EACCES: we "
                      "probably have CAP_DAC_OVERRIDE or equivalent");
    }
  else
    {
      ret = g_chdir (path);
      g_assert_cmpint (ret == 0 ? 0 : errno, ==, EACCES);
      g_assert_cmpint (ret, ==, -1);
    }
#else
  g_test_message ("Cannot test g_chdir() failing with EACCES: "
                  "it's Unix-specific behaviour");
#endif

  ret = g_chmod (path, 0777);
  g_assert_cmpint (ret, ==, 0);
  ret = g_chdir (path);
  g_assert_cmpint (ret, ==, 0);
  cwd = g_get_current_dir ();
  /* We essentially want to check that cwd == path, but we can’t compare the
   * paths directly since the tests might be running under a symlink (for
   * example, /tmp is sometimes a symlink). Compare the inode numbers instead. */
  g_assert_cmpint (g_stat (cwd, &cwd_statbuf), ==, 0);
  g_assert_cmpint (g_stat (path, &path_statbuf), ==, 0);
  g_assert_true (cwd_statbuf.st_dev == path_statbuf.st_dev &&
                 cwd_statbuf.st_ino == path_statbuf.st_ino);
  g_free (cwd);
  g_free (path);

  ret = g_creat ("test-creat", G_TEST_DIR_MODE);
  g_close (ret, &error);
  g_assert_no_error (error);

  ret = g_access ("test-creat", F_OK);
  g_assert_cmpint (ret, ==, 0);

  ret = g_rename ("test-creat", "test-create");
  g_assert_cmpint (ret, ==, 0);

  ret = g_open ("test-create", O_RDONLY, 0666);
  g_close (ret, &error);
  g_assert_no_error (error);

#ifdef G_OS_WIN32
  /* On Windows the 5 permission bit results in a read-only file
   * that cannot be modified in any way (attribute changes included).
   * Remove the read-only attribute via chmod().
   */
  ret = g_chmod ("test-create", 0666);
  g_assert_cmpint (ret, ==, 0);
#endif

  now = time (NULL);

  ut.actime = ut.modtime = now;
  ret = g_utime ("test-create", &ut);
  g_assert_cmpint (ret, ==, 0);

  ret = g_lstat ("test-create", &buf);
  g_assert_cmpint (ret, ==, 0);
  g_assert_cmpint (buf.st_atime, ==, now);
  g_assert_cmpint (buf.st_mtime, ==, now);

  g_chdir ("..");
  g_remove ("mkdir-test/test-create");
  g_rmdir ("mkdir-test");
}

/* Win32 does not support "wb+", but g_fopen() should automatically
 * translate this mode to its alias "w+b".
 * Also check various other file open modes for correct support across
 * platforms.
 * See: https://gitlab.gnome.org/GNOME/glib/merge_requests/119
 */
static void
test_fopen_modes (void)
{
  char        *path = g_build_filename (g_get_tmp_dir (), "temp-fopen", NULL);
  gsize        i;
  const gchar *modes[] =
    {
      "w",
      "r",
      "a",
      "w+",
      "r+",
      "a+",
      "wb",
      "rb",
      "ab",
      "w+b",
      "r+b",
      "a+b",
      "wb+",
      "rb+",
      "ab+",
      "we",
      "re",
      "ae",
      "w+e",
      "r+e",
      "a+e",
      "wbe",
      "rbe",
      "abe",
      "w+be",
      "r+be",
      "a+be",
      "wb+e",
      "rb+e",
      "ab+e",
      "web",
      "reb",
      "aeb",
      "w+eb",
      "r+eb",
      "a+eb",
      "web+",
      "reb+",
      "aeb+",
#ifdef G_OS_WIN32
      /* https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/fopen-wfopen?view=msvc-170#unicode-support */
      "w, ccs=utf-16le",
      "we, ccs=utf-16le",
#endif
    };

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/merge_requests/119");

  if (g_file_test (path, G_FILE_TEST_EXISTS))
    g_error ("failed, %s exists, cannot test g_fopen()", path);

  for (i = 0; i < G_N_ELEMENTS (modes); i++)
    {
      FILE *f;

      g_test_message ("Testing fopen() mode '%s'", modes[i]);

      f = g_fopen (path, modes[i]);
      g_assert_nonnull (f);
      fclose (f);
    }

  g_remove (path);
  g_free (path);
}

#ifdef G_OS_WIN32
#include "../gstdio-private.c"

static int
g_wcscmp0 (const gunichar2 *str1,
           const gunichar2 *str2)
{
  if (!str1)
    return -(str1 != str2);
  if (!str2)
    return str1 != str2;
  return wcscmp (str1, str2);
}

#define g_assert_cmpwcs(s1, cmp, s2, s1u8, s2u8) \
G_STMT_START { \
  const gunichar2 *__s1 = (s1), *__s2 = (s2); \
  if (g_wcscmp0 (__s1, __s2) cmp 0) ; else \
    g_assertion_message_cmpstr (G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC, \
                                #s1u8 " " #cmp " " #s2u8, s1u8, #cmp, s2u8); \
} G_STMT_END

static void
test_win32_pathstrip (void)
{
  gunichar2 *buf;
  gsize i;
#define IDENTITY_TEST(x) { x, x, FALSE }
  struct
  {
    gunichar2 *in;
    gunichar2 *out;
    gboolean   result;
  } testcases[] = {
    IDENTITY_TEST (L"\\\\?\\V"),
    IDENTITY_TEST (L"\\\\?\\Vo"),
    IDENTITY_TEST (L"\\\\?\\Volume{0700f3d3-6d24-11e3-8b2f-806e6f6e6963}\\"),
    IDENTITY_TEST (L"\\??\\V"),
    IDENTITY_TEST (L"\\??\\Vo"),
    IDENTITY_TEST (L"\\??\\Volume{0700f3d3-6d24-11e3-8b2f-806e6f6e6963}\\"),
    IDENTITY_TEST (L"\\\\?\\\x0441:\\"),
    IDENTITY_TEST (L"\\??\\\x0441:\\"),
    IDENTITY_TEST (L"a:\\"),
    IDENTITY_TEST (L"a:\\b\\c"),
    IDENTITY_TEST (L"x"),
#undef IDENTITY_TEST
    {
      L"\\\\?\\c:\\",
             L"c:\\",
      TRUE,
    },
    {
      L"\\\\?\\C:\\",
             L"C:\\",
      TRUE,
    },
    {
      L"\\\\?\\c:\\",
             L"c:\\",
      TRUE,
    },
    {
      L"\\\\?\\C:\\",
             L"C:\\",
      TRUE,
    },
    {
      L"\\\\?\\C:\\",
             L"C:\\",
      TRUE,
    },
    { 0, }
  };

  for (i = 0; testcases[i].in; i++)
    {
      gsize str_len = wcslen (testcases[i].in) + 1;
      gchar *in_u8 = g_utf16_to_utf8 (testcases[i].in, -1, NULL, NULL, NULL);
      gchar *out_u8 = g_utf16_to_utf8 (testcases[i].out, -1, NULL, NULL, NULL);

      g_assert_nonnull (in_u8);
      g_assert_nonnull (out_u8);

      buf = g_new0 (gunichar2, str_len);
      memcpy (buf, testcases[i].in, str_len * sizeof (gunichar2));
      _g_win32_strip_extended_ntobjm_prefix (buf, &str_len);
      g_assert_cmpwcs (buf, ==, testcases[i].out, in_u8, out_u8);
      g_free (buf);
      g_free (in_u8);
      g_free (out_u8);
    }
  /* Check for correct behaviour on non-NUL-terminated strings */
  for (i = 0; testcases[i].in; i++)
    {
      gsize str_len = wcslen (testcases[i].in) + 1;
      wchar_t old_endchar;
      gchar *in_u8 = g_utf16_to_utf8 (testcases[i].in, -1, NULL, NULL, NULL);
      gchar *out_u8 = g_utf16_to_utf8 (testcases[i].out, -1, NULL, NULL, NULL);

      g_assert_nonnull (in_u8);
      g_assert_nonnull (out_u8);

      buf = g_new0 (gunichar2, str_len);
      memcpy (buf, testcases[i].in, (str_len) * sizeof (gunichar2));

      old_endchar = buf[wcslen (testcases[i].out)];
      str_len -= 1;

      if (testcases[i].result)
        {
          /* Given "\\\\?\\C:\\" (len 7, unterminated),
           * we should get "C:\\" (len 3, unterminated).
           * Put a character different from "\\" (4-th character of the buffer)
           * at the end of the unterminated source buffer, into a position
           * where NUL-terminator would normally be. Then later test that 4-th character
           * in the buffer is still the old "\\".
           * After that terminate the string and use normal g_wcscmp0().
           */
          buf[str_len] = old_endchar - 1;
        }

      _g_win32_strip_extended_ntobjm_prefix (buf, &str_len);
      g_assert_cmpuint (old_endchar, ==, buf[wcslen (testcases[i].out)]);
      buf[str_len] = L'\0';
      g_assert_cmpwcs (buf, ==, testcases[i].out, in_u8, out_u8);
      g_free (buf);
      g_free (in_u8);
      g_free (out_u8);
    }
}

#define g_assert_memcmp(m1, cmp, m2, memlen, m1hex, m2hex, testcase_num) \
G_STMT_START { \
  if (memcmp (m1, m2, memlen) cmp 0); else \
    g_assertion_message_cmpstr (G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC, \
                                #m1hex " " #cmp " " #m2hex, m1hex, #cmp, m2hex); \
} G_STMT_END

static gchar *
to_hex (const guchar *buf,
        gsize        len)
{
  gsize i;
  GString *s = g_string_new (NULL);
  if (len > 0)
    g_string_append_printf (s, "%02x", buf[0]);
  for (i = 1; i < len; i++)
    g_string_append_printf (s, " %02x", buf[i]);
  return g_string_free (s, FALSE);
}

static void
test_win32_zero_terminate_symlink (void)
{
  gsize i;
#define TESTCASE(data, len_mod, use_buf, buf_size, terminate, reported_len, returned_string) \
 { (const guchar *) data, wcslen (data) * 2 + len_mod, use_buf, buf_size, terminate, reported_len, (guchar *) returned_string},

  struct
  {
    const guchar *data;
    gsize         data_size;
    gboolean      use_buf;
    gsize         buf_size;
    gboolean      terminate;
    int           reported_len;
    const guchar *returned_string;
  } testcases[] = {
    TESTCASE (L"foobar", +2, TRUE, 12 + 4, FALSE, 12 + 2, "f\0o\0o\0b\0a\0r\0\0\0")
    TESTCASE (L"foobar", +2, TRUE, 12 + 3, FALSE, 12 + 2, "f\0o\0o\0b\0a\0r\0\0\0")
    TESTCASE (L"foobar", +2, TRUE, 12 + 2, FALSE, 12 + 2, "f\0o\0o\0b\0a\0r\0\0\0")
    TESTCASE (L"foobar", +2, TRUE, 12 + 1, FALSE, 12 + 1, "f\0o\0o\0b\0a\0r\0\0")
    TESTCASE (L"foobar", +2, TRUE, 12 + 0, FALSE, 12 + 0, "f\0o\0o\0b\0a\0r\0")
    TESTCASE (L"foobar", +2, TRUE, 12 - 1, FALSE, 12 - 1, "f\0o\0o\0b\0a\0r")
    TESTCASE (L"foobar", +2, TRUE, 12 - 2, FALSE, 12 - 2, "f\0o\0o\0b\0a\0")
    TESTCASE (L"foobar", +2, TRUE, 12 - 3, FALSE, 12 - 3, "f\0o\0o\0b\0a")
    TESTCASE (L"foobar", +1, TRUE, 12 + 4, FALSE, 12 + 1, "f\0o\0o\0b\0a\0r\0\0")
    TESTCASE (L"foobar", +1, TRUE, 12 + 3, FALSE, 12 + 1, "f\0o\0o\0b\0a\0r\0\0")
    TESTCASE (L"foobar", +1, TRUE, 12 + 2, FALSE, 12 + 1, "f\0o\0o\0b\0a\0r\0\0")
    TESTCASE (L"foobar", +1, TRUE, 12 + 1, FALSE, 12 + 1, "f\0o\0o\0b\0a\0r\0\0")
    TESTCASE (L"foobar", +1, TRUE, 12 + 0, FALSE, 12 + 0, "f\0o\0o\0b\0a\0r\0")
    TESTCASE (L"foobar", +1, TRUE, 12 - 1, FALSE, 12 - 1, "f\0o\0o\0b\0a\0r")
    TESTCASE (L"foobar", +1, TRUE, 12 - 2, FALSE, 12 - 2, "f\0o\0o\0b\0a\0")
    TESTCASE (L"foobar", +1, TRUE, 12 - 3, FALSE, 12 - 3, "f\0o\0o\0b\0a")
    TESTCASE (L"foobar", +0, TRUE, 12 + 4, FALSE, 12 + 0, "f\0o\0o\0b\0a\0r\0")
    TESTCASE (L"foobar", +0, TRUE, 12 + 3, FALSE, 12 + 0, "f\0o\0o\0b\0a\0r\0")
    TESTCASE (L"foobar", +0, TRUE, 12 + 2, FALSE, 12 + 0, "f\0o\0o\0b\0a\0r\0")
    TESTCASE (L"foobar", +0, TRUE, 12 + 1, FALSE, 12 + 0, "f\0o\0o\0b\0a\0r\0")
    TESTCASE (L"foobar", +0, TRUE, 12 + 0, FALSE, 12 + 0, "f\0o\0o\0b\0a\0r\0")
    TESTCASE (L"foobar", +0, TRUE, 12 - 1, FALSE, 12 - 1, "f\0o\0o\0b\0a\0r")
    TESTCASE (L"foobar", +0, TRUE, 12 - 2, FALSE, 12 - 2, "f\0o\0o\0b\0a\0")
    TESTCASE (L"foobar", +0, TRUE, 12 - 3, FALSE, 12 - 3, "f\0o\0o\0b\0a")
    TESTCASE (L"foobar", -1, TRUE, 12 + 3, FALSE, 12 - 1, "f\0o\0o\0b\0a\0r")
    TESTCASE (L"foobar", -1, TRUE, 12 + 2, FALSE, 12 - 1, "f\0o\0o\0b\0a\0r")
    TESTCASE (L"foobar", -1, TRUE, 12 + 1, FALSE, 12 - 1, "f\0o\0o\0b\0a\0r")
    TESTCASE (L"foobar", -1, TRUE, 12 + 0, FALSE, 12 - 1, "f\0o\0o\0b\0a\0r")
    TESTCASE (L"foobar", -1, TRUE, 12 - 1, FALSE, 12 - 1, "f\0o\0o\0b\0a\0r")
    TESTCASE (L"foobar", -1, TRUE, 12 - 2, FALSE, 12 - 2, "f\0o\0o\0b\0a\0")
    TESTCASE (L"foobar", -1, TRUE, 12 - 3, FALSE, 12 - 3, "f\0o\0o\0b\0a")
    TESTCASE (L"foobar", -1, TRUE, 12 - 4, FALSE, 12 - 4, "f\0o\0o\0b\0")
    TESTCASE (L"foobar", -2, TRUE, 12 + 2, FALSE, 12 - 2, "f\0o\0o\0b\0a\0")
    TESTCASE (L"foobar", -2, TRUE, 12 + 1, FALSE, 12 - 2, "f\0o\0o\0b\0a\0")
    TESTCASE (L"foobar", -2, TRUE, 12 + 0, FALSE, 12 - 2, "f\0o\0o\0b\0a\0")
    TESTCASE (L"foobar", -2, TRUE, 12 - 1, FALSE, 12 - 2, "f\0o\0o\0b\0a\0")
    TESTCASE (L"foobar", -2, TRUE, 12 - 2, FALSE, 12 - 2, "f\0o\0o\0b\0a\0")
    TESTCASE (L"foobar", -2, TRUE, 12 - 3, FALSE, 12 - 3, "f\0o\0o\0b\0a")
    TESTCASE (L"foobar", -2, TRUE, 12 - 4, FALSE, 12 - 4, "f\0o\0o\0b\0")
    TESTCASE (L"foobar", -2, TRUE, 12 - 5, FALSE, 12 - 5, "f\0o\0o\0b")
    TESTCASE (L"foobar", +2, TRUE, 12 + 4, TRUE, 12 + 2, "f\0o\0o\0b\0a\0r\0\0\0")
    TESTCASE (L"foobar", +2, TRUE, 12 + 3, TRUE, 12 + 2, "f\0o\0o\0b\0a\0r\0\0\0")
    TESTCASE (L"foobar", +2, TRUE, 12 + 2, TRUE, 12 + 2, "f\0o\0o\0b\0a\0r\0\0\0")
    TESTCASE (L"foobar", +2, TRUE, 12 + 1, TRUE, 12 + 1, "f\0o\0o\0b\0a\0r\0\0")
    TESTCASE (L"foobar", +2, TRUE, 12 + 0, TRUE, 12 + 0, "f\0o\0o\0b\0a\0\0\0")
    TESTCASE (L"foobar", +2, TRUE, 12 - 1, TRUE, 12 - 1, "f\0o\0o\0b\0a\0\0")
    TESTCASE (L"foobar", +2, TRUE, 12 - 2, TRUE, 12 - 2, "f\0o\0o\0b\0\0\0")
    TESTCASE (L"foobar", +2, TRUE, 12 - 3, TRUE, 12 - 3, "f\0o\0o\0b\0\0")
    TESTCASE (L"foobar", +1, TRUE, 12 + 4, TRUE, 12 + 1, "f\0o\0o\0b\0a\0r\0\0")
    TESTCASE (L"foobar", +1, TRUE, 12 + 3, TRUE, 12 + 1, "f\0o\0o\0b\0a\0r\0\0")
    TESTCASE (L"foobar", +1, TRUE, 12 + 2, TRUE, 12 + 1, "f\0o\0o\0b\0a\0r\0\0")
    TESTCASE (L"foobar", +1, TRUE, 12 + 1, TRUE, 12 + 1, "f\0o\0o\0b\0a\0r\0\0")
    TESTCASE (L"foobar", +1, TRUE, 12 + 0, TRUE, 12 + 0, "f\0o\0o\0b\0a\0\0\0")
    TESTCASE (L"foobar", +1, TRUE, 12 - 1, TRUE, 12 - 1, "f\0o\0o\0b\0a\0\0")
    TESTCASE (L"foobar", +1, TRUE, 12 - 2, TRUE, 12 - 2, "f\0o\0o\0b\0\0\0")
    TESTCASE (L"foobar", +1, TRUE, 12 - 3, TRUE, 12 - 3, "f\0o\0o\0b\0\0")
    TESTCASE (L"foobar", +0, TRUE, 12 + 4, TRUE, 12 + 1, "f\0o\0o\0b\0a\0r\0\0")
    TESTCASE (L"foobar", +0, TRUE, 12 + 3, TRUE, 12 + 1, "f\0o\0o\0b\0a\0r\0\0")
    TESTCASE (L"foobar", +0, TRUE, 12 + 2, TRUE, 12 + 1, "f\0o\0o\0b\0a\0r\0\0")
    TESTCASE (L"foobar", +0, TRUE, 12 + 1, TRUE, 12 + 1, "f\0o\0o\0b\0a\0r\0\0")
    TESTCASE (L"foobar", +0, TRUE, 12 + 0, TRUE, 12 + 0, "f\0o\0o\0b\0a\0\0\0")
    TESTCASE (L"foobar", +0, TRUE, 12 - 1, TRUE, 12 - 1, "f\0o\0o\0b\0a\0\0")
    TESTCASE (L"foobar", +0, TRUE, 12 - 2, TRUE, 12 - 2, "f\0o\0o\0b\0\0\0")
    TESTCASE (L"foobar", +0, TRUE, 12 - 3, TRUE, 12 - 3, "f\0o\0o\0b\0\0")
    TESTCASE (L"foobar", -1, TRUE, 12 + 3, TRUE, 12 + 1, "f\0o\0o\0b\0a\0r\0\0")
    TESTCASE (L"foobar", -1, TRUE, 12 + 2, TRUE, 12 + 1, "f\0o\0o\0b\0a\0r\0\0")
    TESTCASE (L"foobar", -1, TRUE, 12 + 1, TRUE, 12 + 1, "f\0o\0o\0b\0a\0r\0\0")
    TESTCASE (L"foobar", -1, TRUE, 12 + 0, TRUE, 12 + 0, "f\0o\0o\0b\0a\0\0\0")
    TESTCASE (L"foobar", -1, TRUE, 12 - 1, TRUE, 12 - 1, "f\0o\0o\0b\0a\0\0")
    TESTCASE (L"foobar", -1, TRUE, 12 - 2, TRUE, 12 - 2, "f\0o\0o\0b\0\0\0")
    TESTCASE (L"foobar", -1, TRUE, 12 - 3, TRUE, 12 - 3, "f\0o\0o\0b\0\0")
    TESTCASE (L"foobar", -1, TRUE, 12 - 4, TRUE, 12 - 4, "f\0o\0o\0\0\0")
    TESTCASE (L"foobar", -2, TRUE, 12 + 2, TRUE, 12 - 1, "f\0o\0o\0b\0a\0\0")
    TESTCASE (L"foobar", -2, TRUE, 12 + 1, TRUE, 12 - 1, "f\0o\0o\0b\0a\0\0")
    TESTCASE (L"foobar", -2, TRUE, 12 + 0, TRUE, 12 - 1, "f\0o\0o\0b\0a\0\0")
    TESTCASE (L"foobar", -2, TRUE, 12 - 1, TRUE, 12 - 1, "f\0o\0o\0b\0a\0\0")
    TESTCASE (L"foobar", -2, TRUE, 12 - 2, TRUE, 12 - 2, "f\0o\0o\0b\0\0\0")
    TESTCASE (L"foobar", -2, TRUE, 12 - 3, TRUE, 12 - 3, "f\0o\0o\0b\0\0")
    TESTCASE (L"foobar", -2, TRUE, 12 - 4, TRUE, 12 - 4, "f\0o\0o\0\0\0")
    TESTCASE (L"foobar", -2, TRUE, 12 - 5, TRUE, 12 - 5, "f\0o\0o\0\0")
    TESTCASE (L"foobar", +2, FALSE, 0, FALSE, 12 + 2, "f\0o\0o\0b\0a\0r\0\0\0")
    TESTCASE (L"foobar", +1, FALSE, 0, FALSE, 12 + 1, "f\0o\0o\0b\0a\0r\0\0")
    TESTCASE (L"foobar", +0, FALSE, 0, FALSE, 12 + 0, "f\0o\0o\0b\0a\0r\0")
    TESTCASE (L"foobar", -1, FALSE, 0, FALSE, 12 - 1, "f\0o\0o\0b\0a\0r")
    TESTCASE (L"foobar", -2, FALSE, 0, FALSE, 12 - 2, "f\0o\0o\0b\0a\0")
    TESTCASE (L"foobar", +2, FALSE, 0, TRUE, 12 + 2, "f\0o\0o\0b\0a\0r\0\0\0")
    TESTCASE (L"foobar", +1, FALSE, 0, TRUE, 12 + 1, "f\0o\0o\0b\0a\0r\0\0")
    TESTCASE (L"foobar", +0, FALSE, 0, TRUE, 12 + 1, "f\0o\0o\0b\0a\0r\0\0")
    TESTCASE (L"foobar", -1, FALSE, 0, TRUE, 12 + 1, "f\0o\0o\0b\0a\0r\0\0")
    TESTCASE (L"foobar", -2, FALSE, 0, TRUE, 12 - 1, "f\0o\0o\0b\0a\0\0")
    TESTCASE (L"x", +2, TRUE, 2 + 4, FALSE, 2 + 2, "x\0\0\0")
    TESTCASE (L"x", +2, TRUE, 2 + 3, FALSE, 2 + 2, "x\0\0\0")
    TESTCASE (L"x", +2, TRUE, 2 + 2, FALSE, 2 + 2, "x\0\0\0")
    TESTCASE (L"x", +2, TRUE, 2 + 1, FALSE, 2 + 1, "x\0\0")
    TESTCASE (L"x", +2, TRUE, 2 + 0, FALSE, 2 + 0, "x\0")
    TESTCASE (L"x", +2, TRUE, 2 - 1, FALSE, 2 - 1, "x")
    TESTCASE (L"x", +2, TRUE, 2 - 2, FALSE, 2 - 2, "")
    TESTCASE (L"x", +1, TRUE, 2 + 3, FALSE, 2 + 1, "x\0\0")
    TESTCASE (L"x", +1, TRUE, 2 + 2, FALSE, 2 + 1, "x\0\0")
    TESTCASE (L"x", +1, TRUE, 2 + 1, FALSE, 2 + 1, "x\0\0")
    TESTCASE (L"x", +1, TRUE, 2 + 0, FALSE, 2 + 0, "x\0")
    TESTCASE (L"x", +1, TRUE, 2 - 1, FALSE, 2 - 1, "x")
    TESTCASE (L"x", +1, TRUE, 2 - 2, FALSE, 2 - 2, "")
    TESTCASE (L"x", +0, TRUE, 2 + 2, FALSE, 2 + 0, "x\0")
    TESTCASE (L"x", +0, TRUE, 2 + 1, FALSE, 2 + 0, "x\0")
    TESTCASE (L"x", +0, TRUE, 2 + 0, FALSE, 2 + 0, "x\0")
    TESTCASE (L"x", +0, TRUE, 2 - 1, FALSE, 2 - 1, "x")
    TESTCASE (L"x", +0, TRUE, 2 - 2, FALSE, 2 - 2, "")
    TESTCASE (L"x", -1, TRUE, 2 + 1, FALSE, 2 - 1, "x")
    TESTCASE (L"x", -1, TRUE, 2 + 0, FALSE, 2 - 1, "x")
    TESTCASE (L"x", -1, TRUE, 2 - 1, FALSE, 2 - 1, "x")
    TESTCASE (L"x", -1, TRUE, 2 - 2, FALSE, 2 - 2, "")
    TESTCASE (L"x", -2, TRUE, 2 + 0, FALSE, 2 - 2, "")
    TESTCASE (L"x", -2, TRUE, 2 - 1, FALSE, 2 - 2, "")
    TESTCASE (L"x", -2, TRUE, 2 - 2, FALSE, 2 - 2, "")
    TESTCASE (L"x", +2, TRUE, 2 + 4, TRUE, 2 + 2, "x\0\0\0")
    TESTCASE (L"x", +2, TRUE, 2 + 3, TRUE, 2 + 2, "x\0\0\0")
    TESTCASE (L"x", +2, TRUE, 2 + 2, TRUE, 2 + 2, "x\0\0\0")
    TESTCASE (L"x", +2, TRUE, 2 + 1, TRUE, 2 + 1, "x\0\0")
    TESTCASE (L"x", +2, TRUE, 2 + 0, TRUE, 2 + 0, "\0\0")
    TESTCASE (L"x", +2, TRUE, 2 - 1, TRUE, 2 - 1, "\0")
    TESTCASE (L"x", +2, TRUE, 2 - 2, TRUE, 2 - 2, "")
    TESTCASE (L"x", +1, TRUE, 2 + 3, TRUE, 2 + 1, "x\0\0")
    TESTCASE (L"x", +1, TRUE, 2 + 2, TRUE, 2 + 1, "x\0\0")
    TESTCASE (L"x", +1, TRUE, 2 + 1, TRUE, 2 + 1, "x\0\0")
    TESTCASE (L"x", +1, TRUE, 2 + 0, TRUE, 2 + 0, "\0\0")
    TESTCASE (L"x", +1, TRUE, 2 - 1, TRUE, 2 - 1, "\0")
    TESTCASE (L"x", +1, TRUE, 2 - 2, TRUE, 2 - 2, "")
    TESTCASE (L"x", +0, TRUE, 2 + 2, TRUE, 2 + 1, "x\0\0")
    TESTCASE (L"x", +0, TRUE, 2 + 1, TRUE, 2 + 1, "x\0\0")
    TESTCASE (L"x", +0, TRUE, 2 + 0, TRUE, 2 + 0, "\0\0")
    TESTCASE (L"x", +0, TRUE, 2 - 1, TRUE, 2 - 1, "\0")
    TESTCASE (L"x", +0, TRUE, 2 - 2, TRUE, 2 - 2, "")
    TESTCASE (L"x", -1, TRUE, 2 + 1, TRUE, 2 + 1, "x\0\0")
    TESTCASE (L"x", -1, TRUE, 2 + 0, TRUE, 2 + 0, "\0\0")
    TESTCASE (L"x", -1, TRUE, 2 - 1, TRUE, 2 - 1, "\0")
    TESTCASE (L"x", -1, TRUE, 2 - 2, TRUE, 2 - 2, "")
    TESTCASE (L"x", -2, TRUE, 2 + 0, TRUE, 2 - 2, "")
    TESTCASE (L"x", -2, TRUE, 2 - 1, TRUE, 2 - 2, "")
    TESTCASE (L"x", -2, TRUE, 2 - 2, TRUE, 2 - 2, "")
    TESTCASE (L"x", +2, FALSE, 0, FALSE, 2 + 2, "x\0\0\0")
    TESTCASE (L"x", +1, FALSE, 0, FALSE, 2 + 1, "x\0\0")
    TESTCASE (L"x", +0, FALSE, 0, FALSE, 2 + 0, "x\0")
    TESTCASE (L"x", -1, FALSE, 0, FALSE, 2 - 1, "x")
    TESTCASE (L"x", -2, FALSE, 0, FALSE, 2 - 2, "")
    TESTCASE (L"x", +2, FALSE, 0, TRUE, 2 + 2, "x\0\0\0")
    TESTCASE (L"x", +1, FALSE, 0, TRUE, 2 + 1, "x\0\0")
    TESTCASE (L"x", +0, FALSE, 0, TRUE, 2 + 1, "x\0\0")
    TESTCASE (L"x", -1, FALSE, 0, TRUE, 2 + 1, "x\0\0")
    TESTCASE (L"x", -2, FALSE, 0, TRUE, 2 - 2, "")
    { 0, },
  };
#undef TESTCASE

  for (i = 0; testcases[i].data != NULL; i++)
    {
      gunichar2 *buf;
      int result;
      gchar *buf_hex, *expected_hex;
      if (testcases[i].use_buf)
        buf = g_malloc0 (testcases[i].buf_size + 1); /* +1 to ensure it succeeds with buf_size == 0 */
      else
        buf = NULL;
      result = _g_win32_copy_and_maybe_terminate (testcases[i].data,
                                                  testcases[i].data_size,
                                                  testcases[i].use_buf ? buf : NULL,
                                                  testcases[i].buf_size,
                                                  testcases[i].use_buf ? NULL : &buf,
                                                  testcases[i].terminate);
      if (testcases[i].reported_len != result)
        g_error ("Test %" G_GSIZE_FORMAT " failed, result %d != %d", i, result, testcases[i].reported_len);
      if (buf == NULL && testcases[i].buf_size != 0)
        g_error ("Test %" G_GSIZE_FORMAT " failed, buf == NULL", i);
      g_assert_cmpint (testcases[i].reported_len, ==, result);
      if ((testcases[i].use_buf && testcases[i].buf_size != 0) ||
          (!testcases[i].use_buf && testcases[i].reported_len != 0))
        {
          g_assert_nonnull (buf);
          buf_hex = to_hex ((const guchar *) buf, result);
          expected_hex = to_hex (testcases[i].returned_string, testcases[i].reported_len);
          if (memcmp (buf, testcases[i].returned_string, result) != 0)
            g_error ("Test %" G_GSIZE_FORMAT " failed:\n%s !=\n%s", i, buf_hex, expected_hex);
          g_assert_memcmp (buf, ==, testcases[i].returned_string, testcases[i].reported_len, buf_hex, expected_hex, testcases[i].line);
          g_free (buf_hex);
          g_free (expected_hex);
        }
      g_free (buf);
    }
}

#endif

static void
test_clear_fd_ebadf (void)
{
  char *name = NULL;
  GError *error = NULL;
  int fd;
  int copy_of_fd;
  int errsv;
  gboolean ret;
  GWin32InvalidParameterHandler handler;

  /* We're going to trigger a programming error: attempting to close a
   * fd that was already closed. Make criticals non-fatal. */
  g_assert_true (g_test_undefined ());
  g_log_set_always_fatal (G_LOG_FATAL_MASK);
  g_log_set_fatal_mask ("GLib", G_LOG_FATAL_MASK);
  GLIB_PRIVATE_CALL (g_win32_push_empty_invalid_parameter_handler) (&handler);

  fd = g_file_open_tmp (NULL, &name, &error);
  g_assert_cmpint (fd, !=, -1);
  g_assert_no_error (error);
  g_assert_nonnull (name);
  ret = g_close (fd, &error);
  g_assert_no_error (error);
  assert_fd_was_closed (fd);
  g_assert_true (ret);
  g_unlink (name);
  g_free (name);

  /* Try to close it again with g_close() */
  ret = g_close (fd, NULL);
  errsv = errno;
  g_assert_cmpint (errsv, ==, EBADF);
  assert_fd_was_closed (fd);
  g_assert_false (ret);

  /* Try to close it again with g_clear_fd() */
  copy_of_fd = fd;
  errno = EILSEQ;
  ret = g_clear_fd (&copy_of_fd, NULL);
  errsv = errno;
  g_assert_cmpint (errsv, ==, EBADF);
  assert_fd_was_closed (fd);
  g_assert_false (ret);

#ifdef g_autofree
    {
      g_autofd int close_me = fd;

      /* This avoids clang warnings about the variables being unused */
      g_test_message ("Invalid fd will be closed by autocleanup: %d",
                      close_me);
      errno = EILSEQ;
    }

  errsv = errno;
  g_assert_cmpint (errsv, ==, EILSEQ);
#endif

  GLIB_PRIVATE_CALL (g_win32_pop_invalid_parameter_handler) (&handler);
}

static void
test_clear_fd (void)
{
  char *name = NULL;
  GError *error = NULL;
  int fd;
  int copy_of_fd;
  int errsv;

#ifdef g_autofree
  g_test_summary ("Test g_clear_fd() and g_autofd");
#else
  g_test_summary ("Test g_clear_fd() (g_autofd unsupported by this compiler)");
#endif

  /* g_clear_fd() normalizes any negative number to -1 */
  fd = -23;
  g_clear_fd (&fd, &error);
  g_assert_cmpint (fd, ==, -1);
  g_assert_no_error (error);

  /* Nothing special about g_file_open_tmp, it's just a convenient way
   * to get an open fd */
  fd = g_file_open_tmp (NULL, &name, &error);
  g_assert_cmpint (fd, !=, -1);
  g_assert_no_error (error);
  g_assert_nonnull (name);
  copy_of_fd = fd;
  g_clear_fd (&fd, &error);
  g_assert_cmpint (fd, ==, -1);
  g_assert_no_error (error);
  assert_fd_was_closed (copy_of_fd);
  g_unlink (name);
  g_free (name);

  /* g_clear_fd() is idempotent */
  g_clear_fd (&fd, &error);
  g_assert_cmpint (fd, ==, -1);
  g_assert_no_error (error);

#ifdef g_autofree
  fd = g_file_open_tmp (NULL, &name, &error);
  g_assert_cmpint (fd, !=, -1);
  g_assert_no_error (error);
  g_assert_nonnull (name);

    {
      g_autofd int close_me = fd;
      g_autofd int was_never_set = -42;

      /* This avoids clang warnings about the variables being unused */
      g_test_message ("Will be closed by autocleanup: %d, %d",
                      close_me, was_never_set);
      /* This is one of the few errno values guaranteed by Standard C.
       * We set it here to check that a successful g_autofd close doesn't
       * alter errno. */
      errno = EILSEQ;
    }

  errsv = errno;
  g_assert_cmpint (errsv, ==, EILSEQ);
  assert_fd_was_closed (fd);
  g_unlink (name);
  g_free (name);
#endif

  if (g_test_undefined ())
    {
      g_test_message ("Testing error handling");
      g_test_trap_subprocess ("/fileutils/clear-fd/subprocess/ebadf",
                              0, G_TEST_SUBPROCESS_DEFAULT);
#ifdef g_autofree
      g_test_trap_assert_stderr ("*failed with EBADF*failed with EBADF*failed with EBADF*");
#else
      g_test_trap_assert_stderr ("*failed with EBADF*failed with EBADF*");
#endif
      g_test_trap_assert_passed ();
    }
}

int
main (int   argc,
      char *argv[])
{
  g_setenv ("LC_ALL", "C", TRUE);
  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

#ifdef G_OS_WIN32
  g_test_add_func ("/fileutils/stdio-win32-pathstrip", test_win32_pathstrip);
  g_test_add_func ("/fileutils/stdio-win32-zero-terminate-symlink", test_win32_zero_terminate_symlink);
#endif
  g_test_add_func ("/fileutils/paths", test_paths);
  g_test_add_func ("/fileutils/build-path", test_build_path);
  g_test_add_func ("/fileutils/build-pathv", test_build_pathv);
  g_test_add_func ("/fileutils/build-filename", test_build_filename);
  g_test_add_func ("/fileutils/build-filenamev", test_build_filenamev);
  g_test_add_func ("/fileutils/mkdir-with-parents", test_mkdir_with_parents);
  g_test_add_func ("/fileutils/mkdir-with-parents-permission", test_mkdir_with_parents_permission);
  g_test_add_func ("/fileutils/format-size-for-display", test_format_size_for_display);
  g_test_add_func ("/fileutils/errors", test_file_errors);
  g_test_add_func ("/fileutils/basename", test_basename);
  g_test_add_func ("/fileutils/get-basename", test_get_basename);
  g_test_add_func ("/fileutils/dirname", test_dirname);
  g_test_add_func ("/fileutils/dir-make-tmp", test_dir_make_tmp);
  g_test_add_func ("/fileutils/file-open-tmp", test_file_open_tmp);
  g_test_add_func ("/fileutils/file-test", test_file_test);
  g_test_add_func ("/fileutils/mkstemp", test_mkstemp);
  g_test_add_func ("/fileutils/mkdtemp", test_mkdtemp);
  g_test_add_func ("/fileutils/get-contents", test_get_contents);
  g_test_add_func ("/fileutils/get-contents-large-file", test_get_contents_largefile);
  g_test_add_func ("/fileutils/set-contents", test_set_contents);
  g_test_add_func ("/fileutils/set-contents-full", test_set_contents_full);
  g_test_add_func ("/fileutils/set-contents-full/read-only-file", test_set_contents_full_read_only_file);
  g_test_add_func ("/fileutils/set-contents-full/read-only-directory", test_set_contents_full_read_only_directory);
  g_test_add_func ("/fileutils/read-link", test_read_link);
  g_test_add_func ("/fileutils/stdio-wrappers", test_stdio_wrappers);
  g_test_add_func ("/fileutils/fopen-modes", test_fopen_modes);
  g_test_add_func ("/fileutils/clear-fd", test_clear_fd);
  g_test_add_func ("/fileutils/clear-fd/subprocess/ebadf", test_clear_fd_ebadf);

  return g_test_run ();
}
