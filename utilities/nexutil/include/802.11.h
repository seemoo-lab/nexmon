
/*
 * Fundamental types and constants relating to 802.11
 *
 * Copyright (C) 2023, Broadcom. All Rights Reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#ifndef _802_11_H_
#define _802_11_H_

#ifndef _TYPEDEFS_H_
#include <typedefs.h>
#endif

/* IEEE P802.11-REVme/D1.3 Table 9-313 VHT Operation Information subfields */
typedef enum vht_op_chan_width {
    VHT_OP_CHAN_WIDTH_20_40 = 0,
    VHT_OP_CHAN_WIDTH_80    = 1,
    VHT_OP_CHAN_WIDTH_160   = 2, /* deprecated - IEEE 802.11 REVmc D8.0 Table 11-     25 */
    VHT_OP_CHAN_WIDTH_80_80 = 3  /* deprecated - IEEE 802.11 REVmc D8.0 Table 11-     25 */
} vht_op_chan_width_t;

#endif /* _802_11_H_ */
