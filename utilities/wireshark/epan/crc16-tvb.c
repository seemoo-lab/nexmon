/* crc16-tvb.c
 * CRC-16 tvb routines
 *
 * 2004 Richard van der Hoff <richardv@mxtelecom.com>
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * References:
 *  "A Painless Guide to CRC Error Detection Algorithms", Ross Williams
 *      http://www.repairfaq.org/filipg/LINK/F_crc_v3.html
 *
 *  ITU-T Recommendation V.42 (2002), "Error-Correcting Procedures for
 *      DCEs using asynchronous-to-synchronous conversion", Para. 8.1.1.6.1
 */

#include "config.h"

#include <glib.h>
#include <epan/tvbuff.h>
#include <wsutil/crc16.h>
#include <epan/crc16-tvb.h>
#include <wsutil/crc16-plain.h>


guint16 crc16_ccitt_tvb(tvbuff_t *tvb, guint len)
{
    const guint8 *buf;

    tvb_ensure_bytes_exist(tvb, 0, len);  /* len == -1 not allowed */
    buf = tvb_get_ptr(tvb, 0, len);

    return crc16_ccitt(buf, len);
}

guint16 crc16_x25_ccitt_tvb(tvbuff_t *tvb, guint len)
{
    const guint8 *buf;

    tvb_ensure_bytes_exist(tvb, 0, len);  /* len == -1 not allowed */
    buf = tvb_get_ptr(tvb, 0, len);

    return crc16_x25_ccitt_seed(buf, len, 0xFFFF);
}

guint16 crc16_r3_ccitt_tvb(tvbuff_t *tvb, int offset, guint len)
{
    const guint8 *buf;

    tvb_ensure_bytes_exist(tvb, offset, len);  /* len == -1 not allowed */
    buf = tvb_get_ptr(tvb, offset, len);

    return crc16_x25_ccitt_seed(buf, len, 0);
}

guint16 crc16_ccitt_tvb_offset(tvbuff_t *tvb, guint offset, guint len)
{
    const guint8 *buf;

    tvb_ensure_bytes_exist(tvb, offset, len);  /* len == -1 not allowed */
    buf = tvb_get_ptr(tvb, offset, len);

    return crc16_ccitt(buf, len);
}

guint16 crc16_ccitt_tvb_seed(tvbuff_t *tvb, guint len, guint16 seed)
{
    const guint8 *buf;

    tvb_ensure_bytes_exist(tvb, 0, len);  /* len == -1 not allowed */
    buf = tvb_get_ptr(tvb, 0, len);

    return crc16_ccitt_seed(buf, len, seed);
}

guint16 crc16_ccitt_tvb_offset_seed(tvbuff_t *tvb, guint offset, guint len, guint16 seed)
{
    const guint8 *buf;

    tvb_ensure_bytes_exist(tvb, offset, len);  /* len == -1 not allowed */
    buf = tvb_get_ptr(tvb, offset, len);

    return crc16_ccitt_seed(buf, len, seed);
}

guint16 crc16_iso14443a_tvb_offset(tvbuff_t *tvb, guint offset, guint len)
{
    const guint8 *buf;

    tvb_ensure_bytes_exist(tvb, offset, len);  /* len == -1 not allowed */
    buf = tvb_get_ptr(tvb, offset, len);

    return crc16_iso14443a(buf, len);
}

guint16 crc16_plain_tvb_offset(tvbuff_t *tvb, guint offset, guint len)
{
    guint16 crc = crc16_plain_init();
    const guint8 *buf;

    tvb_ensure_bytes_exist(tvb, offset, len);  /* len == -1 not allowed */
    buf = tvb_get_ptr(tvb, offset, len);

    crc = crc16_plain_update(crc, buf, len);

    return crc16_plain_finalize(crc);
}

guint16 crc16_plain_tvb_offset_seed(tvbuff_t *tvb, guint offset, guint len, guint16 crc)
{
    const guint8 *buf;

    tvb_ensure_bytes_exist(tvb, offset, len);  /* len == -1 not allowed */
    buf = tvb_get_ptr(tvb, offset, len);

    crc = crc16_plain_update(crc, buf, len);

    return crc16_plain_finalize(crc);
}

guint16 crc16_0x9949_tvb_offset_seed(tvbuff_t *tvb, guint offset, guint len, guint16 seed)
{
    const guint8 *buf;

    tvb_ensure_bytes_exist(tvb, offset, len);  /* len == -1 not allowed */
    buf = tvb_get_ptr(tvb, offset, len);

    return crc16_0x9949_seed(buf, len, seed);
}

guint16 crc16_0x3D65_tvb_offset_seed(tvbuff_t *tvb, guint offset, guint len, guint16 seed)
{
    const guint8 *buf;

    tvb_ensure_bytes_exist(tvb, offset, len);  /* len == -1 not allowed */
    buf = tvb_get_ptr(tvb, offset, len);

    return crc16_0x3D65_seed(buf, len, seed);
}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
