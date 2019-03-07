/* Header file for tree data flow functions.
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

#ifndef GCC_TREE_DFA_H
#define GCC_TREE_DFA_H

extern void renumber_gimple_stmt_uids (void);
extern void renumber_gimple_stmt_uids_in_blocks (basic_block *, int);
extern void dump_variable (FILE *, tree);
extern void debug_variable (tree);
extern void dump_dfa_stats (FILE *);
extern void debug_dfa_stats (void);
extern tree ssa_default_def (struct function *, tree);
extern void set_ssa_default_def (struct function *, tree, tree);
extern tree get_or_create_ssa_default_def (struct function *, tree);
extern tree get_ref_base_and_extent (tree, HOST_WIDE_INT *,
				     HOST_WIDE_INT *, HOST_WIDE_INT *);
extern tree get_addr_base_and_unit_offset_1 (tree, HOST_WIDE_INT *,
					     tree (*) (tree));
extern tree get_addr_base_and_unit_offset (tree, HOST_WIDE_INT *);
extern bool stmt_references_abnormal_ssa_name (gimple);
extern void dump_enumerated_decls (FILE *, int);


#endif /* GCC_TREE_DFA_H */
