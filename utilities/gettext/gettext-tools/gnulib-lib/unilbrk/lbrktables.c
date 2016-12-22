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

#include <config.h>

/* Specification.  */
#include "unilbrk/lbrktables.h"

/* Define unilbrkprop, table of line breaking properties.  */
#include "unilbrk/lbrkprop2.h"

const unsigned char unilbrk_table[27][27] =
{
                                /* after */
        /* WJ GL B2 BA BB HY CL CP EX IN NS OP QU IS NU PO PR SY AL H2 H3 ID JL JV JT HL RI */
/* WJ */ { P, I, I, I, I, I, P, P, P, I, I, I, I, P, I, I, I, P, I, I, I, I, I, I, I, I, I, },
/* GL */ { P, I, I, I, I, I, P, P, P, I, I, I, I, P, I, I, I, P, I, I, I, I, I, I, I, I, I, },
/* B2 */ { P, I, P, I, D, I, P, P, P, D, I, D, I, P, D, D, D, P, D, D, D, D, D, D, D, D, D, },
/* BA */ { P, D, D, I, D, I, P, P, P, D, I, D, I, P, D, D, D, P, D, D, D, D, D, D, D, D, D, },
/* BB */ { P, I, I, I, I, I, P, P, P, I, I, I, I, P, I, I, I, P, I, I, I, I, I, I, I, I, I, },
/* HY */ { P, D, D, I, D, I, P, P, P, D, I, D, I, P, I, D, D, P, D, D, D, D, D, D, D, D, D, },
/* CL */ { P, I, D, I, D, I, P, P, P, D, P, D, I, P, D, I, I, P, D, D, D, D, D, D, D, D, D, },
/* CP */ { P, I, D, I, D, I, P, P, P, D, P, D, I, P, I, I, I, P, I, D, D, D, D, D, D, I, D, },
/* EX */ { P, I, D, I, D, I, P, P, P, I, I, D, I, P, D, D, D, P, D, D, D, D, D, D, D, D, D, },
/* IN */ { P, I, D, I, D, I, P, P, P, I, I, D, I, P, D, D, D, P, D, D, D, D, D, D, D, D, D, },
/* NS */ { P, I, D, I, D, I, P, P, P, D, I, D, I, P, D, D, D, P, D, D, D, D, D, D, D, D, D, },
/* OP */ { P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, },
/* QU */ { P, I, I, I, I, I, P, P, P, I, I, P, I, P, I, I, I, P, I, I, I, I, I, I, I, I, I, },
/* IS */ { P, I, D, I, D, I, P, P, P, D, I, D, I, P, I, D, D, P, D, D, D, D, D, D, D, D, D, },
/* NU */ { P, I, D, I, D, I, P, P, P, I, I, I, I, P, I, I, I, P, I, D, D, D, D, D, D, I, D, },
/* PO */ { P, I, D, I, D, I, P, P, P, D, I, I, I, P, I, D, D, P, I, D, D, D, D, D, D, I, D, },
/* PR */ { P, I, D, I, D, I, P, P, P, D, I, I, I, P, I, D, D, P, I, I, I, I, I, I, I, I, D, },
/* SY */ { P, I, D, I, D, I, P, P, P, D, I, D, I, P, I, D, D, P, D, D, D, D, D, D, D, I, D, },
/* AL */ { P, I, D, I, D, I, P, P, P, I, I, I, I, P, I, D, D, P, I, D, D, D, D, D, D, I, D, },
/* H2 */ { P, I, D, I, D, I, P, P, P, I, I, D, I, P, D, I, D, P, D, D, D, D, D, I, I, D, D, },
/* H3 */ { P, I, D, I, D, I, P, P, P, I, I, D, I, P, D, I, D, P, D, D, D, D, D, D, I, D, D, },
/* ID */ { P, I, D, I, D, I, P, P, P, I, I, D, I, P, D, I, D, P, D, D, D, D, D, D, D, D, D, },
/* JL */ { P, I, D, I, D, I, P, P, P, I, I, D, I, P, D, I, D, P, D, I, I, D, I, I, D, D, D, },
/* JV */ { P, I, D, I, D, I, P, P, P, I, I, D, I, P, D, I, D, P, D, D, D, D, D, I, I, D, D, },
/* JT */ { P, I, D, I, D, I, P, P, P, I, I, D, I, P, D, I, D, P, D, D, D, D, D, D, I, D, D, },
/* HL */ { P, I, D, I, D, I, P, P, P, I, I, I, I, P, I, D, D, P, I, D, D, D, D, D, D, I, D, },
/* RI */ { P, I, D, I, D, I, P, P, P, D, I, D, I, P, D, D, D, P, D, D, D, D, D, D, I, D, I, },
/* "" */
/* before */
};
/* Note: The (IS,AL) entry has been changed from I to D. In other words, the
   rule "Do not break between numeric punctuation and alphabetics" is not
   implemented here. We want to break before the HTML tag in strings like
   "<P>Some sentence.</P>"  */
/* Note: The (B2,B2) entry should probably be D instead of P.  */
/* Note: The (PR,ID) entry should probably be D instead of I.  */
/* Note: The (WJ,*) and (GL,*) entries should probably be P instead of I.  */
