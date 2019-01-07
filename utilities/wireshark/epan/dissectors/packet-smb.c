/* packet-smb.c
 * Routines for smb packet dissection
 * Copyright 1999, Richard Sharpe <rsharpe@ns.aus.com>
 * 2001  Rewrite by Ronnie Sahlberg and Guy Harris
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * Copied from packet-pop.c
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
#include <epan/exceptions.h>
#include <epan/strutil.h>
#include <epan/prefs.h>
#include <epan/reassemble.h>
#include <epan/tap.h>
#include <epan/srt_table.h>
#include <epan/expert.h>
#include <epan/to_str.h>

#include "packet-windows-common.h"
#include "packet-smb.h"
#include "packet-ipx.h"
#include "packet-idp.h"
#include "packet-smb-common.h"
#include "packet-smb-mailslot.h"
#include "packet-smb-pipe.h"
#include "packet-ntlmssp.h"
#include "packet-smb2.h"

void proto_register_smb(void);
void proto_reg_handoff_smb(void);

/*
 * Various specifications and documents about SMB can be found in
 *
 *	ftp://ftp.microsoft.com/developr/drg/CIFS/
 *
 * and a CIFS specification from the Storage Networking Industry Association
 * can be found on a link from the page at
 *
 *	http://www.snia.org/tech_activities/CIFS
 *
 * (it supercedes the document at
 *
 *	ftp://ftp.microsoft.com/developr/drg/CIFS/draft-leach-cifs-v1-spec-01.txt
 *
 * ).
 *
 * There are also some Open Group publications documenting CIFS available
 * for download; catalog entries for them are at:
 *
 *	http://www.opengroup.org/products/publications/catalog/c209.htm
 *
 *	http://www.opengroup.org/products/publications/catalog/c195.htm
 *
 * The document "NT LAN Manager SMB File Sharing Protocol Extensions"
 * can be found at
 *
 *	http://www.samba.org/samba/ftp/specs/smb-nt01.doc
 *
 * (or, presumably a similar path under the Samba mirrors).  As the
 * ".doc" indicates, it's a Word document.  Some of the specs from the
 * Microsoft FTP site can be found in the
 *
 *	http://www.samba.org/samba/ftp/specs/
 *
 * directory as well.
 *
 * Beware - these specs may have errors.
 *
 * Microsoft's public protocol specifications, including MS-CIFS and
 * other SMB-related specifications, can be found at
 *
 *	http://msdn.microsoft.com/en-us/library/cc216513.aspx
 *
 * See also
 *
 *	https://wiki.samba.org/index.php/UNIX_Extensions
 */

/* DFS referral entry flags */
#define REFENT_FLAGS_NAME_LIST_REFERRAL  0x0002
#define REFENT_FLAGS_TARGET_SET_BOUNDARY 0x0004


static int proto_smb = -1;
static int hf_smb_cmd = -1;
static int hf_smb_andxcmd = -1;
static int hf_smb_mapped_in = -1;
static int hf_smb_unmapped_in = -1;
static int hf_smb_opened_in = -1;
static int hf_smb_closed_in = -1;
static int hf_smb_key = -1;
static int hf_smb_session_id = -1;
static int hf_smb_sequence_num = -1;
static int hf_smb_group_id = -1;
static int hf_smb_pid = -1;
static int hf_smb_tid = -1;
static int hf_smb_uid = -1;
static int hf_smb_mid = -1;
static int hf_smb_pid_high = -1;
static int hf_smb_sig = -1;
static int hf_smb_response_to = -1;
static int hf_smb_time = -1;
static int hf_smb_response_in = -1;
static int hf_smb_continuation_to = -1;
static int hf_smb_nt_status = -1;
static int hf_smb_error_class = -1;
static int hf_smb_error_code = -1;
static int hf_smb_reserved = -1;
static int hf_smb_create_flags = -1;
static int hf_smb_create_options = -1;
static int hf_smb_share_access = -1;
static int hf_smb_access_mask = -1;
static int hf_smb_flags = -1;
static int hf_smb_flags_lock = -1;
static int hf_smb_flags_receive_buffer = -1;
static int hf_smb_flags_caseless = -1;
static int hf_smb_flags_canon = -1;
static int hf_smb_flags_oplock = -1;
static int hf_smb_flags_notify = -1;
static int hf_smb_flags_response = -1;
static int hf_smb_flags2 = -1;
static int hf_smb_flags2_long_names_allowed = -1;
static int hf_smb_flags2_ea = -1;
static int hf_smb_flags2_sec_sig = -1;
static int hf_smb_flags2_compressed = -1;
static int hf_smb_flags2_sec_sig_required = -1;
static int hf_smb_flags2_long_names_used = -1;
static int hf_smb_flags2_reparse_path = -1;
static int hf_smb_flags2_esn = -1;
static int hf_smb_flags2_dfs = -1;
static int hf_smb_flags2_roe = -1;
static int hf_smb_flags2_nt_error = -1;
static int hf_smb_flags2_string = -1;
static int hf_smb_word_count = -1;
static int hf_smb_byte_count = -1;
static int hf_smb_buffer_format = -1;
static int hf_smb_dialect = -1;
static int hf_smb_dialect_name = -1;
static int hf_smb_dialect_index = -1;
static int hf_smb_max_trans_buf_size = -1;
static int hf_smb_max_mpx_count = -1;
static int hf_smb_max_vcs_num = -1;
static int hf_smb_session_key = -1;
static int hf_smb_server_timezone = -1;
static int hf_smb_challenge_length = -1;
static int hf_smb_challenge = -1;
static int hf_smb_primary_domain = -1;
static int hf_smb_server = -1;
static int hf_smb_max_raw_buf_size = -1;
static int hf_smb_server_guid = -1;
static int hf_smb_volume_guid = -1;
static int hf_smb_security_blob_len = -1;
static int hf_smb_security_blob = -1;
static int hf_smb_sm16 = -1;
static int hf_smb_sm_mode16 = -1;
static int hf_smb_sm_password16 = -1;
static int hf_smb_sm = -1;
static int hf_smb_sm_mode = -1;
static int hf_smb_sm_password = -1;
static int hf_smb_sm_signatures = -1;
static int hf_smb_sm_sig_required = -1;
static int hf_smb_rm = -1;
static int hf_smb_rm_read = -1;
static int hf_smb_rm_write = -1;
static int hf_smb_server_date_time = -1;
static int hf_smb_server_smb_date = -1;
static int hf_smb_server_smb_time = -1;
static int hf_smb_server_cap = -1;
static int hf_smb_server_cap_raw_mode = -1;
static int hf_smb_server_cap_mpx_mode = -1;
static int hf_smb_server_cap_unicode = -1;
static int hf_smb_server_cap_large_files = -1;
static int hf_smb_server_cap_nt_smbs = -1;
static int hf_smb_server_cap_rpc_remote_apis = -1;
static int hf_smb_server_cap_nt_status = -1;
static int hf_smb_server_cap_level_ii_oplocks = -1;
static int hf_smb_server_cap_lock_and_read = -1;
static int hf_smb_server_cap_nt_find = -1;
static int hf_smb_server_cap_dfs = -1;
static int hf_smb_server_cap_infolevel_passthru = -1;
static int hf_smb_server_cap_large_readx = -1;
static int hf_smb_server_cap_large_writex = -1;
static int hf_smb_server_cap_lwio = -1;
static int hf_smb_server_cap_unix = -1;
static int hf_smb_server_cap_compressed_data = -1;
static int hf_smb_server_cap_dynamic_reauth = -1;
static int hf_smb_server_cap_extended_security = -1;
static int hf_smb_system_time = -1;
static int hf_smb_unknown = -1;
static int hf_smb_dir_name = -1;
static int hf_smb_echo_count = -1;
static int hf_smb_echo_data = -1;
static int hf_smb_echo_seq_num = -1;
static int hf_smb_max_buf_size = -1;
static int hf_smb_password = -1;
static int hf_smb_password_len = -1;
static int hf_smb_ansi_password = -1;
static int hf_smb_ansi_password_len = -1;
static int hf_smb_unicode_password = -1;
static int hf_smb_unicode_password_len = -1;
static int hf_smb_path = -1;
static int hf_smb_service = -1;
static int hf_smb_move_flags = -1;
static int hf_smb_move_flags_file = -1;
static int hf_smb_move_flags_dir = -1;
static int hf_smb_move_flags_verify = -1;
static int hf_smb_files_moved = -1;
static int hf_smb_file_access_mask_read_data = -1;
static int hf_smb_file_access_mask_write_data = -1;
static int hf_smb_file_access_mask_append_data = -1;
static int hf_smb_file_access_mask_read_ea = -1;
static int hf_smb_file_access_mask_write_ea = -1;
static int hf_smb_file_access_mask_execute = -1;
static int hf_smb_file_access_mask_read_attribute = -1;
static int hf_smb_file_access_mask_write_attribute = -1;
static int hf_smb_dir_access_mask_list = -1;
static int hf_smb_dir_access_mask_add_file = -1;
static int hf_smb_dir_access_mask_add_subdir = -1;
static int hf_smb_dir_access_mask_read_ea = -1;
static int hf_smb_dir_access_mask_write_ea = -1;
static int hf_smb_dir_access_mask_traverse = -1;
static int hf_smb_dir_access_mask_delete_child = -1;
static int hf_smb_dir_access_mask_read_attribute = -1;
static int hf_smb_dir_access_mask_write_attribute = -1;
static int hf_smb_copy_flags = -1;
static int hf_smb_copy_flags_file = -1;
static int hf_smb_copy_flags_dir = -1;
static int hf_smb_copy_flags_dest_mode = -1;
static int hf_smb_copy_flags_source_mode = -1;
static int hf_smb_copy_flags_verify = -1;
static int hf_smb_copy_flags_tree_copy = -1;
static int hf_smb_copy_flags_ea_action = -1;
static int hf_smb_count = -1;
static int hf_smb_count_low = -1;
static int hf_smb_count_high = -1;
static int hf_smb_file_name = -1;
static int hf_smb_open_function = -1;
static int hf_smb_open_function_open = -1;
static int hf_smb_open_function_create = -1;
static int hf_smb_fid = -1;
static int hf_smb_file_attr_16bit = -1;
static int hf_smb_file_attr_8bit = -1;
static int hf_smb_file_attr_read_only_16bit = -1;
static int hf_smb_file_attr_read_only_8bit = -1;
static int hf_smb_file_attr_hidden_16bit = -1;
static int hf_smb_file_attr_hidden_8bit = -1;
static int hf_smb_file_attr_system_16bit = -1;
static int hf_smb_file_attr_system_8bit = -1;
static int hf_smb_file_attr_volume_16bit = -1;
static int hf_smb_file_attr_volume_8bit = -1;
static int hf_smb_file_attr_directory_16bit = -1;
static int hf_smb_file_attr_directory_8bit = -1;
static int hf_smb_file_attr_archive_16bit = -1;
static int hf_smb_file_attr_archive_8bit = -1;
#if 0
static int hf_smb_file_attr_device = -1;
static int hf_smb_file_attr_normal = -1;
static int hf_smb_file_attr_temporary = -1;
static int hf_smb_file_attr_sparse = -1;
static int hf_smb_file_attr_reparse = -1;
static int hf_smb_file_attr_compressed = -1;
static int hf_smb_file_attr_offline = -1;
static int hf_smb_file_attr_not_content_indexed = -1;
static int hf_smb_file_attr_encrypted = -1;
#endif
static int hf_smb_file_size = -1;
static int hf_smb_search_attribute = -1;
static int hf_smb_search_attribute_read_only = -1;
static int hf_smb_search_attribute_hidden = -1;
static int hf_smb_search_attribute_system = -1;
static int hf_smb_search_attribute_volume = -1;
static int hf_smb_search_attribute_directory = -1;
static int hf_smb_search_attribute_archive = -1;
static int hf_smb_access_mode = -1;
static int hf_smb_access_sharing = -1;
static int hf_smb_access_locality = -1;
static int hf_smb_access_caching = -1;
static int hf_smb_access_writetru = -1;
static int hf_smb_desired_access = -1;
static int hf_smb_granted_access = -1;
static int hf_smb_create_time = -1;
static int hf_smb_modify_time = -1;
static int hf_smb_backup_time = -1;
static int hf_smb_mac_alloc_block_count = -1;
static int hf_smb_mac_alloc_block_size = -1;
static int hf_smb_mac_free_block_count = -1;
static int hf_smb_mac_fndrinfo = -1;
static int hf_smb_mac_root_file_count = -1;
static int hf_smb_mac_root_dir_count = -1;
static int hf_smb_mac_file_count = -1;
static int hf_smb_mac_dir_count = -1;
static int hf_smb_mac_sup = -1;
static int hf_smb_mac_sup_access_ctrl = -1;
static int hf_smb_mac_sup_getset_comments = -1;
static int hf_smb_mac_sup_desktopdb_calls = -1;
static int hf_smb_mac_sup_unique_ids = -1;
static int hf_smb_mac_sup_streams = -1;
static int hf_smb_create_dos_date = -1;
static int hf_smb_create_dos_time = -1;
static int hf_smb_last_write_time = -1;
static int hf_smb_last_write_dos_date = -1;
static int hf_smb_last_write_dos_time = -1;
static int hf_smb_access_time = -1;
static int hf_smb_access_dos_date = -1;
static int hf_smb_access_dos_time = -1;
static int hf_smb_old_file_name = -1;
static int hf_smb_offset = -1;
static int hf_smb_remaining = -1;
static int hf_smb_padding = -1;
static int hf_smb_file_data = -1;
/* static int hf_smb_raw_ea_data = -1; */
static int hf_smb_total_data_len = -1;
static int hf_smb_data_len = -1;
static int hf_smb_data_len_low = -1;
static int hf_smb_data_len_high = -1;
static int hf_smb_seek_mode = -1;
static int hf_smb_data_size = -1;
static int hf_smb_alloc_size = -1;
static int hf_smb_alloc_size64 = -1;
static int hf_smb_max_count = -1;
static int hf_smb_max_count_low = -1;
static int hf_smb_max_count_high = -1;
static int hf_smb_min_count = -1;
static int hf_smb_timeout = -1;
static int hf_smb_high_offset = -1;
static int hf_smb_units = -1;
static int hf_smb_bpu = -1;
static int hf_smb_blocksize = -1;
static int hf_smb_freeunits = -1;
static int hf_smb_data_offset = -1;
static int hf_smb_dcm = -1;
static int hf_smb_request_mask = -1;
static int hf_smb_response_mask = -1;
static int hf_smb_search_id = -1;
static int hf_smb_write_mode = -1;
static int hf_smb_write_mode_write_through = -1;
static int hf_smb_write_mode_return_remaining = -1;
static int hf_smb_write_mode_raw = -1;
static int hf_smb_write_mode_message_start = -1;
static int hf_smb_write_mode_connectionless = -1;
static int hf_smb_resume_key_len = -1;
static int hf_smb_resume_find_id = -1;
static int hf_smb_resume_server_cookie = -1;
static int hf_smb_resume_client_cookie = -1;
static int hf_smb_andxoffset = -1;
static int hf_smb_lock_type = -1;
static int hf_smb_lock_type_large = -1;
static int hf_smb_lock_type_cancel = -1;
static int hf_smb_lock_type_change = -1;
static int hf_smb_lock_type_oplock = -1;
static int hf_smb_lock_type_shared = -1;
static int hf_smb_locking_ol = -1;
static int hf_smb_number_of_locks = -1;
static int hf_smb_number_of_unlocks = -1;
static int hf_smb_lock_long_offset = -1;
static int hf_smb_lock_long_length = -1;
static int hf_smb_file_type = -1;
static int hf_smb_ipc_state = -1;
static int hf_smb_ipc_state_nonblocking = -1;
static int hf_smb_ipc_state_endpoint = -1;
static int hf_smb_ipc_state_pipe_type = -1;
static int hf_smb_ipc_state_read_mode = -1;
static int hf_smb_ipc_state_icount = -1;
static int hf_smb_server_fid = -1;
static int hf_smb_open_flags = -1;
static int hf_smb_open_flags_add_info = -1;
static int hf_smb_open_flags_ex_oplock = -1;
static int hf_smb_open_flags_batch_oplock = -1;
static int hf_smb_open_flags_ealen = -1;
static int hf_smb_open_action = -1;
static int hf_smb_open_action_open = -1;
static int hf_smb_open_action_lock = -1;
static int hf_smb_vc_num = -1;
static int hf_smb_account = -1;
static int hf_smb_os = -1;
static int hf_smb_lanman = -1;
static int hf_smb_setup_action = -1;
static int hf_smb_setup_action_guest = -1;
static int hf_smb_fs = -1;
static int hf_smb_connect_flags = -1;
static int hf_smb_connect_flags_dtid = -1;
static int hf_smb_connect_flags_ext_sig = -1;
static int hf_smb_connect_flags_ext_resp = -1;
static int hf_smb_connect_support = -1;
static int hf_smb_connect_support_search = -1;
static int hf_smb_connect_support_in_dfs = -1;
static int hf_smb_connect_support_csc_mask_vals = -1;
static int hf_smb_connect_support_uniquefilename = -1;
static int hf_smb_connect_support_extended_signature = -1;
static int hf_smb_max_setup_count = -1;
static int hf_smb_total_param_count = -1;
static int hf_smb_total_data_count = -1;
static int hf_smb_max_param_count = -1;
static int hf_smb_max_data_count = -1;
static int hf_smb_param_disp16 = -1;
static int hf_smb_param_count16 = -1;
static int hf_smb_param_offset16 = -1;
static int hf_smb_param_disp32 = -1;
static int hf_smb_param_count32 = -1;
static int hf_smb_param_offset32 = -1;
static int hf_smb_data_disp16 = -1;
static int hf_smb_data_count16 = -1;
static int hf_smb_data_offset16 = -1;
static int hf_smb_data_disp32 = -1;
static int hf_smb_data_count32 = -1;
static int hf_smb_data_offset32 = -1;
static int hf_smb_setup_count = -1;
static int hf_smb_nt_trans_subcmd = -1;
static int hf_smb_nt_ioctl_isfsctl = -1;
static int hf_smb_nt_ioctl_flags_completion_filter = -1;
static int hf_smb_nt_ioctl_flags_root_handle = -1;
static int hf_smb_nt_notify_action = -1;
static int hf_smb_nt_notify_watch_tree = -1;
static int hf_smb_nt_notify_completion_filter = -1;
static int hf_smb_nt_notify_stream_write = -1;
static int hf_smb_nt_notify_stream_size = -1;
static int hf_smb_nt_notify_stream_name = -1;
static int hf_smb_nt_notify_security = -1;
static int hf_smb_nt_notify_ea = -1;
static int hf_smb_nt_notify_creation = -1;
static int hf_smb_nt_notify_last_access = -1;
static int hf_smb_nt_notify_last_write = -1;
static int hf_smb_nt_notify_size = -1;
static int hf_smb_nt_notify_attributes = -1;
static int hf_smb_nt_notify_dir_name = -1;
static int hf_smb_nt_notify_file_name = -1;
static int hf_smb_root_dir_fid = -1;
static int hf_smb_nt_create_disposition = -1;
static int hf_smb_sd_length = -1;
static int hf_smb_ea_list_length = -1;
static int hf_smb_ea_flags = -1;
static int hf_smb_ea_name_length = -1;
static int hf_smb_ea_data_length = -1;
static int hf_smb_ea_name = -1;
static int hf_smb_ea_data = -1;
static int hf_smb_file_name_len = -1;
static int hf_smb_nt_impersonation_level = -1;
static int hf_smb_nt_security_flags = -1;
static int hf_smb_nt_security_flags_context_tracking = -1;
static int hf_smb_nt_security_flags_effective_only = -1;
static int hf_smb_nt_access_mask_generic_read = -1;
static int hf_smb_nt_access_mask_generic_write = -1;
static int hf_smb_nt_access_mask_generic_execute = -1;
static int hf_smb_nt_access_mask_generic_all = -1;
static int hf_smb_nt_access_mask_maximum_allowed = -1;
static int hf_smb_nt_access_mask_system_security = -1;
static int hf_smb_nt_access_mask_synchronize = -1;
static int hf_smb_nt_access_mask_write_owner = -1;
static int hf_smb_nt_access_mask_write_dac = -1;
static int hf_smb_nt_access_mask_read_control = -1;
static int hf_smb_nt_access_mask_delete = -1;
static int hf_smb_nt_access_mask_write_attributes = -1;
static int hf_smb_nt_access_mask_read_attributes = -1;
static int hf_smb_nt_access_mask_delete_child = -1;
static int hf_smb_nt_access_mask_execute = -1;
static int hf_smb_nt_access_mask_write_ea = -1;
static int hf_smb_nt_access_mask_read_ea = -1;
static int hf_smb_nt_access_mask_append = -1;
static int hf_smb_nt_access_mask_write = -1;
static int hf_smb_nt_access_mask_read = -1;
static int hf_smb_nt_create_bits_oplock = -1;
static int hf_smb_nt_create_bits_boplock = -1;
static int hf_smb_nt_create_bits_dir = -1;
static int hf_smb_nt_create_bits_ext_resp = -1;
static int hf_smb_nt_create_options_directory_file = -1;
static int hf_smb_nt_create_options_write_through = -1;
static int hf_smb_nt_create_options_sequential_only = -1;
static int hf_smb_nt_create_options_no_intermediate_buffering = -1;
static int hf_smb_nt_create_options_sync_io_alert = -1;
static int hf_smb_nt_create_options_sync_io_nonalert = -1;
static int hf_smb_nt_create_options_non_directory_file = -1;
static int hf_smb_nt_create_options_create_tree_connection = -1;
static int hf_smb_nt_create_options_complete_if_oplocked = -1;
static int hf_smb_nt_create_options_no_ea_knowledge = -1;
static int hf_smb_nt_create_options_eight_dot_three_only = -1;
static int hf_smb_nt_create_options_random_access = -1;
static int hf_smb_nt_create_options_delete_on_close = -1;
static int hf_smb_nt_create_options_open_by_fileid = -1;
static int hf_smb_nt_create_options_backup_intent = -1;
static int hf_smb_nt_create_options_no_compression = -1;
static int hf_smb_nt_create_options_reserve_opfilter = -1;
static int hf_smb_nt_create_options_open_reparse_point = -1;
static int hf_smb_nt_create_options_open_no_recall = -1;
static int hf_smb_nt_create_options_open_for_free_space_query = -1;
static int hf_smb_nt_share_access_read = -1;
static int hf_smb_nt_share_access_write = -1;
static int hf_smb_nt_share_access_delete = -1;
static int hf_smb_file_eattr = -1;
static int hf_smb_file_eattr_read_only = -1;
static int hf_smb_file_eattr_hidden = -1;
static int hf_smb_file_eattr_system = -1;
static int hf_smb_file_eattr_volume = -1;
static int hf_smb_file_eattr_directory = -1;
static int hf_smb_file_eattr_archive = -1;
static int hf_smb_file_eattr_device = -1;
static int hf_smb_file_eattr_normal = -1;
static int hf_smb_file_eattr_temporary = -1;
static int hf_smb_file_eattr_sparse = -1;
static int hf_smb_file_eattr_reparse = -1;
static int hf_smb_file_eattr_compressed = -1;
static int hf_smb_file_eattr_offline = -1;
static int hf_smb_file_eattr_not_content_indexed = -1;
static int hf_smb_file_eattr_encrypted = -1;
static int hf_smb_size_returned_quota_data = -1;
static int hf_smb_sec_desc_len = -1;
static int hf_smb_nt_qsd = -1;
static int hf_smb_nt_qsd_owner = -1;
static int hf_smb_nt_qsd_group = -1;
static int hf_smb_nt_qsd_dacl = -1;
static int hf_smb_nt_qsd_sacl = -1;
static int hf_smb_extended_attributes = -1;
static int hf_smb_oplock_level = -1;
static int hf_smb_create_action = -1;
static int hf_smb_file_id = -1;
static int hf_smb_file_id_64bit = -1;
static int hf_smb_ea_error_offset = -1;
static int hf_smb_end_of_file = -1;
static int hf_smb_replace = -1;
static int hf_smb_root_dir_handle = -1;
static int hf_smb_target_name_len = -1;
static int hf_smb_target_name = -1;
static int hf_smb_device_type = -1;
static int hf_smb_is_directory = -1;
static int hf_smb_next_entry_offset = -1;
static int hf_smb_change_time = -1;
static int hf_smb_setup_len = -1;
static int hf_smb_print_mode = -1;
static int hf_smb_print_identifier = -1;
static int hf_smb_restart_index = -1;
static int hf_smb_print_queue_date = -1;
static int hf_smb_print_queue_dos_date = -1;
static int hf_smb_print_queue_dos_time = -1;
static int hf_smb_print_status = -1;
static int hf_smb_print_spool_file_number = -1;
static int hf_smb_print_spool_file_size = -1;
static int hf_smb_print_spool_file_name = -1;
static int hf_smb_start_index = -1;
static int hf_smb_originator_name = -1;
static int hf_smb_destination_name = -1;
static int hf_smb_message_len = -1;
static int hf_smb_message = -1;
static int hf_smb_mgid = -1;
static int hf_smb_forwarded_name = -1;
static int hf_smb_machine_name = -1;
static int hf_smb_cancel_to = -1;
static int hf_smb_trans2_subcmd = -1;
static int hf_smb_trans_name = -1;
static int hf_smb_transaction_flags = -1;
static int hf_smb_transaction_flags_dtid = -1;
static int hf_smb_transaction_flags_owt = -1;
static int hf_smb_search_count = -1;
static int hf_smb_search_pattern = -1;
static int hf_smb_ff2 = -1;
static int hf_smb_ff2_backup = -1;
static int hf_smb_ff2_continue = -1;
static int hf_smb_ff2_resume = -1;
static int hf_smb_ff2_close_eos = -1;
static int hf_smb_ff2_close = -1;
static int hf_smb_ff2_information_level = -1;
static int hf_smb_qpi_loi = -1;
static int hf_smb_spi_loi = -1;
#if 0
static int hf_smb_sfi = -1;
static int hf_smb_sfi_writetru = -1;
static int hf_smb_sfi_caching = -1;
#endif
static int hf_smb_storage_type = -1;
static int hf_smb_resume = -1;
static int hf_smb_max_referral_level = -1;
static int hf_smb_qfsi_information_level = -1;
static int hf_smb_sfsi_information_level = -1;
static int hf_smb_number_of_links = -1;
static int hf_smb_delete_pending = -1;
static int hf_smb_index_number = -1;
static int hf_smb_position = -1;
/* static int hf_smb_current_offset = -1; */
static int hf_smb_t2_alignment = -1;
static int hf_smb_t2_stream_name_length = -1;
static int hf_smb_t2_stream_size = -1;
static int hf_smb_t2_stream_name = -1;
static int hf_smb_t2_compressed_file_size = -1;
static int hf_smb_t2_compressed_format = -1;
static int hf_smb_t2_compressed_unit_shift = -1;
static int hf_smb_t2_compressed_chunk_shift = -1;
static int hf_smb_t2_compressed_cluster_shift = -1;
static int hf_smb_t2_marked_for_deletion = -1;
static int hf_smb_dfs_path_consumed = -1;
static int hf_smb_dfs_num_referrals = -1;
static int hf_smb_get_dfs_flags = -1;
static int hf_smb_get_dfs_server_hold_storage = -1;
static int hf_smb_get_dfs_fielding = -1;
static int hf_smb_dfs_referral_version = -1;
static int hf_smb_dfs_referral_size = -1;
static int hf_smb_dfs_referral_server_type = -1;
static int hf_smb_dfs_referral_flags = -1;
static int hf_smb_dfs_referral_flags_name_list_referral = -1;
static int hf_smb_dfs_referral_flags_target_set_boundary = -1;
static int hf_smb_dfs_referral_node_offset = -1;
static int hf_smb_dfs_referral_node = -1;
static int hf_smb_dfs_referral_proximity = -1;
static int hf_smb_dfs_referral_ttl = -1;
static int hf_smb_dfs_referral_path_offset = -1;
static int hf_smb_dfs_referral_path = -1;
static int hf_smb_dfs_referral_alt_path_offset = -1;
static int hf_smb_dfs_referral_alt_path = -1;
static int hf_smb_dfs_referral_domain_offset = -1;
static int hf_smb_dfs_referral_number_of_expnames = -1;
static int hf_smb_dfs_referral_expnames_offset = -1;
static int hf_smb_dfs_referral_domain_name = -1;
static int hf_smb_dfs_referral_expname = -1;
static int hf_smb_dfs_referral_server_guid = -1;
static int hf_smb_end_of_search = -1;
static int hf_smb_last_name_offset = -1;
static int hf_smb_fn_information_level = -1;
static int hf_smb_monitor_handle = -1;
static int hf_smb_change_count = -1;
static int hf_smb_file_index = -1;
static int hf_smb_short_file_name = -1;
static int hf_smb_short_file_name_len = -1;
static int hf_smb_fs_id = -1;
static int hf_smb_sector_unit = -1;
static int hf_smb_fs_units = -1;
static int hf_smb_fs_sector = -1;
static int hf_smb_avail_units = -1;
static int hf_smb_volume_serial_num = -1;
static int hf_smb_volume_label_len = -1;
static int hf_smb_volume_label = -1;
static int hf_smb_free_alloc_units64 = -1;
static int hf_smb_caller_free_alloc_units64 = -1;
static int hf_smb_actual_free_alloc_units64 = -1;
static int hf_smb_max_name_len = -1;
static int hf_smb_fs_name_len = -1;
static int hf_smb_fs_name = -1;
static int hf_smb_device_char = -1;
static int hf_smb_device_char_removable = -1;
static int hf_smb_device_char_read_only = -1;
static int hf_smb_device_char_floppy = -1;
static int hf_smb_device_char_write_once = -1;
static int hf_smb_device_char_remote = -1;
static int hf_smb_device_char_mounted = -1;
static int hf_smb_device_char_virtual = -1;
static int hf_smb_fs_attr = -1;
static int hf_smb_fs_attr_css = -1;
static int hf_smb_fs_attr_cpn = -1;
static int hf_smb_fs_attr_uod = -1;
static int hf_smb_fs_attr_pacls = -1;
static int hf_smb_fs_attr_fc = -1;
static int hf_smb_fs_attr_vq = -1;
static int hf_smb_fs_attr_ssf = -1;
static int hf_smb_fs_attr_srp = -1;
static int hf_smb_fs_attr_srs = -1;
static int hf_smb_fs_attr_sla = -1;
static int hf_smb_fs_attr_vic = -1;
static int hf_smb_fs_attr_soids = -1;
static int hf_smb_fs_attr_se = -1;
static int hf_smb_fs_attr_ns = -1;
static int hf_smb_fs_attr_rov = -1;
static int hf_smb_quota_flags = -1;
static int hf_smb_quota_flags_enabled = -1;
static int hf_smb_quota_flags_deny_disk = -1;
static int hf_smb_quota_flags_log_limit = -1;
static int hf_smb_quota_flags_log_warning = -1;
static int hf_smb_soft_quota_limit = -1;
static int hf_smb_hard_quota_limit = -1;
static int hf_smb_user_quota_used = -1;
static int hf_smb_length_of_sid = -1;
static int hf_smb_user_quota_offset = -1;
static int hf_smb_nt_rename_level = -1;
static int hf_smb_cluster_count = -1;
static int hf_smb_segments = -1;
static int hf_smb_segment = -1;
static int hf_smb_segment_overlap = -1;
static int hf_smb_segment_overlap_conflict = -1;
static int hf_smb_segment_multiple_tails = -1;
static int hf_smb_segment_too_long_fragment = -1;
static int hf_smb_segment_error = -1;
static int hf_smb_segment_count = -1;
static int hf_smb_reassembled_length = -1;
static int hf_smb_pipe_write_len = -1;
static int hf_smb_unix_major_version = -1;
static int hf_smb_unix_minor_version = -1;
static int hf_smb_unix_capability = -1;
static int hf_smb_unix_capability_fcntl = -1;
static int hf_smb_unix_capability_posix_acl = -1;
static int hf_smb_unix_capability_xattr = -1;
static int hf_smb_unix_capability_attr = -1;
static int hf_smb_unix_capability_posix_paths = -1;
static int hf_smb_unix_capability_posix_path_ops = -1;
static int hf_smb_unix_capability_large_read = -1;
static int hf_smb_unix_capability_large_write = -1;
static int hf_smb_unix_capability_encrpytion = -1;
static int hf_smb_unix_capability_mandatory_crypto = -1;
static int hf_smb_unix_capability_proxy = -1;
static int hf_smb_unix_file_link_dest = -1;
static int hf_smb_unix_file_size = -1;
static int hf_smb_unix_file_num_bytes = -1;
static int hf_smb_unix_file_last_status = -1;
static int hf_smb_unix_file_last_access = -1;
static int hf_smb_unix_file_last_change = -1;
static int hf_smb_unix_file_creation_time = -1;
static int hf_smb_unix_file_uid = -1;
static int hf_smb_unix_file_gid = -1;
static int hf_smb_unix_file_type = -1;
static int hf_smb_unix_file_dev_major = -1;
static int hf_smb_unix_file_dev_minor = -1;
static int hf_smb_unix_file_unique_id = -1;
static int hf_smb_unix_file_permissions = -1;
static int hf_smb_unix_file_nlinks = -1;
static int hf_smb_unix_info2_file_flags = -1;
static int hf_smb_unix_info2_file_flags_mask = -1;
static int hf_smb_unix_info2_file_flags_secure_delete = -1;
static int hf_smb_unix_info2_file_flags_enable_undelete = -1;
static int hf_smb_unix_info2_file_flags_synchronous = -1;
static int hf_smb_unix_info2_file_flags_immutable = -1;
static int hf_smb_unix_info2_file_flags_append_only = -1;
static int hf_smb_unix_info2_file_flags_do_not_backup = -1;
static int hf_smb_unix_info2_file_flags_no_update_atime = -1;
static int hf_smb_unix_info2_file_flags_hidden = -1;
static int hf_smb_unix_file_name_length = -1;
static int hf_smb_unix_file_name = -1;
static int hf_smb_unix_find_file_nextoffset = -1;
static int hf_smb_unix_find_file_resumekey = -1;
static int hf_smb_unix_whoami_mapflags = -1;
static int hf_smb_unix_whoami_mapflags_mask = -1;
static int hf_smb_unix_whoami_num_supl_gids = -1;
static int hf_smb_unix_whoami_num_supl_sids = -1;
static int hf_smb_unix_whoami_sids_buflen = -1;
static int hf_smb_disposition_delete_on_close = -1;
static int hf_smb_pipe_info_flag = -1;
static int hf_smb_mode = -1;
static int hf_smb_attribute = -1;
static int hf_smb_reparse_tag = -1;
static int hf_smb_logged_in = -1;
static int hf_smb_logged_out = -1;
static int hf_smb_file_rw_offset = -1;
static int hf_smb_file_rw_length = -1;
static int hf_smb_posix_acl_version = -1;
static int hf_smb_posix_num_file_aces = -1;
static int hf_smb_posix_num_def_aces = -1;
static int hf_smb_posix_ace_type = -1;
static int hf_smb_posix_ace_flags = -1;
static int hf_smb_posix_ace_perm_read = -1;
static int hf_smb_posix_ace_perm_write = -1;
static int hf_smb_posix_ace_perm_execute = -1;
static int hf_smb_posix_ace_perm_owner_uid = -1;
static int hf_smb_posix_ace_perm_owner_gid = -1;
static int hf_smb_posix_ace_perm_uid = -1;
static int hf_smb_posix_ace_perm_gid = -1;
static int hf_smb_trans_data_setup_word = -1;
static int hf_smb_trans_data_parameters = -1;
static int hf_smb_trans_data = -1;
static int hf_smb_extra_byte_parameters = -1;
static int hf_smb_file_access_mask_full_control = -1;
static int hf_smb_dir_access_mask_full_control = -1;
static int hf_smb_word_unk_response_format = -1;
static int hf_smb_nt_transaction_setup = -1;
static int hf_smb_server_component = -1;
static int hf_smb_byte_parameters = -1;
static int hf_smb_word_parameters = -1;

static gint ett_smb = -1;
static gint ett_smb_fid = -1;
static gint ett_smb_tid = -1;
static gint ett_smb_uid = -1;
static gint ett_smb_hdr = -1;
static gint ett_smb_command = -1;
static gint ett_smb_fileattributes = -1;
static gint ett_smb_capabilities = -1;
static gint ett_smb_aflags = -1;
static gint ett_smb_dialect = -1;
static gint ett_smb_dialects = -1;
static gint ett_smb_mode = -1;
static gint ett_smb_rawmode = -1;
static gint ett_smb_flags = -1;
static gint ett_smb_flags2 = -1;
static gint ett_smb_desiredaccess = -1;
static gint ett_smb_search = -1;
static gint ett_smb_file = -1;
static gint ett_smb_openfunction = -1;
static gint ett_smb_filetype = -1;
static gint ett_smb_openaction = -1;
static gint ett_smb_writemode = -1;
static gint ett_smb_lock_type = -1;
static gint ett_smb_ssetupandxaction = -1;
static gint ett_smb_optionsup = -1;
static gint ett_smb_time_date = -1;
static gint ett_smb_move_copy_flags = -1;
static gint ett_smb_file_attributes = -1;
static gint ett_smb_search_resume_key = -1;
static gint ett_smb_search_dir_info = -1;
static gint ett_smb_unlocks = -1;
static gint ett_smb_unlock = -1;
static gint ett_smb_locks = -1;
static gint ett_smb_lock = -1;
static gint ett_smb_open_flags = -1;
static gint ett_smb_ipc_state = -1;
static gint ett_smb_open_action = -1;
static gint ett_smb_setup_action = -1;
static gint ett_smb_connect_flags = -1;
static gint ett_smb_connect_support_bits = -1;
static gint ett_smb_nt_access_mask = -1;
static gint ett_smb_nt_create_bits = -1;
static gint ett_smb_nt_create_options = -1;
static gint ett_smb_nt_share_access = -1;
static gint ett_smb_nt_security_flags = -1;
static gint ett_smb_nt_trans_setup = -1;
static gint ett_smb_nt_trans_data = -1;
static gint ett_smb_nt_trans_param = -1;
static gint ett_smb_nt_notify_completion_filter = -1;
static gint ett_smb_nt_ioctl_flags = -1;
static gint ett_smb_security_information_mask = -1;
static gint ett_smb_print_queue_entry = -1;
static gint ett_smb_transaction_flags = -1;
static gint ett_smb_transaction_params = -1;
static gint ett_smb_find_first2_flags = -1;
static gint ett_smb_mac_support_flags = -1;
#if 0
static gint ett_smb_ioflag = -1;
#endif
static gint ett_smb_transaction_data = -1;
static gint ett_smb_stream_info = -1;
static gint ett_smb_dfs_referrals = -1;
static gint ett_smb_dfs_referral = -1;
static gint ett_smb_dfs_referral_flags = -1;
static gint ett_smb_dfs_referral_expnames = -1;
static gint ett_smb_get_dfs_flags = -1;
static gint ett_smb_ff2_data = -1;
static gint ett_smb_device_characteristics = -1;
static gint ett_smb_fs_attributes = -1;
static gint ett_smb_segments = -1;
static gint ett_smb_segment = -1;
static gint ett_smb_quotaflags = -1;
static gint ett_smb_secblob = -1;
static gint ett_smb_unicode_password = -1;
static gint ett_smb_ea = -1;
static gint ett_smb_unix_capabilities = -1;
static gint ett_smb_unix_whoami_gids = -1;
static gint ett_smb_unix_whoami_sids = -1;
static gint ett_smb_posix_ace = -1;
static gint ett_smb_posix_ace_perms = -1;
static gint ett_smb_info2_file_flags = -1;

static expert_field ei_smb_mal_information_level = EI_INIT;
static expert_field ei_smb_not_implemented = EI_INIT;
static expert_field ei_smb_nt_transaction_setup = EI_INIT;
static expert_field ei_smb_posix_ace_type = EI_INIT;
static expert_field ei_smb_info_level_unknown = EI_INIT;
static expert_field ei_smb_info_level_not_understood = EI_INIT;

static int smb_tap = -1;
static int smb_eo_tap = -1;

static dissector_handle_t gssapi_handle;
static dissector_handle_t ntlmssp_handle;

static const fragment_items smb_frag_items = {
	&ett_smb_segment,
	&ett_smb_segments,

	&hf_smb_segments,
	&hf_smb_segment,
	&hf_smb_segment_overlap,
	&hf_smb_segment_overlap_conflict,
	&hf_smb_segment_multiple_tails,
	&hf_smb_segment_too_long_fragment,
	&hf_smb_segment_error,
	&hf_smb_segment_count,
	NULL,
	&hf_smb_reassembled_length,
	/* Reassembled data field */
	NULL,
	"segments"
};

static proto_tree *top_tree_global = NULL;     /* ugly */

static int dissect_smb_command(tvbuff_t *tvb, packet_info *pinfo, int offset, proto_tree *smb_tree, guint8 cmd, gboolean first_pdu, smb_info_t *si);

#define SMB_NUM_PROCEDURES     256
#define SMB_SRT_TABLE_INDEX    0
#define TRANS2_SRT_TABLE_INDEX 1
#define NT_SRT_TABLE_INDEX     2

static void
smbstat_init(struct register_srt* srt _U_, GArray* srt_array, srt_gui_init_cb gui_callback, void* gui_data)
{
	srt_stat_table *smb_srt_table;
	srt_stat_table *trans2_srt_table;
	srt_stat_table *nt_srt_table;
	guint32 i;

	smb_srt_table = init_srt_table("SMB Commands", NULL, srt_array, SMB_NUM_PROCEDURES, "Commands", "smb.cmd", gui_callback, gui_data, NULL);
	trans2_srt_table = init_srt_table("Transaction2 Sub-Commands", NULL, srt_array, SMB_NUM_PROCEDURES, "Transaction2 Commands", "smb.trans2.cmd", gui_callback, gui_data, NULL);
	nt_srt_table = init_srt_table("NT Transaction Sub-Commands", NULL, srt_array, SMB_NUM_PROCEDURES, "NT Transaction Sub-Commands", "smb.nt.function", gui_callback, gui_data, NULL);
	for (i = 0; i < SMB_NUM_PROCEDURES; i++)
	{
		init_srt_table_row(smb_srt_table, i, val_to_str_ext_const(i, &smb_cmd_vals_ext, "<unknown>"));
		init_srt_table_row(trans2_srt_table, i, val_to_str_ext_const(i, &trans2_cmd_vals_ext, "<unknown>"));
		init_srt_table_row(nt_srt_table, i, val_to_str_ext_const(i, &nt_cmd_vals_ext, "<unknown>"));
	}
}

static int
smbstat_packet(void *pss, packet_info *pinfo, epan_dissect_t *edt _U_, const void *prv)
{
	guint i = 0;
	srt_stat_table *smb_srt_table;
	srt_data_t *data = (srt_data_t *)pss;
	const smb_info_t *si = (const smb_info_t *)prv;

	/* we are only interested in reply packets */
	if (si->request) {
		return 0;
	}
	/* if we havnt seen the request, just ignore it */
	if (!si->sip) {
		return 0;
	}

	if (si->cmd == 0xA0 && si->sip->extra_info_type == SMB_EI_NTI) {
		smb_nt_transact_info_t *sti = (smb_nt_transact_info_t *)si->sip->extra_info;

		/*nt transaction*/
		if (sti) {
			i = NT_SRT_TABLE_INDEX;
			smb_srt_table = g_array_index(data->srt_array, srt_stat_table*, i);
			add_srt_table_data(smb_srt_table, sti->subcmd, &si->sip->req_time, pinfo);
		}
	} else if (si->cmd == 0x32 && si->sip->extra_info_type == SMB_EI_T2I) {
		smb_transact2_info_t *st2i = (smb_transact2_info_t *)si->sip->extra_info;

		/*transaction2*/
		if (st2i) {
			i = TRANS2_SRT_TABLE_INDEX;
			smb_srt_table = g_array_index(data->srt_array, srt_stat_table*, i);
			add_srt_table_data(smb_srt_table, st2i->subcmd, &si->sip->req_time, pinfo);
		}
	} else {
		i = SMB_SRT_TABLE_INDEX;
		smb_srt_table = g_array_index(data->srt_array, srt_stat_table*, i);
		add_srt_table_data(smb_srt_table, si->cmd, &si->sip->req_time, pinfo);
	}

	return 1;

}

/*
 * Macros for use in the main dissector routines for an SMB.
 */

#define WORD_COUNT	\
	/* Word Count */				\
	wc = tvb_get_guint8(tvb, offset);		\
	proto_tree_add_uint(tree, hf_smb_word_count,	\
		tvb, offset, 1, wc);			\
	offset += 1;					\
	if (wc == 0) goto bytecount;

#define BYTE_COUNT	\
	bytecount:					\
	bc = tvb_get_letohs(tvb, offset);		\
	proto_tree_add_uint(tree, hf_smb_byte_count,	\
			tvb, offset, 2, bc);		\
	offset += 2;					\
	if (bc == 0) goto endofcommand;

#define CHECK_BYTE_COUNT(len)	\
	if (bc < len) goto endofcommand;

#define COUNT_BYTES(len)	{\
	int tmp;		\
	tmp = len;		\
	offset += tmp;		\
	bc -= tmp;			\
	}

#define END_OF_SMB	\
	if (bc != 0) { \
		gint bc_remaining; \
		bc_remaining = tvb_reported_length_remaining(tvb, offset); \
		if ( ((gint)bc) > bc_remaining) { \
			bc = bc_remaining; \
		} \
		if (bc) { \
			proto_tree_add_item(tree, hf_smb_extra_byte_parameters, tvb, offset, bc, ENC_NA);		\
		} \
		offset += bc;				\
	}						\
	endofcommand:

/*
 * Macros for use in routines called by them.
 */
#define CHECK_BYTE_COUNT_SUBR(len)	\
	if (*bcp < len) {		\
		*trunc = TRUE;		\
		return offset;		\
	}

#define CHECK_STRING_SUBR(fn)	\
	if (fn == NULL) {	\
		*trunc = TRUE;	\
		return offset;	\
	}

#define COUNT_BYTES_SUBR(len)	\
	offset += len;		\
	*bcp -= len;

/*
 * Macros for use when dissecting transaction parameters and data
 */
#define CHECK_BYTE_COUNT_TRANS(len)	\
	if (bc < len) return offset;

#define CHECK_STRING_TRANS(fn)	\
	if (fn == NULL) return offset;

#define COUNT_BYTES_TRANS(len)	\
	offset += len;		\
	bc -= len;

/*
 * Macros for use in subrroutines dissecting transaction parameters or data
 */
#define CHECK_BYTE_COUNT_TRANS_SUBR(len)	\
	if (*bcp < len) return offset;

#define CHECK_STRING_TRANS_SUBR(fn)	\
	if (fn == NULL) return offset;

#define COUNT_BYTES_TRANS_SUBR(len)	\
	offset += len;			\
	*bcp -= len;

gboolean sid_display_hex = FALSE;
gboolean sid_name_snooping = FALSE;

/* ExportObject preferences variable */
gboolean eosmb_take_name_as_fid = FALSE ;
/* Utility to get an str reprensenting ipv4 or ipv6 address */
const gchar *tree_ip_str(packet_info *pinfo, guint16 cmd) {
	const gchar	*buf;

	if (	cmd == SMB_COM_READ_ANDX ||
		cmd == SMB_COM_READ ||
		cmd == SMB2_COM_READ) {
		buf = address_to_str(wmem_packet_scope(), &pinfo->src);
	} else {
		buf = address_to_str(wmem_packet_scope(), &pinfo->dst);
	}

	return buf;
}


/* ExportObject feed function*/
static void
feed_eo_smb(guint16 cmd, guint16 fid, tvbuff_t * tvb, packet_info *pinfo, guint16 dataoffset, guint32 datalen, guint32 chunk_len,
			guint64 file_offset, smb_info_t *si) {

	smb_eo_t        *eo_info; /* 	eo_info variable to pass info. to
					export object and aux */
	smb_tid_info_t  *tid_info = NULL;
	smb_fid_info_t  *fid_info = NULL;
	smb_fid_info_t  *suspect_fid_info = NULL;
	tvbuff_t        *data_tvb;
	GSList          *GSL_iterator;

	/* Create a new tvb to point to the payload data */
	data_tvb = tvb_new_subset_length(tvb, dataoffset, datalen);
	/* Create the eo_info to pass to the listener */
	eo_info = wmem_new(wmem_packet_scope(), smb_eo_t);

	/* Try to get fid_info and tid_info */
	if (fid_info == NULL) {
		GSL_iterator = si->ct->GSL_fid_info;
		while (GSL_iterator) {
			suspect_fid_info = (smb_fid_info_t *)GSL_iterator->data;
			if (suspect_fid_info->opened_in > pinfo->num) break;
			if ((suspect_fid_info->tid == si->tid) && (suspect_fid_info->fid == fid))
				fid_info = suspect_fid_info;
			GSL_iterator = g_slist_next(GSL_iterator);
			}
		}


	tid_info = (smb_tid_info_t *)wmem_tree_lookup32(si->ct->tid_tree, si->tid);

	/* Construct the eo_info structure */
	eo_info->smbversion = 1;
	if (tid_info) {
		if (tid_info->filename) {
			eo_info->hostname = tid_info->filename;
		} else {
			eo_info->hostname = wmem_strdup_printf(wmem_packet_scope(), "\\\\%s\\TREEID_UNKNOWN", tree_ip_str(pinfo, cmd));
		}
	}
	else            eo_info->hostname = wmem_strdup_printf(wmem_packet_scope(), "\\\\%s\\TREEID_%i", tree_ip_str(pinfo, cmd), si->tid);
	if (fid_info) {
		eo_info->filename = NULL;
		if (fid_info->fsi)
			if (fid_info->fsi->filename)
				eo_info->filename = (gchar *) fid_info->fsi->filename;
		if (!eo_info->filename) eo_info->filename = wmem_strdup_printf(wmem_packet_scope(), "\\FILEID_%i", fid);
		eo_info->fid_type = fid_info->type;
		eo_info->end_of_file = fid_info->end_of_file;
	} else {
		eo_info->fid_type = SMB_FID_TYPE_UNKNOWN;
		eo_info->filename = wmem_strdup_printf(wmem_packet_scope(), "\\FILEID_%i", fid);
		eo_info->end_of_file = 0;
	}
	if (eosmb_take_name_as_fid) {
		eo_info->fid = g_str_hash(eo_info->filename);
	} else {
		eo_info->fid = fid;
	}
	eo_info->tid = si->tid;
	eo_info->uid = si->uid;
	eo_info->payload_len = datalen;
	eo_info->payload_data = tvb_get_ptr(data_tvb, 0, datalen);
	eo_info->smb_file_offset = file_offset;
	eo_info->smb_chunk_len = chunk_len;
	eo_info->cmd = cmd;
	/* Queue data to the listener */

	tap_queue_packet(smb_eo_tap, pinfo, eo_info);
} /* feed_eo_smb */

/* Compare function to maintain the GSL_fid_info ordered
   Order criteria: packet where the fid was opened */
static gint
fid_cmp(smb_fid_info_t *fida, smb_fid_info_t *fidb)
{
	return (fida->opened_in - fidb->opened_in);
}

/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
   These are needed by the reassembly of SMB Transaction payload and DCERPC over SMB
   XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */
static gboolean smb_trans_reassembly = TRUE;
gboolean smb_dcerpc_reassembly = TRUE;

static reassembly_table smb_trans_reassembly_table;

static void
smb_trans_reassembly_init(void)
{
	/*
	 * XXX - addresses_ports_reassembly_table_functions?
	 * Probably correct for SMB-over-NBT and SMB-over-TCP,
	 * as stuff from two different connections should
	 * probably not be combined, but what about other
	 * transports for SMB, e.g. NBF or Netware?
	 */
	reassembly_table_init(&smb_trans_reassembly_table,
	    &addresses_reassembly_table_functions);
}

static fragment_head *
smb_trans_defragment(proto_tree *tree _U_, packet_info *pinfo, tvbuff_t *tvb,
		     int offset, guint count, guint pos, guint totlen, smb_info_t *si)
{
	fragment_head *fd_head = NULL;
	int            more_frags;

	/* Don't pass the reassembly code data that doesn't exist */
	/* Fail if some or all of the fragment is located beyond the total length */
	if ( !tvb_bytes_exist(tvb, offset, count) || (pos > totlen) || (count > totlen) || ((pos + count) > totlen)) {
		THROW(ReportedBoundsError);
	}

	more_frags = totlen > (pos + count);

	DISSECTOR_ASSERT(si);

	if (si->sip == NULL) {
		/*
		 * We don't have the frame number of the request.
		 */
		return NULL;
	}

	if (!pinfo->fd->flags.visited) {
		fd_head = fragment_add(&smb_trans_reassembly_table, tvb, offset,
				       pinfo, si->sip->frame_req, NULL,
				       pos, count, more_frags);
	} else {
		fd_head = fragment_get(&smb_trans_reassembly_table,
				       pinfo, si->sip->frame_req, NULL);
	}

	if (!fd_head || !(fd_head->flags & FD_DEFRAGMENTED)) {
		/* This is continued - mark it as such, so we recognize
		   continuation responses.
		*/
		si->sip->flags |= SMB_SIF_IS_CONTINUED;
	} else {
		/* We've finished reassembling, so there are no more
		   continuation responses.
		*/
		si->sip->flags &= ~SMB_SIF_IS_CONTINUED;
	}

	/* we only show the defragmented packet for the first fragment,
	   or else we might end up with dissecting one HUGE transaction PDU
	   a LOT of times. (first fragment is the only one containing the setup
	   bytes)
	   I have seen ONE Transaction PDU that is ~60kb, spanning many Transaction
	   SMBs. Takes a LOT of time dissecting and is not fun.
	*/
	if ( (pos == 0) && fd_head && (fd_head->flags & FD_DEFRAGMENTED)) {
		return fd_head;
	} else {
		return NULL;
	}
}





/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
   These variables and functions are used to match
   responses with calls
   XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */
/*
 * The information we need to save about a request in order to show the
 * frame number of the request in the dissection of the reply.
 */
typedef struct  {
	guint32	frame;
	guint32 pid_mid;
} smb_saved_info_key_t;

/* unmatched smb_saved_info structures.
   For unmatched smb_saved_info structures we store the smb_saved_info
   structure using the MID and the PID as the key.

   Oh, yes, the key is really a pointer, but we use it as if it was an integer.
   Ugly, yes. Not portable to DEC-20 Yes. But it saves a few bytes.
   The key is the PID in the upper 16 bits and the MID in the lower 16 bits.
*/
static gint
smb_saved_info_equal_unmatched(gconstpointer k1, gconstpointer k2)
{
	register guint32 key1 = GPOINTER_TO_UINT(k1);
	register guint32 key2 = GPOINTER_TO_UINT(k2);
	return key1 == key2;
}
static guint
smb_saved_info_hash_unmatched(gconstpointer k)
{
	register guint32 key = GPOINTER_TO_UINT(k);
	return key;
}

/* matched smb_saved_info structures.
   For matched smb_saved_info structures we store the smb_saved_info
   structure twice in the table using the frame number, and a combination
   of the MID and the PID, as the key.
   The frame number is guaranteed to be unique but if ever someone makes
   some change that will renumber the frames in a capture we are in BIG trouble.
   This is not likely though since that would break (among other things) all the
   reassembly routines as well.

   We also need the MID as there may be more than one SMB request or reply
   in a single frame, and we also need the PID as there may be more than
   one outstanding request with the same MID and different PIDs.
*/
static gint
smb_saved_info_equal_matched(gconstpointer k1, gconstpointer k2)
{
	const smb_saved_info_key_t *key1 = (const smb_saved_info_key_t *)k1;
	const smb_saved_info_key_t *key2 = (const smb_saved_info_key_t *)k2;
	return (key1->frame == key2->frame) && (key1->pid_mid == key2->pid_mid);
}
static guint
smb_saved_info_hash_matched(gconstpointer k)
{
	const smb_saved_info_key_t *key = (const smb_saved_info_key_t *)k;
	return key->frame + key->pid_mid;
}

static GSList *conv_tables = NULL;


/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
   End of request/response matching functions
   XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */



typedef struct _smb_uid_t {
	char *domain;
	char *account;
	int   logged_in;
	int   logged_out;
} smb_uid_t;

static void
smb_file_specific_rights(tvbuff_t *tvb, gint offset, proto_tree *tree, guint32 mask)
{
	static const int * mask_flags[] = {
		&hf_smb_file_access_mask_write_attribute,
		&hf_smb_file_access_mask_read_attribute,
		&hf_smb_file_access_mask_execute,
		&hf_smb_file_access_mask_write_ea,
		&hf_smb_file_access_mask_read_ea,
		&hf_smb_file_access_mask_append_data,
		&hf_smb_file_access_mask_write_data,
		&hf_smb_file_access_mask_read_data,
		NULL
	};

	mask &= 0x0000ffff;
	if (mask == 0x000001ff) {
		proto_tree_add_uint(tree, hf_smb_file_access_mask_full_control, tvb, offset, 4, mask);
	}

	proto_tree_add_bitmask_list_value(tree, tvb, offset, 4, mask_flags, mask);
}
struct access_mask_info smb_file_access_mask_info = {
	"FILE",				/* Name of specific rights */
	smb_file_specific_rights,	/* Dissection function */
	NULL,				/* Generic mapping table */
	NULL				/* Standard mapping table */
};


static void
smb_dir_specific_rights(tvbuff_t *tvb, gint offset, proto_tree *tree, guint32 mask)
{
	static const int * mask_flags[] = {
		&hf_smb_dir_access_mask_write_attribute,
		&hf_smb_dir_access_mask_read_attribute,
		&hf_smb_dir_access_mask_delete_child,
		&hf_smb_dir_access_mask_traverse,
		&hf_smb_dir_access_mask_write_ea,
		&hf_smb_dir_access_mask_read_ea,
		&hf_smb_dir_access_mask_add_subdir,
		&hf_smb_dir_access_mask_add_file,
		&hf_smb_dir_access_mask_list,
		NULL
	};

	mask &= 0x0000ffff;
	if (mask == 0x000001ff) {
		proto_tree_add_uint(tree, hf_smb_dir_access_mask_full_control, tvb, offset, 4, mask);
	}

	proto_tree_add_bitmask_list_value(tree, tvb, offset, 4, mask_flags, mask);
}
struct access_mask_info smb_dir_access_mask_info = {
	"DIR",				/* Name of specific rights */
	smb_dir_specific_rights,	/* Dissection function */
	NULL,				/* Generic mapping table */
	NULL				/* Standard mapping table */
};



static const value_string buffer_format_vals[] = {
	{1,     "Data Block"},
	{2,     "Dialect"},
	{3,     "Pathname"},
	{4,     "ASCII"},
	{5,     "Variable Block"},
	{0,     NULL}
};

#define POSIX_ACE_TYPE_USER_OBJ		0x01
#define POSIX_ACE_TYPE_USER		0x02
#define POSIX_ACE_TYPE_GROUP_OBJ	0x04
#define POSIX_ACE_TYPE_GROUP		0x08
#define POSIX_ACE_TYPE_MASK		0x10
#define POSIX_ACE_TYPE_OTHER		0x20
static const value_string ace_type_vals[] = {
	{POSIX_ACE_TYPE_USER_OBJ,	"User Obj"},
	{POSIX_ACE_TYPE_USER,		"User"},
	{POSIX_ACE_TYPE_GROUP_OBJ,	"Group Obj"},
	{POSIX_ACE_TYPE_GROUP,		"Group"},
	{POSIX_ACE_TYPE_MASK,		"Mask"},
	{POSIX_ACE_TYPE_OTHER,		"Other"},
	{0,     NULL}
};

/*
 * UTIME - this is *almost* like a UNIX time stamp, except that it's
 * in seconds since January 1, 1970, 00:00:00 *local* time, not since
 * January 1, 1970, 00:00:00 GMT.
 *
 * This means we have to do some extra work to convert it.  This code is
 * based on the Samba code:
 *
 *	Unix SMB/Netbios implementation.
 *	Version 1.9.
 *	time handling functions
 *	Copyright (C) Andrew Tridgell 1992-1998
 */

/*
 * Yield the difference between *A and *B, in seconds, ignoring leap
 * seconds.
 */
#define TM_YEAR_BASE 1900

static int
tm_diff(struct tm *a, struct tm *b)
{
	int ay = a->tm_year + (TM_YEAR_BASE - 1);
	int by = b->tm_year + (TM_YEAR_BASE - 1);
	int intervening_leap_days =
	    (ay/4 - by/4) - (ay/100 - by/100) + (ay/400 - by/400);
	int years = ay - by;
	int days =
	    365*years + intervening_leap_days + (a->tm_yday - b->tm_yday);
	int hours = 24*days + (a->tm_hour - b->tm_hour);
	int minutes = 60*hours + (a->tm_min - b->tm_min);
	int seconds = 60*minutes + (a->tm_sec - b->tm_sec);

	return seconds;
}

/*
 * Return the UTC offset in seconds west of UTC, or 0 if it cannot be
 * determined.
 */
static int
TimeZone(time_t t)
{
	struct tm *tm = gmtime(&t);
	struct tm  tm_utc;

	if (tm == NULL)
		return 0;
	tm_utc = *tm;
	tm = localtime(&t);
	if (tm == NULL)
		return 0;
	return tm_diff(&tm_utc, tm);
}

/*
 * Return the same value as TimeZone, but it should be more efficient.
 *
 * We keep a table of DST offsets to prevent calling localtime() on each
 * call of this function. This saves a LOT of time on many unixes.
 *
 * Updated by Paul Eggert <eggert@twinsun.com>
 */
#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif

#ifndef TIME_T_MIN
#define TIME_T_MIN ((time_t) ((time_t)0 < (time_t) -1 ? (time_t) 0 \
		    : ~ (time_t) 0 << (sizeof (time_t) * CHAR_BIT - 1)))
#endif
#ifndef TIME_T_MAX
#define TIME_T_MAX ((time_t) (~ (time_t) 0 - TIME_T_MIN))
#endif

static int
TimeZoneFaster(time_t t)
{
	static struct dst_table {time_t start, end; int zone;} *tdt;
	static struct dst_table *dst_table = NULL;
	static int table_size = 0;
	int        i;
	int        zone = 0;

	if (t == 0)
		t = time(NULL);

	/* Tunis has a 8 day DST region, we need to be careful ... */
#define MAX_DST_WIDTH (365*24*60*60)
#define MAX_DST_SKIP (7*24*60*60)

	for (i = 0; i < table_size; i++) {
		if ((t >= dst_table[i].start) && (t <= dst_table[i].end))
			break;
	}

	if (i < table_size) {
		zone = dst_table[i].zone;
	} else {
		time_t low, high;

		zone = TimeZone(t);
		if (dst_table == NULL)
			tdt = (struct dst_table *)g_malloc(sizeof(dst_table[0])*(i+1));
		else
			tdt = (struct dst_table *)g_realloc(dst_table, sizeof(dst_table[0])*(i+1));
		if (tdt == NULL) {
			g_free(dst_table);
			table_size = 0;
		} else {
			dst_table = tdt;
			table_size++;

			dst_table[i].zone = zone;
			dst_table[i].start = dst_table[i].end = t;

			/* no entry will cover more than 6 months */
			low = t - MAX_DST_WIDTH/2;
			/* XXX - what if t < MAX_DST_WIDTH/2? */

			high = t + MAX_DST_WIDTH/2;
			/* XXX - what if this overflows? */

			/*
			 * Widen the new entry using two bisection searches.
			 */
			while (low+60*60 < dst_table[i].start) {
				if (dst_table[i].start - low > MAX_DST_SKIP*2)
					t = dst_table[i].start - MAX_DST_SKIP;
				else
					t = low + (dst_table[i].start-low)/2;
				if (TimeZone(t) == zone)
					dst_table[i].start = t;
				else
					low = t;
			}

			while (high-60*60 > dst_table[i].end) {
				if (high - dst_table[i].end > MAX_DST_SKIP*2)
					t = dst_table[i].end + MAX_DST_SKIP;
				else
					t = high - (high-dst_table[i].end)/2;
				if (TimeZone(t) == zone)
					dst_table[i].end = t;
				else
					high = t;
			}
		}
	}
	return zone;
}

/*
 * Return the UTC offset in seconds west of UTC, adjusted for extra time
 * offset, for a local time value.  If ut = lt + LocTimeDiff(lt), then
 * lt = ut - TimeDiff(ut), but the converse does not necessarily hold near
 * daylight savings transitions because some local times are ambiguous.
 * LocTimeDiff(t) equals TimeDiff(t) except near daylight savings transitions.
 */
static int
LocTimeDiff(time_t lt)
{
	int    d = TimeZoneFaster(lt);
	time_t t = lt + d;

	/* if overflow occurred, ignore all the adjustments so far */
	if (((t < lt) ^ (d < 0)))
		t = lt;

	/*
	 * Now t should be close enough to the true UTC to yield the
	 * right answer.
	 */
	return TimeZoneFaster(t);
}

static int
dissect_smb_UTIME(tvbuff_t *tvb, proto_tree *tree, int offset, int hf_date)
{
	guint32  timeval;
	nstime_t ts;

	ts.secs = timeval = tvb_get_letohl(tvb, offset);
	ts.nsecs = 0;
	if (timeval == 0xffffffff) {
		proto_tree_add_time_format_value(tree, hf_date, tvb, offset, 4, &ts,
		    "No time specified (0xffffffff)");
		offset += 4;
		return offset;
	}

	/*
	 * We add the local time offset.
	 */
	ts.secs = timeval + LocTimeDiff(timeval);

	proto_tree_add_time(tree, hf_date, tvb, offset, 4, &ts);
	offset += 4;

	return offset;
}

static int
dissect_smb_datetime(tvbuff_t *tvb, proto_tree *parent_tree, int offset,
    int hf_date, int hf_dos_date, int hf_dos_time, gboolean time_first)
{
	guint16     dos_time, dos_date;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct tm   tm;
	time_t      t;
	nstime_t    tv;

	static const int mday_noleap[12] = {
		31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
	};
	static const int mday_leap[12] = {
		31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
	};

#define ISLEAP(y) ((((y) % 4) == 0) && ((((y) % 100) != 0) || (((y) % 400) == 0)))

	if (time_first) {
		dos_time = tvb_get_letohs(tvb, offset);
		dos_date = tvb_get_letohs(tvb, offset+2);
	} else {
		dos_date = tvb_get_letohs(tvb, offset);
		dos_time = tvb_get_letohs(tvb, offset+2);
	}

	if (((dos_date == 0xffff) && (dos_time == 0xffff)) ||
	    ((dos_date == 0) && (dos_time == 0))) {
		/*
		 * No date/time specified.
		 */
		if (parent_tree) {
			tv.secs = 0;
			tv.nsecs = 0;
			proto_tree_add_time_format_value(parent_tree, hf_date, tvb, offset, 4,
			    &tv, "No time specified (0x%08x)",
			    ((dos_date << 16) | dos_time));
		}
		offset += 4;
		return offset;
	}

	tm.tm_sec   = (dos_time & 0x1f) * 2;
	tm.tm_min   = (dos_time>>5)  & 0x3f;
	tm.tm_hour  = (dos_time>>11) & 0x1f;
	tm.tm_mday  = dos_date & 0x1f;
	tm.tm_mon   = ((dos_date>>5) & 0x0f) - 1;
	tm.tm_year  = ((dos_date>>9) & 0x7f) + 1980 - 1900;
	tm.tm_isdst = -1;

	/*
	 * Do some sanity checks before calling "mktime()";
	 * "mktime()" doesn't do them, it "normalizes" out-of-range
	 * values.
	 */
	if ((tm.tm_sec > 59) || (tm.tm_min > 59) || (tm.tm_hour > 23) ||
	    (tm.tm_mon < 0) || (tm.tm_mon > 11) ||
	    (ISLEAP(tm.tm_year + 1900) ?
	     (tm.tm_mday > mday_leap[tm.tm_mon]) :
	     (tm.tm_mday > mday_noleap[tm.tm_mon])) ||
	    ((t = mktime(&tm)) == -1)) {
		/*
		 * Invalid date/time.
		 */
		if (parent_tree) {
			tv.secs = 0;
			tv.nsecs = 0;
			item = proto_tree_add_time_format_value(parent_tree, hf_date, tvb, offset, 4,
			    &tv, "Invalid time (0x%08x)", ((dos_date << 16) | dos_time));
			tree = proto_item_add_subtree(item, ett_smb_time_date);
			if (time_first) {
				proto_tree_add_uint_format(tree, hf_dos_time, tvb, offset, 2, dos_time, "DOS Time: %02d:%02d:%02d (0x%04x)", tm.tm_hour, tm.tm_min, tm.tm_sec, dos_time);
				proto_tree_add_uint_format(tree, hf_dos_date, tvb, offset+2, 2, dos_date, "DOS Date: %04d-%02d-%02d (0x%04x)", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, dos_date);
			} else {
				proto_tree_add_uint_format(tree, hf_dos_date, tvb, offset, 2, dos_date, "DOS Date: %04d-%02d-%02d (0x%04x)", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, dos_date);
				proto_tree_add_uint_format(tree, hf_dos_time, tvb, offset+2, 2, dos_time, "DOS Time: %02d:%02d:%02d (0x%04x)", tm.tm_hour, tm.tm_min, tm.tm_sec, dos_time);
			}
		}
		offset += 4;
		return offset;
	}

	tv.secs = t;
	tv.nsecs = 0;

	if (parent_tree) {
		item = proto_tree_add_time(parent_tree, hf_date, tvb, offset, 4, &tv);
		tree = proto_item_add_subtree(item, ett_smb_time_date);
		if (time_first) {
			proto_tree_add_uint_format(tree, hf_dos_time, tvb, offset, 2, dos_time, "DOS Time: %02d:%02d:%02d (0x%04x)", tm.tm_hour, tm.tm_min, tm.tm_sec, dos_time);
			proto_tree_add_uint_format(tree, hf_dos_date, tvb, offset+2, 2, dos_date, "DOS Date: %04d-%02d-%02d (0x%04x)", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, dos_date);
		} else {
			proto_tree_add_uint_format(tree, hf_dos_date, tvb, offset, 2, dos_date, "DOS Date: %04d-%02d-%02d (0x%04x)", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, dos_date);
			proto_tree_add_uint_format(tree, hf_dos_time, tvb, offset+2, 2, dos_time, "DOS Time: %02d:%02d:%02d (0x%04x)", tm.tm_hour, tm.tm_min, tm.tm_sec, dos_time);
		}
	}

	offset += 4;

	return offset;
}

static const true_false_string tfs_disposition_delete_on_close = {
	"DELETE this file when closed",
	"Normal access, do not delete on close"
};

static const true_false_string tfs_pipe_info_flag = {
	"SET NAMED PIPE mode",
	"Clear NAMED PIPE mode"
};


static const value_string da_access_vals[] = {
	{ 0,		"Open for reading"},
	{ 1,		"Open for writing"},
	{ 2,		"Open for reading and writing"},
	{ 3,		"Open for execute"},
	{0, NULL}
};
static const value_string da_sharing_vals[] = {
	{ 0,		"Compatibility mode"},
	{ 1,		"Deny read/write/execute (exclusive)"},
	{ 2,		"Deny write"},
	{ 3,		"Deny read/execute"},
	{ 4,		"Deny none"},
	{0, NULL}
};
static const value_string da_locality_vals[] = {
	{ 0,		"Locality of reference unknown"},
	{ 1,		"Mainly sequential access"},
	{ 2,		"Mainly random access"},
	{ 3,		"Random access with some locality"},
	{0, NULL}
};
static const true_false_string tfs_da_caching = {
	"Do not cache this file",
	"Caching permitted on this file"
};
static const true_false_string tfs_da_writetru = {
	"Write through enabled",
	"Write through disabled"
};
static int
dissect_access(tvbuff_t *tvb, proto_tree *parent_tree, int offset, int hf_access)
{
	static const int * flags[] = {
		&hf_smb_access_writetru,
		&hf_smb_access_caching,
		&hf_smb_access_locality,
		&hf_smb_access_sharing,
		&hf_smb_access_mode,
		NULL
	};

	proto_tree_add_bitmask(parent_tree, tvb, offset, hf_access, ett_smb_desiredaccess, flags, ENC_LITTLE_ENDIAN);
	offset += 2;

	return offset;
}

#define SMB_FILE_ATTRIBUTE_READ_ONLY			0x00000001
#define SMB_FILE_ATTRIBUTE_HIDDEN			0x00000002
#define SMB_FILE_ATTRIBUTE_SYSTEM			0x00000004
#define SMB_FILE_ATTRIBUTE_VOLUME			0x00000008
#define SMB_FILE_ATTRIBUTE_DIRECTORY			0x00000010
#define SMB_FILE_ATTRIBUTE_ARCHIVE			0x00000020
#define SMB_FILE_ATTRIBUTE_DEVICE			0x00000040
#define SMB_FILE_ATTRIBUTE_NORMAL			0x00000080
#define SMB_FILE_ATTRIBUTE_TEMPORARY			0x00000100
#define SMB_FILE_ATTRIBUTE_SPARSE			0x00000200
#define SMB_FILE_ATTRIBUTE_REPARSE			0x00000400
#define SMB_FILE_ATTRIBUTE_COMPRESSED			0x00000800
#define SMB_FILE_ATTRIBUTE_OFFLINE			0x00001000
#define SMB_FILE_ATTRIBUTE_NOT_CONTENT_INDEXED		0x00002000
#define SMB_FILE_ATTRIBUTE_ENCRYPTED			0x00004000

static const true_false_string tfs_file_attribute_read_only = {
	"READ ONLY",
	"NOT read only",
};
static const true_false_string tfs_file_attribute_hidden = {
	"HIDDEN",
	"NOT hidden"
};
static const true_false_string tfs_file_attribute_system = {
	"SYSTEM file/dir",
	"NOT a system file/dir"
};
static const true_false_string tfs_file_attribute_volume = {
	"VOLUME ID",
	"NOT a volume ID"
};
static const true_false_string tfs_file_attribute_directory = {
	"DIRECTORY",
	"NOT a directory"
};
static const true_false_string tfs_file_attribute_archive = {
	"Modified since last ARCHIVE",
	"Has NOT been modified since last archive"
};
static const true_false_string tfs_file_attribute_device = {
	"A DEVICE",
	"NOT a device"
};
static const true_false_string tfs_file_attribute_normal = {
	"An ordinary file/dir",
	"Has some attribute set"
};
static const true_false_string tfs_file_attribute_temporary = {
	"A TEMPORARY file",
	"NOT a temporary file"
};
static const true_false_string tfs_file_attribute_sparse = {
	"A SPARSE file",
	"NOT a sparse file"
};
static const true_false_string tfs_file_attribute_reparse = {
	"Has an associated REPARSE POINT",
	"Does NOT have an associated reparse point"
};
static const true_false_string tfs_file_attribute_compressed = {
	"COMPRESSED",
	"Uncompressed"
};
static const true_false_string tfs_file_attribute_offline = {
	"OFFLINE",
	"Online"
};
static const true_false_string tfs_file_attribute_not_content_indexed = {
	"CONTENT INDEXED",
	"NOT content indexed"
};
static const true_false_string tfs_file_attribute_encrypted = {
	"This is an ENCRYPTED file",
	"This is NOT an encrypted file"
};

/*
 * Dissects an SMB_FILE_ATTRIBUTES, to use the term given to it by
 * section 2.2.1.2.4 of [MS-CIFS], in cases where it's just file attributes,
 * not search attributes.
 */
static int
dissect_file_attributes(tvbuff_t *tvb, proto_tree *parent_tree, int offset)
{
	static const int * flags[] = {
		&hf_smb_file_attr_archive_16bit,
		&hf_smb_file_attr_directory_16bit,
		&hf_smb_file_attr_volume_16bit,
		&hf_smb_file_attr_system_16bit,
		&hf_smb_file_attr_hidden_16bit,
		&hf_smb_file_attr_read_only_16bit,
		NULL
	};

	proto_tree_add_bitmask(parent_tree, tvb, offset, hf_smb_file_attr_16bit, ett_smb_file_attributes, flags, ENC_LITTLE_ENDIAN);
	offset += 2;

	return offset;
}

/*
 * 3.11 in the SNIA CIFS spec
 * SMB_EXT_FILE_ATTR, section 2.2.1.2.3 in the [MS-CIFS] spec
 */
static int
dissect_file_ext_attr_bits(tvbuff_t *tvb, proto_tree *parent_tree, int offset,
    int len, guint32 mask)
{
	proto_item *item;
	/*
	 * XXX - Network Monitor disagrees on some of the
	 * bits, e.g. the bits above temporary are "atomic write"
	 * and "transaction write", and it says nothing about the
	 * bits above that.
	 *
	 * Does the Win32 API documentation, or the NT Native API book,
	 * suggest anything?
	 */
	static const int * mask_fields[] = {
		&hf_smb_file_eattr_read_only,
		&hf_smb_file_eattr_hidden,
		&hf_smb_file_eattr_system,
		&hf_smb_file_eattr_volume,
		&hf_smb_file_eattr_directory,
		&hf_smb_file_eattr_archive,
		&hf_smb_file_eattr_device,
		&hf_smb_file_eattr_normal,
		&hf_smb_file_eattr_temporary,
		&hf_smb_file_eattr_sparse,
		&hf_smb_file_eattr_reparse,
		&hf_smb_file_eattr_compressed,
		&hf_smb_file_eattr_offline,
		&hf_smb_file_eattr_not_content_indexed,
		&hf_smb_file_eattr_encrypted,
		NULL
	};

	item = proto_tree_add_bitmask_value_with_flags(parent_tree, tvb, offset,
			hf_smb_file_eattr, ett_smb_file_attributes, mask_fields, mask, BMT_NO_APPEND);
	if (len == 0)
		PROTO_ITEM_SET_GENERATED(item);

	offset += len;

	return offset;
}

/* 3.11 */
int
dissect_file_ext_attr(tvbuff_t *tvb, proto_tree *parent_tree, int offset)
{
	guint32 mask;

	mask = tvb_get_letohl(tvb, offset);

	offset = dissect_file_ext_attr_bits(tvb, parent_tree, offset, 4, mask);

	return offset;
}

static int
dissect_dir_info_file_attributes(tvbuff_t *tvb, proto_tree *parent_tree, int offset)
{
	static const int * flags[] = {
		&hf_smb_file_attr_read_only_8bit,
		&hf_smb_file_attr_hidden_8bit,
		&hf_smb_file_attr_system_8bit,
		&hf_smb_file_attr_volume_8bit,
		&hf_smb_file_attr_directory_8bit,
		&hf_smb_file_attr_archive_8bit,
		NULL
	};

	proto_tree_add_bitmask(parent_tree, tvb, offset, hf_smb_file_attr_8bit, ett_smb_file_attributes, flags, ENC_NA);
	offset += 1;

	return offset;
}

static const true_false_string tfs_search_attribute_read_only = {
	"Include READ ONLY files in search results",
	"Do NOT include read only files in search results",
};
static const true_false_string tfs_search_attribute_hidden = {
	"Include HIDDEN files in search results",
	"Do NOT include hidden files in search results"
};
static const true_false_string tfs_search_attribute_system = {
	"Include SYSTEM files in search results",
	"Do NOT include system files in search results"
};
static const true_false_string tfs_search_attribute_volume = {
	"Include VOLUME IDs in search results",
	"Do NOT include volume IDs in search results"
};
static const true_false_string tfs_search_attribute_directory = {
	"Include DIRECTORIES in search results",
	"Do NOT include directories in search results"
};
static const true_false_string tfs_search_attribute_archive = {
	"Include ARCHIVE files in search results",
	"Do NOT include archive files in search results"
};

/*
 * Dissects an SMB_FILE_ATTRIBUTES, to use the term given to it by
 * section 2.2.1.2.4 of [MS-CIFS], in cases where it's search attributes.
 */
static int
dissect_search_attributes(tvbuff_t *tvb, proto_tree *parent_tree, int offset)
{
	static const int * flags[] = {
		&hf_smb_search_attribute_read_only,
		&hf_smb_search_attribute_hidden,
		&hf_smb_search_attribute_system,
		&hf_smb_search_attribute_volume,
		&hf_smb_search_attribute_directory,
		&hf_smb_search_attribute_archive,
		NULL
	};

	proto_tree_add_bitmask(parent_tree, tvb, offset, hf_smb_search_attribute, ett_smb_search, flags, ENC_LITTLE_ENDIAN);
	offset += 2;

	return offset;
}

#if 0
/*
 * XXX - this isn't used.
 * Is this used for anything?  NT Create AndX doesn't use it.
 * Is there some 16-bit attribute field with more bits than Read Only,
 * Hidden, System, Volume ID, Directory, and Archive?
 */
static int
dissect_extended_file_attributes(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree, int offset)
{
	static const int * flags[] = {
		&hf_smb_file_attr_read_only_16bit,
		&hf_smb_file_attr_hidden_16bit,
		&hf_smb_file_attr_system_16bit,
		&hf_smb_file_attr_volume_16bit,
		&hf_smb_file_attr_directory_16bit,
		&hf_smb_file_attr_archive_16bit,
		&hf_smb_file_attr_device,
		&hf_smb_file_attr_normal,
		&hf_smb_file_attr_temporary,
		&hf_smb_file_attr_sparse,
		&hf_smb_file_attr_reparse,
		&hf_smb_file_attr_compressed,
		&hf_smb_file_attr_offline,
		&hf_smb_file_attr_not_content_indexed,
		&hf_smb_file_attr_encrypted,
		NULL
	};

	proto_tree_add_bitmask(parent_tree, tvb, offset, hf_smb_file_eattr, ett_smb_file_attributes, flags, ENC_LITTLE_ENDIAN);
	offset += 2;

	return offset;
}
#endif


#define SERVER_CAP_RAW_MODE            0x00000001
#define SERVER_CAP_MPX_MODE            0x00000002
#define SERVER_CAP_UNICODE             0x00000004
#define SERVER_CAP_LARGE_FILES         0x00000008
#define SERVER_CAP_NT_SMBS             0x00000010
#define SERVER_CAP_RPC_REMOTE_APIS     0x00000020
#define SERVER_CAP_STATUS32            0x00000040
#define SERVER_CAP_LEVEL_II_OPLOCKS    0x00000080
#define SERVER_CAP_LOCK_AND_READ       0x00000100
#define SERVER_CAP_NT_FIND             0x00000200
#define SERVER_CAP_DFS                 0x00001000
#define SERVER_CAP_INFOLEVEL_PASSTHRU  0x00002000
#define SERVER_CAP_LARGE_READX         0x00004000
#define SERVER_CAP_LARGE_WRITEX        0x00008000
#define SERVER_CAP_LWIO                0x00010000
#define SERVER_CAP_UNIX                0x00800000
#define SERVER_CAP_COMPRESSED_DATA     0x02000000
#define SERVER_CAP_DYNAMIC_REAUTH      0x20000000
#define SERVER_CAP_EXTENDED_SECURITY   0x80000000

static const true_false_string tfs_server_cap_raw_mode = {
	"Read Raw and Write Raw are supported",
	"Read Raw and Write Raw are not supported"
};
static const true_false_string tfs_server_cap_mpx_mode = {
	"Read Mpx and Write Mpx are supported",
	"Read Mpx and Write Mpx are not supported"
};
static const true_false_string tfs_server_cap_unicode = {
	"Unicode strings are supported",
	"Unicode strings are not supported"
};
static const true_false_string tfs_server_cap_large_files = {
	"Large files are supported",
	"Large files are not supported",
};
static const true_false_string tfs_server_cap_nt_smbs = {
	"NT SMBs are supported",
	"NT SMBs are not supported"
};
static const true_false_string tfs_server_cap_rpc_remote_apis = {
	"RPC remote APIs are supported",
	"RPC remote APIs are not supported"
};
static const true_false_string tfs_server_cap_nt_status = {
	"NT status codes are supported",
	"NT status codes are not supported"
};
static const true_false_string tfs_server_cap_level_ii_oplocks = {
	"Level 2 oplocks are supported",
	"Level 2 oplocks are not supported"
};
static const true_false_string tfs_server_cap_lock_and_read = {
	"Lock and Read is supported",
	"Lock and Read is not supported"
};
static const true_false_string tfs_server_cap_nt_find = {
	"NT Find is supported",
	"NT Find is not supported"
};
static const true_false_string tfs_server_cap_dfs = {
	"Dfs is supported",
	"Dfs is not supported"
};
static const true_false_string tfs_server_cap_infolevel_passthru = {
	"NT information level request passthrough is supported",
	"NT information level request passthrough is not supported"
};
static const true_false_string tfs_server_cap_large_readx = {
	"Large Read andX is supported",
	"Large Read andX is not supported"
};
static const true_false_string tfs_server_cap_large_writex = {
	"Large Write andX is supported",
	"Large Write andX is not supported"
};
static const true_false_string tfs_server_cap_lwio = {
	"LWIO ioctl/fsctl is supported",
	"LWIO ioctl/fsctl is not supported"
};
static const true_false_string tfs_server_cap_unix = {
	"UNIX extensions are supported",
	"UNIX extensions are not supported"
};
static const true_false_string tfs_server_cap_compressed_data = {
	"Compressed data transfer is supported",
	"Compressed data transfer is not supported"
};
static const true_false_string tfs_server_cap_dynamic_reauth = {
	"Dynamic Reauth is supported",
	"Dynamic Reauth is not supported"
};
static const true_false_string tfs_server_cap_extended_security = {
	"Extended security exchanges are supported",
	"Extended security exchanges are not supported"
};
static int
dissect_negprot_capabilities(tvbuff_t *tvb, proto_tree *parent_tree, int offset)
{
	guint32     mask;

	static const int * flags[] = {
		&hf_smb_server_cap_raw_mode,
		&hf_smb_server_cap_mpx_mode,
		&hf_smb_server_cap_unicode,
		&hf_smb_server_cap_large_files,
		&hf_smb_server_cap_nt_smbs,
		&hf_smb_server_cap_rpc_remote_apis,
		&hf_smb_server_cap_nt_status,
		&hf_smb_server_cap_level_ii_oplocks,
		&hf_smb_server_cap_lock_and_read,
		&hf_smb_server_cap_nt_find,
		&hf_smb_server_cap_dfs,
		&hf_smb_server_cap_infolevel_passthru,
		&hf_smb_server_cap_large_readx,
		&hf_smb_server_cap_large_writex,
		&hf_smb_server_cap_lwio,
		&hf_smb_server_cap_unix,
		&hf_smb_server_cap_compressed_data,
		&hf_smb_server_cap_dynamic_reauth,
		&hf_smb_server_cap_extended_security,
		NULL
	};

	mask = tvb_get_letohl(tvb, offset);
	proto_tree_add_bitmask(parent_tree, tvb, offset, hf_smb_server_cap, ett_smb_capabilities, flags, ENC_LITTLE_ENDIAN);

	return mask;
}

#define RAWMODE_READ   0x01
#define RAWMODE_WRITE  0x02
static const true_false_string tfs_rm_read = {
	"Read Raw is supported",
	"Read Raw is not supported"
};
static const true_false_string tfs_rm_write = {
	"Write Raw is supported",
	"Write Raw is not supported"
};

static int
dissect_negprot_rawmode(tvbuff_t *tvb, proto_tree *parent_tree, int offset)
{
	static const int * flags[] = {
		&hf_smb_rm_read,
		&hf_smb_rm_write,
		NULL
	};

	proto_tree_add_bitmask(parent_tree, tvb, offset, hf_smb_rm, ett_smb_rawmode, flags, ENC_LITTLE_ENDIAN);

	offset += 2;

	return offset;
}

#define SECURITY_MODE_MODE             0x01
#define SECURITY_MODE_PASSWORD         0x02
#define SECURITY_MODE_SIGNATURES       0x04
#define SECURITY_MODE_SIG_REQUIRED     0x08
static const true_false_string tfs_sm_mode = {
	"USER security mode",
	"SHARE security mode"
};
static const true_false_string tfs_sm_password = {
	"ENCRYPTED password. Use challenge/response",
	"PLAINTEXT password"
};
static const true_false_string tfs_sm_signatures = {
	"Security signatures ENABLED",
	"Security signatures NOT enabled"
};
static const true_false_string tfs_sm_sig_required = {
	"Security signatures REQUIRED",
	"Security signatures NOT required"
};

static int
dissect_negprot_security_mode(tvbuff_t *tvb, proto_tree *parent_tree, int offset, int wc)
{
	static const int * flags13[] = {
		&hf_smb_sm_mode16,
		&hf_smb_sm_password16,
		NULL
	};
	static const int * flags17[] = {
		&hf_smb_sm_mode,
		&hf_smb_sm_password,
		&hf_smb_sm_signatures,
		&hf_smb_sm_sig_required,
		NULL
	};

	switch(wc) {
	case 13:
		proto_tree_add_bitmask(parent_tree, tvb, offset, hf_smb_sm16, ett_smb_mode, flags13, ENC_LITTLE_ENDIAN);
		offset += 2;
		break;

	case 17:
		proto_tree_add_bitmask(parent_tree, tvb, offset, hf_smb_sm, ett_smb_mode, flags17, ENC_LITTLE_ENDIAN);
		offset += 1;
		break;
	}

	return offset;
}

#define MAX_DIALECTS 20
struct negprot_dialects {
	int   num;
	char *name[MAX_DIALECTS+1];
};

static int
dissect_negprot_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	proto_tree *tr = NULL;
	guint16     bc;
	guint8      wc;
	struct negprot_dialects *dialects = NULL;

	DISSECTOR_ASSERT(si);

	WORD_COUNT;

	BYTE_COUNT;

	tr = proto_tree_add_subtree(tree, tvb, offset, bc, ett_smb_dialects, NULL, "Requested Dialects");

	if (!pinfo->fd->flags.visited && si->sip) {
		dialects = wmem_new(wmem_file_scope(), struct negprot_dialects);
		dialects->num = 0;
		si->sip->extra_info_type = SMB_EI_DIALECTS;
		si->sip->extra_info = dialects;
	}

	while (bc) {
		int len;
		const guint8 *str;
		proto_item *dit = NULL;
		proto_tree *dtr = NULL;

		/* XXX - what if this runs past bc? */
		tvb_ensure_bytes_exist(tvb, offset+1, 1);
		str = tvb_get_const_stringz(tvb, offset+1, &len);

		if (tr) {
			dit = proto_tree_add_string(tr, hf_smb_dialect, tvb, offset, len+1, str);
			dtr = proto_item_add_subtree(dit, ett_smb_dialect);
		}

		/* Buffer Format */
		CHECK_BYTE_COUNT(1);
		proto_tree_add_item(dtr, hf_smb_buffer_format, tvb, offset, 1,
			ENC_LITTLE_ENDIAN);
		COUNT_BYTES(1);

		/*Dialect Name */
		CHECK_BYTE_COUNT(len);
		proto_tree_add_string(dtr, hf_smb_dialect_name, tvb, offset,
			len, str);
		COUNT_BYTES(len);

		if (!pinfo->fd->flags.visited && dialects && (dialects->num < MAX_DIALECTS)) {
			dialects->name[dialects->num++] = wmem_strdup(wmem_file_scope(), str);
		}
	}


	END_OF_SMB

	return offset;
}

static int
dissect_negprot_response(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	guint8      wc;
	guint16     dialect;
	const char *dn;
	int         dn_len;
	guint16     bc;
	guint16     chl          = 0;
	guint32     caps         = 0;
	gint16      tz;
	const char *dialect_name = NULL;
	struct negprot_dialects *dialects = NULL;

	DISSECTOR_ASSERT(si);

	WORD_COUNT;

	/* Dialect Index */
	dialect = tvb_get_letohs(tvb, offset);

	if (si->sip && (si->sip->extra_info_type == SMB_EI_DIALECTS)) {
		dialects = (struct negprot_dialects *)si->sip->extra_info;
		if (dialect < dialects->num) {
			dialect_name = dialects->name[dialect];
		}
	}
	if (!dialect_name) {
		dialect_name = "unknown";
	}

	switch(wc) {
	case 1:
		if (dialect == 0xffff) {
			proto_tree_add_uint_format_value(tree, hf_smb_dialect_index,
				tvb, offset, 2, dialect,
				"-1, PC NETWORK PROGRAM 1.0 chosen");
		} else {
			proto_tree_add_uint(tree, hf_smb_dialect_index,
				tvb, offset, 2, dialect);
		}
		break;
	case 13:
		proto_tree_add_uint_format_value(tree, hf_smb_dialect_index,
			tvb, offset, 2, dialect,
			"%u, Greater than CORE PROTOCOL and up to LANMAN2.1", dialect);
		break;
	case 17:
		proto_tree_add_uint_format_value(tree, hf_smb_dialect_index,
			tvb, offset, 2, dialect,
			"%u: %s", dialect, dialect_name);
		break;
	default:
		proto_tree_add_item(tree, hf_smb_word_unk_response_format, tvb, offset, wc*2, ENC_NA);
		offset += wc*2;
		goto bytecount;
	}
	offset += 2;

	switch(wc) {
	case 13:
		/* Security Mode */
		offset = dissect_negprot_security_mode(tvb, tree, offset, wc);

		/* Maximum Transmit Buffer Size */
		proto_tree_add_item(tree, hf_smb_max_trans_buf_size,
			tvb, offset, 2, ENC_LITTLE_ENDIAN);
		offset += 2;

		/* Maximum Multiplex Count */
		proto_tree_add_item(tree, hf_smb_max_mpx_count,
			tvb, offset, 2, ENC_LITTLE_ENDIAN);
		offset += 2;

		/* Maximum Vcs Number */
		proto_tree_add_item(tree, hf_smb_max_vcs_num,
			tvb, offset, 2, ENC_LITTLE_ENDIAN);
		offset += 2;

		/* raw mode */
		offset = dissect_negprot_rawmode(tvb, tree, offset);

		/* session key */
		proto_tree_add_item(tree, hf_smb_session_key,
			tvb, offset, 4, ENC_LITTLE_ENDIAN);
		offset += 4;

		/* current time and date at server */
		offset = dissect_smb_datetime(tvb, tree, offset, hf_smb_server_date_time, hf_smb_server_smb_date, hf_smb_server_smb_time,
		    TRUE);

		/* time zone */
		tz = tvb_get_letohs(tvb, offset);
		proto_tree_add_int_format_value(tree, hf_smb_server_timezone, tvb, offset, 2, tz, "%d min from UTC", tz);
		offset += 2;

		/* challenge length */
		chl = tvb_get_letohs(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_challenge_length, tvb, offset, 2, chl);
		offset += 2;

		/* 2 reserved bytes */
		proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 2, ENC_NA);
		offset += 2;

		break;

	case 17:
		/* Security Mode */
		offset = dissect_negprot_security_mode(tvb, tree, offset, wc);

		/* Maximum Multiplex Count */
		proto_tree_add_item(tree, hf_smb_max_mpx_count,
			tvb, offset, 2, ENC_LITTLE_ENDIAN);
		offset += 2;

		/* Maximum Vcs Number */
		proto_tree_add_item(tree, hf_smb_max_vcs_num,
			tvb, offset, 2, ENC_LITTLE_ENDIAN);
		offset += 2;

		/* Maximum Transmit Buffer Size */
		proto_tree_add_item(tree, hf_smb_max_trans_buf_size,
			tvb, offset, 4, ENC_LITTLE_ENDIAN);
		offset += 4;

		/* maximum raw buffer size */
		proto_tree_add_item(tree, hf_smb_max_raw_buf_size,
			tvb, offset, 4, ENC_LITTLE_ENDIAN);
		offset += 4;

		/* session key */
		proto_tree_add_item(tree, hf_smb_session_key,
			tvb, offset, 4, ENC_LITTLE_ENDIAN);
		offset += 4;

		/* server capabilities */
		caps = dissect_negprot_capabilities(tvb, tree, offset);
		offset += 4;

		/* system time */
		offset = dissect_nt_64bit_time(tvb, tree, offset,
				hf_smb_system_time);

		/* time zone */
		tz = tvb_get_letohs(tvb, offset);
		proto_tree_add_int_format_value(tree, hf_smb_server_timezone,
			tvb, offset, 2, tz,
			"%d min from UTC", tz);
		offset += 2;

		/* challenge length */
		chl = tvb_get_guint8(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_challenge_length,
			tvb, offset, 1, chl);
		offset += 1;

		break;
	}

	BYTE_COUNT;

	switch(wc) {
	case 13:
		/* encrypted challenge/response data */
		if (chl) {
			CHECK_BYTE_COUNT(chl);
			proto_tree_add_item(tree, hf_smb_challenge, tvb, offset, chl, ENC_NA);
			COUNT_BYTES(chl);
		}

		/*
		 * Primary domain.
		 *
		 * XXX - not present if negotiated dialect isn't
		 * "DOS LANMAN 2.1" or "LANMAN2.1", but we'd either
		 * have to see the request, or assume what dialect strings
		 * were sent, to determine that.
		 *
		 * Is this something other than a primary domain if the
		 * negotiated dialect is Windows for Workgroups 3.1a?
		 * It appears to be 8 bytes of binary data in at least
		 * one capture - is that an encryption key or something
		 * such as that?
		 */
		dn = get_unicode_or_ascii_string(tvb, &offset,
			si->unicode, &dn_len, FALSE, FALSE, &bc);
		if (dn == NULL)
			goto endofcommand;
		proto_tree_add_string(tree, hf_smb_primary_domain, tvb,
			offset, dn_len, dn);
		COUNT_BYTES(dn_len);
		break;

	case 17:
		if (!(caps & SERVER_CAP_EXTENDED_SECURITY)) {
			/* encrypted challenge/response data */
			/* XXX - is this aligned on an even boundary? */
			if (chl) {
				CHECK_BYTE_COUNT(chl);
				proto_tree_add_item(tree, hf_smb_challenge,
					tvb, offset, chl, ENC_NA);
				COUNT_BYTES(chl);
			}

			/* domain */
			/* this string is special, unicode is flagged in caps */
			/* This string is NOT padded to be 16bit aligned.
			   (seen in actual capture)
			   XXX - I've seen a capture where it appears to be
			   so aligned, but I've also seen captures where
			   it is.  The captures where it appeared to be
			   aligned may have been from buggy servers. */
			/* However, don't get rid of existing setting */
			si->unicode = (caps & SERVER_CAP_UNICODE) || si->unicode;

			dn = get_unicode_or_ascii_string(tvb,
				&offset, si->unicode, &dn_len, TRUE, FALSE,
				&bc);
			if (dn == NULL)
				goto endofcommand;
			proto_tree_add_string(tree, hf_smb_primary_domain,
				tvb, offset, dn_len, dn);
			COUNT_BYTES(dn_len);

			/* server name, seen in w2k pro capture */
			dn = get_unicode_or_ascii_string(tvb,
				&offset, si->unicode, &dn_len, TRUE, FALSE,
				&bc);
			if (dn == NULL)
				goto endofcommand;
			proto_tree_add_string(tree, hf_smb_server,
				tvb, offset, dn_len, dn);
			COUNT_BYTES(dn_len);

		} else {
			proto_item *blob_item;
			guint16 sbloblen;

			/* guid */
			/* XXX - show it in the standard Microsoft format
			   for GUIDs? */
			CHECK_BYTE_COUNT(16);
			proto_tree_add_item(tree, hf_smb_server_guid,
				tvb, offset, 16, ENC_NA);
			COUNT_BYTES(16);

			/* security blob */
			/* If it runs past the end of the captured data, don't
			 * try to put all of it into the protocol tree as the
			 * raw security blob; we might get an exception on
			 * short frames and then we will not see anything at all
			 * of the security blob.
			 */
			sbloblen = bc;
			if (sbloblen > tvb_reported_length_remaining(tvb, offset)) {
				sbloblen = tvb_reported_length_remaining(tvb, offset);
			}
			blob_item = proto_tree_add_item(
				tree, hf_smb_security_blob,
				tvb, offset, sbloblen, ENC_NA);

			/*
			 * If Extended security and BCC == 16, then raw
			 * NTLMSSP is in use. We need to save this info
			 */

			if (bc) {
				tvbuff_t *gssapi_tvb;
				proto_tree *gssapi_tree;

				gssapi_tree = proto_item_add_subtree(
					blob_item, ett_smb_secblob);

				/*
				 * Set the reported length of this to
				 * the reported length of the blob,
				 * rather than the amount of data
				 * available from the blob, so that
				 * we'll throw the right exception if
				 * it's too short.
				 */
				gssapi_tvb = tvb_new_subset(
					tvb, offset, sbloblen, bc);

				call_dissector(
					gssapi_handle, gssapi_tvb, pinfo,
					gssapi_tree);

				if (si->ct)
				  si->ct->raw_ntlmssp = 0;

				COUNT_BYTES(bc);
			}
			else {

			  /*
			   * There is no blob. We just have to make sure
			   * that subsequent routines know to call the
			   * right things ...
			   */

			if (si->ct)
				si->ct->raw_ntlmssp = 1;

			}
		}
		break;
	}

	END_OF_SMB

	return offset;
}


static int
dissect_old_dir_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	int         dn_len;
	const char *dn;
	guint8      wc;
	guint16     bc;

	DISSECTOR_ASSERT(si);

	WORD_COUNT;

	BYTE_COUNT;

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* dir name */
	dn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &dn_len,
		FALSE, FALSE, &bc);

	if ((!pinfo->fd->flags.visited) && si->sip) {
		si->sip->extra_info_type = SMB_EI_FILENAME;
		si->sip->extra_info = wmem_strdup(wmem_file_scope(), dn);
	}

	if (dn == NULL)
		goto endofcommand;
	proto_tree_add_string(tree, hf_smb_dir_name, tvb, offset, dn_len,
		dn);
	COUNT_BYTES(dn_len);

	col_append_fstr(pinfo->cinfo, COL_INFO, ", Directory: %s",
		    format_text(dn, strlen(dn)));

	END_OF_SMB

	return offset;
}

static int
dissect_empty(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	guint8      wc;
	guint16     bc;
	proto_item *item = NULL;

	DISSECTOR_ASSERT(si);

	if (si->sip && (si->sip->extra_info_type == SMB_EI_FILENAME)) {
		item = proto_tree_add_string(tree, hf_smb_file_name, tvb, 0, 0, (const char *)si->sip->extra_info);
		PROTO_ITEM_SET_GENERATED(item);
	}


	WORD_COUNT;

	BYTE_COUNT;

	END_OF_SMB

	return offset;
}

static int
dissect_rename_file_response(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	guint8      wc;
	guint16     bc;
	proto_item *item = NULL;

	DISSECTOR_ASSERT(si);

	if (si->sip && (si->sip->extra_info_type == SMB_EI_RENAMEDATA)) {
		smb_rename_saved_info_t *rni = (smb_rename_saved_info_t *)si->sip->extra_info;

		item = proto_tree_add_string(tree, hf_smb_old_file_name, tvb, 0, 0, rni->old_name);
		PROTO_ITEM_SET_GENERATED(item);
		item = proto_tree_add_string(tree, hf_smb_file_name, tvb, 0, 0, rni->new_name);
		PROTO_ITEM_SET_GENERATED(item);
	}


	WORD_COUNT;

	BYTE_COUNT;

	END_OF_SMB

	return offset;
}

static int
dissect_echo_request(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si _U_)
{
	guint16 ec, bc;
	guint8  wc;

	WORD_COUNT;

	/* echo count */
	ec = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_echo_count, tvb, offset, 2, ec);
	offset += 2;

	BYTE_COUNT;

	if (bc != 0) {
		/* echo data */
		proto_tree_add_item(tree, hf_smb_echo_data, tvb, offset, bc, ENC_NA);
		COUNT_BYTES(bc);
	}

	END_OF_SMB

	return offset;
}

static int
dissect_echo_response(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si _U_)
{
	guint16 bc;
	guint8  wc;

	WORD_COUNT;

	/* echo sequence number */
	proto_tree_add_item(tree, hf_smb_echo_seq_num, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	BYTE_COUNT;

	if (bc != 0) {
		/* echo data */
		proto_tree_add_item(tree, hf_smb_echo_data, tvb, offset, bc, ENC_NA);
		COUNT_BYTES(bc);
	}

	END_OF_SMB

	return offset;
}

static int
dissect_tree_connect_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	int         an_len, pwlen;
	const char *an;
	guint8      wc;
	guint16     bc;

	DISSECTOR_ASSERT(si);

	WORD_COUNT;

	BYTE_COUNT;

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* Path */
	an = get_unicode_or_ascii_string(tvb, &offset,
		si->unicode, &an_len, FALSE, FALSE, &bc);
	if (an == NULL)
		goto endofcommand;
	proto_tree_add_string(tree, hf_smb_path, tvb,
		offset, an_len, an);
	COUNT_BYTES(an_len);

	col_append_fstr(pinfo->cinfo, COL_INFO, ", Path: %s",
		    format_text(an, strlen(an)));

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* password, ANSI */
	/* XXX - what if this runs past bc? */
	pwlen = tvb_strsize(tvb, offset);
	CHECK_BYTE_COUNT(pwlen);
	proto_tree_add_item(tree, hf_smb_password,
		tvb, offset, pwlen, ENC_NA);
	COUNT_BYTES(pwlen);

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* Service */
	/*
	 * XXX - the SNIA CIFS spec "Strings that are never passed in
	 * Unicode are: ... The service name string in the
	 * Tree_Connect_AndX SMB".  Is that claim false?
	 */
	an = get_unicode_or_ascii_string(tvb, &offset,
		si->unicode, &an_len, FALSE, FALSE, &bc);
	if (an == NULL)
		goto endofcommand;
	proto_tree_add_string(tree, hf_smb_service, tvb,
		offset, an_len, an);
	COUNT_BYTES(an_len);

	END_OF_SMB

	return offset;
}

static int
dissect_smb_uid(tvbuff_t *tvb, proto_tree *parent_tree, int offset, smb_info_t *si)
{
	proto_item *item, *subitem;
	proto_tree *tree;
	smb_uid_t  *smb_uid = NULL;

	item = proto_tree_add_uint(parent_tree, hf_smb_uid, tvb, offset, 2, si->uid);
	tree = proto_item_add_subtree(item, ett_smb_uid);

	smb_uid = (smb_uid_t *)wmem_tree_lookup32(si->ct->uid_tree, si->uid);
	if (smb_uid) {
		if (smb_uid->domain && smb_uid->account)
			proto_item_append_text(item, "  (");
		if (smb_uid->domain) {
			proto_item_append_text(item, "%s", smb_uid->domain);
			subitem = proto_tree_add_string(tree, hf_smb_primary_domain, tvb, 0, 0, smb_uid->domain);
			PROTO_ITEM_SET_GENERATED(subitem);
		}
		if (smb_uid->account) {
			proto_item_append_text(item, "\\%s", smb_uid->account);
			subitem = proto_tree_add_string(tree, hf_smb_account, tvb, 0, 0, smb_uid->account);
			PROTO_ITEM_SET_GENERATED(subitem);
		}
		if (smb_uid->domain && smb_uid->account)
			proto_item_append_text(item, ")");
		if (smb_uid->logged_in > 0) {
			subitem = proto_tree_add_uint(tree, hf_smb_logged_in, tvb, 0, 0, smb_uid->logged_in);
			PROTO_ITEM_SET_GENERATED(subitem);
		}
		if (smb_uid->logged_out > 0) {
			subitem = proto_tree_add_uint(tree, hf_smb_logged_out, tvb, 0, 0, smb_uid->logged_out);
			PROTO_ITEM_SET_GENERATED(subitem);
		}
	}
	offset += 2;

	return offset;
}

static int
dissect_smb_tid(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, guint16 tid, gboolean is_created, gboolean is_closed, smb_info_t *si)
{
	proto_item     *it;
	proto_tree     *tr;
	smb_tid_info_t *tid_info = NULL;

	DISSECTOR_ASSERT(si);

	/* tid */
	it = proto_tree_add_uint(tree, hf_smb_tid, tvb, offset, 2, tid);
	tr = proto_item_add_subtree(it, ett_smb_tid);
	offset += 2;

	if ((!pinfo->fd->flags.visited) && is_created) {
		tid_info = wmem_new(wmem_file_scope(), smb_tid_info_t);
		tid_info->opened_in = pinfo->num;
		tid_info->closed_in = 0;
		tid_info->type = SMB_FID_TYPE_UNKNOWN;
		if (si->sip && (si->sip->extra_info_type == SMB_EI_TIDNAME)) {
			tid_info->filename = (char *)si->sip->extra_info;
		} else {
			tid_info->filename = NULL;
		}
		wmem_tree_insert32(si->ct->tid_tree, tid, tid_info);
	}

	if (!tid_info) {
		tid_info = (smb_tid_info_t *)wmem_tree_lookup32_le(si->ct->tid_tree, tid);
	}
	if (!tid_info) {
		return offset;
	}

	if ((!pinfo->fd->flags.visited) && is_closed) {
		tid_info->closed_in = pinfo->num;
	}

	if (tid_info->opened_in) {
		if (tid_info->filename) {
			proto_item_append_text(it, "  (%s)", tid_info->filename);

			it = proto_tree_add_string(tr, hf_smb_path, tvb, 0, 0, tid_info->filename);
			PROTO_ITEM_SET_GENERATED(it);
		}

		it = proto_tree_add_uint(tr, hf_smb_mapped_in, tvb, 0, 0, tid_info->opened_in);
		PROTO_ITEM_SET_GENERATED(it);
	}
	if (tid_info->closed_in) {
		it = proto_tree_add_uint(tr, hf_smb_unmapped_in, tvb, 0, 0, tid_info->closed_in);
		PROTO_ITEM_SET_GENERATED(it);
	}


	return offset;
}

static int
dissect_tree_connect_response(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	guint8  wc;
	guint16 bc;

	WORD_COUNT;

	/* Maximum Buffer Size */
	proto_tree_add_item(tree, hf_smb_max_buf_size, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	/* tid */
	offset = dissect_smb_tid(tvb, pinfo, tree, offset, tvb_get_letohs(tvb, offset), TRUE, FALSE, si);

	BYTE_COUNT;

	END_OF_SMB

	return offset;
}


static const true_false_string tfs_of_create = {
	"Create file if it does not exist",
	"Fail if file does not exist"
};
static const value_string of_open[] = {
	{ 0,		"Fail if file exists"},
	{ 1,		"Open file if it exists"},
	{ 2,		"Truncate file if it exists"},
	{0, NULL}
};
static int
dissect_open_function(tvbuff_t *tvb, proto_tree *parent_tree, int offset)
{
	static const int * flags[] = {
		&hf_smb_open_function_create,
		&hf_smb_open_function_open,
		NULL
	};

	proto_tree_add_bitmask(parent_tree, tvb, offset, hf_smb_open_function, ett_smb_openfunction, flags, ENC_LITTLE_ENDIAN);
	offset += 2;

	return offset;
}


static const true_false_string tfs_mf_file = {
	"Target must be a file",
	"Target needn't be a file"
};
static const true_false_string tfs_mf_dir = {
	"Target must be a directory",
	"Target needn't be a directory"
};
static const true_false_string tfs_mf_verify = {
	"MUST verify all writes",
	"Don't have to verify writes"
};
static int
dissect_move_flags(tvbuff_t *tvb, proto_tree *parent_tree, int offset)
{
	static const int * flags[] = {
		&hf_smb_move_flags_verify,
		&hf_smb_move_flags_dir,
		&hf_smb_move_flags_file,
		NULL
	};

	proto_tree_add_bitmask(parent_tree, tvb, offset, hf_smb_move_flags, ett_smb_move_copy_flags, flags, ENC_LITTLE_ENDIAN);
	offset += 2;

	return offset;
}

static const true_false_string tfs_cf_mode = {
	"ASCII",
	"Binary"
};
static const true_false_string tfs_cf_tree_copy = {
	"Copy is a tree copy",
	"Copy is a file copy"
};
static const true_false_string tfs_cf_ea_action = {
	"Fail copy",
	"Discard EAs"
};
static int
dissect_copy_flags(tvbuff_t *tvb, proto_tree *parent_tree, int offset)
{
	static const int * flags[] = {
		&hf_smb_copy_flags_ea_action,
		&hf_smb_copy_flags_tree_copy,
		&hf_smb_copy_flags_verify,
		&hf_smb_copy_flags_source_mode,
		&hf_smb_copy_flags_dest_mode,
		&hf_smb_copy_flags_dir,
		&hf_smb_copy_flags_file,
		NULL
	};

	proto_tree_add_bitmask(parent_tree, tvb, offset, hf_smb_copy_flags, ett_smb_move_copy_flags, flags, ENC_LITTLE_ENDIAN);
	offset += 2;

	return offset;
}

static int
dissect_move_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	int         fn_len;
	guint16     tid;
	guint16     bc;
	guint8      wc;
	const char *fn;

	DISSECTOR_ASSERT(si);

	WORD_COUNT;

	/* tid */
	tid = tvb_get_letohs(tvb, offset);
	offset = dissect_smb_tid(tvb, pinfo, tree, offset, tid, FALSE, FALSE, si);

	/* open function */
	offset = dissect_open_function(tvb, tree, offset);

	/* move flags */
	offset = dissect_move_flags(tvb, tree, offset);

	BYTE_COUNT;

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* file name */
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len,
		FALSE, FALSE, &bc);
	if (fn == NULL)
		goto endofcommand;
	proto_tree_add_string_format(tree, hf_smb_file_name, tvb, offset,
		fn_len,	fn, "Old File Name: %s", format_text(fn, strlen(fn)));
	COUNT_BYTES(fn_len);

	col_append_fstr(pinfo->cinfo, COL_INFO, ", Old Name: %s",
		    format_text(fn, strlen(fn)));

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* file name */
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len,
		FALSE, FALSE, &bc);
	if (fn == NULL)
		goto endofcommand;
	proto_tree_add_string_format(tree, hf_smb_file_name, tvb, offset,
		fn_len,	fn, "New File Name: %s", format_text(fn, strlen(fn)));
	COUNT_BYTES(fn_len);

	col_append_fstr(pinfo->cinfo, COL_INFO, ", New Name: %s",
		    format_text(fn, strlen(fn)));

	END_OF_SMB

	return offset;
}

static int
dissect_copy_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	int         fn_len;
	guint16     tid;
	guint16     bc;
	guint8      wc;
	const char *fn;

	DISSECTOR_ASSERT(si);

	WORD_COUNT;

	/* tid */
	tid = tvb_get_letohs(tvb, offset);
	offset = dissect_smb_tid(tvb, pinfo, tree, offset, tid, FALSE, FALSE, si);

	/* open function */
	offset = dissect_open_function(tvb, tree, offset);

	/* copy flags */
	offset = dissect_copy_flags(tvb, tree, offset);

	BYTE_COUNT;

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* file name */
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len,
		FALSE, FALSE, &bc);
	if (fn == NULL)
		goto endofcommand;
	proto_tree_add_string_format(tree, hf_smb_file_name, tvb, offset,
		fn_len, fn, "Source File Name: %s", format_text(fn, strlen(fn)));
	COUNT_BYTES(fn_len);

	col_append_fstr(pinfo->cinfo, COL_INFO, ", Source Name: %s",
		    format_text(fn, strlen(fn)));

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* file name */
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len,
		FALSE, FALSE, &bc);
	if (fn == NULL)
		goto endofcommand;
	proto_tree_add_string_format(tree, hf_smb_file_name, tvb, offset,
		fn_len, fn, "Destination File Name: %s",
		format_text(fn, strlen(fn)));
	COUNT_BYTES(fn_len);

	col_append_fstr(pinfo->cinfo, COL_INFO, ", Destination Name: %s", format_text(fn, strlen(fn)));

	END_OF_SMB

	return offset;
}

static int
dissect_move_copy_response(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	int         fn_len;
	const char *fn;
	guint8      wc;
	guint16     bc;

	DISSECTOR_ASSERT(si);

	WORD_COUNT;

	/* # of files moved */
	proto_tree_add_item(tree, hf_smb_files_moved, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	BYTE_COUNT;

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* file name */
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len,
		FALSE, FALSE, &bc);
	if (fn == NULL)
		goto endofcommand;
	proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
		fn);
	COUNT_BYTES(fn_len);

	END_OF_SMB

	return offset;
}

static int
dissect_open_file_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	int         fn_len;
	const char *fn;
	guint8      wc;
	guint16     bc;
	smb_fid_saved_info_t *fsi; /* eo_smb needs to track this info */

	DISSECTOR_ASSERT(si);

	WORD_COUNT;

	/* desired access */
	offset = dissect_access(tvb, tree, offset, hf_smb_desired_access);

	/* Search Attributes */
	offset = dissect_search_attributes(tvb, tree, offset);

	BYTE_COUNT;

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* file name */
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len,
		FALSE, FALSE, &bc);
	if (fn == NULL)
		goto endofcommand;
	proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
		fn);
	COUNT_BYTES(fn_len);

	/* store it for the fid->name/openframe/closeframe matching in
	* dissect_smb_fid()   called from the response.
	*/
	if ((!pinfo->fd->flags.visited) && si->sip && fn) {
		fsi			= wmem_new0(wmem_file_scope(), smb_fid_saved_info_t);
		fsi->filename		= wmem_strdup(wmem_file_scope(), fn);

		si->sip->extra_info_type = SMB_EI_FILEDATA;
		si->sip->extra_info      = fsi;
	}

	col_append_fstr(pinfo->cinfo, COL_INFO, ", Path: %s",
		    format_text(fn, strlen(fn)));

	END_OF_SMB

	return offset;
}



static int
dissect_nt_create_bits(tvbuff_t *tvb, proto_tree *parent_tree, int offset,
    int len, guint32 mask)
{
	proto_item *item = NULL;
	/*
	 * XXX - it's 0x00000016 in at least one capture, but
	 * Network Monitor doesn't say what the 0x00000010 bit is.
	 * Does the Win32 API documentation, or NT Native API book,
	 * suggest anything?
	 *
	 * That is the extended response desired bit ... RJS, from Samba
	 * Well, maybe. Samba thinks it is, and uses it to encode
	 * OpLock granted as the high order bit of the Action field
	 * in the response. However, Windows does not do that. Or at least
	 * Win2K doesn't.
	 */
	static const int * fields[] = {
		&hf_smb_nt_create_bits_oplock,
		&hf_smb_nt_create_bits_boplock,
		&hf_smb_nt_create_bits_dir,
		&hf_smb_nt_create_bits_ext_resp,
		NULL
	};

	item = proto_tree_add_bitmask_value_with_flags(parent_tree, tvb, offset, hf_smb_create_flags, ett_smb_nt_create_bits,
							fields, mask, BMT_NO_APPEND);

	if (len == 0)
		PROTO_ITEM_SET_GENERATED(item);

	offset += len;

	return offset;
}

/* FIXME: need to call dissect_nt_access_mask() instead */
static int
dissect_smb_access_mask_bits(tvbuff_t *tvb, proto_tree *parent_tree,
    int offset, int len, guint32 mask)
{
	proto_item *item;
	/*
	 * Some of these bits come from
	 *
	 *	http://www.samba.org/samba/ftp/specs/smb-nt01.doc
	 *
	 * and others come from the section on ZwOpenFile in "Windows(R)
	 * NT(R)/2000 Native API Reference".
	 */
	static const int * fields[] = {
		&hf_smb_nt_access_mask_read,
		&hf_smb_nt_access_mask_write,
		&hf_smb_nt_access_mask_append,
		&hf_smb_nt_access_mask_read_ea,
		&hf_smb_nt_access_mask_write_ea,
		&hf_smb_nt_access_mask_execute,
		&hf_smb_nt_access_mask_delete_child,
		&hf_smb_nt_access_mask_read_attributes,
		&hf_smb_nt_access_mask_write_attributes,
		&hf_smb_nt_access_mask_delete,
		&hf_smb_nt_access_mask_read_control,
		&hf_smb_nt_access_mask_write_dac,
		&hf_smb_nt_access_mask_write_owner,
		&hf_smb_nt_access_mask_synchronize,
		&hf_smb_nt_access_mask_system_security,
		&hf_smb_nt_access_mask_maximum_allowed,
		&hf_smb_nt_access_mask_generic_all,
		&hf_smb_nt_access_mask_generic_execute,
		&hf_smb_nt_access_mask_generic_write,
		&hf_smb_nt_access_mask_generic_read,
		NULL
	};

	item = proto_tree_add_bitmask_value_with_flags(parent_tree, tvb, offset, hf_smb_access_mask, ett_smb_nt_access_mask,
							fields, mask, BMT_NO_APPEND);

	if (len == 0)
		PROTO_ITEM_SET_GENERATED(item);
	offset += len;

	return offset;
}

int
dissect_smb_access_mask(tvbuff_t *tvb, proto_tree *parent_tree, int offset)
{
	guint32 mask;

	mask = tvb_get_letohl(tvb, offset);

	offset = dissect_smb_access_mask_bits(tvb, parent_tree, offset, 4, mask);

	return offset;
}

#define SHARE_ACCESS_READ	0x00000001
#define SHARE_ACCESS_WRITE	0x00000002
#define SHARE_ACCESS_DELETE	0x00000004

static int
dissect_nt_share_access_bits(tvbuff_t *tvb, proto_tree *parent_tree,
    int offset, int len, guint32 mask)
{
	proto_item *item;
	static const int * fields[] = {
		&hf_smb_nt_share_access_read,
		&hf_smb_nt_share_access_write,
		&hf_smb_nt_share_access_delete,
		NULL
	};

	item = proto_tree_add_bitmask_value(parent_tree, tvb, offset, hf_smb_share_access, ett_smb_nt_share_access, fields, mask);

	if (len == 0)
		PROTO_ITEM_SET_GENERATED(item);

	offset += len;

	return offset;
}

int
dissect_nt_share_access(tvbuff_t *tvb, proto_tree *parent_tree, int offset)
{
	guint32 mask;

	mask = tvb_get_letohl(tvb, offset);

	offset = dissect_nt_share_access_bits(tvb, parent_tree, offset, 4, mask);

	return offset;
}


static int
dissect_nt_create_options_bits(tvbuff_t *tvb, proto_tree *parent_tree,
    int offset, int len, guint32 mask)
{
	proto_item *item;
	/*
	 * From
	 *
	 *	http://www.samba.org/samba/ftp/specs/smb-nt01.doc
	 */
	static const int * fields[] = {
		&hf_smb_nt_create_options_directory_file,
		&hf_smb_nt_create_options_write_through,
		&hf_smb_nt_create_options_sequential_only,
		&hf_smb_nt_create_options_no_intermediate_buffering,
		&hf_smb_nt_create_options_sync_io_alert,
		&hf_smb_nt_create_options_sync_io_nonalert,
		&hf_smb_nt_create_options_non_directory_file,
		&hf_smb_nt_create_options_create_tree_connection,
		&hf_smb_nt_create_options_complete_if_oplocked,
		&hf_smb_nt_create_options_no_ea_knowledge,
		&hf_smb_nt_create_options_eight_dot_three_only,
		&hf_smb_nt_create_options_random_access,
		&hf_smb_nt_create_options_delete_on_close,
		&hf_smb_nt_create_options_open_by_fileid,
		&hf_smb_nt_create_options_backup_intent,
		&hf_smb_nt_create_options_no_compression,
		&hf_smb_nt_create_options_reserve_opfilter,
		&hf_smb_nt_create_options_open_reparse_point,
		&hf_smb_nt_create_options_open_no_recall,
		&hf_smb_nt_create_options_open_for_free_space_query,
		NULL
	};

	item = proto_tree_add_bitmask_value_with_flags(parent_tree, tvb, offset, hf_smb_create_options, ett_smb_nt_create_options, fields, mask, BMT_NO_APPEND);
	if (len == 0)
		PROTO_ITEM_SET_GENERATED(item);

	offset += len;

	return offset;
}

int
dissect_nt_create_options(tvbuff_t *tvb, proto_tree *parent_tree, int offset)
{
	guint32 mask;

	mask = tvb_get_letohl(tvb, offset);

	offset = dissect_nt_create_options_bits(tvb, parent_tree, offset, 4, mask);

	return offset;
}


/* fids are scoped by tcp session */
smb_fid_info_t *
dissect_smb_fid(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset,
    int len, guint16 fid, gboolean is_created, gboolean is_closed, gboolean is_generated, smb_info_t* si)
{
	smb_saved_info_t *sip;
	proto_item       *it;
	proto_tree       *tr;
	smb_fid_info_t   *fid_info         = NULL;
	smb_fid_info_t   *suspect_fid_info = NULL;
	/* We need this to use an array-accessed tree */
	GSList           *GSL_iterator;

	DISSECTOR_ASSERT(si);

	sip = si->sip;

	it = proto_tree_add_uint(tree, hf_smb_fid, tvb, offset, len, fid);
	if (is_generated) {
		PROTO_ITEM_SET_GENERATED(it);
	}
	tr = proto_item_add_subtree(it, ett_smb_fid);
	col_append_fstr(pinfo->cinfo, COL_INFO, ", FID: 0x%04x", fid);

	if ((!pinfo->fd->flags.visited) && is_created) {
		fid_info = wmem_new(wmem_file_scope(), smb_fid_info_t);
		fid_info->opened_in = pinfo->num;
		fid_info->closed_in = 0;
		fid_info->type = SMB_FID_TYPE_UNKNOWN;
		fid_info->fid = fid;
		fid_info->tid = si->tid;
		if (si->sip && (si->sip->extra_info_type == SMB_EI_FILEDATA)) {
			fid_info->fsi = (smb_fid_saved_info_t *)si->sip->extra_info;
		} else {
			fid_info->fsi = NULL;
		}
		/* We don't use the fid_tree anymore to access and
		   maintain the fid information of analyzed files.
		   (was wmem_tree_insert32(si->ct->fid_tree, fid, fid_info);)
		   We'll use a single list instead to keep track of the
		   files (fid) opened.
		   Note that the insert_sorted function allows to insert duplicates
		   but being inside this if section should prevent it */
		si->ct->GSL_fid_info = g_slist_insert_sorted(
					si->ct->GSL_fid_info,
					fid_info,
					(GCompareFunc)fid_cmp);
	}

	if (!fid_info) {
		/* we use the single linked list to access this fid_info
		   (was fid_info = wmem_tree_lookup32(si->ct->fid_tree, fid);) */
		GSL_iterator = si->ct->GSL_fid_info;
		while (GSL_iterator) {
			suspect_fid_info = (smb_fid_info_t *)GSL_iterator->data;
			if (suspect_fid_info->opened_in > pinfo->num) break;
			if ((suspect_fid_info->tid == si->tid) && (suspect_fid_info->fid == fid))
				fid_info = (smb_fid_info_t *)suspect_fid_info;
			GSL_iterator = g_slist_next(GSL_iterator);
		}
	}
	if (!fid_info) {
		return NULL;
	}

	/* Store the fid in the transaction structure and remember if
	   it was in the request or in the reply we saw it
	 */
	if (sip && (!is_generated) && (!pinfo->fd->flags.visited)) {
		sip->fid = fid;
		if (si->request) {
			sip->fid_seen_in_request = TRUE;
		} else {
			sip->fid_seen_in_request = FALSE;
		}
	}

	if ((!pinfo->fd->flags.visited) && is_closed) {
		fid_info->closed_in = pinfo->num;
	}

	if (fid_info->opened_in) {
		it = proto_tree_add_uint(tr, hf_smb_opened_in, tvb, 0, 0, fid_info->opened_in);
		PROTO_ITEM_SET_GENERATED(it);
	}

	if (fid_info->closed_in) {
		it = proto_tree_add_uint(tr, hf_smb_closed_in, tvb, 0, 0, fid_info->closed_in);
		PROTO_ITEM_SET_GENERATED(it);
	}


	if (fid_info->opened_in) {
		if (fid_info->fsi && fid_info->fsi->filename) {
			it = proto_tree_add_string(tr, hf_smb_file_name, tvb, 0, 0, fid_info->fsi->filename);
			PROTO_ITEM_SET_GENERATED(it);
			proto_item_append_text(tr, " (%s)", fid_info->fsi->filename);
			dissect_nt_create_bits(tvb, tr, 0, 0, fid_info->fsi->create_flags);
			dissect_smb_access_mask_bits(tvb, tr, 0, 0, fid_info->fsi->access_mask);
			dissect_file_ext_attr_bits(tvb, tr, 0, 0, fid_info->fsi->file_attributes);
			dissect_nt_share_access_bits(tvb, tr, 0, 0, fid_info->fsi->share_access);
			dissect_nt_create_options_bits(tvb, tr, 0, 0, fid_info->fsi->create_options);
			it = proto_tree_add_uint(tr, hf_smb_nt_create_disposition, tvb, 0, 0, fid_info->fsi->create_disposition);
			PROTO_ITEM_SET_GENERATED(it);
		}
	}

	return fid_info;
}

static int
dissect_open_file_response(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	guint8  wc;
	guint16 bc;
	guint16 fid;
	smb_fid_info_t *fid_info   = NULL; /* eo_smb needs to track this info */
	guint16         fattr;
	gboolean                isdir      = FALSE;

	WORD_COUNT;

	/* fid */
	fid = tvb_get_letohs(tvb, offset);

	fid_info = dissect_smb_fid(tvb, pinfo, tree, offset, 2, fid, TRUE, FALSE, FALSE, si);
	if (fid_info) {
		/* This command is used to create and open a new file or open
		and truncate an existing file to zero length */
		fid_info->end_of_file = 0;
		if (fid_info->fsi) {
			/* File Type */
			fattr = fid_info->fsi->file_attributes;
			/* XXX Volumes considered as directories */
			isdir = (fattr & SMB_FILE_ATTRIBUTE_DIRECTORY) || (fattr & SMB_FILE_ATTRIBUTE_VOLUME);
			if (isdir == 0) {
				fid_info->type = SMB_FID_TYPE_FILE;
			} else {
				fid_info->type = SMB_FID_TYPE_DIR;
			}
		}
	}

	offset += 2;

	/* File Attributes */
	offset = dissect_file_attributes(tvb, tree, offset);

	/* last write time */
	offset = dissect_smb_UTIME(tvb, tree, offset, hf_smb_last_write_time);

	/* File Size */
	proto_tree_add_item(tree, hf_smb_file_size, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	offset += 4;

	/* granted access */
	offset = dissect_access(tvb, tree, offset, hf_smb_granted_access);

	BYTE_COUNT;

	END_OF_SMB

	return offset;
}

static int
dissect_query_information2_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	guint8  wc;
	guint16 bc;
	guint16 fid;

	WORD_COUNT;

	/* fid */
	fid = tvb_get_letohs(tvb, offset);
	dissect_smb_fid(tvb, pinfo, tree, offset, 2, fid, FALSE, FALSE, FALSE, si);
	offset += 2;

	BYTE_COUNT;

	END_OF_SMB

	return offset;
}

static int
dissect_close_print_file_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	guint8  wc;
	guint16 bc;
	guint16 fid;

	WORD_COUNT;

	/* fid */
	fid = tvb_get_letohs(tvb, offset);
	dissect_smb_fid(tvb, pinfo, tree, offset, 2, fid, FALSE, TRUE, FALSE, si);
	offset += 2;

	BYTE_COUNT;

	END_OF_SMB

	return offset;
}

static int
dissect_open_print_file_response(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	guint8  wc;
	guint16 bc;
	guint16 fid;

	WORD_COUNT;

	/* fid */
	fid = tvb_get_letohs(tvb, offset);
	dissect_smb_fid(tvb, pinfo, tree, offset, 2, fid, FALSE, FALSE, FALSE, si);
	offset += 2;

	BYTE_COUNT;

	END_OF_SMB

	return offset;
}

static int
dissect_create_new_response(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	guint8  wc;
	guint16 bc;
	guint16 fid;

	WORD_COUNT;

	/* fid */
	fid = tvb_get_letohs(tvb, offset);
	dissect_smb_fid(tvb, pinfo, tree, offset, 2, fid, TRUE, FALSE, FALSE, si);
	offset += 2;

	BYTE_COUNT;

	END_OF_SMB

	return offset;
}

static int
dissect_flush_file_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	guint8  wc;
	guint16 bc;
	guint16 fid;

	WORD_COUNT;

	/* fid */
	fid = tvb_get_letohs(tvb, offset);
	dissect_smb_fid(tvb, pinfo, tree, offset, 2, fid, FALSE, FALSE, FALSE, si);
	offset += 2;

	BYTE_COUNT;

	END_OF_SMB

	return offset;
}

static int
dissect_create_file_response(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	guint8  wc;
	guint16 bc;
	guint16 fid;
	smb_fid_info_t *fid_info   = NULL; /* eo_smb needs to track this info */
	guint16         fattr;
	gboolean		isdir      = FALSE;

	WORD_COUNT;

	/* fid */
	fid = tvb_get_letohs(tvb, offset);
	fid_info = dissect_smb_fid(tvb, pinfo, tree, offset, 2, fid, TRUE, FALSE, FALSE, si);
	if (fid_info) {
		/* This command is used to create and open a new file or open
		and truncate an existing file to zero length */
		fid_info->end_of_file = 0;
		if (fid_info->fsi) {
			/* File Type */
			fattr = fid_info->fsi->file_attributes;
			/* XXX Volumes considered as directories */
			isdir = (fattr & SMB_FILE_ATTRIBUTE_DIRECTORY) || (fattr & SMB_FILE_ATTRIBUTE_VOLUME);
			if (isdir == 0) {
				fid_info->type = SMB_FID_TYPE_FILE;
			} else {
				fid_info->type = SMB_FID_TYPE_DIR;
			}
		}
	}

	offset += 2;

	BYTE_COUNT;

	END_OF_SMB

	return offset;
}

static int
dissect_create_file_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	int         fn_len;
	const char *fn;
	guint8      wc;
	guint16     bc;
	smb_fid_saved_info_t *fsi; /* eo_smb needs to track this info */
	guint32		file_attributes = 0;

	DISSECTOR_ASSERT(si);

	WORD_COUNT;

	/* file attributes */
	/* We read the two lower bytes into the four-bytes file-attributes, because they are compatible */
	file_attributes = tvb_get_letohs(tvb, offset);
	offset = dissect_file_attributes(tvb, tree, offset);

	/* creation time */
	offset = dissect_smb_UTIME(tvb, tree, offset, hf_smb_create_time);

	BYTE_COUNT;

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* File Name */
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len,
		FALSE, FALSE, &bc);
	if (fn == NULL)
		goto endofcommand;
	proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
		fn);
	COUNT_BYTES(fn_len);

	/* store it for the fid->name/openframe/closeframe matching in
	* dissect_smb_fid()   called from the response.
	*/
	if ((!pinfo->fd->flags.visited) && si->sip && fn) {
		fsi			= wmem_new0(wmem_file_scope(), smb_fid_saved_info_t);
		fsi->filename		= wmem_strdup(wmem_file_scope(), fn);
		fsi->file_attributes	= file_attributes;

		si->sip->extra_info_type = SMB_EI_FILEDATA;
		si->sip->extra_info      = fsi;
	}


	col_append_fstr(pinfo->cinfo, COL_INFO, ", Path: %s",
		    format_text(fn, strlen(fn)));

	END_OF_SMB

	return offset;
}

static int
dissect_close_file_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	guint8  wc;
	guint16 bc, fid;

	WORD_COUNT;

	/* fid */
	fid = tvb_get_letohs(tvb, offset);
	dissect_smb_fid(tvb, pinfo, tree, offset, 2, fid, FALSE, TRUE, FALSE, si);
	offset += 2;

	/* last write time */
	offset = dissect_smb_UTIME(tvb, tree, offset, hf_smb_last_write_time);

	BYTE_COUNT;

	END_OF_SMB

	return offset;
}

static int
dissect_delete_file_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	int         fn_len;
	const char *fn;
	guint8      wc;
	guint16     bc;

	DISSECTOR_ASSERT(si);

	WORD_COUNT;

	/* search attributes */
	offset = dissect_search_attributes(tvb, tree, offset);

	BYTE_COUNT;

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* file name */
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len,
		FALSE, FALSE, &bc);

	if ((!pinfo->fd->flags.visited) && si->sip) {
		si->sip->extra_info_type = SMB_EI_FILENAME;
		si->sip->extra_info = wmem_strdup(wmem_file_scope(), fn);
	}

	if (fn == NULL)
		goto endofcommand;
	proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
		fn);
	COUNT_BYTES(fn_len);

	col_append_fstr(pinfo->cinfo, COL_INFO, ", Path: %s",
		    format_text(fn, strlen(fn)));

	END_OF_SMB

	return offset;
}

static int
dissect_rename_file_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	int         fn_len;
	const char *fn, *old_name = NULL, *new_name = NULL;
	guint8      wc;
	guint16     bc;
	smb_rename_saved_info_t *rni = NULL;

	DISSECTOR_ASSERT(si);

	WORD_COUNT;

	/* search attributes */
	offset = dissect_search_attributes(tvb, tree, offset);

	BYTE_COUNT;

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* old file name */
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len,
		FALSE, FALSE, &bc);
	if (fn == NULL)
		goto endofcommand;
	old_name = fn;
	proto_tree_add_string(tree, hf_smb_old_file_name, tvb, offset, fn_len,
		fn);
	COUNT_BYTES(fn_len);

	col_append_fstr(pinfo->cinfo, COL_INFO, ", Old Name: %s",
		    format_text(fn, strlen(fn)));

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* file name */
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len,
		FALSE, FALSE, &bc);
	if (fn == NULL)
		goto endofcommand;
	new_name = fn;
	proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
		fn);
	COUNT_BYTES(fn_len);

	col_append_fstr(pinfo->cinfo, COL_INFO, ", New Name: %s",
		    format_text(fn, strlen(fn)));

	END_OF_SMB

	/* save the offset/len for this transaction */
	if (si->sip && !pinfo->fd->flags.visited) {
		rni = (smb_rename_saved_info_t *)wmem_alloc(wmem_file_scope(), sizeof(smb_rename_saved_info_t));
		rni->old_name = wmem_strdup(wmem_file_scope(), old_name);
		rni->new_name = wmem_strdup(wmem_file_scope(), new_name);

		si->sip->extra_info_type = SMB_EI_RENAMEDATA;
		si->sip->extra_info = rni;
	}

	return offset;
}

static int
dissect_nt_rename_file_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	int         fn_len;
	const char *fn;
	guint8      wc;
	guint16     bc;

	DISSECTOR_ASSERT(si);

	WORD_COUNT;

	/* search attributes */
	offset = dissect_search_attributes(tvb, tree, offset);

	proto_tree_add_uint(tree, hf_smb_nt_rename_level, tvb, offset, 2, tvb_get_letohs(tvb, offset));
	offset += 2;

	proto_tree_add_item(tree, hf_smb_cluster_count, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	offset += 4;

	BYTE_COUNT;

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* old file name */
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len,
		FALSE, FALSE, &bc);
	if (fn == NULL)
		goto endofcommand;
	proto_tree_add_string(tree, hf_smb_old_file_name, tvb, offset, fn_len,
		fn);
	COUNT_BYTES(fn_len);

	col_append_fstr(pinfo->cinfo, COL_INFO, ", Old Name: %s",
		    format_text(fn, strlen(fn)));

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* file name */
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len,
		FALSE, FALSE, &bc);
	if (fn == NULL)
		goto endofcommand;
	proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
		fn);
	COUNT_BYTES(fn_len);

	col_append_fstr(pinfo->cinfo, COL_INFO, ", New Name: %s",
		    format_text(fn, strlen(fn)));

	END_OF_SMB

	return offset;
}


static int
dissect_query_information_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	guint16     bc;
	guint8      wc;
	const char *fn;
	int         fn_len;

	DISSECTOR_ASSERT(si);

	WORD_COUNT;

	BYTE_COUNT;

	/* Buffer Format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* File Name */
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len,
		FALSE, FALSE, &bc);
	if (fn == NULL)
		goto endofcommand;
	proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
		fn);
	COUNT_BYTES(fn_len);

	col_append_fstr(pinfo->cinfo, COL_INFO, ", Path: %s",
		    format_text(fn, strlen(fn)));

	END_OF_SMB

	return offset;
}

static int
dissect_query_information_response(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si _U_)
{
	guint16 bc;
	guint8  wc;

	WORD_COUNT;

	/* File Attributes */
	offset = dissect_file_attributes(tvb, tree, offset);

	/* Last Write Time */
	offset = dissect_smb_UTIME(tvb, tree, offset, hf_smb_last_write_time);

	/* File Size */
	proto_tree_add_item(tree, hf_smb_file_size, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	offset += 4;

	/* 10 reserved bytes */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 10, ENC_NA);
	offset += 10;

	BYTE_COUNT;

	END_OF_SMB

	return offset;
}

static int
dissect_set_information_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	int         fn_len;
	const char *fn;
	guint8      wc;
	guint16     bc;

	DISSECTOR_ASSERT(si);

	WORD_COUNT;

	/* file attributes */
	offset = dissect_file_attributes(tvb, tree, offset);

	/* last write time */
	offset = dissect_smb_UTIME(tvb, tree, offset, hf_smb_last_write_time);

	/* 10 reserved bytes */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 10, ENC_NA);
	offset += 10;

	BYTE_COUNT;

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* file name */
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len,
		FALSE, FALSE, &bc);
	if (fn == NULL)
		goto endofcommand;
	proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
		fn);
	COUNT_BYTES(fn_len);

	col_append_fstr(pinfo->cinfo, COL_INFO, ", Path: %s",
		    format_text(fn, strlen(fn)));

	END_OF_SMB

	return offset;
}

typedef struct _rw_info_t {
	guint64 offset;
	guint32 len;
	guint16 fid;
} rw_info_t;

static int
dissect_read_file_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	guint8       wc;
	guint16      cnt = 0, bc;
	guint32      ofs = 0;
	unsigned int fid;
	rw_info_t    *rwi                   = NULL;

	WORD_COUNT;

	/* fid */
	fid = tvb_get_letohs(tvb, offset);
	dissect_smb_fid(tvb, pinfo, tree, offset, 2, (guint16) fid, FALSE, FALSE, FALSE, si);
	offset += 2;

	/* read count */
	cnt = tvb_get_letohs(tvb, offset);
	proto_tree_add_item(tree, hf_smb_count, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	/* offset */
	ofs = tvb_get_letohl(tvb, offset);
	proto_tree_add_item(tree, hf_smb_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	offset += 4;

	col_append_fstr(pinfo->cinfo, COL_INFO,
				", %u byte%s at offset %u", cnt,
				(cnt == 1) ? "" : "s", ofs);

	/* remaining */
	proto_tree_add_item(tree, hf_smb_remaining, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	/* save the offset/len for this transaction */
	if (si->sip && !pinfo->fd->flags.visited) {
		rwi = wmem_new(wmem_file_scope(), rw_info_t);
		rwi->offset = ofs;
		rwi->len = cnt;
		rwi->fid = fid;
		si->sip->extra_info_type = SMB_EI_RWINFO;
		si->sip->extra_info = rwi;
	}

	BYTE_COUNT;

	END_OF_SMB

	return offset;
}

int
dissect_file_data(tvbuff_t *tvb, proto_tree *tree, int offset, guint16 bc, guint16 datalen)
{
	int tvblen;

	if (bc > datalen) {
		/* We have some initial padding bytes. */
		/* XXX - use the data offset here instead? */
		proto_tree_add_item(tree, hf_smb_padding, tvb, offset, bc-datalen,
			ENC_NA);
		offset += bc-datalen;
		bc = datalen;
	}
	tvblen = tvb_reported_length_remaining(tvb, offset);
	if (bc > tvblen) {
		proto_tree_add_bytes_format_value(tree, hf_smb_file_data, tvb, offset, tvblen, NULL, "Incomplete. Only %d of %u bytes", tvblen, bc);
		offset += tvblen;
	} else {
		proto_tree_add_item(tree, hf_smb_file_data, tvb, offset, bc, ENC_NA);
		offset += bc;
	}
	return offset;
}

static int
dissect_file_data_dcerpc(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
    proto_tree *top_tree, int offset, guint16 bc, guint16 datalen, guint16 fid,
    void *data)
{
	int       tvblen;
	tvbuff_t *dcerpc_tvb;

	if (bc > datalen) {
		/* We have some initial padding bytes. */
		/* XXX - use the data offset here instead? */
		proto_tree_add_item(tree, hf_smb_padding, tvb, offset, bc-datalen,
			ENC_NA);
		offset += bc-datalen;
		bc = datalen;
	}
	tvblen = tvb_reported_length_remaining(tvb, offset);
	dcerpc_tvb = tvb_new_subset(tvb, offset, tvblen, bc);
	dissect_pipe_dcerpc(dcerpc_tvb, pinfo, top_tree, tree, fid, data);
	if (bc > tvblen)
		offset += tvblen;
	else
		offset += bc;
	return offset;
}

/*
 * transporting DCERPC over SMB seems to be implemented in various
 * ways. We might just assume it can be done by an almost random
 * mix of Trans/Read/Write calls
 *
 * if we suspect dcerpc, just send them all down to packet-smb-pipe.c
 * and let him sort them out
 */
static int
dissect_file_data_maybe_dcerpc(tvbuff_t *tvb, packet_info *pinfo,
    proto_tree *tree, proto_tree *top_tree, int offset, guint16 bc,
    guint16 datalen, guint32 ofs, guint16 fid, smb_info_t *si)
{
	DISSECTOR_ASSERT(si);

	if ( (si->sip && (si->sip->flags & SMB_SIF_TID_IS_IPC)) && (ofs == 0) ) {
		/* dcerpc call */
		return dissect_file_data_dcerpc(tvb, pinfo, tree,
		    top_tree, offset, bc, datalen, fid, si);
	} else {
		/* ordinary file data */
		return dissect_file_data(tvb, tree, offset, bc, datalen);
	}
}

static int
dissect_read_file_response(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	guint16    cnt	   = 0, bc;
	guint8     wc;
	int        fid	   = 0;
	guint32	   datalen = 0, dataoffset = 0;
	guint32	   tvblen;
	rw_info_t *rwi     = NULL;

	DISSECTOR_ASSERT(si);

	WORD_COUNT;

	/* read count */
	cnt = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_count, tvb, offset, 2, cnt);
	offset += 2;

	/* 8 reserved bytes */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 8, ENC_NA);
	offset += 8;
	BYTE_COUNT;

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* data len */
	CHECK_BYTE_COUNT(2);
	proto_tree_add_item(tree, hf_smb_data_len, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	datalen = tvb_get_letohs(tvb, offset);
	COUNT_BYTES(2);
	dataoffset = offset;

	/* file data, might be DCERPC on a pipe */
	if (bc) {
		offset = dissect_file_data_maybe_dcerpc(tvb, pinfo, tree,
		    top_tree_global, offset, bc, bc, 0, (guint16) fid, si);
		bc = 0;
	}

	/* If we have seen the request, then print which FID this refers to */
	if ((si->sip != NULL) && (si->sip->frame_req > 0) && (si->sip->extra_info_type == SMB_EI_FID)) {
		fid = GPOINTER_TO_INT(si->sip->extra_info);
	}

	if (si->sip && (si->sip->extra_info_type == SMB_EI_RWINFO)) {
		rwi = (rw_info_t *)si->sip->extra_info;
	}
	if (rwi) {
		proto_item *it;

		it = proto_tree_add_uint64(tree, hf_smb_file_rw_offset, tvb, 0, 0, rwi->offset);

		PROTO_ITEM_SET_GENERATED(it);
		it = proto_tree_add_uint(tree, hf_smb_file_rw_length, tvb, 0, 0, rwi->len);
		PROTO_ITEM_SET_GENERATED(it);

		/* we need the fid for the call to dcerpc below */
		fid = rwi->fid;
	}

	/* feed the export object tap listener */
	tvblen = tvb_reported_length_remaining(tvb, dataoffset);
	if (have_tap_listener(smb_eo_tap) && (datalen == tvblen) && rwi) {
		feed_eo_smb(SMB_COM_READ, fid, tvb, pinfo, dataoffset, datalen, rwi->len, rwi->offset, si);
	}

	END_OF_SMB

	return offset;
}

static int
dissect_lock_and_read_response(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si _U_)
{
	guint16 cnt, bc;
	guint8  wc;

	WORD_COUNT;

	/* read count */
	cnt = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_count, tvb, offset, 2, cnt);
	offset += 2;

	/* 8 reserved bytes */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 8, ENC_NA);
	offset += 8;

	BYTE_COUNT;

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* data len */
	CHECK_BYTE_COUNT(2);
	proto_tree_add_item(tree, hf_smb_data_len, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(2);

	END_OF_SMB

	return offset;
}



static int
dissect_write_file_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	guint32    ofs	   = 0;
	guint16    cnt	   = 0, bc, fid = 0;
	guint8     wc;
	rw_info_t *rwi	   = NULL;
	guint32	   datalen = 0, dataoffset = 0;
	guint32	   tvblen;

	DISSECTOR_ASSERT(si);

	WORD_COUNT;

	/* fid */
	fid = tvb_get_letohs(tvb, offset);
	dissect_smb_fid(tvb, pinfo, tree, offset, 2, fid, FALSE, FALSE, FALSE, si);
	offset += 2;

	/* write count */
	cnt = tvb_get_letohs(tvb, offset);
	datalen = cnt;
	proto_tree_add_uint(tree, hf_smb_count, tvb, offset, 2, cnt);
	offset += 2;

	/* offset */
	ofs = tvb_get_letohl(tvb, offset);
	proto_tree_add_item(tree, hf_smb_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	offset += 4;

	col_append_fstr(pinfo->cinfo, COL_INFO,
				", %u byte%s at offset %u", cnt,
				(cnt == 1) ? "" : "s", ofs);

	/* save the offset/len for this transaction */
	if (si->sip && !pinfo->fd->flags.visited) {
		rwi	    = (rw_info_t *)wmem_alloc(wmem_file_scope(), sizeof(rw_info_t));
		rwi->offset = ofs;
		rwi->len    = cnt;
		rwi->fid    = fid;

		si->sip->extra_info_type = SMB_EI_RWINFO;
		si->sip->extra_info	 = rwi;
	}
	if (si->sip && (si->sip->extra_info_type == SMB_EI_RWINFO)) {
		rwi = (rw_info_t *)si->sip->extra_info;
	}
	if (rwi) {
		proto_item *it;

		it = proto_tree_add_uint64(tree, hf_smb_file_rw_offset, tvb, 0, 0, rwi->offset);

		PROTO_ITEM_SET_GENERATED(it);
		it = proto_tree_add_uint(tree, hf_smb_file_rw_length, tvb, 0, 0, rwi->len);
		PROTO_ITEM_SET_GENERATED(it);
	}

	/* remaining */
	proto_tree_add_item(tree, hf_smb_remaining, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	BYTE_COUNT;

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* data len */
	CHECK_BYTE_COUNT(2);
	proto_tree_add_item(tree, hf_smb_data_len, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(2);
	dataoffset = offset;

	/* file data, might be DCERPC on a pipe */
	if (bc != 0) {
		offset = dissect_file_data_maybe_dcerpc(tvb, pinfo, tree,
		    top_tree_global, offset, bc, bc, ofs, fid, si);
		bc = 0;
	}

	/* feed the export object tap listener */
	tvblen = tvb_reported_length_remaining(tvb, dataoffset);
	if (have_tap_listener(smb_eo_tap) && (datalen == tvblen) && rwi) {
		feed_eo_smb(SMB_COM_WRITE, fid, tvb, pinfo, dataoffset, datalen, rwi->len, rwi->offset, si);
	}

	END_OF_SMB

	return offset;
}

static int
dissect_write_file_response(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	guint8      wc;
	guint16     bc, cnt;
	rw_info_t  *rwi = NULL;

	DISSECTOR_ASSERT(si);

	WORD_COUNT;

	/* write count */
	cnt = tvb_get_letohs(tvb, offset);
	proto_tree_add_item(tree, hf_smb_count, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	col_append_fstr(pinfo->cinfo, COL_INFO,
				", %u byte%s", cnt, (cnt == 1) ? "" : "s");

	if (si->sip && (si->sip->extra_info_type == SMB_EI_RWINFO)) {
		rwi = (rw_info_t *)si->sip->extra_info;
	}
	if (rwi) {
		proto_item *it;

		it = proto_tree_add_uint64(tree, hf_smb_file_rw_offset, tvb, 0, 0, rwi->offset);

		PROTO_ITEM_SET_GENERATED(it);
		it = proto_tree_add_uint(tree, hf_smb_file_rw_length, tvb, 0, 0, rwi->len);
		PROTO_ITEM_SET_GENERATED(it);
	}

	BYTE_COUNT;

	END_OF_SMB

	return offset;
}

static int
dissect_lock_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	guint8  wc;
	guint16 bc, fid;

	WORD_COUNT;

	/* fid */
	fid = tvb_get_letohs(tvb, offset);
	dissect_smb_fid(tvb, pinfo, tree, offset, 2, fid, FALSE, FALSE, FALSE, si);
	offset += 2;

	/* lock count */
	proto_tree_add_item(tree, hf_smb_count, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	offset += 4;

	/* offset */
	proto_tree_add_item(tree, hf_smb_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	offset += 4;

	BYTE_COUNT;

	END_OF_SMB

	return offset;
}

static int
dissect_create_temporary_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	int         fn_len;
	const char *fn;
	guint8      wc;
	guint16     bc;

	DISSECTOR_ASSERT(si);

	WORD_COUNT;

	/* 2 reserved bytes */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 2, ENC_NA);
	offset += 2;

	/* Creation time */
	offset = dissect_smb_UTIME(tvb, tree, offset, hf_smb_create_time);

	BYTE_COUNT;

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* directory name */
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len,
		FALSE, FALSE, &bc);
	if (fn == NULL)
		goto endofcommand;
	proto_tree_add_string(tree, hf_smb_dir_name, tvb, offset, fn_len,
		fn);
	COUNT_BYTES(fn_len);

	col_append_fstr(pinfo->cinfo, COL_INFO, ", Path: %s",
		    format_text(fn, strlen(fn)));

	END_OF_SMB

	return offset;
}

static int
dissect_create_temporary_response(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	int         fn_len;
	const char *fn;
	guint8      wc;
	guint16     bc, fid;

	DISSECTOR_ASSERT(si);

	WORD_COUNT;

	/* fid */
	fid = tvb_get_letohs(tvb, offset);
	dissect_smb_fid(tvb, pinfo, tree, offset, 2, fid, TRUE, FALSE, FALSE, si);
	offset += 2;

	BYTE_COUNT;

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* file name */
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len,
		FALSE, FALSE, &bc);
	if (fn == NULL)
		goto endofcommand;
	proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
		fn);
	COUNT_BYTES(fn_len);

	END_OF_SMB

	return offset;
}

static const value_string seek_mode_vals[] = {
	{0,	"From Start Of File"},
	{1,	"From Current Position"},
	{2,	"From End Of File"},
	{0,	NULL}
};

static int
dissect_seek_file_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	guint8  wc;
	guint16 bc, fid;

	WORD_COUNT;

	/* fid */
	fid = tvb_get_letohs(tvb, offset);
	dissect_smb_fid(tvb, pinfo, tree, offset, 2, fid, FALSE, FALSE, FALSE, si);
	offset += 2;

	/* Seek Mode */
	proto_tree_add_item(tree, hf_smb_seek_mode, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	/* offset */
	proto_tree_add_item(tree, hf_smb_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	offset += 4;

	BYTE_COUNT;

	END_OF_SMB

	return offset;
}

static int
dissect_seek_file_response(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si _U_)
{
	guint8  wc;
	guint16 bc;

	WORD_COUNT;

	/* offset */
	proto_tree_add_item(tree, hf_smb_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	offset += 4;

	BYTE_COUNT;

	END_OF_SMB

	return offset;
}

static int
dissect_set_information2_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	guint8  wc;
	guint16 bc, fid;

	WORD_COUNT;

	/* fid */
	fid = tvb_get_letohs(tvb, offset);
	dissect_smb_fid(tvb, pinfo, tree, offset, 2, fid, FALSE, FALSE, FALSE, si);
	offset += 2;

	/* create time */
	offset = dissect_smb_datetime(tvb, tree, offset,
		hf_smb_create_time,
		hf_smb_create_dos_date, hf_smb_create_dos_time, FALSE);

	/* access time */
	offset = dissect_smb_datetime(tvb, tree, offset,
		hf_smb_access_time,
		hf_smb_access_dos_date, hf_smb_access_dos_time, FALSE);

	/* last write time */
	offset = dissect_smb_datetime(tvb, tree, offset,
		hf_smb_last_write_time,
		hf_smb_last_write_dos_date, hf_smb_last_write_dos_time, FALSE);

	BYTE_COUNT;

	END_OF_SMB

	return offset;
}

static int
dissect_query_information2_response(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si _U_)
{
	guint8  wc;
	guint16 bc;

	WORD_COUNT;

	/* create time */
	offset = dissect_smb_datetime(tvb, tree, offset,
		hf_smb_create_time,
		hf_smb_create_dos_date, hf_smb_create_dos_time, FALSE);

	/* access time */
	offset = dissect_smb_datetime(tvb, tree, offset,
		hf_smb_access_time,
		hf_smb_access_dos_date, hf_smb_access_dos_time, FALSE);

	/* last write time */
	offset = dissect_smb_datetime(tvb, tree, offset,
		hf_smb_last_write_time,
		hf_smb_last_write_dos_date, hf_smb_last_write_dos_time, FALSE);

	/* data size */
	proto_tree_add_item(tree, hf_smb_data_size, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	offset += 4;

	/* allocation size */
	proto_tree_add_item(tree, hf_smb_alloc_size, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	offset += 4;

	/* File Attributes */
	offset = dissect_file_attributes(tvb, tree, offset);

	BYTE_COUNT;

	END_OF_SMB

	return offset;
}

static int
dissect_write_and_close_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	guint8  wc;
	guint16 cnt = 0;
	guint16 bc, fid;

	WORD_COUNT;

	/* fid */
	fid = tvb_get_letohs(tvb, offset);
	dissect_smb_fid(tvb, pinfo, tree, offset, 2, fid, FALSE, TRUE, FALSE, si);
	offset += 2;

	/* write count */
	cnt = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_count, tvb, offset, 2, cnt);
	offset += 2;

	/* offset */
	proto_tree_add_item(tree, hf_smb_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	offset += 4;

	/* last write time */
	offset = dissect_smb_UTIME(tvb, tree, offset, hf_smb_last_write_time);

	if (wc == 12) {
		/* 12 reserved bytes */
		proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 12, ENC_NA);
		offset += 12;
	}

	BYTE_COUNT;

	/* 1 pad byte */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_padding, tvb, offset, 1, ENC_NA);
	COUNT_BYTES(1);

	offset = dissect_file_data(tvb, tree, offset, cnt, cnt);
	bc = 0;	/* XXX */

	END_OF_SMB

	return offset;
}

static int
dissect_write_and_close_response(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si _U_)
{
	guint8  wc;
	guint16 bc;

	WORD_COUNT;

	/* write count */
	proto_tree_add_item(tree, hf_smb_count, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	BYTE_COUNT;

	END_OF_SMB

	return offset;
}

/* Timeout is defined on page 117 of SMB Protocol Extensions version 2.0
   available at http://us1.samba.org/samba/ftp/SMB-info/DOSEXTP.TXT
*/
static gchar *
smbext20_timeout_msecs_to_str(gint32 timeout)
{
	gchar *buf;
#define SMBEXT20_TIMEOUT_MSECS_TO_STR_MAXLEN 60

	if (timeout <= 0) {
	        buf = (gchar *)wmem_alloc(wmem_packet_scope(), SMBEXT20_TIMEOUT_MSECS_TO_STR_MAXLEN+1);
		if (timeout == 0) {
		        g_snprintf(buf, SMBEXT20_TIMEOUT_MSECS_TO_STR_MAXLEN+1, "Return immediately (0)");
		} else if (timeout == -1) {
		        g_snprintf(buf, SMBEXT20_TIMEOUT_MSECS_TO_STR_MAXLEN+1, "Wait indefinitely (-1)");
		} else if (timeout == -2) {
		        g_snprintf(buf, SMBEXT20_TIMEOUT_MSECS_TO_STR_MAXLEN+1, "Use default timeout (-2)");
		} else {
		        g_snprintf(buf, SMBEXT20_TIMEOUT_MSECS_TO_STR_MAXLEN+1, "Unknown reserved value (%d)", timeout);
		}
		return buf;
	}

	return signed_time_msecs_to_str(wmem_packet_scope(), timeout);
}

static int
dissect_read_raw_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	guint8  wc;
	guint16 bc, fid;
	guint32 to;

	WORD_COUNT;

	/* fid */
	fid = tvb_get_letohs(tvb, offset);
	dissect_smb_fid(tvb, pinfo, tree, offset, 2, fid, FALSE, FALSE, FALSE, si);
	offset += 2;

	/* offset */
	proto_tree_add_item(tree, hf_smb_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	offset += 4;

	/* max count */
	proto_tree_add_item(tree, hf_smb_max_count, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	/* min count */
	proto_tree_add_item(tree, hf_smb_min_count, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	/* timeout */
	to = tvb_get_letohl(tvb, offset);
	proto_tree_add_uint_format_value(tree, hf_smb_timeout, tvb, offset, 4, to, "%s", smbext20_timeout_msecs_to_str(to));
	offset += 4;

	/* 2 reserved bytes */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 2, ENC_NA);
	offset += 2;

	if (wc == 10) {
		/* high offset */
		proto_tree_add_item(tree, hf_smb_high_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		offset += 4;
	}

	BYTE_COUNT;

	END_OF_SMB

	return offset;
}

static int
dissect_query_information_disk_response(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si _U_)
{
	guint8  wc;
	guint16 bc;

	WORD_COUNT;

	/* units */
	proto_tree_add_item(tree, hf_smb_units, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	/* bpu */
	proto_tree_add_item(tree, hf_smb_bpu, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	/* block size */
	proto_tree_add_item(tree, hf_smb_blocksize, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	/* free units */
	proto_tree_add_item(tree, hf_smb_freeunits, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	/* 2 reserved bytes */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 2, ENC_NA);
	offset += 2;

	BYTE_COUNT;

	END_OF_SMB

	return offset;
}

static int
dissect_read_mpx_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	guint8  wc;
	guint16 bc, fid;

	WORD_COUNT;

	/* fid */
	fid = tvb_get_letohs(tvb, offset);
	dissect_smb_fid(tvb, pinfo, tree, offset, 2, fid, FALSE, FALSE, FALSE, si);
	offset += 2;

	/* offset */
	proto_tree_add_item(tree, hf_smb_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	offset += 4;

	/* max count */
	proto_tree_add_item(tree, hf_smb_max_count, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	/* min count */
	proto_tree_add_item(tree, hf_smb_min_count, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	/* 6 reserved bytes */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 6, ENC_NA);
	offset += 6;

	BYTE_COUNT;

	END_OF_SMB

	return offset;
}

static int
dissect_read_mpx_response(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si _U_)
{
	guint16 datalen = 0, bc;
	guint8  wc;

	WORD_COUNT;

	/* offset */
	proto_tree_add_item(tree, hf_smb_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	offset += 4;

	/* count */
	proto_tree_add_item(tree, hf_smb_count, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	/* 2 reserved bytes */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 2, ENC_NA);
	offset += 2;

	/* data compaction mode */
	proto_tree_add_item(tree, hf_smb_dcm, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	/* 2 reserved bytes */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 2, ENC_NA);
	offset += 2;

	/* data len */
	datalen = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_data_len, tvb, offset, 2, datalen);
	offset += 2;

	/* data offset */
	proto_tree_add_item(tree, hf_smb_data_offset, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	BYTE_COUNT;

	/* file data */
	offset = dissect_file_data(tvb, tree, offset, bc, datalen);
	bc = 0;

	END_OF_SMB

	return offset;
}


static const true_false_string tfs_write_mode_write_through = {
	"WRITE THROUGH requested",
	"Write through not requested"
};
static const true_false_string tfs_write_mode_return_remaining = {
	"RETURN REMAINING (pipe/dev) requested",
	"DON'T return remaining (pipe/dev)"
};
static const true_false_string tfs_write_mode_raw = {
	"Use WriteRawNamedPipe (pipe)",
	"DON'T use WriteRawNamedPipe (pipe)"
};
static const true_false_string tfs_write_mode_message_start = {
	"This is the START of a MESSAGE (pipe)",
	"This is NOT the start of a message (pipe)"
};
static const true_false_string tfs_write_mode_connectionless = {
	"CONNECTIONLESS mode requested",
	"Connectionless mode NOT requested"
};

#define WRITE_MODE_CONNECTIONLESS	0x0080
#define WRITE_MODE_MESSAGE_START	0x0008
#define WRITE_MODE_RAW			0x0004
#define WRITE_MODE_RETURN_REMAINING	0x0002
#define WRITE_MODE_WRITE_THROUGH	0x0001

static int
dissect_write_mode(tvbuff_t *tvb, proto_tree *parent_tree, int offset, int bm)
{
	guint16     mask;
	proto_item *item;
	proto_tree *tree;

	mask = tvb_get_letohs(tvb, offset);

	if (parent_tree) {
		item = proto_tree_add_item(parent_tree, hf_smb_write_mode, tvb, offset, 2, ENC_LITTLE_ENDIAN);
		tree = proto_item_add_subtree(item, ett_smb_rawmode);

		if (bm&WRITE_MODE_CONNECTIONLESS) {
			proto_tree_add_boolean(tree, hf_smb_write_mode_connectionless,
				tvb, offset, 2, mask);
		}
		if (bm&WRITE_MODE_MESSAGE_START) {
			proto_tree_add_boolean(tree, hf_smb_write_mode_message_start,
				tvb, offset, 2, mask);
				}
		if (bm&WRITE_MODE_RAW) {
			proto_tree_add_boolean(tree, hf_smb_write_mode_raw,
				tvb, offset, 2, mask);
		}
		if (bm&WRITE_MODE_RETURN_REMAINING) {
			proto_tree_add_boolean(tree, hf_smb_write_mode_return_remaining,
				tvb, offset, 2, mask);
		}
		if (bm&WRITE_MODE_WRITE_THROUGH) {
			proto_tree_add_boolean(tree, hf_smb_write_mode_write_through,
				tvb, offset, 2, mask);
		}
	}

	offset += 2;
	return offset;
}

static int
dissect_write_raw_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	guint32 to;
	guint16 datalen = 0, bc, fid;
	guint8  wc;

	WORD_COUNT;

	/* fid */
	fid = tvb_get_letohs(tvb, offset);
	dissect_smb_fid(tvb, pinfo, tree, offset, 2, fid, FALSE, FALSE, FALSE, si);
	offset += 2;

	/* total data length */
	proto_tree_add_item(tree, hf_smb_total_data_len, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	/* 2 reserved bytes */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 2, ENC_NA);
	offset += 2;

	/* offset */
	proto_tree_add_item(tree, hf_smb_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	offset += 4;

	/* timeout */
	to = tvb_get_letohl(tvb, offset);
	proto_tree_add_uint_format_value(tree, hf_smb_timeout, tvb, offset, 4, to, "%s", smbext20_timeout_msecs_to_str(to));
	offset += 4;

	/* mode */
	offset = dissect_write_mode(tvb, tree, offset, 0x0003);

	/* 4 reserved bytes */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 4, ENC_NA);
	offset += 4;

	/* data len */
	datalen = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_data_len, tvb, offset, 2, datalen);
	offset += 2;

	/* data offset */
	proto_tree_add_item(tree, hf_smb_data_offset, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	BYTE_COUNT;

	/* file data */
	/* XXX - use the data offset to determine where the data starts? */
	offset = dissect_file_data(tvb, tree, offset, bc, datalen);
	bc = 0;

	END_OF_SMB

	return offset;
}

static int
dissect_write_raw_response(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si _U_)
{
	guint8  wc;
	guint16 bc;

	WORD_COUNT;

	/* remaining */
	proto_tree_add_item(tree, hf_smb_remaining, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	BYTE_COUNT;

	END_OF_SMB

	return offset;
}

static int
dissect_write_mpx_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	guint32 to;
	guint16 datalen = 0, bc, fid;
	guint8  wc;

	WORD_COUNT;

	/* fid */
	fid = tvb_get_letohs(tvb, offset);
	dissect_smb_fid(tvb, pinfo, tree, offset, 2, fid, FALSE, FALSE, FALSE, si);
	offset += 2;

	/* total data length */
	proto_tree_add_item(tree, hf_smb_total_data_len, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	/* 2 reserved bytes */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 2, ENC_NA);
	offset += 2;

	/* offset */
	proto_tree_add_item(tree, hf_smb_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	offset += 4;

	/* timeout */
	to = tvb_get_letohl(tvb, offset);
	proto_tree_add_uint_format_value(tree, hf_smb_timeout, tvb, offset, 4, to, "%s", smbext20_timeout_msecs_to_str(to));
	offset += 4;

	/* mode */
	offset = dissect_write_mode(tvb, tree, offset, 0x0083);

	/* request mask */
	proto_tree_add_item(tree, hf_smb_request_mask, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	offset += 4;

	/* data len */
	datalen = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_data_len, tvb, offset, 2, datalen);
	offset += 2;

	/* data offset */
	proto_tree_add_item(tree, hf_smb_data_offset, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	BYTE_COUNT;

	/* file data */
	/* XXX - use the data offset to determine where the data starts? */
	offset = dissect_file_data(tvb, tree, offset, bc, datalen);
	bc = 0;

	END_OF_SMB

	return offset;
}

static int
dissect_write_mpx_response(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si _U_)
{
	guint8  wc;
	guint16 bc;

	WORD_COUNT;

	/* response mask */
	proto_tree_add_item(tree, hf_smb_response_mask, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	offset += 4;

	BYTE_COUNT;

	END_OF_SMB

	return offset;
}

static int
dissect_sid(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si _U_)
{
	guint8  wc;
	guint16 bc;

	WORD_COUNT;

	/* sid */
	proto_tree_add_item(tree, hf_smb_search_id, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	BYTE_COUNT;

	END_OF_SMB

	return offset;
}

static int
dissect_search_resume_key(tvbuff_t *tvb, packet_info *pinfo _U_,
    proto_tree *parent_tree, int offset, guint16 *bcp, gboolean *trunc,
    gboolean has_find_id, smb_info_t *si)
{
	proto_tree *tree;
	int         fn_len;
	const char *fn;
	char        fname[11+1];

	DISSECTOR_ASSERT(si);

	tree = proto_tree_add_subtree(parent_tree, tvb, offset, 21,
			ett_smb_search_resume_key, NULL, "Resume Key");

	/* reserved byte */
	CHECK_BYTE_COUNT_SUBR(1);
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 1, ENC_NA);
	COUNT_BYTES_SUBR(1);

	/* file name */
	fn_len = 11;
	fn = get_unicode_or_ascii_string(tvb, &offset, FALSE/*never Unicode*/, &fn_len,
		TRUE, TRUE, bcp);
	CHECK_STRING_SUBR(fn);
	/* ensure that it's null-terminated */
	g_strlcpy(fname, fn, 11+1);
	proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, 11,
		fname);
	COUNT_BYTES_SUBR(fn_len);

	if (has_find_id) {
		CHECK_BYTE_COUNT_SUBR(1);
		proto_tree_add_item(tree, hf_smb_resume_find_id, tvb, offset, 1, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_SUBR(1);

		/* server cookie */
		CHECK_BYTE_COUNT_SUBR(4);
		proto_tree_add_item(tree, hf_smb_resume_server_cookie, tvb, offset, 4, ENC_NA);
		COUNT_BYTES_SUBR(4);
	} else {
		/* server cookie */
		CHECK_BYTE_COUNT_SUBR(5);
		proto_tree_add_item(tree, hf_smb_resume_server_cookie, tvb, offset, 5, ENC_NA);
		COUNT_BYTES_SUBR(5);
	}

	/* client cookie */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_resume_client_cookie, tvb, offset, 4, ENC_NA);
	COUNT_BYTES_SUBR(4);

	*trunc = FALSE;
	return offset;
}

static int
dissect_search_dir_info(tvbuff_t *tvb, packet_info *pinfo,
    proto_tree *parent_tree, int offset, guint16 *bcp, gboolean *trunc,
    gboolean has_find_id, smb_info_t *si)
{
	proto_tree *tree;
	int         fn_len;
	const char *fn;
	char        fname[13+1];

	DISSECTOR_ASSERT(si);

	tree = proto_tree_add_subtree(parent_tree, tvb, offset, 46,
			ett_smb_search_dir_info, NULL, "Directory Information");

	/* resume key */
	offset = dissect_search_resume_key(tvb, pinfo, tree, offset, bcp,
	    trunc, has_find_id, si);
	if (*trunc)
		return offset;

	/* File Attributes */
	CHECK_BYTE_COUNT_SUBR(1);
	offset = dissect_dir_info_file_attributes(tvb, tree, offset);
	*bcp -= 1;

	/* last write time */
	CHECK_BYTE_COUNT_SUBR(4);
	offset = dissect_smb_datetime(tvb, tree, offset,
		hf_smb_last_write_time,
		hf_smb_last_write_dos_date, hf_smb_last_write_dos_time,
		TRUE);
	*bcp -= 4;

	/* File Size */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_file_size, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* file name */
	fn_len = 13;
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len,
		TRUE, TRUE, bcp);
	CHECK_STRING_SUBR(fn);
	/* ensure that it's null-terminated */
	g_strlcpy(fname, fn, 13+1);
	proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
		fname);
	COUNT_BYTES_SUBR(fn_len);

	*trunc = FALSE;
	return offset;
}


static int
dissect_search_find_request(tvbuff_t *tvb, packet_info *pinfo,
    proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si,
    gboolean has_find_id)
{
	int         fn_len;
	const char *fn;
	guint16     rkl;
	guint8      wc;
	guint16     bc;
	gboolean    trunc;

	DISSECTOR_ASSERT(si);

	WORD_COUNT;

	/* max count */
	proto_tree_add_item(tree, hf_smb_max_count, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	/* Search Attributes */
	offset = dissect_search_attributes(tvb, tree, offset);

	BYTE_COUNT;

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* file name */
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len,
		TRUE, FALSE, &bc);
	if (fn == NULL)
		goto endofcommand;
	proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
		fn);
	COUNT_BYTES(fn_len);

	col_append_fstr(pinfo->cinfo, COL_INFO, ", File: %s",
		    format_text(fn, strlen(fn)));

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* resume key length */
	CHECK_BYTE_COUNT(2);
	rkl = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_resume_key_len, tvb, offset, 2, rkl);
	COUNT_BYTES(2);

	/* resume key */
	if (rkl) {
		offset = dissect_search_resume_key(tvb, pinfo, tree, offset,
		    &bc, &trunc, has_find_id, si);
		if (trunc)
			goto endofcommand;
	}

	END_OF_SMB

	return offset;
}

static int
dissect_search_dir_request(tvbuff_t *tvb, packet_info *pinfo,
    proto_tree *tree, int offset, proto_tree *smb_tree, smb_info_t *si)
{
	return dissect_search_find_request(tvb, pinfo, tree, offset,
	    smb_tree, si, FALSE);
}

static int
dissect_find_request(tvbuff_t *tvb, packet_info *pinfo,
    proto_tree *tree, int offset, proto_tree *smb_tree, smb_info_t *si)
{
	return dissect_search_find_request(tvb, pinfo, tree, offset,
	    smb_tree, si, TRUE);
}

static int
dissect_find_close_request(tvbuff_t *tvb, packet_info *pinfo,
    proto_tree *tree, int offset, proto_tree *smb_tree, smb_info_t *si)
{
	return dissect_search_find_request(tvb, pinfo, tree, offset,
	    smb_tree, si, TRUE);
}

static int
dissect_search_find_response(tvbuff_t *tvb, packet_info *pinfo,
    proto_tree *tree, int offset, proto_tree *smb_tree _U_,
    gboolean has_find_id, smb_info_t *si)
{
	guint16  count = 0;
	guint8   wc;
	guint16  bc;
	gboolean trunc;

	WORD_COUNT;

	/* count */
	count = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_count, tvb, offset, 2, count);
	offset += 2;

	BYTE_COUNT;

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* data len */
	CHECK_BYTE_COUNT(2);
	proto_tree_add_item(tree, hf_smb_data_len, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(2);

	while (count--) {
		offset = dissect_search_dir_info(tvb, pinfo, tree, offset,
		    &bc, &trunc, has_find_id, si);
		if (trunc)
			goto endofcommand;
	}

	END_OF_SMB

	return offset;
}

static int
dissect_search_dir_response(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree, smb_info_t *si)
{
	return dissect_search_find_response(tvb, pinfo, tree, offset, smb_tree,
	    FALSE, si);
}

static int
dissect_find_response(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree, smb_info_t *si)
{
	return dissect_search_find_response(tvb, pinfo, tree, offset, smb_tree,
	    TRUE, si);
}

static int
dissect_find_close_response(tvbuff_t *tvb, packet_info *pinfo _U_,
    proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si _U_)
{
	guint8  wc;
	guint16 bc;
	guint16 data_len;

	WORD_COUNT;

	/* reserved */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 2, ENC_NA);
	offset += 2;

	BYTE_COUNT;

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* data len */
	CHECK_BYTE_COUNT(2);
	data_len = tvb_get_ntohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_data_len, tvb, offset, 2, data_len);
	COUNT_BYTES(2);

	if (data_len != 0) {
		CHECK_BYTE_COUNT(data_len);
		proto_tree_add_item(tree, hf_smb_reserved, tvb, offset,
		    data_len, ENC_NA);
		COUNT_BYTES(data_len);
	}

	END_OF_SMB

	return offset;
}

static const value_string locking_ol_vals[] = {
	{0,	"Client is not holding oplock on this file"},
	{1,	"Level 2 oplock currently held by client"},
	{0, NULL}
};

static const true_false_string tfs_lock_type_large = {
	"Large file locking format requested",
	"Large file locking format not requested"
};
static const true_false_string tfs_lock_type_cancel = {
	"Cancel outstanding lock request",
	"Don't cancel outstanding lock request"
};
static const true_false_string tfs_lock_type_change = {
	"Change lock type",
	"Don't change lock type"
};
static const true_false_string tfs_lock_type_oplock = {
	"This is an oplock break notification/response",
	"This is not an oplock break notification/response"
};
static const true_false_string tfs_lock_type_shared = {
	"This is a shared lock",
	"This is an exclusive lock"
};
static int
dissect_locking_andx_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree, smb_info_t *si)
{
	guint8      wc, cmd    = 0xff, lt = 0, ol = 0;
	guint16     andxoffset = 0, un = 0, ln = 0, bc, fid, num_lock = 0, num_unlock = 0;
	guint32     to;
	proto_item *it         = NULL;
	proto_tree *tr         = NULL;
	int         old_offset = offset;
	smb_locking_saved_info_t *ld = NULL;
	static const int * locks[] = {
		&hf_smb_lock_type_large,
		&hf_smb_lock_type_cancel,
		&hf_smb_lock_type_change,
		&hf_smb_lock_type_oplock,
		&hf_smb_lock_type_shared,
		NULL
	};

	DISSECTOR_ASSERT(si);

	WORD_COUNT;

	/* next smb command */
	cmd = tvb_get_guint8(tvb, offset);
	if (cmd != 0xff) {
		proto_tree_add_uint(tree, hf_smb_andxcmd, tvb, offset, 1, cmd);
	} else {
		proto_tree_add_uint_format_value(tree, hf_smb_andxcmd, tvb, offset, 1, cmd, "No further commands (0xff)");
	}
	offset += 1;

	/* reserved byte */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 1, ENC_NA);
	offset += 1;

	/* andxoffset */
	andxoffset = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_andxoffset, tvb, offset, 2, andxoffset);
	offset += 2;

	/* fid */
	fid = tvb_get_letohs(tvb, offset);
	dissect_smb_fid(tvb, pinfo, tree, offset, 2, fid, FALSE, FALSE, FALSE, si);
	offset += 2;

	/* lock type */
	lt = tvb_get_guint8(tvb, offset);
	proto_tree_add_bitmask(tree, tvb, offset, hf_smb_lock_type, ett_smb_lock_type, locks, ENC_NA);
	offset += 1;

	/* oplock level */
	ol = tvb_get_guint8(tvb, offset);
	proto_tree_add_item(tree, hf_smb_locking_ol, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	offset += 1;

	/* timeout */
	to = tvb_get_letohl(tvb, offset);
	proto_tree_add_uint_format_value(tree, hf_smb_timeout, tvb, offset, 4, to, "%s", smbext20_timeout_msecs_to_str(to));
	offset += 4;

	/* number of unlocks */
	un = tvb_get_letohs(tvb, offset);
	num_unlock = un;
	proto_tree_add_uint(tree, hf_smb_number_of_unlocks, tvb, offset, 2, un);
	offset += 2;

	/* number of locks */
	ln = tvb_get_letohs(tvb, offset);
	num_lock = ln;
	proto_tree_add_uint(tree, hf_smb_number_of_locks, tvb, offset, 2, ln);
	offset += 2;

	BYTE_COUNT;

	/* store the locking data for the response */
	if ((!pinfo->fd->flags.visited) && si->sip) {
		ld = (smb_locking_saved_info_t *)wmem_alloc(wmem_file_scope(), sizeof(smb_locking_saved_info_t));
		ld->type	 = lt;
		ld->oplock_level = ol;
		ld->num_lock	 = num_lock;
		ld->num_unlock	 = num_unlock;
		ld->locks	 = NULL;
		ld->unlocks	 = NULL;
		si->sip->extra_info_type = SMB_EI_LOCKDATA;
		si->sip->extra_info      = ld;
	}

	/* unlocks */
	if (un) {
		old_offset = offset;

		tr = proto_tree_add_subtree(tree, tvb, offset, -1, ett_smb_unlocks, &it, "Unlocks");
		while (un--) {
			proto_tree *ltree_2;
			if (lt&0x10) {
				guint64 val;
				guint16 lock_pid;
				guint64 lock_offset;
				guint64 lock_length;

				/* large lock format */
				ltree_2 = proto_tree_add_subtree(tr, tvb, offset, 20, ett_smb_unlock, NULL, "Unlock");

				/* PID */
				CHECK_BYTE_COUNT(2);
				lock_pid = tvb_get_letohs(tvb, offset);
				proto_tree_add_item(ltree_2, hf_smb_pid, tvb, offset, 2, ENC_LITTLE_ENDIAN);
				COUNT_BYTES(2);

				/* 2 reserved bytes */
				CHECK_BYTE_COUNT(2);
				proto_tree_add_item(ltree_2, hf_smb_reserved, tvb, offset, 2, ENC_NA);
				COUNT_BYTES(2);

				/* offset */
				CHECK_BYTE_COUNT(8);
				val = ((guint64)tvb_get_letohl(tvb, offset)) << 32
				    | tvb_get_letohl(tvb, offset+4);
				lock_offset = val;
				proto_tree_add_uint64(ltree_2, hf_smb_lock_long_offset, tvb, offset, 8, val);
				COUNT_BYTES(8);

				/* length */
				CHECK_BYTE_COUNT(8);
				val = ((guint64)tvb_get_letohl(tvb, offset)) << 32
				    | tvb_get_letohl(tvb, offset+4);
				lock_length = val;
				proto_tree_add_uint64(ltree_2, hf_smb_lock_long_length, tvb, offset, 8, val);
				COUNT_BYTES(8);

				/* remember the unlock for the reply */
				if (ld) {
					smb_lock_info_t *li;
					li = (smb_lock_info_t *)wmem_alloc(wmem_file_scope(), sizeof(smb_lock_info_t));
					li->next = ld->unlocks;
					ld->unlocks = li;
					li->pid = lock_pid;
					li->offset = lock_offset;
					li->length = lock_length;
				}
			} else {
				/* normal lock format */
				ltree_2 = proto_tree_add_subtree(tr, tvb, offset, 10, ett_smb_unlock, NULL, "Unlock");

				/* PID */
				CHECK_BYTE_COUNT(2);
				proto_tree_add_item(ltree_2, hf_smb_pid, tvb, offset, 2, ENC_LITTLE_ENDIAN);
				COUNT_BYTES(2);

				/* offset */
				CHECK_BYTE_COUNT(4);
				proto_tree_add_item(ltree_2, hf_smb_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
				COUNT_BYTES(4);

				/* lock count */
				CHECK_BYTE_COUNT(4);
				proto_tree_add_item(ltree_2, hf_smb_count, tvb, offset, 4, ENC_LITTLE_ENDIAN);
				COUNT_BYTES(4);
			}
		}
		proto_item_set_len(it, offset-old_offset);
		it = NULL;
	}

	/* locks */
	if (ln) {
		old_offset = offset;

		tr = proto_tree_add_subtree(tree, tvb, offset, -1, ett_smb_locks, &it, "Locks");
		while (ln--) {
			proto_tree *ltree_2;
			if (lt&0x10) {
				guint64 val;
				guint16 lock_pid;
				guint64 lock_offset;
				guint64 lock_length;

				/* large lock format */
				ltree_2 = proto_tree_add_subtree(tr, tvb, offset, 20, ett_smb_lock, NULL, "Lock");

				/* PID */
				CHECK_BYTE_COUNT(2);
				lock_pid = tvb_get_letohs(tvb, offset);
				proto_tree_add_item(ltree_2, hf_smb_pid, tvb, offset, 2, ENC_LITTLE_ENDIAN);
				COUNT_BYTES(2);

				/* 2 reserved bytes */
				CHECK_BYTE_COUNT(2);
				proto_tree_add_item(ltree_2, hf_smb_reserved, tvb, offset, 2, ENC_NA);
				COUNT_BYTES(2);

				/* offset */
				CHECK_BYTE_COUNT(8);
				val = ((guint64)tvb_get_letohl(tvb, offset)) << 32
				    | tvb_get_letohl(tvb, offset+4);
				lock_offset = val;
				proto_tree_add_uint64(ltree_2, hf_smb_lock_long_offset, tvb, offset, 8, val);
				COUNT_BYTES(8);

				/* length */
				CHECK_BYTE_COUNT(8);
				val = ((guint64)tvb_get_letohl(tvb, offset)) << 32
				    | tvb_get_letohl(tvb, offset+4);
				lock_length = val;
				proto_tree_add_uint64(ltree_2, hf_smb_lock_long_length, tvb, offset, 8, val);
				COUNT_BYTES(8);

				/* remember the lock for the reply */
				if (ld) {
					smb_lock_info_t *li;
					li = (smb_lock_info_t *)wmem_alloc(wmem_file_scope(), sizeof(smb_lock_info_t));
					li->next = ld->locks;
					ld->locks = li;
					li->pid = lock_pid;
					li->offset = lock_offset;
					li->length = lock_length;
				}
			} else {
				/* normal lock format */
				ltree_2 = proto_tree_add_subtree(tr, tvb, offset, 10, ett_smb_lock, NULL, "Lock");

				/* PID */
				CHECK_BYTE_COUNT(2);
				proto_tree_add_item(ltree_2, hf_smb_pid, tvb, offset, 2, ENC_LITTLE_ENDIAN);
				COUNT_BYTES(2);

				/* offset */
				CHECK_BYTE_COUNT(4);
				proto_tree_add_item(ltree_2, hf_smb_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
				COUNT_BYTES(4);

				/* lock count */
				CHECK_BYTE_COUNT(4);
				proto_tree_add_item(ltree_2, hf_smb_count, tvb, offset, 4, ENC_LITTLE_ENDIAN);
				COUNT_BYTES(4);
			}
		}
		proto_item_set_len(it, offset-old_offset);
		it = NULL;
	}

	END_OF_SMB

	if (it != NULL) {
		/*
		 * We ran out of byte count in the middle of dissecting
		 * the locks or the unlocks; set the site of the item
		 * we were dissecting.
		 */
		proto_item_set_len(it, offset-old_offset);
	}

	if (cmd != 0xff) { 	/* there is an andX command */
		if (andxoffset < offset) {
			THROW(ReportedBoundsError);
		}
		dissect_smb_command(tvb, pinfo, andxoffset, smb_tree, cmd, FALSE, si);
	}

	return offset;
}

static int
dissect_locking_andx_response(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree, smb_info_t *si)
{
	guint8      wc, cmd    = 0xff;
	guint16     andxoffset = 0;
	guint16     bc;

	DISSECTOR_ASSERT(si);

	/* print the lock info from the request */
	if ((si->sip != NULL) && (si->sip->extra_info_type == SMB_EI_LOCKDATA)) {
		smb_locking_saved_info_t *ld;
		proto_item *litem = NULL;
		proto_tree *ltree = NULL;

		ld = (smb_locking_saved_info_t *)si->sip->extra_info;
		if (ld != NULL) {
			proto_tree *ltr;
			smb_lock_info_t *li;
			if (tree) {
				litem = proto_tree_add_item(tree, hf_smb_lock_type, tvb, 0, 0, ENC_NA);
				PROTO_ITEM_SET_GENERATED(litem);
				ltree = proto_item_add_subtree(litem, ett_smb_lock_type);

				proto_tree_add_boolean(ltree, hf_smb_lock_type_large, tvb, 0, 0, ld->type);
				proto_tree_add_boolean(ltree, hf_smb_lock_type_cancel, tvb, 0, 0, ld->type);
				proto_tree_add_boolean(ltree, hf_smb_lock_type_change, tvb, 0, 0, ld->type);
				proto_tree_add_boolean(ltree, hf_smb_lock_type_oplock, tvb, 0, 0, ld->type);
				proto_tree_add_boolean(ltree, hf_smb_lock_type_shared, tvb, 0, 0, ld->type);
				proto_tree_add_uint(ltree, hf_smb_locking_ol, tvb, 0, 0, ld->oplock_level);
				proto_tree_add_uint(ltree, hf_smb_number_of_unlocks, tvb, 0, 0, ld->num_unlock);
				proto_tree_add_uint(ltree, hf_smb_number_of_locks, tvb, 0, 0, ld->num_lock);

				ltr = proto_tree_add_subtree(ltree, tvb, 0, 0, ett_smb_lock, NULL, "Locks");
				li = ld->locks;
				while (li) {
					proto_tree_add_uint(ltr, hf_smb_pid, tvb, 0, 0, li->pid);
					proto_tree_add_uint64(ltr, hf_smb_lock_long_offset, tvb, 0, 0, li->offset);
					proto_tree_add_uint64(ltr, hf_smb_lock_long_length, tvb, 0, 0, li->length);
					li = li->next;
				}
				ltr = proto_tree_add_subtree(ltree, tvb, 0, 0, ett_smb_unlock, NULL, "Unlocks");
				li = ld->unlocks;
				while (li) {
					proto_tree_add_uint(ltr, hf_smb_pid, tvb, 0, 0, li->pid);
					proto_tree_add_uint64(ltr, hf_smb_lock_long_offset, tvb, 0, 0, li->offset);
					proto_tree_add_uint64(ltr, hf_smb_lock_long_length, tvb, 0, 0, li->length);
					li = li->next;
				}
			}
		}
	}

	WORD_COUNT;

	/* next smb command */
	cmd = tvb_get_guint8(tvb, offset);
	if (cmd != 0xff) {
		proto_tree_add_uint(tree, hf_smb_andxcmd, tvb, offset, 1, cmd);
	} else {
		proto_tree_add_uint_format_value(tree, hf_smb_andxcmd, tvb, offset, 1, cmd, "No further commands (0xff)");
	}
	offset += 1;

	/* reserved byte */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 1, ENC_NA);
	offset += 1;

	/* andxoffset */
	andxoffset = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_andxoffset, tvb, offset, 2, andxoffset);
	offset += 2;

	BYTE_COUNT;

	END_OF_SMB

	if (cmd != 0xff) { 	/* there is an andX command */
		if (andxoffset < offset) {
			THROW(ReportedBoundsError);
		}
		dissect_smb_command(tvb, pinfo, andxoffset, smb_tree, cmd, FALSE, si);
	}

	return offset;
}


const value_string oa_open_vals[] = {
	{ 0,		"No action taken?"},
	{ 1,		"The file existed and was opened"},
	{ 2,		"The file did not exist but was created"},
	{ 3,		"The file existed and was truncated"},
	{ 0x8001,       "The file existed and was opened, and an OpLock was granted"},
	{ 0x8002,       "The file did not exist but was created, and an OpLock was granted"},
	{ 0x8003,       "The file existed and was truncated, and an OpLock was granted"},
	{0,	NULL}
};
static const true_false_string tfs_oa_lock = {
	"File is currently opened only by this user",
	"File is opened by another user (or mode not supported by server)"
};
static int
dissect_open_action(tvbuff_t *tvb, proto_tree *parent_tree, int offset)
{
	static const int * flags[] = {
		&hf_smb_open_action_lock,
		&hf_smb_open_action_open,
		NULL
	};

	proto_tree_add_bitmask(parent_tree, tvb, offset, hf_smb_open_action, ett_smb_open_action, flags, ENC_LITTLE_ENDIAN);
	offset += 2;

	return offset;
}

static const true_false_string tfs_open_flags_add_info = {
	"Additional information requested",
	"Additional information not requested"
};
static const true_false_string tfs_open_flags_ex_oplock = {
	"Exclusive oplock requested",
	"Exclusive oplock not requested"
};
static const true_false_string tfs_open_flags_batch_oplock = {
	"Batch oplock requested",
	"Batch oplock not requested"
};
static const true_false_string tfs_open_flags_ealen = {
	"Total length of EAs requested",
	"Total length of EAs not requested"
};
static int
dissect_open_flags(tvbuff_t *tvb, proto_tree *parent_tree, int offset, int bm)
{
	guint16     mask;
	proto_item *item;
	proto_tree *tree;

	mask = tvb_get_letohs(tvb, offset);

	if (parent_tree) {
		item = proto_tree_add_item(parent_tree, hf_smb_open_flags, tvb, offset, 2, ENC_LITTLE_ENDIAN);
		tree = proto_item_add_subtree(item, ett_smb_open_flags);

		if (bm&0x0001) {
			proto_tree_add_boolean(tree, hf_smb_open_flags_add_info,
				tvb, offset, 2, mask);
		}
		if (bm&0x0002) {
			proto_tree_add_boolean(tree, hf_smb_open_flags_ex_oplock,
				tvb, offset, 2, mask);
		}
		if (bm&0x0004) {
			proto_tree_add_boolean(tree, hf_smb_open_flags_batch_oplock,
				tvb, offset, 2, mask);
		}
		if (bm&0x0008) {
			proto_tree_add_boolean(tree, hf_smb_open_flags_ealen,
				tvb, offset, 2, mask);
		}
	}

	offset += 2;

	return offset;
}

/* [MS-CIFS].pdf 2.2.4.64.2 provides the last two file types, however
   [MS-SMB].PDF 2.2.4.9.2 elides value 4, Character mode device.  */
static const value_string filetype_vals[] = {
	{ 0,		"Disk file or directory"},
	{ 1,		"Named pipe in byte mode"},
	{ 2,		"Named pipe in message mode"},
	{ 3,		"Spooled printer"},
	{ 4,		"Character mode device"},
	{ 0xFFFF,	"Unknown file type"},
	{0, NULL}
};
static int
dissect_open_andx_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree, smb_info_t *si)
{
	guint8      wc, cmd    = 0xff;
	guint16     andxoffset = 0, bc;
	guint32     to;
	int         fn_len;
	const char *fn;

	DISSECTOR_ASSERT(si);

	WORD_COUNT;

	/* next smb command */
	cmd = tvb_get_guint8(tvb, offset);
	if (cmd != 0xff) {
		proto_tree_add_uint(tree, hf_smb_andxcmd, tvb, offset, 1, cmd);
	} else {
		proto_tree_add_uint_format_value(tree, hf_smb_andxcmd, tvb, offset, 1, cmd, "No further commands (0xff)");
	}
	offset += 1;

	/* reserved byte */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 1, ENC_NA);
	offset += 1;

	/* andxoffset */
	andxoffset = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_andxoffset, tvb, offset, 2, andxoffset);
	offset += 2;

	/* open flags */
	offset = dissect_open_flags(tvb, tree, offset, 0x0007);

	/* desired access */
	offset = dissect_access(tvb, tree, offset, hf_smb_desired_access);

	/* Search Attributes */
	offset = dissect_search_attributes(tvb, tree, offset);

	/* File Attributes */
	offset = dissect_file_attributes(tvb, tree, offset);

	/* creation time */
	offset = dissect_smb_UTIME(tvb, tree, offset, hf_smb_create_time);

	/* open function */
	offset = dissect_open_function(tvb, tree, offset);

	/* allocation size */
	proto_tree_add_item(tree, hf_smb_alloc_size, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	offset += 4;

	/* timeout, described at http://us1.samba.org/samba/ftp/SMB-info/DOSEXTP.TXT */
	to = tvb_get_letohl(tvb, offset);
	proto_tree_add_uint_format_value(tree, hf_smb_timeout, tvb, offset, 4, to, "%s", smbext20_timeout_msecs_to_str(to));
	offset += 4;

	/* 4 reserved bytes */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 4, ENC_NA);
	offset += 4;

	BYTE_COUNT;

	/* file name */
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len,
		FALSE, FALSE, &bc);
	if (fn == NULL)
		goto endofcommand;
	proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
		fn);
	COUNT_BYTES(fn_len);

	/* Copied this portion of code from create_andx_request
	   to guarantee that fsi and si->sip are always correctly filled out */
	if ((!pinfo->fd->flags.visited) && si->sip && fn) {
		smb_fid_saved_info_t *fsi;

		fsi = wmem_new0(wmem_file_scope(), smb_fid_saved_info_t);
		fsi->filename = wmem_strdup(wmem_file_scope(), fn);

		si->sip->extra_info_type = SMB_EI_FILEDATA;
		si->sip->extra_info = fsi;
	}

	col_append_fstr(pinfo->cinfo, COL_INFO, ", Path: %s",
		    format_text(fn, strlen(fn)));

	END_OF_SMB

	if (cmd != 0xff) { 	/* there is an andX command */
		if (andxoffset < offset) {
			THROW(ReportedBoundsError);
		}
		dissect_smb_command(tvb, pinfo, andxoffset, smb_tree, cmd, FALSE, si);
	}

	return offset;
}

static const true_false_string tfs_ipc_state_nonblocking = {
	"Reads/writes return immediately if no data available",
	"Reads/writes block if no data available"
};
static const value_string ipc_state_endpoint_vals[] = {
	{ 0,		"Consumer end of pipe"},
	{ 1,		"Server end of pipe"},
	{0,	NULL}
};
static const value_string ipc_state_pipe_type_vals[] = {
	{ 0,		"Byte stream pipe"},
	{ 1,		"Message pipe"},
	{0,	NULL}
};
static const value_string ipc_state_read_mode_vals[] = {
	{ 0,		"Read pipe as a byte stream"},
	{ 1,		"Read messages from pipe"},
	{0,	NULL}
};

int
dissect_ipc_state(tvbuff_t *tvb, proto_tree *parent_tree, int offset, gboolean setstate_flag)
{
	static const int * setstate_flags[] = {
		&hf_smb_ipc_state_nonblocking,
		&hf_smb_ipc_state_read_mode,
		NULL
	};
	static const int * not_setstate_flags[] = {
		&hf_smb_ipc_state_nonblocking,
		&hf_smb_ipc_state_endpoint,
		&hf_smb_ipc_state_pipe_type,
		&hf_smb_ipc_state_read_mode,
		&hf_smb_ipc_state_icount,
		NULL
	};

	if (!setstate_flag) {
		proto_tree_add_bitmask(parent_tree, tvb, offset, hf_smb_ipc_state, ett_smb_ipc_state, not_setstate_flags, ENC_LITTLE_ENDIAN);
	} else {
		proto_tree_add_bitmask(parent_tree, tvb, offset, hf_smb_ipc_state, ett_smb_ipc_state, setstate_flags, ENC_LITTLE_ENDIAN);
	}
	offset += 2;

	return offset;
}

static int
dissect_open_andx_response(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree, smb_info_t *si)
{
	guint8          wc, cmd    = 0xff;
	guint16         andxoffset = 0, bc;
	guint16         fid;
	guint16         ftype;
	guint16         fattr;
	smb_fid_info_t *fid_info   = NULL;
	gboolean        isdir      = FALSE;

	WORD_COUNT;

	/* next smb command */
	cmd = tvb_get_guint8(tvb, offset);
	if (cmd != 0xff) {
		proto_tree_add_uint(tree, hf_smb_andxcmd, tvb, offset, 1, cmd);
	} else {
		proto_tree_add_uint_format_value(tree, hf_smb_andxcmd, tvb, offset, 1, cmd, "No further commands (0xff)");
	}
	offset += 1;

	/* reserved byte */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 1, ENC_NA);
	offset += 1;

	/* andxoffset */
	andxoffset = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_andxoffset, tvb, offset, 2, andxoffset);
	offset += 2;

	/* fid */
	fid = tvb_get_letohs(tvb, offset);
	/* we add fid_info= to this call so that we save the result */
	fid_info = dissect_smb_fid(tvb, pinfo, tree, offset, 2, fid, TRUE, FALSE, FALSE, si);

	offset += 2;

	/* File Attributes */
	fattr = tvb_get_letohs(tvb, offset);
	isdir = fattr & SMB_FILE_ATTRIBUTE_DIRECTORY;
	offset = dissect_file_attributes(tvb, tree, offset);

	/* last write time */
	offset = dissect_smb_UTIME(tvb, tree, offset, hf_smb_last_write_time);

	/* File Size */
	/* We store the file_size in the fid_info */
	if (fid_info) {
		fid_info->end_of_file = (guint64) tvb_get_letohl(tvb, offset);
	}
	proto_tree_add_item(tree, hf_smb_file_size, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	offset += 4;

	/* granted access */
	offset = dissect_access(tvb, tree, offset, hf_smb_granted_access);

	/* File Type */
	ftype = tvb_get_letohs(tvb, offset);
	proto_tree_add_item(tree, hf_smb_file_type, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;
	/* Copied from dissect_nt_create_andx_response
	   Try to remember the type of this fid so that we can dissect
	   any future security descriptor (access mask) properly
	*/
	if (fid_info) {
		fid_info->type = SMB_FID_TYPE_UNKNOWN;
	}
	if (ftype == 0) {
		if (isdir == 0) {
			if (fid_info) {
				fid_info->type = SMB_FID_TYPE_FILE;
			}
		} else {
			if (fid_info) {
				fid_info->type = SMB_FID_TYPE_DIR;
			}
		}
	}
	if ((ftype == 2) || (ftype == 1)) {
		if (fid_info) {
			fid_info->type = SMB_FID_TYPE_PIPE;
		}
	}

	/* IPC State */
	offset = dissect_ipc_state(tvb, tree, offset, FALSE);

	/* open_action */
	offset = dissect_open_action(tvb, tree, offset);

	/* server fid */
	proto_tree_add_item(tree, hf_smb_server_fid, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	offset += 4;

	/* 2 reserved bytes */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 2, ENC_NA);
	offset += 2;

	/* [MS-SMB] 2.2.4.1.2 Server Response Extensions */
	if (wc == 19) {
		proto_tree *tr = NULL;

		tr = proto_tree_add_subtree(tree, tvb, offset, 4,
			ett_smb_nt_access_mask, NULL, "Maximal Access Rights");
		offset = dissect_smb_access_mask(tvb, tr, offset);

		tr = proto_tree_add_subtree(tree, tvb, offset, 4,
			ett_smb_nt_access_mask, NULL, "Guest Maximal Access Rights");
		offset = dissect_smb_access_mask(tvb, tr, offset);
	}

	BYTE_COUNT;

	END_OF_SMB

	if (cmd != 0xff) { 	/* there is an andX command */
		if (andxoffset < offset) {
			THROW(ReportedBoundsError);
		}
		dissect_smb_command(tvb, pinfo, andxoffset, smb_tree, cmd, FALSE, si);
	}

	return offset;
}

static int
dissect_read_andx_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree, smb_info_t *si)
{
	guint8        wc, cmd               = 0xff;
	guint16       andxoffset            = 0, bc, maxcnt_low;
	guint32       maxcnt_high;
	guint32       maxcnt                = 0;
	guint32       offsetlow, offsethigh = 0;
	guint64       ofs;
	unsigned int  fid;
	rw_info_t    *rwi                   = NULL;


	DISSECTOR_ASSERT(si);

	WORD_COUNT;

	/* next smb command */
	cmd = tvb_get_guint8(tvb, offset);
	if (cmd != 0xff) {
		proto_tree_add_uint(tree, hf_smb_andxcmd, tvb, offset, 1, cmd);
	} else {
		proto_tree_add_uint_format_value(tree, hf_smb_andxcmd, tvb, offset, 1, cmd, "No further commands (0xff)");
	}
	offset += 1;

	/* reserved byte */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 1, ENC_NA);
	offset += 1;

	/* andxoffset */
	andxoffset = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_andxoffset, tvb, offset, 2, andxoffset);
	offset += 2;

	/* fid */
	fid = tvb_get_letohs(tvb, offset);
	dissect_smb_fid(tvb, pinfo, tree, offset, 2, (guint16) fid, FALSE, FALSE, FALSE, si);
	offset += 2;

	/* offset */
	offsetlow = tvb_get_letohl(tvb, offset);
	proto_tree_add_item(tree, hf_smb_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	offset += 4;

	/* max count low */
	maxcnt_low = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_max_count_low, tvb, offset, 2, maxcnt_low);
	offset += 2;

	/* min count */
	proto_tree_add_item(tree, hf_smb_min_count, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	/*
	 * max count high
	 *
	 * XXX - we should really only do this in case we have seen
	 * LARGE FILE being negotiated.  Unfortunately, we might not
	 * have seen the negotiation phase in the capture....
	 *
	 * XXX - this is shown as a ULONG in the SNIA SMB spec, i.e.
	 * it's 32 bits, but the description says "High 16 bits of
	 * MaxCount if CAP_LARGE_READX".
	 *
	 * The SMB File Sharing Protocol Extensions Version 2.0,
	 * Document Version 3.3 spec doesn't speak of an extra 16
	 * bits in max count, but it does show a 32-bit timeout
	 * after the min count field.
	 *
	 * The Microsoft [MS-SMB] spec shows it as a ULONG named
	 * Timeout_or_MaxCountHigh, which is
	 *
	 *	...extended to be treated as a union of a 32-bit
	 *	Timeout field and a 16-bit MaxCountHigh field.
	 *	When reading from a regular file, the field
	 *	MUST be interpreted as MaxCountHigh and the
	 *	two unused bytes MUST be zero.  When reading from
	 *	a name[sic] pipe or I/O device, the field MUST
	 *	be interpreted as Timeout.
	 *
	 * Timeout is a timeout in milliseconds, with 0xffffffff
	 * and 0xfffffffe having special meaning.
	 *
	 * MaxCountHigh is 16 bits of the MaxCountHigh value
	 * followed by 16 bits of Reserved.
	 *
	 * We fetch and display it as 32 bits for now.
	 *
	 * XXX if maxcount high is 0xFFFFFFFF we assume it is just padding
	 * bytes and we just ignore it.
	 */
	maxcnt_high = tvb_get_letohl(tvb, offset);
	if (maxcnt_high == 0xffffffff) {
		maxcnt_high = 0;
	} else {
		proto_tree_add_uint(tree, hf_smb_max_count_high, tvb, offset, 4, maxcnt_high);
	}

	offset += 4;

	maxcnt = maxcnt_high;
	maxcnt = (maxcnt<<16) | maxcnt_low;

	/* remaining */
	proto_tree_add_item(tree, hf_smb_remaining, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	if (wc == 12) {
		/* high offset */
		offsethigh = tvb_get_letohl(tvb, offset);
		proto_tree_add_item(tree, hf_smb_high_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		offset += 4;
	}

	ofs = offsethigh;
	ofs = (ofs<<32) | offsetlow;

	col_append_fstr(pinfo->cinfo, COL_INFO,
				", %u byte%s at offset %" G_GINT64_MODIFIER "u",
				maxcnt, (maxcnt == 1) ? "" : "s", ofs);

	/* save the offset/len for this transaction */
	if (si->sip && !pinfo->fd->flags.visited) {
		rwi = (rw_info_t *)wmem_alloc(wmem_file_scope(), sizeof(rw_info_t));
		rwi->offset = ofs;
		rwi->len = maxcnt;
		rwi->fid = fid;

		si->sip->extra_info_type = SMB_EI_RWINFO;
		si->sip->extra_info = rwi;
	}
	if (si->sip && (si->sip->extra_info_type == SMB_EI_RWINFO)) {
		rwi = (rw_info_t *)si->sip->extra_info;
	}
	if (rwi) {
		proto_item *it;

		it = proto_tree_add_uint64(tree, hf_smb_file_rw_offset, tvb, 0, 0, rwi->offset);

		PROTO_ITEM_SET_GENERATED(it);
		it = proto_tree_add_uint(tree, hf_smb_file_rw_length, tvb, 0, 0, rwi->len);
		PROTO_ITEM_SET_GENERATED(it);
	}

	BYTE_COUNT;

	END_OF_SMB

	if (cmd != 0xff) { 	/* there is an andX command */
		if (andxoffset < offset) {
			THROW(ReportedBoundsError);
		}
		dissect_smb_command(tvb, pinfo, andxoffset, smb_tree, cmd, FALSE, si);
	}

	return offset;
}

static int
dissect_read_andx_response(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree, smb_info_t *si)
{
	guint8      wc, cmd    = 0xff;
	guint16     andxoffset = 0, bc, datalen_low, dataoffset = 0;
	guint32     datalen    = 0, datalen_high;
	rw_info_t  *rwi        = NULL;
	guint16 fid            = 0; /* was int fid = 0; */

	guint32 tvblen;

	DISSECTOR_ASSERT(si);

	WORD_COUNT;

	/* next smb command */
	cmd = tvb_get_guint8(tvb, offset);
	if (cmd != 0xff) {
		proto_tree_add_uint(tree, hf_smb_andxcmd, tvb, offset, 1, cmd);
	} else {
		proto_tree_add_uint_format_value(tree, hf_smb_andxcmd, tvb, offset, 1, cmd, "No further commands (0xff)");
	}
	offset += 1;

	/* reserved byte */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 1, ENC_NA);
	offset += 1;

	/* andxoffset */
	andxoffset = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_andxoffset, tvb, offset, 2, andxoffset);
	offset += 2;

	/* If we have seen the request, then print which FID this refers to */
	/* first check if we have seen the request */
	if ((si->sip != NULL) && (si->sip->frame_req > 0) && (si->sip->extra_info_type == SMB_EI_FID)) {
		fid = GPOINTER_TO_INT(si->sip->extra_info);
		dissect_smb_fid(tvb, pinfo, tree, 0, 0, (guint16) fid, FALSE, FALSE, FALSE, si);
	}

	if (si->sip && (si->sip->extra_info_type == SMB_EI_RWINFO)) {
		rwi = (rw_info_t *)si->sip->extra_info;
	}
	if (rwi) {
		proto_item *it;

		it = proto_tree_add_uint64(tree, hf_smb_file_rw_offset, tvb, 0, 0, rwi->offset);

		PROTO_ITEM_SET_GENERATED(it);
		it = proto_tree_add_uint(tree, hf_smb_file_rw_length, tvb, 0, 0, rwi->len);
		PROTO_ITEM_SET_GENERATED(it);

		/* we need the fid for the call to dcerpc below */
		fid = rwi->fid;
	}

	/* remaining */
	proto_tree_add_item(tree, hf_smb_remaining, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	/* data compaction mode */
	proto_tree_add_item(tree, hf_smb_dcm, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	/* 2 reserved bytes */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 2, ENC_NA);
	offset += 2;

	/* data len low */
	datalen_low = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_data_len_low, tvb, offset, 2, datalen_low);
	offset += 2;

	/* data offset */
	dataoffset = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_data_offset, tvb, offset, 2, dataoffset);
	offset += 2;

	/*
	 * XXX - the SNIA SMB spec says this is a USHORT, not a
	 * ULONG.
	 *
	 * XXX - we should really only do this in case we have seen
	 * LARGE FILE being negotiated.  Unfortunately, we might not
	 * have seen the negotiation phase in the capture....
	 */
	/* data length high */
	datalen_high = tvb_get_letohl(tvb, offset);
	if (datalen_high == 0xffffffff) {
		datalen_high = 0;
	} else {
		proto_tree_add_uint(tree, hf_smb_data_len_high, tvb, offset, 4, datalen_high);
	}
	offset += 4;

	datalen = datalen_high;
	datalen = (datalen<<16) | datalen_low;


	col_append_fstr(pinfo->cinfo, COL_INFO,
				", %u byte%s", datalen,
				(datalen == 1) ? "" : "s");


	/* 6 reserved bytes */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 6, ENC_NA);
	offset += 6;

	BYTE_COUNT;

	/* file data, might be DCERPC on a pipe */
	if (bc) {
		offset = dissect_file_data_maybe_dcerpc(tvb, pinfo, tree,
		    top_tree_global, offset, bc, (guint16) datalen, 0, (guint16) fid, si);
		bc = 0;
	}

	/* feed the export object tap listener */
	tvblen = tvb_reported_length_remaining(tvb, dataoffset);
	if (have_tap_listener(smb_eo_tap) && (datalen == tvblen) && rwi) {
		feed_eo_smb(SMB_COM_READ_ANDX, fid, tvb, pinfo, dataoffset, datalen, rwi->len, rwi->offset, si);
	}

	END_OF_SMB

	if (cmd != 0xff) {   /* there is an andX command */
		if (andxoffset < offset) {
			THROW(ReportedBoundsError);
		}
		dissect_smb_command(tvb, pinfo, andxoffset, smb_tree, cmd, FALSE, si);
	}

	return offset;
}

static int
dissect_write_andx_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree, smb_info_t *si)
{
	guint8          wc, cmd               = 0xff;
	guint16         andxoffset            = 0, bc, dataoffset = 0, datalen_low, datalen_high;
	guint32         offsetlow, offsethigh = 0;
	guint64         ofs;
	guint32         datalen               = 0;
	guint16 fid                           = 0; /* was unsigned int fid = 0; */
	guint16         mode                  = 0;
	rw_info_t      *rwi                   = NULL;

	guint32		tvblen;

	DISSECTOR_ASSERT(si);

	WORD_COUNT;

	/* next smb command */
	cmd = tvb_get_guint8(tvb, offset);
	if (cmd != 0xff) {
		proto_tree_add_uint(tree, hf_smb_andxcmd, tvb, offset, 1, cmd);
	} else {
		proto_tree_add_uint_format_value(tree, hf_smb_andxcmd, tvb, offset, 1, cmd, "No further commands (0xff)");
	}
	offset += 1;

	/* reserved byte */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 1, ENC_NA);
	offset += 1;

	/* andxoffset */
	andxoffset = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_andxoffset, tvb, offset, 2, andxoffset);
	offset += 2;

	/* fid */
	fid = tvb_get_letohs(tvb, offset);
	dissect_smb_fid(tvb, pinfo, tree, offset, 2, (guint16) fid, FALSE, FALSE, FALSE, si);
	offset += 2;

	/* offset */
	offsetlow = tvb_get_letohl(tvb, offset);
	proto_tree_add_item(tree, hf_smb_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	offset += 4;

	/* reserved */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 4, ENC_NA);
	offset += 4;

	/* mode */
	mode = tvb_get_letohs(tvb, offset);
	offset = dissect_write_mode(tvb, tree, offset, 0x000f);

	/* remaining */
	proto_tree_add_item(tree, hf_smb_remaining, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	/*
	 * XXX - we should really only do this in case we have seen
	 * LARGE FILE being negotiated.  Unfortunately, we might not
	 * have seen the negotiation phase in the capture....
	 */
	/* data length high */
	datalen_high = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_data_len_high, tvb, offset, 2, datalen_high);
	offset += 2;

	/* data len low */
	datalen_low = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_data_len_low, tvb, offset, 2, datalen_low);
	offset += 2;

	datalen = datalen_high;
	datalen = (datalen<<16) | datalen_low;

	/* data offset */
	dataoffset = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_data_offset, tvb, offset, 2, dataoffset);
	offset += 2;

	if (wc == 14) {
		/* high offset */
		offsethigh = tvb_get_letohl(tvb, offset);
		proto_tree_add_item(tree, hf_smb_high_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		offset += 4;
	}

	ofs = offsethigh;
	ofs = (ofs<<32) | offsetlow;

	col_append_fstr(pinfo->cinfo, COL_INFO,
				", %u byte%s at offset %" G_GINT64_MODIFIER "u",
				datalen, (datalen == 1) ? "" : "s", ofs);

	/* save the offset/len for this transaction */
	if (si->sip && !pinfo->fd->flags.visited) {
		rwi	    = (rw_info_t *)wmem_alloc(wmem_file_scope(), sizeof(rw_info_t));
		rwi->offset = ofs;
		rwi->len    = datalen;
		rwi->fid    = fid;

		si->sip->extra_info_type = SMB_EI_RWINFO;
		si->sip->extra_info = rwi;
	}
	if (si->sip && (si->sip->extra_info_type == SMB_EI_RWINFO)) {
		rwi = (rw_info_t *)si->sip->extra_info;
	}
	if (rwi) {
		proto_item *it;

		it = proto_tree_add_uint64(tree, hf_smb_file_rw_offset, tvb, 0, 0, rwi->offset);

		PROTO_ITEM_SET_GENERATED(it);
		it = proto_tree_add_uint(tree, hf_smb_file_rw_length, tvb, 0, 0, rwi->len);
		PROTO_ITEM_SET_GENERATED(it);
	}


	BYTE_COUNT;

	/* if both the MessageStart and the  WriteRawNamedPipe flags are set
	   the first two bytes of the payload is the length of the data.
	   Assume that all WriteAndX PDUs that have MESSAGE_START set to
	   be over the IPC$ share and thus they all transport DCERPC.
	   (if we didn't already know that from the TreeConnect call)
	*/
	if (mode&WRITE_MODE_MESSAGE_START) {
		if (mode&WRITE_MODE_RAW) {
			proto_tree_add_item(tree, hf_smb_pipe_write_len, tvb, offset, 2, ENC_LITTLE_ENDIAN);
			offset += 2;
			dataoffset += 2;
			bc -= 2;
			datalen -= 2;
		}
		if (!pinfo->fd->flags.visited) {
			/* In case we did not see the TreeConnect call,
			   store this TID here as well as a IPC TID
			   so we know that future Read/Writes to this
			   TID is (probably) DCERPC.
			*/
			if (g_hash_table_lookup(si->ct->tid_service, GUINT_TO_POINTER(si->tid))) {
				g_hash_table_remove(si->ct->tid_service, GUINT_TO_POINTER(si->tid));
			}
			g_hash_table_insert(si->ct->tid_service, GUINT_TO_POINTER(si->tid), (void *)TID_IPC);
		}
		if (si->sip) {
			si->sip->flags |= SMB_SIF_TID_IS_IPC;
		}
	}

	/* file data, might be DCERPC on a pipe */
	if (bc != 0) {
		offset = dissect_file_data_maybe_dcerpc(tvb, pinfo, tree,
		    top_tree_global, offset, bc, (guint16) datalen, 0, (guint16) fid, si);
		bc = 0;
	}

	/* feed the export object tap listener */
	tvblen = tvb_reported_length_remaining(tvb, dataoffset);
	if (have_tap_listener(smb_eo_tap) && (datalen == tvblen) && rwi) {
		feed_eo_smb(SMB_COM_WRITE_ANDX, fid, tvb, pinfo, dataoffset, datalen, rwi->len, rwi->offset, si);
	}

	END_OF_SMB

	if (cmd != 0xff) { 	/* there is an andX command */
		if (andxoffset < offset) {
			THROW(ReportedBoundsError);
		}
		dissect_smb_command(tvb, pinfo, andxoffset, smb_tree, cmd, FALSE, si);
	}

	return offset;
}

static int
dissect_write_andx_response(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree, smb_info_t *si)
{
	guint8      wc, cmd    = 0xff;
	guint16     andxoffset = 0, bc, count_low, count_high;
	guint32     count      = 0;
	rw_info_t  *rwi        = NULL;

	DISSECTOR_ASSERT(si);

	WORD_COUNT;

	/* next smb command */
	cmd = tvb_get_guint8(tvb, offset);
	if (cmd != 0xff) {
		proto_tree_add_uint(tree, hf_smb_andxcmd, tvb, offset, 1, cmd);
	} else {
		proto_tree_add_uint_format_value(tree, hf_smb_andxcmd, tvb, offset, 1, cmd, "No further commands (0xff)");
	}
	offset += 1;

	/* reserved byte */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 1, ENC_NA);
	offset += 1;

	/* andxoffset */
	andxoffset = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_andxoffset, tvb, offset, 2, andxoffset);
	offset += 2;


	if (si->sip && (si->sip->extra_info_type == SMB_EI_RWINFO)) {
		rwi = (rw_info_t *)si->sip->extra_info;
	}
	if (rwi) {
		proto_item *it;

		it = proto_tree_add_uint64(tree, hf_smb_file_rw_offset, tvb, 0, 0, rwi->offset);

		PROTO_ITEM_SET_GENERATED(it);
		it = proto_tree_add_uint(tree, hf_smb_file_rw_length, tvb, 0, 0, rwi->len);
		PROTO_ITEM_SET_GENERATED(it);
	}


	/* write count low */
	count_low = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_count_low, tvb, offset, 2, count_low);
	offset += 2;

	/* remaining */
	proto_tree_add_item(tree, hf_smb_remaining, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	/* XXX we should really only do this in case we have seen LARGE FILE being negotiated */
	/* write count high */
	count_high = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_count_high, tvb, offset, 2, count_high);
	offset += 2;

	count = count_high;
	count = (count<<16) | count_low;

	col_append_fstr(pinfo->cinfo, COL_INFO,
				", %u byte%s", count,
				(count == 1) ? "" : "s");

	/* 2 reserved bytes */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 2, ENC_NA);
	offset += 2;

	BYTE_COUNT;

	END_OF_SMB

	if (cmd != 0xff) { 	/* there is an andX command */
		if (andxoffset < offset) {
			THROW(ReportedBoundsError);
		}
		dissect_smb_command(tvb, pinfo, andxoffset, smb_tree, cmd, FALSE, si);
	}

	return offset;
}


static const true_false_string tfs_setup_action_guest = {
	"Logged in as GUEST",
	"Not logged in as GUEST"
};
static int
dissect_setup_action(tvbuff_t *tvb, proto_tree *parent_tree, int offset)
{
	static const int * flags[] = {
		&hf_smb_setup_action_guest,
		NULL
	};

	proto_tree_add_bitmask(parent_tree, tvb, offset, hf_smb_setup_action, ett_smb_setup_action, flags, ENC_LITTLE_ENDIAN);
	offset += 2;

	return offset;
}


static int
dissect_session_setup_andx_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree, smb_info_t *si)
{
	guint8      wc, cmd        = 0xff;
	guint16     bc;
	guint16     andxoffset     = 0;
	int         an_len;
	const char *an;
	int         dn_len;
	const char *dn;
	guint16     pwlen          = 0;
	guint16     sbloblen       = 0, sbloblen_short;
	guint16     apwlen         = 0, upwlen = 0;
	gboolean    unicodeflag;
	static int  ntlmssp_tap_id = 0;
	const ntlmssp_header_t *ntlmssph;

	if (!ntlmssp_tap_id) {
		GString *error_string;
		/* We don't specify any callbacks at all.
		 * Instead we manually fetch the tapped data after the
		 * security blob has been fully dissected and before
		 * we exit from this dissector.
		 */
		error_string = register_tap_listener("ntlmssp", NULL, NULL,
		    TL_IS_DISSECTOR_HELPER, NULL, NULL, NULL);
		if (!error_string) {
			ntlmssp_tap_id = find_tap_id("ntlmssp");
		} else {
			g_string_free(error_string, TRUE);
		}
	}

	DISSECTOR_ASSERT(si);

	WORD_COUNT;

	/* next smb command */
	cmd = tvb_get_guint8(tvb, offset);
	if (cmd != 0xff) {
		proto_tree_add_uint(tree, hf_smb_andxcmd, tvb, offset, 1, cmd);
	} else {
		proto_tree_add_uint_format_value(tree, hf_smb_andxcmd, tvb, offset, 1, cmd, "No further commands (0xff)");
	}
	offset += 1;

	/* reserved byte */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 1, ENC_NA);
	offset += 1;

	/* andxoffset */
	andxoffset = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_andxoffset, tvb, offset, 2, andxoffset);
	offset += 2;

	/* Maximum Buffer Size */
	proto_tree_add_item(tree, hf_smb_max_buf_size, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	/* Maximum Multiplex Count */
	proto_tree_add_item(tree, hf_smb_max_mpx_count, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	/* VC Number */
	proto_tree_add_item(tree, hf_smb_vc_num, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	/* session key */
	proto_tree_add_item(tree, hf_smb_session_key, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	offset += 4;

	switch (wc) {
	case 10:
		/* password length, ASCII*/
		pwlen = tvb_get_letohs(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_password_len,
			tvb, offset, 2, pwlen);
		offset += 2;

		/* 4 reserved bytes */
		proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 4, ENC_NA);
		offset += 4;

		break;

	case 12:
		/* security blob length */
		sbloblen = tvb_get_letohs(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_security_blob_len, tvb, offset, 2, sbloblen);
		offset += 2;

		/* 4 reserved bytes */
		proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 4, ENC_NA);
		offset += 4;

		/* capabilities */
		dissect_negprot_capabilities(tvb, tree, offset);
		offset += 4;

		break;

	case 13:
		/* password length, ANSI*/
		apwlen = tvb_get_letohs(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_ansi_password_len,
			tvb, offset, 2, apwlen);
		offset += 2;

		/* password length, Unicode*/
		upwlen = tvb_get_letohs(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_unicode_password_len,
			tvb, offset, 2, upwlen);
		offset += 2;

		/* 4 reserved bytes */
		proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 4, ENC_NA);
		offset += 4;

		/* capabilities */
		dissect_negprot_capabilities(tvb, tree, offset);
		offset += 4;

		break;
	}

	BYTE_COUNT;

	if (wc == 12) {
		proto_item *blob_item;

		/* security blob */
		/* If it runs past the end of the captured data, don't
		 * try to put all of it into the protocol tree as the
		 * raw security blob; we might get an exception on
		 * short frames and then we will not see anything at all
		 * of the security blob.
		 */
		sbloblen_short = sbloblen;
		if (sbloblen_short > tvb_reported_length_remaining(tvb, offset)) {
			sbloblen_short = tvb_reported_length_remaining(tvb, offset);
		}
		blob_item = proto_tree_add_item(tree, hf_smb_security_blob,
						tvb, offset, sbloblen_short,
						ENC_NA);

		/* As an optimization, because Windows is perverse,
		   we check to see if NTLMSSP is the first part of the
		   blob, and if so, call the NTLMSSP dissector,
		   otherwise we call the GSS-API dissector. This is because
		   Windows can request RAW NTLMSSP, but will happily handle
		   a client that wraps NTLMSSP in SPNEGO
		*/

		if (sbloblen) {
			tvbuff_t *blob_tvb;
			proto_tree *blob_tree;

			blob_tree = proto_item_add_subtree(blob_item,
							   ett_smb_secblob);
			CHECK_BYTE_COUNT(sbloblen);

			/*
			 * Set the reported length of this to the reported
			 * length of the blob, rather than the amount of
			 * data available from the blob, so that we'll
			 * throw the right exception if it's too short.
			 */
			blob_tvb = tvb_new_subset(tvb, offset, sbloblen_short,
						  sbloblen);

			if (si && si->ct && si->ct->raw_ntlmssp &&
			    (tvb_strneql(tvb, offset, "NTLMSSP", 7) == 0)) {
			  call_dissector(ntlmssp_handle, blob_tvb, pinfo,
					 blob_tree);

			}
			else {
			  call_dissector(gssapi_handle, blob_tvb,
					 pinfo, blob_tree);
			}

			/* If we have found a uid->acct_name mapping, store it */
			if (!pinfo->fd->flags.visited && si->sip) {
				int idx = 0;
				if ((ntlmssph = (const ntlmssp_header_t *)fetch_tapped_data(ntlmssp_tap_id, idx + 1 )) != NULL) {
					if (ntlmssph && (ntlmssph->type == 3)) {
						smb_uid_t *smb_uid;

						smb_uid = (smb_uid_t *)wmem_alloc(wmem_file_scope(), sizeof(smb_uid_t));
						smb_uid->logged_in  = -1;
						smb_uid->logged_out = -1;
						smb_uid->domain  = wmem_strdup(wmem_file_scope(), ntlmssph->domain_name);
						smb_uid->account = wmem_strdup(wmem_file_scope(), ntlmssph->acct_name);

						si->sip->extra_info = smb_uid;
						si->sip->extra_info_type = SMB_EI_UID;
					}
				}
			}

			COUNT_BYTES(sbloblen);
		}

		/* OS
		 * Eventhough this field should honour the unicode flag
		 * some ms clients gets this wrong.
		 * At least XP SP1 sends this in ASCII
		 * even when the unicode flag is on.
		 * Test if the first three bytes are "Win"
		 * and if so just override the flag.
		 */
		unicodeflag = si->unicode;
		if ( tvb_strneql(tvb, offset, "Win", 3) == 0 ) {
			unicodeflag = FALSE;
		}
		an = get_unicode_or_ascii_string(tvb, &offset,
			unicodeflag, &an_len, FALSE, FALSE, &bc);
		if (an == NULL)
			goto endofcommand;
		proto_tree_add_string(tree, hf_smb_os, tvb,
			offset, an_len, an);
		COUNT_BYTES(an_len);

		/* LANMAN */
		/* XXX - pre-W2K NT systems appear to stick an extra 2 bytes of
		 * padding/null string/whatever in front of this. W2K doesn't
		 * appear to. I suspect that's a bug that got fixed; I also
		 * suspect that, in practice, nobody ever looks at that field
		 * because the bug didn't appear to get fixed until NT 5.0....
		 *
		 * Eventhough this field should honour the unicode flag
		 * some ms clients gets this wrong.
		 * At least XP SP1 sends this in ASCII
		 * even when the unicode flag is on.
		 * Test if the first three bytes are "Win"
		 * and if so just override the flag.
		 */
		unicodeflag = si->unicode;
		if ( tvb_strneql(tvb, offset, "Win", 3) == 0 ) {
			unicodeflag = FALSE;
		}
		an = get_unicode_or_ascii_string(tvb, &offset,
			unicodeflag, &an_len, FALSE, FALSE, &bc);
		if (an == NULL)
			goto endofcommand;
		proto_tree_add_string(tree, hf_smb_lanman, tvb,
			offset, an_len, an);
		COUNT_BYTES(an_len);

		/* Primary domain */
		/* XXX - pre-W2K NT systems sometimes appear to stick an extra
		 * byte in front of this, at least if all the strings are
		 * ASCII and the account name is empty. Another bug?
		 */
		dn = get_unicode_or_ascii_string(tvb, &offset,
			si->unicode, &dn_len, FALSE, FALSE, &bc);
		if (dn == NULL)
			goto endofcommand;
		proto_tree_add_string(tree, hf_smb_primary_domain, tvb,
			offset, dn_len, dn);
		COUNT_BYTES(dn_len);
	} else {
		switch (wc) {

		case 10:
			if (pwlen) {
				/* password, ASCII */
				CHECK_BYTE_COUNT(pwlen);
				proto_tree_add_item(tree, hf_smb_password,
					tvb, offset, pwlen, ENC_NA);
				COUNT_BYTES(pwlen);
			}

			break;

		case 13:
			if (apwlen) {
				/* password, ANSI */
				CHECK_BYTE_COUNT(apwlen);
				proto_tree_add_item(tree, hf_smb_ansi_password,
					tvb, offset, apwlen, ENC_NA);
				COUNT_BYTES(apwlen);
			}

			if (upwlen) {
				proto_item *item;

				/* password, Unicode */
				CHECK_BYTE_COUNT(upwlen);
				item = proto_tree_add_item(tree, hf_smb_unicode_password,
					tvb, offset, upwlen, ENC_NA);

				if (upwlen > 24) {
					proto_tree *subtree;
					subtree = proto_item_add_subtree(item, ett_smb_unicode_password);
					dissect_ntlmv2_response(tvb, pinfo, subtree, offset, upwlen);
				}

				COUNT_BYTES(upwlen);
			}

			break;
		}

		/* Account Name */
		an = get_unicode_or_ascii_string(tvb, &offset,
			si->unicode, &an_len, FALSE, FALSE, &bc);
		if (an == NULL)
			goto endofcommand;
		proto_tree_add_string(tree, hf_smb_account, tvb, offset, an_len,
			an);
		COUNT_BYTES(an_len);

		/* Primary domain */
		/* XXX - pre-W2K NT systems sometimes appear to stick an extra
		 * byte in front of this, at least if all the strings are
		 * ASCII and the account name is empty. Another bug?
		 */
		dn = get_unicode_or_ascii_string(tvb, &offset,
			si->unicode, &dn_len, FALSE, FALSE, &bc);
		if (dn == NULL)
			goto endofcommand;
		proto_tree_add_string(tree, hf_smb_primary_domain, tvb,
			offset, dn_len, dn);
		COUNT_BYTES(dn_len);

		col_append_str(pinfo->cinfo, COL_INFO, ", User: ");

		if (!dn[0] && !an[0])
			col_append_str(pinfo->cinfo, COL_INFO, "anonymous");
		else
			col_append_fstr(pinfo->cinfo, COL_INFO,
					"%s\\%s",
					format_text(dn, strlen(dn)),
					format_text(an, strlen(an)));

		/* OS */
		an = get_unicode_or_ascii_string(tvb, &offset,
			si->unicode, &an_len, FALSE, FALSE, &bc);
		if (an == NULL)
			goto endofcommand;
		proto_tree_add_string(tree, hf_smb_os, tvb,
			offset, an_len, an);
		COUNT_BYTES(an_len);

		/* LANMAN */
		/* XXX - pre-W2K NT systems appear to stick an extra 2 bytes of
		 * padding/null string/whatever in front of this. W2K doesn't
		 * appear to. I suspect that's a bug that got fixed; I also
		 * suspect that, in practice, nobody ever looks at that field
		 * because the bug didn't appear to get fixed until NT 5.0....
		 */
		an = get_unicode_or_ascii_string(tvb, &offset,
			si->unicode, &an_len, FALSE, FALSE, &bc);
		if (an == NULL)
			goto endofcommand;
		proto_tree_add_string(tree, hf_smb_lanman, tvb,
			offset, an_len, an);
		COUNT_BYTES(an_len);
	}

	END_OF_SMB

	if (cmd != 0xff) { 	/* there is an andX command */
		if (andxoffset < offset) {
			THROW(ReportedBoundsError);
		}
		dissect_smb_command(tvb, pinfo, andxoffset, smb_tree, cmd, FALSE, si);
	}

	return offset;
}

static int
dissect_session_setup_andx_response(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree, smb_info_t *si)
{
	guint8      wc, cmd    = 0xff;
	guint16     andxoffset = 0, bc;
	guint16     sbloblen   = 0;
	int         an_len;
	const char *an;

	DISSECTOR_ASSERT(si);

	WORD_COUNT;

	if (!pinfo->fd->flags.visited && si->sip && si->sip->extra_info &&
	    (si->sip->extra_info_type == SMB_EI_UID)) {
		smb_uid_t *smb_uid;

		smb_uid = (smb_uid_t *)si->sip->extra_info;
		smb_uid->logged_in = pinfo->num;
		wmem_tree_insert32(si->ct->uid_tree, si->uid, smb_uid);
	}

	/* next smb command */
	cmd = tvb_get_guint8(tvb, offset);
	if (cmd != 0xff) {
		proto_tree_add_uint(tree, hf_smb_andxcmd, tvb, offset, 1, cmd);
	} else {
		proto_tree_add_uint_format_value(tree, hf_smb_andxcmd, tvb, offset, 1, cmd, "No further commands (0xff)");
	}
	offset += 1;

	/* reserved byte */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 1, ENC_NA);
	offset += 1;

	/* andxoffset */
	andxoffset = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_andxoffset, tvb, offset, 2, andxoffset);
	offset += 2;

	/* flags */
	offset = dissect_setup_action(tvb, tree, offset);

	if (wc == 4) {
		/* security blob length */
		sbloblen = tvb_get_letohs(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_security_blob_len, tvb, offset, 2, sbloblen);
		offset += 2;
	}

	BYTE_COUNT;

	if (wc == 4) {
		proto_item *blob_item;

		/* security blob */
		/* don't try to eat too much of we might get an exception on
		 * short frames and then we will not see anything at all
		 * of the security blob.
		 */
		if (sbloblen > tvb_reported_length_remaining(tvb, offset)) {
			sbloblen = tvb_reported_length_remaining(tvb, offset);
		}
		blob_item = proto_tree_add_item(tree, hf_smb_security_blob,
						tvb, offset, sbloblen, ENC_NA);

		if (sbloblen) {
			tvbuff_t *blob_tvb;
			proto_tree *blob_tree;

			blob_tree = proto_item_add_subtree(blob_item,
							   ett_smb_secblob);
			CHECK_BYTE_COUNT(sbloblen);

			blob_tvb = tvb_new_subset_length(tvb, offset, sbloblen);

			if (si && si->ct && si->ct->raw_ntlmssp &&
			    (tvb_strneql(tvb, offset, "NTLMSSP", 7) == 0)) {
			  call_dissector(ntlmssp_handle, blob_tvb, pinfo,
					 blob_tree);

			}
			else {
			  call_dissector(gssapi_handle, blob_tvb, pinfo,
					 blob_tree);

			}

			COUNT_BYTES(sbloblen);
		}
	}

	/* OS */
	an = get_unicode_or_ascii_string(tvb, &offset,
		si->unicode, &an_len, FALSE, FALSE, &bc);
	if (an == NULL)
		goto endofcommand;
	proto_tree_add_string(tree, hf_smb_os, tvb,
		offset, an_len, an);
	COUNT_BYTES(an_len);

	/* LANMAN */
	an = get_unicode_or_ascii_string(tvb, &offset,
		si->unicode, &an_len, FALSE, FALSE, &bc);
	if (an == NULL)
		goto endofcommand;
	proto_tree_add_string(tree, hf_smb_lanman, tvb,
		offset, an_len, an);
	COUNT_BYTES(an_len);

	if ((wc == 3) || (wc == 4)) {
		/* Primary domain */
		an = get_unicode_or_ascii_string(tvb, &offset,
			si->unicode, &an_len, FALSE, FALSE, &bc);
		if (an == NULL)
			goto endofcommand;
		proto_tree_add_string(tree, hf_smb_primary_domain, tvb,
			offset, an_len, an);
		COUNT_BYTES(an_len);
	}

	END_OF_SMB

	if (cmd != 0xff) { 	/* there is an andX command */
		if (andxoffset < offset) {
			THROW(ReportedBoundsError);
		}
		dissect_smb_command(tvb, pinfo, andxoffset, smb_tree, cmd, FALSE, si);
	}

	return offset;
}


static int
dissect_empty_andx(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree, smb_info_t *si _U_)
{
	guint8	wc, cmd    = 0xff;
	guint16 andxoffset = 0;
	guint16 bc;

	WORD_COUNT;

	/* next smb command */
	cmd = tvb_get_guint8(tvb, offset);
	if (cmd != 0xff) {
		proto_tree_add_uint(tree, hf_smb_andxcmd, tvb, offset, 1, cmd);
	} else {
		proto_tree_add_uint_format_value(tree, hf_smb_andxcmd, tvb, offset, 1, cmd, "No further commands (0xff)");
	}
	offset += 1;

	/* reserved byte */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 1, ENC_NA);
	offset += 1;

	/* andxoffset */
	andxoffset = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_andxoffset, tvb, offset, 2, andxoffset);
	offset += 2;

	BYTE_COUNT;

	END_OF_SMB

	if (cmd != 0xff) { 	/* there is an andX command */
		if (andxoffset < offset) {
			THROW(ReportedBoundsError);
		}
		dissect_smb_command(tvb, pinfo, andxoffset, smb_tree, cmd, FALSE, si);
	}

	return offset;
}

	/*
	 * From [MS-SMB] - v20100711 Server Message Block (SMB) Protocol Specification
	 * http://download.microsoft.com/download/a/e/6/ae6e4142-aa58-45c6-8dcf-a657e5900cd3/%5BMS-SMB%5D.pdf
	 * 2.2.4.7 SMB_COM_TREE_CONNECT_ANDX (0x75)
	 */

static const true_false_string tfs_connect_support_search = {
	"Exclusive search bits supported",
	"Exclusive search bits not supported"
};
static const true_false_string tfs_connect_support_in_dfs = {
	"Share is in Dfs",
	"Share isn't in Dfs"
};
static const value_string connect_support_csc_mask_vals[] = {
	{ 0,	"Automatic file-to-file reintegration NOT permitted"},
	{ 1,	"Automatic file-to-file reintegration permitted"},
	{ 2,	"Offline caching allow for the share"},
	{ 3,	"Offline caching NOT allow for the share"},
	{0, NULL}
};
static const true_false_string tfs_connect_support_uniquefilename = {
	"Client allow to cache share namespaces",
	"Client NOT allow to cache share namespaces"
};
static const true_false_string tfs_connect_support_extended_signature = {
	"Extended signature",
	"NOT extended signature"
};

static int
dissect_connect_support_bits(tvbuff_t *tvb, proto_tree *parent_tree, int offset)
{
	static const int * flags[] = {
		&hf_smb_connect_support_search,
		&hf_smb_connect_support_in_dfs,
		&hf_smb_connect_support_csc_mask_vals,
		&hf_smb_connect_support_uniquefilename,
		&hf_smb_connect_support_extended_signature,
		NULL
	};

	proto_tree_add_bitmask(parent_tree, tvb, offset, hf_smb_connect_support, ett_smb_connect_support_bits, flags, ENC_LITTLE_ENDIAN);
	offset += 2;

	return offset;
}

static const true_false_string tfs_disconnect_tid = {
	"DISCONNECT TID",
	"Do NOT disconnect TID"
};

static const true_false_string tfs_extended_signature = {
	"Extended Signature",
	"NOT Extended Signature"
};

static const true_false_string tfs_extended_response = {
	"Extended Response",
	"NOT Extended Response"
};

static int
dissect_connect_flags(tvbuff_t *tvb, proto_tree *parent_tree, int offset)
{
	static const int * flags[] = {
		&hf_smb_connect_flags_dtid,
		&hf_smb_connect_flags_ext_sig,
		&hf_smb_connect_flags_ext_resp,
		NULL
	};

	proto_tree_add_bitmask(parent_tree, tvb, offset, hf_smb_connect_flags, ett_smb_connect_flags, flags, ENC_LITTLE_ENDIAN);
	offset += 2;

	return offset;
}

static int
dissect_tree_connect_andx_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree, smb_info_t *si)
{
	guint8        wc, cmd    = 0xff;
	guint16       bc;
	guint16       andxoffset = 0, pwlen = 0;
	int           an_len;
	const guint8 *an;

	DISSECTOR_ASSERT(si);

	WORD_COUNT;

	/* next smb command */
	cmd = tvb_get_guint8(tvb, offset);
	if (cmd != 0xff) {
		proto_tree_add_uint(tree, hf_smb_andxcmd, tvb, offset, 1, cmd);
	} else {
		proto_tree_add_uint_format_value(tree, hf_smb_andxcmd, tvb, offset, 1, cmd, "No further commands (0xff)");
	}
	offset += 1;

	/* reserved byte */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 1, ENC_NA);
	offset += 1;

	/* andxoffset */
	andxoffset = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_andxoffset, tvb, offset, 2, andxoffset);
	offset += 2;

	/* flags */
	offset = dissect_connect_flags(tvb, tree, offset);

	/* password length*/
	pwlen = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_password_len, tvb, offset, 2, pwlen);
	offset += 2;

	BYTE_COUNT;

	/* password */
	CHECK_BYTE_COUNT(pwlen);
	proto_tree_add_item(tree, hf_smb_password,
		tvb, offset, pwlen, ENC_NA);
	COUNT_BYTES(pwlen);

	/* Path */
	an = get_unicode_or_ascii_string(tvb, &offset,
		si->unicode, &an_len, FALSE, FALSE, &bc);
	if (an == NULL)
		goto endofcommand;
	proto_tree_add_string(tree, hf_smb_path, tvb,
		offset, an_len, an);
	COUNT_BYTES(an_len);

	/* store it for the tid->name/openframe/closeframe matching in
	 * dissect_smb_tid()   called from the response.
	 */
	if ((!pinfo->fd->flags.visited) && si->sip && an) {
		si->sip->extra_info_type = SMB_EI_TIDNAME;
		si->sip->extra_info = wmem_strdup(wmem_file_scope(), an);
	}

	col_append_fstr(pinfo->cinfo, COL_INFO, ", Path: %s",
		    format_text(an, strlen(an)));

	/*
	 * NOTE: the Service string is always ASCII, even if the
	 * "strings are Unicode" bit is set in the flags2 field
	 * of the SMB.
	 */

	/* Service */
	/* XXX - what if this runs past bc? */
	an_len = tvb_strsize(tvb, offset);
	CHECK_BYTE_COUNT(an_len);
	proto_tree_add_item_ret_string(tree, hf_smb_service, tvb,
		offset, an_len, ENC_ASCII|ENC_NA, wmem_packet_scope(), &an);
	COUNT_BYTES(an_len);

	END_OF_SMB

	if (cmd != 0xff) { 	/* there is an andX command */
		if (andxoffset < offset) {
			THROW(ReportedBoundsError);
		}
		dissect_smb_command(tvb, pinfo, andxoffset, smb_tree, cmd, FALSE, si);
	}

	return offset;
}


static int
dissect_tree_connect_andx_response(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree, smb_info_t *si)
{
	guint8        wc, cmd = 0xff;
	guint16       andxoffset     = 0;
	guint16       bc;
	int           an_len;
	const guint8 *an;

	DISSECTOR_ASSERT(si);

	WORD_COUNT;

	/* next smb command */
	cmd = tvb_get_guint8(tvb, offset);
	if (cmd != 0xff) {
		proto_tree_add_uint(tree, hf_smb_andxcmd, tvb, offset, 1, cmd);
	} else {
		proto_tree_add_uint_format_value(tree, hf_smb_andxcmd, tvb, offset, 1, cmd, "No further commands (0xff)");
	}
	offset += 1;

	/* reserved byte */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 1, ENC_NA);
	offset += 1;

	/* andxoffset */
	andxoffset = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_andxoffset, tvb, offset, 2, andxoffset);
	offset += 2;

	/* There are three valid formats of tree connect response.
	   All have the first two words: andx_cmd, andx_off,
	   and then have additional words as follows:
		wc=2: (ancient LanMan -- no more words)
		wc=3: (NT, non-ext) opt_support
		wc=7: (NT, extended) opt_support,
			tree_access(2w), guest_access(2w)
	   byte_count follows those words as usual */

	if (wc >= 3) {
		/* flags */
		offset = dissect_connect_support_bits(tvb, tree, offset);
	}

	if (wc == 7) {
		/*
		 * Refer to [MS-SMB] - v20100711
		 * When a server returns extended information, the response
		 * takes the following format, with WordCount = 7.
		 * MaximalShareAccessRights, and GuestMaximalShareAccessRights fields
		 * has added.
		 */
		proto_tree *tr;
		tr = proto_tree_add_subtree(tree, tvb, offset, 4,
			ett_smb_nt_access_mask, NULL, "Maximal Share Access Rights");
		offset = dissect_smb_access_mask(tvb, tr, offset);

		tr = proto_tree_add_subtree(tree, tvb, offset, 4,
			ett_smb_nt_access_mask, NULL, "Guest Maximal Share Access Rights");
		offset = dissect_smb_access_mask(tvb, tr, offset);
	}

	BYTE_COUNT;

	/*
	 * NOTE: even though the SNIA CIFS spec doesn't say there's
	 * a "Service" string if there's a word count of 2, the
	 * document at
	 *
	 *	ftp://ftp.microsoft.com/developr/drg/CIFS/dosextp.txt
	 *
	 * (it's in an ugly format - text intended to be sent to a
	 * printer, with backspaces and overstrikes used for boldfacing
	 * and underlining; UNIX "col -b" can be used to strip the
	 * overstrikes out) says there's a "Service" string there, and
	 * some network traffic has it.
	 */

	/*
	 * NOTE: the Service string is always ASCII, even if the
	 * "strings are Unicode" bit is set in the flags2 field
	 * of the SMB.
	 */

	/* Service */
	/* XXX - what if this runs past bc? */
	an_len = tvb_strsize(tvb, offset);
	CHECK_BYTE_COUNT(an_len);
	proto_tree_add_item_ret_string(tree, hf_smb_service, tvb,
		offset, an_len, ENC_ASCII|ENC_NA, wmem_packet_scope(), &an);
	COUNT_BYTES(an_len);

	/* Now when we know the service type, store it so that we know it for later commands down
	   this tree */
	if (!pinfo->fd->flags.visited) {
		/* Remove any previous entry for this TID */
		if (g_hash_table_lookup(si->ct->tid_service, GUINT_TO_POINTER(si->tid))) {
			g_hash_table_remove(si->ct->tid_service, GUINT_TO_POINTER(si->tid));
		}
		if (strcmp(an, "IPC") == 0) {
			g_hash_table_insert(si->ct->tid_service, GUINT_TO_POINTER(si->tid), (void *)TID_IPC);
		} else {
			g_hash_table_insert(si->ct->tid_service, GUINT_TO_POINTER(si->tid), (void *)TID_NORMAL);
		}
	}

	if (bc != 0) {
		/*
		 * Sometimes this isn't present.
		 */

		/* Native FS */
		an = get_unicode_or_ascii_string(tvb, &offset,
			si->unicode, &an_len, /*TRUE*/FALSE, FALSE,
			&bc);
		if (an == NULL)
			goto endofcommand;
		proto_tree_add_string(tree, hf_smb_fs, tvb,
			offset, an_len, an);
		COUNT_BYTES(an_len);
	}

	END_OF_SMB

	if (cmd != 0xff) { 	/* there is an andX command */
		if (andxoffset < offset) {
			THROW(ReportedBoundsError);
		}
		dissect_smb_command(tvb, pinfo, andxoffset, smb_tree, cmd, FALSE, si);
	}

	return offset;
}



/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
   NT Transaction command  begins here
   XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */
#define NT_TRANS_CREATE		1
#define NT_TRANS_IOCTL		2
#define NT_TRANS_SSD		3
#define NT_TRANS_NOTIFY		4
#define NT_TRANS_RENAME		5
#define NT_TRANS_QSD		6
#define NT_TRANS_GET_USER_QUOTA	7
#define NT_TRANS_SET_USER_QUOTA 8
static const value_string nt_cmd_vals[] = {
	{NT_TRANS_CREATE,		"NT CREATE"},
	{NT_TRANS_IOCTL,		"NT IOCTL"},
	{NT_TRANS_SSD,			"NT SET SECURITY DESC"},
	{NT_TRANS_NOTIFY,		"NT NOTIFY"},
	{NT_TRANS_RENAME,		"NT RENAME"},
	{NT_TRANS_QSD,			"NT QUERY SECURITY DESC"},
	{NT_TRANS_GET_USER_QUOTA,	"NT GET USER QUOTA"},
	{NT_TRANS_SET_USER_QUOTA,	"NT SET USER QUOTA"},
	{0, NULL}
};
value_string_ext nt_cmd_vals_ext = VALUE_STRING_EXT_INIT(nt_cmd_vals);

static const value_string nt_ioctl_isfsctl_vals[] = {
	{0,	"Device IOCTL"},
	{1,	"FS control : FSCTL"},
	{0, NULL}
};

#define NT_IOCTL_FLAGS_ROOT_HANDLE	0x01
static const true_false_string tfs_nt_ioctl_flags_root_handle = {
	"Apply the command to share root handle (MUST BE Dfs)",
	"Apply to this share",
};

static const value_string nt_notify_action_vals[] = {
	{1,	"ADDED (object was added"},
	{2,	"REMOVED (object was removed)"},
	{3,	"MODIFIED (object was modified)"},
	{4,	"RENAMED_OLD_NAME (this is the old name of object)"},
	{5,	"RENAMED_NEW_NAME (this is the new name of object)"},
	{6,	"ADDED_STREAM (a stream was added)"},
	{7,	"REMOVED_STREAM (a stream was removed)"},
	{8,	"MODIFIED_STREAM (a stream was modified)"},
	{0, NULL}
};

static const value_string watch_tree_vals[] = {
	{0,	"Current directory only"},
	{1,	"Subdirectories also"},
	{0, NULL}
};

#define NT_NOTIFY_STREAM_WRITE	0x00000800
#define NT_NOTIFY_STREAM_SIZE	0x00000400
#define NT_NOTIFY_STREAM_NAME	0x00000200
#define NT_NOTIFY_SECURITY	0x00000100
#define NT_NOTIFY_EA		0x00000080
#define NT_NOTIFY_CREATION	0x00000040
#define NT_NOTIFY_LAST_ACCESS	0x00000020
#define NT_NOTIFY_LAST_WRITE	0x00000010
#define NT_NOTIFY_SIZE		0x00000008
#define NT_NOTIFY_ATTRIBUTES	0x00000004
#define NT_NOTIFY_DIR_NAME	0x00000002
#define NT_NOTIFY_FILE_NAME	0x00000001
static const true_false_string tfs_nt_notify_stream_write = {
	"Notify on changes to STREAM WRITE",
	"Do NOT notify on changes to stream write",
};
static const true_false_string tfs_nt_notify_stream_size = {
	"Notify on changes to STREAM SIZE",
	"Do NOT notify on changes to stream size",
};
static const true_false_string tfs_nt_notify_stream_name = {
	"Notify on changes to STREAM NAME",
	"Do NOT notify on changes to stream name",
};
static const true_false_string tfs_nt_notify_security = {
	"Notify on changes to SECURITY",
	"Do NOT notify on changes to security",
};
static const true_false_string tfs_nt_notify_ea = {
	"Notify on changes to EA",
	"Do NOT notify on changes to EA",
};
static const true_false_string tfs_nt_notify_creation = {
	"Notify on changes to CREATION TIME",
	"Do NOT notify on changes to creation time",
};
static const true_false_string tfs_nt_notify_last_access = {
	"Notify on changes to LAST ACCESS TIME",
	"Do NOT notify on changes to last access time",
};
static const true_false_string tfs_nt_notify_last_write = {
	"Notify on changes to LAST WRITE TIME",
	"Do NOT notify on changes to last write time",
};
static const true_false_string tfs_nt_notify_size = {
	"Notify on changes to SIZE",
	"Do NOT notify on changes to size",
};
static const true_false_string tfs_nt_notify_attributes = {
	"Notify on changes to ATTRIBUTES",
	"Do NOT notify on changes to attributes",
};
static const true_false_string tfs_nt_notify_dir_name = {
	"Notify on changes to DIR NAME",
	"Do NOT notify on changes to dir name",
};
static const true_false_string tfs_nt_notify_file_name = {
	"Notify on changes to FILE NAME",
	"Do NOT notify on changes to file name",
};

const value_string create_disposition_vals[] = {
	{0,	"Supersede (supersede existing file (if it exists))"},
	{1,	"Open (if file exists open it, else fail)"},
	{2,	"Create (if file exists fail, else create it)"},
	{3,	"Open If (if file exists open it, else create it)"},
	{4,	"Overwrite (if file exists overwrite, else fail)"},
	{5,	"Overwrite If (if file exists overwrite, else create it)"},
	{0, NULL}
};

const value_string impersonation_level_vals[] = {
	{0,	"Anonymous"},
	{1,	"Identification"},
	{2,	"Impersonation"},
	{3,	"Delegation"},
	{0, NULL}
};

static const true_false_string tfs_nt_security_flags_context_tracking = {
	"Security tracking mode is DYNAMIC",
	"Security tracking mode is STATIC",
};

static const true_false_string tfs_nt_security_flags_effective_only = {
	"ONLY ENABLED aspects of the client's security context are available",
	"ALL aspects of the client's security context are available",
};

static const true_false_string tfs_nt_create_bits_oplock = {
	"Requesting OPLOCK",
	"Does NOT request oplock"
};

static const true_false_string tfs_nt_create_bits_boplock = {
	"Requesting BATCH OPLOCK",
	"Does NOT request batch oplock"
};

/*
 * XXX - must be a directory, and can be a file, or can be a directory,
 * and must be a file?
 */
static const true_false_string tfs_nt_create_bits_dir = {
	"Target of open MUST be a DIRECTORY",
	"Target of open can be a file"
};

static const true_false_string tfs_nt_create_bits_ext_resp = {
	"Extended responses required",
	"Extended responses NOT required"
};

static const true_false_string tfs_nt_access_mask_generic_read = {
	"GENERIC READ is set",
	"Generic read is NOT set"
};
static const true_false_string tfs_nt_access_mask_generic_write = {
	"GENERIC WRITE is set",
	"Generic write is NOT set"
};
static const true_false_string tfs_nt_access_mask_generic_execute = {
	"GENERIC EXECUTE is set",
	"Generic execute is NOT set"
};
static const true_false_string tfs_nt_access_mask_generic_all = {
	"GENERIC ALL is set",
	"Generic all is NOT set"
};
static const true_false_string tfs_nt_access_mask_maximum_allowed = {
	"MAXIMUM ALLOWED is set",
	"Maximum allowed is NOT set"
};
static const true_false_string tfs_nt_access_mask_system_security = {
	"SYSTEM SECURITY is set",
	"System security is NOT set"
};
static const true_false_string tfs_nt_access_mask_synchronize = {
	"Can wait on handle to SYNCHRONIZE on completion of I/O",
	"Can NOT wait on handle to synchronize on completion of I/O"
};
static const true_false_string tfs_nt_access_mask_write_owner = {
	"Can WRITE OWNER (take ownership)",
	"Can NOT write owner (take ownership)"
};
static const true_false_string tfs_nt_access_mask_write_dac = {
	"OWNER may WRITE the DAC",
	"Owner may NOT write to the DAC"
};
static const true_false_string tfs_nt_access_mask_read_control = {
	"READ ACCESS to owner, group and ACL of the SID",
	"Read access is NOT granted to owner, group and ACL of the SID"
};
static const true_false_string tfs_nt_access_mask_delete = {
	"DELETE access",
	"NO delete access"
};
static const true_false_string tfs_nt_access_mask_write_attributes = {
	"WRITE ATTRIBUTES access",
	"NO write attributes access"
};
static const true_false_string tfs_nt_access_mask_read_attributes = {
	"READ ATTRIBUTES access",
	"NO read attributes access"
};
static const true_false_string tfs_nt_access_mask_delete_child = {
	"DELETE CHILD access",
	"NO delete child access"
};
static const true_false_string tfs_nt_access_mask_execute = {
	"EXECUTE access",
	"NO execute access"
};
static const true_false_string tfs_nt_access_mask_write_ea = {
	"WRITE EXTENDED ATTRIBUTES access",
	"NO write extended attributes access"
};
static const true_false_string tfs_nt_access_mask_read_ea = {
	"READ EXTENDED ATTRIBUTES access",
	"NO read extended attributes access"
};
static const true_false_string tfs_nt_access_mask_append = {
	"APPEND access",
	"NO append access"
};
static const true_false_string tfs_nt_access_mask_write = {
	"WRITE access",
	"NO write access"
};
static const true_false_string tfs_nt_access_mask_read = {
	"READ access",
	"NO read access"
};

static const true_false_string tfs_nt_share_access_delete = {
	"Object can be shared for DELETE",
	"Object can NOT be shared for delete"
};
static const true_false_string tfs_nt_share_access_write = {
	"Object can be shared for WRITE",
	"Object can NOT be shared for write"
};
static const true_false_string tfs_nt_share_access_read = {
	"Object can be shared for READ",
	"Object can NOT be shared for read"
};

static const value_string oplock_level_vals[] = {
	{0,	"No oplock granted"},
	{1,	"Exclusive oplock granted"},
	{2,	"Batch oplock granted"},
	{3,	"Level II oplock granted"},
	{0, NULL}
};

static const value_string device_type_vals[] = {
	{0x00000001,	"Beep"},
	{0x00000002,	"CDROM"},
	{0x00000003,	"CDROM Filesystem"},
	{0x00000004,	"Controller"},
	{0x00000005,	"Datalink"},
	{0x00000006,	"Dfs"},
	{0x00000007,	"Disk"},
	{0x00000008,	"Disk Filesystem"},
	{0x00000009,	"Filesystem"},
	{0x0000000a,	"Inport Port"},
	{0x0000000b,	"Keyboard"},
	{0x0000000c,	"Mailslot"},
	{0x0000000d,	"MIDI-In"},
	{0x0000000e,	"MIDI-Out"},
	{0x0000000f,	"Mouse"},
	{0x00000010,	"Multi UNC Provider"},
	{0x00000011,	"Named Pipe"},
	{0x00000012,	"Network"},
	{0x00000013,	"Network Browser"},
	{0x00000014,	"Network Filesystem"},
	{0x00000015,	"NULL"},
	{0x00000016,	"Parallel Port"},
	{0x00000017,	"Physical card"},
	{0x00000018,	"Printer"},
	{0x00000019,	"Scanner"},
	{0x0000001a,	"Serial Mouse port"},
	{0x0000001b,	"Serial port"},
	{0x0000001c,	"Screen"},
	{0x0000001d,	"Sound"},
	{0x0000001e,	"Streams"},
	{0x0000001f,	"Tape"},
	{0x00000020,	"Tape Filesystem"},
	{0x00000021,	"Transport"},
	{0x00000022,	"Unknown"},
	{0x00000023,	"Video"},
	{0x00000024,	"Virtual Disk"},
	{0x00000025,	"WAVE-In"},
	{0x00000026,	"WAVE-Out"},
	{0x00000027,	"8042 Port"},
	{0x00000028,	"Network Redirector"},
	{0x00000029,	"Battery"},
	{0x0000002a,	"Bus Extender"},
	{0x0000002b,	"Modem"},
	{0x0000002c,	"VDM"},
	{0,	NULL}
};
static value_string_ext device_type_vals_ext = VALUE_STRING_EXT_INIT(device_type_vals);

static const value_string is_directory_vals[] = {
	{0,	"This is NOT a directory"},
	{1,	"This is a DIRECTORY"},
	{0, NULL}
};

typedef struct _nt_trans_data {
	int     subcmd;
	guint32 sd_len;
	guint32 ea_len;
} nt_trans_data;



static int
dissect_nt_security_flags(tvbuff_t *tvb, proto_tree *parent_tree, int offset)
{
	static const int * flags[] = {
		&hf_smb_nt_security_flags_context_tracking,
		&hf_smb_nt_security_flags_effective_only,
		NULL
	};

	proto_tree_add_bitmask(parent_tree, tvb, offset, hf_smb_nt_security_flags, ett_smb_nt_security_flags, flags, ENC_NA);
	offset += 1;

	return offset;
}

/*
 * XXX - there are some more flags in the description of "ZwOpenFile()"
 * in "Windows(R) NT(R)/2000 Native API Reference"; do those go over
 * the wire as well?  (The spec at
 *
 *	http://www.samba.org/samba/ftp/specs/smb-nt01.doc
 *
 * says that "the FILE_NO_INTERMEDIATE_BUFFERING option is not exported
 * via the SMB protocol.  The NT redirector should convert this option
 * to FILE_WRITE_THROUGH."
 *
 * The "Sync I/O Alert" and "Sync I/O Nonalert" are given the bit
 * values one would infer from their position in the list of flags for
 * "ZwOpenFile()".  Most of the others probably have those values
 * as well, although "8.3 only" would collide with FILE_OPEN_FOR_RECOVERY,
 * which might go over the wire (for the benefit of backup/restore software).
 */
static const true_false_string tfs_nt_create_options_directory = {
	"File being created/opened must be a directory",
	"File being created/opened must not be a directory"
};
static const true_false_string tfs_nt_create_options_write_through = {
	"Writes should flush buffered data before completing",
	"Writes need not flush buffered data before completing"
};
static const true_false_string tfs_nt_create_options_sequential_only = {
	"The file will only be accessed sequentially",
	"The file might not only be accessed sequentially"
};
static const true_false_string tfs_nt_create_options_no_intermediate_buffering = {
	"NO intermediate buffering is allowed",
	"Intermediate buffering is allowed"
};
static const true_false_string tfs_nt_create_options_sync_io_alert = {
	"All operations SYNCHRONOUS, waits subject to termination from alert",
	"Operations NOT necessarily synchronous"
};
static const true_false_string tfs_nt_create_options_sync_io_nonalert = {
	"All operations SYNCHRONOUS, waits not subject to alert",
	"Operations NOT necessarily synchronous"
};
static const true_false_string tfs_nt_create_options_non_directory = {
	"File being created/opened must not be a directory",
	"File being created/opened must be a directory"
};
static const true_false_string tfs_nt_create_options_create_tree_connection = {
	"Create Tree Connections is SET",
	"Create Tree Connections is NOT set"
};
static const true_false_string tfs_nt_create_options_complete_if_oplocked = {
	"Complete if oplocked is SET",
	"Complete if oplocked is NOT set"
};
static const true_false_string tfs_nt_create_options_no_ea_knowledge = {
	"The client does not understand extended attributes",
	"The client understands extended attributes"
};
static const true_false_string tfs_nt_create_options_eight_dot_three_only = {
	"The client understands only 8.3 file names",
	"The client understands long file names"
};
static const true_false_string tfs_nt_create_options_random_access = {
	"The file will be accessed randomly",
	"The file will not be accessed randomly"
};
static const true_false_string tfs_nt_create_options_delete_on_close = {
	"The file should be deleted when it is closed",
	"The file should not be deleted when it is closed"
};
static const true_false_string tfs_nt_create_options_open_by_fileid = {
	"OpenByFileID bit is SET",
	"OpenByFileID is NOT set"
};
static const true_false_string tfs_nt_create_options_backup_intent = {
	"This is a create with BACKUP INTENT",
	"This is a normal create"
};
static const true_false_string tfs_nt_create_options_no_compression = {
	"Open/Create with NO Compression",
	"Compression is allowed for Open/Create"
};
static const true_false_string tfs_nt_create_options_reserve_opfilter = {
	"Reserve Opfilter is SET",
	"Reserve Opfilter is NOT set"
};
static const true_false_string tfs_nt_create_options_open_reparse_point = {
	"Open a Reparse Point",
	"Normal open"
};
static const true_false_string tfs_nt_create_options_open_no_recall = {
	"Open No Recall is SET",
	"Open no recall is NOT set"
};
static const true_false_string tfs_nt_create_options_open_for_free_space_query = {
	"This is an OPEN FOR FREE SPACE QUERY",
	"This is NOT an open for free space query"
};

int
dissect_nt_notify_completion_filter(tvbuff_t *tvb, proto_tree *parent_tree, int offset)
{
	static const int * flags[] = {
		&hf_smb_nt_notify_file_name,
		&hf_smb_nt_notify_dir_name,
		&hf_smb_nt_notify_attributes,
		&hf_smb_nt_notify_size,
		&hf_smb_nt_notify_last_write,
		&hf_smb_nt_notify_last_access,
		&hf_smb_nt_notify_creation,
		&hf_smb_nt_notify_ea,
		&hf_smb_nt_notify_security,
		&hf_smb_nt_notify_stream_name,
		&hf_smb_nt_notify_stream_size,
		&hf_smb_nt_notify_stream_write,
		NULL
	};

	proto_tree_add_bitmask(parent_tree, tvb, offset, hf_smb_nt_notify_completion_filter, ett_smb_nt_notify_completion_filter, flags, ENC_LITTLE_ENDIAN);
	offset += 4;

	return offset;
}

static int
dissect_nt_ioctl_flags(tvbuff_t *tvb, proto_tree *parent_tree, int offset)
{
	static const int * flags[] = {
		&hf_smb_nt_ioctl_flags_root_handle,
		NULL
	};

	proto_tree_add_bitmask(parent_tree, tvb, offset, hf_smb_nt_ioctl_flags_completion_filter, ett_smb_nt_ioctl_flags, flags, ENC_NA);
	offset += 1;

	return offset;
}

/*
 * From the section on ZwQuerySecurityObject in "Windows(R) NT(R)/2000
 * Native API Reference".
 */
static const true_false_string tfs_nt_qsd_owner = {
	"Requesting OWNER security information",
	"NOT requesting owner security information",
};

static const true_false_string tfs_nt_qsd_group = {
	"Requesting GROUP security information",
	"NOT requesting group security information",
};

static const true_false_string tfs_nt_qsd_dacl = {
	"Requesting DACL security information",
	"NOT requesting DACL security information",
};

static const true_false_string tfs_nt_qsd_sacl = {
	"Requesting SACL security information",
	"NOT requesting SACL security information",
};

#define NT_QSD_OWNER	0x00000001
#define NT_QSD_GROUP	0x00000002
#define NT_QSD_DACL	0x00000004
#define NT_QSD_SACL	0x00000008

int
dissect_security_information_mask(tvbuff_t *tvb, proto_tree *parent_tree, int offset)
{
	static const int * flags[] = {
		&hf_smb_nt_qsd_owner,
		&hf_smb_nt_qsd_group,
		&hf_smb_nt_qsd_dacl,
		&hf_smb_nt_qsd_sacl,
		NULL
	};

	proto_tree_add_bitmask(parent_tree, tvb, offset, hf_smb_nt_qsd, ett_smb_security_information_mask, flags, ENC_LITTLE_ENDIAN);
	offset += 4;

	return offset;
}

static int
dissect_nt_user_quota(tvbuff_t *tvb, proto_tree *tree, int offset, guint16 *bcp)
{
	int     old_offset, old_sid_offset;
	guint32 qsize;

	do {
		old_offset = offset;

		CHECK_BYTE_COUNT_TRANS_SUBR(4);
		qsize = tvb_get_letohl(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_user_quota_offset, tvb, offset, 4, qsize);
		COUNT_BYTES_TRANS_SUBR(4);

		CHECK_BYTE_COUNT_TRANS_SUBR(4);
		/* length of SID */
		proto_tree_add_item(tree, hf_smb_length_of_sid, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS_SUBR(4);

		/* 16 unknown bytes */
		CHECK_BYTE_COUNT_TRANS_SUBR(8);
		proto_tree_add_item(tree, hf_smb_unknown, tvb,
			    offset, 8, ENC_NA);
		COUNT_BYTES_TRANS_SUBR(8);

		/* number of bytes for used quota */
		CHECK_BYTE_COUNT_TRANS_SUBR(8);
		proto_tree_add_item(tree, hf_smb_user_quota_used, tvb, offset, 8, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS_SUBR(8);

		/* number of bytes for quota warning */
		CHECK_BYTE_COUNT_TRANS_SUBR(8);
		proto_tree_add_item(tree, hf_smb_soft_quota_limit, tvb, offset, 8, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS_SUBR(8);

		/* number of bytes for quota limit */
		CHECK_BYTE_COUNT_TRANS_SUBR(8);
		proto_tree_add_item(tree, hf_smb_hard_quota_limit, tvb, offset, 8, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS_SUBR(8);

		/* SID of the user */
		old_sid_offset = offset;
		offset = dissect_nt_sid(tvb, offset, tree, "Quota", NULL, -1);
		*bcp -= (offset-old_sid_offset);

		if (qsize) {
			offset = old_offset+qsize;
		}
	}while (qsize);


	return offset;
}


static int
dissect_nt_trans_data_request(tvbuff_t *tvb, packet_info *pinfo, int offset, proto_tree *parent_tree, int bc, nt_trans_data *ntd, smb_nt_transact_info_t *nti, smb_info_t *si)
{
	proto_tree              *tree;
	int                      old_offset = offset;
	guint16                  bcp        = bc; /* XXX fixme */
	struct access_mask_info *ami        = NULL;
	tvbuff_t                *ioctl_tvb;

	DISSECTOR_ASSERT(si);

	tree = proto_tree_add_subtree_format(parent_tree, tvb, offset, -1,
				ett_smb_nt_trans_data, NULL, "%s Data",
				val_to_str_ext(ntd->subcmd, &nt_cmd_vals_ext, "Unknown NT transaction (%u)"));

	switch(ntd->subcmd) {
	case NT_TRANS_CREATE:
		/* security descriptor */
		if (ntd->sd_len) {
		        offset = dissect_nt_sec_desc(
				tvb, offset, pinfo, tree, NULL, TRUE,
				ntd->sd_len, NULL);
		}

		/* extended attributes */
		if (ntd->ea_len) {
			proto_tree_add_item(tree, hf_smb_extended_attributes, tvb, offset, ntd->ea_len, ENC_NA);
			offset += ntd->ea_len;
		}

		break;
	case NT_TRANS_IOCTL:
		/* ioctl data */
		ioctl_tvb = tvb_new_subset(tvb, offset, MIN((int)bc, tvb_reported_length_remaining(tvb, offset)), bc);
		if (nti) {
			dissect_smb2_ioctl_data(ioctl_tvb, pinfo, tree, top_tree_global, nti->ioctl_function, TRUE, NULL);
		}

		offset += bc;

		break;
	case NT_TRANS_SSD:
		if (nti) {
			switch(nti->fid_type) {
			case SMB_FID_TYPE_FILE:
				ami = &smb_file_access_mask_info;
				break;
			case SMB_FID_TYPE_DIR:
				ami = &smb_dir_access_mask_info;
				break;
			}
		}

		offset = dissect_nt_sec_desc(
			tvb, offset, pinfo, tree, NULL, TRUE, bc, ami);

		if (offset < (old_offset + bc)) {
			offset = old_offset + bc;
		}

		break;
	case NT_TRANS_NOTIFY:
		break;
	case NT_TRANS_RENAME:
		/* XXX not documented */
		break;
	case NT_TRANS_QSD:
		break;
	case NT_TRANS_GET_USER_QUOTA:
		/* unknown 4 bytes */
		proto_tree_add_item(tree, hf_smb_unknown, tvb,
			    offset, 4, ENC_NA);
		offset += 4;

		/* length of SID */
		proto_tree_add_item(tree, hf_smb_length_of_sid, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		offset +=4;

		offset = dissect_nt_sid(tvb, offset, tree, "Quota", NULL, -1);
		break;
	case NT_TRANS_SET_USER_QUOTA:
		offset = dissect_nt_user_quota(tvb, tree, offset, &bcp);
		break;
	}

	/* ooops there were data we didn't know how to process */
	if ((offset-old_offset) < bc) {
		proto_tree_add_item(tree, hf_smb_unknown, tvb, offset,
		    bc - (offset-old_offset), ENC_NA);
		offset += bc - (offset-old_offset);
	}

	return offset;
}

static int
dissect_nt_trans_param_request(tvbuff_t *tvb, packet_info *pinfo, int offset, proto_tree *parent_tree, int len, nt_trans_data *ntd, guint16 bc, smb_nt_transact_info_t *nti, smb_info_t *si)
{
	proto_tree *tree;
	guint32     fn_len, create_flags, access_mask, share_access, create_options;
	const char *fn;

	DISSECTOR_ASSERT(si);

	tree = proto_tree_add_subtree_format(parent_tree, tvb, offset, len,
				ett_smb_nt_trans_param, NULL, "%s Parameters",
				val_to_str_ext(ntd->subcmd, &nt_cmd_vals_ext, "Unknown NT transaction (%u)"));

	switch(ntd->subcmd) {
	case NT_TRANS_CREATE:
		/* Create flags */
		create_flags = tvb_get_letohl(tvb, offset);
		offset = dissect_nt_create_bits(tvb, tree, offset, 4, create_flags);
		bc -= 4;

		/* root directory fid */
		proto_tree_add_item(tree, hf_smb_root_dir_fid, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		COUNT_BYTES(4);

		/* nt access mask */
		access_mask = tvb_get_letohl(tvb, offset);
		offset = dissect_smb_access_mask_bits(tvb, tree, offset, 4, access_mask);
		bc -= 4;

		/* allocation size */
		proto_tree_add_item(tree, hf_smb_alloc_size64, tvb, offset, 8, ENC_LITTLE_ENDIAN);
		COUNT_BYTES(8);

		/* Extended File Attributes */
		offset = dissect_file_ext_attr(tvb, tree, offset);
		bc -= 4;

		/* share access */
		share_access = tvb_get_letohl(tvb, offset);
		offset = dissect_nt_share_access_bits(tvb, tree, offset, 4, share_access);
		bc -= 4;

		/* create disposition */
		proto_tree_add_item(tree, hf_smb_nt_create_disposition, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		COUNT_BYTES(4);

		/* create options */
		create_options = tvb_get_letohl(tvb, offset);
		offset = dissect_nt_create_options_bits(tvb, tree, offset, 4, create_options);
		bc -= 4;

		/* sd length */
		ntd->sd_len = tvb_get_letohl(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_sd_length, tvb, offset, 4, ntd->sd_len);
		COUNT_BYTES(4);

		/* ea length */
		ntd->ea_len = tvb_get_letohl(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_ea_list_length, tvb, offset, 4, ntd->ea_len);
		COUNT_BYTES(4);

		/* file name len */
		fn_len = (guint32)tvb_get_letohl(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_file_name_len, tvb, offset, 4, fn_len);
		COUNT_BYTES(4);

		/* impersonation level */
		proto_tree_add_item(tree, hf_smb_nt_impersonation_level, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		COUNT_BYTES(4);

		/* security flags */
		offset = dissect_nt_security_flags(tvb, tree, offset);
		bc -= 1;

		/* May need to skip alignment padding. */
		if (offset&1) {
			/* pad byte */
			proto_tree_add_item(tree, hf_smb_padding, tvb, offset, 1, ENC_NA);
			offset += 1;
		}

		/* file name */
		fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len, TRUE, TRUE, &bc);
		if (fn != NULL) {
			proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
				fn);
			COUNT_BYTES(fn_len);
		}

		break;
	case NT_TRANS_IOCTL:
		break;
	case NT_TRANS_SSD: {
		guint16 fid;
		smb_fid_info_t *fid_info;

		/* fid */
		fid = tvb_get_letohs(tvb, offset);
		fid_info = dissect_smb_fid(tvb, pinfo, tree, offset, 2, fid, FALSE, FALSE, FALSE, si);
		offset += 2;
		if (nti) {
			if (fid_info) {
				nti->fid_type = fid_info->type;
			} else {
				nti->fid_type = SMB_FID_TYPE_UNKNOWN;
			}
		}

		/* 2 reserved bytes */
		proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 2, ENC_NA);
		offset += 2;

		/* security information */
		offset = dissect_security_information_mask(tvb, tree, offset);
		break;
	}
	case NT_TRANS_NOTIFY:
		break;
	case NT_TRANS_RENAME:
		/* XXX not documented */
		break;
	case NT_TRANS_QSD: {
		guint16 fid;
		smb_fid_info_t *fid_info;

		/* fid */
		fid = tvb_get_letohs(tvb, offset);
		fid_info = dissect_smb_fid(tvb, pinfo, tree, offset, 2, fid, FALSE, FALSE, FALSE, si);
		offset += 2;
		if (nti) {
			if (fid_info) {
				nti->fid_type = fid_info->type;
			} else {
				nti->fid_type = SMB_FID_TYPE_UNKNOWN;
			}
		}

		/* 2 reserved bytes */
		proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 2, ENC_NA);
		offset += 2;

		/* security information */
		offset = dissect_security_information_mask(tvb, tree, offset);
		break;
	}
	case NT_TRANS_GET_USER_QUOTA:
		/* not decoded yet */
		break;
	case NT_TRANS_SET_USER_QUOTA:
		/* not decoded yet */
		break;
	}

	return offset;
}

static int
dissect_nt_trans_setup_request(tvbuff_t *tvb, packet_info *pinfo, int offset, proto_tree *parent_tree, int len, nt_trans_data *ntd, smb_info_t *si)
{
	proto_tree             *tree;
	smb_nt_transact_info_t *nti  = NULL;
	smb_saved_info_t       *sip;

	DISSECTOR_ASSERT(si);
	sip = si->sip;
	if (sip && (sip->extra_info_type == SMB_EI_NTI)) {
		nti = (smb_nt_transact_info_t *)sip->extra_info;
	}

	tree = proto_tree_add_subtree_format(parent_tree, tvb, offset, len,
				ett_smb_nt_trans_setup, NULL, "%s Setup",
				val_to_str_ext(ntd->subcmd, &nt_cmd_vals_ext, "Unknown NT transaction (%u)"));

	switch(ntd->subcmd) {
	case NT_TRANS_CREATE:
		offset += len;
		break;
	case NT_TRANS_IOCTL: {
		guint16 fid;

		/* function code */
		offset = dissect_smb2_ioctl_function(tvb, pinfo, tree, offset, nti ? &nti->ioctl_function : NULL);

		/* fid */
		fid = tvb_get_letohs(tvb, offset);
		dissect_smb_fid(tvb, pinfo, tree, offset, 2, fid, FALSE, FALSE, FALSE, si);
		offset += 2;

		/* isfsctl */
		proto_tree_add_item(tree, hf_smb_nt_ioctl_isfsctl, tvb, offset, 1, ENC_LITTLE_ENDIAN);
		offset += 1;

		/* isflags */
		offset = dissect_nt_ioctl_flags(tvb, tree, offset);

		break;
	}
	case NT_TRANS_SSD:
		offset += len;
		break;
	case NT_TRANS_NOTIFY: {
		guint16 fid;

		/* completion filter */
		offset = dissect_nt_notify_completion_filter(tvb, tree, offset);

		/* fid */
		fid = tvb_get_letohs(tvb, offset);
		dissect_smb_fid(tvb, pinfo, tree, offset, 2, fid, FALSE, FALSE, FALSE, si);
		offset += 2;

		/* watch tree */
		proto_tree_add_item(tree, hf_smb_nt_notify_watch_tree, tvb, offset, 1, ENC_LITTLE_ENDIAN);
		offset += 1;

		/* reserved byte */
		proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 1, ENC_NA);
		offset += 1;

		break;
	}
	case NT_TRANS_RENAME:
		/* XXX not documented */
		offset += len;
		break;
	case NT_TRANS_QSD:
		break;
	case NT_TRANS_GET_USER_QUOTA:
		/* not decoded yet */
		offset += len;
		break;
	case NT_TRANS_SET_USER_QUOTA:
		/* not decoded yet */
		offset += len;
		break;
	}

	return offset;
}


static int
dissect_nt_transaction_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	guint8                  wc, sc;
	guint32                 pc     = 0, pd = 0, po = 0, dc = 0, od = 0, dd = 0;
	guint32                 td     = 0, tp = 0;
	smb_saved_info_t       *sip;
	int                     subcmd;
	nt_trans_data           ntd;
	guint16                 bc;
	guint32                 padcnt;
	smb_nt_transact_info_t *nti    = NULL;
	fragment_head          *r_fd   = NULL;
	tvbuff_t               *pd_tvb = NULL;
	gboolean                save_fragmented;

	save_fragmented = pinfo->fragmented;

	ntd.subcmd = ntd.sd_len = ntd.ea_len = 0;

	DISSECTOR_ASSERT(si);
	sip = si->sip;

	WORD_COUNT;

	if (wc >= 19) {
		/* primary request */
		/* max setup count */
		proto_tree_add_item(tree, hf_smb_max_setup_count, tvb, offset, 1, ENC_LITTLE_ENDIAN);
		offset += 1;

		/* 2 reserved bytes */
		proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 2, ENC_NA);
		offset += 2;
	} else {
		/* secondary request */
		/* 3 reserved bytes */
		proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 3, ENC_NA);
		offset += 3;
	}


	/* total param count */
	tp = tvb_get_letohl(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_total_param_count, tvb, offset, 4, tp);
	offset += 4;

	/* total data count */
	td = tvb_get_letohl(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_total_data_count, tvb, offset, 4, td);
	offset += 4;

	if (wc >= 19) {
		/* primary request */
		/* max param count */
		proto_tree_add_item(tree, hf_smb_max_param_count, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		offset += 4;

		/* max data count */
		proto_tree_add_item(tree, hf_smb_max_data_count, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		offset += 4;
	}

	/* param count */
	pc = tvb_get_letohl(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_param_count32, tvb, offset, 4, pc);
	offset += 4;

	/* param offset */
	po = tvb_get_letohl(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_param_offset32, tvb, offset, 4, po);
	offset += 4;

	/* param displacement */
	if (wc >= 19) {
		/* primary request*/
	} else {
		/* secondary request */

		proto_tree_add_item(tree, hf_smb_param_disp32, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		offset += 4;
	}

	/* data count */
	dc = tvb_get_letohl(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_data_count32, tvb, offset, 4, dc);
	offset += 4;

	/* data offset */
	od = tvb_get_letohl(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_data_offset32, tvb, offset, 4, od);
	offset += 4;

	/* data displacement */
	if (wc >= 19) {
		/* primary request */
	} else {
		/* secondary request */
		dd = tvb_get_letohl(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_data_disp32, tvb, offset, 4, dd);
		offset += 4;
	}

	/* setup count */
	if (wc >= 19) {
		/* primary request */
		sc = tvb_get_guint8(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_setup_count, tvb, offset, 1, sc);
		offset += 1;
	} else {
		/* secondary request */
		sc = 0;
	}

	/* function */
	if (wc >= 19) {
		/* primary request */
		subcmd = tvb_get_letohs(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_nt_trans_subcmd, tvb, offset, 2, subcmd);
		col_append_fstr(pinfo->cinfo, COL_INFO, ", %s",
				val_to_str_ext_const(subcmd, &nt_cmd_vals_ext, "<unknown>"));

		ntd.subcmd = subcmd;
		if (!si->unidir && sip) {
			if (!pinfo->fd->flags.visited) {
				/*
				 * Allocate a new smb_nt_transact_info_t
				 * structure.
				 */
				nti = wmem_new(wmem_file_scope(), smb_nt_transact_info_t);
				nti->subcmd = subcmd;
				nti->fid_type = SMB_FID_TYPE_UNKNOWN;
				nti->ioctl_function = 0;
				sip->extra_info = nti;
				sip->extra_info_type = SMB_EI_NTI;
			} else {
				if (sip->extra_info_type == SMB_EI_NTI) {
					nti = (smb_nt_transact_info_t *)sip->extra_info;
				}
			}
		}
	} else {
		/* secondary request */
		col_append_str(pinfo->cinfo, COL_INFO, " (secondary request)");
	}
	offset += 2;

#if 0	/* XXX this is a padding byte?  I don't think so. -gwr */
	if (offset&1) {
		/* pad byte */
	        proto_tree_add_item(tree, hf_smb_padding, tvb, offset, 1, ENC_NA);
		offset += 1;
	}
#endif

	/* if there were any setup bytes, decode them */
	if (sc) {
		dissect_nt_trans_setup_request(tvb, pinfo, offset, tree, sc*2, &ntd, si);
		offset += sc*2;
	}

	/*
	 * Do we really need to even look at the byte count here?
	 * Servers normally use byte_count only when assembling the
	 * setup, parameters, and data segments.  Once we know
	 * how long each of those are, we should dissect them
	 * using the lengths determined during assembly.
	 */
	BYTE_COUNT;

	/* reassembly of SMB NT Transaction data payload.
	   In this section we do reassembly of both the data and parameters
	   blocks of the SMB transaction command.
	*/
	/* do we need reassembly? */
	if ( (td && (td != dc)) || (tp && (tp != pc)) ) {
		/* oh yeah, either data or parameter section needs
		   reassembly...
		*/
		pinfo->fragmented = TRUE;
		if (smb_trans_reassembly) {
			/* ...and we were told to do reassembly */
			if (pc) {
				r_fd = smb_trans_defragment(tree, pinfo, tvb,
							     po, pc, pd, td+tp, si);
			}
			if ((r_fd == NULL) && dc) {
				r_fd = smb_trans_defragment(tree, pinfo, tvb,
							     od, dc, dd+tp, td+tp, si);
			}
		}
	}

	/* if we got a reassembled fd structure from the reassembly routine we
	   must create pd_tvb from it
	*/
	if (r_fd) {
		proto_item *frag_tree_item;

		pd_tvb = tvb_new_chain(tvb, r_fd->tvb_data);
		add_new_data_source(pinfo, pd_tvb, "Reassembled SMB");

		show_fragment_tree(r_fd, &smb_frag_items, tree, pinfo, pd_tvb, &frag_tree_item);
	}

	if (pd_tvb) {
	  /* we have reassembled data, grab param and data from there */
	  dissect_nt_trans_param_request(pd_tvb, pinfo, 0, tree, tp,
					  &ntd, (guint16) tvb_reported_length(pd_tvb), nti, si);
	  dissect_nt_trans_data_request(pd_tvb, pinfo, tp, tree, td, &ntd, nti, si);
	  COUNT_BYTES(bc); /* We are done */
	} else {
	  /* we do not have reassembled data, just use what we have in the
	     packet as well as we can */
	  /* parameters */
	  if (po > (guint32)offset) {
		/* We have some initial padding bytes.
		*/
		padcnt = po-offset;
		if (padcnt > bc)
			padcnt = bc;
		CHECK_BYTE_COUNT(padcnt);
	        proto_tree_add_item(tree, hf_smb_padding, tvb, offset, padcnt, ENC_NA);
		COUNT_BYTES(padcnt);
	  }
	  if (pc) {
		CHECK_BYTE_COUNT(pc);
		dissect_nt_trans_param_request(tvb, pinfo, offset, tree, pc, &ntd, bc, nti, si);
		COUNT_BYTES(pc);
	  }

	  /* data */
	  if (od > (guint32)offset) {
		/* We have some initial padding bytes.
		*/
		padcnt = od-offset;
		if (padcnt > bc)
			padcnt = bc;
	        proto_tree_add_item(tree, hf_smb_padding, tvb, offset, padcnt, ENC_NA);
		COUNT_BYTES(padcnt);
	  }
	  if (dc) {
		CHECK_BYTE_COUNT(dc);
		dissect_nt_trans_data_request(
			tvb, pinfo, offset, tree, dc, &ntd, nti, si);
		COUNT_BYTES(dc);
	  }
	}

	END_OF_SMB

	pinfo->fragmented = save_fragmented;
	return offset;
}



static int
dissect_nt_trans_data_response(tvbuff_t *tvb, packet_info *pinfo,
			       int offset, proto_tree *parent_tree, int len,
			       nt_trans_data *ntd _U_,
			       smb_nt_transact_info_t *nti, smb_info_t *si)
{
	proto_tree              *tree = NULL;
	guint16                  bcp;
	struct access_mask_info *ami  = NULL;
	tvbuff_t                *ioctl_tvb;

	DISSECTOR_ASSERT(si);

	if (parent_tree) {
		if (nti != NULL) {
			tree = proto_tree_add_subtree_format(parent_tree, tvb, offset, len,
				ett_smb_nt_trans_data, NULL, "%s Data",
				val_to_str_ext(nti->subcmd, &nt_cmd_vals_ext, "Unknown NT Transaction (%u)"));
		} else {
			/*
			 * We never saw the request to which this is a
			 * response.
			 */
			tree = proto_tree_add_subtree(parent_tree, tvb, offset, len,
				ett_smb_nt_trans_data, NULL, "Unknown NT Transaction Data (matching request not seen)");
		}
	}

	if (nti == NULL) {
		offset += len;
		return offset;
	}
	switch(nti->subcmd) {
	case NT_TRANS_CREATE:
		break;
	case NT_TRANS_IOCTL:
		/* ioctl data */
		ioctl_tvb = tvb_new_subset(tvb, offset, MIN((int)len, tvb_reported_length_remaining(tvb, offset)), len);
		dissect_smb2_ioctl_data(ioctl_tvb, pinfo, tree, top_tree_global, nti->ioctl_function, FALSE, NULL);

		offset += len;

		break;
	case NT_TRANS_SSD:
		break;
	case NT_TRANS_NOTIFY:
		break;
	case NT_TRANS_RENAME:
		/* XXX not documented */
		break;
	case NT_TRANS_QSD:
		if (nti) {
			switch(nti->fid_type) {
			case SMB_FID_TYPE_FILE:
				ami = &smb_file_access_mask_info;
				break;
			case SMB_FID_TYPE_DIR:
				ami = &smb_dir_access_mask_info;
				break;
			}
		}
		offset = dissect_nt_sec_desc(
			tvb, offset, pinfo, tree, NULL, TRUE, len, ami);
		break;
	case NT_TRANS_GET_USER_QUOTA:
		bcp = len;
		offset = dissect_nt_user_quota(tvb, tree, offset, &bcp);
		break;
	case NT_TRANS_SET_USER_QUOTA:
		/* not decoded yet */
		break;
	}

	return offset;
}

static int
dissect_nt_trans_param_response(tvbuff_t *tvb, packet_info *pinfo,
				int offset, proto_tree *parent_tree,
				int len, nt_trans_data *ntd _U_, guint16 bc, smb_info_t *si)
{
	proto_tree             *tree     = NULL;
	guint32                 fn_len;
	const char             *fn;
	smb_nt_transact_info_t *nti;
	guint16                 fid;
	int                     old_offset;
	guint32                 neo;
	int                     padcnt;
	smb_fid_info_t         *fid_info = NULL;
	guint16                 ftype;
	guint8                  isdir;
	guint8                  ext_resp = 0;

	DISSECTOR_ASSERT(si);

	if ((si->sip != NULL) && (si->sip->extra_info_type == SMB_EI_NTI))
		nti = (smb_nt_transact_info_t *)si->sip->extra_info;
	else
		nti = NULL;

	if (parent_tree) {
		if (nti != NULL) {
			tree = proto_tree_add_subtree_format(parent_tree, tvb, offset, len,
				ett_smb_nt_trans_param, NULL, "%s Parameters",
				val_to_str_ext(nti->subcmd, &nt_cmd_vals_ext, "Unknown NT Transaction (%u)"));
		} else {
			/*
			 * We never saw the request to which this is a
			 * response.
			 */
			tree = proto_tree_add_subtree(parent_tree, tvb, offset, len,
				ett_smb_nt_trans_param, NULL, "Unknown NT Transaction Parameters (matching request not seen)");
		}
	}

	if (nti == NULL) {
		offset += len;
		return offset;
	}
	switch(nti->subcmd) {
	case NT_TRANS_CREATE:
		/* oplock level */
	        proto_tree_add_item(tree, hf_smb_oplock_level, tvb, offset, 1, ENC_LITTLE_ENDIAN);
		offset += 1;

		/* reserved byte */
		ext_resp = tvb_get_guint8(tvb, offset);
	        proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 1, ENC_NA);
		offset += 1;

		/* fid */
		fid = tvb_get_letohs(tvb, offset);
		fid_info = dissect_smb_fid(tvb, pinfo, tree, offset, 2, fid, TRUE, FALSE, FALSE, si);
		offset += 2;

		/* create action */
		proto_tree_add_item(tree, hf_smb_create_action, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		offset += 4;

		/* ea error offset */
		proto_tree_add_item(tree, hf_smb_ea_error_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		offset += 4;

		/* create time */
		offset = dissect_nt_64bit_time(tvb, tree, offset,
			hf_smb_create_time);

		/* access time */
		offset = dissect_nt_64bit_time(tvb, tree, offset,
			hf_smb_access_time);

		/* last write time */
		offset = dissect_nt_64bit_time(tvb, tree, offset,
			hf_smb_last_write_time);

		/* last change time */
		offset = dissect_nt_64bit_time(tvb, tree, offset,
			hf_smb_change_time);

		/* Extended File Attributes */
		offset = dissect_file_ext_attr(tvb, tree, offset);

		/* allocation size */
		proto_tree_add_item(tree, hf_smb_alloc_size64, tvb, offset, 8, ENC_LITTLE_ENDIAN);
		offset += 8;

		/* end of file */
		proto_tree_add_item(tree, hf_smb_end_of_file, tvb, offset, 8, ENC_LITTLE_ENDIAN);
		offset += 8;

		/* File Type */
		ftype = tvb_get_letohs(tvb, offset);
		proto_tree_add_item(tree, hf_smb_file_type, tvb, offset, 2, ENC_LITTLE_ENDIAN);
		offset += 2;

		/* device state */
		offset = dissect_ipc_state(tvb, tree, offset, FALSE);

		/* is directory */
		isdir = tvb_get_guint8(tvb, offset);
		proto_tree_add_item(tree, hf_smb_is_directory, tvb, offset, 1, ENC_LITTLE_ENDIAN);
		offset += 1;

		/* decode extended response per [MS-SMB] 2.2.7.1.2
		   (volume_guid, file_id, max_acc, guest_acc)
		   Just like dissect_nt_create_andx_response */
		if (ext_resp != 0) {
			proto_tree *tr = NULL;

			/* The first field is a Volume GUID ... */
			proto_tree_add_item(tree, hf_smb_volume_guid,
					    tvb, offset, 16, ENC_NA);
			offset += 16;

			/* The file ID comes next */
			proto_tree_add_item(tree, hf_smb_file_id_64bit,
					    tvb, offset, 8, ENC_LITTLE_ENDIAN);
			offset += 8;

			tr = proto_tree_add_subtree(tree, tvb, offset, 4,
				ett_smb_nt_access_mask, NULL, "Maximal Access Rights");
			offset = dissect_smb_access_mask(tvb, tr, offset);

			tr = proto_tree_add_subtree(tree, tvb, offset, 4,
				ett_smb_nt_access_mask, NULL, "Guest Maximal Access Rights");
			offset = dissect_smb_access_mask(tvb, tr, offset);
		}

		/* Try to remember the type of this fid so that we can dissect
		 * any future security descriptor (access mask) properly
		 */
		if (ftype == 0) {
			if (isdir == 0) {
				if (fid_info) {
					fid_info->type = SMB_FID_TYPE_FILE;
				}
			} else {
				if (fid_info) {
					fid_info->type = SMB_FID_TYPE_DIR;
				}
			}
		}
		if (ftype == 2) {
			if (fid_info) {
				fid_info->type = SMB_FID_TYPE_PIPE;
			}
		}
		break;
	case NT_TRANS_IOCTL:
		break;
	case NT_TRANS_SSD:
		break;
	case NT_TRANS_NOTIFY:
		while (len) {
			old_offset = offset;

			/* next entry offset */
			neo = tvb_get_letohl(tvb, offset);
			proto_tree_add_uint(tree, hf_smb_next_entry_offset, tvb, offset, 4, neo);
			COUNT_BYTES(4);
			len -= 4;
			/* broken implementations */
			if (len < 0) break;

			/* action */
			proto_tree_add_item(tree, hf_smb_nt_notify_action, tvb, offset, 4, ENC_LITTLE_ENDIAN);
			COUNT_BYTES(4);
			len -= 4;
			/* broken implementations */
			if (len < 0) break;

			/* file name len */
			fn_len = (guint32)tvb_get_letohl(tvb, offset);
			proto_tree_add_uint(tree, hf_smb_file_name_len, tvb, offset, 4, fn_len);
			COUNT_BYTES(4);
			len -= 4;
			/* broken implementations */
			if (len < 0) break;

			/* file name */
			fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len, TRUE, TRUE, &bc);
			if (fn == NULL)
				break;
			proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
				fn);
			COUNT_BYTES(fn_len);
			len -= fn_len;
			/* broken implementations */
			if (len < 0) break;

			if (neo == 0)
				break;	/* no more structures */

			/* skip to next structure */
			padcnt = (old_offset + neo) - offset;
			if (padcnt < 0) {
				/*
				 * XXX - this is bogus; flag it?
				 */
				padcnt = 0;
			}
			if (padcnt != 0) {
				COUNT_BYTES(padcnt);
				len -= padcnt;
				/* broken implementations */
				if (len < 0) break;
			}
		}
		break;
	case NT_TRANS_RENAME:
		/* XXX not documented */
		break;
	case NT_TRANS_QSD:
		/*
		 * This appears to be the size of the security
		 * descriptor; the calling sequence of
		 * "ZwQuerySecurityObject()" suggests that it would
		 * be.  The actual security descriptor wouldn't
		 * follow if the max data count in the request
		 * was smaller; this lets the client know how
		 * big a buffer it needs to provide.
		 */
		proto_tree_add_item(tree, hf_smb_sec_desc_len, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		offset += 4;
		break;
	case NT_TRANS_GET_USER_QUOTA:
		proto_tree_add_item(tree, hf_smb_size_returned_quota_data, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		offset += 4;
		break;
	case NT_TRANS_SET_USER_QUOTA:
		/* not decoded yet */
		break;
	}

	return offset;
}

static int
dissect_nt_trans_setup_response(tvbuff_t *tvb, packet_info *pinfo,
				int offset, proto_tree *parent_tree,
				int len, nt_trans_data *ntd _U_, smb_info_t *si)
{
	smb_nt_transact_info_t *nti;

	DISSECTOR_ASSERT(si);

	if ((si->sip != NULL) && (si->sip->extra_info_type == SMB_EI_NTI))
		nti = (smb_nt_transact_info_t *)si->sip->extra_info;
	else
		nti = NULL;

	if (parent_tree) {
		if (nti != NULL) {
			proto_tree_add_bytes_format(parent_tree, hf_smb_nt_transaction_setup, tvb, offset, len,
				NULL, "%s Setup",
				val_to_str_ext(nti->subcmd, &nt_cmd_vals_ext, "Unknown NT Transaction (%u)"));
		} else {
			/*
			 * We never saw the request to which this is a
			 * response.
			 */
			proto_tree_add_expert(parent_tree, pinfo, &ei_smb_nt_transaction_setup, tvb, offset, len);
		}
	}

	if (nti == NULL) {
		offset += len;
		return offset;
	}
	switch(nti->subcmd) {
	case NT_TRANS_CREATE:
		break;
	case NT_TRANS_IOCTL:
		break;
	case NT_TRANS_SSD:
		break;
	case NT_TRANS_NOTIFY:
		break;
	case NT_TRANS_RENAME:
		/* XXX not documented */
		break;
	case NT_TRANS_QSD:
		break;
	case NT_TRANS_GET_USER_QUOTA:
		/* not decoded yet */
		break;
	case NT_TRANS_SET_USER_QUOTA:
		/* not decoded yet */
		break;
	}

	return offset;
}

static int
dissect_nt_transaction_response(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	guint8                  wc, sc;
	guint32                 pc     = 0, po = 0, pd = 0, dc = 0, od = 0, dd = 0;
	guint32                 td     = 0, tp = 0;
	smb_nt_transact_info_t *nti    = NULL;
	static nt_trans_data    ntd;
	guint16                 bc;
	gint32                  padcnt;
	fragment_head          *r_fd   = NULL;
	tvbuff_t               *pd_tvb = NULL;
	gboolean                save_fragmented;

	DISSECTOR_ASSERT(si);

	if ((si->sip != NULL) && (si->sip->extra_info_type == SMB_EI_NTI))
		nti = (smb_nt_transact_info_t *)si->sip->extra_info;
	else
		nti = NULL;

	/* primary request */
	if (nti != NULL) {
		proto_tree_add_uint(tree, hf_smb_nt_trans_subcmd, tvb, 0, 0, nti->subcmd);
		col_append_fstr(pinfo->cinfo, COL_INFO, ", %s",
				val_to_str_ext(nti->subcmd, &nt_cmd_vals_ext, "<unknown (%u)>"));
	} else {
		proto_tree_add_uint_format_value(tree, hf_smb_nt_trans_subcmd, tvb, offset, 0, -1,
			"<unknown function - could not find matching request>");
		col_append_str(pinfo->cinfo, COL_INFO, ", <unknown>");
	}

	WORD_COUNT;

	/* 3 reserved bytes */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 3, ENC_NA);
	offset += 3;

	/* total param count */
	tp = tvb_get_letohl(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_total_param_count, tvb, offset, 4, tp);
	offset += 4;

	/* total data count */
	td = tvb_get_letohl(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_total_data_count, tvb, offset, 4, td);
	offset += 4;

	/* param count */
	pc = tvb_get_letohl(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_param_count32, tvb, offset, 4, pc);
	offset += 4;

	/* param offset */
	po = tvb_get_letohl(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_param_offset32, tvb, offset, 4, po);
	offset += 4;

	/* param displacement */
	pd = tvb_get_letohl(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_param_disp32, tvb, offset, 4, pd);
	offset += 4;

	/* data count */
	dc = tvb_get_letohl(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_data_count32, tvb, offset, 4, dc);
	offset += 4;

	/* data offset */
	od = tvb_get_letohl(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_data_offset32, tvb, offset, 4, od);
	offset += 4;

	/* data displacement */
	dd = tvb_get_letohl(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_data_disp32, tvb, offset, 4, dd);
	offset += 4;

	/* setup count */
	sc = tvb_get_guint8(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_setup_count, tvb, offset, 1, sc);
	offset += 1;

	/* setup data */
	if (sc) {
		dissect_nt_trans_setup_response(tvb, pinfo, offset, tree, sc*2, &ntd, si);
		offset += sc*2;
	}

	BYTE_COUNT;

	/* reassembly of SMB NT Transaction data payload.
	   In this section we do reassembly of both the data and parameters
	   blocks of the SMB transaction command.
	*/
	save_fragmented = pinfo->fragmented;
	/* do we need reassembly? */
	if ( (td && (td != dc)) || (tp && (tp != pc)) ) {
		/* oh yeah, either data or parameter section needs
		   reassembly...
		*/
		pinfo->fragmented = TRUE;
		if (smb_trans_reassembly) {
			/* ...and we were told to do reassembly */
			if (pc) {
				r_fd = smb_trans_defragment(tree, pinfo, tvb,
							     po, pc, pd, td+tp, si);

			}
			if ((r_fd == NULL) && dc) {
				r_fd = smb_trans_defragment(tree, pinfo, tvb,
							     od, dc, dd+tp, td+tp, si);
			}
		}
	}

	/* if we got a reassembled fd structure from the reassembly routine we
	   must create pd_tvb from it
	*/
	if (r_fd) {
		proto_item *frag_tree_item;

		pd_tvb = tvb_new_chain(tvb, r_fd->tvb_data);
		add_new_data_source(pinfo, pd_tvb, "Reassembled SMB");

		show_fragment_tree(r_fd, &smb_frag_items, tree, pinfo, pd_tvb, &frag_tree_item);
	}


	if (pd_tvb) {
	  /* we have reassembled data, grab param and data from there */
	  dissect_nt_trans_param_response(pd_tvb, pinfo, 0, tree, tp,
					  &ntd, (guint16) tvb_reported_length(pd_tvb), si);
	  dissect_nt_trans_data_response(pd_tvb, pinfo, tp, tree, td, &ntd, nti, si);
	  COUNT_BYTES(bc); /* We are done */
	} else {
	  /* we do not have reassembled data, just use what we have in the
	     packet as well as we can */
	  /* parameters */
	  if (po > (guint32)offset) {
	    /* We have some initial padding bytes.
	     */
	    padcnt = po-offset;
	    if (padcnt > bc)
	      padcnt = bc;
	    CHECK_BYTE_COUNT(padcnt);
	    proto_tree_add_item(tree, hf_smb_padding, tvb, offset, padcnt, ENC_NA);
	    COUNT_BYTES(padcnt);
	  }
	  if (pc) {
	    CHECK_BYTE_COUNT(pc);
	    dissect_nt_trans_param_response(tvb, pinfo, offset, tree, pc, &ntd, bc, si);
	    COUNT_BYTES(pc);
	  }

	  /* data */
	  if (od > (guint32)offset) {
	    /* We have some initial padding bytes.
	     */
	    padcnt = od-offset;
	    if (padcnt > bc)
	      padcnt = bc;
	    proto_tree_add_item(tree, hf_smb_padding, tvb, offset, padcnt, ENC_NA);
	    COUNT_BYTES(padcnt);
	  }
	  if (dc) {
	    CHECK_BYTE_COUNT(dc);
	    dissect_nt_trans_data_response(tvb, pinfo, offset, tree, dc, &ntd, nti, si);
	    COUNT_BYTES(dc);
	  }
	}
	pinfo->fragmented = save_fragmented;

	END_OF_SMB

	return offset;
}

/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
   NT Transaction command  ends here
   XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */

static const value_string print_mode_vals[] = {
	{0,	"Text Mode"},
	{1,	"Graphics Mode"},
	{0, NULL}
};

static int
dissect_open_print_file_request(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	int         fn_len;
	const char *fn;
	guint8      wc;
	guint16     bc;

	DISSECTOR_ASSERT(si);

	WORD_COUNT;

	/* setup len */
	proto_tree_add_item(tree, hf_smb_setup_len, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	/* print mode */
	proto_tree_add_item(tree, hf_smb_print_mode, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	BYTE_COUNT;

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* print identifier */
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len, TRUE, FALSE, &bc);
	if (fn == NULL)
		goto endofcommand;
	proto_tree_add_string(tree, hf_smb_print_identifier, tvb, offset, fn_len,
		fn);
	COUNT_BYTES(fn_len);

	END_OF_SMB

	return offset;
}


static int
dissect_write_print_file_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	int     cnt;
	guint8  wc;
	guint16 bc, fid;

	WORD_COUNT;

	/* fid */
	fid = tvb_get_letohs(tvb, offset);
	dissect_smb_fid(tvb, pinfo, tree, offset, 2, fid, FALSE, FALSE, FALSE, si);
	offset += 2;

	BYTE_COUNT;

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* data len */
	CHECK_BYTE_COUNT(2);
	cnt = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_data_len, tvb, offset, 2, cnt);
	COUNT_BYTES(2);

	/* file data */
	offset = dissect_file_data(tvb, tree, offset, (guint16) cnt, (guint16) cnt);

	END_OF_SMB

	return offset;
}


static const value_string print_status_vals[] = {
	{1,	"Held or Stopped"},
	{2,	"Printing"},
	{3,	"Awaiting print"},
	{4,	"In intercept"},
	{5,	"File had error"},
	{6,	"Printer error"},
	{0, NULL}
};

static int
dissect_get_print_queue_request(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si _U_)
{
	guint8  wc;
	guint16 bc;

	WORD_COUNT;

	/* max count */
	proto_tree_add_item(tree, hf_smb_max_count, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	/* start index */
	proto_tree_add_item(tree, hf_smb_start_index, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	BYTE_COUNT;

	END_OF_SMB

	return offset;
}

static int
dissect_print_queue_element(tvbuff_t *tvb, packet_info *pinfo _U_,
    proto_tree *parent_tree, int offset, guint16 *bcp, gboolean *trunc, smb_info_t *si)
{
	proto_tree *tree;
	int         fn_len;
	const char *fn;

	DISSECTOR_ASSERT(si);

	tree = proto_tree_add_subtree(parent_tree, tvb, offset, 28,
			ett_smb_print_queue_entry, NULL, "Queue entry");

	/* queued time */
	CHECK_BYTE_COUNT_SUBR(4);
	offset = dissect_smb_datetime(tvb, tree, offset,
		hf_smb_print_queue_date,
		hf_smb_print_queue_dos_date, hf_smb_print_queue_dos_time, FALSE);
	*bcp -= 4;

	/* status */
	CHECK_BYTE_COUNT_SUBR(1);
	proto_tree_add_item(tree, hf_smb_print_status, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(1);

	/* spool file number */
	CHECK_BYTE_COUNT_SUBR(2);
	proto_tree_add_item(tree, hf_smb_print_spool_file_number, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(2);

	/* spool file size */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_print_spool_file_size, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* reserved byte */
	CHECK_BYTE_COUNT_SUBR(1);
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 1, ENC_NA);
	COUNT_BYTES_SUBR(1);

	/* file name */
	fn_len = 16;
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len, TRUE, TRUE, bcp);
	CHECK_STRING_SUBR(fn);
	proto_tree_add_string(tree, hf_smb_print_spool_file_name, tvb, offset, 16,
		fn);
	COUNT_BYTES_SUBR(fn_len);

	*trunc = FALSE;
	return offset;
}

static int
dissect_get_print_queue_response(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si _U_)
{
	guint16  cnt = 0, len;
	guint8   wc;
	guint16  bc;
	gboolean trunc;

	WORD_COUNT;

	/* count */
	cnt = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_count, tvb, offset, 2, cnt);
	offset += 2;

	/* restart index */
	proto_tree_add_item(tree, hf_smb_restart_index, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	BYTE_COUNT;

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* data len */
	CHECK_BYTE_COUNT(2);
	len = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_data_len, tvb, offset, 2, len);
	COUNT_BYTES(2);

	/* queue elements */
	while (cnt--) {
		offset = dissect_print_queue_element(tvb, pinfo, tree, offset,
		    &bc, &trunc, si);
		if (trunc)
			goto endofcommand;
	}

	END_OF_SMB

	return offset;
}


static int
dissect_send_single_block_message_request(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si _U_)
{
	int     name_len;
	guint16 bc;
	guint8  wc;
	guint16 message_len;

	WORD_COUNT;

	BYTE_COUNT;

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* originator name */
	/* XXX - what if this runs past bc? */
	name_len = tvb_strsize(tvb, offset);
	CHECK_BYTE_COUNT(name_len);
	proto_tree_add_item(tree, hf_smb_originator_name, tvb, offset,
	    name_len, ENC_ASCII|ENC_NA);
	COUNT_BYTES(name_len);

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* destination name */
	/* XXX - what if this runs past bc? */
	name_len = tvb_strsize(tvb, offset);
	CHECK_BYTE_COUNT(name_len);
	proto_tree_add_item(tree, hf_smb_destination_name, tvb, offset,
	    name_len, ENC_ASCII|ENC_NA);
	COUNT_BYTES(name_len);

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* message len */
	CHECK_BYTE_COUNT(2);
	message_len = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_message_len, tvb, offset, 2,
	    message_len);
	COUNT_BYTES(2);

	/* message */
	CHECK_BYTE_COUNT(message_len);
	proto_tree_add_item(tree, hf_smb_message, tvb, offset, message_len,
	    ENC_ASCII|ENC_NA);
	COUNT_BYTES(message_len);

	END_OF_SMB

	return offset;
}

static int
dissect_send_multi_block_message_start_request(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si _U_)
{
	int     name_len;
	guint16 bc;
	guint8  wc;

	WORD_COUNT;

	BYTE_COUNT;

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* originator name */
	/* XXX - what if this runs past bc? */
	name_len = tvb_strsize(tvb, offset);
	CHECK_BYTE_COUNT(name_len);
	proto_tree_add_item(tree, hf_smb_originator_name, tvb, offset,
	    name_len, ENC_ASCII|ENC_NA);
	COUNT_BYTES(name_len);

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* destination name */
	/* XXX - what if this runs past bc? */
	name_len = tvb_strsize(tvb, offset);
	CHECK_BYTE_COUNT(name_len);
	proto_tree_add_item(tree, hf_smb_destination_name, tvb, offset,
	    name_len, ENC_ASCII|ENC_NA);
	COUNT_BYTES(name_len);

	END_OF_SMB

	return offset;
}

static int
dissect_message_group_id(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si _U_)
{
	guint16 bc;
	guint8  wc;

	WORD_COUNT;

	/* message group ID */
	proto_tree_add_item(tree, hf_smb_mgid, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	BYTE_COUNT;

	END_OF_SMB

	return offset;
}

static int
dissect_send_multi_block_message_text_request(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si _U_)
{
	guint16 bc;
	guint8  wc;
	guint16 message_len;

	WORD_COUNT;

	BYTE_COUNT;

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* message len */
	CHECK_BYTE_COUNT(2);
	message_len = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_message_len, tvb, offset, 2,
	    message_len);
	COUNT_BYTES(2);

	/* message */
	CHECK_BYTE_COUNT(message_len);
	proto_tree_add_item(tree, hf_smb_message, tvb, offset, message_len,
	    ENC_ASCII|ENC_NA);
	COUNT_BYTES(message_len);

	END_OF_SMB

	return offset;
}

static int
dissect_forwarded_name(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si _U_)
{
	int     name_len;
	guint16 bc;
	guint8  wc;

	WORD_COUNT;

	BYTE_COUNT;

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* forwarded name */
	/* XXX - what if this runs past bc? */
	name_len = tvb_strsize(tvb, offset);
	CHECK_BYTE_COUNT(name_len);
	proto_tree_add_item(tree, hf_smb_forwarded_name, tvb, offset,
	    name_len, ENC_ASCII|ENC_NA);
	COUNT_BYTES(name_len);

	END_OF_SMB

	return offset;
}

static int
dissect_get_machine_name_response(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si _U_)
{
	int     name_len;
	guint16 bc;
	guint8  wc;

	WORD_COUNT;

	BYTE_COUNT;

	/* buffer format */
	CHECK_BYTE_COUNT(1);
	proto_tree_add_item(tree, hf_smb_buffer_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES(1);

	/* machine name */
	/* XXX - what if this runs past bc? */
	name_len = tvb_strsize(tvb, offset);
	CHECK_BYTE_COUNT(name_len);
	proto_tree_add_item(tree, hf_smb_machine_name, tvb, offset,
	    name_len, ENC_ASCII|ENC_NA);
	COUNT_BYTES(name_len);

	END_OF_SMB

	return offset;
}


static int
dissect_nt_create_andx_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree, smb_info_t *si _U_)
{
	guint8      wc, cmd      = 0xff;
	guint16     andxoffset   = 0;
	guint16     bc;
	int         fn_len;
	const char *fn;
	guint32     create_flags = 0, access_mask = 0, file_attributes = 0;
	guint32     share_access = 0, create_options = 0, create_disposition = 0;

	DISSECTOR_ASSERT(si);

	WORD_COUNT;

	/* next smb command */
	cmd = tvb_get_guint8(tvb, offset);
	if (cmd != 0xff) {
		proto_tree_add_uint(tree, hf_smb_andxcmd, tvb, offset, 1, cmd);
	} else {
		proto_tree_add_uint_format_value(tree, hf_smb_andxcmd, tvb, offset, 1, cmd, "No further commands (0xff)");
	}
	offset += 1;

	/* reserved byte */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 1, ENC_NA);
	offset += 1;

	/* andxoffset */
	andxoffset = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_andxoffset, tvb, offset, 2, andxoffset);
	offset += 2;

	/* reserved byte */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 1, ENC_NA);
	offset += 1;

	/* file name len */
	fn_len = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_file_name_len, tvb, offset, 2, fn_len);
	offset += 2;

	/* Create flags */
	create_flags = tvb_get_letohl(tvb, offset);
	offset = dissect_nt_create_bits(tvb, tree, offset, 4, create_flags);

	/* root directory fid */
	proto_tree_add_item(tree, hf_smb_root_dir_fid, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	offset += 4;

	/* nt access mask */
	access_mask = tvb_get_letohl(tvb, offset);
	offset = dissect_smb_access_mask_bits(tvb, tree, offset, 4, access_mask);

	/* allocation size */
	proto_tree_add_item(tree, hf_smb_alloc_size64, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	offset += 8;

	/* Extended File Attributes */
	file_attributes = tvb_get_letohl(tvb, offset);
	offset = dissect_file_ext_attr_bits(tvb, tree, offset, 4, file_attributes);

	/* share access */
	share_access = tvb_get_letohl(tvb, offset);
	offset = dissect_nt_share_access_bits(tvb, tree, offset, 4, share_access);

	/* create disposition */
	create_disposition = tvb_get_letohl(tvb, offset);
	proto_tree_add_item(tree, hf_smb_nt_create_disposition, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	offset += 4;

	/* create options */
	create_options = tvb_get_letohl(tvb, offset);
	offset = dissect_nt_create_options_bits(tvb, tree, offset, 4, create_options);

	/* impersonation level */
	proto_tree_add_item(tree, hf_smb_nt_impersonation_level, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	offset += 4;

	/* security flags */
	offset = dissect_nt_security_flags(tvb, tree, offset);

	BYTE_COUNT;

	/* file name */
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len, FALSE, FALSE, &bc);
	if (fn == NULL)
		goto endofcommand;
	proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
		fn);
	COUNT_BYTES(fn_len);

	/* store it for the fid->name/openframe/closeframe matching in
	 * dissect_smb_fid()   called from the response.
	 */
	if ((!pinfo->fd->flags.visited) && si->sip && fn) {
		smb_fid_saved_info_t *fsi;

		fsi			 = wmem_new(wmem_file_scope(), smb_fid_saved_info_t);
		fsi->filename		 = wmem_strdup(wmem_file_scope(), fn);
		fsi->create_flags	 = create_flags;
		fsi->access_mask	 = access_mask;
		fsi->file_attributes	 = file_attributes;
		fsi->share_access	 = share_access;
		fsi->create_options	 = create_options;
		fsi->create_disposition	 = create_disposition;

		si->sip->extra_info_type = SMB_EI_FILEDATA;
		si->sip->extra_info	 = fsi;
	}

	col_append_fstr(pinfo->cinfo, COL_INFO, ", Path: %s",
		    format_text(fn, strlen(fn)));

	END_OF_SMB

	if (cmd != 0xff) { 	/* there is an andX command */
		if (andxoffset < offset)
			THROW(ReportedBoundsError);
		dissect_smb_command(tvb, pinfo, andxoffset, smb_tree, cmd, FALSE, si);
	}

	return offset;
}


static int
dissect_nt_create_andx_response(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree, smb_info_t *si)
{
	guint8          wc, cmd    = 0xff;
	guint16         andxoffset = 0;
	guint16         bc;
	guint16         fid        = 0;
	guint16         ftype;
	guint8          isdir;
	smb_fid_info_t *fid_info   = NULL;

	WORD_COUNT;

	/* next smb command */
	cmd = tvb_get_guint8(tvb, offset);
	if (cmd != 0xff) {
		proto_tree_add_uint(tree, hf_smb_andxcmd, tvb, offset, 1, cmd);
	} else {
		proto_tree_add_uint_format_value(tree, hf_smb_andxcmd, tvb, offset, 1, cmd, "No further commands (0xff)");
	}
	offset += 1;

	/* reserved byte */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 1, ENC_NA);
	offset += 1;

	/* andxoffset */
	andxoffset = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_andxoffset, tvb, offset, 2, andxoffset);
	offset += 2;

	/* oplock level */
	proto_tree_add_item(tree, hf_smb_oplock_level, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	offset += 1;

	/* fid */
	fid = tvb_get_letohs(tvb, offset);
	fid_info = dissect_smb_fid(tvb, pinfo, tree, offset, 2, fid, TRUE, FALSE, FALSE, si);
	offset += 2;

	/* create action */
	/*XXX is this really the same as create disposition in the request? it looks so*/
	/* No, it is not. It is the same as the create action from an Open&X request ... RJS */
	proto_tree_add_item(tree, hf_smb_create_action, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	offset += 4;

	/* create time */
	offset = dissect_nt_64bit_time(tvb, tree, offset, hf_smb_create_time);

	/* access time */
	offset = dissect_nt_64bit_time(tvb, tree, offset, hf_smb_access_time);

	/* last write time */
	offset = dissect_nt_64bit_time(tvb, tree, offset,
		hf_smb_last_write_time);

	/* last change time */
	offset = dissect_nt_64bit_time(tvb, tree, offset, hf_smb_change_time);

	/* Extended File Attributes */
	offset = dissect_file_ext_attr(tvb, tree, offset);

	/* allocation size */
	proto_tree_add_item(tree, hf_smb_alloc_size64, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	offset += 8;

	/* end of file */
	/* We store the end of file */
	if (fid_info) {
		fid_info->end_of_file = tvb_get_letoh64(tvb, offset);
	}
	proto_tree_add_item(tree, hf_smb_end_of_file, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	offset += 8;

	/* File Type */
	ftype = tvb_get_letohs(tvb, offset);
	proto_tree_add_item(tree, hf_smb_file_type, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	/* IPC State */
	offset = dissect_ipc_state(tvb, tree, offset, FALSE);

	/* is directory */
	isdir = tvb_get_guint8(tvb, offset);
	proto_tree_add_item(tree, hf_smb_is_directory, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	offset += 1;

	/* Always use the word count to decide if this is an "extended" response.
	   When the server doesn't support the 0x10 flag, it will send a normal
	   34 word response, so the word count is the only way to tell which of
	   the response formats we have.  MS-SMB 2.2.4.9.2
	   Also note that the extended format is actually 50 words, but in a
	   "windows behavior note" they say Windows sets word count to 42.
	   Handle anything 42 or larger as "extended" format. */
	if (wc >= 42) {
		proto_tree *tr = NULL;

		/* The first field is a Volume GUID ... */
		proto_tree_add_item(tree, hf_smb_volume_guid,
				    tvb, offset, 16, ENC_NA);
		offset += 16;

		/* The file ID comes next */
		proto_tree_add_item(tree, hf_smb_file_id_64bit,
				    tvb, offset, 8, ENC_LITTLE_ENDIAN);
		offset += 8;

		tr = proto_tree_add_subtree(tree, tvb, offset, 4,
					  ett_smb_nt_access_mask, NULL, "Maximal Access Rights");

		offset = dissect_smb_access_mask(tvb, tr, offset);

		tr = proto_tree_add_subtree(tree, tvb, offset, 4,
					   ett_smb_nt_access_mask, NULL, "Guest Maximal Access Rights");

		offset = dissect_smb_access_mask(tvb, tr, offset);
	}

	/* Try to remember the type of this fid so that we can dissect
	 * any future security descriptor (access mask) properly
	 */
	if (ftype == 0) {
		if (isdir == 0) {
			if (fid_info) {
				fid_info->type = SMB_FID_TYPE_FILE;
			}
		} else {
			if (fid_info) {
				fid_info->type = SMB_FID_TYPE_DIR;
			}
		}
	}
	if (ftype == 2) {
		if (fid_info) {
			fid_info->type = SMB_FID_TYPE_PIPE;
		}
	}

	BYTE_COUNT;

	END_OF_SMB

	if (cmd != 0xff) { 	/* there is an andX command */
		if (andxoffset < offset)
			THROW(ReportedBoundsError);
		dissect_smb_command(tvb, pinfo, andxoffset, smb_tree, cmd, FALSE, si);
	}

	/* if there was an error, add a generated filename to the tree */
	if (si->nt_status) {
		dissect_smb_fid(tvb, pinfo, tree, 0, 0, fid, TRUE, TRUE, TRUE, si);
	}

	return offset;
}


static int
dissect_nt_cancel_request(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si _U_)
{
	guint8  wc;
	guint16 bc;

	WORD_COUNT;

	BYTE_COUNT;

	END_OF_SMB

	return offset;
}

/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
   BEGIN Transaction/Transaction2 Primary and secondary requests
   XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */


static const value_string trans2_cmd_vals[] = {
	{ 0x00,		"OPEN2" },
	{ 0x01,		"FIND_FIRST2" },
	{ 0x02,		"FIND_NEXT2" },
	{ 0x03,		"QUERY_FS_INFO" },
	{ 0x04,		"SET_FS_INFO" },
	{ 0x05,		"QUERY_PATH_INFO" },
	{ 0x06,		"SET_PATH_INFO" },
	{ 0x07,		"QUERY_FILE_INFO" },
	{ 0x08,		"SET_FILE_INFO" },
	{ 0x09,		"FSCTL" },
	{ 0x0A,		"IOCTL2" },
	{ 0x0B,		"FIND_NOTIFY_FIRST" },
	{ 0x0C,		"FIND_NOTIFY_NEXT" },
	{ 0x0D,		"CREATE_DIRECTORY" },
	{ 0x0E,		"SESSION_SETUP" },
	{ 0x0F,         "Unknown (0x0f)" },  /* dummy so val_to_str_ext can do indexed lookup */
	{ 0x10,		"GET_DFS_REFERRAL" },
	{ 0x11,		"REPORT_DFS_INCONSISTENCY" },
	{ 0,    NULL }
};
value_string_ext trans2_cmd_vals_ext = VALUE_STRING_EXT_INIT(trans2_cmd_vals);

static const true_false_string tfs_tf_dtid = {
	"Also DISCONNECT TID",
	"Do NOT disconnect TID"
};
static const true_false_string tfs_tf_owt = {
	"One Way Transaction (NO RESPONSE)",
	"Two way transaction"
};

static const true_false_string tfs_ff2_backup = {
	"Find WITH backup intent",
	"No backup intent"
};
static const true_false_string tfs_ff2_continue = {
	"CONTINUE search from previous position",
	"New search, do NOT continue from previous position"
};
static const true_false_string tfs_ff2_resume = {
	"Return RESUME keys",
	"Do NOT return resume keys"
};
static const true_false_string tfs_ff2_close_eos = {
	"CLOSE search if END OF SEARCH is reached",
	"Do NOT close search if end of search reached"
};
static const true_false_string tfs_ff2_close = {
	"CLOSE search after this request",
	"Do NOT close search after this request"
};

/* used by
   TRANS2_FIND_FIRST2
*/
static const value_string ff2_il_vals[] = {
	{ 1,		"Info Standard"},
	{ 2,		"Info Query EA Size"},
	{ 3,		"Info Query EAs From List"},
	{ 0x0101,	"Find File Directory Info"},
	{ 0x0102,	"Find File Full Directory Info"},
	{ 0x0103,	"Find File Names Info"},
	{ 0x0104,	"Find File Both Directory Info"},
	{ 0x0105,	"Find File Full Directory Info"},
	{ 0x0106,	"Find File Id Both Directory Info"},
	{ 0x0202,	"Find File Unix"},
	{ 0x020B,	"Find File Unix Info2"},
	{0, NULL}
};

/* values used by :
	TRANS2_QUERY_PATH_INFORMATION
	TRANS2_QUERY_FILE_INFORMATION
*/
static const value_string qpi_loi_vals[] = {
	{ 1,		"Info Standard"},
	{ 2,		"Info Query EA Size"},
	{ 3,		"Info Query EAs From List"},
	{ 4,		"Info Query All EAs"},
	{ 6,		"Info Is Name Valid"},
	{ 0x0101,	"Query File Basic Info"},
	{ 0x0102,	"Query File Standard Info"},
	{ 0x0103,	"Query File EA Info"},
	{ 0x0104,	"Query File Name Info"},
	{ 0x0107,	"Query File All Info"},
	{ 0x0108,	"Query File Alt Name Info"},
	{ 0x0109,	"Query File Stream Info"},
	{ 0x010b,	"Query File Compression Info"},
	{ 0x0200,	"Query File Unix Basic"},
	{ 0x0201,	"Query File Unix Link"},
	{ 0x0202,	"Query File Unix Hardlink"},
	{ 0x0204,	"Query File Posix ACL"},
	{ 0x0205,	"Query File Posix XATTR"},
	{ 0x0206,	"Query File Posix Attr Flags"},
	{ 0x0207,	"Query File Posix Permissions"},
	{ 0x0208,	"Query File Posix Lock"},
	{ 0x020b,	"Query File Unix Info2"},
	{ 1004,		"Query File Basic Info"},
	{ 1005,		"Query File Standard Info"},
	{ 1006,		"Query File Internal Info"},
	{ 1007,		"Query File EA Info"},
	{ 1009,		"Query File Name Info"},
	{ 1010,		"Query File Rename Info"},
	{ 1011,		"Query File Link Info"},
	{ 1012,		"Query File Names Info"},
	{ 1013,		"Query File Disposition Info"},
	{ 1014,		"Query File Position Info"},
	{ 1015,		"Query File Full EA Info"},
	{ 1016,		"Query File Mode Info"},
	{ 1017,		"Query File Alignment Info"},
	{ 1018,		"Query File All Info"},
	{ 1019,		"Query File Allocation Info"},
	{ 1020,		"Query File End of File Info"},
	{ 1021,		"Query File Alt Name Info"},
	{ 1022,		"Query File Stream Info"},
	{ 1023,		"Query File Pipe Info"},
	{ 1024,		"Query File Pipe Local Info"},
	{ 1025,		"Query File Pipe Remote Info"},
	{ 1026,		"Query File Mailslot Query Info"},
	{ 1027,		"Query File Mailslot Set Info"},
	{ 1028,		"Query File Compression Info"},
	{ 1029,		"Query File ObjectID Info"},
	{ 1030,		"Query File Completion Info"},
	{ 1031,		"Query File Move Cluster Info"},
	{ 1032,		"Query File Quota Info"},
	{ 1033,		"Query File Reparsepoint Info"},
	{ 1034,		"Query File Network Open Info"},
	{ 1035,		"Query File Attribute Tag Info"},
	{ 1036,		"Query File Tracking Info"},
	{ 1037,		"Query File Maximum Info"},
	{0, NULL}
};
static value_string_ext qpi_loi_vals_ext = VALUE_STRING_EXT_INIT(qpi_loi_vals);

/* values used by :
	TRANS2_SET_PATH_INFORMATION
	TRANS2_SET_FILE_INFORMATION
	(the SNIA CIFS spec lists some only for TRANS2_SET_FILE_INFORMATION,
	but I'm assuming they apply to TRANS2_SET_PATH_INFORMATION as
	well; note that they're different from the QUERY_PATH_INFORMATION
	and QUERY_FILE_INFORMATION values!)
*/
static const value_string spi_loi_vals[] = {
	{ 1,		"Info Standard"},
	{ 2,		"Info Set EAs"},
	{ 4,		"Info Query All EAs"},
	{ 0x0101,	"Set File Basic Info"},
	{ 0x0102,	"Set File Disposition Info"},
	{ 0x0103,	"Set File Allocation Info"},
	{ 0x0104,	"Set File End Of File Info"},
	{ 0x0200,	"Set File Unix Basic"},
	{ 0x0201,	"Set File Unix Link"},
	{ 0x0202,	"Set File Unix HardLink"},
	{ 0x0204,	"Set File Unix ACL"},
	{ 0x0205,	"Set File Unix XATTR"},
	{ 0x0206,	"Set File Unix Attr Flags"},
	{ 0x0208,	"Set File Posix Lock"},
	{ 0x0209,	"Set File Posix Open"},
	{ 0x020a,	"Set File Posix Unlink"},
	{ 0x020b,	"Set File Unix Info2"},
	{ 1004,         "Set File Basic Info"},
	{ 1010,         "Set Rename Information"},
	{ 1013,         "Set Disposition Information"},
	{ 1014,         "Set Position Information"},
	{ 1016,         "Set Mode Information"},
	{ 1019,         "Set Allocation Information"},
	{ 1020,         "Set EOF Information"},
	{ 1023,         "Set File Pipe Information"},
	{ 1025,         "Set File Pipe Remote Information"},
	{ 1029,         "Set Copy On Write Information"},
	{ 1032,         "Set OLE Class ID Information"},
	{ 1039,         "Set Inherit Context Index Information"},
	{ 1040,         "Set OLE Information (?)"},
	{0, NULL}
};
static value_string_ext spi_loi_vals_ext = VALUE_STRING_EXT_INIT(spi_loi_vals);

static const value_string qfsi_vals[] = {
	{ 1,		"Info Allocation"},
	{ 2,		"Info Volume"},
	{ 0x0101,	"Query FS Label Info"},
	{ 0x0102,	"Query FS Volume Info"},
	{ 0x0103,	"Query FS Size Info"},
	{ 0x0104,	"Query FS Device Info"},
	{ 0x0105,	"Query FS Attribute Info"},
	{ 0x0200,       "Unix Query FS Info"},
	{ 0x0202,       "Unix Query POSIX whoami"},
	{ 0x0301,	"Mac Query FS Info"},
	{ 1001,		"Query FS Label Info"},
	{ 1002,		"Query FS Volume Info"},
	{ 1003,		"Query FS Size Info"},
	{ 1004,		"Query FS Device Info"},
	{ 1005,		"Query FS Attribute Info"},
	{ 1006,		"Query FS Quota Info"},
	{ 1007,		"Query Full FS Size Info"},
	{ 1008,         "Object ID Information"},
	{0, NULL}
};
static value_string_ext qfsi_vals_ext = VALUE_STRING_EXT_INIT(qfsi_vals);

static const value_string sfsi_vals[] = {
	{ 0x203,	"Request Transport Encryption"},
	{ 1006,		"Set FS Quota Info"},
	{0, NULL}
};

static const value_string nt_rename_vals[] = {
	{ 0x0103,	"Create Hard Link"},
	{0, NULL}
};


static const value_string delete_pending_vals[] = {
	{0,	"Normal, no pending delete"},
	{1,	"This object has DELETE PENDING"},
	{0, NULL}
};

static const value_string alignment_vals[] = {
	{0,	"Byte alignment"},
	{1,	"Word (16bit) alignment"},
	{3,	"Long (32bit) alignment"},
	{7,	"8 byte boundary alignment"},
	{0x0f,	"16 byte boundary alignment"},
	{0x1f,	"32 byte boundary alignment"},
	{0x3f,	"64 byte boundary alignment"},
	{0x7f,	"128 byte boundary alignment"},
	{0xff,	"256 byte boundary alignment"},
	{0x1ff,	"512 byte boundary alignment"},
	{0, NULL}
};

static const true_false_string tfs_marked_for_deletion = {
	"File is MARKED FOR DELETION",
	"File is NOT marked for deletion"
};

static const true_false_string tfs_get_dfs_server_hold_storage = {
	"Referral SERVER HOLDS STORAGE for the file",
	"Referral server does NOT hold storage for the file"
};
static const true_false_string tfs_get_dfs_fielding = {
	"The server in referral is FIELDING CAPABLE",
	"The server in referrals is NOT fielding capable"
};

static const true_false_string tfs_dfs_referral_flags_name_list_referral = {
	"A domain/DC referral response",
	"NOT a domain/DC referral response"
};

static const true_false_string tfs_dfs_referral_flags_target_set_boundary = {
	"The first target in the target set",
	"NOT the first target in the target set"
};

static const value_string dfs_referral_server_type_vals[] = {
	{0,	"Non-root targets returned"},
	{1,	"Root targets returns"},
	{0, NULL}
};


static const true_false_string tfs_device_char_removable = {
	"This is a REMOVABLE device",
	"This is NOT a removable device"
};
static const true_false_string tfs_device_char_read_only = {
	"This is a READ-ONLY device",
	"This is NOT a read-only device"
};
static const true_false_string tfs_device_char_floppy = {
	"This is a FLOPPY DISK device",
	"This is NOT a floppy disk device"
};
static const true_false_string tfs_device_char_write_once = {
	"This is a WRITE-ONCE device",
	"This is NOT a write-once device"
};
static const true_false_string tfs_device_char_remote = {
	"This is a REMOTE device",
	"This is NOT a remote device"
};
static const true_false_string tfs_device_char_mounted = {
	"This device is MOUNTED",
	"This device is NOT mounted"
};
static const true_false_string tfs_device_char_virtual = {
	"This is a VIRTUAL device",
	"This is NOT a virtual device"
};


static const true_false_string tfs_fs_attr_css = {
	"This FS supports CASE SENSITIVE SEARCHes",
	"This FS does NOT support case sensitive searches"
};
static const true_false_string tfs_fs_attr_cpn = {
	"This FS supports CASE PRESERVED NAMES",
	"This FS does NOT support case preserved names"
};
static const true_false_string tfs_fs_attr_uod = {
	"This FS supports UNICODE NAMES",
	"This FS does NOT support unicode names"
};
static const true_false_string tfs_fs_attr_pacls = {
	"This FS supports PERSISTENT ACLs",
	"This FS does NOT support persistent acls"
};
static const true_false_string tfs_fs_attr_fc = {
	"This FS supports COMPRESSED FILES",
	"This FS does NOT support compressed files"
};
static const true_false_string tfs_fs_attr_vq = {
	"This FS supports VOLUME QUOTAS",
	"This FS does NOT support volume quotas"
};
static const true_false_string tfs_fs_attr_srp = {
	"This FS supports REPARSE POINTS",
	"This FS does NOT support reparse points"
};
static const true_false_string tfs_fs_attr_srs = {
	"This FS supports REMOTE STORAGE",
	"This FS does NOT support remote storage"
};
static const true_false_string tfs_fs_attr_ssf = {
	"This FS supports SPARSE FILES",
	"This FS does NOT support sparse files"
};
static const true_false_string tfs_fs_attr_sla = {
	"This FS supports LFN APIs",
	"This FS does NOT support lfn apis"
};
static const true_false_string tfs_fs_attr_vic = {
	"This FS VOLUME IS COMPRESSED",
	"This FS volume is NOT compressed"
};
static const true_false_string tfs_fs_attr_soids = {
	"This FS supports OIDs",
	"This FS does NOT support OIDs"
};
static const true_false_string tfs_fs_attr_se = {
	"This FS supports ENCRYPTION",
	"This FS does NOT support encryption"
};
static const true_false_string tfs_fs_attr_ns = {
	"This FS supports NAMED STREAMS",
	"This FS does NOT support named streams"
};
static const true_false_string tfs_fs_attr_rov = {
	"This is a READ ONLY VOLUME",
	"This is a read/write volume"
};

#define FF2_RESUME	0x0004

static int
dissect_ff2_flags(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree, int offset, smb_info_t *si)
{
	guint16               mask;
	smb_transact2_info_t *t2i;
	static const int * flags[] = {
		&hf_smb_ff2_backup,
		&hf_smb_ff2_continue,
		&hf_smb_ff2_resume,
		&hf_smb_ff2_close_eos,
		&hf_smb_ff2_close,
		NULL
	};

	mask = tvb_get_letohs(tvb, offset);

	DISSECTOR_ASSERT(si);

	if ((si->sip != NULL) && (si->sip->extra_info_type == SMB_EI_T2I)) {
		t2i = (smb_transact2_info_t *)si->sip->extra_info;
		if (t2i != NULL) {
			if (!pinfo->fd->flags.visited)
				t2i->resume_keys = (mask & FF2_RESUME);
		}
	}

	proto_tree_add_bitmask(parent_tree, tvb, offset, hf_smb_ff2, ett_smb_find_first2_flags, flags, ENC_LITTLE_ENDIAN);
	offset += 2;

	return offset;
}

#if 0
static int
dissect_sfi_ioflag(tvbuff_t *tvb, proto_tree *parent_tree, int offset)
{
	static const int * flags[] = {
		&hf_smb_sfi_writetru,
		&hf_smb_sfi_caching,
		NULL
	};

	proto_tree_add_bitmask(parent_tree, tvb, offset, hf_smb_sfi, ett_smb_ioflag, flags, ENC_LITTLE_ENDIAN);
	offset += 2;

	return offset;
}
#endif

int
dissect_get_dfs_request_data(tvbuff_t *tvb, packet_info *pinfo,
    proto_tree *tree, int offset, guint16 *bcp, gboolean unicode)
{
	int         fn_len;
	const char *fn;
	guint16     bc = *bcp;

	/* referral level */
	CHECK_BYTE_COUNT_TRANS(2);
	proto_tree_add_item(tree, hf_smb_max_referral_level, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_TRANS(2);

	/* file name */
	fn = get_unicode_or_ascii_string(tvb, &offset, unicode, &fn_len, FALSE, FALSE, &bc);
	CHECK_STRING_TRANS(fn);
	proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
		fn);
	COUNT_BYTES_TRANS(fn_len);

	col_append_fstr(pinfo->cinfo, COL_INFO, ", File: %s",
		    format_text(fn, strlen(fn)));

	*bcp = bc;
	return offset;
}

static int
dissect_transaction2_request_parameters(tvbuff_t *tvb, packet_info *pinfo,
    proto_tree *parent_tree, int offset, int subcmd, guint16 bc, smb_info_t *si)
{
	proto_tree           *tree;
	smb_transact2_info_t *t2i;
	int                   fn_len;
	const char           *fn;

	DISSECTOR_ASSERT(si);

	if ((si->sip != NULL) && (si->sip->extra_info_type == SMB_EI_T2I))
		t2i = (smb_transact2_info_t *)si->sip->extra_info;
	else
		t2i = NULL;

	tree = proto_tree_add_subtree_format(parent_tree, tvb, offset, bc,
				ett_smb_transaction_params, NULL, "%s Parameters",
				val_to_str_ext(subcmd, &trans2_cmd_vals_ext,
					       "Unknown (0x%02x)"));

	switch(subcmd) {
	case 0x0000:	/*TRANS2_OPEN2*/
		/* open flags */
		CHECK_BYTE_COUNT_TRANS(2);
		offset = dissect_open_flags(tvb, tree, offset, 0x000f);
		bc -= 2;

		/* desired access */
		CHECK_BYTE_COUNT_TRANS(2);
		offset = dissect_access(tvb, tree, offset, hf_smb_desired_access);
		bc -= 2;

		/* Search Attributes */
		CHECK_BYTE_COUNT_TRANS(2);
		offset = dissect_search_attributes(tvb, tree, offset);
		bc -= 2;

		/* File Attributes */
		CHECK_BYTE_COUNT_TRANS(2);
		offset = dissect_file_attributes(tvb, tree, offset);
		bc -= 2;

		/* create time */
		CHECK_BYTE_COUNT_TRANS(4);
		offset = dissect_smb_datetime(tvb, tree, offset,
			hf_smb_create_time,
			hf_smb_create_dos_date, hf_smb_create_dos_time,
			TRUE);
		bc -= 4;

		/* open function */
		CHECK_BYTE_COUNT_TRANS(2);
		offset = dissect_open_function(tvb, tree, offset);
		bc -= 2;

		/* allocation size */
		CHECK_BYTE_COUNT_TRANS(4);
		proto_tree_add_item(tree, hf_smb_alloc_size, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS(4);

		/* 10 reserved bytes */
		CHECK_BYTE_COUNT_TRANS(10);
		proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 10, ENC_NA);
		COUNT_BYTES_TRANS(10);

		/* file name */
		fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len, FALSE, FALSE, &bc);
		CHECK_STRING_TRANS(fn);
		proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
			fn);
		COUNT_BYTES_TRANS(fn_len);

		col_append_fstr(pinfo->cinfo, COL_INFO, ", Path: %s",
			    format_text(fn, strlen(fn)));
		break;
	case 0x0001:	/*TRANS2_FIND_FIRST2*/
		/* Search Attributes */
		CHECK_BYTE_COUNT_TRANS(2);
		offset = dissect_search_attributes(tvb, tree, offset);
		bc -= 2;

		/* search count */
		CHECK_BYTE_COUNT_TRANS(2);
		proto_tree_add_item(tree, hf_smb_search_count, tvb, offset, 2, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS(2);

		/* Find First2 flags */
		CHECK_BYTE_COUNT_TRANS(2);
		offset = dissect_ff2_flags(tvb, pinfo, tree, offset, si);
		bc -= 2;

		/* Find First2 information level */
		CHECK_BYTE_COUNT_TRANS(2);
		si->info_level = tvb_get_letohs(tvb, offset);
		if ((t2i != NULL) && !pinfo->fd->flags.visited)
			t2i->info_level = si->info_level;
		proto_tree_add_uint(tree, hf_smb_ff2_information_level, tvb, offset, 2, si->info_level);
		COUNT_BYTES_TRANS(2);

		/* storage type */
		CHECK_BYTE_COUNT_TRANS(4);
		proto_tree_add_item(tree, hf_smb_storage_type, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS(4);

		/* search pattern */
		fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len, FALSE, FALSE, &bc);
		CHECK_STRING_TRANS(fn);
		if (t2i && !t2i->name) {
			t2i->name = wmem_strdup(wmem_file_scope(), fn);
		}
		proto_tree_add_string(tree, hf_smb_search_pattern, tvb, offset, fn_len,
			fn);
		COUNT_BYTES_TRANS(fn_len);

		col_append_fstr(pinfo->cinfo, COL_INFO, ", Pattern: %s",
			    format_text(fn, strlen(fn)));

		break;
	case 0x0002:	/*TRANS2_FIND_NEXT2*/
		/* sid */
		CHECK_BYTE_COUNT_TRANS(2);
		proto_tree_add_item(tree, hf_smb_search_id, tvb, offset, 2, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS(2);

		/* search count */
		CHECK_BYTE_COUNT_TRANS(2);
		proto_tree_add_item(tree, hf_smb_search_count, tvb, offset, 2, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS(2);

		/* Find First2 information level */
		CHECK_BYTE_COUNT_TRANS(2);
		si->info_level = tvb_get_letohs(tvb, offset);
		if ((t2i != NULL) && !pinfo->fd->flags.visited)
			t2i->info_level = si->info_level;
		proto_tree_add_uint(tree, hf_smb_ff2_information_level, tvb, offset, 2, si->info_level);
		COUNT_BYTES_TRANS(2);

		/* resume key */
		CHECK_BYTE_COUNT_TRANS(4);
		proto_tree_add_item(tree, hf_smb_resume, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS(4);

		/* Find First2 flags */
		CHECK_BYTE_COUNT_TRANS(2);
		offset = dissect_ff2_flags(tvb, pinfo, tree, offset, si);
		bc -= 2;

		/* file name */
		fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len, FALSE, FALSE, &bc);
		CHECK_STRING_TRANS(fn);
		proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
			fn);
		COUNT_BYTES_TRANS(fn_len);

		col_append_fstr(pinfo->cinfo, COL_INFO, ", Continue: %s",
			    format_text(fn, strlen(fn)));

		break;
	case 0x0003:	/*TRANS2_QUERY_FS_INFORMATION*/
		/* level of interest */
		CHECK_BYTE_COUNT_TRANS(2);
		si->info_level = tvb_get_letohs(tvb, offset);
		if ((t2i != NULL) && !pinfo->fd->flags.visited)
			t2i->info_level = si->info_level;
		proto_tree_add_uint(tree, hf_smb_qfsi_information_level, tvb, offset, 2, si->info_level);
		COUNT_BYTES_TRANS(2);

		col_append_fstr(pinfo->cinfo, COL_INFO, ", %s",
					val_to_str_ext(si->info_level, &qfsi_vals_ext,
						   "Unknown (0x%02x)"));

		break;
	case 0x0004:	/*TRANS2_SET_FS_INFORMATION*/
		/* level of interest */
		CHECK_BYTE_COUNT_TRANS(4);
		si->info_level = tvb_get_letohs(tvb, offset+2);
		if ((t2i != NULL) && !pinfo->fd->flags.visited)
			t2i->info_level = si->info_level;
		proto_tree_add_uint(tree, hf_smb_sfsi_information_level, tvb, offset+2, 2, si->info_level);
		COUNT_BYTES_TRANS(4);

		col_append_fstr(pinfo->cinfo, COL_INFO, ", %s",
					val_to_str(si->info_level, sfsi_vals,
						   "Unknown (0x%02x)"));

		break;
	case 0x0005:	/*TRANS2_QUERY_PATH_INFORMATION*/
		/* level of interest */
		CHECK_BYTE_COUNT_TRANS(2);
		si->info_level = tvb_get_letohs(tvb, offset);
		if ((t2i != NULL) && !pinfo->fd->flags.visited)
			t2i->info_level = si->info_level;
		proto_tree_add_uint(tree, hf_smb_qpi_loi, tvb, offset, 2, si->info_level);
		COUNT_BYTES_TRANS(2);

		col_append_fstr(
				pinfo->cinfo, COL_INFO, ", %s",
				val_to_str_ext(si->info_level, &qpi_loi_vals_ext,
					   "Unknown (%u)"));

		/* 4 reserved bytes */
		CHECK_BYTE_COUNT_TRANS(4);
		proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 4, ENC_NA);
		COUNT_BYTES_TRANS(4);

		/* file name */
		fn = tvb_get_stringz_enc(wmem_packet_scope(), tvb, offset, &fn_len, (si->unicode ? ENC_UTF_16|ENC_LITTLE_ENDIAN : ENC_ASCII|ENC_NA));

		CHECK_STRING_TRANS(fn);
		proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
			fn);
		COUNT_BYTES_TRANS(fn_len);
		if (t2i && !t2i->name) {
			t2i->name = wmem_strdup(wmem_file_scope(), fn);
		}

		col_append_fstr(pinfo->cinfo, COL_INFO, ", Path: %s",
			    format_text(fn, strlen(fn)));

		break;
	case 0x0006:	/*TRANS2_SET_PATH_INFORMATION*/
		/* level of interest */
		CHECK_BYTE_COUNT_TRANS(2);
		si->info_level = tvb_get_letohs(tvb, offset);
		if ((t2i != NULL) && !pinfo->fd->flags.visited)
			t2i->info_level = si->info_level;
		proto_tree_add_uint(tree, hf_smb_spi_loi, tvb, offset, 2, si->info_level);
		COUNT_BYTES_TRANS(2);

		/* 4 reserved bytes */
		CHECK_BYTE_COUNT_TRANS(4);
		proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 4, ENC_NA);
		COUNT_BYTES_TRANS(4);

		/* file name */
		fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len, FALSE, FALSE, &bc);
		CHECK_STRING_TRANS(fn);
		proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
			fn);
		COUNT_BYTES_TRANS(fn_len);

		col_append_fstr(pinfo->cinfo, COL_INFO, ", Path: %s",
			    format_text(fn, strlen(fn)));

		break;
	case 0x0007: {	/*TRANS2_QUERY_FILE_INFORMATION*/
		guint16 fid;

		/* fid */
		CHECK_BYTE_COUNT_TRANS(2);
		fid = tvb_get_letohs(tvb, offset);
		dissect_smb_fid(tvb, pinfo, tree, offset, 2, fid, FALSE, FALSE, FALSE, si);
		COUNT_BYTES_TRANS(2);

		/* level of interest */
		CHECK_BYTE_COUNT_TRANS(2);
		si->info_level = tvb_get_letohs(tvb, offset);
		if ((t2i != NULL) && !pinfo->fd->flags.visited)
			t2i->info_level = si->info_level;
		proto_tree_add_uint(tree, hf_smb_qpi_loi, tvb, offset, 2, si->info_level);
		COUNT_BYTES_TRANS(2);

		col_append_fstr(
				pinfo->cinfo, COL_INFO, ", %s",
				val_to_str_ext(si->info_level, &qpi_loi_vals_ext,
					   "Unknown (%u)"));

		break;
	}
	case 0x0008: {	/*TRANS2_SET_FILE_INFORMATION*/
		guint16 fid;

		/* fid */
		CHECK_BYTE_COUNT_TRANS(2);
		fid = tvb_get_letohs(tvb, offset);
		dissect_smb_fid(tvb, pinfo, tree, offset, 2, fid, FALSE, FALSE, FALSE, si);
		COUNT_BYTES_TRANS(2);

		/* level of interest */
		CHECK_BYTE_COUNT_TRANS(2);
		si->info_level = tvb_get_letohs(tvb, offset);
		if ((t2i != NULL) && !pinfo->fd->flags.visited)
			t2i->info_level = si->info_level;
		proto_tree_add_uint(tree, hf_smb_spi_loi, tvb, offset, 2, si->info_level);
		COUNT_BYTES_TRANS(2);

#if 0
		/*
		 * XXX - "Microsoft Networks SMB File Sharing Protocol
		 * Extensions Version 3.0, Document Version 1.11,
		 * July 19, 1990" says this is I/O flags, but it's
		 * reserved in the SNIA spec, and some clients appear
		 * to leave junk in it.
		 *
		 * Is this some field used only if a particular
		 * dialect was negotiated, so that clients can feel
		 * safe not setting it if they haven't negotiated that
		 * dialect?  Or do the (non-OS/2) clients simply not care
		 * about that particular OS/2-oriented dialect?
		 */

		/* IO Flag */
		CHECK_BYTE_COUNT_TRANS(2);
		offset = dissect_sfi_ioflag(tvb, tree, offset);
		bc -= 2;
#else
		/* 2 reserved bytes */
		CHECK_BYTE_COUNT_TRANS(2);
		proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 2, ENC_NA);
		COUNT_BYTES_TRANS(2);
#endif

		break;
	}
	case 0x0009:	/*TRANS2_FSCTL*/
		/* this call has no parameter block in the request */

		/*
		 * XXX - "Microsoft Networks SMB File Sharing Protocol
		 * Extensions Version 3.0, Document Version 1.11,
		 * July 19, 1990" says this this contains a
		 * "File system specific parameter block".  (That means
		 * we may not be able to dissect it in any case.)
		 */
		break;
	case 0x000a:	/*TRANS2_IOCTL2*/
		/* this call has no parameter block in the request */

		/*
		 * XXX - "Microsoft Networks SMB File Sharing Protocol
		 * Extensions Version 3.0, Document Version 1.11,
		 * July 19, 1990" says this this contains a
		 * "Device/function specific parameter block".  (That
		 * means we may not be able to dissect it in any case.)
		 */
		break;
	case 0x000b:	/*TRANS2_FIND_NOTIFY_FIRST*/
		/* Search Attributes */
		CHECK_BYTE_COUNT_TRANS(2);
		offset = dissect_search_attributes(tvb, tree, offset);
		bc -= 2;

		/* Number of changes to wait for */
		CHECK_BYTE_COUNT_TRANS(2);
		proto_tree_add_item(tree, hf_smb_change_count, tvb, offset, 2, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS(2);

		/* Find Notify information level */
		CHECK_BYTE_COUNT_TRANS(2);
		si->info_level = tvb_get_letohs(tvb, offset);
		if ((t2i != NULL) && !pinfo->fd->flags.visited)
			t2i->info_level = si->info_level;
		proto_tree_add_uint(tree, hf_smb_fn_information_level, tvb, offset, 2, si->info_level);
		COUNT_BYTES_TRANS(2);

		/* 4 reserved bytes */
		CHECK_BYTE_COUNT_TRANS(4);
		proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 4, ENC_NA);
		COUNT_BYTES_TRANS(4);

		/* file name */
		fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len, FALSE, FALSE, &bc);
		CHECK_STRING_TRANS(fn);
		proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
			fn);
		COUNT_BYTES_TRANS(fn_len);

		col_append_fstr(pinfo->cinfo, COL_INFO, ", Path: %s",
			    format_text(fn, strlen(fn)));

		break;
	case 0x000c:	/*TRANS2_FIND_NOTIFY_NEXT*/
		/* Monitor handle */
		CHECK_BYTE_COUNT_TRANS(2);
		proto_tree_add_item(tree, hf_smb_monitor_handle, tvb, offset, 2, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS(2);

		/* Number of changes to wait for */
		CHECK_BYTE_COUNT_TRANS(2);
		proto_tree_add_item(tree, hf_smb_change_count, tvb, offset, 2, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS(2);

		break;
	case 0x000d:	/*TRANS2_CREATE_DIRECTORY*/
		/* 4 reserved bytes */
		CHECK_BYTE_COUNT_TRANS(4);
		proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 4, ENC_NA);
		COUNT_BYTES_TRANS(4);

		/* dir name */
		fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len,
			FALSE, FALSE, &bc);
		CHECK_STRING_TRANS(fn);
		proto_tree_add_string(tree, hf_smb_dir_name, tvb, offset, fn_len,
			fn);
		COUNT_BYTES_TRANS(fn_len);

		col_append_fstr(pinfo->cinfo, COL_INFO, ", Dir: %s",
			    format_text(fn, strlen(fn)));
		break;
	case 0x000e:	/*TRANS2_SESSION_SETUP*/
		/* XXX unknown structure*/
		break;
	case 0x0010:	/*TRANS2_GET_DFS_REFERRAL*/
		offset = dissect_get_dfs_request_data(tvb, pinfo, tree, offset, &bc, si->unicode);
		break;
	case 0x0011:	/*TRANS2_REPORT_DFS_INCONSISTENCY*/
		/* file name */
		fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len, FALSE, FALSE, &bc);
		CHECK_STRING_TRANS(fn);
		proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
			fn);
		COUNT_BYTES_TRANS(fn_len);

		col_append_fstr(pinfo->cinfo, COL_INFO, ", File: %s",
			    format_text(fn, strlen(fn)));
		break;
	}

	/* ooops there were data we didn't know how to process */
	if (bc != 0) {
		proto_tree_add_item(tree, hf_smb_unknown, tvb, offset, bc, ENC_NA);
		offset += bc;
	}

	return offset;
}

/*
 * XXX - just use "dissect_connect_flags()" here?
 */
static guint16
dissect_transaction_flags(tvbuff_t *tvb, proto_tree *parent_tree, int offset)
{
	guint16     mask;
	static const int * flags[] = {
		&hf_smb_transaction_flags_owt,
		&hf_smb_transaction_flags_dtid,
		NULL
	};

	mask = tvb_get_letohs(tvb, offset);
	proto_tree_add_bitmask(parent_tree, tvb, offset, hf_smb_transaction_flags, ett_smb_transaction_flags, flags, ENC_LITTLE_ENDIAN);

	return mask;
}


static int
dissect_get_dfs_flags(tvbuff_t *tvb, proto_tree *parent_tree, int offset)
{
	static const int * flags[] = {
		&hf_smb_get_dfs_server_hold_storage,
		&hf_smb_get_dfs_fielding,
		NULL
	};

	proto_tree_add_bitmask(parent_tree, tvb, offset, hf_smb_get_dfs_flags, ett_smb_get_dfs_flags, flags, ENC_LITTLE_ENDIAN);

	offset += 2;
	return offset;
}

static int
dissect_dfs_referral_flags(tvbuff_t *tvb, proto_tree *parent_tree, int offset)
{
	static const int * flags[] = {
		&hf_smb_dfs_referral_flags_name_list_referral,
		&hf_smb_dfs_referral_flags_target_set_boundary,
		NULL
	};

	proto_tree_add_bitmask(parent_tree, tvb, offset, hf_smb_dfs_referral_flags, ett_smb_dfs_referral_flags, flags, ENC_LITTLE_ENDIAN);
	offset += 2;

	return offset;
}


/* dfs inconsistency data  (4.4.2)
*/
static int
dissect_dfs_inconsistency_data(tvbuff_t *tvb, packet_info *pinfo _U_,
    proto_tree *tree, int offset, guint16 *bcp, smb_info_t *si)
{
	int         fn_len;
	const char *fn;

	DISSECTOR_ASSERT(si);

	/*XXX shouldn this data hold version and size? unclear from doc*/
	/* referral version */
	CHECK_BYTE_COUNT_TRANS_SUBR(2);
	proto_tree_add_item(tree, hf_smb_dfs_referral_version, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_TRANS_SUBR(2);

	/* referral size */
	CHECK_BYTE_COUNT_TRANS_SUBR(2);
	proto_tree_add_item(tree, hf_smb_dfs_referral_size, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_TRANS_SUBR(2);

	/* referral server type */
	CHECK_BYTE_COUNT_TRANS_SUBR(2);
	proto_tree_add_item(tree, hf_smb_dfs_referral_server_type, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_TRANS_SUBR(2);

	/* referral flags */
	CHECK_BYTE_COUNT_TRANS_SUBR(2);
	offset = dissect_dfs_referral_flags(tvb, tree, offset);
	*bcp  -= 2;

	/* node name */
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len, FALSE, FALSE, bcp);
	CHECK_STRING_TRANS_SUBR(fn);
	proto_tree_add_string(tree, hf_smb_dfs_referral_node, tvb, offset, fn_len,
		fn);
	COUNT_BYTES_TRANS_SUBR(fn_len);

	return offset;
}

static int
dissect_dfs_referral_strings(tvbuff_t *tvb, proto_tree *tree, int hfindex,
			     int nstring, int stroffset, int oldoffset, int offset,
			     guint16 bc, gboolean unicode, int *end)
{
	int         istring;
	const char *str;
	int         str_len;    /* string length including the terminating NULL. */

	if (stroffset <= oldoffset)
		return oldoffset;

	bc -= (stroffset - offset);
	for (istring = 0; istring < nstring; istring++) {
		if ((gint16)bc > 0) {
			str = get_unicode_or_ascii_string(tvb, &stroffset, unicode, &str_len, FALSE, FALSE, &bc);
			CHECK_STRING_TRANS_SUBR(str);
			proto_tree_add_string(tree, hfindex, tvb, stroffset, str_len, str);
			stroffset += str_len;
			bc -= str_len;
			if (end && (*end < stroffset))
				*end = stroffset;
		}
	}

	return offset;
}


static int
dissect_dfs_referral_string(tvbuff_t *tvb, proto_tree *tree, int hfindex,
			    int stroffset, int oldoffset, int offset,
			    guint16 bc, gboolean unicode, int *end)
{
	return dissect_dfs_referral_strings(tvb, tree, hfindex,
					   1, stroffset, oldoffset, offset,
					   bc, unicode, end);
}

static int
dissect_dfs_referral_entry_v2(tvbuff_t *tvb, proto_tree *tree, int oldoffset, int offset,
			      guint16 refflags _U_, guint16 *bcp, gboolean unicode, int *ucstring_end)
{

	guint16 pathoffset;
	guint16 altpathoffset;
	guint16 nodeoffset;

	/* proximity */
	CHECK_BYTE_COUNT_TRANS_SUBR(4);
	proto_tree_add_item(tree, hf_smb_dfs_referral_proximity, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_TRANS_SUBR(4);

	/* ttl */
	CHECK_BYTE_COUNT_TRANS_SUBR(4);
	proto_tree_add_item(tree, hf_smb_dfs_referral_ttl, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_TRANS_SUBR(4);

	/* path offset */
	CHECK_BYTE_COUNT_TRANS_SUBR(2);
	pathoffset = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_dfs_referral_path_offset, tvb, offset, 2, pathoffset);
	COUNT_BYTES_TRANS_SUBR(2);

	/* alt path offset */
	CHECK_BYTE_COUNT_TRANS_SUBR(2);
	altpathoffset = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_dfs_referral_alt_path_offset, tvb, offset, 2, altpathoffset);
	COUNT_BYTES_TRANS_SUBR(2);

	/* node offset */
	CHECK_BYTE_COUNT_TRANS_SUBR(2);
	nodeoffset = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_dfs_referral_node_offset, tvb, offset, 2, nodeoffset);
	COUNT_BYTES_TRANS_SUBR(2);

	/* path */
	if (pathoffset) {
		dissect_dfs_referral_string(tvb, tree, hf_smb_dfs_referral_path,
					    pathoffset+oldoffset, oldoffset, offset,
					    *bcp, unicode, ucstring_end);
	}

	/* alt path */
	if (altpathoffset) {
		dissect_dfs_referral_string(tvb, tree, hf_smb_dfs_referral_alt_path,
					    altpathoffset+oldoffset, oldoffset, offset,
					    *bcp, unicode, ucstring_end);
	}

	/* node */
	if (nodeoffset) {
		dissect_dfs_referral_string(tvb, tree, hf_smb_dfs_referral_node,
					    nodeoffset+oldoffset, oldoffset, offset,
					    *bcp, unicode, ucstring_end);
	}

	return offset;

}


static int
dissect_dfs_referral_entry_v3(tvbuff_t *tvb, proto_tree *tree, int oldoffset, int offset,
			      guint16 refflags, guint16 *bcp, gboolean unicode, int *ucstring_end)
{
	guint16 domoffset;
	guint16 nexpnames;
	guint16 expoffset;
	guint16 pathoffset;
	guint16 altpathoffset;
	guint16 nodeoffset;

	/* ttl */
	CHECK_BYTE_COUNT_TRANS_SUBR(4);
	proto_tree_add_item(tree, hf_smb_dfs_referral_ttl, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_TRANS_SUBR(4);

	if (refflags & REFENT_FLAGS_NAME_LIST_REFERRAL) {
		/* domain name offset */
		CHECK_BYTE_COUNT_TRANS_SUBR(2);
		domoffset = tvb_get_letohs(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_dfs_referral_domain_offset, tvb, offset, 2, domoffset);
		COUNT_BYTES_TRANS_SUBR(2);

		/* number of expanded names*/
		CHECK_BYTE_COUNT_TRANS_SUBR(2);
		nexpnames = tvb_get_letohs(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_dfs_referral_number_of_expnames, tvb, offset, 2, nexpnames);
		COUNT_BYTES_TRANS_SUBR(2);

		/* expanded names offset */
		CHECK_BYTE_COUNT_TRANS_SUBR(2);
		expoffset = tvb_get_letohs(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_dfs_referral_expnames_offset, tvb, offset, 2, expoffset);
		COUNT_BYTES_TRANS_SUBR(2);

		/* padding: zero or 16 bytes, which should be ignored by clients.
		 * we ignore them too.
		 */

		/* domain name */
		if (domoffset) {
			dissect_dfs_referral_string(tvb, tree, hf_smb_dfs_referral_domain_name,
						    domoffset+oldoffset, oldoffset, offset,
						    *bcp, unicode, ucstring_end);
		}
		/* expanded names */
		if (expoffset) {
			proto_tree *exptree;

			exptree = proto_tree_add_subtree(tree, tvb, offset, *bcp, ett_smb_dfs_referral_expnames, NULL, "Expanded Names");

			dissect_dfs_referral_strings(tvb, exptree, hf_smb_dfs_referral_expname,
						     nexpnames, expoffset+oldoffset, oldoffset, offset,
						     *bcp, unicode, ucstring_end);
		}
	} else {
		/* path offset */
		CHECK_BYTE_COUNT_TRANS_SUBR(2);
		pathoffset = tvb_get_letohs(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_dfs_referral_path_offset, tvb, offset, 2, pathoffset);
		COUNT_BYTES_TRANS_SUBR(2);

		/* alt path offset */
		CHECK_BYTE_COUNT_TRANS_SUBR(2);
		altpathoffset = tvb_get_letohs(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_dfs_referral_alt_path_offset, tvb, offset, 2, altpathoffset);
		COUNT_BYTES_TRANS_SUBR(2);

		/* node offset */
		CHECK_BYTE_COUNT_TRANS_SUBR(2);
		nodeoffset = tvb_get_letohs(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_dfs_referral_node_offset, tvb, offset, 2, nodeoffset);
		COUNT_BYTES_TRANS_SUBR(2);

		/* service site guid */
		CHECK_BYTE_COUNT_TRANS_SUBR(16);
		proto_tree_add_item(tree, hf_smb_dfs_referral_server_guid, tvb, offset, 16, ENC_NA);
		COUNT_BYTES_TRANS_SUBR(16);

		/* path */
		if (pathoffset) {
			dissect_dfs_referral_string(tvb, tree, hf_smb_dfs_referral_path,
						    pathoffset+oldoffset, oldoffset, offset,
						    *bcp, unicode, ucstring_end);
		}

		/* alt path */
		if (altpathoffset) {
			dissect_dfs_referral_string(tvb, tree, hf_smb_dfs_referral_alt_path,
						    altpathoffset+oldoffset, oldoffset, offset,
						    *bcp, unicode, ucstring_end);
		}

		/* node */
		if (nodeoffset) {
			dissect_dfs_referral_string(tvb, tree, hf_smb_dfs_referral_node,
						    nodeoffset+oldoffset, oldoffset, offset,
						    *bcp, unicode, ucstring_end);
		}
	}

	return offset;

}

/* get dfs referral data  (4.4.1)
*/
int
dissect_get_dfs_referral_data(tvbuff_t *tvb, packet_info *pinfo _U_,
    proto_tree *tree, int offset, guint16 *bcp, gboolean unicode)
{
	guint16     numref;
	guint16     refsize;
	guint16     refflags;
	int         fn_len;
	const char *fn;
	int         unklen;
	int         ucstring_end;
	int         ucstring_len;

	/* path consumed */
	CHECK_BYTE_COUNT_TRANS_SUBR(2);
	proto_tree_add_item(tree, hf_smb_dfs_path_consumed, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_TRANS_SUBR(2);

	/* num referrals */
	CHECK_BYTE_COUNT_TRANS_SUBR(2);
	numref = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_dfs_num_referrals, tvb, offset, 2, numref);
	COUNT_BYTES_TRANS_SUBR(2);

	/* get dfs flags */
	CHECK_BYTE_COUNT_TRANS_SUBR(2);
	offset = dissect_get_dfs_flags(tvb, tree, offset);
	*bcp -= 2;

	/* XXX - in at least one capture there appears to be 2 bytes
	   of stuff after the Dfs flags, perhaps so that the header
	   in front of the referral list is a multiple of 4 bytes long. */
	CHECK_BYTE_COUNT_TRANS_SUBR(2);
	proto_tree_add_item(tree, hf_smb_padding, tvb, offset, 2, ENC_NA);
	COUNT_BYTES_TRANS_SUBR(2);

	/* if there are any referrals */
	if (numref) {
		proto_item *ref_item;
		proto_tree *ref_tree;
		int old_offset = offset;

		ref_tree = proto_tree_add_subtree(tree,
				tvb, offset, *bcp, ett_smb_dfs_referrals, &ref_item, "Referrals");
		ucstring_end = -1;

		while (numref--) {
			proto_item *ri;
			proto_tree *rt;
			int old_offset_2 = offset;
			guint16 version;

			rt = proto_tree_add_subtree(ref_tree,
					tvb, offset, *bcp, ett_smb_dfs_referral, &ri, "Referral");

			/* referral version */
			CHECK_BYTE_COUNT_TRANS_SUBR(2);
			version = tvb_get_letohs(tvb, offset);
			proto_tree_add_uint(rt, hf_smb_dfs_referral_version,
				tvb, offset, 2, version);
			COUNT_BYTES_TRANS_SUBR(2);

			/* referral size */
			CHECK_BYTE_COUNT_TRANS_SUBR(2);
			refsize = tvb_get_letohs(tvb, offset);
			proto_tree_add_uint(rt, hf_smb_dfs_referral_size, tvb, offset, 2, refsize);
			COUNT_BYTES_TRANS_SUBR(2);

			/* referral server type */
			CHECK_BYTE_COUNT_TRANS_SUBR(2);
			proto_tree_add_item(rt, hf_smb_dfs_referral_server_type, tvb, offset, 2, ENC_LITTLE_ENDIAN);
			COUNT_BYTES_TRANS_SUBR(2);

			/* referral flags */
			CHECK_BYTE_COUNT_TRANS_SUBR(2);
			refflags = tvb_get_letohs(tvb, offset);
			offset = dissect_dfs_referral_flags(tvb, rt, offset);
			*bcp -= 2;

			switch(version) {

			case 1:
				/* node name */
				fn = get_unicode_or_ascii_string(tvb, &offset, unicode, &fn_len, FALSE, FALSE, bcp);
				CHECK_STRING_TRANS_SUBR(fn);
				proto_tree_add_string(rt, hf_smb_dfs_referral_node, tvb, offset, fn_len,
					fn);
				COUNT_BYTES_TRANS_SUBR(fn_len);
				break;

			case 2:
				offset = dissect_dfs_referral_entry_v2(tvb, rt, old_offset_2, offset,
								       refflags, bcp, unicode, &ucstring_end);
				break;
			case 3:
				offset = dissect_dfs_referral_entry_v3(tvb, rt, old_offset_2, offset,
								       refflags, bcp, unicode, &ucstring_end);
				break;
			case 4:
				/* V4 is extactly same as V3, except the version number and
				 * one more ReferralEntryFlags */
				offset = dissect_dfs_referral_entry_v3(tvb, rt, old_offset_2, offset,
								       refflags, bcp, unicode, &ucstring_end);
				break;
			}

			/*
			 * Show anything beyond the length of the referral
			 * as unknown data.
			 */
			unklen = (old_offset_2 + refsize) - offset;
			if (unklen < 0) {
				/*
				 * XXX - the length is bogus.
				 */
				unklen = 0;
			}
			if (unklen != 0) {
				CHECK_BYTE_COUNT_TRANS_SUBR(unklen);
				proto_tree_add_item(rt, hf_smb_unknown, tvb,
				    offset, unklen, ENC_NA);
				COUNT_BYTES_TRANS_SUBR(unklen);
			}

			proto_item_set_len(ri, offset-old_offset_2);
		}

		/*
		 * Treat the offset past the end of the last Unicode
		 * string after the referrals (if any) as the last
		 * offset.
		 */
		if (ucstring_end > offset) {
			ucstring_len = ucstring_end - offset;
			if (*bcp < ucstring_len)
				ucstring_len = *bcp;
			offset += ucstring_len;
			*bcp -= ucstring_len;
		}
		proto_item_set_len(ref_item, offset-old_offset);
	}

	return offset;
}

/* This dissects the standard four 8-byte Windows timestamps ...
 */
static int
dissect_smb_standard_8byte_timestamps(tvbuff_t *tvb,
    packet_info *pinfo _U_, proto_tree *tree,
    int offset, guint16 *bcp, gboolean *trunc)
{
	/* create time */
	CHECK_BYTE_COUNT_SUBR(8);
	offset = dissect_nt_64bit_time(tvb, tree, offset, hf_smb_create_time);
	*bcp -= 8;

	/* access time */
	CHECK_BYTE_COUNT_SUBR(8);
	offset = dissect_nt_64bit_time(tvb, tree, offset, hf_smb_access_time);
	*bcp -= 8;

	/* last write time */
	CHECK_BYTE_COUNT_SUBR(8);
	offset = dissect_nt_64bit_time(tvb, tree, offset,
		hf_smb_last_write_time);
	*bcp -= 8;

	/* last change time */
	CHECK_BYTE_COUNT_SUBR(8);
	offset = dissect_nt_64bit_time(tvb, tree, offset, hf_smb_change_time);
	*bcp -= 8;

	*trunc = FALSE;
	return offset;
}

/* this dissects the SMB_INFO_STANDARD
   as described in 4.2.16.1 of the CIFS 1.0 specification
   or as described in 2.2.8.3.1 of the MS-CIFS specification for query
   section 2.2.8.4.1 of the MS-CIFS specification describes it for set;
   it says that everything past the last write time is "reserved",
   presumably meaning that you can fetch it but not set it
   for now we just use it for both query and set
*/
static int
dissect_qsfi_SMB_INFO_STANDARD(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree,
    int offset, guint16 *bcp, gboolean *trunc)
{
	/* create time */
	CHECK_BYTE_COUNT_SUBR(4);
	offset = dissect_smb_datetime(tvb, tree, offset,
		hf_smb_create_time, hf_smb_create_dos_date, hf_smb_create_dos_time,
		FALSE);
	*bcp -= 4;

	/* access time */
	CHECK_BYTE_COUNT_SUBR(4);
	offset = dissect_smb_datetime(tvb, tree, offset,
		hf_smb_access_time, hf_smb_access_dos_date, hf_smb_access_dos_time,
		FALSE);
	*bcp -= 4;

	/* last write time */
	CHECK_BYTE_COUNT_SUBR(4);
	offset = dissect_smb_datetime(tvb, tree, offset,
		hf_smb_last_write_time, hf_smb_last_write_dos_date, hf_smb_last_write_dos_time,
		FALSE);
	*bcp -= 4;

	/* data size */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_data_size, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* allocation size */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_alloc_size, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* File Attributes */
	CHECK_BYTE_COUNT_SUBR(2);
	offset = dissect_file_attributes(tvb, tree, offset);
	*bcp -= 2;

	/*
	 * The MS-CIFS spec says this doesn't have an EA length field;
	 * the SNIA CIFS spec says it does, as does the 1996
	 * "Microsoft Networks SMB FILE SHARING PROTOCOL Document
	 * Version 6.0p" document.
	 *
	 * Some older SMB documents point to the documentation
	 * for the OS/2 DosQFileInfo() API; the page at
	 *
	 *	http://cyberkinetica.homeunix.net/os2tk45/prcp/111_L2_DosQFileInfo.html
	 *
	 * says that, for level 1 (SMB_INFO_STANDARD), there is no EA
	 * length - that's just for level 2 (SMB_INFO_QUERY_EA_SIZE).
	 *
	 * I've seen captures with it and without it; given the mixed
	 * messages sent by different documents, this is not surprising.
	 *
	 * We display it if it's there; we don't set *trunc if it's
	 * not.
	 *
	 * Note: in FIND_FIRST2/FIND_NEXT2, the EA length is *not*
	 * present.
	 */
	if (*bcp != 0) {
		CHECK_BYTE_COUNT_SUBR(4);
		proto_tree_add_item(tree, hf_smb_ea_list_length, tvb, offset,
		    4, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_SUBR(4);
	}

	*trunc = FALSE;
	return offset;
}

/* this dissects the SMB_INFO_QUERY_EA_SIZE
   as described in 4.2.16.1 of the CIFS 1.0 specification
   and as described in 2.2.8.3.2 of the MS-CIFS specification
*/
static int
dissect_qfi_SMB_INFO_QUERY_EA_SIZE(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree,
    int offset, guint16 *bcp, gboolean *trunc)
{
	/* create time */
	CHECK_BYTE_COUNT_SUBR(4);
	offset = dissect_smb_datetime(tvb, tree, offset,
		hf_smb_create_time, hf_smb_create_dos_date, hf_smb_create_dos_time,
		FALSE);
	*bcp -= 4;

	/* access time */
	CHECK_BYTE_COUNT_SUBR(4);
	offset = dissect_smb_datetime(tvb, tree, offset,
		hf_smb_access_time, hf_smb_access_dos_date, hf_smb_access_dos_time,
		FALSE);
	*bcp -= 4;

	/* last write time */
	CHECK_BYTE_COUNT_SUBR(4);
	offset = dissect_smb_datetime(tvb, tree, offset,
		hf_smb_last_write_time, hf_smb_last_write_dos_date, hf_smb_last_write_dos_time,
		FALSE);
	*bcp -= 4;

	/* data size */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_data_size, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* allocation size */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_alloc_size, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* File Attributes */
	CHECK_BYTE_COUNT_SUBR(2);
	offset = dissect_file_attributes(tvb, tree, offset);
	*bcp -= 2;

	/* ea length */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_ea_list_length, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	*trunc = FALSE;
	return offset;
}

/* this dissects the SMB_INFO_QUERY_EAS_FROM_LIST and SMB_INFO_QUERY_ALL_EAS
   as described in 4.2.16.2
*/
static int
dissect_4_2_16_2(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree,
    int offset, guint16 *bcp, gboolean *trunc)
{
	guint8  name_len;
	guint16 data_len;
	/* EA size */

	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_ea_list_length, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	while (*bcp > 0) {
		proto_item *item;
		proto_tree *subtree;
		int start_offset = offset;
		guint8 *name;

		subtree = proto_tree_add_subtree(
			tree, tvb, offset, 0, ett_smb_ea, &item, "Extended Attribute");

		/* EA flags */

		CHECK_BYTE_COUNT_SUBR(1);
		proto_tree_add_item(
			subtree, hf_smb_ea_flags, tvb, offset, 1, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_SUBR(1);

		/* EA name length */

		name_len = tvb_get_guint8(tvb, offset);

		CHECK_BYTE_COUNT_SUBR(1);
		proto_tree_add_item(
			subtree, hf_smb_ea_name_length, tvb, offset, 1, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_SUBR(1);

		/* EA data length */

		data_len = tvb_get_letohs(tvb, offset);

		CHECK_BYTE_COUNT_SUBR(2);
		proto_tree_add_item(
			subtree, hf_smb_ea_data_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_SUBR(2);

		/* EA name */

		name = tvb_get_string_enc(wmem_packet_scope(), tvb, offset, name_len, ENC_ASCII);
		proto_item_append_text(item, ": %s", format_text(name, strlen(name)));

		CHECK_BYTE_COUNT_SUBR(name_len + 1);
		proto_tree_add_item(
			subtree, hf_smb_ea_name, tvb, offset, name_len + 1,
			ENC_ASCII|ENC_NA);
		COUNT_BYTES_SUBR(name_len + 1);

		/* EA data */

		CHECK_BYTE_COUNT_SUBR(data_len);
		proto_tree_add_item(
			subtree, hf_smb_ea_data, tvb, offset, data_len, ENC_NA);
		COUNT_BYTES_SUBR(data_len);

		proto_item_set_len(item, offset - start_offset);
	}

	*trunc = FALSE;
	return offset;
}

/* this dissects the SMB_INFO_IS_NAME_VALID
   as described in 4.2.16.3
*/
static int
dissect_4_2_16_3(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree,
    int offset, guint16 *bcp, gboolean *trunc, smb_info_t *si)
{
	int         fn_len;
	const char *fn;

	DISSECTOR_ASSERT(si);

	/* file name */
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len, FALSE, FALSE, bcp);
	CHECK_STRING_SUBR(fn);
	proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
		fn);
	COUNT_BYTES_SUBR(fn_len);

	*trunc = FALSE;
	return offset;
}

/* this dissects the SMB_QUERY_FILE_BASIC_INFO
   as described in 4.2.16.4
*/
static int
dissect_4_2_16_4(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
    int offset, guint16 *bcp, gboolean *trunc)
{

	offset = dissect_smb_standard_8byte_timestamps(tvb, pinfo, tree, offset, bcp, trunc);
	if (*trunc) {
	  return offset;
	}

	/* File Attributes */
	CHECK_BYTE_COUNT_SUBR(4);
	offset = dissect_file_ext_attr(tvb, tree, offset);
	*bcp -= 4;

	*trunc = FALSE;
	return offset;
}

/* this dissects the SMB_QUERY_FILE_STANDARD_INFO
   as described in 4.2.16.5 of the SNIA CIFS spec
   and section 2.2.8.3.7 of the MS-CIFS spec
*/
int
dissect_qfi_SMB_FILE_STANDARD_INFO(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree,
    int offset, guint16 *bcp, gboolean *trunc)
{
	/* allocation size */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_alloc_size64, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* end of file */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_end_of_file, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* number of links */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_number_of_links, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* delete pending */
	CHECK_BYTE_COUNT_SUBR(1);
	proto_tree_add_item(tree, hf_smb_delete_pending, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(1);

	/* is directory */
	CHECK_BYTE_COUNT_SUBR(1);
	proto_tree_add_item(tree, hf_smb_is_directory, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(1);

	*trunc = FALSE;
	return offset;
}

/* this dissects the SMB_QUERY_FILE_INTERNAL_INFO
*/
int
dissect_qfi_SMB_FILE_INTERNAL_INFO(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree,
    int offset, guint16 *bcp, gboolean *trunc)
{
	/* file id */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_index_number, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	*trunc = FALSE;
	return offset;
}

/* this dissects the SMB_QUERY_FILE_POSITION_INFO
*/
int
dissect_qsfi_SMB_FILE_POSITION_INFO(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree,
    int offset, guint16 *bcp, gboolean *trunc)
{
	/* file position */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_position, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	*trunc = FALSE;
	return offset;
}

/* this dissects the SMB_QUERY_FILE_MODE_INFO
*/
int
dissect_qsfi_SMB_FILE_MODE_INFO(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree,
    int offset, guint16 *bcp, gboolean *trunc)
{
	/* mode */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_mode, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	*trunc = FALSE;
	return offset;
}

/* this dissects the SMB_QUERY_FILE_ALIGNMENT_INFO
*/
int
dissect_qfi_SMB_FILE_ALIGNMENT_INFO(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree,
    int offset, guint16 *bcp, gboolean *trunc)
{
	/* alignment */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_t2_alignment, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	*trunc = FALSE;
	return offset;
}

/* this dissects the SMB_QUERY_FILE_EA_INFO
   as described in 4.2.16.6 of the SNIA CIFS spec
   and 2.2.8.3.8 of the MS-CIFS spec
*/
int
dissect_qfi_SMB_FILE_EA_INFO(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree,
    int offset, guint16 *bcp, gboolean *trunc)
{
	/* ea length */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_ea_list_length, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	*trunc = FALSE;
	return offset;
}

/* this dissects the SMB_FILE_ALLOCATION_INFO
   as described in 4.2.19.3 in the SNIA CIFS spec
   and the SMB_SET_FILE_ALLOCATION_INFO
   as described in 2.2.8.4.5 in the MS-CIFS spec for set (MS-CIFS doesn't
   say it can be queried)
*/
int
dissect_qsfi_SMB_FILE_ALLOCATION_INFO(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree,
    int offset, guint16 *bcp, gboolean *trunc)
{
	/* allocation size */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_alloc_size64, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	*trunc = FALSE;
	return offset;
}

/* this dissects the SMB_FILE_ENDOFFILE_INFO
   as described in 4.2.19.4 in the SNIA CIFS spec
   and the SMB_SET_FILE_END_OF_FILE_INFO
   as described in 2.2.8.4.6 in the MS-CIFS spec for set (MS-CIFS doesn't
   say it can be queried)
*/
int
dissect_qsfi_SMB_FILE_ENDOFFILE_INFO(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree,
    int offset, guint16 *bcp, gboolean *trunc)
{
	/* offset of end of file */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_end_of_file, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	*trunc = FALSE;
	return offset;
}

/* this dissects the SMB_QUERY_FILE_NAME_INFO
   as described in 4.2.16.7 of the SNIA CIFS spec
   and in 2.2.8.3.9 of the MS-CIFS spec
   this is the same as SMB_QUERY_FILE_ALT_NAME_INFO
   as described in 4.2.16.9 of the SNIA CIFS spec
   and 2.2.8.3.11 of the MS-CIFS spec
   although the latter two are used to fetch the 8.3 name
   rather than the long name
*/
int
dissect_qfi_SMB_FILE_NAME_INFO(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree,
    int offset, guint16 *bcp, gboolean *trunc, gboolean unicode)
{
	int         fn_len;
	const char *fn;

	/* file name len */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_file_name_len, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* file name */
	fn = get_unicode_or_ascii_string(tvb, &offset, unicode, &fn_len, FALSE, FALSE, bcp);
	CHECK_STRING_SUBR(fn);
	proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
		fn);
	COUNT_BYTES_SUBR(fn_len);

	*trunc = FALSE;
	return offset;
}

/* this dissects the SMB_QUERY_FILE_ALL_INFO
   as described in 2.2.8.3.8 of the MS-CIFS spec
   but not as described in 4.2.16.8 since SNIA spec is wrong
*/
static int
dissect_qfi_SMB_FILE_ALL_INFO(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
    int offset, guint16 *bcp, gboolean *trunc, smb_info_t *si)
{
	guint32     fn_len;
	const char *fn;

	DISSECTOR_ASSERT(si);

	offset = dissect_smb_standard_8byte_timestamps(tvb, pinfo, tree, offset, bcp, trunc);
	if (*trunc) {
	  return offset;
	}

	/* File Attributes */
	CHECK_BYTE_COUNT_SUBR(4);
	offset = dissect_file_ext_attr(tvb, tree, offset);
	*bcp   -= 4;

	/* 4 pad bytes */
	offset += 4;
	*bcp   -= 4;

	/* allocation size */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_alloc_size64, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* end of file */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_end_of_file, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* number of links */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_number_of_links, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* delete pending */
	CHECK_BYTE_COUNT_SUBR(1);
	proto_tree_add_item(tree, hf_smb_delete_pending, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(1);

	/* is directory */
	CHECK_BYTE_COUNT_SUBR(1);
	proto_tree_add_item(tree, hf_smb_is_directory, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(1);

	/* 2 pad bytes */
	offset += 2;
	*bcp   -= 2;

	/* ea length */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_ea_list_length, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* file name len */
	CHECK_BYTE_COUNT_SUBR(4);
	fn_len = (guint32)tvb_get_letohl(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_file_name_len, tvb, offset, 4, fn_len);
	COUNT_BYTES_SUBR(4);


	/* file name */
	CHECK_BYTE_COUNT_SUBR(fn_len);
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len, TRUE, TRUE, bcp);
	if (fn != NULL) {
		proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
			fn);
		COUNT_BYTES_SUBR(fn_len);
	}


	if (*trunc)
		return offset;

	return offset;
}

/* this dissects the SMB_QUERY_FILE_STREAM_INFO
   as described in 4.2.16.10 of the SNIA CIFS spec
   and 2.2.8.3.12 of the MS-CIFS spec
*/
int
dissect_qfi_SMB_FILE_STREAM_INFO(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *parent_tree,
    int offset, guint16 *bcp, gboolean *trunc, int unicode)
{
	proto_item *item;
	proto_tree *tree;
	int         old_offset;
	guint32     neo;
	int         fn_len;
	const char *fn;
	int         padcnt;


	for (;;) {
		old_offset = offset;

		/* next entry offset */
		CHECK_BYTE_COUNT_SUBR(4);
		tree = proto_tree_add_subtree(parent_tree, tvb, offset, *bcp, ett_smb_ff2_data, &item, "Stream Info");

		neo = tvb_get_letohl(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_next_entry_offset, tvb, offset, 4, neo);
		COUNT_BYTES_SUBR(4);

		/* stream name len */
		CHECK_BYTE_COUNT_SUBR(4);
		fn_len = tvb_get_letohl(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_t2_stream_name_length, tvb, offset, 4, fn_len);
		COUNT_BYTES_SUBR(4);

		/* stream size */
		CHECK_BYTE_COUNT_SUBR(8);
		proto_tree_add_item(tree, hf_smb_t2_stream_size, tvb, offset, 8, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_SUBR(8);

		/* allocation size */
		CHECK_BYTE_COUNT_SUBR(8);
		proto_tree_add_item(tree, hf_smb_alloc_size64, tvb, offset, 8, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_SUBR(8);

		/* stream name */
		fn = get_unicode_or_ascii_string(tvb, &offset, unicode, &fn_len, FALSE, TRUE, bcp);
		CHECK_STRING_SUBR(fn);
		proto_tree_add_string(tree, hf_smb_t2_stream_name, tvb, offset, fn_len,
			fn);
		COUNT_BYTES_SUBR(fn_len);

		proto_item_append_text(item, ": %s", format_text(fn, strlen(fn)));
		proto_item_set_len(item, offset-old_offset);

		if (neo == 0)
			break;	/* no more structures */

		/* skip to next structure */
		padcnt = (old_offset + neo) - offset;
		if (padcnt < 0) {
			/*
			 * XXX - this is bogus; flag it?
			 */
			padcnt = 0;
		}
		if (padcnt != 0) {
			CHECK_BYTE_COUNT_SUBR(padcnt);
			COUNT_BYTES_SUBR(padcnt);
		}
	}

	*trunc = FALSE;
	return offset;
}

/* this dissects the SMB_QUERY_FILE_COMPRESSION_INFO
   as described in 4.2.16.11 of the SNIA CIFS spec
   and 2.2.8.3.13 of the MS-CIFS spec
*/
int
dissect_qfi_SMB_FILE_COMPRESSION_INFO(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree,
    int offset, guint16 *bcp, gboolean *trunc)
{
	/* compressed file size */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_t2_compressed_file_size, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* compression format */
	CHECK_BYTE_COUNT_SUBR(2);
	proto_tree_add_item(tree, hf_smb_t2_compressed_format, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(2);

	/* compression unit shift */
	CHECK_BYTE_COUNT_SUBR(1);
	proto_tree_add_item(tree, hf_smb_t2_compressed_unit_shift, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(1);

	/* compression chunk shift */
	CHECK_BYTE_COUNT_SUBR(1);
	proto_tree_add_item(tree, hf_smb_t2_compressed_chunk_shift, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(1);

	/* compression cluster shift */
	CHECK_BYTE_COUNT_SUBR(1);
	proto_tree_add_item(tree, hf_smb_t2_compressed_cluster_shift, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(1);

	/* 3 reserved bytes */
	CHECK_BYTE_COUNT_SUBR(3);
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 3, ENC_NA);
	COUNT_BYTES_SUBR(3);

	*trunc = FALSE;
	return offset;
}

/* 4.2.16.12 - SMB_QUERY_FILE_UNIX_BASIC */

static const value_string unix_file_type_vals[] = {
	{ 0, "File" },
	{ 1, "Directory" },
	{ 2, "Symbolic link" },
	{ 3, "Character device" },
	{ 4, "Block device" },
	{ 5, "FIFO" },
	{ 6, "Socket" },
	{ 0, NULL }
};

static int
dissect_4_2_16_12(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree,
		  int offset, guint16 *bcp, gboolean *trunc)
{
	/* End of file (file size) */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_unix_file_size, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* Number of bytes */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_unix_file_num_bytes, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* Last status change */
	CHECK_BYTE_COUNT_SUBR(8);
	offset = dissect_nt_64bit_time(tvb, tree, offset, hf_smb_unix_file_last_status);
	*bcp -= 8;		/* dissect_nt_64bit_time() increments offset */

	/* Last access time */
	CHECK_BYTE_COUNT_SUBR(8);
	offset = dissect_nt_64bit_time(tvb, tree, offset, hf_smb_unix_file_last_access);
	*bcp -= 8;

	/* Last modification time */
	CHECK_BYTE_COUNT_SUBR(8);
	offset = dissect_nt_64bit_time(tvb, tree, offset, hf_smb_unix_file_last_change);
	*bcp -= 8;

	/* File owner uid */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_unix_file_uid, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* File group gid */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_unix_file_gid, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* File type */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_unix_file_type, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* Major device number */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_unix_file_dev_major, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* Minor device number */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_unix_file_dev_minor, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* Unique id */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_unix_file_unique_id, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* Permissions */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_unix_file_permissions, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* Nlinks */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_unix_file_nlinks, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* Sometimes there is one extra byte in the data field which I
	   guess could be padding, but we are only using 4 or 8 byte
	   data types so this is a bit confusing. -tpot */

	*trunc = FALSE;
	return offset;
}

/* 4.2.16.13 - SMB_QUERY_FILE_UNIX_LINK */

static int
dissect_4_2_16_13(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree,
		  int offset, guint16 *bcp, gboolean *trunc, smb_info_t *si)
{
	const char *fn;
	int         fn_len = 0;

	DISSECTOR_ASSERT(si);

	/* Link destination */

	fn = get_unicode_or_ascii_string(
		tvb, &offset, si->unicode, &fn_len, FALSE, TRUE, bcp);

	CHECK_STRING_SUBR(fn);
	proto_tree_add_string(
		tree, hf_smb_unix_file_link_dest, tvb, offset, fn_len, fn);
	COUNT_BYTES_SUBR(fn_len);

	*trunc = FALSE;
	return offset;
}

/* unix ACL
*/
static int
dissect_qspi_unix_acl(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree,
		  int offset, guint16 *bcp, gboolean *trunc)
{
	guint16 num_file_aces;
	static const int *perm_fields[] = {
		&hf_smb_posix_ace_perm_read,
		&hf_smb_posix_ace_perm_write,
		&hf_smb_posix_ace_perm_execute,
		NULL
	};

	/* version */
	CHECK_BYTE_COUNT_SUBR(2);
	proto_tree_add_item(tree, hf_smb_posix_acl_version, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(2);

	/* num file acls */
	CHECK_BYTE_COUNT_SUBR(2);
	num_file_aces = tvb_get_letohs(tvb, offset);
	proto_tree_add_item(tree, hf_smb_posix_num_file_aces, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(2);

	/* num default acls */
	CHECK_BYTE_COUNT_SUBR(2);
	proto_tree_add_item(tree, hf_smb_posix_num_def_aces, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(2);

	while (num_file_aces--) {
		proto_item *it, *type_item;
		proto_tree *tr;
		int old_offset = offset;
		guint8 ace_type;

		tr = proto_tree_add_subtree(tree, tvb, offset, 0, ett_smb_posix_ace, &it, "ACE");

		/* ace type */
		CHECK_BYTE_COUNT_SUBR(1);
		ace_type = tvb_get_guint8(tvb, offset);
		type_item = proto_tree_add_item(tr, hf_smb_posix_ace_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_SUBR(1);

		CHECK_BYTE_COUNT_SUBR(1);
		proto_tree_add_bitmask(tr, tvb, offset, hf_smb_posix_ace_flags, ett_smb_posix_ace_perms, perm_fields, ENC_BIG_ENDIAN);
		COUNT_BYTES_SUBR(1);

		switch(ace_type) {
		case POSIX_ACE_TYPE_USER_OBJ:
			CHECK_BYTE_COUNT_SUBR(4);
			proto_tree_add_item(tr, hf_smb_posix_ace_perm_owner_uid, tvb, offset, 4, ENC_LITTLE_ENDIAN);
			COUNT_BYTES_SUBR(4);

			CHECK_BYTE_COUNT_SUBR(4);
			/* 4 reserved bytes */
			COUNT_BYTES_SUBR(4);
			break;
		case POSIX_ACE_TYPE_GROUP_OBJ:
			CHECK_BYTE_COUNT_SUBR(4);
			proto_tree_add_item(tr, hf_smb_posix_ace_perm_owner_gid, tvb, offset, 4, ENC_LITTLE_ENDIAN);
			COUNT_BYTES_SUBR(4);

			CHECK_BYTE_COUNT_SUBR(4);
			/* 4 reserved bytes */
			COUNT_BYTES_SUBR(4);
			break;

		case POSIX_ACE_TYPE_MASK:
		case POSIX_ACE_TYPE_OTHER:
			CHECK_BYTE_COUNT_SUBR(8);
			/* 8 reserved bytes */
			COUNT_BYTES_SUBR(8);
			break;

		case POSIX_ACE_TYPE_USER:
			CHECK_BYTE_COUNT_SUBR(4);
			proto_tree_add_item(tr, hf_smb_posix_ace_perm_uid, tvb, offset, 4, ENC_LITTLE_ENDIAN);
			COUNT_BYTES_SUBR(4);

			CHECK_BYTE_COUNT_SUBR(4);
			/* 4 reserved bytes */
			COUNT_BYTES_SUBR(4);
			break;

		case POSIX_ACE_TYPE_GROUP:
			CHECK_BYTE_COUNT_SUBR(4);
			proto_tree_add_item(tr, hf_smb_posix_ace_perm_gid, tvb, offset, 4, ENC_LITTLE_ENDIAN);
			COUNT_BYTES_SUBR(4);

			CHECK_BYTE_COUNT_SUBR(4);
			/* 4 reserved bytes */
			COUNT_BYTES_SUBR(4);
			break;
		default:
			expert_add_info(pinfo, type_item, &ei_smb_posix_ace_type);
			CHECK_BYTE_COUNT_SUBR(8);
			/* skip 8 bytes */
			COUNT_BYTES_SUBR(8);
		}

		proto_item_set_len(it, offset-old_offset);
	}

	*trunc = FALSE;
	return offset;
}

static int
dissect_qspi_unix_xattr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
		  int offset, guint16 *bcp _U_, gboolean *trunc)
{
	proto_tree_add_expert(tree, pinfo, &ei_smb_not_implemented, tvb, offset, 0);

	*trunc = FALSE;
	return offset;
}

static int
dissect_qspi_unix_attr_flags(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
		  int offset, guint16 *bcp _U_, gboolean *trunc)
{
	proto_tree_add_expert(tree, pinfo, &ei_smb_not_implemented, tvb, offset, 0);

	*trunc = FALSE;
	return offset;
}

static int
dissect_qpi_unix_permissions(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
		  int offset, guint16 *bcp _U_, gboolean *trunc)
{
	proto_tree_add_expert(tree, pinfo, &ei_smb_not_implemented, tvb, offset, 0);

	*trunc = FALSE;
	return offset;
}

static int
dissect_qspi_unix_lock(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
		  int offset, guint16 *bcp _U_, gboolean *trunc)
{
	proto_tree_add_expert(tree, pinfo, &ei_smb_not_implemented, tvb, offset, 0);

	*trunc = FALSE;
	return offset;
}

static int
dissect_qspi_unix_open(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
		  int offset, guint16 *bcp _U_, gboolean *trunc)
{
	proto_tree_add_expert(tree, pinfo, &ei_smb_not_implemented, tvb, offset, 0);

	*trunc = FALSE;
	return offset;
}

static int
dissect_qspi_unix_unlink(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
		  int offset, guint16 *bcp _U_, gboolean *trunc)
{
	proto_tree_add_expert(tree, pinfo, &ei_smb_not_implemented, tvb, offset, 0);

	*trunc = FALSE;
	return offset;
}

/* SMB_FIND_FILE_UNIX_INFO2 */

#if 0
static const true_false_string tfs_i2f_secure_delete = {
	"File should be erased such that the data is not recoverable",
	"File need not be erased such that the data is not recoverable"
};
static const true_false_string tfs_i2f_enable_undelete = {
	"File should opt-in to a server-specific deletion recovery scheme",
	"File should not opt-in to a server-specific deletion recovery scheme"
};
static const true_false_string tfs_i2f_synchronous = {
	"I/O to this file should be performed synchronously",
	"I/O to this file need not be performed synchronously"
};
static const true_false_string tfs_i2f_immutable = {
	"NO changes can be made to this file",
	"Changes can be made to this file if permissions allow it"
};
static const true_false_string tfs_i2f_append_only = {
	"Only appends can be made to this file",
	"Writes can be made atop existing data in this file"
};
static const true_false_string tfs_i2f_do_not_backup = {
	"Backup programs should ignore this file",
	"Backup programs should not ignore this file"
};
static const true_false_string tfs_i2f_no_update_atime = {
	"The server is not required to update the last access time on this file",
	"The server is required to update the last access time on this file"
};
static const true_false_string tfs_i2f_hidden = {
	"User interface programs may ignore this file",
	"User interface programs should not ignore this file based solely on this flag"
};
#endif

static int
dissect_unix_info2_file_flags(tvbuff_t *tvb, proto_tree *parent_tree, int offset, int hf)
{
	static const int * flags[] = {
		&hf_smb_unix_info2_file_flags_secure_delete,
		&hf_smb_unix_info2_file_flags_enable_undelete,
		&hf_smb_unix_info2_file_flags_synchronous,
		&hf_smb_unix_info2_file_flags_immutable,
		&hf_smb_unix_info2_file_flags_append_only,
		&hf_smb_unix_info2_file_flags_do_not_backup,
		&hf_smb_unix_info2_file_flags_no_update_atime,
		&hf_smb_unix_info2_file_flags_hidden,
		NULL
	};

	proto_tree_add_bitmask(parent_tree, tvb, offset, hf, ett_smb_info2_file_flags, flags, ENC_LITTLE_ENDIAN);
	offset += 4;

	return offset;
}

static int
dissect_qspi_unix_info2(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree,
		  int offset, guint16 *bcp, gboolean *trunc)
{
	/* End of file (file size) */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_unix_file_size, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* Number of bytes (or blocks?  The SNIA spec for UNIX basic
	   info says "bytes", the Samba page for this says "blocks") */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_unix_file_num_bytes, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* Last status change */
	CHECK_BYTE_COUNT_SUBR(8);
	offset = dissect_nt_64bit_time(tvb, tree, offset, hf_smb_unix_file_last_status);
	*bcp -= 8;

	/* Last access time */
	CHECK_BYTE_COUNT_SUBR(8);
	offset = dissect_nt_64bit_time(tvb, tree, offset, hf_smb_unix_file_last_access);
	*bcp -= 8;

	/* Last modification time */
	CHECK_BYTE_COUNT_SUBR(8);
	offset = dissect_nt_64bit_time(tvb, tree, offset, hf_smb_unix_file_last_change);
	*bcp -= 8;

	/* File owner uid */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_unix_file_uid, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* File group gid */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_unix_file_gid, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* File type */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_unix_file_type, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* Major device number */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_unix_file_dev_major, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* Minor device number */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_unix_file_dev_minor, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* Unique id */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_unix_file_unique_id, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* Permissions */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_unix_file_permissions, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* Nlinks */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_unix_file_nlinks, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* Creation time */
	CHECK_BYTE_COUNT_SUBR(8);
	offset = dissect_nt_64bit_time(tvb, tree, offset, hf_smb_unix_file_creation_time);
	*bcp -= 8;

	/* File flags */
	CHECK_BYTE_COUNT_SUBR(4);
	offset = dissect_unix_info2_file_flags(tvb, tree, offset, hf_smb_unix_info2_file_flags);
	*bcp -= 4;

	/* File flags mask */
	CHECK_BYTE_COUNT_SUBR(4);
	offset = dissect_unix_info2_file_flags(tvb, tree, offset, hf_smb_unix_info2_file_flags_mask);
	*bcp -= 4;

	*trunc = FALSE;
	return offset;
}

/* this dissects the SMB_QUERY_FILE_NETWORK_OPEN_INFO
*/
int
dissect_qfi_SMB_FILE_NETWORK_OPEN_INFO(tvbuff_t *tvb,
    packet_info *pinfo, proto_tree *tree,
    int offset, guint16 *bcp, gboolean *trunc)
{

	offset = dissect_smb_standard_8byte_timestamps(tvb, pinfo, tree, offset, bcp, trunc);
	if (*trunc) {
		return offset;
	}

	/* allocation size */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_alloc_size64, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* end of file */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_end_of_file, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* File Attributes */
	CHECK_BYTE_COUNT_SUBR(4);
	offset = dissect_file_ext_attr(tvb, tree, offset);
	*bcp -= 4;

	/* 4 reserved bytes */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 4, ENC_NA);
	COUNT_BYTES_SUBR(4);

	*trunc = FALSE;
	return offset;
}

/* this dissects the SMB_FILE_ATTRIBUTE_TAG_INFO
*/
int
dissect_qfi_SMB_FILE_ATTRIBUTE_TAG_INFO(tvbuff_t *tvb,
    packet_info *pinfo _U_, proto_tree *tree,
    int offset, guint16 *bcp, gboolean *trunc)
{
	/* attribute */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_attribute, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* reparse tag */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_reparse_tag, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	*trunc = FALSE;
	return offset;
}

/* this dissects the SMB_SET_FILE_DISPOSITION_INFO
   as described in 4.2.19.2
*/
static int
dissect_4_2_19_2(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree,
    int offset, guint16 *bcp, gboolean *trunc)
{
	/* marked for deletion? */
	CHECK_BYTE_COUNT_SUBR(1);
	proto_tree_add_item(tree, hf_smb_t2_marked_for_deletion, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(1);

	*trunc = FALSE;
	return offset;
}

/* Set File Rename Info */

static const true_false_string tfs_smb_replace = {
	"Remove target file if it exists",
	"Do NOT remove target file if it exists",
};

static int
dissect_rename_info(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree,
		    int offset, guint16 *bcp, gboolean *trunc, smb_info_t *si)
{
	const char *fn;
	guint32     target_name_len;
	int         fn_len;

	DISSECTOR_ASSERT(si);

	/* Replace flag */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_replace, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* Root directory handle */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_root_dir_handle, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* Target name length */
	CHECK_BYTE_COUNT_SUBR(4);
	target_name_len = tvb_get_letohl(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_target_name_len, tvb, offset, 4, target_name_len);
	COUNT_BYTES_SUBR(4);

	/* Target name */
	fn_len = target_name_len;
	fn = get_unicode_or_ascii_string(
		tvb, &offset, si->unicode, &fn_len, FALSE, TRUE, bcp);

	CHECK_STRING_SUBR(fn);
	proto_tree_add_string(
		tree, hf_smb_target_name, tvb, offset, fn_len, fn);
	COUNT_BYTES_SUBR(fn_len);

	*trunc = FALSE;
	return offset;
}

static int
dissect_disposition_info(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree,
		    int offset, guint16 *bcp, gboolean *trunc, smb_info_t *si)
{
#if 0
	const char *fn;
	guint32     target_name_len;*/
	int         fn_len;
#endif

	DISSECTOR_ASSERT(si);

	/* Disposition flags */
	CHECK_BYTE_COUNT_SUBR(1);
	proto_tree_add_item(tree, hf_smb_disposition_delete_on_close, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(1);

	*trunc = FALSE;
	return offset;
}

int
dissect_sfi_SMB_FILE_PIPE_INFO(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree,
		    int offset, guint16 *bcp, gboolean *trunc)
{
	/* pipe info flag */
	CHECK_BYTE_COUNT_SUBR(1);
	proto_tree_add_item(tree, hf_smb_pipe_info_flag, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(1);

	*trunc = FALSE;
	return offset;
}

/*dissect the data block for TRANS2_QUERY_PATH_INFORMATION and
  TRANS2_QUERY_FILE_INFORMATION*/
static int
dissect_qpi_loi_vals(tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree,
    proto_item *item, int offset, guint16 *bcp, smb_info_t *si)
{
	gboolean    trunc = FALSE;

	if (!*bcp) {
		return offset;
	}

	DISSECTOR_ASSERT(si);

	switch(si->info_level) {
	case 1:		/*Info Standard*/
		offset = dissect_qsfi_SMB_INFO_STANDARD(tvb, pinfo, tree, offset, bcp,
		    &trunc);
		break;

	case 2:		/*Info Query EA Size*/
		offset = dissect_qfi_SMB_INFO_QUERY_EA_SIZE(tvb, pinfo, tree, offset, bcp,
		    &trunc);
		break;
	case 3:		/*Info Query EAs From List*/
	case 4:		/*Info Query All EAs*/
		offset = dissect_4_2_16_2(tvb, pinfo, tree, offset, bcp,
		    &trunc);
		break;
	case 6:		/*Info Is Name Valid*/
		offset = dissect_4_2_16_3(tvb, pinfo, tree, offset, bcp,
		    &trunc, si);
		break;
	case 0x0101:	/*Query File Basic Info*/
	case 1004:	/* SMB_FILE_BASIC_INFORMATION */
		offset = dissect_4_2_16_4(tvb, pinfo, tree, offset, bcp,
		    &trunc);
		break;
	case 0x0102:	/*Query File Standard Info*/
	case 1005:	/* SMB_FILE_STANDARD_INFORMATION */
		offset = dissect_qfi_SMB_FILE_STANDARD_INFO(tvb, pinfo, tree, offset, bcp,
		    &trunc);
		break;
	case 1006:	/* SMB_FILE_INTERNAL_INFORMATION */
		offset = dissect_qfi_SMB_FILE_INTERNAL_INFO(tvb, pinfo, tree, offset, bcp,
		    &trunc);
		break;
	case 0x0103:	/*Query File EA Info*/
	case 1007:	/* SMB_FILE_EA_INFORMATION */
		offset = dissect_qfi_SMB_FILE_EA_INFO(tvb, pinfo, tree, offset, bcp,
		    &trunc);
		break;
	case 0x0104:	/*Query File Name Info*/
	case 1009:	/* SMB_FILE_NAME_INFORMATION */
		offset = dissect_qfi_SMB_FILE_NAME_INFO(tvb, pinfo, tree, offset, bcp,
		    &trunc, si->unicode);
		break;
	case 1014:	/* SMB_FILE_POSITION_INFORMATION */
		offset = dissect_qsfi_SMB_FILE_POSITION_INFO(tvb, pinfo, tree, offset, bcp,
		    &trunc);
		break;
	case 1016:	/* SMB_FILE_MODE_INFORMATION */
		offset = dissect_qsfi_SMB_FILE_MODE_INFO(tvb, pinfo, tree, offset, bcp,
		    &trunc);
		break;
	case 1017:	/* SMB_FILE_ALIGNMENT_INFORMATION */
		offset = dissect_qfi_SMB_FILE_ALIGNMENT_INFO(tvb, pinfo, tree, offset, bcp,
		    &trunc);
		break;
	case 0x0107:	/*Query File All Info*/
	case 1018:	/* SMB_FILE_ALL_INFORMATION */
		offset = dissect_qfi_SMB_FILE_ALL_INFO(tvb, pinfo, tree, offset, bcp,
		    &trunc, si);
		break;
	case 1019:	/* SMB_FILE_ALLOCATION_INFORMATION */
		offset = dissect_qsfi_SMB_FILE_ALLOCATION_INFO(tvb, pinfo, tree, offset, bcp,
		    &trunc);
		break;
	case 1020:	/* SMB_FILE_ENDOFFILE_INFORMATION */
		offset = dissect_qsfi_SMB_FILE_ENDOFFILE_INFO(tvb, pinfo, tree, offset, bcp,
		    &trunc);
		break;
	case 0x0108:	/*Query File Alt File Info*/
	case 1021:	/* SMB_FILE_ALTERNATE_NAME_INFORMATION */
		offset = dissect_qfi_SMB_FILE_NAME_INFO(tvb, pinfo, tree, offset, bcp,
		    &trunc, si->unicode);
		break;
	case 1022:	/* SMB_FILE_STREAM_INFORMATION */
		si->unicode = TRUE;
		/* FALLTHRU */
	case 0x0109:	/*Query File Stream Info*/
		offset = dissect_qfi_SMB_FILE_STREAM_INFO(tvb, pinfo, tree, offset, bcp,
		    &trunc, si->unicode);
		break;
	case 0x010b:	/*Query File Compression Info*/
	case 1028:	/* SMB_FILE_COMPRESSION_INFORMATION */
		offset = dissect_qfi_SMB_FILE_COMPRESSION_INFO(tvb, pinfo, tree, offset, bcp,
		    &trunc);
		break;
	case 1034:     /* SMB_FILE_NETWORK_OPEN_INFO */
		offset = dissect_qfi_SMB_FILE_NETWORK_OPEN_INFO(tvb, pinfo, tree, offset, bcp, &trunc);
		break;
	case 1035:     /* SMB_FILE_ATTRIBUTE_TAG_INFO */
		offset = dissect_qfi_SMB_FILE_ATTRIBUTE_TAG_INFO(tvb, pinfo, tree, offset, bcp, &trunc);
		break;
	case 0x0200:	/* Query File Unix Basic*/
		offset = dissect_4_2_16_12(tvb, pinfo, tree, offset, bcp,
					   &trunc);
		break;
	case 0x0201:	/* Query File Unix Link*/
		offset = dissect_4_2_16_13(tvb, pinfo, tree, offset, bcp,
					   &trunc, si);
		break;
	case 0x0202:	/* Query File Unix HardLink*/
		/* XXX add this from the SNIA doc */
		break;
	case 0x0204:	/* Query File Unix ACL*/
		offset = dissect_qspi_unix_acl(tvb, pinfo, tree, offset, bcp,
					   &trunc);
		break;
	case 0x0205:	/* Query File Unix XATTR*/
		offset = dissect_qspi_unix_xattr(tvb, pinfo, tree, offset, bcp,
					   &trunc);
		break;
	case 0x0206:	/* Query File Unix Attr Flags*/
		offset = dissect_qspi_unix_attr_flags(tvb, pinfo, tree, offset, bcp,
					   &trunc);
		break;
	case 0x0207:	/* Query File Unix Permissions*/
		offset = dissect_qpi_unix_permissions(tvb, pinfo, tree, offset, bcp,
					   &trunc);
		break;
	case 0x0208:	/* Query File Unix Lock*/
		offset = dissect_qspi_unix_lock(tvb, pinfo, tree, offset, bcp,
					   &trunc);
		break;
	case 0x020b:	/* Query File Unix Info2*/
		offset = dissect_qspi_unix_info2(tvb, pinfo, tree, offset, bcp,
					   &trunc);
		break;

	default:
		proto_tree_add_expert(tree, pinfo, &ei_smb_info_level_unknown, tvb, offset, *bcp);
		offset += *bcp;
		*bcp = 0;
		trunc = FALSE;
		break;
	}

	if (trunc) {
		expert_add_info(pinfo, item, &ei_smb_mal_information_level);
	}
	return offset;
}

/*dissect the data block for TRANS2_SET_PATH_INFORMATION and
  TRANS2_SET_FILE_INFORMATION*/
static int
dissect_spi_loi_vals(tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree,
    proto_item *item, int offset, guint16 *bcp, smb_info_t *si)
{
	gboolean    trunc;

	if (!*bcp) {
		return offset;
	}

	DISSECTOR_ASSERT(si);

	switch(si->info_level) {
	case 1:		/*Info Standard*/
		offset = dissect_qsfi_SMB_INFO_STANDARD(tvb, pinfo, tree, offset, bcp,
		    &trunc);
		break;
	case 2:		/*Info Set EAs*/
		offset = dissect_4_2_16_2(tvb, pinfo, tree, offset, bcp,
		    &trunc);
		break;
	case 4:		/*Info Query All EAs - not in [MS-CIFS]*/
		offset = dissect_4_2_16_2(tvb, pinfo, tree, offset, bcp,
		    &trunc);
		break;
	case 0x0101:	/*Set File Basic Info*/
	case 1004:	/* SMB_FILE_BASIC_INFORMATION */
		offset = dissect_4_2_16_4(tvb, pinfo, tree, offset, bcp,
		    &trunc);
		break;
	case 0x0102:	/*Set File Disposition Info*/
		offset = dissect_4_2_19_2(tvb, pinfo, tree, offset, bcp,
		    &trunc);
		break;
	case 0x0103:	/*Set File Allocation Info*/
	case 1019:	/* Set File Allocation Information */
		offset = dissect_qsfi_SMB_FILE_ALLOCATION_INFO(tvb, pinfo, tree, offset, bcp,
		    &trunc);
		break;
	case 0x0104:	/*Set End Of File Info*/
	case 1020:	/* SMB_FILE_ENDOFFILE_INFORMATION */
		offset = dissect_qsfi_SMB_FILE_ENDOFFILE_INFO(tvb, pinfo, tree, offset, bcp,
		    &trunc);
		break;
	case 0x0200:	/*Set File Unix Basic.  Same as query. */
		offset = dissect_4_2_16_12(tvb, pinfo, tree, offset, bcp,
		    &trunc);
		break;
	case 0x0201:	/*Set File Unix Link.  Same as query. */
		offset = dissect_4_2_16_13(tvb, pinfo, tree, offset, bcp,
		    &trunc, si);
		break;
	case 0x0202:	/*Set File Unix HardLink.  Same as link query. */
		offset = dissect_4_2_16_13(tvb, pinfo, tree, offset, bcp,
		    &trunc, si);
		break;
	case 0x0204:	/* Set File Unix ACL*/
		offset = dissect_qspi_unix_acl(tvb, pinfo, tree, offset, bcp,
					   &trunc);
		break;
	case 0x0205:	/* Set File Unix XATTR*/
		offset = dissect_qspi_unix_xattr(tvb, pinfo, tree, offset, bcp,
					   &trunc);
		break;
	case 0x0206:	/* Set File Unix Attr Flags*/
		offset = dissect_qspi_unix_attr_flags(tvb, pinfo, tree, offset, bcp,
					   &trunc);
		break;
	case 0x0208:	/* Set File Unix Lock*/
		offset = dissect_qspi_unix_lock(tvb, pinfo, tree, offset, bcp,
					   &trunc);
		break;
	case 0x0209:	/* Set File Unix Open*/
		offset = dissect_qspi_unix_open(tvb, pinfo, tree, offset, bcp,
					   &trunc);
		break;
	case 0x020a:	/* Set File Unix Unlink*/
		offset = dissect_qspi_unix_unlink(tvb, pinfo, tree, offset, bcp,
					   &trunc);
		break;
	case 0x020b:	/* Set File Unix Info2*/
		offset = dissect_qspi_unix_info2(tvb, pinfo, tree, offset, bcp,
					   &trunc);
		break;
	case 1010:	/* Set File Rename */
		offset = dissect_rename_info(tvb, pinfo, tree, offset, bcp,
		    &trunc, si);
		break;
	case 1013: /* Set Disposition Information */
		offset = dissect_disposition_info(tvb, pinfo, tree, offset, bcp,
		    &trunc, si);
		break;
	case 1014:	/* SMB_FILE_POSITION_INFORMATION */
		offset = dissect_qsfi_SMB_FILE_POSITION_INFO(tvb, pinfo, tree, offset, bcp,
		    &trunc);
		break;
	case 1016:	/* SMB_FILE_MODE_INFORMATION */
		offset = dissect_qsfi_SMB_FILE_MODE_INFO(tvb, pinfo, tree, offset, bcp,
		    &trunc);
		break;
	case 1023: /* Set Pipe Info */
		offset = dissect_sfi_SMB_FILE_PIPE_INFO(tvb, pinfo, tree, offset, bcp,
		    &trunc);
		break;
	case 1025:
	case 1029:
	case 1032:
	case 1039:
	case 1040:
		/* XXX: TODO, extra levels discovered by tridge */
		proto_tree_add_expert(tree, pinfo, &ei_smb_info_level_not_understood, tvb, offset, *bcp);
		offset += *bcp;
		*bcp = 0;
		trunc = FALSE;
		break;

	default:
		proto_tree_add_expert(tree, pinfo, &ei_smb_info_level_unknown, tvb, offset, *bcp);
		offset += *bcp;
		*bcp = 0;
		trunc = FALSE;
		break;
	}

	if (trunc) {
		expert_add_info(pinfo, item, &ei_smb_mal_information_level);
	}
	return offset;
}


static const true_false_string tfs_quota_flags_deny_disk = {
	"DENY DISK SPACE for users exceeding quota limit",
	"Do NOT deny disk space for users exceeding quota limit"
};
static const true_false_string tfs_quota_flags_log_limit = {
	"LOG EVENT when a user exceeds their QUOTA LIMIT",
	"Do NOT log event when a user exceeds their quota limit"
};
static const true_false_string tfs_quota_flags_log_warning = {
	"LOG EVENT when a user exceeds their WARNING LEVEL",
	"Do NOT log event when a user exceeds their warning level"
};
static const true_false_string tfs_quota_flags_enabled = {
	"Quotas are ENABLED of this fs",
	"Quotas are NOT enabled on this fs"
};
static void
dissect_quota_flags(tvbuff_t *tvb, proto_tree *parent_tree, int offset)
{
	static const int *mask[] = {
		&hf_smb_quota_flags_deny_disk,
		&hf_smb_quota_flags_log_warning,
		&hf_smb_quota_flags_log_limit,
		&hf_smb_quota_flags_enabled,
		NULL
	};

	proto_tree_add_bitmask(parent_tree, tvb, offset, hf_smb_quota_flags,
							ett_smb_quotaflags, mask, ENC_NA);
}

int
dissect_nt_quota(tvbuff_t *tvb, proto_tree *tree, int offset, guint16 *bcp)
{
	/* first 24 bytes are unknown */
	CHECK_BYTE_COUNT_TRANS_SUBR(24);
	proto_tree_add_item(tree, hf_smb_unknown, tvb,
		    offset, 24, ENC_NA);
	COUNT_BYTES_TRANS_SUBR(24);

	/* number of bytes for quota warning */
	CHECK_BYTE_COUNT_TRANS_SUBR(8);
	proto_tree_add_item(tree, hf_smb_soft_quota_limit, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_TRANS_SUBR(8);

	/* number of bytes for quota limit */
	CHECK_BYTE_COUNT_TRANS_SUBR(8);
	proto_tree_add_item(tree, hf_smb_hard_quota_limit, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_TRANS_SUBR(8);

	/* one byte of quota flags */
	CHECK_BYTE_COUNT_TRANS_SUBR(1);
	dissect_quota_flags(tvb, tree, offset);
	COUNT_BYTES_TRANS_SUBR(1);

	/* these 7 bytes are unknown */
	CHECK_BYTE_COUNT_TRANS_SUBR(7);
	proto_tree_add_item(tree, hf_smb_unknown, tvb,
		    offset, 7, ENC_NA);
	COUNT_BYTES_TRANS_SUBR(7);

	return offset;
}

static int
dissect_sfsi_request(tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree,
    int offset, guint16 *bcp, smb_info_t *si)
{
	if (!*bcp) {
		return offset;
	}

	DISSECTOR_ASSERT(si);

	switch(si->info_level) {
	case 0x203: /* REQUEST_TRANSPORT_ENCRYPTION */ {
		proto_item *blob_item;
		tvbuff_t *blob_tvb;
		proto_tree *blob_tree;

		/* security blob */
		blob_item = proto_tree_add_item(tree, hf_smb_security_blob,
						tvb, offset,
						tvb_reported_length_remaining(tvb, offset),
						ENC_NA);

		/* As an optimization, because Windows is perverse,
		   we check to see if NTLMSSP is the first part of the
		   blob, and if so, call the NTLMSSP dissector,
		   otherwise we call the GSS-API dissector. This is because
		   Windows can request RAW NTLMSSP, but will happily handle
		   a client that wraps NTLMSSP in SPNEGO
		*/

		blob_tree = proto_item_add_subtree(blob_item,
						   ett_smb_secblob);

		blob_tvb = tvb_new_subset_remaining(tvb, offset);

		if (tvb_strneql(blob_tvb, 0, "NTLMSSP", 7) == 0) {
			call_dissector(ntlmssp_handle, blob_tvb, pinfo, blob_tree);
		} else {
			call_dissector(gssapi_handle, blob_tvb, pinfo, blob_tree);
		}

		offset += tvb_reported_length_remaining(tvb, offset);
		*bcp = 0;
		break;
	}
	case 1006:	/* QUERY_FS_QUOTA_INFO */
		offset = dissect_nt_quota(tvb, tree, offset, bcp);
		break;
	}

	return offset;
}

static int
dissect_sfsi_response(tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree,
    int offset, guint16 *bcp, smb_info_t *si)
{
	if (!*bcp) {
		return offset;
	}

	DISSECTOR_ASSERT(si);

	switch(si->info_level) {
	case 0x203: /* REQUEST_TRANSPORT_ENCRYPTION */ {
		proto_item *blob_item;
		tvbuff_t *blob_tvb;
		proto_tree *blob_tree;

		/* security blob */
		blob_item = proto_tree_add_item(tree, hf_smb_security_blob,
						tvb, offset,
						tvb_reported_length_remaining(tvb, offset),
						ENC_NA);

		/* As an optimization, because Windows is perverse,
		   we check to see if NTLMSSP is the first part of the
		   blob, and if so, call the NTLMSSP dissector,
		   otherwise we call the GSS-API dissector. This is because
		   Windows can request RAW NTLMSSP, but will happily handle
		   a client that wraps NTLMSSP in SPNEGO
		*/

		blob_tree = proto_item_add_subtree(blob_item,
						   ett_smb_secblob);

		blob_tvb = tvb_new_subset_remaining(tvb, offset);

		if (tvb_strneql(blob_tvb, 0, "NTLMSSP", 7) == 0) {
			call_dissector(ntlmssp_handle, blob_tvb, pinfo, blob_tree);
		} else {
			call_dissector(gssapi_handle, blob_tvb, pinfo, blob_tree);
		}

		offset += tvb_reported_length_remaining(tvb, offset);
		*bcp = 0;
		break;
	}
	case 1006:	/* QUERY_FS_QUOTA_INFO */
		/* nothing */
		break;
	}

	return offset;
}

static int
dissect_transaction2_request_data(tvbuff_t *tvb, packet_info *pinfo,
    proto_tree *parent_tree, int offset, int subcmd, guint16 dc, smb_info_t *si)
{
	proto_item *item;
	proto_tree *tree;

	DISSECTOR_ASSERT(si);

	tree = proto_tree_add_subtree_format(parent_tree, tvb, offset, dc,
				ett_smb_transaction_data, &item, "%s Data",
				val_to_str_ext(subcmd, &trans2_cmd_vals_ext,
					       "Unknown (0x%02x)"));

	switch(subcmd) {
	case 0x0000:	/*TRANS2_OPEN2*/
		/* XXX don't know how to decode FEAList */
		break;
	case 0x0001:	/*TRANS2_FIND_FIRST2*/
		/* XXX don't know how to decode FEAList */
		break;
	case 0x0002:	/*TRANS2_FIND_NEXT2*/
		/* XXX don't know how to decode FEAList */
		break;
	case 0x0003:	/*TRANS2_QUERY_FS_INFORMATION*/
		/* no data field in this request */
		break;
	case 0x0004:	/* TRANS2_SET_FS_INFORMATION */
		offset = dissect_sfsi_request(tvb, pinfo, tree, offset, &dc, si);
		break;
	case 0x0005:	/*TRANS2_QUERY_PATH_INFORMATION*/
		/* no data field in this request */
		/*
		 * XXX - "Microsoft Networks SMB File Sharing Protocol
		 * Extensions Version 3.0, Document Version 1.11,
		 * July 19, 1990" says there may be "Additional
		 * FileInfoLevel dependent information" here.
		 *
		 * Was that just a cut-and-pasteo?
		 * TRANS2_SET_PATH_INFORMATION *does* have that information
		 * here.
		 */
		break;
	case 0x0006:	/*TRANS2_SET_PATH_INFORMATION*/
		offset = dissect_spi_loi_vals(tvb, pinfo, tree, item, offset, &dc, si);
		break;
	case 0x0007:	/*TRANS2_QUERY_FILE_INFORMATION*/
		/* no data field in this request */
		/*
		 * XXX - "Microsoft Networks SMB File Sharing Protocol
		 * Extensions Version 3.0, Document Version 1.11,
		 * July 19, 1990" says there may be "Additional
		 * FileInfoLevel dependent information" here.
		 *
		 * Was that just a cut-and-pasteo?
		 * TRANS2_SET_FILE_INFORMATION *does* have that information
		 * here.
		 */
		break;
	case 0x0008:	/*TRANS2_SET_FILE_INFORMATION*/
		offset = dissect_spi_loi_vals(tvb, pinfo, tree, item, offset, &dc, si);
		break;
	case 0x0009:	/*TRANS2_FSCTL*/
		/*XXX don't know how to decode this yet */

		/*
		 * XXX - "Microsoft Networks SMB File Sharing Protocol
		 * Extensions Version 3.0, Document Version 1.11,
		 * July 19, 1990" says this this contains a
		 * "File system specific data block".  (That means we
		 * may not be able to dissect it in any case.)
		 */
		break;
	case 0x000a:	/*TRANS2_IOCTL2*/
		/*XXX don't know how to decode this yet */

		/*
		 * XXX - "Microsoft Networks SMB File Sharing Protocol
		 * Extensions Version 3.0, Document Version 1.11,
		 * July 19, 1990" says this this contains a
		 * "Device/function specific data block".  (That
		 * means we may not be able to dissect it in any case.)
		 */
		break;
	case 0x000b:	/*TRANS2_FIND_NOTIFY_FIRST*/
		/*XXX don't know how to decode this yet */

		/*
		 * XXX - "Microsoft Networks SMB File Sharing Protocol
		 * Extensions Version 3.0, Document Version 1.11,
		 * July 19, 1990" says this this contains "additional
		 * level dependent match data".
		 */
		break;
	case 0x000c:	/*TRANS2_FIND_NOTIFY_NEXT*/
		/*XXX don't know how to decode this yet */

		/*
		 * XXX - "Microsoft Networks SMB File Sharing Protocol
		 * Extensions Version 3.0, Document Version 1.11,
		 * July 19, 1990" says this this contains "additional
		 * level dependent monitor information".
		 */
		break;
	case 0x000d:	/*TRANS2_CREATE_DIRECTORY*/
		/* XXX optional FEAList, unknown what FEAList looks like*/
		break;
	case 0x000e:	/*TRANS2_SESSION_SETUP*/
		/*XXX don't know how to decode this yet */
		break;
	case 0x0010:	/*TRANS2_GET_DFS_REFERRAL*/
		/* no data field in this request */
		break;
	case 0x0011:	/*TRANS2_REPORT_DFS_INCONSISTENCY*/
		offset = dissect_dfs_inconsistency_data(tvb, pinfo, tree, offset, &dc, si);
		break;
	}

	/* ooops there were data we didn't know how to process */
	if (dc != 0) {
		proto_tree_add_item(tree, hf_smb_unknown, tvb, offset, dc, ENC_NA);
		offset += dc;
	}

	return offset;
}


static void
dissect_trans_data(tvbuff_t *s_tvb, tvbuff_t *p_tvb, tvbuff_t *d_tvb,
    proto_tree *tree)
{
	int   i;
	int   offset;
	guint length;

	/*
	 * Show the setup words.
	 */
	if (s_tvb != NULL) {
		length = tvb_reported_length(s_tvb);
		for (i = 0, offset = 0; length >= 2;
		    i++, offset += 2, length -= 2) {
			/*
			 * XXX - add a setup word filterable field?
			 */
			proto_tree_add_uint_format(tree, hf_smb_trans_data_setup_word, s_tvb, offset, 2,
			    tvb_get_letohs(s_tvb, offset), "Setup Word %d: 0x%04x", i, tvb_get_letohs(s_tvb, offset));
		}
	}

	/*
	 * Show the parameters, if any.
	 */
	if (p_tvb != NULL) {
		length = tvb_reported_length(p_tvb);
		if (length != 0) {
			proto_tree_add_item(tree, hf_smb_trans_data_parameters, p_tvb, 0, length, ENC_NA);
		}
	}

	/*
	 * Show the data, if any.
	 */
	if (d_tvb != NULL) {
		length = tvb_reported_length(d_tvb);
		if (length != 0) {
			proto_tree_add_item(tree, hf_smb_trans_data, d_tvb, 0, length, ENC_NA);
		}
	}
}

/* This routine handles the following 4 calls
   Transaction  0x25
   Transaction Secondary 0x26
   Transaction2 0x32
   Transaction2 Secondary 0x33
*/
static int
dissect_transaction_request(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	guint8                wc, sc = 0;
	int                   so     = offset;
	int                   sl     = 0;
	int                   spo    = offset;
	int                   spc    = 0;
	guint16               od     = 0, po = 0, pc = 0, dc = 0, pd, dd = 0;
	int                   subcmd = -1;
	guint32               to;
	int                   an_len;
	const char           *an     = NULL;
	smb_transact2_info_t *t2i;
	smb_transact_info_t  *tri;
	guint16               bc;
	int                   padcnt;
	gboolean              dissected_trans;

	DISSECTOR_ASSERT(si);

	WORD_COUNT;

	if (wc == 8) {
		/*secondary client request*/

		/* total param count, only a 16bit integer here*/
		proto_tree_add_uint(tree, hf_smb_total_param_count, tvb, offset, 2, tvb_get_letohs(tvb, offset));
		offset += 2;

		/* total data count , only 16bit integer here*/
		proto_tree_add_uint(tree, hf_smb_total_data_count, tvb, offset, 2, tvb_get_letohs(tvb, offset));
		offset += 2;

		/* param count */
		pc = tvb_get_letohs(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_param_count16, tvb, offset, 2, pc);
		offset += 2;

		/* param offset */
		po = tvb_get_letohs(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_param_offset16, tvb, offset, 2, po);
		offset += 2;

		/* param disp */
		pd = tvb_get_letohs(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_param_disp16, tvb, offset, 2, pd);
		offset += 2;

		/* data count */
		dc = tvb_get_letohs(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_data_count16, tvb, offset, 2, dc);
		offset += 2;

		/* data offset */
		od = tvb_get_letohs(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_data_offset16, tvb, offset, 2, od);
		offset += 2;

		/* data disp */
		dd = tvb_get_letohs(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_data_disp16, tvb, offset, 2, dd);
		offset += 2;

		if (si->cmd == SMB_COM_TRANSACTION2) {
			guint16 fid;

			/* fid */
			fid = tvb_get_letohs(tvb, offset);
			dissect_smb_fid(tvb, pinfo, tree, offset, 2, fid, FALSE, FALSE, FALSE, si);

			offset += 2;
		}

		/* There are no setup words. */
		so = offset;
		sl = 0;
	} else {
		/* it is not a secondary request */

		/* total param count , only a 16 bit integer here*/
		proto_tree_add_uint(tree, hf_smb_total_param_count, tvb, offset, 2, tvb_get_letohs(tvb, offset));
		offset += 2;

		/* total data count , only 16bit integer here*/
		proto_tree_add_uint(tree, hf_smb_total_data_count, tvb, offset, 2, tvb_get_letohs(tvb, offset));
		offset += 2;

		/* max param count , only 16bit integer here*/
		proto_tree_add_uint(tree, hf_smb_max_param_count, tvb, offset, 2, tvb_get_letohs(tvb, offset));
		offset += 2;

		/* max data count, only 16bit integer here*/
		proto_tree_add_uint(tree, hf_smb_max_data_count, tvb, offset, 2, tvb_get_letohs(tvb, offset));
		offset += 2;

		/* max setup count, only 16bit integer here*/
		proto_tree_add_uint(tree, hf_smb_max_setup_count, tvb, offset, 1, tvb_get_guint8(tvb, offset));
		offset += 1;

		/* reserved byte */
		proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 1, ENC_NA);
		offset += 1;

		/* transaction flags */
		dissect_transaction_flags(tvb, tree, offset);
		offset += 2;

		/* timeout */
		to = tvb_get_letohl(tvb, offset);
		proto_tree_add_uint_format_value(tree, hf_smb_timeout, tvb, offset, 4, to, "%s", smbext20_timeout_msecs_to_str(to));
		offset += 4;

		/* 2 reserved bytes */
		proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 2, ENC_NA);
		offset += 2;

		/* param count */
		pc = tvb_get_letohs(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_param_count16, tvb, offset, 2, pc);
		offset += 2;

		/* param offset */
		po = tvb_get_letohs(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_param_offset16, tvb, offset, 2, po);
		offset += 2;

		/* data count */
		dc = tvb_get_letohs(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_data_count16, tvb, offset, 2, dc);
		offset += 2;

		/* data offset */
		od = tvb_get_letohs(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_data_offset16, tvb, offset, 2, od);
		offset += 2;

		/* data displacement is zero here */
		dd = 0;

		/* setup count */
		sc = tvb_get_guint8(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_setup_count, tvb, offset, 1, sc);
		offset += 1;

		/* reserved byte */
		proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 1, ENC_NA);
		offset += 1;

		/* this is where the setup bytes, if any start */
		so = offset;
		sl = sc*2;

		/* if there were any setup bytes, decode them */
		if (sc) {
			switch(si->cmd) {

			case SMB_COM_TRANSACTION2:
				/* TRANSACTION2 only has one setup word and
				   that is the subcommand code.

				   XXX - except for TRANS2_FSCTL
				   and TRANS2_IOCTL. */
				subcmd = tvb_get_letohs(tvb, offset);
				proto_tree_add_uint(tree, hf_smb_trans2_subcmd,
				    tvb, offset, 2, subcmd);
				col_append_fstr(pinfo->cinfo, COL_INFO, ", %s",
 					    val_to_str_ext(subcmd, &trans2_cmd_vals_ext,
							   "Unknown (0x%02x)"));

				if (!si->unidir) {
					if (!pinfo->fd->flags.visited && si->sip) {
						/*
						 * Allocate a new
						 * smb_transact2_info_t
						 * structure.
						 */
						t2i = (smb_transact2_info_t *)wmem_alloc(wmem_file_scope(), sizeof(smb_transact2_info_t));
						t2i->subcmd = subcmd;
						t2i->info_level = -1;
						t2i->resume_keys = FALSE;
						t2i->name = NULL;
						si->sip->extra_info = t2i;
						si->sip->extra_info_type = SMB_EI_T2I;
					}
				}

				/*
				 * XXX - process TRANS2_FSCTL and
				 * TRANS2_IOCTL setup words here.
				 */
				break;

			case SMB_COM_TRANSACTION:
				/* TRANSACTION setup words processed below */
				break;
			}

			offset += sl;
		}
	}

	BYTE_COUNT;

	if (wc != 8) {
		/* primary request */
		/* name is NULL if transaction2 */
		if (si->cmd == SMB_COM_TRANSACTION) {
			/* Transaction Name */
			an = get_unicode_or_ascii_string(tvb, &offset,
				si->unicode, &an_len, FALSE, FALSE, &bc);
			if (an == NULL)
				goto endofcommand;
			proto_tree_add_string(tree, hf_smb_trans_name, tvb,
				offset, an_len, an);
			COUNT_BYTES(an_len);
		}
	}

	/*
	 * The pipe or mailslot arguments for Transaction start with
	 * the first setup word (or where the first setup word would
	 * be if there were any setup words), and run to the current
	 * offset (which could mean that there aren't any).
	 */
	spo = so;
	spc = offset - spo;

	/* parameters */
	if (po > offset) {
		/* We have some initial padding bytes.
		*/
		padcnt = po-offset;
		if (padcnt > bc)
			padcnt = bc;
		proto_tree_add_item(tree, hf_smb_padding, tvb, offset, padcnt, ENC_NA);
		COUNT_BYTES(padcnt);
	}
	if (pc) {
		CHECK_BYTE_COUNT(pc);
		switch(si->cmd) {

		case SMB_COM_TRANSACTION2:
			/* TRANSACTION2 parameters*/
			offset = dissect_transaction2_request_parameters(tvb,
			    pinfo, tree, offset, subcmd, pc, si);
			bc -= pc;
			break;

		case SMB_COM_TRANSACTION:
			/* TRANSACTION parameters processed below */
			COUNT_BYTES(pc);
			break;
		}
	}

	/* data */
	if (od > offset) {
		/* We have some initial padding bytes.
		*/
		padcnt = od-offset;
		if (padcnt > bc)
			padcnt = bc;
		proto_tree_add_item(tree, hf_smb_padding, tvb, offset, padcnt, ENC_NA);
		COUNT_BYTES(padcnt);
	}
	if (dc) {
		CHECK_BYTE_COUNT(dc);
		switch(si->cmd) {

		case SMB_COM_TRANSACTION2:
			/* TRANSACTION2 data*/
			offset = dissect_transaction2_request_data(tvb, pinfo,
			    tree, offset, subcmd, dc, si);
			bc -= dc;
			break;

		case SMB_COM_TRANSACTION:
			/* TRANSACTION data processed below */
			COUNT_BYTES(dc);
			break;
		}
	}

	/*TRANSACTION request parameters */
	if (si->cmd == SMB_COM_TRANSACTION) {
		/*XXX replace this block with a function and use that one
		     for both requests/responses*/
		if (dd == 0) {
			tvbuff_t *p_tvb, *d_tvb, *s_tvb;
			tvbuff_t *sp_tvb, *pd_tvb;

			if (pc > 0) {
				if (pc>tvb_reported_length_remaining(tvb, po)) {
					p_tvb = tvb_new_subset(tvb, po, tvb_reported_length_remaining(tvb, po), pc);
				} else {
					p_tvb = tvb_new_subset_length(tvb, po, pc);
				}
			} else {
				p_tvb = NULL;
			}
			if (dc > 0) {
				if (dc>tvb_reported_length_remaining(tvb, od)) {
					d_tvb = tvb_new_subset(tvb, od, tvb_reported_length_remaining(tvb, od), dc);
				} else {
					d_tvb = tvb_new_subset_length(tvb, od, dc);
				}
			} else {
				d_tvb = NULL;
			}
			if (sl) {
				if (sl>tvb_reported_length_remaining(tvb, so)) {
					s_tvb = tvb_new_subset(tvb, so, tvb_reported_length_remaining(tvb, so), sl);
				} else {
					s_tvb = tvb_new_subset_length(tvb, so, sl);
				}
			} else {
				s_tvb = NULL;
			}

			if (!si->unidir) {
				if (!pinfo->fd->flags.visited && si->sip) {
					/*
					 * Allocate a new smb_transact_info_t
					 * structure.
					 */
					tri = (smb_transact_info_t *)wmem_alloc(wmem_file_scope(), sizeof(smb_transact_info_t));
					tri->subcmd = -1;
					tri->trans_subcmd = -1;
					tri->function = -1;
					tri->fid = -1;
					tri->lanman_cmd = 0;
					tri->param_descrip = NULL;
					tri->data_descrip = NULL;
					tri->aux_data_descrip = NULL;
					tri->info_level = -1;
					si->sip->extra_info = tri;
					si->sip->extra_info_type = SMB_EI_TRI;
				} else {
					/*
					 * We already filled the structure
					 * in; don't bother doing so again.
					 */
					tri = NULL;
				}
			} else {
				/*
				 * This is a unidirectional message, for
				 * which there will be no reply; don't
				 * bother allocating an "smb_transact_info_t"
				 * structure for it.
				 */
				tri = NULL;
			}
			dissected_trans = FALSE;
			if (an == NULL)
				goto endofcommand;
			if (strncmp("\\PIPE\\", an, 6) == 0) {
				if (tri != NULL)
					tri->subcmd = TRANSACTION_PIPE;

				/*
				 * A tvbuff containing the setup words and
				 * the pipe path.
				 */
				sp_tvb = tvb_new_subset_length(tvb, spo, spc);

				/*
				 * A tvbuff containing the parameters and the
				 * data.
				 */
				pd_tvb = tvb_new_subset_remaining(tvb, po);

				dissected_trans = dissect_pipe_smb(sp_tvb,
				    s_tvb, pd_tvb, p_tvb, d_tvb, an+6, pinfo,
				    top_tree_global, si);

				/* In case we did not see the TreeConnect call,
				   store this TID here as well as a IPC TID
				   so we know that future Read/Writes to this
				   TID is (probably) DCERPC.
				*/
				if (g_hash_table_lookup(si->ct->tid_service, GUINT_TO_POINTER(si->tid))) {
					g_hash_table_remove(si->ct->tid_service, GUINT_TO_POINTER(si->tid));
				}
				g_hash_table_insert(si->ct->tid_service, GUINT_TO_POINTER(si->tid), (void *)TID_IPC);
			} else if (strncmp("\\MAILSLOT\\", an, 10) == 0) {
				if (tri != NULL)
					tri->subcmd = TRANSACTION_MAILSLOT;

				/*
				 * A tvbuff containing the setup words and
				 * the mailslot path.
				 */
				sp_tvb = tvb_new_subset_length(tvb, spo, spc);
				dissected_trans = dissect_mailslot_smb(sp_tvb,
				    s_tvb, d_tvb, an+10, pinfo, top_tree_global, si);
			}
			if (!dissected_trans)
				dissect_trans_data(s_tvb, p_tvb, d_tvb, tree);
		} else {
			col_append_str(pinfo->cinfo, COL_INFO,
					"[transact continuation]");
		}
	}

	END_OF_SMB

	return offset;
}



static int
dissect_4_3_4_1(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree,
    int offset, guint16 *bcp, gboolean *trunc, smb_info_t *si)
{
	int                   fn_len;
	const char           *fn;
	int                   old_offset  = offset;
	proto_item           *item;
	proto_tree           *tree;
	smb_transact2_info_t *t2i;
	gboolean              resume_keys = FALSE;
	guint32               bytes_needed = 0;

	DISSECTOR_ASSERT(si);

	if ((si->sip != NULL) && (si->sip->extra_info_type == SMB_EI_T2I)) {
		t2i = (smb_transact2_info_t *)si->sip->extra_info;
		if (t2i != NULL)
			resume_keys = t2i->resume_keys;
	}

	tree = proto_tree_add_subtree(parent_tree, tvb, offset, *bcp, ett_smb_ff2_data, &item,
		    val_to_str(si->info_level, ff2_il_vals, "Unknown (0x%02x)"));

	/*
	 * Figure out of there are enough bytes to display the whole entry.
	 * This consistes of 22 bytes or 26 bytes if resume_keys, followed
	 * by a length byte and that many chars.
	 */
	bytes_needed = 23 + (resume_keys ? 4 : 0);
	tvb_ensure_bytes_exist(tvb, offset, bytes_needed);

	/* Now, get the length */
	fn_len = tvb_get_guint8(tvb, offset + bytes_needed - 1);
	tvb_ensure_bytes_exist(tvb, offset, bytes_needed + fn_len);

	if (resume_keys) {
		/* resume key */
		CHECK_BYTE_COUNT_SUBR(4);
		proto_tree_add_item(tree, hf_smb_resume, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_SUBR(4);
	}

	/* create time */
	CHECK_BYTE_COUNT_SUBR(4);
	offset = dissect_smb_datetime(tvb, tree, offset,
		hf_smb_create_time,
		hf_smb_create_dos_date, hf_smb_create_dos_time, FALSE);
	*bcp -= 4;

	/* access time */
	CHECK_BYTE_COUNT_SUBR(4);
	offset = dissect_smb_datetime(tvb, tree, offset,
		hf_smb_access_time,
		hf_smb_access_dos_date, hf_smb_access_dos_time, FALSE);
	*bcp -= 4;

	/* last write time */
	CHECK_BYTE_COUNT_SUBR(4);
	offset = dissect_smb_datetime(tvb, tree, offset,
		hf_smb_last_write_time,
		hf_smb_last_write_dos_date, hf_smb_last_write_dos_time, FALSE);
	*bcp -= 4;

	/* data size */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_data_size, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* allocation size */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_alloc_size, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* File Attributes */
	CHECK_BYTE_COUNT_SUBR(2);
	offset = dissect_file_attributes(tvb, tree, offset);
	*bcp -= 2;

	/* file name len */
	CHECK_BYTE_COUNT_SUBR(1);
	fn_len = tvb_get_guint8(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_file_name_len, tvb, offset, 1, fn_len);
	COUNT_BYTES_SUBR(1);
	if (si->unicode)
		fn_len += 2;	/* include terminating '\0' */
	else
		fn_len++;	/* include terminating '\0' */

	/* file name */
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len, FALSE, TRUE, bcp);
	CHECK_STRING_SUBR(fn);
	proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
		fn);
	COUNT_BYTES_SUBR(fn_len);

	col_append_fstr(pinfo->cinfo, COL_INFO, " %s",
		    format_text(fn, strlen(fn)));

	proto_item_append_text(item, " File: %s", format_text(fn, strlen(fn)));
	proto_item_set_len(item, offset-old_offset);

	*trunc = FALSE;
	return offset;
}

static int
dissect_4_3_4_2(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree,
    int offset, guint16 *bcp, gboolean *trunc, smb_info_t *si)
{
	int                   fn_len;
	const char           *fn;
	int                   old_offset  = offset;
	proto_item           *item;
	proto_tree           *tree;
	smb_transact2_info_t *t2i;
	gboolean              resume_keys = FALSE;
	guint32               bytes_needed = 0;

	DISSECTOR_ASSERT(si);

	if ((si->sip != NULL) && (si->sip->extra_info_type == SMB_EI_T2I)) {
		t2i = (smb_transact2_info_t *)si->sip->extra_info;
		if (t2i != NULL)
			resume_keys = t2i->resume_keys;
	}

	tree = proto_tree_add_subtree(parent_tree, tvb, offset, *bcp, ett_smb_ff2_data, &item,
		    val_to_str(si->info_level, ff2_il_vals, "Unknown (0x%02x)"));

	/*
	 * Figure out of there are enough bytes to display the whole entry.
	 * This consistes of 26 bytes or 30 bytes if resume_keys, followed
	 * by a length byte and that many chars.
	 */
	bytes_needed = 27 + (resume_keys ? 4 : 0);
	tvb_ensure_bytes_exist(tvb, offset, bytes_needed);

	/* Now, get the length */
	fn_len = tvb_get_guint8(tvb, offset + bytes_needed - 1);
	tvb_ensure_bytes_exist(tvb, offset, bytes_needed + fn_len);

	if (resume_keys) {
		/* resume key */
		CHECK_BYTE_COUNT_SUBR(4);
		proto_tree_add_item(tree, hf_smb_resume, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_SUBR(4);
	}

	/* create time */
	CHECK_BYTE_COUNT_SUBR(4);
	offset = dissect_smb_datetime(tvb, tree, offset,
		hf_smb_create_time,
		hf_smb_create_dos_date, hf_smb_create_dos_time, FALSE);
	*bcp -= 4;

	/* access time */
	CHECK_BYTE_COUNT_SUBR(4);
	offset = dissect_smb_datetime(tvb, tree, offset,
		hf_smb_access_time,
		hf_smb_access_dos_date, hf_smb_access_dos_time, FALSE);
	*bcp -= 4;

	/* last write time */
	CHECK_BYTE_COUNT_SUBR(4);
	offset = dissect_smb_datetime(tvb, tree, offset,
		hf_smb_last_write_time,
		hf_smb_last_write_dos_date, hf_smb_last_write_dos_time, FALSE);
	*bcp -= 4;

	/* data size */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_data_size, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* allocation size */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_alloc_size, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* File Attributes */
	CHECK_BYTE_COUNT_SUBR(2);
	offset = dissect_file_attributes(tvb, tree, offset);
	*bcp -= 2;

	/* ea length */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_ea_list_length, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* file name len */
	CHECK_BYTE_COUNT_SUBR(1);
	fn_len = tvb_get_guint8(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_file_name_len, tvb, offset, 1, fn_len);
	COUNT_BYTES_SUBR(1);
	if (si->unicode)
		fn_len += 2;	/* include terminating '\0' */
	else
		fn_len++;	/* include terminating '\0' */

	/* file name */
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len, FALSE, TRUE, bcp);
	CHECK_STRING_SUBR(fn);
	proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
		fn);
	COUNT_BYTES_SUBR(fn_len);

	col_append_fstr(pinfo->cinfo, COL_INFO, " %s",
		    format_text(fn, strlen(fn)));

	proto_item_append_text(item, " File: %s", format_text(fn, strlen(fn)));
	proto_item_set_len(item, offset-old_offset);

	*trunc = FALSE;
	return offset;
}

/*
 * According to MS-CIFS 2.2.8.1.3 this is like the function above with the
 * addition of the list of EA name value pairs before the file name.
 *
 * The EAs are formatted as an SMB_FEA as in 2.2.1.2.2. We will deal with
 * this soon.
 */
static int
dissect_4_3_4_3(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree,
    int offset, guint16 *bcp, gboolean *trunc, smb_info_t *si)
{
	int                   fn_len;
	const char           *fn;
	int                   old_offset  = offset;
	int		     ea_size = 0;
	proto_item           *item;
	proto_tree           *tree;
	smb_transact2_info_t *t2i;
	gboolean              resume_keys = FALSE;


	DISSECTOR_ASSERT(si);

	if ((si->sip != NULL) && (si->sip->extra_info_type == SMB_EI_T2I)) {
		t2i = (smb_transact2_info_t *)si->sip->extra_info;
		if (t2i != NULL)
			resume_keys = t2i->resume_keys;
	}

	tree = proto_tree_add_subtree(parent_tree, tvb, offset, *bcp, ett_smb_ff2_data, &item,
		    val_to_str(si->info_level, ff2_il_vals, "Unknown (0x%02x)"));

	if (resume_keys) {
		/* resume key */
		CHECK_BYTE_COUNT_SUBR(4);
		proto_tree_add_item(tree, hf_smb_resume, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_SUBR(4);
	}

	/* create time */
	CHECK_BYTE_COUNT_SUBR(4);
	offset = dissect_smb_datetime(tvb, tree, offset,
		hf_smb_create_time,
		hf_smb_create_dos_date, hf_smb_create_dos_time, FALSE);
	*bcp -= 4;

	/* access time */
	CHECK_BYTE_COUNT_SUBR(4);
	offset = dissect_smb_datetime(tvb, tree, offset,
		hf_smb_access_time,
		hf_smb_access_dos_date, hf_smb_access_dos_time, FALSE);
	*bcp -= 4;

	/* last write time */
	CHECK_BYTE_COUNT_SUBR(4);
	offset = dissect_smb_datetime(tvb, tree, offset,
		hf_smb_last_write_time,
		hf_smb_last_write_dos_date, hf_smb_last_write_dos_time, FALSE);
	*bcp -= 4;

	/* data size */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_data_size, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* allocation size */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_alloc_size, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* File Attributes */
	CHECK_BYTE_COUNT_SUBR(2);
	offset = dissect_file_attributes(tvb, tree, offset);
	*bcp -= 2;

	/* ea length */
	CHECK_BYTE_COUNT_SUBR(4);
	ea_size = tvb_get_letohl(tvb, offset);
	proto_tree_add_item(tree, hf_smb_ea_list_length, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* The EAs ... they are formatted as in MS-CIFS 2.2.1.2.2 */
	proto_tree_add_bytes_format(tree, hf_smb_file_data, tvb, offset, ea_size, NULL, "EAs");
	COUNT_BYTES_SUBR(ea_size);
	*bcp -= ea_size;

	/* file name len */
	CHECK_BYTE_COUNT_SUBR(1);
	fn_len = tvb_get_guint8(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_file_name_len, tvb, offset, 1, fn_len);
	COUNT_BYTES_SUBR(1);
	if (si->unicode)
		fn_len += 2;	/* include terminating '\0' */
	else
		fn_len++;	/* include terminating '\0' */

	/* file name */
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len, FALSE, TRUE, bcp);
	CHECK_STRING_SUBR(fn);
	proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
		fn);
	COUNT_BYTES_SUBR(fn_len);

	col_append_fstr(pinfo->cinfo, COL_INFO, " %s",
		    format_text(fn, strlen(fn)));

	proto_item_append_text(item, " File: %s", format_text(fn, strlen(fn)));
	proto_item_set_len(item, offset-old_offset);

	return offset;
}

static int
dissect_4_3_4_4(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree,
    int offset, guint16 *bcp, gboolean *trunc, smb_info_t *si)
{
	int         fn_len;
	const char *fn;
	int         old_offset = offset;
	proto_item *item;
	proto_tree *tree;
	guint32     neo;
	int         padcnt;

	DISSECTOR_ASSERT(si);

	/*
	 * We check this first before adding the sub-tree so things do not
	 * get ugly.
	 */

	/* next entry offset */
	CHECK_BYTE_COUNT_SUBR(4);
	neo = tvb_get_letohl(tvb, offset);

	/* Ensure we have the bytes we need, which is up to neo */
	tvb_ensure_bytes_exist(tvb, offset, neo ? neo : *bcp);

	tree = proto_tree_add_subtree(parent_tree, tvb, offset, *bcp, ett_smb_ff2_data, &item,
		    val_to_str(si->info_level, ff2_il_vals, "Unknown (0x%02x)"));

	/*
	 * We assume that the presence of a next entry offset implies the
	 * absence of a resume key, as appears to be the case for 4.3.4.6.
	 */

	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_uint(tree, hf_smb_next_entry_offset, tvb, offset, 4, neo);
	COUNT_BYTES_SUBR(4);

	/* file index */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_file_index, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	offset = dissect_smb_standard_8byte_timestamps(tvb, pinfo, tree, offset, bcp, trunc);
	if (*trunc) {
		return offset;
	}

	/* end of file */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_end_of_file, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* allocation size */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_alloc_size64, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* Extended File Attributes */
	CHECK_BYTE_COUNT_SUBR(4);
	offset = dissect_file_ext_attr(tvb, tree, offset);
	*bcp -= 4;

	/* file name len */
	CHECK_BYTE_COUNT_SUBR(4);
	fn_len = tvb_get_letohl(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_file_name_len, tvb, offset, 4, fn_len);
	COUNT_BYTES_SUBR(4);

	/* file name */
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len, FALSE, TRUE, bcp);
	CHECK_STRING_SUBR(fn);
	proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
		fn);
	COUNT_BYTES_SUBR(fn_len);

	col_append_fstr(pinfo->cinfo, COL_INFO, " %s",
		    format_text(fn, strlen(fn)));

	/* skip to next structure */
	if (neo) {
		padcnt = (old_offset + neo) - offset;
		if (padcnt < 0) {
			/*
			 * XXX - this is bogus; flag it?
			 */
			padcnt = 0;
		}
		if (padcnt != 0) {
			CHECK_BYTE_COUNT_SUBR(padcnt);
			COUNT_BYTES_SUBR(padcnt);
		}
	}

	proto_item_append_text(item, " File: %s", format_text(fn, strlen(fn)));
	proto_item_set_len(item, offset-old_offset);

	*trunc = FALSE;
	return offset;
}

static int
dissect_4_3_4_5(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree,
    int offset, guint16 *bcp, gboolean *trunc, smb_info_t *si)
{
	int         fn_len;
	const char *fn;
	int         old_offset = offset;
	proto_item *item;
	proto_tree *tree;
	guint32     neo;
	int         padcnt;

	DISSECTOR_ASSERT(si);

	/*
	 * We check this first before adding the sub-tree so things do not
	 * get ugly.
	 */

	/* next entry offset */
	CHECK_BYTE_COUNT_SUBR(4);
	neo = tvb_get_letohl(tvb, offset);

	/* Ensure we have the bytes we need, which is up to neo */
	tvb_ensure_bytes_exist(tvb, offset, neo ? neo : *bcp);

	tree = proto_tree_add_subtree(parent_tree, tvb, offset, *bcp, ett_smb_ff2_data, &item,
		    val_to_str(si->info_level, ff2_il_vals, "Unknown (0x%02x)"));

	/*
	 * We assume that the presence of a next entry offset implies the
	 * absence of a resume key, as appears to be the case for 4.3.4.6.
	 */

	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_uint(tree, hf_smb_next_entry_offset, tvb, offset, 4, neo);
	COUNT_BYTES_SUBR(4);

	/* file index */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_file_index, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* standard 8-byte timestamps */
	offset = dissect_smb_standard_8byte_timestamps(tvb, pinfo, tree, offset, bcp, trunc);
	if (*trunc) {
		return offset;
	}

	/* end of file */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_end_of_file, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* allocation size */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_alloc_size64, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* Extended File Attributes */
	CHECK_BYTE_COUNT_SUBR(4);
	offset = dissect_file_ext_attr(tvb, tree, offset);
	*bcp -= 4;

	/* file name len */
	CHECK_BYTE_COUNT_SUBR(4);
	fn_len = tvb_get_letohl(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_file_name_len, tvb, offset, 4, fn_len);
	COUNT_BYTES_SUBR(4);

	/* ea length */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_ea_list_length, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* file name */
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len, FALSE, TRUE, bcp);
	CHECK_STRING_SUBR(fn);
	proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
		fn);
	COUNT_BYTES_SUBR(fn_len);

	col_append_fstr(pinfo->cinfo, COL_INFO, " %s",
		    format_text(fn, strlen(fn)));

	/* skip to next structure */
	if (neo) {
		padcnt = (old_offset + neo) - offset;
		if (padcnt < 0) {
			/*
			 * XXX - this is bogus; flag it?
			 */
			padcnt = 0;
		}
		if (padcnt != 0) {
			CHECK_BYTE_COUNT_SUBR(padcnt);
			COUNT_BYTES_SUBR(padcnt);
		}
	}

	proto_item_append_text(item, " File: %s", format_text(fn, strlen(fn)));
	proto_item_set_len(item, offset-old_offset);

	*trunc = FALSE;
	return offset;
}

static int
dissect_4_3_4_6(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree,
    int offset, guint16 *bcp, gboolean *trunc, smb_info_t *si)
{
	int         fn_len, sfn_len;
	const char *fn, *sfn;
	int         old_offset = offset;
	proto_item *item;
	proto_tree *tree;
	guint32     neo;
	int         padcnt;

	DISSECTOR_ASSERT(si);

	/*
	 * We check this first before adding the sub-tree so things do not
	 * get ugly.
	 */

	/* next entry offset */
	CHECK_BYTE_COUNT_SUBR(4);
	neo = tvb_get_letohl(tvb, offset);

	/* Ensure we have the bytes we need, which is up to neo */
	tvb_ensure_bytes_exist(tvb, offset, neo ? neo : *bcp);

	tree = proto_tree_add_subtree(parent_tree, tvb, offset, *bcp, ett_smb_ff2_data, &item,
		    val_to_str(si->info_level, ff2_il_vals, "Unknown (0x%02x)"));

	/*
	 * XXX - I have not seen any of these that contain a resume
	 * key, even though some of the requests had the "return resume
	 * key" flag set.
	 */

	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_uint(tree, hf_smb_next_entry_offset, tvb, offset, 4, neo);
	COUNT_BYTES_SUBR(4);

	/* file index */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_file_index, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* dissect standard 8-byte timestamps */
	offset = dissect_smb_standard_8byte_timestamps(tvb, pinfo, tree, offset, bcp, trunc);
	if (*trunc) {
	  return offset;
	}

	/* end of file */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_end_of_file, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* allocation size */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_alloc_size64, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* Extended File Attributes */
	CHECK_BYTE_COUNT_SUBR(4);
	offset = dissect_file_ext_attr(tvb, tree, offset);
	*bcp -= 4;

	/* file name len */
	CHECK_BYTE_COUNT_SUBR(4);
	fn_len = tvb_get_letohl(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_file_name_len, tvb, offset, 4, fn_len);
	COUNT_BYTES_SUBR(4);

	/*
	 * EA length.
	 *
	 * XXX - in one captures, this has the topmost bit set, and the
	 * rest of the bits have the value 7.  Is the topmost bit being
	 * set some indication that the value *isn't* the length of
	 * the EAs?
	 */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_ea_list_length, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* short file name len */
	CHECK_BYTE_COUNT_SUBR(1);
	sfn_len = tvb_get_guint8(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_short_file_name_len, tvb, offset, 1, sfn_len);
	COUNT_BYTES_SUBR(1);

	/* reserved byte */
	CHECK_BYTE_COUNT_SUBR(1);
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 1, ENC_NA);
	COUNT_BYTES_SUBR(1);

	/* short file name - it's not always in Unicode */
	sfn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &sfn_len, FALSE, TRUE, bcp);
	CHECK_STRING_SUBR(sfn);
	proto_tree_add_string(tree, hf_smb_short_file_name, tvb, offset, 24,
		sfn);
	COUNT_BYTES_SUBR(24);

	/* file name */
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len, FALSE, TRUE, bcp);
	CHECK_STRING_SUBR(fn);
	proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
		fn);
	COUNT_BYTES_SUBR(fn_len);

	col_append_fstr(pinfo->cinfo, COL_INFO, " %s",
		    format_text(fn, strlen(fn)));

	/* skip to next structure */
	if (neo) {
		padcnt = (old_offset + neo) - offset;
		if (padcnt < 0) {
			/*
			 * XXX - this is bogus; flag it?
			 */
			padcnt = 0;
		}
		if (padcnt != 0) {
			CHECK_BYTE_COUNT_SUBR(padcnt);
			COUNT_BYTES_SUBR(padcnt);
		}
	}

	proto_item_append_text(item, " File: %s", format_text(fn, strlen(fn)));
	proto_item_set_len(item, offset-old_offset);

	*trunc = FALSE;
	return offset;
}

static int
dissect_4_3_4_6full(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree,
    int offset, guint16 *bcp, gboolean *trunc, smb_info_t *si)
{
	int         fn_len;
	const char *fn;
	int         old_offset = offset;
	proto_item *item;
	proto_tree *tree;
	guint32     neo;
	int         padcnt;

	DISSECTOR_ASSERT(si);

	/*
	 * We check this first before adding the sub-tree so things do not
	 * get ugly.
	 */

	/* next entry offset */
	CHECK_BYTE_COUNT_SUBR(4);
	neo = tvb_get_letohl(tvb, offset);

	/* Ensure we have the bytes we need, which is up to neo */
	tvb_ensure_bytes_exist(tvb, offset, neo ? neo : *bcp);

	tree = proto_tree_add_subtree(parent_tree, tvb, offset, *bcp, ett_smb_ff2_data, &item,
		    val_to_str(si->info_level, ff2_il_vals, "Unknown (0x%02x)"));

	/*
	 * XXX - I have not seen any of these that contain a resume
	 * key, even though some of the requests had the "return resume
	 * key" flag set.
	 */

	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_uint(tree, hf_smb_next_entry_offset, tvb, offset, 4, neo);
	COUNT_BYTES_SUBR(4);

	/* file index */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_file_index, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* dissect standard 8-byte timestamps */
	offset = dissect_smb_standard_8byte_timestamps(tvb, pinfo, tree, offset, bcp, trunc);
	if (*trunc) {
	  return offset;
	}

	/* end of file */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_end_of_file, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* allocation size */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_alloc_size64, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* Extended File Attributes */
	CHECK_BYTE_COUNT_SUBR(4);
	offset = dissect_file_ext_attr(tvb, tree, offset);
	*bcp -= 4;

	/* file name len */
	CHECK_BYTE_COUNT_SUBR(4);
	fn_len = tvb_get_letohl(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_file_name_len, tvb, offset, 4, fn_len);
	COUNT_BYTES_SUBR(4);

	/*
	 * EA length.
	 *
	 * XXX - in one captures, this has the topmost bit set, and the
	 * rest of the bits have the value 7.  Is the topmost bit being
	 * set some indication that the value *isn't* the length of
	 * the EAs?
	 */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_ea_list_length, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* skip 4 bytes */
	COUNT_BYTES_SUBR(4);

	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_index_number, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* file name */
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len, FALSE, TRUE, bcp);
	CHECK_STRING_SUBR(fn);
	proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
		fn);
	COUNT_BYTES_SUBR(fn_len);

	col_append_fstr(pinfo->cinfo, COL_INFO, " %s",
		    format_text(fn, strlen(fn)));

	/* skip to next structure */
	if (neo) {
		padcnt = (old_offset + neo) - offset;
		if (padcnt < 0) {
			/*
			 * XXX - this is bogus; flag it?
			 */
			padcnt = 0;
		}
		if (padcnt != 0) {
			CHECK_BYTE_COUNT_SUBR(padcnt);
			COUNT_BYTES_SUBR(padcnt);
		}
	}

	proto_item_append_text(item, " File: %s", format_text(fn, strlen(fn)));
	proto_item_set_len(item, offset-old_offset);

	*trunc = FALSE;
	return offset;
}

static int
dissect_4_3_4_6_id_both(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree,
    int offset, guint16 *bcp, gboolean *trunc, smb_info_t *si)
{
	int         fn_len, sfn_len;
	const char *fn, *sfn;
	int         old_offset = offset;
	proto_item *item;
	proto_tree *tree;
	guint32     neo;
	int         padcnt;

	DISSECTOR_ASSERT(si);

	/*
	 * We check this first before adding the sub-tree so things do not
	 * get ugly.
	 */

	/* next entry offset */
	CHECK_BYTE_COUNT_SUBR(4);
	neo = tvb_get_letohl(tvb, offset);

	/* Ensure we have the bytes we need, which is up to neo */
	tvb_ensure_bytes_exist(tvb, offset, neo ? neo : *bcp);

	tree = proto_tree_add_subtree(parent_tree, tvb, offset, *bcp, ett_smb_ff2_data, &item,
		    val_to_str(si->info_level, ff2_il_vals, "Unknown (0x%02x)"));

	/*
	 * XXX - I have not seen any of these that contain a resume
	 * key, even though some of the requests had the "return resume
	 * key" flag set.
	 */

	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_uint(tree, hf_smb_next_entry_offset, tvb, offset, 4, neo);
	COUNT_BYTES_SUBR(4);

	/* file index */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_file_index, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* dissect standard 8-byte timestamps */
	offset = dissect_smb_standard_8byte_timestamps(tvb, pinfo, tree, offset, bcp, trunc);
	if (*trunc) {
	  return offset;
	}

	/* end of file */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_end_of_file, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* allocation size */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_alloc_size64, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* Extended File Attributes */
	CHECK_BYTE_COUNT_SUBR(4);
	offset = dissect_file_ext_attr(tvb, tree, offset);
	*bcp -= 4;

	/* file name len */
	CHECK_BYTE_COUNT_SUBR(4);
	fn_len = tvb_get_letohl(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_file_name_len, tvb, offset, 4, fn_len);
	COUNT_BYTES_SUBR(4);

	/*
	 * EA length.
	 *
	 * XXX - in one captures, this has the topmost bit set, and the
	 * rest of the bits have the value 7.  Is the topmost bit being
	 * set some indication that the value *isn't* the length of
	 * the EAs?
	 */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_ea_list_length, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* short file name len */
	CHECK_BYTE_COUNT_SUBR(1);
	sfn_len = tvb_get_guint8(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_short_file_name_len, tvb, offset, 1, sfn_len);
	COUNT_BYTES_SUBR(1);

	/* reserved byte */
	CHECK_BYTE_COUNT_SUBR(1);
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 1, ENC_NA);
	COUNT_BYTES_SUBR(1);

	/* short file name - it's not always in Unicode */
	sfn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &sfn_len, FALSE, TRUE, bcp);
	CHECK_STRING_SUBR(sfn);
	proto_tree_add_string(tree, hf_smb_short_file_name, tvb, offset, 24,
		sfn);
	COUNT_BYTES_SUBR(24);

	/* reserved bytes */
	CHECK_BYTE_COUNT_SUBR(2);
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 2, ENC_NA);
	COUNT_BYTES_SUBR(2);

	/* file id */
	CHECK_BYTE_COUNT_SUBR(8);
	proto_tree_add_item(tree, hf_smb_index_number, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(8);

	/* file name */
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len, FALSE, TRUE, bcp);
	CHECK_STRING_SUBR(fn);
	proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
		fn);
	COUNT_BYTES_SUBR(fn_len);

	col_append_fstr(pinfo->cinfo, COL_INFO, " %s",
		    format_text(fn, strlen(fn)));

	/* skip to next structure */
	if (neo) {
		padcnt = (old_offset + neo) - offset;
		if (padcnt < 0) {
			/*
			 * XXX - this is bogus; flag it?
			 */
			padcnt = 0;
		}
		if (padcnt != 0) {
			CHECK_BYTE_COUNT_SUBR(padcnt);
			COUNT_BYTES_SUBR(padcnt);
		}
	}

	proto_item_append_text(item, " File: %s", format_text(fn, strlen(fn)));
	proto_item_set_len(item, offset-old_offset);

	*trunc = FALSE;
	return offset;
}

static int
dissect_4_3_4_7(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree,
    int offset, guint16 *bcp, gboolean *trunc, smb_info_t *si)
{
	int         fn_len;
	const char *fn;
	int         old_offset = offset;
	proto_item *item;
	proto_tree *tree;
	guint32     neo;
	int         padcnt;

	DISSECTOR_ASSERT(si);

	/*
	 * We check this first before adding the sub-tree so things do not
	 * get ugly.
	 */

	/* next entry offset */
	CHECK_BYTE_COUNT_SUBR(4);
	neo = tvb_get_letohl(tvb, offset);

	/* Ensure we have the bytes we need, which is up to neo */
	tvb_ensure_bytes_exist(tvb, offset, neo ? neo : *bcp);

	tree = proto_tree_add_subtree(parent_tree, tvb, offset, *bcp, ett_smb_ff2_data, &item,
		    val_to_str(si->info_level, ff2_il_vals, "Unknown (0x%02x)"));

	/*
	 * We assume that the presence of a next entry offset implies the
	 * absence of a resume key, as appears to be the case for 4.3.4.6.
	 */

	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_uint(tree, hf_smb_next_entry_offset, tvb, offset, 4, neo);
	COUNT_BYTES_SUBR(4);

	/* file index */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_file_index, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* file name len */
	CHECK_BYTE_COUNT_SUBR(4);
	fn_len = tvb_get_letohl(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_file_name_len, tvb, offset, 4, fn_len);
	COUNT_BYTES_SUBR(4);

	/* file name */
	fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len, FALSE, TRUE, bcp);
	CHECK_STRING_SUBR(fn);
	proto_tree_add_string(tree, hf_smb_file_name, tvb, offset, fn_len,
		fn);
	COUNT_BYTES_SUBR(fn_len);

	col_append_fstr(pinfo->cinfo, COL_INFO, " %s",
		    format_text(fn, strlen(fn)));

	/* skip to next structure */
	if (neo) {
		padcnt = (old_offset + neo) - offset;
		if (padcnt < 0) {
			/*
			 * XXX - this is bogus; flag it?
			 */
			padcnt = 0;
		}
		if (padcnt != 0) {
			CHECK_BYTE_COUNT_SUBR(padcnt);
			COUNT_BYTES_SUBR(padcnt);
		}
	}

	proto_item_append_text(item, " File: %s", format_text(fn, strlen(fn)));
	proto_item_set_len(item, offset-old_offset);

	*trunc = FALSE;
	return offset;
}

/* 4.3.4.8 - SMB_FIND_FILE_UNIX */

static int
dissect_4_3_4_8(tvbuff_t *tvb, packet_info *pinfo,
		proto_tree *tree, int offset, guint16 *bcp,
		gboolean *trunc, smb_info_t *si)
{
	const char *fn;
	int         fn_len;
	int         pad;

	DISSECTOR_ASSERT(si);

	/* NextEntryOffset */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_unix_find_file_nextoffset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* ResumeKey */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_unix_find_file_resumekey, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* Unix basic info */
	offset = dissect_4_2_16_12(tvb, pinfo, tree, offset, bcp, trunc);
	if (*trunc)
		return offset;

	/* Name */

	fn = get_unicode_or_ascii_string(
		tvb, &offset, si->unicode, &fn_len, FALSE, FALSE, bcp);

	CHECK_STRING_SUBR(fn);
	proto_tree_add_string(
		tree, hf_smb_unix_file_name, tvb, offset, fn_len, fn);
	COUNT_BYTES_SUBR(fn_len);

	/* Pad to 4 bytes */

	if (offset % 4) {
		pad = 4 - (offset % 4);
		COUNT_BYTES_SUBR(pad);
	}

	*trunc = FALSE;
	return offset;
}

static int
dissect_find_file_unix_info2(tvbuff_t *tvb, packet_info *pinfo,
			     proto_tree *tree, int offset, guint16 *bcp,
			     gboolean *trunc, smb_info_t *si)
{
	const char *fn;
	guint32     namelen;
	int         fn_len;
	int         pad;

	DISSECTOR_ASSERT(si);

	/* NextEntryOffset */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_unix_find_file_nextoffset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* ResumeKey */
	CHECK_BYTE_COUNT_SUBR(4);
	proto_tree_add_item(tree, hf_smb_unix_find_file_resumekey, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_SUBR(4);

	/* Unix Info2 */
	offset = dissect_qspi_unix_info2(tvb, pinfo, tree, offset, bcp, trunc);
	if (*trunc)
		return offset;

	/* Name length */
	CHECK_BYTE_COUNT_SUBR(4);
	namelen = tvb_get_letohl(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_unix_file_name_length, tvb, offset, 4, namelen);
	COUNT_BYTES_SUBR(4);

	/* Name */

	/*
	 * namelen could be > 2^31-1; this will catch that.
	 * The length argument to get_unicode_or_ascii_string() is an
	 * int, not an unsigned int, so we have to worry about that.
	 */
	tvb_ensure_bytes_exist(tvb, offset, namelen);
	fn_len = namelen;
	fn = get_unicode_or_ascii_string(
		tvb, &offset, si->unicode, &fn_len, TRUE, TRUE, bcp);

	CHECK_STRING_SUBR(fn);
	proto_tree_add_string(
		tree, hf_smb_unix_file_name, tvb, offset, fn_len, fn);
	COUNT_BYTES_SUBR(fn_len);

	/* Pad to 4 bytes */

	if (offset % 4) {
		pad = 4 - (offset % 4);
		COUNT_BYTES_SUBR(pad);
	}

	*trunc = FALSE;
	return offset;
}

/*dissect the data block for TRANS2_FIND_FIRST2*/
static int
dissect_ff2_response_data(tvbuff_t * tvb, packet_info * pinfo,
    proto_tree * tree, int offset, guint16 *bcp, gboolean *trunc, smb_info_t *si)
{
	if (!*bcp) {
		return offset;
	}

	DISSECTOR_ASSERT(si);

	switch(si->info_level) {
	case 1:		/*Info Standard*/
		offset = dissect_4_3_4_1(tvb, pinfo, tree, offset, bcp,
		    trunc, si);
		break;
	case 2:		/*Info Query EA Size*/
		offset = dissect_4_3_4_2(tvb, pinfo, tree, offset, bcp,
		    trunc, si);
		break;
	case 3:		/* Info Query EAs From List same as
			 * InfoQueryEASize.
			 * Not according to MS-CIFS 2.2.8.1.3. RJS
			 */
		offset = dissect_4_3_4_3(tvb, pinfo, tree, offset, bcp,
		    trunc, si);
		break;
	case 0x0101:	/*Find File Directory Info*/
		offset = dissect_4_3_4_4(tvb, pinfo, tree, offset, bcp,
		    trunc, si);
		break;
	case 0x0102:	/*Find File Full Directory Info*/
		offset = dissect_4_3_4_5(tvb, pinfo, tree, offset, bcp,
		    trunc, si);
		break;
	case 0x0103:	/*Find File Names Info*/
		offset = dissect_4_3_4_7(tvb, pinfo, tree, offset, bcp,
		    trunc, si);
		break;
	case 0x0104:	/*Find File Both Directory Info*/
		offset = dissect_4_3_4_6(tvb, pinfo, tree, offset, bcp,
		    trunc, si);
		break;
	case 0x0105:	/*Find File Full Directory Info*/
		offset = dissect_4_3_4_6full(tvb, pinfo, tree, offset, bcp,
		    trunc, si);
		break;
	case 0x0106:	/*Find File Id Both Directory Info*/
		offset = dissect_4_3_4_6_id_both(tvb, pinfo, tree, offset, bcp,
		    trunc, si);
		break;
	case 0x0202:	/*Find File Unix*/
		offset = dissect_4_3_4_8(tvb, pinfo, tree, offset, bcp,
		    trunc, si);
		break;
	case 0x020B:	/*Find File Unix Info2*/
		offset = dissect_find_file_unix_info2(tvb, pinfo, tree, offset, bcp,
		    trunc, si);
		break;
	default:	/* unknown info level */
		*trunc = FALSE;
		break;
	}
	return offset;
}


/* is this one just wrong and should be dissect_fs0105_attributes above ? */
static int
dissect_fs_attributes(tvbuff_t *tvb, proto_tree *parent_tree, int offset)
{
	static const int * flags[] = {
		/* case sensitive search */
		&hf_smb_fs_attr_css,
		/* case preserved names */
		&hf_smb_fs_attr_cpn,
		/* unicode on disk */
		&hf_smb_fs_attr_uod,
		/* persistent acls */
		&hf_smb_fs_attr_pacls,
		/* file compression */
		&hf_smb_fs_attr_fc,
		/* volume quotas */
		&hf_smb_fs_attr_vq,
		/* sparse files */
		&hf_smb_fs_attr_ssf,
		/* reparse points */
		&hf_smb_fs_attr_srp,
		/* remote storage */
		&hf_smb_fs_attr_srs,
		/* lfn apis */
		&hf_smb_fs_attr_sla,
		/* volume is compressed */
		&hf_smb_fs_attr_vic,
		/* support oids */
		&hf_smb_fs_attr_soids,
		/* encryption */
		&hf_smb_fs_attr_se,
		/* named streams */
		&hf_smb_fs_attr_ns,
		/* read only volume */
		&hf_smb_fs_attr_rov,
		NULL
	};

	proto_tree_add_bitmask(parent_tree, tvb, offset, hf_smb_fs_attr, ett_smb_fs_attributes, flags, ENC_LITTLE_ENDIAN);
	offset += 4;

	return offset;
}


static int
dissect_device_characteristics(tvbuff_t *tvb, proto_tree *parent_tree, int offset)
{
	static const int * mask[] = {
		&hf_smb_device_char_removable,
		&hf_smb_device_char_read_only,
		&hf_smb_device_char_floppy,
		&hf_smb_device_char_write_once,
		&hf_smb_device_char_remote,
		&hf_smb_device_char_mounted,
		&hf_smb_device_char_virtual,
		NULL
	};

	proto_tree_add_bitmask_with_flags(parent_tree, tvb, offset, hf_smb_device_char,
			ett_smb_device_characteristics, mask, ENC_LITTLE_ENDIAN, BMT_NO_APPEND);

	offset += 4;
	return offset;
}

/*dissect the data block for TRANS2_QUERY_FS_INFORMATION*/

static const true_false_string tfs_smb_mac_access_ctrl = {
	"Macintosh Access Control Supported",
	"Macintosh Access Control Not Supported"
};

static const true_false_string tfs_smb_mac_getset_comments = {
	"Macintosh Get & Set Comments Supported",
	"Macintosh Get & Set Comments Not Supported"
};

static const true_false_string tfs_smb_mac_desktopdb_calls = {
	"Macintosh Get & Set Desktop Database Info Supported",
	"Macintosh Get & Set Desktop Database Info Supported"
};

static const true_false_string tfs_smb_mac_unique_ids = {
	"Macintosh Unique IDs Supported",
	"Macintosh Unique IDs Not Supported"
};

static const true_false_string tfs_smb_mac_streams = {
	"Macintosh and Streams Extensions Not Supported",
	"Macintosh and Streams Extensions Supported"
};

int
dissect_qfsi_FS_VOLUME_INFO(tvbuff_t * tvb, packet_info * pinfo _U_, proto_tree * tree, int offset, guint16 *bcp, int unicode)
{
	int         fn_len, vll;
	const char *fn;

	/* create time */
	CHECK_BYTE_COUNT_TRANS_SUBR(8);
	offset = dissect_nt_64bit_time(tvb, tree, offset,
		hf_smb_create_time);
	*bcp -= 8;

	/* volume serial number */
	CHECK_BYTE_COUNT_TRANS_SUBR(4);
	proto_tree_add_item(tree, hf_smb_volume_serial_num, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_TRANS_SUBR(4);

	/* volume label length */
	CHECK_BYTE_COUNT_TRANS_SUBR(4);
	vll = tvb_get_letohl(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_volume_label_len, tvb, offset, 4, vll);
	COUNT_BYTES_TRANS_SUBR(4);

	/* 2 reserved bytes */
	CHECK_BYTE_COUNT_TRANS_SUBR(2);
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 2, ENC_NA);
	COUNT_BYTES_TRANS_SUBR(2);

	/* label */
	fn_len = vll;
	fn = get_unicode_or_ascii_string(tvb, &offset, unicode, &fn_len, FALSE, TRUE, bcp);
	CHECK_STRING_TRANS_SUBR(fn);
	proto_tree_add_string(tree, hf_smb_volume_label, tvb, offset, fn_len,
		fn);
	COUNT_BYTES_TRANS_SUBR(fn_len);

	return offset;
}

int
dissect_qfsi_FS_SIZE_INFO(tvbuff_t * tvb, packet_info * pinfo _U_, proto_tree * tree, int offset, guint16 *bcp)
{
	/* allocation size */
	CHECK_BYTE_COUNT_TRANS_SUBR(8);
	proto_tree_add_item(tree, hf_smb_alloc_size64, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_TRANS_SUBR(8);

	/* free allocation units */
	CHECK_BYTE_COUNT_TRANS_SUBR(8);
	proto_tree_add_item(tree, hf_smb_free_alloc_units64, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_TRANS_SUBR(8);

	/* sectors per unit */
	CHECK_BYTE_COUNT_TRANS_SUBR(4);
	proto_tree_add_item(tree, hf_smb_sector_unit, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_TRANS_SUBR(4);

	/* bytes per sector */
	CHECK_BYTE_COUNT_TRANS_SUBR(4);
	proto_tree_add_item(tree, hf_smb_fs_sector, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_TRANS_SUBR(4);

	return offset;
}

int
dissect_qfsi_FS_DEVICE_INFO(tvbuff_t * tvb, packet_info * pinfo _U_, proto_tree * tree, int offset, guint16 *bcp)
{
	/* device type */
	CHECK_BYTE_COUNT_TRANS_SUBR(4);
	proto_tree_add_item(tree, hf_smb_device_type, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_TRANS_SUBR(4);

	/* device characteristics */
	CHECK_BYTE_COUNT_TRANS_SUBR(4);
	offset = dissect_device_characteristics(tvb, tree, offset);
	*bcp -= 4;

	return offset;
}

int
dissect_qfsi_FS_ATTRIBUTE_INFO(tvbuff_t * tvb, packet_info * pinfo _U_, proto_tree * tree, int offset, guint16 *bcp, int unicode)
{
	int         fn_len, fnl;
	const char *fn;

	/* FS attributes */
	CHECK_BYTE_COUNT_TRANS_SUBR(4);
	offset = dissect_fs_attributes(tvb, tree, offset);
	*bcp -= 4;

	/* max name len */
	CHECK_BYTE_COUNT_TRANS_SUBR(4);
	proto_tree_add_item(tree, hf_smb_max_name_len, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_TRANS_SUBR(4);

	/* fs name length */
	CHECK_BYTE_COUNT_TRANS_SUBR(4);
	fnl = tvb_get_letohl(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_fs_name_len, tvb, offset, 4, fnl);
	COUNT_BYTES_TRANS_SUBR(4);

	/* label */
	fn_len = fnl;
	fn = get_unicode_or_ascii_string(tvb, &offset, unicode, &fn_len, FALSE, TRUE, bcp);
	CHECK_STRING_TRANS_SUBR(fn);
	proto_tree_add_string(tree, hf_smb_fs_name, tvb, offset, fn_len,
		fn);
	COUNT_BYTES_TRANS_SUBR(fn_len);

	return offset;
}

int
dissect_qfsi_FS_OBJECTID_INFO(tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree, int offset, guint16 *bcp)
{
	CHECK_BYTE_COUNT_TRANS_SUBR(64);

	dissect_smb2_FILE_OBJECTID_BUFFER(tvb, pinfo, tree, offset);

	COUNT_BYTES_TRANS_SUBR(64);

	return offset;
}

int
dissect_qfsi_FS_FULL_SIZE_INFO(tvbuff_t * tvb, packet_info * pinfo _U_, proto_tree * tree, int offset, guint16 *bcp)
{
	/* allocation size */
	CHECK_BYTE_COUNT_TRANS_SUBR(8);
	proto_tree_add_item(tree, hf_smb_alloc_size64, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_TRANS_SUBR(8);

	/* caller free allocation units */
	CHECK_BYTE_COUNT_TRANS_SUBR(8);
	proto_tree_add_item(tree, hf_smb_caller_free_alloc_units64, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_TRANS_SUBR(8);

	/* actual free allocation units */
	CHECK_BYTE_COUNT_TRANS_SUBR(8);
	proto_tree_add_item(tree, hf_smb_actual_free_alloc_units64, tvb, offset, 8, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_TRANS_SUBR(8);

	/* sectors per unit */
	CHECK_BYTE_COUNT_TRANS_SUBR(4);
	proto_tree_add_item(tree, hf_smb_sector_unit, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_TRANS_SUBR(4);

	/* bytes per sector */
	CHECK_BYTE_COUNT_TRANS_SUBR(4);
	proto_tree_add_item(tree, hf_smb_fs_sector, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	COUNT_BYTES_TRANS_SUBR(4);

	return offset;
}

static int
dissect_qfsi_vals(tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree,
    int offset, guint16 *bcp, smb_info_t *si)
{
	int         fn_len, vll;
	const char *fn;

	if (!*bcp) {
		return offset;
	}

	DISSECTOR_ASSERT(si);

	switch(si->info_level) {
	case 1:		/* SMB_INFO_ALLOCATION */
		/* filesystem id */
		CHECK_BYTE_COUNT_TRANS_SUBR(4);
		proto_tree_add_item(tree, hf_smb_fs_id, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS_SUBR(4);

		/* sectors per unit */
		CHECK_BYTE_COUNT_TRANS_SUBR(4);
		proto_tree_add_item(tree, hf_smb_sector_unit, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS_SUBR(4);

		/* units */
		CHECK_BYTE_COUNT_TRANS_SUBR(4);
		proto_tree_add_item(tree, hf_smb_fs_units, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS_SUBR(4);

		/* avail units */
		CHECK_BYTE_COUNT_TRANS_SUBR(4);
		proto_tree_add_item(tree, hf_smb_avail_units, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS_SUBR(4);

		/* bytes per sector, only 16bit integer here */
		CHECK_BYTE_COUNT_TRANS_SUBR(2);
		proto_tree_add_uint(tree, hf_smb_fs_sector, tvb, offset, 2, tvb_get_letohs(tvb, offset));
		COUNT_BYTES_TRANS_SUBR(2);

		break;
	case 2:		/* SMB_INFO_VOLUME */
		/* volume serial number */
		CHECK_BYTE_COUNT_TRANS_SUBR(4);
		proto_tree_add_item(tree, hf_smb_volume_serial_num, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS_SUBR(4);

		/* volume label length, only one byte here */
		CHECK_BYTE_COUNT_TRANS_SUBR(1);
		proto_tree_add_uint(tree, hf_smb_volume_label_len, tvb, offset, 1, tvb_get_guint8(tvb, offset));
		COUNT_BYTES_TRANS_SUBR(1);

		/* label - not aligned! */
		fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len, TRUE, FALSE, bcp);
		CHECK_STRING_TRANS_SUBR(fn);
		proto_tree_add_string(tree, hf_smb_volume_label, tvb, offset, fn_len,
			fn);
		COUNT_BYTES_TRANS_SUBR(fn_len);

		break;
	case 0x0101:	/* SMB_QUERY_FS_LABEL_INFO */
	case 1002:	/* SMB_FS_LABEL_INFORMATION */
		/* volume label length */
		CHECK_BYTE_COUNT_TRANS_SUBR(4);
		vll = tvb_get_letohl(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_volume_label_len, tvb, offset, 4, vll);
		COUNT_BYTES_TRANS_SUBR(4);

		/* label */
		fn_len = vll;
		fn = get_unicode_or_ascii_string(tvb, &offset, si->unicode, &fn_len, FALSE, TRUE, bcp);
		CHECK_STRING_TRANS_SUBR(fn);
		proto_tree_add_string(tree, hf_smb_volume_label, tvb, offset, fn_len,
			fn);
		COUNT_BYTES_TRANS_SUBR(fn_len);

		break;
	case 0x0102:	/* SMB_QUERY_FS_VOLUME_INFO */
	case 1001:	/* SMB_FS_VOLUME_INFORMATION */
		offset = dissect_qfsi_FS_VOLUME_INFO(tvb, pinfo, tree, offset, bcp, si->unicode);
		break;
	case 0x0103:	/* SMB_QUERY_FS_SIZE_INFO */
	case 1003:	/* SMB_FS_SIZE_INFORMATION */
		offset = dissect_qfsi_FS_SIZE_INFO(tvb, pinfo, tree, offset, bcp);
		break;
	case 0x0104:	/* SMB_QUERY_FS_DEVICE_INFO */
	case 1004:	/* SMB_FS_DEVICE_INFORMATION */
		offset = dissect_qfsi_FS_DEVICE_INFO(tvb, pinfo, tree, offset, bcp);
		break;
	case 0x0105:	/* SMB_QUERY_FS_ATTRIBUTE_INFO */
	case 1005:	/* SMB_FS_ATTRIBUTE_INFORMATION */
		offset = dissect_qfsi_FS_ATTRIBUTE_INFO(tvb, pinfo, tree, offset, bcp, si->unicode);
		break;
	case 0x200: {	/* SMB_QUERY_CIFS_UNIX_INFO */
		proto_item *item_2 = NULL;
		proto_tree *subtree = NULL;

		/* MajorVersionNumber */
		CHECK_BYTE_COUNT_TRANS_SUBR(2);
		proto_tree_add_item(tree, hf_smb_unix_major_version, tvb, offset, 2, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS_SUBR(2);

		/* MinorVersionNumber */
		CHECK_BYTE_COUNT_TRANS_SUBR(2);
		proto_tree_add_item(tree, hf_smb_unix_minor_version, tvb, offset, 2, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS_SUBR(2);

		/* Capability */

		CHECK_BYTE_COUNT_TRANS_SUBR(8);

		if (tree) {
			item_2 = proto_tree_add_item(tree, hf_smb_unix_capability, tvb, offset, 8, ENC_LITTLE_ENDIAN);
			subtree = proto_item_add_subtree(
				item_2, ett_smb_unix_capabilities);
		}

		proto_tree_add_item(
			subtree, hf_smb_unix_capability_fcntl, tvb, offset, 4,
			ENC_LITTLE_ENDIAN);

		proto_tree_add_item(
			subtree, hf_smb_unix_capability_posix_acl, tvb, offset, 4,
			ENC_LITTLE_ENDIAN);

		proto_tree_add_item(
			subtree, hf_smb_unix_capability_xattr, tvb, offset, 4,
			ENC_LITTLE_ENDIAN);

		proto_tree_add_item(
			subtree, hf_smb_unix_capability_attr, tvb, offset, 4,
			ENC_LITTLE_ENDIAN);

		proto_tree_add_item(
			subtree, hf_smb_unix_capability_posix_paths, tvb, offset, 4,
			ENC_LITTLE_ENDIAN);

		proto_tree_add_item(
			subtree, hf_smb_unix_capability_posix_path_ops, tvb, offset, 4,
			ENC_LITTLE_ENDIAN);

		proto_tree_add_item(
			subtree, hf_smb_unix_capability_large_read, tvb, offset, 4,
			ENC_LITTLE_ENDIAN);

		proto_tree_add_item(
			subtree, hf_smb_unix_capability_large_write, tvb, offset, 4,
			ENC_LITTLE_ENDIAN);

		proto_tree_add_item(
			subtree, hf_smb_unix_capability_encrpytion, tvb, offset, 4,
			ENC_LITTLE_ENDIAN);

		proto_tree_add_item(
			subtree, hf_smb_unix_capability_mandatory_crypto, tvb, offset, 4,
			ENC_LITTLE_ENDIAN);

		proto_tree_add_item(
			subtree, hf_smb_unix_capability_proxy, tvb, offset, 4,
			ENC_LITTLE_ENDIAN);

		COUNT_BYTES_TRANS_SUBR(8);

		break;
	}

	case 0x202: {	/* SMB_QUERY_POSIX_WHOAMI */
		proto_tree *st_gids;
		guint32     num_gids;
		guint       i;
		proto_tree *st_sids;
		int         old_sid_offset;
		guint32     num_sids;
		guint32     sids_buflen;

		/* Mapping flags */
		CHECK_BYTE_COUNT_TRANS_SUBR(4);
		proto_tree_add_item(tree, hf_smb_unix_whoami_mapflags,
				tvb, offset, 4, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS_SUBR(4);

		/* Mapping flags mask */
		CHECK_BYTE_COUNT_TRANS_SUBR(4);
		proto_tree_add_item(tree, hf_smb_unix_whoami_mapflags_mask,
				tvb, offset, 4, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS_SUBR(4);

		/* primary UID */
		CHECK_BYTE_COUNT_TRANS_SUBR(8);
		proto_tree_add_item(tree, hf_smb_unix_file_uid,
				tvb, offset, 8, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS_SUBR(8);

		/* primary GID */
		CHECK_BYTE_COUNT_TRANS_SUBR(8);
		proto_tree_add_item(tree, hf_smb_unix_file_gid,
				tvb, offset, 8, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS_SUBR(8);

		/* number of supplementary GIDs */
		CHECK_BYTE_COUNT_TRANS_SUBR(4);
		num_gids = tvb_get_letohl(tvb, offset);
		proto_tree_add_item(tree, hf_smb_unix_whoami_num_supl_gids,
				tvb, offset, 4, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS_SUBR(4);

		/* number of supplementary SIDs */
		CHECK_BYTE_COUNT_TRANS_SUBR(4);
		num_sids = tvb_get_letohl(tvb, offset);
		proto_tree_add_item(tree, hf_smb_unix_whoami_num_supl_sids,
				tvb, offset, 4, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS_SUBR(4);

		/* supplementary SIDs buffer length */
		CHECK_BYTE_COUNT_TRANS_SUBR(4);
		sids_buflen = tvb_get_letohl(tvb, offset);
		proto_tree_add_item(tree, hf_smb_unix_whoami_sids_buflen,
				tvb, offset, 4, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS_SUBR(4);

		/* pad / reserved (must be zero) */
		CHECK_BYTE_COUNT_TRANS_SUBR(4);
		proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 4, ENC_NA);
		COUNT_BYTES_TRANS_SUBR(4);


		/* GIDs */
		st_gids = proto_tree_add_subtree(tree, tvb, offset, num_gids * 8,
				ett_smb_unix_whoami_gids, NULL, "Supplementary UNIX GIDs");

		for (i = 0; i < num_gids; i++) {
			CHECK_BYTE_COUNT_TRANS_SUBR(8);
			proto_tree_add_item(st_gids, hf_smb_unix_file_gid,
					tvb, offset, 8, ENC_LITTLE_ENDIAN);
			COUNT_BYTES_TRANS_SUBR(8);
		}

		/* SIDs */
		st_sids = proto_tree_add_subtree(tree, tvb, offset, sids_buflen,
				ett_smb_unix_whoami_sids, NULL, "List of SIDs");

		for (i = 0; i < num_sids; i++) {
			old_sid_offset = offset;
			offset = dissect_nt_sid(tvb, offset, st_sids, "SID", NULL, -1);
			CHECK_BYTE_COUNT_TRANS_SUBR(offset-old_sid_offset);
			*bcp -= (offset - old_sid_offset);
		}

		break;
	}

	case 0x301: { 	/* MAC_QUERY_FS_INFO */
		static const int * support_flags[] = {
			&hf_smb_mac_sup_access_ctrl,
			&hf_smb_mac_sup_getset_comments,
			&hf_smb_mac_sup_desktopdb_calls,
			&hf_smb_mac_sup_unique_ids,
			&hf_smb_mac_sup_streams,
			NULL
		};

		/* Create time */
		CHECK_BYTE_COUNT_TRANS_SUBR(8);
		offset = dissect_nt_64bit_time(tvb, tree, offset, hf_smb_create_time);
		*bcp -= 8;
		/* Modify Time */
		CHECK_BYTE_COUNT_TRANS_SUBR(8);
		offset = dissect_nt_64bit_time(tvb, tree, offset, hf_smb_modify_time);
		*bcp -= 8;
		/* Backup Time */
		CHECK_BYTE_COUNT_TRANS_SUBR(8);
		offset = dissect_nt_64bit_time(tvb, tree, offset, hf_smb_backup_time);
		*bcp -= 8;
		/* Allocation blocks */
		CHECK_BYTE_COUNT_TRANS_SUBR(4);
		proto_tree_add_item(tree, hf_smb_mac_alloc_block_count, tvb,
				    offset,
				    4, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS_SUBR(4);
		/* Allocation Block Size */
		CHECK_BYTE_COUNT_TRANS_SUBR(4);
		proto_tree_add_item(tree, hf_smb_mac_alloc_block_size, tvb,
				    offset, 4, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS_SUBR(4);
		/* Free Block Count */
		CHECK_BYTE_COUNT_TRANS_SUBR(4);
		proto_tree_add_item(tree, hf_smb_mac_free_block_count, tvb,
				    offset, 4, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS_SUBR(4);
		/* Finder Info ... */
		CHECK_BYTE_COUNT_TRANS_SUBR(32);
		proto_tree_add_bytes_format_value(tree, hf_smb_mac_fndrinfo, tvb,
					    offset, 32, NULL,
					    "%s",
					    tvb_format_text(tvb, offset, 32));
		COUNT_BYTES_TRANS_SUBR(32);
		/* Number Files */
		CHECK_BYTE_COUNT_TRANS_SUBR(4);
		proto_tree_add_item(tree, hf_smb_mac_root_file_count, tvb,
				    offset, 4, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS_SUBR(4);
		/* Number of Root Directories */
		CHECK_BYTE_COUNT_TRANS_SUBR(4);
		proto_tree_add_item(tree, hf_smb_mac_root_dir_count, tvb,
				    offset, 4, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS_SUBR(4);
		/* Number of files */
		CHECK_BYTE_COUNT_TRANS_SUBR(4);
		proto_tree_add_item(tree, hf_smb_mac_file_count, tvb,
				    offset, 4, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS_SUBR(4);
		/* Dir Count */
		CHECK_BYTE_COUNT_TRANS_SUBR(4);
		proto_tree_add_item(tree, hf_smb_mac_dir_count, tvb,
				    offset, 4, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS_SUBR(4);
		/* Mac Support Flags */
		CHECK_BYTE_COUNT_TRANS_SUBR(4);
		proto_tree_add_bitmask(tree, tvb, offset, hf_smb_mac_sup, ett_smb_mac_support_flags, support_flags, ENC_LITTLE_ENDIAN);
		COUNT_BYTES_TRANS_SUBR(4);
		break;
	}
	case 1006:	/* QUERY_FS_QUOTA_INFO */
		offset = dissect_nt_quota(tvb, tree, offset, bcp);
		break;
	case 1007:	/* SMB_FS_FULL_SIZE_INFORMATION */
		offset = dissect_qfsi_FS_FULL_SIZE_INFO(tvb, pinfo, tree, offset, bcp);
		break;
	case 1008: /* Query Object ID */ {
		offset = dissect_qfsi_FS_OBJECTID_INFO(tvb, pinfo, tree, offset, bcp);
		break;
	    }
	}

	return offset;
}

static int
dissect_transaction2_response_data(tvbuff_t *tvb, packet_info *pinfo,
    proto_tree *parent_tree, smb_info_t *si)
{
	proto_item           *item   = NULL;
	proto_tree           *tree   = NULL;
	smb_transact2_info_t *t2i;
	int                   count;
	gboolean              trunc;
	int                   offset = 0;
	guint16               dc;

	dc = tvb_reported_length(tvb);

	DISSECTOR_ASSERT(si);

	if ((si->sip != NULL) && (si->sip->extra_info_type == SMB_EI_T2I))
		t2i = (smb_transact2_info_t *)si->sip->extra_info;
	else
		t2i = NULL;

	if (parent_tree) {
		if ((t2i != NULL) && (t2i->subcmd != -1)) {
			tree = proto_tree_add_subtree_format(parent_tree, tvb, offset, dc,
				ett_smb_transaction_data, &item, "%s Data",
				val_to_str_ext(t2i->subcmd, &trans2_cmd_vals_ext,
					       "Unknown (0x%02x)"));
		} else {
			tree = proto_tree_add_subtree(parent_tree, tvb, offset, dc,
					ett_smb_transaction_data, &item, "Unknown Transaction2 Data");
		}
	}

	if (t2i == NULL) {
		offset += dc;
		return offset;
	}
	switch(t2i->subcmd) {
	case 0x0000:	/*TRANS2_OPEN2*/
		/* XXX not implemented yet. See SNIA doc */
		break;
	case 0x0001:	/*TRANS2_FIND_FIRST2*/
		/* returned data */
		count = si->info_count;

		if (count == -1) {
			break;
		}

		if (count) {
			col_append_str(pinfo->cinfo, COL_INFO,
				       ", Files:");
		}

		while (count--) {
			offset = dissect_ff2_response_data(tvb, pinfo, tree,
							   offset, &dc, &trunc, si);
			if (trunc)
				break;
		}
		break;
	case 0x0002:	/*TRANS2_FIND_NEXT2*/
		/* returned data */
		count = si->info_count;

		if (count == -1) {
			break;
		}
		if (count) {
			col_append_str(pinfo->cinfo, COL_INFO,
				       ", Files:");
		}

		while (count--) {
			offset = dissect_ff2_response_data(tvb, pinfo, tree,
				offset, &dc, &trunc, si);
			if (trunc)
				break;
		}
		break;
	case 0x0003:	/*TRANS2_QUERY_FS_INFORMATION*/
		offset = dissect_qfsi_vals(tvb, pinfo, tree, offset, &dc, si);
		break;
	case 0x0004:	/*TRANS2_SET_FS_INFORMATION*/
		offset = dissect_sfsi_response(tvb, pinfo, tree, offset, &dc, si);
		break;
	case 0x0005:	/*TRANS2_QUERY_PATH_INFORMATION*/
		offset = dissect_qpi_loi_vals(tvb, pinfo, tree, item, offset, &dc, si);
		break;
	case 0x0006:	/*TRANS2_SET_PATH_INFORMATION*/
		/* no data in this response */
		break;
	case 0x0007:	/*TRANS2_QUERY_FILE_INFORMATION*/
		/* identical to QUERY_PATH_INFO */
		offset = dissect_qpi_loi_vals(tvb, pinfo, tree, item, offset, &dc, si);
		break;
	case 0x0008:	/*TRANS2_SET_FILE_INFORMATION*/
		/* no data in this response */
		break;
	case 0x0009:	/*TRANS2_FSCTL*/
		/* XXX don't know how to dissect this one (yet)*/

		/*
		 * XXX - "Microsoft Networks SMB File Sharing Protocol
		 * Extensions Version 3.0, Document Version 1.11,
		 * July 19, 1990" says this this contains a
		 * "File system specific return data block".
		 * (That means we may not be able to dissect it in any
		 * case.)
		 */
		break;
	case 0x000a:	/*TRANS2_IOCTL2*/
		/* XXX don't know how to dissect this one (yet)*/

		/*
		 * XXX - "Microsoft Networks SMB File Sharing Protocol
		 * Extensions Version 3.0, Document Version 1.11,
		 * July 19, 1990" says this this contains a
		 * "Device/function specific return data block".
		 * (That means we may not be able to dissect it in any
		 * case.)
		 */
		break;
	case 0x000b:	/*TRANS2_FIND_NOTIFY_FIRST*/
		/* XXX don't know how to dissect this one (yet)*/

		/*
		 * XXX - "Microsoft Networks SMB File Sharing Protocol
		 * Extensions Version 3.0, Document Version 1.11,
		 * July 19, 1990" says this this contains "the level
		 * dependent information about the changes which
		 * occurred".
		 */
		break;
	case 0x000c:	/*TRANS2_FIND_NOTIFY_NEXT*/
		/* XXX don't know how to dissect this one (yet)*/

		/*
		 * XXX - "Microsoft Networks SMB File Sharing Protocol
		 * Extensions Version 3.0, Document Version 1.11,
		 * July 19, 1990" says this this contains "the level
		 * dependent information about the changes which
		 * occurred".
		 */
		break;
	case 0x000d:	/*TRANS2_CREATE_DIRECTORY*/
		/* no data in this response */
		break;
	case 0x000e:	/*TRANS2_SESSION_SETUP*/
		/* XXX don't know how to dissect this one (yet)*/
		break;
	case 0x0010:	/*TRANS2_GET_DFS_REFERRAL*/
		offset = dissect_get_dfs_referral_data(tvb, pinfo, tree, offset, &dc, si->unicode);
		break;
	case 0x0011:	/*TRANS2_REPORT_DFS_INCONSISTENCY*/
		/* the SNIA spec appears to say the response has no data */
		break;
	case -1:
		/*
		 * We don't know what the matching request was; don't
		 * bother putting anything else into the tree for the data.
		 */
		offset += dc;
		dc = 0;
		break;
	}

	/* ooops there were data we didn't know how to process */
	if (dc != 0) {
		proto_tree_add_item(tree, hf_smb_unknown, tvb, offset, dc, ENC_NA);
		offset += dc;
	}

	return offset;
}


static int
dissect_transaction2_response_parameters(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree, smb_info_t *si)
{
	proto_item           *item   = NULL;
	proto_tree           *tree   = NULL;
	smb_transact2_info_t *t2i;
	guint16               fid;
	int                   lno;
	int                   offset = 0;
	int                   pc;

	pc = tvb_reported_length(tvb);

	DISSECTOR_ASSERT(si);

	if ((si->sip != NULL) && (si->sip->extra_info_type == SMB_EI_T2I))
		t2i = (smb_transact2_info_t *)si->sip->extra_info;
	else
		t2i = NULL;

	if (parent_tree) {
		if ((t2i != NULL) && (t2i->subcmd != -1)) {
			tree = proto_tree_add_subtree_format(parent_tree, tvb, offset, pc,
				ett_smb_transaction_params, &item, "%s Parameters",
				val_to_str_ext(t2i->subcmd, &trans2_cmd_vals_ext,
					       "Unknown (0x%02x)"));
		} else {
			tree = proto_tree_add_subtree(parent_tree, tvb, offset, pc,
				ett_smb_transaction_params, &item, "Unknown Transaction2 Parameters");
		}
	}

	if (t2i == NULL) {
		offset += pc;
		return offset;
	}
	switch(t2i->subcmd) {
	case 0x00:	/*TRANS2_OPEN2*/
		/* fid */
		fid = tvb_get_letohs(tvb, offset);
		dissect_smb_fid(tvb, pinfo, tree, offset, 2, fid, TRUE, FALSE, FALSE, si);
		offset += 2;

		/*
		 * XXX - Microsoft Networks SMB File Sharing Protocol
		 * Extensions Version 3.0, Document Version 1.11,
		 * July 19, 1990 says that the file attributes, create
		 * time (which it says is the last modification time),
		 * data size, granted access, file type, and IPC state
		 * are returned only if bit 0 is set in the open flags,
		 * and that the EA length is returned only if bit 3
		 * is set in the open flags.  Does that mean that,
		 * at least in that SMB dialect, those fields are not
		 * present in the reply parameters if the bits in
		 * question aren't set?
		 */

		/* File Attributes */
		offset = dissect_file_attributes(tvb, tree, offset);

		/* create time */
		offset = dissect_smb_datetime(tvb, tree, offset,
			hf_smb_create_time,
			hf_smb_create_dos_date, hf_smb_create_dos_time, TRUE);

		/* data size */
		proto_tree_add_item(tree, hf_smb_data_size, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		offset += 4;

		/* granted access */
		offset = dissect_access(tvb, tree, offset, hf_smb_granted_access);

		/* File Type */
		proto_tree_add_item(tree, hf_smb_file_type, tvb, offset, 2, ENC_LITTLE_ENDIAN);
		offset += 2;

		/* IPC State */
		offset = dissect_ipc_state(tvb, tree, offset, FALSE);

		/* open_action */
		offset = dissect_open_action(tvb, tree, offset);

		/* server unique file ID */
		proto_tree_add_item(tree, hf_smb_file_id, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		offset += 4;

		/* ea error offset, only a 16 bit integer here */
		proto_tree_add_uint(tree, hf_smb_ea_error_offset, tvb, offset, 2, tvb_get_letohs(tvb, offset));
		offset += 2;

		/* ea length */
		proto_tree_add_item(tree, hf_smb_ea_list_length, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		offset += 4;

		break;
	case 0x01:	/*TRANS2_FIND_FIRST2*/
		/* Find First2 information level */
		proto_tree_add_uint(tree, hf_smb_ff2_information_level, tvb, 0, 0, si->info_level);

		/* sid */
		proto_tree_add_item(tree, hf_smb_search_id, tvb, offset, 2, ENC_LITTLE_ENDIAN);
		offset += 2;

		/* search count */
		si->info_count = tvb_get_letohs(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_search_count, tvb, offset, 2, si->info_count);
		offset += 2;

		/* end of search */
		proto_tree_add_item(tree, hf_smb_end_of_search, tvb, offset, 2, ENC_LITTLE_ENDIAN);
		offset += 2;

		/* ea error offset, only a 16 bit integer here */
		proto_tree_add_uint(tree, hf_smb_ea_error_offset, tvb, offset, 2, tvb_get_letohs(tvb, offset));
		offset += 2;

		/* last name offset */
		lno = tvb_get_letohs(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_last_name_offset, tvb, offset, 2, lno);
		offset += 2;

		break;
	case 0x02:	/*TRANS2_FIND_NEXT2*/
		/* search count */
		si->info_count = tvb_get_letohs(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_search_count, tvb, offset, 2, si->info_count);
		offset += 2;

		/* end of search */
		proto_tree_add_item(tree, hf_smb_end_of_search, tvb, offset, 2, ENC_LITTLE_ENDIAN);
		offset += 2;

		/* ea_error_offset, only a 16 bit integer here*/
		proto_tree_add_uint(tree, hf_smb_ea_error_offset, tvb, offset, 2, tvb_get_letohs(tvb, offset));
		offset += 2;

		/* last name offset */
		lno = tvb_get_letohs(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_last_name_offset, tvb, offset, 2, lno);
		offset += 2;

		break;
	case 0x03:	/*TRANS2_QUERY_FS_INFORMATION*/
		/* no parameter block here */
		break;
	case 0x05:	/*TRANS2_QUERY_PATH_INFORMATION*/
		/* ea_error_offset, only a 16 bit integer here*/
		proto_tree_add_uint(tree, hf_smb_ea_error_offset, tvb, offset, 2, tvb_get_letohs(tvb, offset));
		offset += 2;

		break;
	case 0x06:	/*TRANS2_SET_PATH_INFORMATION*/
		/* ea_error_offset, only a 16 bit integer here*/
		proto_tree_add_uint(tree, hf_smb_ea_error_offset, tvb, offset, 2, tvb_get_letohs(tvb, offset));
		offset += 2;

		break;
	case 0x07:	/*TRANS2_QUERY_FILE_INFORMATION*/
		/* ea_error_offset, only a 16 bit integer here*/
		proto_tree_add_uint(tree, hf_smb_ea_error_offset, tvb, offset, 2, tvb_get_letohs(tvb, offset));
		offset += 2;

		break;
	case 0x08:	/*TRANS2_SET_FILE_INFORMATION*/
		/* ea_error_offset, only a 16 bit integer here*/
		proto_tree_add_uint(tree, hf_smb_ea_error_offset, tvb, offset, 2, tvb_get_letohs(tvb, offset));
		offset += 2;

		break;
	case 0x09:	/*TRANS2_FSCTL*/
		/* XXX don't know how to dissect this one (yet)*/

		/*
		 * XXX - "Microsoft Networks SMB File Sharing Protocol
		 * Extensions Version 3.0, Document Version 1.11,
		 * July 19, 1990" says this this contains a
		 * "File system specific return parameter block".
		 * (That means we may not be able to dissect it in any
		 * case.)
		 */
		break;
	case 0x0a:	/*TRANS2_IOCTL2*/
		/* XXX don't know how to dissect this one (yet)*/

		/*
		 * XXX - "Microsoft Networks SMB File Sharing Protocol
		 * Extensions Version 3.0, Document Version 1.11,
		 * July 19, 1990" says this this contains a
		 * "Device/function specific return parameter block".
		 * (That means we may not be able to dissect it in any
		 * case.)
		 */
		break;
	case 0x0b:	/*TRANS2_FIND_NOTIFY_FIRST*/
		/* Find Notify information level */
		proto_tree_add_uint(tree, hf_smb_fn_information_level, tvb, 0, 0, si->info_level);

		/* Monitor handle */
		proto_tree_add_item(tree, hf_smb_monitor_handle, tvb, offset, 2, ENC_LITTLE_ENDIAN);
		offset += 2;

		/* Change count */
		si->info_count = tvb_get_letohs(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_change_count, tvb, offset, 2, si->info_count);
		offset += 2;

		/* ea_error_offset, only a 16 bit integer here*/
		proto_tree_add_uint(tree, hf_smb_ea_error_offset, tvb, offset, 2, tvb_get_letohs(tvb, offset));
		offset += 2;

		break;
	case 0x0c:	/*TRANS2_FIND_NOTIFY_NEXT*/
		/* Find Notify information level */
		proto_tree_add_uint(tree, hf_smb_fn_information_level, tvb, 0, 0, si->info_level);

		/* Change count */
		si->info_count = tvb_get_letohs(tvb, offset);
		proto_tree_add_uint(tree, hf_smb_change_count, tvb, offset, 2, si->info_count);
		offset += 2;

		/* ea_error_offset, only a 16 bit integer here*/
		proto_tree_add_uint(tree, hf_smb_ea_error_offset, tvb, offset, 2, tvb_get_letohs(tvb, offset));
		offset += 2;

		break;
	case 0x0d:	/*TRANS2_CREATE_DIRECTORY*/
		/* ea error offset, only a 16 bit integer here */
		proto_tree_add_uint(tree, hf_smb_ea_error_offset, tvb, offset, 2, tvb_get_letohs(tvb, offset));
		offset += 2;

		break;
	case 0x0e:	/*TRANS2_SESSION_SETUP*/
		/* XXX don't know how to dissect this one (yet)*/
		break;
	case 0x10:	/*TRANS2_GET_DFS_REFERRAL*/
		/* XXX don't know how to dissect this one (yet) see SNIA doc*/
		break;
	case 0x11:	/*TRANS2_REPORT_DFS_INCONSISTENCY*/
		/* XXX don't know how to dissect this one (yet) see SNIA doc*/
		break;
	case -1:
		/*
		 * We don't know what the matching request was; don't
		 * bother putting anything else into the tree for the data.
		 */
		offset += pc;
		break;
	}

	/* ooops there were data we didn't know how to process */
	if (offset < pc) {
		proto_tree_add_item(tree, hf_smb_unknown, tvb, offset, pc-offset, ENC_NA);
		offset += pc-offset;
	}
	return offset;
}


static int
dissect_transaction_response(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si)
{
	guint8                sc, wc;
	guint16               od     = 0, po = 0, pc = 0, pd = 0, dc = 0, dd = 0, td = 0, tp = 0;
	smb_transact2_info_t *t2i    = NULL;
	guint16               bc;
	int                   padcnt;
	gboolean              dissected_trans;
	fragment_head        *r_fd   = NULL;
	tvbuff_t             *pd_tvb = NULL, *d_tvb = NULL, *p_tvb = NULL;
	tvbuff_t             *s_tvb  = NULL, *sp_tvb = NULL;
	gboolean              save_fragmented;
	proto_item           *item;

	DISSECTOR_ASSERT(si);

	switch(si->cmd) {
	case SMB_COM_TRANSACTION2:
		/* transaction2 */
		if ((si->sip != NULL) && (si->sip->extra_info_type == SMB_EI_T2I)) {
			t2i = (smb_transact2_info_t *)si->sip->extra_info;
		} else
			t2i = NULL;
		if (t2i == NULL) {
			/*
			 * We didn't see the matching request, so we don't
			 * know what type of transaction this is.
			 */
			proto_tree_add_uint_format_value(tree, hf_smb_trans2_subcmd, tvb, 0, 0, -1,
				"<UNKNOWN> since request packet wasn't seen");
			col_append_str(pinfo->cinfo, COL_INFO, "<unknown>");
		} else {
			si->info_level = t2i->info_level;
			if (t2i->subcmd == -1) {
				/*
				 * We didn't manage to extract the subcommand
				 * from the matching request (perhaps because
				 * the frame was short), so we don't know what
				 * type of transaction this is.
				 */
				proto_tree_add_uint_format_value(tree, hf_smb_trans2_subcmd, tvb, 0, 0, t2i->subcmd,
					"<UNKNOWN> since transaction code wasn't found in request packet");
				col_append_str(pinfo->cinfo, COL_INFO, "<unknown>");
			} else {
				proto_tree_add_uint(tree, hf_smb_trans2_subcmd, tvb, 0, 0, t2i->subcmd);
				switch (t2i->subcmd) {

				case 0x0001:	/* FIND_FIRST2 */
					if (t2i->info_level == -1)
						item = proto_tree_add_uint_format_value(tree, hf_smb_ff2_information_level, tvb, 0, 0, t2i->info_level,
										"<UNKNOWN> since information level wasn't found in request packet");
					else
						item = proto_tree_add_uint(tree, hf_smb_ff2_information_level, tvb, 0, 0, t2i->info_level);
					PROTO_ITEM_SET_GENERATED(item);
					if (t2i->name) {
						item = proto_tree_add_string(tree, hf_smb_search_pattern, tvb, 0, 0, t2i->name);
						PROTO_ITEM_SET_GENERATED(item);
					}
					break;

				case 0x0005:	/* QUERY_PATH_INFORMATION */
					if (t2i->info_level == -1)
						item = proto_tree_add_uint_format_value(tree, hf_smb_qpi_loi, tvb, 0, 0, t2i->info_level,
										"<UNKNOWN> since information level wasn't found in request packet");
					else
						item = proto_tree_add_uint(tree, hf_smb_qpi_loi, tvb, 0, 0, t2i->info_level);
					PROTO_ITEM_SET_GENERATED(item);
					if (t2i->name) {
						item = proto_tree_add_string(tree, hf_smb_file_name, tvb, 0, 0, t2i->name);
						PROTO_ITEM_SET_GENERATED(item);
					}
					break;

				case 0x0007:	/* QUERY_FILE_INFORMATION */
					if (t2i->info_level == -1)
						item = proto_tree_add_uint_format_value(tree, hf_smb_qpi_loi, tvb, 0, 0, t2i->info_level,
										"<UNKNOWN> since information level wasn't found in request packet");
					else
						item = proto_tree_add_uint(tree, hf_smb_qpi_loi, tvb, 0, 0, t2i->info_level);
					PROTO_ITEM_SET_GENERATED(item);
					break;

				case 0x0003:	/* QUERY_FS_INFORMATION */
					if (t2i->info_level == -1)
						item = proto_tree_add_uint_format_value(tree, hf_smb_qfsi_information_level, tvb, 0, 0, si->info_level,
										"<UNKNOWN> since information level wasn't found in request packet");
					else
						item = proto_tree_add_uint(tree, hf_smb_qfsi_information_level, tvb, 0, 0, si->info_level);
					PROTO_ITEM_SET_GENERATED(item);
					break;

				case 0x0004:	/* SET_FS_INFORMATION */
					if (t2i->info_level == -1)
						item = proto_tree_add_uint_format_value(tree, hf_smb_sfsi_information_level, tvb, 0, 0, si->info_level,
										"<UNKNOWN> since information level wasn't found in request packet");
					else
						item = proto_tree_add_uint(tree, hf_smb_sfsi_information_level, tvb, 0, 0, si->info_level);
					PROTO_ITEM_SET_GENERATED(item);
					break;
				}

				col_append_fstr(pinfo->cinfo, COL_INFO, ", %s",
						val_to_str_ext(t2i->subcmd,
							       &trans2_cmd_vals_ext,
							       "<unknown (0x%02x)>"));
			}
		}
		break;
	}

	WORD_COUNT;

	/* total param count, only a 16bit integer here */
	tp = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_total_param_count, tvb, offset, 2, tp);
	offset += 2;

	/* total data count, only a 16 bit integer here */
	td = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_total_data_count, tvb, offset, 2, td);
	offset += 2;

	/* 2 reserved bytes */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 2, ENC_NA);
	offset += 2;

	/* param count */
	pc = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_param_count16, tvb, offset, 2, pc);
	offset += 2;

	/* param offset */
	po = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_param_offset16, tvb, offset, 2, po);
	offset += 2;

	/* param disp */
	pd = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_param_disp16, tvb, offset, 2, pd);
	offset += 2;

	/* data count */
	dc = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_data_count16, tvb, offset, 2, dc);
	offset += 2;

	/* data offset */
	od = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_data_offset16, tvb, offset, 2, od);
	offset += 2;

	/* data disp */
	dd = tvb_get_letohs(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_data_disp16, tvb, offset, 2, dd);
	offset += 2;

	/* setup count */
	sc = tvb_get_guint8(tvb, offset);
	proto_tree_add_uint(tree, hf_smb_setup_count, tvb, offset, 1, sc);
	offset += 1;

	/* reserved byte */
	proto_tree_add_item(tree, hf_smb_reserved, tvb, offset, 1, ENC_NA);
	offset += 1;


	/* if there were any setup bytes, put them in a tvb for later */
	if (sc) {
		if ((2*sc) > tvb_reported_length_remaining(tvb, offset)) {
			s_tvb = tvb_new_subset(tvb, offset, tvb_reported_length_remaining(tvb, offset), 2*sc);
		} else {
			s_tvb = tvb_new_subset_length(tvb, offset, 2*sc);
		}
		sp_tvb = tvb_new_subset_remaining(tvb, offset);
	} else {
		s_tvb  = NULL;
		sp_tvb = NULL;
	}
	offset += 2*sc;


	BYTE_COUNT;


	/* reassembly of SMB Transaction data payload.
	   In this section we do reassembly of both the data and parameters
	   blocks of the SMB transaction command.
	*/
	save_fragmented = pinfo->fragmented;
	/* do we need reassembly? */
	if ( (td != dc) || (tp != pc) ) {
		/* oh yeah, either data or parameter section needs
		   reassembly
		*/
		pinfo->fragmented = TRUE;
		if (smb_trans_reassembly) {
			/* ...and we were told to do reassembly */
			if (pc) {
				r_fd = smb_trans_defragment(tree, pinfo, tvb,
							     po, pc, pd, td+tp, si);

			}
			if ((r_fd == NULL) && dc) {
				r_fd = smb_trans_defragment(tree, pinfo, tvb,
							     od, dc, dd+tp, td+tp, si);
			}
		}
	}

	/* if we got a reassembled fd structure from the reassembly routine we must
	   create pd_tvb from it
	*/
	if (r_fd) {
		proto_item *frag_tree_item;

		pd_tvb = tvb_new_chain(tvb, r_fd->tvb_data);
		add_new_data_source(pinfo, pd_tvb, "Reassembled SMB");
		show_fragment_tree(r_fd, &smb_frag_items, tree, pinfo, pd_tvb, &frag_tree_item);
	}


	if (pd_tvb) {
		/* OK we have reassembled data, extract d_tvb and p_tvb from it */
		if (tp) {
			p_tvb = tvb_new_subset_length(pd_tvb, 0, tp);
		}
		if (td) {
			d_tvb = tvb_new_subset_length(pd_tvb, tp, td);
		}
	} else {
		/* It was not reassembled. Do as best as we can.
		 * in this case we always try to dissect the stuff if
		 * data and param displacement is 0. i.e. for the first
		 * (and maybe only) packet.
		 */
		if ( (pd == 0) && (dd == 0) ) {
			int min;
			int reported_min;
			min = MIN(pc, tvb_reported_length_remaining(tvb, po));
			reported_min = MIN(pc, tvb_reported_length_remaining(tvb, po));
			if (min && reported_min) {
				p_tvb = tvb_new_subset(tvb, po, min, reported_min);
			}
			min = MIN(dc, tvb_reported_length_remaining(tvb, od));
			reported_min = MIN(dc, tvb_reported_length_remaining(tvb, od));
			if (min && reported_min) {
				d_tvb = tvb_new_subset(tvb, od, min, reported_min);
			}
			/*
			 * A tvbuff containing the parameters
			 * and the data.
			 * XXX - check pc and dc as well?
			 */
			if (tvb_reported_length_remaining(tvb, po)) {
				pd_tvb = tvb_new_subset_remaining(tvb, po);
			}
		}
	}



	/* parameters */
	if (po > offset) {
		/* We have some padding bytes.
		*/
		padcnt = po-offset;
		if (padcnt > bc)
			padcnt = bc;
		proto_tree_add_item(tree, hf_smb_padding, tvb, offset, padcnt, ENC_NA);
		COUNT_BYTES(padcnt);
	}
	if ((si->cmd == SMB_COM_TRANSACTION2) && p_tvb) {
		/* TRANSACTION2 parameters*/
		dissect_transaction2_response_parameters(p_tvb, pinfo, tree, si);
	}
	COUNT_BYTES(pc);


	/* data */
	if (od > offset) {
		/* We have some initial padding bytes.
		*/
		padcnt = od-offset;
		if (padcnt > bc)
			padcnt = bc;
		proto_tree_add_item(tree, hf_smb_padding, tvb, offset, padcnt, ENC_NA);
		COUNT_BYTES(padcnt);
	}
	/*
	 * If the data count is bigger than the count of bytes
	 * remaining, clamp it so that the count of bytes remaining
	 * doesn't go negative.
	 */
	if (dc > bc)
		dc = bc;
	COUNT_BYTES(dc);



	/* from now on, everything is in separate tvbuffs so we don't count
	   the bytes with COUNT_BYTES any more.
	   neither do we reference offset any more (which by now points to the
	   first byte AFTER this PDU */


	if ((si->cmd == SMB_COM_TRANSACTION2) && d_tvb) {
		/* TRANSACTION2 parameters*/
		dissect_transaction2_response_data(d_tvb, pinfo, tree, si);
	}


	if (si->cmd == SMB_COM_TRANSACTION) {
		smb_transact_info_t *tri;

		dissected_trans = FALSE;
		if ((si->sip != NULL) && (si->sip->extra_info_type == SMB_EI_TRI))
			tri = (smb_transact_info_t *)si->sip->extra_info;
		else
			tri = NULL;
		if (tri != NULL) {
			switch(tri->subcmd) {

			case TRANSACTION_PIPE:
				/* This function is safe to call for
				   s_tvb == sp_tvb == NULL, i.e. if we don't
				   know them at this point.
				   It's also safe to call if "p_tvb"
				   or "d_tvb" are null.
				*/
				if ( pd_tvb) {
					dissected_trans = dissect_pipe_smb(
						sp_tvb, s_tvb, pd_tvb, p_tvb,
						d_tvb, NULL, pinfo, top_tree_global, si);
				}
				break;

			case TRANSACTION_MAILSLOT:
				/* This one should be safe to call
				   even if s_tvb and sp_tvb is NULL
				*/
				if (d_tvb) {
					dissected_trans = dissect_mailslot_smb(
						sp_tvb, s_tvb, d_tvb, NULL, pinfo,
						top_tree_global, si);
				}
				break;
			}
		}
		if (!dissected_trans) {
			/* This one is safe to call for s_tvb == p_tvb == d_tvb == NULL */
			dissect_trans_data(s_tvb, p_tvb, d_tvb, tree);
		}
	}


	if ( (p_tvb == 0) && (d_tvb == 0) ) {
		col_append_str(pinfo->cinfo, COL_INFO,
				       "[transact continuation]");
	}

	pinfo->fragmented = save_fragmented;
	END_OF_SMB

	return offset;
}


static int
dissect_find_notify_close(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si _U_)
{
	guint8  wc;
	guint16 bc;

	WORD_COUNT;

	/* Monitor handle */
	proto_tree_add_item(tree, hf_smb_monitor_handle, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	BYTE_COUNT;

	END_OF_SMB

	return offset;
}

/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
   END Transaction/Transaction2 Primary and secondary requests
   XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */


static int
dissect_unknown(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, int offset, proto_tree *smb_tree _U_, smb_info_t *si _U_)
{
	guint8  wc;
	guint16 bc;

	WORD_COUNT;

	if (wc != 0) {
		proto_tree_add_item(tree, hf_smb_word_parameters, tvb, offset, wc*2, ENC_NA);
		offset += wc*2;
	}

	BYTE_COUNT;

	if (bc != 0) {
		proto_tree_add_item(tree, hf_smb_byte_parameters, tvb, offset, bc, ENC_NA);
		offset += bc;
		bc = 0;
	}

	END_OF_SMB

	return offset;
}

typedef struct _smb_function {
	int (*request)(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree, smb_info_t *si);
	int (*response)(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree, smb_info_t *si);
} smb_function;

static smb_function smb_dissector[256] = {
	/* 0x00 Create Dir*/                 {dissect_old_dir_request            , dissect_empty},
	/* 0x01 Delete Dir*/                 {dissect_old_dir_request            , dissect_empty},
	/* 0x02 Open File*/                  {dissect_open_file_request          , dissect_open_file_response},
	/* 0x03 Create File*/                {dissect_create_file_request        , dissect_create_file_response},
	/* 0x04 Close File*/                 {dissect_close_file_request         , dissect_empty},
	/* 0x05 Flush File*/                 {dissect_flush_file_request         , dissect_empty},
	/* 0x06 Delete File*/                {dissect_delete_file_request        , dissect_empty},
	/* 0x07 Rename File*/                {dissect_rename_file_request        , dissect_rename_file_response},
	/* 0x08 Query Info*/                 {dissect_query_information_request  , dissect_query_information_response},
	/* 0x09 Set Info*/                   {dissect_set_information_request    , dissect_empty},
	/* 0x0a Read File*/                  {dissect_read_file_request          , dissect_read_file_response},
	/* 0x0b Write File*/                 {dissect_write_file_request         , dissect_write_file_response},
	/* 0x0c Lock Byte Range*/            {dissect_lock_request               , dissect_empty},
	/* 0x0d Unlock Byte Range*/          {dissect_lock_request               , dissect_empty},
	/* 0x0e Create Temp*/                {dissect_create_temporary_request   , dissect_create_temporary_response},
	/* 0x0f Create New*/                 {dissect_create_file_request        , dissect_create_new_response},

	/* 0x10 Check Dir*/                  {dissect_old_dir_request            , dissect_empty},
	/* 0x11 Process Exit*/               {dissect_empty                      , dissect_empty},
	/* 0x12 Seek File*/                  {dissect_seek_file_request          , dissect_seek_file_response},
	/* 0x13 Lock And Read*/              {dissect_read_file_request          , dissect_lock_and_read_response},
	/* 0x14 Write And Unlock*/           {dissect_write_file_request         , dissect_write_file_response},
	/* 0x15 */                           {dissect_unknown                    , dissect_unknown},
	/* 0x16 */                           {dissect_unknown                    , dissect_unknown},
	/* 0x17 */                           {dissect_unknown                    , dissect_unknown},
	/* 0x18 */                           {dissect_unknown                    , dissect_unknown},
	/* 0x19 */                           {dissect_unknown                    , dissect_unknown},
	/* 0x1a Read Raw*/                   {dissect_read_raw_request           , dissect_unknown},
	/* 0x1b Read MPX*/                   {dissect_read_mpx_request           , dissect_read_mpx_response},
	/* 0x1c Read MPX Secondary*/         {dissect_unknown                    , dissect_unknown},
	/* 0x1d Write Raw*/                  {dissect_write_raw_request          , dissect_write_raw_response},
	/* 0x1e Write MPX*/                  {dissect_write_mpx_request          , dissect_write_mpx_response},
	/* 0x1f Write MPX Secondary*/        {dissect_unknown                    , dissect_unknown},

	/* 0x20 Write Complete*/             {dissect_unknown                    , dissect_write_and_close_response},
	/* 0x21 */                           {dissect_unknown                    , dissect_unknown},
	/* 0x22 Set Info2*/                  {dissect_set_information2_request   , dissect_empty},
	/* 0x23 Query Info2*/                {dissect_query_information2_request , dissect_query_information2_response},
	/* 0x24 Locking And X*/              {dissect_locking_andx_request       , dissect_locking_andx_response},
	/* 0x25 Transaction*/                {dissect_transaction_request        , dissect_transaction_response},
	/* 0x26 Transaction Secondary*/      {dissect_transaction_request        , dissect_unknown}, /*This SMB has no response */
	/* 0x27 IOCTL*/                      {dissect_unknown                    , dissect_unknown},
	/* 0x28 IOCTL Secondary*/            {dissect_unknown                    , dissect_unknown},
	/* 0x29 Copy File*/                  {dissect_copy_request               , dissect_move_copy_response},
	/* 0x2a Move File*/                  {dissect_move_request               , dissect_move_copy_response},
	/* 0x2b Echo*/                       {dissect_echo_request               , dissect_echo_response},
	/* 0x2c Write And Close*/            {dissect_write_and_close_request    , dissect_write_and_close_response},
	/* 0x2d Open And X*/                 {dissect_open_andx_request          , dissect_open_andx_response},
	/* 0x2e Read And X*/                 {dissect_read_andx_request          , dissect_read_andx_response},
	/* 0x2f Write And X*/                {dissect_write_andx_request         , dissect_write_andx_response},

	/* 0x30 */                           {dissect_unknown                    , dissect_unknown},
	/* 0x31 Close And Tree Disconnect */ {dissect_close_file_request         , dissect_empty},
	/* 0x32 Transaction2*/	             {dissect_transaction_request        , dissect_transaction_response},
	/* 0x33 Transaction2 Secondary*/     {dissect_transaction_request        , dissect_unknown}, /*This SMB has no response */
	/* 0x34 Find Close2*/                {dissect_sid                        , dissect_empty},
	/* 0x35 Find Notify Close*/          {dissect_find_notify_close          , dissect_empty},
	/* 0x36 */  {dissect_unknown, dissect_unknown},
	/* 0x37 */  {dissect_unknown, dissect_unknown},
	/* 0x38 */  {dissect_unknown, dissect_unknown},
	/* 0x39 */  {dissect_unknown, dissect_unknown},
	/* 0x3a */  {dissect_unknown, dissect_unknown},
	/* 0x3b */  {dissect_unknown, dissect_unknown},
	/* 0x3c */  {dissect_unknown, dissect_unknown},
	/* 0x3d */  {dissect_unknown, dissect_unknown},
	/* 0x3e */  {dissect_unknown, dissect_unknown},
	/* 0x3f */  {dissect_unknown, dissect_unknown},

	/* 0x40 */  {dissect_unknown, dissect_unknown},
	/* 0x41 */  {dissect_unknown, dissect_unknown},
	/* 0x42 */  {dissect_unknown, dissect_unknown},
	/* 0x43 */  {dissect_unknown, dissect_unknown},
	/* 0x44 */  {dissect_unknown, dissect_unknown},
	/* 0x45 */  {dissect_unknown, dissect_unknown},
	/* 0x46 */  {dissect_unknown, dissect_unknown},
	/* 0x47 */  {dissect_unknown, dissect_unknown},
	/* 0x48 */  {dissect_unknown, dissect_unknown},
	/* 0x49 */  {dissect_unknown, dissect_unknown},
	/* 0x4a */  {dissect_unknown, dissect_unknown},
	/* 0x4b */  {dissect_unknown, dissect_unknown},
	/* 0x4c */  {dissect_unknown, dissect_unknown},
	/* 0x4d */  {dissect_unknown, dissect_unknown},
	/* 0x4e */  {dissect_unknown, dissect_unknown},
	/* 0x4f */  {dissect_unknown, dissect_unknown},

	/* 0x50 */  {dissect_unknown, dissect_unknown},
	/* 0x51 */  {dissect_unknown, dissect_unknown},
	/* 0x52 */  {dissect_unknown, dissect_unknown},
	/* 0x53 */  {dissect_unknown, dissect_unknown},
	/* 0x54 */  {dissect_unknown, dissect_unknown},
	/* 0x55 */  {dissect_unknown, dissect_unknown},
	/* 0x56 */  {dissect_unknown, dissect_unknown},
	/* 0x57 */  {dissect_unknown, dissect_unknown},
	/* 0x58 */  {dissect_unknown, dissect_unknown},
	/* 0x59 */  {dissect_unknown, dissect_unknown},
	/* 0x5a */  {dissect_unknown, dissect_unknown},
	/* 0x5b */  {dissect_unknown, dissect_unknown},
	/* 0x5c */  {dissect_unknown, dissect_unknown},
	/* 0x5d */  {dissect_unknown, dissect_unknown},
	/* 0x5e */  {dissect_unknown, dissect_unknown},
	/* 0x5f */  {dissect_unknown, dissect_unknown},

	/* 0x60 */  {dissect_unknown, dissect_unknown},
	/* 0x61 */  {dissect_unknown, dissect_unknown},
	/* 0x62 */  {dissect_unknown, dissect_unknown},
	/* 0x63 */  {dissect_unknown, dissect_unknown},
	/* 0x64 */  {dissect_unknown, dissect_unknown},
	/* 0x65 */  {dissect_unknown, dissect_unknown},
	/* 0x66 */  {dissect_unknown, dissect_unknown},
	/* 0x67 */  {dissect_unknown, dissect_unknown},
	/* 0x68 */  {dissect_unknown, dissect_unknown},
	/* 0x69 */  {dissect_unknown, dissect_unknown},
	/* 0x6a */  {dissect_unknown, dissect_unknown},
	/* 0x6b */  {dissect_unknown, dissect_unknown},
	/* 0x6c */  {dissect_unknown, dissect_unknown},
	/* 0x6d */  {dissect_unknown, dissect_unknown},
	/* 0x6e */  {dissect_unknown, dissect_unknown},
	/* 0x6f */  {dissect_unknown, dissect_unknown},

	/* 0x70 Tree Connect*/	             {dissect_tree_connect_request       , dissect_tree_connect_response},
	/* 0x71 Tree Disconnect*/	     {dissect_empty                      , dissect_empty},
	/* 0x72 Negotiate Protocol*/	     {dissect_negprot_request            , dissect_negprot_response},
	/* 0x73 Session Setup And X*/        {dissect_session_setup_andx_request , dissect_session_setup_andx_response},
	/* 0x74 Logoff And X*/	             {dissect_empty_andx                 , dissect_empty_andx},
	/* 0x75 Tree Connect And X*/         {dissect_tree_connect_andx_request  , dissect_tree_connect_andx_response},
	/* 0x76 */  {dissect_unknown, dissect_unknown},
	/* 0x77 */  {dissect_unknown, dissect_unknown},
	/* 0x78 */  {dissect_unknown, dissect_unknown},
	/* 0x79 */  {dissect_unknown, dissect_unknown},
	/* 0x7a */  {dissect_unknown, dissect_unknown},
	/* 0x7b */  {dissect_unknown, dissect_unknown},
	/* 0x7c */  {dissect_unknown, dissect_unknown},
	/* 0x7d */  {dissect_unknown, dissect_unknown},
	/* 0x7e */  {dissect_unknown, dissect_unknown},
	/* 0x7f */  {dissect_unknown, dissect_unknown},

	/* 0x80 Query Info Disk*/            {dissect_empty              , dissect_query_information_disk_response},
	/* 0x81 Search Dir*/                 {dissect_search_dir_request , dissect_search_dir_response},
	/* 0x82 Find*/                       {dissect_find_request       , dissect_find_response},
	/* 0x83 Find Unique*/                {dissect_find_request       , dissect_find_response},
	/* 0x84 Find Close*/                 {dissect_find_close_request , dissect_find_close_response},
	/* 0x85 */  {dissect_unknown, dissect_unknown},
	/* 0x86 */  {dissect_unknown, dissect_unknown},
	/* 0x87 */  {dissect_unknown, dissect_unknown},
	/* 0x88 */  {dissect_unknown, dissect_unknown},
	/* 0x89 */  {dissect_unknown, dissect_unknown},
	/* 0x8a */  {dissect_unknown, dissect_unknown},
	/* 0x8b */  {dissect_unknown, dissect_unknown},
	/* 0x8c */  {dissect_unknown, dissect_unknown},
	/* 0x8d */  {dissect_unknown, dissect_unknown},
	/* 0x8e */  {dissect_unknown, dissect_unknown},
	/* 0x8f */  {dissect_unknown, dissect_unknown},

	/* 0x90 */  {dissect_unknown, dissect_unknown},
	/* 0x91 */  {dissect_unknown, dissect_unknown},
	/* 0x92 */  {dissect_unknown, dissect_unknown},
	/* 0x93 */  {dissect_unknown, dissect_unknown},
	/* 0x94 */  {dissect_unknown, dissect_unknown},
	/* 0x95 */  {dissect_unknown, dissect_unknown},
	/* 0x96 */  {dissect_unknown, dissect_unknown},
	/* 0x97 */  {dissect_unknown, dissect_unknown},
	/* 0x98 */  {dissect_unknown, dissect_unknown},
	/* 0x99 */  {dissect_unknown, dissect_unknown},
	/* 0x9a */  {dissect_unknown, dissect_unknown},
	/* 0x9b */  {dissect_unknown, dissect_unknown},
	/* 0x9c */  {dissect_unknown, dissect_unknown},
	/* 0x9d */  {dissect_unknown, dissect_unknown},
	/* 0x9e */  {dissect_unknown, dissect_unknown},
	/* 0x9f */  {dissect_unknown, dissect_unknown},

	/* 0xa0 NT Transaction*/             {dissect_nt_transaction_request , dissect_nt_transaction_response},
	/* 0xa1 NT Trans secondary*/         {dissect_nt_transaction_request , dissect_nt_transaction_response},
	/* 0xa2 NT CreateAndX*/              {dissect_nt_create_andx_request , dissect_nt_create_andx_response},
	/* 0xa3 */  {dissect_unknown, dissect_unknown},
	/* 0xa4 NT Cancel*/	             {dissect_nt_cancel_request      , dissect_unknown}, /*no response to this one*/
	/* 0xa5 NT Rename*/                  {dissect_nt_rename_file_request , dissect_empty},
	/* 0xa6 */  {dissect_unknown, dissect_unknown},
	/* 0xa7 */  {dissect_unknown, dissect_unknown},
	/* 0xa8 */  {dissect_unknown, dissect_unknown},
	/* 0xa9 */  {dissect_unknown, dissect_unknown},
	/* 0xaa */  {dissect_unknown, dissect_unknown},
	/* 0xab */  {dissect_unknown, dissect_unknown},
	/* 0xac */  {dissect_unknown, dissect_unknown},
	/* 0xad */  {dissect_unknown, dissect_unknown},
	/* 0xae */  {dissect_unknown, dissect_unknown},
	/* 0xaf */  {dissect_unknown, dissect_unknown},

	/* 0xb0 */  {dissect_unknown, dissect_unknown},
	/* 0xb1 */  {dissect_unknown, dissect_unknown},
	/* 0xb2 */  {dissect_unknown, dissect_unknown},
	/* 0xb3 */  {dissect_unknown, dissect_unknown},
	/* 0xb4 */  {dissect_unknown, dissect_unknown},
	/* 0xb5 */  {dissect_unknown, dissect_unknown},
	/* 0xb6 */  {dissect_unknown, dissect_unknown},
	/* 0xb7 */  {dissect_unknown, dissect_unknown},
	/* 0xb8 */  {dissect_unknown, dissect_unknown},
	/* 0xb9 */  {dissect_unknown, dissect_unknown},
	/* 0xba */  {dissect_unknown, dissect_unknown},
	/* 0xbb */  {dissect_unknown, dissect_unknown},
	/* 0xbc */  {dissect_unknown, dissect_unknown},
	/* 0xbd */  {dissect_unknown, dissect_unknown},
	/* 0xbe */  {dissect_unknown, dissect_unknown},
	/* 0xbf */  {dissect_unknown, dissect_unknown},

	/* 0xc0 Open Print File*/            {dissect_open_print_file_request  , dissect_open_print_file_response},
	/* 0xc1 Write Print File*/           {dissect_write_print_file_request , dissect_empty},
	/* 0xc2 Close Print File*/           {dissect_close_print_file_request , dissect_empty},
	/* 0xc3 Get Print Queue*/            {dissect_get_print_queue_request  , dissect_get_print_queue_response},
	/* 0xc4 */  {dissect_unknown, dissect_unknown},
	/* 0xc5 */  {dissect_unknown, dissect_unknown},
	/* 0xc6 */  {dissect_unknown, dissect_unknown},
	/* 0xc7 */  {dissect_unknown, dissect_unknown},
	/* 0xc8 */  {dissect_unknown, dissect_unknown},
	/* 0xc9 */  {dissect_unknown, dissect_unknown},
	/* 0xca */  {dissect_unknown, dissect_unknown},
	/* 0xcb */  {dissect_unknown, dissect_unknown},
	/* 0xcc */  {dissect_unknown, dissect_unknown},
	/* 0xcd */  {dissect_unknown, dissect_unknown},
	/* 0xce */  {dissect_unknown, dissect_unknown},
	/* 0xcf */  {dissect_unknown, dissect_unknown},

	/* 0xd0 Send Single Block Message*/         {dissect_send_single_block_message_request      , dissect_empty},
	/* 0xd1 Send Broadcast Message*/            {dissect_send_single_block_message_request      , dissect_empty},
	/* 0xd2 Forward User Name*/                 {dissect_forwarded_name                         , dissect_empty},
	/* 0xd3 Cancel Forward*/                    {dissect_forwarded_name                         , dissect_empty},
	/* 0xd4 Get Machine Name*/                  {dissect_empty                                  , dissect_get_machine_name_response},
	/* 0xd5 Send Start of Multi-block Message*/ {dissect_send_multi_block_message_start_request , dissect_message_group_id},
	/* 0xd6 Send End of Multi-block Message*/   {dissect_message_group_id                       , dissect_empty},
	/* 0xd7 Send Text of Multi-block Message*/  {dissect_send_multi_block_message_text_request  , dissect_empty},
	/* 0xd8 SMBreadbulk*/                       {dissect_unknown                                , dissect_unknown},
	/* 0xd9 SMBwritebulk*/                      {dissect_unknown                                , dissect_unknown},
	/* 0xda SMBwritebulkdata*/                  {dissect_unknown                                , dissect_unknown},
	/* 0xdb */  {dissect_unknown, dissect_unknown},
	/* 0xdc */  {dissect_unknown, dissect_unknown},
	/* 0xdd */  {dissect_unknown, dissect_unknown},
	/* 0xde */  {dissect_unknown, dissect_unknown},
	/* 0xdf */  {dissect_unknown, dissect_unknown},

	/* 0xe0 */  {dissect_unknown, dissect_unknown},
	/* 0xe1 */  {dissect_unknown, dissect_unknown},
	/* 0xe2 */  {dissect_unknown, dissect_unknown},
	/* 0xe3 */  {dissect_unknown, dissect_unknown},
	/* 0xe4 */  {dissect_unknown, dissect_unknown},
	/* 0xe5 */  {dissect_unknown, dissect_unknown},
	/* 0xe6 */  {dissect_unknown, dissect_unknown},
	/* 0xe7 */  {dissect_unknown, dissect_unknown},
	/* 0xe8 */  {dissect_unknown, dissect_unknown},
	/* 0xe9 */  {dissect_unknown, dissect_unknown},
	/* 0xea */  {dissect_unknown, dissect_unknown},
	/* 0xeb */  {dissect_unknown, dissect_unknown},
	/* 0xec */  {dissect_unknown, dissect_unknown},
	/* 0xed */  {dissect_unknown, dissect_unknown},
	/* 0xee */  {dissect_unknown, dissect_unknown},
	/* 0xef */  {dissect_unknown, dissect_unknown},

	/* 0xf0 */  {dissect_unknown, dissect_unknown},
	/* 0xf1 */  {dissect_unknown, dissect_unknown},
	/* 0xf2 */  {dissect_unknown, dissect_unknown},
	/* 0xf3 */  {dissect_unknown, dissect_unknown},
	/* 0xf4 */  {dissect_unknown, dissect_unknown},
	/* 0xf5 */  {dissect_unknown, dissect_unknown},
	/* 0xf6 */  {dissect_unknown, dissect_unknown},
	/* 0xf7 */  {dissect_unknown, dissect_unknown},
	/* 0xf8 */  {dissect_unknown, dissect_unknown},
	/* 0xf9 */  {dissect_unknown, dissect_unknown},
	/* 0xfa */  {dissect_unknown, dissect_unknown},
	/* 0xfb */  {dissect_unknown, dissect_unknown},
	/* 0xfc */  {dissect_unknown, dissect_unknown},
	/* 0xfd */  {dissect_unknown, dissect_unknown},
	/* 0xfe */  {dissect_unknown, dissect_unknown},
	/* 0xff */  {dissect_unknown, dissect_unknown},
};

static int
dissect_smb_command(tvbuff_t *tvb, packet_info *pinfo, int offset, proto_tree *smb_tree, guint8 cmd, gboolean first_pdu, smb_info_t *si)
{
	smb_saved_info_t *sip;

	DISSECTOR_ASSERT(si);

	if (cmd != 0xff) {
		proto_item *cmd_item;
		proto_tree *cmd_tree;
		int (*dissector)(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, proto_tree *smb_tree, smb_info_t *si);

		if (first_pdu) {
			col_append_fstr(pinfo->cinfo, COL_INFO,
				"%s %s",
				val_to_str_ext(cmd, &smb_cmd_vals_ext, "Unknown (0x%02x)"),
				(si->request)? "Request" : "Response");
		} else {
			col_append_fstr(pinfo->cinfo, COL_INFO,
				"; %s",
				val_to_str_ext(cmd, &smb_cmd_vals_ext, "Unknown (0x%02x)"));
		}

		cmd_tree = proto_tree_add_subtree_format(smb_tree, tvb, offset, -1,
			ett_smb_command, &cmd_item, "%s %s (0x%02x)",
			val_to_str_ext_const(cmd, &smb_cmd_vals_ext, "Unknown"),
			(si->request)?"Request":"Response",
			cmd);

		/* we track FIDs on a per transaction basis.
		   if this was a request and the fid was seen in a reply
		   we add a "generated" fid tree for this pdu and v.v.
		 */
		sip = si->sip;
		if (sip && sip->fid) {
			if ( (si->request && (!sip->fid_seen_in_request))
			     || ((!si->request) && sip->fid_seen_in_request) ) {
				dissect_smb_fid(tvb, pinfo, cmd_tree, offset, 0, sip->fid, FALSE, FALSE, TRUE, si);
			}
		}

		dissector = (si->request) ?
			smb_dissector[cmd].request : smb_dissector[cmd].response;

		offset = (*dissector)(tvb, pinfo, cmd_tree, offset, smb_tree, si);

		if (!tvb_offset_exists(tvb, offset-1)) {
			THROW(ReportedBoundsError);
		}
		proto_item_set_end(cmd_item, tvb, offset);
	}
	return offset;
}

static const value_string smb_cmd_vals[] = {
	{ 0x00, "Create Directory" },
	{ 0x01, "Delete Directory" },
	{ 0x02, "Open" },
	{ 0x03, "Create" },
	{ 0x04, "Close" },
	{ 0x05, "Flush" },
	{ 0x06, "Delete" },
	{ 0x07, "Rename" },
	{ 0x08, "Query Information" },
	{ 0x09, "Set Information" },
	{ 0x0A, "Read" },
	{ 0x0B, "Write" },
	{ 0x0C, "Lock Byte Range" },
	{ 0x0D, "Unlock Byte Range" },
	{ 0x0E, "Create Temp" },
	{ 0x0F, "Create New" },
	{ 0x10, "Check Directory" },
	{ 0x11, "Process Exit" },
	{ 0x12, "Seek" },
	{ 0x13, "Lock And Read" },
	{ 0x14, "Write And Unlock" },
	{ 0x15, "unknown-0x15" },
	{ 0x16, "unknown-0x16" },
	{ 0x17, "unknown-0x17" },
	{ 0x18, "unknown-0x18" },
	{ 0x19, "unknown-0x19" },
	{ 0x1A, "Read Raw" },
	{ 0x1B, "Read MPX" },
	{ 0x1C, "Read MPX Secondary" },
	{ 0x1D, "Write Raw" },
	{ 0x1E, "Write MPX" },
	{ 0x1F, "Write MPX Secondary" },
	{ 0x20, "Write Complete" },

	/*
	 * To quote
	 *
	 *    http://msdn.microsoft.com/en-us/library/ee442098.aspx
	 *
	 * "This command was introduced in the NT LAN Manager dialect, and
	 * was reserved but not implemented.
	 *
	 * Clients SHOULD NOT send requests using this command code, and
	 * servers receiving requests with this command code SHOULD return
	 * STATUS_NOT_IMPLEMENTED (ERRDOS/ERRbadfunc)."
	 */
	{ 0x21, "Query Server (reserved)" },

	{ 0x22, "Set Information2" },
	{ 0x23, "Query Information2" },
	{ 0x24, "Locking AndX" },
	{ 0x25, "Trans" },
	{ 0x26, "Trans Secondary" },
	{ 0x27, "IOCTL" },
	{ 0x28, "IOCTL Secondary" },
	{ 0x29, "Copy" },
	{ 0x2A, "Move" },
	{ 0x2B, "Echo" },
	{ 0x2C, "Write And Close" },
	{ 0x2D, "Open AndX" },
	{ 0x2E, "Read AndX" },
	{ 0x2F, "Write AndX" },

	/*
	 * To quote
	 *
	 *    http://msdn.microsoft.com/en-us/library/ee442127.aspx
	 *
	 * "This command was reserved but not implemented. It was also never
	 * defined. It is listed in [SNIA], but it is not defined in that
	 * document and does not appear in any other references.
	 *
	 * Clients SHOULD NOT send requests using this command code, and
	 * servers receiving requests with this command code SHOULD return
	 * STATUS_NOT_IMPLEMENTED (ERRDOC/ERRbadfunc)."
	 */
	{ 0x30, "New File Size (reserved)" },

	{ 0x31, "Close And Tree Disconnect" },
	{ 0x32, "Trans2" },
	{ 0x33, "Trans2 Secondary" },
	{ 0x34, "Find Close2" },
	{ 0x35, "Find Notify Close" },
	{ 0x70, "Tree Connect" },
	{ 0x71, "Tree Disconnect" },
	{ 0x72, "Negotiate Protocol" },
	{ 0x73, "Session Setup AndX" },
	{ 0x74, "Logoff AndX" },
	{ 0x75, "Tree Connect AndX" },
	{ 0x80, "Query Information Disk" },
	{ 0x81, "Search" },
	{ 0x82, "Find" },
	{ 0x83, "Find Unique" },
	{ 0x84, "Find Close" },
	{ 0xA0, "NT Trans" },
	{ 0xA1, "NT Trans Secondary" },
	{ 0xA2, "NT Create AndX" },
	{ 0xA3, "unknown-0xA3" },
	{ 0xA4, "NT Cancel" },
	{ 0xA5, "NT Rename" },
	{ 0xC0, "Open Print File" },
	{ 0xC1, "Write Print File" },
	{ 0xC2, "Close Print File" },
	{ 0xC3, "Get Print Queue" },
	{ 0xD0, "Send Single Block Message" },
	{ 0xD1, "Send Broadcast Message" },
	{ 0xD2, "Forward User Name" },
	{ 0xD3, "Cancel Forward" },
	{ 0xD4, "Get Machine Name" },
	{ 0xD5, "Send Start of Multi-block Message" },
	{ 0xD6, "Send End of Multi-block Message" },
	{ 0xD7, "Send Text of Multi-block Message" },
	{ 0xD8, "SMBreadbulk" },
	{ 0xD9, "SMBwritebulk" },
	{ 0xDA, "SMBwritebulkdata" },
	{ 0xFE, "SMBinvalid" },
	{ 0x00, NULL },
};
value_string_ext smb_cmd_vals_ext = VALUE_STRING_EXT_INIT(smb_cmd_vals);


static void
free_hash_tables(gpointer ctarg, gpointer user_data _U_)
{
	conv_tables_t *ct = (conv_tables_t *)ctarg;

	if (ct->unmatched)
		g_hash_table_destroy(ct->unmatched);
	if (ct->matched)
		g_hash_table_destroy(ct->matched);
	if (ct->primaries)
		g_hash_table_destroy(ct->primaries);
	if (ct->tid_service)
		g_hash_table_destroy(ct->tid_service);
	if (ct->GSL_fid_info)
		g_slist_free(ct->GSL_fid_info);
	g_free(ct);
}

static void
smb_cleanup(void)
{
	if (conv_tables) {
		g_slist_foreach(conv_tables, free_hash_tables, NULL);
		g_slist_free(conv_tables);
		conv_tables = NULL;
	}
}

static const value_string errcls_types[] = {
	{ SMB_SUCCESS, "Success"},
	{ SMB_ERRDOS,  "DOS Error"},
	{ SMB_ERRSRV,  "Server Error"},
	{ SMB_ERRHRD,  "Hardware Error"},
	{ SMB_ERRCMD,  "Command Error - Not an SMB format command"},
	{ 0, NULL }
};

/* Error codes for the ERRSRV class */

#define SRV_errors_VALUE_STRING_LIST(XXX)				\
	XXX( SMBE_SRV_error,          1, "Non specific error code")	\
	XXX( SMBE_SRV_badpw,          2, "Bad password")		\
	XXX( SMBE_SRV_badtype,        3, "Reserved")			\
	XXX( SMBE_SRV_access,         4, "No permissions to perform the requested operation") \
	XXX( SMBE_SRV_invnid,         5, "TID invalid")			\
	XXX( SMBE_SRV_invnetname,     6, "Invalid network name. Service not found") \
	XXX( SMBE_SRV_invdevice,      7, "Invalid device")		\
	XXX( SMBE_SRV_unknownsmb,    22, "Unknown SMB, from NT 3.5 response") \
	XXX( SMBE_SRV_qfull,         49, "Print queue full")		\
	XXX( SMBE_SRV_qtoobig,       50, "Queued item too big")		\
	XXX( SMBE_SRV_qeof,          51, "EOF in print queue dump")	\
	XXX( SMBE_SRV_invpfid,       52, "Invalid print file in smb_fid") \
	XXX( SMBE_SRV_smbcmd,        64, "Unrecognised command")	\
	XXX( SMBE_SRV_srverror,      65, "SMB server internal error")	\
	XXX( SMBE_SRV_filespecs,     67, "Fid and pathname invalid combination") \
	XXX( SMBE_SRV_badlink,       68, "Bad link in request ???")	\
	XXX( SMBE_SRV_badpermits,    69, "Access specified for a file is not valid") \
	XXX( SMBE_SRV_badpid,        70, "Bad process id in request")	\
	XXX( SMBE_SRV_setattrmode,   71, "Attribute mode invalid")	\
	XXX( SMBE_SRV_paused,        81, "Message server paused")	\
	XXX( SMBE_SRV_msgoff,        82, "Not receiving messages")	\
	XXX( SMBE_SRV_noroom,        83, "No room for message")		\
	XXX( SMBE_SRV_rmuns,         87, "Too many remote usernames")	\
	XXX( SMBE_SRV_timeout,       88, "Operation timed out")		\
	XXX( SMBE_SRV_noresource,    89, "No resources currently available for request.") \
	XXX( SMBE_SRV_toomanyuids,   90, "Too many userids")		\
	XXX( SMBE_SRV_baduid,        91, "Bad userid")			\
	XXX( SMBE_SRV_useMPX,       250, "Temporarily unable to use raw mode, use MPX mode") \
	XXX( SMBE_SRV_useSTD,       251, "Temporarily unable to use raw mode, use standard mode") \
	XXX( SMBE_SRV_contMPX,      252, "Resume MPX mode")		\
	XXX( SMBE_SRV_badPW,        253, "Bad Password???")		\
	XXX( SMBE_SRV_nosupport, 0xFFFF, "Operation not supported")

#if 0 /* Values not needed */
VALUE_STRING_ENUM(SRV_errors);
#endif
VALUE_STRING_ARRAY(SRV_errors);
static value_string_ext SRV_errors_ext = VALUE_STRING_EXT_INIT(SRV_errors);


/* Error codes for the ERRHRD class */

#define HRD_errors_VALUE_STRING_LIST(XXX)				\
	XXX( SMBE_HRD_nowrite,     19, "Read only media")		\
	XXX( SMBE_HRD_badunit,     20, "Unknown device")		\
	XXX( SMBE_HRD_notready,    21, "Drive not ready")		\
	XXX( SMBE_HRD_badcmd,      22, "Unknown command")		\
	XXX( SMBE_HRD_data,        23, "Data (CRC) error")		\
	XXX( SMBE_HRD_badreq,      24, "Bad request structure length")	\
	XXX( SMBE_HRD_seek,        25, "Seek error")			\
	XXX( SMBE_HRD_badmedia,    26, "Unknown media type")		\
	XXX( SMBE_HRD_badsector,   27, "Sector not found")		\
	XXX( SMBE_HRD_nopaper,     28, "Printer out of paper")		\
	XXX( SMBE_HRD_write,       29, "Write fault")			\
	XXX( SMBE_HRD_read,        30, "Read fault")			\
	XXX( SMBE_HRD_general,     31, "General failure")		\
	/* -- (really part of ERRDOS class ??) -- */			\
	XXX( SMBE_HRD_badshare,    32, "An open conflicts with an existing open") \
	XXX( SMBE_HRD_lock,        33, "Lock conflict/invalid mode, or unlock of another process's lock") \
	/* -- --*/							\
	XXX( SMBE_HRD_wrongdisk,   34, "The wrong disk was found in a drive") \
	XXX( SMBE_HRD_FCBunavail,  35, "No FCBs are available to process request") \
	XXX( SMBE_HRD_sharebufexc, 36, "A sharing buffer has been exceeded") \
	XXX( SMBE_HRD_diskfull,    39, "Disk full???")

#if 0 /* Values not needed */
VALUE_STRING_ENUM(HRD_errors);
#endif
VALUE_STRING_ARRAY(HRD_errors);
static value_string_ext HRD_errors_ext = VALUE_STRING_EXT_INIT(HRD_errors);

static const char *decode_smb_error(guint8 errcls, guint16 errcode)
{

	switch (errcls) {

	case SMB_SUCCESS:

		return("No Error");   /* No error ??? */

	case SMB_ERRDOS:

		return(val_to_str_ext(errcode, &DOS_errors_ext, "Unknown DOS error (%x)"));

	case SMB_ERRSRV:

		return(val_to_str_ext(errcode, &SRV_errors_ext, "Unknown SRV error (%x)"));

	case SMB_ERRHRD:

		return(val_to_str_ext(errcode, &HRD_errors_ext, "Unknown HRD error (%x)"));

	default:

		return("Unknown error class!");

	}

}

static const true_false_string tfs_smb_flags_lock = {
	"Lock&Read, Write&Unlock are supported",
	"Lock&Read, Write&Unlock are not supported"
};
static const true_false_string tfs_smb_flags_receive_buffer = {
	"Receive buffer has been posted",
	"Receive buffer has not been posted"
};
static const true_false_string tfs_smb_flags_caseless = {
	"Path names are caseless",
	"Path names are case sensitive"
};
static const true_false_string tfs_smb_flags_canon = {
	"Pathnames are canonicalized",
	"Pathnames are not canonicalized"
};
static const true_false_string tfs_smb_flags_oplock = {
	"OpLock requested/granted",
	"OpLock not requested/granted"
};
static const true_false_string tfs_smb_flags_notify = {
	"Notify client on all modifications",
	"Notify client only on open"
};
static const true_false_string tfs_smb_flags_response = {
	"Message is a response to the client/redirector",
	"Message is a request to the server"
};

static int
dissect_smb_flags(tvbuff_t *tvb, proto_tree *parent_tree, int offset)
{
	static const int * flags[] = {
		&hf_smb_flags_response,
		&hf_smb_flags_notify,
		&hf_smb_flags_oplock,
		&hf_smb_flags_canon,
		&hf_smb_flags_caseless,
		&hf_smb_flags_receive_buffer,
		&hf_smb_flags_lock,
		NULL
	};

	proto_tree_add_bitmask(parent_tree, tvb, offset, hf_smb_flags, ett_smb_flags, flags, ENC_NA);
	offset += 1;

	return offset;
}



static const true_false_string tfs_smb_flags2_long_names_allowed = {
	"Long file names are allowed in the response",
	"Long file names are not allowed in the response"
};
static const true_false_string tfs_smb_flags2_ea = {
	"Extended attributes are supported",
	"Extended attributes are not supported"
};
static const true_false_string tfs_smb_flags2_sec_sig = {
	"Security signatures are supported",
	"Security signatures are not supported"
};
static const true_false_string tfs_smb_flags2_compressed = {
	"Compression is requested",
	"Compression is not requested"
};
static const true_false_string tfs_smb_flags2_sec_sig_required = {
	"Security signatures are required",
	"Security signatures are not required"
};
static const true_false_string tfs_smb_flags2_long_names_used = {
	"Path names in request are long file names",
	"Path names in request are not long file names"
};
static const true_false_string tfs_smb_flags2_reparse_path = {
	"The request uses a @GMT reparse path",
	"The request does not use a @GMT reparse path"
};
static const true_false_string tfs_smb_flags2_esn = {
	"Extended security negotiation is supported",
	"Extended security negotiation is not supported"
};
static const true_false_string tfs_smb_flags2_dfs = {
	"Resolve pathnames with Dfs",
	"Don't resolve pathnames with Dfs"
};
static const true_false_string tfs_smb_flags2_roe = {
	"Permit reads if execute-only",
	"Don't permit reads if execute-only"
};
static const true_false_string tfs_smb_flags2_nt_error = {
	"Error codes are NT error codes",
	"Error codes are DOS error codes"
};
static const true_false_string tfs_smb_flags2_string = {
	"Strings are Unicode",
	"Strings are ASCII"
};
static int
dissect_smb_flags2(tvbuff_t *tvb, proto_tree *parent_tree, int offset)
{
	static const int * flags[] = {
		&hf_smb_flags2_string,
		&hf_smb_flags2_nt_error,
		&hf_smb_flags2_roe,
		&hf_smb_flags2_dfs,
		&hf_smb_flags2_esn,
		&hf_smb_flags2_reparse_path,
		&hf_smb_flags2_long_names_used,
		&hf_smb_flags2_sec_sig_required,
		&hf_smb_flags2_compressed,
		&hf_smb_flags2_sec_sig,
		&hf_smb_flags2_ea,
		&hf_smb_flags2_long_names_allowed,
		NULL
	};

	proto_tree_add_bitmask(parent_tree, tvb, offset, hf_smb_flags2, ett_smb_flags2, flags, ENC_LITTLE_ENDIAN);
	offset += 2;

	return offset;
}



#define SMB_FLAGS_DIRN 0x80


static int
dissect_smb(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree, void* data _U_)
{
	int                   offset   = 0;
	proto_item           *item;
	proto_tree           *tree, *htree;
	proto_item           *tmp_item = NULL;
	guint8                flags;
	guint16               flags2;
	smb_info_t 	     *si;
	smb_saved_info_t     *sip      = NULL;
	smb_saved_info_key_t  key;
	smb_saved_info_key_t *new_key;
	guint8                errclass = 0;
	guint16               errcode  = 0;
	guint32               pid_mid;
	conversation_t       *conversation;
	nstime_t              t, deltat;

	si = wmem_new0(wmem_packet_scope(), smb_info_t);

	top_tree_global = parent_tree;

	col_set_str(pinfo->cinfo, COL_PROTOCOL, "SMB");
	col_clear(pinfo->cinfo, COL_INFO);

	/* start off using the local variable, we will allocate a new one if we
	   need to*/
	si->cmd = tvb_get_guint8(tvb, offset+4);
	flags = tvb_get_guint8(tvb, offset+9);
	/*
	 * XXX - in some SMB-over-OSI-transport and SMB-over-Vines traffic,
	 * the direction flag appears never to be set, even for what appear
	 * to be replies.  Do some SMB servers fail to set that flag,
	 * under the assumption that the client knows it's a reply because
	 * it received it?
	 */
	si->request = !(flags&SMB_FLAGS_DIRN);
	flags2 = tvb_get_letohs(tvb, offset+10);
	if (flags2 & 0x8000) {
		si->unicode = TRUE; /* Mark them as Unicode */
	} else {
		si->unicode = FALSE;
	}
	si->tid = tvb_get_letohs(tvb, offset+24);
	si->pid = tvb_get_letohs(tvb, offset+26);
	si->uid = tvb_get_letohs(tvb, offset+28);
	si->mid = tvb_get_letohs(tvb, offset+30);
	pid_mid = (si->pid << 16) | si->mid;
	si->info_level = -1;
	si->info_count = -1;

	item = proto_tree_add_item(parent_tree, proto_smb, tvb, offset,
			-1, ENC_NA);
	tree = proto_item_add_subtree(item, ett_smb);

	htree = proto_tree_add_subtree(tree, tvb, offset, 32,
			ett_smb_hdr, NULL, "SMB Header");


	proto_tree_add_uint_format_value(htree, hf_smb_server_component, tvb, offset, 4, tvb_get_letohl(tvb, offset), "SMB");
	offset += 4;  /* Skip the marker */

	/* find which conversation we are part of and get the tables for that
	   conversation*/
	conversation = find_or_create_conversation(pinfo);
	/* see if we already have the smb data for this conversation */
	si->ct = (conv_tables_t *)conversation_get_proto_data(conversation, proto_smb);
	if (!si->ct) {
		/* No, not yet. create it and attach it to the conversation */
		si->ct = (conv_tables_t *)g_malloc(sizeof(conv_tables_t));

		conv_tables = g_slist_prepend(conv_tables, si->ct);
		si->ct->matched = g_hash_table_new(smb_saved_info_hash_matched,
			smb_saved_info_equal_matched);
		si->ct->unmatched = g_hash_table_new(smb_saved_info_hash_unmatched,
			smb_saved_info_equal_unmatched);
		/* We used the same key format as the unmatched entries */
		si->ct->primaries = g_hash_table_new(
			smb_saved_info_hash_unmatched,
			smb_saved_info_equal_unmatched);
		si->ct->tid_service = g_hash_table_new(
			smb_saved_info_hash_unmatched,
			smb_saved_info_equal_unmatched);
		si->ct->raw_ntlmssp = 0;

		si->ct->fid_tree = wmem_tree_new(wmem_file_scope());
		si->ct->tid_tree = wmem_tree_new(wmem_file_scope());
		si->ct->uid_tree = wmem_tree_new(wmem_file_scope());
		/* Initialize the GSL_fid_info for this ct */
		si->ct->GSL_fid_info = NULL;
		conversation_add_proto_data(conversation, proto_smb, si->ct);
	}

	if ( (si->request)
	    &&  (si->mid == 0)
	    &&  (si->uid == 0)
	    &&  (si->pid == 0)
	    &&  (si->tid == 0) ) {
		/* this is a broadcast SMB packet, there will not be a reply.
		   We don't need to do anything
		*/
		si->unidir = TRUE;
	} else if ( (si->cmd == SMB_COM_NT_CANCEL)                  /* NT Cancel */
		   || (si->cmd == SMB_COM_TRANSACTION_SECONDARY)    /* Transaction Secondary */
		   || (si->cmd == SMB_COM_TRANSACTION2_SECONDARY)   /* Transaction2 Secondary */
		   || (si->cmd == SMB_COM_NT_TRANSACT_SECONDARY)) { /* NT Transaction Secondary */
		/* Ok, we got a special request type. This request is either
		   an NT Cancel or a continuation relative to a real request
		   in an earlier packet.  In either case, we don't expect any
		   responses to this packet.  For continuations, any later
		   responses we see really just belong to the original request.
		   Anyway, we want to remember this packet somehow and
		   remember which original request it is associated with so
		   we can say nice things such as "This is a Cancellation to
		   the request in frame x", but we don't want the
		   request/response matching to get messed up.

		   The only thing we do in this case is trying to find which original
		   request we match with and insert an entry for this "special"
		   request for later reference. We continue to reference the original
		   requests smb_saved_info_t but we don't touch it or change anything
		   in it.
		*/

		si->unidir = TRUE;  /*we don't expect an answer to this one*/

		if (!pinfo->fd->flags.visited) {
			/* try to find which original call we match and if we
			   find it add us to the matched table. Don't touch
			   anything else since we don't want this one to mess
			   up the request/response matching. We still consider
			   the initial call the real request and this is only
			   some sort of continuation.
			*/
			/* we only check the unmatched table and assume that the
			   last seen MID matching ours is the right one.
			   This can fail but is better than nothing
			*/
			sip = (smb_saved_info_t *)g_hash_table_lookup(si->ct->unmatched, GUINT_TO_POINTER(pid_mid));
			if (sip != NULL) {
				new_key = (smb_saved_info_key_t *)wmem_alloc(wmem_file_scope(), sizeof(smb_saved_info_key_t));
				new_key->frame = pinfo->num;
				new_key->pid_mid = pid_mid;
				g_hash_table_insert(si->ct->matched, new_key,
				    sip);
			} else {
				if ((si->cmd == SMB_COM_TRANSACTION_SECONDARY)  ||
				    (si->cmd == SMB_COM_TRANSACTION2_SECONDARY) ||
				    (si->cmd == SMB_COM_NT_TRANSACT_SECONDARY)) {
					sip = (smb_saved_info_t *)g_hash_table_lookup(si->ct->primaries, GUINT_TO_POINTER(pid_mid));
				}
			}
		} else {
			/* we have seen this packet before; check the
			   matching table
			*/
			key.frame = pinfo->num;
			key.pid_mid = pid_mid;
			sip = (smb_saved_info_t *)g_hash_table_lookup(si->ct->matched, &key);
			if (sip == NULL) {
			/*
			  We didn't find it.
			  Too bad, unfortunately there is not really much we can
			  do now since this means that we never saw the initial
			  request.
			 */
			}
		}


		if (sip && sip->frame_req) {
			switch(si->cmd) {
			case SMB_COM_NT_CANCEL:
				tmp_item = proto_tree_add_uint(htree, hf_smb_cancel_to,
						    tvb, 0, 0, sip->frame_req);
				PROTO_ITEM_SET_GENERATED(tmp_item);
				break;
			case SMB_COM_TRANSACTION_SECONDARY:
			case SMB_COM_TRANSACTION2_SECONDARY:
			case SMB_COM_NT_TRANSACT_SECONDARY:
				tmp_item = proto_tree_add_uint(htree, hf_smb_continuation_to,
						    tvb, 0, 0, sip->frame_req);
				PROTO_ITEM_SET_GENERATED(tmp_item);
				break;
			}
		} else {
			switch(si->cmd) {
			case SMB_COM_NT_CANCEL:
				proto_tree_add_uint_format_value(htree, hf_smb_cancel_to, tvb, 0, 0, 0, "<unknown frame>");
				break;
			case SMB_COM_TRANSACTION_SECONDARY:
			case SMB_COM_TRANSACTION2_SECONDARY:
			case SMB_COM_NT_TRANSACT_SECONDARY:
				proto_tree_add_uint_format_value(htree, hf_smb_continuation_to, tvb, 0, 0, 0, "<unknown frame>");
				break;
			}
		}
	} else { /* normal bidirectional request or response */
		si->unidir = FALSE;

		if (!pinfo->fd->flags.visited) {
			/* first see if we find an unmatched smb "equal" to
			   the current one
			*/
			sip = (smb_saved_info_t *)g_hash_table_lookup(si->ct->unmatched, GUINT_TO_POINTER(pid_mid));
			if (sip != NULL) {
				gboolean cmd_match = FALSE;

				/*
				 * Make sure the SMB we found was the
				 * same command, or a different command
				 * that's another valid type of reply
				 * to that command.
				 */
				if (si->cmd == sip->cmd) {
					cmd_match = TRUE;
				}
				else if (si->cmd == SMB_COM_NT_CANCEL) {
					cmd_match = TRUE;
				}
				else if ((si->cmd == SMB_COM_TRANSACTION_SECONDARY)
				     && (sip->cmd == SMB_COM_TRANSACTION)) {
					cmd_match = TRUE;
				}
				else if ((si->cmd == SMB_COM_TRANSACTION2_SECONDARY)
				     && (sip->cmd == SMB_COM_TRANSACTION2)) {
					cmd_match = TRUE;
				}
				else if ((si->cmd == SMB_COM_NT_TRANSACT_SECONDARY)
				     && (sip->cmd == SMB_COM_NT_TRANSACT)) {
					cmd_match = TRUE;
				}

				if ( (si->request) || (!cmd_match) ) {
					/* We are processing an SMB request but there was already
					   another "identical" smb request we had not matched yet.
					   This must mean that either we have a retransmission or that the
					   response to the previous one was lost and the client has reused
					   the MID for this conversation. In either case it's not much more
					   we can do than forget the old request and concentrate on the
					   present one instead.

					   We also do this cleanup if we see that the cmd in the original
					   request in sip->cmd is not compatible with the current cmd.
					   This is to prevent matching errors such as if there were two
					   SMBs of different cmds but with identical MID and PID values and
					   if wireshark lost the first reply and the second request.
					*/
					g_hash_table_remove(si->ct->unmatched, GUINT_TO_POINTER(pid_mid));
					sip = NULL; /* XXX should free it as well */
				} else {
					/* we have found a response to some
					   request we have seen earlier.
					   What we do now depends on whether
					   this is the first response to that
					   request we see (id frame_res == 0) or
					   if it's a response to a request
					   for which we've seen an earlier
					   response that's continued.
					*/
					if ((sip->frame_res == 0) ||
					    (sip->flags & SMB_SIF_IS_CONTINUED)) {
						/* OK, it is the first response
						   we have seen to this packet,
						   or it's a continuation of
						   a response we've seen. */
						sip->frame_res = pinfo->num;
						new_key = (smb_saved_info_key_t *)wmem_alloc(wmem_file_scope(), sizeof(smb_saved_info_key_t));
						new_key->frame = sip->frame_res;
						new_key->pid_mid = pid_mid;
						g_hash_table_insert(si->ct->matched, new_key, sip);
						/* We remove the entry for unmatched since we have found a match.
						 * We have to do this since the MID value wraps so quickly (effective only 10 bits)
						 * and if there is packetloss in the trace (maybe due to large holes
						 * created by a sniffer device not being able to keep up
						 * with the line rate.
						 * There is a real possibility that the following would occur which is painful :
						 * 1, -> Request  MID:5
						 * 2, <- Response MID:5
						 * 3, -> Request  MID:5 (missing from capture)
						 * 4, <- Response MID:5
						 * We DON'T want #4 to be presented as a response to #1
						 */
						g_hash_table_remove(si->ct->unmatched, GUINT_TO_POINTER(pid_mid));
					} else {
						/* We have already seen another response to this MID.
						   Since the MID in reality is only something like 10 bits
						   this probably means that we just have a MID that is being
						   reused due to the small MID space and that this is a new
						   command we did not see the original request for.
						*/
						sip = NULL;
					}
				}
			} else {
				if ((si->cmd == SMB_COM_TRANSACTION)  ||
				    (si->cmd == SMB_COM_TRANSACTION2) ||
				    (si->cmd == SMB_COM_NT_TRANSACT)) {
					sip = (smb_saved_info_t *)g_hash_table_lookup(si->ct->primaries, GUINT_TO_POINTER(pid_mid));
				}
			}
			if (si->request) {
				sip = (smb_saved_info_t *)wmem_alloc(wmem_file_scope(), sizeof(smb_saved_info_t));
				sip->frame_req = pinfo->num;
				sip->frame_res = 0;
				sip->req_time = pinfo->abs_ts;
				sip->flags = 0;
				if (g_hash_table_lookup(si->ct->tid_service, GUINT_TO_POINTER(si->tid))
				    == (void *)TID_IPC) {
					sip->flags |= SMB_SIF_TID_IS_IPC;
				}
				sip->cmd = si->cmd;
				sip->extra_info = NULL;
				sip->extra_info_type = SMB_EI_NONE;
				sip->fid = 0;
				sip->fid_seen_in_request = 0;
				g_hash_table_insert(si->ct->unmatched, GUINT_TO_POINTER(pid_mid), sip);
				new_key = (smb_saved_info_key_t *)wmem_alloc(wmem_file_scope(), sizeof(smb_saved_info_key_t));
				new_key->frame = sip->frame_req;
				new_key->pid_mid = pid_mid;
				g_hash_table_insert(si->ct->matched, new_key, sip);

				/* If it is a TRANSACT cmd, insert in hash */
				if ((si->cmd == SMB_COM_TRANSACTION)  ||
				    (si->cmd == SMB_COM_TRANSACTION2) ||
				    (si->cmd == SMB_COM_NT_TRANSACT)) {
					g_hash_table_insert(si->ct->primaries, GUINT_TO_POINTER(pid_mid), sip);
				}
			}
		} else {
			/* we have seen this packet before; check the
			   matching table.
			   If we haven't yet seen the reply, we won't
			   find the info for it; we don't need it, as
			   we only use it to save information, and, as
			   we've seen this packet before, we've already
			   saved the information.
			*/
			key.frame = pinfo->num;
			key.pid_mid = pid_mid;
			sip = (smb_saved_info_t *)g_hash_table_lookup(si->ct->matched, &key);
		}
	}

	/*
	 * Pass the "sip" on to subdissectors through "si".
	 */
	si->sip = sip;

	if (sip != NULL) {
		/*
		 * Put in fields for the frame number of the frame to which
		 * this is a response or the frame with the response to this
		 * frame - if we know the frame number (i.e., it's not 0).
		 */
		if (si->request) {
			if (sip->frame_res != 0) {
				tmp_item = proto_tree_add_uint(htree, hf_smb_response_in, tvb, 0, 0, sip->frame_res);
				PROTO_ITEM_SET_GENERATED(tmp_item);
			}
		} else {
			if (sip->frame_req != 0) {
				tmp_item = proto_tree_add_uint(htree, hf_smb_response_to, tvb, 0, 0, sip->frame_req);
				PROTO_ITEM_SET_GENERATED(tmp_item);
				t = pinfo->abs_ts;
				nstime_delta(&deltat, &t, &sip->req_time);
				tmp_item = proto_tree_add_time(htree, hf_smb_time, tvb,
				    0, 0, &deltat);
				PROTO_ITEM_SET_GENERATED(tmp_item);
			}
		}
	}

	/* smb command */
	proto_tree_add_uint(htree, hf_smb_cmd, tvb, offset, 1, si->cmd);
	offset += 1;

	if (flags2 & 0x4000) {
		/* handle NT 32 bit error code */

		si->nt_status = tvb_get_letohl(tvb, offset);

		proto_tree_add_item(htree, hf_smb_nt_status, tvb, offset, 4,
			ENC_LITTLE_ENDIAN);
		offset += 4;

	} else {
		/* handle DOS error code & class */
		errclass = tvb_get_guint8(tvb, offset);
		proto_tree_add_uint(htree, hf_smb_error_class, tvb, offset, 1,
			errclass);
		offset += 1;

		/* reserved byte */
		proto_tree_add_item(htree, hf_smb_reserved, tvb, offset, 1, ENC_NA);
		offset += 1;

		/* error code */
		/* XXX - the type of this field depends on the value of
		 * "errcls", so there is isn't a single value_string array
		 * fo it, so there can't be a single field for it.
		 */
		errcode = tvb_get_letohs(tvb, offset);
		proto_tree_add_uint_format_value(htree, hf_smb_error_code, tvb,
			offset, 2, errcode, "%s",
			decode_smb_error(errclass, errcode));
		offset += 2;
	}

	/* flags */
	offset = dissect_smb_flags(tvb, htree, offset);

	/* flags2 */
	offset = dissect_smb_flags2(tvb, htree, offset);

	/*
	 * The document at
	 *
	 *	http://www.samba.org/samba/ftp/specs/smbpub.txt
	 *
	 * (a text version of "Microsoft Networks SMB FILE SHARING
	 * PROTOCOL, Document Version 6.0p") says that:
	 *
	 *	the first 2 bytes of these 12 bytes are, for NT Create and X,
	 *	the "High Part of PID";
	 *
	 *	the next four bytes are reserved;
	 *
	 *	the next four bytes are, for SMB-over-IPX (with no
	 *	NetBIOS involved) two bytes of Session ID and two bytes
	 *	of SequenceNumber.
	 *
	 * Network Monitor 2.x dissects the four bytes before the Session ID
	 * as a "Key", and the two bytes after the SequenceNumber as
	 * a "Group ID".
	 *
	 * The "High Part of PID" has been seen in calls other than NT
	 * Create and X, although most of them appear to be I/O on DCE RPC
	 * pipes opened with the NT Create and X in question.
	 */
	proto_tree_add_item(htree, hf_smb_pid_high, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	if ((pinfo->ptype == PT_IPX) &&
	    ((pinfo->match_uint == IPX_SOCKET_NWLINK_SMB_SERVER) ||
	     (pinfo->match_uint == IPX_SOCKET_NWLINK_SMB_REDIR)  ||
	     (pinfo->match_uint == IPX_SOCKET_NWLINK_SMB_MESSENGER))) {
		/*
		 * This is SMB-over-IPX.
		 * XXX - do we have to worry about "sequenced commands",
		 * as per the Samba document?  They say that for
		 * "unsequenced commands" (with a sequence number of 0),
		 * the Mid must be unique, but perhaps the Mid doesn't
		 * have to be unique for sequenced commands.  In at least
		 * one capture with SMB-over-IPX, however, the Mids
		 * are unique even for sequenced commands.
		 */
		/* Key */
		proto_tree_add_item(htree, hf_smb_key, tvb, offset, 4,
		    ENC_LITTLE_ENDIAN);
		offset += 4;

		/* Session ID */
		proto_tree_add_item(htree, hf_smb_session_id, tvb, offset, 2,
		    ENC_LITTLE_ENDIAN);
		offset += 2;

		/* Sequence number */
		proto_tree_add_item(htree, hf_smb_sequence_num, tvb, offset, 2,
		    ENC_LITTLE_ENDIAN);
		offset += 2;

		/* Group ID */
		proto_tree_add_item(htree, hf_smb_group_id, tvb, offset, 2,
		    ENC_LITTLE_ENDIAN);
		offset += 2;
	} else {
		/*
		 * According to http://ubiqx.org/cifs/SMB.html#SMB.4.2.1
		 * and http://ubiqx.org/cifs/SMB.html#SMB.5.5.1 the 8
		 * bytes after the "High part of PID" are an 8-byte
		 * signature ...
		 */
		proto_tree_add_item(htree, hf_smb_sig, tvb, offset, 8, ENC_NA);
		offset += 8;

		proto_tree_add_item(htree, hf_smb_reserved, tvb, offset, 2, ENC_NA);
		offset += 2;
	}

	/* TID
	 * TreeConnectAndX(0x75) is special, here it is the mere fact of
	 * having a response that means that the share was mapped and we
	 * need to track it
	 */
	if (!pinfo->fd->flags.visited && (si->cmd == 0x75) && !si->request) {
		offset = dissect_smb_tid(tvb, pinfo, htree, offset, (guint16)si->tid, TRUE, FALSE, si);
	} else {
		offset = dissect_smb_tid(tvb, pinfo, htree, offset, (guint16)si->tid, FALSE, FALSE, si);
	}

	/* PID */
	proto_tree_add_uint(htree, hf_smb_pid, tvb, offset, 2, si->pid);
	offset += 2;

	/* UID */
	offset = dissect_smb_uid(tvb, htree, offset, si);

	/* MID */
	proto_tree_add_uint(htree, hf_smb_mid, tvb, offset, 2, si->mid);
	offset += 2;

	/* tap the packet before the dissectors are called so we still get
	   the tap listener called even if there is an exception.
	*/
	tap_queue_packet(smb_tap, pinfo, si);
	dissect_smb_command(tvb, pinfo, offset, tree, si->cmd, TRUE, si);

	/* Append error info from this packet to info string. */
	if (!si->request) {
		if (flags2 & 0x4000) {
			/*
			 * The status is an NT status code; was there
			 * an error?
			 */
			if ((si->nt_status & 0xC0000000) == 0xC0000000) {
				/*
				 * Yes.
				 */
				col_append_fstr(
					pinfo->cinfo, COL_INFO, ", Error: %s",
					val_to_str_ext(si->nt_status, &NT_errors_ext,
					    "Unknown (0x%08X)"));
			}
		} else {
			/*
			 * The status is a DOS error class and code; was
			 * there an error?
			 */
			if (errclass != SMB_SUCCESS) {
				/*
				 * Yes.
				 */
				col_append_fstr(
					pinfo->cinfo, COL_INFO, ", Error: %s",
					decode_smb_error(errclass, errcode));
			}
		}
	}
	return tvb_captured_length(tvb);
}

static gboolean
dissect_smb_heur(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree, void *data _U_)
{
	/* must check that this really is a smb packet */
	if (tvb_reported_length(tvb) < 4)
		return FALSE;

	if ( (tvb_get_guint8(tvb, 0) != 0xff)
	    || (tvb_get_guint8(tvb, 1) != 'S')
	    || (tvb_get_guint8(tvb, 2) != 'M')
	    || (tvb_get_guint8(tvb, 3) != 'B') ) {
		return FALSE;
	}

	dissect_smb(tvb, pinfo, parent_tree, data);
	return TRUE;
}

void
proto_register_smb(void)
{
	static hf_register_info hf[] = {
	{ &hf_smb_cmd,
		{ "SMB Command", "smb.cmd", FT_UINT8, BASE_HEX|BASE_EXT_STRING,
		&smb_cmd_vals_ext, 0x0, NULL, HFILL }},

	{ &hf_smb_andxcmd,
		{ "AndXCommand", "smb.cmd", FT_UINT8, BASE_HEX|BASE_EXT_STRING,
		&smb_cmd_vals_ext, 0x0, NULL, HFILL }},

	{ &hf_smb_trans2_subcmd,
		{ "Subcommand", "smb.trans2.cmd", FT_UINT16, BASE_HEX|BASE_EXT_STRING,
		&trans2_cmd_vals_ext, 0, "Subcommand for TRANSACTION2", HFILL }},

	{ &hf_smb_nt_trans_subcmd,
		{ "Function", "smb.nt.function", FT_UINT16, BASE_DEC|BASE_EXT_STRING,
		&nt_cmd_vals_ext, 0, "Function for NT Transaction", HFILL }},

	{ &hf_smb_word_count,
		{ "Word Count (WCT)", "smb.wct", FT_UINT8, BASE_DEC,
		NULL, 0x0, "Word Count, count of parameter words", HFILL }},

	{ &hf_smb_byte_count,
		{ "Byte Count (BCC)", "smb.bcc", FT_UINT16, BASE_DEC,
		NULL, 0x0, "Byte Count, count of data bytes", HFILL }},

	{ &hf_smb_response_to,
		{ "Response to", "smb.response_to", FT_FRAMENUM, BASE_NONE,
		NULL, 0, "This packet is a response to the packet in this frame", HFILL }},

	{ &hf_smb_time,
		{ "Time from request", "smb.time", FT_RELATIVE_TIME, BASE_NONE,
		NULL, 0, "Time between Request and Response for SMB cmds", HFILL }},

	{ &hf_smb_response_in,
		{ "Response in", "smb.response_in", FT_FRAMENUM, BASE_NONE,
		NULL, 0, "The response to this packet is in this packet", HFILL }},

	{ &hf_smb_continuation_to,
		{ "Continuation to", "smb.continuation_to", FT_FRAMENUM, BASE_NONE,
		NULL, 0, "This packet is a continuation to the packet in this frame", HFILL }},

	{ &hf_smb_nt_status,
		{ "NT Status", "smb.nt_status", FT_UINT32, BASE_HEX | BASE_EXT_STRING,
		&NT_errors_ext, 0, "NT Status code", HFILL }},

	{ &hf_smb_error_class,
		{ "Error Class", "smb.error_class", FT_UINT8, BASE_HEX,
		VALS(errcls_types), 0, "DOS Error Class", HFILL }},

	{ &hf_smb_error_code,
		{ "Error Code", "smb.error_code", FT_UINT16, BASE_HEX,
		NULL, 0, "DOS Error Code", HFILL }},

	{ &hf_smb_reserved,
		{ "Reserved", "smb.reserved", FT_BYTES, BASE_NONE,
		NULL, 0, "Reserved bytes, must be zero", HFILL }},

	{ &hf_smb_sig,
		{ "Signature", "smb.signature", FT_BYTES, BASE_NONE,
		NULL, 0, "Signature bytes", HFILL }},

	{ &hf_smb_key,
		{ "Key", "smb.key", FT_UINT32, BASE_HEX,
		NULL, 0, "SMB-over-IPX Key", HFILL }},

	{ &hf_smb_session_id,
		{ "Session ID", "smb.sessid", FT_UINT16, BASE_DEC,
		NULL, 0, "SMB-over-IPX Session ID", HFILL }},

	{ &hf_smb_sequence_num,
		{ "Sequence Number", "smb.sequence_num", FT_UINT16, BASE_DEC,
		NULL, 0, "SMB-over-IPX Sequence Number", HFILL }},

	{ &hf_smb_group_id,
		{ "Group ID", "smb.group_id", FT_UINT16, BASE_DEC,
		NULL, 0, "SMB-over-IPX Group ID", HFILL }},

	{ &hf_smb_pid,
		{ "Process ID", "smb.pid", FT_UINT16, BASE_DEC,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_pid_high,
		{ "Process ID High", "smb.pid.high", FT_UINT16, BASE_DEC,
		NULL, 0, "Process ID High Bytes", HFILL }},

	{ &hf_smb_tid,
		{ "Tree ID", "smb.tid", FT_UINT16, BASE_DEC,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_uid,
		{ "User ID", "smb.uid", FT_UINT16, BASE_DEC,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_mid,
		{ "Multiplex ID", "smb.mid", FT_UINT16, BASE_DEC,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_flags,
		{ "Flags", "smb.flags", FT_UINT8, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_flags_lock,
		{ "Lock and Read", "smb.flags.lock", FT_BOOLEAN, 8,
		TFS(&tfs_smb_flags_lock), 0x01, "Are Lock&Read and Write&Unlock operations supported?", HFILL }},

	{ &hf_smb_flags_receive_buffer,
		{ "Receive Buffer Posted", "smb.flags.receive_buffer", FT_BOOLEAN, 8,
		TFS(&tfs_smb_flags_receive_buffer), 0x02, "Have receive buffers been reported?", HFILL }},

	{ &hf_smb_flags_caseless,
		{ "Case Sensitivity", "smb.flags.caseless", FT_BOOLEAN, 8,
		TFS(&tfs_smb_flags_caseless), 0x08, "Are pathnames caseless or casesensitive?", HFILL }},

	{ &hf_smb_flags_canon,
		{ "Canonicalized Pathnames", "smb.flags.canon", FT_BOOLEAN, 8,
		TFS(&tfs_smb_flags_canon), 0x10, "Are pathnames canonicalized?", HFILL }},

	{ &hf_smb_flags_oplock,
		{ "Oplocks", "smb.flags.oplock", FT_BOOLEAN, 8,
		TFS(&tfs_smb_flags_oplock), 0x20, "Is an oplock requested/granted?", HFILL }},

	{ &hf_smb_flags_notify,
		{ "Notify", "smb.flags.notify", FT_BOOLEAN, 8,
		TFS(&tfs_smb_flags_notify), 0x40, "Notify on open or all?", HFILL }},

	{ &hf_smb_flags_response,
		{ "Request/Response", "smb.flags.response", FT_BOOLEAN, 8,
		TFS(&tfs_smb_flags_response), 0x80, "Is this a request or a response?", HFILL }},

	{ &hf_smb_flags2,
		{ "Flags2", "smb.flags2", FT_UINT16, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_flags2_long_names_allowed,
		{ "Long Names Allowed", "smb.flags2.long_names_allowed", FT_BOOLEAN, 16,
		TFS(&tfs_smb_flags2_long_names_allowed), 0x0001, "Are long file names allowed in the response?", HFILL }},

	{ &hf_smb_flags2_ea,
		{ "Extended Attributes", "smb.flags2.ea", FT_BOOLEAN, 16,
		TFS(&tfs_smb_flags2_ea), 0x0002, "Are extended attributes supported?", HFILL }},

	{ &hf_smb_flags2_sec_sig,
		{ "Security Signatures", "smb.flags2.sec_sig", FT_BOOLEAN, 16,
		TFS(&tfs_smb_flags2_sec_sig), 0x0004, "Are security signatures supported?", HFILL }},

	{ &hf_smb_flags2_compressed,
		{ "Compressed", "smb.flags2.compressed", FT_BOOLEAN, 16,
		TFS(&tfs_smb_flags2_compressed), 0x0008, "Is compression requested?", HFILL }},

	{ &hf_smb_flags2_sec_sig_required,
		{ "Security Signatures Required", "smb.flags2.sec_sig_required", FT_BOOLEAN, 16,
		TFS(&tfs_smb_flags2_sec_sig_required), 0x0010, "Are security signatures required?", HFILL }},

	{ &hf_smb_flags2_long_names_used,
		{ "Long Names Used", "smb.flags2.long_names_used", FT_BOOLEAN, 16,
		TFS(&tfs_smb_flags2_long_names_used), 0x0040, "Are pathnames in this request long file names?", HFILL }},

	{ &hf_smb_flags2_reparse_path,
		{ "Reparse Path", "smb.flags2.reparse_path", FT_BOOLEAN, 16,
		TFS(&tfs_smb_flags2_reparse_path), 0x0400, "The request uses a @GMT reparse path", HFILL }},

	{ &hf_smb_flags2_esn,
		{ "Extended Security Negotiation", "smb.flags2.esn", FT_BOOLEAN, 16,
		TFS(&tfs_smb_flags2_esn), 0x0800, "Is extended security negotiation supported?", HFILL }},

	{ &hf_smb_flags2_dfs,
		{ "Dfs", "smb.flags2.dfs", FT_BOOLEAN, 16,
		TFS(&tfs_smb_flags2_dfs), 0x1000, "Can pathnames be resolved using Dfs?", HFILL }},

	{ &hf_smb_flags2_roe,
		{ "Execute-only Reads", "smb.flags2.roe", FT_BOOLEAN, 16,
		TFS(&tfs_smb_flags2_roe), 0x2000, "Will reads be allowed for execute-only files?", HFILL }},

	{ &hf_smb_flags2_nt_error,
		{ "Error Code Type", "smb.flags2.nt_error", FT_BOOLEAN, 16,
		TFS(&tfs_smb_flags2_nt_error), 0x4000, "Are error codes NT or DOS format?", HFILL }},

	{ &hf_smb_flags2_string,
		{ "Unicode Strings", "smb.flags2.string", FT_BOOLEAN, 16,
		TFS(&tfs_smb_flags2_string), 0x8000, "Are strings ASCII or Unicode?", HFILL }},

	{ &hf_smb_buffer_format,
		{ "Buffer Format", "smb.buffer_format", FT_UINT8, BASE_DEC,
		VALS(buffer_format_vals), 0x0, "Buffer Format, type of buffer", HFILL }},

	{ &hf_smb_dialect,
		{ "Dialect", "smb.dialect", FT_STRING, BASE_NONE,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_dialect_name,
		{ "Name", "smb.dialect.name", FT_STRING, BASE_NONE,
		NULL, 0, "Name of dialect", HFILL }},

	{ &hf_smb_dialect_index,
		{ "Selected Index", "smb.dialect.index", FT_UINT16, BASE_DEC,
		NULL, 0, "Index of selected dialect", HFILL }},

	{ &hf_smb_max_trans_buf_size,
		{ "Max Buffer Size", "smb.max_bufsize", FT_UINT32, BASE_DEC,
		NULL, 0, "Maximum transmit buffer size", HFILL }},

	{ &hf_smb_max_mpx_count,
		{ "Max Mpx Count", "smb.max_mpx_count", FT_UINT16, BASE_DEC,
		NULL, 0, "Maximum pending multiplexed requests", HFILL }},

	{ &hf_smb_max_vcs_num,
		{ "Max VCs", "smb.max_vcs", FT_UINT16, BASE_DEC,
		NULL, 0, "Maximum VCs between client and server", HFILL }},

	{ &hf_smb_session_key,
		{ "Session Key", "smb.session_key", FT_UINT32, BASE_HEX,
		NULL, 0, "Unique token identifying this session", HFILL }},

	{ &hf_smb_server_timezone,
		{ "Server Time Zone", "smb.server_timezone", FT_INT16, BASE_DEC,
		NULL, 0, "Current timezone at server.", HFILL }},

	{ &hf_smb_challenge_length,
		{ "Challenge Length", "smb.challenge_length", FT_UINT16, BASE_DEC,
		NULL, 0, "Challenge_length (must be 0 if not LM2.1 dialect)", HFILL }},

	{ &hf_smb_challenge,
		{ "Challenge", "smb.challenge", FT_BYTES, BASE_NONE,
		NULL, 0, "Challenge Data (for LM2.1 dialect)", HFILL }},

	{ &hf_smb_primary_domain,
		{ "Primary Domain", "smb.primary_domain", FT_STRING, BASE_NONE,
		NULL, 0, "The server's primary domain", HFILL }},

	{ &hf_smb_server,
		{ "Server", "smb.server", FT_STRING, BASE_NONE,
		NULL, 0, "The name of the DC/server", HFILL }},

	{ &hf_smb_max_raw_buf_size,
		{ "Max Raw Buffer", "smb.max_raw", FT_UINT32, BASE_DEC,
		NULL, 0, "Maximum raw buffer size", HFILL }},

	{ &hf_smb_server_guid,
		{ "Server GUID", "smb.server_guid", FT_GUID, BASE_NONE,
		NULL, 0, "Globally unique identifier for this server", HFILL }},

	{ &hf_smb_volume_guid,
		{ "Volume GUID", "smb.volume_guid", FT_GUID, BASE_NONE,
		NULL, 0, "Globally unique identifer for this volume", HFILL }},

	{ &hf_smb_security_blob_len,
		{ "Security Blob Length", "smb.security_blob_len", FT_UINT16, BASE_DEC,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_security_blob,
		{ "Security Blob", "smb.security_blob", FT_BYTES, BASE_NONE,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_sm16,
		{ "Security Mode", "smb.sm", FT_UINT16, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_sm_mode16,
		{ "Mode", "smb.sm.mode", FT_BOOLEAN, 16,
		TFS(&tfs_sm_mode), SECURITY_MODE_MODE, "User or Share security mode?", HFILL }},

	{ &hf_smb_sm_password16,
		{ "Password", "smb.sm.password", FT_BOOLEAN, 16,
		TFS(&tfs_sm_password), SECURITY_MODE_PASSWORD, "Encrypted or plaintext passwords?", HFILL }},

	{ &hf_smb_sm,
		{ "Security Mode", "smb.sm", FT_UINT8, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_sm_mode,
		{ "Mode", "smb.sm.mode", FT_BOOLEAN, 8,
		TFS(&tfs_sm_mode), SECURITY_MODE_MODE, "User or Share security mode?", HFILL }},

	{ &hf_smb_sm_password,
		{ "Password", "smb.sm.password", FT_BOOLEAN, 8,
		TFS(&tfs_sm_password), SECURITY_MODE_PASSWORD, "Encrypted or plaintext passwords?", HFILL }},

	{ &hf_smb_sm_signatures,
		{ "Signatures", "smb.sm.signatures", FT_BOOLEAN, 8,
		TFS(&tfs_sm_signatures), SECURITY_MODE_SIGNATURES, "Are security signatures enabled?", HFILL }},

	{ &hf_smb_sm_sig_required,
		{ "Sig Req", "smb.sm.sig_required", FT_BOOLEAN, 8,
		TFS(&tfs_sm_sig_required), SECURITY_MODE_SIG_REQUIRED, "Are security signatures required?", HFILL }},

	{ &hf_smb_rm,
		{ "Raw Mode", "smb.rm", FT_UINT16, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_rm_read,
		{ "Read Raw", "smb.rm.read", FT_BOOLEAN, 16,
		TFS(&tfs_rm_read), RAWMODE_READ, "Is Read Raw supported?", HFILL }},

	{ &hf_smb_rm_write,
		{ "Write Raw", "smb.rm.write", FT_BOOLEAN, 16,
		TFS(&tfs_rm_write), RAWMODE_WRITE, "Is Write Raw supported?", HFILL }},

	{ &hf_smb_server_date_time,
		{ "Server Date and Time", "smb.server_date_time", FT_ABSOLUTE_TIME, ABSOLUTE_TIME_LOCAL,
		NULL, 0, "Current date and time at server", HFILL }},

	{ &hf_smb_server_smb_date,
		{ "Server Date", "smb.server_date_time.smb_date", FT_UINT16, BASE_HEX,
		NULL, 0, "Current date at server, SMB_DATE format", HFILL }},

	{ &hf_smb_server_smb_time,
		{ "Server Time", "smb.server_date_time.smb_time", FT_UINT16, BASE_HEX,
		NULL, 0, "Current time at server, SMB_TIME format", HFILL }},

	{ &hf_smb_server_cap,
		{ "Capabilities", "smb.server_cap", FT_UINT32, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_server_cap_raw_mode,
		{ "Raw Mode", "smb.server_cap.raw_mode", FT_BOOLEAN, 32,
		TFS(&tfs_server_cap_raw_mode), SERVER_CAP_RAW_MODE, "Are Raw Read and Raw Write supported?", HFILL }},

	{ &hf_smb_server_cap_mpx_mode,
		{ "MPX Mode", "smb.server_cap.mpx_mode", FT_BOOLEAN, 32,
		TFS(&tfs_server_cap_mpx_mode), SERVER_CAP_MPX_MODE, "Are Read Mpx and Write Mpx supported?", HFILL }},

	{ &hf_smb_server_cap_unicode,
		{ "Unicode", "smb.server_cap.unicode", FT_BOOLEAN, 32,
		TFS(&tfs_server_cap_unicode), SERVER_CAP_UNICODE, "Are Unicode strings supported?", HFILL }},

	{ &hf_smb_server_cap_large_files,
		{ "Large Files", "smb.server_cap.large_files", FT_BOOLEAN, 32,
		TFS(&tfs_server_cap_large_files), SERVER_CAP_LARGE_FILES, "Are large files (>4GB) supported?", HFILL }},

	{ &hf_smb_server_cap_nt_smbs,
		{ "NT SMBs", "smb.server_cap.nt_smbs", FT_BOOLEAN, 32,
		TFS(&tfs_server_cap_nt_smbs), SERVER_CAP_NT_SMBS, "Are NT SMBs supported?", HFILL }},

	{ &hf_smb_server_cap_rpc_remote_apis,
		{ "RPC Remote APIs", "smb.server_cap.rpc_remote_apis", FT_BOOLEAN, 32,
		TFS(&tfs_server_cap_rpc_remote_apis), SERVER_CAP_RPC_REMOTE_APIS, "Are RPC Remote APIs supported?", HFILL }},

	{ &hf_smb_server_cap_nt_status,
		{ "NT Status Codes", "smb.server_cap.nt_status", FT_BOOLEAN, 32,
		TFS(&tfs_server_cap_nt_status), SERVER_CAP_STATUS32, "Are NT Status Codes supported?", HFILL }},

	{ &hf_smb_server_cap_level_ii_oplocks,
		{ "Level 2 Oplocks", "smb.server_cap.level_2_oplocks", FT_BOOLEAN, 32,
		TFS(&tfs_server_cap_level_ii_oplocks), SERVER_CAP_LEVEL_II_OPLOCKS, "Are Level 2 oplocks supported?", HFILL }},

	{ &hf_smb_server_cap_lock_and_read,
		{ "Lock and Read", "smb.server_cap.lock_and_read", FT_BOOLEAN, 32,
		TFS(&tfs_server_cap_lock_and_read), SERVER_CAP_LOCK_AND_READ, "Is Lock and Read supported?", HFILL }},

	{ &hf_smb_server_cap_nt_find,
		{ "NT Find", "smb.server_cap.nt_find", FT_BOOLEAN, 32,
		TFS(&tfs_server_cap_nt_find), SERVER_CAP_NT_FIND, "Is NT Find supported?", HFILL }},

	{ &hf_smb_server_cap_dfs,
		{ "Dfs", "smb.server_cap.dfs", FT_BOOLEAN, 32,
		TFS(&tfs_server_cap_dfs), SERVER_CAP_DFS, "Is Dfs supported?", HFILL }},

	{ &hf_smb_server_cap_infolevel_passthru,
		{ "Infolevel Passthru", "smb.server_cap.infolevel_passthru", FT_BOOLEAN, 32,
		TFS(&tfs_server_cap_infolevel_passthru), SERVER_CAP_INFOLEVEL_PASSTHRU, "Is NT information level request passthrough supported?", HFILL }},

	{ &hf_smb_server_cap_large_readx,
		{ "Large ReadX", "smb.server_cap.large_readx", FT_BOOLEAN, 32,
		TFS(&tfs_server_cap_large_readx), SERVER_CAP_LARGE_READX, "Is Large Read andX supported?", HFILL }},

	{ &hf_smb_server_cap_large_writex,
		{ "Large WriteX", "smb.server_cap.large_writex", FT_BOOLEAN, 32,
		TFS(&tfs_server_cap_large_writex), SERVER_CAP_LARGE_WRITEX, "Is Large Write andX supported?", HFILL }},

	{ &hf_smb_server_cap_lwio,
		{ "LWIO", "smb.server_cap.lwio", FT_BOOLEAN, 32,
		TFS(&tfs_server_cap_lwio), SERVER_CAP_LWIO,
		"Is IOCTL/FSCTL supported", HFILL }},

	{ &hf_smb_server_cap_unix,
		{ "UNIX", "smb.server_cap.unix", FT_BOOLEAN, 32,
		TFS(&tfs_server_cap_unix), SERVER_CAP_UNIX , "Are UNIX extensions supported?", HFILL }},

	{ &hf_smb_server_cap_compressed_data,
		{ "Compressed Data", "smb.server_cap.compressed_data", FT_BOOLEAN, 32,
		TFS(&tfs_server_cap_compressed_data), SERVER_CAP_COMPRESSED_DATA, "Is compressed data transfer supported?", HFILL }},

	{ &hf_smb_server_cap_dynamic_reauth,
		{ "Dynamic Reauth", "smb.server_cap.dynamic_reauth", FT_BOOLEAN, 32,
		TFS(&tfs_server_cap_dynamic_reauth), SERVER_CAP_DYNAMIC_REAUTH,
		"Is dynamic reauth supported?", HFILL }},

	{ &hf_smb_server_cap_extended_security,
		{ "Extended Security", "smb.server_cap.extended_security", FT_BOOLEAN, 32,
		TFS(&tfs_server_cap_extended_security), SERVER_CAP_EXTENDED_SECURITY, "Are Extended security exchanges supported?", HFILL }},

	{ &hf_smb_system_time,
		{ "System Time", "smb.system.time", FT_ABSOLUTE_TIME, ABSOLUTE_TIME_LOCAL,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_unknown,
		{ "Unknown Data", "smb.unknown_data", FT_BYTES, BASE_NONE,
		NULL, 0, "Unknown Data. Should be implemented by someone", HFILL }},

	{ &hf_smb_dir_name,
		{ "Directory", "smb.dir_name", FT_STRING, BASE_NONE,
		NULL, 0, "SMB Directory Name", HFILL }},

	{ &hf_smb_echo_count,
		{ "Echo Count", "smb.echo.count", FT_UINT16, BASE_DEC,
		NULL, 0, "Number of times to echo data back", HFILL }},

	{ &hf_smb_echo_data,
		{ "Echo Data", "smb.echo.data", FT_BYTES, BASE_NONE,
		NULL, 0, "Data for SMB Echo Request/Response", HFILL }},

	{ &hf_smb_echo_seq_num,
		{ "Echo Seq Num", "smb.echo.seq_num", FT_UINT16, BASE_DEC,
		NULL, 0, "Sequence number for this echo response", HFILL }},

	{ &hf_smb_max_buf_size,
		{ "Max Buffer", "smb.max_buf", FT_UINT16, BASE_DEC,
		NULL, 0, "Max client buffer size", HFILL }},

	{ &hf_smb_path,
		{ "Path", "smb.path", FT_STRING, BASE_NONE,
		NULL, 0, "Path. Server name and share name", HFILL }},

	{ &hf_smb_service,
		{ "Service", "smb.service", FT_STRING, BASE_NONE,
		NULL, 0, "Service name", HFILL }},

	{ &hf_smb_password,
		{ "Password", "smb.password", FT_BYTES, BASE_NONE,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_ansi_password,
		{ "ANSI Password", "smb.ansi_password", FT_BYTES, BASE_NONE,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_unicode_password,
		{ "Unicode Password", "smb.unicode_password", FT_BYTES, BASE_NONE,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_move_flags,
		{ "Flags", "smb.move.flags", FT_UINT16, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_move_flags_file,
		{ "Must be file", "smb.move.flags.file", FT_BOOLEAN, 16,
		TFS(&tfs_mf_file), 0x0001, "Must target be a file?", HFILL }},

	{ &hf_smb_move_flags_dir,
		{ "Must be directory", "smb.move.flags.dir", FT_BOOLEAN, 16,
		TFS(&tfs_mf_dir), 0x0002, "Must target be a directory?", HFILL }},

	{ &hf_smb_move_flags_verify,
		{ "Verify writes", "smb.move.flags.verify", FT_BOOLEAN, 16,
		TFS(&tfs_mf_verify), 0x0010, "Verify all writes?", HFILL }},

	{ &hf_smb_files_moved,
		{ "Files Moved", "smb.files_moved", FT_UINT16, BASE_DEC,
		NULL, 0, "Number of files moved", HFILL }},

	{ &hf_smb_copy_flags,
		{ "Flags", "smb.copy.flags", FT_UINT16, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_copy_flags_file,
		{ "Must be file", "smb.copy.flags.file", FT_BOOLEAN, 16,
		TFS(&tfs_mf_file), 0x0001, "Must target be a file?", HFILL }},

	{ &hf_smb_copy_flags_dir,
		{ "Must be directory", "smb.copy.flags.dir", FT_BOOLEAN, 16,
		TFS(&tfs_mf_dir), 0x0002, "Must target be a directory?", HFILL }},

	{ &hf_smb_copy_flags_dest_mode,
		{ "Destination mode", "smb.copy.flags.dest_mode", FT_BOOLEAN, 16,
		TFS(&tfs_cf_mode), 0x0004, "Is destination in ASCII?", HFILL }},

	{ &hf_smb_copy_flags_source_mode,
		{ "Source mode", "smb.copy.flags.source_mode", FT_BOOLEAN, 16,
		TFS(&tfs_cf_mode), 0x0008, "Is source in ASCII?", HFILL }},

	{ &hf_smb_copy_flags_verify,
		{ "Verify writes", "smb.copy.flags.verify", FT_BOOLEAN, 16,
		TFS(&tfs_mf_verify), 0x0010, "Verify all writes?", HFILL }},

	{ &hf_smb_copy_flags_tree_copy,
		{ "Tree copy", "smb.copy.flags.tree_copy", FT_BOOLEAN, 16,
		TFS(&tfs_cf_tree_copy), 0x0010, "Is copy a tree copy?", HFILL }},

	{ &hf_smb_copy_flags_ea_action,
		{ "EA action if EAs not supported on dest", "smb.copy.flags.ea_action", FT_BOOLEAN, 16,
		TFS(&tfs_cf_ea_action), 0x0010, "Fail copy if source file has EAs and dest doesn't support EAs?", HFILL }},

	{ &hf_smb_count,
		{ "Count", "smb.count", FT_UINT32, BASE_DEC,
		NULL, 0, "Count number of items/bytes", HFILL }},

	{ &hf_smb_count_low,
		{ "Count Low", "smb.count_low", FT_UINT16, BASE_DEC,
		NULL, 0, "Count number of items/bytes, Low 16 bits", HFILL }},

	{ &hf_smb_count_high,
		{ "Count High (multiply with 64K)", "smb.count_high", FT_UINT16, BASE_DEC,
		NULL, 0, "Count number of items/bytes, High 16 bits", HFILL }},

	{ &hf_smb_file_name,
		{ "File Name", "smb.file", FT_STRING, BASE_NONE,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_open_function,
		{ "Open Function", "smb.open.function", FT_UINT16, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_open_function_create,
		{ "Create", "smb.open.function.create", FT_BOOLEAN, 16,
		TFS(&tfs_of_create), 0x0010, "Create file if it doesn't exist?", HFILL }},

	{ &hf_smb_open_function_open,
		{ "Open", "smb.open.function.open", FT_UINT16, BASE_DEC,
		VALS(of_open), 0x0003, "Action to be taken on open if file exists", HFILL }},

	{ &hf_smb_fid,
		{ "FID", "smb.fid", FT_UINT16, BASE_HEX,
		NULL, 0, "FID: File ID", HFILL }},

	{ &hf_smb_file_attr_16bit,
		{ "File Attributes", "smb.file_attribute", FT_UINT16, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_file_attr_8bit,
		{ "File Attributes", "smb.file_attribute", FT_UINT8, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_file_attr_read_only_16bit,
		{ "Read Only", "smb.file_attribute.read_only", FT_BOOLEAN, 16,
		TFS(&tfs_file_attribute_read_only), SMB_FILE_ATTRIBUTE_READ_ONLY, "READ ONLY file attribute", HFILL }},

	{ &hf_smb_file_attr_read_only_8bit,
		{ "Read Only", "smb.file_attribute.read_only", FT_BOOLEAN, 8,
		TFS(&tfs_file_attribute_read_only), SMB_FILE_ATTRIBUTE_READ_ONLY, "READ ONLY file attribute", HFILL }},

	{ &hf_smb_file_attr_hidden_16bit,
		{ "Hidden", "smb.file_attribute.hidden", FT_BOOLEAN, 16,
		TFS(&tfs_file_attribute_hidden), SMB_FILE_ATTRIBUTE_HIDDEN, "HIDDEN file attribute", HFILL }},

	{ &hf_smb_file_attr_hidden_8bit,
		{ "Hidden", "smb.file_attribute.hidden", FT_BOOLEAN, 8,
		TFS(&tfs_file_attribute_hidden), SMB_FILE_ATTRIBUTE_HIDDEN, "HIDDEN file attribute", HFILL }},

	{ &hf_smb_file_attr_system_16bit,
		{ "System", "smb.file_attribute.system", FT_BOOLEAN, 16,
		TFS(&tfs_file_attribute_system), SMB_FILE_ATTRIBUTE_SYSTEM, "SYSTEM file attribute", HFILL }},

	{ &hf_smb_file_attr_system_8bit,
		{ "System", "smb.file_attribute.system", FT_BOOLEAN, 8,
		TFS(&tfs_file_attribute_system), SMB_FILE_ATTRIBUTE_SYSTEM, "SYSTEM file attribute", HFILL }},

	{ &hf_smb_file_attr_volume_16bit,
		{ "Volume ID", "smb.file_attribute.volume", FT_BOOLEAN, 16,
		TFS(&tfs_file_attribute_volume), SMB_FILE_ATTRIBUTE_VOLUME, "VOLUME file attribute", HFILL }},

	{ &hf_smb_file_attr_volume_8bit,
		{ "Volume ID", "smb.file_attribute.volume", FT_BOOLEAN, 8,
		TFS(&tfs_file_attribute_volume), SMB_FILE_ATTRIBUTE_VOLUME, "VOLUME ID file attribute", HFILL }},

	{ &hf_smb_file_attr_directory_16bit,
		{ "Directory", "smb.file_attribute.directory", FT_BOOLEAN, 16,
		TFS(&tfs_file_attribute_directory), SMB_FILE_ATTRIBUTE_DIRECTORY, "DIRECTORY file attribute", HFILL }},

	{ &hf_smb_file_attr_directory_8bit,
		{ "Directory", "smb.file_attribute.directory", FT_BOOLEAN, 8,
		TFS(&tfs_file_attribute_directory), SMB_FILE_ATTRIBUTE_DIRECTORY, "DIRECTORY file attribute", HFILL }},

	{ &hf_smb_file_attr_archive_16bit,
		{ "Archive", "smb.file_attribute.archive", FT_BOOLEAN, 16,
		TFS(&tfs_file_attribute_archive), SMB_FILE_ATTRIBUTE_ARCHIVE, "ARCHIVE file attribute", HFILL }},

	{ &hf_smb_file_attr_archive_8bit,
		{ "Archive", "smb.file_attribute.archive", FT_BOOLEAN, 8,
		TFS(&tfs_file_attribute_archive), SMB_FILE_ATTRIBUTE_ARCHIVE, "ARCHIVE file attribute", HFILL }},

#if 0
	{ &hf_smb_file_attr_device,
		{ "Device", "smb.file_attribute.device", FT_BOOLEAN, 16,
		TFS(&tfs_file_attribute_device), SMB_FILE_ATTRIBUTE_DEVICE, "Is this file a device?", HFILL }},

	{ &hf_smb_file_attr_normal,
		{ "Normal", "smb.file_attribute.normal", FT_BOOLEAN, 16,
		TFS(&tfs_file_attribute_normal), SMB_FILE_ATTRIBUTE_NORMAL, "Is this a normal file?", HFILL }},

	{ &hf_smb_file_attr_temporary,
		{ "Temporary", "smb.file_attribute.temporary", FT_BOOLEAN, 16,
		TFS(&tfs_file_attribute_temporary), SMB_FILE_ATTRIBUTE_TEMPORARY, "Is this a temporary file?", HFILL }},

	{ &hf_smb_file_attr_sparse,
		{ "Sparse", "smb.file_attribute.sparse", FT_BOOLEAN, 16,
		TFS(&tfs_file_attribute_sparse), SMB_FILE_ATTRIBUTE_SPARSE, "Is this a sparse file?", HFILL }},

	{ &hf_smb_file_attr_reparse,
		{ "Reparse Point", "smb.file_attribute.reparse", FT_BOOLEAN, 16,
		TFS(&tfs_file_attribute_reparse), SMB_FILE_ATTRIBUTE_REPARSE, "Does this file have an associated reparse point?", HFILL }},

	{ &hf_smb_file_attr_compressed,
		{ "Compressed", "smb.file_attribute.compressed", FT_BOOLEAN, 16,
		TFS(&tfs_file_attribute_compressed), SMB_FILE_ATTRIBUTE_COMPRESSED, "Is this file compressed?", HFILL }},

	{ &hf_smb_file_attr_offline,
		{ "Offline", "smb.file_attribute.offline", FT_BOOLEAN, 16,
		TFS(&tfs_file_attribute_offline), SMB_FILE_ATTRIBUTE_OFFLINE, "Is this file offline?", HFILL }},

	{ &hf_smb_file_attr_not_content_indexed,
		{ "Content Indexed", "smb.file_attribute.not_content_indexed", FT_BOOLEAN, 16,
		TFS(&tfs_file_attribute_not_content_indexed), SMB_FILE_ATTRIBUTE_NOT_CONTENT_INDEXED, "May this file be indexed by the content indexing service", HFILL }},

	{ &hf_smb_file_attr_encrypted,
		{ "Encrypted", "smb.file_attribute.encrypted", FT_BOOLEAN, 16,
		TFS(&tfs_file_attribute_encrypted), SMB_FILE_ATTRIBUTE_ENCRYPTED, "Is this file encrypted?", HFILL }},
#endif

	{ &hf_smb_file_size,
		{ "File Size", "smb.file_size", FT_UINT32, BASE_DEC,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_search_attribute,
		{ "Search Attributes", "smb.search.attribute", FT_UINT16, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_search_attribute_read_only,
		{ "Read Only", "smb.search.attribute.read_only", FT_BOOLEAN, 16,
		TFS(&tfs_search_attribute_read_only), SMB_FILE_ATTRIBUTE_READ_ONLY, "READ ONLY search attribute", HFILL }},

	{ &hf_smb_search_attribute_hidden,
		{ "Hidden", "smb.search.attribute.hidden", FT_BOOLEAN, 16,
		TFS(&tfs_search_attribute_hidden), SMB_FILE_ATTRIBUTE_HIDDEN, "HIDDEN search attribute", HFILL }},

	{ &hf_smb_search_attribute_system,
		{ "System", "smb.search.attribute.system", FT_BOOLEAN, 16,
		TFS(&tfs_search_attribute_system), SMB_FILE_ATTRIBUTE_SYSTEM, "SYSTEM search attribute", HFILL }},

	{ &hf_smb_search_attribute_volume,
		{ "Volume ID", "smb.search.attribute.volume", FT_BOOLEAN, 16,
		TFS(&tfs_search_attribute_volume), SMB_FILE_ATTRIBUTE_VOLUME, "VOLUME ID search attribute", HFILL }},

	{ &hf_smb_search_attribute_directory,
		{ "Directory", "smb.search.attribute.directory", FT_BOOLEAN, 16,
		TFS(&tfs_search_attribute_directory), SMB_FILE_ATTRIBUTE_DIRECTORY, "DIRECTORY search attribute", HFILL }},

	{ &hf_smb_search_attribute_archive,
		{ "Archive", "smb.search.attribute.archive", FT_BOOLEAN, 16,
		TFS(&tfs_search_attribute_archive), SMB_FILE_ATTRIBUTE_ARCHIVE, "ARCHIVE search attribute", HFILL }},

	{ &hf_smb_access_mode,
		{ "Access Mode", "smb.access.mode", FT_UINT16, BASE_DEC,
		VALS(da_access_vals), 0x0007, NULL, HFILL }},

	{ &hf_smb_access_sharing,
		{ "Sharing Mode", "smb.access.sharing", FT_UINT16, BASE_DEC,
		VALS(da_sharing_vals), 0x0070, NULL, HFILL }},

	{ &hf_smb_access_locality,
		{ "Locality", "smb.access.locality", FT_UINT16, BASE_DEC,
		VALS(da_locality_vals), 0x0700, "Locality of reference", HFILL }},

	{ &hf_smb_access_caching,
		{ "Caching", "smb.access.caching", FT_BOOLEAN, 16,
		TFS(&tfs_da_caching), 0x1000, "Caching mode?", HFILL }},

	{ &hf_smb_desired_access,
		{ "Desired Access", "smb.access.desired", FT_UINT16, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_granted_access,
		{ "Granted Access", "smb.access.granted", FT_UINT16, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_access_writetru,
		{ "Writethrough", "smb.access.writethrough", FT_BOOLEAN, 16,
		TFS(&tfs_da_writetru), 0x4000, "Writethrough mode?", HFILL }},

	{ &hf_smb_create_time,
		{ "Created", "smb.create.time", FT_ABSOLUTE_TIME, ABSOLUTE_TIME_LOCAL,
		NULL, 0, "Creation Time", HFILL }},

	{ &hf_smb_modify_time,
	        { "Modified", "smb.modify.time", FT_ABSOLUTE_TIME, ABSOLUTE_TIME_LOCAL,
		  NULL, 0, "Modification Time", HFILL }},

	{ &hf_smb_backup_time,
	        { "Backed-up", "smb.backup.time", FT_ABSOLUTE_TIME, ABSOLUTE_TIME_LOCAL,
		  NULL, 0, "Backup time", HFILL}},

	{ &hf_smb_mac_alloc_block_count,
	        { "Allocation Block Count", "smb.alloc.count", FT_UINT32, BASE_DEC,
		  NULL, 0, NULL, HFILL}},

	{ &hf_smb_mac_alloc_block_size,
	        { "Allocation Block Count", "smb.alloc.size", FT_UINT32, BASE_DEC,
		  NULL, 0, "Allocation Block Size", HFILL}},

	{ &hf_smb_mac_free_block_count,
	        { "Free Block Count", "smb.free_block.count", FT_UINT32, BASE_DEC,
		  NULL, 0, NULL, HFILL}},

	{ &hf_smb_mac_root_file_count,
	        { "Root File Count", "smb.root.file.count", FT_UINT32, BASE_DEC,
	        NULL, 0, NULL, HFILL}},

	{ &hf_smb_mac_root_dir_count,
	  { "Root Directory Count", "smb.root.dir.count", FT_UINT32, BASE_DEC,
	    NULL, 0, NULL, HFILL}},

	{ &hf_smb_mac_file_count,
	  { "Root File Count", "smb.file.count", FT_UINT32, BASE_DEC,
	    NULL, 0, "File Count", HFILL}},

	{ &hf_smb_mac_dir_count,
	  { "Root Directory Count", "smb.dir.count", FT_UINT32, BASE_DEC,
	    NULL, 0, "Directory Count", HFILL}},

	{ &hf_smb_mac_sup,
	  { "Mac Support Flags", "smb.mac", FT_UINT32, BASE_HEX,
	    NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_mac_sup_access_ctrl,
	  { "Mac Access Control", "smb.mac.access_control", FT_BOOLEAN, 32,
	    TFS(&tfs_smb_mac_access_ctrl), 0x0010, "Are Mac Access Control Supported", HFILL }},

	{ &hf_smb_mac_sup_getset_comments,
	  { "Get Set Comments", "smb.mac.get_set_comments", FT_BOOLEAN, 32,
	    TFS(&tfs_smb_mac_getset_comments), 0x0020, "Are Mac Get Set Comments supported?", HFILL }},

	{ &hf_smb_mac_sup_desktopdb_calls,
	  { "Desktop DB Calls", "smb.mac.desktop_db_calls", FT_BOOLEAN, 32,
	    TFS(&tfs_smb_mac_desktopdb_calls), 0x0040, "Are Macintosh Desktop DB Calls Supported?", HFILL }},

	{ &hf_smb_mac_sup_unique_ids,
	  { "Macintosh Unique IDs", "smb.mac.uids", FT_BOOLEAN, 32,
	    TFS(&tfs_smb_mac_unique_ids), 0x0080, "Are Unique IDs supported", HFILL }},

	{ &hf_smb_mac_sup_streams,
	  { "Mac Streams", "smb.mac.streams_support", FT_BOOLEAN, 32,
	    TFS(&tfs_smb_mac_streams), 0x0100, "Are Mac Extensions and streams supported?", HFILL }},

	{ &hf_smb_create_dos_date,
		{ "Create Date", "smb.create.smb.date", FT_UINT16, BASE_HEX,
		NULL, 0, "Create Date, SMB_DATE format", HFILL }},

	{ &hf_smb_create_dos_time,
		{ "Create Time", "smb.create.smb.time", FT_UINT16, BASE_HEX,
		NULL, 0, "Create Time, SMB_TIME format", HFILL }},

	{ &hf_smb_last_write_time,
		{ "Last Write", "smb.last_write.time", FT_ABSOLUTE_TIME, ABSOLUTE_TIME_LOCAL,
		NULL, 0, "Time this file was last written to", HFILL }},

	{ &hf_smb_last_write_dos_date,
		{ "Last Write Date", "smb.last_write.smb.date", FT_UINT16, BASE_HEX,
		NULL, 0, "Last Write Date, SMB_DATE format", HFILL }},

	{ &hf_smb_last_write_dos_time,
		{ "Last Write Time", "smb.last_write.smb.time", FT_UINT16, BASE_HEX,
		NULL, 0, "Last Write Time, SMB_TIME format", HFILL }},

	{ &hf_smb_old_file_name,
		{ "Old File Name", "smb.old_file", FT_STRING, BASE_NONE,
		NULL, 0, "Old File Name (When renaming a file)", HFILL }},

	{ &hf_smb_offset,
		{ "Offset", "smb.offset", FT_UINT32, BASE_DEC,
		NULL, 0, "Offset in file", HFILL }},

	{ &hf_smb_remaining,
		{ "Remaining", "smb.remaining", FT_UINT32, BASE_DEC,
		NULL, 0, "Remaining number of bytes", HFILL }},

	{ &hf_smb_padding,
		{ "Padding", "smb.padding", FT_BYTES, BASE_NONE,
		NULL, 0, "Padding or unknown data", HFILL }},

	{ &hf_smb_file_data,
		{ "File Data", "smb.file_data", FT_BYTES, BASE_NONE,
		NULL, 0, "Data read/written to the file", HFILL }},

#if 0
	{ &hf_smb_raw_ea_data,
		{ "EA Data", "smb.ea_data", FT_BYTES, BASE_NONE,
		NULL, 0, "Data in EA list", HFILL }},
#endif

	{ &hf_smb_mac_fndrinfo,
	        { "Finder Info", "smb.mac.finderinfo", FT_BYTES, BASE_NONE,
		  NULL, 0, NULL, HFILL}},

	{ &hf_smb_total_data_len,
		{ "Total Data Length", "smb.total_data_len", FT_UINT16, BASE_DEC,
		NULL, 0, "Total length of data", HFILL }},

	{ &hf_smb_data_len,
		{ "Data Length", "smb.data_len", FT_UINT16, BASE_DEC,
		NULL, 0, "Length of data", HFILL }},

	{ &hf_smb_data_len_low,
		{ "Data Length Low", "smb.data_len_low", FT_UINT16, BASE_DEC,
		NULL, 0, "Length of data, Low 16 bits", HFILL }},

	{ &hf_smb_data_len_high,
		{ "Data Length High (multiply with 64K)", "smb.data_len_high", FT_UINT16, BASE_DEC,
		NULL, 0, "Length of data, High 16 bits", HFILL }},

	{ &hf_smb_seek_mode,
		{ "Seek Mode", "smb.seek_mode", FT_UINT16, BASE_DEC,
		VALS(seek_mode_vals), 0, "Seek Mode, what type of seek", HFILL }},

	{ &hf_smb_access_time,
		{ "Last Access", "smb.access.time", FT_ABSOLUTE_TIME, ABSOLUTE_TIME_LOCAL,
		NULL, 0, "Last Access Time", HFILL }},

	{ &hf_smb_access_dos_date,
		{ "Last Access Date", "smb.access.smb.date", FT_UINT16, BASE_HEX,
		NULL, 0, "Last Access Date, SMB_DATE format", HFILL }},

	{ &hf_smb_access_dos_time,
		{ "Last Access Time", "smb.access.smb.time", FT_UINT16, BASE_HEX,
		NULL, 0, "Last Access Time, SMB_TIME format", HFILL }},

	{ &hf_smb_data_size,
		{ "Data Size", "smb.data_size", FT_UINT32, BASE_DEC,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_alloc_size,
		{ "Allocation Size", "smb.alloc_size", FT_UINT32, BASE_DEC,
		NULL, 0, "Number of bytes to reserve on create or truncate", HFILL }},

	{ &hf_smb_max_count,
		{ "Max Count", "smb.maxcount", FT_UINT16, BASE_DEC,
		NULL, 0, "Maximum Count", HFILL }},

	{ &hf_smb_max_count_low,
		{ "Max Count Low", "smb.maxcount_low", FT_UINT16, BASE_DEC,
		NULL, 0, "Maximum Count, Low 16 bits", HFILL }},

	{ &hf_smb_max_count_high,
		{ "Max Count High (multiply with 64K)", "smb.maxcount_high", FT_UINT16, BASE_DEC,
		NULL, 0, "Maximum Count, High 16 bits", HFILL }},

	{ &hf_smb_min_count,
		{ "Min Count", "smb.mincount", FT_UINT16, BASE_DEC,
		NULL, 0, "Minimum Count", HFILL }},

	{ &hf_smb_timeout,
		{ "Timeout", "smb.timeout", FT_UINT32, BASE_DEC,
		NULL, 0, "Timeout in milliseconds", HFILL }},

	{ &hf_smb_high_offset,
		{ "High Offset", "smb.offset_high", FT_UINT32, BASE_DEC,
		NULL, 0, "High 32 Bits Of File Offset", HFILL }},

	{ &hf_smb_units,
		{ "Total Units", "smb.units", FT_UINT16, BASE_DEC,
		NULL, 0, "Total number of units at server", HFILL }},

	{ &hf_smb_bpu,
		{ "Blocks Per Unit", "smb.bpu", FT_UINT16, BASE_DEC,
		NULL, 0, "Blocks per unit at server", HFILL }},

	{ &hf_smb_blocksize,
		{ "Block Size", "smb.blocksize", FT_UINT16, BASE_DEC,
		NULL, 0, "Block size (in bytes) at server", HFILL }},

	{ &hf_smb_freeunits,
		{ "Free Units", "smb.free_units", FT_UINT16, BASE_DEC,
		NULL, 0, "Number of free units at server", HFILL }},

	{ &hf_smb_data_offset,
		{ "Data Offset", "smb.data_offset", FT_UINT16, BASE_DEC,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_dcm,
		{ "Data Compaction Mode", "smb.dcm", FT_UINT16, BASE_DEC,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_request_mask,
		{ "Request Mask", "smb.request.mask", FT_UINT32, BASE_HEX,
		NULL, 0, "Connectionless mode mask", HFILL }},

	{ &hf_smb_response_mask,
		{ "Response Mask", "smb.response.mask", FT_UINT32, BASE_HEX,
		NULL, 0, "Connectionless mode mask", HFILL }},

	{ &hf_smb_search_id,
		{ "Search ID", "smb.search_id", FT_UINT16, BASE_HEX,
		NULL, 0, "Search ID, handle for find operations", HFILL }},

	{ &hf_smb_write_mode,
		{ "Write Mode", "smb.write.mode", FT_UINT16, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_write_mode_write_through,
		{ "Write Through", "smb.write.mode.write_through", FT_BOOLEAN, 16,
		TFS(&tfs_write_mode_write_through), WRITE_MODE_WRITE_THROUGH, "Write through mode requested?", HFILL }},

	{ &hf_smb_write_mode_return_remaining,
		{ "Return Remaining", "smb.write.mode.return_remaining", FT_BOOLEAN, 16,
		TFS(&tfs_write_mode_return_remaining), WRITE_MODE_RETURN_REMAINING, "Return remaining data responses?", HFILL }},

	{ &hf_smb_write_mode_raw,
		{ "Write Raw", "smb.write.mode.raw", FT_BOOLEAN, 16,
		TFS(&tfs_write_mode_raw), WRITE_MODE_RAW, "Use WriteRawNamedPipe?", HFILL }},

	{ &hf_smb_write_mode_message_start,
		{ "Message Start", "smb.write.mode.message_start", FT_BOOLEAN, 16,
		TFS(&tfs_write_mode_message_start), WRITE_MODE_MESSAGE_START, "Is this the start of a message?", HFILL }},

	{ &hf_smb_write_mode_connectionless,
		{ "Connectionless", "smb.write.mode.connectionless", FT_BOOLEAN, 16,
		TFS(&tfs_write_mode_connectionless), WRITE_MODE_CONNECTIONLESS, "Connectionless mode requested?", HFILL }},

	{ &hf_smb_resume_key_len,
		{ "Resume Key Length", "smb.resume.key_len", FT_UINT16, BASE_DEC,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_resume_find_id,
		{ "Find ID", "smb.resume.find_id", FT_UINT8, BASE_HEX,
		NULL, 0, "Handle for Find operation", HFILL }},

	{ &hf_smb_resume_server_cookie,
		{ "Server Cookie", "smb.resume.server.cookie", FT_BYTES, BASE_NONE,
		NULL, 0, "Cookie, must not be modified by the client", HFILL }},

	{ &hf_smb_resume_client_cookie,
		{ "Client Cookie", "smb.resume.client.cookie", FT_BYTES, BASE_NONE,
		NULL, 0, "Cookie, must not be modified by the server", HFILL }},

	{ &hf_smb_andxoffset,
		{ "AndXOffset", "smb.andxoffset", FT_UINT16, BASE_DEC,
		NULL, 0, "Offset to next command in this SMB packet", HFILL }},

	{ &hf_smb_lock_type,
		{ "Lock Type", "smb.lock.type", FT_UINT8, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_lock_type_large,
		{ "Large Files", "smb.lock.type.large", FT_BOOLEAN, 8,
		TFS(&tfs_lock_type_large), 0x10, "Large file locking requested?", HFILL }},

	{ &hf_smb_lock_type_cancel,
		{ "Cancel", "smb.lock.type.cancel", FT_BOOLEAN, 8,
		TFS(&tfs_lock_type_cancel), 0x08, "Cancel outstanding lock requests?", HFILL }},

	{ &hf_smb_lock_type_change,
		{ "Change", "smb.lock.type.change", FT_BOOLEAN, 8,
		TFS(&tfs_lock_type_change), 0x04, "Change type of lock?", HFILL }},

	{ &hf_smb_lock_type_oplock,
		{ "Oplock Break", "smb.lock.type.oplock_release", FT_BOOLEAN, 8,
		TFS(&tfs_lock_type_oplock), 0x02, "Is this a notification of, or a response to, an oplock break?", HFILL }},

	{ &hf_smb_lock_type_shared,
		{ "Shared", "smb.lock.type.shared", FT_BOOLEAN, 8,
		TFS(&tfs_lock_type_shared), 0x01, "Shared or exclusive lock requested?", HFILL }},

	{ &hf_smb_locking_ol,
		{ "Oplock Level", "smb.locking.oplock.level", FT_UINT8, BASE_DEC,
		VALS(locking_ol_vals), 0, "Level of existing oplock at client (if any)", HFILL }},

	{ &hf_smb_number_of_locks,
		{ "Number of Locks", "smb.locking.num_locks", FT_UINT16, BASE_DEC,
		NULL, 0, "Number of lock requests in this request", HFILL }},

	{ &hf_smb_number_of_unlocks,
		{ "Number of Unlocks", "smb.locking.num_unlocks", FT_UINT16, BASE_DEC,
		NULL, 0, "Number of unlock requests in this request", HFILL }},

	{ &hf_smb_lock_long_length,
		{ "Length", "smb.lock.length", FT_UINT64, BASE_DEC,
		NULL, 0, "Length of lock/unlock region", HFILL }},

	{ &hf_smb_lock_long_offset,
		{ "Offset", "smb.lock.offset", FT_UINT64, BASE_DEC,
		NULL, 0, "Offset in the file of lock/unlock region", HFILL }},

	{ &hf_smb_file_type,
		{ "File Type", "smb.file_type", FT_UINT16, BASE_DEC,
		VALS(filetype_vals), 0, "Type of file", HFILL }},

	{ &hf_smb_ipc_state,
		{ "IPC State", "smb.ipc_state", FT_UINT16, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_ipc_state_nonblocking,
		{ "Nonblocking", "smb.ipc_state.nonblocking", FT_BOOLEAN, 16,
		TFS(&tfs_ipc_state_nonblocking), 0x8000, "Is I/O to this pipe nonblocking?", HFILL }},

	{ &hf_smb_ipc_state_endpoint,
		{ "Endpoint", "smb.ipc_state.endpoint", FT_UINT16, BASE_DEC,
		VALS(ipc_state_endpoint_vals), 0x4000, "Which end of the pipe this is", HFILL }},

	{ &hf_smb_ipc_state_pipe_type,
		{ "Pipe Type", "smb.ipc_state.pipe_type", FT_UINT16, BASE_DEC,
		VALS(ipc_state_pipe_type_vals), 0x0c00, "What type of pipe this is", HFILL }},

	{ &hf_smb_ipc_state_read_mode,
		{ "Read Mode", "smb.ipc_state.read_mode", FT_UINT16, BASE_DEC,
		VALS(ipc_state_read_mode_vals), 0x0300, "How this pipe should be read", HFILL }},

	{ &hf_smb_ipc_state_icount,
		{ "Icount", "smb.ipc_state.icount", FT_UINT16, BASE_DEC,
		NULL, 0x00FF, "Count to control pipe instancing", HFILL }},

	{ &hf_smb_server_fid,
		{ "Server FID", "smb.server_fid", FT_UINT32, BASE_HEX,
		NULL, 0, "Server unique File ID", HFILL }},

	{ &hf_smb_open_flags,
		{ "Flags", "smb.open.flags", FT_UINT16, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_open_flags_add_info,
		{ "Additional Info", "smb.open.flags.add_info", FT_BOOLEAN, 16,
		TFS(&tfs_open_flags_add_info), 0x0001, "Additional Information Requested?", HFILL }},

	{ &hf_smb_open_flags_ex_oplock,
		{ "Exclusive Oplock", "smb.open.flags.ex_oplock", FT_BOOLEAN, 16,
		TFS(&tfs_open_flags_ex_oplock), 0x0002, "Exclusive Oplock Requested?", HFILL }},

	{ &hf_smb_open_flags_batch_oplock,
		{ "Batch Oplock", "smb.open.flags.batch_oplock", FT_BOOLEAN, 16,
		TFS(&tfs_open_flags_batch_oplock), 0x0004, "Batch Oplock Requested?", HFILL }},

	{ &hf_smb_open_flags_ealen,
		{ "Total EA Len", "smb.open.flags.ealen", FT_BOOLEAN, 16,
		TFS(&tfs_open_flags_ealen), 0x0008, "Total EA Len Requested?", HFILL }},

	{ &hf_smb_open_action,
		{ "Action", "smb.open.action", FT_UINT16, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_open_action_open,
		{ "Open Action", "smb.open.action.open", FT_UINT16, BASE_DEC,
		VALS(oa_open_vals), 0x0003, "Open Action, how the file was opened", HFILL }},

	{ &hf_smb_open_action_lock,
		{ "Exclusive Open", "smb.open.action.lock", FT_BOOLEAN, 16,
		TFS(&tfs_oa_lock), 0x8000, "Is this file opened by another user?", HFILL }},

	{ &hf_smb_vc_num,
		{ "VC Number", "smb.vc", FT_UINT16, BASE_DEC,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_password_len,
		{ "Password Length", "smb.pwlen", FT_UINT16, BASE_DEC,
		NULL, 0, "Length of password", HFILL }},

	{ &hf_smb_ansi_password_len,
		{ "ANSI Password Length", "smb.ansi_pwlen", FT_UINT16, BASE_DEC,
		NULL, 0, "Length of ANSI password", HFILL }},

	{ &hf_smb_unicode_password_len,
		{ "Unicode Password Length", "smb.unicode_pwlen", FT_UINT16, BASE_DEC,
		NULL, 0, "Length of Unicode password", HFILL }},

	{ &hf_smb_account,
		{ "Account", "smb.account", FT_STRING, BASE_NONE,
		NULL, 0, "Account, username", HFILL }},

	{ &hf_smb_os,
		{ "Native OS", "smb.native_os", FT_STRING, BASE_NONE,
		NULL, 0, "Which OS we are running", HFILL }},

	{ &hf_smb_lanman,
		{ "Native LAN Manager", "smb.native_lanman", FT_STRING, BASE_NONE,
		NULL, 0, "Which LANMAN protocol we are running", HFILL }},

	{ &hf_smb_setup_action,
		{ "Action", "smb.setup.action", FT_UINT16, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_setup_action_guest,
		{ "Guest", "smb.setup.action.guest", FT_BOOLEAN, 16,
		TFS(&tfs_setup_action_guest), 0x0001, "Client logged in as GUEST?", HFILL }},

	{ &hf_smb_fs,
		{ "Native File System", "smb.native_fs", FT_STRING, BASE_NONE,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_connect_flags,
		{ "Flags", "smb.connect.flags", FT_UINT16, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_connect_flags_dtid,
		{ "Disconnect TID", "smb.connect.flags.dtid", FT_BOOLEAN, 16,
		TFS(&tfs_disconnect_tid), 0x0001, "Disconnect TID?", HFILL }},

	{ &hf_smb_connect_flags_ext_sig,
		{ "Extended Signature", "smb.connect.flags.extendedsig", FT_BOOLEAN, 16,
		TFS(&tfs_extended_signature), 0x0004, "Extended signature?", HFILL }},

	{ &hf_smb_connect_flags_ext_resp,
		{ "Extended Response", "smb.connect.flags.extendedresp", FT_BOOLEAN, 16,
		TFS(&tfs_extended_response), 0x0008, "Extended response?", HFILL }},

	{ &hf_smb_connect_support,
		{ "Optional Support", "smb.connect.support", FT_UINT16, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_connect_support_search,
		{ "Search Bits", "smb.connect.support.search", FT_BOOLEAN, 16,
		TFS(&tfs_connect_support_search), 0x0001, "Exclusive Search Bits supported?", HFILL }},

	{ &hf_smb_connect_support_in_dfs,
		{ "In Dfs", "smb.connect.support.dfs", FT_BOOLEAN, 16,
		TFS(&tfs_connect_support_in_dfs), 0x0002, "Is this in a Dfs tree?", HFILL }},

	{ &hf_smb_connect_support_csc_mask_vals,
		{ "CSC Mask", "smb.connect.support.cscmask", FT_UINT16, BASE_DEC,
		VALS(connect_support_csc_mask_vals), 0x000c, "CSC mask?", HFILL }},

	{ &hf_smb_connect_support_uniquefilename,
		{ "Unique File Name", "smb.connect.support.uniqfilename", FT_BOOLEAN, 16,
		TFS(&tfs_connect_support_uniquefilename), 0x0010, "Unique file name supported?", HFILL }},

	{ &hf_smb_connect_support_extended_signature,
		{ "Extended Signatures", "smb.connect.support.extendedsig", FT_BOOLEAN, 16,
		TFS(&tfs_connect_support_extended_signature), 0x0020, "Extended signatures?", HFILL }},

	{ &hf_smb_max_setup_count,
		{ "Max Setup Count", "smb.msc", FT_UINT8, BASE_DEC,
		NULL, 0, "Maximum number of setup words to return", HFILL }},

	{ &hf_smb_total_param_count,
		{ "Total Parameter Count", "smb.tpc", FT_UINT32, BASE_DEC,
		NULL, 0, "Total number of parameter bytes", HFILL }},

	{ &hf_smb_total_data_count,
		{ "Total Data Count", "smb.tdc", FT_UINT32, BASE_DEC,
		NULL, 0, "Total number of data bytes", HFILL }},

	{ &hf_smb_max_param_count,
		{ "Max Parameter Count", "smb.mpc", FT_UINT32, BASE_DEC,
		NULL, 0, "Maximum number of parameter bytes to return", HFILL }},

	{ &hf_smb_max_data_count,
		{ "Max Data Count", "smb.mdc", FT_UINT32, BASE_DEC,
		NULL, 0, "Maximum number of data bytes to return", HFILL }},

	{ &hf_smb_param_disp16,
		{ "Parameter Displacement", "smb.pd", FT_UINT16, BASE_DEC,
		NULL, 0, "Displacement of these parameter bytes", HFILL }},

	{ &hf_smb_param_count16,
		{ "Parameter Count", "smb.pc", FT_UINT16, BASE_DEC,
		NULL, 0, "Number of parameter bytes in this buffer", HFILL }},

	{ &hf_smb_param_offset16,
		{ "Parameter Offset", "smb.po", FT_UINT16, BASE_DEC,
		NULL, 0, "Offset (from header start) to parameters", HFILL }},

	{ &hf_smb_param_disp32,
		{ "Parameter Displacement", "smb.pd", FT_UINT32, BASE_DEC,
		NULL, 0, "Displacement of these parameter bytes", HFILL }},

	{ &hf_smb_param_count32,
		{ "Parameter Count", "smb.pc", FT_UINT32, BASE_DEC,
		NULL, 0, "Number of parameter bytes in this buffer", HFILL }},

	{ &hf_smb_param_offset32,
		{ "Parameter Offset", "smb.po", FT_UINT32, BASE_DEC,
		NULL, 0, "Offset (from header start) to parameters", HFILL }},

	{ &hf_smb_data_count16,
		{ "Data Count", "smb.dc", FT_UINT16, BASE_DEC,
		NULL, 0, "Number of data bytes in this buffer", HFILL }},

	{ &hf_smb_data_disp16,
		{ "Data Displacement", "smb.data_disp", FT_UINT16, BASE_DEC,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_data_offset16,
		{ "Data Offset", "smb.data_offset", FT_UINT16, BASE_DEC,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_data_count32,
		{ "Data Count", "smb.dc", FT_UINT32, BASE_DEC,
		NULL, 0, "Number of data bytes in this buffer", HFILL }},

	{ &hf_smb_data_disp32,
		{ "Data Displacement", "smb.data_disp", FT_UINT32, BASE_DEC,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_data_offset32,
		{ "Data Offset", "smb.data_offset", FT_UINT32, BASE_DEC,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_setup_count,
		{ "Setup Count", "smb.sc", FT_UINT8, BASE_DEC,
		NULL, 0, "Number of setup words in this buffer", HFILL }},

	{ &hf_smb_nt_ioctl_isfsctl,
		{ "IsFSctl", "smb.nt.ioctl.isfsctl", FT_UINT8, BASE_DEC,
		VALS(nt_ioctl_isfsctl_vals), 0, "Is this a device IOCTL (FALSE) or FS Control (TRUE)", HFILL }},

	{ &hf_smb_nt_ioctl_flags_completion_filter,
		{ "Completion Filter", "smb.nt.ioctl.completion_filter", FT_UINT8, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_nt_ioctl_flags_root_handle,
		{ "Root Handle", "smb.nt.ioctl.flags.root_handle", FT_BOOLEAN, 8,
		TFS(&tfs_nt_ioctl_flags_root_handle), NT_IOCTL_FLAGS_ROOT_HANDLE, "Apply to this share or root Dfs share", HFILL }},

	{ &hf_smb_nt_notify_action,
		{ "Action", "smb.nt.notify.action", FT_UINT32, BASE_DEC,
		VALS(nt_notify_action_vals), 0, "Which action caused this notify response", HFILL }},

	{ &hf_smb_nt_notify_watch_tree,
		{ "Watch Tree", "smb.nt.notify.watch_tree", FT_UINT8, BASE_DEC,
		VALS(watch_tree_vals), 0, "Should Notify watch subdirectories also?", HFILL }},


	{ &hf_smb_nt_notify_completion_filter,
		{ "Completion Filter", "smb.nt.notify.completion_filter", FT_UINT32, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},
	{ &hf_smb_nt_notify_file_name,
		{ "File Name Change", "smb.nt.notify.file_name", FT_BOOLEAN, 32,
		TFS(&tfs_nt_notify_file_name), NT_NOTIFY_FILE_NAME, "Notify on changes to file name", HFILL }},

	{ &hf_smb_nt_notify_dir_name,
		{ "Directory Name Change", "smb.nt.notify.dir_name", FT_BOOLEAN, 32,
		TFS(&tfs_nt_notify_dir_name), NT_NOTIFY_DIR_NAME, "Notify on changes to directory name", HFILL }},

	{ &hf_smb_nt_notify_attributes,
		{ "Attribute Change", "smb.nt.notify.attributes", FT_BOOLEAN, 32,
		TFS(&tfs_nt_notify_attributes), NT_NOTIFY_ATTRIBUTES, "Notify on changes to attributes", HFILL }},

	{ &hf_smb_nt_notify_size,
		{ "Size Change", "smb.nt.notify.size", FT_BOOLEAN, 32,
		TFS(&tfs_nt_notify_size), NT_NOTIFY_SIZE, "Notify on changes to size", HFILL }},

	{ &hf_smb_nt_notify_last_write,
		{ "Last Write Change", "smb.nt.notify.last_write", FT_BOOLEAN, 32,
		TFS(&tfs_nt_notify_last_write), NT_NOTIFY_LAST_WRITE, "Notify on changes to last write", HFILL }},

	{ &hf_smb_nt_notify_last_access,
		{ "Last Access Change", "smb.nt.notify.last_access", FT_BOOLEAN, 32,
		TFS(&tfs_nt_notify_last_access), NT_NOTIFY_LAST_ACCESS, "Notify on changes to last access", HFILL }},

	{ &hf_smb_nt_notify_creation,
		{ "Created Change", "smb.nt.notify.creation", FT_BOOLEAN, 32,
		TFS(&tfs_nt_notify_creation), NT_NOTIFY_CREATION, "Notify on changes to creation time", HFILL }},

	{ &hf_smb_nt_notify_ea,
		{ "EA Change", "smb.nt.notify.ea", FT_BOOLEAN, 32,
		TFS(&tfs_nt_notify_ea), NT_NOTIFY_EA, "Notify on changes to Extended Attributes", HFILL }},

	{ &hf_smb_nt_notify_security,
		{ "Security Change", "smb.nt.notify.security", FT_BOOLEAN, 32,
		TFS(&tfs_nt_notify_security), NT_NOTIFY_SECURITY, "Notify on changes to security settings", HFILL }},

	{ &hf_smb_nt_notify_stream_name,
		{ "Stream Name Change", "smb.nt.notify.stream_name", FT_BOOLEAN, 32,
		TFS(&tfs_nt_notify_stream_name), NT_NOTIFY_STREAM_NAME, "Notify on changes to stream name?", HFILL }},

	{ &hf_smb_nt_notify_stream_size,
		{ "Stream Size Change", "smb.nt.notify.stream_size", FT_BOOLEAN, 32,
		TFS(&tfs_nt_notify_stream_size), NT_NOTIFY_STREAM_SIZE, "Notify on changes of stream size", HFILL }},

	{ &hf_smb_nt_notify_stream_write,
		{ "Stream Write", "smb.nt.notify.stream_write", FT_BOOLEAN, 32,
		TFS(&tfs_nt_notify_stream_write), NT_NOTIFY_STREAM_WRITE, "Notify on stream write?", HFILL }},


	{ &hf_smb_root_dir_fid,
		{ "Root FID", "smb.rfid", FT_UINT32, BASE_HEX,
		NULL, 0, "Open is relative to this FID (if nonzero)", HFILL }},

	{ &hf_smb_alloc_size64,
		{ "Allocation Size", "smb.alloc_size64", FT_UINT64, BASE_DEC,
		NULL, 0, "Number of bytes to reserve on create or truncate", HFILL }},

	{ &hf_smb_nt_create_disposition,
		{ "Disposition", "smb.create.disposition", FT_UINT32, BASE_DEC,
		VALS(create_disposition_vals), 0, "Create disposition, what to do if the file does/does not exist", HFILL }},

	{ &hf_smb_sd_length,
		{ "SD Length", "smb.sd.length", FT_UINT32, BASE_DEC,
		NULL, 0, "Total length of security descriptor", HFILL }},

	{ &hf_smb_ea_list_length,
		{ "EA List Length", "smb.ea.list_length", FT_UINT32, BASE_DEC,
		NULL, 0, "Total length of extended attributes", HFILL }},

	{ &hf_smb_ea_flags,
		{ "EA Flags", "smb.ea.flags", FT_UINT8, BASE_HEX,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_ea_name_length,
		{ "EA Name Length", "smb.ea.name_length", FT_UINT8, BASE_DEC,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_ea_data_length,
		{ "EA Data Length", "smb.ea.data_length", FT_UINT16, BASE_DEC,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_ea_name,
		{ "EA Name", "smb.ea.name", FT_STRING, BASE_NONE,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_ea_data,
		{ "EA Data", "smb.ea.data", FT_BYTES, BASE_NONE,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_file_name_len,
		{ "File Name Len", "smb.file_name_len", FT_UINT32, BASE_DEC,
		NULL, 0, "Length of File Name", HFILL }},

	{ &hf_smb_nt_impersonation_level,
		{ "Impersonation", "smb.impersonation.level", FT_UINT32, BASE_DEC,
		VALS(impersonation_level_vals), 0, "Impersonation level", HFILL }},

	{ &hf_smb_nt_security_flags,
		{ "Security Flags", "smb.security.flags", FT_UINT8, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_nt_security_flags_context_tracking,
		{ "Context Tracking", "smb.security.flags.context_tracking", FT_BOOLEAN, 8,
		TFS(&tfs_nt_security_flags_context_tracking), 0x01, "Is security tracking static or dynamic?", HFILL }},

	{ &hf_smb_nt_security_flags_effective_only,
		{ "Effective Only", "smb.security.flags.effective_only", FT_BOOLEAN, 8,
		TFS(&tfs_nt_security_flags_effective_only), 0x02, "Are only enabled or all aspects uf the users SID available?", HFILL }},

	{ &hf_smb_nt_access_mask_generic_read,
		{ "Generic Read", "smb.access.generic_read", FT_BOOLEAN, 32,
		TFS(&tfs_nt_access_mask_generic_read), 0x80000000, "Is generic read allowed for this object?", HFILL }},

	{ &hf_smb_nt_access_mask_generic_write,
		{ "Generic Write", "smb.access.generic_write", FT_BOOLEAN, 32,
		TFS(&tfs_nt_access_mask_generic_write), 0x40000000, "Is generic write allowed for this object?", HFILL }},

	{ &hf_smb_nt_access_mask_generic_execute,
		{ "Generic Execute", "smb.access.generic_execute", FT_BOOLEAN, 32,
		TFS(&tfs_nt_access_mask_generic_execute), 0x20000000, "Is generic execute allowed for this object?", HFILL }},

	{ &hf_smb_nt_access_mask_generic_all,
		{ "Generic All", "smb.access.generic_all", FT_BOOLEAN, 32,
		TFS(&tfs_nt_access_mask_generic_all), 0x10000000, "Is generic all allowed for this attribute", HFILL }},

	{ &hf_smb_nt_access_mask_maximum_allowed,
		{ "Maximum Allowed", "smb.access.maximum_allowed", FT_BOOLEAN, 32,
		TFS(&tfs_nt_access_mask_maximum_allowed), 0x02000000, "?", HFILL }},

	{ &hf_smb_nt_access_mask_system_security,
		{ "System Security", "smb.access.system_security", FT_BOOLEAN, 32,
		TFS(&tfs_nt_access_mask_system_security), 0x01000000, "Access to a system ACL?", HFILL }},

	{ &hf_smb_nt_access_mask_synchronize,
		{ "Synchronize", "smb.access.synchronize", FT_BOOLEAN, 32,
		TFS(&tfs_nt_access_mask_synchronize), 0x00100000, "Windows NT: synchronize access", HFILL }},

	{ &hf_smb_nt_access_mask_write_owner,
		{ "Write Owner", "smb.access.write_owner", FT_BOOLEAN, 32,
		TFS(&tfs_nt_access_mask_write_owner), 0x00080000, "Can owner write to the object?", HFILL }},

	{ &hf_smb_nt_access_mask_write_dac,
		{ "Write DAC", "smb.access.write_dac", FT_BOOLEAN, 32,
		TFS(&tfs_nt_access_mask_write_dac), 0x00040000, "Is write allowed to the owner group or ACLs?", HFILL }},

	{ &hf_smb_nt_access_mask_read_control,
		{ "Read Control", "smb.access.read_control", FT_BOOLEAN, 32,
		TFS(&tfs_nt_access_mask_read_control), 0x00020000, "Are reads allowed of owner, group and ACL data of the SID?", HFILL }},

	{ &hf_smb_nt_access_mask_delete,
		{ "Delete", "smb.access.delete", FT_BOOLEAN, 32,
		TFS(&tfs_nt_access_mask_delete), 0x00010000, "Can object be deleted", HFILL }},

	{ &hf_smb_nt_access_mask_write_attributes,
		{ "Write Attributes", "smb.access.write_attributes", FT_BOOLEAN, 32,
		TFS(&tfs_nt_access_mask_write_attributes), 0x00000100, "Can object's attributes be written", HFILL }},

	{ &hf_smb_nt_access_mask_read_attributes,
		{ "Read Attributes", "smb.access.read_attributes", FT_BOOLEAN, 32,
		TFS(&tfs_nt_access_mask_read_attributes), 0x00000080, "Can object's attributes be read", HFILL }},

	{ &hf_smb_nt_access_mask_delete_child,
		{ "Delete Child", "smb.access.delete_child", FT_BOOLEAN, 32,
		TFS(&tfs_nt_access_mask_delete_child), 0x00000040, "Can object's subdirectories be deleted", HFILL }},

	/*
	 * "Execute" for files, "traverse" for directories.
	 */
	{ &hf_smb_nt_access_mask_execute,
		{ "Execute", "smb.access.execute", FT_BOOLEAN, 32,
		TFS(&tfs_nt_access_mask_execute), 0x00000020, "Can object be executed (if file) or traversed (if directory)", HFILL }},

	{ &hf_smb_nt_access_mask_write_ea,
		{ "Write EA", "smb.access.write_ea", FT_BOOLEAN, 32,
		TFS(&tfs_nt_access_mask_write_ea), 0x00000010, "Can object's extended attributes be written", HFILL }},

	{ &hf_smb_nt_access_mask_read_ea,
		{ "Read EA", "smb.access.read_ea", FT_BOOLEAN, 32,
		TFS(&tfs_nt_access_mask_read_ea), 0x00000008, "Can object's extended attributes be read", HFILL }},

	/*
	 * "Append data" for files, "add subdirectory" for directories,
	 * "create pipe instance" for named pipes.
	 */
	{ &hf_smb_nt_access_mask_append,
		{ "Append", "smb.access.append", FT_BOOLEAN, 32,
		TFS(&tfs_nt_access_mask_append), 0x00000004, "Can object's contents be appended to", HFILL }},

	/*
	 * "Write data" for files and pipes, "add file" for directory.
	 */
	{ &hf_smb_nt_access_mask_write,
		{ "Write", "smb.access.write", FT_BOOLEAN, 32,
		TFS(&tfs_nt_access_mask_write), 0x00000002, "Can object's contents be written", HFILL }},

	/*
	 * "Read data" for files and pipes, "list directory" for directory.
	 */
	{ &hf_smb_nt_access_mask_read,
		{ "Read", "smb.access.read", FT_BOOLEAN, 32,
		TFS(&tfs_nt_access_mask_read), 0x00000001, "Can object's contents be read", HFILL }},

	{ &hf_smb_nt_create_bits_oplock,
		{ "Exclusive Oplock", "smb.nt.create.oplock", FT_BOOLEAN, 32,
		TFS(&tfs_nt_create_bits_oplock), 0x00000002, "Is an oplock requested", HFILL }},

	{ &hf_smb_nt_create_bits_boplock,
		{ "Batch Oplock", "smb.nt.create.batch_oplock", FT_BOOLEAN, 32,
		TFS(&tfs_nt_create_bits_boplock), 0x00000004, "Is a batch oplock requested?", HFILL }},

	{ &hf_smb_nt_create_bits_dir,
		{ "Create Directory", "smb.nt.create.dir", FT_BOOLEAN, 32,
		TFS(&tfs_nt_create_bits_dir), 0x00000008, "Must target of open be a directory?", HFILL }},

	{ &hf_smb_nt_create_bits_ext_resp,
	  { "Extended Response", "smb.nt.create.ext", FT_BOOLEAN, 32,
	    TFS(&tfs_nt_create_bits_ext_resp), 0x00000010, "Extended response required?", HFILL }},

	{ &hf_smb_nt_create_options_directory_file,
		{ "Directory", "smb.nt.create_options.directory", FT_BOOLEAN, 32,
		TFS(&tfs_nt_create_options_directory), 0x00000001, "Should file being opened/created be a directory?", HFILL }},

	{ &hf_smb_nt_create_options_write_through,
		{ "Write Through", "smb.nt.create_options.write_through", FT_BOOLEAN, 32,
		TFS(&tfs_nt_create_options_write_through), 0x00000002, "Should writes to the file write buffered data out before completing?", HFILL }},

	{ &hf_smb_nt_create_options_sequential_only,
		{ "Sequential Only", "smb.nt.create_options.sequential_only", FT_BOOLEAN, 32,
		TFS(&tfs_nt_create_options_sequential_only), 0x00000004, "Will access to this file only be sequential?", HFILL }},

	{ &hf_smb_nt_create_options_no_intermediate_buffering,
		{ "Intermediate Buffering", "smb.nt.create_options.intermediate_buffering", FT_BOOLEAN, 32,
		TFS(&tfs_nt_create_options_no_intermediate_buffering), 0x00000008, "Is intermediate buffering allowed?", HFILL }},

	{ &hf_smb_nt_create_options_sync_io_alert,
		{ "Sync I/O Alert", "smb.nt.create_options.sync_io_alert", FT_BOOLEAN, 32,
		TFS(&tfs_nt_create_options_sync_io_alert), 0x00000010, "All operations are performed synchronous", HFILL}},

	{ &hf_smb_nt_create_options_sync_io_nonalert,
		{ "Sync I/O Nonalert", "smb.nt.create_options.sync_io_nonalert", FT_BOOLEAN, 32,
		TFS(&tfs_nt_create_options_sync_io_nonalert), 0x00000020, "All operations are synchronous and may block", HFILL}},

	{ &hf_smb_nt_create_options_non_directory_file,
		{ "Non-Directory", "smb.nt.create_options.non_directory", FT_BOOLEAN, 32,
		TFS(&tfs_nt_create_options_non_directory), 0x00000040, "Should file being opened/created be a non-directory?", HFILL }},

	{ &hf_smb_nt_create_options_create_tree_connection,
		{ "Create Tree Connection", "smb.nt.create_options.create_tree_connection", FT_BOOLEAN, 32,
		TFS(&tfs_nt_create_options_create_tree_connection), 0x00000080, "Create Tree Connection flag", HFILL }},

	{ &hf_smb_nt_create_options_complete_if_oplocked,
		{ "Complete If Oplocked", "smb.nt.create_options.complete_if_oplocked", FT_BOOLEAN, 32,
		TFS(&tfs_nt_create_options_complete_if_oplocked), 0x00000100, "Complete if oplocked flag", HFILL }},

	{ &hf_smb_nt_create_options_no_ea_knowledge,
		{ "No EA Knowledge", "smb.nt.create_options.no_ea_knowledge", FT_BOOLEAN, 32,
		TFS(&tfs_nt_create_options_no_ea_knowledge), 0x00000200, "Does the client not understand extended attributes?", HFILL }},

	{ &hf_smb_nt_create_options_eight_dot_three_only,
		{ "8.3 Only", "smb.nt.create_options.eight_dot_three_only", FT_BOOLEAN, 32,
		TFS(&tfs_nt_create_options_eight_dot_three_only), 0x00000400, "Does the client understand only 8.3 filenames?", HFILL }},

	{ &hf_smb_nt_create_options_random_access,
		{ "Random Access", "smb.nt.create_options.random_access", FT_BOOLEAN, 32,
		TFS(&tfs_nt_create_options_random_access), 0x00000800, "Will the client be accessing the file randomly?", HFILL }},

	{ &hf_smb_nt_create_options_delete_on_close,
		{ "Delete On Close", "smb.nt.create_options.delete_on_close", FT_BOOLEAN, 32,
		TFS(&tfs_nt_create_options_delete_on_close), 0x00001000, "Should the file be deleted when closed?", HFILL }},
	{ &hf_smb_nt_create_options_open_by_fileid,
		{ "Open By FileID", "smb.nt.create_options.open_by_fileid", FT_BOOLEAN, 32,
		TFS(&tfs_nt_create_options_open_by_fileid), 0x00002000, "Open file by inode", HFILL }},

	{ &hf_smb_nt_create_options_backup_intent,
		{ "Backup Intent", "smb.nt.create_options.backup_intent", FT_BOOLEAN, 32,
		TFS(&tfs_nt_create_options_backup_intent), 0x00004000, "Is this opened by BACKUP ADMIN for backup intent?", HFILL }},

	{ &hf_smb_nt_create_options_no_compression,
		{ "No Compression", "smb.nt.create_options.no_compression", FT_BOOLEAN, 32,
		TFS(&tfs_nt_create_options_no_compression), 0x00008000, "Is compression allowed?", HFILL }},

	{ &hf_smb_nt_create_options_reserve_opfilter,
		{ "Reserve Opfilter", "smb.nt.create_options.reserve_opfilter", FT_BOOLEAN, 32,
		TFS(&tfs_nt_create_options_reserve_opfilter), 0x00100000, "Reserve Opfilter flag", HFILL }},

	{ &hf_smb_nt_create_options_open_reparse_point,
		{ "Open Reparse Point", "smb.nt.create_options.open_reparse_point", FT_BOOLEAN, 32,
		TFS(&tfs_nt_create_options_open_reparse_point), 0x00200000, "Is this an open of a reparse point or of the normal file?", HFILL }},

	{ &hf_smb_nt_create_options_open_no_recall,
		{ "Open No Recall", "smb.nt.create_options.open_no_recall", FT_BOOLEAN, 32,
		TFS(&tfs_nt_create_options_open_no_recall), 0x00400000, "Open no recall flag", HFILL }},

	{ &hf_smb_nt_create_options_open_for_free_space_query,
		{ "Open For Free Space query", "smb.nt.create_options.open_for_free_space_query", FT_BOOLEAN, 32,
		TFS(&tfs_nt_create_options_open_for_free_space_query), 0x00800000, "Open For Free Space Query flag", HFILL }},

	{ &hf_smb_nt_share_access_read,
		{ "Read", "smb.share.access.read", FT_BOOLEAN, 32,
		TFS(&tfs_nt_share_access_read), SHARE_ACCESS_READ, "Can the object be shared for reading?", HFILL }},

	{ &hf_smb_nt_share_access_write,
		{ "Write", "smb.share.access.write", FT_BOOLEAN, 32,
		TFS(&tfs_nt_share_access_write), SHARE_ACCESS_WRITE, "Can the object be shared for write?", HFILL }},

	{ &hf_smb_nt_share_access_delete,
		{ "Delete", "smb.share.access.delete", FT_BOOLEAN, 32,
		TFS(&tfs_nt_share_access_delete), SHARE_ACCESS_DELETE, NULL, HFILL }},

	{ &hf_smb_file_eattr,
		{ "File Attributes", "smb.file_attribute", FT_UINT32, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_file_eattr_read_only,
		{ "Read Only", "smb.file_attribute.read_only", FT_BOOLEAN, 32,
		TFS(&tfs_file_attribute_read_only), SMB_FILE_ATTRIBUTE_READ_ONLY, "READ ONLY file attribute", HFILL }},

	{ &hf_smb_file_eattr_hidden,
		{ "Hidden", "smb.file_attribute.hidden", FT_BOOLEAN, 32,
		TFS(&tfs_file_attribute_hidden), SMB_FILE_ATTRIBUTE_HIDDEN, "HIDDEN file attribute", HFILL }},

	{ &hf_smb_file_eattr_system,
		{ "System", "smb.file_attribute.system", FT_BOOLEAN, 32,
		TFS(&tfs_file_attribute_system), SMB_FILE_ATTRIBUTE_SYSTEM, "SYSTEM file attribute", HFILL }},

	{ &hf_smb_file_eattr_volume,
		{ "Volume ID", "smb.file_attribute.volume", FT_BOOLEAN, 32,
		TFS(&tfs_file_attribute_volume), SMB_FILE_ATTRIBUTE_VOLUME, "VOLUME file attribute", HFILL }},

	{ &hf_smb_file_eattr_directory,
		{ "Directory", "smb.file_attribute.directory", FT_BOOLEAN, 32,
		TFS(&tfs_file_attribute_directory), SMB_FILE_ATTRIBUTE_DIRECTORY, "DIRECTORY file attribute", HFILL }},

	{ &hf_smb_file_eattr_archive,
		{ "Archive", "smb.file_attribute.archive", FT_BOOLEAN, 32,
		TFS(&tfs_file_attribute_archive), SMB_FILE_ATTRIBUTE_ARCHIVE, "ARCHIVE file attribute", HFILL }},

	{ &hf_smb_file_eattr_device,
		{ "Device", "smb.file_attribute.device", FT_BOOLEAN, 32,
		TFS(&tfs_file_attribute_device), SMB_FILE_ATTRIBUTE_DEVICE, "Is this file a device?", HFILL }},

	{ &hf_smb_file_eattr_normal,
		{ "Normal", "smb.file_attribute.normal", FT_BOOLEAN, 32,
		TFS(&tfs_file_attribute_normal), SMB_FILE_ATTRIBUTE_NORMAL, "Is this a normal file?", HFILL }},

	{ &hf_smb_file_eattr_temporary,
		{ "Temporary", "smb.file_attribute.temporary", FT_BOOLEAN, 32,
		TFS(&tfs_file_attribute_temporary), SMB_FILE_ATTRIBUTE_TEMPORARY, "Is this a temporary file?", HFILL }},

	{ &hf_smb_file_eattr_sparse,
		{ "Sparse", "smb.file_attribute.sparse", FT_BOOLEAN, 32,
		TFS(&tfs_file_attribute_sparse), SMB_FILE_ATTRIBUTE_SPARSE, "Is this a sparse file?", HFILL }},

	{ &hf_smb_file_eattr_reparse,
		{ "Reparse Point", "smb.file_attribute.reparse", FT_BOOLEAN, 32,
		TFS(&tfs_file_attribute_reparse), SMB_FILE_ATTRIBUTE_REPARSE, "Does this file have an associated reparse point?", HFILL }},

	{ &hf_smb_file_eattr_compressed,
		{ "Compressed", "smb.file_attribute.compressed", FT_BOOLEAN, 32,
		TFS(&tfs_file_attribute_compressed), SMB_FILE_ATTRIBUTE_COMPRESSED, "Is this file compressed?", HFILL }},

	{ &hf_smb_file_eattr_offline,
		{ "Offline", "smb.file_attribute.offline", FT_BOOLEAN, 32,
		TFS(&tfs_file_attribute_offline), SMB_FILE_ATTRIBUTE_OFFLINE, "Is this file offline?", HFILL }},

	{ &hf_smb_file_eattr_not_content_indexed,
		{ "Content Indexed", "smb.file_attribute.not_content_indexed", FT_BOOLEAN, 32,
		TFS(&tfs_file_attribute_not_content_indexed), SMB_FILE_ATTRIBUTE_NOT_CONTENT_INDEXED, "May this file be indexed by the content indexing service", HFILL }},

	{ &hf_smb_file_eattr_encrypted,
		{ "Encrypted", "smb.file_attribute.encrypted", FT_BOOLEAN, 32,
		TFS(&tfs_file_attribute_encrypted), SMB_FILE_ATTRIBUTE_ENCRYPTED, "Is this file encrypted?", HFILL }},

	{ &hf_smb_size_returned_quota_data,
		{ "Size of returned Quota data", "smb.size_returned_quota_data", FT_UINT32, BASE_DEC,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_sec_desc_len,
		{ "NT Security Descriptor Length", "smb.sec_desc_len", FT_UINT32, BASE_DEC,
		NULL, 0, "Security Descriptor Length", HFILL }},

	{ &hf_smb_nt_qsd,
		{ "Security Information", "smb.nt_qsd", FT_UINT32, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_nt_qsd_owner,
		{ "Owner", "smb.nt_qsd.owner", FT_BOOLEAN, 32,
		TFS(&tfs_nt_qsd_owner), NT_QSD_OWNER, "Is owner security information being queried?", HFILL }},

	{ &hf_smb_nt_qsd_group,
		{ "Group", "smb.nt_qsd.group", FT_BOOLEAN, 32,
		TFS(&tfs_nt_qsd_group), NT_QSD_GROUP, "Is group security information being queried?", HFILL }},

	{ &hf_smb_nt_qsd_dacl,
		{ "DACL", "smb.nt_qsd.dacl", FT_BOOLEAN, 32,
		TFS(&tfs_nt_qsd_dacl), NT_QSD_DACL, "Is DACL security information being queried?", HFILL }},

	{ &hf_smb_nt_qsd_sacl,
		{ "SACL", "smb.nt_qsd.sacl", FT_BOOLEAN, 32,
		TFS(&tfs_nt_qsd_sacl), NT_QSD_SACL, "Is SACL security information being queried?", HFILL }},

	{ &hf_smb_extended_attributes,
		{ "Extended Attributes", "smb.ext_attr", FT_BYTES, BASE_NONE,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_oplock_level,
		{ "Oplock level", "smb.oplock.level", FT_UINT8, BASE_DEC,
		VALS(oplock_level_vals), 0, "Level of oplock granted", HFILL }},

	{ &hf_smb_create_action,
		{ "Create action", "smb.create.action", FT_UINT32, BASE_DEC,
		VALS(oa_open_vals), 0, "Type of action taken", HFILL }},

	{ &hf_smb_file_id,
		{ "Server unique file ID", "smb.create.file_id", FT_UINT32, BASE_HEX,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_file_id_64bit,
		{ "Server unique 64-bit file ID", "smb.create.file_id_64b", FT_UINT64, BASE_HEX,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_ea_error_offset,
		{ "EA Error offset", "smb.ea.error_offset", FT_UINT32, BASE_DEC,
		NULL, 0, "Offset into EA list if EA error", HFILL }},

	{ &hf_smb_end_of_file,
		{ "End Of File", "smb.end_of_file", FT_UINT64, BASE_DEC,
		NULL, 0, "Offset to the first free byte in the file", HFILL }},

	{ &hf_smb_replace,
		{ "Replace", "smb.replace", FT_BOOLEAN, BASE_NONE,
		TFS(&tfs_smb_replace), 0x0, "Remove target if it exists?", HFILL }},

	{ &hf_smb_root_dir_handle,
		{ "Root Directory Handle", "smb.root_dir_handle", FT_UINT32, BASE_HEX,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_target_name_len,
		{ "Target name length", "smb.target_name_len", FT_UINT32, BASE_DEC,
		NULL, 0, "Length of target file name", HFILL }},

	{ &hf_smb_target_name,
		{ "Target name", "smb.target_name", FT_STRING, BASE_NONE,
		NULL, 0, "Target file name", HFILL }},

	{ &hf_smb_device_type,
		{ "Device Type", "smb.device.type", FT_UINT32, BASE_HEX | BASE_EXT_STRING,
		&device_type_vals_ext, 0, "Type of device", HFILL }},

	{ &hf_smb_is_directory,
		{ "Is Directory", "smb.is_directory", FT_UINT8, BASE_DEC,
		VALS(is_directory_vals), 0, "Is this object a directory?", HFILL }},

	{ &hf_smb_next_entry_offset,
		{ "Next Entry Offset", "smb.next_entry_offset", FT_UINT32, BASE_DEC,
		NULL, 0, "Offset to next entry", HFILL }},

	{ &hf_smb_change_time,
		{ "Change", "smb.change.time", FT_ABSOLUTE_TIME, ABSOLUTE_TIME_LOCAL,
		NULL, 0, "Last Change Time", HFILL }},

	{ &hf_smb_setup_len,
		{ "Setup Len", "smb.print.setup.len", FT_UINT16, BASE_DEC,
		NULL, 0, "Length of printer setup data", HFILL }},

	{ &hf_smb_print_mode,
		{ "Mode", "smb.print.mode", FT_UINT16, BASE_DEC,
		VALS(print_mode_vals), 0, "Text or Graphics mode", HFILL }},

	{ &hf_smb_print_identifier,
		{ "Identifier", "smb.print.identifier", FT_STRING, BASE_NONE,
		NULL, 0, "Identifier string for this print job", HFILL }},

	{ &hf_smb_restart_index,
		{ "Restart Index", "smb.print.restart_index", FT_UINT16, BASE_DEC,
		NULL, 0, "Index of entry after last returned", HFILL }},

	{ &hf_smb_print_queue_date,
		{ "Queued", "smb.print.queued.date", FT_ABSOLUTE_TIME, ABSOLUTE_TIME_LOCAL,
		NULL, 0, "Date when this entry was queued", HFILL }},

	{ &hf_smb_print_queue_dos_date,
		{ "Queued Date", "smb.print.queued.smb.date", FT_UINT16, BASE_HEX,
		NULL, 0, "Date when this print job was queued, SMB_DATE format", HFILL }},

	{ &hf_smb_print_queue_dos_time,
		{ "Queued Time", "smb.print.queued.smb.time", FT_UINT16, BASE_HEX,
		NULL, 0, "Time when this print job was queued, SMB_TIME format", HFILL }},

	{ &hf_smb_print_status,
		{ "Status", "smb.print.status", FT_UINT8, BASE_HEX,
		VALS(print_status_vals), 0, "Status of this entry", HFILL }},

	{ &hf_smb_print_spool_file_number,
		{ "Spool File Number", "smb.print.spool.file_number", FT_UINT16, BASE_DEC,
		NULL, 0, "Spool File Number, assigned by the spooler", HFILL }},

	{ &hf_smb_print_spool_file_size,
		{ "Spool File Size", "smb.print.spool.file_size", FT_UINT32, BASE_DEC,
		NULL, 0, "Number of bytes in spool file", HFILL }},

	{ &hf_smb_print_spool_file_name,
		{ "Name", "smb.print.spool.name", FT_STRINGZ, BASE_NONE,
		NULL, 0, "Name of client that submitted this job", HFILL }},

	{ &hf_smb_start_index,
		{ "Start Index", "smb.print.start_index", FT_UINT16, BASE_DEC,
		NULL, 0, "First queue entry to return", HFILL }},

	{ &hf_smb_originator_name,
		{ "Originator Name", "smb.originator_name", FT_STRINGZ, BASE_NONE,
		NULL, 0, "Name of sender of message", HFILL }},

	{ &hf_smb_destination_name,
		{ "Destination Name", "smb.destination_name", FT_STRINGZ, BASE_NONE,
		NULL, 0, "Name of recipient of message", HFILL }},

	{ &hf_smb_message_len,
		{ "Message Len", "smb.message.len", FT_UINT16, BASE_DEC,
		NULL, 0, "Length of message", HFILL }},

	{ &hf_smb_message,
		{ "Message", "smb.message", FT_STRING, BASE_NONE,
		NULL, 0, "Message text", HFILL }},

	{ &hf_smb_mgid,
		{ "Message Group ID", "smb.mgid", FT_UINT16, BASE_DEC,
		NULL, 0, "Message group ID for multi-block messages", HFILL }},

	{ &hf_smb_forwarded_name,
		{ "Forwarded Name", "smb.forwarded_name", FT_STRINGZ, BASE_NONE,
		NULL, 0, "Recipient name being forwarded", HFILL }},

	{ &hf_smb_machine_name,
		{ "Machine Name", "smb.machine_name", FT_STRINGZ, BASE_NONE,
		NULL, 0, "Name of target machine", HFILL }},

	{ &hf_smb_cancel_to,
		{ "Cancel to", "smb.cancel_to", FT_FRAMENUM, BASE_NONE,
		NULL, 0, "This packet is a cancellation of the packet in this frame", HFILL }},

	{ &hf_smb_trans_name,
		{ "Transaction Name", "smb.trans_name", FT_STRING, BASE_NONE,
		NULL, 0, "Name of transaction", HFILL }},

	{ &hf_smb_transaction_flags,
		{ "Flags", "smb.transaction.flags", FT_UINT16, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_transaction_flags_dtid,
		{ "Disconnect TID", "smb.transaction.flags.dtid", FT_BOOLEAN, 16,
		TFS(&tfs_tf_dtid), 0x0001, "Disconnect TID?", HFILL }},

	{ &hf_smb_transaction_flags_owt,
		{ "One Way Transaction", "smb.transaction.flags.owt", FT_BOOLEAN, 16,
		TFS(&tfs_tf_owt), 0x0002, "One Way Transaction (no response)?", HFILL }},

	{ &hf_smb_search_count,
		{ "Search Count", "smb.search_count", FT_UINT16, BASE_DEC,
		NULL, 0, "Maximum number of search entries to return", HFILL }},

	{ &hf_smb_search_pattern,
		{ "Search Pattern", "smb.search_pattern", FT_STRING, BASE_NONE,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_ff2,
		{ "Flags", "smb.find_first2.flags", FT_UINT16, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_ff2_backup,
		{ "Backup Intent", "smb.find_first2.flags.backup", FT_BOOLEAN, 16,
		TFS(&tfs_ff2_backup), 0x0010, "Find with backup intent", HFILL }},

	{ &hf_smb_ff2_continue,
		{ "Continue", "smb.find_first2.flags.continue", FT_BOOLEAN, 16,
		TFS(&tfs_ff2_continue), 0x0008, "Continue search from previous ending place", HFILL }},

	{ &hf_smb_ff2_resume,
		{ "Resume", "smb.find_first2.flags.resume", FT_BOOLEAN, 16,
		TFS(&tfs_ff2_resume), FF2_RESUME, "Return resume keys for each entry found", HFILL }},

	{ &hf_smb_ff2_close_eos,
		{ "Close on EOS", "smb.find_first2.flags.eos", FT_BOOLEAN, 16,
		TFS(&tfs_ff2_close_eos), 0x0002, "Close search if end of search reached", HFILL }},

	{ &hf_smb_ff2_close,
		{ "Close", "smb.find_first2.flags.close", FT_BOOLEAN, 16,
		TFS(&tfs_ff2_close), 0x0001, "Close search after this request", HFILL }},

	{ &hf_smb_ff2_information_level,
		{ "Level of Interest", "smb.ff2_loi", FT_UINT16, BASE_DEC,
		VALS(ff2_il_vals), 0, "Level of interest for FIND_FIRST2 command", HFILL }},

	{ &hf_smb_qpi_loi,
		{ "Level of Interest", "smb.qpi_loi", FT_UINT16, BASE_DEC,
		VALS(qpi_loi_vals), 0, "Level of interest for TRANSACTION[2] QUERY_{FILE,PATH}_INFO commands", HFILL }},

	{ &hf_smb_spi_loi,
		{ "Level of Interest", "smb.spi_loi", FT_UINT16, BASE_DEC | BASE_EXT_STRING,
		&spi_loi_vals_ext, 0, "Level of interest for TRANSACTION[2] SET_{FILE,PATH}_INFO commands", HFILL }},

#if 0
	{ &hf_smb_sfi,
		{ "IO Flag", "smb.sfi_flags", FT_UINT16, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_sfi_writetru,
		{ "Writethrough", "smb.sfi_writethrough", FT_BOOLEAN, 16,
		TFS(&tfs_da_writetru), 0x0010, "Writethrough mode?", HFILL }},

	{ &hf_smb_sfi_caching,
		{ "Caching", "smb.sfi_caching", FT_BOOLEAN, 16,
		TFS(&tfs_da_caching), 0x0020, "Caching mode?", HFILL }},
#endif

	{ &hf_smb_storage_type,
		{ "Storage Type", "smb.storage_type", FT_UINT32, BASE_DEC,
		NULL, 0, "Type of storage", HFILL }},

	{ &hf_smb_resume,
		{ "Resume Key", "smb.resume", FT_UINT32, BASE_DEC,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_max_referral_level,
		{ "Max Referral Level", "smb.max_referral_level", FT_UINT16, BASE_DEC,
		NULL, 0, "Latest referral version number understood", HFILL }},

	{ &hf_smb_qfsi_information_level,
		{ "Level of Interest", "smb.qfsi_loi", FT_UINT16, BASE_HEX | BASE_EXT_STRING,
		&qfsi_vals_ext, 0, "Level of interest for QUERY_FS_INFORMATION2 command", HFILL }},

	{ &hf_smb_sfsi_information_level,
		{ "Level of Interest", "smb.sfsi_loi", FT_UINT16, BASE_HEX,
		VALS(sfsi_vals), 0, "Level of interest for SET_FS_INFORMATION2 command", HFILL }},

	{ &hf_smb_nt_rename_level,
		{ "Level of Interest", "smb.ntr_loi", FT_UINT16, BASE_DEC,
		VALS(nt_rename_vals), 0, "NT Rename level", HFILL }},

	{ &hf_smb_cluster_count,
		{ "Cluster count", "smb.ntr_clu", FT_UINT32, BASE_DEC,
		NULL, 0, "Number of clusters", HFILL }},

	{ &hf_smb_number_of_links,
		{ "Link Count", "smb.link_count", FT_UINT32, BASE_DEC,
		NULL, 0, "Number of hard links to the file", HFILL }},

	{ &hf_smb_delete_pending,
		{ "Delete Pending", "smb.delete_pending", FT_UINT16, BASE_DEC,
		VALS(delete_pending_vals), 0, "Is this object about to be deleted?", HFILL }},

	{ &hf_smb_index_number,
		{ "Index Number", "smb.index_number", FT_UINT64, BASE_HEX,
		NULL, 0, "File system unique identifier", HFILL }},

	{ &hf_smb_position,
		{ "Position", "smb.position", FT_UINT64, BASE_DEC,
		NULL, 0, "File position", HFILL }},

#if 0
	{ &hf_smb_current_offset,
		{ "Current Offset", "smb.offset", FT_UINT64, BASE_DEC,
		NULL, 0, "Current offset in the file", HFILL }},
#endif

	{ &hf_smb_t2_alignment,
		{ "Alignment", "smb.alignment", FT_UINT32, BASE_DEC,
		VALS(alignment_vals), 0, "What alignment do we require for buffers", HFILL }},

	{ &hf_smb_t2_stream_name_length,
		{ "Stream Name Length", "smb.stream_name_len", FT_UINT32, BASE_DEC,
		NULL, 0, "Length of stream name", HFILL }},

	{ &hf_smb_t2_stream_size,
		{ "Stream Size", "smb.stream_size", FT_UINT64, BASE_DEC,
		NULL, 0, "Size of the stream in number of bytes", HFILL }},

	{ &hf_smb_t2_stream_name,
		{ "Stream Name", "smb.stream_name", FT_STRING, BASE_NONE,
		NULL, 0, "Name of the stream", HFILL }},

	{ &hf_smb_t2_compressed_file_size,
		{ "Compressed Size", "smb.compressed.file_size", FT_UINT64, BASE_DEC,
		NULL, 0, "Size of the compressed file", HFILL }},

	{ &hf_smb_t2_compressed_format,
		{ "Compression Format", "smb.compressed.format", FT_UINT16, BASE_DEC,
		NULL, 0, "Compression algorithm used", HFILL }},

	{ &hf_smb_t2_compressed_unit_shift,
		{ "Unit Shift", "smb.compressed.unit_shift", FT_UINT8, BASE_DEC,
		NULL, 0, "Size of the stream in number of bytes", HFILL }},

	{ &hf_smb_t2_compressed_chunk_shift,
		{ "Chunk Shift", "smb.compressed.chunk_shift", FT_UINT8, BASE_DEC,
		NULL, 0, "Allocated size of the stream in number of bytes", HFILL }},

	{ &hf_smb_t2_compressed_cluster_shift,
		{ "Cluster Shift", "smb.compressed.cluster_shift", FT_UINT8, BASE_DEC,
		NULL, 0, "Allocated size of the stream in number of bytes", HFILL }},

	{ &hf_smb_t2_marked_for_deletion,
		{ "Marked for Deletion", "smb.marked_for_deletion", FT_BOOLEAN, BASE_NONE,
		TFS(&tfs_marked_for_deletion), 0x0, "Marked for deletion?", HFILL }},

	{ &hf_smb_dfs_path_consumed,
		{ "Path Consumed", "smb.dfs.path_consumed", FT_UINT16, BASE_DEC,
		NULL, 0, "Number of RequestFilename bytes client", HFILL }},

	{ &hf_smb_dfs_num_referrals,
		{ "Num Referrals", "smb.dfs.num_referrals", FT_UINT16, BASE_DEC,
		NULL, 0, "Number of referrals in this pdu", HFILL }},

	{ &hf_smb_get_dfs_flags,
		{ "Flags", "smb.dfs.flags", FT_UINT16, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_get_dfs_server_hold_storage,
		{ "Hold Storage", "smb.dfs.flags.server_hold_storage", FT_BOOLEAN, 16,
		TFS(&tfs_get_dfs_server_hold_storage), 0x02, "The servers in referrals should hold storage for the file", HFILL }},

	{ &hf_smb_get_dfs_fielding,
		{ "Fielding", "smb.dfs.flags.fielding", FT_BOOLEAN, 16,
		TFS(&tfs_get_dfs_fielding), 0x01, "The servers in referrals are capable of fielding", HFILL }},

	{ &hf_smb_dfs_referral_version,
		{ "Version", "smb.dfs.referral.version", FT_UINT16, BASE_DEC,
		NULL, 0, "Version of referral element", HFILL }},

	{ &hf_smb_dfs_referral_size,
		{ "Size", "smb.dfs.referral.size", FT_UINT16, BASE_DEC,
		NULL, 0, "Size of referral element", HFILL }},

	{ &hf_smb_dfs_referral_server_type,
		{ "Server Type", "smb.dfs.referral.server.type", FT_UINT16, BASE_DEC,
		VALS(dfs_referral_server_type_vals), 0, "Type of referral server", HFILL }},

	{ &hf_smb_dfs_referral_flags,
		{ "Flags", "smb.dfs.referral.flags", FT_UINT16, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_dfs_referral_flags_name_list_referral,
		{ "NameListReferral", "smb.dfs.referral.flags.name_list_referral", FT_BOOLEAN, 16,
		TFS(&tfs_dfs_referral_flags_name_list_referral), REFENT_FLAGS_NAME_LIST_REFERRAL, "Is a domain/DC referral response?", HFILL }},

	{ &hf_smb_dfs_referral_flags_target_set_boundary,
		{ "TargetSetBoundary", "smb.dfs.referral.flags.target_set_boundary", FT_BOOLEAN, 16,
		TFS(&tfs_dfs_referral_flags_target_set_boundary), REFENT_FLAGS_TARGET_SET_BOUNDARY, "Is this a first target in the target set?", HFILL }},

	{ &hf_smb_dfs_referral_node_offset,
		{ "Node Offset", "smb.dfs.referral.node_offset", FT_UINT16, BASE_DEC,
		NULL, 0, "Offset of name of entity to visit next", HFILL }},

	{ &hf_smb_dfs_referral_node,
		{ "Node", "smb.dfs.referral.node", FT_STRING, BASE_NONE,
		NULL, 0, "Name of entity to visit next", HFILL }},

	{ &hf_smb_dfs_referral_proximity,
		{ "Proximity", "smb.dfs.referral.proximity", FT_UINT32, BASE_DEC,
		NULL, 0, "Hint describing proximity of this server to the client", HFILL }},

	{ &hf_smb_dfs_referral_ttl,
		{ "TTL", "smb.dfs.referral.ttl", FT_UINT32, BASE_DEC,
		NULL, 0, "Number of seconds the client can cache this referral", HFILL }},

	{ &hf_smb_dfs_referral_path_offset,
		{ "Path Offset", "smb.dfs.referral.path_offset", FT_UINT16, BASE_DEC,
		NULL, 0, "Offset of Dfs Path that matched pathconsumed", HFILL }},

	{ &hf_smb_dfs_referral_path,
		{ "Path", "smb.dfs.referral.path", FT_STRING, BASE_NONE,
		NULL, 0, "Dfs Path that matched pathconsumed", HFILL }},

	{ &hf_smb_dfs_referral_alt_path_offset,
		{ "Alt Path Offset", "smb.dfs.referral.alt_path_offset", FT_UINT16, BASE_DEC,
		NULL, 0, "Offset of alternative(8.3) Path that matched pathconsumed", HFILL }},

	{ &hf_smb_dfs_referral_alt_path,
		{ "Alt Path", "smb.dfs.referral.alt_path", FT_STRING, BASE_NONE,
		NULL, 0, "Alternative(8.3) Path that matched pathconsumed", HFILL }},

	{ &hf_smb_dfs_referral_domain_offset,
		{ "Domain Offset", "smb.dfs.referral.domain_offset", FT_UINT16, BASE_DEC,
		NULL, 0, "Offset of Dfs Path that matched pathconsumed", HFILL }},

	{ &hf_smb_dfs_referral_number_of_expnames,
		{ "Number of Expanded Names", "smb.dfs.referral.number_of_expnames", FT_UINT16, BASE_DEC,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_dfs_referral_expnames_offset,
		{ "Expanded Names Offset", "smb.dfs.referral.expnames_offset", FT_UINT16, BASE_DEC,
		NULL, 0, "Offset of Dfs Path that matched pathconsumed", HFILL }},

	{ &hf_smb_dfs_referral_domain_name,
		{ "Domain Name", "smb.dfs.referral.domain_name", FT_STRING, BASE_NONE,
		NULL, 0, "Dfs referral domain name", HFILL }},

	{ &hf_smb_dfs_referral_expname,
		{ "Expanded Name", "smb.dfs.referral.expname", FT_STRING, BASE_NONE,
		NULL, 0, "Dfs expanded name", HFILL }},

	{ &hf_smb_dfs_referral_server_guid,
		{ "Server GUID", "smb.dfs.referral.server_guid", FT_GUID, BASE_NONE,
		NULL, 0, "Globally unique identifier for this server", HFILL }},

	{ &hf_smb_end_of_search,
		{ "End Of Search", "smb.end_of_search", FT_UINT16, BASE_DEC,
		NULL, 0, "Was last entry returned?", HFILL }},

	{ &hf_smb_last_name_offset,
		{ "Last Name Offset", "smb.last_name_offset", FT_UINT16, BASE_DEC,
		NULL, 0, "If non-0 this is the offset into the datablock for the file name of the last entry", HFILL }},

	{ &hf_smb_fn_information_level,
		{ "Level of Interest", "smb.fn_loi", FT_UINT16, BASE_DEC,
		NULL, 0, "Level of interest for FIND_NOTIFY command", HFILL }},

	{ &hf_smb_monitor_handle,
		{ "Monitor Handle", "smb.monitor_handle", FT_UINT16, BASE_HEX,
		NULL, 0, "Handle for Find Notify operations", HFILL }},

	{ &hf_smb_change_count,
		{ "Change Count", "smb.change_count", FT_UINT16, BASE_DEC,
		NULL, 0, "Number of changes to wait for", HFILL }},

	{ &hf_smb_file_index,
		{ "File Index", "smb.file_index", FT_UINT32, BASE_DEC,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_short_file_name,
		{ "Short File Name", "smb.short_file", FT_STRING, BASE_NONE,
		NULL, 0, "Short (8.3) File Name", HFILL }},

	{ &hf_smb_short_file_name_len,
		{ "Short File Name Len", "smb.short_file_name_len", FT_UINT32, BASE_DEC,
		NULL, 0, "Length of Short (8.3) File Name", HFILL }},

	{ &hf_smb_fs_id,
		{ "FS Id", "smb.fs_id", FT_UINT32, BASE_DEC,
		NULL, 0, "File System ID (NT Server always returns 0)", HFILL }},

	{ &hf_smb_sector_unit,
		{ "Sectors/Unit", "smb.fs_sector_per_unit", FT_UINT32, BASE_DEC,
		NULL, 0, "Sectors per allocation unit", HFILL }},

	{ &hf_smb_fs_units,
		{ "Total Units", "smb.fs_units", FT_UINT32, BASE_DEC,
		NULL, 0, "Total number of units on this filesystem", HFILL }},

	{ &hf_smb_fs_sector,
		{ "Bytes per Sector", "smb.fs_bytes_per_sector", FT_UINT32, BASE_DEC,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_avail_units,
		{ "Available Units", "smb.avail.units", FT_UINT32, BASE_DEC,
		NULL, 0, "Total number of available units on this filesystem", HFILL }},

	{ &hf_smb_volume_serial_num,
		{ "Volume Serial Number", "smb.volume.serial", FT_UINT32, BASE_HEX,
		NULL, 0, NULL, HFILL }},

	{ &hf_smb_volume_label_len,
		{ "Label Length", "smb.volume.label.len", FT_UINT32, BASE_DEC,
		NULL, 0, "Length of volume label", HFILL }},

	{ &hf_smb_volume_label,
		{ "Label", "smb.volume.label", FT_STRING, BASE_NONE,
		NULL, 0, "Volume label", HFILL }},

	{ &hf_smb_free_alloc_units64,
		{ "Free Units", "smb.free_alloc_units", FT_UINT64, BASE_DEC,
		NULL, 0, "Number of free allocation units", HFILL }},

	{ &hf_smb_caller_free_alloc_units64,
		{ "Caller Free Units", "smb.caller_free_alloc_units", FT_UINT64, BASE_DEC,
		NULL, 0, "Number of caller free allocation units", HFILL }},

	{ &hf_smb_actual_free_alloc_units64,
		{ "Actual Free Units", "smb.actual_free_alloc_units", FT_UINT64, BASE_DEC,
		NULL, 0, "Number of actual free allocation units", HFILL }},

	{ &hf_smb_soft_quota_limit,
		{ "(Soft) Quota Threshold", "smb.quota.soft.default", FT_UINT64, BASE_DEC,
		NULL, 0, "Soft Quota threshold", HFILL }},

	{ &hf_smb_hard_quota_limit,
		{ "(Hard) Quota Limit", "smb.quota.hard.default", FT_UINT64, BASE_DEC,
		NULL, 0, "Hard Quota limit", HFILL }},

	{ &hf_smb_user_quota_used,
		{ "Quota Used", "smb.quota.used", FT_UINT64, BASE_DEC,
		NULL, 0, "How much Quota is used by this user", HFILL }},

	{ &hf_smb_max_name_len,
		{ "Max name length", "smb.fs_max_name_len", FT_UINT32, BASE_DEC,
		NULL, 0, "Maximum length of each file name component in number of bytes", HFILL }},

	{ &hf_smb_fs_name_len,
		{ "Label Length", "smb.fs_name.len", FT_UINT32, BASE_DEC,
		NULL, 0, "Length of filesystem name in bytes", HFILL }},

	{ &hf_smb_fs_name,
		{ "FS Name", "smb.fs_name", FT_STRING, BASE_NONE,
		NULL, 0, "Name of filesystem", HFILL }},

	{ &hf_smb_device_char,
		{ "Device Characteristics", "smb.device", FT_UINT32, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_device_char_removable,
		{ "Removable", "smb.device.removable", FT_BOOLEAN, 32,
		TFS(&tfs_device_char_removable), 0x00000001, "Is this a removable device", HFILL }},

	{ &hf_smb_device_char_read_only,
		{ "Read Only", "smb.device.read_only", FT_BOOLEAN, 32,
		TFS(&tfs_device_char_read_only), 0x00000002, "Is this a read-only device", HFILL }},

	{ &hf_smb_device_char_floppy,
		{ "Floppy", "smb.device.floppy", FT_BOOLEAN, 32,
		TFS(&tfs_device_char_floppy), 0x00000004, "Is this a floppy disk", HFILL }},

	{ &hf_smb_device_char_write_once,
		{ "Write Once", "smb.device.write_once", FT_BOOLEAN, 32,
		TFS(&tfs_device_char_write_once), 0x00000008, "Is this a write-once device", HFILL }},

	{ &hf_smb_device_char_remote,
		{ "Remote", "smb.device.remote", FT_BOOLEAN, 32,
		TFS(&tfs_device_char_remote), 0x00000010, "Is this a remote device", HFILL }},

	{ &hf_smb_device_char_mounted,
		{ "Mounted", "smb.device.mounted", FT_BOOLEAN, 32,
		TFS(&tfs_device_char_mounted), 0x00000020, "Is this a mounted device", HFILL }},

	{ &hf_smb_device_char_virtual,
		{ "Virtual", "smb.device.virtual", FT_BOOLEAN, 32,
		TFS(&tfs_device_char_virtual), 0x00000040, "Is this a virtual device", HFILL }},

	{ &hf_smb_fs_attr,
		{ "FS Attributes", "smb.fs_attr", FT_UINT32, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_fs_attr_css,
		{ "Case Sensitive Search", "smb.fs_attr.css", FT_BOOLEAN, 32,
		TFS(&tfs_fs_attr_css), 0x00000001, "Does this FS support Case Sensitive Search?", HFILL }},

	{ &hf_smb_fs_attr_cpn,
		{ "Case Preserving", "smb.fs_attr.cpn", FT_BOOLEAN, 32,
		TFS(&tfs_fs_attr_cpn), 0x00000002, "Will this FS Preserve Name Case?", HFILL }},

	{ &hf_smb_fs_attr_uod,
		{ "Unicode On Disk", "smb.fs_attr.uod", FT_BOOLEAN, 32,
		TFS(&tfs_fs_attr_uod), 0x00000004, "Does this FS support Unicode On Disk?", HFILL }},

	{ &hf_smb_fs_attr_pacls,
		{ "Persistent ACLs", "smb.fs_attr.pacls", FT_BOOLEAN, 32,
		TFS(&tfs_fs_attr_pacls), 0x00000008, "Does this FS support Persistent ACLs?", HFILL }},

	{ &hf_smb_fs_attr_fc,
		{ "Compression", "smb.fs_attr.fc", FT_BOOLEAN, 32,
		TFS(&tfs_fs_attr_fc), 0x00000010, "Does this FS support File Compression?", HFILL }},

	{ &hf_smb_fs_attr_vq,
		{ "Volume Quotas", "smb.fs_attr.vq", FT_BOOLEAN, 32,
		TFS(&tfs_fs_attr_vq), 0x00000020, "Does this FS support Volume Quotas?", HFILL }},

	{ &hf_smb_fs_attr_ssf,
		{ "Sparse Files", "smb.fs_attr.ssf", FT_BOOLEAN, 32,
		TFS(&tfs_fs_attr_ssf), 0x00000040, "Does this FS support SPARSE FILES?", HFILL }},

	{ &hf_smb_fs_attr_srp,
		{ "Reparse Points", "smb.fs_attr.srp", FT_BOOLEAN, 32,
		TFS(&tfs_fs_attr_srp), 0x00000080, "Does this FS support REPARSE POINTS?", HFILL }},

	{ &hf_smb_fs_attr_srs,
		{ "Remote Storage", "smb.fs_attr.srs", FT_BOOLEAN, 32,
		TFS(&tfs_fs_attr_srs), 0x00000100, "Does this FS support REMOTE STORAGE?", HFILL }},

	{ &hf_smb_fs_attr_sla,
		{ "LFN APIs", "smb.fs_attr.sla", FT_BOOLEAN, 32,
		TFS(&tfs_fs_attr_sla), 0x00004000, "Does this FS support LFN APIs?", HFILL }},

	{ &hf_smb_fs_attr_vic,
		{ "Volume Is Compressed", "smb.fs_attr.vis", FT_BOOLEAN, 32,
		TFS(&tfs_fs_attr_vic), 0x00008000, "Is this FS on a compressed volume?", HFILL }},

	{ &hf_smb_fs_attr_soids,
		{ "Supports OIDs", "smb.fs_attr.soids", FT_BOOLEAN, 32,
		TFS(&tfs_fs_attr_soids), 0x00010000, "Does this FS support OIDs?", HFILL }},

	{ &hf_smb_fs_attr_se,
		{ "Supports Encryption", "smb.fs_attr.se", FT_BOOLEAN, 32,
		TFS(&tfs_fs_attr_se), 0x00020000, "Does this FS support encryption?", HFILL }},

	{ &hf_smb_fs_attr_ns,
		{ "Named Streams", "smb.fs_attr.ns", FT_BOOLEAN, 32,
		TFS(&tfs_fs_attr_ns), 0x00040000, "Does this FS support named streams?", HFILL }},

	{ &hf_smb_fs_attr_rov,
		{ "Read Only Volume", "smb.fs_attr.rov", FT_BOOLEAN, 32,
		TFS(&tfs_fs_attr_rov), 0x00080000, "Is this FS on a read only volume?", HFILL }},

	{ &hf_smb_length_of_sid,
		{ "Length of SID", "smb.length_of_sid", FT_UINT32, BASE_DEC,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_user_quota_offset,
		{ "Next Offset", "smb.quota.user.offset", FT_UINT32, BASE_DEC,
		NULL, 0, "Relative offset to next user quota structure", HFILL }},

	{ &hf_smb_pipe_write_len,
		{ "Pipe Write Len", "smb.pipe.write_len", FT_UINT16, BASE_DEC,
		NULL, 0, "Number of bytes written to pipe", HFILL }},

	{ &hf_smb_quota_flags_deny_disk,
		{ "Deny Disk", "smb.quota.flags.deny_disk", FT_BOOLEAN, 8,
		TFS(&tfs_quota_flags_deny_disk), 0x02, "Is the default quota limit enforced?", HFILL }},

	{ &hf_smb_quota_flags_log_limit,
		{ "Log Limit", "smb.quota.flags.log_limit", FT_BOOLEAN, 8,
		TFS(&tfs_quota_flags_log_limit), 0x20, "Should the server log an event when the limit is exceeded?", HFILL }},

	{ &hf_smb_quota_flags_log_warning,
		{ "Log Warning", "smb.quota.flags.log_warning", FT_BOOLEAN, 8,
		TFS(&tfs_quota_flags_log_warning), 0x10, "Should the server log an event when the warning level is exceeded?", HFILL }},

	{ &hf_smb_quota_flags,
		{ "Quota Flags", "smb.quota.flags", FT_UINT8, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_quota_flags_enabled,
		{ "Enabled", "smb.quota.flags.enabled", FT_BOOLEAN, 8,
		TFS(&tfs_quota_flags_enabled), 0x01, "Is quotas enabled of this FS?", HFILL }},

	{ &hf_smb_segment_overlap,
		{ "Fragment overlap",	"smb.segment.overlap", FT_BOOLEAN, BASE_NONE, NULL, 0x0,
			"Fragment overlaps with other fragments", HFILL }},

	{ &hf_smb_segment_overlap_conflict,
		{ "Conflicting data in fragment overlap",	"smb.segment.overlap.conflict", FT_BOOLEAN, BASE_NONE, NULL, 0x0,
			"Overlapping fragments contained conflicting data", HFILL }},

	{ &hf_smb_segment_multiple_tails,
		{ "Multiple tail fragments found",	"smb.segment.multipletails", FT_BOOLEAN, BASE_NONE, NULL, 0x0,
			"Several tails were found when defragmenting the packet", HFILL }},

	{ &hf_smb_segment_too_long_fragment,
		{ "Fragment too long",	"smb.segment.toolongfragment", FT_BOOLEAN, BASE_NONE, NULL, 0x0,
			"Fragment contained data past end of packet", HFILL }},

	{ &hf_smb_segment_error,
		{ "Defragmentation error", "smb.segment.error", FT_FRAMENUM, BASE_NONE, NULL, 0x0,
			"Defragmentation error due to illegal fragments", HFILL }},

	{ &hf_smb_segment_count,
		{ "Fragment count", "smb.fragment.count", FT_UINT32, BASE_DEC, NULL, 0x0,
			NULL, HFILL }},

	{ &hf_smb_reassembled_length,
		{ "Reassembled SMB length", "smb.reassembled.length", FT_UINT32, BASE_DEC, NULL, 0x0,
			"The total length of the reassembled payload", HFILL }},

	{ &hf_smb_opened_in,
		{ "Opened in", "smb.fid.opened_in", FT_FRAMENUM, BASE_NONE, NULL, 0x0,
			"The frame this fid was opened", HFILL }},

	{ &hf_smb_closed_in,
		{ "Closed in", "smb.fid.closed_in", FT_FRAMENUM, BASE_NONE, NULL, 0x0,
			"The frame this fid was closed", HFILL }},

	{ &hf_smb_mapped_in,
		{ "Mapped in", "smb.fid.mapped_in", FT_FRAMENUM, BASE_NONE, NULL, 0x0,
			"The frame this share was mapped", HFILL }},

	{ &hf_smb_unmapped_in,
		{ "Unmapped in", "smb.fid.unmapped_in", FT_FRAMENUM, BASE_NONE, NULL, 0x0,
			"The frame this share was unmapped", HFILL }},

	{ &hf_smb_segment,
		{ "SMB Segment", "smb.segment", FT_FRAMENUM, BASE_NONE, NULL, 0x0,
			NULL, HFILL }},

	{ &hf_smb_segments,
		{ "SMB Segments", "smb.segment.segments", FT_NONE, BASE_NONE, NULL, 0x0,
			NULL, HFILL }},

	{ &hf_smb_unix_major_version,
	  { "Major Version", "smb.unix.major_version", FT_UINT16, BASE_DEC,
	    NULL, 0, "UNIX Major Version", HFILL }},

	{ &hf_smb_unix_minor_version,
	  { "Minor Version", "smb.unix.minor_version", FT_UINT16, BASE_DEC,
	    NULL, 0, "UNIX Minor Version", HFILL }},

	{ &hf_smb_unix_capability,
		{ "Capabilities", "smb.unix.capability", FT_UINT64, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_unix_capability_fcntl,
	  { "FCNTL Capability", "smb.unix.capability.fcntl", FT_BOOLEAN, 32,
		TFS(&tfs_set_notset), 0x00000001, NULL, HFILL }},

	{ &hf_smb_unix_capability_posix_acl,
	  { "POSIX ACL Capability", "smb.unix.capability.posix_acl", FT_BOOLEAN, 32,
		TFS(&tfs_set_notset), 0x00000002, NULL, HFILL }},

	{ &hf_smb_unix_capability_xattr,
	  { "EA Capability", "smb.unix.capability.ea", FT_BOOLEAN, 32,
		TFS(&tfs_set_notset), 0x00000004, NULL, HFILL }},

	{ &hf_smb_unix_capability_attr,
	  { "Additional Attributes Capability", "smb.unix.capability.attr", FT_BOOLEAN, 32,
		TFS(&tfs_set_notset), 0x00000008, NULL, HFILL }},

	{ &hf_smb_unix_capability_posix_paths,
	  { "POSIX Pathnames Capability", "smb.unix.capability.posix_paths", FT_BOOLEAN, 32,
		TFS(&tfs_set_notset), 0x00000010, NULL, HFILL }},

	{ &hf_smb_unix_capability_posix_path_ops,
	  { "POSIX Path Operations Capability", "smb.unix.capability.posix_path_ops", FT_BOOLEAN, 32,
		TFS(&tfs_set_notset), 0x00000020, NULL, HFILL }},

	{ &hf_smb_unix_capability_large_read,
	  { "Large Read Capability", "smb.unix.capability.large_read", FT_BOOLEAN, 32,
		TFS(&tfs_set_notset), 0x00000040, NULL, HFILL }},

	{ &hf_smb_unix_capability_large_write,
	  { "Large Write Capability", "smb.unix.capability.large_write", FT_BOOLEAN, 32,
		TFS(&tfs_set_notset), 0x00000080, NULL, HFILL }},

	{ &hf_smb_unix_capability_encrpytion,
	  { "Encryption Capability", "smb.unix.capability.encrpytion", FT_BOOLEAN, 32,
		TFS(&tfs_set_notset), 0x00000100, NULL, HFILL }},

	{ &hf_smb_unix_capability_mandatory_crypto,
	  { "Mandatory Encryption Capability", "smb.unix.capability.mandatory_crypto", FT_BOOLEAN, 32,
		TFS(&tfs_set_notset), 0x00000200, NULL, HFILL }},

	{ &hf_smb_unix_capability_proxy,
	  { "Proxy Capability", "smb.unix.capability.proxy", FT_BOOLEAN, 32,
		TFS(&tfs_set_notset), 0x00000400, NULL, HFILL }},

	{ &hf_smb_file_access_mask_read_data,
	  { "Read Data", "smb.file.accessmask.read_data", FT_BOOLEAN, 32,
		TFS(&tfs_set_notset), 0x00000001, NULL, HFILL }},

	{ &hf_smb_file_access_mask_write_data,
	  { "Write Data", "smb.file.accessmask.write_data", FT_BOOLEAN, 32,
		TFS(&tfs_set_notset), 0x00000002, NULL, HFILL }},

	{ &hf_smb_file_access_mask_append_data,
	  { "Append Data", "smb.file.accessmask.append_data", FT_BOOLEAN, 32,
		TFS(&tfs_set_notset), 0x00000004, NULL, HFILL }},

	{ &hf_smb_file_access_mask_read_ea,
	  { "Read EA", "smb.file.accessmask.read_ea", FT_BOOLEAN, 32,
		TFS(&tfs_set_notset), 0x00000008, NULL, HFILL }},

	{ &hf_smb_file_access_mask_write_ea,
	  { "Write EA", "smb.file.accessmask.write_ea", FT_BOOLEAN, 32,
		TFS(&tfs_set_notset), 0x00000010, NULL, HFILL }},

	{ &hf_smb_file_access_mask_execute,
	  { "Execute", "smb.file.accessmask.execute", FT_BOOLEAN, 32,
		TFS(&tfs_set_notset), 0x00000020, NULL, HFILL }},

	{ &hf_smb_file_access_mask_read_attribute,
	  { "Read Attribute", "smb.file.accessmask.read_attribute", FT_BOOLEAN, 32,
		TFS(&tfs_set_notset), 0x00000080, NULL, HFILL }},

	{ &hf_smb_file_access_mask_write_attribute,
	  { "Write Attribute", "smb.file.accessmask.write_attribute", FT_BOOLEAN, 32,
		TFS(&tfs_set_notset), 0x00000100, NULL, HFILL }},

	{ &hf_smb_dir_access_mask_list,
	  { "List", "smb.dir.accessmask.list", FT_BOOLEAN, 32,
		TFS(&tfs_set_notset), 0x00000001, NULL, HFILL }},

	{ &hf_smb_dir_access_mask_add_file,
	  { "Add File", "smb.dir.accessmask.add_file", FT_BOOLEAN, 32,
		TFS(&tfs_set_notset), 0x00000002, NULL, HFILL }},

	{ &hf_smb_dir_access_mask_add_subdir,
	  { "Add Subdir", "smb.dir.accessmask.add_subdir", FT_BOOLEAN, 32,
		TFS(&tfs_set_notset), 0x00000004, NULL, HFILL }},

	{ &hf_smb_dir_access_mask_read_ea,
	  { "Read EA", "smb.dir.accessmask.read_ea", FT_BOOLEAN, 32,
		TFS(&tfs_set_notset), 0x00000008, NULL, HFILL }},

	{ &hf_smb_dir_access_mask_write_ea,
	  { "Write EA", "smb.dir.accessmask.write_ea", FT_BOOLEAN, 32,
		TFS(&tfs_set_notset), 0x00000010, NULL, HFILL }},

	{ &hf_smb_dir_access_mask_traverse,
	  { "Traverse", "smb.dir.accessmask.traverse", FT_BOOLEAN, 32,
		TFS(&tfs_set_notset), 0x00000020, NULL, HFILL }},

	{ &hf_smb_dir_access_mask_delete_child,
	  { "Delete Child", "smb.dir.accessmask.delete_child", FT_BOOLEAN, 32,
		TFS(&tfs_set_notset), 0x00000040, NULL, HFILL }},

	{ &hf_smb_dir_access_mask_read_attribute,
	  { "Read Attribute", "smb.dir.accessmask.read_attribute", FT_BOOLEAN, 32,
		TFS(&tfs_set_notset), 0x00000080, NULL, HFILL }},

	{ &hf_smb_dir_access_mask_write_attribute,
	  { "Write Attribute", "smb.dir.accessmask.write_attribute", FT_BOOLEAN, 32,
		TFS(&tfs_set_notset), 0x00000100, NULL, HFILL }},

	{ &hf_smb_unix_file_link_dest,
	  { "Link destination", "smb.unix.file.link_dest", FT_STRING,
	    BASE_NONE, NULL, 0, NULL, HFILL }},

	{ &hf_smb_unix_file_size,
	  { "File size", "smb.unix.file.size", FT_UINT64, BASE_DEC,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_unix_file_num_bytes,
	  { "Number of bytes", "smb.unix.file.num_bytes", FT_UINT64, BASE_DEC,
	    NULL, 0, "Number of bytes used to store the file", HFILL }},

	{ &hf_smb_unix_file_last_status,
	  { "Last status change", "smb.unix.file.stime", FT_ABSOLUTE_TIME, ABSOLUTE_TIME_LOCAL,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_unix_file_last_access,
	  { "Last access", "smb.unix.file.atime", FT_ABSOLUTE_TIME, ABSOLUTE_TIME_LOCAL,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_unix_file_last_change,
	  { "Last modification", "smb.unix.file.mtime", FT_ABSOLUTE_TIME, ABSOLUTE_TIME_LOCAL,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_unix_file_creation_time,
	  { "Creation time", "smb.unix.file.crtime", FT_ABSOLUTE_TIME, ABSOLUTE_TIME_LOCAL,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_unix_file_uid,
	  { "UID", "smb.unix.file.uid", FT_UINT64, BASE_DEC,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_unix_file_gid,
	  { "GID", "smb.unix.file.gid", FT_UINT64, BASE_DEC,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_unix_file_type,
	  { "File type", "smb.unix.file.file_type", FT_UINT32, BASE_DEC,
	    VALS(unix_file_type_vals), 0, NULL, HFILL }},

	{ &hf_smb_unix_file_dev_major,
	  { "Major device", "smb.unix.file.dev_major", FT_UINT64, BASE_HEX,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_unix_file_dev_minor,
	  { "Minor device", "smb.unix.file.dev_minor", FT_UINT64, BASE_HEX,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_unix_file_unique_id,
	  { "Unique ID", "smb.unix.file.unique_id", FT_UINT64, BASE_HEX,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_unix_file_permissions,
	  { "File permissions", "smb.unix.file.perms", FT_UINT64, BASE_HEX,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_unix_file_nlinks,
	  { "Num links", "smb.unix.file.num_links", FT_UINT64, BASE_DEC,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_unix_info2_file_flags,
	  { "Flags", "smb.unix_info2.file.flags", FT_UINT32, BASE_HEX,
	     NULL, 0, NULL, HFILL }},

	{ &hf_smb_unix_info2_file_flags_mask,
	  { "Flags mask", "smb.unix_info2.file.flags_mask", FT_UINT32, BASE_HEX,
	     NULL, 0, NULL, HFILL }},

	{ &hf_smb_unix_info2_file_flags_secure_delete,
	  { "Secure delete", "smb.unix_info2.file.flags.secure_delete", FT_BOOLEAN, 32,
	     NULL, 0x00000001, NULL, HFILL }},

	{ &hf_smb_unix_info2_file_flags_enable_undelete,
	  { "Enable undelete", "smb.unix_info2.file.flags.enable_undelete", FT_BOOLEAN, 32,
	     NULL, 0x00000002, NULL, HFILL }},

	{ &hf_smb_unix_info2_file_flags_synchronous,
	  { "Synchronous", "smb.unix_info2.file.flags.synchronous", FT_BOOLEAN, 32,
	     NULL, 0x00000004, NULL, HFILL }},

	{ &hf_smb_unix_info2_file_flags_immutable,
	  { "Immutable", "smb.unix_info2.file.flags.immutable", FT_BOOLEAN, 32,
	     NULL, 0x00000008, NULL, HFILL }},

	{ &hf_smb_unix_info2_file_flags_append_only,
	  { "Append-only", "smb.unix_info2.file.flags.append_only", FT_BOOLEAN, 32,
	     NULL, 0x00000010, NULL, HFILL }},

	{ &hf_smb_unix_info2_file_flags_do_not_backup,
	  { "Do not backup", "smb.unix_info2.file.flags.do_not_backup", FT_BOOLEAN, 32,
	     NULL, 0x00000020, NULL, HFILL }},

	{ &hf_smb_unix_info2_file_flags_no_update_atime,
	  { "Don't update atime", "smb.unix_info2.file.flags.no_update_atime", FT_BOOLEAN, 32,
	     NULL, 0x00000040, NULL, HFILL }},

	{ &hf_smb_unix_info2_file_flags_hidden,
	  { "Hidden", "smb.unix_info2.file.flags.hidden", FT_BOOLEAN, 32,
	     NULL, 0x00000080, NULL, HFILL }},

	{ &hf_smb_unix_file_name_length,
	  { "File name length", "smb.unix.file.name_length", FT_UINT32, BASE_DEC,
	     NULL, 0, NULL, HFILL }},

	{ &hf_smb_unix_file_name,
	  { "File name", "smb.unix.file.name", FT_STRING,
	    BASE_NONE, NULL, 0, NULL, HFILL }},

	{ &hf_smb_unix_find_file_nextoffset,
	  { "Next entry offset", "smb.unix.find_file.next_offset", FT_UINT32, BASE_DEC,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_unix_find_file_resumekey,
	  { "Resume key", "smb.unix.find_file.resume_key", FT_UINT32, BASE_DEC,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_unix_whoami_mapflags,
	  { "UNIX whoami mapping flags", "smb.unix.whoami.mapflags", FT_UINT32, BASE_DEC,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_unix_whoami_mapflags_mask,
	  { "UNIX whoami mapping flags mask", "smb.unix.whoami.mapflags_mask", FT_UINT32, BASE_DEC,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_unix_whoami_num_supl_gids,
	  { "Number of supplementary UNIX GIDs", "smb.unix.whoami.num_gids", FT_UINT32, BASE_DEC,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_unix_whoami_num_supl_sids,
	  { "Number of supplementary SIDs", "smb.unix.whoami.num_sids", FT_UINT32, BASE_DEC,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_unix_whoami_sids_buflen,
	  { "Supplementary SIDs buffer length", "smb.unix.whoami.sids_buflen", FT_UINT32, BASE_DEC,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_create_flags,
	  { "Create Flags", "smb.create_flags", FT_UINT32, BASE_HEX,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_create_options,
	  { "Create Options", "smb.create_options", FT_UINT32, BASE_HEX,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_share_access,
	  { "Share Access", "smb.share_access", FT_UINT32, BASE_HEX,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_access_mask,
	  { "Access Mask", "smb.access_mask", FT_UINT32, BASE_HEX,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_mode,
	  { "Mode", "smb.mode", FT_UINT32, BASE_HEX,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_attribute,
	  { "Attribute", "smb.attribute", FT_UINT32, BASE_HEX,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_reparse_tag,
	  { "Reparse Tag", "smb.reparse_tag", FT_UINT32, BASE_HEX,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_disposition_delete_on_close,
	  { "Delete on close", "smb.disposition.delete_on_close", FT_BOOLEAN, 8,
	    TFS(&tfs_disposition_delete_on_close), 0x01, NULL, HFILL }},

	{ &hf_smb_pipe_info_flag,
	  { "Pipe Info", "smb.pipe_info_flag", FT_BOOLEAN, 8,
	    TFS(&tfs_pipe_info_flag), 0x01, NULL, HFILL }},

	{ &hf_smb_logged_in,
	  { "Logged In", "smb.logged_in", FT_FRAMENUM, BASE_NONE,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_logged_out,
	  { "Logged Out", "smb.logged_out", FT_FRAMENUM, BASE_NONE,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_file_rw_offset,
	  { "File Offset", "smb.file.rw.offset", FT_UINT64, BASE_DEC,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_file_rw_length,
	  { "File RW Length", "smb.file.rw.length", FT_UINT32, BASE_DEC,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_posix_acl_version,
	  { "Posix ACL version", "smb.posix_acl.version", FT_UINT16, BASE_DEC,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_posix_num_file_aces,
	  { "Number of file ACEs", "smb.posix_acl.num_file_aces", FT_UINT16, BASE_DEC,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_posix_num_def_aces,
	  { "Number of default ACEs", "smb.posix_acl.num_def_aces", FT_UINT16, BASE_DEC,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_posix_ace_type,
	  { "ACE Type", "smb.posix_acl.ace_type", FT_UINT8, BASE_DEC,
	    VALS(ace_type_vals), 0, NULL, HFILL }},

	{ &hf_smb_posix_ace_flags,
	  { "Permissions", "smb.posix_acl.ace_perms", FT_UINT8, BASE_HEX,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_posix_ace_perm_read,
	  {"READ", "smb.posix_acl.ace_perms.read", FT_BOOLEAN, 8,
	   NULL, 0x04, NULL, HFILL}},

	{ &hf_smb_posix_ace_perm_write,
	  {"WRITE", "smb.posix_acl.ace_perms.write", FT_BOOLEAN, 8,
	   NULL, 0x02, NULL, HFILL}},

	{ &hf_smb_posix_ace_perm_execute,
	  {"EXECUTE", "smb.posix_acl.ace_perms.execute", FT_BOOLEAN, 8,
	   NULL, 0x01, NULL, HFILL}},

	{ &hf_smb_posix_ace_perm_owner_uid,
	  { "Owner UID", "smb.posix_acl.ace_perms.owner_uid", FT_UINT32, BASE_DEC,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_posix_ace_perm_owner_gid,
	  { "Owner GID", "smb.posix_acl.ace_perms.owner_gid", FT_UINT32, BASE_DEC,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_posix_ace_perm_uid,
	  { "UID", "smb.posix_acl.ace_perms.uid", FT_UINT32, BASE_DEC,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_posix_ace_perm_gid,
	  { "GID", "smb.posix_acl.ace_perms.gid", FT_UINT32, BASE_DEC,
	    NULL, 0, NULL, HFILL }},

	{ &hf_smb_trans_data_setup_word,
		{ "Setup Word", "smb.trans_data.setup_word", FT_UINT16, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_trans_data_parameters,
		{ "Parameters", "smb.trans_data.parameters", FT_BYTES, BASE_NONE,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_trans_data,
		{ "Data", "smb.trans_data", FT_BYTES, BASE_NONE,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_extra_byte_parameters,
		{ "Extra byte parameters", "smb.extra_byte_parameters", FT_BYTES, BASE_NONE,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_file_access_mask_full_control,
		{ "FULL CONTROL", "smb.file.accessmask.full_control", FT_UINT32, BASE_HEX,
		NULL, 0x000001ff, NULL, HFILL }},

	{ &hf_smb_dir_access_mask_full_control,
		{ "FULL CONTROL", "smb.dir.accessmask.full_control", FT_UINT32, BASE_HEX,
		NULL, 0x000001ff, NULL, HFILL }},

	{ &hf_smb_word_unk_response_format,
		{ "Words for unknown response format", "smb.word_unk_response_format", FT_BYTES, BASE_NONE,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_nt_transaction_setup,
		{ "NT Transaction Setup", "smb.nt_transaction_setup", FT_BYTES, BASE_NONE,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_server_component,
		{ "Server Component", "smb.server_component", FT_UINT32, BASE_HEX,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_byte_parameters,
		{ "Byte parameters", "smb.byte_parameters", FT_BYTES, BASE_NONE,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_smb_word_parameters,
		{ "Word parameters", "smb.word_parameters", FT_BYTES, BASE_NONE,
		NULL, 0x0, NULL, HFILL }},
	};

	static gint *ett[] = {
		&ett_smb,
		&ett_smb_fid,
		&ett_smb_tid,
		&ett_smb_uid,
		&ett_smb_hdr,
		&ett_smb_command,
		&ett_smb_fileattributes,
		&ett_smb_capabilities,
		&ett_smb_aflags,
		&ett_smb_dialect,
		&ett_smb_dialects,
		&ett_smb_mode,
		&ett_smb_rawmode,
		&ett_smb_flags,
		&ett_smb_flags2,
		&ett_smb_desiredaccess,
		&ett_smb_search,
		&ett_smb_file,
		&ett_smb_openfunction,
		&ett_smb_filetype,
		&ett_smb_openaction,
		&ett_smb_writemode,
		&ett_smb_lock_type,
		&ett_smb_ssetupandxaction,
		&ett_smb_optionsup,
		&ett_smb_time_date,
		&ett_smb_move_copy_flags,
		&ett_smb_file_attributes,
		&ett_smb_search_resume_key,
		&ett_smb_search_dir_info,
		&ett_smb_unlocks,
		&ett_smb_unlock,
		&ett_smb_locks,
		&ett_smb_lock,
		&ett_smb_open_flags,
		&ett_smb_ipc_state,
		&ett_smb_open_action,
		&ett_smb_setup_action,
		&ett_smb_connect_flags,
		&ett_smb_connect_support_bits,
		&ett_smb_nt_access_mask,
		&ett_smb_nt_create_bits,
		&ett_smb_nt_create_options,
		&ett_smb_nt_share_access,
		&ett_smb_nt_security_flags,
		&ett_smb_nt_trans_setup,
		&ett_smb_nt_trans_data,
		&ett_smb_nt_trans_param,
		&ett_smb_nt_notify_completion_filter,
		&ett_smb_nt_ioctl_flags,
		&ett_smb_security_information_mask,
		&ett_smb_print_queue_entry,
		&ett_smb_transaction_flags,
		&ett_smb_transaction_params,
		&ett_smb_find_first2_flags,
#if 0
		&ett_smb_ioflag,
#endif
		&ett_smb_transaction_data,
		&ett_smb_stream_info,
		&ett_smb_dfs_referrals,
		&ett_smb_dfs_referral,
		&ett_smb_dfs_referral_flags,
		&ett_smb_dfs_referral_expnames,
		&ett_smb_get_dfs_flags,
		&ett_smb_ff2_data,
		&ett_smb_device_characteristics,
		&ett_smb_fs_attributes,
		&ett_smb_segments,
		&ett_smb_segment,
		&ett_smb_quotaflags,
		&ett_smb_secblob,
		&ett_smb_mac_support_flags,
		&ett_smb_unicode_password,
		&ett_smb_ea,
		&ett_smb_unix_capabilities,
		&ett_smb_unix_whoami_gids,
		&ett_smb_unix_whoami_sids,
		&ett_smb_posix_ace,
		&ett_smb_posix_ace_perms,
		&ett_smb_info2_file_flags
	};

	static ei_register_info ei[] = {
		{ &ei_smb_mal_information_level, { "smb.information_level.malformed", PI_MALFORMED, PI_ERROR, "Information level structure goes past the end of the transation data.", EXPFILL }},
		{ &ei_smb_not_implemented, { "smb.not_implemented", PI_UNDECODED, PI_WARN, "Not Implemented yet", EXPFILL }},
		{ &ei_smb_nt_transaction_setup, { "smb.nt_transaction_setup.unknown", PI_PROTOCOL, PI_NOTE, "Unknown NT Transaction Setup (matching request not seen)", EXPFILL }},
		{ &ei_smb_posix_ace_type, { "smb.posix_acl.ace_type.unknown", PI_PROTOCOL, PI_WARN, "Unknown posix ace type", EXPFILL }},
		{ &ei_smb_info_level_unknown, { "smb.info_level_unknown", PI_PROTOCOL, PI_WARN, "Information level unknown", EXPFILL }},
		{ &ei_smb_info_level_not_understood, { "smb.info_level_not_understood", PI_PROTOCOL, PI_WARN, "Information level not understood", EXPFILL }},
	};

	module_t *smb_module;
	expert_module_t* expert_smb;

	proto_smb = proto_register_protocol("SMB (Server Message Block Protocol)",
	    "SMB", "smb");
	proto_register_subtree_array(ett, array_length(ett));
	proto_register_field_array(proto_smb, hf, array_length(hf));
	expert_smb = expert_register_protocol(proto_smb);
	expert_register_field_array(expert_smb, ei, array_length(ei));

	proto_do_register_windows_common(proto_smb);

	register_cleanup_routine(&smb_cleanup);
	smb_module = prefs_register_protocol(proto_smb, NULL);
	prefs_register_bool_preference(smb_module, "trans_reassembly",
		"Reassemble SMB Transaction payload",
		"Whether the dissector should reassemble the payload of SMB Transaction commands spanning multiple SMB PDUs",
		&smb_trans_reassembly);
	prefs_register_bool_preference(smb_module, "dcerpc_reassembly",
		"Reassemble DCERPC over SMB",
		"Whether the dissector should reassemble DCERPC over SMB commands",
		&smb_dcerpc_reassembly);
	prefs_register_bool_preference(smb_module, "sid_name_snooping",
		"Snoop SID to Name mappings",
		"Whether the dissector should snoop SMB and related CIFS protocols to discover and display Names associated with SIDs",
		&sid_name_snooping);

	/* For display of SIDs and RIDs in Hex option */
	prefs_register_bool_preference(smb_module, "sid_display_hex",
		"Display SIDs in Hex",
		"Whether the dissector should display SIDs and RIDs in hexadecimal rather than decimal",
		&sid_display_hex);

	/* Will Export Object take name as fid ? */
	prefs_register_bool_preference(smb_module, "eosmb_take_name_as_fid",
		"Use the full file name as File ID when exporting an SMB object",
		"Whether the export object functionality will take the full path file name as file identifier",
		&eosmb_take_name_as_fid);

	register_init_routine(smb_trans_reassembly_init);
	smb_tap = register_tap("smb");

	/* Register the tap for the "Export Object" function */
	smb_eo_tap = register_tap("smb_eo"); /* SMB Export Object tap */

	register_dissector("smb", dissect_smb, proto_smb);

	register_srt_table(proto_smb, NULL, 3, smbstat_packet, smbstat_init, NULL);
}

void
proto_reg_handoff_smb(void)
{
	dissector_handle_t smb_handle;

	gssapi_handle  = find_dissector_add_dependency("gssapi", proto_smb);
	ntlmssp_handle = find_dissector_add_dependency("ntlmssp", proto_smb);

	heur_dissector_add("netbios", dissect_smb_heur, "SMB over Netbios", "smb_netbios", proto_smb, HEURISTIC_ENABLE);
	heur_dissector_add("smb_direct", dissect_smb_heur, "SMB over SMB Direct", "smb_smb_direct", proto_smb, HEURISTIC_ENABLE);
	heur_dissector_add("cotp", dissect_smb_heur, "SMB over COTP", "smb_cotp", proto_smb, HEURISTIC_ENABLE);
	heur_dissector_add("vines_spp", dissect_smb_heur, "SMB over Vines", "smb_vines", proto_smb, HEURISTIC_ENABLE);

	smb_handle = find_dissector("smb");
	dissector_add_uint("ipx.socket", IPX_SOCKET_NWLINK_SMB_SERVER, smb_handle);
	dissector_add_uint("ipx.socket", IPX_SOCKET_NWLINK_SMB_REDIR, smb_handle);
	dissector_add_uint("ipx.socket", IPX_SOCKET_NWLINK_SMB_MESSENGER, smb_handle);
	dissector_add_uint("spp.socket", IDP_SOCKET_SMB, smb_handle);
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
