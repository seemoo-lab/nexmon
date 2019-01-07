/* Output stream for CSS styled text, producing HTML output.
   Copyright (C) 2006-2007, 2015-2016 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2006.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <config.h>

/* Specification.  */
#include "html-styled-ostream.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "html-ostream.h"

#include "binary-io.h"
#ifndef O_TEXT
# define O_TEXT 0
#endif

#include "error.h"
#include "safe-read.h"
#include "xalloc.h"
#include "gettext.h"

#define _(str) gettext (str)


struct html_styled_ostream : struct styled_ostream
{
fields:
  /* The destination stream.  */
  ostream_t destination;
  /* A HTML aware wrapper around the destination stream.  */
  html_ostream_t html_destination;
};

/* Implementation of ostream_t methods.  */

static void
html_styled_ostream::write_mem (html_styled_ostream_t stream,
                                const void *data, size_t len)
{
  html_ostream_write_mem (stream->html_destination, data, len);
}

static void
html_styled_ostream::flush (html_styled_ostream_t stream)
{
  html_ostream_flush (stream->html_destination);
}

static void
html_styled_ostream::free (html_styled_ostream_t stream)
{
  html_ostream_free (stream->html_destination);
  ostream_write_str (stream->destination, "</body>\n");
  ostream_write_str (stream->destination, "</html>\n");
}

/* Implementation of styled_ostream_t methods.  */

static void
html_styled_ostream::begin_use_class (html_styled_ostream_t stream,
                                      const char *classname)
{
  html_ostream_begin_span (stream->html_destination, classname);
}

static void
html_styled_ostream::end_use_class (html_styled_ostream_t stream,
                                    const char *classname)
{
  html_ostream_end_span (stream->html_destination, classname);
}

/* Constructor.  */

html_styled_ostream_t
html_styled_ostream_create (ostream_t destination, const char *css_filename)
{
  html_styled_ostream_t stream =
    XMALLOC (struct html_styled_ostream_representation);

  stream->base.base.vtable = &html_styled_ostream_vtable;
  stream->destination = destination;
  stream->html_destination = html_ostream_create (destination);

  ostream_write_str (stream->destination, "<?xml version=\"1.0\"?>\n");
  /* HTML 4.01 or XHTML 1.0?
     Use HTML 4.01.  This is conservative.  Before switching to XHTML 1.0,
     verify that in the output
       - all HTML element names are in lowercase,
       - all empty elements are denoted like <br/> or <p></p>,
       - every attribute specification is in assignment form, like
         <table border="1">,
       - every <a name="..."> element also has an 'id' attribute,
       - special characters like < > & " are escaped in the <style> and
         <script> elements.  */
  ostream_write_str (stream->destination,
                     "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\n");
  ostream_write_str (stream->destination, "<html>\n");
  ostream_write_str (stream->destination, "<head>\n");
  if (css_filename != NULL)
    {
      ostream_write_str (stream->destination, "<style type=\"text/css\">\n"
                                              "<!--\n");

      /* Include the contents of CSS_FILENAME literally.  */
      {
        int fd;
        char buf[4096];

        fd = open (css_filename, O_RDONLY | O_TEXT);
        if (fd < 0)
          error (EXIT_FAILURE, errno,
                 _("error while opening \"%s\" for reading"),
                 css_filename);

        for (;;)
          {
            size_t n_read = safe_read (fd, buf, sizeof (buf));
            if (n_read == SAFE_READ_ERROR)
              error (EXIT_FAILURE, errno, _("error reading \"%s\""),
                     css_filename);
            if (n_read == 0)
              break;

            ostream_write_mem (stream->destination, buf, n_read);
          }

        if (close (fd) < 0)
          error (EXIT_FAILURE, errno, _("error after reading \"%s\""),
                 css_filename);
      }

      ostream_write_str (stream->destination, "-->\n"
                                              "</style>\n");
    }
  ostream_write_str (stream->destination, "</head>\n");
  ostream_write_str (stream->destination, "<body>\n");

  return stream;
}
