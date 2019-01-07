/* Reading file lists.
   Copyright (C) 2001-2003, 2015-2016 Free Software Foundation, Inc.
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

#ifndef _FILE_LIST_H
#define _FILE_LIST_H

#include "str-list.h"


#ifdef __cplusplus
extern "C" {
#endif


/* Read list of filenames from a file.
   One filename per line.  Lines starting with # and whitespace lines are
   ignored.  Trailing whitespace is removed.  */
extern string_list_ty *read_names_from_file (const char *file_name);


#ifdef __cplusplus
}
#endif


#endif /* _FILE_LIST_H */
