/* Test of Unicode compliance of normalization of UTF-32 strings.
   Copyright (C) 2009-2026 Free Software Foundation, Inc.

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

#include <stddef.h>

#include "unitypes.h"
#include "uninorm.h"

/* The NormalizationTest.txt is from www.unicode.org, with stripped comments:
     sed -e 's| *#.*||' < .../ucd/NormalizationTest.txt \
                        > tests/uninorm/NormalizationTest.txt
   It is only used to verify the compliance of this implementation of the
   Unicode normalization forms.  It is not used by the library code, only
   by the unit tests.  */

/* Representation of a line in the NormalizationTest.txt file.  */
struct normalization_test_line
{
  unsigned int lineno;
  uint32_t *sequences[5];
};

/* Representation of a delimited part of the NormalizationTest.txt file.  */
struct normalization_test_part
{
  struct normalization_test_line *lines;
  size_t lines_length;
};

/* Representation of the entire NormalizationTest.txt file.  */
struct normalization_test_file
{
  struct normalization_test_part parts[6];
  /* The set of c1 values from part 1, sorted in ascending order, with a
     sentinel value of 0x110000 at the end.  */
  ucs4_t *part1_c1_sorted;
  /* The filename of the NormalizationTest.txt file.  */
  char *filename;
};

/* Read the NormalizationTest.txt file and return its contents.  */
extern void
       read_normalization_test_file (const char *filename,
                                     struct normalization_test_file *file);

/* Perform the first compliance test.  */
extern void
       test_specific (const struct normalization_test_file *file,
                      int (*check) (const uint32_t *c1, size_t c1_length,
                                    const uint32_t *c2, size_t c2_length,
                                    const uint32_t *c3, size_t c3_length,
                                    const uint32_t *c4, size_t c4_length,
                                    const uint32_t *c5, size_t c5_length));

/* Perform the second compliance test.  */
extern void
       test_other (const struct normalization_test_file *file, uninorm_t nf);

/* Free the representation of the NormalizationTest.txt file.  */
extern void
       free_normalization_test_file (struct normalization_test_file *file);
