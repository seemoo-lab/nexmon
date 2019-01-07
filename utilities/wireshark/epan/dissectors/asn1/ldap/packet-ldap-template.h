/* packet-ldap.h
 * Routines for ros packet dissection
 * Copyright 2005, Anders Broman <anders.broman@ericsson.com>
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

#ifndef __PACKET_LDAP_H__
#define __PACKET_LDAP_H__

# include <epan/packet.h>  /* for dissector_*_t types */
#include "ws_symbol_export.h"

/*
 * These are all APPLICATION types; the value is the type tag.
 */
#define LDAP_REQ_BIND               0
#define LDAP_REQ_UNBIND             2
#define LDAP_REQ_SEARCH             3
#define LDAP_REQ_MODIFY             6
#define LDAP_REQ_ADD                8
#define LDAP_REQ_DELETE             10
#define LDAP_REQ_MODRDN             12
#define LDAP_REQ_COMPARE            14
#define LDAP_REQ_ABANDON            16
#define LDAP_REQ_EXTENDED           23	/* LDAP V3 only */

#define LDAP_RES_BIND               1
#define LDAP_RES_SEARCH_ENTRY       4
#define LDAP_RES_SEARCH_REF         19	/* LDAP V3 only */
#define LDAP_RES_SEARCH_RESULT      5
#define LDAP_RES_MODIFY             7
#define LDAP_RES_ADD                9
#define LDAP_RES_DELETE             11
#define LDAP_RES_MODRDN             13
#define LDAP_RES_COMPARE            15
#define LDAP_RES_EXTENDED           24	/* LDAP V3 only */
#define LDAP_RES_INTERMEDIATE       25	/* LDAP V3 only */

/*
 * These are all CONTEXT types; the value is the type tag.
 */

/* authentication type tags */
#define LDAP_AUTH_SIMPLE        0
#define LDAP_AUTH_KRBV4LDAP     1	/* LDAP V2 only */
#define LDAP_AUTH_KRBV4DSA      2	/* LDAP V2 only */
#define LDAP_AUTH_SASL          3	/* LDAP V3 only */

/* filter type tags */
#define LDAP_FILTER_AND         0
#define LDAP_FILTER_OR          1
#define LDAP_FILTER_NOT         2
#define LDAP_FILTER_EQUALITY    3
#define LDAP_FILTER_SUBSTRINGS  4
#define LDAP_FILTER_GE          5
#define LDAP_FILTER_LE          6
#define LDAP_FILTER_PRESENT     7
#define LDAP_FILTER_APPROX      8
#define LDAP_FILTER_EXTENSIBLE  9	/* LDAP V3 only */

#define LDAP_MOD_ADD            0
#define LDAP_MOD_DELETE         1
#define LDAP_MOD_REPLACE        2
#define LDAP_MOD_INCREMENT      3

#define LDAP_SASL_MAX_BUF	1024*1024

#define NETLOGON_NT_VERSION_1					1
#define NETLOGON_NT_VERSION_5					2
#define NETLOGON_NT_VERSION_5EX					4
#define NETLOGON_NT_VERSION_5EX_WITH_IP			8
#define NETLOGON_NT_VERSION_WITH_CLOSEST_SITE	16

#define LOGON_SAM_LOGON_RESPONSE        19
#define LOGON_SAM_LOGON_RESPONSE_EX     23

typedef struct ldap_call_response {
  gboolean is_request;
  guint32 req_frame;
  nstime_t req_time;
  guint32 rep_frame;
  guint messageId;
  guint protocolOpTag;
} ldap_call_response_t;

WS_DLL_PUBLIC
int dissect_mscldap_string(tvbuff_t *tvb, int offset, char *str, int max_len, gboolean prepend_dot _U_);

WS_DLL_PUBLIC const value_string ldap_procedure_names[];

/*#include "packet-ldap-exp.h" */

#endif  /* PACKET_LDAP_H */


