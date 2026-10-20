/*
 * Copyright 2018 LLVM contributors
 *
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 * Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 */

/* Simpler gnu89 version of StandaloneFuzzTargetMain.c from LLVM */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

extern int LLVMFuzzerTestOneInput (const unsigned char *data, size_t size);

int
main (int argc, char **argv)
{
  FILE *f;
  long tell_result;
  size_t n_read, len;
  unsigned char *buf;

  if (argc < 2)
    return 1;

  f = fopen (argv[1], "r");
  assert (f);
  fseek (f, 0, SEEK_END);
  tell_result = ftell (f);
  assert (tell_result >= 0);
  len = (size_t) tell_result;
  fseek (f, 0, SEEK_SET);
  buf = (unsigned char*) malloc (len);
  n_read = fread (buf, 1, len, f);
  assert (n_read == len);
  LLVMFuzzerTestOneInput (buf, len);

  free (buf);
  fclose (f);
  printf ("Done!\n");
  return 0;
}
