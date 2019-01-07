/* in_cksum.h
 * Declaration of Internet checksum routine.
 *
 * Copyright (c) 1988, 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef __IN_CKSUM_H__
#define __IN_CKSUM_H__

#include "ws_symbol_export.h"

typedef struct {
	const guint8 *ptr;
	int	len;
} vec_t;

#define SET_CKSUM_VEC_PTR(vecelem, data, length) \
	G_STMT_START { \
		vecelem.ptr = (data); \
		vecelem.len = (length); \
	} G_STMT_END

#define SET_CKSUM_VEC_TVB(vecelem, tvb, offset, length) \
	G_STMT_START { \
		vecelem.len = (length); \
		vecelem.ptr = tvb_get_ptr((tvb), (offset), vecelem.len); \
	} G_STMT_END

WS_DLL_PUBLIC guint16 ip_checksum(const guint8 *ptr, int len);

WS_DLL_PUBLIC guint16 ip_checksum_tvb(tvbuff_t *tvb, int offset, int len);

WS_DLL_PUBLIC int in_cksum(const vec_t *vec, int veclen);

guint16 in_cksum_shouldbe(guint16 sum, guint16 computed_sum);

#endif /* __IN_CKSUM_H__ */
