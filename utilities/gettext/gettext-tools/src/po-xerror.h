/* Error handling during reading and writing of PO files.
   Copyright (C) 2005-2026 Free Software Foundation, Inc.

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

#ifndef _PO_XERROR_H
#define _PO_XERROR_H

/* A thin wrapper around xerror-handler.h.  */
#include "xerror-handler.h"

#define PO_SEVERITY_WARNING     CAT_SEVERITY_WARNING     /* just a warning, tell the user */
#define PO_SEVERITY_ERROR       CAT_SEVERITY_ERROR       /* an error, the operation cannot complete */
#define PO_SEVERITY_FATAL_ERROR CAT_SEVERITY_FATAL_ERROR /* an error, the operation must be aborted */

#define po_xerror  (textmode_xerror_handler->xerror)
#define po_xerror2 (textmode_xerror_handler->xerror2)

#endif /* _PO_XERROR_H */
