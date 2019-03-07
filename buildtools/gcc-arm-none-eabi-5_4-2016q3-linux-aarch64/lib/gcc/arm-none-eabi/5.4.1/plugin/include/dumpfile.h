/* Definitions for the shared dumpfile.
   Copyright (C) 2004-2015 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */


#ifndef GCC_DUMPFILE_H
#define GCC_DUMPFILE_H 1

#include "line-map.h"

/* Different tree dump places.  When you add new tree dump places,
   extend the DUMP_FILES array in dumpfile.c.  */
enum tree_dump_index
{
  TDI_none,			/* No dump */
  TDI_cgraph,                   /* dump function call graph.  */
  TDI_inheritance,              /* dump type inheritance graph.  */
  TDI_tu,			/* dump the whole translation unit.  */
  TDI_class,			/* dump class hierarchy.  */
  TDI_original,			/* dump each function before optimizing it */
  TDI_generic,			/* dump each function after genericizing it */
  TDI_nested,			/* dump each function after unnesting it */
  TDI_tree_all,                 /* enable all the GENERIC/GIMPLE dumps.  */
  TDI_rtl_all,                  /* enable all the RTL dumps.  */
  TDI_ipa_all,                  /* enable all the IPA dumps.  */

  TDI_end
};

/* Bit masks to control dumping. Not all values are applicable to all
   dumps. Add new ones at the end. When you define new values, extend
   the DUMP_OPTIONS array in dumpfile.c. The TDF_* flags coexist with
   MSG_* flags (for -fopt-info) and the bit values must be chosen to
   allow that.  */
#define TDF_ADDRESS	(1 << 0)	/* dump node addresses */
#define TDF_SLIM	(1 << 1)	/* don't go wild following links */
#define TDF_RAW  	(1 << 2)	/* don't unparse the function */
#define TDF_DETAILS	(1 << 3)	/* show more detailed info about
					   each pass */
#define TDF_STATS	(1 << 4)	/* dump various statistics about
					   each pass */
#define TDF_BLOCKS	(1 << 5)	/* display basic block boundaries */
#define TDF_VOPS	(1 << 6)	/* display virtual operands */
#define TDF_LINENO	(1 << 7)	/* display statement line numbers */
#define TDF_UID		(1 << 8)	/* display decl UIDs */

#define TDF_TREE	(1 << 9)	/* is a tree dump */
#define TDF_RTL		(1 << 10)	/* is a RTL dump */
#define TDF_IPA		(1 << 11)	/* is an IPA dump */
#define TDF_STMTADDR	(1 << 12)	/* Address of stmt.  */

#define TDF_GRAPH	(1 << 13)	/* a graph dump is being emitted */
#define TDF_MEMSYMS	(1 << 14)	/* display memory symbols in expr.
                                           Implies TDF_VOPS.  */

#define TDF_DIAGNOSTIC	(1 << 15)	/* A dump to be put in a diagnostic
					   message.  */
#define TDF_VERBOSE     (1 << 16)       /* A dump that uses the full tree
					   dumper to print stmts.  */
#define TDF_RHS_ONLY	(1 << 17)	/* a flag to only print the RHS of
					   a gimple stmt.  */
#define TDF_ASMNAME	(1 << 18)	/* display asm names of decls  */
#define TDF_EH		(1 << 19)	/* display EH region number
					   holding this gimple statement.  */
#define TDF_NOUID	(1 << 20)	/* omit UIDs from dumps.  */
#define TDF_ALIAS	(1 << 21)	/* display alias information  */
#define TDF_ENUMERATE_LOCALS (1 << 22)	/* Enumerate locals by uid.  */
#define TDF_CSELIB	(1 << 23)	/* Dump cselib details.  */
#define TDF_SCEV	(1 << 24)	/* Dump SCEV details.  */
#define TDF_COMMENT	(1 << 25)	/* Dump lines with prefix ";;"  */
#define MSG_OPTIMIZED_LOCATIONS  (1 << 26)  /* -fopt-info optimized sources */
#define MSG_MISSED_OPTIMIZATION  (1 << 27)  /* missed opportunities */
#define MSG_NOTE                 (1 << 28)  /* general optimization info */
#define MSG_ALL         (MSG_OPTIMIZED_LOCATIONS | MSG_MISSED_OPTIMIZATION \
                         | MSG_NOTE)


/* Flags to control high-level -fopt-info dumps.  Usually these flags
   define a group of passes.  An optimization pass can be part of
   multiple groups.  */
#define OPTGROUP_NONE        (0)
#define OPTGROUP_IPA         (1 << 1)   /* IPA optimization passes */
#define OPTGROUP_LOOP        (1 << 2)   /* Loop optimization passes */
#define OPTGROUP_INLINE      (1 << 3)   /* Inlining passes */
#define OPTGROUP_VEC         (1 << 4)   /* Vectorization passes */
#define OPTGROUP_OTHER       (1 << 5)   /* All other passes */
#define OPTGROUP_ALL	     (OPTGROUP_IPA | OPTGROUP_LOOP | OPTGROUP_INLINE \
                              | OPTGROUP_VEC | OPTGROUP_OTHER)

/* Define a tree dump switch.  */
struct dump_file_info
{
  const char *suffix;           /* suffix to give output file.  */
  const char *swtch;            /* command line dump switch */
  const char *glob;             /* command line glob  */
  const char *pfilename;        /* filename for the pass-specific stream  */
  const char *alt_filename;     /* filename for the -fopt-info stream  */
  FILE *pstream;                /* pass-specific dump stream  */
  FILE *alt_stream;             /* -fopt-info stream */
  int pflags;                   /* dump flags */
  int optgroup_flags;           /* optgroup flags for -fopt-info */
  int alt_flags;                /* flags for opt-info */
  int pstate;                   /* state of pass-specific stream */
  int alt_state;                /* state of the -fopt-info stream */
  int num;                      /* dump file number */
  bool owns_strings;            /* fields "suffix", "swtch", "glob" can be
				   const strings, or can be dynamically
				   allocated, needing free.  */
};

/* In dumpfile.c */
extern FILE *dump_begin (int, int *);
extern void dump_end (int, FILE *);
extern int opt_info_switch_p (const char *);
extern const char *dump_flag_name (int);
extern void dump_printf (int, const char *, ...) ATTRIBUTE_PRINTF_2;
extern void dump_printf_loc (int, source_location,
                             const char *, ...) ATTRIBUTE_PRINTF_3;
extern void dump_basic_block (int, basic_block, int);
extern void dump_generic_expr_loc (int, source_location, int, tree);
extern void dump_generic_expr (int, int, tree);
extern void dump_gimple_stmt_loc (int, source_location, int, gimple, int);
extern void dump_gimple_stmt (int, int, gimple, int);
extern void print_combine_total_stats (void);
extern bool enable_rtl_dump_file (void);

/* In tree-dump.c  */
extern void dump_node (const_tree, int, FILE *);

/* In combine.c  */
extern void dump_combine_total_stats (FILE *);
/* In cfghooks.c  */
extern void dump_bb (FILE *, basic_block, int, int);

/* Global variables used to communicate with passes.  */
extern FILE *dump_file;
extern FILE *alt_dump_file;
extern int dump_flags;
extern const char *dump_file_name;

/* Return true if any of the dumps is enabled, false otherwise. */
static inline bool
dump_enabled_p (void)
{
  return (dump_file || alt_dump_file);
}

namespace gcc {

class dump_manager
{
public:

  dump_manager ();
  ~dump_manager ();

  /* Register a dumpfile.

     TAKE_OWNERSHIP determines whether callee takes ownership of strings
     SUFFIX, SWTCH, and GLOB. */
  unsigned int
  dump_register (const char *suffix, const char *swtch, const char *glob,
		 int flags, int optgroup_flags,
		 bool take_ownership);

  /* Return the dump_file_info for the given phase.  */
  struct dump_file_info *
  get_dump_file_info (int phase) const;

  struct dump_file_info *
  get_dump_file_info_by_switch (const char *swtch) const;

  /* Return the name of the dump file for the given phase.
     If the dump is not enabled, returns NULL.  */
  char *
  get_dump_file_name (int phase) const;

  char *
  get_dump_file_name (struct dump_file_info *dfi) const;

  int
  dump_switch_p (const char *arg);

  /* Start a dump for PHASE. Store user-supplied dump flags in
     *FLAG_PTR.  Return the number of streams opened.  Set globals
     DUMP_FILE, and ALT_DUMP_FILE to point to the opened streams, and
     set dump_flags appropriately for both pass dump stream and
     -fopt-info stream. */
  int
  dump_start (int phase, int *flag_ptr);

  /* Finish a tree dump for PHASE and close associated dump streams.  Also
     reset the globals DUMP_FILE, ALT_DUMP_FILE, and DUMP_FLAGS.  */
  void
  dump_finish (int phase);

  FILE *
  dump_begin (int phase, int *flag_ptr);

  /* Returns nonzero if tree dump PHASE has been initialized.  */
  int
  dump_initialized_p (int phase) const;

  /* Returns the switch name of PHASE.  */
  const char *
  dump_flag_name (int phase) const;

private:

  int
  dump_phase_enabled_p (int phase) const;

  int
  dump_switch_p_1 (const char *arg, struct dump_file_info *dfi, bool doglob);

  int
  dump_enable_all (int flags, const char *filename);

  int
  opt_info_enable_passes (int optgroup_flags, int flags, const char *filename);

private:

  /* Dynamically registered dump files and switches.  */
  int m_next_dump;
  struct dump_file_info *m_extra_dump_files;
  size_t m_extra_dump_files_in_use;
  size_t m_extra_dump_files_alloced;

  /* Grant access to dump_enable_all.  */
  friend bool ::enable_rtl_dump_file (void);

  /* Grant access to opt_info_enable_passes.  */
  friend int ::opt_info_switch_p (const char *arg);

}; // class dump_manager

} // namespace gcc

#endif /* GCC_DUMPFILE_H */
