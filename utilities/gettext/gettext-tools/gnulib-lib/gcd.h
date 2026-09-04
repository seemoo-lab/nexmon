/* Arithmetic.
   Copyright (C) 2001-2002, 2006, 2009-2026 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2001.

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

#ifndef _GCD_H
#define _GCD_H

/* Before including this file, you may define:
     GCD_WORD_T         The parameter type and result type of the gcd function.
                        It should be an unsigned integer type that is either
                        a built-in type or defined in <stddef.h> or <stdint.h>.

   The definition of GCD_WORD_T needs to be the same across the entire
   application.  Therefore it is best placed in <config.h>.
 */

#ifdef GCD_WORD_T
/* Make sure that GCD_WORD_T is defined as a type.  */
# include <stddef.h>
# include <stdint.h>
#else
# define GCD_WORD_T unsigned long
#endif

#ifdef __cplusplus
extern "C" {
#endif


/* Return the greatest common divisor of a > 0 and b > 0.  */
extern GCD_WORD_T gcd (GCD_WORD_T a, GCD_WORD_T b);


#ifdef __cplusplus
}
#endif

#endif /* _GCD_H */
