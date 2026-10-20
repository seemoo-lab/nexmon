/* hwf-riscv.c - Detect hardware features - RISC-V part
 * Copyright (C) 2025 Jussi Kivilinna <jussi.kivilinna@iki.fi>
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

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#if defined(HAVE_SYS_AUXV_H) && (defined(HAVE_GETAUXVAL) || \
    defined(HAVE_ELF_AUX_INFO))
#include <sys/auxv.h>
#endif
#if defined(__linux__) && defined(HAVE_SYSCALL)
# include <sys/syscall.h>
#endif

#include "g10lib.h"
#include "hwf-common.h"

#if !defined (__riscv)
# error Module build for wrong CPU.
#endif


#if defined(HAVE_SYS_AUXV_H) && defined(HAVE_ELF_AUX_INFO) && \
    !defined(HAVE_GETAUXVAL) && defined(AT_HWCAP)
#define HAVE_GETAUXVAL
static unsigned long getauxval(unsigned long type)
{
  unsigned long auxval = 0;
  int err;

  /* FreeBSD provides 'elf_aux_info' function that does the same as
   * 'getauxval' on Linux. */

  err = elf_aux_info (type, &auxval, sizeof(auxval));
  if (err)
    {
      errno = err;
      auxval = 0;
    }

  return auxval;
}
#endif


#undef HAS_SYS_AT_HWCAP
#if defined(__linux__) || \
    (defined(HAVE_SYS_AUXV_H) && defined(HAVE_GETAUXVAL))
#define HAS_SYS_AT_HWCAP 1

struct hwcap_feature_map_s {
  unsigned int hwcap_flag;
  unsigned int hwf_flag;
};

/* Note: These macros have same values on Linux and FreeBSD. */
#ifndef AT_HWCAP
# define AT_HWCAP        16
#endif
#ifndef AT_HWCAP2
# define AT_HWCAP2       26
#endif

#define HWCAP_ISA(l)     (1U << (unsigned int)(l - 'a'))
#define HWCAP_ISA_IMAFDC (HWCAP_ISA('i') | HWCAP_ISA('m') | \
			  HWCAP_ISA('a') | HWCAP_ISA('f') | \
			  HWCAP_ISA('d') | HWCAP_ISA('c'))

static const struct hwcap_feature_map_s hwcap_features[] =
  {
    { HWCAP_ISA_IMAFDC,  HWF_RISCV_IMAFDC },
    { HWCAP_ISA('v'),    HWF_RISCV_V },
    { HWCAP_ISA('b'),    HWF_RISCV_B },
    { HWCAP_ISA('b'),    HWF_RISCV_ZBB },
  };

static int
get_hwcap(unsigned int *hwcap)
{
  struct { unsigned long a_type; unsigned long a_val; } auxv;
  FILE *f;
  int err = -1;
  static int hwcap_initialized = 0;
  static unsigned int stored_hwcap = 0;

  if (hwcap_initialized)
    {
      *hwcap = stored_hwcap;
      return 0;
    }

#if defined(HAVE_SYS_AUXV_H) && defined(HAVE_GETAUXVAL)
  errno = 0;
  auxv.a_val = getauxval (AT_HWCAP);
  if (errno == 0)
    {
      stored_hwcap |= auxv.a_val;
      hwcap_initialized = 1;
    }

  if (hwcap_initialized && stored_hwcap)
    {
      *hwcap = stored_hwcap;
      return 0;
    }
#endif

  f = fopen("/proc/self/auxv", "r");
  if (!f)
    {
      *hwcap = stored_hwcap;
      return -1;
    }

  while (fread(&auxv, sizeof(auxv), 1, f) > 0)
    {
      if (auxv.a_type == AT_HWCAP)
        {
          stored_hwcap |= auxv.a_val;
          hwcap_initialized = 1;
        }
    }

  if (hwcap_initialized)
    err = 0;

  fclose(f);
  *hwcap = stored_hwcap;
  return err;
}

static unsigned int
detect_riscv_at_hwcap(void)
{
  unsigned int hwcap;
  unsigned int features = 0;
  unsigned int i;

  if (get_hwcap(&hwcap) < 0)
    return features;

  for (i = 0; i < DIM(hwcap_features); i++)
    {
      unsigned int hwcap_flag = hwcap_features[i].hwcap_flag;
      if ((hwcap & hwcap_flag) == hwcap_flag)
        features |= hwcap_features[i].hwf_flag;
    }

  return features;
}

#endif /* HAS_SYS_AT_HWCAP */


#undef HAS_SYS_HWPROBE
#if defined(__linux__) && defined(HAVE_SYSCALL)
#define HAS_SYS_HWPROBE 1

#ifndef __NR_riscv_hwprobe
#define __NR_riscv_hwprobe 258
#endif

#define HWF_RISCV_HWPROBE_KEY_BASE_BEHAVIOR 3
#define HWF_RISCV_HWPROBE_BASE_BEHAVIOR_IMA (1U << 0)

#define HWF_RISCV_HWPROBE_KEY_IMA_EXT_0     4
#define HWF_RISCV_HWPROBE_IMA_FD            (1U << 0)
#define HWF_RISCV_HWPROBE_IMA_C             (1U << 1)
#define HWF_RISCV_HWPROBE_IMA_V             (1U << 2)
#define HWF_RISCV_HWPROBE_EXT_ZBA           (1U << 3)
#define HWF_RISCV_HWPROBE_EXT_ZBB           (1U << 4)
#define HWF_RISCV_HWPROBE_EXT_ZBS           (1U << 5)
#define HWF_RISCV_HWPROBE_EXT_ZBC           (1U << 7)
#define HWF_RISCV_HWPROBE_EXT_ZVKB          (1U << 19)
#define HWF_RISCV_HWPROBE_EXT_ZVKG          (1U << 20)
#define HWF_RISCV_HWPROBE_EXT_ZVKNED        (1U << 21)
#define HWF_RISCV_HWPROBE_EXT_ZVKNHA        (1U << 22)
#define HWF_RISCV_HWPROBE_EXT_ZVKNHB        (1U << 23)
#define HWF_RISCV_HWPROBE_EXT_ZICOND        (U64_C(1) << 35)

#define HWF_RISCV_HWPROBE_IMA_FDC (HWF_RISCV_HWPROBE_IMA_FD \
				   | HWF_RISCV_HWPROBE_IMA_C)

struct hwf_riscv_hwprobe_s {
  u64 key;
  u64 value;
};

struct hwprobe_feature_map_s {
  unsigned int ima_ext_0_flag;
  unsigned int hwf_flag;
};

static const struct hwprobe_feature_map_s hwprobe_features[] =
  {
    { HWF_RISCV_HWPROBE_IMA_FDC,     HWF_RISCV_IMAFDC },
    { HWF_RISCV_HWPROBE_IMA_V,       HWF_RISCV_V },
    { HWF_RISCV_HWPROBE_EXT_ZBB,     HWF_RISCV_ZBB },
    { HWF_RISCV_HWPROBE_EXT_ZBC,     HWF_RISCV_ZBC },
    { HWF_RISCV_HWPROBE_EXT_ZBA
      | HWF_RISCV_HWPROBE_EXT_ZBB
      | HWF_RISCV_HWPROBE_EXT_ZBS,   HWF_RISCV_B },
    { HWF_RISCV_HWPROBE_EXT_ZVKB,    HWF_RISCV_ZVKB },
    { HWF_RISCV_HWPROBE_EXT_ZVKG,    HWF_RISCV_ZVKG },
    { HWF_RISCV_HWPROBE_EXT_ZVKNED,  HWF_RISCV_ZVKNED },
    { HWF_RISCV_HWPROBE_EXT_ZVKNHA,  HWF_RISCV_ZVKNHA },
    { HWF_RISCV_HWPROBE_EXT_ZVKNHB,  HWF_RISCV_ZVKNHB },
  };

static int
hwf_riscv_hwprobe(struct hwf_riscv_hwprobe_s *pairs, size_t pair_count,
	      size_t cpu_count, unsigned long *cpus, unsigned int flags)
{
  return syscall(__NR_riscv_hwprobe, pairs, pair_count, cpu_count, cpus, flags);
}

static unsigned int
detect_riscv_hwprobe(void)
{
  const int base_behavior_idx = 0;
  const int ima_ext_0_idx = base_behavior_idx + 1;
  struct hwf_riscv_hwprobe_s reqs[ima_ext_0_idx + 1];
  unsigned int features = 0;
  unsigned int i;
  int ret;

  memset(reqs, 0, sizeof(reqs));
  reqs[base_behavior_idx].key = HWF_RISCV_HWPROBE_KEY_BASE_BEHAVIOR;
  reqs[ima_ext_0_idx].key = HWF_RISCV_HWPROBE_KEY_IMA_EXT_0;

  ret = hwf_riscv_hwprobe(reqs, DIM(reqs), 0, NULL, 0);
  if (ret < 0)
    return 0;

  for (i = 0; i < DIM(hwprobe_features); i++)
    {
      unsigned int ima_ext_0_flag = hwprobe_features[i].ima_ext_0_flag;
      if ((reqs[base_behavior_idx].value & HWF_RISCV_HWPROBE_BASE_BEHAVIOR_IMA)
	  && (reqs[ima_ext_0_idx].value & ima_ext_0_flag) == ima_ext_0_flag)
        features |= hwprobe_features[i].hwf_flag;
    }

  return features;
}

#endif /* HAS_SYS_HWPROBE */


static unsigned int
detect_riscv_hwf_by_toolchain (void)
{
  unsigned int features = 0;

  /* Detect CPU features required by toolchain. */

#if defined(__riscv_i) && __riscv_i >= 1000000 && \
    defined(__riscv_m) && __riscv_m >= 1000000 && \
    defined(__riscv_a) && __riscv_a >= 1000000 && \
    defined(__riscv_f) && __riscv_f >= 1000000 && \
    defined(__riscv_d) && __riscv_d >= 1000000 && \
    defined(__riscv_c) && __riscv_c >= 1000000
  features |= HWF_RISCV_IMAFDC;
#endif

#if defined(__riscv_zbb) && __riscv_zbb >= 1000000 && \
    defined(HAVE_GCC_INLINE_ASM_RISCV)
  {
    unsigned int tmp = 0;

    /* Early test for Zbb instructions to detect faulty toolchain
     * configuration. */
    asm volatile (".option push;\n\t"
		  ".option arch, +zbb;\n\t"
		  "cpop %0, %1;\n\t"
		  ".option pop;\n\t"
		  : "=r" (tmp)
		  : "r" (321));

    features |= HWF_RISCV_ZBB;
  }
#endif

#if defined(__riscv_zba) && __riscv_zba >= 1000000 && \
    defined(__riscv_zbb) && __riscv_zbb >= 1000000 && \
    defined(__riscv_zbs) && __riscv_zbs >= 1000000 && \
    defined(HAVE_GCC_INLINE_ASM_RISCV)
  {
    unsigned int tmp = 0;

    /* Early test for Zba instructions to detect faulty toolchain
     * configuration. */
    asm volatile (".option push;\n\t"
		  ".option arch, +zba;\n\t"
		  "sh2add %0, %1, %2;\n\t"
		  ".option pop;\n\t"
		  : "=r" (tmp)
		  : "r" (321), "r" (123));

    /* Early test for Zbb instructions to detect faulty toolchain
     * configuration. */
    asm volatile (".option push;\n\t"
		  ".option arch, +zbb;\n\t"
		  "cpop %0, %1;\n\t"
		  ".option pop;\n\t"
		  : "=r" (tmp)
		  : "r" (321));

    /* Early test for Zbs instructions to detect faulty toolchain
     * configuration. */
    asm volatile (".option push;\n\t"
		  ".option arch, +zbs;\n\t"
		  "bclr %0, %1, %2;\n\t"
		  ".option pop;\n\t"
		  : "=r" (tmp)
		  : "r" (321), "r" (15));

    features |= HWF_RISCV_B;
  }
#endif

#if defined(__riscv_zbc) && __riscv_zbc >= 1000000 && \
    defined(HAVE_GCC_INLINE_ASM_RISCV)
  {
    unsigned int tmp = 0;

    /* Early test for Zbc instructions to detect faulty toolchain
     * configuration. */
    asm volatile (".option push;\n\t"
		  ".option arch, +zbc;\n\t"
		  "clmulr %0, %1, %2;\n\t"
		  ".option pop;\n\t"
		  : "=r" (tmp)
		  : "r" (123), "r" (321));

    features |= HWF_RISCV_ZBC;
  }
#endif

#if defined(__riscv_v) && __riscv_v >= 1000000 && \
    defined(HAVE_GCC_INLINE_ASM_RISCV_V)
  {
    unsigned int tmp = 0;

    /* Early test for RVV instructions to detect faulty toolchain
     * configuration. */
    asm volatile (".option push;\n\t"
		  ".option arch, +v;\n\t"
		  "vsetvli %0, %1, e8, m1, ta, ma;\n\t"
		  "vxor.vv v1, v1, v1;\n\t"
		  ".option pop;\n\t"
		  : "=r" (tmp)
		  : "r" (~0)
		  : "vl", "vtype", "v1");

    features |= HWF_RISCV_V;
  }
#endif

  return features;
}

unsigned int
_gcry_hwf_detect_riscv (void)
{
  unsigned int features = 0;

#if defined (HAS_SYS_AT_HWCAP)
  features |= detect_riscv_at_hwcap ();
#endif

#if defined (HAS_SYS_HWPROBE)
  features |= detect_riscv_hwprobe ();
#endif

  features |= detect_riscv_hwf_by_toolchain ();

  /* Require VLEN >= 128-bit for "riscv-v" HWF. */
  if (features & HWF_RISCV_V)
    {
      unsigned int vlmax = 0;

#if defined(HAVE_GCC_INLINE_ASM_RISCV_V)
      asm volatile (".option push;\n\t"
		    ".option arch, +v;\n\t"
		    "vsetvli %0, %1, e8, m1, ta, ma;\n\t"
		    ".option pop;\n\t"
		    : "=r" (vlmax)
		    : "r" (~0)
		    : "vl", "vtype");
#endif

      if (vlmax < 16)
	{
	  features &= ~HWF_RISCV_V;
	}
    }

  return features;
}
