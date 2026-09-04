/* Reading PO files.
   Copyright (C) 1995-2026 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible.  */

#ifndef _READ_CATALOG_FILE_H
#define _READ_CATALOG_FILE_H

#include "read-catalog.h"


#ifdef __cplusplus
extern "C" {
#endif


/* Read the input file with the name INPUT_NAME.  The ending .po is added
   if necessary.  If INPUT_NAME is not an absolute file name and the file is
   not found, the list of directories in "dir-list.h" is searched.  Returns
   a list of messages.  */
extern msgdomain_list_ty *
       read_catalog_file (const char *input_name,
                          catalog_input_format_ty input_syntax);


#ifdef __cplusplus
}
#endif


#endif /* _READ_CATALOG_FILE_H */
