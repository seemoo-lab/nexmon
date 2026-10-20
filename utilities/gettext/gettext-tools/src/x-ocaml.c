/* xgettext OCaml backend.
   Copyright (C) 2020-2026 Free Software Foundation, Inc.

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
#include "x-ocaml.h"

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

#define _(s) gettext (s)

/* Use tree-sitter.
   Documentation: <https://tree-sitter.github.io/tree-sitter/using-parsers>  */
#include <tree_sitter/api.h>
extern const TSLanguage *tree_sitter_ocaml (void);


/* The OCaml syntax is defined in <https://ocaml.org/docs/language>.

   String syntax: Strings are delimited by double-quotes or by {id| |id} pairs.
   Backslash is the escape character. Among the escape sequences, there is in
   particular backslash-newline-spaces_or_tabs and \u{nnnn}.
   Reference: <https://ocaml.org/manual/5.3/lex.html#sss:stringliterals>

   Comment syntax: Comments start with '(*' and end with '*)' and can be nested.
   References: <https://ocaml.org/manual/5.3/lex.html#sss:lex:comments>
               <https://ocaml.org/docs/tour-of-ocaml>
 */

#define DEBUG_OCAML 0


/* ====================== Keyword set customization.  ====================== */

/* If true extract all strings.  */
static bool extract_all = false;

static hash_table keywords;
static bool default_keywords = true;

void
x_ocaml_extract_all (void)
{
  extract_all = true;
}

void
x_ocaml_keyword (const char *name)
{
  if (name == NULL)
    default_keywords = false;
  else
    {
      if (keywords.table == NULL)
        hash_init (&keywords, 100);

      const char *end;
      struct callshape shape;
      split_keywordspec (name, &end, &shape);

      /* The characters between name and end should form a valid identifier.
         A colon means an invalid parse in split_keywordspec().  */
      const char *colon = strchr (name, ':');
      if (colon == NULL || colon >= end)
        insert_keyword_callshape (&keywords, name, end - name, &shape);
    }
}

/* Finish initializing the keywords hash table.
   Called after argument processing, before each file is processed.  */
static void
init_keywords ()
{
  if (default_keywords)
    {
      /* Compatible with ocaml-gettext/src/bin/ocaml-xgettext/xgettext.ml.  */
      /* When adding new keywords here, also update the documentation in
         xgettext.texi!  */
      x_ocaml_keyword ("s_");
      x_ocaml_keyword ("f_");
      x_ocaml_keyword ("sn_:1,2");
      x_ocaml_keyword ("fn_:1,2");
      x_ocaml_keyword ("gettext:2");
      x_ocaml_keyword ("fgettext:2");
      x_ocaml_keyword ("dgettext:3");
      x_ocaml_keyword ("fdgettext:3");
      x_ocaml_keyword ("dcgettext:3");
      x_ocaml_keyword ("fdcgettext:3");
      x_ocaml_keyword ("ngettext:2,3");
      x_ocaml_keyword ("fngettext:2,3");
      x_ocaml_keyword ("dngettext:3,4");
      x_ocaml_keyword ("fdngettext:3,4");
      x_ocaml_keyword ("dcngettext:3,4");
      x_ocaml_keyword ("fdcngettext:3,4");
      default_keywords = false;
    }
}

/* This function currently has no effect.  */
void
init_flag_table_ocaml (void)
{
  /* Compatible with ocaml-gettext/src/bin/ocaml-xgettext/xgettext.ml.  */
  xgettext_record_flag ("s_:1:impossible-ocaml-format");
  xgettext_record_flag ("f_:1:ocaml-format");
  xgettext_record_flag ("sn_:1:impossible-ocaml-format");
  xgettext_record_flag ("sn_:2:impossible-ocaml-format");
  xgettext_record_flag ("fn_:1:ocaml-format");
  xgettext_record_flag ("fn_:2:ocaml-format");
  xgettext_record_flag ("gettext:2:impossible-ocaml-format");
  xgettext_record_flag ("fgettext:2:ocaml-format");
  xgettext_record_flag ("dgettext:3:impossible-ocaml-format");
  xgettext_record_flag ("fdgettext:3:ocaml-format");
  xgettext_record_flag ("dcgettext:3:impossible-ocaml-format");
  xgettext_record_flag ("fdcgettext:3:ocaml-format");
  xgettext_record_flag ("ngettext:2:impossible-ocaml-format");
  xgettext_record_flag ("ngettext:3:impossible-ocaml-format");
  xgettext_record_flag ("fngettext:2:ocaml-format");
  xgettext_record_flag ("fngettext:3:ocaml-format");
  xgettext_record_flag ("dngettext:3:impossible-ocaml-format");
  xgettext_record_flag ("dngettext:4:impossible-ocaml-format");
  xgettext_record_flag ("fdngettext:3:ocaml-format");
  xgettext_record_flag ("fdngettext:4:ocaml-format");
  xgettext_record_flag ("dcngettext:3:impossible-ocaml-format");
  xgettext_record_flag ("dcngettext:4:impossible-ocaml-format");
  xgettext_record_flag ("fdcngettext:3:ocaml-format");
  xgettext_record_flag ("fdcngettext:4:ocaml-format");
}


/* ======================== Parsing via tree-sitter. ======================== */
/* To understand this code, look at
     tree-sitter-ocaml/grammars/ocaml/src/node-types.json
   and
     tree-sitter-ocaml/grammars/ocaml/src/grammar.json
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
     strcmp (ts_node_type (node), "string") == 0
   it is faster to do
     ts_node_symbol (node) == ts_symbol_string
 */
static TSSymbol ts_symbol_comment;
static TSSymbol ts_symbol_string;
static TSSymbol ts_symbol_string_content;
static TSSymbol ts_symbol_escape_sequence;
static TSSymbol ts_symbol_quoted_string;
static TSSymbol ts_symbol_quoted_string_content;
static TSSymbol ts_symbol_infix_expression;
static TSSymbol ts_symbol_concat_operator;
static TSSymbol ts_symbol_application_expression;
static TSSymbol ts_symbol_value_path;
static TSSymbol ts_symbol_value_name;
static TSSymbol ts_symbol_parenthesized_expression;
static TSSymbol ts_symbol_lparen;
static TSSymbol ts_symbol_rparen;
static TSFieldId ts_field_operator;
static TSFieldId ts_field_left;
static TSFieldId ts_field_right;
static TSFieldId ts_field_function;

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
  #if DEBUG_OCAML
  fprintf (stderr, "LCL=%d LNCL=%d node=[%s]|%s|\n", last_comment_line, last_non_comment_line, ts_node_type (node), ts_node_string (node));
  #endif
  if (last_comment_line < last_non_comment_line
      && last_non_comment_line < ts_node_line_number (node))
    /* We have skipped over a newline.  This newline terminated a line
       with non-comment tokens, after the last comment line.  */
    savable_comment_reset ();

  if (ts_node_symbol (node) == ts_symbol_comment)
    {
      string_desc_t entire =
        sd_new_addr (ts_node_end_byte (node) - ts_node_start_byte (node),
                     contents + ts_node_start_byte (node));
      /* It should start and end with the comment markers.  */
      if (!(sd_length (entire) >= 4
            && sd_char_at (entire, 0) == '('
            && sd_char_at (entire, 1) == '*'
            && sd_char_at (entire, sd_length (entire) - 2) == '*'
            && sd_char_at (entire, sd_length (entire) - 1) == ')'))
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

/* Determines whether NODE represents the string concatenation operator '^'.  */
static bool
is_string_concatenation_operator (TSNode node)
{
  if (ts_node_symbol (node) == ts_symbol_concat_operator)
    {
      string_desc_t operator_string =
        sd_new_addr (ts_node_end_byte (node) - ts_node_start_byte (node),
                     contents + ts_node_start_byte (node));
      if (sd_equals (operator_string, sd_from_c ("^")))
        return true;
    }
  return false;
}

/* Determines whether NODE represents a string literal or the concatenation
   of string literals (via the '^' operator).  */
static bool
is_string_literal (TSNode node)
{
 start:
  if (ts_node_symbol (node) == ts_symbol_string
      || ts_node_symbol (node) == ts_symbol_quoted_string)
    return true;
  if (ts_node_symbol (node) == ts_symbol_infix_expression
      && is_string_concatenation_operator (ts_node_child_by_field_id (node, ts_field_operator))
      /* Recurse into the left and right subnodes.  */
      && is_string_literal (ts_node_child_by_field_id (node, ts_field_left)))
    {
      /*return is_string_literal (ts_node_child_by_field_id (node, ts_field_right));*/
      node = ts_node_child_by_field_id (node, ts_field_right);
      goto start;
    }
  if (ts_node_symbol (node) == ts_symbol_parenthesized_expression)
    {
      uint32_t count = ts_node_child_count (node);
      if (count > 0
          && ts_node_symbol (ts_node_child (node, 0)) == ts_symbol_lparen
          && ts_node_symbol (ts_node_child (node, count - 1)) == ts_symbol_rparen)
        {
          uint32_t subnodes = 0;
          uint32_t last_subnode_index = 0;
          for (uint32_t i = 1; i < count - 1; i++)
            {
              TSNode subnode = ts_node_child (node, i);
              if (ts_node_is_named (subnode)
                  && ts_node_symbol (subnode) != ts_symbol_comment)
                {
                  subnodes++;
                  last_subnode_index = i;
                }
            }
          if (subnodes == 1)
            {
              TSNode subnode = ts_node_child (node, last_subnode_index);
              /* Recurse.  */
              /*return is_string_literal (subnode);*/
              node = subnode;
              goto start;
            }
        }
    }

  return false;
}

/* Appends the string literal pieces from NODE to BUFFER.  */
static void
string_literal_accumulate_pieces (TSNode node,
                                  struct string_buffer *buffer)
{
 start:
  if (ts_node_symbol (node) == ts_symbol_string)
    {
      uint32_t count = ts_node_named_child_count (node);
      for (uint32_t i = 0; i < count; i++)
        {
          TSNode subnode = ts_node_named_child (node, i);
          if (ts_node_symbol (subnode) == ts_symbol_string_content)
            {
              const char *subnode_start = contents + ts_node_start_byte (subnode);
              const char *subnode_end = contents + ts_node_end_byte (subnode);
              uint32_t subcount = ts_node_child_count (subnode);
              #if DEBUG_OCAML
              {
                fprintf (stderr, "string_content children:\n");
                for (uint32_t j = 0; j < subcount; j++)
                  fprintf (stderr, "%u -> [%s]|%s|\n", j, ts_node_type (ts_node_child (subnode, j)), ts_node_string (ts_node_child (subnode, j)));
              }
              #endif
              /* Iterate over the children nodes of type escape_sequence.
                 Other children nodes, such as conversion_specification or
                 pretty_printing_indication, can be ignored.  */
              for (uint32_t j = 0; j < subcount; j++)
                {
                  TSNode subsubnode = ts_node_child (subnode, j);
                  if (ts_node_symbol (subsubnode) == ts_symbol_escape_sequence)
                    {
                      const char *escape_start = contents + ts_node_start_byte (subsubnode);
                      const char *escape_end = contents + ts_node_end_byte (subsubnode);
                      sb_xappend_desc (buffer,
                                       sd_new_addr (escape_start - subnode_start, subnode_start));

                      /* The escape sequence must start with a backslash.  */
                      if (!(escape_end - escape_start >= 2 && escape_start[0] == '\\'))
                        abort ();
                      /* tree-sitter's grammar.js allows more escape sequences
                         than the OCaml system.  Give a warning for those cases
                         where the OCaml system gives an error.  */
                      bool invalid = false;
                      if (escape_end - escape_start >= 2
                          && (escape_start[1] == '\n' || escape_start[1] == '\r'))
                        /* backslash-newline-spaces_or_tabs  */
                        ;
                      else if (escape_end - escape_start == 2)
                        {
                          switch (escape_start[1])
                            {
                            case '\\':
                            case '"':
                            case '\'':
                            case ' ':
                              sb_xappend1 (buffer, escape_start[1]);
                              break;
                            case 'n':
                              sb_xappend1 (buffer, '\n');
                              break;
                            case 'r':
                              sb_xappend1 (buffer, '\r');
                              break;
                            case 't':
                              sb_xappend1 (buffer, '\t');
                              break;
                            case 'b':
                              sb_xappend1 (buffer, 0x08);
                              break;
                            default:
                              abort ();
                            }
                        }
                      else if (escape_end - escape_start == 4
                               && (escape_start[1] >= '0'
                                   && escape_start[1] <= '9'))
                        {
                          /* Only exactly 3 decimal digits are accepted.  */
                          unsigned int value = 0;
                          for (const char *p = escape_start + 1; p < escape_end; p++)
                            {
                              /* No overflow is possible.  */
                              char c = *p;
                              if (c >= '0' && c <= '9')
                                value = value * 10 + (c - '0');
                              else
                                abort ();
                            }
                          if (value > 0xFF)
                            invalid = true;
                          if (!invalid)
                            sb_xappend1 (buffer, (unsigned char) value);
                        }
                      else if (escape_end - escape_start == 4
                               && escape_start[1] == 'x')
                        {
                          /* Only exactly 2 hexadecimal digits are accepted.  */
                          unsigned int value = 0;
                          for (const char *p = escape_start + 2; p < escape_end; p++)
                            {
                              /* No overflow is possible.  */
                              char c = *p;
                              if (c >= '0' && c <= '9')
                                value = (value << 4) + (c - '0');
                              else if (c >= 'A' && c <= 'Z')
                                value = (value << 4) + (c - 'A' + 10);
                              else if (c >= 'a' && c <= 'z')
                                value = (value << 4) + (c - 'a' + 10);
                              else
                                abort ();
                            }
                          sb_xappend1 (buffer, (unsigned char) value);
                        }
                      else if (escape_end - escape_start == 5
                               && escape_start[1] == 'o')
                        {
                          /* Only exactly 3 octal digits are accepted.  */
                          unsigned int value = 0;
                          for (const char *p = escape_start + 2; p < escape_end; p++)
                            {
                              /* No overflow is possible.  */
                              char c = *p;
                              if (c >= '0' && c <= '7')
                                value = (value << 3) + (c - '0');
                              else
                                abort ();
                            }
                          if (value > 0xFF)
                            abort ();
                          sb_xappend1 (buffer, (unsigned char) value);
                        }
                      else if (escape_end - escape_start > 4
                               && escape_start[1] == 'u'
                               && escape_start[2] == '{'
                               && escape_end[-1] == '}')
                        {
                          if (escape_end - escape_start <= 4 + 6)
                            {
                              /* 1 to 6 hexadecimal digits are accepted.  */
                              unsigned int value = 0;
                              for (const char *p = escape_start + 3; p < escape_end - 1; p++)
                                {
                                  /* No overflow is possible.  */
                                  char c = *p;
                                  if (c >= '0' && c <= '9')
                                    value = (value << 4) + (c - '0');
                                  else if (c >= 'A' && c <= 'Z')
                                    value = (value << 4) + (c - 'A' + 10);
                                  else if (c >= 'a' && c <= 'z')
                                    value = (value << 4) + (c - 'a' + 10);
                                  else
                                    abort ();
                                }
                              if (value >= 0x110000
                                  || (value >= 0xD800 && value <= 0xDFFF))
                                invalid = true;
                              if (!invalid)
                                {
                                  uint8_t buf[6];
                                  int n = u8_uctomb (buf, value, sizeof (buf));
                                  if (n > 0)
                                    sb_xappend_desc (buffer,
                                                     sd_new_addr (n, (const char *) buf));
                                  else
                                    invalid = true;
                                }
                            }
                          else
                            invalid = true;
                        }
                      else
                        abort ();
                      if (invalid)
                        {
                          size_t line_number = ts_node_line_number (subnode);
                          if_error (IF_SEVERITY_WARNING,
                                    logical_file_name, line_number, (size_t)(-1), false,
                                    _("invalid escape sequence in string"));
                        }

                      subnode_start = escape_end;
                    }
                }
              sb_xappend_desc (buffer,
                               sd_new_addr (subnode_end - subnode_start, subnode_start));
            }
          else
            abort ();
        }
    }
  else if (ts_node_symbol (node) == ts_symbol_quoted_string)
    {
      uint32_t count = ts_node_named_child_count (node);
      for (uint32_t i = 0; i < count; i++)
        {
          TSNode subnode = ts_node_named_child (node, i);
          if (ts_node_symbol (subnode) == ts_symbol_quoted_string_content)
            {
              /* We can ignore the children nodes here, since none of them can
                 be of type escape_sequence.  */
              string_desc_t subnode_string =
                sd_new_addr (ts_node_end_byte (subnode) - ts_node_start_byte (subnode),
                             contents + ts_node_start_byte (subnode));
              sb_xappend_desc (buffer, subnode_string);
            }
        }
    }
  else if (ts_node_symbol (node) == ts_symbol_infix_expression
           && is_string_concatenation_operator (ts_node_child_by_field_id (node, ts_field_operator)))
    {
      /* Recurse into the left and right subnodes.  */
      string_literal_accumulate_pieces (ts_node_child_by_field_id (node, ts_field_left), buffer);
      /*string_literal_accumulate_pieces (ts_node_child_by_field_id (node, ts_field_right), buffer);*/
      node = ts_node_child_by_field_id (node, ts_field_right);
      goto start;
    }
  else if (ts_node_symbol (node) == ts_symbol_parenthesized_expression)
    {
      uint32_t count = ts_node_child_count (node);
      /* is_string_literal has already checked that the first child node is '(',
         that the last child node is ')', and that in-between there is exactly
         one non-comment node.  */
      if (!(count > 0))
        abort ();
      for (uint32_t i = 1; i < count - 1; i++)
        {
          TSNode subnode = ts_node_child (node, i);
          if (ts_node_is_named (subnode)
              && ts_node_symbol (subnode) != ts_symbol_comment)
            {
              /* Recurse.  */
              /*string_literal_accumulate_pieces (subnode, buffer);*/
              node = subnode;
              goto start;
            }
        }
      abort ();
    }
  else
    abort ();
}

/* Combines the pieces of a string literal or concatenated string literal.
   Returns a freshly allocated, mostly UTF-8 encoded string.  */
static char *
string_literal_value (TSNode node)
{
  struct string_buffer buffer;
  sb_init (&buffer);
  string_literal_accumulate_pieces (node, &buffer);
  return sb_xdupfree_c (&buffer);
}

/* --------------------- Parsing and string extraction --------------------- */

/* Context lookup table.  */
static flag_context_list_table_ty *flag_context_list_table;

/* Maximum supported nesting depth.  */
#define MAX_NESTING_DEPTH 1000

static int nesting_depth;

/* The file is parsed into an abstract syntax tree.  Scan the syntax tree,
   looking for a keyword in function position of a application_expression,
   followed by followed by a string among the arguments.
   When we see this pattern, we have something to remember.

     Normal handling: Look for
       keyword ... msgid ...
     Plural handling: Look for
       keyword ... msgid ... msgid_plural ...

   We use recursion because the arguments before msgid or between msgid
   and msgid_plural can contain subexpressions of the same form.  */

/* Forward declarations.  */
static void extract_from_node (TSNode node,
                               bool ignore,
                               flag_region_ty *outer_region,
                               message_list_ty *mlp);

/* Extracts messages from the function application consisting of
     - FUNCTION_NODE: a tree node of type 'value_path',
     - FUNCTION_NAME_NODE: a tree node of type 'value_name',
       the last named node of FUNCTION_NODE,
     - ARGS_NODE: a tree node of type 'application_expression',
       of which FUNCTION_NAME is the 'function' field.
   Extracted messages are added to MLP.  */
static void
extract_from_function_call (TSNode function_node,
                            TSNode function_name_node,
                            TSNode args_node,
                            flag_region_ty *outer_region,
                            message_list_ty *mlp)
{
  uint32_t args_count = ts_node_child_count (args_node);

  string_desc_t function_name =
    sd_new_addr (ts_node_end_byte (function_name_node) - ts_node_start_byte (function_name_node),
                 contents + ts_node_start_byte (function_name_node));

  /* Context iterator.  */
  flag_context_list_iterator_ty next_context_iter =
    flag_context_list_iterator (
      flag_context_list_table_lookup (
        flag_context_list_table,
        sd_data (function_name), sd_length (function_name)));

  /* Information associated with the callee.  */
  const struct callshapes *next_shapes = NULL;

  /* Look in the keywords table.  */
  void *keyword_value;
  if (hash_find_entry (&keywords,
                       sd_data (function_name), sd_length (function_name),
                       &keyword_value)
      == 0)
    next_shapes = (const struct callshapes *) keyword_value;

  if (next_shapes != NULL)
    {
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
              && ts_node_symbol (arg_node) != ts_symbol_comment
              && !ts_node_eq (arg_node, function_node))
            {
              arg++;
              flag_region_ty *arg_region =
                inheriting_region (outer_region,
                                   flag_context_list_iterator_advance (
                                     &next_context_iter));

              bool already_extracted = false;
              if (is_string_literal (arg_node))
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
                              _("too many open parentheses"));
                  extract_from_node (arg_node,
                                     false,
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

  for (uint32_t i = 0; i < args_count; i++)
    {
      TSNode arg_node = ts_node_child (args_node, i);
      handle_comments (arg_node);
      if (ts_node_is_named (arg_node)
          && ts_node_symbol (arg_node) != ts_symbol_comment)
        {
          flag_region_ty *arg_region =
            inheriting_region (outer_region,
                               flag_context_list_iterator_advance (
                                 &next_context_iter));

          if (++nesting_depth > MAX_NESTING_DEPTH)
            if_error (IF_SEVERITY_FATAL_ERROR,
                      logical_file_name, ts_node_line_number (arg_node), (size_t)(-1), false,
                      _("too many open parentheses"));
          extract_from_node (arg_node,
                             false,
                             arg_region,
                             mlp);
          nesting_depth--;

          unref_region (arg_region);
        }
    }
}

/* Extracts messages in the syntax tree NODE.
   Extracted messages are added to MLP.  */
static void
extract_from_node (TSNode node,
                   bool ignore,
                   flag_region_ty *outer_region,
                   message_list_ty *mlp)
{
  if (extract_all && !ignore && is_string_literal (node))
    {
      lex_pos_ty pos;
      pos.file_name = logical_file_name;
      pos.line_number = ts_node_line_number (node);

      char *string = string_literal_value (node);

      remember_a_message (mlp, NULL, string, true, false,
                          outer_region, &pos,
                          NULL, savable_comment, true);
    }

  if (ts_node_symbol (node) == ts_symbol_application_expression
      && ts_node_named_child_count (node) >= 2)
    {
      TSNode function_node = ts_node_named_child (node, 0);
      /* This is the field called 'function'.  */
      if (! ts_node_eq (ts_node_child_by_field_id (node, ts_field_function),
                        function_node))
        abort ();
      if (ts_node_symbol (function_node) == ts_symbol_value_path
          && ts_node_named_child_count (function_node) > 0)
        {
          TSNode function_name_node =
            ts_node_named_child (function_node,
                                 ts_node_named_child_count (function_node) - 1);
          if (ts_node_symbol (function_name_node) == ts_symbol_value_name)
            {
              extract_from_function_call (function_node, function_name_node, node,
                                          outer_region,
                                          mlp);
              return;
            }
        }
    }

  /* Recurse.  */
  if (!(ts_node_symbol (node) == ts_symbol_comment))
    {
      ignore = ignore || is_string_literal (node);
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
                             ignore,
                             outer_region,
                             mlp);
          nesting_depth--;
       }
    }
}

void
extract_ocaml (FILE *f,
               const char *real_filename, const char *logical_filename,
               flag_context_list_table_ty *flag_table,
               msgdomain_list_ty *mdlp)
{
  message_list_ty *mlp = mdlp->item[0]->messages;

  logical_file_name = xstrdup (logical_filename);

  last_comment_line = -1;
  last_non_comment_line = -1;

  flag_context_list_table = flag_table;
  nesting_depth = 0;

  init_keywords ();

  if (ts_language == NULL)
    {
      ts_language = tree_sitter_ocaml ();
      ts_symbol_comment                  = ts_language_symbol ("comment", true);
      ts_symbol_string                   = ts_language_symbol ("string", true);
      ts_symbol_string_content           = ts_language_symbol ("string_content", true);
      ts_symbol_escape_sequence          = ts_language_symbol ("escape_sequence", true);
      ts_symbol_quoted_string            = ts_language_symbol ("quoted_string", true);;
      ts_symbol_quoted_string_content    = ts_language_symbol ("quoted_string_content", true);;
      ts_symbol_infix_expression         = ts_language_symbol ("infix_expression", true);
      ts_symbol_concat_operator          = ts_language_symbol ("concat_operator", true);
      ts_symbol_application_expression   = ts_language_symbol ("application_expression", true);
      ts_symbol_value_path               = ts_language_symbol ("value_path", true);
      ts_symbol_value_name               = ts_language_symbol ("value_name", true);
      ts_symbol_parenthesized_expression = ts_language_symbol ("parenthesized_expression", true);
      ts_symbol_lparen                   = ts_language_symbol ("(", false);
      ts_symbol_rparen                   = ts_language_symbol (")", false);
      ts_field_operator = ts_language_field ("operator");
      ts_field_left     = ts_language_field ("left");
      ts_field_right    = ts_language_field ("right");
      ts_field_function = ts_language_field ("function");
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

  /* OCaml source files are "expected to be" UTF-8 encoded.
     <https://ocaml.org/manual/5.3/lex.html#sss:lex:text-encoding>  */
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

  #if DEBUG_OCAML
  /* For debugging: Print the tree.  */
  {
    char *tree_as_string = ts_node_string (ts_tree_root_node (tree));
    fprintf (stderr, "Syntax tree: %s\n", tree_as_string);
    free (tree_as_string);
  }
  #endif

  contents = contents_data;

  extract_from_node (ts_tree_root_node (tree),
                     false,
                     null_context_region (),
                     mlp);

  ts_tree_delete (tree);
  ts_parser_delete (parser);
  free (contents_data);

  logical_file_name = NULL;
}
