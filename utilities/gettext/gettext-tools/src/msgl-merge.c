/* Merging a .po file with a .pot file.
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

#include <config.h>
#include <alloca.h>

/* Specification.  */
#include "msgl-merge.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "message.h"
#include "read-catalog-file.h"
#include "msgl-charset.h"
#include "format.h"
#include "xalloc.h"
#include "xmalloca.h"
#include "obstack.h"
#include "c-strstr.h"
#include "c-strcase.h"
#include "po-charset.h"
#include "msgl-iconv.h"
#include "msgl-fsearch.h"
#include "glthread/lock.h"
#include "lang-table.h"
#include "plural-exp.h"
#include "plural-count.h"
#include "msgl-check.h"
#include "po-xerror.h"
#include "xerror-handler.h"
#include "xvasprintf.h"
#include "gettext.h"

#define _(str) gettext (str)

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free free


/* If true do not print unneeded messages.  */
bool quiet;

/* Verbosity level.  */
int verbosity_level;

/* Apply the .pot file to each of the domains in the PO file.  */
bool multi_domain_mode = false;

/* Produce output for msgfmt, not for a translator.
   msgfmt ignores
     - untranslated messages,
     - fuzzy messages, except the header entry,
     - obsolete messages.
   Therefore output for msgfmt does not need to include such messages.  */
bool for_msgfmt = false;

/* Determines whether to use fuzzy matching.  */
bool use_fuzzy_matching = true;

/* Determines whether to keep old msgids as previous msgids.  */
bool keep_previous = false;

/* Language (ISO-639 code) and optional territory (ISO-3166 code).  */
const char *catalogname = NULL;

/* List of user-specified compendiums.  */
message_list_list_ty *compendiums;

/* List of corresponding filenames.  */
string_list_ty *compendium_filenames;


struct statistics
{
  size_t merged;
  size_t fuzzied;
  size_t missing;
  size_t obsolete;
};


/* Data structure representing the messages with known translations.
   They are composed of
     - A message list from def.po,
     - The compendiums.
   The data structure is optimized for exact and fuzzy searches.  */
typedef struct definitions_ty definitions_ty;
struct definitions_ty
{
  /* A list of message lists.  The first comes from def.po, the other ones
     from the compendiums.  Each message list has a built-in hash table,
     for speed when doing the exact searches.  */
  message_list_list_ty *lists;

  /* A fuzzy index of the current list of non-compendium messages, for speed
     when doing fuzzy searches.  Used only if use_fuzzy_matching is true.  */
  message_fuzzy_index_ty *curr_findex;
  /* A once-only execution guard for the initialization of the fuzzy index.
     Needed for OpenMP.  */
  gl_lock_define(, curr_findex_init_lock)

  /* A fuzzy index of the compendiums, for speed when doing fuzzy searches.
     Used only if use_fuzzy_matching is true and compendiums != NULL.  */
  message_fuzzy_index_ty *comp_findex;
  /* A once-only execution guard for the initialization of the fuzzy index.
     Needed for OpenMP.  */
  gl_lock_define(, comp_findex_init_lock)

  /* The canonical encoding of the definitions and the compendiums.
     Only used for fuzzy matching.  */
  const char *canon_charset;
};

static inline void
definitions_init (definitions_ty *definitions, const char *canon_charset)
{
  definitions->lists = message_list_list_alloc ();
  message_list_list_append (definitions->lists, NULL);
  if (compendiums != NULL)
    message_list_list_append_list (definitions->lists, compendiums);
  definitions->curr_findex = NULL;
  gl_lock_init (definitions->curr_findex_init_lock);
  definitions->comp_findex = NULL;
  gl_lock_init (definitions->comp_findex_init_lock);
  definitions->canon_charset = canon_charset;
}

/* Return the current list of non-compendium messages.  */
static inline message_list_ty *
definitions_current_list (const definitions_ty *definitions)
{
  return definitions->lists->item[0];
}

/* Set the current list of non-compendium messages.  */
static inline void
definitions_set_current_list (definitions_ty *definitions, message_list_ty *mlp)
{
  definitions->lists->item[0] = mlp;
  if (definitions->curr_findex != NULL)
    {
      message_fuzzy_index_free (definitions->curr_findex);
      definitions->curr_findex = NULL;
    }
}

/* Create the fuzzy index for the current list of non-compendium messages.
   Used only if use_fuzzy_matching is true.  */
static inline void
definitions_init_curr_findex (definitions_ty *definitions)
{
  /* Protect against concurrent execution.  */
  gl_lock_lock (definitions->curr_findex_init_lock);
  if (definitions->curr_findex == NULL)
    definitions->curr_findex =
      message_fuzzy_index_alloc (definitions_current_list (definitions),
                                 definitions->canon_charset);
  gl_lock_unlock (definitions->curr_findex_init_lock);
}

/* Create the fuzzy index for the compendium messages.
   Used only if use_fuzzy_matching is true and compendiums != NULL.  */
static inline void
definitions_init_comp_findex (definitions_ty *definitions)
{
  /* Protect against concurrent execution.  */
  gl_lock_lock (definitions->comp_findex_init_lock);
  if (definitions->comp_findex == NULL)
    {
      /* Combine all the compendium message lists into a single one.  Don't
         bother checking for duplicates.  */
      message_list_ty *all_compendium = message_list_alloc (false);
      for (size_t i = 0; i < compendiums->nitems; i++)
        {
          message_list_ty *mlp = compendiums->item[i];

          for (size_t j = 0; j < mlp->nitems; j++)
            message_list_append (all_compendium, mlp->item[j]);
        }

      /* Create the fuzzy index from it.  */
      definitions->comp_findex =
        message_fuzzy_index_alloc (all_compendium, definitions->canon_charset);
    }
  gl_lock_unlock (definitions->comp_findex_init_lock);
}

/* Exact search.  */
static inline message_ty *
definitions_search (const definitions_ty *definitions,
                    const char *msgctxt, const char *msgid)
{
  return message_list_list_search (definitions->lists, msgctxt, msgid);
}

/* Fuzzy search.
   Used only if use_fuzzy_matching is true.  */
static inline message_ty *
definitions_search_fuzzy (definitions_ty *definitions,
                          const char *msgctxt, const char *msgid)
{
  message_ty *mp1;
  if (false)
    {
      /* Old, slow code.  */
      mp1 =
        message_list_search_fuzzy (definitions_current_list (definitions),
                                   msgctxt, msgid);
    }
  else
    {
      /* Speedup through early abort in fstrcmp(), combined with pre-sorting
         of the messages through a hashed index.  */
      /* Create the fuzzy index lazily.  */
      if (definitions->curr_findex == NULL)
        definitions_init_curr_findex (definitions);
      mp1 = message_fuzzy_index_search (definitions->curr_findex,
                                        msgctxt, msgid,
                                        FUZZY_THRESHOLD, false);
    }

  if (compendiums != NULL)
    {
      double lower_bound_for_mp2 =
        (mp1 != NULL
         ? fuzzy_search_goal_function (mp1, msgctxt, msgid, 0.0)
         : FUZZY_THRESHOLD);
      /* This lower bound must be >= FUZZY_THRESHOLD.  */
      if (!(lower_bound_for_mp2 >= FUZZY_THRESHOLD))
        abort ();

      /* Create the fuzzy index lazily.  */
      if (definitions->comp_findex == NULL)
        definitions_init_comp_findex (definitions);

      message_ty *mp2 =
        message_fuzzy_index_search (definitions->comp_findex,
                                    msgctxt, msgid,
                                    lower_bound_for_mp2, true);

      /* Choose the best among mp1, mp2.  */
      if (mp1 == NULL
          || (mp2 != NULL
              && (fuzzy_search_goal_function (mp2, msgctxt, msgid,
                                              lower_bound_for_mp2)
                  > lower_bound_for_mp2)))
        mp1 = mp2;
    }

  return mp1;
}

static inline void
definitions_destroy (definitions_ty *definitions)
{
  message_list_list_free (definitions->lists, 2);
  if (definitions->curr_findex != NULL)
    message_fuzzy_index_free (definitions->curr_findex);
  if (definitions->comp_findex != NULL)
    message_fuzzy_index_free (definitions->comp_findex);
}


/* A silent error logger.  We are only interested in knowing whether errors
   occurred at all.  */
static void
silent_error_logger (void *data, const char *format, ...)
#if (defined __GNUC__ && ((__GNUC__ == 2 && __GNUC_MINOR__ >= 7) || __GNUC__ > 2)) || defined __clang__
     __attribute__ ((__format__ (__printf__, 2, 3)))
#endif
;
static void
silent_error_logger (void *data, const char *format, ...)
{
}


/* Another silent error logger.  */
static void
silent_xerror (int severity,
               const struct message_ty *message,
               const char *filename, size_t lineno, size_t column,
               int multiline_p, const char *message_text)
{
}
static void
silent_xerror2 (int severity,
                const struct message_ty *message1,
                const char *filename1, size_t lineno1, size_t column1,
                int multiline_p1, const char *message_text1,
                const struct message_ty *message2,
                const char *filename2, size_t lineno2, size_t column2,
                int multiline_p2, const char *message_text2)
{
}

static message_ty *
message_merge (message_ty *def, message_ty *ref, bool force_fuzzy,
               const struct plural_distribution *distribution)
{
  const char *msgstr;
  size_t msgstr_len;
  const char *prev_msgctxt;
  const char *prev_msgid;
  const char *prev_msgid_plural;

  /* Take the msgid from the reference.  When fuzzy matches are made,
     the definition will not be unique, but the reference will be -
     usually because it has only been slightly changed.  */

  /* Take the msgstr from the definition.  The msgstr of the reference
     is usually empty, as it was generated by xgettext.  If we currently
     process the header entry we have to merge the msgstr by using the
     Report-Msgid-Bugs-To and POT-Creation-Date fields from the reference.  */
  if (is_header (ref))
    {
      /* Oh, oh.  The header entry and we have something to fill in.  */
      static const struct
      {
        const char *name;
        size_t len;
      } known_fields[] =
      {
        { "Project-Id-Version:", sizeof ("Project-Id-Version:") - 1 },
#define PROJECT_ID              0
        { "Report-Msgid-Bugs-To:", sizeof ("Report-Msgid-Bugs-To:") - 1 },
#define REPORT_MSGID_BUGS_TO    1
        { "POT-Creation-Date:", sizeof ("POT-Creation-Date:") - 1 },
#define POT_CREATION_DATE       2
        { "PO-Revision-Date:", sizeof ("PO-Revision-Date:") - 1 },
#define PO_REVISION_DATE        3
        { "Last-Translator:", sizeof ("Last-Translator:") - 1 },
#define LAST_TRANSLATOR         4
        { "Language-Team:", sizeof ("Language-Team:") - 1 },
#define LANGUAGE_TEAM           5
        { "Language:", sizeof ("Language:") - 1 },
#define LANGUAGE                6
        { "MIME-Version:", sizeof ("MIME-Version:") - 1 },
#define MIME_VERSION            7
        { "Content-Type:", sizeof ("Content-Type:") - 1 },
#define CONTENT_TYPE            8
        { "Content-Transfer-Encoding:",
          sizeof ("Content-Transfer-Encoding:") - 1 }
#define CONTENT_TRANSFER        9
      };
#define UNKNOWN 10
      struct
      {
        const char *string;
        size_t len;
      } header_fields[UNKNOWN + 1];

      /* Clear all fields.  */
      memset (header_fields, '\0', sizeof (header_fields));

      /* Prepare a temporary memory pool.  */
      struct obstack pool;
      obstack_init (&pool);

      const char *cp = def->msgstr;
      while (*cp != '\0')
        {
          const char *endp = strchr (cp, '\n');
          int terminated = endp != NULL;
          size_t len;

          if (!terminated)
            {
              /* Add a trailing newline.  */
              endp = strchr (cp, '\0');

              len = endp - cp + 1;

              char *copy = (char *) obstack_alloc (&pool, len + 1);
              stpcpy (stpcpy (copy, cp), "\n");
              cp = copy;
            }
          else
            {
              len = (endp - cp) + 1;
              ++endp;
            }

          /* Compare with any of the known fields.  */
          size_t cnt;
          for (cnt = 0;
               cnt < sizeof (known_fields) / sizeof (known_fields[0]);
               ++cnt)
            if (c_strncasecmp (cp, known_fields[cnt].name, known_fields[cnt].len)
                == 0)
              break;

          if (cnt < sizeof (known_fields) / sizeof (known_fields[0]))
            {
              header_fields[cnt].string = &cp[known_fields[cnt].len];
              header_fields[cnt].len = len - known_fields[cnt].len;
            }
          else
            {
              /* It's an unknown field.  Append content to what is already
                 known.  */
              char *extended =
                (char *) obstack_alloc (&pool,
                                        header_fields[UNKNOWN].len + len + 1);
              if (header_fields[UNKNOWN].string)
                memcpy (extended, header_fields[UNKNOWN].string,
                        header_fields[UNKNOWN].len);
              memcpy (&extended[header_fields[UNKNOWN].len], cp, len);
              extended[header_fields[UNKNOWN].len + len] = '\0';
              header_fields[UNKNOWN].string = extended;
              header_fields[UNKNOWN].len += len;
            }

          cp = endp;
        }

      /* Set the Language field if specified on the command line.  */
      if (catalogname != NULL)
        {
          /* Prepend a space and append a newline.  */
          size_t len = strlen (catalogname);
          char *copy = (char *) obstack_alloc (&pool, 1 + len + 1 + 1);
          stpcpy (stpcpy (stpcpy (copy, " "), catalogname), "\n");
          header_fields[LANGUAGE].string = copy;
          header_fields[LANGUAGE].len = strlen (header_fields[LANGUAGE].string);
        }
      /* Add a Language field to PO files that don't have one.  The Language
         field was introduced in gettext-0.18.  */
      else if (header_fields[LANGUAGE].string == NULL)
        {
          const char *language_team_ptr = header_fields[LANGUAGE_TEAM].string;

          if (language_team_ptr != NULL)
            {
              size_t language_team_len = header_fields[LANGUAGE_TEAM].len;

              /* Trim leading blanks.  */
              while (language_team_len > 0
                     && (*language_team_ptr == ' '
                         || *language_team_ptr == '\t'))
                {
                  language_team_ptr++;
                  language_team_len--;
                }

              /* Trim trailing blanks.  */
              while (language_team_len > 0
                     && (language_team_ptr[language_team_len - 1] == ' '
                         || language_team_ptr[language_team_len - 1] == '\t'))
                language_team_len--;

              /* Trim last word, if it looks like an URL or email address.  */
              {
                size_t i;

                for (i = language_team_len; i > 0; i--)
                  if (language_team_ptr[i - 1] == ' '
                      || language_team_ptr[i - 1] == '\t')
                    break;
                /* The last word: language_team_ptr[i..language_team_len-1].  */
                if (i < language_team_len
                    && (language_team_ptr[i] == '<'
                        || language_team_ptr[language_team_len - 1] == '>'
                        || memchr (language_team_ptr, '@', language_team_len)
                           != NULL
                        || memchr (language_team_ptr, '/', language_team_len)
                           != NULL))
                  {
                    /* Trim last word and blanks before it.  */
                    while (i > 0
                           && (language_team_ptr[i - 1] == ' '
                               || language_team_ptr[i - 1] == '\t'))
                      i--;
                    language_team_len = i;
                  }
              }

              /* The rest of the Language-Team field should be the english name
                 of the language.  Convert to ISO 639 and ISO 3166 syntax.  */
              for (size_t i = 0; i < language_variant_table_size; i++)
                if (strlen (language_variant_table[i].english)
                    == language_team_len
                    && memcmp (language_variant_table[i].english,
                               language_team_ptr, language_team_len) == 0)
                  {
                    header_fields[LANGUAGE].string =
                      language_variant_table[i].code;
                    break;
                  }
              if (header_fields[LANGUAGE].string == NULL)
                for (size_t i = 0; i < language_table_size; i++)
                  if (strlen (language_table[i].english) == language_team_len
                      && memcmp (language_table[i].english,
                                 language_team_ptr, language_team_len) == 0)
                    {
                      header_fields[LANGUAGE].string = language_table[i].code;
                      break;
                    }
              if (header_fields[LANGUAGE].string != NULL)
                {
                  /* Prepend a space and append a newline.  */
                  const char *str = header_fields[LANGUAGE].string;
                  size_t len = strlen (str);
                  char *copy = (char *) obstack_alloc (&pool, 1 + len + 1 + 1);
                  stpcpy (stpcpy (stpcpy (copy, " "), str), "\n");
                  header_fields[LANGUAGE].string = copy;
                }
              else
                header_fields[LANGUAGE].string = " \n";
              header_fields[LANGUAGE].len =
                strlen (header_fields[LANGUAGE].string);
            }
        }

      {
        const char *msgid_bugs_ptr =
          c_strstr (ref->msgstr, "Report-Msgid-Bugs-To:");
        if (msgid_bugs_ptr != NULL)
          {
            msgid_bugs_ptr += sizeof ("Report-Msgid-Bugs-To:") - 1;

            const char *endp = strchr (msgid_bugs_ptr, '\n');
            size_t msgid_bugs_len;
            if (endp == NULL)
              {
                /* Add a trailing newline.  */
                endp = strchr (msgid_bugs_ptr, '\0');
                msgid_bugs_len = (endp - msgid_bugs_ptr) + 1;
                char *extended = (char *) obstack_alloc (&pool, msgid_bugs_len + 1);
                stpcpy (stpcpy (extended, msgid_bugs_ptr), "\n");
                msgid_bugs_ptr = extended;
              }
            else
              msgid_bugs_len = (endp - msgid_bugs_ptr) + 1;

            header_fields[REPORT_MSGID_BUGS_TO].string = msgid_bugs_ptr;
            header_fields[REPORT_MSGID_BUGS_TO].len = msgid_bugs_len;
          }
      }

      {
        const char *pot_date_ptr = c_strstr (ref->msgstr, "POT-Creation-Date:");
        if (pot_date_ptr != NULL)
          {
            pot_date_ptr += sizeof ("POT-Creation-Date:") - 1;

            const char *endp = strchr (pot_date_ptr, '\n');
            size_t pot_date_len;
            if (endp == NULL)
              {
                /* Add a trailing newline.  */
                endp = strchr (pot_date_ptr, '\0');
                pot_date_len = (endp - pot_date_ptr) + 1;
                char *extended = (char *) obstack_alloc (&pool, pot_date_len + 1);
                stpcpy (stpcpy (extended, pot_date_ptr), "\n");
                pot_date_ptr = extended;
              }
            else
              pot_date_len = (endp - pot_date_ptr) + 1;

            header_fields[POT_CREATION_DATE].string = pot_date_ptr;
            header_fields[POT_CREATION_DATE].len = pot_date_len;
          }
      }

      /* Concatenate all the various fields.  */
      {
        size_t len;
        {
          len = 0;
          for (size_t cnt = 0; cnt < UNKNOWN; ++cnt)
            if (header_fields[cnt].string != NULL)
              len += known_fields[cnt].len + header_fields[cnt].len;
          len += header_fields[UNKNOWN].len;
        }
        char *newp = XNMALLOC (len + 1, char);
        cp = newp;
        newp[len] = '\0';

#define IF_FILLED(idx)                                                        \
        if (header_fields[idx].string)                                        \
          newp = stpncpy (stpcpy (newp, known_fields[idx].name),              \
                          header_fields[idx].string, header_fields[idx].len)

        IF_FILLED (PROJECT_ID);
        IF_FILLED (REPORT_MSGID_BUGS_TO);
        IF_FILLED (POT_CREATION_DATE);
        IF_FILLED (PO_REVISION_DATE);
        IF_FILLED (LAST_TRANSLATOR);
        IF_FILLED (LANGUAGE_TEAM);
        IF_FILLED (LANGUAGE);
        IF_FILLED (MIME_VERSION);
        IF_FILLED (CONTENT_TYPE);
        IF_FILLED (CONTENT_TRANSFER);
        if (header_fields[UNKNOWN].string != NULL)
          stpcpy (newp, header_fields[UNKNOWN].string);

#undef IF_FILLED
      }

      /* Free the temporary memory pool.  */
      obstack_free (&pool, NULL);

      msgstr = cp;
      msgstr_len = strlen (cp) + 1;

      prev_msgctxt = NULL;
      prev_msgid = NULL;
      prev_msgid_plural = NULL;
    }
  else
    {
      msgstr = def->msgstr;
      msgstr_len = def->msgstr_len;

      if (def->is_fuzzy)
        {
          prev_msgctxt = def->prev_msgctxt;
          prev_msgid = def->prev_msgid;
          prev_msgid_plural = def->prev_msgid_plural;
        }
      else
        {
          prev_msgctxt = def->msgctxt;
          prev_msgid = def->msgid;
          prev_msgid_plural = def->msgid_plural;
        }
    }

  message_ty *result =
    message_alloc (ref->msgctxt != NULL ? xstrdup (ref->msgctxt) : NULL,
                   xstrdup (ref->msgid), ref->msgid_plural,
                   msgstr, msgstr_len, &def->pos);

  /* Take the comments from the definition file.  There will be none at
     all in the reference file, as it was generated by xgettext.  */
  if (def->comment)
    for (size_t j = 0; j < def->comment->nitems; ++j)
      message_comment_append (result, def->comment->item[j]);

  /* Take the dot comments from the reference file, as they are
     generated by xgettext.  Any in the definition file are old ones
     collected by previous runs of xgettext and msgmerge.  */
  if (ref->comment_dot)
    for (size_t j = 0; j < ref->comment_dot->nitems; ++j)
      message_comment_dot_append (result, ref->comment_dot->item[j]);

  /* The flags are mixed in a special way.  Some informations come
     from the reference message (such as format/no-format), others
     come from the definition file (fuzzy or not).  */
  result->is_fuzzy = def->is_fuzzy | force_fuzzy;

  /* If ref and def have the same msgid but different msgid_plural, it's
     a reason to mark the result fuzzy.  */
  if (!result->is_fuzzy
      && (ref->msgid_plural != NULL
          ? def->msgid_plural == NULL
            || strcmp (ref->msgid_plural, def->msgid_plural) != 0
          : def->msgid_plural != NULL))
    result->is_fuzzy = true;

  for (size_t i = 0; i < NFORMATS; i++)
    {
      result->is_format[i] = ref->is_format[i];

      /* If the reference message is marked as being a format specifier,
         but the definition message is not, we check if the resulting
         message would pass "msgfmt -c".  If yes, then all is fine.  If
         not, we add a fuzzy marker, because
         1. the message needs the translator's attention,
         2. msgmerge must not transform a PO file which passes "msgfmt -c"
            into a PO file which doesn't.  */
      if (!result->is_fuzzy
          && possible_format_p (ref->is_format[i])
          && !possible_format_p (def->is_format[i])
          && check_msgid_msgstr_format_i (ref->msgid, ref->msgid_plural,
                                          msgstr, msgstr_len, i, ref->range,
                                          distribution,
                                          silent_error_logger, NULL)
             > 0)
        result->is_fuzzy = true;
    }

  result->range = ref->range;
  /* If the definition message was assuming a certain range, but the reference
     message does not specify a range any more or specifies a range that is
     not the same or a subset, we add a fuzzy marker, because
       1. the message needs the translator's attention,
       2. msgmerge must not transform a PO file which passes "msgfmt -c"
          into a PO file which doesn't.  */
  if (!result->is_fuzzy
      && has_range_p (def->range)
      && !(has_range_p (ref->range)
           && ref->range.min >= def->range.min
           && ref->range.max <= def->range.max))
    result->is_fuzzy = true;

  result->do_wrap = ref->do_wrap;

  /* Insert previous msgid, commented out with "#|".
     Do so only when --previous is specified, for backward compatibility.
     Since the "previous msgid" represents the original msgid that led to
     the current msgstr,
       - we can omit it if the resulting message is not fuzzy or is
         untranslated (but do this in a later pass, since result->is_fuzzy
         is not finalized at this point),
       - otherwise, if the corresponding message from the definition file
         was translated (not fuzzy), we use that message's msgid,
       - otherwise, we use that message's prev_msgid.  */
  if (keep_previous)
    {
      result->prev_msgctxt = prev_msgctxt;
      result->prev_msgid = prev_msgid;
      result->prev_msgid_plural = prev_msgid_plural;
    }

  /* If the reference message was obsolete, make the resulting message
     obsolete.  This case doesn't occur for POT files, but users sometimes
     use PO files that are themselves the result of msgmerge instead of POT
     files.  */
  result->obsolete = ref->obsolete;

  /* Take the file position comments from the reference file, as they
     are generated by xgettext.  Any in the definition file are old ones
     collected by previous runs of xgettext and msgmerge.  */
  for (size_t j = 0; j < ref->filepos_count; ++j)
    {
      lex_pos_ty *pp = &ref->filepos[j];
      message_comment_filepos (result, pp->file_name, pp->line_number);
    }

  /* Special postprocessing is needed if the reference message is a
     plural form and the definition message isn't, or vice versa.  */
  if (ref->msgid_plural != NULL)
    {
      if (def->msgid_plural == NULL)
        result->used = 1;
    }
  else
    {
      if (def->msgid_plural != NULL)
        result->used = 2;
    }

  /* All done, return the merged message to the caller.  */
  return result;
}


#define DOT_FREQUENCY 10

static void
match_domain (const char *definitions_file_name,
              const char *references_file_name,
              definitions_ty *definitions, message_list_ty *refmlp,
              message_list_ty *resultmlp,
              struct statistics *stats, unsigned int *processed)
{
  {
    unsigned long int nplurals;
    char *untranslated_plural_msgstr;
    struct plural_distribution distribution;

    {
      message_ty *header_entry =
        message_list_search (definitions_current_list (definitions), NULL, "");
      const struct expression *plural_expr;
      extract_plural_expression (header_entry ? header_entry->msgstr : NULL,
                                 &plural_expr, &nplurals);
      untranslated_plural_msgstr = XNMALLOC (nplurals, char);
      memset (untranslated_plural_msgstr, '\0', nplurals);

      /* Determine the plural distribution of the plural_expr formula.  */
      {
        /* Disable error output temporarily.  */
        unsigned int error_count = 0;
        struct xerror_handler local_xerror_handler =
          {
            silent_xerror,
            silent_xerror2,
            &error_count
          };

        if (check_plural_eval (plural_expr, nplurals, header_entry,
                               &distribution, &local_xerror_handler) > 0)
          {
            distribution.expr = NULL;
            distribution.often = NULL;
            distribution.often_length = 0;
            distribution.histogram = NULL;
          }
      }
    }

    /* Most of the time is spent in definitions_search_fuzzy.
       Perform it in a separate loop that can be parallelized by an OpenMP
       capable compiler.  */
    struct search_result { message_ty *found; bool fuzzy; } *search_results =
      XNMALLOC (refmlp->nitems, struct search_result);
    {
      long int nn = refmlp->nitems;
      long int jj;

      /* Tell the OpenMP capable compiler to distribute this loop across
         several threads.  The schedule is dynamic, because for some messages
         the loop body can be executed very quickly, whereas for others it takes
         a long time.
         Notes: The Sun Workshop 6.2 C compiler does not allow a space between
         '#' and 'pragma'.  The MSVC 14 C compiler does not allow a 'for' loop
         with a variable declaration.  */
      #ifdef _OPENMP
       #pragma omp parallel for schedule(dynamic)
      #endif
      for (jj = 0; jj < nn; jj++)
        {
          message_ty *refmsg = refmlp->item[jj];

          /* Because merging can take a while we print something to signal
             we are not dead.  */
          if (!quiet && verbosity_level <= 1 && *processed % DOT_FREQUENCY == 0)
            fputc ('.', stderr);
          #ifdef _OPENMP
           #pragma omp atomic
          #endif
          (*processed)++;

          /* See if it is in the other file.  */
          message_ty *defmsg =
            definitions_search (definitions, refmsg->msgctxt, refmsg->msgid);
          if (defmsg != NULL)
            {
              search_results[jj].found = defmsg;
              search_results[jj].fuzzy = false;
            }
          else if (!is_header (refmsg)
                   /* If the message was not defined at all, try to find a very
                      similar message, it could be a typo, or the suggestion may
                      help.  */
                   && use_fuzzy_matching
                   && ((defmsg =
                          definitions_search_fuzzy (definitions,
                                                    refmsg->msgctxt,
                                                    refmsg->msgid)) != NULL))
            {
              search_results[jj].found = defmsg;
              search_results[jj].fuzzy = true;
            }
          else
            search_results[jj].found = NULL;
        }
    }

    for (size_t j = 0; j < refmlp->nitems; j++)
      {
        message_ty *refmsg = refmlp->item[j];

        /* See if it is in the other file.
           This used definitions_search.  */
        if (search_results[j].found != NULL && !search_results[j].fuzzy)
          {
            message_ty *defmsg = search_results[j].found;
            /* Merge the reference with the definition: take the #. and
               #: comments from the reference, take the # comments from
               the definition, take the msgstr from the definition.  Add
               this merged entry to the output message list.  */
            message_ty *mp =
              message_merge (defmsg, refmsg, false, &distribution);

            /* When producing output for msgfmt, omit messages that are
               untranslated or fuzzy (except the header entry).  */
            if (!(for_msgfmt
                  && (mp->msgstr[0] == '\0' /* untranslated? */
                      || (mp->is_fuzzy && !is_header (mp))))) /* fuzzy? */
              {
                message_list_append (resultmlp, mp);

                /* Remember that this message has been used, when we scan
                   later to see if anything was omitted.  */
                defmsg->used = 1;
              }

            stats->merged++;
          }
        else if (!is_header (refmsg))
          {
            /* If the message was not defined at all, try to find a very
               similar message, it could be a typo, or the suggestion may
               help.  This search assumed use_fuzzy_matching and used
               definitions_search_fuzzy.  */
            if (search_results[j].found != NULL && search_results[j].fuzzy)
              {
                message_ty *defmsg = search_results[j].found;

                if (verbosity_level > 1)
                  po_xerror2 (PO_SEVERITY_ERROR,
                              refmsg, NULL, 0, 0, false,
                              _("this message is used but not defined"),
                              defmsg, NULL, 0, 0, false,
                              _("but this definition is similar"));

                /* Merge the reference with the definition: take the #. and
                   #: comments from the reference, take the # comments from
                   the definition, take the msgstr from the definition.  Add
                   this merged entry to the output message list.  */
                message_ty *mp =
                  message_merge (defmsg, refmsg, true, &distribution);

                message_list_append (resultmlp, mp);

                /* Remember that this message has been used, when we scan
                   later to see if anything was omitted.  */
                defmsg->used = 1;

                stats->fuzzied++;
                if (!quiet && verbosity_level <= 1)
                  /* Always print a dot if we handled a fuzzy match.  */
                  fputc ('.', stderr);
              }
            else
              {
                if (verbosity_level > 1)
                  po_xerror (PO_SEVERITY_ERROR, refmsg, NULL, 0, 0, false,
                             xasprintf (
                               _("this message is used but not defined in %s"),
                               definitions_file_name));

                message_ty *mp = message_copy (refmsg);

                /* Test if mp is untranslated.  (It most likely is.)  */
                bool is_untranslated = true;
                {
                  const char *p;
                  const char *pend;
                  for (p = mp->msgstr, pend = p + mp->msgstr_len; p < pend; p++)
                    if (*p != '\0')
                      {
                        is_untranslated = false;
                        break;
                      }
                }

                if (mp->msgid_plural != NULL && is_untranslated)
                  {
                    /* Change mp->msgstr_len consecutive empty strings into
                       nplurals consecutive empty strings.  */
                    if (nplurals > mp->msgstr_len)
                      mp->msgstr = untranslated_plural_msgstr;
                    mp->msgstr_len = nplurals;
                  }

                /* When producing output for msgfmt, omit messages that are
                   untranslated or fuzzy (except the header entry).  */
                if (!(for_msgfmt && (is_untranslated || mp->is_fuzzy)))
                  {
                    message_list_append (resultmlp, mp);
                  }

                stats->missing++;
              }
          }
      }

    free (search_results);
  }

  /* Now postprocess the problematic merges.  This is needed because we
     want the result to pass the "msgfmt -c -v" check.  */
  {
    /* message_merge sets mp->used to 1 or 2, depending on the problem.
       Compute the bitwise OR of all these.  */
    int problematic = 0;

    for (size_t j = 0; j < resultmlp->nitems; j++)
      problematic |= resultmlp->item[j]->used;

    if (problematic)
      {
        unsigned long int nplurals = 0;

        if (problematic & 1)
          {
            /* Need to know nplurals of the result domain.  */
            message_ty *header_entry =
              message_list_search (resultmlp, NULL, "");

            nplurals = get_plural_count (header_entry
                                         ? header_entry->msgstr
                                         : NULL);
          }

        for (size_t j = 0; j < resultmlp->nitems; j++)
          {
            message_ty *mp = resultmlp->item[j];

            if ((mp->used & 1) && (nplurals > 0))
              {
                /* ref->msgid_plural != NULL but def->msgid_plural == NULL.
                   Use a copy of def->msgstr for each possible plural form.  */
                if (verbosity_level > 1)
                  po_xerror (PO_SEVERITY_ERROR, mp, NULL, 0, 0, false,
                             _("this message should define plural forms"));

                size_t new_msgstr_len = nplurals * mp->msgstr_len;
                char *new_msgstr = XNMALLOC (new_msgstr_len, char);
                {
                  char *p;
                  unsigned long i;
                  for (i = 0, p = new_msgstr; i < nplurals; i++)
                    {
                      memcpy (p, mp->msgstr, mp->msgstr_len);
                      p += mp->msgstr_len;
                    }
                }
                mp->msgstr = new_msgstr;
                mp->msgstr_len = new_msgstr_len;
                mp->is_fuzzy = true;
              }

            if ((mp->used & 2) && (mp->msgstr_len > strlen (mp->msgstr) + 1))
              {
                /* ref->msgid_plural == NULL but def->msgid_plural != NULL.
                   Use only the first among the plural forms.  */

                if (verbosity_level > 1)
                  po_xerror (PO_SEVERITY_ERROR, mp, NULL, 0, 0, false,
                             _("this message should not define plural forms"));

                mp->msgstr_len = strlen (mp->msgstr) + 1;
                mp->is_fuzzy = true;
              }

            /* Postprocessing of this message is done.  */
            mp->used = 0;
          }
      }
  }

  /* Now that mp->is_fuzzy is finalized for all messages, remove the
     "previous msgid" information from all messages that are not fuzzy or
     are untranslated.  */
  for (size_t j = 0; j < resultmlp->nitems; j++)
    {
      message_ty *mp = resultmlp->item[j];

      if (!mp->is_fuzzy || mp->msgstr[0] == '\0')
        {
          mp->prev_msgctxt = NULL;
          mp->prev_msgid = NULL;
          mp->prev_msgid_plural = NULL;
        }
    }
}

msgdomain_list_ty *
merge (const char *definitions_file_name,
       catalog_input_format_ty definitions_file_syntax,
       const char *references_file_name,
       catalog_input_format_ty references_file_syntax,
       msgdomain_list_ty **defp)
{
  struct statistics stats;
  stats.merged = stats.fuzzied = stats.missing = stats.obsolete = 0;

  /* This is the definitions file, created by a human.  */
  msgdomain_list_ty *def =
    read_catalog_file (definitions_file_name, definitions_file_syntax);

  /* This is the references file, created by groping the sources with
     the xgettext program.  */
  msgdomain_list_ty *ref =
    read_catalog_file (references_file_name, references_file_syntax);
  check_pot_charset (ref, references_file_name);
  /* Add a dummy header entry, if the references file contains none.  */
  for (size_t k = 0; k < ref->nitems; k++)
    if (message_list_search (ref->item[k]->messages, NULL, "") == NULL)
      {
        static lex_pos_ty pos = { __FILE__, __LINE__ };
        message_ty *refheader = message_alloc (NULL, "", NULL, "", 1, &pos);

        message_list_prepend (ref->item[k]->messages, refheader);
      }

  /* The references file can be either in ASCII or in UTF-8.  If it is
     in UTF-8, we have to convert the definitions and the compendiums to
     UTF-8 as well.  */
  {
    bool was_utf8 = false;
    for (size_t k = 0; k < ref->nitems; k++)
      {
        message_list_ty *mlp = ref->item[k]->messages;

        for (size_t j = 0; j < mlp->nitems; j++)
          if (is_header (mlp->item[j]) && !mlp->item[j]->obsolete)
            {
              const char *header = mlp->item[j]->msgstr;

              if (header != NULL)
                {
                  const char *charsetstr = c_strstr (header, "charset=");

                  if (charsetstr != NULL)
                    {
                      charsetstr += strlen ("charset=");
                      size_t len = strcspn (charsetstr, " \t\n");
                      if (len == strlen ("UTF-8")
                          && c_strncasecmp (charsetstr, "UTF-8", len) == 0)
                        was_utf8 = true;
                    }
                }
            }
        }
    if (was_utf8)
      {
        def = iconv_msgdomain_list (def, po_charset_utf8, true,
                                    definitions_file_name,
                                    textmode_xerror_handler);
        if (compendiums != NULL)
          for (size_t k = 0; k < compendiums->nitems; k++)
            iconv_message_list (compendiums->item[k], NULL, po_charset_utf8,
                                compendium_filenames->item[k],
                                textmode_xerror_handler);
      }
    else if (compendiums != NULL && compendiums->nitems > 0)
      {
        /* Ensure that the definitions and the compendiums are in the same
           encoding.  Prefer the encoding of the definitions file, if
           possible; otherwise, if the definitions file is empty and the
           compendiums are all in the same encoding, use that encoding;
           otherwise, use UTF-8.  */
        bool conversion_done = false;
        {
          char *charset = NULL;

          /* Get the encoding of the definitions file.  */
          for (size_t k = 0; k < def->nitems; k++)
            {
              message_list_ty *mlp = def->item[k]->messages;

              for (size_t j = 0; j < mlp->nitems; j++)
                if (is_header (mlp->item[j]) && !mlp->item[j]->obsolete)
                  {
                    const char *header = mlp->item[j]->msgstr;

                    if (header != NULL)
                      {
                        const char *charsetstr = c_strstr (header, "charset=");

                        if (charsetstr != NULL)
                          {
                            charsetstr += strlen ("charset=");
                            size_t len = strcspn (charsetstr, " \t\n");
                            charset = (char *) xmalloca (len + 1);
                            memcpy (charset, charsetstr, len);
                            charset[len] = '\0';
                            break;
                          }
                      }
                  }
              if (charset != NULL)
                break;
            }
          if (charset != NULL)
            {
              const char *canon_charset = po_charset_canonicalize (charset);

              if (canon_charset != NULL)
                {
                  bool all_compendiums_iconvable = true;

                  if (compendiums != NULL)
                    for (size_t k = 0; k < compendiums->nitems; k++)
                      if (!is_message_list_iconvable (compendiums->item[k],
                                                      NULL, canon_charset))
                        {
                          all_compendiums_iconvable = false;
                          break;
                        }

                  if (all_compendiums_iconvable)
                    {
                      /* Convert the compendiums to def's encoding.  */
                      if (compendiums != NULL)
                        for (size_t k = 0; k < compendiums->nitems; k++)
                          iconv_message_list (compendiums->item[k],
                                              NULL, canon_charset,
                                              compendium_filenames->item[k],
                                              textmode_xerror_handler);
                      conversion_done = true;
                    }
                }
              freea (charset);
            }
        }
        if (!conversion_done)
          {
            if (def->nitems == 0
                || (def->nitems == 1 && def->item[0]->messages->nitems == 0))
              {
                /* The definitions file is empty.
                   Compare the encodings of the compendiums.  */
                const char *common_canon_charset = NULL;

                for (size_t k = 0; k < compendiums->nitems; k++)
                  {
                    message_list_ty *mlp = compendiums->item[k];
                    char *charset = NULL;
                    const char *canon_charset = NULL;

                    for (size_t j = 0; j < mlp->nitems; j++)
                      if (is_header (mlp->item[j]) && !mlp->item[j]->obsolete)
                        {
                          const char *header = mlp->item[j]->msgstr;

                          if (header != NULL)
                            {
                              const char *charsetstr =
                                c_strstr (header, "charset=");

                              if (charsetstr != NULL)
                                {
                                  charsetstr += strlen ("charset=");
                                  size_t len = strcspn (charsetstr, " \t\n");
                                  charset = (char *) xmalloca (len + 1);
                                  memcpy (charset, charsetstr, len);
                                  charset[len] = '\0';

                                  break;
                                }
                            }
                        }
                    if (charset != NULL)
                      {
                        canon_charset = po_charset_canonicalize (charset);
                        freea (charset);
                      }
                    /* If no charset declaration was found in this file,
                       or if it is not a valid encoding name, or if it
                       differs from the common charset found so far,
                       we have no common charset.  */
                    if (canon_charset == NULL
                        || (common_canon_charset != NULL
                            && canon_charset != common_canon_charset))
                      {
                        common_canon_charset = NULL;
                        break;
                      }
                    common_canon_charset = canon_charset;
                  }

                if (common_canon_charset != NULL)
                  /* No conversion needed in this case.  */
                  conversion_done = true;
              }
            if (!conversion_done)
              {
                /* It's too hairy to find out what would be the optimal target
                   encoding.  So, convert everything to UTF-8.  */
                def = iconv_msgdomain_list (def, po_charset_utf8, true,
                                            definitions_file_name,
                                            textmode_xerror_handler);
                if (compendiums != NULL)
                  for (size_t k = 0; k < compendiums->nitems; k++)
                    iconv_message_list (compendiums->item[k],
                                        NULL, po_charset_utf8,
                                        compendium_filenames->item[k],
                                        textmode_xerror_handler);
              }
          }
      }
  }

  /* Determine canonicalized encoding name of the definitions now, after
     conversion.  Only used for fuzzy matching.  */
  const char *def_canon_charset;
  if (use_fuzzy_matching)
    {
      def_canon_charset = def->encoding;
      if (def_canon_charset == NULL)
        {
          char *charset = NULL;

          /* Get the encoding of the definitions file.  */
          for (size_t k = 0; k < def->nitems; k++)
            {
              message_list_ty *mlp = def->item[k]->messages;

              for (size_t j = 0; j < mlp->nitems; j++)
                if (is_header (mlp->item[j]) && !mlp->item[j]->obsolete)
                  {
                    const char *header = mlp->item[j]->msgstr;

                    if (header != NULL)
                      {
                        const char *charsetstr = c_strstr (header, "charset=");

                        if (charsetstr != NULL)
                          {
                            charsetstr += strlen ("charset=");
                            size_t len = strcspn (charsetstr, " \t\n");
                            charset = (char *) xmalloca (len + 1);
                            memcpy (charset, charsetstr, len);
                            charset[len] = '\0';
                            break;
                          }
                      }
                  }
              if (charset != NULL)
                break;
            }
          if (charset != NULL)
            def_canon_charset = po_charset_canonicalize (charset);
          if (def_canon_charset == NULL)
            /* Unspecified encoding.  Assume unibyte encoding.  */
            def_canon_charset = po_charset_ascii;
        }
    }
  else
    def_canon_charset = NULL;

  /* Initialize and preprocess the total set of message definitions.  */
  definitions_ty definitions;
  definitions_init (&definitions, def_canon_charset);
  message_list_ty *empty_list = message_list_alloc (false);

  msgdomain_list_ty *result = msgdomain_list_alloc (false);
  unsigned int processed = 0;

  /* Every reference must be matched with its definition. */
  if (!multi_domain_mode)
    for (size_t k = 0; k < ref->nitems; k++)
      {
        const char *domain = ref->item[k]->domain;
        message_list_ty *refmlp = ref->item[k]->messages;
        message_list_ty *resultmlp =
          msgdomain_list_sublist (result, domain, true);

        message_list_ty *defmlp = msgdomain_list_sublist (def, domain, false);
        if (defmlp == NULL)
          defmlp = empty_list;
        definitions_set_current_list (&definitions, defmlp);

        match_domain (definitions_file_name, references_file_name,
                      &definitions, refmlp, resultmlp,
                      &stats, &processed);
      }
  else
    {
      /* Apply the references messages in the default domain to each of
         the definition domains.  */
      message_list_ty *refmlp = ref->item[0]->messages;

      for (size_t k = 0; k < def->nitems; k++)
        {
          const char *domain = def->item[k]->domain;
          message_list_ty *defmlp = def->item[k]->messages;

          /* Ignore the default message domain if it has no messages.  */
          if (k > 0 || defmlp->nitems > 0)
            {
              message_list_ty *resultmlp =
                msgdomain_list_sublist (result, domain, true);

              definitions_set_current_list (&definitions, defmlp);

              match_domain (definitions_file_name, references_file_name,
                            &definitions, refmlp, resultmlp,
                            &stats, &processed);
            }
        }
    }

  definitions_destroy (&definitions);

  if (!for_msgfmt)
    {
      /* Look for messages in the definition file, which are not present
         in the reference file, indicating messages which defined but not
         used in the program.  Don't scan the compendium(s).  */
      for (size_t k = 0; k < def->nitems; ++k)
        {
          const char *domain = def->item[k]->domain;
          message_list_ty *defmlp = def->item[k]->messages;

          for (size_t j = 0; j < defmlp->nitems; j++)
            {
              message_ty *defmsg = defmlp->item[j];

              if (!defmsg->used)
                {
                  /* Remember the old translation although it is not used anymore.
                     But we mark it as obsolete.  */
                  message_ty *mp = message_copy (defmsg);
                  /* Clear the extracted comments.  */
                  if (mp->comment_dot != NULL)
                    {
                      string_list_free (mp->comment_dot);
                      mp->comment_dot = NULL;
                    }
                  /* Clear the file position comments.  */
                  if (mp->filepos != NULL)
                    {
                      for (size_t i = 0; i < mp->filepos_count; i++)
                        free ((char *) mp->filepos[i].file_name);
                      mp->filepos_count = 0;
                      free (mp->filepos);
                      mp->filepos = NULL;
                    }
                  /* Mark as obsolete.   */
                  mp->obsolete = true;

                  message_list_append (msgdomain_list_sublist (result, domain, true),
                                       mp);
                  stats.obsolete++;
                }
            }
        }
    }

  /* Determine the known a-priori encoding, if any.  */
  if (def->encoding == ref->encoding)
    result->encoding = def->encoding;

  /* Report some statistics.  */
  if (verbosity_level > 0)
    fprintf (stderr, _("%s\
Read %ld old + %ld reference, \
merged %ld, fuzzied %ld, missing %ld, obsolete %ld.\n"),
             !quiet && verbosity_level <= 1 ? "\n" : "",
             (long) def->nitems, (long) ref->nitems,
             (long) stats.merged, (long) stats.fuzzied, (long) stats.missing,
             (long) stats.obsolete);
  else if (!quiet)
    fputs (_(" done.\n"), stderr);

  /* Return results.  */
  *defp = def;
  return result;
}
