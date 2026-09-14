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

/* Prologue {{{1 */

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#ifndef HAVE_SYSCTLBYNAME
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#endif
#ifdef HAVE_POLL
#include <poll.h>
#endif
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <gstdio.h>
#include <dirent.h>

#if defined(__ANDROID__) && (__ANDROID_API__ < 26)
#include <mntent.h>
/* the shared object of recent bionic libc's have hasmntopt symbol, but
   some a possible common build environment for android, termux ends
   up with insufficient __ANDROID_API__ value for building.
*/
extern char* hasmntopt(const struct mntent* mnt, const char* opt);
#endif

#if HAVE_SYS_STATFS_H
#include <sys/statfs.h>
#endif
#if HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif
#if HAVE_SYS_VFS_H
#include <sys/vfs.h>
#elif HAVE_SYS_MOUNT_H
#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <sys/mount.h>
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

#include "gunixmounts.h"
#include "gunixmounts-private.h"
#include "gfile.h"
#include "gfilemonitor.h"
#include "glibintl.h"
#include "glocalfile.h"
#include "gstdio.h"
#include "gthemedicon.h"
#include "gcontextspecificgroup.h"


#ifdef HAVE_MNTENT_H
static const char *_resolve_dev_root (void);
#endif

/**
 * GUnixMountType:
 * @G_UNIX_MOUNT_TYPE_UNKNOWN: Unknown Unix mount type.
 * @G_UNIX_MOUNT_TYPE_FLOPPY: Floppy disk Unix mount type.
 * @G_UNIX_MOUNT_TYPE_CDROM: CDROM Unix mount type.
 * @G_UNIX_MOUNT_TYPE_NFS: Network File System (NFS) Unix mount type.
 * @G_UNIX_MOUNT_TYPE_ZIP: ZIP Unix mount type.
 * @G_UNIX_MOUNT_TYPE_JAZ: JAZZ Unix mount type.
 * @G_UNIX_MOUNT_TYPE_MEMSTICK: Memory Stick Unix mount type.
 * @G_UNIX_MOUNT_TYPE_CF: Compact Flash Unix mount type.
 * @G_UNIX_MOUNT_TYPE_SM: Smart Media Unix mount type.
 * @G_UNIX_MOUNT_TYPE_SDMMC: SD/MMC Unix mount type.
 * @G_UNIX_MOUNT_TYPE_IPOD: iPod Unix mount type.
 * @G_UNIX_MOUNT_TYPE_CAMERA: Digital camera Unix mount type.
 * @G_UNIX_MOUNT_TYPE_HD: Hard drive Unix mount type.
 * 
 * Types of Unix mounts.
 **/
typedef enum {
  G_UNIX_MOUNT_TYPE_UNKNOWN,
  G_UNIX_MOUNT_TYPE_FLOPPY,
  G_UNIX_MOUNT_TYPE_CDROM,
  G_UNIX_MOUNT_TYPE_NFS,
  G_UNIX_MOUNT_TYPE_ZIP,
  G_UNIX_MOUNT_TYPE_JAZ,
  G_UNIX_MOUNT_TYPE_MEMSTICK,
  G_UNIX_MOUNT_TYPE_CF,
  G_UNIX_MOUNT_TYPE_SM,
  G_UNIX_MOUNT_TYPE_SDMMC,
  G_UNIX_MOUNT_TYPE_IPOD,
  G_UNIX_MOUNT_TYPE_CAMERA,
  G_UNIX_MOUNT_TYPE_HD
} GUnixMountType;

struct _GUnixMountEntry {
  char *mount_path;
  char *device_path;
  char *root_path;
  char *filesystem_type;
  char *options;
  gboolean is_read_only;
  gboolean is_system_internal;
};

G_DEFINE_BOXED_TYPE (GUnixMountEntry, g_unix_mount_entry, g_unix_mount_entry_copy, g_unix_mount_entry_free)

struct _GUnixMountPoint {
  char *mount_path;
  char *device_path;
  char *filesystem_type;
  char *options;
  gboolean is_read_only;
  gboolean is_user_mountable;
  gboolean is_loopback;
};

G_DEFINE_BOXED_TYPE (GUnixMountPoint, g_unix_mount_point,
                     g_unix_mount_point_copy, g_unix_mount_point_free)

static GList *_g_get_unix_mounts (void);
static GUnixMountEntry **_g_unix_mounts_get_from_file (const char *table_path,
                                                       uint64_t   *time_read_out,
                                                       size_t     *n_entries_out);
static GList *_g_get_unix_mount_points (void);
static GUnixMountPoint **_g_unix_mount_points_get_from_file (const char *table_path,
                                                             uint64_t   *time_read_out,
                                                             size_t     *n_points_out);
static gboolean proc_mounts_watch_is_running (void);

G_LOCK_DEFINE_STATIC (proc_mounts_source);

/* Protected by proc_mounts_source lock */
static guint64 mount_poller_time = 0;
static GSource *proc_mounts_watch_source = NULL;

#ifdef HAVE_SYS_MNTTAB_H
#define MNTOPT_RO	"ro"
#endif

#ifdef HAVE_MNTENT_H
#include <mntent.h>
#ifdef HAVE_LIBMOUNT
#include <libmount.h>
#endif
#elif defined (HAVE_SYS_MNTTAB_H)
#include <sys/mnttab.h>
#if defined(__sun) && !defined(mnt_opts)
#define mnt_opts mnt_mntopts
#endif
#endif

#ifdef HAVE_SYS_VFSTAB_H
#include <sys/vfstab.h>
#endif

#if defined(HAVE_SYS_MNTCTL_H) && defined(HAVE_SYS_VMOUNT_H) && defined(HAVE_SYS_VFS_H)
#include <sys/mntctl.h>
#include <sys/vfs.h>
#include <sys/vmount.h>
#include <fshelp.h>
#endif

#if (defined(HAVE_GETVFSSTAT) || defined(HAVE_GETFSSTAT) || defined(HAVE_GETFSENT)) && defined(HAVE_FSTAB_H) && defined(HAVE_SYS_MOUNT_H)
#include <sys/param.h>
#include <sys/mount.h>
#include <fstab.h>
#ifdef HAVE_SYS_UCRED_H
#include <sys/ucred.h>
#endif
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#endif

#ifndef HAVE_SETMNTENT
#define setmntent(f,m) g_fopen (f, m)
#endif
#ifndef HAVE_ENDMNTENT
#define endmntent(f) fclose(f)
#endif

#ifdef HAVE_LIBMOUNT
/* Protected by proc_mounts_source lock */
static struct libmnt_monitor *proc_mounts_monitor = NULL;
#endif

static guint64 get_mounts_timestamp (void);
static guint64 get_mount_points_timestamp (void);

static int
compare_str (const char * key,
             const char * const *element)
{
  return strcmp (key, *element);
}

static gboolean
is_in (const char *value, const char *set[], gsize set_size)
{
  return bsearch (value, set, set_size, sizeof (char *), (GCompareFunc)compare_str) != NULL;
}

/* Marked as unused because these are only used on some platform variants, but
 * working out the #if sequence for that would be too much for my little brain. */
static GList *unix_mount_entry_array_free_to_list (GUnixMountEntry **entries,
                                                   size_t            n_entries) G_GNUC_UNUSED;
static GList *unix_mount_point_array_free_to_list (GUnixMountPoint **points,
                                                   size_t            n_points) G_GNUC_UNUSED;

/* Helper to convert to a list for the old API.
 * Steals ownership of the @entries array. */
static GList *
unix_mount_entry_array_free_to_list (GUnixMountEntry **entries,
                                     size_t            n_entries)
{
  GList *l = NULL;

  for (size_t i = 0; i < n_entries; i++)
    l = g_list_prepend (l, g_steal_pointer (&entries[i]));

  g_free (entries);

  return g_list_reverse (l);
}

/* Helper to convert to a list for the old API.
 * Steals ownership of the @entries array. */
static GList *
unix_mount_point_array_free_to_list (GUnixMountPoint **points,
                                     size_t            n_points)
{
  GList *l = NULL;

  for (size_t i = 0; i < n_points; i++)
    l = g_list_prepend (l, g_steal_pointer (&points[i]));

  g_free (points);

  return g_list_reverse (l);
}

/**
 * g_unix_is_mount_path_system_internal:
 * @mount_path: (type filename): a mount path, e.g. `/media/disk` or `/usr`
 *
 * Determines if @mount_path is considered an implementation of the
 * OS.
 *
 * This is primarily used for hiding mountable and mounted volumes
 * that only are used in the OS and has little to no relevance to the
 * casual user.
 *
 * Returns: true if @mount_path is considered an implementation detail
 *    of the OS; false otherwise
 **/
gboolean
g_unix_is_mount_path_system_internal (const char *mount_path)
{
  if (is_in (mount_path, system_mount_paths, G_N_ELEMENTS (system_mount_paths)))
    return TRUE;

  /* Kept separate from sorted list as they may vary */
  if (g_str_equal (GLIB_LOCALSTATEDIR, mount_path) ||
      g_str_equal (GLIB_RUNSTATEDIR, mount_path))
    return TRUE;

  if (g_str_has_prefix (mount_path, "/dev/") ||
      g_str_has_prefix (mount_path, "/proc/") ||
      g_str_has_prefix (mount_path, "/sys/"))
    return TRUE;

  if (g_str_has_suffix (mount_path, "/.gvfs"))
    return TRUE;

  return FALSE;
}

/**
 * g_unix_is_system_fs_type:
 * @fs_type: a file system type, e.g. `procfs` or `tmpfs`
 *
 * Determines if @fs_type is considered a type of file system which is only
 * used in implementation of the OS.
 *
 * This is primarily used for hiding mounted volumes that are intended as APIs
 * for programs to read, and system administrators at a shell; rather than
 * something that should, for example, appear in a GUI. For example, the Linux
 * `/proc` filesystem.
 *
 * The list of file system types considered ‘system’ ones may change over time.
 *
 * Returns: true if @fs_type is considered an implementation detail of the OS;
 *    false otherwise
 * Since: 2.56
 */
gboolean
g_unix_is_system_fs_type (const char *fs_type)
{
  /* keep sorted for bsearch */
  const char *ignore_fs[] = {
    "adfs",
    "afs",
    "auto",
    "autofs",
    "autofs4",
    "cgroup",
    "cgroup2",
    "configfs",
    "cxfs",
    "debugfs",
    "devfs",
    "devpts",
    "devtmpfs",
    "ecryptfs",
    "fdescfs",
    "fuse.gvfsd-fuse",
    "fuse.portal",
    "fusectl",
    "gfs",
    "gfs2",
    "gpfs",
    "hugetlbfs",
    "kernfs",
    "linprocfs",
    "linsysfs",
    "lustre",
    "lustre_lite",
    "mfs",
    "mqueue",
    "ncpfs",
    "nfsd",
    "nullfs",
    "ocfs2",
    "overlay",
    "proc",
    "procfs",
    "pstore",
    "ptyfs",
    "rootfs",
    "rpc_pipefs",
    "securityfs",
    "selinuxfs",
    "sysfs",
    "tmpfs",
    "usbfs",
  };

  g_return_val_if_fail (fs_type != NULL && *fs_type != '\0', FALSE);

  return is_in (fs_type, ignore_fs, G_N_ELEMENTS (ignore_fs));
}

/**
 * g_unix_is_system_device_path:
 * @device_path: a device path, e.g. `/dev/loop0` or `nfsd`
 *
 * Determines if @device_path is considered a block device path which is only
 * used in implementation of the OS.
 *
 * This is primarily used for hiding mounted volumes that are intended as APIs
 * for programs to read, and system administrators at a shell; rather than
 * something that should, for example, appear in a GUI. For example, the Linux
 * `/proc` filesystem.
 *
 * The list of device paths considered ‘system’ ones may change over time.
 *
 * Returns: true if @device_path is considered an implementation detail of
 *    the OS; false otherwise
 * Since: 2.56
 */
gboolean
g_unix_is_system_device_path (const char *device_path)
{
  /* keep sorted for bsearch */
  const char *ignore_devices[] = {
    "/dev/loop",
    "/dev/vn",
    "devpts",
    "nfsd",
    "none",
    "sunrpc",
  };

  g_return_val_if_fail (device_path != NULL && *device_path != '\0', FALSE);

  return is_in (device_path, ignore_devices, G_N_ELEMENTS (ignore_devices));
}

static gboolean
guess_system_internal (const char *mountpoint,
                       const char *fs,
                       const char *device,
                       const char *root)
{
  if (g_unix_is_system_fs_type (fs))
    return TRUE;
  
  if (g_unix_is_system_device_path (device))
    return TRUE;

  if (g_unix_is_mount_path_system_internal (mountpoint))
    return TRUE;

  /* It is not possible to reliably detect mounts which were created by bind
   * operation. mntent-based _g_get_unix_mounts() implementation blindly skips
   * mounts with a device path that is repeated (e.g. mounts created by bind
   * operation, btrfs subvolumes). This usually chooses the most important
   * mounts (i.e. which points to the root of filesystem), but it doesn't work
   * in all cases and also it is not ideal that those mounts are completely
   * ignored (e.g. x-gvfs-show doesn't work for them, trash backend can't handle
   * files on btrfs subvolumes). libmount-based _g_get_unix_mounts()
   * implementation provides a root path. So there is no need to completely
   * ignore those mounts, because e.g. our volume monitors can use the root path
   * to not mengle those mounts with the "regular" mounts (i.e. which points to
   * the root). But because those mounts usually just duplicate other mounts and
   * are completely ignored with mntend-based implementation, let's mark them as
   * system internal. Given the different approaches it doesn't mean that all
   * mounts which were ignored will be system internal now, but this should work
   * in most cases. For more info, see g_unix_mount_entry_get_root_path()
   * annotation, comment in mntent-based _g_get_unix_mounts() implementation and
   * the https://gitlab.gnome.org/GNOME/glib/issues/1271 issue.
   */
  if (root != NULL && g_strcmp0 (root, "/") != 0)
    return TRUE;

  return FALSE;
}

/* GUnixMounts (ie: mtab) implementations {{{1 */

static GUnixMountEntry *
create_unix_mount_entry (const char *device_path,
                         const char *mount_path,
                         const char *root_path,
                         const char *filesystem_type,
                         const char *options,
                         gboolean    is_read_only)
{
  GUnixMountEntry *mount_entry = NULL;

  mount_entry = g_new0 (GUnixMountEntry, 1);
  mount_entry->device_path = g_strdup (device_path);
  mount_entry->mount_path = g_strdup (mount_path);
  mount_entry->root_path = g_strdup (root_path);
  mount_entry->filesystem_type = g_strdup (filesystem_type);
  mount_entry->options = g_strdup (options);
  mount_entry->is_read_only = is_read_only;

  mount_entry->is_system_internal =
    guess_system_internal (mount_entry->mount_path,
                           mount_entry->filesystem_type,
                           mount_entry->device_path,
                           mount_entry->root_path);

  return mount_entry;
}

static GUnixMountPoint *
create_unix_mount_point (const char *device_path,
                         const char *mount_path,
                         const char *filesystem_type,
                         const char *options,
                         gboolean    is_read_only,
                         gboolean    is_user_mountable,
                         gboolean    is_loopback)
{
  GUnixMountPoint *mount_point = NULL;

  mount_point = g_new0 (GUnixMountPoint, 1);
  mount_point->device_path = g_strdup (device_path);
  mount_point->mount_path = g_strdup (mount_path);
  mount_point->filesystem_type = g_strdup (filesystem_type);
  mount_point->options = g_strdup (options);
  mount_point->is_read_only = is_read_only;
  mount_point->is_user_mountable = is_user_mountable;
  mount_point->is_loopback = is_loopback;

  return mount_point;
}

/* mntent.h (Linux, GNU, NSS) {{{2 */
#ifdef HAVE_MNTENT_H

#ifdef HAVE_LIBMOUNT

/* For documentation on /proc/self/mountinfo see
 * http://www.kernel.org/doc/Documentation/filesystems/proc.txt
 */
#define PROC_MOUNTINFO_PATH "/proc/self/mountinfo"

static GUnixMountEntry **
_g_unix_mounts_get_from_file (const char *table_path,
                              uint64_t   *time_read_out,
                              size_t     *n_entries_out)
{
  struct libmnt_table *table = NULL;
  struct libmnt_iter* iter = NULL;
  struct libmnt_fs *fs = NULL;
  GUnixMountEntry *mount_entry = NULL;
  GPtrArray *return_array = NULL;

  if (time_read_out != NULL)
    *time_read_out = get_mounts_timestamp ();

  return_array = g_ptr_array_new_null_terminated (0, (GDestroyNotify) g_unix_mount_entry_free, TRUE);
  table = mnt_new_table ();
  if (mnt_table_parse_mtab (table, table_path) < 0)
    goto out;

  iter = mnt_new_iter (MNT_ITER_FORWARD);
  while (mnt_table_next_fs (table, iter, &fs) == 0)
    {
      const char *device_path = NULL;
      char *mount_options = NULL;
      unsigned long mount_flags = 0;
      gboolean is_read_only = FALSE;

      device_path = mnt_fs_get_source (fs);
      if (g_strcmp0 (device_path, "/dev/root") == 0)
        device_path = _resolve_dev_root ();

      mount_options = mnt_fs_strdup_options (fs);
      if (mount_options)
        {
          mnt_optstr_get_flags (mount_options, &mount_flags, mnt_get_builtin_optmap (MNT_LINUX_MAP));
          g_free (mount_options);
        }
      is_read_only = (mount_flags & MS_RDONLY) ? TRUE : FALSE;

      mount_entry = create_unix_mount_entry (device_path,
                                             mnt_fs_get_target (fs),
                                             mnt_fs_get_root (fs),
                                             mnt_fs_get_fstype (fs),
                                             mnt_fs_get_options (fs),
                                             is_read_only);

      g_ptr_array_add (return_array, g_steal_pointer (&mount_entry));
    }
  mnt_free_iter (iter);

 out:
  mnt_free_table (table);

  if (n_entries_out != NULL)
    *n_entries_out = return_array->len;

  return (GUnixMountEntry **) g_ptr_array_free (g_steal_pointer (&return_array), FALSE);
}

static GList *
_g_get_unix_mounts (void)
{
  GUnixMountEntry **entries = NULL;
  size_t n_entries = 0;

  entries = _g_unix_mounts_get_from_file (NULL  /* default libmount filename */,
                                          NULL, &n_entries);

  return unix_mount_entry_array_free_to_list (g_steal_pointer (&entries), n_entries);
}

#else

static const char *
get_mtab_read_file (void)
{
#ifdef _PATH_MOUNTED
# ifdef __linux__
  return "/proc/mounts";
# else
  return _PATH_MOUNTED;
# endif
#else
  return "/etc/mtab";
#endif
}

#ifndef HAVE_GETMNTENT_R
G_LOCK_DEFINE_STATIC(getmntent);
#endif

static GUnixMountEntry **
_g_unix_mounts_get_from_file (const char *table_path,
                              uint64_t   *time_read_out,
                              size_t     *n_entries_out)
{
#ifdef HAVE_GETMNTENT_R
  struct mntent ent;
  char buf[1024];
#endif
  struct mntent *mntent;
  FILE *file;
  GUnixMountEntry *mount_entry;
  GHashTable *mounts_hash;
  GPtrArray *return_array = NULL;

  if (time_read_out != NULL)
    *time_read_out = get_mounts_timestamp ();

  file = setmntent (table_path, "re");
  if (file == NULL)
    return NULL;

  return_array = g_ptr_array_new_null_terminated (0, (GDestroyNotify) g_unix_mount_entry_free, TRUE);
  mounts_hash = g_hash_table_new (g_str_hash, g_str_equal);
  
#ifdef HAVE_GETMNTENT_R
  while ((mntent = getmntent_r (file, &ent, buf, sizeof (buf))) != NULL)
#else
  G_LOCK (getmntent);
  while ((mntent = getmntent (file)) != NULL)
#endif
    {
      const char *device_path = NULL;
      gboolean is_read_only = FALSE;

      /* ignore any mnt_fsname that is repeated and begins with a '/'
       *
       * We do this to avoid being fooled by --bind mounts, since
       * these have the same device as the location they bind to.
       * It's not an ideal solution to the problem, but it's likely that
       * the most important mountpoint is first and the --bind ones after
       * that aren't as important. So it should work.
       *
       * The '/' is to handle procfs, tmpfs and other no device mounts.
       */
      if (mntent->mnt_fsname != NULL &&
	  mntent->mnt_fsname[0] == '/' &&
	  g_hash_table_lookup (mounts_hash, mntent->mnt_fsname))
        continue;

      if (g_strcmp0 (mntent->mnt_fsname, "/dev/root") == 0)
        device_path = _resolve_dev_root ();
      else
        device_path = mntent->mnt_fsname;

#if defined (HAVE_HASMNTOPT)
      if (hasmntopt (mntent, MNTOPT_RO) != NULL)
	is_read_only = TRUE;
#endif

      mount_entry = create_unix_mount_entry (device_path,
                                             mntent->mnt_dir,
                                             NULL,
                                             mntent->mnt_type,
                                             mntent->mnt_opts,
                                             is_read_only);

      g_hash_table_insert (mounts_hash,
			   mount_entry->device_path,
			   mount_entry->device_path);

      g_ptr_array_add (return_array, g_steal_pointer (&mount_entry));
    }
  g_hash_table_destroy (mounts_hash);
  
  endmntent (file);

#ifndef HAVE_GETMNTENT_R
  G_UNLOCK (getmntent);
#endif
  
  if (n_entries_out != NULL)
    *n_entries_out = return_array->len;

  return (GUnixMountEntry **) g_ptr_array_free (g_steal_pointer (&return_array), FALSE);
}

static GList *
_g_get_unix_mounts (void)
{
  GUnixMountEntry **entries = NULL;
  size_t n_entries = 0;

  entries = _g_unix_mounts_get_from_file (get_mtab_read_file (), NULL, &n_entries);

  return unix_mount_entry_array_free_to_list (g_steal_pointer (&entries), n_entries);
}

#endif /* HAVE_LIBMOUNT */

static const char *
get_mtab_monitor_file (void)
{
  static const char *mountinfo_path = NULL;
#ifdef HAVE_LIBMOUNT
  struct stat buf;
#endif

  if (mountinfo_path != NULL)
    return mountinfo_path;

#ifdef HAVE_LIBMOUNT
  /* The mtab file is still used by some distros, so it has to be monitored in
   * order to avoid races between g_unix_mounts_get and "mounts-changed" signal:
   * https://bugzilla.gnome.org/show_bug.cgi?id=782814
   */
  if (mnt_has_regular_mtab (&mountinfo_path, NULL))
    {
      return mountinfo_path;
    }

  if (stat (PROC_MOUNTINFO_PATH, &buf) == 0)
    {
      mountinfo_path = PROC_MOUNTINFO_PATH;
      return mountinfo_path;
    }
#endif

#ifdef _PATH_MOUNTED
# ifdef __linux__
  mountinfo_path = "/proc/mounts";
# else
  mountinfo_path = _PATH_MOUNTED;
# endif
#else
  mountinfo_path = "/etc/mtab";
#endif

  return mountinfo_path;
}

/* mnttab.h {{{2 */
#elif defined (HAVE_SYS_MNTTAB_H)

G_LOCK_DEFINE_STATIC(getmntent);

static const char *
get_mtab_read_file (void)
{
#ifdef _PATH_MOUNTED
  return _PATH_MOUNTED;
#else	
  return "/etc/mnttab";
#endif
}

static const char *
get_mtab_monitor_file (void)
{
  return get_mtab_read_file ();
}

static GUnixMountEntry **
_g_unix_mounts_get_from_file (const char *table_path,
                              uint64_t   *time_read_out,
                              size_t     *n_entries_out)
{
  struct mnttab mntent;
  FILE *file;
  GUnixMountEntry *mount_entry;
  GPtrArray *return_array = NULL;

  if (time_read_out != NULL)
    *time_read_out = get_mounts_timestamp ();

  file = setmntent (table_path, "re");
  if (file == NULL)
    return NULL;

  return_array = g_ptr_array_new_null_terminated (0, (GDestroyNotify) g_unix_mount_entry_free, TRUE);

  G_LOCK (getmntent);
  while (! getmntent (file, &mntent))
    {
      gboolean is_read_only = FALSE;

#if defined (HAVE_HASMNTOPT)
      if (hasmntopt (&mntent, MNTOPT_RO) != NULL)
	is_read_only = TRUE;
#endif

      mount_entry = create_unix_mount_entry (mntent.mnt_special,
                                             mntent.mnt_mountp,
                                             NULL,
                                             mntent.mnt_fstype,
                                             mntent.mnt_opts,
                                             is_read_only);

      g_ptr_array_add (return_array, g_steal_pointer (&mount_entry));
    }
  
  endmntent (file);
  
  G_UNLOCK (getmntent);

  if (n_entries_out != NULL)
    *n_entries_out = return_array->len;

  return (GUnixMountEntry **) g_ptr_array_free (g_steal_pointer (&return_array), FALSE);
}

static GList *
_g_get_unix_mounts (void)
{
  GUnixMountEntry **entries = NULL;
  size_t n_entries = 0;

  entries = _g_unix_mounts_get_from_file (get_mtab_read_file (), NULL, &n_entries);

  return unix_mount_entry_array_free_to_list (g_steal_pointer (&entries), n_entries);
}

/* mntctl.h (AIX) {{{2 */
#elif defined(HAVE_SYS_MNTCTL_H) && defined(HAVE_SYS_VMOUNT_H) && defined(HAVE_SYS_VFS_H)

static const char *
get_mtab_monitor_file (void)
{
  return NULL;
}

static GList *
_g_get_unix_mounts (void)
{
  struct vfs_ent *fs_info;
  struct vmount *vmount_info;
  int vmount_number;
  unsigned int vmount_size;
  int current;
  GList *return_list;
  
  if (mntctl (MCTL_QUERY, sizeof (vmount_size), &vmount_size) != 0)
    {
      g_warning ("Unable to know the number of mounted volumes");
      
      return NULL;
    }

  vmount_info = (struct vmount*)g_malloc (vmount_size);

  vmount_number = mntctl (MCTL_QUERY, vmount_size, vmount_info);
  
  if (vmount_info->vmt_revision != VMT_REVISION)
    g_warning ("Bad vmount structure revision number, want %d, got %d", VMT_REVISION, vmount_info->vmt_revision);

  if (vmount_number < 0)
    {
      g_warning ("Unable to recover mounted volumes information");
      
      g_free (vmount_info);
      return NULL;
    }
  
  return_list = NULL;
  while (vmount_number > 0)
    {
      gboolean is_read_only = FALSE;

      fs_info = getvfsbytype (vmount_info->vmt_gfstype);

      /* is_removable = (vmount_info->vmt_flags & MNT_REMOVABLE) ? 1 : 0; */
      is_read_only = (vmount_info->vmt_flags & MNT_READONLY) ? 1 : 0;

      mount_entry = create_unix_mount_entry (vmt2dataptr (vmount_info, VMT_OBJECT),
                                             vmt2dataptr (vmount_info, VMT_STUB),
                                             NULL,
                                             fs_info == NULL ? "unknown" : fs_info->vfsent_name,
                                             NULL,
                                             is_read_only);

      return_list = g_list_prepend (return_list, mount_entry);
      
      vmount_info = (struct vmount *)( (char*)vmount_info 
				       + vmount_info->vmt_length);
      vmount_number--;
    }
  
  g_free (vmount_info);
  
  return g_list_reverse (return_list);
}

static GUnixMountEntry **
_g_unix_mounts_get_from_file (const char *table_path,
                              uint64_t   *time_read_out,
                              size_t     *n_entries_out)
{
  /* Not supported on mntctl() systems. */
  if (time_read_out != NULL)
    *time_read_out = 0;
  if (n_entries_out != NULL)
    *n_entries_out = 0;

  return NULL;
}

/* sys/mount.h {{{2 */
#elif (defined(HAVE_GETVFSSTAT) || defined(HAVE_GETFSSTAT)) && defined(HAVE_FSTAB_H) && defined(HAVE_SYS_MOUNT_H)

static const char *
get_mtab_monitor_file (void)
{
  return NULL;
}

static GList *
_g_get_unix_mounts (void)
{
#if defined(USE_STATVFS)
  struct statvfs *mntent = NULL;
#elif defined(USE_STATFS)
  struct statfs *mntent = NULL;
#else
  #error statfs juggling failed
#endif
  size_t bufsize;
  int num_mounts, i;
  GUnixMountEntry *mount_entry;
  GList *return_list;
  
  /* Pass NOWAIT to avoid blocking trying to update NFS mounts. */
#if defined(USE_STATVFS) && defined(HAVE_GETVFSSTAT)
  num_mounts = getvfsstat (NULL, 0, ST_NOWAIT);
#elif defined(USE_STATFS) && defined(HAVE_GETFSSTAT)
  num_mounts = getfsstat (NULL, 0, MNT_NOWAIT);
#endif
  if (num_mounts == -1)
    return NULL;

  bufsize = num_mounts * sizeof (*mntent);
  mntent = g_malloc (bufsize);
#if defined(USE_STATVFS) && defined(HAVE_GETVFSSTAT)
  num_mounts = getvfsstat (mntent, bufsize, ST_NOWAIT);
#elif defined(USE_STATFS) && defined(HAVE_GETFSSTAT)
  num_mounts = getfsstat (mntent, bufsize, MNT_NOWAIT);
#endif
  if (num_mounts == -1)
    return NULL;
  
  return_list = NULL;
  
  for (i = 0; i < num_mounts; i++)
    {
      gboolean is_read_only = FALSE;

#if defined(USE_STATVFS)
      if (mntent[i].f_flag & ST_RDONLY)
#elif defined(USE_STATFS)
      if (mntent[i].f_flags & MNT_RDONLY)
#else
      #error statfs juggling failed
#endif
        is_read_only = TRUE;

      mount_entry = create_unix_mount_entry (mntent[i].f_mntfromname,
                                             mntent[i].f_mntonname,
                                             NULL,
                                             mntent[i].f_fstypename,
                                             NULL,
                                             is_read_only);

      return_list = g_list_prepend (return_list, mount_entry);
    }

  g_free (mntent);
  
  return g_list_reverse (return_list);
}

static GUnixMountEntry **
_g_unix_mounts_get_from_file (const char *table_path,
                              uint64_t   *time_read_out,
                              size_t     *n_entries_out)
{
  /* Not supported on getvfsstat()/getfsstat() systems. */
  if (time_read_out != NULL)
    *time_read_out = 0;
  if (n_entries_out != NULL)
    *n_entries_out = 0;

  return NULL;
}

/* Interix {{{2 */
#elif defined(__INTERIX)

static const char *
get_mtab_monitor_file (void)
{
  return NULL;
}

static GList *
_g_get_unix_mounts (void)
{
  DIR *dirp;
  GList* return_list = NULL;
  char filename[9 + NAME_MAX];

  dirp = opendir ("/dev/fs");
  if (!dirp)
    {
      g_warning ("unable to read /dev/fs!");
      return NULL;
    }

  while (1)
    {
      struct statvfs statbuf;
      struct dirent entry;
      struct dirent* result;
      
      if (readdir_r (dirp, &entry, &result) || result == NULL)
        break;
      
      strcpy (filename, "/dev/fs/");
      strcat (filename, entry.d_name);
      
      if (statvfs (filename, &statbuf) == 0)
        {
          GUnixMountEntry* mount_entry = g_new0(GUnixMountEntry, 1);
          
          mount_entry->mount_path = g_strdup (statbuf.f_mntonname);
          mount_entry->device_path = g_strdup (statbuf.f_mntfromname);
          mount_entry->filesystem_type = g_strdup (statbuf.f_fstypename);
          
          if (statbuf.f_flag & ST_RDONLY)
            mount_entry->is_read_only = TRUE;
          
          return_list = g_list_prepend(return_list, mount_entry);
        }
    }
  
  return_list = g_list_reverse (return_list);
  
  closedir (dirp);

  return return_list;
}

static GUnixMountEntry **
_g_unix_mounts_get_from_file (const char *table_path,
                              uint64_t   *time_read_out,
                              size_t     *n_entries_out)
{
  /* Not supported on Interix systems. */
  if (time_read_out != NULL)
    *time_read_out = 0;
  if (n_entries_out != NULL)
    *n_entries_out = 0;

  return NULL;
}

/* QNX {{{2 */
#elif defined (HAVE_QNX)

static char *
get_mtab_monitor_file (void)
{
  /* TODO: Not implemented */
  return NULL;
}

static GUnixMountEntry **
_g_unix_mounts_get_from_file (const char *table_path,
                              uint64_t   *time_read_out,
                              size_t     *n_entries_out)
{
  /* Not implemented, as per _g_get_unix_mounts() below */
  if (time_read_out != NULL)
    *time_read_out = 0;
  if (n_entries_out != NULL)
    *n_entries_out = 0;

  return NULL;
}

static GList *
_g_get_unix_mounts (void)
{
  /* TODO: Not implemented */
  return NULL;
}

/* Common code {{{2 */
#else
#error No _g_get_unix_mounts() implementation for system
#endif

/* GUnixMountPoints (ie: fstab) implementations {{{1 */

/* _g_get_unix_mount_points():
 * read the fstab.
 * don't return swap and ignore mounts.
 */

static char *
get_fstab_file (void)
{
#ifdef HAVE_LIBMOUNT
  return (char *) mnt_get_fstab_path ();
#else
#if defined(HAVE_SYS_MNTCTL_H) && defined(HAVE_SYS_VMOUNT_H) && defined(HAVE_SYS_VFS_H)
  /* AIX */
  return "/etc/filesystems";
#elif defined(_PATH_MNTTAB)
  return _PATH_MNTTAB;
#elif defined(VFSTAB)
  return VFSTAB;
#else
  return "/etc/fstab";
#endif
#endif
}

/* mntent.h (Linux, GNU, NSS) {{{2 */
#ifdef HAVE_MNTENT_H

#ifdef HAVE_LIBMOUNT

static GUnixMountPoint **
_g_unix_mount_points_get_from_file (const char *table_path,
                                    uint64_t   *time_read_out,
                                    size_t     *n_points_out)
{
  struct libmnt_table *table = NULL;
  struct libmnt_iter* iter = NULL;
  struct libmnt_fs *fs = NULL;
  GUnixMountPoint *mount_point = NULL;
  GPtrArray *return_array = NULL;

  if (time_read_out != NULL)
    *time_read_out = get_mount_points_timestamp ();

  return_array = g_ptr_array_new_null_terminated (0, (GDestroyNotify) g_unix_mount_point_free, TRUE);
  table = mnt_new_table ();
  if (mnt_table_parse_fstab (table, table_path) < 0)
    goto out;

  iter = mnt_new_iter (MNT_ITER_FORWARD);
  while (mnt_table_next_fs (table, iter, &fs) == 0)
    {
      const char *device_path = NULL;
      const char *mount_path = NULL;
      const char *mount_fstype = NULL;
      char *mount_options = NULL;
      gboolean is_read_only = FALSE;
      gboolean is_user_mountable = FALSE;
      gboolean is_loopback = FALSE;

      mount_path = mnt_fs_get_target (fs);
      if ((strcmp (mount_path, "ignore") == 0) ||
          (strcmp (mount_path, "swap") == 0) ||
          (strcmp (mount_path, "none") == 0))
        continue;

      mount_fstype = mnt_fs_get_fstype (fs);
      mount_options = mnt_fs_strdup_options (fs);
      if (mount_options)
        {
          unsigned long mount_flags = 0;
          unsigned long userspace_flags = 0;

          mnt_optstr_get_flags (mount_options, &mount_flags, mnt_get_builtin_optmap (MNT_LINUX_MAP));
          mnt_optstr_get_flags (mount_options, &userspace_flags, mnt_get_builtin_optmap (MNT_USERSPACE_MAP));

          /* We ignore bind fstab entries, as we ignore bind mounts anyway */
          if (mount_flags & MS_BIND)
            {
              g_free (mount_options);
              continue;
            }

          is_read_only = (mount_flags & MS_RDONLY) != 0;
          is_loopback = (userspace_flags & MNT_MS_LOOP) != 0;

          if ((mount_fstype != NULL && g_strcmp0 ("supermount", mount_fstype) == 0) ||
              ((userspace_flags & MNT_MS_USER) &&
               (g_strstr_len (mount_options, -1, "user_xattr") == NULL)) ||
              (userspace_flags & MNT_MS_USERS) ||
              (userspace_flags & MNT_MS_OWNER))
            {
              is_user_mountable = TRUE;
            }
        }

      device_path = mnt_fs_get_source (fs);
      if (g_strcmp0 (device_path, "/dev/root") == 0)
        device_path = _resolve_dev_root ();

      mount_point = create_unix_mount_point (device_path,
                                             mount_path,
                                             mount_fstype,
                                             mount_options,
                                             is_read_only,
                                             is_user_mountable,
                                             is_loopback);
      if (mount_options)
        g_free (mount_options);

      g_ptr_array_add (return_array, g_steal_pointer (&mount_point));
    }
  mnt_free_iter (iter);

 out:
  mnt_free_table (table);

  if (n_points_out != NULL)
    *n_points_out = return_array->len;

  return (GUnixMountPoint **) g_ptr_array_free (g_steal_pointer (&return_array), FALSE);
}

static GList *
_g_get_unix_mount_points (void)
{
  GUnixMountPoint **points = NULL;
  size_t n_points = 0;

  points = _g_unix_mount_points_get_from_file (NULL  /* default libmount filename */,
                                               NULL, &n_points);

  return unix_mount_point_array_free_to_list (g_steal_pointer (&points), n_points);
}

#else

static GUnixMountPoint **
_g_unix_mount_points_get_from_file (const char *table_path,
                                    uint64_t   *time_read_out,
                                    size_t     *n_points_out)
{
#ifdef HAVE_GETMNTENT_R
  struct mntent ent;
  char buf[1024];
#endif
  struct mntent *mntent;
  FILE *file;
  GUnixMountPoint *mount_point;
  GPtrArray *return_array = NULL;

  if (time_read_out != NULL)
    *time_read_out = get_mount_points_timestamp ();

  file = setmntent (table_path, "re");
  if (file == NULL)
    {
      if (n_points_out != NULL)
        *n_points_out = 0;
      return NULL;
    }

  return_array = g_ptr_array_new_null_terminated (0, (GDestroyNotify) g_unix_mount_point_free, TRUE);

#ifdef HAVE_GETMNTENT_R
  while ((mntent = getmntent_r (file, &ent, buf, sizeof (buf))) != NULL)
#else
  G_LOCK (getmntent);
  while ((mntent = getmntent (file)) != NULL)
#endif
    {
      const char *device_path = NULL;
      gboolean is_read_only = FALSE;
      gboolean is_user_mountable = FALSE;
      gboolean is_loopback = FALSE;

      if ((strcmp (mntent->mnt_dir, "ignore") == 0) ||
          (strcmp (mntent->mnt_dir, "swap") == 0) ||
          (strcmp (mntent->mnt_dir, "none") == 0))
	continue;

#ifdef HAVE_HASMNTOPT
      /* We ignore bind fstab entries, as we ignore bind mounts anyway */
      if (hasmntopt (mntent, "bind"))
        continue;
#endif

      if (strcmp (mntent->mnt_fsname, "/dev/root") == 0)
        device_path = _resolve_dev_root ();
      else
        device_path = mntent->mnt_fsname;

#ifdef HAVE_HASMNTOPT
      if (hasmntopt (mntent, MNTOPT_RO) != NULL)
	is_read_only = TRUE;

      if (hasmntopt (mntent, "loop") != NULL)
	is_loopback = TRUE;

#endif

      if ((mntent->mnt_type != NULL && strcmp ("supermount", mntent->mnt_type) == 0)
#ifdef HAVE_HASMNTOPT
	  || (hasmntopt (mntent, "user") != NULL
	      && hasmntopt (mntent, "user") != hasmntopt (mntent, "user_xattr"))
	  || hasmntopt (mntent, "users") != NULL
	  || hasmntopt (mntent, "owner") != NULL
#endif
	  )
	is_user_mountable = TRUE;

      mount_point = create_unix_mount_point (device_path,
                                             mntent->mnt_dir,
                                             mntent->mnt_type,
                                             mntent->mnt_opts,
                                             is_read_only,
                                             is_user_mountable,
                                             is_loopback);

      g_ptr_array_add (return_array, g_steal_pointer (&mount_point));
    }
  
  endmntent (file);

#ifndef HAVE_GETMNTENT_R
  G_UNLOCK (getmntent);
#endif

  if (n_points_out != NULL)
    *n_points_out = return_array->len;

  return (GUnixMountPoint **) g_ptr_array_free (g_steal_pointer (&return_array), FALSE);
}

static GList *
_g_get_unix_mount_points (void)
{
  GUnixMountPoint **points = NULL;
  size_t n_points = 0;

  points = _g_unix_mount_points_get_from_file (get_fstab_file (),
                                               NULL, &n_points);

  return unix_mount_point_array_free_to_list (g_steal_pointer (&points), n_points);
}

#endif /* HAVE_LIBMOUNT */

/* mnttab.h {{{2 */
#elif defined (HAVE_SYS_MNTTAB_H)

static GUnixMountPoint **
_g_unix_mount_points_get_from_file (const char *table_path,
                                    uint64_t   *time_read_out,
                                    size_t     *n_points_out)
{
  struct mnttab mntent;
  FILE *file;
  GUnixMountPoint *mount_point;
  GPtrArray *return_array = NULL;

  if (time_read_out != NULL)
    *time_read_out = get_mount_points_timestamp ();

  file = setmntent (table_path, "re");
  if (file == NULL)
    {
      if (n_points_out != NULL)
        *n_points_out = 0;
      return NULL;
    }

  return_array = g_ptr_array_new_null_terminated (0, (GDestroyNotify) g_unix_mount_point_free, TRUE);
  
  G_LOCK (getmntent);
  while (! getmntent (file, &mntent))
    {
      gboolean is_read_only = FALSE;
      gboolean is_user_mountable = FALSE;
      gboolean is_loopback = FALSE;

      if ((strcmp (mntent.mnt_mountp, "ignore") == 0) ||
          (strcmp (mntent.mnt_mountp, "swap") == 0) ||
          (strcmp (mntent.mnt_mountp, "none") == 0))
	continue;

#ifdef HAVE_HASMNTOPT
      if (hasmntopt (&mntent, MNTOPT_RO) != NULL)
	is_read_only = TRUE;

      if (hasmntopt (&mntent, "lofs") != NULL)
	is_loopback = TRUE;
#endif

      if ((mntent.mnt_fstype != NULL)
#ifdef HAVE_HASMNTOPT
	  || (hasmntopt (&mntent, "user") != NULL
	      && hasmntopt (&mntent, "user") != hasmntopt (&mntent, "user_xattr"))
	  || hasmntopt (&mntent, "users") != NULL
	  || hasmntopt (&mntent, "owner") != NULL
#endif
	  )
	is_user_mountable = TRUE;

      mount_point = create_unix_mount_point (mntent.mnt_special,
                                             mntent.mnt_mountp,
                                             mntent.mnt_fstype,
                                             mntent.mnt_mntopts,
                                             is_read_only,
                                             is_user_mountable,
                                             is_loopback);

      g_ptr_array_add (return_array, g_steal_pointer (&mount_point));
    }
  
  endmntent (file);
  G_UNLOCK (getmntent);

  if (n_points_out != NULL)
    *n_points_out = return_array->len;

  return (GUnixMountPoint **) g_ptr_array_free (g_steal_pointer (&return_array), FALSE);
}

static GList *
_g_get_unix_mount_points (void)
{
  GUnixMountPoint **points = NULL;
  size_t n_points = 0;

  points = _g_unix_mount_points_get_from_file (get_fstab_file (),
                                               NULL, &n_points);

  return unix_mount_point_array_free_to_list (g_steal_pointer (&points), n_points);
}

/* mntctl.h (AIX) {{{2 */
#elif defined(HAVE_SYS_MNTCTL_H) && defined(HAVE_SYS_VMOUNT_H) && defined(HAVE_SYS_VFS_H)

/* functions to parse /etc/filesystems on aix */

/* read character, ignoring comments (begin with '*', end with '\n' */
static int
aix_fs_getc (FILE *fd)
{
  int c;
  
  while ((c = getc (fd)) == '*')
    {
      while (((c = getc (fd)) != '\n') && (c != EOF))
	;
    }
}

/* eat all continuous spaces in a file */
static int
aix_fs_ignorespace (FILE *fd)
{
  int c;
  
  while ((c = aix_fs_getc (fd)) != EOF)
    {
      if (!g_ascii_isspace (c))
	{
	  ungetc (c,fd);
	  return c;
	}
    }
  
  return EOF;
}

/* read one word from file */
static int
aix_fs_getword (FILE *fd, 
                char *word)
{
  int c;
  
  aix_fs_ignorespace (fd);

  while (((c = aix_fs_getc (fd)) != EOF) && !g_ascii_isspace (c))
    {
      if (c == '"')
	{
	  while (((c = aix_fs_getc (fd)) != EOF) && (c != '"'))
	    *word++ = c;
	  else
	    *word++ = c;
	}
    }
  *word = 0;
  
  return c;
}

typedef struct {
  char mnt_mount[PATH_MAX];
  char mnt_special[PATH_MAX];
  char mnt_fstype[16];
  char mnt_options[128];
} AixMountTableEntry;

/* read mount points properties */
static int
aix_fs_get (FILE               *fd, 
            AixMountTableEntry *prop)
{
  static char word[PATH_MAX] = { 0 };
  char value[PATH_MAX];
  
  /* read stanza */
  if (word[0] == 0)
    {
      if (aix_fs_getword (fd, word) == EOF)
	return EOF;
    }

  word[strlen(word) - 1] = 0;
  g_strlcpy (prop->mnt_mount, word, sizeof (prop->mnt_mount));
  
  /* read attributes and value */
  
  while (aix_fs_getword (fd, word) != EOF)
    {
      /* test if is attribute or new stanza */
      if (word[strlen(word) - 1] == ':')
	return 0;
      
      /* read "=" */
      aix_fs_getword (fd, value);
      
      /* read value */
      aix_fs_getword (fd, value);
      
      if (strcmp (word, "dev") == 0)
	g_strlcpy (prop->mnt_special, value, sizeof (prop->mnt_special));
      else if (strcmp (word, "vfs") == 0)
	g_strlcpy (prop->mnt_fstype, value, sizeof (prop->mnt_fstype));
      else if (strcmp (word, "options") == 0)
	g_strlcpy (prop->mnt_options, value, sizeof (prop->mnt_options));
    }
  
  return 0;
}

static GUnixMountPoint **
_g_unix_mount_points_get_from_file (const char *table_path,
                                    uint64_t   *time_read_out,
                                    size_t     *n_points_out)
{
  struct mntent *mntent;
  FILE *file;
  GUnixMountPoint *mount_point;
  AixMountTableEntry mntent;
  GPtrArray *return_array = NULL;

  if (time_read_out != NULL)
    *time_read_out = get_mount_points_timestamp ();

  file = setmntent (table_path, "re");
  if (file == NULL)
    {
      if (n_points_out != NULL)
        *n_points_out = 0;
      return NULL;
    }

  return_array = g_ptr_array_new_null_terminated (0, (GDestroyNotify) g_unix_mount_point_free, TRUE);

  while (!aix_fs_get (file, &mntent))
    {
      if (strcmp ("cdrfs", mntent.mnt_fstype) == 0)
	{
          mount_point = create_unix_mount_point (mntent.mnt_special,
                                                 mntent.mnt_mount,
                                                 mntent.mnt_fstype,
                                                 mntent.mnt_options,
                                                 TRUE,
                                                 TRUE,
                                                 FALSE);

          g_ptr_array_add (return_array, g_steal_pointer (&mount_point));
	}
    }
	
  endmntent (file);

  if (n_points_out != NULL)
    *n_points_out = return_array->len;

  return (GUnixMountPoint **) g_ptr_array_free (g_steal_pointer (&return_array), FALSE);
}

static GList *
_g_get_unix_mount_points (void)
{
  GUnixMountPoint **points = NULL;
  size_t n_points = 0;

  points = _g_unix_mount_points_get_from_file (get_fstab_file (),
                                               NULL, &n_points);

  return unix_mount_point_array_free_to_list (g_steal_pointer (&points), n_points);
}

#elif (defined(HAVE_GETVFSSTAT) || defined(HAVE_GETFSSTAT) || defined(HAVE_GETFSENT)) && defined(HAVE_FSTAB_H) && defined(HAVE_SYS_MOUNT_H)

static GList *
_g_get_unix_mount_points (void)
{
  struct fstab *fstab = NULL;
  GUnixMountPoint *mount_point;
  GList *return_list = NULL;
  G_LOCK_DEFINE_STATIC (fsent);
#ifdef HAVE_SYS_SYSCTL_H
  uid_t uid = getuid ();
  int usermnt = 0;
  struct stat sb;
#endif

#ifdef HAVE_SYS_SYSCTL_H
#if defined(HAVE_SYSCTLBYNAME)
  {
    size_t len = sizeof(usermnt);

    sysctlbyname ("vfs.usermount", &usermnt, &len, NULL, 0);
  }
#elif defined(CTL_VFS) && defined(VFS_USERMOUNT)
  {
    int mib[2];
    size_t len = sizeof(usermnt);
    
    mib[0] = CTL_VFS;
    mib[1] = VFS_USERMOUNT;
    sysctl (mib, 2, &usermnt, &len, NULL, 0);
  }
#elif defined(CTL_KERN) && defined(KERN_USERMOUNT)
  {
    int mib[2];
    size_t len = sizeof(usermnt);
    
    mib[0] = CTL_KERN;
    mib[1] = KERN_USERMOUNT;
    sysctl (mib, 2, &usermnt, &len, NULL, 0);
  }
#endif
#endif

  G_LOCK (fsent);
  if (!setfsent ())
    {
      G_UNLOCK (fsent);
      return NULL;
    }

  while ((fstab = getfsent ()) != NULL)
    {
      gboolean is_read_only = FALSE;
      gboolean is_user_mountable = FALSE;

      if (strcmp (fstab->fs_vfstype, "swap") == 0)
	continue;

      if (strcmp (fstab->fs_type, "ro") == 0)
	is_read_only = TRUE;

#ifdef HAVE_SYS_SYSCTL_H
      if (usermnt != 0)
        {
          if (uid == 0 ||
              (stat (fstab->fs_file, &sb) == 0 && sb.st_uid == uid))
            {
              is_user_mountable = TRUE;
            }
        }
#endif

      mount_point = create_unix_mount_point (fstab->fs_spec,
                                             fstab->fs_file,
                                             fstab->fs_vfstype,
                                             fstab->fs_mntops,
                                             is_read_only,
                                             is_user_mountable,
                                             FALSE);

      return_list = g_list_prepend (return_list, mount_point);
    }

  endfsent ();
  G_UNLOCK (fsent);

  return g_list_reverse (return_list);
}

static GUnixMountPoint **
_g_unix_mount_points_get_from_file (const char *table_path,
                                    uint64_t   *time_read_out,
                                    size_t     *n_points_out)
{
  /* Not supported on getfsent() systems. */
  if (time_read_out != NULL)
    *time_read_out = 0;
  if (n_points_out != NULL)
    *n_points_out = 0;
  return NULL;
}

/* Common code {{{2 */
#else
#error No g_get_mount_table() implementation for system
#endif

static guint64
get_mounts_timestamp (void)
{
  const char *monitor_file;
  struct stat buf;
  guint64 timestamp = 0;

  G_LOCK (proc_mounts_source);

  monitor_file = get_mtab_monitor_file ();
  /* Don't return mtime for /proc/ files */
  if (monitor_file && !g_str_has_prefix (monitor_file, "/proc/"))
    {
      if (stat (monitor_file, &buf) == 0)
        timestamp = buf.st_mtime;
    }
  else if (proc_mounts_watch_is_running ())
    {
      /* it's being monitored by poll, so return mount_poller_time */
      timestamp = mount_poller_time;
    }
  else
    {
      /* Case of /proc/ file not being monitored - Be on the safe side and
       * send a new timestamp to force g_unix_mount_entries_changed_since() to
       * return TRUE so any application caches depending on it (like eg.
       * the one in GIO) get invalidated and don't hold possibly outdated
       * data - see Bug 787731 */
     timestamp = g_get_monotonic_time ();
    }

  G_UNLOCK (proc_mounts_source);

  return timestamp;
}

static guint64
get_mount_points_timestamp (void)
{
  const char *monitor_file;
  struct stat buf;

  monitor_file = get_fstab_file ();
  if (monitor_file)
    {
      if (stat (monitor_file, &buf) == 0)
        return (guint64)buf.st_mtime;
    }
  return 0;
}

/**
 * g_unix_mounts_get:
 * @time_read: (out) (optional): return location for a timestamp
 *
 * Gets a list of [struct@GioUnix.MountEntry] instances representing the Unix
 * mounts.
 *
 * If @time_read is set, it will be filled with the mount timestamp, allowing
 * for checking if the mounts have changed with
 * [func@GioUnix.mount_entries_changed_since].
 *
 * Returns: (element-type GUnixMountEntry) (transfer full): a list of the
 *    Unix mounts
 * Deprecated: 2.84: Use [func@GioUnix.mount_entries_get] instead.
 */
GList *
g_unix_mounts_get (guint64 *time_read)
{
  return g_unix_mount_entries_get (time_read);
}

/**
 * g_unix_mount_entries_get:
 * @time_read: (out) (optional): return location for a timestamp
 *
 * Gets a list of [struct@GioUnix.MountEntry] instances representing the Unix
 * mounts.
 *
 * If @time_read is set, it will be filled with the mount timestamp, allowing
 * for checking if the mounts have changed with
 * [func@GioUnix.mount_entries_changed_since].
 *
 * Returns: (element-type GUnixMountEntry) (transfer full): a list of the
 *    Unix mounts
 * Since: 2.84
 */
GList *
g_unix_mount_entries_get (guint64 *time_read)
{
  if (time_read)
    *time_read = get_mounts_timestamp ();

  return _g_get_unix_mounts ();
}

/**
 * g_unix_mounts_get_from_file:
 * @table_path: path to the mounts table file (for example `/proc/self/mountinfo`)
 * @time_read_out: (optional) (out caller-allocates): return location for the
 *   modification time of @table_path
 * @n_entries_out: (optional) (out caller-allocates): return location for the
 *   number of mount entries returned
 *
 * Gets an array of [struct@GioUnix.MountEntry]s containing the Unix mounts
 * listed in @table_path.
 *
 * This is a generalized version of [func@GioUnix.mount_entries_get], mainly
 * intended for internal testing use. Note that [func@GioUnix.mount_entries_get]
 * may parse multiple hierarchical table files, so this function is not a direct
 * superset of its functionality.
 *
 * If there is an error reading or parsing the file, `NULL` will be returned
 * and both out parameters will be set to `0`.
 *
 * Returns: (transfer full) (array length=n_entries_out) (nullable): mount
 *   entries, or `NULL` if there was an error loading them
 * Since: 2.82
 * Deprecated: 2.84: Use [func@GioUnix.mount_entries_get_from_file] instead.
 */
GUnixMountEntry **
g_unix_mounts_get_from_file (const char *table_path,
                             uint64_t   *time_read_out,
                             size_t     *n_entries_out)
{
  return g_unix_mount_entries_get_from_file (table_path, time_read_out, n_entries_out);
}

/**
 * g_unix_mount_entries_get_from_file:
 * @table_path: path to the mounts table file (for example `/proc/self/mountinfo`)
 * @time_read_out: (optional) (out caller-allocates): return location for the
 *   modification time of @table_path
 * @n_entries_out: (optional) (out caller-allocates): return location for the
 *   number of mount entries returned
 *
 * Gets an array of [struct@GioUnix.MountEntry]s containing the Unix mounts
 * listed in @table_path.
 *
 * This is a generalized version of [func@GioUnix.mount_entries_get], mainly
 * intended for internal testing use. Note that [func@GioUnix.mount_entries_get]
 * may parse multiple hierarchical table files, so this function is not a direct
 * superset of its functionality.
 *
 * If there is an error reading or parsing the file, `NULL` will be returned
 * and both out parameters will be set to `0`.
 *
 * Returns: (transfer full) (array length=n_entries_out) (nullable): mount
 *   entries, or `NULL` if there was an error loading them
 * Since: 2.84
 */
GUnixMountEntry **
g_unix_mount_entries_get_from_file (const char *table_path,
                                    uint64_t   *time_read_out,
                                    size_t     *n_entries_out)
{
  return _g_unix_mounts_get_from_file (table_path, time_read_out, n_entries_out);
}

/**
 * g_unix_mount_at:
 * @mount_path: (type filename): path for a possible Unix mount
 * @time_read: (out) (optional): return location for a timestamp
 * 
 * Gets a [struct@GioUnix.MountEntry] for a given mount path.
 *
 * If @time_read is set, it will be filled with a Unix timestamp for checking
 * if the mounts have changed since with
 * [func@GioUnix.mount_entries_changed_since].
 * 
 * If more mounts have the same mount path, the last matching mount
 * is returned.
 *
 * This will return `NULL` if there is no mount point at @mount_path.
 *
 * Returns: (transfer full) (nullable): a [struct@GioUnix.MountEntry]
 * Deprecated: 2.84: Use [func@GioUnix.MountEntry.at] instead.
 **/
GUnixMountEntry *
g_unix_mount_at (const char *mount_path,
		 guint64    *time_read)
{
  return g_unix_mount_entry_at (mount_path, time_read);
}

/**
 * g_unix_mount_entry_at:
 * @mount_path: (type filename): path for a possible Unix mount
 * @time_read: (out) (optional): return location for a timestamp
 *
 * Gets a [struct@GioUnix.MountEntry] for a given mount path.
 *
 * If @time_read is set, it will be filled with a Unix timestamp for checking
 * if the mounts have changed since with
 * [func@GioUnix.mount_entries_changed_since].
 *
 * If more mounts have the same mount path, the last matching mount
 * is returned.
 *
 * This will return `NULL` if there is no mount point at @mount_path.
 *
 * Returns: (transfer full) (nullable): a [struct@GioUnix.MountEntry]
 * Since: 2.84
 **/
GUnixMountEntry *
g_unix_mount_entry_at (const char *mount_path,
		       guint64    *time_read)
{
  GList *mounts, *l;
  GUnixMountEntry *mount_entry, *found;
  
  mounts = g_unix_mount_entries_get (time_read);

  found = NULL;
  for (l = mounts; l != NULL; l = l->next)
    {
      mount_entry = l->data;

      if (strcmp (mount_path, mount_entry->mount_path) == 0)
        {
          if (found != NULL)
            g_unix_mount_entry_free (found);

          found = mount_entry;
        }
      else
        g_unix_mount_entry_free (mount_entry);
    }
  g_list_free (mounts);

  return found;
}

/**
 * g_unix_mount_for:
 * @file_path: (type filename): file path on some Unix mount
 * @time_read: (out) (optional): return location for a timestamp
 *
 * Gets a [struct@GioUnix.MountEntry] for a given file path.
 *
 * If @time_read is set, it will be filled with a Unix timestamp for checking
 * if the mounts have changed since with
 * [func@GioUnix.mount_entries_changed_since].
 *
 * If more mounts have the same mount path, the last matching mount
 * is returned.
 *
 * This will return `NULL` if looking up the mount entry fails, if
 * @file_path doesn’t exist or there is an I/O error.
 *
 * Returns: (transfer full)  (nullable): a [struct@GioUnix.MountEntry]
 * Since: 2.52
 * Deprecated: 2.84: Use [func@GioUnix.MountEntry.for] instead.
 **/
GUnixMountEntry *
g_unix_mount_for (const char *file_path,
                  guint64    *time_read)
{
  return g_unix_mount_entry_for (file_path, time_read);
}

/**
 * g_unix_mount_entry_for:
 * @file_path: (type filename): file path on some Unix mount
 * @time_read: (out) (optional): return location for a timestamp
 *
 * Gets a [struct@GioUnix.MountEntry] for a given file path.
 *
 * If @time_read is set, it will be filled with a Unix timestamp for checking
 * if the mounts have changed since with
 * [func@GioUnix.mount_entries_changed_since].
 *
 * If more mounts have the same mount path, the last matching mount
 * is returned.
 *
 * This will return `NULL` if looking up the mount entry fails, if
 * @file_path doesn’t exist or there is an I/O error.
 *
 * Returns: (transfer full)  (nullable): a [struct@GioUnix.MountEntry]
 * Since: 2.84
 **/
GUnixMountEntry *
g_unix_mount_entry_for (const char *file_path,
                        guint64    *time_read)
{
  GUnixMountEntry *entry;

  g_return_val_if_fail (file_path != NULL, NULL);

  entry = g_unix_mount_entry_at (file_path, time_read);
  if (entry == NULL)
    {
      char *topdir;

      topdir = _g_local_file_find_topdir_for (file_path);
      if (topdir != NULL)
        {
          entry = g_unix_mount_entry_at (topdir, time_read);
          g_free (topdir);
        }
    }

  return entry;
}

static gpointer
copy_mount_point_cb (gconstpointer src,
                     gpointer      data)
{
  GUnixMountPoint *src_mount_point = (GUnixMountPoint *) src;
  return g_unix_mount_point_copy (src_mount_point);
}

/**
 * g_unix_mount_points_get:
 * @time_read: (out) (optional): return location for a timestamp
 *
 * Gets a list of [struct@GioUnix.MountPoint] instances representing the Unix
 * mount points.
 *
 * If @time_read is set, it will be filled with the mount timestamp, allowing
 * for checking if the mounts have changed with
 * [func@GioUnix.mount_points_changed_since].
 *
 * Returns: (element-type GUnixMountPoint) (transfer full): a list of the Unix
 *    mount points
 **/
GList *
g_unix_mount_points_get (guint64 *time_read)
{
  static GList *mnt_pts_last = NULL;
  static guint64 time_read_last = 0;
  GList *mnt_pts = NULL;
  guint64 time_read_now;
  G_LOCK_DEFINE_STATIC (unix_mount_points);

  G_LOCK (unix_mount_points);

  time_read_now = get_mount_points_timestamp ();
  if (time_read_now != time_read_last || mnt_pts_last == NULL)
    {
      time_read_last = time_read_now;
      g_list_free_full (mnt_pts_last, (GDestroyNotify) g_unix_mount_point_free);
      mnt_pts_last = _g_get_unix_mount_points ();
    }
  mnt_pts = g_list_copy_deep (mnt_pts_last, copy_mount_point_cb, NULL);

  G_UNLOCK (unix_mount_points);

  if (time_read)
    *time_read = time_read_now;

  return mnt_pts;
}

/**
 * g_unix_mount_points_get_from_file:
 * @table_path: path to the mount points table file (for example `/etc/fstab`)
 * @time_read_out: (optional) (out caller-allocates): return location for the
 *   modification time of @table_path
 * @n_points_out: (optional) (out caller-allocates): return location for the
 *   number of mount points returned
 *
 * Gets an array of [struct@GioUnix.MountPoint]s containing the Unix mount
 * points listed in @table_path.
 *
 * This is a generalized version of [func@GioUnix.mount_points_get], mainly
 * intended for internal testing use. Note that [func@GioUnix.mount_points_get]
 * may parse multiple hierarchical table files, so this function is not a direct
 * superset of its functionality.
 *
 * If there is an error reading or parsing the file, `NULL` will be returned
 * and both out parameters will be set to `0`.
 *
 * Returns: (transfer full) (array length=n_points_out) (nullable): mount
 *   points, or `NULL` if there was an error loading them
 * Since: 2.82
 */
GUnixMountPoint **
g_unix_mount_points_get_from_file (const char *table_path,
                                   uint64_t   *time_read_out,
                                   size_t     *n_points_out)
{
  return _g_unix_mount_points_get_from_file (table_path, time_read_out, n_points_out);
}

/**
 * g_unix_mount_point_at:
 * @mount_path: (type filename): path for a possible Unix mount point
 * @time_read: (out) (optional): return location for a timestamp
 *
 * Gets a [struct@GioUnix.MountPoint] for a given mount path.
 *
 * If @time_read is set, it will be filled with a Unix timestamp for checking if
 * the mount points have changed since with
 * [func@GioUnix.mount_points_changed_since].
 *
 * If more mount points have the same mount path, the last matching mount point
 * is returned.
 *
 * Returns: (transfer full) (nullable): a [struct@GioUnix.MountPoint], or `NULL`
 *    if no match is found
 * Since: 2.66
 **/
GUnixMountPoint *
g_unix_mount_point_at (const char *mount_path,
                       guint64    *time_read)
{
  GList *mount_points, *l;
  GUnixMountPoint *mount_point, *found;

  mount_points = g_unix_mount_points_get (time_read);

  found = NULL;
  for (l = mount_points; l != NULL; l = l->next)
    {
      mount_point = l->data;

      if (strcmp (mount_path, mount_point->mount_path) == 0)
        {
          if (found != NULL)
            g_unix_mount_point_free (found);

          found = mount_point;
        }
      else
        g_unix_mount_point_free (mount_point);
    }
  g_list_free (mount_points);

  return found;
}

/**
 * g_unix_mounts_changed_since:
 * @time: a timestamp
 * 
 * Checks if the Unix mounts have changed since a given Unix time.
 * 
 * Returns: true if the mounts have changed since @time; false otherwise
 * Deprecated: 2.84: Use [func@GioUnix.mount_entries_changed_since] instead.
 **/
gboolean
g_unix_mounts_changed_since (guint64 time)
{
  return g_unix_mount_entries_changed_since (time);
}

/**
 * g_unix_mount_entries_changed_since:
 * @time: a timestamp
 *
 * Checks if the Unix mounts have changed since a given Unix time.
 *
 * This can only work reliably if a [class@GioUnix.MountMonitor] is running in
 * the process, otherwise changes in the mount entries file (such as
 * `/proc/self/mountinfo` on Linux) cannot be detected and, as a result, this
 * function has to conservatively always return `TRUE`.
 *
 * It is more efficient to use [signal@GioUnix.MountMonitor::mounts-changed] to
 * be signalled of changes to the mount entries, rather than polling using this
 * function. This function is more appropriate for infrequently determining
 * cache validity.
 *
 * Returns: true if the mounts have changed since @time; false otherwise
 * Since 2.84
 **/
gboolean
g_unix_mount_entries_changed_since (guint64 time)
{
  return get_mounts_timestamp () != time;
}

/**
 * g_unix_mount_points_changed_since:
 * @time: a timestamp
 * 
 * Checks if the Unix mount points have changed since a given Unix time.
 * 
 * Unlike [func@GioUnix.mount_entries_changed_since], this function can work
 * reliably without a [class@GioUnix.MountMonitor] running, as it accesses the
 * static mount point information (such as `/etc/fstab` on Linux), which has a
 * valid modification time.
 *
 * It is more efficient to use [signal@GioUnix.MountMonitor::mountpoints-changed]
 * to be signalled of changes to the mount points, rather than polling using
 * this function. This function is more appropriate for infrequently determining
 * cache validity.
 *
 * Returns: true if the mount points have changed since @time; false otherwise
 **/
gboolean
g_unix_mount_points_changed_since (guint64 time)
{
  return get_mount_points_timestamp () != time;
}

/* GUnixMountMonitor {{{1 */

enum {
  MOUNTS_CHANGED,
  MOUNTPOINTS_CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

struct _GUnixMountMonitor {
  GObject parent;

  GMainContext *context;
};

struct _GUnixMountMonitorClass {
  GObjectClass parent_class;
};


G_DEFINE_TYPE (GUnixMountMonitor, g_unix_mount_monitor, G_TYPE_OBJECT)

static GContextSpecificGroup  mount_monitor_group;
static GFileMonitor          *fstab_monitor;
static GFileMonitor          *mtab_monitor;
static GList                 *mount_poller_mounts;
static guint                  mtab_file_changed_id;

/* Called with proc_mounts_source lock held. */
static gboolean
proc_mounts_watch_is_running (void)
{
  return proc_mounts_watch_source != NULL &&
         !g_source_is_destroyed (proc_mounts_watch_source);
}

static void
fstab_file_changed (GFileMonitor      *monitor,
                    GFile             *file,
                    GFile             *other_file,
                    GFileMonitorEvent  event_type,
                    gpointer           user_data)
{
  if (event_type != G_FILE_MONITOR_EVENT_CHANGED &&
      event_type != G_FILE_MONITOR_EVENT_CREATED &&
      event_type != G_FILE_MONITOR_EVENT_DELETED)
    return;

  g_context_specific_group_emit (&mount_monitor_group, signals[MOUNTPOINTS_CHANGED]);
}

static gboolean
mtab_file_changed_cb (gpointer user_data)
{
  mtab_file_changed_id = 0;
  g_context_specific_group_emit (&mount_monitor_group, signals[MOUNTS_CHANGED]);

  return G_SOURCE_REMOVE;
}

static void
mtab_file_changed (GFileMonitor      *monitor,
                   GFile             *file,
                   GFile             *other_file,
                   GFileMonitorEvent  event_type,
                   gpointer           user_data)
{
  GMainContext *context;
  GSource *source;

  if (event_type != G_FILE_MONITOR_EVENT_CHANGED &&
      event_type != G_FILE_MONITOR_EVENT_CREATED &&
      event_type != G_FILE_MONITOR_EVENT_DELETED)
    return;

  /* Skip accumulated events from file monitor which we are not able to handle
   * in a real time instead of emitting mounts_changed signal several times.
   * This should behave equally to GIOChannel based monitoring. See Bug 792235.
   */
  if (mtab_file_changed_id > 0)
    return;

  context = g_main_context_get_thread_default ();
  if (!context)
    context = g_main_context_default ();

  source = g_idle_source_new ();
  g_source_set_priority (source, G_PRIORITY_DEFAULT);
  g_source_set_callback (source, mtab_file_changed_cb, NULL, NULL);
  g_source_set_static_name (source, "[gio] mtab_file_changed_cb");
  g_source_attach (source, context);
  g_source_unref (source);
}

static gboolean
proc_mounts_changed (GIOChannel   *channel,
                     GIOCondition  cond,
                     gpointer      user_data)
{
  gboolean has_changed = FALSE;

#ifdef HAVE_LIBMOUNT
  if (cond & G_IO_IN)
    {
      G_LOCK (proc_mounts_source);
      if (proc_mounts_monitor != NULL)
        {
          int ret;

          /* The mnt_monitor_next_change function needs to be used to avoid false-positives. */
          ret = mnt_monitor_next_change (proc_mounts_monitor, NULL, NULL);
          if (ret == 0)
            {
              has_changed = TRUE;
              ret = mnt_monitor_event_cleanup (proc_mounts_monitor);
            }

          if (ret < 0)
            g_debug ("mnt_monitor_next_change failed: %s", g_strerror (-ret));
        }
      G_UNLOCK (proc_mounts_source);
    }
#endif

  if (cond & G_IO_ERR)
    has_changed = TRUE;

  if (has_changed)
    {
      G_LOCK (proc_mounts_source);
      mount_poller_time = (guint64) g_get_monotonic_time ();
      G_UNLOCK (proc_mounts_source);

      g_context_specific_group_emit (&mount_monitor_group, signals[MOUNTS_CHANGED]);
    }

  return TRUE;
}

static gboolean
mount_change_poller (gpointer user_data)
{
  GList *current_mounts, *new_it, *old_it;
  gboolean has_changed = FALSE;

  current_mounts = _g_get_unix_mounts ();

  for ( new_it = current_mounts, old_it = mount_poller_mounts;
        new_it != NULL && old_it != NULL;
        new_it = g_list_next (new_it), old_it = g_list_next (old_it) )
    {
      if (g_unix_mount_entry_compare (new_it->data, old_it->data) != 0)
        {
          has_changed = TRUE;
          break;
        }
    }
  if (!(new_it == NULL && old_it == NULL))
    has_changed = TRUE;

  g_list_free_full (mount_poller_mounts, (GDestroyNotify) g_unix_mount_entry_free);

  mount_poller_mounts = current_mounts;

  if (has_changed)
    {
      G_LOCK (proc_mounts_source);
      mount_poller_time = (guint64) g_get_monotonic_time ();
      G_UNLOCK (proc_mounts_source);

      g_context_specific_group_emit (&mount_monitor_group, signals[MOUNTPOINTS_CHANGED]);
    }

  return TRUE;
}


static void
mount_monitor_stop (void)
{
  if (fstab_monitor)
    {
      g_file_monitor_cancel (fstab_monitor);
      g_object_unref (fstab_monitor);
    }

  G_LOCK (proc_mounts_source);
  if (proc_mounts_watch_source != NULL)
    {
      g_source_destroy (proc_mounts_watch_source);
      proc_mounts_watch_source = NULL;
    }

#ifdef HAVE_LIBMOUNT
  g_clear_pointer (&proc_mounts_monitor, mnt_unref_monitor);
#endif
  G_UNLOCK (proc_mounts_source);

  if (mtab_monitor)
    {
      g_file_monitor_cancel (mtab_monitor);
      g_object_unref (mtab_monitor);
    }

  if (mtab_file_changed_id)
    {
      g_source_remove (mtab_file_changed_id);
      mtab_file_changed_id = 0;
    }

  g_list_free_full (mount_poller_mounts, (GDestroyNotify) g_unix_mount_entry_free);
}

static void
mount_monitor_start (void)
{
  GFile *file;

  if (get_fstab_file () != NULL)
    {
      file = g_file_new_for_path (get_fstab_file ());
      fstab_monitor = g_file_monitor_file (file, 0, NULL, NULL);
      g_object_unref (file);

      g_signal_connect (fstab_monitor, "changed", (GCallback)fstab_file_changed, NULL);
    }

  if (get_mtab_monitor_file () != NULL)
    {
      const gchar *mtab_path;

      mtab_path = get_mtab_monitor_file ();
      /* Monitoring files in /proc/ is special - can't just use GFileMonitor.
       * See 'man proc' for more details.
       */
      if (g_str_has_prefix (mtab_path, "/proc/"))
        {
          GIOChannel *proc_mounts_channel = NULL;
          GError *error = NULL;
#ifdef HAVE_LIBMOUNT
          int ret;

          G_LOCK (proc_mounts_source);

          proc_mounts_monitor = mnt_new_monitor ();
          ret = mnt_monitor_enable_kernel (proc_mounts_monitor, TRUE);
          if (ret < 0)
            g_warning ("mnt_monitor_enable_kernel failed: %s", g_strerror (-ret));

          ret = mnt_monitor_enable_userspace (proc_mounts_monitor, TRUE, NULL);
          if (ret < 0)
            g_warning ("mnt_monitor_enable_userspace failed: %s", g_strerror (-ret));

#ifdef HAVE_MNT_MONITOR_VEIL_KERNEL
          ret = mnt_monitor_veil_kernel (proc_mounts_monitor, TRUE);
          if (ret < 0)
            g_warning ("mnt_monitor_veil_kernel failed: %s", g_strerror (-ret));
#endif

          ret = mnt_monitor_get_fd (proc_mounts_monitor);
          if (ret >= 0)
            {
              proc_mounts_channel = g_io_channel_unix_new (ret);
            }
          else
            {
              g_debug ("mnt_monitor_get_fd failed: %s", g_strerror (-ret));
              g_clear_pointer (&proc_mounts_monitor, mnt_unref_monitor);

              /* The mnt_monitor_get_fd function failed e.g. inotify limits are
               * exceeded. Let's try to silently fallback to the old behavior.
               * See: https://gitlab.gnome.org/GNOME/tracker-miners/-/issues/315
               */
            }

          G_UNLOCK (proc_mounts_source);
#endif
          if (proc_mounts_channel == NULL)
            proc_mounts_channel = g_io_channel_new_file (mtab_path, "r", &error);

          if (error != NULL)
            {
              g_warning ("Error creating IO channel for %s: %s (%s, %d)", mtab_path,
                         error->message, g_quark_to_string (error->domain), error->code);
              g_error_free (error);
            }
          else
            {
              G_LOCK (proc_mounts_source);

#ifdef HAVE_LIBMOUNT
              if (proc_mounts_monitor != NULL)
                proc_mounts_watch_source = g_io_create_watch (proc_mounts_channel, G_IO_IN);
#endif
              if (proc_mounts_watch_source == NULL)
                proc_mounts_watch_source = g_io_create_watch (proc_mounts_channel, G_IO_ERR);

              mount_poller_time = (guint64) g_get_monotonic_time ();
              g_source_set_callback (proc_mounts_watch_source,
                                     (GSourceFunc) proc_mounts_changed,
                                     NULL, NULL);
              g_source_attach (proc_mounts_watch_source,
                               g_main_context_get_thread_default ());
              g_source_unref (proc_mounts_watch_source);
              g_io_channel_unref (proc_mounts_channel);

              G_UNLOCK (proc_mounts_source);
            }
        }
      else
        {
          file = g_file_new_for_path (mtab_path);
          mtab_monitor = g_file_monitor_file (file, 0, NULL, NULL);
          g_object_unref (file);
          g_signal_connect (mtab_monitor, "changed", (GCallback)mtab_file_changed, NULL);
        }
    }
  else
    {
      G_LOCK (proc_mounts_source);

      proc_mounts_watch_source = g_timeout_source_new_seconds (3);
      mount_poller_mounts = _g_get_unix_mounts ();
      mount_poller_time = (guint64)g_get_monotonic_time ();
      g_source_set_callback (proc_mounts_watch_source,
                             mount_change_poller,
                             NULL, NULL);
      g_source_attach (proc_mounts_watch_source,
                       g_main_context_get_thread_default ());
      g_source_unref (proc_mounts_watch_source);

      G_UNLOCK (proc_mounts_source);
    }
}

static void
g_unix_mount_monitor_finalize (GObject *object)
{
  GUnixMountMonitor *monitor;

  monitor = G_UNIX_MOUNT_MONITOR (object);

  g_context_specific_group_remove (&mount_monitor_group, monitor->context, monitor, mount_monitor_stop);

  G_OBJECT_CLASS (g_unix_mount_monitor_parent_class)->finalize (object);
}

static void
g_unix_mount_monitor_class_init (GUnixMountMonitorClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = g_unix_mount_monitor_finalize;
 
  /**
   * GUnixMountMonitor::mounts-changed:
   * @monitor: the object on which the signal is emitted
   * 
   * Emitted when the Unix mount entries have changed.
   */ 
  signals[MOUNTS_CHANGED] =
    g_signal_new (I_("mounts-changed"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  0,
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * GUnixMountMonitor::mountpoints-changed:
   * @monitor: the object on which the signal is emitted
   * 
   * Emitted when the Unix mount points have changed.
   */
  signals[MOUNTPOINTS_CHANGED] =
    g_signal_new (I_("mountpoints-changed"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  0,
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);
}

static void
g_unix_mount_monitor_init (GUnixMountMonitor *monitor)
{
}

/**
 * g_unix_mount_monitor_set_rate_limit:
 * @mount_monitor: a [class@GioUnix.MountMonitor]
 * @limit_msec: a integer with the limit (in milliseconds) to poll for changes
 *
 * This function does nothing.
 *
 * Before 2.44, this was a partially-effective way of controlling the
 * rate at which events would be reported under some uncommon
 * circumstances.  Since @mount_monitor is a singleton, it also meant
 * that calling this function would have side effects for other users of
 * the monitor.
 *
 * Since: 2.18
 * Deprecated: 2.44: This function does nothing. Don’t call it.
 */
void
g_unix_mount_monitor_set_rate_limit (GUnixMountMonitor *mount_monitor,
                                     gint               limit_msec)
{
}

/**
 * g_unix_mount_monitor_get:
 *
 * Gets the [class@GioUnix.MountMonitor] for the current thread-default main
 * context.
 *
 * The mount monitor can be used to monitor for changes to the list of
 * mounted filesystems as well as the list of mount points (ie: fstab
 * entries).
 *
 * You must only call [method@GObject.Object.unref] on the return value from
 * under the same main context as you called this function.
 *
 * Returns: (transfer full): the [class@GioUnix.MountMonitor]
 * Since: 2.44
 **/
GUnixMountMonitor *
g_unix_mount_monitor_get (void)
{
  return g_context_specific_group_get (&mount_monitor_group,
                                       G_TYPE_UNIX_MOUNT_MONITOR,
                                       G_STRUCT_OFFSET(GUnixMountMonitor, context),
                                       mount_monitor_start);
}

/**
 * g_unix_mount_monitor_new:
 *
 * Deprecated alias for [func@GioUnix.MountMonitor.get].
 *
 * This function was never a true constructor, which is why it was
 * renamed.
 *
 * Returns: a [class@GioUnix.MountMonitor]
 * Deprecated: 2.44: Use [func@GioUnix.MountMonitor.get] instead.
 */
GUnixMountMonitor *
g_unix_mount_monitor_new (void)
{
  return g_unix_mount_monitor_get ();
}

/* GUnixMount {{{1 */
/**
 * g_unix_mount_free:
 * @mount_entry: a [struct@GioUnix.MountEntry]
 * 
 * Frees a Unix mount.
 *
 * Deprecated: 2.84: Use [method@GioUnix.MountEntry.free] instead.
 */
void
g_unix_mount_free (GUnixMountEntry *mount_entry)
{
  g_unix_mount_entry_free (mount_entry);
}

/**
 * g_unix_mount_entry_free:
 * @mount_entry: a [struct@GioUnix.MountEntry]
 *
 * Frees a Unix mount.
 *
 * Since: 2.84
 */
void
g_unix_mount_entry_free (GUnixMountEntry *mount_entry)
{
  g_return_if_fail (mount_entry != NULL);

  g_free (mount_entry->mount_path);
  g_free (mount_entry->device_path);
  g_free (mount_entry->root_path);
  g_free (mount_entry->filesystem_type);
  g_free (mount_entry->options);
  g_free (mount_entry);
}

/**
 * g_unix_mount_copy:
 * @mount_entry: a [struct@GioUnix.MountEntry]
 *
 * Makes a copy of @mount_entry.
 *
 * Returns: (transfer full): a new [struct@GioUnix.MountEntry]
 * Since: 2.54
 * Deprecated: 2.84: Use [method@GioUnix.MountEntry.copy] instead.
 */
GUnixMountEntry *
g_unix_mount_copy (GUnixMountEntry *mount_entry)
{
  return g_unix_mount_entry_copy (mount_entry);
}

/**
 * g_unix_mount_entry_copy:
 * @mount_entry: a [struct@GioUnix.MountEntry]
 *
 * Makes a copy of @mount_entry.
 *
 * Returns: (transfer full): a new [struct@GioUnix.MountEntry]
 * Since: 2.84
 */
GUnixMountEntry *
g_unix_mount_entry_copy (GUnixMountEntry *mount_entry)
{
  GUnixMountEntry *copy;

  g_return_val_if_fail (mount_entry != NULL, NULL);

  copy = g_new0 (GUnixMountEntry, 1);
  copy->mount_path = g_strdup (mount_entry->mount_path);
  copy->device_path = g_strdup (mount_entry->device_path);
  copy->root_path = g_strdup (mount_entry->root_path);
  copy->filesystem_type = g_strdup (mount_entry->filesystem_type);
  copy->options = g_strdup (mount_entry->options);
  copy->is_read_only = mount_entry->is_read_only;
  copy->is_system_internal = mount_entry->is_system_internal;

  return copy;
}

/**
 * g_unix_mount_point_free:
 * @mount_point: Unix mount point to free.
 * 
 * Frees a Unix mount point.
 */
void
g_unix_mount_point_free (GUnixMountPoint *mount_point)
{
  g_return_if_fail (mount_point != NULL);

  g_free (mount_point->mount_path);
  g_free (mount_point->device_path);
  g_free (mount_point->filesystem_type);
  g_free (mount_point->options);
  g_free (mount_point);
}

/**
 * g_unix_mount_point_copy:
 * @mount_point: a [struct@GioUnix.MountPoint]
 *
 * Makes a copy of @mount_point.
 *
 * Returns: (transfer full): a new [struct@GioUnix.MountPoint]
 * Since: 2.54
 */
GUnixMountPoint*
g_unix_mount_point_copy (GUnixMountPoint *mount_point)
{
  GUnixMountPoint *copy;

  g_return_val_if_fail (mount_point != NULL, NULL);

  copy = g_new0 (GUnixMountPoint, 1);
  copy->mount_path = g_strdup (mount_point->mount_path);
  copy->device_path = g_strdup (mount_point->device_path);
  copy->filesystem_type = g_strdup (mount_point->filesystem_type);
  copy->options = g_strdup (mount_point->options);
  copy->is_read_only = mount_point->is_read_only;
  copy->is_user_mountable = mount_point->is_user_mountable;
  copy->is_loopback = mount_point->is_loopback;

  return copy;
}

/**
 * g_unix_mount_compare:
 * @mount1: first [struct@GioUnix.MountEntry] to compare
 * @mount2: second [struct@GioUnix.MountEntry] to compare
 * 
 * Compares two Unix mounts.
 * 
 * Returns: `1`, `0` or `-1` if @mount1 is greater than, equal to,
 *    or less than @mount2, respectively
 * Deprecated: 2.84: Use [method@GioUnix.MountEntry.compare] instead.
 */
gint
g_unix_mount_compare (GUnixMountEntry *mount1,
		      GUnixMountEntry *mount2)
{
  return g_unix_mount_entry_compare (mount1, mount2);
}

/**
 * g_unix_mount_entry_compare:
 * @mount1: first [struct@GioUnix.MountEntry] to compare
 * @mount2: second [struct@GioUnix.MountEntry] to compare
 *
 * Compares two Unix mounts.
 *
 * Returns: `1`, `0` or `-1` if @mount1 is greater than, equal to,
 *    or less than @mount2, respectively
 * Since: 2.84
 */
gint
g_unix_mount_entry_compare (GUnixMountEntry *mount1,
                            GUnixMountEntry *mount2)
{
  int res;

  g_return_val_if_fail (mount1 != NULL && mount2 != NULL, 0);
  
  res = g_strcmp0 (mount1->mount_path, mount2->mount_path);
  if (res != 0)
    return res;
	
  res = g_strcmp0 (mount1->device_path, mount2->device_path);
  if (res != 0)
    return res;

  res = g_strcmp0 (mount1->root_path, mount2->root_path);
  if (res != 0)
    return res;

  res = g_strcmp0 (mount1->filesystem_type, mount2->filesystem_type);
  if (res != 0)
    return res;

  res = g_strcmp0 (mount1->options, mount2->options);
  if (res != 0)
    return res;

  res =  mount1->is_read_only - mount2->is_read_only;
  if (res != 0)
    return res;
  
  return 0;
}

/**
 * g_unix_mount_get_mount_path:
 * @mount_entry: a [struct@GioUnix.MountEntry] to get the mount path for
 * 
 * Gets the mount path for a Unix mount.
 * 
 * Returns: (type filename): the mount path for @mount_entry
 * Deprecated: 2.84: Use [method@GioUnix.MountEntry.get_mount_path] instead.
 */
const gchar *
g_unix_mount_get_mount_path (GUnixMountEntry *mount_entry)
{
  return g_unix_mount_entry_get_mount_path (mount_entry);
}

/**
 * g_unix_mount_entry_get_mount_path:
 * @mount_entry: a [struct@GioUnix.MountEntry] to get the mount path for
 *
 * Gets the mount path for a Unix mount.
 *
 * Returns: (type filename): the mount path for @mount_entry
 * Since: 2.84
 */
const gchar *
g_unix_mount_entry_get_mount_path (GUnixMountEntry *mount_entry)
{
  g_return_val_if_fail (mount_entry != NULL, NULL);

  return mount_entry->mount_path;
}

/**
 * g_unix_mount_get_device_path:
 * @mount_entry: a [struct@GioUnix.MountEntry]
 * 
 * Gets the device path for a Unix mount.
 * 
 * Returns: (type filename): a string containing the device path
 * Deprecated: 2.84: Use [method@GioUnix.MountEntry.get_device_path] instead.
 */
const gchar *
g_unix_mount_get_device_path (GUnixMountEntry *mount_entry)
{
  return g_unix_mount_entry_get_device_path (mount_entry);
}

/**
 * g_unix_mount_entry_get_device_path:
 * @mount_entry: a [struct@GioUnix.MountEntry]
 *
 * Gets the device path for a Unix mount.
 *
 * Returns: (type filename): a string containing the device path
 * Since: 2.84
 */
const gchar *
g_unix_mount_entry_get_device_path (GUnixMountEntry *mount_entry)
{
  g_return_val_if_fail (mount_entry != NULL, NULL);

  return mount_entry->device_path;
}

/**
 * g_unix_mount_get_root_path:
 * @mount_entry: a [struct@GioUnix.MountEntry]
 * 
 * Gets the root of the mount within the filesystem.
 *
 * This is useful e.g. for mounts created by bind operation, or btrfs subvolumes.
 * 
 * For example, the root path is equal to `/` for a mount created by
 * `mount /dev/sda1 /mnt/foo` and `/bar` for
 * `mount --bind /mnt/foo/bar /mnt/bar`.
 *
 * Returns: (nullable): a string containing the root, or `NULL` if not supported
 * Since: 2.60
 * Deprecated: 2.84: Use [method@GioUnix.MountEntry.get_root_path] instead.
 */
const gchar *
g_unix_mount_get_root_path (GUnixMountEntry *mount_entry)
{
  return g_unix_mount_entry_get_root_path (mount_entry);
}

/**
 * g_unix_mount_entry_get_root_path:
 * @mount_entry: a [struct@GioUnix.MountEntry]
 *
 * Gets the root of the mount within the filesystem. This is useful e.g. for
 * mounts created by bind operation, or btrfs subvolumes.
 *
 * For example, the root path is equal to `/` for a mount created by
 * `mount /dev/sda1 /mnt/foo` and `/bar` for
 * `mount --bind /mnt/foo/bar /mnt/bar`.
 *
 * Returns: (nullable): a string containing the root, or `NULL` if not supported
 * Since: 2.84
 */
const gchar *
g_unix_mount_entry_get_root_path (GUnixMountEntry *mount_entry)
{
  g_return_val_if_fail (mount_entry != NULL, NULL);

  return mount_entry->root_path;
}

/**
 * g_unix_mount_get_fs_type:
 * @mount_entry: a [struct@GioUnix.MountEntry]
 * 
 * Gets the filesystem type for the Unix mount.
 * 
 * Returns: a string containing the file system type
 * Deprecated: 2.84: Use [method@GioUnix.MountEntry.get_fs_type] instead.
 */
const gchar *
g_unix_mount_get_fs_type (GUnixMountEntry *mount_entry)
{
  return g_unix_mount_entry_get_fs_type (mount_entry);
}

/**
 * g_unix_mount_entry_get_fs_type:
 * @mount_entry: a [struct@GioUnix.MountEntry]
 *
 * Gets the filesystem type for the Unix mount.
 *
 * Returns: a string containing the file system type
 * Since: 2.84
 */
const gchar *
g_unix_mount_entry_get_fs_type (GUnixMountEntry *mount_entry)
{
  g_return_val_if_fail (mount_entry != NULL, NULL);

  return mount_entry->filesystem_type;
}

/**
 * g_unix_mount_get_options:
 * @mount_entry: a [struct@GioUnix.MountEntry]
 * 
 * Gets a comma separated list of mount options for the Unix mount.
 * 
 * For example: `rw,relatime,seclabel,data=ordered`.
 * 
 * This is similar to [method@GioUnix.MountPoint.get_options], but it takes
 * a [struct@GioUnix.MountEntry] as an argument.
 *
 * Returns: (nullable): a string containing the options, or `NULL` if not
 *    available.
 * Since: 2.58
 * Deprecated: 2.84: Use [method@GioUnix.MountEntry.get_options] instead.
 */
const gchar *
g_unix_mount_get_options (GUnixMountEntry *mount_entry)
{
  return g_unix_mount_entry_get_options (mount_entry);
}

/**
 * g_unix_mount_entry_get_options:
 * @mount_entry: a [struct@GioUnix.MountEntry]
 *
 * Gets a comma separated list of mount options for the Unix mount.
 *
 * For example: `rw,relatime,seclabel,data=ordered`.
 *
 * This is similar to [method@GioUnix.MountPoint.get_options], but it takes
 * a [struct@GioUnix.MountEntry] as an argument.
 *
 * Returns: (nullable): a string containing the options, or `NULL` if not
 *    available.
 * Since: 2.84
 */
const gchar *
g_unix_mount_entry_get_options (GUnixMountEntry *mount_entry)
{
  g_return_val_if_fail (mount_entry != NULL, NULL);

  return mount_entry->options;
}

/**
 * g_unix_mount_is_readonly:
 * @mount_entry: a [struct@GioUnix.MountEntry]
 * 
 * Checks if a Unix mount is mounted read only.
 * 
 * Returns: true if @mount_entry is read only; false otherwise
 * Deprecated: 2.84: Use [method@GioUnix.MountEntry.is_readonly] instead.
 */
gboolean
g_unix_mount_is_readonly (GUnixMountEntry *mount_entry)
{
  return g_unix_mount_entry_is_readonly (mount_entry);
}

/**
 * g_unix_mount_entry_is_readonly:
 * @mount_entry: a [struct@GioUnix.MountEntry]
 *
 * Checks if a Unix mount is mounted read only.
 *
 * Returns: true if @mount_entry is read only; false otherwise
 * Since: 2.84
 */
gboolean
g_unix_mount_entry_is_readonly (GUnixMountEntry *mount_entry)
{
  g_return_val_if_fail (mount_entry != NULL, FALSE);

  return mount_entry->is_read_only;
}

/**
 * g_unix_mount_is_system_internal:
 * @mount_entry: a [struct@GioUnix.MountEntry]
 *
 * Checks if a Unix mount is a system mount.
 *
 * This is the Boolean OR of
 * [func@GioUnix.is_system_fs_type], [func@GioUnix.is_system_device_path] and
 * [func@GioUnix.is_mount_path_system_internal] on @mount_entry’s properties.
 * 
 * The definition of what a ‘system’ mount entry is may change over time as new
 * file system types and device paths are ignored.
 *
 * Returns: true if the Unix mount is for a system path; false otherwise
 * Deprecated: 2.84: Use [method@GioUnix.MountEntry.is_system_internal] instead.
 */
gboolean
g_unix_mount_is_system_internal (GUnixMountEntry *mount_entry)
{
  return g_unix_mount_entry_is_system_internal (mount_entry);
}

/**
 * g_unix_mount_entry_is_system_internal:
 * @mount_entry: a [struct@GioUnix.MountEntry]
 *
 * Checks if a Unix mount is a system mount.
 *
 * This is the Boolean OR of
 * [func@GioUnix.is_system_fs_type], [func@GioUnix.is_system_device_path] and
 * [func@GioUnix.is_mount_path_system_internal] on @mount_entry’s properties.
 *
 * The definition of what a ‘system’ mount entry is may change over time as new
 * file system types and device paths are ignored.
 *
 * Returns: true if the Unix mount is for a system path; false otherwise
 * Since: 2.84
 */
gboolean
g_unix_mount_entry_is_system_internal (GUnixMountEntry *mount_entry)
{
  g_return_val_if_fail (mount_entry != NULL, FALSE);

  return mount_entry->is_system_internal;
}

/* GUnixMountPoint {{{1 */
/**
 * g_unix_mount_point_compare:
 * @mount1: a [struct@GioUnix.MountPoint]
 * @mount2: a [struct@GioUnix.MountPoint]
 * 
 * Compares two Unix mount points.
 * 
 * Returns: `1`, `0` or `-1` if @mount1 is greater than, equal to,
 *    or less than @mount2, respectively
 */
gint
g_unix_mount_point_compare (GUnixMountPoint *mount1,
			    GUnixMountPoint *mount2)
{
  int res;

  g_return_val_if_fail (mount1 != NULL && mount2 != NULL, 0);

  res = g_strcmp0 (mount1->mount_path, mount2->mount_path);
  if (res != 0) 
    return res;
	
  res = g_strcmp0 (mount1->device_path, mount2->device_path);
  if (res != 0) 
    return res;
	
  res = g_strcmp0 (mount1->filesystem_type, mount2->filesystem_type);
  if (res != 0) 
    return res;

  res = g_strcmp0 (mount1->options, mount2->options);
  if (res != 0) 
    return res;

  res =  mount1->is_read_only - mount2->is_read_only;
  if (res != 0) 
    return res;

  res = mount1->is_user_mountable - mount2->is_user_mountable;
  if (res != 0) 
    return res;

  res = mount1->is_loopback - mount2->is_loopback;
  if (res != 0)
    return res;
  
  return 0;
}

/**
 * g_unix_mount_point_get_mount_path:
 * @mount_point: a [struct@GioUnix.MountPoint]
 * 
 * Gets the mount path for a Unix mount point.
 * 
 * Returns: (type filename): a string containing the mount path
 */
const gchar *
g_unix_mount_point_get_mount_path (GUnixMountPoint *mount_point)
{
  g_return_val_if_fail (mount_point != NULL, NULL);

  return mount_point->mount_path;
}

/**
 * g_unix_mount_point_get_device_path:
 * @mount_point: a [struct@GioUnix.MountPoint]
 * 
 * Gets the device path for a Unix mount point.
 * 
 * Returns: (type filename): a string containing the device path
 */
const gchar *
g_unix_mount_point_get_device_path (GUnixMountPoint *mount_point)
{
  g_return_val_if_fail (mount_point != NULL, NULL);

  return mount_point->device_path;
}

/**
 * g_unix_mount_point_get_fs_type:
 * @mount_point: a [struct@GioUnix.MountPoint]
 * 
 * Gets the file system type for the mount point.
 * 
 * Returns: a string containing the file system type
 */
const gchar *
g_unix_mount_point_get_fs_type (GUnixMountPoint *mount_point)
{
  g_return_val_if_fail (mount_point != NULL, NULL);

  return mount_point->filesystem_type;
}

/**
 * g_unix_mount_point_get_options:
 * @mount_point: a [struct@GioUnix.MountPoint]
 * 
 * Gets the options for the mount point.
 * 
 * Returns: (nullable): a string containing the options
 * Since: 2.32
 */
const gchar *
g_unix_mount_point_get_options (GUnixMountPoint *mount_point)
{
  g_return_val_if_fail (mount_point != NULL, NULL);

  return mount_point->options;
}

/**
 * g_unix_mount_point_is_readonly:
 * @mount_point: a [struct@GioUnix.MountPoint]
 * 
 * Checks if a Unix mount point is read only.
 * 
 * Returns: true if a mount point is read only; false otherwise
 */
gboolean
g_unix_mount_point_is_readonly (GUnixMountPoint *mount_point)
{
  g_return_val_if_fail (mount_point != NULL, FALSE);

  return mount_point->is_read_only;
}

/**
 * g_unix_mount_point_is_user_mountable:
 * @mount_point: a [struct@GioUnix.MountPoint]
 * 
 * Checks if a Unix mount point is mountable by the user.
 * 
 * Returns: true if the mount point is user mountable; false otherwise
 */
gboolean
g_unix_mount_point_is_user_mountable (GUnixMountPoint *mount_point)
{
  g_return_val_if_fail (mount_point != NULL, FALSE);

  return mount_point->is_user_mountable;
}

/**
 * g_unix_mount_point_is_loopback:
 * @mount_point: a [struct@GioUnix.MountPoint]
 * 
 * Checks if a Unix mount point is a loopback device.
 * 
 * Returns: true if the mount point is a loopback device; false otherwise
 */
gboolean
g_unix_mount_point_is_loopback (GUnixMountPoint *mount_point)
{
  g_return_val_if_fail (mount_point != NULL, FALSE);

  return mount_point->is_loopback;
}

static GUnixMountType
guess_mount_type (const char *mount_path,
		  const char *device_path,
		  const char *filesystem_type)
{
  GUnixMountType type;
  char *basename;

  type = G_UNIX_MOUNT_TYPE_UNKNOWN;
  
  if ((strcmp (filesystem_type, "udf") == 0) ||
      (strcmp (filesystem_type, "iso9660") == 0) ||
      (strcmp (filesystem_type, "cd9660") == 0))
    type = G_UNIX_MOUNT_TYPE_CDROM;
  else if ((strcmp (filesystem_type, "nfs") == 0) ||
           (strcmp (filesystem_type, "nfs4") == 0))
    type = G_UNIX_MOUNT_TYPE_NFS;
  else if (g_str_has_prefix (device_path, "/vol/dev/diskette/") ||
	   g_str_has_prefix (device_path, "/dev/fd") ||
	   g_str_has_prefix (device_path, "/dev/floppy"))
    type = G_UNIX_MOUNT_TYPE_FLOPPY;
  else if (g_str_has_prefix (device_path, "/dev/cdrom") ||
	   g_str_has_prefix (device_path, "/dev/acd") ||
	   g_str_has_prefix (device_path, "/dev/cd"))
    type = G_UNIX_MOUNT_TYPE_CDROM;
  else if (g_str_has_prefix (device_path, "/vol/"))
    {
      const char *name = mount_path + strlen ("/");
      
      if (g_str_has_prefix (name, "cdrom"))
	type = G_UNIX_MOUNT_TYPE_CDROM;
      else if (g_str_has_prefix (name, "floppy") ||
	       g_str_has_prefix (device_path, "/vol/dev/diskette/")) 
	type = G_UNIX_MOUNT_TYPE_FLOPPY;
      else if (g_str_has_prefix (name, "rmdisk")) 
	type = G_UNIX_MOUNT_TYPE_ZIP;
      else if (g_str_has_prefix (name, "jaz"))
	type = G_UNIX_MOUNT_TYPE_JAZ;
      else if (g_str_has_prefix (name, "memstick"))
	type = G_UNIX_MOUNT_TYPE_MEMSTICK;
    }
  else
    {
      basename = g_path_get_basename (mount_path);
      
      if (g_str_has_prefix (basename, "cdr") ||
	  g_str_has_prefix (basename, "cdwriter") ||
	  g_str_has_prefix (basename, "burn") ||
	  g_str_has_prefix (basename, "dvdr"))
	type = G_UNIX_MOUNT_TYPE_CDROM;
      else if (g_str_has_prefix (basename, "floppy"))
	type = G_UNIX_MOUNT_TYPE_FLOPPY;
      else if (g_str_has_prefix (basename, "zip"))
	type = G_UNIX_MOUNT_TYPE_ZIP;
      else if (g_str_has_prefix (basename, "jaz"))
	type = G_UNIX_MOUNT_TYPE_JAZ;
      else if (g_str_has_prefix (basename, "camera"))
	type = G_UNIX_MOUNT_TYPE_CAMERA;
      else if (g_str_has_prefix (basename, "memstick") ||
	       g_str_has_prefix (basename, "memory_stick") ||
	       g_str_has_prefix (basename, "ram"))
	type = G_UNIX_MOUNT_TYPE_MEMSTICK;
      else if (g_str_has_prefix (basename, "compact_flash"))
	type = G_UNIX_MOUNT_TYPE_CF;
      else if (g_str_has_prefix (basename, "smart_media"))
	type = G_UNIX_MOUNT_TYPE_SM;
      else if (g_str_has_prefix (basename, "sd_mmc"))
	type = G_UNIX_MOUNT_TYPE_SDMMC;
      else if (g_str_has_prefix (basename, "ipod"))
	type = G_UNIX_MOUNT_TYPE_IPOD;
      
      g_free (basename);
    }
  
  if (type == G_UNIX_MOUNT_TYPE_UNKNOWN)
    type = G_UNIX_MOUNT_TYPE_HD;
  
  return type;
}

/**
 * g_unix_mount_entry_guess_type:
 * @mount_entry: a [struct@GioUnix.MountEntry]
 *
 * Guesses the type of a Unix mount entry.
 * 
 * If the mount type cannot be determined, returns
 * [enum@GioUnix.MountType.UNKNOWN].
 * 
 * Returns: a [enum@GioUnix.MountType]
 */
static GUnixMountType
g_unix_mount_entry_guess_type (GUnixMountEntry *mount_entry)
{
  g_return_val_if_fail (mount_entry != NULL, G_UNIX_MOUNT_TYPE_UNKNOWN);
  g_return_val_if_fail (mount_entry->mount_path != NULL, G_UNIX_MOUNT_TYPE_UNKNOWN);
  g_return_val_if_fail (mount_entry->device_path != NULL, G_UNIX_MOUNT_TYPE_UNKNOWN);
  g_return_val_if_fail (mount_entry->filesystem_type != NULL, G_UNIX_MOUNT_TYPE_UNKNOWN);

  return guess_mount_type (mount_entry->mount_path,
			   mount_entry->device_path,
			   mount_entry->filesystem_type);
}

/**
 * g_unix_mount_point_guess_type:
 * @mount_point: a [struct@GioUnix.MountPoint]
 * 
 * Guesses the type of a Unix mount point.
 *
 * If the mount type cannot be determined, returns
 * [enum@GioUnix.MountType.UNKNOWN].
 * 
 * Returns: a [enum@GioUnix.MountType]
 */
static GUnixMountType
g_unix_mount_point_guess_type (GUnixMountPoint *mount_point)
{
  g_return_val_if_fail (mount_point != NULL, G_UNIX_MOUNT_TYPE_UNKNOWN);
  g_return_val_if_fail (mount_point->mount_path != NULL, G_UNIX_MOUNT_TYPE_UNKNOWN);
  g_return_val_if_fail (mount_point->device_path != NULL, G_UNIX_MOUNT_TYPE_UNKNOWN);
  g_return_val_if_fail (mount_point->filesystem_type != NULL, G_UNIX_MOUNT_TYPE_UNKNOWN);

  return guess_mount_type (mount_point->mount_path,
			   mount_point->device_path,
			   mount_point->filesystem_type);
}

static const char *
type_to_icon (GUnixMountType type, gboolean is_mount_point, gboolean use_symbolic)
{
  const char *icon_name;
  
  switch (type)
    {
    case G_UNIX_MOUNT_TYPE_HD:
      if (is_mount_point)
        icon_name = use_symbolic ? "drive-removable-media-symbolic" : "drive-removable-media";
      else
        icon_name = use_symbolic ? "drive-harddisk-symbolic" : "drive-harddisk";
      break;
    case G_UNIX_MOUNT_TYPE_FLOPPY:
    case G_UNIX_MOUNT_TYPE_ZIP:
    case G_UNIX_MOUNT_TYPE_JAZ:
      if (is_mount_point)
        icon_name = use_symbolic ? "drive-removable-media-symbolic" : "drive-removable-media";
      else
        icon_name = use_symbolic ? "media-removable-symbolic" : "media-floppy";
      break;
    case G_UNIX_MOUNT_TYPE_CDROM:
      if (is_mount_point)
        icon_name = use_symbolic ? "drive-optical-symbolic" : "drive-optical";
      else
        icon_name = use_symbolic ? "media-optical-symbolic" : "media-optical";
      break;
    case G_UNIX_MOUNT_TYPE_NFS:
        icon_name = use_symbolic ? "folder-remote-symbolic" : "folder-remote";
      break;
    case G_UNIX_MOUNT_TYPE_MEMSTICK:
      if (is_mount_point)
        icon_name = use_symbolic ? "drive-removable-media-symbolic" : "drive-removable-media";
      else
        icon_name = use_symbolic ? "media-removable-symbolic" : "media-flash";
      break;
    case G_UNIX_MOUNT_TYPE_CAMERA:
      if (is_mount_point)
        icon_name = use_symbolic ? "drive-removable-media-symbolic" : "drive-removable-media";
      else
        icon_name = use_symbolic ? "camera-photo-symbolic" : "camera-photo";
      break;
    case G_UNIX_MOUNT_TYPE_IPOD:
      if (is_mount_point)
        icon_name = use_symbolic ? "drive-removable-media-symbolic" : "drive-removable-media";
      else
        icon_name = use_symbolic ? "multimedia-player-symbolic" : "multimedia-player";
      break;
    case G_UNIX_MOUNT_TYPE_UNKNOWN:
    default:
      if (is_mount_point)
        icon_name = use_symbolic ? "drive-removable-media-symbolic" : "drive-removable-media";
      else
        icon_name = use_symbolic ? "drive-harddisk-symbolic" : "drive-harddisk";
      break;
    }

  return icon_name;
}

/**
 * g_unix_mount_guess_name:
 * @mount_entry: a [struct@GioUnix.MountEntry]
 *
 * Guesses the name of a Unix mount entry.
 * 
 * The result is a translated string.
 *
 * Returns: (transfer full): a newly allocated translated string
 * Deprecated: 2.84: Use [method@GioUnix.MountEntry.guess_name] instead.
 */
gchar *
g_unix_mount_guess_name (GUnixMountEntry *mount_entry)
{
  return g_unix_mount_entry_guess_name (mount_entry);
}

/**
 * g_unix_mount_entry_guess_name:
 * @mount_entry: a [struct@GioUnix.MountEntry]
 *
 * Guesses the name of a Unix mount entry.
 *
 * The result is a translated string.
 *
 * Returns: (transfer full): a newly allocated translated string
 * Since: 2.84
 */
gchar *
g_unix_mount_entry_guess_name (GUnixMountEntry *mount_entry)
{
  char *name;

  if (strcmp (mount_entry->mount_path, "/") == 0)
    name = g_strdup (_("Filesystem root"));
  else
    name = g_filename_display_basename (mount_entry->mount_path);

  return name;
}

/**
 * g_unix_mount_guess_icon:
 * @mount_entry: a [struct@GioUnix.MountEntry]
 * 
 * Guesses the icon of a Unix mount entry.
 *
 * Returns: (transfer full): a [iface@Gio.Icon]
 * Deprecated: 2.84: Use [method@GioUnix.MountEntry.guess_icon] instead.
 */
GIcon *
g_unix_mount_guess_icon (GUnixMountEntry *mount_entry)
{
  return g_unix_mount_entry_guess_icon (mount_entry);
}

/**
 * g_unix_mount_entry_guess_icon:
 * @mount_entry: a [struct@GioUnix.MountEntry]
 *
 * Guesses the icon of a Unix mount entry.
 *
 * Returns: (transfer full): a [iface@Gio.Icon]
 * Since: 2.84
 */
GIcon *
g_unix_mount_entry_guess_icon (GUnixMountEntry *mount_entry)
{
  return g_themed_icon_new_with_default_fallbacks (type_to_icon (g_unix_mount_entry_guess_type (mount_entry), FALSE, FALSE));
}

/**
 * g_unix_mount_guess_symbolic_icon:
 * @mount_entry: a [struct@GioUnix.MountEntry]
 *
 * Guesses the symbolic icon of a Unix mount entry.
 *
 * Returns: (transfer full): a [iface@Gio.Icon]
 * Since: 2.34
 * Deprecated: 2.84: Use [method@GioUnix.MountEntry.guess_symbolic_icon] instead.
 */
GIcon *
g_unix_mount_guess_symbolic_icon (GUnixMountEntry *mount_entry)
{
  return g_unix_mount_entry_guess_symbolic_icon (mount_entry);
}

/**
 * g_unix_mount_entry_guess_symbolic_icon:
 * @mount_entry: a [struct@GioUnix.MountEntry]
 *
 * Guesses the symbolic icon of a Unix mount entry.
 *
 * Returns: (transfer full): a [iface@Gio.Icon]
 * Since: 2.84
 */
GIcon *
g_unix_mount_entry_guess_symbolic_icon (GUnixMountEntry *mount_entry)
{
  return g_themed_icon_new_with_default_fallbacks (type_to_icon (g_unix_mount_entry_guess_type (mount_entry), FALSE, TRUE));
}

/**
 * g_unix_mount_point_guess_name:
 * @mount_point: a [struct@GioUnix.MountPoint]
 *
 * Guesses the name of a Unix mount point.
 * 
 * The result is a translated string.
 *
 * Returns: (transfer full): a newly allocated translated string
 */
gchar *
g_unix_mount_point_guess_name (GUnixMountPoint *mount_point)
{
  char *name;

  if (strcmp (mount_point->mount_path, "/") == 0)
    name = g_strdup (_("Filesystem root"));
  else
    name = g_filename_display_basename (mount_point->mount_path);

  return name;
}

/**
 * g_unix_mount_point_guess_icon:
 * @mount_point: a [struct@GioUnix.MountPoint]
 * 
 * Guesses the icon of a Unix mount point.
 *
 * Returns: (transfer full): a [iface@Gio.Icon]
 */
GIcon *
g_unix_mount_point_guess_icon (GUnixMountPoint *mount_point)
{
  return g_themed_icon_new_with_default_fallbacks (type_to_icon (g_unix_mount_point_guess_type (mount_point), TRUE, FALSE));
}

/**
 * g_unix_mount_point_guess_symbolic_icon:
 * @mount_point: a [struct@GioUnix.MountPoint]
 *
 * Guesses the symbolic icon of a Unix mount point.
 *
 * Returns: (transfer full): a [iface@Gio.Icon]
 * Since: 2.34
 */
GIcon *
g_unix_mount_point_guess_symbolic_icon (GUnixMountPoint *mount_point)
{
  return g_themed_icon_new_with_default_fallbacks (type_to_icon (g_unix_mount_point_guess_type (mount_point), TRUE, TRUE));
}

/**
 * g_unix_mount_guess_can_eject:
 * @mount_entry: a [struct@GioUnix.MountEntry]
 * 
 * Guesses whether a Unix mount entry can be ejected.
 *
 * Returns: true if @mount_entry is deemed to be ejectable; false otherwise
 * Deprecated: 2.84: Use [method@GioUnix.MountEntry.guess_can_eject] instead.
 */
gboolean
g_unix_mount_guess_can_eject (GUnixMountEntry *mount_entry)
{
  return g_unix_mount_entry_guess_can_eject (mount_entry);
}

/**
 * g_unix_mount_entry_guess_can_eject:
 * @mount_entry: a [struct@GioUnix.MountEntry]
 *
 * Guesses whether a Unix mount entry can be ejected.
 *
 * Returns: true if @mount_entry is deemed to be ejectable; false otherwise
 * Since: 2.84
 */
gboolean
g_unix_mount_entry_guess_can_eject (GUnixMountEntry *mount_entry)
{
  GUnixMountType guessed_type;

  guessed_type = g_unix_mount_entry_guess_type (mount_entry);
  if (guessed_type == G_UNIX_MOUNT_TYPE_IPOD ||
      guessed_type == G_UNIX_MOUNT_TYPE_CDROM)
    return TRUE;

  return FALSE;
}

/**
 * g_unix_mount_guess_should_display:
 * @mount_entry: a [struct@GioUnix.MountEntry]
 * 
 * Guesses whether a Unix mount entry should be displayed in the UI.
 *
 * Returns: true if @mount_entry is deemed to be displayable; false otherwise
 * Deprecated: 2.84: Use [method@GioUnix.MountEntry.guess_should_display] instead.
 */
gboolean
g_unix_mount_guess_should_display (GUnixMountEntry *mount_entry)
{
  return g_unix_mount_entry_guess_should_display (mount_entry);
}

/**
 * g_unix_mount_entry_guess_should_display:
 * @mount_entry: a [struct@GioUnix.MountEntry]
 *
 * Guesses whether a Unix mount entry should be displayed in the UI.
 *
 * Returns: true if @mount_entry is deemed to be displayable; false otherwise
 * Since: 2.84
 */
gboolean
g_unix_mount_entry_guess_should_display (GUnixMountEntry *mount_entry)
{
  const char *mount_path;
  const gchar *user_name;
  gsize user_name_len;

  /* Never display internal mountpoints */
  if (g_unix_mount_entry_is_system_internal (mount_entry))
    return FALSE;
  
  /* Only display things in /media (which are generally user mountable)
     and home dir (fuse stuff) and /run/media/$USER */
  mount_path = mount_entry->mount_path;
  if (mount_path != NULL)
    {
      const gboolean running_as_root = (getuid () == 0);
      gboolean is_in_runtime_dir = FALSE;

      /* Hide mounts within a dot path, suppose it was a purpose to hide this mount */
      if (g_strstr_len (mount_path, -1, "/.") != NULL)
        return FALSE;

      /* Check /run/media/$USER/. If running as root, display any mounts below
       * /run/media/. */
      if (running_as_root)
        {
          if (strncmp (mount_path, "/run/media/", strlen ("/run/media/")) == 0)
            is_in_runtime_dir = TRUE;
        }
      else
        {
          user_name = g_get_user_name ();
          user_name_len = strlen (user_name);
          if (strncmp (mount_path, "/run/media/", strlen ("/run/media/")) == 0 &&
              strncmp (mount_path + strlen ("/run/media/"), user_name, user_name_len) == 0 &&
              mount_path[strlen ("/run/media/") + user_name_len] == '/')
            is_in_runtime_dir = TRUE;
        }

      if (is_in_runtime_dir || g_str_has_prefix (mount_path, "/media/"))
        {
          char *path;
          /* Avoid displaying mounts that are not accessible to the user.
           *
           * See http://bugzilla.gnome.org/show_bug.cgi?id=526320 for why we
           * want to avoid g_access() for mount points which can potentially
           * block or fail stat()'ing, such as network mounts.
           */
          path = g_path_get_dirname (mount_path);
          if (g_str_has_prefix (path, "/media/"))
            {
              if (g_access (path, R_OK|X_OK) != 0) 
                {
                  g_free (path);
                  return FALSE;
                }
            }
          g_free (path);

          if (mount_entry->device_path && mount_entry->device_path[0] == '/')
           {
             struct stat st;
             if (g_stat (mount_entry->device_path, &st) == 0 &&
                 S_ISBLK(st.st_mode) &&
                 g_access (mount_path, R_OK|X_OK) != 0)
               return FALSE;
           }
          return TRUE;
        }
      
      if (g_str_has_prefix (mount_path, g_get_home_dir ()) && 
          mount_path[strlen (g_get_home_dir())] == G_DIR_SEPARATOR)
        return TRUE;
    }
  
  return FALSE;
}

/**
 * g_unix_mount_point_guess_can_eject:
 * @mount_point: a [struct@GioUnix.MountPoint]
 * 
 * Guesses whether a Unix mount point can be ejected.
 *
 * Returns: true if @mount_point is deemed to be ejectable; false otherwise
 */
gboolean
g_unix_mount_point_guess_can_eject (GUnixMountPoint *mount_point)
{
  GUnixMountType guessed_type;

  guessed_type = g_unix_mount_point_guess_type (mount_point);
  if (guessed_type == G_UNIX_MOUNT_TYPE_IPOD ||
      guessed_type == G_UNIX_MOUNT_TYPE_CDROM)
    return TRUE;

  return FALSE;
}

/* Utility functions {{{1 */

#ifdef HAVE_MNTENT_H
/* borrowed from gtk/gtkfilesystemunix.c in GTK on 02/23/2006 */
static void
_canonicalize_filename (gchar *filename)
{
  gchar *p, *q;
  gboolean last_was_slash = FALSE;
  
  p = filename;
  q = filename;
  
  while (*p)
    {
      if (*p == G_DIR_SEPARATOR)
        {
          if (!last_was_slash)
            *q++ = G_DIR_SEPARATOR;
          
          last_was_slash = TRUE;
        }
      else
        {
          if (last_was_slash && *p == '.')
            {
              if (*(p + 1) == G_DIR_SEPARATOR ||
                  *(p + 1) == '\0')
                {
                  if (*(p + 1) == '\0')
                    break;
                  
                  p += 1;
                }
              else if (*(p + 1) == '.' &&
                       (*(p + 2) == G_DIR_SEPARATOR ||
                        *(p + 2) == '\0'))
                {
                  if (q > filename + 1)
                    {
                      q--;
                      while (q > filename + 1 &&
                             *(q - 1) != G_DIR_SEPARATOR)
                        q--;
                    }
                  
                  if (*(p + 2) == '\0')
                    break;
                  
                  p += 2;
                }
              else
                {
                  *q++ = *p;
                  last_was_slash = FALSE;
                }
            }
          else
            {
              *q++ = *p;
              last_was_slash = FALSE;
            }
        }
      
      p++;
    }
  
  if (q > filename + 1 && *(q - 1) == G_DIR_SEPARATOR)
    q--;
  
  *q = '\0';
}

static char *
_resolve_symlink (const char *file)
{
  GError *error;
  char *dir;
  char *link;
  char *f;
  char *f1;
  
  f = g_strdup (file);
  
  while (g_file_test (f, G_FILE_TEST_IS_SYMLINK)) 
    {
      link = g_file_read_link (f, &error);
      if (link == NULL) 
        {
          g_error_free (error);
          g_free (f);
          f = NULL;
          goto out;
        }
    
      dir = g_path_get_dirname (f);
      f1 = g_strdup_printf ("%s/%s", dir, link);
      g_free (dir);
      g_free (link);
      g_free (f);
      f = f1;
    }
  
 out:
  if (f != NULL)
    _canonicalize_filename (f);
  return f;
}

static const char *
_resolve_dev_root (void)
{
  static gboolean have_real_dev_root = FALSE;
  static char real_dev_root[256];
  struct stat statbuf;
  
  /* see if it's cached already */
  if (have_real_dev_root)
    goto found;
  
  /* otherwise we're going to find it right away.. */
  have_real_dev_root = TRUE;
  
  if (stat ("/dev/root", &statbuf) == 0) 
    {
      if (! S_ISLNK (statbuf.st_mode)) 
        {
          dev_t root_dev = statbuf.st_dev;
          FILE *f;
      
          /* see if device with similar major:minor as /dev/root is mention
           * in /etc/mtab (it usually is) 
           */
          f = g_fopen ("/etc/mtab", "re");
          if (f != NULL) 
            {
	      struct mntent *entp;
#ifdef HAVE_GETMNTENT_R        
              struct mntent ent;
              char buf[1024];
              while ((entp = getmntent_r (f, &ent, buf, sizeof (buf))) != NULL) 
                {
#else
	      G_LOCK (getmntent);
	      while ((entp = getmntent (f)) != NULL) 
                { 
#endif          
                  if (stat (entp->mnt_fsname, &statbuf) == 0 &&
                      statbuf.st_dev == root_dev) 
                    {
                      strncpy (real_dev_root, entp->mnt_fsname, sizeof (real_dev_root) - 1);
                      real_dev_root[sizeof (real_dev_root) - 1] = '\0';
                      fclose (f);
                      goto found;
                    }
                }

              /* endmntent() calls fclose() for us, but scan-build doesn’t know that */
#if !G_ANALYZER_ANALYZING
              endmntent (f);
#else
              fclose (f);
#endif

#ifndef HAVE_GETMNTENT_R
	      G_UNLOCK (getmntent);
#endif
            }                                        
      
          /* no, that didn't work.. next we could scan /dev ... but I digress.. */
      
        } 
       else 
        {
          char *resolved;
          resolved = _resolve_symlink ("/dev/root");
          if (resolved != NULL)
            {
              strncpy (real_dev_root, resolved, sizeof (real_dev_root) - 1);
              real_dev_root[sizeof (real_dev_root) - 1] = '\0';
              g_free (resolved);
              goto found;
            }
        }
    }
  
  /* bah sucks.. */
  strcpy (real_dev_root, "/dev/root");
  
found:
  return real_dev_root;
}
#endif

/* Epilogue {{{1 */
/* vim:set foldmethod=marker: */
