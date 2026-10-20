/* t-cipher-internal.c - Regression tests for cipher-internal.h helpers
 * Copyright (C) 2026 Jussi Kivilinna <jussi.kivilinna@iki.fi>
 *
 * This file is part of Libgcrypt.
 *
 * Libgcrypt is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Libgcrypt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/g10lib.h"
#include "../cipher/cipher-internal.h"

#define PGM "t-cipher-internal"

static int verbose;
static int debug;
static int error_count;

static void
print_hilo (const char *label, u64 v)
{
  fprintf (stderr, "%s=%08lx%08lx", label,
           (unsigned long)(u32)(v >> 32), (unsigned long)(u32)v);
}

static void
check_bytecounter_add (u64 start, u64 add)
{
  u32 ctr[2];
  u64 want = start + add;
  int want_ovf = add > (~(u64)0 - start);
  int got_ovf;

  ctr[0] = (u32)start;
  ctr[1] = (u32)(start >> 32);

  got_ovf = cipher_bytecounter_add (ctr, (size_t)add);

  if ((((u64)ctr[1] << 32) | ctr[0]) != want || !!got_ovf != !!want_ovf)
    {
      error_count++;
      print_hilo ("FAIL: start", start);
      print_hilo (" add", add);
      print_hilo (" -> got", ((u64)ctr[1] << 32) | ctr[0]);
      fprintf (stderr, " ovf=%d,", !!got_ovf);
      print_hilo (" want", want);
      fprintf (stderr, " ovf=%d\n", !!want_ovf);
    }
  else if (debug)
    {
      print_hilo ("ok: start", start);
      print_hilo (" add", add);
      print_hilo (" -> ", want);
      fprintf (stderr, " ovf=%d\n", !!want_ovf);
    }
}

static void
test_bytecounter_add (void)
{
  static const u64 starts[] = {
    U64_C (0x0000000000000000), U64_C (0x0000000000000001),
    U64_C (0x0000000090000000), U64_C (0x00000000ffffffff),
    U64_C (0x00000001ffffffff), U64_C (0xfffffffe00000000),
    U64_C (0x123456789abcdef0)
  };
  static const u64 adds[] = {
    U64_C (0), U64_C (1), U64_C (15), U64_C (16),
    U64_C (0x000000007fffffff), U64_C (0x0000000080000000),
    U64_C (0x00000000ffffffff), U64_C (0x0000000100000000),
    U64_C (0x0000000100000010), U64_C (0x0000000180000000),
    U64_C (0x0000001000000000), U64_C (0x000000fffffffff0),
    U64_C (0xffffffffffffffff)
  };
  unsigned int s, a;

  if (verbose)
    fprintf (stderr, "  checking cipher_bytecounter_add\n");

  for (s = 0; s < DIM (starts); s++)
    for (a = 0; a < DIM (adds); a++)
      {
        if (sizeof (size_t) < 8 && adds[a] > U64_C (0xffffffff))
          continue;
        check_bytecounter_add (starts[s], adds[a]);
      }

  if (sizeof (size_t) > 4)
    {
      u32 ctr[2] = { 0, 0 };
      cipher_bytecounter_add (ctr, (size_t)U64_C (0x100000000));
      if (ctr[0] != 0 || ctr[1] != 1)
        {
          error_count++;
          fprintf (stderr, "FAIL: 4 GiB add gave %08lx:%08lx,"
                   " expected 00000001:00000000\n",
                   (unsigned long)ctr[1], (unsigned long)ctr[0]);
        }
    }
}

int
main (int argc, char **argv)
{
  int last_argc = -1;

  if (argc)
    { argc--; argv++; }

  while (argc && last_argc != argc)
    {
      last_argc = argc;
      if (!strcmp (*argv, "--"))
        {
          argc--; argv++;
          break;
        }
      else if (!strcmp (*argv, "--verbose"))
        {
          verbose++;
          argc--; argv++;
        }
      else if (!strcmp (*argv, "--debug"))
        {
          verbose = debug = 1;
          argc--; argv++;
        }
    }

  if (verbose)
    fprintf (stderr, "Starting cipher-internal checks.\n");

  test_bytecounter_add ();

  if (error_count)
    fprintf (stderr, PGM ": %d test(s) failed\n", error_count);

  return !!error_count;
}
