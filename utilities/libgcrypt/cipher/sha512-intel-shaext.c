/* sha512-intel-shaext.c - SHA512 accelerated with Intel SHA512 extension.
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

#include <config.h>

#include "types.h"

#if defined(HAVE_GCC_INLINE_ASM_SHA512) && \
    defined(USE_SHA512) && defined(ENABLE_SHAEXT_SUPPORT)

#if _GCRY_GCC_VERSION >= 40400 /* 4.4 */
/* Prevent compiler from issuing SSE instructions between asm blocks. */
#  pragma GCC target("no-sse")
#endif
#if __clang__
#  pragma clang attribute push (__attribute__((target("no-sse"))), apply_to = function)
#endif

#define NO_INSTRUMENT_FUNCTION __attribute__((no_instrument_function))

#define ASM_FUNC_ATTR NO_INSTRUMENT_FUNCTION

/* Two macros to be called prior and after the use of SHA512
  instructions.  There should be no external function calls between
  the use of these macros.  There purpose is to make sure that the
  SSE regsiters are cleared and won't reveal any information about
  the key or the data.  */
#ifdef __WIN64__
/* XMM6-XMM15 are callee-saved registers on WIN64. */
# define shaext_prepare_variable char win64tmp[2*16]
# define shaext_prepare_variable_size sizeof(win64tmp)
# define shaext_prepare()                                               \
   do { asm volatile ("movdqu %%xmm6, 0*16(%0)\n"                       \
		      "movdqu %%xmm7, 1*16(%0)\n"                       \
		      :                                                 \
		      : "r" (&win64tmp[0])                              \
		      : "memory");                                      \
  } while (0)
# define shaext_cleanup(tmp0,tmp1)                                      \
   do { asm volatile ("vpxor %%ymm0, %%ymm0, %%ymm0\n\t"                \
		      "vmovdqu %%ymm0, (%0)\n\t"                        \
		      "vmovdqu %%ymm0, (%1)\n\t"                        \
		      "vzeroall\n\t"                                    \
		      "movdqu 0*16(%2), %%xmm6\n"                       \
		      "movdqu 1*16(%2), %%xmm7\n"                       \
		      :                                                 \
		      : "r" (tmp0), "r" (tmp1), "r" (&win64tmp[0])      \
		      : "memory");                                      \
  } while (0)
#else
# define shaext_prepare_variable
# define shaext_prepare_variable_size 0
# define shaext_prepare() do { } while (0)
# define shaext_cleanup(tmp0,tmp1)                                      \
   do { asm volatile ("vpxor %%ymm0, %%ymm0, %%ymm0\n\t"                \
		      "vmovdqu %%ymm0, (%0)\n\t"                        \
		      "vmovdqu %%ymm0, (%1)\n\t"                        \
		      "vzeroall\n\t"                                    \
		      :                                                 \
		      : "r" (tmp0), "r" (tmp1)                          \
		      : "memory");                                      \
  } while (0)
#endif

/*
 * Transform nblks*128 bytes (nblks*16 64-bit words) at DATA.
 */
unsigned int ASM_FUNC_ATTR
_gcry_sha512_transform_intel_shaext(u64 state[8], const unsigned char *data,
				    size_t nblks, const u64 k[80])
{
  static const unsigned char bshuf_mask[16] __attribute__ ((aligned (16))) =
    { 7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8 };
  char save_buf[2 * 32 + 31];
  char *abef_save;
  char *cdgh_save;
  shaext_prepare_variable;

  if (nblks == 0)
    return 0;

  shaext_prepare ();

  asm volatile ("" : "=r" (abef_save) : "0" (save_buf) : "memory");
  abef_save = abef_save + (-(uintptr_t)abef_save & 31);
  cdgh_save = abef_save + 32;

  /* Load state.  State is {a,b,c,d} and {e,f,g,h} in memory, repack to
     ABEF (YMM elem order {f,e,b,a}) in YMM1 and CDGH ({h,g,d,c}) in YMM2. */
  asm volatile ("vpshufd $0x4e, 0*32(%[state]), %%ymm3\n\t" /* {b,a,d,c} */
		"vpshufd $0x4e, 1*32(%[state]), %%ymm4\n\t" /* {f,e,h,g} */
		"vperm2i128 $0x02, %%ymm4, %%ymm3, %%ymm1\n\t" /* {f,e,b,a} */
		"vperm2i128 $0x13, %%ymm4, %%ymm3, %%ymm2\n\t" /* {h,g,d,c} */
		:
		: [state] "r" (state)
		: "memory" );

  /* Load message */
  asm volatile ("vbroadcasti128 %[mask], %%ymm7\n\t"
		"vmovdqu 0*32(%[data]), %%ymm3\n\t"
		"vmovdqu 1*32(%[data]), %%ymm4\n\t"
		"vmovdqu 2*32(%[data]), %%ymm5\n\t"
		"vmovdqu 3*32(%[data]), %%ymm6\n\t"
		"vpshufb %%ymm7, %%ymm3, %%ymm3\n\t"
		"vpshufb %%ymm7, %%ymm4, %%ymm4\n\t"
		"vpshufb %%ymm7, %%ymm5, %%ymm5\n\t"
		"vpshufb %%ymm7, %%ymm6, %%ymm6\n\t"
		:
		: [data] "r" (data), [mask] "m" (*bshuf_mask)
		: "memory" );
  data += 128;

  do
    {
      /* Save state */
      asm volatile ("vmovdqa %%ymm1, (%[abef_save])\n\t"
		    "vmovdqa %%ymm2, (%[cdgh_save])\n\t"
		    :
		    : [abef_save] "r" (abef_save), [cdgh_save] "r" (cdgh_save)
		    : "memory" );


      /* Rounds 0..3 */
      asm volatile ("vpaddq 32*0(%[k]), %%ymm3, %%ymm0\n\t"
		    "vsha512rnds2 %%xmm0, %%ymm1, %%ymm2\n\t"
		    "vextracti128 $1, %%ymm0, %%xmm0\n\t"
		    "vsha512rnds2 %%xmm0, %%ymm2, %%ymm1\n\t"
		    :
		    : [k] "r" (k)
		    : "memory" );

      /* Rounds 4..7 */
      asm volatile ("vpaddq 32*1(%[k]), %%ymm4, %%ymm0\n\t"
		    "vsha512rnds2 %%xmm0, %%ymm1, %%ymm2\n\t"
		    "vextracti128 $1, %%ymm0, %%xmm0\n\t"
		    "vsha512rnds2 %%xmm0, %%ymm2, %%ymm1\n\t"
		    "vsha512msg1 %%xmm4, %%ymm3\n\t"
		    :
		    : [k] "r" (k)
		    : "memory" );

      /* Rounds 8..11 */
      asm volatile ("vpaddq 32*2(%[k]), %%ymm5, %%ymm0\n\t"
		    "vsha512rnds2 %%xmm0, %%ymm1, %%ymm2\n\t"
		    "vextracti128 $1, %%ymm0, %%xmm0\n\t"
		    "vsha512rnds2 %%xmm0, %%ymm2, %%ymm1\n\t"
		    "vsha512msg1 %%xmm5, %%ymm4\n\t"
		    :
		    : [k] "r" (k)
		    : "memory" );

#define ROUND(gr, MSG0, MSG1, MSG2, MSG3) \
      asm volatile ("vpaddq 32*" #gr "(%[k]), %%ymm" #MSG0 ", %%ymm0\n\t" \
		    "vsha512rnds2 %%xmm0, %%ymm1, %%ymm2\n\t" \
		    "vperm2i128 $0x21, %%ymm" #MSG0 ", %%ymm" #MSG3 ", %%ymm7\n\t" \
		    "vpalignr $8, %%ymm" #MSG3 ", %%ymm7, %%ymm7\n\t" \
		    "vpaddq %%ymm7, %%ymm" #MSG1 ", %%ymm" #MSG1 "\n\t" \
		    "vsha512msg2 %%ymm" #MSG0 ", %%ymm" #MSG1 "\n\t" \
		    "vextracti128 $1, %%ymm0, %%xmm0\n\t" \
		    "vsha512rnds2 %%xmm0, %%ymm2, %%ymm1\n\t" \
		    "vsha512msg1 %%xmm" #MSG0 ", %%ymm" #MSG3 "\n\t" \
		    : \
		    : [k] "r" (k) \
		    : "memory" )

      /* Rounds 12..15 to 64..67 (message schedule for W[16..79]). */
      ROUND(3, 6, 3, 4, 5);
      ROUND(4, 3, 4, 5, 6);
      ROUND(5, 4, 5, 6, 3);
      ROUND(6, 5, 6, 3, 4);
      ROUND(7, 6, 3, 4, 5);
      ROUND(8, 3, 4, 5, 6);
      ROUND(9, 4, 5, 6, 3);
      ROUND(10, 5, 6, 3, 4);
      ROUND(11, 6, 3, 4, 5);
      ROUND(12, 3, 4, 5, 6);
      ROUND(13, 4, 5, 6, 3);
      ROUND(14, 5, 6, 3, 4);
      ROUND(15, 6, 3, 4, 5);
      ROUND(16, 3, 4, 5, 6);

      if (--nblks == 0)
	break;

/* Final two message groups: finalize MSG1 but no further message schedule. */
#define ROUND_FINAL(gr, MSG0, MSG1, MSG3) \
      asm volatile ("vpaddq 32*" #gr "(%[k]), %%ymm" #MSG0 ", %%ymm0\n\t" \
		    "vsha512rnds2 %%xmm0, %%ymm1, %%ymm2\n\t" \
		    "vperm2i128 $0x21, %%ymm" #MSG0 ", %%ymm" #MSG3 ", %%ymm7\n\t" \
		    "vpalignr $8, %%ymm" #MSG3 ", %%ymm7, %%ymm7\n\t" \
		    "vpaddq %%ymm7, %%ymm" #MSG1 ", %%ymm" #MSG1 "\n\t" \
		    "vsha512msg2 %%ymm" #MSG0 ", %%ymm" #MSG1 "\n\t" \
		    "vextracti128 $1, %%ymm0, %%xmm0\n\t" \
		    "vsha512rnds2 %%xmm0, %%ymm2, %%ymm1\n\t" \
		    : \
		    : [k] "r" (k) \
		    : "memory" )

      /* Rounds 68..71 */
      ROUND_FINAL(17, 4, 5, 3);

      asm volatile ("vmovdqu 0*32(%[data]), %%ymm3\n\t"
		    :
		    : [data] "r" (data)
		    : "memory" );

      /* Rounds 72..75 */
      ROUND_FINAL(18, 5, 6, 4);

      asm volatile ("vbroadcasti128 %[mask], %%ymm7\n\t" /* Reload mask */
		    "vmovdqu 1*32(%[data]), %%ymm4\n\t"
		    "vpshufb %%ymm7, %%ymm3, %%ymm3\n\t"
		    :
		    : [data] "r" (data), [mask] "m" (*bshuf_mask)
		    : "memory" );

      /* Rounds 76..79 */
      asm volatile ("vpaddq 32*19(%[k]), %%ymm6, %%ymm0\n\t"
		      "vmovdqu 2*32(%[data]), %%ymm5\n\t"
		    "vsha512rnds2 %%xmm0, %%ymm1, %%ymm2\n\t"
                      "vmovdqu 3*32(%[data]), %%ymm6\n\t"
		      "vpshufb %%ymm7, %%ymm4, %%ymm4\n\t"
		    "vextracti128 $1, %%ymm0, %%xmm0\n\t"
		      "vpshufb %%ymm7, %%ymm5, %%ymm5\n\t"
		    "vsha512rnds2 %%xmm0, %%ymm2, %%ymm1\n\t"
		      "vpshufb %%ymm7, %%ymm6, %%ymm6\n\t"
		    :
		    : [k] "r" (k), [data] "r" (data)
		    : "memory" );

      data += 128;

      /* Merge states */
      asm volatile ("vpaddq (%[abef_save]), %%ymm1, %%ymm1\n\t"
		    "vpaddq (%[cdgh_save]), %%ymm2, %%ymm2\n\t"
		    :
		    : [abef_save] "r" (abef_save), [cdgh_save] "r" (cdgh_save)
		    : "memory" );
    }
  while (1);

  /* Rounds 68..71 */
  ROUND_FINAL(17, 4, 5, 3);
  /* Rounds 72..75 */
  ROUND_FINAL(18, 5, 6, 4);

  /* Rounds 76..79 */
  asm volatile ("vpaddq 32*19(%[k]), %%ymm6, %%ymm0\n\t"
		"vsha512rnds2 %%xmm0, %%ymm1, %%ymm2\n\t"
		"vextracti128 $1, %%ymm0, %%xmm0\n\t"
		"vsha512rnds2 %%xmm0, %%ymm2, %%ymm1\n\t"
		:
		: [k] "r" (k)
		: "memory" );

  /* Merge states */
  asm volatile ("vpaddq (%[abef_save]), %%ymm1, %%ymm1\n\t"
		"vpaddq (%[cdgh_save]), %%ymm2, %%ymm2\n\t"
		:
		: [abef_save] "r" (abef_save), [cdgh_save] "r" (cdgh_save)
		: "memory" );

  /* Store state.  ABEF=YMM1 ({f,e,b,a}), CDGH=YMM2 ({h,g,d,c}). */
  asm volatile ("vperm2i128 $0x31, %%ymm2, %%ymm1, %%ymm3\n\t" /* {b,a,d,c} */
		"vperm2i128 $0x20, %%ymm2, %%ymm1, %%ymm4\n\t" /* {f,e,h,g} */
		"vpshufd $0x4e, %%ymm3, %%ymm3\n\t"            /* {a,b,c,d} */
		"vpshufd $0x4e, %%ymm4, %%ymm4\n\t"            /* {e,f,g,h} */
		"vmovdqu %%ymm3, 0*32(%[state])\n\t"
		"vmovdqu %%ymm4, 1*32(%[state])\n\t"
		:
		: [state] "r" (state)
		: "memory" );

  shaext_cleanup (abef_save, cdgh_save);
  return 0;
}

#if __clang__
#  pragma clang attribute pop
#endif

#endif /* HAVE_GCC_INLINE_ASM_SHA512 */
