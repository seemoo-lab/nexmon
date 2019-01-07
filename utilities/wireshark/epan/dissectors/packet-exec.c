/* packet-exec.c
 * Routines for exec (rexec) dissection
 * Copyright 2006, Stephen Fisher (see AUTHORS file)
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * Based on BSD rexecd code/man page and parts of packet-rlogin.c
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
#include <epan/conversation.h>
#include <epan/prefs.h>
#include <wsutil/str_util.h>

/* The exec protocol uses TCP port 512 per its IANA assignment */
#define EXEC_PORT 512

/* Variables for our preferences */
static gboolean preference_info_show_username = TRUE;
static gboolean preference_info_show_command = FALSE;

void proto_register_exec(void);
void proto_reg_handoff_exec(void);

/* Initialize the protocol and registered fields */
static int proto_exec = -1;

static int hf_exec_stderr_port = -1;
static int hf_exec_username    = -1;
static int hf_exec_password    = -1;
static int hf_exec_command     = -1;
static int hf_exec_client_server_data = -1;
static int hf_exec_server_client_data = -1;

/* Initialize the subtree pointers */
static gint ett_exec = -1;

#define EXEC_STDERR_PORT_LEN 5
#define EXEC_USERNAME_LEN 16
#define EXEC_PASSWORD_LEN 16
#define EXEC_COMMAND_LEN 256 /* Longer depending on server operating system? */

/* Initialize the structure that will be tied to each conversation.
 * This is used to display the username and/or command in the INFO column of
 * each packet of the conversation. */

typedef enum {
	NONE,
	WAIT_FOR_STDERR_PORT,
	WAIT_FOR_USERNAME,
	WAIT_FOR_PASSWORD,
	WAIT_FOR_COMMAND,
	WAIT_FOR_DATA
} exec_session_state_t;


typedef struct {
	/* Packet number within the conversation */
	guint first_packet_number, second_packet_number;
	guint third_packet_number, fourth_packet_number;

	/* The following variables are given values from session_state_t
	 * above to keep track of where we are in the beginning of the session
	 * (when the username and other fields show up).  This is necessary for
	 * when the user clicks randomly through the initial packets instead of
	 * going in order.
	 */

	/* Track where we are in the conversation */
	exec_session_state_t state;
	exec_session_state_t first_packet_state, second_packet_state;
	exec_session_state_t third_packet_state, fourth_packet_state;

	gchar *username;
	gchar *command;
} exec_hash_entry_t;


static int
dissect_exec(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
	/* Set up structures needed to add the protocol subtree and manage it */
	proto_item *ti;
	proto_tree *exec_tree=NULL;

	/* Variables for extracting and displaying data from the packet */
	guchar *field_stringz; /* Temporary storage for each field we extract */

	gint length;
	guint offset = 0;
	conversation_t *conversation;
	exec_hash_entry_t *hash_info;

	conversation = find_or_create_conversation(pinfo);

	/* Retrieve information from conversation
	 * or add it if it isn't there yet
	 */
	hash_info = (exec_hash_entry_t *)conversation_get_proto_data(conversation, proto_exec);
	if(!hash_info){
		hash_info = wmem_new(wmem_file_scope(), exec_hash_entry_t);

		hash_info->first_packet_number = pinfo->num;
		hash_info->second_packet_number = 0;
		hash_info->third_packet_number  = 0;
		hash_info->fourth_packet_number  = 0;

		hash_info->state = WAIT_FOR_STDERR_PORT; /* The first field we'll see */

		/* Start with empty username and command strings */
		hash_info->username=NULL;
		hash_info->command=NULL;

		/* These will be set on the first pass by the first
		 * four packets of the conversation
		 */
		hash_info->first_packet_state  = NONE;
		hash_info->second_packet_state = NONE;
		hash_info->third_packet_state  = NONE;
		hash_info->fourth_packet_state  = NONE;

		conversation_add_proto_data(conversation, proto_exec, hash_info);
	}

	/* Store the number of the first three packets of this conversation
	 * as we reach them the first time */

	if(!hash_info->second_packet_number
	&& pinfo->num > hash_info->first_packet_number){
		/* We're on the second packet of the conversation */
		hash_info->second_packet_number = pinfo->num;
	} else if(hash_info->second_packet_number
	 && !hash_info->third_packet_number
	 && pinfo->num > hash_info->second_packet_number) {
		/* We're on the third packet of the conversation */
		hash_info->third_packet_number = pinfo->num;
	} else if(hash_info->third_packet_number
	 && !hash_info->fourth_packet_number
	 && pinfo->num > hash_info->third_packet_number) {
		/* We're on the fourth packet of the conversation */
		hash_info->fourth_packet_number = pinfo->num;
	}

	/* Save this packet's state so we can retrieve it if this packet
	 * is selected again later.  If the packet's state was already stored,
	 * then retrieve it */
	if(pinfo->num == hash_info->first_packet_number){
		if(hash_info->first_packet_state == NONE){
			hash_info->first_packet_state = hash_info->state;
		} else {
			hash_info->state = hash_info->first_packet_state;
		}
	}

	if(pinfo->num == hash_info->second_packet_number){
		if(hash_info->second_packet_state == NONE){
			hash_info->second_packet_state = hash_info->state;
		} else {
			hash_info->state = hash_info->second_packet_state;
		}
	}

	if(pinfo->num == hash_info->third_packet_number){
		if(hash_info->third_packet_state == NONE){
			hash_info->third_packet_state = hash_info->state;
		} else {
			hash_info->state = hash_info->third_packet_state;
		}
	}

	if(pinfo->num == hash_info->fourth_packet_number){
		if(hash_info->fourth_packet_state == NONE){
			hash_info->fourth_packet_state = hash_info->state;
		} else {
			hash_info->state = hash_info->fourth_packet_state;
		}
	}

	col_set_str(pinfo->cinfo, COL_PROTOCOL, "EXEC");

	/* First, clear the info column */
	col_clear(pinfo->cinfo, COL_INFO);

	/*username */
	if(hash_info->username && preference_info_show_username == TRUE){
		col_append_fstr(pinfo->cinfo, COL_INFO, "Username:%s ", hash_info->username);
	}

	/* Command */
	if(hash_info->command && preference_info_show_command == TRUE){
		col_append_fstr(pinfo->cinfo, COL_INFO, "Command:%s ", hash_info->command);
	}

	/* create display subtree for the protocol */
	ti = proto_tree_add_item(tree, proto_exec, tvb, 0, -1, ENC_NA);
	exec_tree = proto_item_add_subtree(ti, ett_exec);

	/* If this packet doesn't end with a null terminated string,
	 * then it must be session data only and we can skip looking
	 * for the other fields.
	 */
	if(tvb_find_guint8(tvb, tvb_captured_length(tvb)-1, 1, '\0') == -1){
		hash_info->state = WAIT_FOR_DATA;
	}

	if(hash_info->state == WAIT_FOR_STDERR_PORT
	&& tvb_reported_length_remaining(tvb, offset)){
		field_stringz = tvb_get_stringz_enc(wmem_packet_scope(), tvb, offset, &length, ENC_ASCII);

		/* Check if this looks like the stderr_port field.
		 * It is optional, so it may only be 1 character long
		 * (the NULL)
		 */
		if(length == 1 || (isdigit_string(field_stringz)
		&& length <= EXEC_STDERR_PORT_LEN)){
			proto_tree_add_string(exec_tree, hf_exec_stderr_port, tvb, offset, length, (gchar*)field_stringz);
			 /* Next field we need */
			hash_info->state = WAIT_FOR_USERNAME;
		} else {
			/* Since the data doesn't match this field, it must be data only */
			hash_info->state = WAIT_FOR_DATA;
		}

		/* Used if the next field is in the same packet */
		offset += length;
	}


	if(hash_info->state == WAIT_FOR_USERNAME
	&& tvb_reported_length_remaining(tvb, offset)){
		field_stringz = tvb_get_stringz_enc(wmem_packet_scope(), tvb, offset, &length, ENC_ASCII);

		/* Check if this looks like the username field */
		if(length != 1 && length <= EXEC_USERNAME_LEN
		&& isprint_string(field_stringz)){
			proto_tree_add_string(exec_tree, hf_exec_username, tvb, offset, length, (gchar*)field_stringz);

			/* Store the username so we can display it in the
			 * info column of the entire conversation
			 */
			if(!hash_info->username){
				hash_info->username=wmem_strdup(wmem_file_scope(), (gchar*)field_stringz);
			}

			 /* Next field we need */
			hash_info->state = WAIT_FOR_PASSWORD;
		} else {
			/* Since the data doesn't match this field, it must be data only */
			hash_info->state = WAIT_FOR_DATA;
		}

		/* Used if the next field is in the same packet */
		offset += length;
	}


	if(hash_info->state == WAIT_FOR_PASSWORD
	&& tvb_reported_length_remaining(tvb, offset)){
		field_stringz = tvb_get_stringz_enc(wmem_packet_scope(), tvb, offset, &length, ENC_ASCII);

		/* Check if this looks like the password field */
		if(length != 1 && length <= EXEC_PASSWORD_LEN
		&& isprint_string(field_stringz)){
			proto_tree_add_string(exec_tree, hf_exec_password, tvb, offset, length, (gchar*)field_stringz);

			/* Next field we need */
			hash_info->state = WAIT_FOR_COMMAND;
		} else {
			/* Since the data doesn't match this field, it must be data only */
			hash_info->state = WAIT_FOR_DATA;
		}

		/* Used if the next field is in the same packet */
		offset += length;
		 /* Next field we are looking for */
		hash_info->state = WAIT_FOR_COMMAND;
	}


	if(hash_info->state == WAIT_FOR_COMMAND
	&& tvb_reported_length_remaining(tvb, offset)){
		field_stringz = tvb_get_stringz_enc(wmem_packet_scope(), tvb, offset, &length, ENC_ASCII);

		/* Check if this looks like the command field */
		if(length != 1 && length <= EXEC_COMMAND_LEN
		&& isprint_string(field_stringz)){
			proto_tree_add_string(exec_tree, hf_exec_command, tvb, offset, length, (gchar*)field_stringz);

			/* Store the command so we can display it in the
			 * info column of the entire conversation
			 */
			if(!hash_info->command){
				hash_info->command=wmem_strdup(wmem_file_scope(), (gchar*)field_stringz);
			}

		} else {
			/* Since the data doesn't match this field, it must be data only */
			hash_info->state = WAIT_FOR_DATA;
		}
	}


	if(hash_info->state == WAIT_FOR_DATA
	&& tvb_reported_length_remaining(tvb, offset)){
		if(pinfo->destport == EXEC_PORT){
			/* Packet going to the server */
			/* offset = 0 since the whole packet is data */
			proto_tree_add_item(exec_tree, hf_exec_client_server_data, tvb, 0, -1, ENC_NA);

			col_append_str(pinfo->cinfo, COL_INFO, "Client -> Server data");
		} else {
			/* This packet must be going back to the client */
			/* offset = 0 since the whole packet is data */
			proto_tree_add_item(exec_tree, hf_exec_server_client_data, tvb, 0, -1, ENC_NA);

			col_append_str(pinfo->cinfo, COL_INFO, "Server -> Client Data");
		}
	}

	/* We haven't seen all of the fields yet */
	if(hash_info->state < WAIT_FOR_DATA){
		col_set_str(pinfo->cinfo, COL_INFO, "Session Establishment");
	}
	return tvb_captured_length(tvb);
}

void
proto_register_exec(void)
{
	static hf_register_info hf[] =
	{
	{ &hf_exec_stderr_port, { "Stderr port (optional)", "exec.stderr_port",
		FT_STRINGZ, BASE_NONE, NULL, 0,
		"Client port that is listening for stderr stream from server", HFILL } },

	{ &hf_exec_username, { "Client username", "exec.username",
		FT_STRINGZ, BASE_NONE, NULL, 0,
		"Username client uses to log in to the server.", HFILL } },

	{ &hf_exec_password, { "Client password", "exec.password",
		FT_STRINGZ, BASE_NONE, NULL, 0,
		"Password client uses to log in to the server.", HFILL } },

	{ &hf_exec_command, { "Command to execute", "exec.command",
		FT_STRINGZ, BASE_NONE, NULL, 0,
		"Command client is requesting the server to run.", HFILL } },

	{ &hf_exec_client_server_data, { "Client -> Server Data", "exec.client_server_data",
		FT_BYTES, BASE_NONE, NULL, 0,
		NULL, HFILL } },

	{ &hf_exec_server_client_data, { "Server -> Client Data", "exec.server_client_data",
		FT_BYTES, BASE_NONE, NULL, 0,
		NULL, HFILL } },

	};

	static gint *ett[] =
	{
		&ett_exec
	};

	module_t *exec_module;

	/* Register the protocol name and description */
	proto_exec = proto_register_protocol("Remote Process Execution", "EXEC", "exec");

	/* Required function calls to register the header fields and subtrees used */
	proto_register_field_array(proto_exec, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));

	/* Register preferences module */
	exec_module = prefs_register_protocol(proto_exec, NULL);

	/* Register our preferences */
	prefs_register_bool_preference(exec_module, "info_show_username",
		 "Show username in info column",
		 "Controls the display of the session's username in the info column.  This is only displayed if the packet containing it was seen during this capture session.", &preference_info_show_username);

	prefs_register_bool_preference(exec_module, "info_show_command",
		 "Show command in info column",
		 "Controls the display of the command being run on the server by this session in the info column.  This is only displayed if the packet containing it was seen during this capture session.", &preference_info_show_command);
}


/* Entry function */
void
proto_reg_handoff_exec(void)
{
	dissector_handle_t exec_handle;

	exec_handle = create_dissector_handle(dissect_exec, proto_exec);
	dissector_add_uint("tcp.port", EXEC_PORT, exec_handle);
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
