/* Line breaking auxiliary tables.
   Copyright (C) 2001-2003, 2006-2016 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2001.

   This program is free software: you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "unitypes.h"

/* Line breaking classification.  */

enum
{
  /* Values >= 27 are resolved at run time. */
  LBP_BK = 27, /* mandatory break */
/*LBP_CR,         carriage return - not used here because it's a DOSism */
/*LBP_LF,         line feed - not used here because it's a DOSism */
  LBP_CM = 28, /* attached characters and combining marks */
/*LBP_NL,         next line - not used here because it's equivalent to LBP_BK */
/*LBP_SG,         surrogates - not used here because they are not characters */
  LBP_WJ =  0, /* word joiner */
  LBP_ZW = 29, /* zero width space */
  LBP_GL =  1, /* non-breaking (glue) */
  LBP_SP = 30, /* space */
  LBP_B2 =  2, /* break opportunity before and after */
  LBP_BA =  3, /* break opportunity after */
  LBP_BB =  4, /* break opportunity before */
  LBP_HY =  5, /* hyphen */
  LBP_CB = 31, /* contingent break opportunity */
  LBP_CL =  6, /* closing punctuation */
  LBP_CP =  7, /* closing parenthesis */
  LBP_EX =  8, /* exclamation/interrogation */
  LBP_IN =  9, /* inseparable */
  LBP_NS = 10, /* non starter */
  LBP_OP = 11, /* opening punctuation */
  LBP_QU = 12, /* ambiguous quotation */
  LBP_IS = 13, /* infix separator (numeric) */
  LBP_NU = 14, /* numeric */
  LBP_PO = 15, /* postfix (numeric) */
  LBP_PR = 16, /* prefix (numeric) */
  LBP_SY = 17, /* symbols allowing breaks */
  LBP_AI = 32, /* ambiguous (alphabetic or ideograph) */
  LBP_AL = 18, /* ordinary alphabetic and symbol characters */
/*LBP_CJ,         conditional Japanese starters, resolved to NS */
  LBP_H2 = 19, /* Hangul LV syllable */
  LBP_H3 = 20, /* Hangul LVT syllable */
  LBP_HL = 25, /* Hebrew letter */
  LBP_ID = 21, /* ideographic */
  LBP_JL = 22, /* Hangul L Jamo */
  LBP_JV = 23, /* Hangul V Jamo */
  LBP_JT = 24, /* Hangul T Jamo */
  LBP_RI = 26, /* regional indicator */
  LBP_SA = 33, /* complex context (South East Asian) */
  LBP_XX = 34  /* unknown */
};

#include "lbrkprop1.h"

static inline unsigned char
unilbrkprop_lookup (ucs4_t uc)
{
  unsigned int index1 = uc >> lbrkprop_header_0;
  if (index1 < lbrkprop_header_1)
    {
      int lookup1 = unilbrkprop.level1[index1];
      if (lookup1 >= 0)
        {
          unsigned int index2 = (uc >> lbrkprop_header_2) & lbrkprop_header_3;
          int lookup2 = unilbrkprop.level2[lookup1 + index2];
          if (lookup2 >= 0)
            {
              unsigned int index3 = uc & lbrkprop_header_4;
              return unilbrkprop.level3[lookup2 + index3];
            }
        }
    }
  return LBP_XX;
}

/* Table indexed by two line breaking classifications.  */
#define D 1  /* direct break opportunity, empty in table 7.3 of UTR #14 */
#define I 2  /* indirect break opportunity, '%' in table 7.3 of UTR #14 */
#define P 3  /* prohibited break,           '^' in table 7.3 of UTR #14 */

extern const unsigned char unilbrk_table[27][27];

/* We don't support line breaking of complex-context dependent characters
   (Thai, Lao, Myanmar, Khmer) yet, because it requires dictionary lookup. */
