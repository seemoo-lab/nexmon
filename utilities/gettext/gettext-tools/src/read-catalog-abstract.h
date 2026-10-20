/* Reading textual message catalogs (such as PO files), abstract class.
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

/* Written by Peter Miller, Ulrich Drepper, and Bruno Haible.  */

#ifndef _READ_CATALOG_ABSTRACT_H
#define _READ_CATALOG_ABSTRACT_H

#include <stdbool.h>
#include <stdio.h>

#include "message.h"
#include "xerror-handler.h"
#include "str-list.h"


#ifdef __cplusplus
extern "C" {
#endif


/* Note: the _t suffix is reserved by ANSI C, so the _ty suffix is
   used to indicate a type name.  */

/* The following pair of structures cooperate to create an "Object" in
   the OO sense.  We are simply doing it manually, rather than with the
   help of an OO compiler.  This implementation allows polymorphism
   and inheritance - more than enough for the immediate needs.  */

/* This abstract base class implements the parsing of the catalog file.
   Several syntaxes are supported (see type catalog_input_format_ty below).
   Derived classes implement methods that are invoked when a particular
   element (message, comment, etc.) is seen.  */

/* Forward declaration.  */
struct abstract_catalog_reader_ty;


/* This first structure, playing the role of the "Class" in OO sense,
   contains pointers to functions.  Each function is a method for the
   class (base or derived).  Use a NULL pointer where no action is
   required.  */

typedef struct abstract_catalog_reader_class_ty
        abstract_catalog_reader_class_ty;
struct abstract_catalog_reader_class_ty
{
  /* How many bytes to malloc for an instance of this class.  */
  size_t size;

  /* What to do immediately after the instance is malloc()ed.  */
  void (*constructor) (struct abstract_catalog_reader_ty *catr);

  /* What to do immediately before the instance is free()ed.  */
  void (*destructor) (struct abstract_catalog_reader_ty *catr);

  /* This method is invoked before the parse, but after the file is
     opened by the lexer.  */
  void (*parse_brief) (struct abstract_catalog_reader_ty *catr);

  /* This method is invoked after the parse, but before the file is
     closed by the lexer.  The intention is to make consistency checks
     against the file here, and emit the errors through the lex_error*
     functions.  */
  void (*parse_debrief) (struct abstract_catalog_reader_ty *catr);

  /* What to do with a domain directive.  */
  void (*directive_domain) (struct abstract_catalog_reader_ty *catr,
                            char *name, lex_pos_ty *name_pos);

  /* What to do with a message directive.  */
  void (*directive_message) (struct abstract_catalog_reader_ty *catr,
                             char *msgctxt,
                             char *msgid, lex_pos_ty *msgid_pos,
                             char *msgid_plural,
                             char *msgstr, size_t msgstr_len,
                             lex_pos_ty *msgstr_pos,
                             char *prev_msgctxt,
                             char *prev_msgid, char *prev_msgid_plural,
                             bool force_fuzzy, bool obsolete);

  /* What to do with a plain-vanilla comment.  The expectation is that
     they will be accumulated, and added to the next message
     definition seen.  Or completely ignored.  */
  void (*comment) (struct abstract_catalog_reader_ty *catr, const char *s);

  /* What to do with a comment that starts with a dot (i.e. extracted
     by xgettext).  The expectation is that they will be accumulated,
     and added to the next message definition seen.  Or completely
     ignored.  */
  void (*comment_dot) (struct abstract_catalog_reader_ty *catr, const char *s);

  /* What to do with a file position seen in a comment (i.e. a message
     location comment extracted by xgettext).  The expectation is that
     they will be accumulated, and added to the next message
     definition seen.  Or completely ignored.  */
  void (*comment_filepos) (struct abstract_catalog_reader_ty *catr,
                           const char *file_name, size_t line_number);

  /* What to do with a comment that starts with a ',' or '!'; this is a
     special comment.  One of the possible uses is to indicate a
     inexact translation.  */
  void (*comment_special) (struct abstract_catalog_reader_ty *catr,
                           const char *s);
};


/* This next structure defines the base class passed to the methods.
   Derived methods will often need to cast their first argument before
   using it (this corresponds to the implicit 'this' argument in C++).

   When declaring derived classes, use the ABSTRACT_CATALOG_READER_TY define
   at the start of the structure, to declare inherited instance variables,
   etc.  */

#define ABSTRACT_CATALOG_READER_TY \
  abstract_catalog_reader_class_ty *methods;                            \
                                                                        \
  /* The error handler.  */                                             \
  xerror_handler_ty xeh;                                                \
                                                                        \
  /* True if comments shall be handled, false if they shall be          \
     ignored. */                                                        \
  bool pass_comments;                                                   \
                                                                        \
  /* True if obsolete entries shall be considered as valid.  */         \
  bool pass_obsolete_entries;                                           \
                                                                        \
  /* Representation of U+2068 FIRST STRONG ISOLATE (FSI) in the         \
     PO file's encoding, or NULL if not available.  */                  \
  const char *po_lex_isolate_start;                                     \
  /* Representation of U+2069 POP DIRECTIONAL ISOLATE (PDI) in the      \
     PO file's encoding, or NULL if not available.  */                  \
  const char *po_lex_isolate_end;                                       \

typedef struct abstract_catalog_reader_ty abstract_catalog_reader_ty;
struct abstract_catalog_reader_ty
{
  ABSTRACT_CATALOG_READER_TY
};


/* This structure describes a textual catalog input format.  */
struct catalog_input_format
{
  /* Parses the contents of FP, invoking the appropriate callbacks.  */
  void (*parse) (abstract_catalog_reader_ty *catr, FILE *fp,
                 const char *real_filename, const char *logical_filename,
                 bool is_pot_role, string_list_ty *arena);

  /* Whether the parse function always produces messages encoded in UTF-8
     encoding.  */
  bool produces_utf8;
};

typedef const struct catalog_input_format * catalog_input_format_ty;


/* Allocate a fresh abstract_catalog_reader_ty (or derived class) instance and
   call its constructor.  */
extern abstract_catalog_reader_ty *
       catalog_reader_alloc (abstract_catalog_reader_class_ty *method_table,
                             xerror_handler_ty xerror_handler);

/* Read a PO file from a stream, and dispatch to the various
   abstract_catalog_reader_class_ty methods.  */
extern void
       catalog_reader_parse (abstract_catalog_reader_ty *catr, FILE *fp,
                             const char *real_filename,
                             const char *logical_filename,
                             bool is_pot_role,
                             catalog_input_format_ty input_syntax,
                             string_list_ty *arena);

/* Call the destructor and deallocate a abstract_catalog_reader_ty (or derived
   class) instance.  */
extern void
       catalog_reader_free (abstract_catalog_reader_ty *catr);


/* Callbacks used by read-po-gram.y, read-properties.c, read-stringtable.c,
   indirectly from catalog_reader_parse.  */
/* This callback is called whenever a domain directive has been seen.
   It invokes the 'directive_domain' method.  */
extern void
       catalog_reader_seen_domain (abstract_catalog_reader_ty *catr,
                                   char *name, lex_pos_ty *name_pos);
/* This callback is called whenever a message has been seen.
   It invokes the 'directive_message' method.  */
extern void
       catalog_reader_seen_message (abstract_catalog_reader_ty *catr,
                                    char *msgctxt,
                                    char *msgid, lex_pos_ty *msgid_pos,
                                    char *msgid_plural,
                                    char *msgstr, size_t msgstr_len,
                                    lex_pos_ty *msgstr_pos,
                                    char *prev_msgctxt,
                                    char *prev_msgid, char *prev_msgid_plural,
                                    bool force_fuzzy, bool obsolete);
/* This callback is called whenever a plain comment (a.k.a. translator comment)
   has been seen.  It invokes the 'comment' method.  */
extern void
       catalog_reader_seen_comment (abstract_catalog_reader_ty *catr,
                                    const char *s);
/* This callback is called whenever a dot comment (a.k.a. extracted comment)
   has been seen.  It invokes the 'comment_dot' method.  */
extern void
       catalog_reader_seen_comment_dot (abstract_catalog_reader_ty *catr,
                                        const char *s);
/* This callback is called whenever a source file reference has been seen.
   It invokes the 'comment_filepos' method.  */
extern void
       catalog_reader_seen_comment_filepos (abstract_catalog_reader_ty *catr,
                                            const char *file_name,
                                            size_t line_number);
/* This callback is called whenever a special comment (#,) has been seen.
   It invokes the 'comment_special' method.  */
extern void
       catalog_reader_seen_comment_special (abstract_catalog_reader_ty *catr,
                                            const char *s);
/* This callback is called whenever a generic comment line has been seen.
   It parses s and invokes the appropriate method.  */
extern void
       catalog_reader_seen_generic_comment (abstract_catalog_reader_ty *catr,
                                            const char *s);


#ifdef __cplusplus
}
#endif


#endif /* _READ_CATALOG_ABSTRACT_H */
