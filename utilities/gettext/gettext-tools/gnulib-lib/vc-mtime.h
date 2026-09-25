/* Return the version-control based modification time of a file.
   Copyright (C) 2025-2026 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2025.  */

#ifndef _VC_MTIME_H
#define _VC_MTIME_H

/* Get struct timespec.  */
#include <time.h>

/* The "version-controlled modification time" vc_mtime(F) of a file F
   is defined as:
     - If F is under version control and not modified locally:
       the time of the last change of F in the version control system.
     - Otherwise: The modification time of F on disk.

   For now, the only VCS supported by this module is git.  (hg and svn are
   hardly in use any more.)

   This has the properties that:
     - Different users who have checked out the same git repo on different
       machines, at different times, and not done local modifications,
       get the same vc_mtime(F).
     - If a user has modified F locally, the modification time of that file
       counts.
     - If that user then reverts the modification, they then again get the
       same vc_mtime(F) as everyone else.
     - Different users who have unpacked the same tarball (without .git
       directory) on different machines, at different times, also get the same
       vc_mtime(F) [but possibly a different one than when the .git directory
       was present].  (Assuming a POSIX compliant file system.)
     - When a user commits local modifications into git, this only increases
       (not decreases) the vc_mtime(F).

   The purpose of the version-controlled modification time is to produce a
   reproducible timestamp(Z) of a file Z that depends on files X1, ..., Xn,
   in such a way that
     - timestamp(Z) is reproducible, that is, different users on different
       machines get the same value.
     - timestamp(Z) is related to reality.  It's not just a dummy, like what
       is suggested in <https://reproducible-builds.org/docs/timestamps/>.
     - One can arrange for timestamp(Z) to respect the modification time
       relations of a build system.

   There are two uses of such a timestamp:
     - It can be set as the modification time of file Z in a file system, or
     - It can be embedded in Z, with the purpose of telling a user how old
       the file Z is.  For example, in PDF files or in generated documentation,
       such a time is embedded in a special place.

   The simplest example is a file Z that depends on files X1, ..., Xn.
   Generally one will define
     timestamp(Z) = max (vc_mtime(X1), ..., vc_mtime(Xn))
   for an embedded timestamp, or
     timestamp(Z) = max (vc_mtime(X1), ..., vc_mtime(Xn)) + 1 second
   for a time stamp in a file system.  The added second
     1. accounts for fractional seconds in mtime(X1), ..., mtime(Xn),
     2. allows for 'make' implementation that attempt to rebuild Z
        if mtime(Z) == mtime(Xi).

   A more complicated example is when there are intermediate built files, not
   under version control. For example, if the build process produces
     X1, X2 -> Y1
     X3, X4 -> Y2
     Y1, Y2, X5 -> Z
   where Y1 and Y2 are intermediate built files, you should ignore the
   mtime(Y1), mtime(Y2), and consider only the vc_mtime(X1), ..., vc_mtime(X5).
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Determines the version-controlled modification time of FILENAME, stores it
   in *MTIME, and returns 0.
   Upon failure, it returns -1.  */
extern int vc_mtime (struct timespec *mtime, const char *filename);

/* Determines the maximum of the version-controlled modification times of
   FILENAMES[0..NFILES-1], and returns 0.
   Upon failure, it returns -1.
   NFILES must be > 0.  */
extern int max_vc_mtime (struct timespec *max_of_mtimes,
                         size_t nfiles, const char * const *filenames);

#ifdef __cplusplus
}
#endif

#endif /* _VC_MTIME_H */
