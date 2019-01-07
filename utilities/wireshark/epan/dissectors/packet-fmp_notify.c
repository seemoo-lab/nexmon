/* packet-fmp_notify.c
 * Routines for fmp dissection
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <epan/packet.h>

#include "packet-rpc.h"
#include "packet-fmp.h"

void proto_register_fmp_notify(void);
void proto_reg_handoff_fmp_notify(void);

#define FMP_NOTIFY_PROG		1001912
#define FMP_NOTIFY_VERSION_2	      2

/*
 * FMP/NOTIFY Procedures
 */
#define FMP_NOTIFY_DownGrade		1
#define FMP_NOTIFY_RevokeList		2
#define FMP_NOTIFY_RevokeAll		3
#define FMP_NOTIFY_FileSetEof		4
#define FMP_NOTIFY_RequestDone		5
#define FMP_NOTIFY_volFreeze		6
#define FMP_NOTIFY_revokeHandleList	7

typedef enum {
	FMP_LIST_USER_QUOTA_EXCEEDED  = 0,
	FMP_LIST_GROUP_QUOTA_EXCEEDED = 1,
	FMP_LIST_SERVER_RESOURCE_LOW  = 2
} revokeHandleListReason;

static int proto_fmp_notify = -1;
static int hf_fmp_handleListLen = -1;
static int hf_fmp_notify_procedure = -1;
static int hf_fmp_fsID = -1;
/* static int hf_fmp_fsBlkSz = -1; */
static int hf_fmp_sessionHandle = -1;
static int hf_fmp_fmpFHandle = -1;
static int hf_fmp_msgNum = -1;
static int hf_fmp_fileSize = -1;
static int hf_fmp_cookie = -1;
static int hf_fmp_firstLogBlk = -1;
static int hf_fmp_numBlksReq = -1;
static int hf_fmp_status = -1;
static int hf_fmp_extentList_len = -1;
static int hf_fmp_numBlks = -1;
static int hf_fmp_volID = -1;
static int hf_fmp_startOffset = -1;
static int hf_fmp_extent_state = -1;
static int hf_fmp_revokeHandleListReason = -1;

static gint ett_fmp_notify = -1;
static gint ett_fmp_notify_hlist = -1;
static gint ett_fmp_extList = -1;
static gint ett_fmp_ext = -1;


static int dissect_fmp_notify_extentList(tvbuff_t *, int, packet_info *, proto_tree *);

static int
dissect_fmp_notify_status(tvbuff_t *tvb, int offset, proto_tree *tree, int *rval)
{
	fmpStat status;

	status = (fmpStat)tvb_get_ntohl(tvb, offset);

	switch (status) {
	case FMP_OK:
		*rval = 0;
		break;
	case FMP_IOERROR:
		*rval = 1;
		break;
	case FMP_NOMEM:
		*rval = 1;
		break;
	case FMP_NOACCESS:
		*rval = 1;
		break;
	 case FMP_INVALIDARG:
		*rval = 1;
		break;
	case FMP_FSFULL:
		*rval = 0;
		break;
	case FMP_QUEUE_FULL:
		*rval = 1;
		break;
	case FMP_WRONG_MSG_NUM:
		*rval = 1;
		break;
	case FMP_SESSION_LOST:
		*rval = 1;
		break;
	case FMP_HOT_SESSION:
		*rval = 0;
		break;

	case FMP_COLD_SESSION:
		*rval = 0;
		break;
	case FMP_CLIENT_TERMINATED:
		*rval = 0;
		break;
	case FMP_WRITER_LOST_BLK:
		*rval = 1;
		break;
	case FMP_REQUEST_QUEUED:
		*rval = 0;
		break;
	case FMP_FALL_BACK:
		*rval = 0;
		break;
	case FMP_REQUEST_CANCELLED:
		*rval = 1;
		break;

	       case FMP_WRITER_ZEROED_BLK:
		*rval = 0;
		break;
	case FMP_NOTIFY_ERROR:
		*rval = 1;
		break;
	case FMP_WRONG_HANDLE:
		*rval = 0;
		break;
	case FMP_DUPLICATE_OPEN:
		*rval = 1;
		break;
	case FMP_PLUGIN_NOFUNC:
		*rval = 1;
		break;
	default:
		*rval = 1;
		break;
	}

	offset = dissect_rpc_uint32(tvb, tree, hf_fmp_status , offset);
	return offset;

}

static int
dissect_revokeHandleListReason(tvbuff_t *tvb, int offset, proto_tree *tree)
{
	proto_tree_add_item(tree, hf_fmp_revokeHandleListReason, tvb, offset, 4, ENC_BIG_ENDIAN);
	offset += 4;
	return offset;
}

static int
dissect_handleList(tvbuff_t *tvb, int offset, packet_info *pinfo _U_,
		   proto_tree *tree)
{

	int	    numHandles;
	int	    listLength;
	int	    i;
	proto_tree *handleListTree;

	numHandles = tvb_get_ntohl(tvb, offset);
	listLength = 4;

	for (i = 0; i < numHandles; i++) {
		listLength += (4 + tvb_get_ntohl(tvb, offset + listLength));
	}

	handleListTree =  proto_tree_add_subtree(tree, tvb, offset, listLength,
					      ett_fmp_notify_hlist, NULL, "Handle List");

	offset = dissect_rpc_uint32(tvb,  handleListTree,
				    hf_fmp_handleListLen, offset);

	for (i = 0; i < numHandles; i++) {
		offset = dissect_rpc_data(tvb, handleListTree,
					  hf_fmp_fmpFHandle, offset);/*	 changed */
	}

	return offset;
}

static int
dissect_FMP_NOTIFY_DownGrade_request(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, void* data _U_)
{
	int offset = 0;

	offset = dissect_rpc_data(tvb,	tree, hf_fmp_sessionHandle,
				  offset);
	offset = dissect_rpc_data(tvb,	tree, hf_fmp_fmpFHandle, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_fmp_msgNum, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_fmp_firstLogBlk,
				    offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_fmp_numBlksReq, offset);
	return offset;
}

static int
dissect_FMP_NOTIFY_DownGrade_reply(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, void* data _U_)
{
	int rval;

	return dissect_fmp_notify_status(tvb, 0,tree, &rval);
}

static int
dissect_FMP_NOTIFY_RevokeList_request(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, void* data _U_)
{
	int offset = 0;

	offset = dissect_rpc_data(tvb,	tree, hf_fmp_sessionHandle,
				  offset);
	offset = dissect_rpc_data(tvb, tree, hf_fmp_fmpFHandle, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_fmp_msgNum, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_fmp_firstLogBlk,
				    offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_fmp_numBlksReq, offset);
	return offset;
}

static int
dissect_FMP_NOTIFY_RevokeList_reply(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, void* data _U_)
{
	int rval;

	return dissect_fmp_notify_status(tvb, 0, tree, &rval);
}

static int
dissect_FMP_NOTIFY_RevokeAll_request(tvbuff_t *tvb,
				     packet_info *pinfo _U_, proto_tree *tree, void* data _U_)
{
	int offset = 0;
	offset = dissect_rpc_data(tvb, tree, hf_fmp_sessionHandle,
				  offset);
	offset = dissect_rpc_data(tvb, tree, hf_fmp_fmpFHandle, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_fmp_msgNum, offset);
	return offset;
}

static int
dissect_FMP_NOTIFY_RevokeAll_reply(tvbuff_t *tvb,
				   packet_info *pinfo _U_, proto_tree *tree, void* data _U_)
{
	int rval;

	return dissect_fmp_notify_status(tvb, 0, tree, &rval);
}

static int
dissect_FMP_NOTIFY_FileSetEof_request(tvbuff_t *tvb,
				      packet_info *pinfo _U_, proto_tree *tree, void* data _U_)
{
	int offset = 0;
	offset = dissect_rpc_data(tvb, tree, hf_fmp_sessionHandle,
				  offset);
	offset = dissect_rpc_data(tvb, tree, hf_fmp_fmpFHandle, offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_fmp_msgNum, offset);
	offset = dissect_rpc_uint64(tvb, tree, hf_fmp_fileSize, offset);
	return offset;
}

static int
dissect_FMP_NOTIFY_FileSetEof_reply(tvbuff_t *tvb,
				    packet_info *pinfo _U_, proto_tree *tree, void* data _U_)
{
	int rval;

	return dissect_fmp_notify_status(tvb, 0, tree, &rval);
}

static int
dissect_FMP_NOTIFY_RequestDone_request(tvbuff_t *tvb,
				       packet_info *pinfo, proto_tree *tree, void* data _U_)
{
	int rval;
	int offset = 0;

	offset = dissect_fmp_notify_status(tvb, offset,tree, &rval);
	if (rval == 0) {
		offset = dissect_rpc_data(tvb,	tree,
					  hf_fmp_sessionHandle, offset);
		offset = dissect_rpc_data(tvb, tree, hf_fmp_fmpFHandle,
					  offset);
		offset = dissect_rpc_uint32(tvb, tree, hf_fmp_msgNum,
					    offset);
		offset = dissect_rpc_uint32(tvb, tree, hf_fmp_cookie,
					    offset);
		offset = dissect_fmp_notify_extentList(tvb, offset, pinfo, tree);
	}
	return offset;
}

static int
dissect_FMP_NOTIFY_RequestDone_reply(tvbuff_t *tvb,
				     packet_info *pinfo _U_, proto_tree *tree, void* data _U_)
{
	int rval;

	return dissect_fmp_notify_status(tvb, 0, tree, &rval);
}

static int
dissect_FMP_NOTIFY_volFreeze_request(tvbuff_t *tvb,
				     packet_info *pinfo _U_, proto_tree *tree, void* data _U_)
{
	int offset = 0;

	offset = dissect_rpc_data(tvb, tree, hf_fmp_sessionHandle,
				  offset);
	offset = dissect_rpc_uint32(tvb, tree, hf_fmp_fsID, offset);
	return offset;
}

static int
dissect_FMP_NOTIFY_volFreeze_reply(tvbuff_t *tvb,
				   packet_info *pinfo _U_, proto_tree *tree, void* data _U_)
{
	int rval;

	return dissect_fmp_notify_status(tvb, 0, tree, &rval);
}

static int
dissect_FMP_NOTIFY_revokeHandleList_request(tvbuff_t *tvb,
					    packet_info *pinfo, proto_tree *tree, void* data _U_)
{
	int offset = 0;

	offset = dissect_rpc_data(tvb, tree, hf_fmp_sessionHandle,
										  offset);
	offset = dissect_revokeHandleListReason(tvb, offset, tree);
	offset = dissect_handleList(tvb, offset, pinfo, tree);
	return offset;
}

static int
dissect_FMP_NOTIFY_revokeHandleList_reply(tvbuff_t *tvb,
					  packet_info *pinfo _U_, proto_tree *tree, void* data _U_)
{
	int rval;

	return dissect_fmp_notify_status(tvb, 0, tree, &rval);
}

/*
 * proc number, "proc name", dissect_request, dissect_reply
 */
static const vsff fmp_notify2_proc[] = {
	{ 0,						"NULL",
	  dissect_rpc_void,
	  dissect_rpc_void, },

	{ FMP_NOTIFY_DownGrade,				"DownGrade",
	  dissect_FMP_NOTIFY_DownGrade_request,
	  dissect_FMP_NOTIFY_DownGrade_reply },

	{ FMP_NOTIFY_RevokeList,			"RevokeList",
	  dissect_FMP_NOTIFY_RevokeList_request,
	  dissect_FMP_NOTIFY_RevokeList_reply },

	{ FMP_NOTIFY_RevokeAll,				"RevokeAll",
	  dissect_FMP_NOTIFY_RevokeAll_request,
	  dissect_FMP_NOTIFY_RevokeAll_reply },

	{ FMP_NOTIFY_FileSetEof,			"FileSetEof",
	  dissect_FMP_NOTIFY_FileSetEof_request,
	  dissect_FMP_NOTIFY_FileSetEof_reply },

	{ FMP_NOTIFY_RequestDone,			"RequestDone",
	  dissect_FMP_NOTIFY_RequestDone_request,
	  dissect_FMP_NOTIFY_RequestDone_reply },

	{ FMP_NOTIFY_volFreeze,				"volFreeze",
	  dissect_FMP_NOTIFY_volFreeze_request,
	  dissect_FMP_NOTIFY_volFreeze_reply },

	{ FMP_NOTIFY_revokeHandleList,			"revokeHandleList",
	  dissect_FMP_NOTIFY_revokeHandleList_request,
	  dissect_FMP_NOTIFY_revokeHandleList_reply },

	{ 0,		NULL,		NULL,		NULL }
};

static const rpc_prog_vers_info fmp_notify_vers_info[] = {
	{ FMP_NOTIFY_VERSION_2, fmp_notify2_proc, &hf_fmp_notify_procedure }
};

static const value_string fmp_notify_proc_vals[] = {
	{ 0, "NULL" },
	{ 1, "DownGrade" },
	{ 2, "RevokeList" },
	{ 3, "RevokeAll" },
	{ 4, "FileSetEof" },
	{ 5, "RequestDone" },
	{ 6, "VolFreeze" },
	{ 7, "RevokeHandleList" },
	{ 0,NULL}
};

static const value_string fmp_status_vals[] = {
	{  0, "OK"},
	{  5, "IOERROR"},
	{ 12, "NOMEM"},
	{ 13, "NOACCESS"},
	{ 22, "INVALIDARG"},
	{ 28, "FSFULL"},
	{ 79, "QUEUE_FULL"},
	{500, "WRONG_MSG_NUM"},
	{501, "SESSION_LOST"},
	{502, "HOT_SESSION"},
	{503, "COLD_SESSION"},
	{504, "CLIENT_TERMINATED"},
	{505, "WRITER_LOST_BLK"},
	{506, "FMP_REQUEST_QUEUED"},
	{507, "FMP_FALL_BACK"},
	{508, "REQUEST_CANCELLED"},
	{509, "WRITER_ZEROED_BLK"},
	{510, "NOTIFY_ERROR"},
	{511, "FMP_WRONG_HANDLE"},
	{512, "DUPLICATE_OPEN"},
	{600, "PLUGIN_NOFUNC"},
	{0,NULL}

};

static const value_string fmp_revokeHandleListReason_vals[] = {
	{FMP_LIST_USER_QUOTA_EXCEEDED,  "LIST_USER_QUOTA_EXCEEDED"},
	{FMP_LIST_GROUP_QUOTA_EXCEEDED, "LIST_GROUP_QUOTA_EXCEEDED"},
	{FMP_LIST_SERVER_RESOURCE_LOW,  "LIST_SERVER_RESOURCE_LOW"},
	{0,NULL}
};

static int
dissect_fmp_notify_extentState(tvbuff_t *tvb, int offset, proto_tree *tree)
{
	offset = dissect_rpc_uint32(tvb, tree, hf_fmp_extent_state,
				    offset);

	return offset;
}

static int
dissect_fmp_notify_extent(tvbuff_t *tvb, int offset, packet_info *pinfo _U_,
		   proto_tree *tree, guint32 ext_num)
{
	proto_tree *extTree;

	extTree = proto_tree_add_subtree_format(tree, tvb, offset, 20 ,
				      ett_fmp_ext, NULL, "Extent (%u)", (guint32) ext_num);

	offset = dissect_rpc_uint32(tvb,  extTree, hf_fmp_firstLogBlk,
				    offset);
	offset = dissect_rpc_uint32(tvb, extTree, hf_fmp_numBlks,
				    offset);
	offset = dissect_rpc_uint32(tvb, extTree, hf_fmp_volID, offset);
	offset = dissect_rpc_uint32(tvb, extTree, hf_fmp_startOffset,
				    offset);
	offset = dissect_fmp_notify_extentState(tvb, offset, extTree);

	return offset;
}


static int
dissect_fmp_notify_extentList(tvbuff_t *tvb, int offset, packet_info *pinfo,
		       proto_tree *tree)
{
	guint32	    numExtents;
	guint32	    totalLength;
	proto_tree *extListTree;
	guint32	    i;

	numExtents = tvb_get_ntohl(tvb, offset);
	totalLength = 4 + (20 * numExtents);

	extListTree =  proto_tree_add_subtree(tree, tvb, offset, totalLength,
					   ett_fmp_extList, NULL, "Extent List");

	offset = dissect_rpc_uint32(tvb, extListTree,
				    hf_fmp_extentList_len, offset);

	for (i = 0; i < numExtents; i++) {
		offset = dissect_fmp_notify_extent(tvb, offset, pinfo, extListTree, i+1);
	}

	return offset;
}

void
proto_register_fmp_notify(void)
{
	static hf_register_info hf[] = {
		{ &hf_fmp_notify_procedure, {
			"Procedure", "fmp_notify.notify_procedure", FT_UINT32, BASE_DEC,
			VALS(fmp_notify_proc_vals) , 0, NULL, HFILL }},	       /* New addition */

		{ &hf_fmp_status, {
			"Status", "fmp_notify.status", FT_UINT32, BASE_DEC,
			VALS(fmp_status_vals), 0, "Reply Status", HFILL }},

		{ &hf_fmp_extentList_len, {
			"Extent List length", "fmp_notify.extentListLength",
			FT_UINT32, BASE_DEC, NULL, 0, NULL, HFILL }},

		{ &hf_fmp_numBlks, {
			"Number Blocks", "fmp_notify.numBlks", FT_UINT32,
			BASE_DEC, NULL, 0, NULL, HFILL }},

		{ &hf_fmp_volID, {
			"Volume ID", "fmp_notify.volID", FT_UINT32,
			BASE_DEC, NULL, 0, NULL, HFILL }},

		{ &hf_fmp_startOffset, {
			"Start Offset", "fmp_notify.startOffset", FT_UINT32,
			BASE_DEC, NULL, 0, NULL, HFILL }},

		{ &hf_fmp_extent_state, {
			"Extent State", "fmp_notify.extentState", FT_UINT32,
			BASE_DEC, NULL, 0, NULL, HFILL }},

		{ &hf_fmp_handleListLen, {
			"Number of File Handles", "fmp_notify.handleListLength",
			FT_UINT32, BASE_DEC, NULL, 0,
			NULL, HFILL }},

		{ &hf_fmp_sessionHandle, {
	    "Session Handle", "fmp_notify.sessHandle", FT_BYTES, BASE_NONE,
	    NULL, 0, NULL, HFILL }},


	{ &hf_fmp_fsID, {
	    "File System ID", "fmp_notify.fsID", FT_UINT32, BASE_HEX,
	    NULL, 0, NULL, HFILL }},

#if 0
	{ &hf_fmp_fsBlkSz, {
	    "FS Block Size", "fmp_notify.fsBlkSz", FT_UINT32, BASE_DEC,
	    NULL, 0, NULL, HFILL }},
#endif

	{ &hf_fmp_numBlksReq, {
	    "Number Blocks Requested", "fmp_notify.numBlksReq", FT_UINT32,
	    BASE_DEC, NULL, 0, NULL, HFILL }},


	{ &hf_fmp_msgNum, {
	    "Message Number", "fmp_notify.msgNum", FT_UINT32, BASE_DEC,
	    NULL, 0, NULL, HFILL }},

	{ &hf_fmp_cookie, {
	    "Cookie", "fmp_notify.cookie", FT_UINT32, BASE_HEX,
	    NULL, 0, "Cookie for FMP_REQUEST_QUEUED Resp", HFILL }},


	{ &hf_fmp_firstLogBlk, {
	    "First Logical Block", "fmp_notify.firstLogBlk", FT_UINT32,
	    BASE_DEC, NULL, 0, "First Logical File Block", HFILL }},


	{ &hf_fmp_fileSize, {
	    "File Size", "fmp_notify.fileSize", FT_UINT64, BASE_DEC,
	    NULL, 0, NULL, HFILL }},

		{ &hf_fmp_fmpFHandle, {
	    "FMP File Handle", "fmp_notify.fmpFHandle",
	    FT_BYTES, BASE_NONE, NULL, 0, NULL,
	    HFILL }},

	{ &hf_fmp_revokeHandleListReason,
	  { "Reason", "fmp.revokeHandleListReason",
	    FT_UINT32, BASE_DEC, VALS(fmp_revokeHandleListReason_vals), 0,
	    NULL, HFILL }},


	};

	static gint *ett[] = {
		&ett_fmp_notify,
		&ett_fmp_notify_hlist,
		&ett_fmp_extList,
		&ett_fmp_ext
	};

	proto_fmp_notify =
		proto_register_protocol("File Mapping Protocol Nofity",
					"FMP/NOTIFY", "fmp_notify");
	proto_register_field_array(proto_fmp_notify, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));

}

void
proto_reg_handoff_fmp_notify(void)
{
	/* Register the protocol as RPC */
	rpc_init_prog(proto_fmp_notify, FMP_NOTIFY_PROG, ett_fmp_notify,
                      G_N_ELEMENTS(fmp_notify_vers_info), fmp_notify_vers_info);
}

/*
 * Editor modelines  -	http://www.wireshark.org/tools/modelines.html
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
