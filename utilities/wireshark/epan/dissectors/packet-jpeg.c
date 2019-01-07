/* packet-jpeg.c
 *
 * Routines for RFC 2435 JPEG dissection
 *
 * Copyright 2006
 * Erwin Rol <erwin@erwinrol.com>
 * Copyright 2001,
 * Francisco Javier Cabello Torres, <fjcabello@vtools.es>
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

#define NEW_PROTO_TREE_API

#include "config.h"

#include <epan/packet.h>

#include <epan/rtp_pt.h>

#include "packet-ber.h"

void proto_register_jpeg(void);
void proto_reg_handoff_jpeg(void);

static dissector_handle_t jpeg_handle;

static header_field_info *hfi_jpeg = NULL;

#define JPEG_HFI_INIT HFI_INIT(proto_jpeg)

/* JPEG header fields */
static header_field_info hfi_rtp_jpeg_main_hdr JPEG_HFI_INIT = {
	"Main Header",
	"jpeg.main_hdr",
	FT_NONE, BASE_NONE, NULL, 0,
	NULL, HFILL
};

static header_field_info hfi_rtp_jpeg_main_hdr_ts JPEG_HFI_INIT = {
	"Type Specific",
	"jpeg.main_hdr.ts",
	FT_UINT8, BASE_DEC, NULL, 0,
	NULL, HFILL
};

static header_field_info hfi_rtp_jpeg_main_hdr_offs JPEG_HFI_INIT = {
	"Fragment Offset",
	"jpeg.main_hdr.offset",
	FT_UINT24, BASE_DEC, NULL, 0,
	NULL, HFILL
};

static header_field_info hfi_rtp_jpeg_main_hdr_type JPEG_HFI_INIT = {
	"Type",
	"jpeg.main_hdr.type",
	FT_UINT8, BASE_DEC, NULL, 0,
	NULL, HFILL
};

static header_field_info hfi_rtp_jpeg_main_hdr_q JPEG_HFI_INIT = {
	"Q",
	"jpeg.main_hdr.q",
	FT_UINT8, BASE_DEC, NULL, 0,
	NULL, HFILL
};

static header_field_info hfi_rtp_jpeg_main_hdr_width JPEG_HFI_INIT = {
	"Width",
	"jpeg.main_hdr.width",
	FT_UINT8, BASE_DEC, NULL, 0,
	NULL, HFILL
};

static header_field_info hfi_rtp_jpeg_main_hdr_height JPEG_HFI_INIT = {
	"Height",
	"jpeg.main_hdr.height",
	FT_UINT8, BASE_DEC, NULL, 0,
	NULL, HFILL
};

static header_field_info hfi_rtp_jpeg_restart_hdr JPEG_HFI_INIT = {
	"Restart Marker Header",
	"jpeg.restart_hdr",
	FT_NONE, BASE_NONE, NULL, 0,
	NULL, HFILL
};

static header_field_info hfi_rtp_jpeg_restart_hdr_interval JPEG_HFI_INIT = {
	"Restart Interval",
	"jpeg.restart_hdr.interval",
	FT_UINT16, BASE_DEC, NULL, 0,
	NULL, HFILL
};

static header_field_info hfi_rtp_jpeg_restart_hdr_f JPEG_HFI_INIT = {
	"F",
	"jpeg.restart_hdr.f",
	FT_UINT16, BASE_DEC, NULL, 0x8000,
	NULL, HFILL
};

static header_field_info hfi_rtp_jpeg_restart_hdr_l JPEG_HFI_INIT = {
	"L",
	"jpeg.restart_hdr.l",
	FT_UINT16, BASE_DEC, NULL, 0x4000,
	NULL, HFILL
};

static header_field_info hfi_rtp_jpeg_restart_hdr_count JPEG_HFI_INIT = {
	"Restart Count",
	"jpeg.restart_hdr.count",
	FT_UINT16, BASE_DEC, NULL, 0x3FFF,
	NULL, HFILL
};

static header_field_info hfi_rtp_jpeg_qtable_hdr JPEG_HFI_INIT = {
	"Quantization Table Header",
	"jpeg.qtable_hdr",
	FT_NONE, BASE_NONE, NULL, 0,
	NULL, HFILL
};

static header_field_info hfi_rtp_jpeg_qtable_hdr_mbz JPEG_HFI_INIT = {
	"MBZ",
	"jpeg.qtable_hdr.mbz",
	FT_UINT8, BASE_DEC, NULL, 0,
	NULL, HFILL
};

static header_field_info hfi_rtp_jpeg_qtable_hdr_prec JPEG_HFI_INIT = {
	"Precision",
	"jpeg.qtable_hdr.precision",
	FT_UINT8, BASE_DEC, NULL, 0,
	NULL, HFILL
};

static header_field_info hfi_rtp_jpeg_qtable_hdr_length JPEG_HFI_INIT = {
	"Length",
	"jpeg.qtable_hdr.length",
	FT_UINT16, BASE_DEC, NULL, 0,
	NULL, HFILL
};

static header_field_info hfi_rtp_jpeg_qtable_hdr_data JPEG_HFI_INIT = {
	"Quantization Table Data",
	"jpeg.qtable_hdr.data",
	FT_BYTES, BASE_NONE, NULL, 0,
	NULL, HFILL
};


static header_field_info hfi_rtp_jpeg_payload JPEG_HFI_INIT = {
	"Payload",
	"jpeg.payload",
	FT_BYTES, BASE_NONE, NULL, 0,
	NULL, HFILL
};


/* JPEG fields defining a sub tree */
static gint ett_jpeg = -1;

static int
dissect_jpeg( tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_ )
{
	proto_item *ti = NULL;
	proto_tree *jpeg_tree = NULL;
	proto_tree *main_hdr_tree = NULL;
	proto_tree *restart_hdr_tree = NULL;
	proto_tree *qtable_hdr_tree = NULL;
	guint32 fragment_offset = 0;
	guint16 len = 0;
	guint8 type = 0;
	guint8 q = 0;
	gint h = 0;
	gint w = 0;

	unsigned int offset       = 0;

	col_set_str(pinfo->cinfo, COL_PROTOCOL, "JPEG");

	col_set_str(pinfo->cinfo, COL_INFO, "JPEG message");

	if ( tree ) {
		ti = proto_tree_add_item( tree, hfi_jpeg, tvb, offset, -1, ENC_NA );
		jpeg_tree = proto_item_add_subtree( ti, ett_jpeg );

		ti = proto_tree_add_item(jpeg_tree, &hfi_rtp_jpeg_main_hdr, tvb, offset, 8, ENC_NA);
		main_hdr_tree = proto_item_add_subtree(ti, ett_jpeg);

		proto_tree_add_item(main_hdr_tree, &hfi_rtp_jpeg_main_hdr_ts, tvb, offset, 1, ENC_BIG_ENDIAN);
		offset += 1;
		proto_tree_add_item(main_hdr_tree, &hfi_rtp_jpeg_main_hdr_offs, tvb, offset, 3, ENC_BIG_ENDIAN);
		fragment_offset = tvb_get_ntoh24(tvb, offset);
		offset += 3;
		proto_tree_add_item(main_hdr_tree, &hfi_rtp_jpeg_main_hdr_type, tvb, offset, 1, ENC_BIG_ENDIAN);
		type = tvb_get_guint8(tvb, offset);
		offset += 1;
		proto_tree_add_item(main_hdr_tree, &hfi_rtp_jpeg_main_hdr_q, tvb, offset, 1, ENC_BIG_ENDIAN);
		q = tvb_get_guint8(tvb, offset);
		offset += 1;
		w = tvb_get_guint8(tvb, offset) * 8;
		proto_tree_add_uint(main_hdr_tree, &hfi_rtp_jpeg_main_hdr_width, tvb, offset, 1, w);
		offset += 1;
		h = tvb_get_guint8(tvb, offset) * 8;
		proto_tree_add_uint(main_hdr_tree, &hfi_rtp_jpeg_main_hdr_height, tvb, offset, 1, h);
		offset += 1;

		if (type >= 64 && type <= 127) {
			ti = proto_tree_add_item(jpeg_tree, &hfi_rtp_jpeg_restart_hdr, tvb, offset, 4, ENC_NA);
			restart_hdr_tree = proto_item_add_subtree(ti, ett_jpeg);
			proto_tree_add_item(restart_hdr_tree, &hfi_rtp_jpeg_restart_hdr_interval, tvb, offset, 2, ENC_BIG_ENDIAN);
			offset += 2;
			proto_tree_add_item(restart_hdr_tree, &hfi_rtp_jpeg_restart_hdr_f, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(restart_hdr_tree, &hfi_rtp_jpeg_restart_hdr_l, tvb, offset, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(restart_hdr_tree, &hfi_rtp_jpeg_restart_hdr_count, tvb, offset, 2, ENC_BIG_ENDIAN);
			offset += 2;
		}

		if (q >= 128 && fragment_offset == 0) {
			ti = proto_tree_add_item(jpeg_tree, &hfi_rtp_jpeg_qtable_hdr, tvb, offset, -1, ENC_NA);
			qtable_hdr_tree = proto_item_add_subtree(ti, ett_jpeg);
			proto_tree_add_item(qtable_hdr_tree, &hfi_rtp_jpeg_qtable_hdr_mbz, tvb, offset, 1, ENC_BIG_ENDIAN);
			offset += 1;
			proto_tree_add_item(qtable_hdr_tree, &hfi_rtp_jpeg_qtable_hdr_prec, tvb, offset, 1, ENC_BIG_ENDIAN);
			offset += 1;
			proto_tree_add_item(qtable_hdr_tree, &hfi_rtp_jpeg_qtable_hdr_length, tvb, offset, 2, ENC_BIG_ENDIAN);
			len = tvb_get_ntohs(tvb, offset);
			offset += 2;
			if (len > 0) {
				proto_tree_add_item(qtable_hdr_tree, &hfi_rtp_jpeg_qtable_hdr_data, tvb, offset, len, ENC_NA);
				offset += len;
			}
			proto_item_set_len(ti, len + 4);
		}

		/* The rest of the packet is the JPEG data */
		proto_tree_add_item( jpeg_tree, &hfi_rtp_jpeg_payload, tvb, offset, -1, ENC_NA );
	}
	return tvb_captured_length(tvb);
}

void
proto_register_jpeg(void)
{
#ifndef HAVE_HFI_SECTION_INIT
	static header_field_info *hfi[] =
	{
		&hfi_rtp_jpeg_main_hdr,
		&hfi_rtp_jpeg_main_hdr_ts,
		&hfi_rtp_jpeg_main_hdr_offs,
		&hfi_rtp_jpeg_main_hdr_type,
		&hfi_rtp_jpeg_main_hdr_q,
		&hfi_rtp_jpeg_main_hdr_width,
		&hfi_rtp_jpeg_main_hdr_height,
		&hfi_rtp_jpeg_restart_hdr,
		&hfi_rtp_jpeg_restart_hdr_interval,
		&hfi_rtp_jpeg_restart_hdr_f,
		&hfi_rtp_jpeg_restart_hdr_l,
		&hfi_rtp_jpeg_restart_hdr_count,
		&hfi_rtp_jpeg_qtable_hdr,
		&hfi_rtp_jpeg_qtable_hdr_mbz,
		&hfi_rtp_jpeg_qtable_hdr_prec,
		&hfi_rtp_jpeg_qtable_hdr_length,
		&hfi_rtp_jpeg_qtable_hdr_data,
		&hfi_rtp_jpeg_payload,
	};
#endif

	static gint *ett[] =
	{
		&ett_jpeg,
	};

	int proto_jpeg;

	proto_jpeg = proto_register_protocol("RFC 2435 JPEG","JPEG","jpeg");
	hfi_jpeg = proto_registrar_get_nth(proto_jpeg);

	proto_register_fields(proto_jpeg, hfi, array_length(hfi));
	proto_register_subtree_array(ett, array_length(ett));

	jpeg_handle = create_dissector_handle(dissect_jpeg, proto_jpeg);

	/* RFC 2798 */
	register_ber_oid_dissector_handle("0.9.2342.19200300.100.1.60", jpeg_handle, proto_jpeg, "jpegPhoto");
}

void
proto_reg_handoff_jpeg(void)
{
	dissector_add_uint("rtp.pt", PT_JPEG, jpeg_handle);
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
