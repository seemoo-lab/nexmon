/* A string or file based input stream.
   Copyright (C) 2024-2026 Free Software Foundation, Inc.

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2024.  */

#ifndef _SF_ISTREAM_H
#define _SF_ISTREAM_H

#include <stdio.h>

#include "string-desc.h"

/* An input stream type that can read from a file or from a string.  */
typedef struct sf_istream sf_istream_t;
struct sf_istream
{
  /* The input file stream, when reading from a file.  */
  FILE *fp;
  /* The input area, when reading from a string.  */
  const char *input;
  const char *input_end;
};

#ifdef __cplusplus
extern "C" {
#endif

/* Initializes STREAM to read from FP.
   FP must be a FILE stream open for reading.  */
extern void sf_istream_init_from_file (sf_istream_t *stream, FILE *fp);
/* Initializes STREAM to read from a NUL-terminated string INPUT.
   The contents of INPUT must stay available and unchanged as long as STREAM
   is in use.  */
extern void sf_istream_init_from_string (sf_istream_t *stream,
                                         const char *input);
/* Initializes STREAM to read from a string INPUT.
   The contents of INPUT must stay available and unchanged as long as STREAM
   is in use.  Operations on STREAM will not modify the contents of INPUT.  */
extern void sf_istream_init_from_string_desc (sf_istream_t *stream,
                                              string_desc_t input);

/* Reads a single 'char' from STREAM, and returns it as an 'unsigned char'.
   Returns EOF when the end of stream was already reached.  */
extern int sf_getc (sf_istream_t *stream);

/* Tests whether STREAM has encountered an error.
   You may want to call this function after sf_getc (stream) has
   returned EOF; in other situations it is guaranteed to return 0.  */
extern int sf_ferror (sf_istream_t *stream);

/* Assuming that C was the last value returned by sf_getc (stream),
   this call pushes back C onto the stream.
   Only 1 character of pushback is guaranteed.  */
extern void sf_ungetc (sf_istream_t *stream, int c);

/* Frees all memory held by STREAM.
   This call has no effect on the arguments provided to sf_istream_init_*.  */
extern void sf_free (sf_istream_t *stream);

#ifdef __cplusplus
}
#endif

#endif /* _SF_ISTREAM_H */
