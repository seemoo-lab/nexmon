/* Output stream that converts the output to another encoding.
   Copyright (C) 2006, 2015-2016 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2006.

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

#ifndef _ICONV_OSTREAM_H
#define _ICONV_OSTREAM_H

/* Note that this stream does not provide accurate error messages with line
   and column number when the conversion fails.  */

#include "ostream.h"


struct iconv_ostream : struct ostream
{
methods:
};


#ifdef __cplusplus
extern "C" {
#endif


#if HAVE_ICONV

/* Create an output stream that converts from FROM_ENCODING to TO_ENCODING,
   writing the result to DESTINATION.  */
extern iconv_ostream_t iconv_ostream_create (const char *from_encoding,
                                             const char *to_encoding,
                                             ostream_t destination);

#endif /* HAVE_ICONV */


#ifdef __cplusplus
}
#endif

#endif /* _ICONV_OSTREAM_H */
