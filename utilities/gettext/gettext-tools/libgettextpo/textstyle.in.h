/* Dummy replacement for part of the public API of the libtextstyle library.
   Copyright (C) 2006-2007, 2019-2026 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2019.  */

/* This file is used as replacement when libtextstyle with its include file
   <textstyle.h> is not found.
   It supports the essential API and implements it in a way that does not
   provide text styling.  That is, it produces plain text output via <stdio.h>
   FILE objects.
   Thus, it allows a package to be build with or without a dependency to
   libtextstyle, with very few occurrences of '#if HAVE_LIBTEXTSTYLE'.

   Restriction:
   It assumes that freopen() is not being called on stdout and stderr.  */

#ifndef _TEXTSTYLE_H
#define _TEXTSTYLE_H

/* This file uses _GL_UNNAMED, HAVE_TCDRAIN.  */
#if !_GL_CONFIG_H_INCLUDED
 #error "Please include config.h first."
#endif

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#if HAVE_TCDRAIN
# include <termios.h>
#endif

/* An __attribute__ __format__ specifier for a function that takes a format
   string and arguments, where the format string directives are the ones
   standardized by ISO C99 and POSIX.
   _GL_ATTRIBUTE_SPEC_PRINTF_STANDARD  */
/* __gnu_printf__ is supported in GCC >= 4.4.  */
#ifndef _GL_ATTRIBUTE_SPEC_PRINTF_STANDARD
# if (__GNUC__ + (__GNUC_MINOR__ >= 4) > 4) && !defined __clang__
#  define _GL_ATTRIBUTE_SPEC_PRINTF_STANDARD __gnu_printf__
# else
#  define _GL_ATTRIBUTE_SPEC_PRINTF_STANDARD __printf__
# endif
#endif

/* _GL_UNNAMED (ID) is the "name" of an unnamed function parameter.  */
#ifndef _GL_UNNAMED
# if ((defined __STDC_VERSION__ ? __STDC_VERSION__ : 0) < 202311 \
      && !defined __cplusplus)
#  define _GL_UNNAMED(id) unnamed_##id _GL_ATTRIBUTE_UNUSED
# else
#  define _GL_UNNAMED(id)
# endif
#endif

/* ----------------------------- From ostream.h ----------------------------- */

/* Describes the scope of a flush operation.  */
typedef enum
{
  /* Flushes buffers in this ostream_t.
     Use this value if you want to write to the underlying ostream_t.  */
  FLUSH_THIS_STREAM = 0,
  /* Flushes all buffers in the current process.
     Use this value if you want to write to the same target through a
     different file descriptor or a FILE stream.  */
  FLUSH_THIS_PROCESS = 1,
  /* Flushes buffers in the current process and attempts to flush the buffers
     in the kernel.
     Use this value so that some other process (or the kernel itself)
     may write to the same target.  */
  FLUSH_ALL = 2
} ostream_flush_scope_t;


/* An output stream is an object to which one can feed a sequence of bytes.  */

typedef FILE * ostream_t;

static inline void
ostream_write_mem (ostream_t stream, const void *data, size_t len)
{
  if (len > 0)
    fwrite (data, 1, len, stream);
}

static inline void
ostream_flush (ostream_t stream, ostream_flush_scope_t scope)
{
  fflush (stream);
  if (scope == FLUSH_ALL)
    {
      int fd = fileno (stream);
      if (fd >= 0)
        {
          /* For streams connected to a disk file:  */
          fsync (fd);
          #if HAVE_TCDRAIN
          /* For streams connected to a terminal:  */
          {
            int retval;

            do
              retval = tcdrain (fd);
            while (retval < 0 && errno == EINTR);
          }
          #endif
        }
    }
}

static inline void
ostream_free (ostream_t stream)
{
  if (stream == stdin || stream == stderr)
    fflush (stream);
  else
    fclose (stream);
}

static inline void
ostream_write_str (ostream_t stream, const char *string)
{
  ostream_write_mem (stream, string, strlen (string));
}

static inline ptrdiff_t ostream_printf (ostream_t stream,
                                        const char *format, ...)
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1) || defined __clang__
  __attribute__ ((__format__ (_GL_ATTRIBUTE_SPEC_PRINTF_STANDARD, 2, 3)))
#endif
  ;
static inline ptrdiff_t
ostream_printf (ostream_t stream, const char *format, ...)
{
  va_list args;

  va_start (args, format);
  char *temp_string;
  ptrdiff_t ret = vasprintf (&temp_string, format, args);
  va_end (args);
  if (ret >= 0)
    {
      if (ret > 0)
        ostream_write_str (stream, temp_string);
      free (temp_string);
    }
  return ret;
}

static inline ptrdiff_t ostream_vprintf (ostream_t stream,
                                         const char *format, va_list args)
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1) || defined __clang__
  __attribute__ ((__format__ (_GL_ATTRIBUTE_SPEC_PRINTF_STANDARD, 2, 0)))
#endif
  ;
static inline ptrdiff_t
ostream_vprintf (ostream_t stream, const char *format, va_list args)
{
  char *temp_string;
  ptrdiff_t ret = vasprintf (&temp_string, format, args);
  if (ret >= 0)
    {
      if (ret > 0)
        ostream_write_str (stream, temp_string);
      free (temp_string);
    }
  return ret;
}

/* ------------------------- From styled-ostream.h ------------------------- */

typedef ostream_t styled_ostream_t;

#define styled_ostream_write_mem ostream_write_mem
#define styled_ostream_flush ostream_flush
#define styled_ostream_free ostream_free

static inline void
styled_ostream_begin_use_class (styled_ostream_t _GL_UNNAMED (stream),
                                const char *_GL_UNNAMED (classname))
{
}

static inline void
styled_ostream_end_use_class (styled_ostream_t _GL_UNNAMED (stream),
                              const char *_GL_UNNAMED (classname))
{
}

static inline const char *
styled_ostream_get_hyperlink_ref (styled_ostream_t _GL_UNNAMED (stream))
{
  return NULL;
}

static inline const char *
styled_ostream_get_hyperlink_id (styled_ostream_t _GL_UNNAMED (stream))
{
  return NULL;
}

static inline void
styled_ostream_set_hyperlink (styled_ostream_t _GL_UNNAMED (stream),
                              const char *_GL_UNNAMED (ref),
                              const char *_GL_UNNAMED (id))
{
}

static inline void
styled_ostream_flush_to_current_style (styled_ostream_t _GL_UNNAMED (stream))
{
}

static inline bool
is_instance_of_styled_ostream (ostream_t _GL_UNNAMED (stream))
{
  return false;
}

/* -------------------------- From file-ostream.h -------------------------- */

typedef ostream_t file_ostream_t;

#define file_ostream_write_mem ostream_write_mem
#define file_ostream_flush ostream_flush
#define file_ostream_free ostream_free

static inline FILE *
file_ostream_get_stdio_stream (file_ostream_t stream)
{
  return stream;
}

static inline file_ostream_t
file_ostream_create (FILE *fp)
{
  return fp;
}

static inline bool
is_instance_of_file_ostream (ostream_t _GL_UNNAMED (stream))
{
  return true;
}

/* --------------------------- From fd-ostream.h --------------------------- */

typedef ostream_t fd_ostream_t;

#define fd_ostream_write_mem ostream_write_mem
#define fd_ostream_flush ostream_flush
#define fd_ostream_free ostream_free

static inline int
fd_ostream_get_descriptor (fd_ostream_t stream)
{
  return fileno (stream);
}

static inline const char *
fd_ostream_get_filename (fd_ostream_t _GL_UNNAMED (stream))
{
  return NULL;
}

static inline bool
fd_ostream_is_buffered (fd_ostream_t _GL_UNNAMED (stream))
{
  return false;
}

static inline fd_ostream_t
fd_ostream_create (int fd, const char *_GL_UNNAMED (filename),
                   bool _GL_UNNAMED (buffered))
{
  if (fd == 1)
    return stdout;
  else if (fd == 2)
    return stderr;
  else
    return fdopen (fd, "w");
}

static inline bool
is_instance_of_fd_ostream (ostream_t stream)
{
  return fileno (stream) >= 0;
}

/* -------------------------- From term-ostream.h -------------------------- */

typedef int term_color_t;
enum
{
  COLOR_DEFAULT = -1  /* unknown */
};

typedef enum
{
  WEIGHT_NORMAL = 0,
  WEIGHT_BOLD,
  WEIGHT_DEFAULT = WEIGHT_NORMAL
} term_weight_t;

typedef enum
{
  POSTURE_NORMAL = 0,
  POSTURE_ITALIC, /* same as oblique */
  POSTURE_DEFAULT = POSTURE_NORMAL
} term_posture_t;

typedef enum
{
  UNDERLINE_OFF = 0,
  UNDERLINE_ON,
  UNDERLINE_DEFAULT = UNDERLINE_OFF
} term_underline_t;

typedef enum
{
  TTYCTL_AUTO = 0,  /* Automatic best-possible choice.  */
  TTYCTL_NONE,      /* No control.
                       Result: Garbled output can occur, and the terminal can
                       be left in any state when the program is interrupted.  */
  TTYCTL_PARTIAL,   /* Signal handling.
                       Result: Garbled output can occur, but the terminal will
                       be left in the default state when the program is
                       interrupted.  */
  TTYCTL_FULL       /* Signal handling and disabling echo and flush-upon-signal.
                       Result: No garbled output, and the terminal will
                       be left in the default state when the program is
                       interrupted.  */
} ttyctl_t;

typedef ostream_t term_ostream_t;

#define term_ostream_write_mem ostream_write_mem
#define term_ostream_flush ostream_flush
#define term_ostream_free ostream_free

static inline term_color_t
term_ostream_rgb_to_color (term_ostream_t _GL_UNNAMED (stream),
                           int _GL_UNNAMED (red),
                           int _GL_UNNAMED (green),
                           int _GL_UNNAMED (blue))
{
  return COLOR_DEFAULT;
}

static inline term_color_t
term_ostream_get_color (term_ostream_t _GL_UNNAMED (stream))
{
  return COLOR_DEFAULT;
}

static inline void
term_ostream_set_color (term_ostream_t _GL_UNNAMED (stream),
                        term_color_t _GL_UNNAMED (color))
{
}

static inline term_color_t
term_ostream_get_bgcolor (term_ostream_t _GL_UNNAMED (stream))
{
  return COLOR_DEFAULT;
}

static inline void
term_ostream_set_bgcolor (term_ostream_t _GL_UNNAMED (stream),
                          term_color_t _GL_UNNAMED (color))
{
}

static inline term_weight_t
term_ostream_get_weight (term_ostream_t _GL_UNNAMED (stream))
{
  return WEIGHT_DEFAULT;
}

static inline void
term_ostream_set_weight (term_ostream_t _GL_UNNAMED (stream),
                         term_weight_t _GL_UNNAMED (weight))
{
}

static inline term_posture_t
term_ostream_get_posture (term_ostream_t _GL_UNNAMED (stream))
{
  return POSTURE_DEFAULT;
}

static inline void
term_ostream_set_posture (term_ostream_t _GL_UNNAMED (stream),
                          term_posture_t _GL_UNNAMED (posture))
{
}

static inline term_underline_t
term_ostream_get_underline (term_ostream_t _GL_UNNAMED (stream))
{
  return UNDERLINE_DEFAULT;
}

static inline void
term_ostream_set_underline (term_ostream_t _GL_UNNAMED (stream),
                            term_underline_t _GL_UNNAMED (underline))
{
}

static inline const char *
term_ostream_get_hyperlink_ref (term_ostream_t _GL_UNNAMED (stream))
{
  return NULL;
}

static inline const char *
term_ostream_get_hyperlink_id (term_ostream_t _GL_UNNAMED (stream))
{
  return NULL;
}

static inline void
term_ostream_set_hyperlink (term_ostream_t _GL_UNNAMED (stream),
                            const char *_GL_UNNAMED (ref),
                            const char *_GL_UNNAMED (id))
{
}

static inline void
term_ostream_flush_to_current_style (term_ostream_t stream)
{
  fflush (stream);
}

#define term_ostream_get_descriptor fd_ostream_get_descriptor
#define term_ostream_get_filename fd_ostream_get_filename

static inline ttyctl_t
term_ostream_get_tty_control (term_ostream_t _GL_UNNAMED (stream))
{
  return TTYCTL_NONE;
}

static inline ttyctl_t
term_ostream_get_effective_tty_control (term_ostream_t _GL_UNNAMED (stream))
{
  return TTYCTL_NONE;
}

static inline term_ostream_t
term_ostream_create (int fd, const char *filename,
                     ttyctl_t _GL_UNNAMED (tty_control))
{
  return fd_ostream_create (fd, filename, true);
}

#define is_instance_of_term_ostream is_instance_of_fd_ostream

/* ------------------------- From memory-ostream.h ------------------------- */

typedef ostream_t memory_ostream_t;

#define memory_ostream_write_mem ostream_write_mem
#define memory_ostream_flush ostream_flush
#define memory_ostream_free ostream_free

static inline void
memory_ostream_contents (memory_ostream_t _GL_UNNAMED (stream),
                         const void **bufp, size_t *buflenp)
{
  *bufp = NULL;
  *buflenp = 0;
}

static inline memory_ostream_t
memory_ostream_create (void)
{
  /* Not supported without the real libtextstyle.  */
  abort ();
  return NULL;
}

static inline bool
is_instance_of_memory_ostream (ostream_t _GL_UNNAMED (stream))
{
  return false;
}

/* -------------------------- From html-ostream.h -------------------------- */

typedef ostream_t html_ostream_t;

#define html_ostream_write_mem ostream_write_mem
#define html_ostream_flush ostream_flush
#define html_ostream_free ostream_free

static inline void
html_ostream_begin_span (html_ostream_t _GL_UNNAMED (stream),
                         const char *_GL_UNNAMED (classname))
{
}

static inline void
html_ostream_end_span (html_ostream_t _GL_UNNAMED (stream),
                       const char *_GL_UNNAMED (classname))
{
}

static inline const char *
html_ostream_get_hyperlink_ref (html_ostream_t _GL_UNNAMED (stream))
{
  return NULL;
}

static inline void
html_ostream_set_hyperlink_ref (html_ostream_t _GL_UNNAMED (stream),
                                const char *_GL_UNNAMED (ref))
{
}

static inline void
html_ostream_flush_to_current_style (html_ostream_t _GL_UNNAMED (stream))
{
}

static inline ostream_t
html_ostream_get_destination (html_ostream_t _GL_UNNAMED (stream))
{
  return NULL;
}

static inline html_ostream_t
html_ostream_create (ostream_t _GL_UNNAMED (destination))
{
  /* Not supported without the real libtextstyle.  */
  abort ();
  return NULL;
}

static inline bool
is_instance_of_html_ostream (ostream_t _GL_UNNAMED (stream))
{
  return false;
}

/* ----------------------- From term-styled-ostream.h ----------------------- */

typedef styled_ostream_t term_styled_ostream_t;

#define term_styled_ostream_write_mem ostream_write_mem
#define term_styled_ostream_flush ostream_flush
#define term_styled_ostream_free ostream_free
#define term_styled_ostream_begin_use_class styled_ostream_begin_use_class
#define term_styled_ostream_end_use_class styled_ostream_end_use_class
#define term_styled_ostream_get_hyperlink_ref styled_ostream_get_hyperlink_ref
#define term_styled_ostream_get_hyperlink_id styled_ostream_get_hyperlink_id
#define term_styled_ostream_set_hyperlink styled_ostream_set_hyperlink
#define term_styled_ostream_flush_to_current_style styled_ostream_flush_to_current_style

static inline term_ostream_t
term_styled_ostream_get_destination (term_styled_ostream_t stream)
{
  return stream;
}

static inline const char *
term_styled_ostream_get_css_filename (term_styled_ostream_t _GL_UNNAMED (stream))
{
  return NULL;
}

static inline term_styled_ostream_t
term_styled_ostream_create (int fd, const char *filename,
                            ttyctl_t _GL_UNNAMED (tty_control),
                            const char *_GL_UNNAMED (css_filename))
{
  return fd_ostream_create (fd, filename, true);
}

#define is_instance_of_term_styled_ostream is_instance_of_term_ostream

/* ----------------------- From html-styled-ostream.h ----------------------- */

typedef styled_ostream_t html_styled_ostream_t;

#define html_styled_ostream_write_mem ostream_write_mem
#define html_styled_ostream_flush ostream_flush
#define html_styled_ostream_free ostream_free
#define html_styled_ostream_begin_use_class styled_ostream_begin_use_class
#define html_styled_ostream_end_use_class styled_ostream_end_use_class
#define html_styled_ostream_get_hyperlink_ref styled_ostream_get_hyperlink_ref
#define html_styled_ostream_get_hyperlink_id styled_ostream_get_hyperlink_id
#define html_styled_ostream_set_hyperlink styled_ostream_set_hyperlink
#define html_styled_ostream_flush_to_current_style styled_ostream_flush_to_current_style

static inline ostream_t
html_styled_ostream_get_destination (html_styled_ostream_t _GL_UNNAMED (stream))
{
  return NULL;
}

static inline html_ostream_t
html_styled_ostream_get_html_destination (html_styled_ostream_t _GL_UNNAMED (stream))
{
  return NULL;
}

static inline const char *
html_styled_ostream_get_css_filename (html_styled_ostream_t _GL_UNNAMED (stream))
{
  return NULL;
}

static inline html_styled_ostream_t
html_styled_ostream_create (ostream_t _GL_UNNAMED (destination),
                            const char *_GL_UNNAMED (css_filename))
{
  /* Not supported without the real libtextstyle.  */
  abort ();
  return NULL;
}

static inline bool
is_instance_of_html_styled_ostream (ostream_t _GL_UNNAMED (stream))
{
  return false;
}

/* ----------------------- From noop-styled-ostream.h ----------------------- */

typedef styled_ostream_t noop_styled_ostream_t;

#define noop_styled_ostream_write_mem ostream_write_mem
#define noop_styled_ostream_flush ostream_flush
#define noop_styled_ostream_free ostream_free
#define noop_styled_ostream_begin_use_class styled_ostream_begin_use_class
#define noop_styled_ostream_end_use_class styled_ostream_end_use_class
#define noop_styled_ostream_get_hyperlink_ref styled_ostream_get_hyperlink_ref
#define noop_styled_ostream_get_hyperlink_id styled_ostream_get_hyperlink_id
#define noop_styled_ostream_set_hyperlink styled_ostream_set_hyperlink
#define noop_styled_ostream_flush_to_current_style styled_ostream_flush_to_current_style

static inline ostream_t
noop_styled_ostream_get_destination (noop_styled_ostream_t stream)
{
  return stream;
}

static inline bool
noop_styled_ostream_is_owning_destination (noop_styled_ostream_t _GL_UNNAMED (stream))
{
  return true;
}

static inline noop_styled_ostream_t
noop_styled_ostream_create (ostream_t destination, bool pass_ownership)
{
  if (!pass_ownership)
    /* Not supported without the real libtextstyle.  */
    abort ();
  return destination;
}

static inline bool
is_instance_of_noop_styled_ostream (ostream_t _GL_UNNAMED (stream))
{
  return false;
}

/* ------------------------------ From color.h ------------------------------ */

#define color_test_mode false

enum color_option { color_no, color_tty, color_yes, color_html };
#define color_mode color_no

#define style_file_name NULL

static inline bool
handle_color_option (const char *_GL_UNNAMED (option))
{
  return false;
}

static inline void
handle_style_option (const char *_GL_UNNAMED (option))
{
}

static inline void
print_color_test (void)
{
  /* Not supported without the real libtextstyle.  */
  abort ();
}

static inline void
style_file_prepare (const char *_GL_UNNAMED (style_file_envvar),
                    const char *_GL_UNNAMED (stylesdir_envvar),
                    const char *_GL_UNNAMED (stylesdir_after_install),
                    const char *_GL_UNNAMED (default_style_file))
{
}

/* ------------------------------ From misc.h ------------------------------ */

static inline styled_ostream_t
styled_ostream_create (int fd, const char *filename,
                       ttyctl_t _GL_UNNAMED (tty_control),
                       const char *_GL_UNNAMED (css_filename))
{
  return fd_ostream_create (fd, filename, true);
}

static inline void
libtextstyle_set_failure_exit_code (int _GL_UNNAMED (exit_code))
{
}

#endif /* _TEXTSTYLE_H */
