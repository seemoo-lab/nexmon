# Copyright (C) 2002-2004, 2006-2018 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

double_exponent_test = '''
#include <float.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#define NWORDS \
  ((sizeof (double) + sizeof (unsigned int) - 1) / sizeof (unsigned int))
typedef union { double value; unsigned int word[NWORDS]; } memory_double;
static unsigned int ored_words[NWORDS];
static unsigned int anded_words[NWORDS];
static void add_to_ored_words (double x)
{
  memory_double m;
  size_t i;
  /* Clear it first, in case sizeof (double) < sizeof (memory_double).  */
  memset (&m, 0, sizeof (memory_double));
  m.value = x;
  for (i = 0; i < NWORDS; i++)
    {
      ored_words[i] |= m.word[i];
      anded_words[i] &= m.word[i];
    }
}
int main ()
{
  size_t j;
  FILE *fp = stdout;
  if (fp == NULL)
    return 1;
  for (j = 0; j < NWORDS; j++)
    anded_words[j] = ~ (unsigned int) 0;
  add_to_ored_words (0.25);
  add_to_ored_words (0.5);
  add_to_ored_words (1.0);
  add_to_ored_words (2.0);
  add_to_ored_words (4.0);
  /* Remove bits that are common (e.g. if representation of the first mantissa
     bit is explicit).  */
  for (j = 0; j < NWORDS; j++)
    ored_words[j] &= ~anded_words[j];
  /* Now find the nonzero word.  */
  for (j = 0; j < NWORDS; j++)
    if (ored_words[j] != 0)
      break;
  if (j < NWORDS)
    {
      size_t i;
      for (i = j + 1; i < NWORDS; i++)
        if (ored_words[i] != 0)
          {
            fprintf (fp, "-1/-1");
            return (fclose (fp) != 0);
          }
      for (i = 0; ; i++)
        if ((ored_words[j] >> i) & 1)
          {
            fprintf (fp, "%d/%d", (int) j, (int) i);
            return (fclose (fp) != 0);
          }
    }
  fprintf (fp, "-1/-1");
  return (fclose (fp) != 0);
}
'''

gl_cv_cc_double_expbit0_word = -1
gl_cv_cc_double_expbit0_bit = -1

if meson.can_run_host_binaries()
  run_result = cc.run(double_exponent_test,
      name : 'where to find the exponent in a \'double\'')
  if run_result.compiled() and run_result.returncode() == 0
    out = run_result.stdout ().split ('/')
    if out.length () == 2
      gl_cv_cc_double_expbit0_word = out[0].to_int ()
      gl_cv_cc_double_expbit0_bit = out[1].to_int ()
    endif
  endif
else
  # On ARM, there are two 'double' floating-point formats, used by
  # different sets of instructions: The older FPA instructions assume
  # that they are stored in big-endian word order, while the words
  # (like integer types) are stored in little-endian byte order.
  # The newer VFP instructions assume little-endian order
  # consistently.
  if (cc.get_define ('arm') == '' and
      cc.get_define ('__arm') == '' and
      cc.get_define ('__arm__') == '')
    gl_cv_cc_double_expbit0_bit = 20
    if host_machine.endian () == 'big'
      gl_cv_cc_double_expbit0_word = 0
    else
      gl_cv_cc_double_expbit0_word = 1
    endif
  endif
endif
