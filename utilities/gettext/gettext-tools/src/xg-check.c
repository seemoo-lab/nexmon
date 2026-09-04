/* Checking of messages in POT files: so-called "syntax checks".
   Copyright (C) 2015-2026 Free Software Foundation, Inc.

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

/* Written by Daiki Ueno and Bruno Haible.  */

#include <config.h>

/* Specification.  */
#include "xg-check.h"

#include <stdlib.h>
#include <string.h>

#include "xalloc.h"
#include "xvasprintf.h"
#include "message.h"
#include "format.h"
#include "po-xerror.h"
#include "if-error.h"
#include "sentence.h"
#include "c-ctype.h"
#include "c-strstr.h"
#include "unictype.h"
#include "unistr.h"
#include "quote.h"
#include "gettext.h"

#define _(str) gettext (str)

#define SIZEOF(a) (sizeof(a) / sizeof(a[0]))


/* Function that implements a single syntax check.
   MP is a message.
   Returns the number of errors that were seen and reported.  */
typedef int (* syntax_check_function) (const message_ty *mp);


/* ----- Implementation of the sc_ellipsis_unicode syntax check. ----- */

/* Determine whether a string (msgid or msgid_plural) contains an ASCII
   ellipsis.  */
static bool
string_has_ascii_ellipsis (const char *string)
{
  const char *str = string;
  const char *str_limit = str + strlen (string);
  while (str < str_limit)
    {
      ucs4_t ending_char;
      const char *end = sentence_end (str, &ending_char);

      /* sentence_end doesn't treat '...' specially.  */
      const char *cp = end - (ending_char == '.' ? 2 : 3);

      if (cp >= str && memcmp (cp, "...", 3) == 0)
        return true;

      str = end + 1;
    }
  return false;
}

/* Determine whether a message contains an ASCII ellipsis.  */
static bool
message_has_ascii_ellipsis (const message_ty *mp)
{
  return string_has_ascii_ellipsis (mp->msgid)
         || (mp->msgid_plural != NULL
             && string_has_ascii_ellipsis (mp->msgid_plural));
}

static int
syntax_check_ellipsis_unicode (const message_ty *mp)
{
  if (message_has_ascii_ellipsis (mp))
    {
      po_xerror (PO_SEVERITY_ERROR, mp, NULL, 0, 0, false,
                 _("ASCII ellipsis ('...') instead of Unicode"));
      return 1;
    }

  return 0;
}


/* ----- Implementation of the sc_space_ellipsis syntax check. ----- */

/* Determine whether a string (msgid or msgid_plural) contains a space before
   an ellipsis.  */
static bool
string_has_space_ellipsis (const char *string)
{
  const char *str = string;
  const char *str_limit = str + strlen (string);
  while (str < str_limit)
    {
      ucs4_t ending_char;
      const char *end = sentence_end (str, &ending_char);

      const char *ellipsis = NULL;
      if (ending_char == 0x2026)
        ellipsis = end;
      else if (ending_char == '.')
        {
          /* sentence_end doesn't treat '...' specially.  */
          const char *cp = end - 2;
          if (cp >= str && memcmp (cp, "...", 3) == 0)
            ellipsis = cp;
        }
      else
        {
          /* Look for a '...'.  */
          const char *cp = end - 3;
          if (cp >= str && memcmp (cp, "...", 3) == 0)
            ellipsis = cp;
          else
            {
              /* Look for a U+2026.  */
              ucs4_t uc = 0xfffd;
              for (cp = end - 1; cp >= str; cp--)
                {
                  u8_mbtouc (&uc, (const unsigned char *) cp, end - cp);
                  if (uc != 0xfffd)
                    break;
                }

              if (uc == 0x2026)
                ellipsis = cp;
            }
        }

      if (ellipsis)
        {
          /* Look at the character before ellipsis.  */
          ucs4_t uc = 0xfffd;
          for (const char *cp = ellipsis - 1; cp >= str; cp--)
            {
              u8_mbtouc (&uc, (const unsigned char *) cp, ellipsis - cp);
              if (uc != 0xfffd)
                break;
            }

          if (uc != 0xfffd && uc_is_space (uc))
            return true;
        }

      str = end + 1;
    }
  return false;
}

/* Determine whether a message contains a space before an ellipsis.  */
static bool
message_has_space_ellipsis (const message_ty *mp)
{
  return string_has_space_ellipsis (mp->msgid)
         || (mp->msgid_plural != NULL
             && string_has_space_ellipsis (mp->msgid_plural));
}

static int
syntax_check_space_ellipsis (const message_ty *mp)
{
  if (message_has_space_ellipsis (mp))
    {
      po_xerror (PO_SEVERITY_ERROR, mp, NULL, 0, 0, false,
                 _("space before ellipsis found in user visible string"));
      return 1;
    }

  return 0;
}


/* ----- Implementation of the sc_quote_unicode syntax check. ----- */

struct callback_arg
{
  const message_ty *mp;
  int seen_errors;
};

static void
syntax_check_quote_unicode_callback (char quote, const char *quoted,
                                     size_t quoted_length, void *data)
{
  struct callback_arg *arg = data;

  switch (quote)
    {
    case '"':
      po_xerror (PO_SEVERITY_ERROR, arg->mp, NULL, 0, 0, false,
                 _("ASCII double quote used instead of Unicode"));
      arg->seen_errors++;
      break;

    case '\'':
      po_xerror (PO_SEVERITY_ERROR, arg->mp, NULL, 0, 0, false,
                 _("ASCII single quote used instead of Unicode"));
      arg->seen_errors++;
      break;

    default:
      break;
    }
}

static int
syntax_check_quote_unicode (const message_ty *mp)
{
  struct callback_arg arg;
  arg.mp = mp;
  arg.seen_errors = 0;

  scan_quoted (mp->msgid, strlen (mp->msgid),
               syntax_check_quote_unicode_callback, &arg);
  if (mp->msgid_plural != NULL)
    scan_quoted (mp->msgid_plural, strlen (mp->msgid_plural),
                 syntax_check_quote_unicode_callback, &arg);

  return arg.seen_errors;
}


/* ----- Implementation of the sc_bullet_unicode syntax check. ----- */

struct bullet_ty
{
  int c;
  size_t depth;
};

struct bullet_stack_ty
{
  struct bullet_ty *items;
  size_t nitems;
  size_t nitems_max;
};

static struct bullet_stack_ty bullet_stack;

static int
syntax_check_bullet_unicode_string (const message_ty *mp, const char *msgid)
{
  bool seen_error = false;

  bullet_stack.nitems = 0;
  struct bullet_ty *last_bullet = NULL;

  {
    const char *str = msgid;
    const char *str_limit = str + strlen (msgid);
    while (str < str_limit)
      {
        const char *p = str;

        while (p < str_limit && c_isspace (*p))
          p++;

        if ((*p == '*' || *p == '-') && *(p + 1) == ' ')
          {
            size_t depth = p - str;
            if (last_bullet == NULL || depth > last_bullet->depth)
              {
                struct bullet_ty bullet;
                bullet.c = *p;
                bullet.depth = depth;

                if (bullet_stack.nitems >= bullet_stack.nitems_max)
                  {
                    bullet_stack.nitems_max = 2 * bullet_stack.nitems_max + 4;
                    bullet_stack.items = xrealloc (bullet_stack.items,
                                                   bullet_stack.nitems_max
                                                   * sizeof (struct bullet_ty));
                  }

                last_bullet = &bullet_stack.items[bullet_stack.nitems++];
                memcpy (last_bullet, &bullet, sizeof (struct bullet_ty));
              }
            else
              {
                if (depth < last_bullet->depth)
                  {
                    if (bullet_stack.nitems > 1)
                      {
                        bullet_stack.nitems--;
                        last_bullet =
                          &bullet_stack.items[bullet_stack.nitems - 1];
                      }
                    else
                      last_bullet = NULL;
                  }

                if (last_bullet && depth == last_bullet->depth)
                  {
                    if (last_bullet->c != *p)
                      last_bullet->c = *p;
                    else
                      {
                        seen_error = true;
                        break;
                      }
                  }
              }
          }
        else
          {
            bullet_stack.nitems = 0;
            last_bullet = NULL;
          }

        const char *end = strchrnul (str, '\n');
        str = end + 1;
      }
  }

  if (seen_error)
    {
      char *msg = xasprintf (_("ASCII bullet ('%c') instead of Unicode"),
                             last_bullet->c);
      po_xerror (PO_SEVERITY_ERROR, mp, NULL, 0, 0, false, msg);
      free (msg);
      return 1;
    }

  return 0;
}

static int
syntax_check_bullet_unicode (const message_ty *mp)
{
  return syntax_check_bullet_unicode_string (mp, mp->msgid)
         + (mp->msgid_plural != NULL
            ? syntax_check_bullet_unicode_string (mp, mp->msgid_plural)
            : 0);
}


/* ----- Implementation of the sc_url syntax check. ----- */
/* This check is enabled by default.  It produces a warning, not an error.  */

/* Determine whether a string (msgid or msgid_plural) contains a URL.  */
static bool
string_has_url (const char *string)
{
  /* Test for the common pattern of URLs that reside on the internet
     (not "file:").  */
  static const char *patterns[] =
  {
    /* We can afford to be silent about 'mailto:' here, because it is
       almost always followed by an email address, that we report though
       the sc_email check.  */
    #if 0
    "mailto:",
    #endif
    "http://", "https://",
    "ftp://",
    "irc://", "ircs://"
  };

  for (size_t i = 0; i < SIZEOF (patterns); i++)
    {
      const char *pattern = patterns[i];
      /* msgid and msgid_plural are typically entirely ASCII.  Therefore here
         it's OK to use the <c-ctype.h> functions; no need for UTF-8 aware
         <unictype.h> functions.  */
      for (const char *string_tail = string;;)
        {
          const char *found = c_strstr (string_tail, pattern);
          if (found == NULL)
            break;
          /* Test whether the pattern starts at a word boundary.  */
          if (found == string_tail || !(c_isalnum (found[-1]) || found[-1] == '_'))
            {
              /* Find the end of the URL.  */
              const char *found_end = found + strlen (pattern);
              const char *p = found_end;
              while (*p != '\0'
                     && !(c_isspace (*p) || *p == '<' || *p == '>' || *p == '"'))
                p++;
              if (p > found_end)
                {
                  /* Here *p == '\0' or
                     (c_isspace (*p) || *p == '<' || *p == '>' || *p == '"').
                     This implies !(c_isalnum (*p) || *p == '_').  */
                  /* In case of a "mailto" URL, test for a '@'.  */
                  if (!(i == 0) || memchr (found, '@', p - found_end) != NULL)
                    {
                      /* Yes, it looks like a URL.  */
                      return true;
                    }
                }
            }
          string_tail = found + 1;
        }
    }

  return false;
}

/* Determine whether a message contains a URL.  */
static bool
message_has_url (const message_ty *mp)
{
  return string_has_url (mp->msgid)
         || (mp->msgid_plural != NULL && string_has_url (mp->msgid_plural));
}

static int
syntax_check_url (const message_ty *mp)
{
  if (message_has_url (mp))
    if_error (IF_SEVERITY_WARNING,
              mp->pos.file_name, mp->pos.line_number, (size_t)(-1), false,
              _("Message contains an embedded URL.  Better move it out of the translatable string, see %s"),
              "https://www.gnu.org/software/gettext/manual/html_node/No-embedded-URLs.html");
  return 0;
}


/* ----- Implementation of the sc_email syntax check. ----- */
/* This check is enabled by default.  It produces a warning, not an error.  */

/* Determine whether a string (msgid or msgid_plural) contains an
   email address.  */
static bool
string_has_email (const char *string)
{
  for (const char *string_tail = string;;)
    {
      /* An email address consists of LOCALPART@DOMAIN.  */
      const char *at = strchr (string_tail, '@');
      if (at == NULL)
        break;
      /* Find the start of the email address.  */
      const char *start;
      {
        const char *p = at;
        while (p > string)
          {
            char c = p[-1];
            if (!(c_isalnum (c)
                  || c == '!' || c == '#' || c == '$' || c == '%' || c == '&'
                  || c == '\'' || c == '*' || c == '+' || c == '-' || c == '.'
                  || c == '/' || c == '=' || c == '?' || c == '^' || c == '_'
                  || c == '`' || c == '{' || c == '|' || c == '}' || c == '~'))
              break;
            /* Consecutive dots not allowed.  */
            if (c == '.' && p[0] == '.')
              break;
            p--;
          }
        start = p;
      }
      if (start < at && start[0] != '.' && at[-1] != '.')
        {
          /* Find the end of the email address.  */
          const char *end;
          const char *last_dot_in_domain = NULL;
          {
            const char *p = at + 1;
            while (*p != '\0')
              {
                char c = *p;
                if (!(c_isalnum (c) || c == '-' || c == '.'))
                  break;
                /* Consecutive dots not allowed.  */
                if (c == '.' && p[-1] == '.')
                  break;
                if (c == '.')
                  last_dot_in_domain = p;
                p++;
              }
            end = p;
          }
          if (at + 1 < end && at[1] != '.' && end[-1] != '.'
              /* The domain should contain a dot.  */
              && last_dot_in_domain != NULL
              /* We can't enumerate all the possible top-level domains, but at
                 least we know that they are all 2 or more characters long.  */
              && end - (last_dot_in_domain + 1) >= 2)
            {
              /* Yes, it looks like an email address.  */
              return true;
            }
        }
      string_tail = at + 1;
    }

  return false;
}

/* Determine whether a message contains an email address.  */
static bool
message_has_email (const message_ty *mp)
{
  return string_has_email (mp->msgid)
         || (mp->msgid_plural != NULL && string_has_email (mp->msgid_plural));
}

static int
syntax_check_email (const message_ty *mp)
{
  if (message_has_email (mp))
    if_error (IF_SEVERITY_WARNING,
              mp->pos.file_name, mp->pos.line_number, (size_t)(-1), false,
              _("Message contains an embedded email address.  Better move it out of the translatable string, see %s"),
              "https://www.gnu.org/software/gettext/manual/html_node/No-embedded-URLs.html");

  return 0;
}


/* ---------------------- List of all syntax checks. ---------------------- */
static const syntax_check_function sc_funcs[NSYNTAXCHECKS] =
{
  syntax_check_ellipsis_unicode,
  syntax_check_space_ellipsis,
  syntax_check_quote_unicode,
  syntax_check_bullet_unicode,
  syntax_check_url,
  syntax_check_email
};


/* Perform all syntax checks on a non-obsolete message.
   Return the number of errors that were seen.  */
static int
syntax_check_message (const message_ty *mp)
{
  int seen_errors = 0;

  for (int i = 0; i < NSYNTAXCHECKS; i++)
    if (mp->do_syntax_check[i] == yes)
      seen_errors += sc_funcs[i] (mp);

  return seen_errors;
}


/* Signal an error when checking format strings.  */
struct formatstring_error_logger_locals
{
  const lex_pos_ty *pos;
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
  if_verror (IF_SEVERITY_ERROR,
             l->pos->file_name, l->pos->line_number, (size_t)(-1), false,
             format, args);
  va_end (args);
}


/* Perform all format checks on a non-obsolete message.
   Return the number of errors that were seen.  */
static int
format_check_message (const message_ty *mp)
{
  int seen_errors = 0;

  if (mp->msgid_plural != NULL)
    {
      /* Look for format string incompatibilities between msgid and
         msgid_plural.  */
      for (size_t i = 0; i < NFORMATS; i++)
        if (possible_format_p (mp->is_format[i]))
          {
            struct formatstring_parser *parser = formatstring_parsers[i];
            char *invalid_reason1 = NULL;
            void *descr1 =
              parser->parse (mp->msgid, false, NULL, &invalid_reason1);
            char *invalid_reason2 = NULL;
            void *descr2 =
              parser->parse (mp->msgid_plural, false, NULL, &invalid_reason2);

            if (descr1 != NULL && descr2 != NULL)
              {
                struct formatstring_error_logger_locals locals;
                locals.pos = &mp->pos;
                if (parser->check (descr2, descr1, false,
                                   formatstring_error_logger, &locals,
                                   "msgid_plural", "msgid"))
                  seen_errors++;
              }

            if (descr2 != NULL)
              parser->free (descr2);
            else
              free (invalid_reason2);
            if (descr1 != NULL)
              parser->free (descr1);
            else
              free (invalid_reason1);
          }
      }

  return seen_errors;
}


/* Perform all checks on a message list.
   Return the number of errors that were seen.  */
int
xgettext_check_message_list (message_list_ty *mlp)
{
  int seen_errors = 0;

  for (size_t j = 0; j < mlp->nitems; j++)
    {
      message_ty *mp = mlp->item[j];

      if (!is_header (mp))
        {
          seen_errors += syntax_check_message (mp) + format_check_message (mp);
        }
    }

  return seen_errors;
}
