/* Checking of messages in POT files: so-called "syntax checks".
   Copyright (C) 2015-2026 Free Software Foundation, Inc.

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

/* Written by Daiki Ueno.  */

#ifndef _XG_CHECK_H
#define _XG_CHECK_H 1

#include "message.h"


#ifdef __cplusplus
extern "C" {
#endif


/* Perform all checks on a message list.
   Return the number of errors that were seen.  */
extern int xgettext_check_message_list (message_list_ty *mlp);


#ifdef __cplusplus
}
#endif

#endif /* _XG_CHECK_H */
