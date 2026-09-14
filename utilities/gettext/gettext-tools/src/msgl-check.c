/* Checking of messages in PO files.
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

/* Written by Ulrich Drepper, Bruno Haible, and Daiki Ueno.  */

#include <config.h>

/* Specification.  */
#include "msgl-check.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "c-ctype.h"
#include "xalloc.h"
#include "xvasprintf.h"
#include "po-xerror.h"
#include "format.h"
#include "plural-exp.h"
#include "plural-eval.h"
#include "plural-table.h"
#include "c-strstr.h"
#include "message.h"
#include "gettext.h"

#define _(str) gettext (str)

#define SIZEOF(a) (sizeof(a) / sizeof(a[0]))


/* Evaluates the plural formula for min <= n <= max
   and returns the estimated number of times the value j was assumed.  */
static unsigned int
plural_expression_histogram (const struct plural_distribution *self,
                             int min, int max, unsigned long j)
{
  if (min < 0)
    min = 0;
  /* Limit the number of evaluations.  Nothing interesting happens beyond
     1000.  */
  if (max - min > 1000)
    max = min + 1000;
  if (min <= max)
    {
      const struct expression *expr = self->expr;

      unsigned int count = 0;
      for (unsigned long n = min; n <= max; n++)
        {
          struct eval_result res = plural_eval (expr, n);

          if (res.status == PE_OK && res.value == j)
            count++;
        }

      return count;
    }
  else
    return 0;
}


/* Check the values returned by plural_eval.
   Signals the errors through po_xerror.
   Return the number of errors that were seen.
   If no errors, returns in *DISTRIBUTION information about the plural_eval
   values distribution.  */
int
check_plural_eval (const struct expression *plural_expr,
                   unsigned long nplurals_value,
                   const message_ty *header,
                   struct plural_distribution *distribution,
                   xerror_handler_ty xeh)
{
  /* Do as if the plural formula assumes a value N infinitely often if it
     assumes it at least 5 times.  */
#define OFTEN 5
  unsigned char *array;

  /* Allocate a distribution array.  */
  if (nplurals_value <= 100)
    array = XCALLOC (nplurals_value, unsigned char);
  else
    /* nplurals_value is nonsense.  Don't risk an out-of-memory.  */
    array = NULL;

  for (unsigned long n = 0; n <= 1000; n++)
    {
      struct eval_result res = plural_eval (plural_expr, n);
      if (res.status != PE_OK)
        {
          if (res.status == PE_INTDIV)
            xeh->xerror (CAT_SEVERITY_ERROR, header, NULL, 0, 0, false,
                         _("plural expression can produce division by zero"));
          else if (res.status == PE_INTOVF)
            xeh->xerror (CAT_SEVERITY_ERROR, header, NULL, 0, 0, false,
                         _("plural expression can produce integer overflow"));
          else if (res.status == PE_STACKOVF)
            xeh->xerror (CAT_SEVERITY_ERROR, header, NULL, 0, 0, false,
                         _("plural expression can produce stack overflow"));
          else
            /* Other res.status values should not occur.  */
            abort ();

          free (array);
          return 1;
        }

      unsigned long val = res.value;

      if ((long) val < 0)
        {
          xeh->xerror (CAT_SEVERITY_ERROR, header, NULL, 0, 0, false,
                       _("plural expression can produce negative values"));
          free (array);
          return 1;
        }
      else if (val >= nplurals_value)
        {
          char *msg =
            xasprintf (_("nplurals = %lu but plural expression can produce values as large as %lu"),
                       nplurals_value, val);
          xeh->xerror (CAT_SEVERITY_ERROR, header, NULL, 0, 0, false, msg);
          free (msg);
          free (array);
          return 1;
        }

      if (array != NULL && array[val] < OFTEN)
        array[val]++;
    }

  /* Normalize the array[val] statistics.  */
  if (array != NULL)
    {
      for (unsigned long val = 0; val < nplurals_value; val++)
        array[val] = (array[val] == OFTEN ? 1 : 0);
    }

  distribution->expr = plural_expr;
  distribution->often = array;
  distribution->often_length = (array != NULL ? nplurals_value : 0);
  distribution->histogram = plural_expression_histogram;

  return 0;
#undef OFTEN
}


/* Try to help the translator by looking up the right plural formula for her.
   Return a freshly allocated multiline help string, or NULL.  */
static char *
plural_help (const char *nullentry)
{
  struct plural_table_entry *ptentry = NULL;

  {
    const char *language = c_strstr (nullentry, "Language: ");
    if (language != NULL)
      {
        language += 10;

        size_t len = strcspn (language, " \t\n");
        if (len > 0)
          {
            for (size_t j = 0; j < plural_table_size; j++)
              if (len == strlen (plural_table[j].lang)
                  && strncmp (language, plural_table[j].lang, len) == 0)
                {
                  ptentry = &plural_table[j];
                  break;
                }
          }
      }
  }

  if (ptentry == NULL)
    {
      const char *language = c_strstr (nullentry, "Language-Team: ");
      if (language != NULL)
        {
          language += 15;

          for (size_t j = 0; j < plural_table_size; j++)
            if (str_startswith (language, plural_table[j].language))
              {
                ptentry = &plural_table[j];
                break;
              }
        }
    }

  if (ptentry != NULL)
    {
      char *helpline1 =
        xasprintf (_("Try using the following, valid for %s:"),
                   ptentry->language);
      char *help =
        xasprintf ("%s\n\"Plural-Forms: %s\\n\"\n",
                   helpline1, ptentry->value);
      free (helpline1);
      return help;
    }
  return NULL;
}


/* Perform plural expression checking.
   Return the number of errors that were seen.
   If no errors, returns in *DISTRIBUTION information about the plural_eval
   values distribution.  */
static int
check_plural (const message_list_ty *mlp,
              int ignore_untranslated_messages,
              int ignore_fuzzy_messages,
              struct plural_distribution *distributionp,
              xerror_handler_ty xeh)
{
  int seen_errors = 0;

  /* Determine whether mlp has plural entries.  */
  const message_ty *has_plural = NULL;
  unsigned long min_nplurals = ULONG_MAX;
  const message_ty *min_pos = NULL;
  unsigned long max_nplurals = 0;
  const message_ty *max_pos = NULL;
  for (size_t j = 0; j < mlp->nitems; j++)
    {
      message_ty *mp = mlp->item[j];

      if (!mp->obsolete
          && !(ignore_untranslated_messages && mp->msgstr[0] == '\0')
          && !(ignore_fuzzy_messages && (mp->is_fuzzy && !is_header (mp)))
          && mp->msgid_plural != NULL)
        {
          if (has_plural == NULL)
            has_plural = mp;

          unsigned long n;
          {
            n = 0;
            const char *p;
            const char *p_end;
            for (p = mp->msgstr, p_end = p + mp->msgstr_len;
                 p < p_end;
                 p += strlen (p) + 1)
              n++;
          }

          if (min_nplurals > n)
            {
              min_nplurals = n;
              min_pos = mp;
            }
          if (max_nplurals < n)
            {
              max_nplurals = n;
              max_pos = mp;
            }
        }
    }

  struct plural_distribution distribution;
  distribution.expr = NULL;
  distribution.often = NULL;
  distribution.often_length = 0;
  distribution.histogram = NULL;

  /* Look at the plural entry for this domain.
     Cf. function extract_plural_expression.  */
  message_ty *header = message_list_search (mlp, NULL, "");
  if (header != NULL && !header->obsolete)
    {
      const char *nullentry = header->msgstr;

      const char *plural = c_strstr (nullentry, "plural=");
      const char *nplurals = c_strstr (nullentry, "nplurals=");
      if (plural == NULL && has_plural != NULL)
        {
          const char *msg1 =
            _("message catalog has plural form translations");
          const char *msg2 =
            _("but header entry lacks a \"plural=EXPRESSION\" attribute");
          char *help = plural_help (nullentry);

          if (help != NULL)
            {
              char *msg2ext = xasprintf ("%s\n%s", msg2, help);
              xeh->xerror2 (CAT_SEVERITY_ERROR,
                            has_plural, NULL, 0, 0, false, msg1,
                            header, NULL, 0, 0, true, msg2ext);
              free (msg2ext);
              free (help);
            }
          else
            xeh->xerror2 (CAT_SEVERITY_ERROR,
                          has_plural, NULL, 0, 0, false, msg1,
                          header, NULL, 0, 0, false, msg2);

          seen_errors++;
        }
      if (nplurals == NULL && has_plural != NULL)
        {
          const char *msg1 =
            _("message catalog has plural form translations");
          const char *msg2 =
            _("but header entry lacks a \"nplurals=INTEGER\" attribute");
          char *help = plural_help (nullentry);

          if (help != NULL)
            {
              char *msg2ext = xasprintf ("%s\n%s", msg2, help);
              xeh->xerror2 (CAT_SEVERITY_ERROR,
                            has_plural, NULL, 0, 0, false, msg1,
                            header, NULL, 0, 0, true, msg2ext);
              free (msg2ext);
              free (help);
            }
          else
            xeh->xerror2 (CAT_SEVERITY_ERROR,
                          has_plural, NULL, 0, 0, false, msg1,
                          header, NULL, 0, 0, false, msg2);

          seen_errors++;
        }
      if (plural != NULL && nplurals != NULL)
        {
          /* First check the number.  */
          nplurals += 9;
          unsigned long int nplurals_value;
          {
            while (*nplurals != '\0' && c_isspace ((unsigned char) *nplurals))
              ++nplurals;
            const char *endp = nplurals;
            nplurals_value = 0;
            if (*nplurals >= '0' && *nplurals <= '9')
              nplurals_value = strtoul (nplurals, (char **) &endp, 10);
            if (nplurals == endp)
              {
                const char *msg = _("invalid nplurals value");
                char *help = plural_help (nullentry);

                if (help != NULL)
                  {
                    char *msgext = xasprintf ("%s\n%s", msg, help);
                    xeh->xerror (CAT_SEVERITY_ERROR, header, NULL, 0, 0, true,
                                 msgext);
                    free (msgext);
                    free (help);
                  }
                else
                  xeh->xerror (CAT_SEVERITY_ERROR, header, NULL, 0, 0, false,
                               msg);

                seen_errors++;
              }
          }

          /* Then check the expression.  */
          plural += 7;
          const struct expression *plural_expr;
          {
            struct parse_args args;
            args.cp = plural;
            if (parse_plural_expression (&args) != 0)
              {
                const char *msg = _("invalid plural expression");
                char *help = plural_help (nullentry);

                if (help != NULL)
                  {
                    char *msgext = xasprintf ("%s\n%s", msg, help);
                    xeh->xerror (CAT_SEVERITY_ERROR, header, NULL, 0, 0, true,
                                 msgext);
                    free (msgext);
                    free (help);
                  }
                else
                  xeh->xerror (CAT_SEVERITY_ERROR, header, NULL, 0, 0, false,
                               msg);

                seen_errors++;
              }
            plural_expr = args.res;
          }

          /* See whether nplurals and plural fit together.  */
          if (!seen_errors)
            seen_errors =
              check_plural_eval (plural_expr, nplurals_value, header,
                                 &distribution, xeh);

          /* Check the number of plurals of the translations.  */
          if (!seen_errors)
            {
              if (min_nplurals < nplurals_value)
                {
                  char *msg1 =
                    xasprintf (_("nplurals = %lu"), nplurals_value);
                  char *msg2 =
                    xasprintf (ngettext ("but some messages have only one plural form",
                                         "but some messages have only %lu plural forms",
                                         min_nplurals),
                               min_nplurals);
                  xeh->xerror2 (CAT_SEVERITY_ERROR,
                                header, NULL, 0, 0, false, msg1,
                                min_pos, NULL, 0, 0, false, msg2);
                  free (msg2);
                  free (msg1);
                  seen_errors++;
                }
              else if (max_nplurals > nplurals_value)
                {
                  char *msg1 =
                    xasprintf (_("nplurals = %lu"), nplurals_value);
                  char *msg2 =
                    xasprintf (ngettext ("but some messages have one plural form",
                                         "but some messages have %lu plural forms",
                                         max_nplurals),
                               max_nplurals);
                  xeh->xerror2 (CAT_SEVERITY_ERROR,
                                header, NULL, 0, 0, false, msg1,
                                max_pos, NULL, 0, 0, false, msg2);
                  free (msg2);
                  free (msg1);
                  seen_errors++;
                }
              /* The only valid case is max_nplurals <= n <= min_nplurals,
                 which means either has_plural == NULL or
                 max_nplurals = n = min_nplurals.  */
            }
        }
      else
        goto no_plural;
    }
  else
    {
      if (has_plural != NULL)
        {
          xeh->xerror (CAT_SEVERITY_ERROR, has_plural, NULL, 0, 0, false,
                       _("message catalog has plural form translations, but lacks a header entry with \"Plural-Forms: nplurals=INTEGER; plural=EXPRESSION;\""));
          seen_errors++;
        }
     no_plural:
      /* By default, the Germanic formula (n != 1) is used.  */
      distribution.expr = &germanic_plural;
      {
        unsigned char *array = XCALLOC (2, unsigned char);
        array[1] = 1;
        distribution.often = array;
      }
      distribution.often_length = 2;
      distribution.histogram = plural_expression_histogram;
    }

  /* distribution is not needed if we report errors.
     Also, if there was an error due to  max_nplurals > nplurals_value,
     we must not use distribution because we would be doing out-of-bounds
     array accesses.  */
  if (seen_errors > 0)
    free ((unsigned char *) distribution.often);
  else
    *distributionp = distribution;

  return seen_errors;
}


/* Signal an error when checking format strings.  */
struct formatstring_error_logger_locals
{
  xerror_handler_ty xeh;
  const message_ty *curr_mp;
  lex_pos_ty curr_msgid_pos;
};
static void
formatstring_error_logger (void *data, const char *format, ...)
#if (defined __GNUC__ && ((__GNUC__ == 2 && __GNUC_MINOR__ >= 7) || __GNUC__ > 2)) || defined __clang__
     __attribute__ ((__format__ (__printf__, 2, 3)))
#endif
;
static void
formatstring_error_logger (void *data, const char *format, ...)
{
  struct formatstring_error_logger_locals *l =
    (struct formatstring_error_logger_locals *) data;
  va_list args;
  va_start (args, format);

  char *msg;
  if (vasprintf (&msg, format, args) < 0)
    l->xeh->xerror (CAT_SEVERITY_FATAL_ERROR, NULL, NULL, 0, 0, false,
                    _("memory exhausted"));

  va_end (args);
  l->xeh->xerror (CAT_SEVERITY_ERROR,
                  l->curr_mp,
                  l->curr_msgid_pos.file_name, l->curr_msgid_pos.line_number,
                  (size_t)(-1), false, msg);
  free (msg);
}


/* Perform miscellaneous checks on a message.
   PLURAL_DISTRIBUTION is either NULL or an array of nplurals elements,
   PLURAL_DISTRIBUTION[j] being true if the value j appears to be assumed
   infinitely often by the plural formula.
   PLURAL_DISTRIBUTION_LENGTH is the length of the PLURAL_DISTRIBUTION
   array.  */
static int
check_pair (const message_ty *mp,
            const char *msgid,
            const lex_pos_ty *msgid_pos,
            const char *msgid_plural,
            const char *msgstr, size_t msgstr_len,
            const enum is_format is_format[NFORMATS],
            int check_newlines,
            int check_format_strings,
            const struct plural_distribution *distribution,
            int check_compatibility,
            int check_accelerators, char accelerator_char,
            xerror_handler_ty xeh)
{
  /* If the msgid string is empty we have the special entry reserved for
     information about the translation.  */
  if (msgid[0] == '\0')
    return 0;

  int seen_errors = 0;

  if (check_newlines)
    {
      /* Test 1: check whether all or none of the strings begin with a '\n'.  */
      {
        int has_newline = (msgid[0] == '\n');
        #define TEST_NEWLINE(p) (p[0] == '\n')
        if (msgid_plural != NULL)
          {
            if (TEST_NEWLINE(msgid_plural) != has_newline)
              {
                xeh->xerror (CAT_SEVERITY_ERROR,
                             mp, msgid_pos->file_name, msgid_pos->line_number,
                             (size_t)(-1), false,
                             _("'msgid' and 'msgid_plural' entries do not both begin with '\\n'"));
                seen_errors++;
              }
            const char *p;
            unsigned int j;
            for (p = msgstr, j = 0; p < msgstr + msgstr_len; p += strlen (p) + 1, j++)
              if (TEST_NEWLINE(p) != has_newline)
                {
                  char *msg =
                    xasprintf (_("'msgid' and 'msgstr[%u]' entries do not both begin with '\\n'"),
                               j);
                  xeh->xerror (CAT_SEVERITY_ERROR,
                               mp, msgid_pos->file_name, msgid_pos->line_number,
                               (size_t)(-1), false, msg);
                  free (msg);
                  seen_errors++;
                }
          }
        else
          {
            if (TEST_NEWLINE(msgstr) != has_newline)
              {
                xeh->xerror (CAT_SEVERITY_ERROR,
                             mp, msgid_pos->file_name, msgid_pos->line_number,
                             (size_t)(-1), false,
                             _("'msgid' and 'msgstr' entries do not both begin with '\\n'"));
                seen_errors++;
              }
          }
        #undef TEST_NEWLINE
      }

      /* Test 2: check whether all or none of the strings end with a '\n'.  */
      {
        int has_newline = (msgid[strlen (msgid) - 1] == '\n');
        #define TEST_NEWLINE(p) (p[0] != '\0' && p[strlen (p) - 1] == '\n')
        if (msgid_plural != NULL)
          {
            if (TEST_NEWLINE(msgid_plural) != has_newline)
              {
                xeh->xerror (CAT_SEVERITY_ERROR,
                             mp, msgid_pos->file_name, msgid_pos->line_number,
                             (size_t)(-1), false,
                             _("'msgid' and 'msgid_plural' entries do not both end with '\\n'"));
                seen_errors++;
              }
            const char *p;
            unsigned int j;
            for (p = msgstr, j = 0; p < msgstr + msgstr_len; p += strlen (p) + 1, j++)
              if (TEST_NEWLINE(p) != has_newline)
                {
                  char *msg =
                    xasprintf (_("'msgid' and 'msgstr[%u]' entries do not both end with '\\n'"),
                               j);
                  xeh->xerror (CAT_SEVERITY_ERROR,
                               mp, msgid_pos->file_name, msgid_pos->line_number,
                               (size_t)(-1), false, msg);
                  free (msg);
                  seen_errors++;
                }
          }
        else
          {
            if (TEST_NEWLINE(msgstr) != has_newline)
              {
                xeh->xerror (CAT_SEVERITY_ERROR,
                             mp, msgid_pos->file_name, msgid_pos->line_number,
                             (size_t)(-1), false,
                             _("'msgid' and 'msgstr' entries do not both end with '\\n'"));
                seen_errors++;
              }
          }
        #undef TEST_NEWLINE
      }
    }

  if (check_compatibility && msgid_plural != NULL)
    {
      xeh->xerror (CAT_SEVERITY_ERROR,
                   mp, msgid_pos->file_name, msgid_pos->line_number,
                   (size_t)(-1), false,
                   _("plural handling is a GNU gettext extension"));
      seen_errors++;
    }

  if (check_format_strings)
    /* Test 3: Check whether both formats strings contain the same number
       of format specifications.  */
    {
      struct formatstring_error_logger_locals locals;
      locals.xeh = xeh;
      locals.curr_mp = mp;
      locals.curr_msgid_pos = *msgid_pos;
      seen_errors +=
        check_msgid_msgstr_format (msgid, msgid_plural, msgstr, msgstr_len,
                                   is_format, mp->range, distribution,
                                   formatstring_error_logger, &locals);
    }

  if (check_accelerators && msgid_plural == NULL)
    /* Test 4: Check that if msgid is a menu item with a keyboard accelerator,
       the msgstr has an accelerator as well.  A keyboard accelerator is
       designated by an immediately preceding '&'.  We cannot check whether
       two accelerators collide, only whether the translator has bothered
       thinking about them.  */
    {
      /* We are only interested in msgids that contain exactly one '&'.  */
      const char *p = strchr (msgid, accelerator_char);
      if (p != NULL && strchr (p + 1, accelerator_char) == NULL)
        {
          /* Count the number of '&' in msgstr, but ignore '&&'.  */
          unsigned int count = 0;
          for (p = msgstr; (p = strchr (p, accelerator_char)) != NULL; p++)
            if (p[1] == accelerator_char)
              p++;
            else
              count++;

          if (count == 0)
            {
              char *msg =
                xasprintf (_("msgstr lacks the keyboard accelerator mark '%c'"),
                           accelerator_char);
              xeh->xerror (CAT_SEVERITY_ERROR,
                           mp, msgid_pos->file_name, msgid_pos->line_number,
                           (size_t)(-1), false, msg);
              free (msg);
              seen_errors++;
            }
          else if (count > 1)
            {
              char *msg =
                xasprintf (_("msgstr has too many keyboard accelerator marks '%c'"),
                           accelerator_char);
              xeh->xerror (CAT_SEVERITY_ERROR,
                           mp, msgid_pos->file_name, msgid_pos->line_number,
                           (size_t)(-1), false, msg);
              free (msg);
              seen_errors++;
            }
        }
    }

  return seen_errors;
}


/* Perform miscellaneous checks on a header entry.  */
static int
check_header_entry (const message_ty *mp, const char *msgstr_string,
                    xerror_handler_ty xeh)
{
  static const char *required_fields[] =
  {
    "Project-Id-Version", "PO-Revision-Date", "Last-Translator",
    "Language-Team", "MIME-Version", "Content-Type",
    "Content-Transfer-Encoding",
    /* These are recommended but not yet required.  */
    "Language"
  };
  static const char *default_values[] =
  {
    "PACKAGE VERSION", "YEAR-MO-DA HO:MI+ZONE", "FULL NAME <EMAIL@ADDRESS>", "LANGUAGE <LL@li.org>", NULL,
    "text/plain; charset=CHARSET", "ENCODING",
    ""
  };
  const size_t nfields = SIZEOF (required_fields);
  /* FIXME: We could check if a required header field is missing and
     report it as error.  However, it's could be too rigorous and
     break backward compatibility.  */
#if 0
  const size_t nrequiredfields = nfields - 1;
#endif
  int seen_errors = 0;

  for (int cnt = 0; cnt < nfields; ++cnt)
    {
#if 0
      int severity =
        (cnt < nrequiredfields ? CAT_SEVERITY_ERROR : CAT_SEVERITY_WARNING);
#else
      int severity =
        CAT_SEVERITY_WARNING;
#endif
      const char *field = required_fields[cnt];
      size_t len = strlen (field);

      const char *line;
      for (line = msgstr_string; *line != '\0'; )
        {
          if (strncmp (line, field, len) == 0 && line[len] == ':')
            {
              const char *p = line + len + 1;

              /* Test whether the field's value, starting at p, is the default
                 value.  */
              if (*p == ' ')
                p++;
              if (default_values[cnt] != NULL
                  && str_startswith (p, default_values[cnt]))
                {
                  p += strlen (default_values[cnt]);
                  if (*p == '\0' || *p == '\n')
                    {
                      char *msg =
                        xasprintf (_("header field '%s' still has the initial default value\n"),
                                   field);
                      xeh->xerror (severity, mp, NULL, 0, 0, true, msg);
                      free (msg);
                      if (severity == CAT_SEVERITY_ERROR)
                        seen_errors++;
                    }
                }
              break;
            }
          line = strchrnul (line, '\n');
          if (*line == '\n')
            line++;
        }
      if (*line == '\0')
        {
          char *msg =
            xasprintf (_("header field '%s' missing in header\n"),
                       field);
          xeh->xerror (severity, mp, NULL, 0, 0, true, msg);
          free (msg);
          if (severity == CAT_SEVERITY_ERROR)
            seen_errors++;
        }
    }
  return seen_errors;
}


/* Perform all checks on a non-obsolete message.
   Return the number of errors that were seen.  */
int
check_message (const message_ty *mp,
               const lex_pos_ty *msgid_pos,
               int check_newlines,
               int check_format_strings,
               const struct plural_distribution *distribution,
               int check_header,
               int check_compatibility,
               int check_accelerators, char accelerator_char,
               xerror_handler_ty xeh)
{
  int seen_errors = 0;

  if (check_header && is_header (mp))
    seen_errors += check_header_entry (mp, mp->msgstr, xeh);

  seen_errors += check_pair (mp,
                             mp->msgid, msgid_pos, mp->msgid_plural,
                             mp->msgstr, mp->msgstr_len,
                             mp->is_format,
                             check_newlines,
                             check_format_strings,
                             distribution,
                             check_compatibility,
                             check_accelerators, accelerator_char,
                             xeh);
  return seen_errors;
}


/* Perform all checks on a message list.
   Return the number of errors that were seen.  */
int
check_message_list (const message_list_ty *mlp,
                    int ignore_untranslated_messages,
                    int ignore_fuzzy_messages,
                    int check_newlines,
                    int check_format_strings,
                    int check_header,
                    int check_compatibility,
                    int check_accelerators, char accelerator_char,
                    xerror_handler_ty xeh)
{
  int seen_errors = 0;

  struct plural_distribution distribution;
  distribution.expr = NULL;
  distribution.often = NULL;
  distribution.often_length = 0;
  distribution.histogram = NULL;

  if (check_header)
    seen_errors += check_plural (mlp, ignore_untranslated_messages,
                                 ignore_fuzzy_messages, &distribution, xeh);

  for (size_t j = 0; j < mlp->nitems; j++)
    {
      message_ty *mp = mlp->item[j];

      if (!mp->obsolete
          && !(ignore_untranslated_messages && mp->msgstr[0] == '\0')
          && !(ignore_fuzzy_messages && (mp->is_fuzzy && !is_header (mp))))
        seen_errors += check_message (mp, &mp->pos,
                                      check_newlines,
                                      check_format_strings,
                                      &distribution,
                                      check_header, check_compatibility,
                                      check_accelerators, accelerator_char,
                                      xeh);
    }

  return seen_errors;
}
