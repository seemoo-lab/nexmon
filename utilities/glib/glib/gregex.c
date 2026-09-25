/* GRegex -- regular expression API wrapper around PCRE.
 *
 * Copyright (C) 1999, 2000 Scott Wimer
 * Copyright (C) 2004, Matthias Clasen <mclasen@redhat.com>
 * Copyright (C) 2005 - 2007, Marco Barisione <marco@barisione.org>
 * Copyright (C) 2022, Marco Trevisan <marco.trevisan@canonical.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdint.h>
#include <string.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#include "gtypes.h"
#include "gregex.h"
#include "glibintl.h"
#include "glist.h"
#include "gmessages.h"
#include "gstrfuncs.h"
#include "gatomic.h"
#include "gtestutils.h"
#include "gthread.h"

/**
 * GRegex:
 *
 * A `GRegex` is a compiled form of a regular expression.
 * 
 * After instantiating a `GRegex`, you can use its methods to find matches
 * in a string, replace matches within a string, or split the string at matches.
 *
 * `GRegex` implements regular expression pattern matching using syntax and 
 * semantics (such as character classes, quantifiers, and capture groups) 
 * similar to Perl regular expression. See the 
 * [PCRE documentation](man:pcre2pattern(3)) for details.
 *
 * A typical scenario for regex pattern matching is to check if a string 
 * matches a pattern. The following statements implement this scenario.
 * 
 * ``` { .c }
 * const char *regex_pattern = ".*GLib.*";
 * const char *string_to_search = "You will love the GLib implementation of regex";
 * g_autoptr(GMatchInfo) match_info = NULL;
 * g_autoptr(GRegex) regex = NULL;
 *
 * regex = g_regex_new (regex_pattern, G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT, NULL);
 * g_assert (regex != NULL);
 * 
 * if (g_regex_match (regex, string_to_search, G_REGEX_MATCH_DEFAULT, &match_info))
 *   {
 *     int start_pos, end_pos;
 *     g_match_info_fetch_pos (match_info, 0, &start_pos, &end_pos);
 *     g_print ("Match successful! Overall pattern matches bytes %d to %d\n", start_pos, end_pos);
 *   }
 * else
 *   {
 *     g_print ("No match!\n");
 *   }
 * ```
 * 
 * The constructor for `GRegex` includes two sets of bitmapped flags:

 * * [flags@GLib.RegexCompileFlags]—These flags 
 * control how GLib compiles the regex. There are options for case 
 * sensitivity, multiline, ignoring whitespace, etc.
 * * [flags@GLib.RegexMatchFlags]—These flags control 
 * `GRegex`’s matching behavior, such as anchoring and customizing definitions 
 * for newline characters.
 * 
 * Some regex patterns include backslash assertions, such as `\d` (digit) or 
 * `\D` (non-digit). The regex pattern must escape those backslashes. For 
 * example, the pattern `"\\d\\D"` matches a digit followed by a non-digit.
 *
 * GLib’s implementation of pattern matching includes a `start_position` 
 * argument for some of the match, replace, and split methods. Specifying 
 * a start position provides flexibility when you want to ignore the first 
 * _n_ characters of a string, but want to incorporate backslash assertions 
 * at character _n_ - 1. For example, a database field contains inconsistent
 * spelling for a job title: `healthcare provider` and `health-care provider`.
 * The database manager wants to make the spelling consistent by adding a 
 * hyphen when it is missing. The following regex pattern tests for the string 
 * `care` preceded by a non-word boundary character (instead of a hyphen) 
 * and followed by a space.
 *
 * ``` { .c }
 * const char *regex_pattern = "\\Bcare\\s";
 * ```
 *
 * An efficient way to match with this pattern is to start examining at 
 * `start_position` 6 in the string `healthcare` or `health-care`.

 * ``` { .c }
 * const char *regex_pattern = "\\Bcare\\s";
 * const char *string_to_search = "healthcare provider";
 * g_autoptr(GMatchInfo) match_info = NULL;
 * g_autoptr(GRegex) regex = NULL;
 *
 * regex = g_regex_new (
 *   regex_pattern,
 *   G_REGEX_DEFAULT,
 *   G_REGEX_MATCH_DEFAULT,
 *   NULL);
 * g_assert (regex != NULL);
 * 
 * g_regex_match_full (
 *   regex, 
 *   string_to_search, 
 *   -1,
 *   6, // position of 'c' in the test string.
 *   G_REGEX_MATCH_DEFAULT, 
 *   &match_info,
 *   NULL);
 * ```
 * 
 * The method [method@GLib.Regex.match_full] (and other methods implementing 
 * `start_pos`) allow for lookback before the start position to determine if 
 * the previous character satisfies an assertion.
 *
 * Unless you set the [flags@GLib.RegexCompileFlags.RAW] as one of 
 * the `GRegexCompileFlags`, all the strings passed to `GRegex` methods must 
 * be encoded in UTF-8. The lengths and the positions inside the strings are 
 * in bytes and not in characters, so, for instance, `\xc3\xa0` (i.e., `à`) 
 * is two bytes long but it is treated as a single character. If you set 
 * `G_REGEX_RAW`, the strings can be non-valid UTF-8 strings and a byte is 
 * treated as a character, so `\xc3\xa0` is two bytes and two characters long.
 *
 * Regarding line endings, `\n` matches a `\n` character, and `\r` matches 
 * a `\r` character. More generally, `\R` matches all typical line endings: 
 * CR + LF (`\r\n`), LF (linefeed, U+000A, `\n`), VT (vertical tab, U+000B, 
 * `\v`), FF (formfeed, U+000C, `\f`), CR (carriage return, U+000D, `\r`), 
 * NEL (next line, U+0085), LS (line separator, U+2028), and PS (paragraph 
 * separator, U+2029).
 * 
 * The behaviour of the dot, circumflex, and dollar metacharacters are 
 * affected by newline characters. By default, `GRegex` matches any newline 
 * character matched by `\R`. You can limit the matched newline characters by 
 * specifying the [flags@GLib.RegexMatchFlags.NEWLINE_CR], 
 * [flags@GLib.RegexMatchFlags.NEWLINE_LF], and 
 * [flags@GLib.RegexMatchFlags.NEWLINE_CRLF] compile options, and 
 * with [flags@GLib.RegexMatchFlags.NEWLINE_ANY], 
 * [flags@GLib.RegexMatchFlags.NEWLINE_CR], 
 * [flags@GLib.RegexMatchFlags.NEWLINE_LF] and 
 * [flags@GLib.RegexMatchFlags.NEWLINE_CRLF] match options. 
 * These settings are also relevant when compiling a pattern if 
 * [flags@GLib.RegexCompileFlags.EXTENDED] is set and an unescaped 
 * `#` outside a character class is encountered. This indicates a comment 
 * that lasts until after the next newline.
 * 
 * Because `GRegex` does not modify its internal state between creation and 
 * destruction, you can create and modify the same `GRegex` instance from 
 * different threads. In contrast, [struct@GLib.MatchInfo] is not thread safe.
 * 
 * The regular expression low-level functionalities are obtained through
 * the excellent [PCRE](http://www.pcre.org/) library written by Philip Hazel.
 *
 * Since: 2.14
 */

#define G_REGEX_PCRE_GENERIC_MASK (PCRE2_ANCHORED       | \
                                   PCRE2_NO_UTF_CHECK   | \
                                   PCRE2_ENDANCHORED)

/* Mask of all the possible values for GRegexCompileFlags. */
#define G_REGEX_COMPILE_MASK (G_REGEX_DEFAULT          | \
                              G_REGEX_CASELESS         | \
                              G_REGEX_MULTILINE        | \
                              G_REGEX_DOTALL           | \
                              G_REGEX_EXTENDED         | \
                              G_REGEX_ANCHORED         | \
                              G_REGEX_DOLLAR_ENDONLY   | \
                              G_REGEX_UNGREEDY         | \
                              G_REGEX_RAW              | \
                              G_REGEX_NO_AUTO_CAPTURE  | \
                              G_REGEX_OPTIMIZE         | \
                              G_REGEX_FIRSTLINE        | \
                              G_REGEX_DUPNAMES         | \
                              G_REGEX_NEWLINE_CR       | \
                              G_REGEX_NEWLINE_LF       | \
                              G_REGEX_NEWLINE_CRLF     | \
                              G_REGEX_NEWLINE_ANYCRLF  | \
                              G_REGEX_BSR_ANYCRLF)

#define G_REGEX_PCRE2_COMPILE_MASK (PCRE2_ALLOW_EMPTY_CLASS    | \
                                    PCRE2_ALT_BSUX             | \
                                    PCRE2_AUTO_CALLOUT         | \
                                    PCRE2_CASELESS             | \
                                    PCRE2_DOLLAR_ENDONLY       | \
                                    PCRE2_DOTALL               | \
                                    PCRE2_DUPNAMES             | \
                                    PCRE2_EXTENDED             | \
                                    PCRE2_FIRSTLINE            | \
                                    PCRE2_MATCH_UNSET_BACKREF  | \
                                    PCRE2_MULTILINE            | \
                                    PCRE2_NEVER_UCP            | \
                                    PCRE2_NEVER_UTF            | \
                                    PCRE2_NO_AUTO_CAPTURE      | \
                                    PCRE2_NO_AUTO_POSSESS      | \
                                    PCRE2_NO_DOTSTAR_ANCHOR    | \
                                    PCRE2_NO_START_OPTIMIZE    | \
                                    PCRE2_UCP                  | \
                                    PCRE2_UNGREEDY             | \
                                    PCRE2_UTF                  | \
                                    PCRE2_NEVER_BACKSLASH_C    | \
                                    PCRE2_ALT_CIRCUMFLEX       | \
                                    PCRE2_ALT_VERBNAMES        | \
                                    PCRE2_USE_OFFSET_LIMIT     | \
                                    PCRE2_EXTENDED_MORE        | \
                                    PCRE2_LITERAL              | \
                                    PCRE2_MATCH_INVALID_UTF    | \
                                    G_REGEX_PCRE_GENERIC_MASK)

#define G_REGEX_COMPILE_NONPCRE_MASK (PCRE2_UTF)

/* Mask of all the possible values for GRegexMatchFlags. */
#define G_REGEX_MATCH_MASK (G_REGEX_MATCH_DEFAULT          | \
                            G_REGEX_MATCH_ANCHORED         | \
                            G_REGEX_MATCH_NOTBOL           | \
                            G_REGEX_MATCH_NOTEOL           | \
                            G_REGEX_MATCH_NOTEMPTY         | \
                            G_REGEX_MATCH_PARTIAL          | \
                            G_REGEX_MATCH_NEWLINE_CR       | \
                            G_REGEX_MATCH_NEWLINE_LF       | \
                            G_REGEX_MATCH_NEWLINE_CRLF     | \
                            G_REGEX_MATCH_NEWLINE_ANY      | \
                            G_REGEX_MATCH_NEWLINE_ANYCRLF  | \
                            G_REGEX_MATCH_BSR_ANYCRLF      | \
                            G_REGEX_MATCH_BSR_ANY          | \
                            G_REGEX_MATCH_PARTIAL_SOFT     | \
                            G_REGEX_MATCH_PARTIAL_HARD     | \
                            G_REGEX_MATCH_NOTEMPTY_ATSTART)

#define G_REGEX_PCRE2_MATCH_MASK (PCRE2_NOTBOL                      |\
                                  PCRE2_NOTEOL                      |\
                                  PCRE2_NOTEMPTY                    |\
                                  PCRE2_NOTEMPTY_ATSTART            |\
                                  PCRE2_PARTIAL_SOFT                |\
                                  PCRE2_PARTIAL_HARD                |\
                                  PCRE2_NO_JIT                      |\
                                  PCRE2_COPY_MATCHED_SUBJECT        |\
                                  G_REGEX_PCRE_GENERIC_MASK)

/* TODO: Support PCRE2_NEWLINE_NUL */
#define G_REGEX_NEWLINE_MASK (PCRE2_NEWLINE_CR |     \
                              PCRE2_NEWLINE_LF |     \
                              PCRE2_NEWLINE_CRLF |   \
                              PCRE2_NEWLINE_ANYCRLF)

/* Some match options are not supported when using JIT as stated in the
 * pcre2jit man page under the «UNSUPPORTED OPTIONS AND PATTERN ITEMS» section:
 *   https://www.pcre.org/current/doc/html/pcre2jit.html#SEC5
 */
#define G_REGEX_PCRE2_JIT_UNSUPPORTED_OPTIONS (PCRE2_ANCHORED | \
                                               PCRE2_ENDANCHORED)

#define G_REGEX_COMPILE_NEWLINE_MASK (G_REGEX_NEWLINE_CR      | \
                                      G_REGEX_NEWLINE_LF      | \
                                      G_REGEX_NEWLINE_CRLF    | \
                                      G_REGEX_NEWLINE_ANYCRLF)

#define G_REGEX_MATCH_NEWLINE_MASK (G_REGEX_MATCH_NEWLINE_CR      | \
                                    G_REGEX_MATCH_NEWLINE_LF      | \
                                    G_REGEX_MATCH_NEWLINE_CRLF    | \
                                    G_REGEX_MATCH_NEWLINE_ANY    | \
                                    G_REGEX_MATCH_NEWLINE_ANYCRLF)

/* if the string is in UTF-8 use g_utf8_ functions, else use
 * use just +/- 1. */
#define NEXT_CHAR(re, s) (((re)->regex_compile_opts & G_REGEX_RAW) ? \
                                ((s) + 1) : \
                                g_utf8_next_char (s))
#define PREV_CHAR(re, s) (((re)->regex_compile_opts & G_REGEX_RAW) ? \
                                ((s) - 1) : \
                                g_utf8_prev_char (s))

struct _GMatchInfo
{
  gint ref_count;               /* the ref count (atomic) */
  GRegex *regex;                /* the regex */
  uint32_t match_opts;          /* pcre match options used at match time on the regex */
  gint matches;                 /* number of matching sub patterns, guaranteed to be <= (n_subpatterns + 1) if doing a single match (rather than matching all) */
  uint32_t n_subpatterns;       /* total number of sub patterns in the regex */
  size_t pos;                   /* position in the string where last match left off; check @pos_valid before using */
  gboolean pos_valid;           /* whether @pos is valid; will be false when reaching the end of the string */
  size_t n_offsets;             /* number of offsets */
  gint *offsets;                /* array of offsets paired 0,1 ; 2,3 ; 3,4 etc */
  gint *workspace;              /* workspace for pcre2_dfa_match() */
  PCRE2_SIZE n_workspace;       /* number of workspace elements */
  const gchar *string;          /* string passed to the match function */
  size_t string_len;            /* length of string, in bytes */
  pcre2_match_context *match_context;
  pcre2_match_data *match_data;
  pcre2_jit_stack *jit_stack;
};

typedef enum
{
  JIT_STATUS_DEFAULT,
  JIT_STATUS_ENABLED,
  JIT_STATUS_DISABLED
} JITStatus;

struct _GRegex
{
  gint ref_count;               /* the ref count for the immutable part (atomic) */
  gchar *pattern;               /* the pattern */
  pcre2_code *pcre_re;          /* compiled form of the pattern */
  uint32_t pcre2_compile_opts;  /* options used at compile time on the pattern, pcre2 values */
  GRegexCompileFlags regex_compile_opts; /* options used at compile time on the pattern, gregex values */
  uint32_t match_opts;          /* pcre2 options used at match time on the regex */
  GRegexMatchFlags orig_match_opts; /* options used as default match options, gregex values */
  uint32_t jit_options;         /* options which were enabled for jit compiler */
  JITStatus jit_status;         /* indicates the status of jit compiler for this compiled regex */
  /* The jit_status here does _not_ correspond to whether we used the JIT in the last invocation,
   * which may be affected by match_options or a JIT_STACK_LIMIT error, but whether it was ever
   * enabled for the current regex AND current set of jit_options.
   * JIT_STATUS_DEFAULT means enablement was never tried,
   * JIT_STATUS_ENABLED means it was tried and successful (even if we're not currently using it),
   * and JIT_STATUS_DISABLED means it was tried and failed (so we shouldn't try again).
   */
};

/* TRUE if ret is an error code, FALSE otherwise. */
#define IS_PCRE2_ERROR(ret) ((ret) < PCRE2_ERROR_NOMATCH && (ret) != PCRE2_ERROR_PARTIAL)

typedef struct _InterpolationData InterpolationData;
static gboolean  interpolation_list_needs_match (GList *list);
static gboolean  interpolate_replacement        (const GMatchInfo *match_info,
                                                 GString *result,
                                                 gpointer data);
static GList    *split_replacement              (const gchar *replacement,
                                                 GError **error);
static void      free_interpolation_data        (InterpolationData *data);

static uint32_t
get_pcre2_compile_options (GRegexCompileFlags compile_flags)
{
  /* Maps compile flags to pcre2 values */
  uint32_t pcre2_flags = 0;

  if (compile_flags & G_REGEX_CASELESS)
    pcre2_flags |= PCRE2_CASELESS;
  if (compile_flags & G_REGEX_MULTILINE)
    pcre2_flags |= PCRE2_MULTILINE;
  if (compile_flags & G_REGEX_DOTALL)
    pcre2_flags |= PCRE2_DOTALL;
  if (compile_flags & G_REGEX_EXTENDED)
    pcre2_flags |= PCRE2_EXTENDED;
  if (compile_flags & G_REGEX_ANCHORED)
    pcre2_flags |= PCRE2_ANCHORED;
  if (compile_flags & G_REGEX_DOLLAR_ENDONLY)
    pcre2_flags |= PCRE2_DOLLAR_ENDONLY;
  if (compile_flags & G_REGEX_UNGREEDY)
    pcre2_flags |= PCRE2_UNGREEDY;
  if (!(compile_flags & G_REGEX_RAW))
    pcre2_flags |= PCRE2_UTF;
  if (compile_flags & G_REGEX_NO_AUTO_CAPTURE)
    pcre2_flags |= PCRE2_NO_AUTO_CAPTURE;
  if (compile_flags & G_REGEX_FIRSTLINE)
    pcre2_flags |= PCRE2_FIRSTLINE;
  if (compile_flags & G_REGEX_DUPNAMES)
    pcre2_flags |= PCRE2_DUPNAMES;

  return pcre2_flags & G_REGEX_PCRE2_COMPILE_MASK;
}

static uint32_t
get_pcre2_match_options (GRegexMatchFlags   match_flags,
                         GRegexCompileFlags compile_flags)
{
  /* Maps match flags to pcre2 values */
  uint32_t pcre2_flags = 0;

  if (match_flags & G_REGEX_MATCH_ANCHORED)
    pcre2_flags |= PCRE2_ANCHORED;
  if (match_flags & G_REGEX_MATCH_NOTBOL)
    pcre2_flags |= PCRE2_NOTBOL;
  if (match_flags & G_REGEX_MATCH_NOTEOL)
    pcre2_flags |= PCRE2_NOTEOL;
  if (match_flags & G_REGEX_MATCH_NOTEMPTY)
    pcre2_flags |= PCRE2_NOTEMPTY;
  if (match_flags & G_REGEX_MATCH_PARTIAL_SOFT)
    pcre2_flags |= PCRE2_PARTIAL_SOFT;
  if (match_flags & G_REGEX_MATCH_PARTIAL_HARD)
    pcre2_flags |= PCRE2_PARTIAL_HARD;
  if (match_flags & G_REGEX_MATCH_NOTEMPTY_ATSTART)
    pcre2_flags |= PCRE2_NOTEMPTY_ATSTART;

  if (compile_flags & G_REGEX_RAW)
    pcre2_flags |= PCRE2_NO_UTF_CHECK;

  return pcre2_flags & G_REGEX_PCRE2_MATCH_MASK;
}

static GRegexCompileFlags
g_regex_compile_flags_from_pcre2 (uint32_t pcre2_flags)
{
  GRegexCompileFlags compile_flags = G_REGEX_DEFAULT;

  if (pcre2_flags & PCRE2_CASELESS)
    compile_flags |= G_REGEX_CASELESS;
  if (pcre2_flags & PCRE2_MULTILINE)
    compile_flags |= G_REGEX_MULTILINE;
  if (pcre2_flags & PCRE2_DOTALL)
    compile_flags |= G_REGEX_DOTALL;
  if (pcre2_flags & PCRE2_EXTENDED)
    compile_flags |= G_REGEX_EXTENDED;
  if (pcre2_flags & PCRE2_ANCHORED)
    compile_flags |= G_REGEX_ANCHORED;
  if (pcre2_flags & PCRE2_DOLLAR_ENDONLY)
    compile_flags |= G_REGEX_DOLLAR_ENDONLY;
  if (pcre2_flags & PCRE2_UNGREEDY)
    compile_flags |= G_REGEX_UNGREEDY;
  if (!(pcre2_flags & PCRE2_UTF))
    compile_flags |= G_REGEX_RAW;
  if (pcre2_flags & PCRE2_NO_AUTO_CAPTURE)
    compile_flags |= G_REGEX_NO_AUTO_CAPTURE;
  if (pcre2_flags & PCRE2_FIRSTLINE)
    compile_flags |= G_REGEX_FIRSTLINE;
  if (pcre2_flags & PCRE2_DUPNAMES)
    compile_flags |= G_REGEX_DUPNAMES;

  return compile_flags & G_REGEX_COMPILE_MASK;
}

static GRegexMatchFlags
g_regex_match_flags_from_pcre2 (uint32_t pcre2_flags)
{
  GRegexMatchFlags match_flags = G_REGEX_MATCH_DEFAULT;

  if (pcre2_flags & PCRE2_ANCHORED)
    match_flags |= G_REGEX_MATCH_ANCHORED;
  if (pcre2_flags & PCRE2_NOTBOL)
    match_flags |= G_REGEX_MATCH_NOTBOL;
  if (pcre2_flags & PCRE2_NOTEOL)
    match_flags |= G_REGEX_MATCH_NOTEOL;
  if (pcre2_flags & PCRE2_NOTEMPTY)
    match_flags |= G_REGEX_MATCH_NOTEMPTY;
  if (pcre2_flags & PCRE2_PARTIAL_SOFT)
    match_flags |= G_REGEX_MATCH_PARTIAL_SOFT;
  if (pcre2_flags & PCRE2_PARTIAL_HARD)
    match_flags |= G_REGEX_MATCH_PARTIAL_HARD;
  if (pcre2_flags & PCRE2_NOTEMPTY_ATSTART)
    match_flags |= G_REGEX_MATCH_NOTEMPTY_ATSTART;

  return (match_flags & G_REGEX_MATCH_MASK);
}

static uint32_t
get_pcre2_newline_compile_options (GRegexCompileFlags compile_flags)
{
  compile_flags &= G_REGEX_COMPILE_NEWLINE_MASK;

  switch (compile_flags)
    {
    case G_REGEX_NEWLINE_CR:
      return PCRE2_NEWLINE_CR;
    case G_REGEX_NEWLINE_LF:
      return PCRE2_NEWLINE_LF;
    case G_REGEX_NEWLINE_CRLF:
      return PCRE2_NEWLINE_CRLF;
    case G_REGEX_NEWLINE_ANYCRLF:
      return PCRE2_NEWLINE_ANYCRLF;
    default:
      if (compile_flags != 0)
        return 0;

      return PCRE2_NEWLINE_ANY;
    }
}

static uint32_t
get_pcre2_newline_match_options (GRegexMatchFlags match_flags)
{
  switch (match_flags & G_REGEX_MATCH_NEWLINE_MASK)
    {
    case G_REGEX_MATCH_NEWLINE_CR:
      return PCRE2_NEWLINE_CR;
    case G_REGEX_MATCH_NEWLINE_LF:
      return PCRE2_NEWLINE_LF;
    case G_REGEX_MATCH_NEWLINE_CRLF:
      return PCRE2_NEWLINE_CRLF;
    case G_REGEX_MATCH_NEWLINE_ANY:
      return PCRE2_NEWLINE_ANY;
    case G_REGEX_MATCH_NEWLINE_ANYCRLF:
      return PCRE2_NEWLINE_ANYCRLF;
    default:
      return 0;
    }
}

static uint32_t
get_pcre2_bsr_compile_options (GRegexCompileFlags compile_flags)
{
  if (compile_flags & G_REGEX_BSR_ANYCRLF)
    return PCRE2_BSR_ANYCRLF;

  return PCRE2_BSR_UNICODE;
}

static uint32_t
get_pcre2_bsr_match_options (GRegexMatchFlags match_flags)
{
  if (match_flags & G_REGEX_MATCH_BSR_ANYCRLF)
    return PCRE2_BSR_ANYCRLF;

  if (match_flags & G_REGEX_MATCH_BSR_ANY)
    return PCRE2_BSR_UNICODE;

  return 0;
}

static char *
get_pcre2_error_string (int errcode)
{
  PCRE2_UCHAR8 error_msg[2048];
  int err_length;

  err_length = pcre2_get_error_message (errcode, error_msg,
                                        G_N_ELEMENTS (error_msg));

  if (err_length <= 0)
    return NULL;

  /* The array is always filled with a trailing zero */
  g_assert ((size_t) err_length < G_N_ELEMENTS (error_msg));
  return g_memdup2 (error_msg, err_length + 1);
}

static const gchar *
translate_match_error (gint errcode)
{
  switch (errcode)
    {
    case PCRE2_ERROR_NOMATCH:
      /* not an error */
      break;
    case PCRE2_ERROR_NULL:
      /* NULL argument, this should not happen in GRegex */
      g_critical ("A NULL argument was passed to PCRE");
      break;
    case PCRE2_ERROR_BADOPTION:
      return "bad options";
    case PCRE2_ERROR_BADMAGIC:
      return _("corrupted object");
    case PCRE2_ERROR_NOMEMORY:
      return _("out of memory");
    case PCRE2_ERROR_NOSUBSTRING:
      /* not used by pcre2_match() */
      break;
    case PCRE2_ERROR_MATCHLIMIT:
    case PCRE2_ERROR_CALLOUT:
      /* callouts are not implemented */
      break;
    case PCRE2_ERROR_BADUTFOFFSET:
      /* we do not check if strings are valid */
      break;
    case PCRE2_ERROR_PARTIAL:
      /* not an error */
      break;
    case PCRE2_ERROR_INTERNAL:
      return _("internal error");
    case PCRE2_ERROR_DFA_UITEM:
      return _("the pattern contains items not supported for partial matching");
    case PCRE2_ERROR_DFA_UCOND:
      return _("back references as conditions are not supported for partial matching");
    case PCRE2_ERROR_DFA_WSSIZE:
      /* handled expanding the workspace */
      break;
    case PCRE2_ERROR_DFA_RECURSE:
    case PCRE2_ERROR_RECURSIONLIMIT:
      return _("recursion limit reached");
    case PCRE2_ERROR_BADOFFSET:
      return _("bad offset");
    case PCRE2_ERROR_RECURSELOOP:
      return _("recursion loop");
    case PCRE2_ERROR_JIT_BADOPTION:
      /* should not happen in GRegex since we check modes before each match */
      return _("matching mode is requested that was not compiled for JIT");
    default:
      break;
    }
  return NULL;
}

static char *
get_match_error_message (int errcode)
{
  const char *msg = translate_match_error (errcode);
  char *error_string;

  if (msg)
    return g_strdup (msg);

  error_string = get_pcre2_error_string (errcode);

  if (error_string)
    return error_string;

  return g_strdup (_("unknown error"));
}

static void
translate_compile_error (gint *errcode, const gchar **errmsg)
{
  /* If errcode is known we put the translatable error message in
   * errmsg. If errcode is unknown we put the generic
   * G_REGEX_ERROR_COMPILE error code in errcode.
   * Note that there can be more PCRE errors with the same GRegexError
   * and that some PCRE errors are useless for us.
   */
  gint original_errcode = *errcode;

  *errcode = -1;
  *errmsg = NULL;

  switch (original_errcode)
    {
    case PCRE2_ERROR_END_BACKSLASH:
      *errcode = G_REGEX_ERROR_STRAY_BACKSLASH;
      *errmsg = _("\\ at end of pattern");
      break;
    case PCRE2_ERROR_END_BACKSLASH_C:
      *errcode = G_REGEX_ERROR_MISSING_CONTROL_CHAR;
      *errmsg = _("\\c at end of pattern");
      break;
    case PCRE2_ERROR_UNKNOWN_ESCAPE:
    case PCRE2_ERROR_UNSUPPORTED_ESCAPE_SEQUENCE:
      *errcode = G_REGEX_ERROR_UNRECOGNIZED_ESCAPE;
      *errmsg = _("unrecognized character following \\");
      break;
    case PCRE2_ERROR_QUANTIFIER_OUT_OF_ORDER:
      *errcode = G_REGEX_ERROR_QUANTIFIERS_OUT_OF_ORDER;
      *errmsg = _("numbers out of order in {} quantifier");
      break;
    case PCRE2_ERROR_QUANTIFIER_TOO_BIG:
      *errcode = G_REGEX_ERROR_QUANTIFIER_TOO_BIG;
      *errmsg = _("number too big in {} quantifier");
      break;
    case PCRE2_ERROR_MISSING_SQUARE_BRACKET:
      *errcode = G_REGEX_ERROR_UNTERMINATED_CHARACTER_CLASS;
      *errmsg = _("missing terminating ] for character class");
      break;
    case PCRE2_ERROR_ESCAPE_INVALID_IN_CLASS:
      *errcode = G_REGEX_ERROR_INVALID_ESCAPE_IN_CHARACTER_CLASS;
      *errmsg = _("invalid escape sequence in character class");
      break;
    case PCRE2_ERROR_CLASS_RANGE_ORDER:
      *errcode = G_REGEX_ERROR_RANGE_OUT_OF_ORDER;
      *errmsg = _("range out of order in character class");
      break;
    case PCRE2_ERROR_QUANTIFIER_INVALID:
    case PCRE2_ERROR_INTERNAL_UNEXPECTED_REPEAT:
      *errcode = G_REGEX_ERROR_NOTHING_TO_REPEAT;
      *errmsg = _("nothing to repeat");
      break;
    case PCRE2_ERROR_INVALID_AFTER_PARENS_QUERY:
      *errcode = G_REGEX_ERROR_UNRECOGNIZED_CHARACTER;
      *errmsg = _("unrecognized character after (? or (?-");
      break;
    case PCRE2_ERROR_POSIX_CLASS_NOT_IN_CLASS:
      *errcode = G_REGEX_ERROR_POSIX_NAMED_CLASS_OUTSIDE_CLASS;
      *errmsg = _("POSIX named classes are supported only within a class");
      break;
    case PCRE2_ERROR_POSIX_NO_SUPPORT_COLLATING:
      *errcode = G_REGEX_ERROR_POSIX_COLLATING_ELEMENTS_NOT_SUPPORTED;
      *errmsg = _("POSIX collating elements are not supported");
      break;
    case PCRE2_ERROR_MISSING_CLOSING_PARENTHESIS:
    case PCRE2_ERROR_UNMATCHED_CLOSING_PARENTHESIS:
    case PCRE2_ERROR_PARENS_QUERY_R_MISSING_CLOSING:
      *errcode = G_REGEX_ERROR_UNMATCHED_PARENTHESIS;
      *errmsg = _("missing terminating )");
      break;
    case PCRE2_ERROR_BAD_SUBPATTERN_REFERENCE:
      *errcode = G_REGEX_ERROR_INEXISTENT_SUBPATTERN_REFERENCE;
      *errmsg = _("reference to non-existent subpattern");
      break;
    case PCRE2_ERROR_MISSING_COMMENT_CLOSING:
      *errcode = G_REGEX_ERROR_UNTERMINATED_COMMENT;
      *errmsg = _("missing ) after comment");
      break;
    case PCRE2_ERROR_PATTERN_TOO_LARGE:
      *errcode = G_REGEX_ERROR_EXPRESSION_TOO_LARGE;
      *errmsg = _("regular expression is too large");
      break;
    case PCRE2_ERROR_MISSING_CONDITION_CLOSING:
      *errcode = G_REGEX_ERROR_MALFORMED_CONDITION;
      *errmsg = _("malformed number or name after (?(");
      break;
    case PCRE2_ERROR_LOOKBEHIND_NOT_FIXED_LENGTH:
      *errcode = G_REGEX_ERROR_VARIABLE_LENGTH_LOOKBEHIND;
      *errmsg = _("lookbehind assertion is not fixed length");
      break;
    case PCRE2_ERROR_TOO_MANY_CONDITION_BRANCHES:
      *errcode = G_REGEX_ERROR_TOO_MANY_CONDITIONAL_BRANCHES;
      *errmsg = _("conditional group contains more than two branches");
      break;
    case PCRE2_ERROR_CONDITION_ASSERTION_EXPECTED:
      *errcode = G_REGEX_ERROR_ASSERTION_EXPECTED;
      *errmsg = _("assertion expected after (?(");
      break;
    case PCRE2_ERROR_BAD_RELATIVE_REFERENCE:
      *errcode = G_REGEX_ERROR_INVALID_RELATIVE_REFERENCE;
      *errmsg = _("a numbered reference must not be zero");
      break;
    case PCRE2_ERROR_UNKNOWN_POSIX_CLASS:
      *errcode = G_REGEX_ERROR_UNKNOWN_POSIX_CLASS_NAME;
      *errmsg = _("unknown POSIX class name");
      break;
    case PCRE2_ERROR_CODE_POINT_TOO_BIG:
    case PCRE2_ERROR_INVALID_HEXADECIMAL:
      *errcode = G_REGEX_ERROR_HEX_CODE_TOO_LARGE;
      *errmsg = _("character value in \\x{...} sequence is too large");
      break;
    case PCRE2_ERROR_LOOKBEHIND_INVALID_BACKSLASH_C:
      *errcode = G_REGEX_ERROR_SINGLE_BYTE_MATCH_IN_LOOKBEHIND;
      *errmsg = _("\\C not allowed in lookbehind assertion");
      break;
    case PCRE2_ERROR_MISSING_NAME_TERMINATOR:
      *errcode = G_REGEX_ERROR_MISSING_SUBPATTERN_NAME_TERMINATOR;
      *errmsg = _("missing terminator in subpattern name");
      break;
    case PCRE2_ERROR_DUPLICATE_SUBPATTERN_NAME:
      *errcode = G_REGEX_ERROR_DUPLICATE_SUBPATTERN_NAME;
      *errmsg = _("two named subpatterns have the same name");
      break;
    case PCRE2_ERROR_MALFORMED_UNICODE_PROPERTY:
      *errcode = G_REGEX_ERROR_MALFORMED_PROPERTY;
      *errmsg = _("malformed \\P or \\p sequence");
      break;
    case PCRE2_ERROR_UNKNOWN_UNICODE_PROPERTY:
      *errcode = G_REGEX_ERROR_UNKNOWN_PROPERTY;
      *errmsg = _("unknown property name after \\P or \\p");
      break;
    case PCRE2_ERROR_SUBPATTERN_NAME_TOO_LONG:
      *errcode = G_REGEX_ERROR_SUBPATTERN_NAME_TOO_LONG;
      *errmsg = _("subpattern name is too long (maximum 32 characters)");
      break;
    case PCRE2_ERROR_TOO_MANY_NAMED_SUBPATTERNS:
      *errcode = G_REGEX_ERROR_TOO_MANY_SUBPATTERNS;
      *errmsg = _("too many named subpatterns (maximum 10,000)");
      break;
    case PCRE2_ERROR_OCTAL_BYTE_TOO_BIG:
      *errcode = G_REGEX_ERROR_INVALID_OCTAL_VALUE;
      *errmsg = _("octal value is greater than \\377");
      break;
    case PCRE2_ERROR_DEFINE_TOO_MANY_BRANCHES:
      *errcode = G_REGEX_ERROR_TOO_MANY_BRANCHES_IN_DEFINE;
      *errmsg = _("DEFINE group contains more than one branch");
      break;
    case PCRE2_ERROR_INTERNAL_UNKNOWN_NEWLINE:
      *errcode = G_REGEX_ERROR_INCONSISTENT_NEWLINE_OPTIONS;
      *errmsg = _("inconsistent NEWLINE options");
      break;
    case PCRE2_ERROR_BACKSLASH_G_SYNTAX:
      *errcode = G_REGEX_ERROR_MISSING_BACK_REFERENCE;
      *errmsg = _("\\g is not followed by a braced, angle-bracketed, or quoted name or "
                  "number, or by a plain number");
      break;
#ifdef PCRE2_ERROR_MISSING_NUMBER_TERMINATOR
    case PCRE2_ERROR_MISSING_NUMBER_TERMINATOR:
      *errcode = G_REGEX_ERROR_MISSING_BACK_REFERENCE;
      *errmsg = _("syntax error in subpattern number (missing terminator?)");
      break;
#endif
    case PCRE2_ERROR_VERB_ARGUMENT_NOT_ALLOWED:
      *errcode = G_REGEX_ERROR_BACKTRACKING_CONTROL_VERB_ARGUMENT_FORBIDDEN;
      *errmsg = _("an argument is not allowed for (*ACCEPT), (*FAIL), or (*COMMIT)");
      break;
    case PCRE2_ERROR_VERB_UNKNOWN:
      *errcode = G_REGEX_ERROR_UNKNOWN_BACKTRACKING_CONTROL_VERB;
      *errmsg = _("(*VERB) not recognized");
      break;
    case PCRE2_ERROR_SUBPATTERN_NUMBER_TOO_BIG:
      *errcode = G_REGEX_ERROR_NUMBER_TOO_BIG;
      *errmsg = _("number is too big");
      break;
    case PCRE2_ERROR_SUBPATTERN_NAME_EXPECTED:
      *errcode = G_REGEX_ERROR_MISSING_SUBPATTERN_NAME;
      *errmsg = _("missing subpattern name after (?&");
      break;
    case PCRE2_ERROR_SUBPATTERN_NAMES_MISMATCH:
      *errcode = G_REGEX_ERROR_EXTRA_SUBPATTERN_NAME;
      *errmsg = _("different names for subpatterns of the same number are not allowed");
      break;
    case PCRE2_ERROR_MARK_MISSING_ARGUMENT:
      *errcode = G_REGEX_ERROR_BACKTRACKING_CONTROL_VERB_ARGUMENT_REQUIRED;
      *errmsg = _("(*MARK) must have an argument");
      break;
    case PCRE2_ERROR_BACKSLASH_C_SYNTAX:
      *errcode = G_REGEX_ERROR_INVALID_CONTROL_CHAR;
      *errmsg = _( "\\c must be followed by an ASCII character");
      break;
    case PCRE2_ERROR_BACKSLASH_K_SYNTAX:
      *errcode = G_REGEX_ERROR_MISSING_NAME;
      *errmsg = _("\\k is not followed by a braced, angle-bracketed, or quoted name");
      break;
    case PCRE2_ERROR_BACKSLASH_N_IN_CLASS:
      *errcode = G_REGEX_ERROR_NOT_SUPPORTED_IN_CLASS;
      *errmsg = _("\\N is not supported in a class");
      break;
    case PCRE2_ERROR_VERB_NAME_TOO_LONG:
      *errcode = G_REGEX_ERROR_NAME_TOO_LONG;
      *errmsg = _("name is too long in (*MARK), (*PRUNE), (*SKIP), or (*THEN)");
      break;
    case PCRE2_ERROR_INTERNAL_CODE_OVERFLOW:
      *errcode = G_REGEX_ERROR_INTERNAL;
      *errmsg = _("code overflow");
      break;
    case PCRE2_ERROR_UNRECOGNIZED_AFTER_QUERY_P:
      *errcode = G_REGEX_ERROR_UNRECOGNIZED_CHARACTER;
      *errmsg = _("unrecognized character after (?P");
      break;
    case PCRE2_ERROR_INTERNAL_OVERRAN_WORKSPACE:
      *errcode = G_REGEX_ERROR_INTERNAL;
      *errmsg = _("overran compiling workspace");
      break;
    case PCRE2_ERROR_INTERNAL_MISSING_SUBPATTERN:
      *errcode = G_REGEX_ERROR_INTERNAL;
      *errmsg = _("previously-checked referenced subpattern not found");
      break;
    case PCRE2_ERROR_HEAP_FAILED:
    case PCRE2_ERROR_INTERNAL_PARSED_OVERFLOW:
    case PCRE2_ERROR_UNICODE_NOT_SUPPORTED:
    case PCRE2_ERROR_UNICODE_DISALLOWED_CODE_POINT:
    case PCRE2_ERROR_NO_SURROGATES_IN_UTF16:
    case PCRE2_ERROR_INTERNAL_BAD_CODE_LOOKBEHINDS:
    case PCRE2_ERROR_UNICODE_PROPERTIES_UNAVAILABLE:
    case PCRE2_ERROR_INTERNAL_STUDY_ERROR:
    case PCRE2_ERROR_UTF_IS_DISABLED:
    case PCRE2_ERROR_UCP_IS_DISABLED:
    case PCRE2_ERROR_INTERNAL_BAD_CODE_AUTO_POSSESS:
    case PCRE2_ERROR_BACKSLASH_C_LIBRARY_DISABLED:
    case PCRE2_ERROR_INTERNAL_BAD_CODE:
    case PCRE2_ERROR_INTERNAL_BAD_CODE_IN_SKIP:
      *errcode = G_REGEX_ERROR_INTERNAL;
      break;
    case PCRE2_ERROR_INVALID_SUBPATTERN_NAME:
    case PCRE2_ERROR_CLASS_INVALID_RANGE:
    case PCRE2_ERROR_ZERO_RELATIVE_REFERENCE:
    case PCRE2_ERROR_PARENTHESES_STACK_CHECK:
    case PCRE2_ERROR_LOOKBEHIND_TOO_COMPLICATED:
    case PCRE2_ERROR_CALLOUT_NUMBER_TOO_BIG:
    case PCRE2_ERROR_MISSING_CALLOUT_CLOSING:
    case PCRE2_ERROR_ESCAPE_INVALID_IN_VERB:
    case PCRE2_ERROR_NULL_PATTERN:
    case PCRE2_ERROR_BAD_OPTIONS:
    case PCRE2_ERROR_PARENTHESES_NEST_TOO_DEEP:
    case PCRE2_ERROR_BACKSLASH_O_MISSING_BRACE:
    case PCRE2_ERROR_INVALID_OCTAL:
    case PCRE2_ERROR_CALLOUT_STRING_TOO_LONG:
    case PCRE2_ERROR_BACKSLASH_U_CODE_POINT_TOO_BIG:
    case PCRE2_ERROR_MISSING_OCTAL_OR_HEX_DIGITS:
    case PCRE2_ERROR_VERSION_CONDITION_SYNTAX:
    case PCRE2_ERROR_CALLOUT_NO_STRING_DELIMITER:
    case PCRE2_ERROR_CALLOUT_BAD_STRING_DELIMITER:
    case PCRE2_ERROR_BACKSLASH_C_CALLER_DISABLED:
    case PCRE2_ERROR_QUERY_BARJX_NEST_TOO_DEEP:
    case PCRE2_ERROR_PATTERN_TOO_COMPLICATED:
    case PCRE2_ERROR_LOOKBEHIND_TOO_LONG:
    case PCRE2_ERROR_PATTERN_STRING_TOO_LONG:
    case PCRE2_ERROR_BAD_LITERAL_OPTIONS:
    default:
      *errcode = G_REGEX_ERROR_COMPILE;
      break;
    }

  g_assert (*errcode != -1);
}

/* GMatchInfo */

static GMatchInfo *
match_info_new (const GRegex     *regex,
                const gchar      *string,
                size_t            string_len,
                size_t            start_position,
                GRegexMatchFlags  match_options,
                gboolean          is_dfa)
{
  GMatchInfo *match_info;

  match_info = g_new0 (GMatchInfo, 1);
  match_info->ref_count = 1;
  match_info->regex = g_regex_ref ((GRegex *)regex);
  match_info->string = string;
  match_info->string_len = string_len;
  match_info->matches = PCRE2_ERROR_NOMATCH;
  match_info->pos = start_position;
  match_info->pos_valid = TRUE;
  match_info->match_opts =
    get_pcre2_match_options (match_options, regex->regex_compile_opts);

  pcre2_pattern_info (regex->pcre_re, PCRE2_INFO_CAPTURECOUNT,
                      &match_info->n_subpatterns);

  match_info->match_context = pcre2_match_context_create (NULL);

  if (is_dfa)
    {
      /* These values should be enough for most cases, if they are not
       * enough g_regex_match_all_full() will expand them. */
      match_info->n_workspace = 100;
      match_info->workspace = g_new (gint, match_info->n_workspace);
    }

  match_info->n_offsets = 2;
  match_info->offsets = g_new0 (gint, match_info->n_offsets);
  /* Set an invalid position for the previous match. */
  match_info->offsets[0] = -1;
  match_info->offsets[1] = -1;

  match_info->match_data = pcre2_match_data_create_from_pattern (
      match_info->regex->pcre_re,
      NULL);

  return match_info;
}

static gboolean
recalc_match_offsets (GMatchInfo *match_info,
                      GError     **error)
{
  PCRE2_SIZE *ovector;
  uint32_t ovector_size = 0;
  uint32_t pre_n_offset;

  g_assert (!IS_PCRE2_ERROR (match_info->matches));

  if (match_info->matches == PCRE2_ERROR_PARTIAL)
    ovector_size = 1;
  else if (match_info->matches > 0)
    ovector_size = match_info->matches;

  g_assert (ovector_size != 0);

  if (pcre2_get_ovector_count (match_info->match_data) < ovector_size)
    {
      g_set_error (error, G_REGEX_ERROR, G_REGEX_ERROR_MATCH,
                   _("Error while matching regular expression %s: %s"),
                   match_info->regex->pattern, _("code overflow"));
      return FALSE;
    }

  pre_n_offset = match_info->n_offsets;
  match_info->n_offsets = ovector_size * 2;
  ovector = pcre2_get_ovector_pointer (match_info->match_data);

  if (match_info->n_offsets != pre_n_offset)
    {
      match_info->offsets = g_realloc_n (match_info->offsets,
                                         match_info->n_offsets,
                                         sizeof (gint));
    }

  for (size_t i = 0; i < match_info->n_offsets; i++)
    {
      match_info->offsets[i] = (int) ovector[i];
    }

  return TRUE;
}

static JITStatus
enable_jit_with_match_options (GMatchInfo  *match_info,
                               uint32_t  match_options)
{
  gint retval;
  uint32_t old_jit_options, new_jit_options;

  if (!(match_info->regex->regex_compile_opts & G_REGEX_OPTIMIZE))
    return JIT_STATUS_DISABLED;

  if (match_info->regex->jit_status == JIT_STATUS_DISABLED)
    return JIT_STATUS_DISABLED;

  if (match_options & G_REGEX_PCRE2_JIT_UNSUPPORTED_OPTIONS)
    return JIT_STATUS_DISABLED;

  old_jit_options = match_info->regex->jit_options;
  new_jit_options = old_jit_options | PCRE2_JIT_COMPLETE;
  if (match_options & PCRE2_PARTIAL_HARD)
    new_jit_options |= PCRE2_JIT_PARTIAL_HARD;
  if (match_options & PCRE2_PARTIAL_SOFT)
    new_jit_options |= PCRE2_JIT_PARTIAL_SOFT;

  /* no new options enabled */
  if (new_jit_options == old_jit_options)
    {
      g_assert (match_info->regex->jit_status != JIT_STATUS_DEFAULT);
      return match_info->regex->jit_status;
    }

  retval = pcre2_jit_compile (match_info->regex->pcre_re, new_jit_options);
  if (retval == 0)
    {
      match_info->regex->jit_status = JIT_STATUS_ENABLED;

      match_info->regex->jit_options = new_jit_options;
      /* Set min stack size for JIT to 32KiB and max to 512KiB */
      match_info->jit_stack = pcre2_jit_stack_create (1 << 15, 1 << 19, NULL);
      pcre2_jit_stack_assign (match_info->match_context, NULL, match_info->jit_stack);
    }
  else
    {
      match_info->regex->jit_status = JIT_STATUS_DISABLED;

      switch (retval)
        {
        case PCRE2_ERROR_NOMEMORY:
          g_debug ("JIT compilation was requested with G_REGEX_OPTIMIZE, "
                   "but JIT was unable to allocate executable memory for the "
                   "compiler. Falling back to interpretive code.");
          break;
        case PCRE2_ERROR_JIT_BADOPTION:
          g_debug ("JIT compilation was requested with G_REGEX_OPTIMIZE, "
                   "but JIT support is not available. Falling back to "
                   "interpretive code.");
          break;
        default:
          g_debug ("JIT compilation was requested with G_REGEX_OPTIMIZE, "
                   "but request for JIT support had unexpectedly failed (error %d). "
                   "Falling back to interpretive code.",
                   retval);
          break;
        }
    }

  return match_info->regex->jit_status;

  g_assert_not_reached ();
}

/**
 * g_match_info_get_regex:
 * @match_info: a #GMatchInfo
 *
 * Returns #GRegex object used in @match_info. It belongs to Glib
 * and must not be freed. Use g_regex_ref() if you need to keep it
 * after you free @match_info object.
 *
 * Returns: (transfer none): #GRegex object used in @match_info
 *
 * Since: 2.14
 */
GRegex *
g_match_info_get_regex (const GMatchInfo *match_info)
{
  g_return_val_if_fail (match_info != NULL, NULL);
  return match_info->regex;
}

/**
 * g_match_info_get_string:
 * @match_info: a #GMatchInfo
 *
 * Returns the string searched with @match_info. This is the
 * string passed to g_regex_match() or g_regex_replace() so
 * you may not free it before calling this function.
 *
 * Returns: the string searched with @match_info
 *
 * Since: 2.14
 */
const gchar *
g_match_info_get_string (const GMatchInfo *match_info)
{
  g_return_val_if_fail (match_info != NULL, NULL);
  return match_info->string;
}

/**
 * g_match_info_ref:
 * @match_info: a #GMatchInfo
 *
 * Increases reference count of @match_info by 1.
 *
 * Returns: @match_info
 *
 * Since: 2.30
 */
GMatchInfo       *
g_match_info_ref (GMatchInfo *match_info)
{
  g_return_val_if_fail (match_info != NULL, NULL);
  g_atomic_int_inc (&match_info->ref_count);
  return match_info;
}

/**
 * g_match_info_unref:
 * @match_info: a #GMatchInfo
 *
 * Decreases reference count of @match_info by 1. When reference count drops
 * to zero, it frees all the memory associated with the match_info structure.
 *
 * Since: 2.30
 */
void
g_match_info_unref (GMatchInfo *match_info)
{
  if (g_atomic_int_dec_and_test (&match_info->ref_count))
    {
      g_regex_unref (match_info->regex);
      if (match_info->match_context)
        pcre2_match_context_free (match_info->match_context);
      if (match_info->jit_stack)
        pcre2_jit_stack_free (match_info->jit_stack);
      if (match_info->match_data)
        pcre2_match_data_free (match_info->match_data);
      g_free (match_info->offsets);
      g_free (match_info->workspace);
      g_free (match_info);
    }
}

/**
 * g_match_info_free:
 * @match_info: (nullable): a #GMatchInfo, or %NULL
 *
 * If @match_info is not %NULL, calls g_match_info_unref(); otherwise does
 * nothing.
 *
 * Since: 2.14
 */
void
g_match_info_free (GMatchInfo *match_info)
{
  if (match_info == NULL)
    return;

  g_match_info_unref (match_info);
}

/**
 * g_match_info_next:
 * @match_info: a #GMatchInfo structure
 * @error: location to store the error occurring, or %NULL to ignore errors
 *
 * Scans for the next match using the same parameters of the previous
 * call to g_regex_match_full() or g_regex_match() that returned
 * @match_info.
 *
 * The match is done on the string passed to the match function, so you
 * cannot free it before calling this function.
 *
 * Returns: %TRUE is the string matched, %FALSE otherwise
 *
 * Since: 2.14
 */
gboolean
g_match_info_next (GMatchInfo  *match_info,
                   GError     **error)
{
  JITStatus jit_status;
  gint prev_match_start;
  gint prev_match_end;
  uint32_t opts;

  g_return_val_if_fail (match_info != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
  g_return_val_if_fail (match_info->pos_valid, FALSE);

  prev_match_start = match_info->offsets[0];
  prev_match_end = match_info->offsets[1];

  if (match_info->pos > match_info->string_len)
    {
      /* we have reached the end of the string */
      match_info->pos_valid = FALSE;
      match_info->matches = PCRE2_ERROR_NOMATCH;
      return FALSE;
    }

  opts = match_info->regex->match_opts | match_info->match_opts;

  jit_status = enable_jit_with_match_options (match_info, opts);
  if (jit_status == JIT_STATUS_ENABLED)
    {
      match_info->matches = pcre2_jit_match (match_info->regex->pcre_re,
                                             (PCRE2_SPTR8) match_info->string,
                                             match_info->string_len,
                                             match_info->pos,
                                             opts,
                                             match_info->match_data,
                                             match_info->match_context);
      /* if the JIT stack limit was reached, fall back to non-JIT matching in
       * the next conditional statement */
      if (match_info->matches == PCRE2_ERROR_JIT_STACKLIMIT)
        {
          g_debug ("PCRE2 JIT stack limit reached, falling back to "
                   "non-optimized matching.");
          opts |= PCRE2_NO_JIT;
          jit_status = JIT_STATUS_DISABLED;
        }
    }

  if (jit_status != JIT_STATUS_ENABLED)
    {
      match_info->matches = pcre2_match (match_info->regex->pcre_re,
                                         (PCRE2_SPTR8) match_info->string,
                                         match_info->string_len,
                                         match_info->pos,
                                         opts,
                                         match_info->match_data,
                                         match_info->match_context);
    }

  if (IS_PCRE2_ERROR (match_info->matches))
    {
      gchar *error_msg = get_match_error_message (match_info->matches);

      g_set_error (error, G_REGEX_ERROR, G_REGEX_ERROR_MATCH,
                   _("Error while matching regular expression %s: %s"),
                   match_info->regex->pattern, error_msg);
      g_clear_pointer (&error_msg, g_free);
      return FALSE;
    }
  else if (match_info->matches == 0)
    {
      /* info->offsets is too small. */
      match_info->n_offsets *= 2;

      /* uint32_t is the type accepted by pcre2_match_data_create() */
      g_assert (match_info->n_offsets <= UINT32_MAX);

      match_info->offsets = g_realloc_n (match_info->offsets,
                                         match_info->n_offsets,
                                         sizeof (gint));

      pcre2_match_data_free (match_info->match_data);
      match_info->match_data = pcre2_match_data_create (match_info->n_offsets, NULL);

      return g_match_info_next (match_info, error);
    }
  else if (match_info->matches == PCRE2_ERROR_NOMATCH)
    {
      /* We're done with this match info */
      match_info->pos_valid = FALSE;
      return FALSE;
    }
  else
    if (!recalc_match_offsets (match_info, error))
      return FALSE;

  /* avoid infinite loops if the pattern is an empty string or something
   * equivalent */
  g_assert (match_info->offsets[1] >= 0);
  if (match_info->pos == (size_t) match_info->offsets[1])
    {
      if (match_info->pos > match_info->string_len)
        {
          /* we have reached the end of the string */
          match_info->pos_valid = FALSE;
          match_info->matches = PCRE2_ERROR_NOMATCH;
          return FALSE;
        }

      match_info->pos = NEXT_CHAR (match_info->regex,
                                   &match_info->string[match_info->pos]) -
                                   match_info->string;
      match_info->pos_valid = TRUE;
    }
  else
    {
      g_assert (match_info->offsets[1] >= 0);
      match_info->pos = match_info->offsets[1];
      match_info->pos_valid = TRUE;
    }

  g_assert (match_info->matches < 0 ||
            (size_t) match_info->matches <= (size_t) match_info->n_subpatterns + 1);

  /* it's possible to get two identical matches when we are matching
   * empty strings, for instance if the pattern is "(?=[A-Z0-9])" and
   * the string is "RegExTest" we have:
   *  - search at position 0: match from 0 to 0
   *  - search at position 1: match from 3 to 3
   *  - search at position 3: match from 3 to 3 (duplicate)
   *  - search at position 4: match from 5 to 5
   *  - search at position 5: match from 5 to 5 (duplicate)
   *  - search at position 6: no match -> stop
   * so we have to ignore the duplicates.
   * see bug #515944: http://bugzilla.gnome.org/show_bug.cgi?id=515944 */
  if (match_info->matches >= 0 &&
      prev_match_start == match_info->offsets[0] &&
      prev_match_end == match_info->offsets[1])
    {
      /* ignore this match and search the next one */
      return g_match_info_next (match_info, error);
    }

  return match_info->matches >= 0;
}

/**
 * g_match_info_matches:
 * @match_info: a #GMatchInfo structure
 *
 * Returns whether the previous match operation succeeded.
 *
 * Returns: %TRUE if the previous match operation succeeded,
 *   %FALSE otherwise
 *
 * Since: 2.14
 */
gboolean
g_match_info_matches (const GMatchInfo *match_info)
{
  g_return_val_if_fail (match_info != NULL, FALSE);

  return match_info->matches >= 0;
}

/**
 * g_match_info_get_match_count:
 * @match_info: a #GMatchInfo structure
 *
 * Retrieves the number of matched substrings (including substring 0,
 * that is the whole matched text), so 1 is returned if the pattern
 * has no substrings in it and 0 is returned if the match failed.
 *
 * If the last match was obtained using the DFA algorithm, that is
 * using g_regex_match_all() or g_regex_match_all_full(), the retrieved
 * count is not that of the number of capturing parentheses but that of
 * the number of matched substrings.
 *
 * Returns: Number of matched substrings, or -1 if an error occurred
 *
 * Since: 2.14
 */
gint
g_match_info_get_match_count (const GMatchInfo *match_info)
{
  g_return_val_if_fail (match_info, -1);

  if (match_info->matches == PCRE2_ERROR_NOMATCH)
    /* no match */
    return 0;
  else if (match_info->matches < PCRE2_ERROR_NOMATCH)
    /* error */
    return -1;
  else
    /* match */
    return match_info->matches;
}

/**
 * g_match_info_is_partial_match:
 * @match_info: a #GMatchInfo structure
 *
 * Usually if the string passed to g_regex_match*() matches as far as
 * it goes, but is too short to match the entire pattern, %FALSE is
 * returned. There are circumstances where it might be helpful to
 * distinguish this case from other cases in which there is no match.
 *
 * Consider, for example, an application where a human is required to
 * type in data for a field with specific formatting requirements. An
 * example might be a date in the form ddmmmyy, defined by the pattern
 * "^\d?\d(jan|feb|mar|apr|may|jun|jul|aug|sep|oct|nov|dec)\d\d$".
 * If the application sees the user’s keystrokes one by one, and can
 * check that what has been typed so far is potentially valid, it is
 * able to raise an error as soon as a mistake is made.
 *
 * GRegex supports the concept of partial matching by means of the
 * %G_REGEX_MATCH_PARTIAL_SOFT and %G_REGEX_MATCH_PARTIAL_HARD flags.
 * When they are used, the return code for
 * g_regex_match() or g_regex_match_full() is, as usual, %TRUE
 * for a complete match, %FALSE otherwise. But, when these functions
 * return %FALSE, you can check if the match was partial calling
 * g_match_info_is_partial_match().
 *
 * The difference between %G_REGEX_MATCH_PARTIAL_SOFT and
 * %G_REGEX_MATCH_PARTIAL_HARD is that when a partial match is encountered
 * with %G_REGEX_MATCH_PARTIAL_SOFT, matching continues to search for a
 * possible complete match, while with %G_REGEX_MATCH_PARTIAL_HARD matching
 * stops at the partial match.
 * When both %G_REGEX_MATCH_PARTIAL_SOFT and %G_REGEX_MATCH_PARTIAL_HARD
 * are set, the latter takes precedence.
 *
 * There were formerly some restrictions on the pattern for partial matching.
 * The restrictions no longer apply.
 *
 * If the match was partial g_match_info_fetch(), g_match_info_fetch_pos()
 * and g_match_info_fetch_all() can be called to retrieve the text and positions
 * of the entire match, i.e. only for sub expression `0`.
 *
 * See pcrepartial(3) for more information on partial matching.
 *
 * Returns: %TRUE if the match was partial, %FALSE otherwise
 *
 * Since: 2.14
 */
gboolean
g_match_info_is_partial_match (const GMatchInfo *match_info)
{
  g_return_val_if_fail (match_info != NULL, FALSE);

  return match_info->matches == PCRE2_ERROR_PARTIAL;
}

/**
 * g_match_info_expand_references:
 * @match_info: (nullable): a #GMatchInfo or %NULL
 * @string_to_expand: the string to expand
 * @error: location to store the error occurring, or %NULL to ignore errors
 *
 * Returns a new string containing the text in @string_to_expand with
 * references and escape sequences expanded. References refer to the last
 * match done with @string against @regex and have the same syntax used by
 * g_regex_replace().
 *
 * The @string_to_expand must be UTF-8 encoded even if %G_REGEX_RAW was
 * passed to g_regex_new().
 *
 * The backreferences are extracted from the string passed to the match
 * function, so you cannot call this function after freeing the string.
 *
 * @match_info may be %NULL in which case @string_to_expand must not
 * contain references. For instance "foo\n" does not refer to an actual
 * pattern and '\n' merely will be replaced with \n character,
 * while to expand "\0" (whole match) one needs the result of a match.
 * Use g_regex_check_replacement() to find out whether @string_to_expand
 * contains references.
 *
 * Returns: (nullable): the expanded string, or %NULL if an error occurred
 *
 * Since: 2.14
 */
gchar *
g_match_info_expand_references (const GMatchInfo  *match_info,
                                const gchar       *string_to_expand,
                                GError           **error)
{
  GString *result;
  GList *list;
  GError *tmp_error = NULL;

  g_return_val_if_fail (string_to_expand != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  list = split_replacement (string_to_expand, &tmp_error);
  if (tmp_error != NULL)
    {
      g_propagate_error (error, tmp_error);
      return NULL;
    }

  if (!match_info && interpolation_list_needs_match (list))
    {
      g_critical ("String '%s' contains references to the match, can't "
                  "expand references without GMatchInfo object",
                  string_to_expand);
      return NULL;
    }

  result = g_string_sized_new (strlen (string_to_expand));
  interpolate_replacement (match_info, result, list);

  g_list_free_full (list, (GDestroyNotify) free_interpolation_data);

  return g_string_free (result, FALSE);
}

/**
 * g_match_info_fetch:
 * @match_info: #GMatchInfo structure
 * @match_num: number of the sub expression
 *
 * Retrieves the text matching the @match_num'th capturing
 * parentheses. 0 is the full text of the match, 1 is the first paren
 * set, 2 the second, and so on.
 *
 * If @match_num is a valid sub pattern but it didn't match anything
 * (e.g. sub pattern 1, matching "b" against "(a)?b") then an empty
 * string is returned.
 * When a partial match is reported via g_match_info_is_partial_match()
 * only the full text of the match can be queried (@match_num must be `0`).
 *
 * If the match was obtained using the DFA algorithm, that is using
 * g_regex_match_all() or g_regex_match_all_full(), the retrieved
 * string is not that of a set of parentheses but that of a matched
 * substring. Substrings are matched in reverse order of length, so
 * 0 is the longest match.
 *
 * The string is fetched from the string passed to the match function,
 * so you cannot call this function after freeing the string.
 *
 * Returns: (nullable): The matched substring, or %NULL if an error
 *     occurred. You have to free the string yourself
 *
 * Since: 2.14
 */
gchar *
g_match_info_fetch (const GMatchInfo *match_info,
                    gint              match_num)
{
  gchar *match = NULL;
  gint start, end;

  g_return_val_if_fail (match_info != NULL, NULL);
  g_return_val_if_fail (match_num >= 0, NULL);

  /* match_num does not exist or it didn't matched, i.e. matching "b"
   * against "(a)?b" then group 0 is empty. */
  if (!g_match_info_fetch_pos (match_info, match_num, &start, &end))
    match = NULL;
  else if (start == -1)
    match = g_strdup ("");
  else
    match = g_strndup (&match_info->string[start], end - start);

  return match;
}

/**
 * g_match_info_fetch_pos:
 * @match_info: #GMatchInfo structure
 * @match_num: number of the capture parenthesis
 * @start_pos: (out) (optional): pointer to location where to store
 *     the start position, or %NULL
 * @end_pos: (out) (optional): pointer to location where to store
 *     the end position (the byte after the final byte of the match), or %NULL
 *
 * Returns the start and end positions (in bytes) of a successfully matching 
 * capture parenthesis.
 * 
 * Valid values for @match_num are `0` for the full text of the match,
 * `1` for the first paren set, `2` for the second, and so on.
 * When a partial match is reported via g_match_info_is_partial_match()
 * only the full text of the match can be queried (@match_num must be `0`).
 *
 * As @end_pos is set to the byte after the final byte of the match (on success),
 * the length of the match can be calculated as `end_pos - start_pos`.
 *
 * As a best practice, initialize @start_pos and @end_pos to identifiable 
 * values, such as `G_MAXINT`, so that you can test if 
 * `g_match_info_fetch_pos()` actually changed the value for a given 
 * capture parenthesis.
 *
 * The parameter @match_num corresponds to a matched capture parenthesis. The 
 * actual value you use for @match_num depends on the method used to generate
 * @match_info. The following sections describe those methods.
 * 
 * ## Methods Using Non-deterministic Finite Automata Matching
 *
 * The methods [method@GLib.Regex.match] and [method@GLib.Regex.match_full]
 * return a [struct@GLib.MatchInfo] using traditional (greedy) pattern
 * matching, also known as 
 * [Non-deterministic Finite Automaton](https://en.wikipedia.org/wiki/Nondeterministic_finite_automaton)
 * (NFA) matching. You pass the returned `GMatchInfo` from these methods to 
 * `g_match_info_fetch_pos()` to determine the start and end positions 
 * of capture parentheses. The values for @match_num correspond to the capture 
 * parentheses in order, with `0` corresponding to the entire matched string.
 * 
 * @match_num can refer to a capture parenthesis with no match. For example, 
 * the string `b` matches against the pattern `(a)?b`, but the capture
 * parenthesis `(a)` has no match. In this case, `g_match_info_fetch_pos()`
 * returns true and sets @start_pos and @end_pos to `-1` when called with
 * `match_num` as `1` (for `(a)`).
 *
 * For an expanded example, a regex pattern is `(a)?(.*?)the (.*)`, 
 * and a candidate string is `glib regexes are the best`. In this scenario 
 * there are four capture parentheses numbered 0–3: an implicit one 
 * for the entire string, and three explicitly declared in the regex pattern.
 *
 * Given this example, the following table describes the return values 
 * from `g_match_info_fetch_pos()` for various values of @match_num.
 *
 * `match_num` | Contents | Return value | Returned `start_pos` | Returned `end_pos`
 * ----------- | -------- | ------------ | -------------------- | ------------------
 * 0 | Matches entire string | True | 0 | 25
 * 1 | Does not match first character | True | -1 | -1
 * 2 | All text before `the ` | True | 0 | 17
 * 3 | All text after `the ` | True | 21 | 25
 * 4 | Capture paren out of range | False | Unchanged | Unchanged
 *
 * The following code sample and output implements this example.
 *
 * ``` { .c }
 * #include <glib.h>
 *
 * int
 * main (int argc, char *argv[])
 * {
 *   g_autoptr(GError) local_error = NULL;
 *   const char *regex_pattern = "(a)?(.*?)the (.*)";
 *   const char *test_string = "glib regexes are the best";
 *   g_autoptr(GRegex) regex = NULL;
 *
 *   regex = g_regex_new (regex_pattern,
 *                        G_REGEX_DEFAULT,
 *                        G_REGEX_MATCH_DEFAULT,
 *                        &local_error);
 *   if (regex == NULL)
 *     {
 *       g_printerr ("Error creating regex: %s\n", local_error->message);
 *       return 1;
 *     }
 *
 *   g_autoptr(GMatchInfo) match_info = NULL;
 *   g_regex_match (regex, test_string, G_REGEX_MATCH_DEFAULT, &match_info);
 *
 *   int n_matched_strings = g_match_info_get_match_count (match_info);
 *
 *   // Print header line
 *   g_print ("match_num Contents                  Return value returned start_pos returned end_pos\n");
 *
 *   // Iterate over each capture paren, including one that is out of range as a demonstration.
 *   for (int match_num = 0; match_num <= n_matched_strings; match_num++)
 *     {
 *       gboolean found_match;
 *       g_autofree char *paren_string = NULL;
 *       int start_pos = G_MAXINT;
 *       int end_pos = G_MAXINT;
 *
 *       found_match = g_match_info_fetch_pos (match_info,
 *                                             match_num,
 *                                             &start_pos,
 *                                             &end_pos);
 *
 *       // If no match, display N/A as the found string.
 *       if (start_pos == G_MAXINT || start_pos == -1)
 *         paren_string = g_strdup ("N/A");
 *       else
 *         paren_string = g_strndup (test_string + start_pos, end_pos - start_pos);
 *
 *       g_print ("%-9d %-25s %-12d %-18d %d\n", match_num, paren_string, found_match, start_pos, end_pos);
 *     }
 *
 *   return 0;
 * }
 * ```
 *
 * ```
 * match_num Contents                  Return value returned start_pos returned end_pos
 * 0         glib regexes are the best 1            0                  25
 * 1         N/A                       1            -1                 -1
 * 2         glib regexes are          1            0                  17
 * 3         best                      1            21                 25
 * 4         N/A                       0            2147483647         2147483647
 * ```
 * ## Methods Using Deterministic Finite Automata Matching
 *
 * The methods [method@GLib.Regex.match_all] and 
 * [method@GLib.Regex.match_all_full]
 * return a `GMatchInfo` using
 * [Deterministic Finite Automaton](https://en.wikipedia.org/wiki/Deterministic_finite_automaton)
 * (DFA) pattern matching. This algorithm detects overlapping matches. You pass
 * the returned `GMatchInfo` from these methods to `g_match_info_fetch_pos()`
 * to determine the start and end positions of each overlapping match. Use the 
 * method [method@GLib.MatchInfo.get_match_count] to determine the number 
 * of overlapping matches.
 *
 * For example, a regex pattern is `<.*>`, and a candidate string is 
 * `<a> <b> <c>`. In this scenario there are three implicit capture 
 * parentheses: one for the entire string, one for `<a> <b>`, and one for `<a>`.
 *
 * Given this example, the following table describes the return values from
 * `g_match_info_fetch_pos()` for various values of @match_num.
 *
 * `match_num` | Contents | Return value | Returned `start_pos` | Returned `end_pos`
 * ----------- | -------- | ------------ | -------------------- | ------------------
 * 0 | Matches entire string | True | 0 | 11
 * 1 | Matches `<a> <b>` | True | 0 | 7
 * 2 | Matches `<a>` | True | 0 | 3
 * 3 | Capture paren out of range | False | Unchanged | Unchanged
 *
 * The following code sample and output implements this example.
 *
 * ``` { .c }
 * #include <glib.h>
 *
 * int
 * main (int argc, char *argv[])
 * {
 *   g_autoptr(GError) local_error = NULL;
 *   const char *regex_pattern = "<.*>";
 *   const char *test_string = "<a> <b> <c>";
 *   g_autoptr(GRegex) regex = NULL;
 * 
 *   regex = g_regex_new (regex_pattern,
 *                        G_REGEX_DEFAULT,
 *                        G_REGEX_MATCH_DEFAULT,
 *                        &local_error);
 *   if (regex == NULL)
 *     {
 *       g_printerr ("Error creating regex: %s\n", local_error->message);
 *       return -1;
 *     }
 *
 *   g_autoptr(GMatchInfo) match_info = NULL;
 *   g_regex_match_all (regex, test_string, G_REGEX_MATCH_DEFAULT, &match_info);
 *
 *   int n_matched_strings = g_match_info_get_match_count (match_info);
 *
 *   // Print header line 
 *   g_print ("match_num Contents                  Return value returned start_pos returned end_pos\n");
 * 
 *   // Iterate over each capture paren, including one that is out of range as a demonstration.
 *   for (int match_num = 0; match_num <= n_matched_strings; match_num++)
 *     {
 *       gboolean found_match;
 *       g_autofree char *paren_string = NULL;
 *       int start_pos = G_MAXINT;
 *       int end_pos = G_MAXINT;
 *
 *       found_match = g_match_info_fetch_pos (match_info, match_num, &start_pos, &end_pos);
 *
 *       // If no match, display N/A as the found string.
 *       if (start_pos == G_MAXINT || start_pos == -1)
 *         paren_string = g_strdup ("N/A");
 *       else
 *         paren_string = g_strndup (test_string + start_pos, end_pos - start_pos);
 *
 *       g_print ("%-9d %-25s %-12d %-18d %d\n", match_num, paren_string, found_match, start_pos, end_pos);
 *     }
 *
 *   return 0;
 * }
 * ```
 *
 * ```
 * match_num Contents                  Return value returned start_pos returned end_pos
 * 0         <a> <b> <c>               1            0                  11
 * 1         <a> <b>                   1            0                  7
 * 2         <a>                       1            0                  3
 * 3         N/A                       0            2147483647         2147483647
 * ```
 *
 * Returns: True if @match_num is within range, false otherwise. If
 *   the capture paren has a match, @start_pos and @end_pos contain the 
 *   start and end positions (in bytes) of the matching substring. If the 
 *   capture paren has no match, @start_pos and @end_pos are `-1`. If 
 *   @match_num is out of range, @start_pos and @end_pos are left unchanged.
 *
 * Since: 2.14
 */
gboolean
g_match_info_fetch_pos (const GMatchInfo *match_info,
                        gint              match_num,
                        gint             *start_pos,
                        gint             *end_pos)
{
  size_t match_num_unsigned;
  gint matches;

  g_return_val_if_fail (match_info != NULL, FALSE);
  g_return_val_if_fail (match_num >= 0, FALSE);

  match_num_unsigned = (size_t) match_num;

  /* check whether there was an error */
  if (match_info->matches == PCRE2_ERROR_PARTIAL)
    {
      if (match_num_unsigned >= 1)
        return FALSE;
      matches = 1;
    }
  else
    {
      matches = match_info->matches;
      if (matches < 0)
        return FALSE;
      /* make sure the sub expression number they're requesting is less than
       * the total number of sub expressions in the regex. When matching all
       * (g_regex_match_all()), also compare against the number of matches */
      if (match_num_unsigned >= MAX ((size_t) match_info->n_subpatterns + 1, (size_t) matches))
        return FALSE;
    }

  if (start_pos != NULL)
    *start_pos = (match_num_unsigned < (size_t) matches) ? match_info->offsets[2 * match_num_unsigned] : -1;

  if (end_pos != NULL)
    *end_pos = (match_num_unsigned < (size_t) matches) ? match_info->offsets[2 * match_num_unsigned + 1] : -1;

  return TRUE;
}

/*
 * Returns number of first matched subpattern with name @name.
 * There may be more than one in case when DUPNAMES is used,
 * and not all subpatterns with that name match;
 * pcre2_substring_number_from_name() does not work in that case.
 */
static gint
get_matched_substring_number (const GMatchInfo *match_info,
                              const gchar      *name)
{
  gint entrysize;
  PCRE2_SPTR first, last;
  guchar *entry;

  if (!(match_info->regex->pcre2_compile_opts & PCRE2_DUPNAMES))
    return pcre2_substring_number_from_name (match_info->regex->pcre_re, (PCRE2_SPTR8) name);

  /* This code is analogous to code from pcre2_substring.c:
   * pcre2_substring_get_byname() */
  entrysize = pcre2_substring_nametable_scan (match_info->regex->pcre_re,
                                              (PCRE2_SPTR8) name,
                                              &first,
                                              &last);

  if (entrysize <= 0)
    return entrysize;

  for (entry = (guchar*) first; entry <= (guchar*) last; entry += entrysize)
    {
      guint n = (entry[0] << 8) + entry[1];
      if (n * 2 < match_info->n_offsets && match_info->offsets[n * 2] >= 0)
        return n;
    }

  return (first[0] << 8) + first[1];
}

/**
 * g_match_info_fetch_named:
 * @match_info: #GMatchInfo structure
 * @name: name of the subexpression
 *
 * Retrieves the text matching the capturing parentheses named @name.
 *
 * If @name is a valid sub pattern name but it didn't match anything
 * (e.g. sub pattern `"X"`, matching `"b"` against `"(?P<X>a)?b"`)
 * then an empty string is returned.
 *
 * The string is fetched from the string passed to the match function,
 * so you cannot call this function after freeing the string.
 *
 * Returns: (nullable): The matched substring, or %NULL if an error
 *     occurred. You have to free the string yourself
 *
 * Since: 2.14
 */
gchar *
g_match_info_fetch_named (const GMatchInfo *match_info,
                          const gchar      *name)
{
  gint num;

  g_return_val_if_fail (match_info != NULL, NULL);
  g_return_val_if_fail (name != NULL, NULL);

  num = get_matched_substring_number (match_info, name);
  if (num < 0)
    return NULL;
  else
    return g_match_info_fetch (match_info, num);
}

/**
 * g_match_info_fetch_named_pos:
 * @match_info: #GMatchInfo structure
 * @name: name of the subexpression
 * @start_pos: (out) (optional): pointer to location where to store
 *     the start position, or %NULL
 * @end_pos: (out) (optional): pointer to location where to store
 *     the end position (the byte after the final byte of the match), or %NULL
 *
 * Retrieves the position in bytes of the capturing parentheses named @name.
 *
 * If @name is a valid sub pattern name but it didn't match anything
 * (e.g. sub pattern `"X"`, matching `"b"` against `"(?P<X>a)?b"`)
 * then @start_pos and @end_pos are set to -1 and %TRUE is returned.
 *
 * As @end_pos is set to the byte after the final byte of the match (on success),
 * the length of the match can be calculated as `end_pos - start_pos`.
 *
 * Returns: %TRUE if the position was fetched, %FALSE otherwise.
 *     If the position cannot be fetched, @start_pos and @end_pos
 *     are left unchanged.
 *
 * Since: 2.14
 */
gboolean
g_match_info_fetch_named_pos (const GMatchInfo *match_info,
                              const gchar      *name,
                              gint             *start_pos,
                              gint             *end_pos)
{
  gint num;

  g_return_val_if_fail (match_info != NULL, FALSE);
  g_return_val_if_fail (name != NULL, FALSE);

  num = get_matched_substring_number (match_info, name);
  if (num < 0)
    return FALSE;

  return g_match_info_fetch_pos (match_info, num, start_pos, end_pos);
}

/**
 * g_match_info_fetch_all:
 * @match_info: a #GMatchInfo structure
 *
 * Bundles up pointers to each of the matching substrings from a match
 * and stores them in an array of gchar pointers. The first element in
 * the returned array is the match number 0, i.e. the entire matched
 * text.
 *
 * If a sub pattern didn't match anything (e.g. sub pattern 1, matching
 * "b" against "(a)?b") then an empty string is inserted.
 *
 * When a partial match is reported via g_match_info_is_partial_match()
 * only the full text of the match will be returned, i.e. an array of size 1.
 *
 * If the last match was obtained using the DFA algorithm, that is using
 * g_regex_match_all() or g_regex_match_all_full(), the retrieved
 * strings are not that matched by sets of parentheses but that of the
 * matched substring. Substrings are matched in reverse order of length,
 * so the first one is the longest match.
 *
 * The strings are fetched from the string passed to the match function,
 * so you cannot call this function after freeing the string.
 *
 * Returns: (transfer full): a %NULL-terminated array of gchar *
 *     pointers.  It must be freed using g_strfreev(). If the previous
 *     match failed %NULL is returned
 *
 * Since: 2.14
 */
gchar **
g_match_info_fetch_all (const GMatchInfo *match_info)
{
  gchar **result;
  gint matches, i;

  g_return_val_if_fail (match_info != NULL, NULL);

  matches = (match_info->matches == PCRE2_ERROR_PARTIAL) ? 1 : match_info->matches;
  if (matches < 0)
    return NULL;

  result = g_new (gchar *, matches + 1);
  for (i = 0; i < matches; i++)
    result[i] = g_match_info_fetch (match_info, i);
  result[i] = NULL;

  return result;
}


/* GRegex */

G_DEFINE_QUARK (g-regex-error-quark, g_regex_error)

/**
 * g_regex_ref:
 * @regex: a #GRegex
 *
 * Increases reference count of @regex by 1.
 *
 * Returns: @regex
 *
 * Since: 2.14
 */
GRegex *
g_regex_ref (GRegex *regex)
{
  g_return_val_if_fail (regex != NULL, NULL);
  g_atomic_int_inc (&regex->ref_count);
  return regex;
}

/**
 * g_regex_unref:
 * @regex: a #GRegex
 *
 * Decreases reference count of @regex by 1. When reference count drops
 * to zero, it frees all the memory associated with the regex structure.
 *
 * Since: 2.14
 */
void
g_regex_unref (GRegex *regex)
{
  g_return_if_fail (regex != NULL);

  if (g_atomic_int_dec_and_test (&regex->ref_count))
    {
      g_free (regex->pattern);
      if (regex->pcre_re != NULL)
        pcre2_code_free (regex->pcre_re);
      g_free (regex);
    }
}

static pcre2_code * regex_compile (const gchar  *pattern,
                                   uint32_t      compile_options,
                                   uint32_t      newline_options,
                                   uint32_t      bsr_options,
                                   GError      **error);

static uint32_t get_pcre2_inline_compile_options (pcre2_code *re,
                                                  uint32_t    compile_options);

/**
 * g_regex_new:
 * @pattern: the regular expression
 * @compile_options: compile options for the regular expression, or 0
 * @match_options: match options for the regular expression, or 0
 * @error: return location for a #GError
 *
 * Compiles the regular expression to an internal form, and does
 * the initial setup of the #GRegex structure.
 *
 * Returns: (nullable): a #GRegex structure or %NULL if an error occurred. Call
 *   g_regex_unref() when you are done with it
 *
 * Since: 2.14
 */
GRegex *
g_regex_new (const gchar         *pattern,
             GRegexCompileFlags   compile_options,
             GRegexMatchFlags     match_options,
             GError             **error)
{
  GRegex *regex;
  pcre2_code *re;
  static gsize initialised = 0;
  uint32_t pcre_compile_options;
  uint32_t pcre_match_options;
  uint32_t newline_options;
  uint32_t bsr_options;

  g_return_val_if_fail (pattern != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  g_return_val_if_fail ((compile_options & ~(G_REGEX_COMPILE_MASK |
                                             G_REGEX_JAVASCRIPT_COMPAT)) == 0, NULL);
G_GNUC_END_IGNORE_DEPRECATIONS
  g_return_val_if_fail ((match_options & ~G_REGEX_MATCH_MASK) == 0, NULL);

  if (g_once_init_enter (&initialised))
    {
      int supports_utf8;

      pcre2_config (PCRE2_CONFIG_UNICODE, &supports_utf8);
      if (!supports_utf8)
        g_critical (_("PCRE library is compiled without UTF8 support"));

      g_once_init_leave (&initialised, supports_utf8 ? 1 : 2);
    }

  if (G_UNLIKELY (initialised != 1))
    {
      g_set_error_literal (error, G_REGEX_ERROR, G_REGEX_ERROR_COMPILE, 
                           _("PCRE library is compiled with incompatible options"));
      return NULL;
    }

  pcre_compile_options = get_pcre2_compile_options (compile_options);
  pcre_match_options = get_pcre2_match_options (match_options, compile_options);

  newline_options = get_pcre2_newline_match_options (match_options);
  if (newline_options == 0)
    newline_options = get_pcre2_newline_compile_options (compile_options);

  if (newline_options == 0)
    {
      g_set_error (error, G_REGEX_ERROR, G_REGEX_ERROR_INCONSISTENT_NEWLINE_OPTIONS,
                   "Invalid newline flags");
      return NULL;
    }

  bsr_options = get_pcre2_bsr_match_options (match_options);
  if (!bsr_options)
    bsr_options = get_pcre2_bsr_compile_options (compile_options);

  re = regex_compile (pattern, pcre_compile_options,
                      newline_options, bsr_options, error);
  if (re == NULL)
    return NULL;

  pcre_compile_options |=
    get_pcre2_inline_compile_options (re, pcre_compile_options);

  regex = g_new0 (GRegex, 1);
  regex->ref_count = 1;
  regex->pattern = g_strdup (pattern);
  regex->pcre_re = re;
  regex->pcre2_compile_opts = pcre_compile_options;
  regex->regex_compile_opts = compile_options;
  regex->match_opts = pcre_match_options;
  regex->orig_match_opts = match_options;

  return regex;
}

static pcre2_code *
regex_compile (const gchar  *pattern,
               uint32_t      compile_options,
               uint32_t      newline_options,
               uint32_t      bsr_options,
               GError      **error)
{
  pcre2_code *re;
  pcre2_compile_context *context;
  const gchar *errmsg;
  PCRE2_SIZE erroffset;
  gint errcode;

  context = pcre2_compile_context_create (NULL);

  /* set newline options */
  if (pcre2_set_newline (context, newline_options) != 0)
    {
      g_set_error (error, G_REGEX_ERROR,
                   G_REGEX_ERROR_INCONSISTENT_NEWLINE_OPTIONS,
                   "Invalid newline flags");
      pcre2_compile_context_free (context);
      return NULL;
    }

  /* set bsr options */
  if (pcre2_set_bsr (context, bsr_options) != 0)
    {
      g_set_error (error, G_REGEX_ERROR,
                   G_REGEX_ERROR_INCONSISTENT_NEWLINE_OPTIONS,
                   "Invalid BSR flags");
      pcre2_compile_context_free (context);
      return NULL;
    }

  /* In case UTF-8 mode is used, also set PCRE2_NO_UTF_CHECK */
  if (compile_options & PCRE2_UTF)
    compile_options |= PCRE2_NO_UTF_CHECK;

  compile_options |= PCRE2_UCP;

  /* compile the pattern */
  re = pcre2_compile ((PCRE2_SPTR8) pattern,
                      PCRE2_ZERO_TERMINATED,
                      compile_options,
                      &errcode,
                      &erroffset,
                      context);
  pcre2_compile_context_free (context);

  /* if the compilation failed, set the error member and return
   * immediately */
  if (re == NULL)
    {
      GError *tmp_error;
      gchar *offset_str;
      gchar *pcre2_errmsg = NULL;
      int original_errcode;

      /* Translate the PCRE error code to GRegexError and use a translated
       * error message if possible */
      original_errcode = errcode;
      translate_compile_error (&errcode, &errmsg);

      if (!errmsg)
        {
          errmsg = _("unknown error");
          pcre2_errmsg = get_pcre2_error_string (original_errcode);
        }

      /* PCRE uses byte offsets but we want to show character offsets */
      erroffset = g_utf8_pointer_to_offset (pattern, &pattern[erroffset]);

      offset_str = g_strdup_printf ("%" G_GSIZE_FORMAT, erroffset);
      tmp_error = g_error_new (G_REGEX_ERROR, errcode,
                               _("Error while compiling regular expression ‘%s’ "
                                 "at char %s: %s"),
                               pattern, offset_str,
                               pcre2_errmsg ? pcre2_errmsg : errmsg);
      g_propagate_error (error, tmp_error);
      g_free (offset_str);
      g_clear_pointer (&pcre2_errmsg, g_free);

      return NULL;
    }

  return re;
}

static uint32_t
get_pcre2_inline_compile_options (pcre2_code *re,
                                  uint32_t    compile_options)
{
  uint32_t pcre_compile_options;
  uint32_t nonpcre_compile_options;

  /* For options set at the beginning of the pattern, pcre puts them into
   * compile options, e.g. "(?i)foo" will make the pcre structure store
   * PCRE2_CASELESS even though it wasn't explicitly given for compilation. */
  nonpcre_compile_options = compile_options & G_REGEX_COMPILE_NONPCRE_MASK;
  pcre2_pattern_info (re, PCRE2_INFO_ALLOPTIONS, &pcre_compile_options);
  compile_options = pcre_compile_options & G_REGEX_PCRE2_COMPILE_MASK;
  compile_options |= nonpcre_compile_options;

  if (!(compile_options & PCRE2_DUPNAMES))
    {
      uint32_t jchanged = 0;
      pcre2_pattern_info (re, PCRE2_INFO_JCHANGED, &jchanged);
      if (jchanged)
        compile_options |= PCRE2_DUPNAMES;
    }

  return compile_options;
}

/**
 * g_regex_get_pattern:
 * @regex: a #GRegex structure
 *
 * Gets the pattern string associated with @regex, i.e. a copy of
 * the string passed to g_regex_new().
 *
 * Returns: the pattern of @regex
 *
 * Since: 2.14
 */
const gchar *
g_regex_get_pattern (const GRegex *regex)
{
  g_return_val_if_fail (regex != NULL, NULL);

  return regex->pattern;
}

/**
 * g_regex_get_max_backref:
 * @regex: a #GRegex
 *
 * Returns the number of the highest back reference
 * in the pattern, or 0 if the pattern does not contain
 * back references.
 *
 * Returns: the number of the highest back reference
 *
 * Since: 2.14
 */
gint
g_regex_get_max_backref (const GRegex *regex)
{
  uint32_t value;

  pcre2_pattern_info (regex->pcre_re, PCRE2_INFO_BACKREFMAX, &value);

  return value;
}

/**
 * g_regex_get_capture_count:
 * @regex: a #GRegex
 *
 * Returns the number of capturing subpatterns in the pattern.
 *
 * Returns: the number of capturing subpatterns
 *
 * Since: 2.14
 */
gint
g_regex_get_capture_count (const GRegex *regex)
{
  uint32_t value;

  pcre2_pattern_info (regex->pcre_re, PCRE2_INFO_CAPTURECOUNT, &value);

  return value;
}

/**
 * g_regex_get_has_cr_or_lf:
 * @regex: a #GRegex structure
 *
 * Checks whether the pattern contains explicit CR or LF references.
 *
 * Returns: %TRUE if the pattern contains explicit CR or LF references
 *
 * Since: 2.34
 */
gboolean
g_regex_get_has_cr_or_lf (const GRegex *regex)
{
  uint32_t value;

  pcre2_pattern_info (regex->pcre_re, PCRE2_INFO_HASCRORLF, &value);

  return !!value;
}

/**
 * g_regex_get_max_lookbehind:
 * @regex: a #GRegex structure
 *
 * Gets the number of characters in the longest lookbehind assertion in the
 * pattern. This information is useful when doing multi-segment matching using
 * the partial matching facilities.
 *
 * Returns: the number of characters in the longest lookbehind assertion.
 *
 * Since: 2.38
 */
gint
g_regex_get_max_lookbehind (const GRegex *regex)
{
  uint32_t max_lookbehind;

  pcre2_pattern_info (regex->pcre_re, PCRE2_INFO_MAXLOOKBEHIND,
                      &max_lookbehind);

  return max_lookbehind;
}

/**
 * g_regex_get_compile_flags:
 * @regex: a #GRegex
 *
 * Returns the compile options that @regex was created with.
 *
 * Depending on the version of PCRE that is used, this may or may not
 * include flags set by option expressions such as `(?i)` found at the
 * top-level within the compiled pattern.
 *
 * Returns: flags from #GRegexCompileFlags
 *
 * Since: 2.26
 */
GRegexCompileFlags
g_regex_get_compile_flags (const GRegex *regex)
{
  GRegexCompileFlags extra_flags;
  uint32_t info_value;

  g_return_val_if_fail (regex != NULL, 0);

  /* Preserve original G_REGEX_OPTIMIZE */
  extra_flags = (regex->regex_compile_opts & G_REGEX_OPTIMIZE);

  /* Also include the newline options */
  pcre2_pattern_info (regex->pcre_re, PCRE2_INFO_NEWLINE, &info_value);
  switch (info_value)
    {
    case PCRE2_NEWLINE_ANYCRLF:
      extra_flags |= G_REGEX_NEWLINE_ANYCRLF;
      break;
    case PCRE2_NEWLINE_CRLF:
      extra_flags |= G_REGEX_NEWLINE_CRLF;
      break;
    case PCRE2_NEWLINE_LF:
      extra_flags |= G_REGEX_NEWLINE_LF;
      break;
    case PCRE2_NEWLINE_CR:
      extra_flags |= G_REGEX_NEWLINE_CR;
      break;
    default:
      break;
    }

  /* Also include the bsr options */
  pcre2_pattern_info (regex->pcre_re, PCRE2_INFO_BSR, &info_value);
  switch (info_value)
    {
    case PCRE2_BSR_ANYCRLF:
      extra_flags |= G_REGEX_BSR_ANYCRLF;
      break;
    default:
      break;
    }

  return g_regex_compile_flags_from_pcre2 (regex->pcre2_compile_opts) | extra_flags;
}

/**
 * g_regex_get_match_flags:
 * @regex: a #GRegex
 *
 * Returns the match options that @regex was created with.
 *
 * Returns: flags from #GRegexMatchFlags
 *
 * Since: 2.26
 */
GRegexMatchFlags
g_regex_get_match_flags (const GRegex *regex)
{
  uint32_t flags;

  g_return_val_if_fail (regex != NULL, 0);

  flags = g_regex_match_flags_from_pcre2 (regex->match_opts);
  flags |= (regex->orig_match_opts & G_REGEX_MATCH_NEWLINE_MASK);
  flags |= (regex->orig_match_opts & (G_REGEX_MATCH_BSR_ANY | G_REGEX_MATCH_BSR_ANYCRLF));

  return flags;
}

/**
 * g_regex_match_simple:
 * @pattern: the regular expression
 * @string: the string to scan for matches
 * @compile_options: compile options for the regular expression, or 0
 * @match_options: match options, or 0
 *
 * Scans for a match in @string for @pattern.
 *
 * This function is equivalent to g_regex_match() but it does not
 * require to compile the pattern with g_regex_new(), avoiding some
 * lines of code when you need just to do a match without extracting
 * substrings, capture counts, and so on.
 *
 * If this function is to be called on the same @pattern more than
 * once, it's more efficient to compile the pattern once with
 * g_regex_new() and then use g_regex_match().
 *
 * Returns: %TRUE if the string matched, %FALSE otherwise
 *
 * Since: 2.14
 */
gboolean
g_regex_match_simple (const gchar        *pattern,
                      const gchar        *string,
                      GRegexCompileFlags  compile_options,
                      GRegexMatchFlags    match_options)
{
  GRegex *regex;
  gboolean result;

  regex = g_regex_new (pattern, compile_options, G_REGEX_MATCH_DEFAULT, NULL);
  if (!regex)
    return FALSE;
  result = g_regex_match_full (regex, string, -1, 0, match_options, NULL, NULL);
  g_regex_unref (regex);
  return result;
}

/**
 * g_regex_match:
 * @regex: a #GRegex structure from g_regex_new()
 * @string: the string to scan for matches
 * @match_options: match options
 * @match_info: (out) (optional): pointer to location where to store
 *     the #GMatchInfo, or %NULL if you do not need it
 *
 * Scans for a match in @string for the pattern in @regex.
 * The @match_options are combined with the match options specified
 * when the @regex structure was created, letting you have more
 * flexibility in reusing #GRegex structures.
 *
 * Unless %G_REGEX_RAW is specified in the options, @string must be valid UTF-8.
 *
 * A #GMatchInfo structure, used to get information on the match,
 * is stored in @match_info if not %NULL. Note that if @match_info
 * is not %NULL then it is created even if the function returns %FALSE,
 * i.e. you must free it regardless if regular expression actually matched.
 *
 * To retrieve all the non-overlapping matches of the pattern in
 * string you can use g_match_info_next().
 *
 * |[<!-- language="C" --> 
 * static void
 * print_uppercase_words (const gchar *string)
 * {
 *   // Print all uppercase-only words.
 *   GRegex *regex;
 *   GMatchInfo *match_info;
 *  
 *   regex = g_regex_new ("[A-Z]+", G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT, NULL);
 *   g_regex_match (regex, string, 0, &match_info);
 *   while (g_match_info_matches (match_info))
 *     {
 *       gchar *word = g_match_info_fetch (match_info, 0);
 *       g_print ("Found: %s\n", word);
 *       g_free (word);
 *       g_match_info_next (match_info, NULL);
 *     }
 *   g_match_info_free (match_info);
 *   g_regex_unref (regex);
 * }
 * ]|
 *
 * @string is not copied and is used in #GMatchInfo internally. If
 * you use any #GMatchInfo method (except g_match_info_free()) after
 * freeing or modifying @string then the behaviour is undefined.
 *
 * Returns: %TRUE is the string matched, %FALSE otherwise
 *
 * Since: 2.14
 */
gboolean
g_regex_match (const GRegex      *regex,
               const gchar       *string,
               GRegexMatchFlags   match_options,
               GMatchInfo       **match_info)
{
  return g_regex_match_full (regex, string, -1, 0, match_options,
                             match_info, NULL);
}

/**
 * g_regex_match_full:
 * @regex: a #GRegex structure from g_regex_new()
 * @string: the string to scan for matches
 * @string_len: the length of @string, in bytes, or -1 if @string is nul-terminated
 * @start_position: starting index of the string to match, in bytes
 * @match_options: match options
 * @match_info: (out) (optional): pointer to location where to store
 *     the #GMatchInfo, or %NULL if you do not need it
 * @error: location to store the error occurring, or %NULL to ignore errors
 *
 * Scans for a match in @string for the pattern in @regex.
 * The @match_options are combined with the match options specified
 * when the @regex structure was created, letting you have more
 * flexibility in reusing #GRegex structures.
 *
 * Setting @start_position differs from just passing over a shortened
 * string and setting %G_REGEX_MATCH_NOTBOL in the case of a pattern
 * that begins with any kind of lookbehind assertion, such as "\b".
 *
 * Unless %G_REGEX_RAW is specified in the options, @string must be valid UTF-8.
 *
 * A #GMatchInfo structure, used to get information on the match, is
 * stored in @match_info if not %NULL. Note that if @match_info is
 * not %NULL then it is created even if the function returns %FALSE,
 * i.e. you must free it regardless if regular expression actually
 * matched.
 *
 * @string is not copied and is used in #GMatchInfo internally. If
 * you use any #GMatchInfo method (except g_match_info_free()) after
 * freeing or modifying @string then the behaviour is undefined.
 *
 * To retrieve all the non-overlapping matches of the pattern in
 * string you can use g_match_info_next().
 *
 * |[<!-- language="C" --> 
 * static void
 * print_uppercase_words (const gchar *string)
 * {
 *   // Print all uppercase-only words.
 *   GRegex *regex;
 *   GMatchInfo *match_info;
 *   GError *error = NULL;
 *   
 *   regex = g_regex_new ("[A-Z]+", G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT, NULL);
 *   g_regex_match_full (regex, string, -1, 0, 0, &match_info, &error);
 *   while (g_match_info_matches (match_info))
 *     {
 *       gchar *word = g_match_info_fetch (match_info, 0);
 *       g_print ("Found: %s\n", word);
 *       g_free (word);
 *       g_match_info_next (match_info, &error);
 *     }
 *   g_match_info_free (match_info);
 *   g_regex_unref (regex);
 *   if (error != NULL)
 *     {
 *       g_printerr ("Error while matching: %s\n", error->message);
 *       g_error_free (error);
 *     }
 * }
 * ]|
 *
 * Returns: %TRUE is the string matched, %FALSE otherwise
 *
 * Since: 2.14
 */
gboolean
g_regex_match_full (const GRegex      *regex,
                    const gchar       *string,
                    gssize             string_len,
                    gint               start_position,
                    GRegexMatchFlags   match_options,
                    GMatchInfo       **match_info,
                    GError           **error)
{
  GMatchInfo *info;
  gboolean match_ok;
  size_t string_len_unsigned;

  g_return_val_if_fail (regex != NULL, FALSE);
  g_return_val_if_fail (string != NULL, FALSE);
  g_return_val_if_fail (start_position >= 0, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
  g_return_val_if_fail ((match_options & ~G_REGEX_MATCH_MASK) == 0, FALSE);

  string_len_unsigned = (string_len < 0) ? strlen (string) : (size_t) string_len;

  info = match_info_new (regex, string, string_len_unsigned, start_position,
                         match_options, FALSE);
  match_ok = g_match_info_next (info, error);
  if (match_info != NULL)
    *match_info = info;
  else
    g_match_info_free (info);

  return match_ok;
}

/**
 * g_regex_match_all:
 * @regex: a #GRegex structure from g_regex_new()
 * @string: the string to scan for matches
 * @match_options: match options
 * @match_info: (out) (optional): pointer to location where to store
 *     the #GMatchInfo, or %NULL if you do not need it
 *
 * Using the standard algorithm for regular expression matching only
 * the longest match in the string is retrieved. This function uses
 * a different algorithm so it can retrieve all the possible matches.
 * For more documentation see g_regex_match_all_full().
 *
 * A #GMatchInfo structure, used to get information on the match, is
 * stored in @match_info if not %NULL. Note that if @match_info is
 * not %NULL then it is created even if the function returns %FALSE,
 * i.e. you must free it regardless if regular expression actually
 * matched.
 *
 * @string is not copied and is used in #GMatchInfo internally. If
 * you use any #GMatchInfo method (except g_match_info_free()) after
 * freeing or modifying @string then the behaviour is undefined.
 *
 * Returns: %TRUE is the string matched, %FALSE otherwise
 *
 * Since: 2.14
 */
gboolean
g_regex_match_all (const GRegex      *regex,
                   const gchar       *string,
                   GRegexMatchFlags   match_options,
                   GMatchInfo       **match_info)
{
  return g_regex_match_all_full (regex, string, -1, 0, match_options,
                                 match_info, NULL);
}

/**
 * g_regex_match_all_full:
 * @regex: a #GRegex structure from g_regex_new()
 * @string: the string to scan for matches
 * @string_len: the length of @string, in bytes, or -1 if @string is nul-terminated
 * @start_position: starting index of the string to match, in bytes
 * @match_options: match options
 * @match_info: (out) (optional): pointer to location where to store
 *     the #GMatchInfo, or %NULL if you do not need it
 * @error: location to store the error occurring, or %NULL to ignore errors
 *
 * Using the standard algorithm for regular expression matching only
 * the longest match in the @string is retrieved, it is not possible
 * to obtain all the available matches. For instance matching
 * `"<a> <b> <c>"` against the pattern `"<.*>"`
 * you get `"<a> <b> <c>"`.
 *
 * This function uses a different algorithm (called DFA, i.e. deterministic
 * finite automaton), so it can retrieve all the possible matches, all
 * starting at the same point in the string. For instance matching
 * `"<a> <b> <c>"` against the pattern `"<.*>"`
 * you would obtain three matches: `"<a> <b> <c>"`,
 * `"<a> <b>"` and `"<a>"`.
 *
 * The number of matched strings is retrieved using
 * g_match_info_get_match_count(). To obtain the matched strings and
 * their position you can use, respectively, g_match_info_fetch() and
 * g_match_info_fetch_pos(). Note that the strings are returned in
 * reverse order of length; that is, the longest matching string is
 * given first.
 *
 * Note that the DFA algorithm is slower than the standard one and it
 * is not able to capture substrings, so backreferences do not work.
 *
 * Setting @start_position differs from just passing over a shortened
 * string and setting %G_REGEX_MATCH_NOTBOL in the case of a pattern
 * that begins with any kind of lookbehind assertion, such as "\b".
 *
 * Unless %G_REGEX_RAW is specified in the options, @string must be valid UTF-8.
 *
 * A #GMatchInfo structure, used to get information on the match, is
 * stored in @match_info if not %NULL. Note that if @match_info is
 * not %NULL then it is created even if the function returns %FALSE,
 * i.e. you must free it regardless if regular expression actually
 * matched.
 *
 * @string is not copied and is used in #GMatchInfo internally. If
 * you use any #GMatchInfo method (except g_match_info_free()) after
 * freeing or modifying @string then the behaviour is undefined.
 *
 * Returns: %TRUE is the string matched, %FALSE otherwise
 *
 * Since: 2.14
 */
gboolean
g_regex_match_all_full (const GRegex      *regex,
                        const gchar       *string,
                        gssize             string_len,
                        gint               start_position,
                        GRegexMatchFlags   match_options,
                        GMatchInfo       **match_info,
                        GError           **error)
{
  GMatchInfo *info;
  gboolean done;
  pcre2_code *pcre_re;
  gboolean retval;
  uint32_t newline_options;
  uint32_t bsr_options;
  size_t string_len_unsigned;

  g_return_val_if_fail (regex != NULL, FALSE);
  g_return_val_if_fail (string != NULL, FALSE);
  g_return_val_if_fail (start_position >= 0, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
  g_return_val_if_fail ((match_options & ~G_REGEX_MATCH_MASK) == 0, FALSE);

  string_len_unsigned = (string_len < 0) ? strlen (string) : (size_t) string_len;

  newline_options = get_pcre2_newline_match_options (match_options);
  if (!newline_options)
    newline_options = get_pcre2_newline_compile_options (regex->regex_compile_opts);

  bsr_options = get_pcre2_bsr_match_options (match_options);
  if (!bsr_options)
    bsr_options = get_pcre2_bsr_compile_options (regex->regex_compile_opts);

  /* For PCRE2 we need to turn off PCRE2_NO_AUTO_POSSESS, which is an
   * optimization for normal regex matching, but results in omitting some
   * shorter matches here, and an observable behaviour change.
   *
   * DFA matching is rather niche, and very rarely used according to
   * codesearch.debian.net, so don't bother caching the recompiled RE. */
  pcre_re = regex_compile (regex->pattern,
                           regex->pcre2_compile_opts | PCRE2_NO_AUTO_POSSESS,
                           newline_options, bsr_options, error);
  if (pcre_re == NULL)
    return FALSE;

  info = match_info_new (regex, string, string_len_unsigned, start_position,
                         match_options, TRUE);

  done = FALSE;
  while (!done)
    {
      done = TRUE;
      info->matches = pcre2_dfa_match (pcre_re,
                                       (PCRE2_SPTR8) info->string, info->string_len,
                                       info->pos,
                                       (regex->match_opts | info->match_opts),
                                       info->match_data,
                                       info->match_context,
                                       info->workspace, info->n_workspace);
      if (info->matches == PCRE2_ERROR_DFA_WSSIZE)
        {
          /* info->workspace is too small. */
          info->n_workspace *= 2;
          info->workspace = g_realloc_n (info->workspace,
                                         info->n_workspace,
                                         sizeof (gint));
          done = FALSE;
        }
      else if (info->matches == 0)
        {
          /* info->offsets is too small. */
          info->n_offsets *= 2;

          /* uint32_t is the type accepted by pcre2_match_data_create() */
          g_assert (info->n_offsets <= UINT32_MAX);

          info->offsets = g_realloc_n (info->offsets,
                                       info->n_offsets,
                                       sizeof (gint));
          pcre2_match_data_free (info->match_data);
          info->match_data = pcre2_match_data_create (info->n_offsets, NULL);
          done = FALSE;
        }
      else if (IS_PCRE2_ERROR (info->matches))
        {
          gchar *error_msg = get_match_error_message (info->matches);

          g_set_error (error, G_REGEX_ERROR, G_REGEX_ERROR_MATCH,
                       _("Error while matching regular expression %s: %s"),
                       regex->pattern, error_msg);
          g_clear_pointer (&error_msg, g_free);
        }
      else if (info->matches != PCRE2_ERROR_NOMATCH)
        {
          if (!recalc_match_offsets (info, error))
            info->matches = PCRE2_ERROR_NOMATCH;
        }
    }

  pcre2_code_free (pcre_re);

  /* don’t assert that (info->matches <= info->n_subpatterns + 1) as that only
   * holds true for a single match, rather than matching all */

  /* set info->pos_valid to false so that a call to g_match_info_next() fails. */
  info->pos_valid = FALSE;
  retval = info->matches >= 0;

  if (match_info != NULL)
    *match_info = info;
  else
    g_match_info_free (info);

  return retval;
}

/**
 * g_regex_get_string_number:
 * @regex: #GRegex structure
 * @name: name of the subexpression
 *
 * Retrieves the number of the subexpression named @name.
 *
 * Returns: The number of the subexpression or -1 if @name
 *   does not exists
 *
 * Since: 2.14
 */
gint
g_regex_get_string_number (const GRegex *regex,
                           const gchar  *name)
{
  gint num;

  g_return_val_if_fail (regex != NULL, -1);
  g_return_val_if_fail (name != NULL, -1);

  num = pcre2_substring_number_from_name (regex->pcre_re, (PCRE2_SPTR8) name);
  if (num == PCRE2_ERROR_NOSUBSTRING)
    num = -1;

  return num;
}

/**
 * g_regex_split_simple:
 * @pattern: the regular expression
 * @string: the string to scan for matches
 * @compile_options: compile options for the regular expression, or 0
 * @match_options: match options, or 0
 *
 * Breaks the string on the pattern, and returns an array of
 * the tokens. If the pattern contains capturing parentheses,
 * then the text for each of the substrings will also be returned.
 * If the pattern does not match anywhere in the string, then the
 * whole string is returned as the first token.
 *
 * This function is equivalent to g_regex_split() but it does
 * not require to compile the pattern with g_regex_new(), avoiding
 * some lines of code when you need just to do a split without
 * extracting substrings, capture counts, and so on.
 *
 * If this function is to be called on the same @pattern more than
 * once, it's more efficient to compile the pattern once with
 * g_regex_new() and then use g_regex_split().
 *
 * As a special case, the result of splitting the empty string ""
 * is an empty vector, not a vector containing a single string.
 * The reason for this special case is that being able to represent
 * an empty vector is typically more useful than consistent handling
 * of empty elements. If you do need to represent empty elements,
 * you'll need to check for the empty string before calling this
 * function.
 *
 * A pattern that can match empty strings splits @string into
 * separate characters wherever it matches the empty string between
 * characters. For example splitting "ab c" using as a separator
 * "\s*", you will get "a", "b" and "c".
 *
 * Returns: (transfer full): a %NULL-terminated array of strings. Free
 * it using g_strfreev()
 *
 * Since: 2.14
 **/
gchar **
g_regex_split_simple (const gchar        *pattern,
                      const gchar        *string,
                      GRegexCompileFlags  compile_options,
                      GRegexMatchFlags    match_options)
{
  GRegex *regex;
  gchar **result;

  regex = g_regex_new (pattern, compile_options, 0, NULL);
  if (!regex)
    return NULL;

  result = g_regex_split_full (regex, string, -1, 0, match_options, 0, NULL);
  g_regex_unref (regex);
  return result;
}

/**
 * g_regex_split:
 * @regex: a #GRegex structure
 * @string: the string to split with the pattern
 * @match_options: match time option flags
 *
 * Breaks the string on the pattern, and returns an array of the tokens.
 * If the pattern contains capturing parentheses, then the text for each
 * of the substrings will also be returned. If the pattern does not match
 * anywhere in the string, then the whole string is returned as the first
 * token.
 *
 * As a special case, the result of splitting the empty string "" is an
 * empty vector, not a vector containing a single string. The reason for
 * this special case is that being able to represent an empty vector is
 * typically more useful than consistent handling of empty elements. If
 * you do need to represent empty elements, you'll need to check for the
 * empty string before calling this function.
 *
 * A pattern that can match empty strings splits @string into separate
 * characters wherever it matches the empty string between characters.
 * For example splitting "ab c" using as a separator "\s*", you will get
 * "a", "b" and "c".
 *
 * Returns: (transfer full): a %NULL-terminated gchar ** array. Free
 * it using g_strfreev()
 *
 * Since: 2.14
 **/
gchar **
g_regex_split (const GRegex     *regex,
               const gchar      *string,
               GRegexMatchFlags  match_options)
{
  return g_regex_split_full (regex, string, -1, 0,
                             match_options, 0, NULL);
}

/**
 * g_regex_split_full:
 * @regex: a #GRegex structure
 * @string: the string to split with the pattern
 * @string_len: the length of @string, in bytes, or -1 if @string is nul-terminated
 * @start_position: starting index of the string to match, in bytes
 * @match_options: match time option flags
 * @max_tokens: the maximum number of tokens to split @string into.
 *   If this is less than 1, the string is split completely
 * @error: return location for a #GError
 *
 * Breaks the string on the pattern, and returns an array of the tokens.
 * If the pattern contains capturing parentheses, then the text for each
 * of the substrings will also be returned. If the pattern does not match
 * anywhere in the string, then the whole string is returned as the first
 * token.
 *
 * As a special case, the result of splitting the empty string "" is an
 * empty vector, not a vector containing a single string. The reason for
 * this special case is that being able to represent an empty vector is
 * typically more useful than consistent handling of empty elements. If
 * you do need to represent empty elements, you'll need to check for the
 * empty string before calling this function.
 *
 * A pattern that can match empty strings splits @string into separate
 * characters wherever it matches the empty string between characters.
 * For example splitting "ab c" using as a separator "\s*", you will get
 * "a", "b" and "c".
 *
 * Setting @start_position differs from just passing over a shortened
 * string and setting %G_REGEX_MATCH_NOTBOL in the case of a pattern
 * that begins with any kind of lookbehind assertion, such as "\b".
 *
 * Returns: (transfer full): a %NULL-terminated gchar ** array. Free
 * it using g_strfreev()
 *
 * Since: 2.14
 **/
gchar **
g_regex_split_full (const GRegex      *regex,
                    const gchar       *string,
                    gssize             string_len,
                    gint               start_position,
                    GRegexMatchFlags   match_options,
                    gint               max_tokens,
                    GError           **error)
{
  GError *tmp_error = NULL;
  GMatchInfo *match_info;
  GList *list, *last;
  gint i;
  gint token_count;
  gboolean match_ok;
  /* position of the last separator. */
  size_t last_separator_end;
  /* was the last match 0 bytes long? */
  gboolean last_match_is_empty;
  /* the returned array of char **s */
  gchar **string_list;
  size_t string_len_unsigned, start_position_unsigned;

  g_return_val_if_fail (regex != NULL, NULL);
  g_return_val_if_fail (string != NULL, NULL);
  g_return_val_if_fail (start_position >= 0, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  g_return_val_if_fail ((match_options & ~G_REGEX_MATCH_MASK) == 0, NULL);

  if (max_tokens <= 0)
    max_tokens = G_MAXINT;

  string_len_unsigned = (string_len < 0) ? strlen (string) : (size_t) string_len;
  start_position_unsigned = (size_t) start_position;  /* see pre-condition above */

  /* zero-length string */
  if (string_len_unsigned - start_position_unsigned == 0)
    return g_new0 (gchar *, 1);

  if (max_tokens == 1)
    {
      string_list = g_new0 (gchar *, 2);
      string_list[0] = g_strndup (&string[start_position_unsigned],
                                  string_len_unsigned - start_position_unsigned);
      return string_list;
    }

  list = NULL;
  token_count = 0;
  last_separator_end = start_position_unsigned;
  last_match_is_empty = FALSE;

  match_ok = g_regex_match_full (regex, string, string_len_unsigned, start_position_unsigned,
                                 match_options, &match_info, &tmp_error);

  while (tmp_error == NULL)
    {
      if (match_ok)
        {
          last_match_is_empty =
                    (match_info->offsets[0] == match_info->offsets[1]);

          /* we need to skip empty separators at the same position of the end
           * of another separator. e.g. the string is "a b" and the separator
           * is " *", so from 1 to 2 we have a match and at position 2 we have
           * an empty match. */
          g_assert (match_info->offsets[1] >= 0);
          if (last_separator_end != (size_t) match_info->offsets[1])
            {
              gchar *token;
              gint match_count;

              token = g_strndup (string + last_separator_end,
                                 match_info->offsets[0] - last_separator_end);
              list = g_list_prepend (list, token);
              token_count++;

              /* if there were substrings, these need to be added to
               * the list. */
              match_count = g_match_info_get_match_count (match_info);
              if (match_count > 1)
                {
                  for (i = 1; i < match_count; i++)
                    list = g_list_prepend (list, g_match_info_fetch (match_info, i));
                }
            }
        }
      else
        {
          /* if there was no match, copy to end of string. */
          if (!last_match_is_empty)
            {
              gchar *token = g_strndup (string + last_separator_end,
                                        match_info->string_len - last_separator_end);
              list = g_list_prepend (list, token);
            }
          /* no more tokens, end the loop. */
          break;
        }

      /* -1 to leave room for the last part. */
      if (token_count >= max_tokens - 1)
        {
          /* we have reached the maximum number of tokens, so we copy
           * the remaining part of the string. */
          if (last_match_is_empty)
            {
              /* the last match was empty, so we have moved one char
               * after the real position to avoid empty matches at the
               * same position. */
              const char *prev_char = PREV_CHAR (regex, &string[match_info->pos]);
              g_assert (prev_char >= string);
              match_info->pos = prev_char - string;
              match_info->pos_valid = TRUE;
            }

          g_assert (match_info->pos_valid);

          /* the if is needed in the case we have terminated the available
           * tokens, but we are at the end of the string, so there are no
           * characters left to copy. */
          if (string_len_unsigned > match_info->pos)
            {
              gchar *token = g_strndup (string + match_info->pos,
                                        string_len_unsigned - match_info->pos);
              list = g_list_prepend (list, token);
            }
          /* end the loop. */
          break;
        }

      last_separator_end = match_info->pos;
      if (last_match_is_empty)
        /* if the last match was empty, g_match_info_next() has moved
         * forward to avoid infinite loops, but we still need to copy that
         * character. */
        last_separator_end = PREV_CHAR (regex, &string[last_separator_end]) - string;

      match_ok = g_match_info_next (match_info, &tmp_error);
    }
  g_match_info_free (match_info);
  if (tmp_error != NULL)
    {
      g_propagate_error (error, tmp_error);
      g_list_free_full (list, g_free);
      return NULL;
    }

  string_list = g_new (gchar *, g_list_length (list) + 1);
  i = 0;
  for (last = g_list_last (list); last; last = g_list_previous (last))
    string_list[i++] = last->data;
  string_list[i] = NULL;
  g_list_free (list);

  return string_list;
}

enum
{
  REPL_TYPE_STRING,
  REPL_TYPE_CHARACTER,
  REPL_TYPE_SYMBOLIC_REFERENCE,
  REPL_TYPE_NUMERIC_REFERENCE,
  REPL_TYPE_CHANGE_CASE
};

typedef enum
{
  CHANGE_CASE_NONE         = 1 << 0,
  CHANGE_CASE_UPPER        = 1 << 1,
  CHANGE_CASE_LOWER        = 1 << 2,
  CHANGE_CASE_UPPER_SINGLE = 1 << 3,
  CHANGE_CASE_LOWER_SINGLE = 1 << 4,
  CHANGE_CASE_SINGLE_MASK  = CHANGE_CASE_UPPER_SINGLE | CHANGE_CASE_LOWER_SINGLE,
  CHANGE_CASE_LOWER_MASK   = CHANGE_CASE_LOWER | CHANGE_CASE_LOWER_SINGLE,
  CHANGE_CASE_UPPER_MASK   = CHANGE_CASE_UPPER | CHANGE_CASE_UPPER_SINGLE
} G_GNUC_FLAG_ENUM ChangeCase;

struct _InterpolationData
{
  gchar     *text;
  gint       type;
  gint       num;
  gchar      c;
  ChangeCase change_case;
};

static void
free_interpolation_data (InterpolationData *data)
{
  g_free (data->text);
  g_free (data);
}

static const gchar *
expand_escape (const gchar        *replacement,
               const gchar        *p,
               InterpolationData  *data,
               GError            **error)
{
  const gchar *q, *r;
  gint x, d, h, i;
  const gchar *error_detail;
  gint base = 0;
  GError *tmp_error = NULL;

  p++;
  switch (*p)
    {
    case 't':
      p++;
      data->c = '\t';
      data->type = REPL_TYPE_CHARACTER;
      break;
    case 'n':
      p++;
      data->c = '\n';
      data->type = REPL_TYPE_CHARACTER;
      break;
    case 'v':
      p++;
      data->c = '\v';
      data->type = REPL_TYPE_CHARACTER;
      break;
    case 'r':
      p++;
      data->c = '\r';
      data->type = REPL_TYPE_CHARACTER;
      break;
    case 'f':
      p++;
      data->c = '\f';
      data->type = REPL_TYPE_CHARACTER;
      break;
    case 'a':
      p++;
      data->c = '\a';
      data->type = REPL_TYPE_CHARACTER;
      break;
    case 'b':
      p++;
      data->c = '\b';
      data->type = REPL_TYPE_CHARACTER;
      break;
    case '\\':
      p++;
      data->c = '\\';
      data->type = REPL_TYPE_CHARACTER;
      break;
    case 'x':
      p++;
      x = 0;
      if (*p == '{')
        {
          p++;
          do
            {
              h = g_ascii_xdigit_value (*p);
              if (h < 0)
                {
                  error_detail = _("hexadecimal digit or “}” expected");
                  goto error;
                }
              x = x * 16 + h;
              p++;
            }
          while (*p != '}');
          p++;
        }
      else
        {
          for (i = 0; i < 2; i++)
            {
              h = g_ascii_xdigit_value (*p);
              if (h < 0)
                {
                  error_detail = _("hexadecimal digit expected");
                  goto error;
                }
              x = x * 16 + h;
              p++;
            }
        }
      data->type = REPL_TYPE_STRING;
      data->text = g_new0 (gchar, 8);
      g_unichar_to_utf8 (x, data->text);
      break;
    case 'l':
      p++;
      data->type = REPL_TYPE_CHANGE_CASE;
      data->change_case = CHANGE_CASE_LOWER_SINGLE;
      break;
    case 'u':
      p++;
      data->type = REPL_TYPE_CHANGE_CASE;
      data->change_case = CHANGE_CASE_UPPER_SINGLE;
      break;
    case 'L':
      p++;
      data->type = REPL_TYPE_CHANGE_CASE;
      data->change_case = CHANGE_CASE_LOWER;
      break;
    case 'U':
      p++;
      data->type = REPL_TYPE_CHANGE_CASE;
      data->change_case = CHANGE_CASE_UPPER;
      break;
    case 'E':
      p++;
      data->type = REPL_TYPE_CHANGE_CASE;
      data->change_case = CHANGE_CASE_NONE;
      break;
    case 'g':
      p++;
      if (*p != '<')
        {
          error_detail = _("missing “<” in symbolic reference");
          goto error;
        }
      q = p + 1;
      do
        {
          p++;
          if (!*p)
            {
              error_detail = _("unfinished symbolic reference");
              goto error;
            }
        }
      while (*p != '>');
      if (p - q == 0)
        {
          error_detail = _("zero-length symbolic reference");
          goto error;
        }
      if (g_ascii_isdigit (*q))
        {
          x = 0;
          do
            {
              h = g_ascii_digit_value (*q);
              if (h < 0)
                {
                  error_detail = _("digit expected");
                  p = q;
                  goto error;
                }
              x = x * 10 + h;
              q++;
            }
          while (q != p);
          data->num = x;
          data->type = REPL_TYPE_NUMERIC_REFERENCE;
        }
      else
        {
          r = q;
          do
            {
              if (!g_ascii_isalnum (*r))
                {
                  error_detail = _("illegal symbolic reference");
                  p = r;
                  goto error;
                }
              r++;
            }
          while (r != p);
          data->text = g_strndup (q, p - q);
          data->type = REPL_TYPE_SYMBOLIC_REFERENCE;
        }
      p++;
      break;
    case '0':
      /* if \0 is followed by a number is an octal number representing a
       * character, else it is a numeric reference. */
      if (g_ascii_digit_value (*g_utf8_next_char (p)) >= 0)
        {
          base = 8;
          p = g_utf8_next_char (p);
        }
      G_GNUC_FALLTHROUGH;
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      x = 0;
      d = 0;
      for (i = 0; i < 3; i++)
        {
          h = g_ascii_digit_value (*p);
          if (h < 0)
            break;
          if (h > 7)
            {
              if (base == 8)
                break;
              else
                base = 10;
            }
          if (i == 2 && base == 10)
            break;
          x = x * 8 + h;
          d = d * 10 + h;
          p++;
        }
      if (base == 8 || i == 3)
        {
          data->type = REPL_TYPE_STRING;
          data->text = g_new0 (gchar, 8);
          g_unichar_to_utf8 (x, data->text);
        }
      else
        {
          data->type = REPL_TYPE_NUMERIC_REFERENCE;
          data->num = d;
        }
      break;
    case 0:
      error_detail = _("stray final “\\”");
      goto error;
      break;
    default:
      error_detail = _("unknown escape sequence");
      goto error;
    }

  return p;

 error:
  /* G_GSSIZE_FORMAT doesn't work with gettext, so we use %lu */
  tmp_error = g_error_new (G_REGEX_ERROR,
                           G_REGEX_ERROR_REPLACE,
                           _("Error while parsing replacement "
                             "text “%s” at char %lu: %s"),
                           replacement,
                           (gulong)(p - replacement),
                           error_detail);
  g_propagate_error (error, tmp_error);

  return NULL;
}

static GList *
split_replacement (const gchar  *replacement,
                   GError      **error)
{
  GList *list = NULL;
  InterpolationData *data;
  const gchar *p, *start;

  start = p = replacement;
  while (*p)
    {
      if (*p == '\\')
        {
          data = g_new0 (InterpolationData, 1);
          start = p = expand_escape (replacement, p, data, error);
          if (p == NULL)
            {
              g_list_free_full (list, (GDestroyNotify) free_interpolation_data);
              free_interpolation_data (data);

              return NULL;
            }
          list = g_list_prepend (list, data);
        }
      else
        {
          p++;
          if (*p == '\\' || *p == '\0')
            {
              if (p - start > 0)
                {
                  data = g_new0 (InterpolationData, 1);
                  data->text = g_strndup (start, p - start);
                  data->type = REPL_TYPE_STRING;
                  list = g_list_prepend (list, data);
                }
            }
        }
    }

  return g_list_reverse (list);
}

/* Change the case of c based on change_case.
 * g_ascii_to*() will happily pass through non-ASCII bytes unchanged. */
#define UTF8_CHANGE_CASE(c, change_case) \
        (((change_case) & CHANGE_CASE_LOWER_MASK) ? \
                g_unichar_tolower (c) : \
                g_unichar_toupper (c))
#define RAW_CHANGE_CASE(c, change_case) \
        (((change_case) & CHANGE_CASE_LOWER_MASK) ? \
                g_ascii_tolower (c) : \
                g_ascii_toupper (c))

/* If @text_is_raw is set, @text might not be valid UTF-8 (but will be
 * nul-terminated). */
static void
string_append (GString     *string,
               const gchar *text,
               gboolean     text_is_raw,
               ChangeCase  *change_case)
{
  if (text[0] == '\0')
    return;

  if (*change_case == CHANGE_CASE_NONE)
    {
      g_string_append (string, text);
    }
  else if (*change_case & CHANGE_CASE_SINGLE_MASK)
    {
      if (!text_is_raw)
        {
          gunichar c = g_utf8_get_char (text);
          g_string_append_unichar (string, UTF8_CHANGE_CASE (c, *change_case));
          g_string_append (string, g_utf8_next_char (text));
        }
      else
        {
          g_string_append_c (string, RAW_CHANGE_CASE (text[0], *change_case));
          g_string_append (string, text + 1);
        }

      *change_case = CHANGE_CASE_NONE;
    }
  else
    {
      if (!text_is_raw)
        {
          while (*text != '\0')
            {
              gunichar c = g_utf8_get_char (text);
              g_string_append_unichar (string, UTF8_CHANGE_CASE (c, *change_case));
              text = g_utf8_next_char (text);
            }
        }
      else
        {
          while (*text != '\0')
            {
              char c = *text;
              g_string_append_c (string, RAW_CHANGE_CASE (c, *change_case));
              text++;
            }
        }
    }
}

/* @match_info is (nullable) */
static gboolean
interpolate_replacement (const GMatchInfo *match_info,
                         GString          *result,
                         gpointer          data)
{
  GList *list;
  InterpolationData *idata;
  gchar *match;
  ChangeCase change_case = CHANGE_CASE_NONE;
  gboolean is_raw = (match_info != NULL && (match_info->regex->regex_compile_opts & G_REGEX_RAW));

  for (list = data; list; list = list->next)
    {
      idata = list->data;
      switch (idata->type)
        {
        case REPL_TYPE_STRING:
          string_append (result, idata->text, is_raw, &change_case);
          break;
        case REPL_TYPE_CHARACTER:
          g_string_append_c (result, UTF8_CHANGE_CASE (idata->c, change_case));
          if (change_case & CHANGE_CASE_SINGLE_MASK)
            change_case = CHANGE_CASE_NONE;
          break;
        case REPL_TYPE_NUMERIC_REFERENCE:
          match = g_match_info_fetch (match_info, idata->num);
          if (match)
            {
              string_append (result, match, is_raw, &change_case);
              g_free (match);
            }
          break;
        case REPL_TYPE_SYMBOLIC_REFERENCE:
          match = g_match_info_fetch_named (match_info, idata->text);
          if (match)
            {
              string_append (result, match, is_raw, &change_case);
              g_free (match);
            }
          break;
        case REPL_TYPE_CHANGE_CASE:
          change_case = idata->change_case;
          break;
        }
    }

  return FALSE;
}

/* whether actual match_info is needed for replacement, i.e.
 * whether there are references
 */
static gboolean
interpolation_list_needs_match (GList *list)
{
  while (list != NULL)
    {
      InterpolationData *data = list->data;

      if (data->type == REPL_TYPE_SYMBOLIC_REFERENCE ||
          data->type == REPL_TYPE_NUMERIC_REFERENCE)
        {
          return TRUE;
        }

      list = list->next;
    }

  return FALSE;
}

/**
 * g_regex_replace:
 * @regex: a #GRegex structure
 * @string: the string to perform matches against
 * @string_len: the length of @string, in bytes, or -1 if @string is nul-terminated
 * @start_position: starting index of the string to match, in bytes
 * @replacement: text to replace each match with
 * @match_options: options for the match
 * @error: location to store the error occurring, or %NULL to ignore errors
 *
 * Replaces all occurrences of the pattern in @regex with the
 * replacement text. Backreferences of the form `\number` or
 * `\g<number>` in the replacement text are interpolated by the
 * number-th captured subexpression of the match, `\g<name>` refers
 * to the captured subexpression with the given name. `\0` refers
 * to the complete match, but `\0` followed by a number is the octal
 * representation of a character. To include a literal `\` in the
 * replacement, write `\\\\`.
 *
 * There are also escapes that changes the case of the following text:
 *
 * - `\l`: Convert to lower case the next character
 * - `\u`: Convert to upper case the next character
 * - `\L`: Convert to lower case until the next `\E`
 * - `\U`: Convert to upper case until the next `\E`
 * - `\E`: End case modification
 *
 * If you do not need to use backreferences use g_regex_replace_literal().
 *
 * The @replacement string must be UTF-8 encoded even if %G_REGEX_RAW was
 * passed to g_regex_new(). If you want to use not UTF-8 encoded strings
 * you can use g_regex_replace_literal().
 *
 * Setting @start_position differs from just passing over a shortened
 * string and setting %G_REGEX_MATCH_NOTBOL in the case of a pattern that
 * begins with any kind of lookbehind assertion, such as `"\b"`.
 *
 * Returns: a newly allocated string containing the replacements
 *
 * Since: 2.14
 */
gchar *
g_regex_replace (const GRegex      *regex,
                 const gchar       *string,
                 gssize             string_len,
                 gint               start_position,
                 const gchar       *replacement,
                 GRegexMatchFlags   match_options,
                 GError           **error)
{
  gchar *result;
  GList *list;
  GError *tmp_error = NULL;

  g_return_val_if_fail (regex != NULL, NULL);
  g_return_val_if_fail (string != NULL, NULL);
  g_return_val_if_fail (start_position >= 0, NULL);
  g_return_val_if_fail (replacement != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  g_return_val_if_fail ((match_options & ~G_REGEX_MATCH_MASK) == 0, NULL);

  list = split_replacement (replacement, &tmp_error);
  if (tmp_error != NULL)
    {
      g_propagate_error (error, tmp_error);
      return NULL;
    }

  result = g_regex_replace_eval (regex,
                                 string, string_len, start_position,
                                 match_options,
                                 interpolate_replacement,
                                 (gpointer)list,
                                 &tmp_error);
  if (tmp_error != NULL)
    g_propagate_error (error, tmp_error);

  g_list_free_full (list, (GDestroyNotify) free_interpolation_data);

  return result;
}

static gboolean
literal_replacement (const GMatchInfo *match_info,
                     GString          *result,
                     gpointer          data)
{
  g_string_append (result, data);
  return FALSE;
}

/**
 * g_regex_replace_literal:
 * @regex: a #GRegex structure
 * @string: the string to perform matches against
 * @string_len: the length of @string, in bytes, or -1 if @string is nul-terminated
 * @start_position: starting index of the string to match, in bytes
 * @replacement: text to replace each match with
 * @match_options: options for the match
 * @error: location to store the error occurring, or %NULL to ignore errors
 *
 * Replaces all occurrences of the pattern in @regex with the
 * replacement text. @replacement is replaced literally, to
 * include backreferences use g_regex_replace().
 *
 * Setting @start_position differs from just passing over a
 * shortened string and setting %G_REGEX_MATCH_NOTBOL in the
 * case of a pattern that begins with any kind of lookbehind
 * assertion, such as "\b".
 *
 * Returns: a newly allocated string containing the replacements
 *
 * Since: 2.14
 */
gchar *
g_regex_replace_literal (const GRegex      *regex,
                         const gchar       *string,
                         gssize             string_len,
                         gint               start_position,
                         const gchar       *replacement,
                         GRegexMatchFlags   match_options,
                         GError           **error)
{
  g_return_val_if_fail (replacement != NULL, NULL);
  g_return_val_if_fail ((match_options & ~G_REGEX_MATCH_MASK) == 0, NULL);

  return g_regex_replace_eval (regex,
                               string, string_len, start_position,
                               match_options,
                               literal_replacement,
                               (gpointer)replacement,
                               error);
}

/**
 * g_regex_replace_eval:
 * @regex: a #GRegex structure from g_regex_new()
 * @string: string to perform matches against
 * @string_len: the length of @string, in bytes, or -1 if @string is nul-terminated
 * @start_position: starting index of the string to match, in bytes
 * @match_options: options for the match
 * @eval: (scope call): a function to call for each match
 * @user_data: user data to pass to the function
 * @error: location to store the error occurring, or %NULL to ignore errors
 *
 * Replaces occurrences of the pattern in regex with the output of
 * @eval for that occurrence.
 *
 * Setting @start_position differs from just passing over a shortened
 * string and setting %G_REGEX_MATCH_NOTBOL in the case of a pattern
 * that begins with any kind of lookbehind assertion, such as "\b".
 *
 * The following example uses g_regex_replace_eval() to replace multiple
 * strings at once:
 * |[<!-- language="C" --> 
 * static gboolean
 * eval_cb (const GMatchInfo *info,
 *          GString          *res,
 *          gpointer          data)
 * {
 *   gchar *match;
 *   gchar *r;
 *
 *    match = g_match_info_fetch (info, 0);
 *    r = g_hash_table_lookup ((GHashTable *)data, match);
 *    g_string_append (res, r);
 *    g_free (match);
 *
 *    return FALSE;
 * }
 *
 * ...
 *
 * GRegex *reg;
 * GHashTable *h;
 * gchar *res;
 *
 * h = g_hash_table_new (g_str_hash, g_str_equal);
 *
 * g_hash_table_insert (h, "1", "ONE");
 * g_hash_table_insert (h, "2", "TWO");
 * g_hash_table_insert (h, "3", "THREE");
 * g_hash_table_insert (h, "4", "FOUR");
 *
 * reg = g_regex_new ("1|2|3|4", G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT, NULL);
 * res = g_regex_replace_eval (reg, text, -1, 0, 0, eval_cb, h, NULL);
 * g_hash_table_destroy (h);
 *
 * ...
 * ]|
 *
 * Returns: a newly allocated string containing the replacements
 *
 * Since: 2.14
 */
gchar *
g_regex_replace_eval (const GRegex        *regex,
                      const gchar         *string,
                      gssize               string_len,
                      gint                 start_position,
                      GRegexMatchFlags     match_options,
                      GRegexEvalCallback   eval,
                      gpointer             user_data,
                      GError             **error)
{
  GMatchInfo *match_info;
  GString *result;
  size_t str_pos = 0;
  gboolean done = FALSE;
  GError *tmp_error = NULL;
  size_t string_len_unsigned;

  g_return_val_if_fail (regex != NULL, NULL);
  g_return_val_if_fail (string != NULL, NULL);
  g_return_val_if_fail (start_position >= 0, NULL);
  g_return_val_if_fail (eval != NULL, NULL);
  g_return_val_if_fail ((match_options & ~G_REGEX_MATCH_MASK) == 0, NULL);

  string_len_unsigned = (string_len < 0) ? strlen (string) : (size_t) string_len;

  result = g_string_sized_new (string_len_unsigned);

  /* run down the string making matches. */
  g_regex_match_full (regex, string, string_len_unsigned, start_position,
                      match_options, &match_info, &tmp_error);
  while (!done && g_match_info_matches (match_info))
    {
      g_string_append_len (result,
                           string + str_pos,
                           match_info->offsets[0] - str_pos);
      done = (*eval) (match_info, result, user_data);
      str_pos = match_info->offsets[1];
      g_match_info_next (match_info, &tmp_error);
    }
  g_match_info_free (match_info);
  if (tmp_error != NULL)
    {
      g_propagate_error (error, tmp_error);
      g_string_free (result, TRUE);
      return NULL;
    }

  g_string_append_len (result, string + str_pos, string_len_unsigned - str_pos);
  return g_string_free (result, FALSE);
}

/**
 * g_regex_check_replacement:
 * @replacement: the replacement string
 * @has_references: (out) (optional): location to store information about
 *   references in @replacement or %NULL
 * @error: location to store error
 *
 * Checks whether @replacement is a valid replacement string
 * (see g_regex_replace()), i.e. that all escape sequences in
 * it are valid.
 *
 * If @has_references is not %NULL then @replacement is checked
 * for pattern references. For instance, replacement text 'foo\n'
 * does not contain references and may be evaluated without information
 * about actual match, but '\0\1' (whole match followed by first
 * subpattern) requires valid #GMatchInfo object.
 *
 * Returns: whether @replacement is a valid replacement string
 *
 * Since: 2.14
 */
gboolean
g_regex_check_replacement (const gchar  *replacement,
                           gboolean     *has_references,
                           GError      **error)
{
  GList *list;
  GError *tmp = NULL;

  list = split_replacement (replacement, &tmp);

  if (tmp)
  {
    g_propagate_error (error, tmp);
    return FALSE;
  }

  if (has_references)
    *has_references = interpolation_list_needs_match (list);

  g_list_free_full (list, (GDestroyNotify) free_interpolation_data);

  return TRUE;
}

/**
 * g_regex_escape_nul:
 * @string: the string to escape
 * @length: the length of @string
 *
 * Escapes the nul characters in @string to "\x00".  It can be used
 * to compile a regex with embedded nul characters.
 *
 * For completeness, @length can be -1 for a nul-terminated string.
 * In this case the output string will be of course equal to @string.
 *
 * Returns: a newly-allocated escaped string
 *
 * Since: 2.30
 */
gchar *
g_regex_escape_nul (const gchar *string,
                    gint         length)
{
  GString *escaped;
  const gchar *p, *piece_start, *end;
  gint backslashes;

  g_return_val_if_fail (string != NULL, NULL);

  if (length < 0)
    return g_strdup (string);

  end = string + length;
  p = piece_start = string;
  escaped = g_string_sized_new (length + 1);

  backslashes = 0;
  while (p < end)
    {
      switch (*p)
        {
        case '\0':
          if (p != piece_start)
            {
              /* copy the previous piece. */
              g_string_append_len (escaped, piece_start, p - piece_start);
            }
          if ((backslashes & 1) == 0)
            g_string_append_c (escaped, '\\');
          g_string_append_c (escaped, 'x');
          g_string_append_c (escaped, '0');
          g_string_append_c (escaped, '0');
          piece_start = ++p;
          backslashes = 0;
          break;
        case '\\':
          backslashes++;
          ++p;
          break;
        default:
          backslashes = 0;
          p = g_utf8_next_char (p);
          break;
        }
    }

  if (piece_start < end)
    g_string_append_len (escaped, piece_start, end - piece_start);

  return g_string_free (escaped, FALSE);
}

/**
 * g_regex_escape_string:
 * @string: the string to escape
 * @length: the length of @string, in bytes, or -1 if @string is nul-terminated
 *
 * Escapes the special characters used for regular expressions
 * in @string, for instance "a.b*c" becomes "a\.b\*c". This
 * function is useful to dynamically generate regular expressions.
 *
 * @string can contain nul characters that are replaced with "\0",
 * in this case remember to specify the correct length of @string
 * in @length.
 *
 * Returns: a newly-allocated escaped string
 *
 * Since: 2.14
 */
gchar *
g_regex_escape_string (const gchar *string,
                       gint         length)
{
  GString *escaped;
  const char *p, *piece_start, *end;
  size_t length_unsigned;

  g_return_val_if_fail (string != NULL, NULL);

  length_unsigned = (length < 0) ? strlen (string) : (size_t) length;

  end = string + length_unsigned;
  p = piece_start = string;
  escaped = g_string_sized_new (length_unsigned + 1);

  while (p < end)
    {
      switch (*p)
        {
        case '\0':
        case '\\':
        case '|':
        case '(':
        case ')':
        case '[':
        case ']':
        case '{':
        case '}':
        case '^':
        case '$':
        case '*':
        case '+':
        case '?':
        case '.':
          if (p != piece_start)
            /* copy the previous piece. */
            g_string_append_len (escaped, piece_start, p - piece_start);
          g_string_append_c (escaped, '\\');
          if (*p == '\0')
            g_string_append_c (escaped, '0');
          else
            g_string_append_c (escaped, *p);
          piece_start = ++p;
          break;
        default:
          p = g_utf8_next_char (p);
          break;
        }
  }

  if (piece_start < end)
    g_string_append_len (escaped, piece_start, end - piece_start);

  return g_string_free (escaped, FALSE);
}
