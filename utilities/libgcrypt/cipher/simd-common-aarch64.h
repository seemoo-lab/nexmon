/* simd-common-aarch64.h  -  Common macros for AArch64 SIMD code
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

#ifndef GCRY_SIMD_COMMON_AARCH64_H
#define GCRY_SIMD_COMMON_AARCH64_H

#include <config.h>

#define memory_barrier_with_vec(a) __asm__("" : "+w"(a) :: "memory")

#define clear_vec_regs() __asm__ volatile("movi v0.16b, #0;\n" \
					  "movi v1.16b, #0;\n" \
					  "movi v2.16b, #0;\n" \
					  "movi v3.16b, #0;\n" \
					  "movi v4.16b, #0;\n" \
					  "movi v5.16b, #0;\n" \
					  "movi v6.16b, #0;\n" \
					  "movi v7.16b, #0;\n" \
					  /* v8-v15 are ABI callee saved and \
					   * get cleared by function \
					   * epilog when used. */ \
					  "movi v16.16b, #0;\n" \
					  "movi v17.16b, #0;\n" \
					  "movi v18.16b, #0;\n" \
					  "movi v19.16b, #0;\n" \
					  "movi v20.16b, #0;\n" \
					  "movi v21.16b, #0;\n" \
					  "movi v22.16b, #0;\n" \
					  "movi v23.16b, #0;\n" \
					  "movi v24.16b, #0;\n" \
					  "movi v25.16b, #0;\n" \
					  "movi v26.16b, #0;\n" \
					  "movi v27.16b, #0;\n" \
					  "movi v28.16b, #0;\n" \
					  "movi v29.16b, #0;\n" \
					  "movi v30.16b, #0;\n" \
					  "movi v31.16b, #0;\n" \
					  ::: "memory", "v0", "v1", "v2", \
					      "v3", "v4", "v5", "v6", "v7", \
					      "v16", "v17", "v18", "v19", \
					      "v20", "v21", "v22", "v23", \
					      "v24", "v25", "v26", "v27", \
					      "v28", "v29", "v30", "v31")

#endif /* GCRY_SIMD_COMMON_AARCH64_H */
