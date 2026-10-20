/* simd-common-ppc.h  -  Common macros for PowerPC SIMD code
 *
 * Copyright (C) 2024 Jussi Kivilinna <jussi.kivilinna@iki.fi>
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

#ifndef GCRY_SIMD_COMMON_PPC_H
#define GCRY_SIMD_COMMON_PPC_H

#include <config.h>

#define memory_barrier_with_vec(a) __asm__("" : "+wa"(a) :: "memory")

#define clear_vec_regs() __asm__ volatile("xxlxor 0, 0, 0\n" \
				          "xxlxor 1, 1, 1\n" \
				          "xxlxor 2, 2, 2\n" \
				          "xxlxor 3, 3, 3\n" \
				          "xxlxor 4, 4, 4\n" \
				          "xxlxor 5, 5, 5\n" \
				          "xxlxor 6, 6, 6\n" \
				          "xxlxor 7, 7, 7\n" \
				          "xxlxor 8, 8, 8\n" \
				          "xxlxor 9, 9, 9\n" \
				          "xxlxor 10, 10, 10\n" \
				          "xxlxor 11, 11, 11\n" \
				          "xxlxor 12, 12, 12\n" \
				          "xxlxor 13, 13, 13\n" \
				          "xxlxor 32, 32, 32\n" \
				          "xxlxor 33, 33, 33\n" \
				          "xxlxor 34, 34, 34\n" \
				          "xxlxor 35, 35, 35\n" \
				          "xxlxor 36, 36, 36\n" \
				          "xxlxor 37, 37, 37\n" \
				          "xxlxor 38, 38, 38\n" \
				          "xxlxor 39, 39, 39\n" \
				          "xxlxor 40, 40, 40\n" \
				          "xxlxor 41, 41, 41\n" \
				          "xxlxor 42, 42, 42\n" \
				          "xxlxor 43, 43, 43\n" \
				          "xxlxor 44, 44, 44\n" \
				          "xxlxor 45, 45, 45\n" \
				          "xxlxor 46, 46, 46\n" \
				          "xxlxor 47, 47, 47\n" \
				          "xxlxor 48, 48, 48\n" \
				          "xxlxor 49, 49, 49\n" \
				          "xxlxor 50, 50, 50\n" \
				          "xxlxor 51, 51, 51\n" \
					  ::: "vs0", "vs1", "vs2", "vs3", \
					      "vs4", "vs5", "vs6", "vs7", \
					      "vs8", "vs9", "vs10", "vs11", \
					      "vs12", "vs13", \
					      /* vs14-vs31 (f14-f31) are */ \
					      /* ABI callee saved. */ \
					      "vs32", "vs33", "vs34", "vs35", \
					      "vs36", "vs37", "vs38", "vs39", \
					      "vs40", "vs41", "vs42", "vs43", \
					      "vs44", "vs45", "vs46", "vs47", \
					      "vs48", "vs49", "vs50", "vs51", \
					      /* vs52-vs63 (v20-v31) are */ \
					      /* ABI callee saved. */ \
					      "memory")

#endif /* GCRY_SIMD_COMMON_PPC_H */
