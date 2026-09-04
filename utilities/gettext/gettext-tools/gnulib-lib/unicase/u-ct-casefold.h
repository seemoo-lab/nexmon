/* Casefolding mapping for Unicode substrings (locale dependent).
   Copyright (C) 2009-2026 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2009.

   This file is free software.
   It is dual-licensed under "the GNU LGPLv3+ or the GNU GPLv2+".
   You can redistribute it and/or modify it under either
     - the terms of the GNU Lesser General Public License as published
       by the Free Software Foundation, either version 3, or (at your
       option) any later version, or
     - the terms of the GNU General Public License as published by the
       Free Software Foundation; either version 2, or (at your option)
       any later version, or
     - the same dual license "the GNU LGPLv3+ or the GNU GPLv2+".

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License and the GNU General Public License
   for more details.

   You should have received a copy of the GNU Lesser General Public
   License and of the GNU General Public License along with this
   program.  If not, see <https://www.gnu.org/licenses/>.  */

UNIT *
FUNC (const UNIT *s, size_t n,
      casing_prefix_context_t prefix_context,
      casing_suffix_context_t suffix_context,
      const char *iso639_language,
      uninorm_t nf,
      UNIT *resultbuf, size_t *lengthp)
{
  /* Implement the three definitions of caseless matching, as described in
     Unicode 5.0, section "Default caseless matching":
       - If no normalization is requested, simply apply the casefolding.
           X -> toCasefold(X).
       - If canonical normalization is requested, apply it, and apply an NFD
         before.
           X -> NFD(toCasefold(NFD(X))).
       - If compatibility normalization is requested, apply it twice, apply
         the normalization after each, and apply an NFD before:
           X -> NFKD(toCasefold(NFKD(toCasefold(NFD(X))))).  */
  if (nf == NULL)
    /* X -> toCasefold(X) */
    return U_CASEMAP (s, n, prefix_context, suffix_context, iso639_language,
                      uc_tocasefold, offsetof (struct special_casing_rule, casefold[0]),
                      NULL,
                      resultbuf, lengthp);
  else
    {
      uninorm_t nfd = uninorm_decomposing_form (nf);
      /* X -> nf(toCasefold(NFD(X))) or
         X -> nf(toCasefold(nfd(toCasefold(NFD(X)))))  */
      int repeat = (uninorm_is_compat_decomposing (nf) ? 2 : 1);

      UNIT tmpbuf1[2048 / sizeof (UNIT)];
      size_t tmp1_length = sizeof (tmpbuf1) / sizeof (UNIT);
      UNIT *tmp1 = U_NORMALIZE (UNINORM_NFD, s, n, tmpbuf1, &tmp1_length);
      if (tmp1 == NULL)
        /* errno is set here.  */
        return NULL;

      do
        {
          UNIT tmpbuf2[2048 / sizeof (UNIT)];
          size_t tmp2_length = sizeof (tmpbuf2) / sizeof (UNIT);
          UNIT *tmp2 = U_CASEMAP (tmp1, tmp1_length,
                                  prefix_context, suffix_context, iso639_language,
                                  uc_tocasefold, offsetof (struct special_casing_rule, casefold[0]),
                                  NULL,
                                  tmpbuf2, &tmp2_length);
          if (tmp2 == NULL)
            {
              int saved_errno = errno;
              if (tmp1 != tmpbuf1)
                free (tmp1);
              errno = saved_errno;
              return NULL;
            }

          if (tmp1 != tmpbuf1)
            free (tmp1);

          if (repeat > 1)
            {
              tmp1_length = sizeof (tmpbuf1) / sizeof (UNIT);
              tmp1 = U_NORMALIZE (nfd, tmp2, tmp2_length,
                                  tmpbuf1, &tmp1_length);
            }
          else
            /* Last run through this loop.  */
            tmp1 = U_NORMALIZE (nf, tmp2, tmp2_length,
                                resultbuf, lengthp);
          if (tmp1 == NULL)
            {
              int saved_errno = errno;
              if (tmp2 != tmpbuf2)
                free (tmp2);
              errno = saved_errno;
              return NULL;
            }

          if (tmp2 != tmpbuf2)
            free (tmp2);
        }
      while (--repeat > 0);

      return tmp1;
    }
}
