/* Reading textual message catalogs (such as PO files).
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

#ifndef _READ_CATALOG_H
#define _READ_CATALOG_H

#include "message.h"
#include "read-catalog-abstract.h"
#include "xerror-handler.h"
#include "str-list.h"

#include <stdbool.h>
#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif


/* The following pair of structures cooperate to create a derived class from
   class abstract_catalog_reader_ty.  (See read-catalog-abstract.h for an
   explanation.)

   This derived class stores the contents of the message catalog in memory,
   converting it to an 'msgdomain_list_ty *'.

   It is called the "default" catalog reader because useful variants can be
   implemented with little effort by creating a derived class of class
   default_catalog_reader_ty.  */

/* Forward declaration.  */
struct default_catalog_reader_ty;


typedef struct default_catalog_reader_class_ty default_catalog_reader_class_ty;
struct default_catalog_reader_class_ty
{
  /* Methods inherited from superclass.  */
  struct abstract_catalog_reader_class_ty super;

  /* How to change the current domain.  */
  void (*set_domain) (struct default_catalog_reader_ty *dcatr,
                      char *name, lex_pos_ty *name_pos);

  /* How to add a message to the list.  */
  void (*add_message) (struct default_catalog_reader_ty *dcatr,
                       char *msgctxt,
                       char *msgid, lex_pos_ty *msgid_pos, char *msgid_plural,
                       char *msgstr, size_t msgstr_len, lex_pos_ty *msgstr_pos,
                       char *prev_msgctxt,
                       char *prev_msgid,
                       char *prev_msgid_plural,
                       bool force_fuzzy, bool obsolete);

  /* How to modify a new message before adding it to the list.  */
  void (*frob_new_message) (struct default_catalog_reader_ty *dcatr,
                            message_ty *mp,
                            const lex_pos_ty *msgid_pos,
                            const lex_pos_ty *msgstr_pos);
};


#define DEFAULT_CATALOG_READER_TY \
  ABSTRACT_CATALOG_READER_TY                                            \
                                                                        \
  /* If true, pay attention to comments and filepos comments.  */       \
  bool handle_comments;                                                 \
                                                                        \
  /* If false, domain directives lead to an error message.  */          \
  bool allow_domain_directives;                                         \
                                                                        \
  /* If false, duplicate msgids in the same domain and file generate an \
     error.  If true, such msgids are allowed; the caller should treat  \
     them appropriately.  */                                            \
  bool allow_duplicates;                                                \
                                                                        \
  /* If true, allow duplicates if they have the same translation.  */   \
  bool allow_duplicates_if_same_msgstr;                                 \
                                                                        \
  /* File name used in error messages.  */                              \
  const char *file_name;                                                \
                                                                        \
  /* List of messages already appeared in the current file.  */         \
  msgdomain_list_ty *mdlp;                                              \
                                                                        \
  /* Name of domain we are currently examining.  */                     \
  const char *domain;                                                   \
                                                                        \
  /* List of messages belonging to the current domain.  */              \
  message_list_ty *mlp;                                                 \
                                                                        \
  /* Accumulate comments for next message directive.  */                \
  string_list_ty *comment;                                              \
  string_list_ty *comment_dot;                                          \
                                                                        \
  /* Accumulate filepos comments for the next message directive.  */    \
  size_t filepos_count;                                                 \
  lex_pos_ty *filepos;                                                  \
                                                                        \
  /* Flags transported in special comments.  */                         \
  bool is_fuzzy;                                                        \
  enum is_format is_format[NFORMATS];                                   \
  struct argument_range range;                                          \
  enum is_wrap do_wrap;                                                 \

typedef struct default_catalog_reader_ty default_catalog_reader_ty;
struct default_catalog_reader_ty
{
  DEFAULT_CATALOG_READER_TY
};

extern void default_constructor (abstract_catalog_reader_ty *catr);
extern void default_destructor (abstract_catalog_reader_ty *catr);
extern void default_parse_brief (abstract_catalog_reader_ty *catr);
extern void default_parse_debrief (abstract_catalog_reader_ty *catr);
extern void default_directive_domain (abstract_catalog_reader_ty *catr,
                                      char *name, lex_pos_ty *name_pos);
extern void default_directive_message (abstract_catalog_reader_ty *catr,
                                       char *msgctxt,
                                       char *msgid,
                                       lex_pos_ty *msgid_pos,
                                       char *msgid_plural,
                                       char *msgstr, size_t msgstr_len,
                                       lex_pos_ty *msgstr_pos,
                                       char *prev_msgctxt,
                                       char *prev_msgid,
                                       char *prev_msgid_plural,
                                       bool force_fuzzy, bool obsolete);
extern void default_comment (abstract_catalog_reader_ty *catr, const char *s);
extern void default_comment_dot (abstract_catalog_reader_ty *catr,
                                 const char *s);
extern void default_comment_filepos (abstract_catalog_reader_ty *catr,
                                     const char *file_name, size_t line_number);
extern void default_comment_special (abstract_catalog_reader_ty *catr,
                                     const char *s);
extern void default_set_domain (default_catalog_reader_ty *dcatr,
                                char *name, lex_pos_ty *name_pos);
extern void default_add_message (default_catalog_reader_ty *dcatr,
                                 char *msgctxt,
                                 char *msgid,
                                 lex_pos_ty *msgid_pos,
                                 char *msgid_plural,
                                 char *msgstr, size_t msgstr_len,
                                 lex_pos_ty *msgstr_pos,
                                 char *prev_msgctxt,
                                 char *prev_msgid,
                                 char *prev_msgid_plural,
                                 bool force_fuzzy, bool obsolete);

/* Allocate a fresh default_catalog_reader_ty (or derived class) instance and
   call its constructor.  */
extern default_catalog_reader_ty *
       default_catalog_reader_alloc (default_catalog_reader_class_ty *method_table,
                                     xerror_handler_ty xerror_handler);


/* If false, duplicate msgids in the same domain and file generate an error.
   If true, such msgids are allowed; the caller should treat them
   appropriately.  Defaults to false.  */
extern LIBGETTEXTSRC_DLL_VARIABLE bool allow_duplicates;

/* Read the input file from a stream.  Returns a list of messages.  */
extern msgdomain_list_ty *
       read_catalog_stream (FILE *fp,
                            const char *real_filename,
                            const char *logical_filename,
                            catalog_input_format_ty input_syntax,
                            xerror_handler_ty xerror_handler,
                            string_list_ty *arena);


#ifdef __cplusplus
}
#endif


#endif /* _READ_CATALOG_H */
