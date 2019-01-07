/* Reading Java ResourceBundles.
   Copyright (C) 2001-2002, 2015-2016 Free Software Foundation, Inc.
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

#ifndef _READ_JAVA_H
#define _READ_JAVA_H

#include "message.h"

/* Read the Java resource given by resource_name and locale_name.
   Returns a list of messages.  */
extern msgdomain_list_ty *
       msgdomain_read_java (const char *resource_name,
                            const char *locale_name);

#endif /* _READ_JAVA_H */
