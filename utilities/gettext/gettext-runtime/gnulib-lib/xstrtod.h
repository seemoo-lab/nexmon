/* Error-checking interface to strtod-like functions.

   Copyright (C) 1996, 1998, 2003-2004, 2006, 2009-2026 Free Software
   Foundation, Inc.

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

/* Written by Jim Meyering.  */

#ifndef XSTRTOD_H
#define XSTRTOD_H 1

#ifdef __cplusplus
extern "C" {
#endif


/* Converts the initial portion of the string starting at STR (if PTR != NULL)
   or the entire string starting at STR (if PTR == NULL) to a number of type
   'double'.
   CONVERT should be a conversion function with the same calling conventions
   as strtod, such as strtod or c_strtod.
   If successful, it returns true, with *RESULT set to the result.
   If it fails, it returns false, with *RESULT and errno set.
   In total, there are the following cases:

   Condition                                          RET     *RESULT     errno
   -----------------------------------------------------------------------------
   conversion error: no number parsed                 false   0.0         EINVAL or 0
   PTR == NULL, number parsed but junk after number   false   value       0
   NaN                                                true    NaN         0
   ±Infinity                                          true    ±HUGE_VAL   0
   overflow                                           false   ±HUGE_VAL   ERANGE
   gradual underflow                                  true    near zero   0
   flush-to-zero underflow                            true    ±0.0        ERANGE
   other finite value                                 true    value       0

   In both cases, if PTR != NULL, *PTR is set to point to the character after
   the parsed number.  */
bool xstrtod (const char *str, const char **ptr, double *result,
              double (*convert) (char const *, char **));

/* Converts the initial portion of the string starting at STR (if PTR != NULL)
   or the entire string starting at STR (if PTR == NULL) to a number of type
   'long double'.
   CONVERT should be a conversion function with the same calling conventions
   as strtold, such as strtold or c_strtold.
   If successful, it returns true, with *RESULT set to the result.
   If it fails, it returns false, with *RESULT and errno set.
   In total, there are the following cases:

   Condition                                          RET     *RESULT     errno
   -----------------------------------------------------------------------------
   conversion error: no number parsed                 false   0.0L        EINVAL or 0
   PTR == NULL, number parsed but junk after number   false   value       0
   NaN                                                true    NaN         0
   ±Infinity                                          true    ±HUGE_VALL  0
   overflow                                           false   ±HUGE_VALL  ERANGE
   gradual underflow                                  true    near zero   0
   flush-to-zero underflow                            true    ±0.0L       ERANGE
   other finite value                                 true    value       0

   In both cases, if PTR != NULL, *PTR is set to point to the character after
   the parsed number.  */
bool xstrtold (const char *str, const char **ptr, long double *result,
               long double (*convert) (char const *, char **));


#ifdef __cplusplus
}
#endif

#endif /* not XSTRTOD_H */
