/* xgettext D backend.
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
#include "x-d.h"

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
#include "string-buffer-reversed.h"
#include "c-ctype.h"
#include "html5-entities.h"
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
#include "byteswap.h"
#include "po-charset.h"
#include "gettext.h"

#define _(s) gettext(s)

/* Use tree-sitter.
   Documentation: <https://tree-sitter.github.io/tree-sitter/using-parsers>  */
#include <tree_sitter/api.h>
extern const TSLanguage *tree_sitter_d (void);


/* The D syntax is defined in <https://dlang.org/spec/spec.html>.
   The design principle of this language appears to be: "If there are two ways
   to get a certain feature, find three more equivalent ways, and support all
   five in the language."
   Examples:
     - There are 5 supported encodings for the source code.
     - There are 3 supported syntaxes for comments.
     - There are 10 supported syntaxes for string literals (not even counting
       the interpolation expression sequences).
     - There are 4 supported ways of including a Unicode character in a
       double-quoted string.
   This guarantees
     - a steep learning curve for the junior programmers,
     - that even senior programmers never fully master the language,
     - that teams of developers will eternally fight over code style and
       irrelevant details,
     - and a high implementation complexity for the language and its runtime.
 */

#define DEBUG_D 0


/* ====================== Keyword set customization.  ====================== */

/* If true extract all strings.  */
static bool extract_all = false;

static hash_table function_keywords;
static hash_table template_keywords;
static bool default_keywords = true;


void
x_d_extract_all ()
{
  extract_all = true;
}


void
x_d_keyword (const char *name)
{
  if (name == NULL)
    default_keywords = false;
  else
    {
      if (function_keywords.table == NULL)
        hash_init (&function_keywords, 100);
      if (template_keywords.table == NULL)
        hash_init (&template_keywords, 100);

      const char *end;
      struct callshape shape;
      split_keywordspec (name, &end, &shape);

      /* The characters between name and end should form a valid identifier,
         possibly with a trailing '!'.
         A colon means an invalid parse in split_keywordspec().  */
      const char *colon = strchr (name, ':');
      if (colon == NULL || colon >= end)
        {
          if (end > name && end[-1] == '!')
            insert_keyword_callshape (&template_keywords, name, end - 1 - name,
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
      /* When adding new keywords here, also update the documentation in
         xgettext.texi!  */
      x_d_keyword ("gettext");
      x_d_keyword ("dgettext:2");
      x_d_keyword ("dcgettext:2");
      x_d_keyword ("ngettext:1,2");
      x_d_keyword ("dngettext:2,3");
      x_d_keyword ("dcngettext:2,3");
      x_d_keyword ("pgettext:1c,2");
      x_d_keyword ("dpgettext:2c,3");
      x_d_keyword ("dcpgettext:2c,3");
      x_d_keyword ("npgettext:1c,2,3");
      x_d_keyword ("dnpgettext:2c,3,4");
      x_d_keyword ("dcnpgettext:2c,3,4");
      default_keywords = false;
    }
}

void
init_flag_table_d ()
{
  xgettext_record_flag ("gettext:1:pass-c-format");
  xgettext_record_flag ("dgettext:2:pass-c-format");
  xgettext_record_flag ("dcgettext:2:pass-c-format");
  xgettext_record_flag ("ngettext:1:pass-c-format");
  xgettext_record_flag ("ngettext:2:pass-c-format");
  xgettext_record_flag ("dngettext:2:pass-c-format");
  xgettext_record_flag ("dngettext:3:pass-c-format");
  xgettext_record_flag ("dcngettext:2:pass-c-format");
  xgettext_record_flag ("dcngettext:3:pass-c-format");
  xgettext_record_flag ("pgettext:2:pass-c-format");
  xgettext_record_flag ("dpgettext:3:pass-c-format");
  xgettext_record_flag ("dcpgettext:3:pass-c-format");
  xgettext_record_flag ("npgettext:2:pass-c-format");
  xgettext_record_flag ("npgettext:3:pass-c-format");
  xgettext_record_flag ("dnpgettext:3:pass-c-format");
  xgettext_record_flag ("dnpgettext:4:pass-c-format");
  xgettext_record_flag ("dcnpgettext:3:pass-c-format");
  xgettext_record_flag ("dcnpgettext:4:pass-c-format");
  xgettext_record_flag ("gettext:1:pass-d-format");
  xgettext_record_flag ("dgettext:2:pass-d-format");
  xgettext_record_flag ("dcgettext:2:pass-d-format");
  xgettext_record_flag ("ngettext:1:pass-d-format");
  xgettext_record_flag ("ngettext:2:pass-d-format");
  xgettext_record_flag ("dngettext:2:pass-d-format");
  xgettext_record_flag ("dngettext:3:pass-d-format");
  xgettext_record_flag ("dcngettext:2:pass-d-format");
  xgettext_record_flag ("dcngettext:3:pass-d-format");
  xgettext_record_flag ("pgettext:2:pass-d-format");
  xgettext_record_flag ("dpgettext:3:pass-d-format");
  xgettext_record_flag ("dcpgettext:3:pass-d-format");
  xgettext_record_flag ("npgettext:2:pass-d-format");
  xgettext_record_flag ("npgettext:3:pass-d-format");
  xgettext_record_flag ("dnpgettext:3:pass-d-format");
  xgettext_record_flag ("dnpgettext:4:pass-d-format");
  xgettext_record_flag ("dcnpgettext:3:pass-d-format");
  xgettext_record_flag ("dcnpgettext:4:pass-d-format");

  /* Module core.stdc.stdio
     <https://dlang.org/library/core/stdc/stdio.html>  */
  xgettext_record_flag ("fprintf:2:c-format");
  xgettext_record_flag ("vfprintf:2:c-format");
  xgettext_record_flag ("printf:1:c-format");
  xgettext_record_flag ("vprintf:1:c-format");
  xgettext_record_flag ("sprintf:2:c-format");
  xgettext_record_flag ("vsprintf:2:c-format");
  xgettext_record_flag ("snprintf:3:c-format");
  xgettext_record_flag ("vsnprintf:3:c-format");

  /* Module std.format
     <https://dlang.org/library/std/format.html>  */
  xgettext_record_flag ("format:1:d-format");
  xgettext_record_flag ("sformat:2:d-format");
}


/* ======================== Parsing via tree-sitter. ======================== */
/* To understand this code, look at
     tree-sitter-d/src/node-types.json
   and
     tree-sitter-d/src/grammar.json
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

MAYBE_UNUSED static TSFieldId
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
static TSSymbol ts_symbol_comment;
static TSSymbol ts_symbol_string_literal;
static TSSymbol ts_symbol_quoted_string;
static TSSymbol ts_symbol_escape_sequence;
static TSSymbol ts_symbol_htmlentity;
static TSSymbol ts_symbol_raw_string;
static TSSymbol ts_symbol_hex_string;
static TSSymbol ts_symbol_binary_expression;
static TSSymbol ts_symbol_add_expression;
static TSSymbol ts_symbol_expression;
static TSSymbol ts_symbol_identifier;
static TSSymbol ts_symbol_property_expression;
static TSSymbol ts_symbol_call_expression;
static TSSymbol ts_symbol_named_arguments;
static TSSymbol ts_symbol_named_argument;
static TSSymbol ts_symbol_template_instance;
static TSSymbol ts_symbol_template_arguments;
static TSSymbol ts_symbol_template_argument;
static TSSymbol ts_symbol_unittest_declaration;
static TSSymbol ts_symbol_tilde; /* ~ */

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
  #if DEBUG_D && 0
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
      /* It should either start with two slashes...  */
      if (sd_length (entire) >= 2
          && sd_char_at (entire, 0) == '/'
          && sd_char_at (entire, 1) == '/')
        {
          save_comment_line (sd_substring (entire, 2, sd_length (entire)));
          last_comment_line = ts_node_end_point (node).row + 1;
        }
      /* ... or it should start and end with the C comment markers or
         with the D nested comment markers.  */
      else if (sd_length (entire) >= 4
               && sd_char_at (entire, 0) == '/'
               && ((sd_char_at (entire, 1) == '*'
                    && sd_char_at (entire, sd_length (entire) - 2) == '*')
                   || (sd_char_at (entire, 1) == '+'
                       && sd_char_at (entire, sd_length (entire) - 2) == '+'))
               && sd_char_at (entire, sd_length (entire) - 1) == '/')
        {
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
        abort ();
    }
  else
    last_non_comment_line = ts_node_line_number (node);
}

/* ---------------------------- String literals ---------------------------- */

/* Determines whether NODE is an 'add_expression' with a '~' operator between
   two operands.  If so, it returns the indices of the two operands.  */
static bool
is_add_expression_with_tilde (TSNode node,
                              uint32_t *left_operand_index,
                              uint32_t *right_operand_index)
{
  if (ts_node_symbol (node) == ts_symbol_add_expression)
    {
      uint32_t count = ts_node_child_count (node);
      uint32_t other_subnodes = 0;
      for (uint32_t i = 0; i < count; i++)
        {
          TSNode subnode = ts_node_child (node, i);
          if (!(ts_node_symbol (subnode) == ts_symbol_comment
                || (ts_node_symbol (subnode) == ts_symbol_tilde
                    && other_subnodes == 1)))
            {
              switch (other_subnodes)
                {
                case 0:
                  *left_operand_index = i;
                  break;
                case 1:
                  *right_operand_index = i;
                  break;
                case 2:
                default:
                  return false;
                }
              other_subnodes++;
            }
        }
      return other_subnodes == 2;
    }
  else
    return false;
}

/* Determines whether NODE represents a string literal or the concatenation
   of string literals (via the '~' operator).  */
static bool
is_string_literal (TSNode node)
{
 start:
  if (ts_node_symbol (node) == ts_symbol_string_literal)
    {
      string_desc_t node_contents =
        sd_new_addr (ts_node_end_byte (node) - ts_node_start_byte (node),
                     contents + ts_node_start_byte (node));
      #if DEBUG_D && 0
      fprintf (stderr, "[%s]|%s|%.*s|\n", ts_node_type (node), ts_node_string (node), (int) sd_length (node_contents), sd_data (node_contents));
      #if 0
      uint32_t count = ts_node_child_count (node);
      for (uint32_t i = 0; i < count; i++)
        {
          TSNode subnode = ts_node_named_child (node, i);
          string_desc_t subnode_contents =
            sd_new_addr (ts_node_end_byte (subnode) - ts_node_start_byte (subnode),
                         contents + ts_node_start_byte (subnode));
          fprintf (stderr, "%u -> [%s]|%s|%.*s|\n", i, ts_node_type (subnode), ts_node_string (subnode), (int) sd_length (subnode_contents), sd_data (subnode_contents));
          uint32_t count2 = ts_node_child_count (subnode);
          for (uint32_t j = 0; j < count2; j++)
            {
              fprintf (stderr, "%u %u -> [%s]|%s|\n", i, j, ts_node_type (ts_node_child (subnode, j)), ts_node_string (ts_node_child (subnode, j)));
            }
        }
      #endif
      #endif
      /* tree-sitter-d does not do a good job of dissecting the string literal
         into its constituents.  Therefore we have to look at the node's entire
         contents and dissect ourselves.  */
      /* Interpolation expression sequences look like string literals but are
         not, since they need a '.text' call to convert to string.  */
      if (sd_char_at (node_contents, 0) == 'i')
        return false;
      /* We only want string literals with 'char' elements, not 'wchar' or
         'dchar'.  */
      if (sd_char_at (node_contents, sd_length (node_contents) - 1) == 'w'
          || sd_char_at (node_contents, sd_length (node_contents) - 1) == 'd')
        return false;
      return true;
    }
  if (ts_node_symbol (node) == ts_symbol_binary_expression
      && ts_node_child_count (node) == 1)
    {
      TSNode subnode = ts_node_child (node, 0);
      uint32_t left_index;
      uint32_t right_index;
      if (is_add_expression_with_tilde (subnode, &left_index, &right_index)
          /* Recurse into the left and right subnodes.  */
          && is_string_literal (ts_node_child (subnode, right_index)))
        {
          /*return is_string_literal (ts_node_child (subnode, left_index));*/
          node = ts_node_child (subnode, left_index);
          goto start;
        }
    }
  if (ts_node_symbol (node) == ts_symbol_expression
      && ts_node_named_child_count (node) == 1)
    {
      TSNode subnode = ts_node_named_child (node, 0);
      /* Recurse.  */
      /*return is_string_literal (subnode);*/
      node = subnode;
      goto start;
    }
  return false;
}

/* Prepends the string literal pieces from NODE to BUFFER.  */
static void
string_literal_accumulate_pieces (TSNode node,
                                  struct string_buffer_reversed *buffer)
{
 start:
  if (ts_node_symbol (node) == ts_symbol_string_literal)
    {
      /* tree-sitter-d does not do a good job of dissecting the string literal
         into its constituents.  Therefore we have to look at the node's entire
         contents and dissect ourselves.  The only help we get is the list of
         escape sequences in a double-quoted string literal:
         (string_literal (quoted_string (escape_sequence) ... (escape_sequence)))
       */
      string_desc_t node_contents =
        sd_new_addr (ts_node_end_byte (node) - ts_node_start_byte (node),
                     contents + ts_node_start_byte (node));
      #if DEBUG_D && 0
      fprintf (stderr, "[%s]|%s|%.*s|\n", ts_node_type (node), ts_node_string (node), (int) sd_length (node_contents), sd_data (node_contents));
      #endif
      /* Drop StringPostfix.  */
      if (sd_length (node_contents) >= 1
          && sd_char_at (node_contents, sd_length (node_contents) - 1) == 'c')
        node_contents = sd_substring (node_contents, 0, sd_length (node_contents) - 1);
      /* Distinguish the various cases.  */
      if (sd_length (node_contents) >= 2
          && sd_char_at (node_contents, 0) == '"'
          && sd_char_at (node_contents, sd_length (node_contents) - 1) == '"')
        {
          /* A double-quoted string.  */
          if (ts_node_child_count (node) != 1)
            abort ();
          TSNode subnode = ts_node_child (node, 0);
          if (ts_node_symbol (subnode) != ts_symbol_quoted_string)
            abort ();
          node_contents = sd_substring (node_contents, 1, sd_length (node_contents) - 1);
          const char *ptr = sd_data (node_contents) + sd_length (node_contents);
          /* Iterate through the nodes of type escape_sequence under the subnode.  */
          uint32_t count = ts_node_named_child_count (subnode);
          for (uint32_t i = count; i > 0; )
            {
              i--;
              TSNode escnode = ts_node_named_child (subnode, i);
              if (ts_node_symbol (escnode) == ts_symbol_escape_sequence
                  || ts_node_symbol (escnode) == ts_symbol_htmlentity)
                {
                  const char *escape_start = contents + ts_node_start_byte (escnode);
                  const char *escape_end = contents + ts_node_end_byte (escnode);
                  if (escape_end < ptr)
                    sbr_xprepend_desc (buffer, sd_new_addr (ptr - escape_end, escape_end));

                  /* The escape sequence must start with a backslash.  */
                  if (!(escape_end - escape_start >= 2 && escape_start[0] == '\\'))
                    abort ();
                  /* tree-sitter's grammar.js allows more escape sequences than the
                     specification.  Give a warning for the invalid cases.  */
                  bool invalid = false;
                  if (escape_end - escape_start == 2)
                    {
                      switch (escape_start[1])
                        {
                        case '\'':
                        case '"':
                        case '?':
                        case '\\':
                          sbr_xprepend1 (buffer, escape_start[1]);
                          break;
                        case '0': case '1': case '2': case '3':
                        case '4': case '5': case '6': case '7':
                          sbr_xprepend1 (buffer, escape_start[1] - '0');
                          break;
                        case 'a':
                          sbr_xprepend1 (buffer, 0x07);
                          break;
                        case 'b':
                          sbr_xprepend1 (buffer, 0x08);
                          break;
                        case 'f':
                          sbr_xprepend1 (buffer, 0x0C);
                          break;
                        case 'n':
                          sbr_xprepend1 (buffer, '\n');
                          break;
                        case 'r':
                          sbr_xprepend1 (buffer, '\r');
                          break;
                        case 't':
                          sbr_xprepend1 (buffer, '\t');
                          break;
                        case 'v':
                          sbr_xprepend1 (buffer, 0x0B);
                          break;
                        default:
                          invalid = true;
                          break;
                        }
                    }
                  else if (escape_start[1] >= '0' && escape_start[1] <= '7')
                    {
                      unsigned int value = 0;
                      /* Only up to 3 octal digits are accepted.  */
                      if (escape_end - escape_start <= 1 + 3)
                        {
                          for (const char *p = escape_start + 1; p < escape_end; p++)
                            {
                              /* No overflow is possible.  */
                              char c = *p;
                              if (c >= '0' && c <= '7')
                                value = (value << 3) + (c - '0');
                              else
                                invalid = true;
                            }
                          if (value > 0xFF)
                            invalid = true;
                        }
                      if (!invalid)
                        sbr_xprepend1 (buffer, (unsigned char) value);
                    }
                  else if ((escape_start[1] == 'x' && escape_end - escape_start == 2 + 2)
                           || (escape_start[1] == 'u' && escape_end - escape_start == 2 + 4)
                           || (escape_start[1] == 'U' && escape_end - escape_start == 2 + 8))
                    {
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
                            invalid = true;
                        }
                      if (escape_start[1] == 'x')
                        {
                          if (!invalid)
                            sbr_xprepend1 (buffer, (unsigned char) value);
                        }
                      else if (value < 0x110000 && !(value >= 0xD800 && value < 0xE000))
                        {
                          uint8_t buf[6];
                          int n = u8_uctomb (buf, value, sizeof (buf));
                          if (!(n > 0))
                            abort ();
                          sbr_xprepend_desc (buffer, sd_new_addr (n, (const char *) buf));
                        }
                      else
                        invalid = true;
                    }
                  else if (escape_start[1] == '&' && escape_end[-1] == ';')
                    {
                      /* A named character entity.  */
                      string_desc_t entity =
                        sd_new_addr (escape_end - escape_start - 3, escape_start + 2);
                      const char *value = html5_lookup (entity);
                      if (value != NULL)
                        sbr_xprepend_c (buffer, value);
                      else
                        invalid = true;
                    }
                  else
                    invalid = true;
                  if (invalid)
                    {
                      size_t line_number = ts_node_line_number (escnode);
                      if_error (IF_SEVERITY_WARNING,
                                logical_file_name, line_number, (size_t)(-1), false,
                                _("invalid escape sequence in string"));
                    }

                  ptr = escape_start;
                }
              else
                abort ();
            }
          sbr_xprepend_desc (buffer, sd_substring (node_contents, 0, ptr - sd_data (node_contents)));
        }
      else if (sd_length (node_contents) >= 3
               && sd_char_at (node_contents, 0) == 'x'
               && sd_char_at (node_contents, 1) == '"'
               && sd_char_at (node_contents, sd_length (node_contents) - 1) == '"')
        {
          /* A hex string.  */
          if (ts_node_child_count (node) != 1)
            abort ();
          TSNode subnode = ts_node_child (node, 0);
          if (ts_node_symbol (subnode) != ts_symbol_hex_string)
            abort ();
          node_contents = sd_substring (node_contents, 2, sd_length (node_contents) - 1);
          int shift = 0;
          int value = 0;
          for (ptrdiff_t i = sd_length (node_contents) - 1; i >= 0; i--)
            {
              char c = sd_char_at (node_contents, i);
              if (c >= '0' && c <= '9')
                {
                  value += (c - '0') << shift;
                  shift += 4;
                }
              else if (c >= 'A' && c <= 'F')
                {
                  value += (c - 'A' + 10) << shift;
                  shift += 4;
                }
              else if (c >= 'a' && c <= 'f')
                {
                  value += (c - 'a' + 10) << shift;
                  shift += 4;
                }
              if (shift == 8)
                {
                  sbr_xprepend1 (buffer, value);
                  value = 0;
                  shift = 0;
                }
            }
          /* If shift == 4 here, there was an odd number of hex digits.  */
        }
      else
        {
          /* A raw string, delimited string, or token string.  */
          if (sd_char_at (node_contents, 0) == 'q')
            {
              if (sd_length (node_contents) >= 3
                  && sd_char_at (node_contents, 1) == '{'
                  && sd_char_at (node_contents, sd_length (node_contents) - 1) == '}')
                /* A token string.  */
                node_contents = sd_substring (node_contents, 2, sd_length (node_contents) - 1);
              else if (sd_length (node_contents) >= 3
                       && sd_char_at (node_contents, 1) == '"'
                       && sd_char_at (node_contents, sd_length (node_contents) - 1) == '"')
                {
                  /* A delimited string.  */
                  node_contents = sd_substring (node_contents, 2, sd_length (node_contents) - 1);
                  if (sd_length (node_contents) >= 2
                      && ((sd_char_at (node_contents, 0) == '('
                           && sd_char_at (node_contents, sd_length (node_contents) - 1) == ')')
                          || (sd_char_at (node_contents, 0) == '['
                              && sd_char_at (node_contents, sd_length (node_contents) - 1) == ']')
                          || (sd_char_at (node_contents, 0) == '{'
                              && sd_char_at (node_contents, sd_length (node_contents) - 1) == '}')
                          || (sd_char_at (node_contents, 0) == '<'
                              && sd_char_at (node_contents, sd_length (node_contents) - 1) == '>')
                          || (sd_char_at (node_contents, 0) == sd_char_at (node_contents, sd_length (node_contents) - 1)
                              && !(c_isalpha (sd_char_at (node_contents, 0)) || sd_char_at (node_contents, 0) == '_'))))
                    node_contents = sd_substring (node_contents, 1, sd_length (node_contents) - 1);
                  else
                    {
                      ptrdiff_t first_newline = sd_index (node_contents, '\n');
                      if (first_newline < 0)
                        abort ();
                      ptrdiff_t last_newline = sd_last_index (node_contents, '\n');
                      if (last_newline < 0)
                        abort ();
                      string_desc_t delimiter = sd_substring (node_contents, last_newline + 1, sd_length (node_contents));
                      size_t delimiter_length = sd_length (delimiter);
                      if (delimiter_length == 0)
                        abort ();
                      if (!((first_newline == delimiter_length
                             || (first_newline == delimiter_length + 1
                                 && sd_char_at (node_contents, delimiter_length) == '\r'))
                            && sd_equals (sd_substring (node_contents, 0, delimiter_length), delimiter)))
                        abort ();
                      node_contents = sd_substring (node_contents, first_newline + 1, last_newline + 1);
                    }
                }
              else
                abort ();
            }
          else if (sd_length (node_contents) >= 3
                   && sd_char_at (node_contents, 0) == 'r'
                   && sd_char_at (node_contents, 1) == '"'
                   && sd_char_at (node_contents, sd_length (node_contents) - 1) == '"')
            /* A raw string.  */
            node_contents = sd_substring (node_contents, 2, sd_length (node_contents) - 1);
          else if (sd_length (node_contents) >= 2
                   && sd_char_at (node_contents, 0) == '`'
                   && sd_char_at (node_contents, sd_length (node_contents) - 1) == '`')
            /* A raw string.  */
            node_contents = sd_substring (node_contents, 1, sd_length (node_contents) - 1);
          else
            abort ();

          sbr_xprepend_desc (buffer, node_contents);
        }
    }
  else if (ts_node_symbol (node) == ts_symbol_binary_expression
           && ts_node_child_count (node) == 1)
    {
      TSNode subnode = ts_node_child (node, 0);
      uint32_t left_index;
      uint32_t right_index;
      if (is_add_expression_with_tilde (subnode, &left_index, &right_index))
        {
          /* Recurse into the left and right subnodes.  */
          string_literal_accumulate_pieces (ts_node_child (subnode, right_index), buffer);
          /*string_literal_accumulate_pieces (ts_node_child (subnode, left_index), buffer);*/
          node = ts_node_child (subnode, left_index);
          goto start;
        }
      else
        abort ();
    }
  else if (ts_node_symbol (node) == ts_symbol_expression
           && ts_node_named_child_count (node) == 1)
    {
      TSNode subnode = ts_node_named_child (node, 0);
      /* Recurse.  */
      /*string_literal_accumulate_pieces (subnode, buffer);*/
      node = subnode;
      goto start;
    }
  else
    abort ();
}

/* Combines the pieces of a string or template_string or concatenated
   string literal.
   Returns a freshly allocated, mostly UTF-8 encoded string.  */
static char *
string_literal_value (TSNode node)
{
  struct string_buffer_reversed buffer;
  sbr_init (&buffer);
  string_literal_accumulate_pieces (node, &buffer);
  return sbr_xdupfree_c (&buffer);
}

/* --------------------- Parsing and string extraction --------------------- */

/* Context lookup table.  */
static flag_context_list_table_ty *flag_context_list_table;

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
                               bool ignore,
                               bool callee_in_call_expression,
                               flag_region_ty *outer_region,
                               message_list_ty *mlp);

/* Extracts messages from the function call NODE consisting of
     - CALLEE_NODE: a tree node of type 'identifier' or 'property_expression',
     - ARGS_NODE: a tree node of type 'named_arguments'.
   Extracted messages are added to MLP.  */
static void
extract_from_function_call (TSNode node,
                            TSNode callee_node,
                            TSNode args_node,
                            flag_region_ty *outer_region,
                            message_list_ty *mlp)
{
  uint32_t args_count = ts_node_child_count (args_node);

  TSNode function_node;
  if (ts_node_symbol (callee_node) == ts_symbol_identifier)
    function_node = callee_node;
  else if (ts_node_symbol (callee_node) == ts_symbol_property_expression)
    function_node = ts_node_child (callee_node, ts_node_child_count (callee_node) - 1);
  else
    abort ();

  flag_context_list_iterator_ty next_context_iter;

  if (ts_node_symbol (function_node) == ts_symbol_identifier)
    {
      string_desc_t function_name =
        sd_new_addr (ts_node_end_byte (function_node) - ts_node_start_byte (function_node),
                     contents + ts_node_start_byte (function_node));

      /* Context iterator.  */
      next_context_iter =
        flag_context_list_iterator (
          flag_context_list_table_lookup (
            flag_context_list_table,
            sd_data (function_name), sd_length (function_name)));

      void *keyword_value;
      if (hash_find_entry (&function_keywords,
                           sd_data (function_name), sd_length (function_name),
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
          uint32_t arg = 0;

          /* The first part of the 'property_expression' is treated as the first
             argument.  Cf. <https://dlang.org/spec/function.html#pseudo-member>  */
          if (ts_node_symbol (callee_node) == ts_symbol_property_expression)
            {
              arg++;
              flag_region_ty *arg_region =
                inheriting_region (outer_region,
                                   flag_context_list_iterator_advance (
                                     &next_context_iter));

              bool already_extracted = false;
              TSNode arg_expr_node = ts_node_child (callee_node, 0);
              if (is_string_literal (arg_expr_node))
                {
                  lex_pos_ty pos;
                  pos.file_name = logical_file_name;
                  pos.line_number = ts_node_line_number (arg_expr_node);

                  char *string = string_literal_value (arg_expr_node);

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
                              logical_file_name, ts_node_line_number (arg_expr_node), (size_t)(-1), false,
                              _("too many open parentheses, brackets, or braces"));
                  extract_from_node (arg_expr_node,
                                     false,
                                     false,
                                     arg_region,
                                     mlp);
                  nesting_depth--;
                }

              {
                /* Handle the potential comments in the callee_node, between
                   arg_expr_node and function_node.  */
                uint32_t count = ts_node_child_count (callee_node);
                for (uint32_t i = 1; i < count; i++)
                  {
                    TSNode subnode = ts_node_child (callee_node, i);
                    if (ts_node_eq (subnode, function_node))
                      break;
                    handle_comments (subnode);
                  }
              }

              unref_region (arg_region);
            }

          /* Handle the potential comments in node, between
             callee_node and args_node.  */
          {
            uint32_t count = ts_node_child_count (node);
            for (uint32_t i = 1; i < count; i++)
              {
                TSNode subnode = ts_node_child (node, i);
                if (ts_node_eq (subnode, args_node))
                  break;
                handle_comments (subnode);
              }
          }

          for (uint32_t i = 0; i < args_count; i++)
            {
              TSNode arg_node = ts_node_child (args_node, i);
              handle_comments (arg_node);
              if (ts_node_is_named (arg_node)
                  && ts_node_symbol (arg_node) != ts_symbol_comment)
                {
                  if (ts_node_symbol (arg_node) != ts_symbol_named_argument)
                    abort ();
                  arg++;
                  flag_region_ty *arg_region =
                    inheriting_region (outer_region,
                                       flag_context_list_iterator_advance (
                                         &next_context_iter));

                  bool already_extracted = false;
                  if (ts_node_child_count (arg_node) == 1)
                    {
                      TSNode arg_expr_node = ts_node_child (arg_node, 0);
                      if (is_string_literal (arg_expr_node))
                        {
                          lex_pos_ty pos;
                          pos.file_name = logical_file_name;
                          pos.line_number = ts_node_line_number (arg_expr_node);

                          char *string = string_literal_value (arg_expr_node);

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
                    }

                  if (!already_extracted)
                    {
                      if (++nesting_depth > MAX_NESTING_DEPTH)
                        if_error (IF_SEVERITY_FATAL_ERROR,
                                  logical_file_name, ts_node_line_number (arg_node), (size_t)(-1), false,
                                  _("too many open parentheses, brackets, or braces"));
                      extract_from_node (arg_node,
                                         false,
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
    }
  else
    next_context_iter = null_context_list_iterator;

  /* Recurse.  */

  /* Current argument number.  */
  MAYBE_UNUSED uint32_t arg = 0;

  /* The first part of the 'property_expression' is treated as the first
     argument.  Cf. <https://dlang.org/spec/function.html#pseudo-member>  */
  if (ts_node_symbol (callee_node) == ts_symbol_property_expression)
    {
      arg++;
      flag_region_ty *arg_region =
        inheriting_region (outer_region,
                           flag_context_list_iterator_advance (
                             &next_context_iter));
      TSNode arg_expr_node = ts_node_child (callee_node, 0);

      if (++nesting_depth > MAX_NESTING_DEPTH)
        if_error (IF_SEVERITY_FATAL_ERROR,
                  logical_file_name, ts_node_line_number (arg_expr_node), (size_t)(-1), false,
                  _("too many open parentheses, brackets, or braces"));
      extract_from_node (arg_expr_node,
                         false,
                         false,
                         arg_region,
                         mlp);
      nesting_depth--;

      {
        /* Handle the potential comments in the callee_node, between
           arg_expr_node and function_node.  */
        uint32_t count = ts_node_child_count (callee_node);
        for (uint32_t i = 1; i < count; i++)
          {
            TSNode subnode = ts_node_child (callee_node, i);
            if (ts_node_eq (subnode, function_node))
              break;
            handle_comments (subnode);
          }
      }

      unref_region (arg_region);
    }

  /* Handle the potential comments in node, between
     callee_node and args_node.  */
  {
    uint32_t count = ts_node_child_count (node);
    for (uint32_t i = 1; i < count; i++)
      {
        TSNode subnode = ts_node_child (node, i);
        if (ts_node_eq (subnode, args_node))
          break;
        handle_comments (subnode);
      }
  }

  for (uint32_t i = 0; i < args_count; i++)
    {
      TSNode arg_node = ts_node_child (args_node, i);
      handle_comments (arg_node);
      if (ts_node_is_named (arg_node)
          && ts_node_symbol (arg_node) != ts_symbol_comment)
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
                             false,
                             false,
                             arg_region,
                             mlp);
          nesting_depth--;

          unref_region (arg_region);
        }
    }
}

/* Extracts messages from the function call consisting of
     - CALLEE_NODE: a tree node of type 'property_expression'.
   Extracted messages are added to MLP.  */
static void
extract_from_function_call_without_args (TSNode callee_node,
                                         flag_region_ty *outer_region,
                                         message_list_ty *mlp)
{
  TSNode function_node = ts_node_child (callee_node, ts_node_child_count (callee_node) - 1);

  flag_context_list_iterator_ty next_context_iter;

  if (ts_node_symbol (function_node) == ts_symbol_identifier)
    {
      string_desc_t function_name =
        sd_new_addr (ts_node_end_byte (function_node) - ts_node_start_byte (function_node),
                     contents + ts_node_start_byte (function_node));

      /* Context iterator.  */
      next_context_iter =
        flag_context_list_iterator (
          flag_context_list_table_lookup (
            flag_context_list_table,
            sd_data (function_name), sd_length (function_name)));

      void *keyword_value;
      if (hash_find_entry (&function_keywords,
                           sd_data (function_name), sd_length (function_name),
                           &keyword_value)
          == 0)
        {
          /* The callee has some information associated with it.  */
          const struct callshapes *next_shapes = keyword_value;

          /* We have a function, named by a relevant identifier, with an implicit
             argument list.  */

          struct arglist_parser *argparser =
            arglist_parser_alloc (mlp, next_shapes);

          /* Current argument number.  */
          uint32_t arg = 0;

          /* The first part of the 'property_expression' is treated as the first
             argument.  Cf. <https://dlang.org/spec/function.html#pseudo-member>  */
          arg++;
          flag_region_ty *arg_region =
            inheriting_region (outer_region,
                               flag_context_list_iterator_advance (
                                 &next_context_iter));

          bool already_extracted = false;
          TSNode arg_expr_node = ts_node_child (callee_node, 0);
          if (is_string_literal (arg_expr_node))
            {
              lex_pos_ty pos;
              pos.file_name = logical_file_name;
              pos.line_number = ts_node_line_number (arg_expr_node);

              char *string = string_literal_value (arg_expr_node);

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
                          logical_file_name, ts_node_line_number (arg_expr_node), (size_t)(-1), false,
                          _("too many open parentheses, brackets, or braces"));
              extract_from_node (arg_expr_node,
                                 false,
                                 false,
                                 arg_region,
                                 mlp);
              nesting_depth--;
            }

          {
            /* Handle the potential comments in the callee_node, between
               arg_expr_node and function_node.  */
            uint32_t count = ts_node_child_count (callee_node);
            for (uint32_t i = 1; i < count; i++)
              {
                TSNode subnode = ts_node_child (callee_node, i);
                if (ts_node_eq (subnode, function_node))
                  break;
                handle_comments (subnode);
              }
          }

          unref_region (arg_region);

          arglist_parser_done (argparser, arg);
          return;
        }
    }
  else
    next_context_iter = null_context_list_iterator;

  /* Recurse.  */

  /* Current argument number.  */
  MAYBE_UNUSED uint32_t arg = 0;

  /* The first part of the 'property_expression' is treated as the first
     argument.  Cf. <https://dlang.org/spec/function.html#pseudo-member>  */
  arg++;
  flag_region_ty *arg_region =
    inheriting_region (outer_region,
                       flag_context_list_iterator_advance (
                         &next_context_iter));
  TSNode arg_expr_node = ts_node_child (callee_node, 0);

  if (++nesting_depth > MAX_NESTING_DEPTH)
    if_error (IF_SEVERITY_FATAL_ERROR,
              logical_file_name, ts_node_line_number (arg_expr_node), (size_t)(-1), false,
              _("too many open parentheses, brackets, or braces"));
  extract_from_node (arg_expr_node,
                     false,
                     false,
                     arg_region,
                     mlp);
  nesting_depth--;

  {
    /* Handle the potential comments in the callee_node, between
       arg_expr_node and function_node.  */
    uint32_t count = ts_node_child_count (callee_node);
    for (uint32_t i = 1; i < count; i++)
      {
        TSNode subnode = ts_node_child (callee_node, i);
        if (ts_node_eq (subnode, function_node))
          break;
        handle_comments (subnode);
      }
  }

  unref_region (arg_region);
}

/* Extracts messages from the template instantiation NODE consisting of
     - IDENTIFIER_NODE: a tree node of type 'identifier',
     - ARGS_NODE: a tree node of type 'template_arguments'.
   Extracted messages are added to MLP.  */
static void
extract_from_template_instantiation (TSNode node,
                                     TSNode identifier_node,
                                     TSNode args_node,
                                     flag_region_ty *outer_region,
                                     message_list_ty *mlp)
{
  uint32_t args_count = ts_node_child_count (args_node);

  string_desc_t template_name =
    sd_new_addr (ts_node_end_byte (identifier_node) - ts_node_start_byte (identifier_node),
                 contents + ts_node_start_byte (identifier_node));

  /* Handle the potential comments in node, between
     identifier_node and args_node.  */
  {
    uint32_t count = ts_node_child_count (node);
    for (uint32_t i = 1; i < count; i++)
      {
        TSNode subnode = ts_node_child (node, i);
        if (ts_node_eq (subnode, args_node))
          break;
        handle_comments (subnode);
      }
  }

  /* Context iterator.  */
  flag_context_list_iterator_ty next_context_iter =
    flag_context_list_iterator (
      flag_context_list_table_lookup (
        flag_context_list_table,
        sd_data (template_name), sd_length (template_name)));

  void *keyword_value;
  if (hash_find_entry (&template_keywords,
                       sd_data (template_name), sd_length (template_name),
                       &keyword_value)
      == 0)
    {
      /* The identifier has some information associated with it.  */
      const struct callshapes *next_shapes = keyword_value;

      /* We have a template instantiation, named by a relevant identifier, with
         either a single argument or an argument list.  */

      struct arglist_parser *argparser =
        arglist_parser_alloc (mlp, next_shapes);

      /* Current argument number.  */
      uint32_t arg = 0;

      for (uint32_t i = 0; i < args_count; i++)
        {
          TSNode arg_node = ts_node_child (args_node, i);
          handle_comments (arg_node);
          if (ts_node_is_named (arg_node)
              && ts_node_symbol (arg_node) != ts_symbol_comment)
            {
              if (ts_node_symbol (arg_node) == ts_symbol_template_argument)
                {
                  arg++;
                  flag_region_ty *arg_region =
                    inheriting_region (outer_region,
                                       flag_context_list_iterator_advance (
                                         &next_context_iter));

                  bool already_extracted = false;
                  if (ts_node_child_count (arg_node) == 1)
                    {
                      TSNode arg_expr_node = ts_node_child (arg_node, 0);
                      if (is_string_literal (arg_expr_node))
                        {
                          lex_pos_ty pos;
                          pos.file_name = logical_file_name;
                          pos.line_number = ts_node_line_number (arg_expr_node);

                          char *string = string_literal_value (arg_expr_node);

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
                    }

                  if (!already_extracted)
                    {
                      if (++nesting_depth > MAX_NESTING_DEPTH)
                        if_error (IF_SEVERITY_FATAL_ERROR,
                                  logical_file_name, ts_node_line_number (arg_node), (size_t)(-1), false,
                                  _("too many open parentheses, brackets, or braces"));
                      extract_from_node (arg_node,
                                         false,
                                         false,
                                         arg_region,
                                         mlp);
                      nesting_depth--;
                    }

                  unref_region (arg_region);
                }
              else /* Assume a single template argument.  */
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
                                  _("too many open parentheses, brackets, or braces"));
                      extract_from_node (arg_node,
                                         false,
                                         false,
                                         arg_region,
                                         mlp);
                      nesting_depth--;
                    }

                    unref_region (arg_region);
                }
            }
        }
      arglist_parser_done (argparser, arg);
      return;
    }

  /* Recurse.  */

  /* Current argument number.  */
  MAYBE_UNUSED uint32_t arg = 0;

  for (uint32_t i = 0; i < args_count; i++)
    {
      TSNode arg_node = ts_node_child (args_node, i);
      handle_comments (arg_node);
      if (ts_node_is_named (arg_node)
          && ts_node_symbol (arg_node) != ts_symbol_comment)
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
                             false,
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
                   bool callee_in_call_expression,
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

  if (ts_node_symbol (node) == ts_symbol_call_expression
      && ts_node_named_child_count (node) >= 2)
    {
      TSNode callee_node = ts_node_named_child (node, 0);
      if (ts_node_symbol (callee_node) == ts_symbol_identifier
          || ts_node_symbol (callee_node) == ts_symbol_property_expression)
        {
          uint32_t ncount = ts_node_named_child_count (node);
          uint32_t a;
          for (a = 1; a < ncount; a++)
            if (ts_node_symbol (ts_node_named_child (node, a)) == ts_symbol_named_arguments)
              break;
          if (a < ncount)
            {
              TSNode args_node = ts_node_named_child (node, a);
              if (ts_node_symbol (args_node) != ts_symbol_named_arguments)
                abort ();
              extract_from_function_call (node, callee_node, args_node,
                                          outer_region,
                                          mlp);
              return;
            }
        }
    }

  if (!callee_in_call_expression
      && ts_node_symbol (node) == ts_symbol_property_expression)
    {
      /* A 'property_expression' that is not in the position of the callee in a
         call_expression is treated like a call_expression with 0 arguments.  */
      extract_from_function_call_without_args (node,
                                               outer_region,
                                               mlp);
      return;
    }

  if (ts_node_symbol (node) == ts_symbol_template_instance
      && ts_node_named_child_count (node) >= 2)
    {
      TSNode identifier_node = ts_node_named_child (node, 0);
      if (ts_node_symbol (identifier_node) == ts_symbol_identifier)
        {
          uint32_t ncount = ts_node_named_child_count (node);
          uint32_t a;
          for (a = 1; a < ncount; a++)
            if (ts_node_symbol (ts_node_named_child (node, a)) == ts_symbol_template_arguments)
              break;
          if (a < ncount)
            {
              TSNode args_node = ts_node_named_child (node, a);
              if (ts_node_symbol (args_node) != ts_symbol_template_arguments)
                abort ();
              extract_from_template_instantiation (node,
                                                   identifier_node, args_node,
                                                   outer_region,
                                                   mlp);
              return;
            }
        }
    }

  #if DEBUG_D && 0
  if (ts_node_symbol (node) == ts_symbol_call_expression)
    {
      TSNode subnode = ts_node_named_child (node, 0);
      fprintf (stderr, "-> %s\n", ts_node_string (subnode));
      if (ts_node_symbol (subnode) == ts_symbol_identifier)
        {
          string_desc_t subnode_string =
            sd_new_addr (ts_node_end_byte (subnode) - ts_node_start_byte (subnode),
                         contents + ts_node_start_byte (subnode));
          if (sd_equals (subnode_string, sd_from_c ("gettext")))
            {
              TSNode argsnode = ts_node_named_child (node, 1);
              fprintf (stderr, "gettext arguments: %s\n", ts_node_string (argsnode));
              fprintf (stderr, "gettext children:\n");
              uint32_t count = ts_node_named_child_count (node);
              for (uint32_t i = 0; i < count; i++)
                fprintf (stderr, "%u -> %s\n", i, ts_node_string (ts_node_named_child (node, i)));
            }
        }
    }
  #endif

  /* Recurse.  */
  if (ts_node_symbol (node) != ts_symbol_comment
      /* Ignore the code in unit tests.  Translators are not supposed to
         localize unit tests, only production code.  */
      && ts_node_symbol (node) != ts_symbol_unittest_declaration)
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
                             i == 0 && ts_node_symbol (node) == ts_symbol_call_expression,
                             outer_region,
                             mlp);
          nesting_depth--;
       }
    }
}

void
extract_d (FILE *f,
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
      ts_language = tree_sitter_d ();
      ts_symbol_comment              = ts_language_symbol ("comment", true);
      ts_symbol_string_literal       = ts_language_symbol ("string_literal", true);
      ts_symbol_quoted_string        = ts_language_symbol ("quoted_string", true);
      ts_symbol_escape_sequence      = ts_language_symbol ("escape_sequence", true);
      ts_symbol_htmlentity           = ts_language_symbol ("htmlentity", true);
      ts_symbol_raw_string           = ts_language_symbol ("raw_string", true);
      ts_symbol_hex_string           = ts_language_symbol ("hex_string", true);
      ts_symbol_binary_expression    = ts_language_symbol ("binary_expression", true);
      ts_symbol_add_expression       = ts_language_symbol ("add_expression", true);
      ts_symbol_expression           = ts_language_symbol ("expression", true);
      ts_symbol_identifier           = ts_language_symbol ("identifier", true);
      ts_symbol_property_expression  = ts_language_symbol ("property_expression", true);
      ts_symbol_call_expression      = ts_language_symbol ("call_expression", true);
      ts_symbol_named_arguments      = ts_language_symbol ("named_arguments", true);
      ts_symbol_named_argument       = ts_language_symbol ("named_argument", true);
      ts_symbol_template_instance    = ts_language_symbol ("template_instance", true);
      ts_symbol_template_arguments   = ts_language_symbol ("template_arguments", true);
      ts_symbol_template_argument    = ts_language_symbol ("template_argument", true);
      ts_symbol_unittest_declaration = ts_language_symbol ("unittest_declaration", true);
      ts_symbol_tilde                = ts_language_symbol ("~", false);
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

  /* D source files are UTF-8 or UTF-16 or UTF-32 encoded.
     See <https://dlang.org/spec/lex.html#source_text>.
     But tree-sitter supports only the UTF-8 case, and we want the 'contents'
     variable above to be in an ASCII-compatible encoding as well.  */
  if (u8_check ((uint8_t *) contents_data, contents_length) != NULL)
    {
      /* The file is not UTF-8 encoded.
         Note: contents_data is malloc()ed and therefore suitably aligned.  */
      /* Test whether it is UTF-32 encoded.
         The disambiguation is automatic, because the file is supposed to
         contain at least one U+000A, and U+0A000000 is invalid.  */
      if ((contents_length % 4) == 0)
        {
          for (int round = 0; round < 2; round++)
            {
              if (u32_check ((uint32_t *) contents_data, contents_length / 4) == NULL)
                {
                  /* Convert from UTF-32 to UTF-8.  */
                  size_t u8_contents_length;
                  uint8_t *u8_contents_data =
                    u32_to_u8 ((uint32_t *) contents_data, contents_length / 4,
                               NULL, &u8_contents_length);
                  if (u8_contents_data != NULL)
                    {
                      free (contents_data);
                      contents_length = u8_contents_length;
                      contents_data = (char *) u8_contents_data;
                      goto converted;
                    }
                }
              for (size_t i = 0; i < contents_length / 4; i++)
                ((uint32_t *) contents_data)[i] = bswap_32 (((uint32_t *) contents_data)[i]);
            }
        }
      /* Test whether it is UTF-16 encoded.
         Disambiguate between UTF-16BE and UTF-16LE 1. by looking at the BOM, if present,
         2. by looking at the number of characters U+000A vs. U+0A00 (a heuristic).  */
      if ((contents_length % 2) == 0)
        {
          bool swap;
          if (((uint16_t *) contents_data)[0] == 0xFEFF)
            swap = false;
          else if (((uint16_t *) contents_data)[0] == 0xFFFE)
            swap = true;
          else
            {
              size_t count_000A = 0;
              size_t count_0A00 = 0;
              for (size_t i = 0; i < contents_length / 2; i++)
                {
                  uint16_t uc = ((uint16_t *) contents_data)[i];
                  count_000A += (uc == 0x000A);
                  count_0A00 += (uc == 0x0A00);
                }
              swap = (count_0A00 > count_000A);
            }
          if (swap)
            {
              for (size_t i = 0; i < contents_length / 2; i++)
                ((uint16_t *) contents_data)[i] = bswap_16 (((uint16_t *) contents_data)[i]);
            }
          if (u16_check ((uint16_t *) contents_data, contents_length / 2) == NULL)
            {
              /* Convert from UTF-16 to UTF-8.  */
              size_t u8_contents_length;
              uint8_t *u8_contents_data =
                u16_to_u8 ((uint16_t *) contents_data, contents_length / 2,
                           NULL, &u8_contents_length);
              if (u8_contents_data != NULL)
                {
                  free (contents_data);
                  contents_length = u8_contents_length;
                  contents_data = (char *) u8_contents_data;
                  goto converted;
                }
            }
        }
      error (EXIT_FAILURE, 0,
             _("file \"%s\" is unsupported because not UTF-8 or UTF-16 or UTF-32 encoded"),
             real_filename);
    }
 converted:
  if (u8_check ((uint8_t *) contents_data, contents_length) != NULL)
    abort ();
  xgettext_current_source_encoding = po_charset_utf8;

  /* Create a parser.  */
  TSParser *parser = ts_parser_new ();

  /* Set the parser's language.  */
  ts_parser_set_language (parser, ts_language);

  /* Parse the file, producing a syntax tree.  */
  TSTree *tree = ts_parser_parse_string (parser, NULL, contents_data, contents_length);

  #if DEBUG_D
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
                     false,
                     null_context_region (),
                     mlp);

  ts_tree_delete (tree);
  ts_parser_delete (parser);
  free (contents_data);

  logical_file_name = NULL;
}
