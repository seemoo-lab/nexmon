/* xgettext TypeScript and TSX backends.
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

/* The languages TypeScript and TSX (= TypeScript with JSX) are very similar.
   The extractor code is therefore nearly identical.  */

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
extern const TSLanguage *TREE_SITTER_LANGUAGE (void);


/* The TypeScript syntax is defined in https://www.typescriptlang.org/docs/.  */

#define DEBUG_TYPESCRIPT 0


/* ====================== Keyword set customization.  ====================== */

/* If true extract all strings.  */
static bool extract_all = false;

static hash_table keywords;
static bool default_keywords = true;


void
NOTE_OPTION_EXTRACT_ALL ()
{
  extract_all = true;
}


void
NOTE_OPTION_KEYWORD (const char *name)
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
      /* Same as in x-javascript.c.  */
      /* When adding new keywords here, also update the documentation in
         xgettext.texi!  */
      NOTE_OPTION_KEYWORD ("gettext");
      NOTE_OPTION_KEYWORD ("dgettext:2");
      NOTE_OPTION_KEYWORD ("dcgettext:2");
      NOTE_OPTION_KEYWORD ("ngettext:1,2");
      NOTE_OPTION_KEYWORD ("dngettext:2,3");
      NOTE_OPTION_KEYWORD ("pgettext:1c,2");
      NOTE_OPTION_KEYWORD ("dpgettext:2c,3");
      NOTE_OPTION_KEYWORD ("_");
      default_keywords = false;
    }
}

void
INIT_FLAG_TABLE ()
{
  /* Same as in x-javascript.c.  */
  xgettext_record_flag ("gettext:1:pass-javascript-format");
  xgettext_record_flag ("dgettext:2:pass-javascript-format");
  xgettext_record_flag ("dcgettext:2:pass-javascript-format");
  xgettext_record_flag ("ngettext:1:pass-javascript-format");
  xgettext_record_flag ("ngettext:2:pass-javascript-format");
  xgettext_record_flag ("dngettext:2:pass-javascript-format");
  xgettext_record_flag ("dngettext:3:pass-javascript-format");
  xgettext_record_flag ("pgettext:2:pass-javascript-format");
  xgettext_record_flag ("dpgettext:3:pass-javascript-format");
  xgettext_record_flag ("_:1:pass-javascript-format");
}


/* ======================== Parsing via tree-sitter. ======================== */
/* To understand this code, look at
     tree-sitter-typescript/typescript/src/node-types.json
   and
     tree-sitter-typescript/typescript/src/grammar.json
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
static TSSymbol ts_symbol_comment;
static TSSymbol ts_symbol_string;
static TSSymbol ts_symbol_string_fragment;
static TSSymbol ts_symbol_escape_sequence;
static TSSymbol ts_symbol_template_string;
static TSSymbol ts_symbol_binary_expression;
static TSSymbol ts_symbol_identifier;
static TSSymbol ts_symbol_call_expression;
static TSSymbol ts_symbol_arguments;
static TSSymbol ts_symbol_plus; /* + */
static TSFieldId ts_field_function;
static TSFieldId ts_field_arguments;
static TSFieldId ts_field_operator;
static TSFieldId ts_field_left;
static TSFieldId ts_field_right;

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
  #if DEBUG_TYPESCRIPT && 0
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
      /* ... or it should start and end with the C comment markers.  */
      else if (sd_length (entire) >= 4
               && sd_char_at (entire, 0) == '/'
               && sd_char_at (entire, 1) == '*'
               && sd_char_at (entire, sd_length (entire) - 2) == '*'
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

/* --------------------- string_buffer_reversed_unicode --------------------- */

/* This type is like string_buffer_reversed with mostly UTF-8 contents, except
   that it also handles Unicode surrogates: The combination of a low and a high
   surrogate is converted to a single Unicode code point, whereas lone
   surrogates are converted to U+FFFD (like 'struct mixed_string_buffer' does).
 */
struct string_buffer_reversed_unicode
{
  struct string_buffer_reversed sbr;
  /* The second half of an UTF-16 surrogate character.  */
  unsigned short utf16_surr;
  /* Its line number.  */
  size_t utf16_surr_line_number;
};

/* Initializes a 'struct string_buffer_reversed_unicode'.  */
static inline void
sbru_init (struct string_buffer_reversed_unicode *buffer)
  _GL_ATTRIBUTE_ACQUIRE_CAPABILITY (buffer->sbr.data);
static inline void
sbru_init (struct string_buffer_reversed_unicode *buffer)
{
  sbr_init (&buffer->sbr);
  buffer->utf16_surr = 0;
}

/* Auxiliary function: Handle the attempt to prepend a lone surrogate to
   BUFFER.  */
static void
sbru_prepend_lone_surrogate (struct string_buffer_reversed_unicode *buffer,
                             ucs4_t uc, size_t line_number)
{
  /* A half surrogate is invalid, therefore use U+FFFD instead.
     It may be valid in a particular programming language.
     But a half surrogate is invalid in UTF-8:
       - RFC 3629 says
           "The definition of UTF-8 prohibits encoding character
            numbers between U+D800 and U+DFFF".
       - Unicode 4.0 chapter 3
         <http://www.unicode.org/versions/Unicode4.0.0/ch03.pdf>
         section 3.9, p.77, says
           "Because surrogate code points are not Unicode scalar
            values, any UTF-8 byte sequence that would otherwise
            map to code points D800..DFFF is ill-formed."
         and in table 3-6, p. 78, does not mention D800..DFFF.
       - The unicode.org FAQ question "How do I convert an unpaired
         UTF-16 surrogate to UTF-8?" has the answer
           "By representing such an unpaired surrogate on its own
            as a 3-byte sequence, the resulting UTF-8 data stream
            would become ill-formed."
     So use U+FFFD instead.  */
  if_error (IF_SEVERITY_WARNING,
            logical_file_name, line_number, (size_t)(-1), false,
            _("lone surrogate U+%04X"), uc);
  string_desc_t fffd = /* U+FFFD in UTF-8 encoding.  */
    sd_new_addr (3, "\357\277\275");
  sbr_xprepend_desc (&buffer->sbr, fffd);
}

/* Auxiliary function: Flush buffer->utf16_surr into buffer->sbr.  */
static inline void
sbru_flush_utf16_surr (struct string_buffer_reversed_unicode *buffer)
{
  if (buffer->utf16_surr != 0)
    {
      sbru_prepend_lone_surrogate (buffer,
                                   buffer->utf16_surr,
                                   buffer->utf16_surr_line_number);
      buffer->utf16_surr = 0;
    }
}

/* Prepends the character C to BUFFER.  */
static void
sbru_xprepend1 (struct string_buffer_reversed_unicode *buffer, char c)
{
  sbru_flush_utf16_surr (buffer);
  sbr_xprepend1 (&buffer->sbr, c);
}

/* Prepends the contents of the memory area S to BUFFER.  */
static void
sbru_xprepend_desc (struct string_buffer_reversed_unicode *buffer,
                    string_desc_t s)
{
  sbru_flush_utf16_surr (buffer);
  sbr_xprepend_desc (&buffer->sbr, s);
}

/* Prepends a Unicode code point C to BUFFER.  */
static void
sbru_xprepend_unicode (struct string_buffer_reversed_unicode *buffer,
                       ucs4_t c, TSNode node)
{
  /* Test whether this character and the previous one form a Unicode
     surrogate character pair.  */
  if (buffer->utf16_surr != 0 && (c >= 0xd800 && c < 0xdc00))
    {
      unsigned short utf16buf[2];
      utf16buf[0] = c;
      utf16buf[1] = buffer->utf16_surr;

      ucs4_t uc;
      if (u16_mbtouc (&uc, utf16buf, 2) != 2)
        abort ();

      uint8_t buf[6];
      int n = u8_uctomb (buf, uc, sizeof (buf));
      if (!(n > 0))
        abort ();
      sbr_xprepend_desc (&buffer->sbr, sd_new_addr (n, (const char *) buf));

      buffer->utf16_surr = 0;
    }
  else
    {
      sbru_flush_utf16_surr (buffer);

      if (c >= 0xdc00 && c < 0xe000)
        {
          buffer->utf16_surr = c;
          buffer->utf16_surr_line_number = ts_node_line_number (node);
        }
      else if (c >= 0xd800 && c < 0xdc00)
        sbru_prepend_lone_surrogate (buffer, c, ts_node_line_number (node));
      else
        {
          uint8_t buf[6];
          int n = u8_uctomb (buf, c, sizeof (buf));
          if (!(n > 0))
            abort ();
          sbr_xprepend_desc (&buffer->sbr, sd_new_addr (n, (const char *) buf));
        }
    }
}

/* Returns the contents of BUFFER (with an added trailing NUL, that is,
   as a C string), and frees all other memory held by BUFFER.
   Returns NULL if there was an error earlier.
   It is the responsibility of the caller to free() the result.  */
static char *
sbru_xdupfree_c (struct string_buffer_reversed_unicode *buffer)
  _GL_ATTRIBUTE_MALLOC _GL_ATTRIBUTE_DEALLOC_FREE
  _GL_ATTRIBUTE_RETURNS_NONNULL
  _GL_ATTRIBUTE_RELEASE_CAPABILITY (buffer->sbr.data);
static char *
sbru_xdupfree_c (struct string_buffer_reversed_unicode *buffer)
{
  sbru_flush_utf16_surr (buffer);
  return sbr_xdupfree_c (&buffer->sbr);
}

/* ---------------------------- String literals ---------------------------- */

/* Determines whether NODE represents a string literal or the concatenation
   of string literals (via the '+' operator).  */
static bool
is_string_literal (TSNode node)
{
 start:
  if (ts_node_symbol (node) == ts_symbol_string
      || ts_node_symbol (node) == ts_symbol_template_string)
    {
      /* Test whether all named children nodes are of type 'string_fragment' or
         'escape_sequence' (and thus none of type 'template_substitution' or
         'ERROR').  */
      uint32_t count = ts_node_named_child_count (node);
      for (uint32_t i = 0; i < count; i++)
        {
          TSNode subnode = ts_node_named_child (node, i);
          if (!(ts_node_symbol (subnode) == ts_symbol_string_fragment
                || ts_node_symbol (subnode) == ts_symbol_escape_sequence))
            return false;
        }
      return true;
    }
  if (ts_node_symbol (node) == ts_symbol_binary_expression
      && ts_node_symbol (ts_node_child_by_field_id (node, ts_field_operator)) == ts_symbol_plus
      /* Recurse into the left and right subnodes.  */
      && is_string_literal (ts_node_child_by_field_id (node, ts_field_right)))
    {
      /*return is_string_literal (ts_node_child_by_field_id (node, ts_field_left));*/
      node = ts_node_child_by_field_id (node, ts_field_left);
      goto start;
    }
  return false;
}

/* Prepends the string literal pieces from NODE to BUFFER.  */
static void
string_literal_accumulate_pieces (TSNode node,
                                  struct string_buffer_reversed_unicode *buffer)
{
 start:
  if (ts_node_symbol (node) == ts_symbol_string
      || ts_node_symbol (node) == ts_symbol_template_string)
    {
      uint32_t count = ts_node_named_child_count (node);
      for (uint32_t i = count; i > 0; )
        {
          i--;
          TSNode subnode = ts_node_named_child (node, i);
          if (ts_node_symbol (subnode) == ts_symbol_string_fragment)
            {
              string_desc_t subnode_string =
                sd_new_addr (ts_node_end_byte (subnode) - ts_node_start_byte (subnode),
                             contents + ts_node_start_byte (subnode));
              sbru_xprepend_desc (buffer, subnode_string);
            }
          else if (ts_node_symbol (subnode) == ts_symbol_escape_sequence)
            {
              const char *escape_start = contents + ts_node_start_byte (subnode);
              const char *escape_end = contents + ts_node_end_byte (subnode);
              /* The escape sequence must start with a backslash.  */
              if (!(escape_end - escape_start >= 2 && escape_start[0] == '\\'))
                abort ();
              /* tree-sitter's grammar.js allows more escape sequences than the
                 tsc compiler.  Give a warning for those case where the tsc
                 compiler gives an error.  */
              bool invalid = false;
              if (escape_end - escape_start == 2)
                {
                  switch (escape_start[1])
                    {
                    case '\n':
                      break;
                    case '\\':
                    case '"':
                      sbru_xprepend1 (buffer, escape_start[1]);
                      break;
                    case 'b':
                      sbru_xprepend1 (buffer, 0x08);
                      break;
                    case 'f':
                      sbru_xprepend1 (buffer, 0x0C);
                      break;
                    case 'n':
                      sbru_xprepend1 (buffer, '\n');
                      break;
                    case 'r':
                      sbru_xprepend1 (buffer, '\r');
                      break;
                    case 't':
                      sbru_xprepend1 (buffer, '\t');
                      break;
                    case 'v':
                      sbru_xprepend1 (buffer, 0x0B);
                      break;
                    default:
                      invalid = true;
                      break;
                    }
                }
              else if (escape_end - escape_start == 3
                       && escape_start[1] == '\r' && escape_start[2] == '\n')
                /* Backslash-newline with a Windows CRLF.  */
                ;
              else if (escape_start[1] >= '0' && escape_start[1] <= '7')
                {
                  /* It's not clear whether octal escape sequences should be
                     supported.  On one hand, they are supported in JavaScript.
                     On the other hand, tsc says:
                     "error TS1487: Octal escape sequences are not allowed."  */
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
                    sbru_xprepend1 (buffer, (unsigned char) value);
                }
              else if ((escape_start[1] == 'x' && escape_end - escape_start == 2 + 2)
                       || (escape_start[1] == 'u' && escape_end - escape_start == 2 + 4))
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
                        sbru_xprepend1 (buffer, (unsigned char) value);
                    }
                  else
                    sbru_xprepend_unicode (buffer, value, subnode);
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
                    sbru_xprepend_unicode (buffer, value, subnode);
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
    }
  else if (ts_node_symbol (node) == ts_symbol_binary_expression
           && ts_node_symbol (ts_node_child_by_field_id (node, ts_field_operator)) == ts_symbol_plus)
    {
      /* Recurse into the left and right subnodes.  */
      string_literal_accumulate_pieces (ts_node_child_by_field_id (node, ts_field_right), buffer);
      /*string_literal_accumulate_pieces (ts_node_child_by_field_id (node, ts_field_left), buffer);*/
      node = ts_node_child_by_field_id (node, ts_field_left);
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
  if (ts_node_symbol (node) == ts_symbol_string
      && ts_node_named_child_count (node) == 1)
    {
      TSNode subnode = ts_node_named_child (node, 0);
      if (ts_node_symbol (subnode) == ts_symbol_string_fragment)
        {
          /* Optimize the frequent special case of a normal string literal
             that is non-empty and has no escape sequences.  */
          string_desc_t subnode_string =
            sd_new_addr (ts_node_end_byte (subnode) - ts_node_start_byte (subnode),
                         contents + ts_node_start_byte (subnode));
          return xsd_c (subnode_string);
        }
    }

  /* The general case.  */
  struct string_buffer_reversed_unicode buffer;
  sbru_init (&buffer);
  string_literal_accumulate_pieces (node, &buffer);
  return sbru_xdupfree_c (&buffer);
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
                               flag_region_ty *outer_region,
                               message_list_ty *mlp);

/* Extracts messages from the function call consisting of
     - CALLEE_NODE: a tree node of type 'identifier',
     - ARGS_NODE: a tree node of type 'arguments'.
   Extracted messages are added to MLP.  */
static void
extract_from_function_call (TSNode callee_node,
                            TSNode args_node,
                            flag_region_ty *outer_region,
                            message_list_ty *mlp)
{
  uint32_t args_count = ts_node_child_count (args_node);

  string_desc_t callee_name =
    sd_new_addr (ts_node_end_byte (callee_node) - ts_node_start_byte (callee_node),
                 contents + ts_node_start_byte (callee_node));

  /* Context iterator.  */
  flag_context_list_iterator_ty next_context_iter =
    flag_context_list_iterator (
      flag_context_list_table_lookup (
        flag_context_list_table,
        sd_data (callee_name), sd_length (callee_name)));

  void *keyword_value;
  if (hash_find_entry (&keywords,
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
              && ts_node_symbol (arg_node) != ts_symbol_comment)
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

  if (ts_node_symbol (node) == ts_symbol_call_expression
      && ts_node_named_child_count (node) >= 2)
    {
      TSNode callee_node = ts_node_named_child (node, 0);
      /* This is the field called 'function'.  */
      if (! ts_node_eq (ts_node_child_by_field_id (node, ts_field_function),
                        callee_node))
        abort ();
      if (ts_node_symbol (callee_node) == ts_symbol_identifier)
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

  #if DEBUG_TYPESCRIPT && 0
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
  #endif

  /* Recurse.  */
  if (ts_node_symbol (node) != ts_symbol_comment)
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
EXTRACT (FILE *f,
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
      ts_language = TREE_SITTER_LANGUAGE ();
      ts_symbol_comment           = ts_language_symbol ("comment", true);
      ts_symbol_string            = ts_language_symbol ("string", true);
      ts_symbol_string_fragment   = ts_language_symbol ("string_fragment", true);
      ts_symbol_escape_sequence   = ts_language_symbol ("escape_sequence", true);
      ts_symbol_template_string   = ts_language_symbol ("template_string", true);
      ts_symbol_binary_expression = ts_language_symbol ("binary_expression", true);
      ts_symbol_identifier        = ts_language_symbol ("identifier", true);
      ts_symbol_call_expression   = ts_language_symbol ("call_expression", true);
      ts_symbol_arguments         = ts_language_symbol ("arguments", true);
      ts_symbol_plus              = ts_language_symbol ("+", false);
      ts_field_function  = ts_language_field ("function");
      ts_field_arguments = ts_language_field ("arguments");
      ts_field_operator  = ts_language_field ("operator");
      ts_field_left      = ts_language_field ("left");
      ts_field_right     = ts_language_field ("right");
    }

  /* Read the file into memory.  */
  char *contents_data;
  size_t contents_length;
  contents_data = read_file (real_filename, 0, &contents_length);
  if (contents_data == NULL)
    error (EXIT_FAILURE, errno, _("error while reading \"%s\""),
           real_filename);

  /* tree-sitter works only on files whose size fits in an uint32_t.  */
  if (contents_length > 0xFFFFFFFFUL)
    error (EXIT_FAILURE, 0, _("file \"%s\" is unsupported because too large"),
           real_filename);

  /* TypeScript source files are usually UTF-8 encoded.  */
  if (u8_check ((uint8_t *) contents_data, contents_length) != NULL)
    error (EXIT_FAILURE, 0,
           _("file \"%s\" is unsupported because not UTF-8 encoded"),
           real_filename);
  xgettext_current_source_encoding = po_charset_utf8;

  /* Create a parser.  */
  TSParser *parser = ts_parser_new ();

  /* Set the parser's language.  */
  ts_parser_set_language (parser, ts_language);

  /* Parse the file, producing a syntax tree.  */
  TSTree *tree = ts_parser_parse_string (parser, NULL, contents_data, contents_length);

  #if DEBUG_TYPESCRIPT
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
