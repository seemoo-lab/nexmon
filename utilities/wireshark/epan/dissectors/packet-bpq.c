/* packet-bpq.c
 *
 * Routines for Amateur Packet Radio protocol dissection
 * Copyright 2005,2006,2007,2008,2009,2010,2012 R.W. Stearn <richard@rns-stearn.demon.co.uk>
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

/*
 * This dissector is for:
 *   Ethernet encapsulated Amateur AX.25 (AX.25 over Ethernet)
 *
 * Information was drawn from:
 *   ?
 *
 * It uses Ether ID 0x08ff which is not not officially registered.
 *
 */

#include "config.h"

#include <epan/packet.h>
#include <epan/etypes.h>
#include <epan/capture_dissectors.h>

#include "packet-ax25.h"

#define STRLEN	80

#define BPQ_HEADER_SIZE	2 /* length of bpq_len */

void proto_register_bpq(void);
void proto_reg_handoff_bpq(void);

static dissector_handle_t bpq_handle;
static dissector_handle_t ax25_handle;

static int proto_bpq            = -1;
static int hf_bpq_len		= -1;

static gint ett_bpq = -1;

static int
dissect_bpq( tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree, void* data _U_ )
{
	proto_item *ti;
	proto_tree *bpq_tree;
	int	    offset;
	guint16	    bpq_len;
	tvbuff_t   *next_tvb;


	col_set_str( pinfo->cinfo, COL_PROTOCOL, "BPQ" );

	col_clear( pinfo->cinfo, COL_INFO );

	/* protocol offset for the BPQ header */
	offset = 0;

	bpq_len = tvb_get_letohs( tvb, offset );

	col_add_fstr( pinfo->cinfo, COL_INFO, "%u", bpq_len );

	if ( parent_tree )
		{
		/* protocol offset for the BPQ header */
		offset = 0;

		/* create display subtree for the protocol */
		ti = proto_tree_add_protocol_format( parent_tree, proto_bpq, tvb, offset, BPQ_HEADER_SIZE,
			"BPQ, Len: %u",
			bpq_len & 0xfff	/* XXX - lower 12 bits? */
			);

		bpq_tree = proto_item_add_subtree( ti, ett_bpq );

		proto_tree_add_item( bpq_tree, hf_bpq_len, tvb, offset, BPQ_HEADER_SIZE, ENC_LITTLE_ENDIAN );

	}

	offset += BPQ_HEADER_SIZE;

	/* XXX - use the length */
	next_tvb = tvb_new_subset_remaining( tvb, offset );
	call_dissector( ax25_handle, next_tvb, pinfo, parent_tree );
	return tvb_captured_length(tvb);
}

static gboolean
capture_bpq( const guchar *pd, int offset, int len, capture_packet_info_t *cpinfo, const union wtap_pseudo_header *pseudo_header)
{
	int l_offset;

	if ( ! BYTES_ARE_IN_FRAME( offset, len, BPQ_HEADER_SIZE ) )
		return FALSE;

	l_offset = offset;
	l_offset += BPQ_HEADER_SIZE; /* step over bpq header to point at the AX.25 packet*/
	return capture_ax25( pd, l_offset, len, cpinfo, pseudo_header );
}

void
proto_register_bpq(void)
{
	/* Setup list of header fields */
	static hf_register_info hf[] = {
		{ &hf_bpq_len,
			{ "BPQ len",			"bpq.len",
			FT_UINT16, BASE_DEC, NULL, 0x0,
			NULL, HFILL }
		},
	};

	/* Setup protocol subtree array */
	static gint *ett[] = {
		&ett_bpq,
	};

	/* Register the protocol name and description */
	proto_bpq = proto_register_protocol( "Amateur Radio BPQ", "BPQ", "bpq" );

	/* Register the dissector */
	bpq_handle = register_dissector("bpq", dissect_bpq, proto_bpq);

	/* Required function calls to register the header fields and subtrees used */
	proto_register_field_array( proto_bpq, hf, array_length( hf ) );
	proto_register_subtree_array( ett, array_length( ett ) );
}

void
proto_reg_handoff_bpq(void)
{
	dissector_add_uint("ethertype", ETHERTYPE_BPQ, bpq_handle);
	register_capture_dissector("ethertype", ETHERTYPE_BPQ, capture_bpq, proto_bpq);

	/* BPQ is only implemented for AX.25 */
	ax25_handle     = find_dissector_add_dependency( "ax25", proto_bpq );

}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: t
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 noexpandtab:
 * :indentSize=8:tabSize=8:noTabs=false:
 */
