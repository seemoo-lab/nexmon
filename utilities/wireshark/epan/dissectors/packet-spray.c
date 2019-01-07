/* packet-spray.c
 * 2001  Ronnie Sahlberg   <See AUTHORS for email>
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

#include "config.h"

#include "packet-rpc.h"
#include "packet-spray.h"

void proto_register_spray(void);
void proto_reg_handoff_spray(void);

static int proto_spray = -1;
static int hf_spray_procedure_v1 = -1;
static int hf_spray_sprayarr = -1;
static int hf_spray_counter = -1;
static int hf_spray_clock = -1;
static int hf_spray_sec = -1;
static int hf_spray_usec = -1;

static gint ett_spray = -1;
static gint ett_spray_clock = -1;


static int
dissect_get_reply(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, void* data _U_)
{
	proto_item* lock_item = NULL;
	proto_tree* lock_tree = NULL;
	int offset = 0;

	offset = dissect_rpc_uint32(tvb, tree,
			hf_spray_counter, offset);

	lock_item = proto_tree_add_item(tree, hf_spray_clock, tvb,
			offset, -1, ENC_NA);

	lock_tree = proto_item_add_subtree(lock_item, ett_spray_clock);

	offset = dissect_rpc_uint32(tvb, lock_tree,
			hf_spray_sec, offset);

	offset = dissect_rpc_uint32(tvb, lock_tree,
			hf_spray_usec, offset);

	return offset;
}

static int
dissect_spray_call(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, void* data _U_)
{
	return dissect_rpc_data(tvb, tree, hf_spray_sprayarr, 0);
}

/* proc number, "proc name", dissect_request, dissect_reply */
static const vsff spray1_proc[] = {
	{ SPRAYPROC_NULL,	"NULL",
		dissect_rpc_void,	dissect_rpc_void },
	{ SPRAYPROC_SPRAY,	"SPRAY",
		dissect_spray_call,	dissect_rpc_void },
	{ SPRAYPROC_GET,	"GET",
		dissect_rpc_void,	dissect_get_reply },
	{ SPRAYPROC_CLEAR,	"CLEAR",
		dissect_rpc_void,	dissect_rpc_void },
	{ 0,	NULL,		NULL,				NULL }
};
static const value_string spray1_proc_vals[] = {
	{ SPRAYPROC_NULL,	"NULL" },
	{ SPRAYPROC_SPRAY,	"SPRAY" },
	{ SPRAYPROC_GET,	"GET" },
	{ SPRAYPROC_CLEAR,	"CLEAR" },
	{ 0,	NULL }
};

static const rpc_prog_vers_info spray_vers_info[] = {
	{ 1, spray1_proc, &hf_spray_procedure_v1 },
};

void
proto_register_spray(void)
{
	static hf_register_info hf[] = {
		{ &hf_spray_procedure_v1, {
			"V1 Procedure", "spray.procedure_v1", FT_UINT32, BASE_DEC,
			VALS(spray1_proc_vals), 0, NULL, HFILL }},
		{ &hf_spray_sprayarr, {
			"Data", "spray.sprayarr", FT_BYTES, BASE_NONE,
			NULL, 0, "Sprayarr data", HFILL }},

		{ &hf_spray_counter, {
			"counter", "spray.counter", FT_UINT32, BASE_DEC,
			NULL, 0, NULL, HFILL }},

		{ &hf_spray_clock, {
			"clock", "spray.clock", FT_NONE, BASE_NONE,
			NULL, 0, NULL, HFILL }},

		{ &hf_spray_sec, {
			"sec", "spray.sec", FT_UINT32, BASE_DEC,
			NULL, 0, "Seconds", HFILL }},

		{ &hf_spray_usec, {
			"usec", "spray.usec", FT_UINT32, BASE_DEC,
			NULL, 0, "Microseconds", HFILL }}

	};

	static gint *ett[] = {
		&ett_spray,
		&ett_spray_clock,
	};

	proto_spray = proto_register_protocol("SPRAY", "SPRAY", "spray");
	proto_register_field_array(proto_spray, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
}

void
proto_reg_handoff_spray(void)
{
	/* Register the protocol as RPC */
	rpc_init_prog(proto_spray, SPRAY_PROGRAM, ett_spray,
	    G_N_ELEMENTS(spray_vers_info), spray_vers_info);
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
