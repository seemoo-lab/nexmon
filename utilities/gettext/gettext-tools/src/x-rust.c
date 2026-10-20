/* xgettext Rust backend.
   Copyright (C) 2001-2026 Free Software Foundation, Inc.

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

#include <config.h>

/* Specification.  */
#include "x-rust.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <error.h>
#include "message.h"
#include "string-desc.h"
#include "xstring-desc.h"
#include "string-buffer.h"
#include "xgettext.h"
#include "xg-pos.h"
#include "xg-mixed-string.h"
#include "xg-arglist-context.h"
#include "xg-arglist-callshape.h"
#include "xg-arglist-parser.h"
#include "xg-message.h"
#include "if-error.h"
#include "xalloc.h"
#include "read-file.h"
#include "unistr.h"
#include "po-charset.h"
#include "gettext.h"

#define _(s) gettext(s)

/* Use tree-sitter.
   Documentation: <https://tree-sitter.github.io/tree-sitter/using-parsers>  */
#include <tree_sitter/api.h>
extern const TSLanguage *tree_sitter_rust (void);


/* The Rust syntax is defined in https://doc.rust-lang.org/1.84.0/reference/index.html.
   String syntax:
   https://doc.rust-lang.org/1.84.0/reference/tokens.html#character-and-string-literals
 */

#define DEBUG_RUST 0


/* ====================== Keyword set customization.  ====================== */

/* If true extract all strings.  */
static bool extract_all = false;

static hash_table function_keywords;
static hash_table macro_keywords;
static bool default_keywords = true;


void
x_rust_extract_all ()
{
  extract_all = true;
}


void
x_rust_keyword (const char *name)
{
  if (name == NULL)
    default_keywords = false;
  else
    {
      if (function_keywords.table == NULL)
        hash_init (&function_keywords, 100);
      if (macro_keywords.table == NULL)
        hash_init (&macro_keywords, 100);

      const char *end;
      struct callshape shape;
      split_keywordspec (name, &end, &shape);

      /* The characters between name and end should form a valid Rust
         identifier, possibly with a trailing '!'.
         A colon means an invalid parse in split_keywordspec().  */
      const char *colon = strchr (name, ':');
      if (colon == NULL || colon >= end)
        {
          if (end > name && end[-1] == '!')
            insert_keyword_callshape (&macro_keywords, name, end - 1 - name,
                                      &shape);
          else
            insert_keyword_callshape (&function_keywords, name, end - name,
                                      &shape);
        }
    }
}

/* Finish initializing the keywords hash table.
   Called after argument processing, before each file is processed.  */
static void
init_keywords ()
{
  if (default_keywords)
    {
      /* These are the functions defined by the 'gettext-rs' Rust package.
         https://docs.rs/gettext-rs/latest/gettextrs/#functions  */
      /* When adding new keywords here, also update the documentation in
         xgettext.texi!  */
      x_rust_keyword ("gettext");
      x_rust_keyword ("dgettext:2");
      x_rust_keyword ("dcgettext:2");
      x_rust_keyword ("ngettext:1,2");
      x_rust_keyword ("dngettext:2,3");
      x_rust_keyword ("dcngettext:2,3");
      x_rust_keyword ("pgettext:1c,2");
      x_rust_keyword ("npgettext:1c,2,3");
      default_keywords = false;
    }
}

/* The flag_table_rust is split into two tables, one for functions and one for
   macros.  */
flag_context_list_table_ty flag_table_rust_functions;
flag_context_list_table_ty flag_table_rust_macros;

void
init_flag_table_rust ()
{
  /* These are the functions defined by the 'gettext-rs' Rust package.
     https://docs.rs/gettext-rs/latest/gettextrs/#functions  */
  xgettext_record_flag ("gettext:1:pass-rust-format");
  xgettext_record_flag ("dgettext:2:pass-rust-format");
  xgettext_record_flag ("dcgettext:2:pass-rust-format");
  xgettext_record_flag ("ngettext:1:pass-rust-format");
  xgettext_record_flag ("ngettext:2:pass-rust-format");
  xgettext_record_flag ("dngettext:2:pass-rust-format");
  xgettext_record_flag ("dngettext:3:pass-rust-format");
  xgettext_record_flag ("dcngettext:2:pass-rust-format");
  xgettext_record_flag ("dcngettext:3:pass-rust-format");
  xgettext_record_flag ("pgettext:2:pass-rust-format");
  xgettext_record_flag ("npgettext:2:pass-rust-format");
  xgettext_record_flag ("npgettext:3:pass-rust-format");
  /* These are the functions whose argument is a format string.
     https://github.com/clitic/formatx  */
  xgettext_record_flag ("formatx!:1:rust-format");
}


/* ======================== Parsing via tree-sitter. ======================== */
/* To understand this code, look at
     tree-sitter-rust/src/node-types.json
   and
     tree-sitter-rust/src/grammar.json
 */

/* The tree-sitter's language object.  */
static const TSLanguage *ts_language;

/* ------------------------- Node types and symbols ------------------------- */

static TSSymbol
ts_language_symbol (const char *name, bool is_named)
{
  TSSymbol result =
    ts_language_symbol_for_name (ts_language, name, strlen (name), is_named);
  if (result == 0)
    /* If we get here, the grammar has evolved in an incompatible way.  */
    abort ();
  return result;
}

static TSFieldId
ts_language_field (const char *name)
{
  TSFieldId result =
    ts_language_field_id_for_name (ts_language, name, strlen (name));
  if (result == 0)
    /* If we get here, the grammar has evolved in an incompatible way.  */
    abort ();
  return result;
}

/* Optimization:
   Instead of
     strcmp (ts_node_type (node), "string_literal") == 0
   it is faster to do
     ts_node_symbol (node) == ts_symbol_string_literal
 */
static TSSymbol ts_symbol_line_comment;
static TSSymbol ts_symbol_block_comment;
static TSSymbol ts_symbol_string_literal;
static TSSymbol ts_symbol_raw_string_literal;
static TSSymbol ts_symbol_string_content;
static TSSymbol ts_symbol_escape_sequence;
static TSSymbol ts_symbol_identifier;
static TSSymbol ts_symbol_scoped_identifier;
static TSSymbol ts_symbol_call_expression;
static TSSymbol ts_symbol_macro_invocation;
static TSSymbol ts_symbol_arguments;
static TSSymbol ts_symbol_token_tree;
static TSSymbol ts_symbol_open_paren; /* ( */
static TSSymbol ts_symbol_close_paren; /* ) */
static TSSymbol ts_symbol_comma; /* , */
static TSSymbol ts_symbol_exclam; /* ! */
static TSFieldId ts_field_function;
static TSFieldId ts_field_arguments;
static TSFieldId ts_field_macro;

static inline size_t
ts_node_line_number (TSNode node)
{
  return ts_node_start_point (node).row + 1;
}

/* -------------------------------- The file -------------------------------- */

/* The entire contents of the file being analyzed.  */
static const char *contents;

/* -------------------------------- Comments -------------------------------- */

/* These are for tracking whether comments count as immediately before
   keyword.  */
static int last_comment_line;
static int last_non_comment_line;

/* Saves a comment line.  */
static void save_comment_line (string_desc_t gist)
{
  /* Remove leading whitespace.  */
  while (sd_length (gist) > 0
         && (sd_char_at (gist, 0) == ' '
             || sd_char_at (gist, 0) == '\t'))
    gist = sd_substring (gist, 1, sd_length (gist));
  /* Remove trailing whitespace.  */
  size_t len = sd_length (gist);
  while (len > 0
         && (sd_char_at (gist, len - 1) == ' '
             || sd_char_at (gist, len - 1) == '\t'))
    len--;
  gist = sd_substring (gist, 0, len);
  savable_comment_add (sd_c (gist));
}

/* Does the comment handling for NODE.
   Updates savable_comment, last_comment_line, last_non_comment_line.
   It is important that this function gets called
     - for each node (not only the named nodes!),
     - in depth-first traversal order.  */
static void handle_comments (TSNode node)
{
  #if DEBUG_RUST
  fprintf (stderr, "LCL=%d LNCL=%d node=[%s]|%s|\n", last_comment_line, last_non_comment_line, ts_node_type (node), ts_node_string (node));
  #endif
  if (last_comment_line < last_non_comment_line
      && last_non_comment_line < ts_node_line_number (node))
    /* We have skipped over a newline.  This newline terminated a line
       with non-comment tokens, after the last comment line.  */
    savable_comment_reset ();

  if (ts_node_symbol (node) == ts_symbol_line_comment)
    {
      string_desc_t entire =
        sd_new_addr (ts_node_end_byte (node) - ts_node_start_byte (node),
                     contents + ts_node_start_byte (node));
      /* It should start with two slashes.  */
      if (!(sd_length (entire) >= 2
            && sd_char_at (entire, 0) == '/'
            && sd_char_at (entire, 1) == '/'))
        abort ();
      save_comment_line (sd_substring (entire, 2, sd_length (entire)));
      last_comment_line = ts_node_end_point (node).row + 1;
    }
  else if (ts_node_symbol (node) == ts_symbol_block_comment)
    {
      string_desc_t entire =
        sd_new_addr (ts_node_end_byte (node) - ts_node_start_byte (node),
                     contents + ts_node_start_byte (node));
      /* It should start and end with the C comment markers.  */
      if (!(sd_length (entire) >= 4
            && sd_char_at (entire, 0) == '/'
            && sd_char_at (entire, 1) == '*'
            && sd_char_at (entire, sd_length (entire) - 2) == '*'
            && sd_char_at (entire, sd_length (entire) - 1) == '/'))
        abort ();
      string_desc_t gist = sd_substring (entire, 2, sd_length (entire) - 2);
      /* Split into lines.
         Remove leading and trailing whitespace from each line.  */
      for (;;)
        {
          ptrdiff_t nl_index = sd_index (gist, '\n');
          if (nl_index >= 0)
            {
              save_comment_line (sd_substring (gist, 0, nl_index));
              gist = sd_substring (gist, nl_index + 1, sd_length (gist));
            }
          else
            {
              save_comment_line (gist);
              break;
            }
        }
      last_comment_line = ts_node_end_point (node).row + 1;
    }
  else
    last_non_comment_line = ts_node_line_number (node);
}

/* ---------------------------- String literals ---------------------------- */

/* Combines the pieces of a string_literal or raw_string_literal.
   Returns a freshly allocated UTF-8 encoded string.  */
static char *
string_literal_value (TSNode node)
{
  if (ts_node_named_child_count (node) == 1)
    {
      TSNode subnode = ts_node_named_child (node, 0);
      if (ts_node_symbol (subnode) == ts_symbol_string_content)
        {
          /* Optimize the frequent special case of a string literal
             that is non-empty and has no escape sequences.  */
          string_desc_t subnode_string =
            sd_new_addr (ts_node_end_byte (subnode) - ts_node_start_byte (subnode),
                         contents + ts_node_start_byte (subnode));
          return xsd_c (subnode_string);
        }
    }

  /* The general case.  */
  struct string_buffer buffer;
  sb_init (&buffer);
  uint32_t count = ts_node_named_child_count (node);
  bool skip_leading_whitespace = false;
  for (uint32_t i = 0; i < count; i++)
    {
      TSNode subnode = ts_node_named_child (node, i);
      if (ts_node_symbol (subnode) == ts_symbol_string_content)
        {
          string_desc_t subnode_string =
            sd_new_addr (ts_node_end_byte (subnode) - ts_node_start_byte (subnode),
                         contents + ts_node_start_byte (subnode));
          if (skip_leading_whitespace)
            {
              /* After backslash-newline, skip ASCII whitespace.  */
              while (sd_length (subnode_string) > 0
                     && (sd_char_at (subnode_string, 0) == ' '
                         || sd_char_at (subnode_string, 0) == '\t'))
                subnode_string = sd_substring (subnode_string, 1, sd_length (subnode_string));
            }
          sb_xappend_desc (&buffer, subnode_string);
          skip_leading_whitespace = false;
        }
      else if (ts_node_symbol (subnode) == ts_symbol_escape_sequence)
        {
          const char *escape_start = contents + ts_node_start_byte (subnode);
          const char *escape_end = contents + ts_node_end_byte (subnode);
          /* The escape sequence must start with a backslash.  */
          if (!(escape_end - escape_start >= 2 && escape_start[0] == '\\'))
            abort ();
          skip_leading_whitespace = false;
          /* tree-sitter's grammar.js allows more escape sequences than
             the Rust documentation and the Rust compiler.  Give a warning
             for those case where the Rust compiler gives an error.  */
          bool invalid = false;
          if (escape_end - escape_start == 2)
            {
              switch (escape_start[1])
                {
                case '\\':
                case '"':
                case '\'':
                  sb_xappend1 (&buffer, escape_start[1]);
                  break;
                case 'n':
                  sb_xappend1 (&buffer, '\n');
                  break;
                case 'r':
                  sb_xappend1 (&buffer, '\r');
                  break;
                case 't':
                  sb_xappend1 (&buffer, '\t');
                  break;
                case '\n':
                  skip_leading_whitespace = true;
                  break;
                default:
                  invalid = true;
                  break;
                }
            }
          else if (escape_start[1] == 'x')
            {
              unsigned int value = 0;
              for (const char *p = escape_start + 2; p < escape_end; p++)
                {
                  /* Only 2 hexadecimal digits are accepted.
                     No overflow is possible.  */
                  char c = *p;
                  if (c >= '0' && c <= '9')
                    value = (value << 4) + (c - '0');
                  else if (c >= 'A' && c <= 'Z')
                    value = (value << 4) + (c - 'A' + 10);
                  else if (c >= 'a' && c <= 'z')
                    value = (value << 4) + (c - 'a' + 10);
                  else
                    invalid = true;
                }
              if (!invalid)
                {
                  uint8_t buf[6];
                  int n = u8_uctomb (buf, value, sizeof (buf));
                  if (n > 0)
                    sb_xappend_desc (&buffer, sd_new_addr (n, (const char *) buf));
                  else
                    invalid = true;
                }
            }
          else if (escape_start[1] == 'u'
                   && escape_end - escape_start > 4
                   && escape_start[2] == '{' && escape_end[-1] == '}')
            {
              unsigned int value = 0;
              for (const char *p = escape_start + 3; p < escape_end - 1; p++)
                {
                  char c = *p;
                  if (c >= '0' && c <= '9')
                    value = (value << 4) + (c - '0');
                  else if (c >= 'A' && c <= 'Z')
                    value = (value << 4) + (c - 'A' + 10);
                  else if (c >= 'a' && c <= 'z')
                    value = (value << 4) + (c - 'a' + 10);
                  else
                    invalid = true;
                  if (value >= 0x110000)
                    invalid = true;
                  if (invalid)
                    break;
                }
              if (!invalid)
                {
                  uint8_t buf[6];
                  int n = u8_uctomb (buf, value, sizeof (buf));
                  if (n > 0)
                    sb_xappend_desc (&buffer, sd_new_addr (n, (const char *) buf));
                  else
                    invalid = true;
                }
            }
          else
            invalid = true;
          if (invalid)
            {
              size_t line_number = ts_node_line_number (subnode);
              if_error (IF_SEVERITY_WARNING,
                        logical_file_name, line_number, (size_t)(-1), false,
                        _("invalid escape sequence in string"));
            }
        }
      else
        abort ();
    }
  return sb_xdupfree_c (&buffer);
}

/* --------------------- Parsing and string extraction --------------------- */

/* Maximum supported nesting depth.  */
#define MAX_NESTING_DEPTH 1000

static int nesting_depth;

/* The file is parsed into an abstract syntax tree.  Scan the syntax tree,
   looking for a keyword in identifier position of a call_expression or
   macro_invocation, followed by followed by a string among the arguments.
   When we see this pattern, we have something to remember.

     Normal handling: Look for
       keyword ( ... msgid ... )
     Plural handling: Look for
       keyword ( ... msgid ... msgid_plural ... )

   We handle macro_invocation separately from call_expression, because in
   a macro_invocation spaces are allowed between the identifier and the '!'
   (i.e. 'println !' is as valid as 'println!').  Looking for 'println!'
   would make the code more complicated.

   We use recursion because the arguments before msgid or between msgid
   and msgid_plural can contain subexpressions of the same form.  */

/* Forward declarations.  */
static void extract_from_node (TSNode node,
                               flag_region_ty *outer_region,
                               message_list_ty *mlp);

/* Extracts messages from the function call consisting of
     - CALLEE_NODE: a tree node of type 'identifier' or
       a tree node of type 'scoped_identifier' with at least 1 named child,
     - ARGS_NODE: a tree node of type 'arguments'.
   Extracted messages are added to MLP.  */
static void
extract_from_function_call (TSNode callee_node,
                            TSNode args_node,
                            flag_region_ty *outer_region,
                            message_list_ty *mlp)
{
  uint32_t args_count = ts_node_child_count (args_node);

  TSNode callee_last_identifier_node;
  if (ts_node_symbol (callee_node) == ts_symbol_identifier)
    callee_last_identifier_node = callee_node;
  else
    /* Here ts_node_named_child_count (callee_node) >= 1.  */
    callee_last_identifier_node =
      ts_node_named_child (callee_node, ts_node_named_child_count (callee_node) - 1);

  string_desc_t callee_name =
    sd_new_addr (ts_node_end_byte (callee_last_identifier_node) - ts_node_start_byte (callee_last_identifier_node),
                 contents + ts_node_start_byte (callee_last_identifier_node));

  /* Context iterator.  */
  flag_context_list_iterator_ty next_context_iter =
    flag_context_list_iterator (
      flag_context_list_table_lookup (
        &flag_table_rust_functions,
        sd_data (callee_name), sd_length (callee_name)));

  void *keyword_value;
  if (hash_find_entry (&function_keywords,
                       sd_data (callee_name), sd_length (callee_name),
                       &keyword_value)
      == 0)
    {
      /* The callee has some information associated with it.  */
      const struct callshapes *next_shapes = keyword_value;

      /* We have a function, named by a relevant identifier, with an argument
         list.  */

      struct arglist_parser *argparser =
        arglist_parser_alloc (mlp, next_shapes);

      /* Current argument number.  */
      uint32_t arg;

      arg = 0;
      for (uint32_t i = 0; i < args_count; i++)
        {
          TSNode arg_node = ts_node_child (args_node, i);
          handle_comments (arg_node);
          if (ts_node_is_named (arg_node)
              && !(ts_node_symbol (arg_node) == ts_symbol_line_comment
                   || ts_node_symbol (arg_node) == ts_symbol_block_comment))
            {
              arg++;
              flag_region_ty *arg_region =
                inheriting_region (outer_region,
                                   flag_context_list_iterator_advance (
                                     &next_context_iter));

              bool already_extracted = false;
              if (ts_node_symbol (arg_node) == ts_symbol_string_literal
                  || ts_node_symbol (arg_node) == ts_symbol_raw_string_literal)
                {
                  lex_pos_ty pos;
                  pos.file_name = logical_file_name;
                  pos.line_number = ts_node_line_number (arg_node);

                  char *string = string_literal_value (arg_node);

                  if (extract_all)
                    {
                      remember_a_message (mlp, NULL, string, true, false,
                                          arg_region, &pos,
                                          NULL, savable_comment, true);
                      already_extracted = true;
                    }
                  else
                    {
                      mixed_string_ty *mixed_string =
                        mixed_string_alloc_utf8 (string, lc_string,
                                                 pos.file_name, pos.line_number);
                      arglist_parser_remember (argparser, arg, mixed_string,
                                               arg_region,
                                               pos.file_name, pos.line_number,
                                               savable_comment, true);
                    }
                }

              if (!already_extracted)
                {
                  if (++nesting_depth > MAX_NESTING_DEPTH)
                    if_error (IF_SEVERITY_FATAL_ERROR,
                              logical_file_name, ts_node_line_number (arg_node), (size_t)(-1), false,
                              _("too many open parentheses, brackets, or braces"));
                  extract_from_node (arg_node,
                                     arg_region,
                                     mlp);
                  nesting_depth--;
                }

              unref_region (arg_region);
            }
        }
      arglist_parser_done (argparser, arg);
      return;
    }

  /* Recurse.  */

  /* Current argument number.  */
  uint32_t arg;

  arg = 0;
  for (uint32_t i = 0; i < args_count; i++)
    {
      TSNode arg_node = ts_node_child (args_node, i);
      handle_comments (arg_node);
      if (ts_node_is_named (arg_node)
          && !(ts_node_symbol (arg_node) == ts_symbol_line_comment
               || ts_node_symbol (arg_node) == ts_symbol_block_comment))
        {
          arg++;
          flag_region_ty *arg_region =
            inheriting_region (outer_region,
                               flag_context_list_iterator_advance (
                                 &next_context_iter));

          if (++nesting_depth > MAX_NESTING_DEPTH)
            if_error (IF_SEVERITY_FATAL_ERROR,
                      logical_file_name, ts_node_line_number (arg_node), (size_t)(-1), false,
                      _("too many open parentheses, brackets, or braces"));
          extract_from_node (arg_node,
                             arg_region,
                             mlp);
          nesting_depth--;

          unref_region (arg_region);
        }
    }
}

/* Extracts messages from a function call like syntax in a macro invocation,
   consisting of
     - CALLEE_NODE: a tree node of type 'identifier', or NULL for a mere
       parenthesized expression,
     - ARGS_NODE: a tree node of type 'token_tree'.
   Extracted messages are added to MLP.  */
static void
extract_from_function_call_like (TSNode *callee_node, bool callee_is_macro,
                                 TSNode args_node,
                                 flag_region_ty *outer_region,
                                 message_list_ty *mlp)
{
  /* We have a macro, named by a relevant identifier, with an argument list.
     The args_node contains the argument tokens (some of them of type
     token_tree).  They don't contain 'call_expression' and such.  Instead,
     we need to recognize function call expressions ourselves.  */
  uint32_t args_count = ts_node_child_count (args_node);

  /* Context iterator.  */
  flag_context_list_iterator_ty next_context_iter;

  void *keyword_value;
  if (callee_node != NULL)
    {
      string_desc_t callee_name =
        sd_new_addr (ts_node_end_byte (*callee_node) - ts_node_start_byte (*callee_node),
                     contents + ts_node_start_byte (*callee_node));

      next_context_iter =
        (args_count >= 2
         && ts_node_symbol (ts_node_child (args_node, 0)) == ts_symbol_open_paren
         ? flag_context_list_iterator (
             flag_context_list_table_lookup (
               callee_is_macro ? &flag_table_rust_macros : &flag_table_rust_functions,
               sd_data (callee_name), sd_length (callee_name)))
         : null_context_list_iterator);
      if (hash_find_entry (callee_is_macro ? &macro_keywords : &function_keywords,
                           sd_data (callee_name), sd_length (callee_name),
                           &keyword_value)
          == 0)
        {
          if (keyword_value == NULL)
            abort ();
        }
      else
        keyword_value = NULL;
    }
  else
    {
      next_context_iter = passthrough_context_list_iterator;
      keyword_value = NULL;
    }

  if (keyword_value != NULL)
    {
      /* The callee has some information associated with it.  */
      const struct callshapes *next_shapes = keyword_value;

      #if DEBUG_RUST
      {
        fprintf (stderr, "children:\n");
        for (uint32_t i = 0; i < args_count; i++)
          fprintf (stderr, "%u -> [%s]|%s|\n", i, ts_node_type (ts_node_child (args_node, i)), ts_node_string (ts_node_child (args_node, i)));
      }
      #endif

      /* We are only interested in argument lists of the form (<TOKENS>),
         not [<TOKENS>] or {<TOKENS>}.  */
      if (args_count >= 2
          && ts_node_symbol (ts_node_child (args_node, 0)) == ts_symbol_open_paren
          && ts_node_symbol (ts_node_child (args_node, args_count - 1)) == ts_symbol_close_paren)
        {
          struct arglist_parser *argparser =
            arglist_parser_alloc (mlp, next_shapes);
          /* Current argument number.  */
          uint32_t arg;
          flag_region_ty *arg_region;
          uint32_t prev2_token_in_same_arg;
          uint32_t prev1_token_in_same_arg;

          arg = 0;
          for (uint32_t i = 0; i < args_count; i++)
            {
              TSNode arg_node = ts_node_child (args_node, i);
              handle_comments (arg_node);
              if (i == 0 || ts_node_symbol (arg_node) == ts_symbol_comma)
                {
                  /* The next argument starts here.  */
                  arg++;
                  if (i > 0)
                    unref_region (arg_region);
                  arg_region =
                    inheriting_region (outer_region,
                                       flag_context_list_iterator_advance (
                                         &next_context_iter));
                  prev2_token_in_same_arg = 0;
                  prev1_token_in_same_arg = 0;
                }
              else
                {
                  bool already_extracted = false;
                  if (ts_node_symbol (arg_node) == ts_symbol_string_literal
                      || ts_node_symbol (arg_node) == ts_symbol_raw_string_literal)
                    {
                      lex_pos_ty pos;
                      pos.file_name = logical_file_name;
                      pos.line_number = ts_node_line_number (arg_node);

                      char *string = string_literal_value (arg_node);

                      if (extract_all)
                        {
                          remember_a_message (mlp, NULL, string, true, false,
                                              arg_region, &pos,
                                              NULL, savable_comment, true);
                          already_extracted = true;
                        }
                      else
                        {
                          mixed_string_ty *mixed_string =
                            mixed_string_alloc_utf8 (string, lc_string,
                                                     pos.file_name, pos.line_number);
                          arglist_parser_remember (argparser, arg, mixed_string,
                                                   arg_region,
                                                   pos.file_name, pos.line_number,
                                                   savable_comment, true);
                        }
                    }

                  if (++nesting_depth > MAX_NESTING_DEPTH)
                    if_error (IF_SEVERITY_FATAL_ERROR,
                              logical_file_name, ts_node_line_number (arg_node), (size_t)(-1), false,
                              _("too many open parentheses, brackets, or braces"));
                  if (ts_node_symbol (arg_node) == ts_symbol_token_tree)
                    {
                      if (prev1_token_in_same_arg > 0
                          && ts_node_symbol (ts_node_child (args_node, prev1_token_in_same_arg)) == ts_symbol_identifier)
                        {
                          /* A token sequence that looks like a function call.  */
                          TSNode identifier_node = ts_node_child (args_node, prev1_token_in_same_arg);
                          extract_from_function_call_like (
                                             &identifier_node, false,
                                             arg_node,
                                             arg_region,
                                             mlp);
                        }
                      else if (prev2_token_in_same_arg > 0
                               && ts_node_symbol (ts_node_child (args_node, prev2_token_in_same_arg)) == ts_symbol_identifier
                               && ts_node_symbol (ts_node_child (args_node, prev1_token_in_same_arg)) == ts_symbol_exclam)
                        {
                          /* A token sequence that looks like a macro invocation.  */
                          TSNode identifier_node = ts_node_child (args_node, prev2_token_in_same_arg);
                          extract_from_function_call_like (
                                             &identifier_node, true,
                                             arg_node,
                                             arg_region,
                                             mlp);
                        }
                      else
                        /* A token sequence that looks like a parenthesized expression.  */
                        extract_from_function_call_like (
                                           NULL, false,
                                           arg_node,
                                           arg_region,
                                           mlp);
                    }
                  else
                    {
                      if (!already_extracted)
                        extract_from_node (arg_node,
                                           arg_region,
                                           mlp);
                    }
                  nesting_depth--;

                  if (!(ts_node_symbol (arg_node) == ts_symbol_line_comment
                        || ts_node_symbol (arg_node) == ts_symbol_block_comment))
                    {
                      prev2_token_in_same_arg = prev1_token_in_same_arg;
                      prev1_token_in_same_arg = i;
                    }
                }
            }
          if (arg > 0)
            unref_region (arg_region);
          arglist_parser_done (argparser, arg);
          return;
        }
    }

  /* Recurse.  */

  /* Current argument number.  */
  uint32_t arg;
  flag_region_ty *arg_region;
  uint32_t prev2_token_in_same_arg;
  uint32_t prev1_token_in_same_arg;

  arg = 0;
  for (uint32_t i = 0; i < args_count; i++)
    {
      TSNode arg_node = ts_node_child (args_node, i);
      handle_comments (arg_node);
      if (i == 0 || ts_node_symbol (arg_node) == ts_symbol_comma)
        {
          /* The next argument starts here.  */
          arg++;
          if (i > 0)
            unref_region (arg_region);
          arg_region =
            inheriting_region (outer_region,
                               flag_context_list_iterator_advance (
                                 &next_context_iter));
          prev2_token_in_same_arg = 0;
          prev1_token_in_same_arg = 0;
        }
      else
        {
          if (++nesting_depth > MAX_NESTING_DEPTH)
            if_error (IF_SEVERITY_FATAL_ERROR,
                      logical_file_name, ts_node_line_number (arg_node), (size_t)(-1), false,
                      _("too many open parentheses, brackets, or braces"));
          if (ts_node_symbol (arg_node) == ts_symbol_token_tree)
            {
              if (prev1_token_in_same_arg > 0
                  && ts_node_symbol (ts_node_child (args_node, prev1_token_in_same_arg)) == ts_symbol_identifier)
                {
                  /* A token sequence that looks like a function call.  */
                  TSNode identifier_node = ts_node_child (args_node, prev1_token_in_same_arg);
                  extract_from_function_call_like (
                                     &identifier_node, false,
                                     arg_node,
                                     arg_region,
                                     mlp);
                }
              else if (prev2_token_in_same_arg > 0
                       && ts_node_symbol (ts_node_child (args_node, prev2_token_in_same_arg)) == ts_symbol_identifier
                       && ts_node_symbol (ts_node_child (args_node, prev1_token_in_same_arg)) == ts_symbol_exclam)
                {
                  /* A token sequence that looks like a macro invocation.  */
                  TSNode identifier_node = ts_node_child (args_node, prev2_token_in_same_arg);
                  extract_from_function_call_like (
                                     &identifier_node, true,
                                     arg_node,
                                     arg_region,
                                     mlp);
                }
              else
                /* A token sequence that looks like a parenthesized expression.  */
                extract_from_function_call_like (
                                   NULL, false,
                                   arg_node,
                                   arg_region,
                                   mlp);
            }
          else
            extract_from_node (arg_node,
                               arg_region,
                               mlp);
          nesting_depth--;

          if (!(ts_node_symbol (arg_node) == ts_symbol_line_comment
                || ts_node_symbol (arg_node) == ts_symbol_block_comment))
            {
              prev2_token_in_same_arg = prev1_token_in_same_arg;
              prev1_token_in_same_arg = i;
            }
        }
    }
  if (arg > 0)
    unref_region (arg_region);
}

/* Extracts messages in the syntax tree NODE.
   Extracted messages are added to MLP.  */
static void
extract_from_node (TSNode node,
                   flag_region_ty *outer_region,
                   message_list_ty *mlp)
{
  if (extract_all
      && (ts_node_symbol (node) == ts_symbol_string_literal
          || ts_node_symbol (node) == ts_symbol_raw_string_literal))
    {
      lex_pos_ty pos;
      pos.file_name = logical_file_name;
      pos.line_number = ts_node_line_number (node);

      char *string = string_literal_value (node);

      remember_a_message (mlp, NULL, string, true, false,
                          outer_region, &pos,
                          NULL, savable_comment, true);
    }

  if (ts_node_symbol (node) == ts_symbol_call_expression
      && ts_node_named_child_count (node) >= 2)
    {
      TSNode callee_node = ts_node_named_child (node, 0);
      /* This is the field called 'function'.  */
      if (! ts_node_eq (ts_node_child_by_field_id (node, ts_field_function),
                        callee_node))
        abort ();
      if (ts_node_symbol (callee_node) == ts_symbol_identifier
          || (ts_node_symbol (callee_node) == ts_symbol_scoped_identifier
              && ts_node_named_child_count (callee_node) >= 1))
        {
          TSNode args_node = ts_node_child_by_field_id (node, ts_field_arguments);
          /* This is the field called 'arguments'.  */
          if (ts_node_symbol (args_node) == ts_symbol_arguments)
            {
              /* Handle the potential comments between 'function' and 'arguments'.  */
              {
                uint32_t count = ts_node_child_count (node);
                for (uint32_t i = 0; i < count; i++)
                  {
                    TSNode subnode = ts_node_child (node, i);
                    if (ts_node_eq (subnode, args_node))
                      break;
                    handle_comments (subnode);
                  }
              }
              extract_from_function_call (callee_node, args_node,
                                          outer_region,
                                          mlp);
              return;
            }
        }
    }

  if (ts_node_symbol (node) == ts_symbol_macro_invocation
      && ts_node_named_child_count (node) >= 2)
    {
      TSNode callee_node = ts_node_named_child (node, 0);
      /* This is the field called 'macro'.  */
      if (! ts_node_eq (ts_node_child_by_field_id (node, ts_field_macro),
                        callee_node))
        abort ();
      if (ts_node_symbol (callee_node) == ts_symbol_identifier)
        {
          /* We have to search for the args_node.
             It is not always = ts_node_named_child (node, 1),
             namely when there are comments before it.  */
          uint32_t count = ts_node_child_count (node);
          for (uint32_t args_index = 0; args_index < count; args_index++)
            {
              TSNode args_node = ts_node_child (node, args_index);
              if (ts_node_symbol (args_node) == ts_symbol_token_tree)
                {
                  /* Handle the potential comments between 'macro' and the args_node.  */
                  for (uint32_t i = 0; i < count; i++)
                    {
                      TSNode subnode = ts_node_child (node, i);
                      if (ts_node_eq (subnode, args_node))
                        break;
                      handle_comments (subnode);
                    }
                  extract_from_function_call_like (&callee_node, true,
                                                   args_node,
                                                   outer_region,
                                                   mlp);
                  return;
                }
            }
        }
    }

  #if DEBUG_RUST
  if (ts_node_symbol (node) == ts_symbol_call_expression)
    {
      TSNode subnode = ts_node_child_by_field_id (node, ts_field_function);
      fprintf (stderr, "-> %s\n", ts_node_string (subnode));
      if (ts_node_symbol (subnode) == ts_symbol_identifier)
        {
          string_desc_t subnode_string =
            sd_new_addr (ts_node_end_byte (subnode) - ts_node_start_byte (subnode),
                         contents + ts_node_start_byte (subnode));
          if (sd_equals (subnode_string, sd_from_c ("gettext")))
            {
              TSNode argsnode = ts_node_child_by_field_id (node, ts_field_arguments);
              fprintf (stderr, "gettext arguments: %s\n", ts_node_string (argsnode));
              fprintf (stderr, "gettext children:\n");
              uint32_t count = ts_node_named_child_count (node);
              for (uint32_t i = 0; i < count; i++)
                fprintf (stderr, "%u -> %s\n", i, ts_node_string (ts_node_named_child (node, i)));
            }
        }
    }
  if (ts_node_symbol (node) == ts_symbol_macro_invocation)
    {
      TSNode subnode = ts_node_child_by_field_id (node, ts_field_macro);
      if (ts_node_symbol (subnode) == ts_symbol_identifier)
        {
          string_desc_t subnode_string =
            sd_new_addr (ts_node_end_byte (subnode) - ts_node_start_byte (subnode),
                         contents + ts_node_start_byte (subnode));
          fprintf (stderr, "identifier=%s\n", xsd_c (subnode_string));
          if (sd_equals (subnode_string, sd_from_c ("println")))
            {
              fprintf (stderr, "children:\n");
              uint32_t count = ts_node_child_count (node);
              for (uint32_t i = 0; i < count; i++)
                fprintf (stderr, "%u -> [%s]|%s|\n", i, ts_node_type (ts_node_child (node, i)), ts_node_string (ts_node_child (node, i)));
            }
        }
    }
  #endif

  /* Recurse.  */
  if (!(ts_node_symbol (node) == ts_symbol_line_comment
        || ts_node_symbol (node) == ts_symbol_block_comment))
    {
      uint32_t count = ts_node_child_count (node);
      for (uint32_t i = 0; i < count; i++)
        {
          TSNode subnode = ts_node_child (node, i);
          handle_comments (subnode);
          if (++nesting_depth > MAX_NESTING_DEPTH)
            if_error (IF_SEVERITY_FATAL_ERROR,
                      logical_file_name, ts_node_line_number (subnode), (size_t)(-1), false,
                      _("too many open parentheses, brackets, or braces"));
          extract_from_node (subnode,
                             outer_region,
                             mlp);
          nesting_depth--;
       }
    }
}

void
extract_rust (FILE *f,
              const char *real_filename, const char *logical_filename,
              flag_context_list_table_ty *flag_table,
              msgdomain_list_ty *mdlp)
{
  message_list_ty *mlp = mdlp->item[0]->messages;

  logical_file_name = xstrdup (logical_filename);

  last_comment_line = -1;
  last_non_comment_line = -1;

  nesting_depth = 0;

  init_keywords ();

  if (ts_language == NULL)
    {
      ts_language = tree_sitter_rust ();
      ts_symbol_line_comment       = ts_language_symbol ("line_comment", true);
      ts_symbol_block_comment      = ts_language_symbol ("block_comment", true);
      ts_symbol_string_literal     = ts_language_symbol ("string_literal", true);
      ts_symbol_raw_string_literal = ts_language_symbol ("raw_string_literal", true);
      ts_symbol_string_content     = ts_language_symbol ("string_content", true);
      ts_symbol_escape_sequence    = ts_language_symbol ("escape_sequence", true);
      ts_symbol_identifier         = ts_language_symbol ("identifier", true);
      ts_symbol_scoped_identifier  = ts_language_symbol ("scoped_identifier", true);
      ts_symbol_call_expression    = ts_language_symbol ("call_expression", true);
      ts_symbol_macro_invocation   = ts_language_symbol ("macro_invocation", true);
      ts_symbol_arguments          = ts_language_symbol ("arguments", true);
      ts_symbol_token_tree         = ts_language_symbol ("token_tree", true);
      ts_symbol_open_paren         = ts_language_symbol ("(", false);
      ts_symbol_close_paren        = ts_language_symbol (")", false);
      ts_symbol_comma              = ts_language_symbol (",", false);
      ts_symbol_exclam             = ts_language_symbol ("!", false);
      ts_field_function  = ts_language_field ("function");
      ts_field_arguments = ts_language_field ("arguments");
      ts_field_macro     = ts_language_field ("macro");
    }

  /* Read the file into memory.  */
  size_t contents_length;
  char *contents_data = read_file (real_filename, 0, &contents_length);
  if (contents_data == NULL)
    error (EXIT_FAILURE, errno, _("error while reading \"%s\""),
           real_filename);

  /* tree-sitter works only on files whose size fits in an uint32_t.  */
  if (contents_length > 0xFFFFFFFFUL)
    error (EXIT_FAILURE, 0, _("file \"%s\" is unsupported because too large"),
           real_filename);

  /* Rust source files are UTF-8 encoded.
     <https://doc.rust-lang.org/1.6.0/reference.html#input-format>  */
  if (u8_check ((uint8_t *) contents_data, contents_length) != NULL)
    error (EXIT_FAILURE, 0,
           _("file \"%s\" is invalid because not UTF-8 encoded"),
           real_filename);
  xgettext_current_source_encoding = po_charset_utf8;

  /* Create a parser.  */
  TSParser *parser = ts_parser_new ();

  /* Set the parser's language.  */
  ts_parser_set_language (parser, ts_language);

  /* Parse the file, producing a syntax tree.  */
  TSTree *tree = ts_parser_parse_string (parser, NULL, contents_data, contents_length);

  #if DEBUG_RUST
  /* For debugging: Print the tree.  */
  {
    char *tree_as_string = ts_node_string (ts_tree_root_node (tree));
    fprintf (stderr, "Syntax tree: %s\n", tree_as_string);
    free (tree_as_string);
  }
  #endif

  contents = contents_data;

  extract_from_node (ts_tree_root_node (tree),
                     null_context_region (),
                     mlp);

  ts_tree_delete (tree);
  ts_parser_delete (parser);
  free (contents_data);

  logical_file_name = NULL;
}
