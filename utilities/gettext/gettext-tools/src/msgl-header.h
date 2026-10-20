/* Message list header manipulation.
   Copyright (C) 2007-2026 Free Software Foundation, Inc.

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

#ifndef _MSGL_HEADER_H
#define _MSGL_HEADER_H

#include "message.h"


#ifdef __cplusplus
extern "C" {
#endif


/* Set the 'charset' value in the 'Content-Type:' field to the given value.
   HEADER_MP is a message that satisfies the is_header() predicate.
   CHARSETSTR is a pointer into its msgstr, right after the "charset=" substring.
   VALUE is the new charset value.  */
extern void
       header_set_charset (message_ty *header_mp, const char *charsetstr,
                           const char *value);

/* Set the given field to the given value.
   The FIELD name ends in a colon.
   The VALUE will have a space prepended and a newline appended by this
   function.  */
extern void
       msgdomain_list_set_header_field (msgdomain_list_ty *mdlp,
                                        const char *field, const char *value);

/* Remove the given field from the header.
   The FIELD name ends in a colon.  */
extern void
       message_list_delete_header_field (message_list_ty *mlp,
                                         const char *field);


#ifdef __cplusplus
}
#endif


#endif /* _MSGL_HEADER_H */
