/* packet-dcerpc-messenger.c
 * Routines for SMB \PIPE\msgsvc packet disassembly
 * Copyright 2003 Ronnie Sahlberg
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

#include <epan/packet.h>
#include "packet-dcerpc.h"
#include "packet-dcerpc-nt.h"
#include "packet-windows-common.h"

void proto_register_dcerpc_messenger(void);
void proto_reg_handoff_dcerpc_messenger(void);

static int proto_dcerpc_messenger = -1;
static int hf_messenger_opnum = -1;
static int hf_messenger_rc = -1;
static int hf_messenger_server = -1;
static int hf_messenger_client = -1;
static int hf_messenger_message = -1;

static gint ett_dcerpc_messenger = -1;


/* Windows messenger service listens on two endpoints:
 *   \pipe\msgsvc named pipe
 *   a dynamic UDP port
 */

static e_guid_t uuid_dcerpc_messenger = {
	0x5a7b91f8, 0xff00, 0x11d0,
	{ 0xa9, 0xb2, 0x00, 0xc0, 0x4f, 0xb6, 0xe6, 0xfc}
};

static guint16 ver_dcerpc_messenger = 1;



/*
 * IDL  [in][string][ref] char *server;
 * IDL  [in][string][ref] char *client;
 * IDL  [in][string][ref] char *message;
 */
static int
messenger_dissect_send_message_rqst(tvbuff_t *tvb, int offset, packet_info *pinfo,
			    proto_tree *tree, dcerpc_info *di, guint8 *drep)
{
	offset = dissect_ndr_pointer(tvb, offset, pinfo, tree, di, drep,
			dissect_ndr_char_cvstring, NDR_POINTER_REF,
			"Server", hf_messenger_server);
	offset = dissect_ndr_pointer(tvb, offset, pinfo, tree, di, drep,
			dissect_ndr_char_cvstring, NDR_POINTER_REF,
			"Client", hf_messenger_client);
	offset = dissect_ndr_pointer(tvb, offset, pinfo, tree, di, drep,
			dissect_ndr_char_cvstring, NDR_POINTER_REF,
			"Message", hf_messenger_message);


	return offset;
}
static int
messenger_dissect_send_message_reply(tvbuff_t *tvb, int offset, packet_info *pinfo,
			    proto_tree *tree, dcerpc_info *di, guint8 *drep)
{
	offset = dissect_ntstatus(tvb, offset, pinfo, tree, di, drep,
				  hf_messenger_rc, NULL);

	return offset;
}



static dcerpc_sub_dissector dcerpc_messenger_dissectors[] = {
	{0, "NetrSendMessage",
		messenger_dissect_send_message_rqst,
		messenger_dissect_send_message_reply },
	{0, NULL, NULL,  NULL }
};

void
proto_register_dcerpc_messenger(void)
{
	static hf_register_info hf[] = {

		{ &hf_messenger_opnum,
		  { "Operation", "messenger.opnum", FT_UINT16, BASE_DEC,
		    NULL, 0x0, NULL, HFILL }},

		{ &hf_messenger_rc,
		  { "Return code", "messenger.rc", FT_UINT32, BASE_HEX | BASE_EXT_STRING, &NT_errors_ext, 0x0, NULL, HFILL }},

		{ &hf_messenger_server, {
		"Server", "messenger.server",
		FT_STRING, BASE_NONE, NULL, 0, "Server to send the message to", HFILL }},

		{ &hf_messenger_client, {
		"Client", "messenger.client",
		FT_STRING, BASE_NONE, NULL, 0, "Client that sent the message", HFILL }},

		{ &hf_messenger_message, {
		"Message", "messenger.message",
		FT_STRING, BASE_NONE, NULL, 0, "The message being sent", HFILL }}

	};

	static gint *ett[] = {
		&ett_dcerpc_messenger
	};

	proto_dcerpc_messenger = proto_register_protocol(
		"Microsoft Messenger Service", "Messenger", "messenger");

	proto_register_field_array (proto_dcerpc_messenger, hf, array_length (hf));
	proto_register_subtree_array(ett, array_length(ett));

}

void
proto_reg_handoff_dcerpc_messenger(void)
{
	/* Register protocol as dcerpc */

	dcerpc_init_uuid(proto_dcerpc_messenger, ett_dcerpc_messenger, &uuid_dcerpc_messenger,
			 ver_dcerpc_messenger, dcerpc_messenger_dissectors, hf_messenger_opnum);
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
