/* Copying of files.
   Copyright (C) 2001-2003, 2006-2007, 2009-2016 Free Software Foundation, Inc.
   Written by Bruno Haible <haible@clisp.cons.org>, 2001.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */


#include <config.h>

/* Specification.  */
#include "copy-file.h"

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#if HAVE_UTIME || HAVE_UTIMES
# if HAVE_UTIME_H
#  include <utime.h>
# else
#  include <sys/utime.h>
# endif
#endif

#include "error.h"
#include "ignore-value.h"
#include "safe-read.h"
#include "full-write.h"
#include "acl.h"
#include "binary-io.h"
#include "quote.h"
#include "gettext.h"
#include "xalloc.h"

#define _(str) gettext (str)

enum { IO_SIZE = 32 * 1024 };

int
qcopy_file_preserving (const char *src_filename, const char *dest_filename)
{
  int err = 0;
  int src_fd;
  struct stat statbuf;
  int mode;
  int dest_fd;
  char *buf = xmalloc (IO_SIZE);

  src_fd = open (src_filename, O_RDONLY | O_BINARY);
  if (src_fd < 0)
    {
      err = GL_COPY_ERR_OPEN_READ;
      goto error;
    }
  if (fstat (src_fd, &statbuf) < 0)
    {
      err = GL_COPY_ERR_OPEN_READ;
      goto error_src;
    }

  mode = statbuf.st_mode & 07777;

  dest_fd = open (dest_filename, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0600);
  if (dest_fd < 0)
    {
      err = GL_COPY_ERR_OPEN_BACKUP_WRITE;
      goto error_src;
    }

  /* Copy the file contents.  */
  for (;;)
    {
      size_t n_read = safe_read (src_fd, buf, IO_SIZE);
      if (n_read == SAFE_READ_ERROR)
        {
          err = GL_COPY_ERR_READ;
          goto error_src_dest;
        }
      if (n_read == 0)
        break;

      if (full_write (dest_fd, buf, n_read) < n_read)
        {
          err = GL_COPY_ERR_WRITE;
          goto error_src_dest;
        }
    }

  free (buf);
  buf = NULL; /* To avoid double free in error case.  */

#if !USE_ACL
  if (close (dest_fd) < 0)
    {
      err = GL_COPY_ERR_WRITE;
      goto error_src;
    }
  if (close (src_fd) < 0)
    {
      err = GL_COPY_ERR_AFTER_READ;
      goto error;
    }
#endif

  /* Preserve the access and modification times.  */
#if HAVE_UTIME
  {
    struct utimbuf ut;

    ut.actime = statbuf.st_atime;
    ut.modtime = statbuf.st_mtime;
    utime (dest_filename, &ut);
  }
#elif HAVE_UTIMES
  {
    struct timeval ut[2];

    ut[0].tv_sec = statbuf.st_atime; ut[0].tv_usec = 0;
    ut[1].tv_sec = statbuf.st_mtime; ut[1].tv_usec = 0;
    utimes (dest_filename, &ut);
  }
#endif

#if HAVE_CHOWN
  /* Preserve the owner and group.  */
  ignore_value (chown (dest_filename, statbuf.st_uid, statbuf.st_gid));
#endif

  /* Preserve the access permissions.  */
#if USE_ACL
  switch (qcopy_acl (src_filename, src_fd, dest_filename, dest_fd, mode))
    {
    case -2:
      err = GL_COPY_ERR_GET_ACL;
      goto error_src_dest;
    case -1:
      err = GL_COPY_ERR_SET_ACL;
      goto error_src_dest;
    }
#else
  chmod (dest_filename, mode);
#endif

#if USE_ACL
  if (close (dest_fd) < 0)
    {
      err = GL_COPY_ERR_WRITE;
      goto error_src;
    }
  if (close (src_fd) < 0)
    {
      err = GL_COPY_ERR_AFTER_READ;
      goto error;
    }
#endif

  return 0;

 error_src_dest:
  close (dest_fd);
 error_src:
  close (src_fd);
 error:
  free (buf);
  return err;
}

void
copy_file_preserving (const char *src_filename, const char *dest_filename)
{
  switch (qcopy_file_preserving (src_filename, dest_filename))
    {
    case 0:
      return;

    case GL_COPY_ERR_OPEN_READ:
      error (EXIT_FAILURE, errno, _("error while opening %s for reading"),
             quote (src_filename));

    case GL_COPY_ERR_OPEN_BACKUP_WRITE:
      error (EXIT_FAILURE, errno, _("cannot open backup file %s for writing"),
             quote (dest_filename));

    case GL_COPY_ERR_READ:
      error (EXIT_FAILURE, errno, _("error reading %s"),
             quote (src_filename));

    case GL_COPY_ERR_WRITE:
      error (EXIT_FAILURE, errno, _("error writing %s"),
             quote (dest_filename));

    case GL_COPY_ERR_AFTER_READ:
      error (EXIT_FAILURE, errno, _("error after reading %s"),
             quote (src_filename));

    case GL_COPY_ERR_GET_ACL:
      error (EXIT_FAILURE, errno, "%s", quote (src_filename));

    case GL_COPY_ERR_SET_ACL:
      error (EXIT_FAILURE, errno, _("preserving permissions for %s"),
             quote (dest_filename));

    default:
      abort ();
    }
}
