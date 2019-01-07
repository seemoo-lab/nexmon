/* packet-imap.c
 * Routines for imap packet dissection
 * Copyright 1999, Richard Sharpe <rsharpe@ns.aus.com>
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * Copied from packet-tftp.c
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
#include <epan/strutil.h>
#include "packet-ssl.h"
#include "packet-ssl-utils.h"

void proto_register_imap(void);
void proto_reg_handoff_imap(void);

static int proto_imap = -1;
static int hf_imap_isrequest = -1;
static int hf_imap_line = -1;
static int hf_imap_request = -1;
static int hf_imap_request_tag = -1;
static int hf_imap_response = -1;
static int hf_imap_response_tag = -1;
static int hf_imap_request_command = -1;
static int hf_imap_response_status = -1;
static int hf_imap_request_folder = -1;
static int hf_imap_request_uid = -1;

static gint ett_imap = -1;
static gint ett_imap_reqresp = -1;

static dissector_handle_t imap_handle;
static dissector_handle_t ssl_handle;

static gboolean imap_ssl_heuristic = TRUE;

#define TCP_PORT_IMAP     143
#define TCP_PORT_SSL_IMAP 993
#define MAX_BUFFER        1024
#define IMAP_HEUR_LEN     5

typedef struct imap_state {
  gboolean  ssl_requested;
  gint      ssl_heur_tries_left;
} imap_state_t;

/* Heuristic to detect plaintext or TLS ciphertext IMAP */
static gboolean
check_imap_heur(tvbuff_t *tvb)
{
  const gchar *s;
  gint i;

  if (!tvb_bytes_exist(tvb, 0, IMAP_HEUR_LEN)) {
    return TRUE;
  }

  s = (const gchar *)tvb_get_ptr(tvb, 0, IMAP_HEUR_LEN);
  for (i = 0; i < IMAP_HEUR_LEN; i++) {
    if (!g_ascii_isprint(s[i])) {
      return FALSE;
    }
  }
  return TRUE;
}

static int
dissect_imap(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
  gboolean        is_request;
  proto_tree      *imap_tree, *reqresp_tree;
  proto_item      *ti, *hidden_item;
  gint            offset = 0;
  gint            uid_offset = 0;
  gint            folder_offset = 0;
  const guchar    *line;
  const guchar    *uid_line;
  const guchar    *folder_line;
  gint            next_offset;
  int             linelen;
  int             tokenlen;
  int             uid_tokenlen;
  int             folder_tokenlen;
  const guchar    *next_token;
  const guchar    *uid_next_token;
  const guchar    *folder_next_token;
  guchar          *tokenbuf;
  guchar          *command_token;
  int             iter;
  int             commandlen;
  conversation_t *conversation;
  imap_state_t   *session_state;

  conversation = find_or_create_conversation(pinfo);
  session_state = (imap_state_t *)conversation_get_proto_data(conversation, proto_imap);
  if (!session_state) {
    session_state = wmem_new0(wmem_file_scope(), imap_state_t);
    session_state->ssl_requested = FALSE;
    if (imap_ssl_heuristic)
      session_state->ssl_heur_tries_left = 2;
    else
      session_state->ssl_heur_tries_left = -1; /* Disabled */
    conversation_add_proto_data(conversation, proto_imap, session_state);
  }

  if (imap_ssl_heuristic && session_state->ssl_heur_tries_left < 0) {
    /* Preference changed to enabled */
    session_state->ssl_heur_tries_left = 2;
  }
  else if (!imap_ssl_heuristic && session_state->ssl_heur_tries_left >= 0) {
    /* Preference changed to disabled */
    session_state->ssl_heur_tries_left = -1;
  }

  /*
   * It is possible the IMAP session is already running over TLS and the
   * STARTTLS request/response happened before the capture began. Don't assume
   * we have plaintext without performing some heuristic checks first.
   * We have three cases:
   *   1. capture includes STARTTLS command: no need for heuristics
   *   2. capture starts with STARTTLS OK response: next frame will be TLS (need to retry heuristic)
   *   3. capture start after STARTTLS negotiation: current frame is TLS
   */
  if (session_state->ssl_heur_tries_left > 0) {
    session_state->ssl_heur_tries_left--;
    if (!check_imap_heur(tvb)) {
      ssl_starttls_post_ack(ssl_handle, pinfo, imap_handle);
      session_state->ssl_heur_tries_left = 0;
      return call_dissector(ssl_handle, tvb, pinfo, tree);
    }
  }

  tokenbuf = (guchar *)wmem_alloc0(wmem_packet_scope(), MAX_BUFFER);
  command_token = (guchar *)wmem_alloc0(wmem_packet_scope(), MAX_BUFFER);
  commandlen = 0;
  folder_offset = 0;
  folder_tokenlen = 0;
  folder_line = NULL;
  col_set_str(pinfo->cinfo, COL_PROTOCOL, "IMAP");


  if (pinfo->match_uint == pinfo->destport)
    is_request = TRUE;
  else
    is_request = FALSE;

  /*
   * Put the first line from the buffer into the summary
   * (but leave out the line terminator).
   */
  linelen = tvb_find_line_end(tvb, offset, -1, &next_offset, FALSE);
  line = tvb_get_ptr(tvb, offset, linelen);

  col_add_fstr(pinfo->cinfo, COL_INFO, "%s: %s", is_request ? "Request" : "Response", format_text(line, linelen));

  {
    ti = proto_tree_add_item(tree, proto_imap, tvb, offset, -1, ENC_NA);
    imap_tree = proto_item_add_subtree(ti, ett_imap);

    hidden_item = proto_tree_add_boolean(imap_tree, hf_imap_isrequest, tvb, 0, 0, is_request);
    PROTO_ITEM_SET_HIDDEN(hidden_item);

    while(tvb_offset_exists(tvb, offset)) {

      /*
       * Find the end of each line
       *
       * Note that "tvb_find_line_end()" will return a value that is
       * not longer than what's in the buffer, so the "tvb_get_ptr()"
       * call won't throw an exception.
       */
      linelen = tvb_find_line_end(tvb, offset, -1, &next_offset, FALSE);
      line = tvb_get_ptr(tvb, offset, linelen);

      /*
       * Put the line into the protocol tree.
       */
      ti = proto_tree_add_item(imap_tree, hf_imap_line, tvb, offset, next_offset - offset, ENC_ASCII|ENC_NA);

      reqresp_tree = proto_item_add_subtree(ti, ett_imap_reqresp);

      /*
       * Check that the line doesn't begin with '*', because that's a continuation line.
       * Otherwise if a tag is present then extract tokens.
       */
      if ( (line) && ((line[0] != '*') || (TRUE == is_request)) ) {
        /*
         * Show each line as tags + requests or replies.
         */

        /*
         * Extract the first token, and, if there is a first
         * token, add it as the request or reply tag.
         */
        tokenlen = get_token_len(line, line + linelen, &next_token);
        if (tokenlen != 0) {
          proto_tree_add_item(reqresp_tree, (is_request) ? hf_imap_request_tag : hf_imap_response_tag, tvb, offset, tokenlen, ENC_ASCII|ENC_NA);

          offset += (gint) (next_token - line);
          linelen -= (int) (next_token - line);
          line = next_token;
        }

        /*
         * Extract second token, and, if there is a second
         * token, and it's not uid, add it as the request or reply command.
         */
        tokenlen = get_token_len(line, line + linelen, &next_token);
        if (tokenlen != 0) {
          for (iter = 0; iter < tokenlen && iter < MAX_BUFFER-1; iter++) {
            tokenbuf[iter] = g_ascii_tolower(line[iter]);
          }
          if (tree && is_request && strncmp(tokenbuf, "uid", tokenlen) == 0) {
            proto_tree_add_item(reqresp_tree, hf_imap_request_uid, tvb, offset, tokenlen, ENC_ASCII|ENC_NA);
            /*
             * UID is a precursor to a command, if following the tag,
              * so move to next token to grab the actual command.
              */
            uid_offset = offset;
            uid_offset += (gint) (next_token - line);
            uid_line = next_token;
            uid_tokenlen = get_token_len(uid_line, uid_line + (linelen - tokenlen), &uid_next_token);
            if (tokenlen != 0) {
              proto_tree_add_item(reqresp_tree, hf_imap_request_command, tvb, uid_offset, uid_tokenlen, ENC_ASCII|ENC_NA);

              /*
               * Save command string to do specialized processing.
               */
              for (iter = 0; iter < uid_tokenlen && iter < MAX_BUFFER-1; iter++) {
                command_token[iter] = g_ascii_tolower(uid_line[iter]);
              }
              commandlen = uid_tokenlen;

              folder_offset = uid_offset;
              folder_offset += (gint) (uid_next_token - uid_line);
              folder_line = uid_next_token;
              folder_tokenlen = get_token_len(folder_line, folder_line + (linelen - tokenlen - uid_tokenlen), &folder_next_token);
            }
          } else {
            /*
             * Not a UID request so perform normal parsing.
             */
            proto_tree_add_item(reqresp_tree, (is_request) ? hf_imap_request_command : hf_imap_response_status, tvb, offset, tokenlen, ENC_ASCII|ENC_NA);

            if (is_request) {
              /*
               * Save command string to do specialized processing.
               */
              for (iter = 0; iter < tokenlen && iter < 256; iter++) {
                command_token[iter] = g_ascii_tolower(line[iter]);
              }
              commandlen = tokenlen;

              folder_offset = offset;
              folder_offset += (gint) (next_token - line);
              folder_line = next_token;
              folder_tokenlen = get_token_len(folder_line, folder_line + (linelen - tokenlen - 1), &folder_next_token);
            }
          }

          if (tree && commandlen > 0 && (
              strncmp(command_token, "select", commandlen) == 0 ||
              strncmp(command_token, "examine", commandlen) == 0 ||
              strncmp(command_token, "create", commandlen) == 0 ||
              strncmp(command_token, "delete", commandlen) == 0 ||
              strncmp(command_token, "rename", commandlen) == 0 ||
              strncmp(command_token, "subscribe", commandlen) == 0 ||
              strncmp(command_token, "unsubscribe", commandlen) == 0 ||
              strncmp(command_token, "status", commandlen) == 0 ||
              strncmp(command_token, "append", commandlen) == 0 ||
              strncmp(command_token, "search", commandlen) == 0)) {
            /*
             * These commands support folder as an argument,
             * so parse out the folder name.
             */
            if (folder_tokenlen != 0)
              proto_tree_add_item(reqresp_tree, hf_imap_request_folder, tvb, folder_offset, folder_tokenlen, ENC_ASCII|ENC_NA);
          }

          if (tree && is_request && (NULL != folder_line) && strncmp(command_token, "copy", commandlen) == 0) {
            /*
             * Handle the copy command separately since folder
             * is the second argument for this command.
             */
            folder_offset += (gint) (folder_next_token - folder_line);
            folder_line = folder_next_token;
            folder_tokenlen = get_token_len(folder_line, folder_line + (linelen - tokenlen), &folder_next_token);

            if (folder_tokenlen != 0)
              proto_tree_add_item(reqresp_tree, hf_imap_request_folder, tvb, folder_offset, folder_tokenlen, ENC_ASCII|ENC_NA);
          }

          /* If not yet switched to TLS, check for STARTTLS. */
          if (session_state->ssl_requested) {
            if (!is_request && strncmp(tokenbuf, "ok", tokenlen) == 0) {
              /* STARTTLS accepted, next reply will be TLS. */
              ssl_starttls_ack(ssl_handle, pinfo, imap_handle);
              if (session_state->ssl_heur_tries_left > 0) {
                session_state->ssl_heur_tries_left = 0;
              }
            }
            session_state->ssl_requested = FALSE;
          }
          if (is_request && commandlen > 0 &&
            strncmp(command_token, "starttls", commandlen) == 0) {
            /* If next response is OK, then TLS should be commenced. */
            session_state->ssl_requested = TRUE;
          }
        }

        /*
         * Add the rest of the line as request or reply data.
         */
        if (linelen != 0) {
          proto_tree_add_item(reqresp_tree, (is_request) ? hf_imap_request : hf_imap_response, tvb, offset, linelen, ENC_ASCII|ENC_NA);
        }

      }

      offset = next_offset; /* Skip over last line and \r\n at the end of it */
    }
  }

  return tvb_captured_length(tvb);
}

void
proto_register_imap(void)
{
  static hf_register_info hf[] = {

    { &hf_imap_isrequest,
      { "Request", "imap.isrequest",
         FT_BOOLEAN, BASE_NONE, NULL, 0x0,
         "TRUE if IMAP request, FALSE otherwise", HFILL }
    },
    { &hf_imap_line,
      { "Line", "imap.line",
        FT_STRINGZ, BASE_NONE, NULL, 0x0,
        "A line of an IMAP message", HFILL }
    },
    { &hf_imap_request,
      { "Request", "imap.request",
        FT_STRINGZ, BASE_NONE, NULL, 0x0,
        "Remainder of request line", HFILL }
    },
    { &hf_imap_request_tag,
      { "Request Tag", "imap.request_tag",
        FT_STRINGZ, BASE_NONE, NULL, 0x0,
        "First token of request line", HFILL }
    },
    { &hf_imap_response,
      { "Response", "imap.response",
        FT_STRINGZ, BASE_NONE, NULL, 0x0,
        "Remainder of response line", HFILL }
    },
    { &hf_imap_response_tag,
      { "Response Tag", "imap.response_tag",
      FT_STRINGZ, BASE_NONE, NULL, 0x0,
      "First token of response line", HFILL }
    },
    { &hf_imap_request_command,
      { "Request Command", "imap.request.command",
      FT_STRINGZ, BASE_NONE, NULL, 0x0,
      "Request command name", HFILL }
    },
    { &hf_imap_response_status,
      { "Response Status", "imap.response.status",
      FT_STRINGZ, BASE_NONE, NULL, 0x0,
      "Response status code", HFILL }
    },
    { &hf_imap_request_folder,
      { "Request Folder", "imap.request.folder",
        FT_STRINGZ, BASE_NONE, NULL, 0x0,
        "Request command folder", HFILL }
    },
    { &hf_imap_request_uid,
      { "Request isUID", "imap.request.command.uid",
      FT_BOOLEAN, BASE_NONE, NULL, 0x0,
      "Request command uid", HFILL }
    }
  };

  static gint *ett[] = {
    &ett_imap,
    &ett_imap_reqresp,
  };

  module_t *imap_module;

  proto_imap = proto_register_protocol("Internet Message Access Protocol", "IMAP", "imap");

  imap_handle = register_dissector("imap", dissect_imap, proto_imap);

  proto_register_field_array(proto_imap, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));

  imap_module = prefs_register_protocol(proto_imap, NULL);
  prefs_register_bool_preference(imap_module, "ssl_heuristic",
                                   "Use heuristic detection for TLS",
                                   "Whether to use heuristics for post-STARTTLS detection of encrypted IMAP conversations",
                                   &imap_ssl_heuristic);
}

void
proto_reg_handoff_imap(void)
{
  dissector_add_uint("tcp.port", TCP_PORT_IMAP, imap_handle);
  ssl_dissector_add(TCP_PORT_SSL_IMAP, imap_handle);
  ssl_handle = find_dissector("ssl");
}
/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 2
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=2 tabstop=8 expandtab:
 * :indentSize=2:tabSize=8:noTabs=true
 */
