/* Header file for CFG cleanup for trees.
   Copyright (C) 2013-2015 Free Software Foundation, Inc.

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

#ifndef GCC_TREE_CFGCLEANUP_H
#define GCC_TREE_CFGCLEANUP_H

/* In tree-cfgcleanup.c  */
extern bitmap cfgcleanup_altered_bbs;
extern bool cleanup_tree_cfg (void);
extern bool fixup_noreturn_call (gimple stmt);

#endif /* GCC_TREE_CFGCLEANUP_H */
