/* Command line option handling.  Interactions with diagnostics code.
   Copyright (C) 2010-2019 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#ifndef GCC_OPTS_DIAGNOSTIC_H
#define GCC_OPTS_DIAGNOSTIC_H

extern char *option_name (diagnostic_context *context, int option_index,
			  diagnostic_t orig_diag_kind, diagnostic_t diag_kind);
#endif
