/* packet-mysql.c
 * Routines for mysql packet dissection
 *
 * Huagang XIE <huagang@intruvert.com>
 *
 * MySQL 4.1+ protocol by Axel Schwenke <axel@mysql.com>
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
 *
 *
 * the protocol spec at
 *  https://dev.mysql.com/doc/internals/en/client-server-protocol.html
 * and MySQL source code
 */

/* create extra output for conversation tracking */
/* #define CTDEBUG 1 */

#include "config.h"

#include <epan/packet.h>
#include <epan/prefs.h>
#include <epan/expert.h>
#include <epan/strutil.h>
#include <epan/proto_data.h>
#include "packet-tcp.h"
#include "packet-ssl-utils.h"

void proto_register_mysql(void);
void proto_reg_handoff_mysql(void);

/* port for protocol registration */
#define TCP_PORT_MySQL   3306

/* client/server capabilities
 * Source: http://dev.mysql.com/doc/internals/en/capability-flags.html
 * Source: mysql_com.h
 */
#define MYSQL_CAPS_LP 0x0001 /* CLIENT_LONG_PASSWORD */
#define MYSQL_CAPS_FR 0x0002 /* CLIENT_FOUND_ROWS */
#define MYSQL_CAPS_LF 0x0004 /* CLIENT_LONG_FLAG */
#define MYSQL_CAPS_CD 0x0008 /* CLIENT_CONNECT_WITH_DB */
#define MYSQL_CAPS_NS 0x0010 /* CLIENT_NO_SCHEMA */
#define MYSQL_CAPS_CP 0x0020 /* CLIENT_COMPRESS */
#define MYSQL_CAPS_OB 0x0040 /* CLIENT_ODBC */
#define MYSQL_CAPS_LI 0x0080 /* CLIENT_LOCAL_FILES */
#define MYSQL_CAPS_IS 0x0100 /* CLIENT_IGNORE_SPACE */
#define MYSQL_CAPS_CU 0x0200 /* CLIENT_PROTOCOL_41 */
#define MYSQL_CAPS_IA 0x0400 /* CLIENT_INTERACTIVE */
#define MYSQL_CAPS_SL 0x0800 /* CLIENT_SSL */
#define MYSQL_CAPS_II 0x1000 /* CLIENT_IGNORE_SPACE */
#define MYSQL_CAPS_TA 0x2000 /* CLIENT_TRANSACTIONS */
#define MYSQL_CAPS_RS 0x4000 /* CLIENT_RESERVED */
#define MYSQL_CAPS_SC 0x8000 /* CLIENT_SECURE_CONNECTION */


/* field flags */
#define MYSQL_FLD_NOT_NULL_FLAG       0x0001
#define MYSQL_FLD_PRI_KEY_FLAG        0x0002
#define MYSQL_FLD_UNIQUE_KEY_FLAG     0x0004
#define MYSQL_FLD_MULTIPLE_KEY_FLAG   0x0008
#define MYSQL_FLD_BLOB_FLAG           0x0010
#define MYSQL_FLD_UNSIGNED_FLAG       0x0020
#define MYSQL_FLD_ZEROFILL_FLAG       0x0040
#define MYSQL_FLD_BINARY_FLAG         0x0080
#define MYSQL_FLD_ENUM_FLAG           0x0100
#define MYSQL_FLD_AUTO_INCREMENT_FLAG 0x0200
#define MYSQL_FLD_TIMESTAMP_FLAG      0x0400
#define MYSQL_FLD_SET_FLAG            0x0800

/* extended capabilities: 4.1+ client only
 *
 * These are libmysqlclient flags and NOT present
 * in the protocol:
 * CLIENT_SSL_VERIFY_SERVER_CERT (1UL << 30)
 * CLIENT_REMEMBER_OPTIONS (1UL << 31)
 */
#define MYSQL_CAPS_MS 0x0001 /* CLIENT_MULTI_STATMENTS */
#define MYSQL_CAPS_MR 0x0002 /* CLIENT_MULTI_RESULTS */
#define MYSQL_CAPS_PM 0x0004 /* CLIENT_PS_MULTI_RESULTS */
#define MYSQL_CAPS_PA 0x0008 /* CLIENT_PLUGIN_AUTH */
#define MYSQL_CAPS_CA 0x0010 /* CLIENT_CONNECT_ATTRS */
#define MYSQL_CAPS_AL 0x0020 /* CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA */
#define MYSQL_CAPS_EP 0x0040 /* CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS */
#define MYSQL_CAPS_ST 0x0080 /* CLIENT_SESSION_TRACK */
#define MYSQL_CAPS_DE 0x0100 /* CLIENT_DEPRECATE_EOF */
#define MYSQL_CAPS_UNUSED 0xFE00

/* status bitfield */
#define MYSQL_STAT_IT 0x0001
#define MYSQL_STAT_AC 0x0002
#define MYSQL_STAT_MR 0x0004
#define MYSQL_STAT_MU 0x0008
#define MYSQL_STAT_BI 0x0010
#define MYSQL_STAT_NI 0x0020
#define MYSQL_STAT_CR 0x0040
#define MYSQL_STAT_LR 0x0080
#define MYSQL_STAT_DR 0x0100
#define MYSQL_STAT_BS 0x0200
#define MYSQL_STAT_SESSION_STATE_CHANGED 0x0400
#define MYSQL_STAT_QUERY_WAS_SLOW 0x0800
#define MYSQL_STAT_PS_OUT_PARAMS 0x1000

/* bitfield for MYSQL_REFRESH */
#define MYSQL_RFSH_GRANT   1   /* Refresh grant tables */
#define MYSQL_RFSH_LOG     2   /* Start on new log file */
#define MYSQL_RFSH_TABLES  4   /* close all tables */
#define MYSQL_RFSH_HOSTS   8   /* Flush host cache */
#define MYSQL_RFSH_STATUS  16  /* Flush status variables */
#define MYSQL_RFSH_THREADS 32  /* Flush thread cache */
#define MYSQL_RFSH_SLAVE   64  /* Reset master info and restart slave thread */
#define MYSQL_RFSH_MASTER  128 /* Remove all bin logs in the index and truncate the index */

/* MySQL command codes */
#define MYSQL_SLEEP               0  /* not from client */
#define MYSQL_QUIT                1
#define MYSQL_INIT_DB             2
#define MYSQL_QUERY               3
#define MYSQL_FIELD_LIST          4
#define MYSQL_CREATE_DB           5
#define MYSQL_DROP_DB             6
#define MYSQL_REFRESH             7
#define MYSQL_SHUTDOWN            8
#define MYSQL_STATISTICS          9
#define MYSQL_PROCESS_INFO        10
#define MYSQL_CONNECT             11 /* not from client */
#define MYSQL_PROCESS_KILL        12
#define MYSQL_DEBUG               13
#define MYSQL_PING                14
#define MYSQL_TIME                15 /* not from client */
#define MYSQL_DELAY_INSERT        16 /* not from client */
#define MYSQL_CHANGE_USER         17
#define MYSQL_BINLOG_DUMP         18 /* replication */
#define MYSQL_TABLE_DUMP          19 /* replication */
#define MYSQL_CONNECT_OUT         20 /* replication */
#define MYSQL_REGISTER_SLAVE      21 /* replication */
#define MYSQL_STMT_PREPARE        22
#define MYSQL_STMT_EXECUTE        23
#define MYSQL_STMT_SEND_LONG_DATA 24
#define MYSQL_STMT_CLOSE          25
#define MYSQL_STMT_RESET          26
#define MYSQL_SET_OPTION          27
#define MYSQL_STMT_FETCH          28

/* MySQL cursor types */

#define MYSQL_CURSOR_TYPE_NO_CURSOR  0
#define MYSQL_CURSOR_TYPE_READ_ONLY  1
#define MYSQL_CURSOR_TYPE_FOR_UPDATE 2
#define MYSQL_CURSOR_TYPE_SCROLLABLE 4

/* MySQL parameter flags -- used internally by the dissector */

#define MYSQL_PARAM_FLAG_STREAMED 0x01

/* Compression states, internal to the dissector */
#define MYSQL_COMPRESS_NONE   0
#define MYSQL_COMPRESS_INIT   1
#define MYSQL_COMPRESS_ACTIVE 2

/* decoding table: command */
static const value_string mysql_command_vals[] = {
	{MYSQL_SLEEP,   "SLEEP"},
	{MYSQL_QUIT,   "Quit"},
	{MYSQL_INIT_DB,  "Use Database"},
	{MYSQL_QUERY,   "Query"},
	{MYSQL_FIELD_LIST, "Show Fields"},
	{MYSQL_CREATE_DB,  "Create Database"},
	{MYSQL_DROP_DB , "Drop Database"},
	{MYSQL_REFRESH , "Refresh"},
	{MYSQL_SHUTDOWN , "Shutdown"},
	{MYSQL_STATISTICS , "Statistics"},
	{MYSQL_PROCESS_INFO , "Process List"},
	{MYSQL_CONNECT , "Connect"},
	{MYSQL_PROCESS_KILL , "Kill Server Thread"},
	{MYSQL_DEBUG , "Dump Debuginfo"},
	{MYSQL_PING , "Ping"},
	{MYSQL_TIME , "Time"},
	{MYSQL_DELAY_INSERT , "Insert Delayed"},
	{MYSQL_CHANGE_USER , "Change User"},
	{MYSQL_BINLOG_DUMP , "Send Binlog"},
	{MYSQL_TABLE_DUMP, "Send Table"},
	{MYSQL_CONNECT_OUT, "Slave Connect"},
	{MYSQL_REGISTER_SLAVE, "Register Slave"},
	{MYSQL_STMT_PREPARE, "Prepare Statement"},
	{MYSQL_STMT_EXECUTE, "Execute Statement"},
	{MYSQL_STMT_SEND_LONG_DATA, "Send BLOB"},
	{MYSQL_STMT_CLOSE, "Close Statement"},
	{MYSQL_STMT_RESET, "Reset Statement"},
	{MYSQL_SET_OPTION, "Set Option"},
	{MYSQL_STMT_FETCH, "Fetch Data"},
	{0, NULL}
};

/* decoding table: exec_flags */
static const value_string mysql_exec_flags_vals[] = {
	{MYSQL_CURSOR_TYPE_NO_CURSOR, "Defaults"},
	{MYSQL_CURSOR_TYPE_READ_ONLY, "Read-only cursor"},
	{MYSQL_CURSOR_TYPE_FOR_UPDATE, "Cursor for update"},
	{MYSQL_CURSOR_TYPE_SCROLLABLE, "Scrollable cursor"},
	{0, NULL}
};

/* decoding table: new_parameter_bound_flag */
static const value_string mysql_new_parameter_bound_flag_vals[] = {
	{0, "Subsequent call"},
	{1, "First call or rebound"},
	{0, NULL}
};

/* decoding table: exec_time_sign */
static const value_string mysql_exec_time_sign_vals[] = {
	{0, "Positive"},
	{1, "Negative"},
	{0, NULL}
};

#if 0
/* charset: pre-4.1 used the term 'charset', later changed to 'collation' */
static const value_string mysql_charset_vals[] = {
	{1,  "big5"},
	{2,  "czech"},
	{3,  "dec8"},
	{4,  "dos" },
	{5,  "german1"},
	{6,  "hp8"},
	{7,  "koi8_ru"},
	{8,  "latin1"},
	{9,  "latin2"},
	{9,  "swe7 "},
	{10, "usa7"},
	{11, "ujis"},
	{12, "sjis"},
	{13, "cp1251"},
	{14, "danish"},
	{15, "hebrew"},
	{16, "win1251"},
	{17, "tis620"},
	{18, "euc_kr"},
	{19, "estonia"},
	{20, "hungarian"},
	{21, "koi8_ukr"},
	{22, "win1251ukr"},
	{23, "gb2312"},
	{24, "greek"},
	{25, "win1250"},
	{26, "croat"},
	{27, "gbk"},
	{28, "cp1257"},
	{29, "latin5"},
	{0, NULL}
};
#endif


/* collation codes may change over time, recreate with the following SQL

SELECT CONCAT('  {', ID, ',"', CHARACTER_SET_NAME, ' COLLATE ', COLLATION_NAME, '"},')
FROM INFORMATION_SCHEMA.COLLATIONS
ORDER BY ID
INTO OUTFILE '/tmp/mysql-collations';

*/
static const value_string mysql_collation_vals[] = {
	{3,   "dec8 COLLATE dec8_swedish_ci"},
	{4,   "cp850 COLLATE cp850_general_ci"},
	{5,   "latin1 COLLATE latin1_german1_ci"},
	{6,   "hp8 COLLATE hp8_english_ci"},
	{7,   "koi8r COLLATE koi8r_general_ci"},
	{8,   "latin1 COLLATE latin1_swedish_ci"},
	{9,   "latin2 COLLATE latin2_general_ci"},
	{10,  "swe7 COLLATE swe7_swedish_ci"},
	{11,  "ascii COLLATE ascii_general_ci"},
	{14,  "cp1251 COLLATE cp1251_bulgarian_ci"},
	{15,  "latin1 COLLATE latin1_danish_ci"},
	{16,  "hebrew COLLATE hebrew_general_ci"},
	{20,  "latin7 COLLATE latin7_estonian_cs"},
	{21,  "latin2 COLLATE latin2_hungarian_ci"},
	{22,  "koi8u COLLATE koi8u_general_ci"},
	{23,  "cp1251 COLLATE cp1251_ukrainian_ci"},
	{25,  "greek COLLATE greek_general_ci"},
	{26,  "cp1250 COLLATE cp1250_general_ci"},
	{27,  "latin2 COLLATE latin2_croatian_ci"},
	{29,  "cp1257 COLLATE cp1257_lithuanian_ci"},
	{30,  "latin5 COLLATE latin5_turkish_ci"},
	{31,  "latin1 COLLATE latin1_german2_ci"},
	{32,  "armscii8 COLLATE armscii8_general_ci"},
	{33,  "utf8 COLLATE utf8_general_ci"},
	{36,  "cp866 COLLATE cp866_general_ci"},
	{37,  "keybcs2 COLLATE keybcs2_general_ci"},
	{38,  "macce COLLATE macce_general_ci"},
	{39,  "macroman COLLATE macroman_general_ci"},
	{40,  "cp852 COLLATE cp852_general_ci"},
	{41,  "latin7 COLLATE latin7_general_ci"},
	{42,  "latin7 COLLATE latin7_general_cs"},
	{43,  "macce COLLATE macce_bin"},
	{44,  "cp1250 COLLATE cp1250_croatian_ci"},
	{45,  "utf8mb4 COLLATE utf8mb4_general_ci"},
	{46,  "utf8mb4 COLLATE utf8mb4_bin"},
	{47,  "latin1 COLLATE latin1_bin"},
	{48,  "latin1 COLLATE latin1_general_ci"},
	{49,  "latin1 COLLATE latin1_general_cs"},
	{50,  "cp1251 COLLATE cp1251_bin"},
	{51,  "cp1251 COLLATE cp1251_general_ci"},
	{52,  "cp1251 COLLATE cp1251_general_cs"},
	{53,  "macroman COLLATE macroman_bin"},
	{57,  "cp1256 COLLATE cp1256_general_ci"},
	{58,  "cp1257 COLLATE cp1257_bin"},
	{59,  "cp1257 COLLATE cp1257_general_ci"},
	{63,  "binary COLLATE binary"},
	{64,  "armscii8 COLLATE armscii8_bin"},
	{65,  "ascii COLLATE ascii_bin"},
	{66,  "cp1250 COLLATE cp1250_bin"},
	{67,  "cp1256 COLLATE cp1256_bin"},
	{68,  "cp866 COLLATE cp866_bin"},
	{69,  "dec8 COLLATE dec8_bin"},
	{70,  "greek COLLATE greek_bin"},
	{71,  "hebrew COLLATE hebrew_bin"},
	{72,  "hp8 COLLATE hp8_bin"},
	{73,  "keybcs2 COLLATE keybcs2_bin"},
	{74,  "koi8r COLLATE koi8r_bin"},
	{75,  "koi8u COLLATE koi8u_bin"},
	{77,  "latin2 COLLATE latin2_bin"},
	{78,  "latin5 COLLATE latin5_bin"},
	{79,  "latin7 COLLATE latin7_bin"},
	{80,  "cp850 COLLATE cp850_bin"},
	{81,  "cp852 COLLATE cp852_bin"},
	{82,  "swe7 COLLATE swe7_bin"},
	{83,  "utf8 COLLATE utf8_bin"},
	{92,  "geostd8 COLLATE geostd8_general_ci"},
	{93,  "geostd8 COLLATE geostd8_bin"},
	{94,  "latin1 COLLATE latin1_spanish_ci"},
	{99,  "cp1250 COLLATE cp1250_polish_ci"},
	{192, "utf8 COLLATE utf8_unicode_ci"},
	{193, "utf8 COLLATE utf8_icelandic_ci"},
	{194, "utf8 COLLATE utf8_latvian_ci"},
	{195, "utf8 COLLATE utf8_romanian_ci"},
	{196, "utf8 COLLATE utf8_slovenian_ci"},
	{197, "utf8 COLLATE utf8_polish_ci"},
	{198, "utf8 COLLATE utf8_estonian_ci"},
	{199, "utf8 COLLATE utf8_spanish_ci"},
	{200, "utf8 COLLATE utf8_swedish_ci"},
	{201, "utf8 COLLATE utf8_turkish_ci"},
	{202, "utf8 COLLATE utf8_czech_ci"},
	{203, "utf8 COLLATE utf8_danish_ci"},
	{204, "utf8 COLLATE utf8_lithuanian_ci"},
	{205, "utf8 COLLATE utf8_slovak_ci"},
	{206, "utf8 COLLATE utf8_spanish2_ci"},
	{207, "utf8 COLLATE utf8_roman_ci"},
	{208, "utf8 COLLATE utf8_persian_ci"},
	{209, "utf8 COLLATE utf8_esperanto_ci"},
	{210, "utf8 COLLATE utf8_hungarian_ci"},
	{211, "utf8 COLLATE utf8_sinhala_ci"},
	{212, "utf8 COLLATE utf8_german2_ci"},
	{213, "utf8 COLLATE utf8_croatian_ci"},
	{214, "utf8 COLLATE utf8_unicode_520_ci"},
	{215, "utf8 COLLATE utf8_vietnamese_ci"},
	{223, "utf8 COLLATE utf8_general_mysql500_ci"},
	{224, "utf8mb4 COLLATE utf8mb4_unicode_ci"},
	{225, "utf8mb4 COLLATE utf8mb4_icelandic_ci"},
	{226, "utf8mb4 COLLATE utf8mb4_latvian_ci"},
	{227, "utf8mb4 COLLATE utf8mb4_romanian_ci"},
	{228, "utf8mb4 COLLATE utf8mb4_slovenian_ci"},
	{229, "utf8mb4 COLLATE utf8mb4_polish_ci"},
	{230, "utf8mb4 COLLATE utf8mb4_estonian_ci"},
	{231, "utf8mb4 COLLATE utf8mb4_spanish_ci"},
	{232, "utf8mb4 COLLATE utf8mb4_swedish_ci"},
	{233, "utf8mb4 COLLATE utf8mb4_turkish_ci"},
	{234, "utf8mb4 COLLATE utf8mb4_czech_ci"},
	{235, "utf8mb4 COLLATE utf8mb4_danish_ci"},
	{236, "utf8mb4 COLLATE utf8mb4_lithuanian_ci"},
	{237, "utf8mb4 COLLATE utf8mb4_slovak_ci"},
	{238, "utf8mb4 COLLATE utf8mb4_spanish2_ci"},
	{239, "utf8mb4 COLLATE utf8mb4_roman_ci"},
	{240, "utf8mb4 COLLATE utf8mb4_persian_ci"},
	{241, "utf8mb4 COLLATE utf8mb4_esperanto_ci"},
	{242, "utf8mb4 COLLATE utf8mb4_hungarian_ci"},
	{243, "utf8mb4 COLLATE utf8mb4_sinhala_ci"},
	{244, "utf8mb4 COLLATE utf8mb4_german2_ci"},
	{245, "utf8mb4 COLLATE utf8mb4_croatian_ci"},
	{246, "utf8mb4 COLLATE utf8mb4_unicode_520_ci"},
	{247, "utf8mb4 COLLATE utf8mb4_vietnamese_ci"},
	{0, NULL}
};


/* allowed MYSQL_SHUTDOWN levels */
static const value_string mysql_shutdown_vals[] = {
	{0,   "default"},
	{1,   "wait for connections to finish"},
	{2,   "wait for transactions to finish"},
	{8,   "wait for updates to finish"},
	{16,  "wait flush all buffers"},
	{17,  "wait flush critical buffers"},
	{254, "kill running queries"},
	{255, "kill connections"},
	{0, NULL}
};


/* allowed MYSQL_SET_OPTION values */
static const value_string mysql_option_vals[] = {
	{0, "multi statements on"},
	{1, "multi statements off"},
	{0, NULL}
};

static const value_string mysql_session_track_type_vals[] = {
	{0, "SESSION_SYSVARS_TRACKER"},
	{1, "CURRENT_SCHEMA_TRACKER"},
	{2, "SESSION_STATE_CHANGE_TRACKER"},
	{0, NULL}
};

/* protocol id */
static int proto_mysql = -1;

/* dissector configuration */
static gboolean mysql_desegment = TRUE;
static gboolean mysql_showquery = FALSE;

/* expand-the-tree flags */
static gint ett_mysql = -1;
static gint ett_server_greeting = -1;
static gint ett_login_request = -1;
static gint ett_caps = -1;
static gint ett_extcaps = -1;
static gint ett_stat = -1;
static gint ett_request = -1;
static gint ett_refresh = -1;
static gint ett_field_flags = -1;
static gint ett_exec_param = -1;
static gint ett_session_track = -1;
static gint ett_session_track_data = -1;
static gint ett_connattrs = -1;
static gint ett_connattrs_attr = -1;

/* protocol fields */
static int hf_mysql_caps_server = -1;
static int hf_mysql_caps_client = -1;
static int hf_mysql_cap_long_password = -1;
static int hf_mysql_cap_found_rows = -1;
static int hf_mysql_cap_long_flag = -1;
static int hf_mysql_cap_connect_with_db = -1;
static int hf_mysql_cap_no_schema = -1;
static int hf_mysql_cap_compress = -1;
static int hf_mysql_cap_odbc = -1;
static int hf_mysql_cap_local_files = -1;
static int hf_mysql_cap_ignore_space = -1;
static int hf_mysql_cap_change_user = -1;
static int hf_mysql_cap_interactive = -1;
static int hf_mysql_cap_ssl = -1;
static int hf_mysql_cap_ignore_sigpipe = -1;
static int hf_mysql_cap_transactions = -1;
static int hf_mysql_cap_reserved = -1;
static int hf_mysql_cap_secure_connect = -1;
static int hf_mysql_extcaps_server = -1;
static int hf_mysql_extcaps_client = -1;
static int hf_mysql_cap_multi_statements = -1;
static int hf_mysql_cap_multi_results = -1;
static int hf_mysql_cap_ps_multi_results = -1;
static int hf_mysql_cap_plugin_auth = -1;
static int hf_mysql_cap_connect_attrs = -1;
static int hf_mysql_cap_plugin_auth_lenenc_client_data = -1;
static int hf_mysql_cap_client_can_handle_expired_passwords = -1;
static int hf_mysql_cap_session_track = -1;
static int hf_mysql_cap_deprecate_eof = -1;
static int hf_mysql_cap_unused = -1;
static int hf_mysql_server_language = -1;
static int hf_mysql_server_status = -1;
static int hf_mysql_stat_it = -1;
static int hf_mysql_stat_ac = -1;
static int hf_mysql_stat_mr = -1;
static int hf_mysql_stat_mu = -1;
static int hf_mysql_stat_bi = -1;
static int hf_mysql_stat_ni = -1;
static int hf_mysql_stat_cr = -1;
static int hf_mysql_stat_lr = -1;
static int hf_mysql_stat_dr = -1;
static int hf_mysql_stat_bs = -1;
static int hf_mysql_stat_session_state_changed = -1;
static int hf_mysql_stat_query_was_slow = -1;
static int hf_mysql_stat_ps_out_params = -1;
static int hf_mysql_refresh = -1;
static int hf_mysql_rfsh_grants = -1;
static int hf_mysql_rfsh_log = -1;
static int hf_mysql_rfsh_tables = -1;
static int hf_mysql_rfsh_hosts = -1;
static int hf_mysql_rfsh_status = -1;
static int hf_mysql_rfsh_threads = -1;
static int hf_mysql_rfsh_slave = -1;
static int hf_mysql_rfsh_master = -1;
static int hf_mysql_packet_length = -1;
static int hf_mysql_packet_number = -1;
static int hf_mysql_request = -1;
static int hf_mysql_command = -1;
static int hf_mysql_error_code = -1;
static int hf_mysql_error_string = -1;
static int hf_mysql_sqlstate = -1;
static int hf_mysql_message = -1;
static int hf_mysql_payload = -1;
static int hf_mysql_server_greeting = -1;
static int hf_mysql_session_track = -1;
static int hf_mysql_session_track_type = -1;
static int hf_mysql_session_track_length = -1;
static int hf_mysql_session_track_data = -1;
static int hf_mysql_session_track_data_length = -1;
static int hf_mysql_session_track_sysvar_length = -1;
static int hf_mysql_session_track_sysvar_name = -1;
static int hf_mysql_session_track_sysvar_value = -1;
static int hf_mysql_session_track_schema = -1;
static int hf_mysql_session_track_schema_length = -1;
static int hf_mysql_session_state_change = -1;
static int hf_mysql_protocol = -1;
static int hf_mysql_version  = -1;
static int hf_mysql_login_request = -1;
static int hf_mysql_max_packet = -1;
static int hf_mysql_user = -1;
static int hf_mysql_table_name = -1;
static int hf_mysql_schema = -1;
static int hf_mysql_client_auth_plugin = -1;
static int hf_mysql_connattrs = -1;
static int hf_mysql_connattrs_length = -1;
static int hf_mysql_connattrs_attr = -1;
static int hf_mysql_connattrs_name_length = -1;
static int hf_mysql_connattrs_name = -1;
static int hf_mysql_connattrs_value_length = -1;
static int hf_mysql_connattrs_value = -1;
static int hf_mysql_thread_id  = -1;
static int hf_mysql_salt = -1;
static int hf_mysql_salt2 = -1;
static int hf_mysql_auth_plugin_length = -1;
static int hf_mysql_auth_plugin = -1;
static int hf_mysql_charset = -1;
static int hf_mysql_passwd = -1;
static int hf_mysql_unused = -1;
static int hf_mysql_affected_rows = -1;
static int hf_mysql_insert_id = -1;
static int hf_mysql_num_warn = -1;
static int hf_mysql_thd_id = -1;
static int hf_mysql_stmt_id = -1;
static int hf_mysql_query = -1;
static int hf_mysql_shutdown = -1;
static int hf_mysql_option = -1;
static int hf_mysql_num_rows = -1;
static int hf_mysql_param = -1;
static int hf_mysql_num_params = -1;
static int hf_mysql_exec_flags4 = -1;
static int hf_mysql_exec_flags5 = -1;
static int hf_mysql_exec_iter = -1;
static int hf_mysql_binlog_position = -1;
static int hf_mysql_binlog_flags = -1;
static int hf_mysql_binlog_server_id = -1;
static int hf_mysql_binlog_file_name = -1;
static int hf_mysql_eof = -1;
static int hf_mysql_num_fields = -1;
static int hf_mysql_extra = -1;
static int hf_mysql_fld_catalog  = -1;
static int hf_mysql_fld_db = -1;
static int hf_mysql_fld_table = -1;
static int hf_mysql_fld_org_table = -1;
static int hf_mysql_fld_name = -1;
static int hf_mysql_fld_org_name = -1;
static int hf_mysql_fld_charsetnr = -1;
static int hf_mysql_fld_length = -1;
static int hf_mysql_fld_type = -1;
static int hf_mysql_fld_flags = -1;
static int hf_mysql_fld_not_null = -1;
static int hf_mysql_fld_primary_key = -1;
static int hf_mysql_fld_unique_key = -1;
static int hf_mysql_fld_multiple_key = -1;
static int hf_mysql_fld_blob = -1;
static int hf_mysql_fld_unsigned = -1;
static int hf_mysql_fld_zero_fill = -1;
static int hf_mysql_fld_binary = -1;
static int hf_mysql_fld_enum = -1;
static int hf_mysql_fld_auto_increment = -1;
static int hf_mysql_fld_timestamp = -1;
static int hf_mysql_fld_set = -1;
static int hf_mysql_fld_decimals = -1;
static int hf_mysql_fld_default = -1;
static int hf_mysql_row_text = -1;
static int hf_mysql_new_parameter_bound_flag = -1;
static int hf_mysql_exec_param = -1;
static int hf_mysql_exec_unsigned = -1;
static int hf_mysql_exec_field_longlong = -1;
static int hf_mysql_exec_field_string = -1;
static int hf_mysql_exec_field_double = -1;
static int hf_mysql_exec_field_datetime_length = -1;
static int hf_mysql_exec_field_year = -1;
static int hf_mysql_exec_field_month = -1;
static int hf_mysql_exec_field_day = -1;
static int hf_mysql_exec_field_hour = -1;
static int hf_mysql_exec_field_minute = -1;
static int hf_mysql_exec_field_second = -1;
static int hf_mysql_exec_field_second_b = -1;
static int hf_mysql_exec_field_long = -1;
static int hf_mysql_exec_field_tiny = -1;
static int hf_mysql_exec_field_short = -1;
static int hf_mysql_exec_field_float = -1;
static int hf_mysql_exec_field_time_length = -1;
static int hf_mysql_exec_field_time_sign = -1;
static int hf_mysql_exec_field_time_days = -1;
static int hf_mysql_auth_switch_request_status = -1;
static int hf_mysql_auth_switch_request_name = -1;
static int hf_mysql_auth_switch_request_data = -1;
static int hf_mysql_auth_switch_response_data = -1;
static int hf_mysql_compressed_packet_length = -1;
static int hf_mysql_compressed_packet_length_uncompressed = -1;
static int hf_mysql_compressed_packet_number = -1;

static dissector_handle_t mysql_handle;
static dissector_handle_t ssl_handle;

static expert_field ei_mysql_eof = EI_INIT;
static expert_field ei_mysql_dissector_incomplete = EI_INIT;
static expert_field ei_mysql_streamed_param = EI_INIT;
static expert_field ei_mysql_prepare_response_needed = EI_INIT;
static expert_field ei_mysql_unknown_response = EI_INIT;
static expert_field ei_mysql_command = EI_INIT;

/* type constants */
static const value_string type_constants[] = {
	{0x00, "FIELD_TYPE_DECIMAL"    },
	{0x01, "FIELD_TYPE_TINY"       },
	{0x02, "FIELD_TYPE_SHORT"      },
	{0x03, "FIELD_TYPE_LONG"       },
	{0x04, "FIELD_TYPE_FLOAT"      },
	{0x05, "FIELD_TYPE_DOUBLE"     },
	{0x06, "FIELD_TYPE_NULL"       },
	{0x07, "FIELD_TYPE_TIMESTAMP"  },
	{0x08, "FIELD_TYPE_LONGLONG"   },
	{0x09, "FIELD_TYPE_INT24"      },
	{0x0a, "FIELD_TYPE_DATE"       },
	{0x0b, "FIELD_TYPE_TIME"       },
	{0x0c, "FIELD_TYPE_DATETIME"   },
	{0x0d, "FIELD_TYPE_YEAR"       },
	{0x0e, "FIELD_TYPE_NEWDATE"    },
	{0x0f, "FIELD_TYPE_VARCHAR"    },
	{0x10, "FIELD_TYPE_BIT"        },
	{0xf6, "FIELD_TYPE_NEWDECIMAL" },
	{0xf7, "FIELD_TYPE_ENUM"       },
	{0xf8, "FIELD_TYPE_SET"        },
	{0xf9, "FIELD_TYPE_TINY_BLOB"  },
	{0xfa, "FIELD_TYPE_MEDIUM_BLOB"},
	{0xfb, "FIELD_TYPE_LONG_BLOB"  },
	{0xfc, "FIELD_TYPE_BLOB"       },
	{0xfd, "FIELD_TYPE_VAR_STRING" },
	{0xfe, "FIELD_TYPE_STRING"     },
	{0xff, "FIELD_TYPE_GEOMETRY"   },
	{0, NULL}
};

typedef enum mysql_state {
	UNDEFINED,
	LOGIN,
	REQUEST,
	RESPONSE_OK,
	RESPONSE_MESSAGE,
	RESPONSE_TABULAR,
	RESPONSE_SHOW_FIELDS,
	FIELD_PACKET,
	ROW_PACKET,
	RESPONSE_PREPARE,
	PREPARED_PARAMETERS,
	PREPARED_FIELDS,
	AUTH_SWITCH_REQUEST,
	AUTH_SWITCH_RESPONSE
} mysql_state_t;

#ifdef CTDEBUG
static const value_string state_vals[] = {
	{UNDEFINED,            "undefined"},
	{LOGIN,                "login"},
	{REQUEST,              "request"},
	{RESPONSE_OK,          "response OK"},
	{RESPONSE_MESSAGE,     "response message"},
	{RESPONSE_TABULAR,     "tabular response"},
	{RESPONSE_SHOW_FIELDS, "response to SHOW FIELDS"},
	{FIELD_PACKET,         "field packet"},
	{ROW_PACKET,           "row packet"},
	{RESPONSE_PREPARE,     "response to PREPARE"},
	{PREPARED_PARAMETERS,  "parameters in response to PREPARE"},
	{PREPARED_FIELDS,      "fields in response to PREPARE"},
	{AUTH_SWITCH_REQUEST,  "authentication switch request"},
	{AUTH_SWITCH_RESPONSE, "authentication switch response"},
	{0, NULL}
};
#endif

typedef struct mysql_conn_data {
	guint16 srv_caps;
	guint16 srv_caps_ext;
	guint16 clnt_caps;
	guint16 clnt_caps_ext;
	mysql_state_t state;
	guint16 stmt_num_params;
	guint16 stmt_num_fields;
	wmem_tree_t* stmts;
#ifdef CTDEBUG
	guint32 generation;
#endif
	guint8 major_version;
	guint32 frame_start_ssl;
	guint32 frame_start_compressed;
	guint8 compressed_state;
} mysql_conn_data_t;

struct mysql_frame_data {
	mysql_state_t state;
};

typedef struct my_stmt_data {
	guint16 nparam;
	guint8* param_flags;
} my_stmt_data_t;

typedef struct mysql_exec_dissector {
	guint8 type;
	guint8 unsigned_flag;
	void (*dissector)(tvbuff_t *tvb, int *param_offset, proto_item *field_tree);
} mysql_exec_dissector_t;

/* function prototypes */
static int mysql_dissect_error_packet(tvbuff_t *tvb, packet_info *pinfo, int offset, proto_tree *tree);
static int mysql_dissect_ok_packet(tvbuff_t *tvb, packet_info *pinfo, int offset, proto_tree *tree, mysql_conn_data_t *conn_data);
static int mysql_dissect_server_status(tvbuff_t *tvb, int offset, proto_tree *tree, guint16 *server_status);
static int mysql_dissect_caps(tvbuff_t *tvb, int offset, proto_tree *tree, int mysql_caps, guint16 *caps);
static int mysql_dissect_extcaps(tvbuff_t *tvb, int offset, proto_tree *tree, int mysql_extcaps, guint16 *caps);
static int mysql_dissect_result_header(tvbuff_t *tvb, packet_info *pinfo, int offset, proto_tree *tree, mysql_conn_data_t *conn_data);
static int mysql_dissect_field_packet(tvbuff_t *tvb, int offset, proto_tree *tree, mysql_conn_data_t *conn_data);
static int mysql_dissect_row_packet(tvbuff_t *tvb, int offset, proto_tree *tree);
static int mysql_dissect_response_prepare(tvbuff_t *tvb, int offset, proto_tree *tree, mysql_conn_data_t *conn_data);
static int mysql_dissect_auth_switch_request(tvbuff_t *tvb, packet_info *pinfo, int offset, proto_tree *tree, mysql_conn_data_t *conn_data);
static int mysql_dissect_auth_switch_response(tvbuff_t *tvb, packet_info *pinfo, int offset, proto_tree *tree, mysql_conn_data_t *conn_data);
static void mysql_dissect_exec_string(tvbuff_t *tvb, int *param_offset, proto_item *field_tree);
static void mysql_dissect_exec_datetime(tvbuff_t *tvb, int *param_offset, proto_item *field_tree);
static void mysql_dissect_exec_tiny(tvbuff_t *tvb, int *param_offset, proto_item *field_tree);
static void mysql_dissect_exec_short(tvbuff_t *tvb, int *param_offset, proto_item *field_tree);
static void mysql_dissect_exec_long(tvbuff_t *tvb, int *param_offset, proto_item *field_tree);
static void mysql_dissect_exec_float(tvbuff_t *tvb, int *param_offset, proto_item *field_tree);
static void mysql_dissect_exec_double(tvbuff_t *tvb, int *param_offset, proto_item *field_tree);
static void mysql_dissect_exec_longlong(tvbuff_t *tvb, int *param_offset, proto_item *field_tree);
static void mysql_dissect_exec_null(tvbuff_t *tvb, int *param_offset, proto_item *field_tree);
static char mysql_dissect_exec_param(proto_item *req_tree, tvbuff_t *tvb, int *offset,
		int *param_offset, guint8 param_flags, packet_info *pinfo);
static void mysql_dissect_exec_primitive(tvbuff_t *tvb, int *param_offset,
		proto_item *field_tree, const int hfindex, const int offset);
static void mysql_dissect_exec_time(tvbuff_t *tvb, int *param_offset, proto_item *field_tree);

static gint my_tvb_strsize(tvbuff_t *tvb, int offset);
static int tvb_get_fle(tvbuff_t *tvb, int offset, guint64 *res, guint8 *is_null);

static const mysql_exec_dissector_t mysql_exec_dissectors[] = {
	{ 0x01, 0, mysql_dissect_exec_tiny },
	{ 0x02, 0, mysql_dissect_exec_short },
	{ 0x03, 0, mysql_dissect_exec_long },
	{ 0x04, 0, mysql_dissect_exec_float },
	{ 0x05, 0, mysql_dissect_exec_double },
	{ 0x06, 0, mysql_dissect_exec_null },
	{ 0x07, 0, mysql_dissect_exec_datetime },
	{ 0x08, 0, mysql_dissect_exec_longlong },
	{ 0x0a, 0, mysql_dissect_exec_datetime },
	{ 0x0b, 0, mysql_dissect_exec_time },
	{ 0x0c, 0, mysql_dissect_exec_datetime },
	{ 0xf6, 0, mysql_dissect_exec_string },
	{ 0xfc, 0, mysql_dissect_exec_string },
	{ 0xfd, 0, mysql_dissect_exec_string },
	{ 0xfe, 0, mysql_dissect_exec_string },
	{ 0x00, 0, NULL },
};

static const int *mysql_rfsh_flags[] = {
	&hf_mysql_rfsh_grants,
	&hf_mysql_rfsh_log,
	&hf_mysql_rfsh_tables,
	&hf_mysql_rfsh_hosts,
	&hf_mysql_rfsh_status,
	&hf_mysql_rfsh_threads,
	&hf_mysql_rfsh_slave,
	&hf_mysql_rfsh_master,
	NULL
};

static const int *mysql_stat_flags[] = {
	&hf_mysql_stat_it,
	&hf_mysql_stat_ac,
	&hf_mysql_stat_mr,
	&hf_mysql_stat_mu,
	&hf_mysql_stat_bi,
	&hf_mysql_stat_ni,
	&hf_mysql_stat_cr,
	&hf_mysql_stat_lr,
	&hf_mysql_stat_dr,
	&hf_mysql_stat_bs,
	&hf_mysql_stat_session_state_changed,
	&hf_mysql_stat_query_was_slow,
	&hf_mysql_stat_ps_out_params,
	NULL
};

static const int *mysql_caps_flags[] = {
	&hf_mysql_cap_long_password,
	&hf_mysql_cap_found_rows,
	&hf_mysql_cap_long_flag,
	&hf_mysql_cap_connect_with_db,
	&hf_mysql_cap_no_schema,
	&hf_mysql_cap_compress,
	&hf_mysql_cap_odbc,
	&hf_mysql_cap_local_files,
	&hf_mysql_cap_ignore_space,
	&hf_mysql_cap_change_user,
	&hf_mysql_cap_interactive,
	&hf_mysql_cap_ssl,
	&hf_mysql_cap_ignore_sigpipe,
	&hf_mysql_cap_transactions,
	&hf_mysql_cap_reserved,
	&hf_mysql_cap_secure_connect,
	NULL
};

static const int * mysql_extcaps_flags[] = {
	&hf_mysql_cap_multi_statements,
	&hf_mysql_cap_multi_results,
	&hf_mysql_cap_ps_multi_results,
	&hf_mysql_cap_plugin_auth,
	&hf_mysql_cap_connect_attrs,
	&hf_mysql_cap_plugin_auth_lenenc_client_data,
	&hf_mysql_cap_client_can_handle_expired_passwords,
	&hf_mysql_cap_session_track,
	&hf_mysql_cap_deprecate_eof,
	&hf_mysql_cap_unused,
	NULL
};

static const int * mysql_fld_flags[] = {
	&hf_mysql_fld_not_null,
	&hf_mysql_fld_primary_key,
	&hf_mysql_fld_unique_key,
	&hf_mysql_fld_multiple_key,
	&hf_mysql_fld_blob,
	&hf_mysql_fld_unsigned,
	&hf_mysql_fld_zero_fill,
	&hf_mysql_fld_binary,
	&hf_mysql_fld_enum,
	&hf_mysql_fld_auto_increment,
	&hf_mysql_fld_timestamp,
	&hf_mysql_fld_set,
	NULL
};

static int
mysql_dissect_greeting(tvbuff_t *tvb, packet_info *pinfo, int offset,
		       proto_tree *tree, mysql_conn_data_t *conn_data)
{
	gint protocol;
	gint lenstr;
	int ver_offset;

	proto_item *tf;
	proto_item *greeting_tree;

	protocol= tvb_get_guint8(tvb, offset);

	if (protocol == 0xff) {
		return mysql_dissect_error_packet(tvb, pinfo, offset+1, tree);
	}

	conn_data->state= LOGIN;

	tf = proto_tree_add_item(tree, hf_mysql_server_greeting, tvb, offset, -1, ENC_NA);
	greeting_tree = proto_item_add_subtree(tf, ett_server_greeting);

	col_append_fstr(pinfo->cinfo, COL_INFO, " proto=%d", protocol) ;

	proto_tree_add_item(greeting_tree, hf_mysql_protocol, tvb, offset, 1, ENC_NA);

	offset += 1;

	/* version string */
	lenstr = tvb_strsize(tvb,offset);
	col_append_fstr(pinfo->cinfo, COL_INFO, " version=%s",
			tvb_format_text(tvb, offset, lenstr-1));

	proto_tree_add_item(greeting_tree, hf_mysql_version, tvb, offset, lenstr, ENC_ASCII|ENC_NA);
	conn_data->major_version = 0;
	for (ver_offset = 0; ver_offset < lenstr; ver_offset++) {
		guint8 ver_char = tvb_get_guint8(tvb, offset + ver_offset);
		if (ver_char == '.') break;
		conn_data->major_version = conn_data->major_version * 10 + ver_char - '0';
	}
	offset += lenstr;

	/* 4 bytes little endian thread_id */
	proto_tree_add_item(greeting_tree, hf_mysql_thread_id, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	offset += 4;

	/* salt string */
	lenstr = tvb_strsize(tvb,offset);
	proto_tree_add_item(greeting_tree, hf_mysql_salt, tvb, offset, lenstr, ENC_ASCII|ENC_NA);
	offset += lenstr;

	/* rest is optional */
	if (!tvb_reported_length_remaining(tvb, offset)) return offset;

	/* 2 bytes CAPS */
	offset = mysql_dissect_caps(tvb, offset, greeting_tree, hf_mysql_caps_server, &conn_data->srv_caps);

	/* rest is optional */
	if (!tvb_reported_length_remaining(tvb, offset)) return offset;

	proto_tree_add_item(greeting_tree, hf_mysql_server_language, tvb, offset, 1, ENC_NA);
	offset += 1; /* for charset */

	offset = mysql_dissect_server_status(tvb, offset, greeting_tree, NULL);

	/* 2 bytes ExtCAPS */
	offset = mysql_dissect_extcaps(tvb, offset, greeting_tree, hf_mysql_extcaps_server, &conn_data->srv_caps_ext);

	/* 1 byte Auth Plugin Length */
	proto_tree_add_item(greeting_tree, hf_mysql_auth_plugin_length, tvb, offset, 1, ENC_NA);
	offset += 1;

	/* 10 bytes unused */
	proto_tree_add_item(greeting_tree, hf_mysql_unused, tvb, offset, 10, ENC_NA);
	offset += 10;

	/* 4.1+ server: rest of salt */
	if (tvb_reported_length_remaining(tvb, offset)) {
		lenstr = tvb_strsize(tvb,offset);
		proto_tree_add_item(greeting_tree, hf_mysql_salt2, tvb, offset, lenstr, ENC_ASCII|ENC_NA);
		offset += lenstr;
	}

	/* 5.x server: auth plugin */
	if (tvb_reported_length_remaining(tvb, offset)) {
		lenstr = tvb_strsize(tvb,offset);
		proto_tree_add_item(greeting_tree, hf_mysql_auth_plugin, tvb, offset, lenstr, ENC_ASCII|ENC_NA);
		offset += lenstr;
	}

	return offset;
}


/*
  Add a connect attributs entry to the connattrs subtree

  return bytes read
*/
static int
add_connattrs_entry_to_tree(tvbuff_t *tvb, packet_info *pinfo _U_, proto_item *tree, int offset) {
	guint64 lenstr;
	int orig_offset = offset, lenfle;
	proto_item *ti;
	proto_tree *connattrs_tree;
	const guint8 *str;

	ti = proto_tree_add_item(tree, hf_mysql_connattrs_attr, tvb, offset, 1, ENC_NA);
	connattrs_tree = proto_item_add_subtree(ti, ett_connattrs_attr);

	lenfle = tvb_get_fle(tvb, offset, &lenstr, NULL);
	proto_tree_add_uint64(connattrs_tree, hf_mysql_connattrs_name_length, tvb, offset, lenfle, lenstr);
	offset += lenfle;

	proto_tree_add_item_ret_string(connattrs_tree, hf_mysql_connattrs_name, tvb, offset, (gint)lenstr, ENC_ASCII|ENC_NA, wmem_packet_scope(), &str);
	proto_item_append_text(ti, " - %s", str);
	offset += (int)lenstr;

	lenfle = tvb_get_fle(tvb, offset, &lenstr, NULL);
	proto_tree_add_uint64(connattrs_tree, hf_mysql_connattrs_value_length, tvb, offset, lenfle, lenstr);
	offset += lenfle;

	proto_tree_add_item_ret_string(connattrs_tree, hf_mysql_connattrs_value, tvb, offset, (gint)lenstr, ENC_ASCII|ENC_NA, wmem_packet_scope(), &str);
	proto_item_append_text(ti, ": %s", str);
	offset += (int)lenstr;

	proto_item_set_len(ti, offset - orig_offset);

	return (offset - orig_offset);
}

static int
mysql_dissect_login(tvbuff_t *tvb, packet_info *pinfo, int offset,
		    proto_tree *tree, mysql_conn_data_t *conn_data)
{
	gint lenstr;

	proto_item *tf;
	proto_item *login_tree;

	/* after login there can be OK or DENIED */
	conn_data->state = RESPONSE_OK;

	tf = proto_tree_add_item(tree, hf_mysql_login_request, tvb, offset, -1, ENC_NA);
	login_tree = proto_item_add_subtree(tf, ett_login_request);

	offset = mysql_dissect_caps(tvb, offset, login_tree, hf_mysql_caps_client, &conn_data->clnt_caps);

	if (!(conn_data->frame_start_ssl) && conn_data->clnt_caps & MYSQL_CAPS_SL) /* Next packet will be use SSL */
	{
		col_set_str(pinfo->cinfo, COL_INFO, "Response: SSL Handshake");
		conn_data->frame_start_ssl = pinfo->num;
		ssl_starttls_ack(ssl_handle, pinfo, mysql_handle);
	}
	if (conn_data->clnt_caps & MYSQL_CAPS_CU) /* 4.1 protocol */
	{
		offset = mysql_dissect_extcaps(tvb, offset, login_tree, hf_mysql_extcaps_client, &conn_data->clnt_caps_ext);

		proto_tree_add_item(login_tree, hf_mysql_max_packet, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		offset += 4;

		proto_tree_add_item(login_tree, hf_mysql_charset, tvb, offset, 1, ENC_NA);
		offset += 1; /* for charset */

		offset += 23; /* filler bytes */

	} else { /* pre-4.1 */
		proto_tree_add_item(login_tree, hf_mysql_max_packet, tvb, offset, 3, ENC_LITTLE_ENDIAN);
		offset += 3;
	}

	/* User name */
	lenstr = my_tvb_strsize(tvb, offset);
	col_append_fstr(pinfo->cinfo, COL_INFO, " user=%s",
			tvb_format_text(tvb, offset, lenstr-1));
	proto_tree_add_item(login_tree, hf_mysql_user, tvb, offset, lenstr, ENC_ASCII|ENC_NA);
	offset += lenstr;

	/* rest is optional */
	if (!tvb_reported_length_remaining(tvb, offset)) return offset;

	/* password: asciiz or length+ascii */
	if (conn_data->clnt_caps & MYSQL_CAPS_SC) {
		lenstr = tvb_get_guint8(tvb, offset);
		offset += 1;
	} else {
		lenstr = my_tvb_strsize(tvb, offset);
	}
	if (tree && lenstr > 1) {
		proto_tree_add_item(login_tree, hf_mysql_passwd, tvb, offset, lenstr, ENC_NA);
	}
	offset += lenstr;

	/* optional: initial schema */
	if (conn_data->clnt_caps & MYSQL_CAPS_CD)
	{
		lenstr= my_tvb_strsize(tvb,offset);
		if(lenstr<0){
			return offset;
		}

		col_append_fstr(pinfo->cinfo, COL_INFO, " db=%s",
			tvb_format_text(tvb, offset, lenstr-1));

		proto_tree_add_item(login_tree, hf_mysql_schema, tvb, offset, lenstr, ENC_ASCII|ENC_NA);
		offset += lenstr;
	}

	/* optional: authentication plugin */
	if (conn_data->clnt_caps_ext & MYSQL_CAPS_PA)
	{
		conn_data->state = AUTH_SWITCH_REQUEST;
		lenstr= my_tvb_strsize(tvb,offset);
		proto_tree_add_item(login_tree, hf_mysql_client_auth_plugin, tvb, offset, lenstr, ENC_ASCII|ENC_NA);
		offset += lenstr;
	}

	/* optional: connection attributes */
	if (conn_data->clnt_caps_ext & MYSQL_CAPS_CA && tvb_reported_length_remaining(tvb, offset))
	{
		proto_tree *connattrs_tree;
		int lenfle;
		guint64 connattrs_length;
		int length;

		lenfle = tvb_get_fle(tvb, offset, &connattrs_length, NULL);
		tf = proto_tree_add_item(login_tree, hf_mysql_connattrs, tvb, offset, (guint32)connattrs_length, ENC_NA);
		connattrs_tree = proto_item_add_subtree(tf, ett_connattrs);
		proto_tree_add_uint64(connattrs_tree, hf_mysql_connattrs_length, tvb, offset, lenfle, connattrs_length);
		offset += lenfle;

		while (connattrs_length > 0) {
			length = add_connattrs_entry_to_tree(tvb, pinfo, connattrs_tree, offset);
			offset += length;
			connattrs_length -= length;
		}
	}

	return offset;
}


static void
mysql_dissect_exec_string(tvbuff_t *tvb, int *param_offset, proto_item *field_tree)
{
	guint32 param_len32;
	guint8 param_len;

	param_len = tvb_get_guint8(tvb, *param_offset);

	switch (param_len) {
		case 0xfc: /* 252 - 64k chars */
			*param_offset += 1;
			param_len32 = tvb_get_letohs(tvb, *param_offset);
			proto_tree_add_item(field_tree, hf_mysql_exec_field_string,
					    tvb, *param_offset, 2, ENC_ASCII | ENC_LITTLE_ENDIAN);
			*param_offset += param_len32 + 2;
			break;
		case 0xfd: /* 64k - 16M chars */
			*param_offset += 1;
			param_len32 = tvb_get_letoh24(tvb, *param_offset);
			proto_tree_add_item(field_tree, hf_mysql_exec_field_string,
					    tvb, *param_offset, 3, ENC_ASCII | ENC_LITTLE_ENDIAN);
			*param_offset += param_len32 + 3;
			break;
		default: /* < 252 chars */
			proto_tree_add_item(field_tree, hf_mysql_exec_field_string,
					    tvb, *param_offset, 1, ENC_ASCII | ENC_NA);
			*param_offset += param_len + 1;
			break;
	}
}

static void
mysql_dissect_exec_time(tvbuff_t *tvb, int *param_offset, proto_item *field_tree)
{
	guint8 param_len;

	param_len = tvb_get_guint8(tvb, *param_offset);
	proto_tree_add_item(field_tree, hf_mysql_exec_field_time_length,
			    tvb, *param_offset, 1, ENC_NA);
	*param_offset += 1;
	if (param_len >= 1) {
		proto_tree_add_item(field_tree, hf_mysql_exec_field_time_sign,
				    tvb, *param_offset, 1, ENC_NA);
	}
	if (param_len >= 5) {
		proto_tree_add_item(field_tree, hf_mysql_exec_field_time_days,
				    tvb, *param_offset + 1, 4, ENC_LITTLE_ENDIAN);
	}
	if (param_len >= 8) {
		proto_tree_add_item(field_tree, hf_mysql_exec_field_hour,
				    tvb, *param_offset + 5, 1, ENC_NA);
		proto_tree_add_item(field_tree, hf_mysql_exec_field_minute,
				    tvb, *param_offset + 6, 1, ENC_NA);
		proto_tree_add_item(field_tree, hf_mysql_exec_field_second,
				    tvb, *param_offset + 7, 1, ENC_NA);
	}
	if (param_len >= 12) {
		proto_tree_add_item(field_tree, hf_mysql_exec_field_second_b,
				    tvb, *param_offset + 8, 4, ENC_LITTLE_ENDIAN);
	}
	*param_offset += param_len;
}

static void
mysql_dissect_exec_datetime(tvbuff_t *tvb, int *param_offset, proto_item *field_tree)
{
	guint8 param_len;

	param_len = tvb_get_guint8(tvb, *param_offset);
	proto_tree_add_item(field_tree, hf_mysql_exec_field_datetime_length,
			    tvb, *param_offset, 1, ENC_NA);
	*param_offset += 1;
	if (param_len >= 2) {
		proto_tree_add_item(field_tree, hf_mysql_exec_field_year,
				    tvb, *param_offset, 2, ENC_LITTLE_ENDIAN);
	}
	if (param_len >= 4) {
		proto_tree_add_item(field_tree, hf_mysql_exec_field_month,
				    tvb, *param_offset + 2, 1, ENC_NA);
		proto_tree_add_item(field_tree, hf_mysql_exec_field_day,
				    tvb, *param_offset + 3, 1, ENC_NA);
	}
	if (param_len >= 7) {
		proto_tree_add_item(field_tree, hf_mysql_exec_field_hour,
				    tvb, *param_offset + 4, 1, ENC_NA);
		proto_tree_add_item(field_tree, hf_mysql_exec_field_minute,
				    tvb, *param_offset + 5, 1, ENC_NA);
		proto_tree_add_item(field_tree, hf_mysql_exec_field_second,
				    tvb, *param_offset + 6, 1, ENC_NA);
	}
	if (param_len >= 11) {
		proto_tree_add_item(field_tree, hf_mysql_exec_field_second_b,
				    tvb, *param_offset + 7, 4, ENC_LITTLE_ENDIAN);
	}
	*param_offset += param_len;
}

static void
mysql_dissect_exec_primitive(tvbuff_t *tvb, int *param_offset,
			     proto_item *field_tree, const int hfindex,
			     const int offset)
{
	proto_tree_add_item(field_tree, hfindex, tvb,
			    *param_offset, offset, ENC_LITTLE_ENDIAN);
	*param_offset += offset;
}

static void
mysql_dissect_exec_tiny(tvbuff_t *tvb, int *param_offset, proto_item *field_tree)
{
	mysql_dissect_exec_primitive(tvb, param_offset, field_tree, hf_mysql_exec_field_tiny, 1);
}

static void
mysql_dissect_exec_short(tvbuff_t *tvb, int *param_offset, proto_item *field_tree)
{
	mysql_dissect_exec_primitive(tvb, param_offset, field_tree, hf_mysql_exec_field_short, 2);
}

static void
mysql_dissect_exec_long(tvbuff_t *tvb, int *param_offset, proto_item *field_tree)
{
	mysql_dissect_exec_primitive(tvb, param_offset, field_tree, hf_mysql_exec_field_long, 4);
}

static void
mysql_dissect_exec_float(tvbuff_t *tvb, int *param_offset, proto_item *field_tree)
{
	mysql_dissect_exec_primitive(tvb, param_offset, field_tree, hf_mysql_exec_field_float, 4);
}

static void
mysql_dissect_exec_double(tvbuff_t *tvb, int *param_offset, proto_item *field_tree)
{
	mysql_dissect_exec_primitive(tvb, param_offset, field_tree, hf_mysql_exec_field_double, 8);
}

static void
mysql_dissect_exec_longlong(tvbuff_t *tvb, int *param_offset, proto_item *field_tree)
{
	mysql_dissect_exec_primitive(tvb, param_offset, field_tree, hf_mysql_exec_field_longlong, 8);
}

static void
mysql_dissect_exec_null(tvbuff_t *tvb _U_, int *param_offset _U_, proto_item *field_tree _U_)
{}

static char
mysql_dissect_exec_param(proto_item *req_tree, tvbuff_t *tvb, int *offset,
			 int *param_offset, guint8 param_flags,
			 packet_info *pinfo)
{
	guint8 param_type, param_unsigned;
	proto_item *tf;
	proto_item *field_tree;
	int dissector_index = 0;

	tf = proto_tree_add_item(req_tree, hf_mysql_exec_param, tvb, *offset, 2, ENC_NA);
	field_tree = proto_item_add_subtree(tf, ett_stat);
	proto_tree_add_item(field_tree, hf_mysql_fld_type, tvb, *offset, 1, ENC_NA);
	param_type = tvb_get_guint8(tvb, *offset);
	*offset += 1; /* type */
	proto_tree_add_item(field_tree, hf_mysql_exec_unsigned, tvb, *offset, 1, ENC_NA);
	param_unsigned = tvb_get_guint8(tvb, *offset);
	*offset += 1; /* signedness */
	if ((param_flags & MYSQL_PARAM_FLAG_STREAMED) == MYSQL_PARAM_FLAG_STREAMED) {
		expert_add_info(pinfo, field_tree, &ei_mysql_streamed_param);
		return 1;
	}
	while (mysql_exec_dissectors[dissector_index].dissector != NULL) {
		if (mysql_exec_dissectors[dissector_index].type == param_type &&
		    mysql_exec_dissectors[dissector_index].unsigned_flag == param_unsigned) {
			mysql_exec_dissectors[dissector_index].dissector(tvb, param_offset, field_tree);
			return 1;
		}
		dissector_index++;
	}
	return 0;
}

static int
mysql_dissect_request(tvbuff_t *tvb,packet_info *pinfo, int offset,
		      proto_tree *tree, mysql_conn_data_t *conn_data)
{
	gint opcode;
	gint lenstr;
	proto_item *tf = NULL, *ti;
	proto_item *req_tree;
	guint32 stmt_id;
	my_stmt_data_t *stmt_data;
	int stmt_pos, param_offset;

	if(conn_data->state == AUTH_SWITCH_RESPONSE){
		return mysql_dissect_auth_switch_response(tvb, pinfo, offset, tree, conn_data);
	}

	tf = proto_tree_add_item(tree, hf_mysql_request, tvb, offset, 1, ENC_NA);
	req_tree = proto_item_add_subtree(tf, ett_request);

	opcode = tvb_get_guint8(tvb, offset);
	col_append_fstr(pinfo->cinfo, COL_INFO, " %s", val_to_str(opcode, mysql_command_vals, "Unknown (%u)"));

	proto_tree_add_item(req_tree, hf_mysql_command, tvb, offset, 1, ENC_NA);
	proto_item_append_text(tf, " %s", val_to_str(opcode, mysql_command_vals, "Unknown (%u)"));
	offset += 1;


	switch (opcode) {

	case MYSQL_QUIT:
		break;

	case MYSQL_PROCESS_INFO:
		conn_data->state = RESPONSE_TABULAR;
		break;

	case MYSQL_DEBUG:
	case MYSQL_PING:
		conn_data->state = RESPONSE_OK;
		break;

	case MYSQL_STATISTICS:
		conn_data->state = RESPONSE_MESSAGE;
		break;

	case MYSQL_INIT_DB:
	case MYSQL_CREATE_DB:
	case MYSQL_DROP_DB:
		lenstr = my_tvb_strsize(tvb, offset);
		proto_tree_add_item(req_tree, hf_mysql_schema, tvb, offset, lenstr, ENC_ASCII|ENC_NA);
		offset += lenstr;
		conn_data->state = RESPONSE_OK;
		break;

	case MYSQL_QUERY:
		lenstr = my_tvb_strsize(tvb, offset);
		proto_tree_add_item(req_tree, hf_mysql_query, tvb, offset, lenstr, ENC_ASCII|ENC_NA);
		if (mysql_showquery) {
			col_append_fstr(pinfo->cinfo, COL_INFO, " { %s } ",
					tvb_format_text(tvb, offset, lenstr));
		}
		offset += lenstr;
		conn_data->state = RESPONSE_TABULAR;
		break;

	case MYSQL_STMT_PREPARE:
		lenstr = my_tvb_strsize(tvb, offset);
		proto_tree_add_item(req_tree, hf_mysql_query, tvb, offset, lenstr, ENC_ASCII|ENC_NA);
		offset += lenstr;
		conn_data->state = RESPONSE_PREPARE;
		break;

	case MYSQL_STMT_CLOSE:
		proto_tree_add_item(req_tree, hf_mysql_stmt_id, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		offset += 4;
		conn_data->state = REQUEST;
		break;

	case MYSQL_STMT_RESET:
		proto_tree_add_item(req_tree, hf_mysql_stmt_id, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		offset += 4;
		conn_data->state = RESPONSE_OK;
		break;

	case MYSQL_FIELD_LIST:
		lenstr = my_tvb_strsize(tvb, offset);
		proto_tree_add_item(req_tree, hf_mysql_table_name, tvb,  offset, lenstr, ENC_ASCII|ENC_NA);
		offset += lenstr;
		conn_data->state = RESPONSE_SHOW_FIELDS;
		break;

	case MYSQL_PROCESS_KILL:
		proto_tree_add_item(req_tree, hf_mysql_thd_id, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		offset += 4;
		conn_data->state = RESPONSE_OK;
		break;

	case MYSQL_CHANGE_USER:
		lenstr = tvb_strsize(tvb, offset);
		proto_tree_add_item(req_tree, hf_mysql_user, tvb,  offset, lenstr, ENC_ASCII|ENC_NA);
		offset += lenstr;

		if (conn_data->clnt_caps & MYSQL_CAPS_SC) {
			lenstr = tvb_get_guint8(tvb, offset);
			offset += 1;
		} else {
			lenstr = tvb_strsize(tvb, offset);
		}
		proto_tree_add_item(req_tree, hf_mysql_passwd, tvb, offset, lenstr, ENC_NA);
		offset += lenstr;

		lenstr = my_tvb_strsize(tvb, offset);
		proto_tree_add_item(req_tree, hf_mysql_schema, tvb, offset, lenstr, ENC_ASCII|ENC_NA);
		offset += lenstr;

		if (tvb_reported_length_remaining(tvb, offset) > 0) {
			proto_tree_add_item(req_tree, hf_mysql_charset, tvb, offset, 1, ENC_NA);
			offset += 2; /* for charset */
		}
		conn_data->state = RESPONSE_OK;

		/* optional: authentication plugin */
		if (conn_data->clnt_caps_ext & MYSQL_CAPS_PA)
		{
			conn_data->state = AUTH_SWITCH_REQUEST;
			lenstr= my_tvb_strsize(tvb,offset);
			proto_tree_add_item(req_tree, hf_mysql_client_auth_plugin, tvb, offset, lenstr, ENC_ASCII|ENC_NA);
			offset += lenstr;
		}

		/* optional: connection attributes */
		if (conn_data->clnt_caps_ext & MYSQL_CAPS_CA)
		{
			proto_tree *connattrs_tree;
			int lenfle;
			guint64 connattrs_length;
			int length;

			lenfle = tvb_get_fle(tvb, offset, &connattrs_length, NULL);
			tf = proto_tree_add_item(req_tree, hf_mysql_connattrs, tvb, offset, (guint32)connattrs_length, ENC_NA);
			connattrs_tree = proto_item_add_subtree(tf, ett_connattrs);
			proto_tree_add_uint64(connattrs_tree, hf_mysql_connattrs_length, tvb, offset, lenfle, connattrs_length);
			offset += lenfle;

			while (connattrs_length > 0) {
				length = add_connattrs_entry_to_tree(tvb, pinfo, connattrs_tree, offset);
				offset += length;
				connattrs_length -= length;
			}
		}
		break;

	case MYSQL_REFRESH:
		proto_tree_add_bitmask_with_flags(req_tree, tvb, offset,
hf_mysql_refresh, ett_refresh, mysql_rfsh_flags, ENC_BIG_ENDIAN, BMT_NO_APPEND);
		offset += 1;
		conn_data->state= RESPONSE_OK;
		break;

	case MYSQL_SHUTDOWN:
		proto_tree_add_item(req_tree, hf_mysql_shutdown, tvb, offset, 1, ENC_NA);
		offset += 1;
		conn_data->state = RESPONSE_OK;
		break;

	case MYSQL_SET_OPTION:
		proto_tree_add_item(req_tree, hf_mysql_option, tvb, offset, 2, ENC_LITTLE_ENDIAN);
		offset += 2;
		conn_data->state = RESPONSE_OK;
		break;

	case MYSQL_STMT_FETCH:
		proto_tree_add_item(req_tree, hf_mysql_stmt_id, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		offset += 4;

		proto_tree_add_item(req_tree, hf_mysql_num_rows, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		offset += 4;
		conn_data->state = RESPONSE_TABULAR;
		break;

	case MYSQL_STMT_SEND_LONG_DATA:
		proto_tree_add_item(req_tree, hf_mysql_stmt_id, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		stmt_id = tvb_get_letohl(tvb, offset);
		offset += 4;

		stmt_data = (my_stmt_data_t *)wmem_tree_lookup32(conn_data->stmts, stmt_id);
		if (stmt_data != NULL) {
			guint16 data_param = tvb_get_letohs(tvb, offset);
			if (stmt_data->nparam > data_param) {
				stmt_data->param_flags[data_param] |= MYSQL_PARAM_FLAG_STREAMED;
			}
		}

		proto_tree_add_item(req_tree, hf_mysql_param, tvb, offset, 2, ENC_LITTLE_ENDIAN);
		offset += 2;

		/* rest is data */
		lenstr = tvb_reported_length_remaining(tvb, offset);
		if (tree &&  lenstr > 0) {
			proto_tree_add_item(req_tree, hf_mysql_payload, tvb, offset, lenstr, ENC_NA);
		}
		offset += lenstr;
		conn_data->state = REQUEST;
		break;

	case MYSQL_STMT_EXECUTE:
		proto_tree_add_item(req_tree, hf_mysql_stmt_id, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		stmt_id = tvb_get_letohl(tvb, offset);
		offset += 4;

		if (conn_data->major_version >= 5) {
			proto_tree_add_item(req_tree, hf_mysql_exec_flags5, tvb, offset, 1, ENC_NA);
		} else {
			proto_tree_add_item(req_tree, hf_mysql_exec_flags4, tvb, offset, 1, ENC_NA);
		}
		offset += 1;

		proto_tree_add_item(req_tree, hf_mysql_exec_iter, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		offset += 4;

		stmt_data = (my_stmt_data_t *)wmem_tree_lookup32(conn_data->stmts, stmt_id);
		if (stmt_data != NULL) {
			if (stmt_data->nparam != 0) {
				guint8 stmt_bound;
				offset += (stmt_data->nparam + 7) / 8; /* NULL bitmap */
				proto_tree_add_item(req_tree, hf_mysql_new_parameter_bound_flag, tvb, offset, 1, ENC_NA);
				stmt_bound = tvb_get_guint8(tvb, offset);
				offset += 1;
				if (stmt_bound == 1) {
					param_offset = offset + stmt_data->nparam * 2;
					for (stmt_pos = 0; stmt_pos < stmt_data->nparam; stmt_pos++) {
						if (!mysql_dissect_exec_param(req_tree, tvb, &offset, &param_offset,
									      stmt_data->param_flags[stmt_pos], pinfo))
							break;
					}
					offset = param_offset;
				}
			}
		} else {
			lenstr = tvb_reported_length_remaining(tvb, offset);
			if (tree &&  lenstr > 0) {
				ti = proto_tree_add_item(req_tree, hf_mysql_payload, tvb, offset, lenstr, ENC_NA);
				expert_add_info(pinfo, ti, &ei_mysql_prepare_response_needed);
			}
			offset += lenstr;
		}
#if 0
/* FIXME: rest needs metadata about statement */
#else
		lenstr = tvb_reported_length_remaining(tvb, offset);
		if (tree &&  lenstr > 0) {
			ti = proto_tree_add_item(req_tree, hf_mysql_payload, tvb, offset, lenstr, ENC_NA);
			expert_add_info_format(pinfo, ti, &ei_mysql_dissector_incomplete, "FIXME: execute dissector incomplete");
		}
		offset += lenstr;
#endif
		conn_data->state= RESPONSE_TABULAR;
		break;

	case MYSQL_BINLOG_DUMP:
		proto_tree_add_item(req_tree, hf_mysql_binlog_position, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		offset += 4;

		proto_tree_add_item(req_tree, hf_mysql_binlog_flags, tvb, offset, 2, ENC_BIG_ENDIAN);
		offset += 2;

		proto_tree_add_item(req_tree, hf_mysql_binlog_server_id, tvb, offset, 4, ENC_LITTLE_ENDIAN);
		offset += 4;

		/* binlog file name ? */
		lenstr = tvb_reported_length_remaining(tvb, offset);
		if (tree &&  lenstr > 0) {
			proto_tree_add_item(req_tree, hf_mysql_binlog_file_name, tvb, offset, lenstr, ENC_ASCII|ENC_NA);
		}
		offset += lenstr;

		conn_data->state = REQUEST;
		break;
/* FIXME: implement replication packets */
	case MYSQL_TABLE_DUMP:
	case MYSQL_CONNECT_OUT:
	case MYSQL_REGISTER_SLAVE:
		ti = proto_tree_add_item(req_tree, hf_mysql_payload, tvb, offset, -1, ENC_NA);
		expert_add_info_format(pinfo, ti, &ei_mysql_dissector_incomplete, "FIXME: implement replication packets");
		offset += tvb_reported_length_remaining(tvb, offset);
		conn_data->state = REQUEST;
		break;

	default:
		ti = proto_tree_add_item(req_tree, hf_mysql_payload, tvb, offset, -1, ENC_NA);
		expert_add_info(pinfo, ti, &ei_mysql_command);
		offset += tvb_reported_length_remaining(tvb, offset);
		conn_data->state = UNDEFINED;
	}

	return offset;
}

/*
 * Decode the header of a compressed packet
 * https://dev.mysql.com/doc/internals/en/compressed-packet-header.html
 */
static int
mysql_dissect_compressed_header(tvbuff_t *tvb, int offset, proto_tree *mysql_tree)
{
	proto_tree_add_item(mysql_tree, hf_mysql_compressed_packet_length, tvb, offset, 3, ENC_LITTLE_ENDIAN);
	offset += 3;

	proto_tree_add_item(mysql_tree, hf_mysql_compressed_packet_number, tvb, offset, 1, ENC_NA);
	offset += 1;

	proto_tree_add_item(mysql_tree, hf_mysql_compressed_packet_length_uncompressed, tvb, offset, 3, ENC_LITTLE_ENDIAN);
	offset += 3;

	return offset;
}

static int
mysql_dissect_response(tvbuff_t *tvb, packet_info *pinfo, int offset,
		       proto_tree *tree, mysql_conn_data_t *conn_data)
{
	gint response_code;
	gint lenstr;
	proto_item *ti;
	guint16 server_status = 0;

	response_code = tvb_get_guint8(tvb, offset);

	if (response_code == 0xff ) {
		offset = mysql_dissect_error_packet(tvb, pinfo, offset+1, tree);
		conn_data->state= REQUEST;
	}

	else if (response_code == 0xfe && tvb_reported_length_remaining(tvb, offset) < 9) {

		ti = proto_tree_add_item(tree, hf_mysql_eof, tvb, offset, 1, ENC_NA);

		offset += 1;

		/* pre-4.1 packet ends here */
		if (tvb_reported_length_remaining(tvb, offset)) {
			proto_tree_add_item(tree, hf_mysql_num_warn, tvb, offset, 2, ENC_LITTLE_ENDIAN);
			offset = mysql_dissect_server_status(tvb, offset+2, tree, &server_status);
		}

		if (conn_data->state == FIELD_PACKET) {
			conn_data->state= ROW_PACKET;
		} else if (conn_data->state == ROW_PACKET) {
			if (server_status & MYSQL_STAT_MU) {
				conn_data->state= RESPONSE_TABULAR;
			} else {
				conn_data->state= REQUEST;
			}
		} else if (conn_data->state == PREPARED_PARAMETERS) {
			if (conn_data->stmt_num_fields > 0) {
				conn_data->state= PREPARED_FIELDS;
			} else {
				conn_data->state= REQUEST;
			}
		} else if (conn_data->state == PREPARED_FIELDS) {
			conn_data->state= REQUEST;
		} else {
			/* This should be an unreachable case */
			conn_data->state= REQUEST;
			expert_add_info(pinfo, ti, &ei_mysql_eof);
		}
	}

	else if (response_code == 0) {
		if (conn_data->state == RESPONSE_PREPARE) {
			offset = mysql_dissect_response_prepare(tvb, offset, tree, conn_data);
		} else if (tvb_reported_length_remaining(tvb, offset+1)  > tvb_get_fle(tvb, offset+1, NULL, NULL)) {
			offset = mysql_dissect_ok_packet(tvb, pinfo, offset+1, tree, conn_data);
			if (conn_data->compressed_state == MYSQL_COMPRESS_INIT) {
				/* This is the OK packet which follows the compressed protocol setup */
				conn_data->compressed_state = MYSQL_COMPRESS_ACTIVE;
			}
		} else {
			offset = mysql_dissect_result_header(tvb, pinfo, offset, tree, conn_data);
		}
	}

	else {
		switch (conn_data->state) {
		case RESPONSE_MESSAGE:
			if ((lenstr = tvb_reported_length_remaining(tvb, offset))) {
				proto_tree_add_item(tree, hf_mysql_message, tvb, offset, lenstr, ENC_ASCII|ENC_NA);
				offset += lenstr;
			}
			conn_data->state = REQUEST;
			break;

		case RESPONSE_TABULAR:
			offset = mysql_dissect_result_header(tvb, pinfo, offset, tree, conn_data);
			break;

		case FIELD_PACKET:
		case RESPONSE_SHOW_FIELDS:
		case RESPONSE_PREPARE:
		case PREPARED_PARAMETERS:
			offset = mysql_dissect_field_packet(tvb, offset, tree, conn_data);
			break;

		case ROW_PACKET:
			offset = mysql_dissect_row_packet(tvb, offset, tree);
			break;

		case PREPARED_FIELDS:
			offset = mysql_dissect_field_packet(tvb, offset, tree, conn_data);
			break;

		case AUTH_SWITCH_REQUEST:
			offset = mysql_dissect_auth_switch_request(tvb, pinfo, offset, tree, conn_data);
			break;


		default:
			ti = proto_tree_add_item(tree, hf_mysql_payload, tvb, offset, -1, ENC_NA);
			expert_add_info(pinfo, ti, &ei_mysql_unknown_response);
			offset += tvb_reported_length_remaining(tvb, offset);
			conn_data->state = UNDEFINED;
		}
	}

	return offset;
}


static int
mysql_dissect_error_packet(tvbuff_t *tvb, packet_info *pinfo,
			   int offset, proto_tree *tree)
{
	col_append_fstr(pinfo->cinfo, COL_INFO, " Error %d", tvb_get_letohs(tvb, offset));

	proto_tree_add_item(tree, hf_mysql_error_code, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	if (tvb_get_guint8(tvb, offset) == '#')
	{
		offset += 1;
		proto_tree_add_item(tree, hf_mysql_sqlstate, tvb, offset, 5, ENC_ASCII|ENC_NA);
		offset += 5;
	}

	proto_tree_add_item(tree, hf_mysql_error_string, tvb, offset, -1, ENC_ASCII|ENC_NA);
	offset += tvb_reported_length_remaining(tvb, offset);

	return offset;
}

/*
  Add a session track entry to the session tracking subtree

  return bytes read
*/
static int
add_session_tracker_entry_to_tree(tvbuff_t *tvb, packet_info *pinfo, proto_item *tree, int offset) {
	guint8 data_type; /* session tracker type */
	guint64 length; /* complete length of session tracking entry */
	guint64 lenstr;
	int orig_offset = offset, lenfle;
	proto_item *item, *ti;
	proto_tree *session_track_tree;

	ti = proto_tree_add_item(tree, hf_mysql_session_track, tvb, offset, 1, ENC_NA);
	session_track_tree = proto_item_add_subtree(ti, ett_session_track);

	proto_tree_add_item(session_track_tree, hf_mysql_session_track_type, tvb, offset, 1, ENC_BIG_ENDIAN);
	data_type = tvb_get_guint8(tvb, offset);
	offset += 1;

	lenfle = tvb_get_fle(tvb, offset, &length, NULL);
	proto_tree_add_uint64(session_track_tree, hf_mysql_session_track_length, tvb, offset, lenfle, length);
	offset += lenfle;

	switch (data_type) {
	case 0: /* SESSION_SYSVARS_TRACKER */
		lenfle = tvb_get_fle(tvb, offset, &lenstr, NULL);
		proto_tree_add_uint64(session_track_tree, hf_mysql_session_track_sysvar_length, tvb, offset, lenfle, lenstr);
		offset += lenfle;

		proto_tree_add_item(session_track_tree, hf_mysql_session_track_sysvar_name, tvb, offset, (gint)lenstr, ENC_ASCII|ENC_NA);
		offset += (int)lenstr;

		lenfle = tvb_get_fle(tvb, offset, &lenstr, NULL);
		proto_tree_add_uint64(session_track_tree, hf_mysql_session_track_sysvar_length, tvb, offset, lenfle, lenstr);
		offset += lenfle;

		proto_tree_add_item(session_track_tree, hf_mysql_session_track_sysvar_value, tvb, offset, (gint)lenstr, ENC_ASCII|ENC_NA);
		offset += (int)lenstr;
		break;
	case 1: /* CURRENT_SCHEMA_TRACKER */
		lenfle = tvb_get_fle(tvb, offset, &lenstr, NULL);
		proto_tree_add_uint64(session_track_tree, hf_mysql_session_track_schema_length, tvb, offset, lenfle, lenstr);
		offset += lenfle;

		proto_tree_add_item(session_track_tree, hf_mysql_session_track_schema, tvb, offset, (gint)lenstr, ENC_ASCII|ENC_NA);
		offset += (int)lenstr;
		break;
	case 2: /* SESSION_STATE_CHANGE_TRACKER */
		proto_tree_add_item(session_track_tree, hf_mysql_session_state_change, tvb, offset, 1, ENC_ASCII|ENC_NA);
		offset++;
		break;
	default: /* unsupported types skipped */
		item = proto_tree_add_item(session_track_tree, hf_mysql_payload, tvb, offset, (gint)length, ENC_NA);
		expert_add_info_format(pinfo, item, &ei_mysql_dissector_incomplete, "FIXME: unrecognized session tracker data");
		offset += (int)length;
	}
	proto_item_set_len(ti, offset - orig_offset);

	return (offset - orig_offset);
}

static int
mysql_dissect_ok_packet(tvbuff_t *tvb, packet_info *pinfo, int offset,
			proto_tree *tree, mysql_conn_data_t *conn_data)
{
	guint64 lenstr;
	guint64 affected_rows;
	guint64 insert_id;
	int fle;
	guint16 server_status = 0;

	col_append_str(pinfo->cinfo, COL_INFO, " OK" );

	fle = tvb_get_fle(tvb, offset, &affected_rows, NULL);
	proto_tree_add_uint64(tree, hf_mysql_affected_rows, tvb, offset, fle, affected_rows);
	offset += fle;

	fle= tvb_get_fle(tvb, offset, &insert_id, NULL);
	if (tree && insert_id) {
		proto_tree_add_uint64(tree, hf_mysql_insert_id, tvb, offset, fle, insert_id);
	}
	offset += fle;

	if (tvb_reported_length_remaining(tvb, offset) > 0) {
		offset = mysql_dissect_server_status(tvb, offset, tree, &server_status);

		/* 4.1+ protocol only: 2 bytes number of warnings */
		if (conn_data->clnt_caps & conn_data->srv_caps & MYSQL_CAPS_CU) {
			proto_tree_add_item(tree, hf_mysql_num_warn, tvb, offset, 2, ENC_LITTLE_ENDIAN);
		offset += 2;
		}
	}

	if (conn_data->clnt_caps_ext & MYSQL_CAPS_ST) {
		if (tvb_reported_length_remaining(tvb, offset) > 0) {
			guint64 session_track_length;
			proto_item *tf;
			proto_item *session_track_tree = NULL;
			int length;

			offset += tvb_get_fle(tvb, offset, &lenstr, NULL);
			/* first read the optional message */
			if (lenstr) {
				proto_tree_add_item(tree, hf_mysql_message, tvb, offset, (gint)lenstr, ENC_ASCII|ENC_NA);
				offset += (int)lenstr;
			}

			/* session state tracking */
			if (server_status & MYSQL_STAT_SESSION_STATE_CHANGED) {
				fle = tvb_get_fle(tvb, offset, &session_track_length, NULL);
				tf = proto_tree_add_item(tree, hf_mysql_session_track_data, tvb, offset, -1, ENC_NA);
				session_track_tree = proto_item_add_subtree(tf, ett_session_track_data);
				proto_tree_add_uint64(tf, hf_mysql_session_track_data_length, tvb, offset, fle, session_track_length);
				offset += fle;

				while (session_track_length > 0) {
					length = add_session_tracker_entry_to_tree(tvb, pinfo, session_track_tree, offset);
					offset += length;
					session_track_length -= length;
				}
			}
		}
	} else {
		/* optional: message string */
		if (tvb_reported_length_remaining(tvb, offset) > 0) {
			lenstr = tvb_reported_length_remaining(tvb, offset);
			proto_tree_add_item(tree, hf_mysql_message, tvb, offset, (gint)lenstr, ENC_ASCII|ENC_NA);
			offset += (int)lenstr;
		}
	}

	conn_data->state = REQUEST;
	return offset;
}


static int
mysql_dissect_server_status(tvbuff_t *tvb, int offset, proto_tree *tree, guint16 *server_status)
{

	if (server_status) {
		*server_status = tvb_get_letohs(tvb, offset);
	}
	proto_tree_add_bitmask_with_flags(tree, tvb, offset, hf_mysql_server_status, ett_stat, mysql_stat_flags, ENC_LITTLE_ENDIAN, BMT_NO_APPEND);

	offset += 2;

	return offset;
}


static int
mysql_dissect_caps(tvbuff_t *tvb, int offset, proto_tree *tree, int mysql_caps, guint16 *caps)
{

	*caps= tvb_get_letohs(tvb, offset);

	proto_tree_add_bitmask_with_flags(tree, tvb, offset, mysql_caps, ett_caps, mysql_caps_flags, ENC_LITTLE_ENDIAN, BMT_NO_APPEND);

	offset += 2;
	return offset;
}

static int
mysql_dissect_extcaps(tvbuff_t *tvb, int offset, proto_tree *tree, int mysql_extcaps, guint16 *ext_caps)
{

	*ext_caps= tvb_get_letohs(tvb, offset);

	proto_tree_add_bitmask_with_flags(tree, tvb, offset, mysql_extcaps, ett_extcaps, mysql_extcaps_flags, ENC_LITTLE_ENDIAN, BMT_NO_APPEND);

	offset += 2;
	return offset;
}


static int
mysql_dissect_result_header(tvbuff_t *tvb, packet_info *pinfo, int offset,
			    proto_tree *tree, mysql_conn_data_t *conn_data)
{
	gint fle;
	guint64 num_fields, extra;

	col_append_str(pinfo->cinfo, COL_INFO, " TABULAR" );

	fle = tvb_get_fle(tvb, offset, &num_fields, NULL);
	proto_tree_add_uint64(tree, hf_mysql_num_fields, tvb, offset, fle, num_fields);
	offset += fle;

	if (tvb_reported_length_remaining(tvb, offset)) {
		fle = tvb_get_fle(tvb, offset, &extra, NULL);
		proto_tree_add_uint64(tree, hf_mysql_extra, tvb, offset, fle, extra);
		offset += fle;
	}

	if (num_fields) {
		conn_data->state = FIELD_PACKET;
	} else {
		conn_data->state = ROW_PACKET;
	}

	return offset;
}


/*
 * Add length encoded string to tree
 */
static int
mysql_field_add_lestring(tvbuff_t *tvb, int offset, proto_tree *tree, int field)
{
	guint64 lelen;
	guint8 is_null;

	offset += tvb_get_fle(tvb, offset, &lelen, &is_null);
	if(is_null)
		proto_tree_add_string(tree, field, tvb, offset, 4, "NULL");
	else
	{
		proto_tree_add_item(tree, field, tvb, offset, (int)lelen, ENC_NA);
		/* Prevent infinite loop due to overflow */
		if (offset + (int)lelen < offset) {
			offset = tvb_reported_length(tvb);
		}
		else {
			offset += (int)lelen;
		}
	}
	return offset;
}


static int
mysql_dissect_field_packet(tvbuff_t *tvb, int offset, proto_tree *tree, mysql_conn_data_t *conn_data _U_)
{

	offset = mysql_field_add_lestring(tvb, offset, tree, hf_mysql_fld_catalog);
	offset = mysql_field_add_lestring(tvb, offset, tree, hf_mysql_fld_db);
	offset = mysql_field_add_lestring(tvb, offset, tree, hf_mysql_fld_table);
	offset = mysql_field_add_lestring(tvb, offset, tree, hf_mysql_fld_org_table);
	offset = mysql_field_add_lestring(tvb, offset, tree, hf_mysql_fld_name);
	offset = mysql_field_add_lestring(tvb, offset, tree, hf_mysql_fld_org_name);
	offset +=1; /* filler */

	proto_tree_add_item(tree, hf_mysql_fld_charsetnr, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2; /* charset */

	proto_tree_add_item(tree, hf_mysql_fld_length, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	offset += 4; /* length */

	proto_tree_add_item(tree, hf_mysql_fld_type, tvb, offset, 1, ENC_NA);
	offset += 1; /* type */

	proto_tree_add_bitmask_with_flags(tree, tvb, offset, hf_mysql_fld_flags, ett_field_flags, mysql_fld_flags, ENC_LITTLE_ENDIAN, BMT_NO_APPEND);
	offset += 2; /* flags */

	proto_tree_add_item(tree, hf_mysql_fld_decimals, tvb, offset, 1, ENC_NA);
	offset += 1; /* decimals */

	offset += 2; /* filler */

	/* default (Only use for show fields) */
	if (tree && tvb_reported_length_remaining(tvb, offset) > 0) {
		offset = mysql_field_add_lestring(tvb, offset, tree, hf_mysql_fld_default);
	}
	return offset;
}


static int
mysql_dissect_row_packet(tvbuff_t *tvb, int offset, proto_tree *tree)
{
	while (tvb_reported_length_remaining(tvb, offset) > 0) {
		offset = mysql_field_add_lestring(tvb, offset, tree, hf_mysql_row_text);
	}

	return offset;
}


static int
mysql_dissect_response_prepare(tvbuff_t *tvb, int offset, proto_tree *tree, mysql_conn_data_t *conn_data)
{
	my_stmt_data_t *stmt_data;
	guint32 stmt_id;
	int flagsize;

	/* 0, marker for OK packet */
	offset += 1;
	proto_tree_add_item(tree, hf_mysql_stmt_id, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	stmt_id = tvb_get_letohl(tvb, offset);
	offset += 4;
	proto_tree_add_item(tree, hf_mysql_num_fields, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	conn_data->stmt_num_fields = tvb_get_letohs(tvb, offset);
	offset += 2;
	proto_tree_add_item(tree, hf_mysql_num_params, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	conn_data->stmt_num_params = tvb_get_letohs(tvb, offset);
	stmt_data = wmem_new(wmem_file_scope(), struct my_stmt_data);
	stmt_data->nparam = conn_data->stmt_num_params;
	flagsize = (int)(sizeof(guint8) * stmt_data->nparam);
	stmt_data->param_flags = (guint8 *)wmem_alloc(wmem_file_scope(), flagsize);
	memset(stmt_data->param_flags, 0, flagsize);
	wmem_tree_insert32(conn_data->stmts, stmt_id, stmt_data);
	offset += 2;
	/* Filler */
	offset += 1;
	proto_tree_add_item(tree, hf_mysql_num_warn, tvb, offset, 2, ENC_LITTLE_ENDIAN);

	if (conn_data->stmt_num_params > 0)
		conn_data->state = PREPARED_PARAMETERS;
	else if (conn_data->stmt_num_fields > 0)
		conn_data->state = PREPARED_FIELDS;
	else
		conn_data->state = REQUEST;

	return offset + tvb_reported_length_remaining(tvb, offset);
}


static int
mysql_dissect_auth_switch_request(tvbuff_t *tvb, packet_info *pinfo, int offset, proto_tree *tree, mysql_conn_data_t *conn_data _U_)
{
	gint lenstr;

	col_set_str(pinfo->cinfo, COL_INFO, "Auth Switch Request" );
	conn_data->state = AUTH_SWITCH_RESPONSE;

	/* Status (Always 0xfe) */
	proto_tree_add_item(tree, hf_mysql_auth_switch_request_status, tvb, offset, 1, ENC_LITTLE_ENDIAN);
	offset += 1;

	/* name */
	lenstr = my_tvb_strsize(tvb, offset);
	proto_tree_add_item(tree, hf_mysql_auth_switch_request_name, tvb, offset, lenstr, ENC_ASCII|ENC_NA);
	offset += lenstr;

	/* Data */
	lenstr = my_tvb_strsize(tvb, offset);
	proto_tree_add_item(tree, hf_mysql_auth_switch_request_data, tvb, offset, lenstr, ENC_NA);
	offset += lenstr;

	return offset + tvb_reported_length_remaining(tvb, offset);

}
static int
mysql_dissect_auth_switch_response(tvbuff_t *tvb, packet_info *pinfo, int offset, proto_tree *tree, mysql_conn_data_t *conn_data _U_)
{
	gint lenstr;

	col_set_str(pinfo->cinfo, COL_INFO, "Auth Switch Response" );

	/* Data */
	lenstr = my_tvb_strsize(tvb, offset);
	proto_tree_add_item(tree, hf_mysql_auth_switch_response_data, tvb, offset, lenstr, ENC_NA);
	offset += lenstr;

	return offset + tvb_reported_length_remaining(tvb, offset);

}
/*
 get length of string in packet buffer

 SYNOPSIS
   my_tvb_strsize()
     tvb      packet buffer
     offset   current offset

 DESCRIPTION
   deliver length of string, delimited by either \0 or end of buffer

 RETURN VALUE
   length of string found, including \0 (if present)

*/
static gint
my_tvb_strsize(tvbuff_t *tvb, int offset)
{
	gint len = tvb_strnlen(tvb, offset, -1);
	if (len == -1) {
		len = tvb_reported_length_remaining(tvb, offset);
	} else {
		len++; /* the trailing \0 */
	}
	return len;
}

/*
 read "field length encoded" value from packet buffer

 SYNOPSIS
   tvb_get_fle()
     tvb     in    packet buffer
     offset  in    offset in buffer
     res     out   where to store FLE value, may be NULL
     is_null out   where to store ISNULL flag, may be NULL

 DESCRIPTION
   read FLE from packet buffer and store its value and ISNULL flag
   in caller provided variables

 RETURN VALUE
   length of FLE
*/
static int
tvb_get_fle(tvbuff_t *tvb, int offset, guint64 *res, guint8 *is_null)
{
	guint8 prefix;

	prefix = tvb_get_guint8(tvb, offset);

	if (is_null)
		*is_null = 0;

	switch (prefix) {
	case 251:
		if (res)
			*res = 0;
		if (is_null)
			*is_null = 1;
		break;
	case 252:
		if (res)
			*res = tvb_get_letohs(tvb, offset+1);
		return 3;
	case 253:
		if (res)
			*res = tvb_get_letohl(tvb, offset+1);
		return 5;
	case 254:
		if (res)
			*res = tvb_get_letoh64(tvb, offset+1);
		return 9;
	default:
		if (res)
			*res = prefix;
	}

	return 1;
}

/* dissector helper: length of PDU */
static guint
get_mysql_pdu_len(packet_info *pinfo _U_, tvbuff_t *tvb, int offset, void *data _U_)
{
	int tvb_remain= tvb_reported_length_remaining(tvb, offset);
	guint plen= tvb_get_letoh24(tvb, offset);

	if ((tvb_remain - plen) == 7) {
		return plen + 7; /* compressed header 3+1+3 (len+id+cmp_len) */
	} else {
		return plen + 4; /* regular header 3+1 (len+id) */
	}
}

/* dissector main function: handle one PDU */
static int
dissect_mysql_pdu(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
	proto_tree      *mysql_tree= NULL;
	proto_item      *ti;
	conversation_t  *conversation;
	int             offset = 0;
	guint           packet_number;
	gboolean        is_response, is_ssl = FALSE;
	mysql_conn_data_t  *conn_data;
#ifdef CTDEBUG
	mysql_state_t conn_state_in, conn_state_out, frame_state;
	guint64         generation;
	proto_item *pi;
#endif
	struct mysql_frame_data  *mysql_frame_data_p;

	/* get conversation, create if necessary*/
	conversation= find_or_create_conversation(pinfo);

	/* get associated state information, create if necessary */
	conn_data= (mysql_conn_data_t *)conversation_get_proto_data(conversation, proto_mysql);
	if (!conn_data) {
		conn_data= wmem_new(wmem_file_scope(), mysql_conn_data_t);
		conn_data->srv_caps= 0;
		conn_data->clnt_caps= 0;
		conn_data->clnt_caps_ext= 0;
		conn_data->state= UNDEFINED;
		conn_data->stmts= wmem_tree_new(wmem_file_scope());
#ifdef CTDEBUG
		conn_data->generation= 0;
#endif
		conn_data->major_version= 0;
		conn_data->frame_start_ssl= 0;
		conn_data->frame_start_compressed= 0;
		conn_data->compressed_state= MYSQL_COMPRESS_NONE;
		conversation_add_proto_data(conversation, proto_mysql, conn_data);
	}

	mysql_frame_data_p = (struct mysql_frame_data *)p_get_proto_data(wmem_file_scope(), pinfo, proto_mysql, 0);
	if (!mysql_frame_data_p) {
		/*  We haven't seen this frame before.  Store the state of the
		 *  conversation now so if/when we dissect the frame again
		 *  we'll start with the same state.
		 */
		mysql_frame_data_p = wmem_new(wmem_file_scope(), struct mysql_frame_data);
		mysql_frame_data_p->state = conn_data->state;
		p_add_proto_data(wmem_file_scope(), pinfo, proto_mysql, 0, mysql_frame_data_p);

	} else if (conn_data->state != FIELD_PACKET  && conn_data->state != ROW_PACKET ) {
		/*  We have seen this frame before.  Set the connection state
		 *  to whatever state it had the first time we saw this frame
		 *  (e.g., based on whatever frames came before it).
		 *  The state may change as we dissect this packet.
		 *  XXX: I think the logic of the above else if test is as follows:
		 *       During the first (sequential) dissection pass thru the capture
		 *       file the conversation connection state as of the beginning of each frame
		 *       is saved in the connection_state for that frame.
		 *       Any state changes *within* a mysql "message" (ie: query/response/etc)
		 *       while processing mysql PDUS (aka "packets") in that message must be preserved.
		 *       It appears that FIELD_PACKET & ROW_PACKET are the only two
		 *       state changes which can occur within a mysql message which affect
		 *       subsequent processing within the message.
		 *       Question: Does this logic work OK for a reassembled message ?
		 */
		 conn_data->state= mysql_frame_data_p->state;
	}

	if ((conn_data->frame_start_compressed) && (pinfo->num > conn_data->frame_start_compressed)) {
		if (conn_data->compressed_state == MYSQL_COMPRESS_ACTIVE) {
			offset = mysql_dissect_compressed_header(tvb, offset, tree);
		}
	}

	if (tree) {
		ti = proto_tree_add_item(tree, proto_mysql, tvb, offset, -1, ENC_NA);
		mysql_tree = proto_item_add_subtree(ti, ett_mysql);
		proto_tree_add_item(mysql_tree, hf_mysql_packet_length, tvb, offset, 3, ENC_LITTLE_ENDIAN);
	}
	offset+= 3;

	col_set_str(pinfo->cinfo, COL_PROTOCOL, "MySQL");

	if (pinfo->destport == pinfo->match_uint) {
		is_response= FALSE;
	} else {
		is_response= TRUE;
	}

	packet_number = tvb_get_guint8(tvb, offset);
	proto_tree_add_item(mysql_tree, hf_mysql_packet_number, tvb, offset, 1, ENC_NA);
	offset += 1;

#ifdef CTDEBUG
	conn_state_in= conn_data->state;
	frame_state = mysql_frame_data_p->state;
	generation= conn_data->generation;
	if (tree) {
		pi = proto_tree_add_debug_text(mysql_tree, "conversation: %p", conversation);
		PROTO_ITEM_SET_GENERATED(pi);
		pi = proto_tree_add_debug_text(mysql_tree, "generation: %" G_GINT64_MODIFIER "d", generation);
		PROTO_ITEM_SET_GENERATED(pi);
		pi = proto_tree_add_debug_text(mysql_tree, "conn state: %s (%u)",
				    val_to_str(conn_state_in, state_vals, "Unknown (%u)"),
				    conn_state_in);
		PROTO_ITEM_SET_GENERATED(pi);
		pi = proto_tree_add_debug_text(mysql_tree, "frame state: %s (%u)",
				    val_to_str(frame_state, state_vals, "Unknown (%u)"),
				    frame_state);
		PROTO_ITEM_SET_GENERATED(pi);
	}
#endif

	is_ssl = proto_is_frame_protocol(pinfo->layers, "ssl");

	if (is_response) {
		if (packet_number == 0 ) {
			col_set_str(pinfo->cinfo, COL_INFO, "Server Greeting");
			offset = mysql_dissect_greeting(tvb, pinfo, offset, mysql_tree, conn_data);
		} else {
			col_set_str(pinfo->cinfo, COL_INFO, "Response");
			offset = mysql_dissect_response(tvb, pinfo, offset, mysql_tree, conn_data);
		}
	} else {
		if (packet_number == 1 || (packet_number == 2 && is_ssl)) {
			col_set_str(pinfo->cinfo, COL_INFO, "Login Request");
			offset = mysql_dissect_login(tvb, pinfo, offset, mysql_tree, conn_data);
			if (conn_data->srv_caps & MYSQL_CAPS_CP) {
				if (conn_data->clnt_caps & MYSQL_CAPS_CP) {
					conn_data->frame_start_compressed = pinfo->num;
					conn_data->compressed_state = MYSQL_COMPRESS_INIT;
				}
			}
		} else {
			col_set_str(pinfo->cinfo, COL_INFO, "Request");
			offset = mysql_dissect_request(tvb, pinfo, offset, mysql_tree, conn_data);
		}
	}

#ifdef CTDEBUG
	conn_state_out= conn_data->state;
	++(conn_data->generation);
	pi = proto_tree_add_debug_text(mysql_tree, "next proto state: %s (%u)",
			    val_to_str(conn_state_out, state_vals, "Unknown (%u)"),
			    conn_state_out);
	PROTO_ITEM_SET_GENERATED(pi);
#endif

	/* remaining payload indicates an error */
	if (tree && tvb_reported_length_remaining(tvb, offset) > 0) {
		ti = proto_tree_add_item(mysql_tree, hf_mysql_payload, tvb, offset, -1, ENC_NA);
		expert_add_info(pinfo, ti, &ei_mysql_dissector_incomplete);
	}

	return tvb_reported_length(tvb);
}

/* dissector entrypoint, handles TCP-desegmentation */
static int
dissect_mysql(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data)
{
	tcp_dissect_pdus(tvb, pinfo, tree, mysql_desegment, 3,
			 get_mysql_pdu_len, dissect_mysql_pdu, data);

	return tvb_reported_length(tvb);
}

/* protocol registration */
void proto_register_mysql(void)
{
	static hf_register_info hf[]=
	{
		{ &hf_mysql_packet_length,
		{ "Packet Length", "mysql.packet_length",
		FT_UINT24, BASE_DEC, NULL,  0x0,
		NULL, HFILL }},

		{ &hf_mysql_packet_number,
		{ "Packet Number", "mysql.packet_number",
		FT_UINT8, BASE_DEC, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_request,
		{ "Request Command", "mysql.request",
		FT_NONE, BASE_NONE, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_command,
		{ "Command", "mysql.command",
		FT_UINT8, BASE_DEC, VALS(mysql_command_vals), 0x0,
		NULL, HFILL }},

		{ &hf_mysql_error_code,
		{ "Error Code", "mysql.error_code",
		FT_UINT16, BASE_DEC, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_error_string,
		{ "Error message", "mysql.error.message",
		FT_STRING, BASE_NONE, NULL, 0x0,
		"Error string in case of MySQL error message", HFILL }},

		{ &hf_mysql_sqlstate,
		{ "SQL state", "mysql.sqlstate",
		FT_STRING, BASE_NONE, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_message,
		{ "Message", "mysql.message",
		FT_STRINGZ, BASE_NONE, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_server_greeting,
		{ "Server Greeting", "mysql.server_greeting",
		FT_NONE, BASE_NONE, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_protocol,
		{ "Protocol", "mysql.protocol",
		FT_UINT8, BASE_DEC, NULL, 0x0,
		"Protocol Version", HFILL }},

		{ &hf_mysql_version,
		{ "Version", "mysql.version",
		FT_STRINGZ, BASE_NONE, NULL, 0x0,
		"MySQL Version", HFILL }},

		{ &hf_mysql_session_track,
		{ "Session Track", "mysql.session_track",
		  FT_NONE, BASE_NONE, NULL, 0x0,
		  NULL, HFILL }},

		{ &hf_mysql_session_track_type,
		{ "Session tracking type", "mysql.session_track.type",
		  FT_UINT8, BASE_DEC, VALS(mysql_session_track_type_vals), 0x0,
		  NULL, HFILL }},

		{ &hf_mysql_session_track_length,
		{ "Session tracking length", "mysql.session_track.length",
		  FT_UINT64, BASE_DEC, NULL, 0x0,
		  NULL, HFILL }},

		{ &hf_mysql_session_track_data,
		{ "Session tracking data", "mysql.session_track.data",
		  FT_NONE, BASE_NONE, NULL, 0x0,
		  NULL, HFILL }},

		{ &hf_mysql_session_track_data_length,
		{ "Session tracking data length", "mysql.session_track.data.length",
		  FT_UINT64, BASE_DEC, NULL, 0x0,
		  NULL, HFILL }},

		{ &hf_mysql_session_track_sysvar_length,
		{ "System variable change Length", "mysql.session_track.sysvar.length",
		  FT_UINT64, BASE_DEC, NULL, 0x0,
		  NULL, HFILL }},

		{ &hf_mysql_session_track_sysvar_name,
		{ "System variable change Name", "mysql.session_track.sysvar.name",
		  FT_STRINGZ, BASE_NONE, NULL, 0x0,
		  NULL, HFILL }},

		{ &hf_mysql_session_track_sysvar_value,
		{ "System variable change Value", "mysql.session_track.sysvar.value",
		  FT_STRINGZ, BASE_NONE, NULL, 0x0,
		  NULL, HFILL }},

		{ &hf_mysql_session_track_schema_length,
		{ "Schema change length", "mysql.session_track.schema.length",
		  FT_UINT64, BASE_DEC, NULL, 0x0,
		  NULL, HFILL }},

		{ &hf_mysql_session_track_schema,
		{ "Schema change", "mysql.session_track.schema",
		  FT_STRINGZ, BASE_NONE, NULL, 0x0,
		  NULL, HFILL }},

		{ &hf_mysql_session_state_change,
		{ "State change", "mysql.session_track.state_change",
		  FT_STRINGZ, BASE_NONE, NULL, 0x0,
		  NULL, HFILL }},

		{ &hf_mysql_caps_server,
		{ "Server Capabilities", "mysql.caps.server",
		FT_UINT16, BASE_HEX, NULL, 0x0,
		"MySQL Capabilities", HFILL }},

		{ &hf_mysql_caps_client,
		{ "Client Capabilities", "mysql.caps.client",
		FT_UINT16, BASE_HEX, NULL, 0x0,
		"MySQL Capabilities", HFILL }},

		{ &hf_mysql_cap_long_password,
		{ "Long Password","mysql.caps.lp",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_CAPS_LP,
		NULL, HFILL }},

		{ &hf_mysql_cap_found_rows,
		{ "Found Rows","mysql.caps.fr",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_CAPS_FR,
		NULL, HFILL }},

		{ &hf_mysql_cap_long_flag,
		{ "Long Column Flags","mysql.caps.lf",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_CAPS_LF,
		NULL, HFILL }},

		{ &hf_mysql_cap_connect_with_db,
		{ "Connect With Database","mysql.caps.cd",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_CAPS_CD,
		NULL, HFILL }},

		{ &hf_mysql_cap_no_schema,
		{ "Don't Allow database.table.column","mysql.caps.ns",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_CAPS_NS,
		NULL, HFILL }},

		{ &hf_mysql_cap_compress,
		{ "Can use compression protocol","mysql.caps.cp",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_CAPS_CP,
		NULL, HFILL }},

		{ &hf_mysql_cap_odbc,
		{ "ODBC Client","mysql.caps.ob",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_CAPS_OB,
		NULL, HFILL }},

		{ &hf_mysql_cap_local_files,
		{ "Can Use LOAD DATA LOCAL","mysql.caps.li",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_CAPS_LI,
		NULL, HFILL }},

		{ &hf_mysql_cap_ignore_space,
		{ "Ignore Spaces before '('","mysql.caps.is",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_CAPS_IS,
		NULL, HFILL }},

		{ &hf_mysql_cap_change_user,
		{ "Speaks 4.1 protocol (new flag)","mysql.caps.cu",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_CAPS_CU,
		NULL, HFILL }},

		{ &hf_mysql_cap_interactive,
		{ "Interactive Client","mysql.caps.ia",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_CAPS_IA,
		NULL, HFILL }},

		{ &hf_mysql_cap_ssl,
		{ "Switch to SSL after handshake","mysql.caps.sl",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_CAPS_SL,
		NULL, HFILL }},

		{ &hf_mysql_cap_ignore_sigpipe,
		{ "Ignore sigpipes","mysql.caps.ii",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_CAPS_II,
		NULL, HFILL }},

		{ &hf_mysql_cap_transactions,
		{ "Knows about transactions","mysql.caps.ta",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_CAPS_TA,
		NULL, HFILL }},

		{ &hf_mysql_cap_reserved,
		{ "Speaks 4.1 protocol (old flag)","mysql.caps.rs",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_CAPS_RS,
		NULL, HFILL }},

		{ &hf_mysql_cap_secure_connect,
		{ "Can do 4.1 authentication","mysql.caps.sc",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_CAPS_SC,
		NULL, HFILL }},

		{ &hf_mysql_extcaps_server,
		{ "Extended Server Capabilities", "mysql.extcaps.server",
		FT_UINT16, BASE_HEX, NULL, 0x0,
		"MySQL Extended Capabilities", HFILL }},

		{ &hf_mysql_extcaps_client,
		{ "Extended Client Capabilities", "mysql.extcaps.client",
		FT_UINT16, BASE_HEX, NULL, 0x0,
		"MySQL Extended Capabilities", HFILL }},

		{ &hf_mysql_cap_multi_statements,
		{ "Multiple statements","mysql.caps.ms",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_CAPS_MS,
		NULL, HFILL }},

		{ &hf_mysql_cap_multi_results,
		{ "Multiple results","mysql.caps.mr",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_CAPS_MR,
		NULL, HFILL }},

		{ &hf_mysql_cap_ps_multi_results,
		{ "PS Multiple results","mysql.caps.pm",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_CAPS_PM,
		NULL, HFILL }},

		{ &hf_mysql_cap_plugin_auth,
		{ "Plugin Auth","mysql.caps.pa",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_CAPS_PA,
		NULL, HFILL }},

		{ &hf_mysql_cap_connect_attrs,
		{ "Connect attrs","mysql.caps.ca",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_CAPS_CA,
		NULL, HFILL }},

		{ &hf_mysql_cap_plugin_auth_lenenc_client_data,
		{ "Plugin Auth LENENC Client Data","mysql.caps.pm",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_CAPS_AL,
		NULL, HFILL }},

		{ &hf_mysql_cap_client_can_handle_expired_passwords,
		{ "Client can handle expired passwords","mysql.caps.ep",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_CAPS_EP,
		NULL, HFILL }},

		{ &hf_mysql_cap_session_track,
		{ "Session variable tracking","mysql.caps.session_track",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_CAPS_ST,
		NULL, HFILL }},

		{ &hf_mysql_cap_deprecate_eof,
		{ "Deprecate EOF","mysql.caps.deprecate_eof",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_CAPS_DE,
		NULL, HFILL }},

		{ &hf_mysql_cap_unused,
		{ "Unused","mysql.caps.unused",
		FT_UINT16, BASE_HEX, NULL, MYSQL_CAPS_UNUSED,
		NULL, HFILL }},

		{ &hf_mysql_login_request,
		{ "Login Request", "mysql.login_request",
		FT_NONE, BASE_NONE, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_max_packet,
		{ "MAX Packet", "mysql.max_packet",
		FT_UINT24, BASE_DEC, NULL, 0x0,
		"MySQL Max packet", HFILL }},

		{ &hf_mysql_charset,
		{ "Charset", "mysql.charset",
		FT_UINT8, BASE_DEC, VALS(mysql_collation_vals), 0x0,
		"MySQL Charset", HFILL }},

		{ &hf_mysql_table_name,
		{ "Table Name", "mysql.table_name",
		FT_STRINGZ, BASE_NONE, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_user,
		{ "Username", "mysql.user",
		FT_STRINGZ, BASE_NONE, NULL, 0x0,
		"Login Username", HFILL }},

		{ &hf_mysql_schema,
		{ "Schema", "mysql.schema",
		FT_STRING, BASE_NONE, NULL, 0x0,
		"Login Schema", HFILL }},

		{ &hf_mysql_client_auth_plugin,
		{ "Client Auth Plugin", "mysql.client_auth_plugin",
		FT_STRING, BASE_NONE, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_connattrs,
		{ "Connection Attributes", "mysql.connattrs",
		FT_NONE, BASE_NONE, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_connattrs_length,
		{ "Connection Attributes length", "mysql.connattrs.length",
		  FT_UINT64, BASE_DEC, NULL, 0x0,
		  NULL, HFILL }},

		{ &hf_mysql_connattrs_attr,
		{ "Connection Attribute", "mysql.connattrs.attr",
		  FT_NONE, BASE_NONE, NULL, 0x0,
		  NULL, HFILL }},

		{ &hf_mysql_connattrs_name_length,
		{ "Connection Attribute Name Length", "mysql.connattrs.name.length",
		  FT_UINT64, BASE_DEC, NULL, 0x0,
		  NULL, HFILL }},

		{ &hf_mysql_connattrs_name,
		{ "Connection Attribute Name", "mysql.connattrs.name",
		  FT_STRINGZ, BASE_NONE, NULL, 0x0,
		  NULL, HFILL }},

		{ &hf_mysql_connattrs_value_length,
		{ "Connection Attribute Name Length", "mysql.connattrs.name.length",
		  FT_UINT64, BASE_DEC, NULL, 0x0,
		  NULL, HFILL }},

		{ &hf_mysql_connattrs_value,
		{ "Connection Attribute Value", "mysql.connattrs.value",
		  FT_STRINGZ, BASE_NONE, NULL, 0x0,
		  NULL, HFILL }},

		{ &hf_mysql_salt,
		{ "Salt", "mysql.salt",
		FT_STRINGZ, BASE_NONE, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_salt2,
		{ "Salt", "mysql.salt2",
		FT_STRINGZ, BASE_NONE, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_auth_plugin_length,
		{ "Authentication Plugin Length", "mysql.auth_plugin.length",
		FT_UINT8, BASE_DEC, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_auth_plugin,
		{ "Authentication Plugin", "mysql.auth_plugin",
		FT_STRINGZ, BASE_NONE, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_thread_id,
		{ "Thread ID", "mysql.thread_id",
		FT_UINT32, BASE_DEC, NULL, 0x0,
		"MySQL Thread ID", HFILL }},

		{ &hf_mysql_server_language,
		{ "Server Language", "mysql.server_language",
		FT_UINT8, BASE_DEC, VALS(mysql_collation_vals), 0x0,
		"MySQL Charset", HFILL }},

		{ &hf_mysql_server_status,
		{ "Server Status", "mysql.server_status",
		FT_UINT16, BASE_HEX, NULL, 0x0,
		"MySQL Status", HFILL }},

		{ &hf_mysql_stat_it,
		{ "In transaction", "mysql.stat.it",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_STAT_IT,
		NULL, HFILL }},

		{ &hf_mysql_stat_ac,
		{ "AUTO_COMMIT", "mysql.stat.ac",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_STAT_AC,
		NULL, HFILL }},

		{ &hf_mysql_stat_mr,
		{ "More results", "mysql.stat.mr",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_STAT_MR,
		NULL, HFILL }},

		{ &hf_mysql_stat_mu,
		{ "Multi query - more resultsets", "mysql.stat.mu",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_STAT_MU,
		NULL, HFILL }},

		{ &hf_mysql_stat_bi,
		{ "Bad index used", "mysql.stat.bi",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_STAT_BI,
		NULL, HFILL }},

		{ &hf_mysql_stat_ni,
		{ "No index used", "mysql.stat.ni",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_STAT_NI,
		NULL, HFILL }},

		{ &hf_mysql_stat_cr,
		{ "Cursor exists", "mysql.stat.cr",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_STAT_CR,
		NULL, HFILL }},

		{ &hf_mysql_stat_lr,
		{ "Last row sent", "mysql.stat.lr",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_STAT_LR,
		NULL, HFILL }},

		{ &hf_mysql_stat_dr,
		{ "database dropped", "mysql.stat.dr",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_STAT_DR,
		NULL, HFILL }},

		{ &hf_mysql_stat_bs,
		{ "No backslash escapes", "mysql.stat.bs",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_STAT_BS,
		NULL, HFILL }},

		{ &hf_mysql_stat_session_state_changed,
		{ "Session state changed", "mysql.stat.session_state_changed",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_STAT_SESSION_STATE_CHANGED,
		NULL, HFILL }},

		{ &hf_mysql_stat_query_was_slow,
		{ "Query was slow", "mysql.stat.query_was_slow",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_STAT_QUERY_WAS_SLOW,
		NULL, HFILL }},

		{ &hf_mysql_stat_ps_out_params,
		{ "PS Out Params", "mysql.stat.ps_out_params",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_STAT_PS_OUT_PARAMS,
		NULL, HFILL }},

		{ &hf_mysql_refresh,
		{ "Refresh Option", "mysql.refresh",
		FT_UINT8, BASE_HEX, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_rfsh_grants,
		{ "reload permissions", "mysql.rfsh.grants",
		FT_BOOLEAN, 8, TFS(&tfs_set_notset), MYSQL_RFSH_GRANT,
		NULL, HFILL }},

		{ &hf_mysql_rfsh_log,
		{ "flush logfiles", "mysql.rfsh.log",
		FT_BOOLEAN, 8, TFS(&tfs_set_notset), MYSQL_RFSH_LOG,
		NULL, HFILL }},

		{ &hf_mysql_rfsh_tables,
		{ "flush tables", "mysql.rfsh.tables",
		FT_BOOLEAN, 8, TFS(&tfs_set_notset), MYSQL_RFSH_TABLES,
		NULL, HFILL }},

		{ &hf_mysql_rfsh_hosts,
		{ "flush hosts", "mysql.rfsh.hosts",
		FT_BOOLEAN, 8, TFS(&tfs_set_notset), MYSQL_RFSH_HOSTS,
		NULL, HFILL }},

		{ &hf_mysql_rfsh_status,
		{ "reset statistics", "mysql.rfsh.status",
		FT_BOOLEAN, 8, TFS(&tfs_set_notset), MYSQL_RFSH_STATUS,
		NULL, HFILL }},

		{ &hf_mysql_rfsh_threads,
		{ "empty thread cache", "mysql.rfsh.threads",
		FT_BOOLEAN, 8, TFS(&tfs_set_notset), MYSQL_RFSH_THREADS,
		NULL, HFILL }},

		{ &hf_mysql_rfsh_slave,
		{ "flush slave status", "mysql.rfsh.slave",
		FT_BOOLEAN, 8, TFS(&tfs_set_notset), MYSQL_RFSH_SLAVE,
		NULL, HFILL }},

		{ &hf_mysql_rfsh_master,
		{ "flush master status", "mysql.rfsh.master",
		FT_BOOLEAN, 8, TFS(&tfs_set_notset), MYSQL_RFSH_MASTER,
		NULL, HFILL }},

		{ &hf_mysql_unused,
		{ "Unused", "mysql.unused",
		FT_BYTES, BASE_NONE, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_passwd,
		{ "Password", "mysql.passwd",
		FT_BYTES, BASE_NONE, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_payload,
		{ "Payload", "mysql.payload",
		FT_BYTES, BASE_NONE, NULL, 0x0,
		"Additional Payload", HFILL }},

		{ &hf_mysql_affected_rows,
		{ "Affected Rows", "mysql.affected_rows",
		FT_UINT64, BASE_DEC, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_insert_id,
		{ "Last INSERT ID", "mysql.insert_id",
		FT_UINT64, BASE_DEC, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_num_warn,
		{ "Warnings", "mysql.warnings",
		FT_UINT16, BASE_DEC, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_thd_id,
		{ "Thread ID", "mysql.thd_id",
		FT_UINT32, BASE_DEC, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_stmt_id,
		{ "Statement ID", "mysql.stmt_id",
		FT_UINT32, BASE_DEC, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_query,
		{ "Statement", "mysql.query",
		FT_STRING, BASE_NONE, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_shutdown,
		{ "Shutdown Level", "mysql.shutdown",
		FT_UINT8, BASE_DEC, VALS(mysql_shutdown_vals), 0x0,
		NULL, HFILL }},

		{ &hf_mysql_option,
		{ "Option", "mysql.option",
		FT_UINT16, BASE_DEC, VALS(mysql_option_vals), 0x0,
		NULL, HFILL }},

		{ &hf_mysql_param,
		{ "Parameter", "mysql.param",
		FT_UINT16, BASE_DEC, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_num_params,
		{ "Number of parameter", "mysql.num_params",
		FT_UINT16, BASE_DEC, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_num_rows,
		{ "Rows to fetch", "mysql.num_rows",
		FT_UINT32, BASE_DEC, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_exec_flags4,
		{ "Flags (unused)", "mysql.exec_flags",
		FT_UINT8, BASE_DEC, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_exec_flags5,
		{ "Flags", "mysql.exec_flags",
		FT_UINT8, BASE_DEC, VALS(mysql_exec_flags_vals), 0x0,
		NULL, HFILL }},

		{ &hf_mysql_new_parameter_bound_flag,
		{ "New parameter bound flag", "mysql.new_parameter_bound_flag",
		FT_UINT8, BASE_DEC, VALS(mysql_new_parameter_bound_flag_vals), 0x0,
		NULL, HFILL }},

		{ &hf_mysql_exec_iter,
		{ "Iterations (unused)", "mysql.exec_iter",
		FT_UINT32, BASE_DEC, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_binlog_position,
		{ "Binlog Position", "mysql.binlog.position",
		FT_UINT32, BASE_DEC, NULL, 0x0,
		"Position to start at", HFILL }},

		{ &hf_mysql_binlog_flags,
		{ "Binlog Flags", "mysql.binlog.flags",
		FT_UINT16, BASE_HEX, NULL, 0x0,
		"(currently not used; always 0)", HFILL }},

		{ &hf_mysql_binlog_server_id,
		{ "Binlog server id", "mysql.binlog.server_id",
		FT_UINT16, BASE_HEX, NULL, 0x0,
		"server_id of the slave", HFILL }},

		{ &hf_mysql_binlog_file_name,
		{ "Binlog file name", "mysql.binlog.file_name",
		FT_STRINGZ, BASE_NONE, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_eof,
		{ "EOF marker", "mysql.eof",
		FT_UINT8, BASE_DEC, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_num_fields,
		{ "Number of fields", "mysql.num_fields",
		FT_UINT64, BASE_DEC, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_extra,
		{ "Extra data", "mysql.extra",
		FT_UINT64, BASE_DEC, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_fld_catalog,
		{ "Catalog", "mysql.field.catalog",
		FT_STRING, BASE_NONE, NULL, 0x0,
		"Field: catalog", HFILL }},

		{ &hf_mysql_fld_db,
		{ "Database", "mysql.field.db",
		FT_STRING, BASE_NONE, NULL, 0x0,
		"Field: database", HFILL }},

		{ &hf_mysql_fld_table,
		{ "Table", "mysql.field.table",
		FT_STRING, BASE_NONE, NULL, 0x0,
		"Field: table", HFILL }},

		{ &hf_mysql_fld_org_table,
		{ "Original table", "mysql.field.org_table",
		FT_STRING, BASE_NONE, NULL, 0x0,
		"Field: original table", HFILL }},

		{ &hf_mysql_fld_name,
		{ "Name", "mysql.field.name",
		FT_STRING, BASE_NONE, NULL, 0x0,
		"Field: name", HFILL }},

		{ &hf_mysql_fld_org_name,
		{ "Original name", "mysql.field.org_name",
		FT_STRING, BASE_NONE, NULL, 0x0,
		"Field: original name", HFILL }},

		{ &hf_mysql_fld_charsetnr,
		{ "Charset number", "mysql.field.charsetnr",
		FT_UINT16, BASE_DEC, VALS(mysql_collation_vals), 0x0,
		"Field: charset number", HFILL }},

		{ &hf_mysql_fld_length,
		{ "Length", "mysql.field.length",
		FT_UINT32, BASE_DEC, NULL, 0x0,
		"Field: length", HFILL }},

		{ &hf_mysql_fld_type,
		{ "Type", "mysql.field.type",
		FT_UINT8, BASE_DEC, VALS(type_constants), 0x0,
		"Field: type", HFILL }},

		{ &hf_mysql_fld_flags,
		{ "Flags", "mysql.field.flags",
		FT_UINT16, BASE_HEX, NULL, 0x0,
		"Field: flags", HFILL }},

		{ &hf_mysql_fld_not_null,
		{ "Not null", "mysql.field.flags.not_null",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_FLD_NOT_NULL_FLAG,
		"Field: flag not null", HFILL }},

		{ &hf_mysql_fld_primary_key,
		{ "Primary key", "mysql.field.flags.primary_key",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_FLD_PRI_KEY_FLAG,
		"Field: flag primary key", HFILL }},

		{ &hf_mysql_fld_unique_key,
		{ "Unique key", "mysql.field.flags.unique_key",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_FLD_UNIQUE_KEY_FLAG,
		"Field: flag unique key", HFILL }},

		{ &hf_mysql_fld_multiple_key,
		{ "Multiple key", "mysql.field.flags.multiple_key",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_FLD_MULTIPLE_KEY_FLAG,
		"Field: flag multiple key", HFILL }},

		{ &hf_mysql_fld_blob,
		{ "Blob", "mysql.field.flags.blob",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_FLD_BLOB_FLAG,
		"Field: flag blob", HFILL }},

		{ &hf_mysql_fld_unsigned,
		{ "Unsigned", "mysql.field.flags.unsigned",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_FLD_UNSIGNED_FLAG,
		"Field: flag unsigned", HFILL }},

		{ &hf_mysql_fld_zero_fill,
		{ "Zero fill", "mysql.field.flags.zero_fill",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_FLD_ZEROFILL_FLAG,
		"Field: flag zero fill", HFILL }},

		{ &hf_mysql_fld_binary,
		{ "Binary", "mysql.field.flags.binary",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_FLD_BINARY_FLAG,
		"Field: flag binary", HFILL }},

		{ &hf_mysql_fld_enum,
		{ "Enum", "mysql.field.flags.enum",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_FLD_ENUM_FLAG,
		"Field: flag enum", HFILL }},

		{ &hf_mysql_fld_auto_increment,
		{ "Auto increment", "mysql.field.flags.auto_increment",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_FLD_AUTO_INCREMENT_FLAG,
		"Field: flag auto increment", HFILL }},

		{ &hf_mysql_fld_timestamp,
		{ "Timestamp", "mysql.field.flags.timestamp",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_FLD_TIMESTAMP_FLAG,
		"Field: flag timestamp", HFILL }},

		{ &hf_mysql_fld_set,
		{ "Set", "mysql.field.flags.set",
		FT_BOOLEAN, 16, TFS(&tfs_set_notset), MYSQL_FLD_SET_FLAG,
		"Field: flag set", HFILL }},

		{ &hf_mysql_fld_decimals,
		{ "Decimals", "mysql.field.decimals",
		FT_UINT8, BASE_DEC, NULL, 0x0,
		"Field: decimals", HFILL }},

		{ &hf_mysql_fld_default,
		{ "Default", "mysql.field.default",
		FT_STRING, BASE_NONE, NULL, 0x0,
		"Field: default", HFILL }},

		{ &hf_mysql_row_text,
		{ "text", "mysql.row.text",
		FT_STRING, BASE_NONE, NULL, 0x0,
		"Field: row packet text", HFILL }},

		{ &hf_mysql_exec_param,
		{ "Parameter", "mysql.exec_param",
		FT_NONE, BASE_NONE, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_exec_unsigned,
		{ "Unsigned", "mysql.exec.unsigned",
		FT_UINT8, BASE_DEC, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_exec_field_longlong,
		{ "Value", "mysql.exec.field.longlong",
		FT_INT64, BASE_DEC, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_exec_field_string,
		{ "Value", "mysql.exec.field.string",
		FT_UINT_STRING, BASE_NONE, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_exec_field_double,
		{ "Value", "mysql.exec.field.double",
		FT_DOUBLE, BASE_NONE, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_exec_field_datetime_length,
		{ "Length", "mysql.exec.field.datetime.length",
		FT_INT8, BASE_DEC, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_exec_field_year,
		{ "Year", "mysql.exec.field.year",
		FT_INT16, BASE_DEC, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_exec_field_month,
		{ "Month", "mysql.exec.field.month",
		FT_INT8, BASE_DEC, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_exec_field_day,
		{ "Day", "mysql.exec.field.day",
		FT_INT8, BASE_DEC, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_exec_field_hour,
		{ "Hour", "mysql.exec.field.hour",
		FT_INT8, BASE_DEC, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_exec_field_minute,
		{ "Minute", "mysql.exec.field.minute",
		FT_INT8, BASE_DEC, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_exec_field_second,
		{ "Second", "mysql.exec.field.second",
		FT_INT8, BASE_DEC, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_exec_field_second_b,
		{ "Billionth of a second", "mysql.exec.field.secondb",
		FT_INT32, BASE_DEC, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_exec_field_long,
		{ "Value", "mysql.exec.field.long",
		FT_INT32, BASE_DEC, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_exec_field_tiny,
		{ "Value", "mysql.exec.field.tiny",
		FT_INT8, BASE_DEC, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_exec_field_short,
		{ "Value", "mysql.exec.field.short",
		FT_INT16, BASE_DEC, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_exec_field_float,
		{ "Value", "mysql.exec.field.float",
		FT_FLOAT, BASE_NONE, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_exec_field_time_length,
		{ "Length", "mysql.exec.field.time.length",
		FT_INT8, BASE_DEC, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_exec_field_time_sign,
		{ "Flags", "mysql.exec.field.time.sign",
		FT_UINT8, BASE_DEC, VALS(mysql_exec_time_sign_vals), 0x0,
		NULL, HFILL }},

		{ &hf_mysql_exec_field_time_days,
		{ "Days", "mysql.exec.field.time.days",
		FT_INT32, BASE_DEC, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_auth_switch_request_status,
		{ "Status", "mysql.auth_switch_request.status",
		FT_UINT8, BASE_HEX, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_auth_switch_request_name,
		{ "Auth Method Name", "mysql.auth_switch_request.name",
		FT_STRING, BASE_NONE, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_auth_switch_request_data,
		{ "Auth Method Data", "mysql.auth_switch_request.data",
		FT_BYTES, BASE_NONE, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_auth_switch_response_data,
		{ "Auth Method Data", "mysql.auth_switch_response.data",
		FT_BYTES, BASE_NONE, NULL, 0x0,
		NULL, HFILL }},

		{ &hf_mysql_compressed_packet_length,
		{ "Compressed Packet Length", "mysql.compressed_packet_length",
		FT_UINT24, BASE_DEC, NULL,  0x0,
		NULL, HFILL }},

		{ &hf_mysql_compressed_packet_number,
		{ "Compressed Packet Number", "mysql.compressed_packet_number",
		FT_UINT24, BASE_DEC, NULL,  0x0,
		NULL, HFILL }},

		{ &hf_mysql_compressed_packet_length_uncompressed,
		{ "Uncompressed Packet Length", "mysql.compressed_packet_length_uncompressed",
		FT_UINT24, BASE_DEC, NULL,  0x0,
		NULL, HFILL }},
	};

	static gint *ett[]=
	{
		&ett_mysql,
		&ett_server_greeting,
		&ett_login_request,
		&ett_caps,
		&ett_extcaps,
		&ett_stat,
		&ett_request,
		&ett_refresh,
		&ett_field_flags,
		&ett_exec_param,
		&ett_session_track,
		&ett_session_track_data,
		&ett_connattrs,
		&ett_connattrs_attr
	};

	static ei_register_info ei[] = {
		{ &ei_mysql_dissector_incomplete, { "mysql.dissector_incomplete", PI_UNDECODED, PI_WARN, "FIXME - dissector is incomplete", EXPFILL }},
		{ &ei_mysql_streamed_param, { "mysql.streamed_param", PI_SEQUENCE, PI_CHAT, "This parameter was streamed, its value can be found in Send BLOB packets", EXPFILL }},
		{ &ei_mysql_prepare_response_needed, { "mysql.prepare_response_needed", PI_UNDECODED, PI_WARN, "PREPARE Response packet is needed to dissect the payload", EXPFILL }},
		{ &ei_mysql_command, { "mysql.command.invalid", PI_PROTOCOL, PI_WARN, "Unknown/invalid command code", EXPFILL }},
		{ &ei_mysql_eof, { "mysql.eof.wrong_state", PI_PROTOCOL, PI_WARN, "EOF Marker found while connection in wrong state.", EXPFILL }},
		{ &ei_mysql_unknown_response, { "mysql.unknown_response", PI_UNDECODED, PI_WARN, "unknown/invalid response", EXPFILL }},
	};

	module_t *mysql_module;
	expert_module_t* expert_mysql;

	proto_mysql = proto_register_protocol("MySQL Protocol", "MySQL", "mysql");
	proto_register_field_array(proto_mysql, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
	expert_mysql = expert_register_protocol(proto_mysql);
	expert_register_field_array(expert_mysql, ei, array_length(ei));

	mysql_module = prefs_register_protocol(proto_mysql, NULL);
	prefs_register_bool_preference(mysql_module, "desegment_buffers",
				       "Reassemble MySQL buffers spanning multiple TCP segments",
				       "Whether the MySQL dissector should reassemble MySQL buffers spanning multiple TCP segments."
				       " To use this option, you must also enable \"Allow subdissectors to reassemble TCP streams\" in the TCP protocol settings.",
				       &mysql_desegment);
	prefs_register_bool_preference(mysql_module, "show_sql_query",
				       "Show SQL Query string in INFO column",
				       "Whether the MySQL dissector should display the SQL query string in the INFO column.",
				       &mysql_showquery);

	mysql_handle = register_dissector("mysql", dissect_mysql, proto_mysql);
}

/* dissector registration */
void proto_reg_handoff_mysql(void)
{
	ssl_handle = find_dissector("ssl");
	dissector_add_uint("tcp.port", TCP_PORT_MySQL, mysql_handle);
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
