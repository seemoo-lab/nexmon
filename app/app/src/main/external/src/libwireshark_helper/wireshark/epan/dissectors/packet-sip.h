/* packet-sip.h
 *
 * $Id: packet-sip.h 27342 2009-02-01 20:52:38Z etxrab $
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __PACKET_SIP_H__
#define __PACKET_SIP_H__

#include <epan/packet.h>

typedef struct _sip_info_value_t
{
    gchar	*request_method;
    guint	 response_code;
	guchar	resend;
	guint32 setup_time;
    /* added for VoIP calls analysis, see gtk/voip_calls.c*/
    gchar   *tap_call_id;
    gchar   *tap_from_addr;
    gchar   *tap_to_addr;
    guint32 tap_cseq_number;
    gchar   *reason_phrase;
} sip_info_value_t;

extern void dfilter_store_sip_from_addr(tvbuff_t *tvb,proto_tree *tree,guint parameter_offset, 
					  guint parameter_len);

#endif
