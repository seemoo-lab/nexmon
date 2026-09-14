/* Test parts of the API.
   Copyright (C) 2018-2026 Free Software Foundation, Inc.

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

#include <textstyle.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main ()
{
  /* Test based on libtextstyle/adhoc-tests/hello.c.  */

  memory_ostream_t mstream = memory_ostream_create ();

  style_file_name = TOP_SRCDIR "adhoc-tests/hello-default.css";
  styled_ostream_t stream =
    html_styled_ostream_create (mstream, style_file_name);

  ostream_write_str (stream, "Hello ");

  /* Associate the entire full name in CSS class 'name'.  */
  styled_ostream_begin_use_class (stream, "name");

  ostream_write_str (stream, "Dr. ");
  styled_ostream_begin_use_class (stream, "boy-name");
  /* Start a hyperlink.  */
  styled_ostream_set_hyperlink (stream, "https://en.wikipedia.org/wiki/Linus_Pauling", NULL);
  ostream_write_str (stream, "Linus");
  styled_ostream_end_use_class (stream, "boy-name");
  ostream_write_str (stream, " Pauling");
  /* End the current hyperlink.  */
  styled_ostream_set_hyperlink (stream, NULL, NULL);

  /* Terminate the name.  */
  styled_ostream_end_use_class (stream, "name");

  ostream_write_str (stream, "!\n");

  /* Flush and close the stream.  */
  styled_ostream_free (stream);

  const void *buf;
  size_t buflen;
  memory_ostream_contents (mstream, &buf, &buflen);

  const char *expected =
    "<?xml version=\"1.0\"?>\n"
    "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\n"
    "<html>\n"
    "<head>\n"
    "<style type=\"text/css\">\n"
    "<!--\n"
    "/* This file is in the public domain.\n"
    "\n"
    "   Styling rules for the color-hello example.  */\n"
    "\n"
    ".name      { text-decoration : underline; }\n"
    ".boy-name  { background-color : rgb(123,201,249); }\n"
    ".girl-name { background-color : rgb(250,149,158); }\n"
    "-->\n"
    "</style>\n"
    "</head>\n"
    "<body>\n"
    "Hello&nbsp;<span class=\"name\">Dr.&nbsp;</span><a href=\"https://en.wikipedia.org/wiki/Linus_Pauling\"><span class=\"name\"><span class=\"boy-name\">Linus</span>&nbsp;Pauling</span></a>!<br/></body>\n"
    "</html>\n";
  assert (buflen == strlen (expected));
  assert (memcmp ((const char *) buf, expected, buflen) == 0);

  ostream_free (mstream);

  return 0;
}
