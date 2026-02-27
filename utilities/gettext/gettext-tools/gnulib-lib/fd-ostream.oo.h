/* Output stream referring to a file descriptor.
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

#ifndef _FD_OSTREAM_H
#define _FD_OSTREAM_H

#include <stdbool.h>

#include "ostream.h"


struct fd_ostream : struct ostream
{
methods:
};


#ifdef __cplusplus
extern "C" {
#endif


/* Create an output stream referring to the file descriptor FD.
   FILENAME is used only for error messages.
   Note that the resulting stream must be closed before FD can be closed.  */
extern fd_ostream_t fd_ostream_create (int fd, const char *filename,
                                       bool buffered);


#ifdef __cplusplus
}
#endif

#endif /* _FD_OSTREAM_H */
