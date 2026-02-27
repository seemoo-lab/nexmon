/* Output stream that produces HTML output.
   Copyright (C) 2006-2009, 2015-2016 Free Software Foundation, Inc.
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
#include "html-ostream.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gl_xlist.h"
#include "gl_array_list.h"
#include "unistr.h"
#include "xalloc.h"

struct html_ostream : struct ostream
{
fields:
  /* The destination stream.  */
  ostream_t destination;
  /* The stack of active CSS classes.  */
  gl_list_t /* <char *> */ class_stack;
  /* Current and last size of the active portion of this stack.  Always
     size(class_stack) == max(curr_class_stack_size,last_class_stack_size).  */
  size_t curr_class_stack_size;
  size_t last_class_stack_size;
  /* Last few bytes that could not yet be converted.  */
  #define BUFSIZE 6
  char buf[BUFSIZE];
  size_t buflen;
};

/* Implementation of ostream_t methods.  */

static void
emit_pending_spans (html_ostream_t stream, bool shrink_stack)
{
  if (stream->curr_class_stack_size > stream->last_class_stack_size)
    {
      size_t i;

      for (i = stream->last_class_stack_size; i < stream->curr_class_stack_size; i++)
        {
          char *classname = (char *) gl_list_get_at (stream->class_stack, i);

          ostream_write_str (stream->destination, "<span class=\"");
          ostream_write_str (stream->destination, classname);
          ostream_write_str (stream->destination, "\">");
        }
      stream->last_class_stack_size = stream->curr_class_stack_size;
    }
  else if (stream->curr_class_stack_size < stream->last_class_stack_size)
    {
      size_t i = stream->last_class_stack_size;

      while (i > stream->curr_class_stack_size)
        {
          char *classname;

          --i;
          classname = (char *) gl_list_get_at (stream->class_stack, i);
          ostream_write_str (stream->destination, "</span>");
          if (shrink_stack)
            {
              gl_list_remove_at (stream->class_stack, i);
              free (classname);
            }
        }
      stream->last_class_stack_size = stream->curr_class_stack_size;
    }
}

static void
html_ostream::write_mem (html_ostream_t stream, const void *data, size_t len)
{
  if (len > 0)
    {
      #define BUFFERSIZE 2048
      char inbuffer[BUFFERSIZE];
      size_t inbufcount;

      inbufcount = stream->buflen;
      if (inbufcount > 0)
        memcpy (inbuffer, stream->buf, inbufcount);
      for (;;)
        {
          /* At this point, inbuffer[0..inbufcount-1] is filled.  */
          {
            /* Combine the previous rest with a chunk of new input.  */
            size_t n =
              (len <= BUFFERSIZE - inbufcount ? len : BUFFERSIZE - inbufcount);

            if (n > 0)
              {
                memcpy (inbuffer + inbufcount, data, n);
                data = (char *) data + n;
                inbufcount += n;
                len -= n;
              }
          }
          {
            /* Handle complete UTF-8 characters.  */
            const char *inptr = inbuffer;
            size_t insize = inbufcount;

            while (insize > 0)
              {
                unsigned char c0;
                ucs4_t uc;
                int nbytes;

                c0 = ((const unsigned char *) inptr)[0];
                if (insize < (c0 < 0xc0 ? 1 : c0 < 0xe0 ? 2 : c0 < 0xf0 ? 3 :
                              c0 < 0xf8 ? 4 : c0 < 0xfc ? 5 : 6))
                  break;

                nbytes = u8_mbtouc (&uc, (const unsigned char *) inptr, insize);

                if (uc == '\n')
                  {
                    size_t prev_class_stack_size = stream->curr_class_stack_size;
                    stream->curr_class_stack_size = 0;
                    emit_pending_spans (stream, false);
                    ostream_write_str (stream->destination, "<br/>");
                    stream->curr_class_stack_size = prev_class_stack_size;
                  }
                else
                  {
                    emit_pending_spans (stream, true);

                    switch (uc)
                      {
                      case '"':
                        ostream_write_str (stream->destination, "&quot;");
                        break;
                      case '&':
                        ostream_write_str (stream->destination, "&amp;");
                        break;
                      case '<':
                        ostream_write_str (stream->destination, "&lt;");
                        break;
                      case '>':
                        /* Needed to avoid "]]>" in the output.  */
                        ostream_write_str (stream->destination, "&gt;");
                        break;
                      case ' ':
                        /* Needed because HTML viewers merge adjacent spaces
                           and drop spaces adjacent to <br> and similar.  */
                        ostream_write_str (stream->destination, "&nbsp;");
                        break;
                      default:
                        if (uc >= 0x20 && uc < 0x7F)
                          {
                            /* Output ASCII characters as such.  */
                            char bytes[1];
                            bytes[0] = uc;
                            ostream_write_mem (stream->destination, bytes, 1);
                          }
                        else
                          {
                            /* Output non-ASCII characters in #&nnn;
                               notation.  */
                            char bytes[32];
                            sprintf (bytes, "&#%d;", (int) uc);
                            ostream_write_str (stream->destination, bytes);
                          }
                        break;
                      }
                  }

                inptr += nbytes;
                insize -= nbytes;
              }
            /* Put back the unconverted part.  */
            if (insize > BUFSIZE)
              abort ();
            if (len == 0)
              {
                if (insize > 0)
                  memcpy (stream->buf, inptr, insize);
                stream->buflen = insize;
                break;
              }
            if (insize > 0)
              memmove (inbuffer, inptr, insize);
            inbufcount = insize;
          }
        }
      #undef BUFFERSIZE
    }
}

static void
html_ostream::flush (html_ostream_t stream)
{
  /* There's nothing to do here, since stream->buf[] contains only a few
     bytes that don't correspond to a character, and it's not worth closing
     the open spans.  */
}

static void
html_ostream::free (html_ostream_t stream)
{
  stream->curr_class_stack_size = 0;
  emit_pending_spans (stream, true);
  gl_list_free (stream->class_stack);
  free (stream);
}

/* Implementation of html_ostream_t methods.  */

static void
html_ostream::begin_span (html_ostream_t stream, const char *classname)
{
  if (stream->last_class_stack_size > stream->curr_class_stack_size
      && strcmp ((char *) gl_list_get_at (stream->class_stack,
                                          stream->curr_class_stack_size),
                 classname) != 0)
    emit_pending_spans (stream, true);
  /* Now either
       last_class_stack_size <= curr_class_stack_size
       - in this case we have to append the given CLASSNAME -
     or
       last_class_stack_size > curr_class_stack_size
       && class_stack[curr_class_stack_size] == CLASSNAME
       - in this case we only need to increment curr_class_stack_size.  */
  if (stream->last_class_stack_size <= stream->curr_class_stack_size)
    gl_list_add_at (stream->class_stack, stream->curr_class_stack_size,
                    xstrdup (classname));
  stream->curr_class_stack_size++;
}

static void
html_ostream::end_span (html_ostream_t stream, const char *classname)
{
  if (!(stream->curr_class_stack_size > 0
        && strcmp ((char *) gl_list_get_at (stream->class_stack,
                                            stream->curr_class_stack_size - 1),
                   classname) == 0))
    /* Improperly nested begin_span/end_span calls.  */
    abort ();
  stream->curr_class_stack_size--;
}

/* Constructor.  */

html_ostream_t
html_ostream_create (ostream_t destination)
{
  html_ostream_t stream = XMALLOC (struct html_ostream_representation);

  stream->base.vtable = &html_ostream_vtable;
  stream->destination = destination;
  stream->class_stack =
    gl_list_create_empty (GL_ARRAY_LIST, NULL, NULL, NULL, true);
  stream->curr_class_stack_size = 0;
  stream->last_class_stack_size = 0;
  stream->buflen = 0;

  return stream;
}
