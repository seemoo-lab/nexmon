/* Abstract output stream for CSS styled text.
   Copyright (C) 2006, 2015-2016 Free Software Foundation, Inc.
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

#ifndef _STYLED_OSTREAM_H
#define _STYLED_OSTREAM_H

#include "ostream.h"


/* A styled output stream is an object to which one can feed a sequence of
   bytes, marking some runs of text as belonging to specific CSS classes,
   where the rendering of the CSS classes is defined through a CSS (cascading
   style sheet).  */

struct styled_ostream : struct ostream
{
methods:

  /* Start a run of text belonging to CLASSNAME.  The CLASSNAME is the name
     of a CSS class.  It can be chosen arbitrarily and customized through
     an inline or external CSS.  */
  void begin_use_class (styled_ostream_t stream, const char *classname);

  /* End a run of text belonging to CLASSNAME.
     The begin_use_class / end_use_class calls must match properly.  */
  void end_use_class (styled_ostream_t stream, const char *classname);
};


#endif /* _STYLED_OSTREAM_H */
