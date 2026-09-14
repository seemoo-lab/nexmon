/* Test of conversion of multibyte character to wide character
   on native Windows in the UTF-8 environment.
   Copyright (C) 2024-2026 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2024.  */

#include <config.h>

#include <wchar.h>

#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "macros.h"

int
main (void)
{
#ifdef _UCRT
  /* Test that MB_CUR_MAX and mbrtowc() work as expected in a UTF-8 locale.  */
  mbstate_t state;
  wchar_t wc;
  size_t ret;

  if (setlocale (LC_ALL, "") == NULL)
    return 1;

  ASSERT (MB_CUR_MAX >= 4);

  {
    char input[] = "B\303\274\303\237er"; /* "Büßer" */
    memset (&state, '\0', sizeof (mbstate_t));

    wc = (wchar_t) 0xBADFACE;
    ret = mbrtowc (&wc, input, 1, &state);
    ASSERT (ret == 1);
    ASSERT (wc == 'B');
    ASSERT (mbsinit (&state));
    input[0] = '\0';

    wc = (wchar_t) 0xBADFACE;
    ret = mbrtowc (&wc, input + 1, 1, &state);
    ASSERT (ret == (size_t)(-2));
    ASSERT (wc == (wchar_t) 0xBADFACE);
    ASSERT (!mbsinit (&state));
    input[1] = '\0';

    wc = (wchar_t) 0xBADFACE;
    ret = mbrtowc (&wc, input + 2, 5, &state);
    ASSERT (ret == 1);
    ASSERT (wctob (wc) == EOF);
    ASSERT (wc == 0x00FC);
    ASSERT (mbsinit (&state));
    input[2] = '\0';

    /* Test support of NULL first argument.  */
    ret = mbrtowc (NULL, input + 3, 4, &state);
    ASSERT (ret == 2);
    ASSERT (mbsinit (&state));

    wc = (wchar_t) 0xBADFACE;
    ret = mbrtowc (&wc, input + 3, 4, &state);
    ASSERT (ret == 2);
    ASSERT (wctob (wc) == EOF);
    ASSERT (wc == 0x00DF);
    ASSERT (mbsinit (&state));
    input[3] = '\0';
    input[4] = '\0';

    wc = (wchar_t) 0xBADFACE;
    ret = mbrtowc (&wc, input + 5, 2, &state);
    ASSERT (ret == 1);
    ASSERT (wc == 'e');
    ASSERT (mbsinit (&state));
    input[5] = '\0';

    wc = (wchar_t) 0xBADFACE;
    ret = mbrtowc (&wc, input + 6, 1, &state);
    ASSERT (ret == 1);
    ASSERT (wc == 'r');
    ASSERT (mbsinit (&state));

    /* Test some invalid input.  */
    memset (&state, '\0', sizeof (mbstate_t));
    wc = (wchar_t) 0xBADFACE;
    ret = mbrtowc (&wc, "\377", 1, &state); /* 0xFF */
    ASSERT (ret == (size_t)-1);
    ASSERT (errno == EILSEQ);

    memset (&state, '\0', sizeof (mbstate_t));
    wc = (wchar_t) 0xBADFACE;
    ret = mbrtowc (&wc, "\303\300", 2, &state); /* 0xC3 0xC0 */
    ASSERT (ret == (size_t)-1);
    ASSERT (errno == EILSEQ);

    memset (&state, '\0', sizeof (mbstate_t));
    wc = (wchar_t) 0xBADFACE;
    ret = mbrtowc (&wc, "\343\300", 2, &state); /* 0xE3 0xC0 */
    ASSERT (ret == (size_t)-1);
    ASSERT (errno == EILSEQ);

    memset (&state, '\0', sizeof (mbstate_t));
    wc = (wchar_t) 0xBADFACE;
    ret = mbrtowc (&wc, "\343\300\200", 3, &state); /* 0xE3 0xC0 0x80 */
    ASSERT (ret == (size_t)-1);
    ASSERT (errno == EILSEQ);

    memset (&state, '\0', sizeof (mbstate_t));
    wc = (wchar_t) 0xBADFACE;
    ret = mbrtowc (&wc, "\343\200\300", 3, &state); /* 0xE3 0x80 0xC0 */
    ASSERT (ret == (size_t)-1);
    ASSERT (errno == EILSEQ);

    memset (&state, '\0', sizeof (mbstate_t));
    wc = (wchar_t) 0xBADFACE;
    ret = mbrtowc (&wc, "\363\300", 2, &state); /* 0xF3 0xC0 */
    ASSERT (ret == (size_t)-1);
    ASSERT (errno == EILSEQ);

    memset (&state, '\0', sizeof (mbstate_t));
    wc = (wchar_t) 0xBADFACE;
    ret = mbrtowc (&wc, "\363\300\200\200", 4, &state); /* 0xF3 0xC0 0x80 0x80 */
    ASSERT (ret == (size_t)-1);
    ASSERT (errno == EILSEQ);

    memset (&state, '\0', sizeof (mbstate_t));
    wc = (wchar_t) 0xBADFACE;
    ret = mbrtowc (&wc, "\363\200\300", 3, &state); /* 0xF3 0x80 0xC0 */
    ASSERT (ret == (size_t)-1);
    ASSERT (errno == EILSEQ);

    memset (&state, '\0', sizeof (mbstate_t));
    wc = (wchar_t) 0xBADFACE;
    ret = mbrtowc (&wc, "\363\200\300\200", 4, &state); /* 0xF3 0x80 0xC0 0x80 */
    ASSERT (ret == (size_t)-1);
    ASSERT (errno == EILSEQ);

    memset (&state, '\0', sizeof (mbstate_t));
    wc = (wchar_t) 0xBADFACE;
    ret = mbrtowc (&wc, "\363\200\200\300", 4, &state); /* 0xF3 0x80 0x80 0xC0 */
    ASSERT (ret == (size_t)-1);
    ASSERT (errno == EILSEQ);
  }

  return test_exit_status;
#else
  fputs ("Skipping test: not using the UCRT runtime\n", stderr);
  return 77;
#endif
}
