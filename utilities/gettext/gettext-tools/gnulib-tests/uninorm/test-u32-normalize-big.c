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

/* Written by Bruno Haible <bruno@clisp.org>, 2009.  */

#include <config.h>

/* Specification.  */
#include "test-u32-normalize-big.h"

#if GNULIB_TEST_UNINORM_U32_NORMALIZE

#include <stdio.h>
#include <stdlib.h>

#include "xalloc.h"
#include "unistr.h"
#define NO_MAIN_HERE
#include "macros.h"

#define ASSERT_WITH_LINE(expr, file, line) \
  do                                                                         \
    {                                                                        \
      if (!(expr))                                                           \
        {                                                                    \
          fprintf (stderr, "%s:%d: assertion failed for %s:%u\n",            \
                   __FILE__, __LINE__, file, line);                          \
          fflush (stderr);                                                   \
          abort ();                                                          \
        }                                                                    \
    }                                                                        \
  while (0)

static int
cmp_ucs4_t (const void *a, const void *b)
{
  ucs4_t a_value = *(const ucs4_t *)a;
  ucs4_t b_value = *(const ucs4_t *)b;
  return (a_value < b_value ? -1 : a_value > b_value ? 1 : 0);
}

void
read_normalization_test_file (const char *filename,
                              struct normalization_test_file *file)
{
  FILE *stream;
  unsigned int lineno;
  struct normalization_test_line *lines;
  size_t lines_length;
  size_t lines_allocated;

  stream = fopen (filename, "r");
  if (stream == NULL)
    {
      fprintf (stderr, "error during fopen of '%s'\n", filename);
      exit (1);
    }

  for (int part_index = 0; part_index < 6; part_index++)
    {
      file->parts[part_index].lines = NULL;
      file->parts[part_index].lines_length = 0;
    }

  lineno = 0;

  int part_index = -1;
  lines = NULL;
  lines_length = 0;
  lines_allocated = 0;

  for (;;)
    {
      char buf[1000+1];
      char *ptr;
      int c;
      struct normalization_test_line line;

      lineno++;

      /* Read a line.  */
      ptr = buf;
      do
        {
          c = getc (stream);
          if (c == EOF || c == '\n')
            break;
          *ptr++ = c;
        }
      while (ptr < buf + 1000);
      *ptr = '\0';
      if (c == EOF)
        break;

      /* Ignore empty lines and comment lines.  */
      if (buf[0] == '\0' || buf[0] == '#')
        continue;

      /* Handle lines that introduce a new part.  */
      if (buf[0] == '@')
        {
          /* Switch to the next part.  */
          if (part_index >= 0)
            {
              lines =
                (struct normalization_test_line *)
                xreallocarray (lines, lines_length, sizeof *lines);
              file->parts[part_index].lines = lines;
              file->parts[part_index].lines_length = lines_length;
            }
          part_index++;
          lines = NULL;
          lines_length = 0;
          lines_allocated = 0;
          continue;
        }

      /* It's a line containing 5 sequences of Unicode characters.
         Parse it and append it to the current part.  */
      if (!(part_index >= 0 && part_index < 6))
        {
          fprintf (stderr, "unexpected structure of '%s'\n", filename);
          exit (1);
        }
      ptr = buf;
      line.lineno = lineno;
      for (size_t sequence_index = 0; sequence_index < 5; sequence_index++)
        line.sequences[sequence_index] = NULL;
      for (size_t sequence_index = 0; sequence_index < 5; sequence_index++)
        {
          uint32_t *sequence = XNMALLOC (1, uint32_t);
          size_t sequence_length = 0;

          for (;;)
            {
              char *endptr;
              unsigned int uc;

              uc = strtoul (ptr, &endptr, 16);
              if (endptr == ptr)
                break;
              ptr = endptr;

              /* Append uc to the sequence.  */
              sequence =
                (uint32_t *)
                xreallocarray (sequence, sequence_length + 2, sizeof *sequence);
              sequence[sequence_length] = uc;
              sequence_length++;

              if (*ptr == ' ')
                ptr++;
            }
          if (sequence_length == 0)
            {
              fprintf (stderr, "empty character sequence in '%s'\n", filename);
              exit (1);
            }
          sequence[sequence_length] = 0; /* terminator */

          line.sequences[sequence_index] = sequence;

          if (*ptr != ';')
            {
              fprintf (stderr, "error parsing '%s'\n", filename);
              exit (1);
            }
          ptr++;
        }

      /* Append the line to the current part.  */
      if (lines_length == lines_allocated)
        {
          lines_allocated = 2 * lines_allocated;
          if (lines_allocated < 7)
            lines_allocated = 7;
          lines =
            (struct normalization_test_line *)
            xreallocarray (lines, lines_allocated, sizeof *lines);
        }
      lines[lines_length] = line;
      lines_length++;
    }

  if (part_index >= 0)
    {
      lines =
        (struct normalization_test_line *)
        xreallocarray (lines, lines_length, sizeof *lines);
      file->parts[part_index].lines = lines;
      file->parts[part_index].lines_length = lines_length;
    }

  {
    /* Collect all c1 values from the part 1 in an array.  */
    const struct normalization_test_part *p = &file->parts[1];
    ucs4_t *c1_array = XNMALLOC (p->lines_length + 1, ucs4_t);

    for (size_t line_index = 0; line_index < p->lines_length; line_index++)
      {
        const uint32_t *sequence = p->lines[line_index].sequences[0];
        /* In part 1, every sequences[0] consists of a single character.  */
        if (!(sequence[0] != 0 && sequence[1] == 0))
          abort ();
        c1_array[line_index] = sequence[0];
      }

    /* Sort this array.  */
    qsort (c1_array, p->lines_length, sizeof (ucs4_t), cmp_ucs4_t);

    /* Add the sentinel at the end.  */
    c1_array[p->lines_length] = 0x110000;

    file->part1_c1_sorted = c1_array;
  }

  file->filename = xstrdup (filename);

  if (ferror (stream) || fclose (stream))
    {
      fprintf (stderr, "error reading from '%s'\n", filename);
      exit (1);
    }
}

void
test_specific (const struct normalization_test_file *file,
               int (*check) (const uint32_t *c1, size_t c1_length,
                             const uint32_t *c2, size_t c2_length,
                             const uint32_t *c3, size_t c3_length,
                             const uint32_t *c4, size_t c4_length,
                             const uint32_t *c5, size_t c5_length))
{
  for (size_t part_index = 0; part_index < 6; part_index++)
    {
      const struct normalization_test_part *p = &file->parts[part_index];

      for (size_t line_index = 0; line_index < p->lines_length; line_index++)
        {
          const struct normalization_test_line *l = &p->lines[line_index];

          ASSERT_WITH_LINE (check (l->sequences[0], u32_strlen (l->sequences[0]),
                                   l->sequences[1], u32_strlen (l->sequences[1]),
                                   l->sequences[2], u32_strlen (l->sequences[2]),
                                   l->sequences[3], u32_strlen (l->sequences[3]),
                                   l->sequences[4], u32_strlen (l->sequences[4]))
                            == 0,
                            file->filename, l->lineno);
        }
    }
}

void
test_other (const struct normalization_test_file *file, uninorm_t nf)
{
  /* Check that for every character not listed in part 1 of the
     NormalizationTest.txt file, the character maps to itself in each
     of the four normalization forms.  */
  const ucs4_t *p = file->part1_c1_sorted;

  for (ucs4_t uc = 0; uc < 0x110000; uc++)
    {
      if (uc >= 0xD800 && uc < 0xE000)
        {
          /* A surrogate, not a character.  Skip uc.  */
        }
      else if (uc == *p)
        {
          /* Skip uc.  */
          p++;
        }
      else
        {
          uint32_t input[1];
          size_t length;
          uint32_t *result;

          input[0] = uc;
          result = u32_normalize (nf, input, 1, NULL, &length);
          ASSERT (result != NULL && length == 1 && result[0] == uc);

          free (result);
        }
    }
}

void
free_normalization_test_file (struct normalization_test_file *file)
{
  for (size_t part_index = 0; part_index < 6; part_index++)
    {
      const struct normalization_test_part *p = &file->parts[part_index];

      for (size_t line_index = 0; line_index < p->lines_length; line_index++)
        {
          const struct normalization_test_line *l = &p->lines[line_index];

          for (size_t sequence_index = 0; sequence_index < 5; sequence_index++)
            free (l->sequences[sequence_index]);
        }
      free (p->lines);
    }
  free (file->part1_c1_sorted);
  free (file->filename);
}

#endif
