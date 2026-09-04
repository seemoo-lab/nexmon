/* GLib testing framework examples and tests
 *
 * Copyright © 2017 Endless Mobile, Inc.
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

#include <glib.h>

#ifndef G_OS_UNIX
#error This is a Unix-specific test
#endif

#include <errno.h>
#include <locale.h>

#ifdef HAVE_XLOCALE_H
/* Needed on macOS and FreeBSD for uselocale() */
#include <xlocale.h>
#endif

#include <glib/gstdio.h>
#include <gio/gio.h>
#include <gio/gunixmounts.h>

#include "../gunixmounts-private.h"

/* We test all of the old g_unix_mount_*() API before it was renamed to
 * g_unix_mount_entry_*(). The old API calls the new API, so both methods get
 * tested at once. */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void
test_is_system_fs_type (void)
{
  g_assert_true (g_unix_is_system_fs_type ("tmpfs"));
  g_assert_false (g_unix_is_system_fs_type ("ext4"));

  /* Check that some common network file systems aren’t considered ‘system’. */
  g_assert_false (g_unix_is_system_fs_type ("cifs"));
  g_assert_false (g_unix_is_system_fs_type ("nfs"));
  g_assert_false (g_unix_is_system_fs_type ("nfs4"));
  g_assert_false (g_unix_is_system_fs_type ("smbfs"));
}

static void
test_is_system_device_path (void)
{
  g_assert_true (g_unix_is_system_device_path ("devpts"));
  g_assert_false (g_unix_is_system_device_path ("/"));
}

static void
test_system_mount_paths_sorted (void)
{
  size_t i;
  size_t n_paths = G_N_ELEMENTS (system_mount_paths);

  g_test_summary ("Verify that system_mount_paths array is sorted for bsearch");

  for (i = 1; i < n_paths; i++)
    {
      int cmp = strcmp (system_mount_paths[i - 1], system_mount_paths[i]);
      if (cmp > 0)
        {
          g_test_fail_printf ("system_mount_paths array is not sorted: "
                               "\"%s\" should come before \"%s\"",
                               system_mount_paths[i - 1],
                               system_mount_paths[i]);
          return;
        }
    }
}

static void
assert_cmp_icon (GIcon    *icon,
                 gboolean  expected_icon)
{
  if (expected_icon)
    {
      char *icon_str = NULL;

      /* While it would be nice to compare the icon value, that would make these
       * tests depend on the icon themes installed. So just compare nullness. */
      g_assert_nonnull (icon);
      icon_str = g_icon_to_string (icon);
      g_test_message ("Icon: %s", icon_str);
      g_free (icon_str);
    }
  else
    {
      g_assert_null (icon);
    }
}

static void
test_get_mount_points (void)
{
  GUnixMountPoint **points = NULL;
  uint64_t time_read = 0;
  size_t n_points = 0;
  int fd = -1;
  char *tmp_file = NULL;
  gboolean res;
#ifdef HAVE_USELOCALE
  locale_t original_locale;
  locale_t new_locale;
  locale_t result;
#endif
  const char *fake_fstab = "# Some comment\n"
    "/dev/mapper/fedora-root /                       ext4    defaults,x-systemd.device-timeout=0 1 1\n"
    "UUID=1234-ABCD /boot                   ext4    defaults        1 2\n"
    "UUID=ABCD-1234          /boot/efi               vfat    umask=0077,shortname=winnt,ro 0 2\n"
    "/dev/mapper/fedora-home /home                   ext4    defaults,x-systemd.device-timeout=0 1 2\n"
    "/dev/mapper/fedora-swap none                    swap    defaults,x-systemd.device-timeout=0 0 0\n"
    "/dev/mapper/unused      none                    ext4    defaults\n";
  const struct
    {
      const char *device_path;
      const char *fs_type;
      const char *options;
      gboolean is_readonly;
      gboolean is_user_mountable;
      gboolean is_loopback;
      gboolean guessed_icon;
      gboolean guessed_symbolic_icon;
      const char *guessed_name;
      gboolean guessed_can_eject;
    }
  expected_points[] =
    {
      {
        .device_path = "/dev/mapper/fedora-root",
        .fs_type = "ext4",
        .options = "defaults,x-systemd.device-timeout=0",
        .is_readonly = FALSE,
        .is_user_mountable = FALSE,
        .is_loopback = FALSE,
        .guessed_icon = TRUE,
        .guessed_symbolic_icon = TRUE,
        .guessed_name = "Filesystem root",
        .guessed_can_eject = FALSE,
      },
      {
        .device_path = "UUID=1234-ABCD",
        .fs_type = "ext4",
        .options = "defaults",
        .is_readonly = FALSE,
        .is_user_mountable = FALSE,
        .is_loopback = FALSE,
        .guessed_icon = TRUE,
        .guessed_symbolic_icon = TRUE,
        .guessed_name = "boot",
        .guessed_can_eject = FALSE,
      },
      {
        .device_path = "UUID=ABCD-1234",
        .fs_type = "vfat",
        .options = "umask=0077,shortname=winnt,ro",
        .is_readonly = TRUE,
        .is_user_mountable = FALSE,
        .is_loopback = FALSE,
        .guessed_icon = TRUE,
        .guessed_symbolic_icon = TRUE,
        .guessed_name = "efi",
        .guessed_can_eject = FALSE,
      },
      {
        .device_path = "/dev/mapper/fedora-home",
        .fs_type = "ext4",
        .options = "defaults,x-systemd.device-timeout=0",
        .is_readonly = FALSE,
        .is_user_mountable = FALSE,
        .is_loopback = FALSE,
        .guessed_icon = TRUE,
        .guessed_symbolic_icon = TRUE,
        .guessed_name = "home",
        .guessed_can_eject = FALSE,
      },
      /* the swap partition is ignored */
      /* as is the ignored unused partition */
    };

  g_test_summary ("Basic test of g_unix_mount_points_get_from_file()");

  fd = g_file_open_tmp ("unix-mounts-XXXXXX", &tmp_file, NULL);
  g_assert (fd != -1);
  close (fd);

  res = g_file_set_contents (tmp_file, fake_fstab, -1, NULL);
  g_assert (res);

  points = g_unix_mount_points_get_from_file (tmp_file, &time_read, &n_points);

  if (points == NULL)
    {
      /* Some platforms may not support parsing a specific mount point file */
      g_assert_cmpuint (time_read, ==, 0);
      g_assert_cmpuint (n_points, ==, 0);
      g_test_skip ("Parsing mount points from a file not supported on this platform");
      return;
    }

  g_assert_nonnull (points);

  /* Check the properties of the mount points. This needs to be done in a known
   * locale, because the guessed mount point name is translatable. */
  g_assert_cmpuint (n_points, ==, G_N_ELEMENTS (expected_points));

#ifdef HAVE_USELOCALE
  original_locale = uselocale ((locale_t) 0);
  g_assert_true (original_locale != (locale_t) 0);
  new_locale = newlocale (LC_ALL_MASK, "C", (locale_t) 0);
  g_assert_true (new_locale != (locale_t) 0);
  result = uselocale (new_locale);
  g_assert_true (result == original_locale);
#endif  /* HAVE_USELOCALE */

  for (size_t i = 0; i < n_points; i++)
    {
      GIcon *icon = NULL;
      char *name = NULL;

      g_assert_cmpstr (g_unix_mount_point_get_device_path (points[i]), ==, expected_points[i].device_path);
      g_assert_cmpstr (g_unix_mount_point_get_fs_type (points[i]), ==, expected_points[i].fs_type);
      g_assert_cmpstr (g_unix_mount_point_get_options (points[i]), ==, expected_points[i].options);
      g_assert_true (g_unix_mount_point_is_readonly (points[i]) == expected_points[i].is_readonly);
      g_assert_true (g_unix_mount_point_is_user_mountable (points[i]) == expected_points[i].is_user_mountable);
      g_assert_true (g_unix_mount_point_is_loopback (points[i]) == expected_points[i].is_loopback);

      icon = g_unix_mount_point_guess_icon (points[i]);
      assert_cmp_icon (icon, expected_points[i].guessed_icon);
      g_clear_object (&icon);

      icon = g_unix_mount_point_guess_symbolic_icon (points[i]);
      assert_cmp_icon (icon, expected_points[i].guessed_symbolic_icon);
      g_clear_object (&icon);

      name = g_unix_mount_point_guess_name (points[i]);
#ifdef HAVE_USELOCALE
      g_assert_cmpstr (name, ==, expected_points[i].guessed_name);
#else
      g_assert_nonnull (name);
#endif
      g_free (name);

      g_assert_true (g_unix_mount_point_guess_can_eject (points[i]) == expected_points[i].guessed_can_eject);
    }

  for (size_t i = 0; i < n_points; i++)
    g_unix_mount_point_free (points[i]);
  g_free (points);
  g_free (tmp_file);

#ifdef HAVE_USELOCALE
  result = uselocale (original_locale);
  g_assert_true (result == new_locale);
  freelocale (new_locale);
#endif
}

static void
test_get_mount_entries (void)
{
  GUnixMountEntry **entries = NULL;
  uint64_t time_read = 0;
  size_t n_entries = 0;
  int fd = -1;
  char *tmp_file = NULL;
  gboolean res;
#ifdef HAVE_LIBMOUNT
  /* This is an edited /proc/self/mountinfo file, used because that’s what
   * libmount parses by default, and it allows for the representation of root_path. */
  const char *fake_mtab = "# Some comment\n"
    "67 1 253:1 / / rw,relatime shared:1 - ext4 /dev/mapper/fedora-root rw,seclabel\n"
    "35 67 0:6 / /dev rw,nosuid shared:2 - devtmpfs devtmpfs rw,seclabel,size=4096k,nr_inodes=1995515,mode=755,inode64\n"
    "1537 1080 253:1 /usr/share/fonts /run/host/fonts ro,nosuid,nodev,relatime master:1 - ext4 /dev/mapper/fedora-root rw,seclabel\n";
#else
  /* This is an edited /proc/mounts, used because that’s what getmntent() parses
   * (and it can’t parse mountinfo files). Unfortunately this means that
   * non-NULL root_path values are not representable. */
  const char *fake_mtab = "# Some comment\n"
    "/dev/mapper/fedora-root / ext4 rw,relatime,seclabel 0 0\n"
    "devtmpfs /dev devtmpfs rw,nosuid,seclabel,size=4096k,nr_inodes=1995515,mode=755,inode64 0 0\n";
#endif
  const struct
    {
      const char *device_path;
      const char *fs_type;
      const char *mount_path;
      const char *options;
      const char *root_path;  /* if supported */
    }
  expected_entries[] =
    {
      {
        .device_path = "/dev/mapper/fedora-root",
        .fs_type = "ext4",
        .mount_path = "/",
        .options = "rw,relatime,seclabel",
        .root_path = "/",
      },
      {
        .device_path = "devtmpfs",
        .fs_type = "devtmpfs",
        .mount_path = "/dev",
        .options = "rw,nosuid,seclabel,size=4096k,nr_inodes=1995515,mode=755,inode64",
        .root_path = "/",
      },
#ifdef HAVE_LIBMOUNT
      {
        .device_path = "/dev/mapper/fedora-root",
        .fs_type = "ext4",
        .mount_path = "/run/host/fonts",
        .options = "ro,nosuid,nodev,relatime,seclabel",
        .root_path = "/usr/share/fonts",
      },
#endif
    };

  g_test_summary ("Basic test of g_unix_mounts_get_from_file()");

  fd = g_file_open_tmp ("unix-mounts-XXXXXX", &tmp_file, NULL);
  g_assert (fd != -1);
  close (fd);

  res = g_file_set_contents (tmp_file, fake_mtab, -1, NULL);
  g_assert (res);

  entries = g_unix_mounts_get_from_file (tmp_file, &time_read, &n_entries);

  if (entries == NULL)
    {
      /* Some platforms may not support parsing a specific mount entry file */
      g_assert_cmpuint (time_read, ==, 0);
      g_assert_cmpuint (n_entries, ==, 0);
      g_test_skip ("Parsing mount entries from a file not supported on this platform");
      return;
    }

  g_assert_nonnull (entries);

  /* Check the properties of the mount entries. */
  g_assert_cmpuint (n_entries, ==, G_N_ELEMENTS (expected_entries));

  for (size_t i = 0; i < n_entries; i++)
    {
      g_assert_cmpstr (g_unix_mount_get_device_path (entries[i]), ==, expected_entries[i].device_path);
      g_assert_cmpstr (g_unix_mount_get_fs_type (entries[i]), ==, expected_entries[i].fs_type);
      g_assert_cmpstr (g_unix_mount_get_mount_path (entries[i]), ==, expected_entries[i].mount_path);
      g_assert_cmpstr (g_unix_mount_get_options (entries[i]), ==, expected_entries[i].options);

      /* root_path is only supported by libmount */
#ifdef HAVE_LIBMOUNT
      g_assert_cmpstr (g_unix_mount_get_root_path (entries[i]), ==, expected_entries[i].root_path);
#else
      g_assert_null (g_unix_mount_get_root_path (entries[i]));
#endif
    }

  for (size_t i = 0; i < n_entries; i++)
    g_unix_mount_free (entries[i]);
  g_free (entries);
  g_free (tmp_file);
}

G_GNUC_END_IGNORE_DEPRECATIONS

int
main (int   argc,
      char *argv[])
{
  setlocale (LC_ALL, "");

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/unix-mounts/is-system-fs-type", test_is_system_fs_type);
  g_test_add_func ("/unix-mounts/is-system-device-path", test_is_system_device_path);
  g_test_add_func ("/unix-mounts/system-mount-paths-sorted", test_system_mount_paths_sorted);
  g_test_add_func ("/unix-mounts/get-mount-points", test_get_mount_points);
  g_test_add_func ("/unix-mounts/get-mount-entries", test_get_mount_entries);

  return g_test_run ();
}
