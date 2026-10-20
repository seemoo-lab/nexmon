/* Temporary files with automatic cleanup.
   Copyright (C) 2006-2026 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2006.

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of the
   License, or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>

/* Specification.  */
#include "clean-temp-simple.h"
#include "clean-temp-private.h"

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <error.h>
#include "fatal-signal.h"
#include "asyncsafe-spin.h"
#include "glthread/lock.h"
#include "glthread/once.h"
#include "thread-optim.h"
#include "gl_list.h"
#include "gl_linkedhash_list.h"
#include "hashkey-string.h"
#include "gettext.h"

#define _(msgid) dgettext (GNULIB_TEXT_DOMAIN, msgid)


/* Lock that protects the file_cleanup_list from concurrent modification in
   different threads.  */
gl_lock_define_initialized (static, file_cleanup_list_lock)

/* List of all temporary files without temporary directories.  */
static gl_list_t /* <char *> */ volatile file_cleanup_list;


/* List of all temporary directories.  */
struct all_tempdirs dir_cleanup_list /* = { NULL, 0, 0 } */;


/* List of all open file descriptors to temporary files.  */
gl_list_t /* <closeable_fd *> */ volatile descriptors;


/* For the subdirs and for the files, we use a gl_list_t of type LINKEDHASH.
   Why?  We need a data structure that

     1) Can contain an arbitrary number of 'char *' values.  The strings
        are compared via strcmp, not pointer comparison.
     2) Has insertion and deletion operations that are fast: ideally O(1),
        or possibly O(log n).  This is important for GNU sort, which may
        create a large number of temporary files.
     3) Allows iteration through all elements from within a signal handler.
     4) May or may not allow duplicates.  It doesn't matter here, since
        any file or subdir can only be removed once.

   Criterion 1) would allow any gl_list_t or gl_oset_t implementation.

   Criterion 2) leaves only GL_LINKEDHASH_LIST, GL_TREEHASH_LIST, or
   GL_TREE_OSET.

   Criterion 3) puts at disadvantage GL_TREEHASH_LIST and GL_TREE_OSET.
   Namely, iteration through the elements of a binary tree requires access
   to many ->left, ->right, ->parent pointers. However, the rebalancing
   code for insertion and deletion in an AVL or red-black tree is so
   complicated that we cannot assume that >left, ->right, ->parent pointers
   are in a consistent state throughout these operations.  Therefore, to
   avoid a crash in the signal handler, all destructive operations to the
   lists would have to be protected by a
       block_fatal_signals ();
       ...
       unblock_fatal_signals ();
   pair.  Which causes extra system calls.

   Criterion 3) would also discourage GL_ARRAY_LIST and GL_CARRAY_LIST,
   if they were not already excluded.  Namely, these implementations use
   xrealloc(), leaving a time window in which in the list->elements pointer
   points to already deallocated memory.  To avoid a crash in the signal
   handler at such a moment, all destructive operations would have to
   protected by block/unblock_fatal_signals (), in this case too.

   A list of type GL_LINKEDHASH_LIST without duplicates fulfills all
   requirements:
     2) Insertion and deletion are O(1) on average.
     3) The gl_list_iterator, gl_list_iterator_next implementations do
        not trigger memory allocations, nor other system calls, and are
        therefore safe to be called from a signal handler.
        Furthermore, since SIGNAL_SAFE_LIST is defined, the implementation
        of the destructive functions ensures that the list structure is
        safe to be traversed at any moment, even when interrupted by an
        asynchronous signal.
 */


/* The set of fatal signal handlers.
   Cached here because we are not allowed to call get_fatal_signal_set ()
   from a signal handler.  */
static const sigset_t *fatal_signal_set /* = NULL */;

static void
init_fatal_signal_set (void)
{
  if (fatal_signal_set == NULL)
    fatal_signal_set = get_fatal_signal_set ();
}


/* Close a file descriptor.
   Avoids race conditions with normal thread code or signal-handler code that
   might want to close the same file descriptor.  */
_GL_ASYNC_SAFE int
clean_temp_asyncsafe_close (struct closeable_fd *element)
{
  sigset_t saved_mask;
  int ret;
  int saved_errno;

  asyncsafe_spin_lock (&element->lock, true, fatal_signal_set, &saved_mask);
  if (!element->closed)
    {
      ret = close (element->fd);
      saved_errno = errno;
      element->closed = true;
    }
  else
    {
      ret = 0;
      saved_errno = 0;
    }
  asyncsafe_spin_unlock (&element->lock, true, &saved_mask);
  element->done = true;

  errno = saved_errno;
  return ret;
}
/* Initializations for use of this function.  */
void
clean_temp_init_asyncsafe_close (void)
{
  init_fatal_signal_set ();
}

/* The signal handler.  It gets called asynchronously.  */
static _GL_ASYNC_SAFE void
cleanup_action (_GL_UNUSED int sig)
{
  /* First close all file descriptors to temporary files.  */
  {
    gl_list_t fds = descriptors;

    if (fds != NULL)
      {
        gl_list_iterator_t iter = gl_list_iterator (fds);
        const void *element;
        while (gl_list_iterator_next (&iter, &element, NULL))
          {
            clean_temp_asyncsafe_close ((struct closeable_fd *) element);
          }
        gl_list_iterator_free (&iter);
      }
  }

  {
    gl_list_t files = file_cleanup_list;

    if (files != NULL)
      {
        gl_list_iterator_t iter = gl_list_iterator (files);
        const void *element;
        while (gl_list_iterator_next (&iter, &element, NULL))
          {
            const char *file = (const char *) element;
            unlink (file);
          }
        gl_list_iterator_free (&iter);
      }
  }

  for (size_t i = 0; i < dir_cleanup_list.tempdir_count; i++)
    {
      struct tempdir *dir = dir_cleanup_list.tempdir_list[i];

      if (dir != NULL)
        {
          {
            /* First cleanup the files in the subdirectories.  */
            gl_list_iterator_t iter = gl_list_iterator (dir->files);
            const void *element;
            while (gl_list_iterator_next (&iter, &element, NULL))
              {
                const char *file = (const char *) element;
                unlink (file);
              }
            gl_list_iterator_free (&iter);
          }
          {
            /* Then cleanup the subdirectories.  */
            gl_list_iterator_t iter = gl_list_iterator (dir->subdirs);
            const void *element;
            while (gl_list_iterator_next (&iter, &element, NULL))
              {
                const char *subdir = (const char *) element;
                rmdir (subdir);
              }
            gl_list_iterator_free (&iter);
          }

          /* Then cleanup the temporary directory itself.  */
          rmdir (dir->dirname);
        }
    }
}


/* Set to -1 if initialization of this facility failed.  */
static int volatile init_failed /* = 0 */;

/* Initializes this facility.  */
static void
do_clean_temp_init (void)
{
  /* Initialize the data used by the cleanup handler.  */
  init_fatal_signal_set ();
  /* Register the cleanup handler.  */
  if (at_fatal_signal (&cleanup_action) < 0)
    init_failed = -1;
}

/* Ensure that do_clean_temp_init is called once only.  */
gl_once_define(static, clean_temp_once)

/* Initializes this facility upon first use.
   Return 0 upon success, or -1 if there was a memory allocation problem.  */
int
clean_temp_init (void)
{
  gl_once (clean_temp_once, do_clean_temp_init);
  return init_failed;
}


/* Remove a file, with optional error message.
   Return 0 upon success, or -1 if there was some problem.  */
int
clean_temp_unlink (const char *absolute_file_name, bool cleanup_verbose)
{
  if (unlink (absolute_file_name) < 0 && cleanup_verbose
      && errno != ENOENT)
    {
      error (0, errno,
             _("cannot remove temporary file %s"), absolute_file_name);
      return -1;
    }
  return 0;
}


/* ============= Temporary files without temporary directories ============= */

/* Register the given ABSOLUTE_FILE_NAME as being a file that needs to be
   removed.
   Should be called before the file ABSOLUTE_FILE_NAME is created.
   Return 0 upon success, or -1 if there was a memory allocation problem.  */
int
register_temporary_file (const char *absolute_file_name)
{
  bool mt = gl_multithreaded ();

  if (mt) gl_lock_lock (file_cleanup_list_lock);

  int ret = 0;

  /* Make sure that this facility and the file_cleanup_list are initialized.  */
  if (file_cleanup_list == NULL)
    {
      if (clean_temp_init () < 0)
        {
          ret = -1;
          goto done;
        }
      file_cleanup_list =
        gl_list_nx_create_empty (GL_LINKEDHASH_LIST,
                                 hashkey_string_equals,
                                 hashkey_string_hash,
                                 NULL, false);
      if (file_cleanup_list == NULL)
        {
          ret = -1;
          goto done;
        }
    }

  /* Add absolute_file_name to file_cleanup_list, without duplicates.  */
  if (gl_list_search (file_cleanup_list, absolute_file_name) == NULL)
    {
      char *absolute_file_name_copy = strdup (absolute_file_name);
      if (absolute_file_name_copy == NULL)
        {
          ret = -1;
          goto done;
        }
      if (gl_list_nx_add_first (file_cleanup_list, absolute_file_name_copy)
          == NULL)
        {
          free (absolute_file_name_copy);
          ret = -1;
          goto done;
        }
    }

 done:
  if (mt) gl_lock_unlock (file_cleanup_list_lock);

  return ret;
}

/* Unregister the given ABSOLUTE_FILE_NAME as being a file that needs to be
   removed.
   Should be called when the file ABSOLUTE_FILE_NAME could not be created.  */
void
unregister_temporary_file (const char *absolute_file_name)
{
  bool mt = gl_multithreaded ();

  if (mt) gl_lock_lock (file_cleanup_list_lock);

  gl_list_t list = file_cleanup_list;
  if (list != NULL)
    {
      gl_list_node_t node = gl_list_search (list, absolute_file_name);
      if (node != NULL)
        {
          char *old_string = (char *) gl_list_node_value (list, node);

          gl_list_remove_node (list, node);
          free (old_string);
        }
    }

  if (mt) gl_lock_unlock (file_cleanup_list_lock);
}

/* Remove the given ABSOLUTE_FILE_NAME and unregister it.
   CLEANUP_VERBOSE determines whether errors are reported to standard error.
   Return 0 upon success, or -1 if there was some problem.  */
int
cleanup_temporary_file (const char *absolute_file_name, bool cleanup_verbose)
{
  int err = clean_temp_unlink (absolute_file_name, cleanup_verbose);
  unregister_temporary_file (absolute_file_name);

  return err;
}
