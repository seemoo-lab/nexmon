/* packet-dcom-dispatch.h
 * Routines for DCOM IDispatch
 *
 * $Id: packet-dcom-dispatch.h 18196 2006-05-21 04:49:01Z sahlberg $
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

/* see packet-dcom.c for details about DCOM */

#ifndef __PACKET_DCERPC_DCOM_DISPATCH_H
#define __PACKET_DCERPC_DCOM_DISPATCH_H

extern int
dissect_IDispatch_GetTypeInfoCount_resp(tvbuff_t *tvb, int offset,
	packet_info *pinfo, proto_tree *tree, guint8 *drep);

extern int
dissect_IDispatch_GetTypeInfo_rqst(tvbuff_t *tvb, int offset,
	packet_info *pinfo, proto_tree *tree, guint8 *drep);

extern int
dissect_IDispatch_GetTypeInfo_resp(tvbuff_t *tvb, int offset,
	packet_info *pinfo, proto_tree *tree, guint8 *drep);

extern int
dissect_IDispatch_GetIDsOfNames_rqst(tvbuff_t *tvb, int offset,
	packet_info *pinfo, proto_tree *tree, guint8 *drep);

extern int
dissect_IDispatch_GetIDsOfNames_resp(tvbuff_t *tvb, int offset,
	packet_info *pinfo, proto_tree *tree, guint8 *drep);

extern int
dissect_IDispatch_Invoke_rqst(tvbuff_t *tvb, int offset,
	packet_info *pinfo, proto_tree *tree, guint8 *drep);

extern int
dissect_IDispatch_Invoke_resp(tvbuff_t *tvb, int offset,
	packet_info *pinfo, proto_tree *tree, guint8 *drep);

#endif /* __PACKET_DCERPC_DCOM_DISPATCH_H */
