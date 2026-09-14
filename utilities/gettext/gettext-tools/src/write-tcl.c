/* Writing tcl/msgcat .msg files.
   Copyright (C) 2002-2026 Free Software Foundation, Inc.

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
#include <alloca.h>

/* Specification.  */
#include "write-tcl.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <error.h>
#include "xerror.h"
#include "message.h"
#include "msgl-iconv.h"
#include "xerror-handler.h"
#include "msgl-header.h"
#include "po-charset.h"
#include "xalloc.h"
#include "xmalloca.h"
#include "concat-filename.h"
#include "fwriteerror.h"
#include "unistr.h"
#include "gettext.h"

#define _(str) gettext (str)


static const char hexdigit[] = "0123456789abcdef";

/* Write a string in Tcl 8 Unicode notation to the given stream.
   Tcl 8 uses Unicode for its internal string representation.
   In tcl-8.3.3, the .msg files are read in using the locale dependent
   encoding.  The only way to specify strings in an encoding independent
   form is the \unnnn notation.  Newer tcl versions have this fixed:
   they read the .msg files in UTF-8 encoding.  */
static void
write_tcl8_string (FILE *stream, const char *str)
{
  const char *str_limit = str + strlen (str);

  fprintf (stream, "\"");
  while (str < str_limit)
    {
      ucs4_t uc;
      unsigned int count = u8_mbtouc (&uc, (const unsigned char *) str, str_limit - str);
      if (uc < 0x10000)
        {
          /* Single UCS-2 'char'.  */
          if (uc == 0x000a)
            fprintf (stream, "\\n");
          else if (uc == 0x000d)
            fprintf (stream, "\\r");
          else if (uc == 0x0022)
            fprintf (stream, "\\\"");
          else if (uc == 0x0024)
            fprintf (stream, "\\$");
          else if (uc == 0x005b)
            fprintf (stream, "\\[");
          else if (uc == 0x005c)
            fprintf (stream, "\\\\");
          else if (uc == 0x005d)
            fprintf (stream, "\\]");
          /* Need to escape '{' and '}' because we have opening braces
             outside the strings.  */
          else if (uc == 0x007b)
            fprintf (stream, "\\{");
          else if (uc == 0x007d)
            fprintf (stream, "\\}");
          else if (uc >= 0x0020 && uc < 0x007f)
            fprintf (stream, "%c", (int) uc);
          else
            fprintf (stream, "\\u%c%c%c%c",
                     hexdigit[(uc >> 12) & 0x0f], hexdigit[(uc >> 8) & 0x0f],
                     hexdigit[(uc >> 4) & 0x0f], hexdigit[uc & 0x0f]);
        }
      else
        /* The \unnnn notation doesn't support characters >= 0x10000.
           (See also <https://core.tcl-lang.org/tcl/tktview/d10d6ddf29>.)
           We output them as UTF-8 byte sequences and hope that either
           the Tcl version reading them will be new enough or that the
           user is using an UTF-8 locale.  */
        fwrite (str, 1, count, stream);
      str += count;
    }
  fprintf (stream, "\"");
}


/* Write a string in Tcl 9 Unicode notation to the given stream.
   Tcl 9 uses Unicode for its internal string representation,
   but unlike Tcl 8, requires \Uxxxxxxxx syntax instead of \uxxxx\uyyyy
   syntax (understood by Tcl 8.6) for characters outside the BMP.  */
static void
write_tcl9_string (FILE *stream, const char *str)
{
  const char *str_limit = str + strlen (str);

  fprintf (stream, "\"");
  while (str < str_limit)
    {
      ucs4_t uc;
      unsigned int count = u8_mbtouc (&uc, (const unsigned char *) str, str_limit - str);
      if (uc < 0x10000)
        {
          /* Single UCS-2 'char'.  */
          if (uc == 0x000a)
            fprintf (stream, "\\n");
          else if (uc == 0x000d)
            fprintf (stream, "\\r");
          else if (uc == 0x0022)
            fprintf (stream, "\\\"");
          else if (uc == 0x0024)
            fprintf (stream, "\\$");
          else if (uc == 0x005b)
            fprintf (stream, "\\[");
          else if (uc == 0x005c)
            fprintf (stream, "\\\\");
          else if (uc == 0x005d)
            fprintf (stream, "\\]");
          /* Need to escape '{' and '}' because we have opening braces
             outside the strings.  */
          else if (uc == 0x007b)
            fprintf (stream, "\\{");
          else if (uc == 0x007d)
            fprintf (stream, "\\}");
          else if (uc >= 0x0020 && uc < 0x007f)
            fprintf (stream, "%c", (int) uc);
          else
            fprintf (stream, "\\u%c%c%c%c",
                     hexdigit[(uc >> 12) & 0x0f], hexdigit[(uc >> 8) & 0x0f],
                     hexdigit[(uc >> 4) & 0x0f], hexdigit[uc & 0x0f]);
        }
      else
        fprintf (stream, "\\U%c%c%c%c%c%c%c%c",
                 hexdigit[(uc >> 28) & 0x0f], hexdigit[(uc >> 24) & 0x0f],
                 hexdigit[(uc >> 20) & 0x0f], hexdigit[(uc >> 16) & 0x0f],
                 hexdigit[(uc >> 12) & 0x0f], hexdigit[(uc >> 8) & 0x0f],
                 hexdigit[(uc >> 4) & 0x0f], hexdigit[uc & 0x0f]);
      str += count;
    }
  fprintf (stream, "\"");
}


/* Determine whether a string has no characters outside the Unicode BMP.  */
static bool
is_entirely_ucs2 (const char *str)
{
  const char *str_limit = str + strlen (str);
  while (str < str_limit)
    {
      ucs4_t uc;
      unsigned int count = u8_mbtouc (&uc, (const unsigned char *) str, str_limit - str);
      if (uc >= 0x10000)
        return false;
      str += count;
    }
  return true;
}


/* Write a string either as a Tcl string literal or as a Tcl expression.  */
static void
write_tcl_string (FILE *stream, const char *str)
{
  if (is_entirely_ucs2 (str))
    /* write_tcl8_string and write_tcl9_string produce the same external
       representation for this string.  */
    write_tcl8_string (stream, str);
  else
    {
      /* Use this syntax:
         [expr { $tcl_version < 9 ? "😃" : "\U0001F603" }]
         So that we don't need to assume an UTF-8 locale in Tcl >= 9.0.
         Cf. <https://core.tcl-lang.org/tcl/tktview/d10d6ddf295864389294b966163a23a58b9a1e72>  */
      fprintf (stream, "[expr { $tcl_version < 9 ? ");
      write_tcl8_string (stream, str);
      fprintf (stream, " : ");
      write_tcl9_string (stream, str);
      fprintf (stream, " }]");
    }
}


static void
write_msg (FILE *output_file, message_list_ty *mlp, const char *locale_name)
{
  /* We don't care about esthetic formattic of the output (like respecting
     a maximum line width, or including the translator comments) because
     the \unnnn notation is unesthetic anyway.  Translators shall edit
     the PO file.  */
  for (size_t j = 0; j < mlp->nitems; j++)
    {
      message_ty *mp = mlp->item[j];

      if (is_header (mp))
        /* Tcl's msgcat unit ignores this, but msgunfmt needs it.  */
        fprintf (output_file, "set ::msgcat::header ");
      else
        {
          fprintf (output_file, "::msgcat::mcset %s ", locale_name);
          write_tcl_string (output_file, mp->msgid);
          fprintf (output_file, " ");
        }
      write_tcl_string (output_file, mp->msgstr);
      fprintf (output_file, "\n");
    }
}

int
msgdomain_write_tcl (message_list_ty *mlp, const char *canon_encoding,
                     const char *locale_name,
                     const char *directory)
{
  /* If no entry for this domain don't even create the file.  */
  if (mlp->nitems == 0)
    return 0;

  /* Determine whether mlp has entries with context.  */
  {
    bool has_context = false;
    for (size_t j = 0; j < mlp->nitems; j++)
      if (mlp->item[j]->msgctxt != NULL)
        has_context = true;

    if (has_context)
      {
        multiline_error (xstrdup (""),
                         xstrdup (_("\
message catalog has context dependent translations\n\
but the Tcl message catalog format doesn't support contexts\n")));
        return 1;
      }
  }

  /* Determine whether mlp has plural entries.  */
  {
    bool has_plural = false;
    for (size_t j = 0; j < mlp->nitems; j++)
      if (mlp->item[j]->msgid_plural != NULL)
        has_plural = true;

    if (has_plural)
      {
        multiline_error (xstrdup (""),
                         xstrdup (_("\
message catalog has plural form translations\n\
but the Tcl message catalog format doesn't support plural handling\n")));
        return 1;
      }
  }

  /* Convert the messages to Unicode.  */
  iconv_message_list (mlp, canon_encoding, po_charset_utf8, NULL,
                      textmode_xerror_handler);

  /* Support for "reproducible builds": Delete information that may vary
     between builds in the same conditions.  */
  message_list_delete_header_field (mlp, "POT-Creation-Date:");

  /* Now create the file.  */
  {
    /* Convert the locale name to lowercase and remove any encoding.  */
    size_t len = strlen (locale_name);
    char *frobbed_locale_name = (char *) xmalloca (len + 1);
    {
      memcpy (frobbed_locale_name, locale_name, len + 1);
      for (char *p = frobbed_locale_name; *p != '\0'; p++)
        if (*p >= 'A' && *p <= 'Z')
          *p = *p - 'A' + 'a';
        else if (*p == '.')
          {
            *p = '\0';
            break;
          }
    }

    char *file_name = xconcatenated_filename (directory, frobbed_locale_name, ".msg");

    FILE *output_file = fopen (file_name, "wb");
    if (output_file == NULL)
      {
        error (0, errno, _("error while opening \"%s\" for writing"),
               file_name);
        freea (frobbed_locale_name);
        return 1;
      }

    write_msg (output_file, mlp, frobbed_locale_name);

    /* Make sure nothing went wrong.  */
    if (fwriteerror (output_file))
      error (EXIT_FAILURE, errno, _("error while writing \"%s\" file"),
             file_name);

    freea (frobbed_locale_name);
  }

  return 0;
}
