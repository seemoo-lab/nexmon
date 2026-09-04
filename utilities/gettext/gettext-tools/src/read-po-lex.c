/* GNU gettext - internationalization aids
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

/* Specification.  */
#include "read-po-lex.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#if HAVE_ICONV
# include <iconv.h>
#endif

#include <error.h>
#include "attribute.h"
#include "c-ctype.h"
#include "uniwidth.h"
#include "gettext.h"
#include "po-charset.h"
#include "xalloc.h"
#include "xvasprintf.h"
#include "xstrerror.h"
#include "po-error.h"
#include "xerror-handler.h"
#include "xmalloca.h"
#if !IN_LIBGETTEXTPO
# include "basename-lgpl.h"
# include "progname.h"
#endif
#include "c-strstr.h"
#include "pos.h"
#include "message.h"
#include "str-list.h"
#include "read-po.h"
#include "read-po-internal.h"
#include "read-po-gram.h"

#define _(str) gettext(str)

#if HAVE_DECL_GETC_UNLOCKED
# undef getc
# define getc getc_unlocked
#endif


/* Error handling during the parsing of a PO file.
   These functions can access ps->gram_pos and ps->gram_pos_column.  */

void
po_gram_error (struct po_parser_state *ps, const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  char *buffer;
  if (vasprintf (&buffer, fmt, ap) < 0)
    ps->catr->xeh->xerror (CAT_SEVERITY_FATAL_ERROR, NULL, NULL, 0, 0, false,
                           _("memory exhausted"));
  va_end (ap);
  ps->catr->xeh->xerror (CAT_SEVERITY_ERROR, NULL,
                         ps->gram_pos.file_name, ps->gram_pos.line_number,
                         ps->gram_pos_column + 1, false, buffer);
  free (buffer);

  if (*(ps->catr->xeh->error_message_count_p) >= gram_max_allowed_errors)
    ps->catr->xeh->xerror (CAT_SEVERITY_FATAL_ERROR, NULL, NULL, 0, 0, false,
                           _("too many errors, aborting"));
}

void
po_gram_error_at_line (abstract_catalog_reader_ty *catr, const lex_pos_ty *pp,
                       const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  char *buffer;
  if (vasprintf (&buffer, fmt, ap) < 0)
    catr->xeh->xerror (CAT_SEVERITY_FATAL_ERROR, NULL, NULL, 0, 0, false,
                       _("memory exhausted"));
  va_end (ap);
  catr->xeh->xerror (CAT_SEVERITY_ERROR, NULL, pp->file_name, pp->line_number,
                     (size_t)(-1), false, buffer);
  free (buffer);

  if (*(catr->xeh->error_message_count_p) >= gram_max_allowed_errors)
    catr->xeh->xerror (CAT_SEVERITY_FATAL_ERROR, NULL, NULL, 0, 0, false,
                       _("too many errors, aborting"));
}


/* Charset handling while parsing PO files.  */

/* Initialize the PO file's encoding.  */
static void
po_lex_charset_init (struct po_parser_state *ps)
{
  ps->po_lex_charset = NULL;
  ps->catr->po_lex_isolate_start = NULL;
  ps->catr->po_lex_isolate_end = NULL;
#if HAVE_ICONV
  ps->po_lex_iconv = (iconv_t)(-1);
#endif
  ps->po_lex_weird_cjk = false;
}

/* Set the PO file's encoding from the header entry.
   If is_pot_role is true, "charset=CHARSET" is expected and does not deserve
   a warning.  */
void
po_lex_charset_set (struct po_parser_state *ps,
                    const char *header_entry,
                    const char *filename, bool is_pot_role)
{
  /* Verify the validity of CHARSET.  It is necessary
     1. for the correct treatment of multibyte characters containing
        0x5C bytes in the PO lexer,
     2. so that at run time, gettext() can call iconv() to convert
        msgstr.  */
  const char *charsetstr = c_strstr (header_entry, "charset=");

  if (charsetstr != NULL)
    {
      charsetstr += strlen ("charset=");
      size_t len = strcspn (charsetstr, " \t\n");

      char *charset = (char *) xmalloca (len + 1);
      memcpy (charset, charsetstr, len);
      charset[len] = '\0';

      const char *canon_charset = po_charset_canonicalize (charset);
      if (canon_charset == NULL)
        {
          /* Don't warn for POT files, because POT files usually contain
             only ASCII msgids.  */
          size_t filenamelen = strlen (filename);

          if (!(strcmp (charset, "CHARSET") == 0
                && ((filenamelen >= 4
                     && memcmp (filename + filenamelen - 4, ".pot", 4) == 0)
                    || is_pot_role)))
            {
              char *warning_message =
                xasprintf (_("\
Charset \"%s\" is not a portable encoding name.\n\
Message conversion to user's charset might not work.\n"),
                           charset);
              ps->catr->xeh->xerror (CAT_SEVERITY_WARNING, NULL,
                                     filename, (size_t)(-1), (size_t)(-1), true,
                                     warning_message);
              free (warning_message);
            }
        }
      else
        {
          ps->po_lex_charset = canon_charset;

          if (strcmp (canon_charset, "UTF-8") == 0)
            {
              ps->catr->po_lex_isolate_start = "\xE2\x81\xA8";
              ps->catr->po_lex_isolate_end = "\xE2\x81\xA9";
            }
          else if (strcmp (canon_charset, "GB18030") == 0)
            {
              ps->catr->po_lex_isolate_start = "\x81\x36\xAC\x34";
              ps->catr->po_lex_isolate_end = "\x81\x36\xAC\x35";
            }
          else
            {
              /* The other encodings don't contain U+2068, U+2069.  */
              ps->catr->po_lex_isolate_start = NULL;
              ps->catr->po_lex_isolate_end = NULL;
            }

#if HAVE_ICONV
          if (ps->po_lex_iconv != (iconv_t)(-1))
            iconv_close (ps->po_lex_iconv);
#endif

          /* The old Solaris/openwin msgfmt and GNU msgfmt <= 0.10.35
             don't know about multibyte encodings, and require a spurious
             backslash after every multibyte character whose last byte is
             0x5C.  Some programs, like vim, distribute PO files in this
             broken format.  GNU msgfmt must continue to support this old
             PO file format when the Makefile requests it.  */
          const char *envval = getenv ("OLD_PO_FILE_INPUT");
          if (envval != NULL && *envval != '\0')
            {
              /* Assume the PO file is in old format, with extraneous
                 backslashes.  */
#if HAVE_ICONV
              ps->po_lex_iconv = (iconv_t)(-1);
#endif
              ps->po_lex_weird_cjk = false;
            }
          else
            {
              /* Use iconv() to parse multibyte characters.  */
#if HAVE_ICONV
              ps->po_lex_iconv = iconv_open ("UTF-8", ps->po_lex_charset);
              if (ps->po_lex_iconv == (iconv_t)(-1))
                {
                  const char *progname;
# if IN_LIBGETTEXTPO
                  progname = "libgettextpo";
# else
                  progname = last_component (program_name);
# endif

                  char *warning_message =
                    xasprintf (_("\
Charset \"%s\" is not supported. %s relies on iconv(),\n\
and iconv() does not support \"%s\".\n"),
                               ps->po_lex_charset, progname, ps->po_lex_charset);

                  const char *recommendation;
# if !defined _LIBICONV_VERSION || (_LIBICONV_VERSION == 0x10b && defined __APPLE__)
                  recommendation = _("\
Installing GNU libiconv and then reinstalling GNU gettext\n\
would fix this problem.\n");
# else
                  recommendation = "";
# endif

                  /* Test for a charset which has double-byte characters
                     ending in 0x5C.  For these encodings, the string parser
                     is likely to be confused if it can't see the character
                     boundaries.  */
                  ps->po_lex_weird_cjk = po_is_charset_weird_cjk (ps->po_lex_charset);
                  const char *note;
                  if (po_is_charset_weird (ps->po_lex_charset)
                      && !ps->po_lex_weird_cjk)
                    note = _("Continuing anyway, expect parse errors.");
                  else
                    note = _("Continuing anyway.");

                  char *whole_message =
                    xasprintf ("%s%s%s\n",
                               warning_message, recommendation, note);

                  ps->catr->xeh->xerror (CAT_SEVERITY_WARNING, NULL,
                                         filename, (size_t)(-1), (size_t)(-1),
                                         true, whole_message);

                  free (whole_message);
                  free (warning_message);
                }
#else
              /* Test for a charset which has double-byte characters
                 ending in 0x5C.  For these encodings, the string parser
                 is likely to be confused if it can't see the character
                 boundaries.  */
              ps->po_lex_weird_cjk = po_is_charset_weird_cjk (ps->po_lex_charset);
              if (po_is_charset_weird (ps->po_lex_charset) && !ps->po_lex_weird_cjk)
                {
                  const char *progname;
# if IN_LIBGETTEXTPO
                  progname = "libgettextpo";
# else
                  progname = last_component (program_name);
# endif

                  char *warning_message =
                    xasprintf (_("\
Charset \"%s\" is not supported. %s relies on iconv().\n\
This version was built without iconv().\n"),
                               ps->po_lex_charset, progname);

                  const char *recommendation = _("\
Installing GNU libiconv and then reinstalling GNU gettext\n\
would fix this problem.\n");

                  const char *note = _("Continuing anyway, expect parse errors.");

                  char *whole_message =
                    xasprintf ("%s%s%s\n",
                               warning_message, recommendation, note);

                  ps->catr->xeh->xerror (CAT_SEVERITY_WARNING, NULL,
                                         filename, (size_t)(-1), (size_t)(-1),
                                         true, whole_message);

                  free (whole_message);
                  free (warning_message);
                }
#endif
            }
        }
      freea (charset);
    }
  else
    {
      /* Don't warn for POT files, because POT files usually contain
         only ASCII msgids.  */
      size_t filenamelen = strlen (filename);

      if (!(filenamelen >= 4
            && memcmp (filename + filenamelen - 4, ".pot", 4) == 0))
        ps->catr->xeh->xerror (CAT_SEVERITY_WARNING,
                               NULL, filename, (size_t)(-1), (size_t)(-1), true,
                               _("\
Charset missing in header.\n\
Message conversion to user's charset will not work.\n"));
    }
}

/* Finish up with the PO file's encoding.  */
static void
po_lex_charset_close (struct po_parser_state *ps)
{
  ps->po_lex_charset = NULL;
  ps->catr->po_lex_isolate_start = NULL;
  ps->catr->po_lex_isolate_end = NULL;
#if HAVE_ICONV
  if (ps->po_lex_iconv != (iconv_t)(-1))
    {
      iconv_close (ps->po_lex_iconv);
      ps->po_lex_iconv = (iconv_t)(-1);
    }
#endif
  ps->po_lex_weird_cjk = false;
}


/* The lowest level of PO file parsing converts bytes to multibyte characters.
   This is needed
   1. for C compatibility: ISO C 99 section 5.1.1.2 says that the first
      translation phase maps bytes to characters.
   2. to keep track of the current column, for the sake of precise error
      location. Emacs compile.el interprets the column in error messages
      by default as a screen column number, not as character number.
   3. to avoid skipping backslash-newline in the midst of a multibyte
      character. If XY is a multibyte character,  X \ newline Y  is invalid.
 */

/* A version of memcpy optimized for the case n <= 1.  */
static inline void
memcpy_small (void *dst, const void *src, size_t n)
{
  if (n > 0)
    {
      char *q = (char *) dst;
      const char *p = (const char *) src;

      *q = *p;
      if (--n > 0)
        do *++q = *++p; while (--n > 0);
    }
}

/* EOF (not a real character) is represented with bytes = 0 and
   uc_valid = false.  */
static inline bool
mb_iseof (const mbchar_t mbc)
{
  return (mbc->bytes == 0);
}

/* Access the current character.  */
static inline const char *
mb_ptr (const mbchar_t mbc)
{
  return mbc->buf;
}
static inline size_t
mb_len (const mbchar_t mbc)
{
  return mbc->bytes;
}

/* Comparison of characters.  */

static inline bool
mb_iseq (const mbchar_t mbc, char sc)
{
  /* Note: It is wrong to compare only mbc->uc, because when the encoding is
     SHIFT_JIS, mbc->buf[0] == '\\' corresponds to mbc->uc == 0x00A5, but we
     want to treat it as an escape character, although it looks like a Yen
     sign.  */
#if HAVE_ICONV && 0
  if (mbc->uc_valid)
    return (mbc->uc == sc); /* wrong! */
  else
#endif
    return (mbc->bytes == 1 && mbc->buf[0] == sc);
}

MAYBE_UNUSED static inline bool
mb_isnul (const mbchar_t mbc)
{
#if HAVE_ICONV
  if (mbc->uc_valid)
    return (mbc->uc == 0);
  else
#endif
    return (mbc->bytes == 1 && mbc->buf[0] == 0);
}

MAYBE_UNUSED static inline int
mb_cmp (const mbchar_t mbc1, const mbchar_t mbc2)
{
#if HAVE_ICONV
  if (mbc1->uc_valid && mbc2->uc_valid)
    return (int) mbc1->uc - (int) mbc2->uc;
  else
#endif
    return (mbc1->bytes == mbc2->bytes
            ? memcmp (mbc1->buf, mbc2->buf, mbc1->bytes)
            : mbc1->bytes < mbc2->bytes
              ? (memcmp (mbc1->buf, mbc2->buf, mbc1->bytes) > 0 ? 1 : -1)
              : (memcmp (mbc1->buf, mbc2->buf, mbc2->bytes) >= 0 ? 1 : -1));
}

MAYBE_UNUSED static inline bool
mb_equal (const mbchar_t mbc1, const mbchar_t mbc2)
{
#if HAVE_ICONV
  if (mbc1->uc_valid && mbc2->uc_valid)
    return mbc1->uc == mbc2->uc;
  else
#endif
    return (mbc1->bytes == mbc2->bytes
            && memcmp (mbc1->buf, mbc2->buf, mbc1->bytes) == 0);
}

/* <ctype.h>, <wctype.h> classification.  */

MAYBE_UNUSED static inline bool
mb_isascii (const mbchar_t mbc)
{
#if HAVE_ICONV
  if (mbc->uc_valid)
    return (mbc->uc >= 0x0000 && mbc->uc <= 0x007F);
  else
#endif
    return (mbc->bytes == 1
#if CHAR_MIN < 0x00 /* to avoid gcc warning */
            && mbc->buf[0] >= 0x00
#endif
#if CHAR_MAX > 0x7F /* to avoid gcc warning */
            && mbc->buf[0] <= 0x7F
#endif
           );
}

/* Extra <wchar.h> function.  */

/* Unprintable characters appear as a small box of width 1.  */
#define MB_UNPRINTABLE_WIDTH 1

static int
mb_width (struct po_parser_state *ps, const mbchar_t mbc)
{
#if HAVE_ICONV
  if (mbc->uc_valid)
    {
      ucs4_t uc = mbc->uc;
      const char *encoding =
        (ps->po_lex_iconv != (iconv_t)(-1) ? ps->po_lex_charset : "");
      int w = uc_width (uc, encoding);
      /* For unprintable characters, arbitrarily return 0 for control
         characters (except tab) and MB_UNPRINTABLE_WIDTH otherwise.  */
      if (w >= 0)
        return w;
      if (uc >= 0x0000 && uc <= 0x001F)
        {
          if (uc == 0x0009)
            return 8 - (ps->gram_pos_column & 7);
          return 0;
        }
      if ((uc >= 0x007F && uc <= 0x009F) || (uc >= 0x2028 && uc <= 0x2029))
        return 0;
      return MB_UNPRINTABLE_WIDTH;
    }
  else
#endif
    {
      if (mbc->bytes == 1)
        {
          if (
#if CHAR_MIN < 0x00 /* to avoid gcc warning */
              mbc->buf[0] >= 0x00 &&
#endif
              mbc->buf[0] <= 0x1F)
            {
              if (mbc->buf[0] == 0x09)
                return 8 - (ps->gram_pos_column & 7);
              return 0;
            }
          if (mbc->buf[0] == 0x7F)
            return 0;
        }
      return MB_UNPRINTABLE_WIDTH;
    }
}

/* Output.  */
MAYBE_UNUSED static inline void
mb_putc (const mbchar_t mbc, FILE *stream)
{
  fwrite (mbc->buf, 1, mbc->bytes, stream);
}

/* Assignment.  */
MAYBE_UNUSED static inline void
mb_setascii (mbchar_t mbc, char sc)
{
  mbc->bytes = 1;
#if HAVE_ICONV
  mbc->uc_valid = 1;
  mbc->uc = sc;
#endif
  mbc->buf[0] = sc;
}

/* Copying a character.  */
static inline void
mb_copy (mbchar_t new_mbc, const mbchar_t old_mbc)
{
  memcpy_small (&new_mbc->buf[0], &old_mbc->buf[0], old_mbc->bytes);
  new_mbc->bytes = old_mbc->bytes;
#if HAVE_ICONV
  if ((new_mbc->uc_valid = old_mbc->uc_valid))
    new_mbc->uc = old_mbc->uc;
#endif
}


/* Multibyte character input.  */

static inline void
mbfile_init (mbfile_t mbf, FILE *stream)
{
  mbf->fp = stream;
  mbf->eof_seen = false;
  mbf->pushback_count = 0;
  mbf->bufcount = 0;
}

/* Read the next multibyte character from mbf and put it into mbc.
   If a read error occurs, errno is set and ferror (mbf->fp) becomes true.  */
static void
mbfile_getc (struct po_parser_state *ps, mbchar_t mbc, mbfile_t mbf)
{
  /* Return character pushed back, if there is one.  */
  if (mbf->pushback_count > 0)
    {
      mbf->pushback_count--;
      mb_copy (mbc, &mbf->pushback[mbf->pushback_count]);
      return;
    }

  /* If EOF has already been seen, don't use getc.  This matters if
     mbf->fp is connected to an interactive tty.  */
  if (mbf->eof_seen)
    goto eof;

  /* Before using iconv, we need at least one byte.  */
  if (mbf->bufcount == 0)
    {
      int c = getc (mbf->fp);
      if (c == EOF)
        {
          mbf->eof_seen = true;
          goto eof;
        }
      mbf->buf[0] = (unsigned char) c;
      mbf->bufcount++;
    }

  size_t bytes;

#if HAVE_ICONV
  if (ps->po_lex_iconv != (iconv_t)(-1))
    {
      /* Use iconv on an increasing number of bytes.  Read only as many
         bytes from mbf->fp as needed.  This is needed to give reasonable
         interactive behaviour when mbf->fp is connected to an interactive
         tty.  */
      for (;;)
        {
          unsigned char scratchbuf[64];
          const char *inptr = &mbf->buf[0];
          size_t insize = mbf->bufcount;
          char *outptr = (char *) &scratchbuf[0];
          size_t outsize = sizeof (scratchbuf);

          size_t res = iconv (ps->po_lex_iconv,
                              (ICONV_CONST char **) &inptr, &insize,
                              &outptr, &outsize);
          /* We expect that a character has been produced if and only if
             some input bytes have been consumed.  */
          if ((insize < mbf->bufcount) != (outsize < sizeof (scratchbuf)))
            abort ();
          if (outsize == sizeof (scratchbuf))
            {
              /* No character has been produced.  Must be an error.  */
              if (res != (size_t)(-1))
                abort ();

              if (errno == EILSEQ)
                {
                  /* An invalid multibyte sequence was encountered.  */
                  /* Return a single byte.  */
                  if (ps->signal_eilseq)
                    po_gram_error (ps, _("invalid multibyte sequence"));
                  bytes = 1;
                  mbc->uc_valid = false;
                  break;
                }
              else if (errno == EINVAL)
                {
                  /* An incomplete multibyte character.  */
                  int c;

                  if (mbf->bufcount == MBCHAR_BUF_SIZE)
                    {
                      /* An overlong incomplete multibyte sequence was
                         encountered.  */
                      /* Return a single byte.  */
                      bytes = 1;
                      mbc->uc_valid = false;
                      break;
                    }

                  /* Read one more byte and retry iconv.  */
                  c = getc (mbf->fp);
                  if (c == EOF)
                    {
                      mbf->eof_seen = true;
                      if (ferror (mbf->fp))
                        goto eof;
                      if (ps->signal_eilseq)
                        po_gram_error (ps, _("incomplete multibyte sequence at end of file"));
                      bytes = mbf->bufcount;
                      mbc->uc_valid = false;
                      break;
                    }
                  mbf->buf[mbf->bufcount++] = (unsigned char) c;
                  if (c == '\n')
                    {
                      if (ps->signal_eilseq)
                        po_gram_error (ps, _("incomplete multibyte sequence at end of line"));
                      bytes = mbf->bufcount - 1;
                      mbc->uc_valid = false;
                      break;
                    }
                }
              else
                {
                  int err = errno;
                  ps->catr->xeh->xerror (CAT_SEVERITY_FATAL_ERROR,
                                         NULL, NULL, 0, 0, false,
                                         xstrerror (_("iconv failure"), err));
                }
            }
          else
            {
              size_t outbytes = sizeof (scratchbuf) - outsize;
              bytes = mbf->bufcount - insize;

              /* We expect that one character has been produced.  */
              if (bytes == 0)
                abort ();
              if (outbytes == 0)
                abort ();
              /* Convert it from UTF-8 to UCS-4.  */
              if (u8_mbtoucr (&mbc->uc, scratchbuf, outbytes) < (int) outbytes)
                {
                  /* scratchbuf contains an out-of-range Unicode character
                     (> 0x10ffff).  */
                  if (ps->signal_eilseq)
                    po_gram_error (ps, _("invalid multibyte sequence"));
                  mbc->uc_valid = false;
                  break;
                }
              mbc->uc_valid = true;
              break;
            }
        }
    }
  else
#endif
    {
      if (ps->po_lex_weird_cjk
          /* Special handling of encodings with CJK structure.  */
          && (unsigned char) mbf->buf[0] >= 0x80)
        {
          if (mbf->bufcount == 1)
            {
              /* Read one more byte.  */
              int c = getc (mbf->fp);
              if (c == EOF)
                {
                  if (ferror (mbf->fp))
                    {
                      mbf->eof_seen = true;
                      goto eof;
                    }
                }
              else
                {
                  mbf->buf[1] = (unsigned char) c;
                  mbf->bufcount++;
                }
            }
          if (mbf->bufcount >= 2 && (unsigned char) mbf->buf[1] >= 0x30)
            /* Return a double byte.  */
            bytes = 2;
          else
            /* Return a single byte.  */
            bytes = 1;
        }
      else
        {
          /* Return a single byte.  */
          bytes = 1;
        }
#if HAVE_ICONV
      mbc->uc_valid = false;
#endif
    }

  /* Return the multibyte sequence mbf->buf[0..bytes-1].  */
  memcpy_small (&mbc->buf[0], &mbf->buf[0], bytes);
  mbc->bytes = bytes;

  mbf->bufcount -= bytes;
  if (mbf->bufcount > 0)
    {
      /* It's not worth calling memmove() for so few bytes.  */
      unsigned int count = mbf->bufcount;
      char *p = &mbf->buf[0];

      do
        {
          *p = *(p + bytes);
          p++;
        }
      while (--count > 0);
    }
  return;

eof:
  /* An mbchar_t with bytes == 0 is used to indicate EOF.  */
  mbc->bytes = 0;
#if HAVE_ICONV
  mbc->uc_valid = false;
#endif
  return;
}

static void
mbfile_ungetc (const mbchar_t mbc, mbfile_t mbf)
{
  if (mbf->pushback_count >= MBFILE_MAX_PUSHBACK)
    abort ();
  mb_copy (&mbf->pushback[mbf->pushback_count], mbc);
  mbf->pushback_count++;
}


/* Prepare lexical analysis.  */
void
lex_start (struct po_parser_state *ps,
           FILE *fp, const char *real_filename, const char *logical_filename,
           string_list_ty *arena)
{
  /* Ignore the logical_filename, because PO file entries already have
     their file names attached.  But use real_filename for error messages.  */
  {
    char *allocated_file_name = xstrdup (real_filename);
    string_list_append_move (arena, allocated_file_name);
    ps->gram_pos.file_name = allocated_file_name;
  }

  mbfile_init (ps->mbf, fp);

  ps->gram_pos.line_number = 1;
  ps->gram_pos_column = 0;
  ps->signal_eilseq = true;
  ps->po_lex_obsolete = false;
  ps->po_lex_previous = false;
  po_lex_charset_init (ps);
  ps->buf = NULL;
  ps->bufmax = 0;
}

/* Terminate lexical analysis.  */
void
lex_end (struct po_parser_state *ps)
{
  ps->gram_pos.file_name = NULL;
  ps->gram_pos.line_number = 0;
  po_lex_charset_close (ps);
  free (ps->buf);
}


/* Read a single character, collapsing the Windows CRLF line terminator
   to a single LF.
   Supports 1 character of pushback (via mbfile_ungetc).  */
static void
mbfile_getc_normalized (struct po_parser_state *ps, mbchar_t mbc, mbfile_t mbf)
{
  mbfile_getc (ps, mbc, ps->mbf);
  if (!mb_iseof (mbc) && mb_iseq (mbc, '\r'))
    {
      mbchar_t mbc2;
      mbfile_getc (ps, mbc2, ps->mbf);

      if (!mb_iseof (mbc2))
        {
          if (mb_iseq (mbc2, '\n'))
            /* Eliminate the CR.  */
            mb_copy (mbc, mbc2);
          else
            {
              mbfile_ungetc (mbc2, ps->mbf);
              /* If we get here, the caller can still do
                   mbfile_ungetc (mbc, ps->mbf);
                 since mbfile_getc supports 2 characters of pushback.  */
            }
        }
    }
}


/* Read a single character, dealing with backslash-newline.
   Also keep track of the current line number and column number.  */
static void
lex_getc (struct po_parser_state *ps, mbchar_t mbc)
{
  for (;;)
    {
      mbfile_getc_normalized (ps, mbc, ps->mbf);

      if (mb_iseof (mbc))
        {
          if (ferror (ps->mbf->fp))
           bomb:
            {
              int err = errno;
              ps->catr->xeh->xerror (CAT_SEVERITY_FATAL_ERROR,
                                     NULL, NULL, 0, 0, false,
                                     xstrerror (xasprintf (_("error while reading \"%s\""),
                                                           ps->gram_pos.file_name),
                                                err));
            }
          break;
        }

      if (mb_iseq (mbc, '\n'))
        {
          ps->gram_pos.line_number++;
          ps->gram_pos_column = 0;
          break;
        }

      ps->gram_pos_column += mb_width (ps, mbc);

      if (mb_iseq (mbc, '\\'))
        {
          mbchar_t mbc2;
          mbfile_getc_normalized (ps, mbc2, ps->mbf);

          if (mb_iseof (mbc2))
            {
              if (ferror (ps->mbf->fp))
                goto bomb;
              break;
            }

          if (!mb_iseq (mbc2, '\n'))
            {
              mbfile_ungetc (mbc2, ps->mbf);
              break;
            }

          ps->gram_pos.line_number++;
          ps->gram_pos_column = 0;
        }
      else
        break;
    }
}


static void
lex_ungetc (struct po_parser_state *ps, const mbchar_t mbc)
{
  if (!mb_iseof (mbc))
    {
      if (mb_iseq (mbc, '\n'))
        /* Decrement the line number, but don't care about the column.  */
        ps->gram_pos.line_number--;
      else
        /* Decrement the column number.  Also works well enough for tabs.  */
        ps->gram_pos_column -= mb_width (ps, mbc);

      mbfile_ungetc (mbc, ps->mbf);
    }
}


static int
keyword_p (struct po_parser_state *ps, const char *s)
{
  if (!ps->po_lex_previous)
    {
      if (!strcmp (s, "domain"))
        return DOMAIN;
      if (!strcmp (s, "msgid"))
        return MSGID;
      if (!strcmp (s, "msgid_plural"))
        return MSGID_PLURAL;
      if (!strcmp (s, "msgstr"))
        return MSGSTR;
      if (!strcmp (s, "msgctxt"))
        return MSGCTXT;
    }
  else
    {
      /* Inside a "#|" context, the keywords have a different meaning.  */
      if (!strcmp (s, "msgid"))
        return PREV_MSGID;
      if (!strcmp (s, "msgid_plural"))
        return PREV_MSGID_PLURAL;
      if (!strcmp (s, "msgctxt"))
        return PREV_MSGCTXT;
    }
  po_gram_error_at_line (ps->catr, &ps->gram_pos,
                         _("keyword \"%s\" unknown"), s);
  return NAME;
}


static int
control_sequence (struct po_parser_state *ps)
{
  mbchar_t mbc;
  lex_getc (ps, mbc);

  if (mb_len (mbc) == 1)
    switch (mb_ptr (mbc) [0])
      {
      case 'n':
        return '\n';

      case 't':
        return '\t';

      case 'b':
        return '\b';

      case 'r':
        return '\r';

      case 'f':
        return '\f';

      case 'v':
        return '\v';

      case 'a':
        return '\a';

      case '\\':
      case '"':
        return mb_ptr (mbc) [0];

      case '0': case '1': case '2': case '3':
      case '4': case '5': case '6': case '7':
        {
          int val = 0;
          int max = 0;
          for (;;)
            {
              char c = mb_ptr (mbc) [0];
              /* Warning: not portable, can't depend on '0'..'7' ordering.  */
              val = val * 8 + (c - '0');
              if (++max == 3)
                break;
              lex_getc (ps, mbc);
              if (mb_len (mbc) == 1)
                switch (mb_ptr (mbc) [0])
                  {
                  case '0': case '1': case '2': case '3':
                  case '4': case '5': case '6': case '7':
                    continue;

                  default:
                    break;
                  }
              lex_ungetc (ps, mbc);
              break;
            }
          return val;
        }

      case 'x':
        {
          lex_getc (ps, mbc);
          if (mb_iseof (mbc) || mb_len (mbc) != 1
              || !c_isxdigit (mb_ptr (mbc) [0]))
            break;

          int val = 0;
          for (;;)
            {
              char c = mb_ptr (mbc) [0];
              val *= 16;
              if (c_isdigit (c))
                /* Warning: not portable, can't depend on '0'..'9' ordering */
                val += c - '0';
              else if (c_isupper (c))
                /* Warning: not portable, can't depend on 'A'..'F' ordering */
                val += c - 'A' + 10;
              else
                /* Warning: not portable, can't depend on 'a'..'f' ordering */
                val += c - 'a' + 10;

              lex_getc (ps, mbc);
              if (mb_len (mbc) == 1)
                switch (mb_ptr (mbc) [0])
                  {
                  case '0': case '1': case '2': case '3': case '4':
                  case '5': case '6': case '7': case '8': case '9':
                  case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
                  case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
                    continue;

                  default:
                    break;
                  }
              lex_ungetc (ps, mbc);
              break;
            }
          return val;
        }

      /* FIXME: \u and \U are not handled.  */
      }
  lex_ungetc (ps, mbc);
  po_gram_error (ps, _("invalid control sequence"));
  return ' ';
}


/* Return the next token in the PO file.  The return codes are defined
   in "read-po-gram.h".  Associated data is put in 'po_gram_lval'.  */
int
po_gram_lex (union PO_GRAM_STYPE *lval, struct po_parser_state *ps)
{
  /* Cache ps->buf and ps->bufmax in local variables.  */
  char *buf = ps->buf;
  size_t bufmax = ps->bufmax;

  for (;;)
    {
      mbchar_t mbc;
      lex_getc (ps, mbc);

      if (mb_iseof (mbc))
        /* Yacc want this for end of file.  */
        return 0;

      if (mb_len (mbc) == 1)
        switch (mb_ptr (mbc) [0])
          {
          case '\n':
            ps->po_lex_obsolete = false;
            ps->po_lex_previous = false;
            /* Ignore whitespace, not relevant for the grammar.  */
            break;

          case ' ':
          case '\t':
          case '\r':
          case '\f':
          case '\v':
            /* Ignore whitespace, not relevant for the grammar.  */
            break;

          case '#':
            lex_getc (ps, mbc);
            if (mb_iseq (mbc, '~'))
              /* A pseudo-comment beginning with #~ is found.  This is
                 not a comment.  It is the format for obsolete entries.
                 We simply discard the "#~" prefix.  The following
                 characters are expected to be well formed.  */
              {
                ps->po_lex_obsolete = true;
                /* A pseudo-comment beginning with #~| denotes a previous
                   untranslated string in an obsolete entry.  This does not
                   make much sense semantically, and is implemented here
                   for completeness only.  */
                lex_getc (ps, mbc);
                if (mb_iseq (mbc, '|'))
                  ps->po_lex_previous = true;
                else
                  lex_ungetc (ps, mbc);
                break;
              }
            if (mb_iseq (mbc, '|'))
              /* A pseudo-comment beginning with #| is found.  This is
                 the previous untranslated string.  We discard the "#|"
                 prefix, but change the keywords and string returns
                 accordingly.  */
              {
                ps->po_lex_previous = true;
                break;
              }

            /* Accumulate comments into a buffer.  If we have been asked
               to pass comments, generate a COMMENT token, otherwise
               discard it.  */
            ps->signal_eilseq = false;
            if (ps->catr->pass_comments)
              {
                {
                  size_t bufpos = 0;
                  for (;;)
                    {
                      while (bufpos + mb_len (mbc) >= bufmax)
                        {
                          bufmax += 100;
                          buf = xrealloc (buf, bufmax);
                          ps->bufmax = bufmax;
                          ps->buf = buf;
                        }
                      if (mb_iseof (mbc) || mb_iseq (mbc, '\n'))
                        break;

                      memcpy_small (&buf[bufpos], mb_ptr (mbc), mb_len (mbc));
                      bufpos += mb_len (mbc);

                      lex_getc (ps, mbc);
                    }
                  buf[bufpos] = '\0';
                }

                lval->string.string = buf;
                lval->string.pos = ps->gram_pos;
                lval->string.obsolete = ps->po_lex_obsolete;
                ps->po_lex_obsolete = false;
                ps->signal_eilseq = true;
                return COMMENT;
              }
            else
              {
                /* We do this in separate loop because collecting large
                   comments while they get not passed to the upper layers
                   is not very efficient.  */
                while (!mb_iseof (mbc) && !mb_iseq (mbc, '\n'))
                  lex_getc (ps, mbc);
                ps->po_lex_obsolete = false;
                ps->signal_eilseq = true;
              }
            break;

          case '"':
            /* Accumulate a string.  */
            {
              size_t bufpos = 0;
              for (;;)
                {
                  lex_getc (ps, mbc);
                  while (bufpos + mb_len (mbc) >= bufmax)
                    {
                      bufmax += 100;
                      buf = xrealloc (buf, bufmax);
                      ps->bufmax = bufmax;
                      ps->buf = buf;
                    }
                  if (mb_iseof (mbc))
                    {
                      po_gram_error_at_line (ps->catr, &ps->gram_pos,
                                             _("end-of-file within string"));
                      break;
                    }
                  if (mb_iseq (mbc, '\n'))
                    {
                      po_gram_error_at_line (ps->catr, &ps->gram_pos,
                                             _("end-of-line within string"));
                      break;
                    }
                  if (mb_iseq (mbc, '"'))
                    break;
                  if (mb_iseq (mbc, '\\'))
                    buf[bufpos++] = control_sequence (ps);
                  else
                    {
                      /* Add mbc to the accumulator.  */
                      memcpy_small (&buf[bufpos], mb_ptr (mbc), mb_len (mbc));
                      bufpos += mb_len (mbc);
                    }
                }
              buf[bufpos] = '\0';
            }

            /* Strings cannot contain the msgctxt separator, because it cannot
               be faithfully represented in the msgid of a .mo file.  */
            if (strchr (buf, MSGCTXT_SEPARATOR) != NULL)
              po_gram_error_at_line (ps->catr, &ps->gram_pos,
                                     _("context separator <EOT> within string"));

            /* FIXME: Treatment of embedded \000 chars is incorrect.  */
            lval->string.string = xstrdup (buf);
            lval->string.pos = ps->gram_pos;
            lval->string.obsolete = ps->po_lex_obsolete;
            return (ps->po_lex_previous ? PREV_STRING : STRING);

          case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
          case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
          case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
          case 's': case 't': case 'u': case 'v': case 'w': case 'x':
          case 'y': case 'z':
          case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
          case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
          case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
          case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
          case 'Y': case 'Z':
          case '_': case '$':
            {
              size_t bufpos = 0;
              for (;;)
                {
                  char c = mb_ptr (mbc) [0];
                  if (bufpos + 1 >= bufmax)
                    {
                      bufmax += 100;
                      buf = xrealloc (buf, bufmax);
                      ps->bufmax = bufmax;
                      ps->buf = buf;
                    }
                  buf[bufpos++] = c;
                  lex_getc (ps, mbc);
                  if (mb_len (mbc) == 1)
                    switch (mb_ptr (mbc) [0])
                      {
                      default:
                        break;
                      case 'a': case 'b': case 'c': case 'd': case 'e':
                      case 'f': case 'g': case 'h': case 'i': case 'j':
                      case 'k': case 'l': case 'm': case 'n': case 'o':
                      case 'p': case 'q': case 'r': case 's': case 't':
                      case 'u': case 'v': case 'w': case 'x': case 'y':
                      case 'z':
                      case 'A': case 'B': case 'C': case 'D': case 'E':
                      case 'F': case 'G': case 'H': case 'I': case 'J':
                      case 'K': case 'L': case 'M': case 'N': case 'O':
                      case 'P': case 'Q': case 'R': case 'S': case 'T':
                      case 'U': case 'V': case 'W': case 'X': case 'Y':
                      case 'Z':
                      case '_': case '$':
                      case '0': case '1': case '2': case '3': case '4':
                      case '5': case '6': case '7': case '8': case '9':
                        continue;
                      }
                  break;
                }
              lex_ungetc (ps, mbc);

              buf[bufpos] = '\0';
            }
            {
              int k = keyword_p (ps, buf);
              if (k == NAME)
                {
                  lval->string.string = xstrdup (buf);
                  lval->string.pos = ps->gram_pos;
                  lval->string.obsolete = ps->po_lex_obsolete;
                }
              else
                {
                  lval->pos.pos = ps->gram_pos;
                  lval->pos.obsolete = ps->po_lex_obsolete;
                }
              return k;
            }

          case '0': case '1': case '2': case '3': case '4':
          case '5': case '6': case '7': case '8': case '9':
            {
              size_t bufpos = 0;
              for (;;)
                {
                  char c = mb_ptr (mbc) [0];
                  if (bufpos + 1 >= bufmax)
                    {
                      bufmax += 100;
                      buf = xrealloc (buf, bufmax + 1);
                      ps->bufmax = bufmax;
                      ps->buf = buf;
                    }
                  buf[bufpos++] = c;
                  lex_getc (ps, mbc);
                  if (mb_len (mbc) == 1)
                    switch (mb_ptr (mbc) [0])
                      {
                      default:
                        break;

                      case '0': case '1': case '2': case '3': case '4':
                      case '5': case '6': case '7': case '8': case '9':
                        continue;
                      }
                  break;
                }
              lex_ungetc (ps, mbc);

              buf[bufpos] = '\0';
            }

            lval->number.number = atol (buf);
            lval->number.pos = ps->gram_pos;
            lval->number.obsolete = ps->po_lex_obsolete;
            return NUMBER;

          case '[':
            lval->pos.pos = ps->gram_pos;
            lval->pos.obsolete = ps->po_lex_obsolete;
            return '[';

          case ']':
            lval->pos.pos = ps->gram_pos;
            lval->pos.obsolete = ps->po_lex_obsolete;
            return ']';

          default:
            /* This will cause a syntax error.  */
            return JUNK;
          }
      else
        /* This will cause a syntax error.  */
        return JUNK;
    }
}
