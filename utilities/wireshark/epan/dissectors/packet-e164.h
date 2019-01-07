/* packet-e164.h
 * E164 tables
 * Copyright 2004, Anders Broman <anders.broman@ericsson.com>
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
 */

#ifndef __PACKET_E164_H__
#define __PACKET_E164_H__

#include <epan/value_string.h>
#include "ws_symbol_export.h"

extern const value_string E164_country_code_value[];
extern const value_string E164_International_Networks_vals[];

#define E164_NA_INTERNATIONAL_NUMBER 4

typedef enum {
	NONE,
	CALLING_PARTY_NUMBER,
	CALLED_PARTY_NUMBER
} e164_number_type_t;

typedef struct {
	e164_number_type_t e164_number_type;
	guint nature_of_address;
	const char *E164_number_str;	/* E164 number string */
	guint E164_number_length;	/* Length of the E164_number string */
} e164_info_t;

typedef enum {
	E164_ENC_BINARY,
	E164_ENC_BCD,
	E164_ENC_UTF8
} e164_encoding_t;

extern void dissect_e164_number(tvbuff_t *tvb, proto_tree *tree, int offset, int length, e164_info_t e164_info);
WS_DLL_PUBLIC void dissect_e164_cc(tvbuff_t *tvb, proto_tree *tree, int offset, e164_encoding_t encoding);
WS_DLL_PUBLIC const gchar * dissect_e164_msisdn(tvbuff_t *tvb, proto_tree *tree, int offset, int length, e164_encoding_t encoding);
#endif
