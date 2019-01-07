/* xgettext glade backend.
   Copyright (C) 2002-2003, 2006, 2015-2016 Free Software Foundation, Inc.
   Written by Bruno Haible <haible@clisp.cons.org>, 2002.

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


#include <stdio.h>

#include "message.h"
#include "xgettext.h"


#ifdef __cplusplus
extern "C" {
#endif


/* The scanner is implemented as ITS rules, in its/glade[12].its and
   its/gtkbuilder.its.  */

#define EXTENSIONS_GLADE                                             \
  { "glade",     NULL    },                                          \
  { "glade2",    NULL    },                                          \
  { "ui",        NULL    },                                          \

#define SCANNERS_GLADE \
  { "glade",            NULL, NULL, NULL, NULL, NULL },              \


#ifdef __cplusplus
}
#endif
