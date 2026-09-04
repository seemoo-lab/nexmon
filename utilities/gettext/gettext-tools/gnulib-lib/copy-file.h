/* Copying of files.
   Copyright (C) 2001-2003, 2009-2026 Free Software Foundation, Inc.
   Written by Bruno Haible <haible@clisp.cons.org>, 2001.

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


/* This file uses _GL_ATTRIBUTE_DEPRECATED.  */
#if !_GL_CONFIG_H_INCLUDED
 #error "Please include config.h first."
#endif

#ifdef __cplusplus
extern "C" {
#endif


/* Error codes returned by qcopy_file_preserving or copy_file_to.  */
enum
{
  GL_COPY_ERR_OPEN_READ = -1,
  GL_COPY_ERR_OPEN_BACKUP_WRITE = -2,
  GL_COPY_ERR_READ = -3,
  GL_COPY_ERR_WRITE = -4,
  GL_COPY_ERR_AFTER_READ = -5,
  GL_COPY_ERR_GET_ACL = -6,
  GL_COPY_ERR_SET_ACL = -7
};


/* Copy a regular file: from src_filename to dest_filename.
   The destination file is assumed to be a backup file.
   Modification times, owner, group and access permissions are preserved as
   far as possible (similarly to what 'cp -p SRC DEST' would do).
   Return 0 if successful, otherwise set errno and return one of the error
   codes above.  */
extern int qcopy_file_preserving (const char *src_filename, const char *dest_filename);

/* Copy a regular file: from src_filename to dest_filename.
   The destination file is assumed to be a backup file.
   Modification times, owner, group and access permissions are preserved as
   far as possible (similarly to what 'cp -p SRC DEST' would do).
   Exit upon failure.  */
extern void xcopy_file_preserving (const char *src_filename, const char *dest_filename);

/* Old name of xcopy_file_preserving.  */
_GL_ATTRIBUTE_DEPRECATED void copy_file_preserving (const char *src_filename, const char *dest_filename);


/* Copy a regular file: from src_filename to dest_filename.
   The source file is assumed to be not confidential.
   Modification times, owner, group and access permissions of src_filename
   are *not* copied over to dest_filename (similarly to what 'cat SRC > DEST'
   would do; if DEST already exists, this is the same as what 'cp SRC DEST'
   would do.)
   Return 0 if successful, otherwise set errno and return one of the error
   codes above.  */
extern int copy_file_to (const char *src_filename, const char *dest_filename);

/* Copy a regular file: from src_filename to dest_filename.
   The source file is assumed to be not confidential.
   Modification times, owner, group and access permissions of src_filename
   are *not* copied over to dest_filename (similarly to what 'cat SRC > DEST'
   would do; if DEST already exists, this is the same as what 'cp SRC DEST'
   would do.)
   Exit upon failure.  */
extern void xcopy_file_to (const char *src_filename, const char *dest_filename);


#ifdef __cplusplus
}
#endif
