/* packet-ntp.c
 * Routines for NTP packet dissection
 * Copyright 1999, Nathan Neulinger <nneul@umr.edu>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "config.h"

#include <math.h>

#include <epan/packet.h>
#include <epan/expert.h>
#include <epan/addr_resolv.h>
#include <epan/tvbparse.h>

#include "packet-ntp.h"

void proto_register_ntp(void);
void proto_reg_handoff_ntp(void);

/*
 * Dissecting NTP packets version 3 and 4 (RFC5905, RFC2030, RFC1769, RFC1361,
 * RFC1305).
 *
 * Those packets have simple structure:
 *                      1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |LI | VN  |Mode |    Stratum    |     Poll      |   Precision   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                          Root Delay                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                       Root Dispersion                         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                    Reference Identifier                       |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                   Reference Timestamp (64)                    |
 * |                                                               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                   Originate Timestamp (64)                    |
 * |                                                               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                    Receive Timestamp (64)                     |
 * |                                                               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                    Transmit Timestamp (64)                    |
 * |                                                               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                 Key Identifier (optional) (32)                |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                 Message Digest (optional) (128/160)           |
 * |                                                               |
 * |                                                               |
 * |                                                               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * NTP timestamps are represented as a 64-bit unsigned fixed-point number,
 * in seconds relative to 0h on 1 January 1900. The integer part is in the
 * first 32 bits and the fraction part in the last 32 bits.
 *
 *
 * NTP Control messages as defined in version 2, 3 and 4 (RFC1119, RFC1305) use
 * the following structure:
 *                      1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |00 | VN  | 110 |R E M| OpCode  |           Sequence            |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |            Status             |        Association ID         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |            Offset             |             Count             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                                                               |
 * |                     Data (468 octets max)                     |
 * |                                                               |
 * |                               |        Padding (zeros)        |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                 Authenticator (optional) (96)                 |
 * |                                                               |
 * |                                                               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * Not yet implemented: complete dissection of TPCTRL_OP_SETTRAP,
 * NTPCTRL_OP_ASYNCMSG, NTPCTRL_OP_UNSETTRAPSETTRAP Control-Messages
 *
 */

#define UDP_PORT_NTP	123
#define TCP_PORT_NTP	123

/* Leap indicator, 2bit field is used to warn of a inserted/deleted
 * second, or clock unsynchronized indication.
 */
#define NTP_LI_MASK	0xC0

#define NTP_LI_NONE	0
#define NTP_LI_61	1
#define NTP_LI_59	2
#define NTP_LI_UNKNOWN	3

static const value_string li_types[] = {
	{ NTP_LI_NONE,	  "no warning" },
	{ NTP_LI_61,	  "last minute of the day has 61 seconds" },
	{ NTP_LI_59,	  "last minute of the day has 59 seconds" },
	{ NTP_LI_UNKNOWN, "unknown (clock unsynchronized)" },
	{ 0,		  NULL}
};

/* Version info, 3bit field informs about NTP version used in particular
 * packet. According to rfc2030, version info could be only 3 or 4, but I
 * have noticed packets with 1 or even 6 as version numbers. They are
 * produced as a result of ntptrace command. Are those packets malformed
 * on purpose? I don't know yet, probably some browsing through ntp sources
 * would help. My solution is to put them as reserved for now.
 */
#define NTP_VN_MASK	0x38

static const value_string ver_nums[] = {
	{ 0,	"reserved" },
	{ 1,	"NTP Version 1" },
	{ 2,	"NTP Version 2" },
	{ 3,	"NTP Version 3" },
	{ 4,	"NTP Version 4" },
	{ 5,	"reserved" },
	{ 6,	"reserved" },
	{ 7,	"reserved" },
	{ 0,	NULL}
};

/* Mode, 3bit field representing mode of communication.
 */
#define NTP_MODE_MASK   7

#define NTP_MODE_RSV	0
#define NTP_MODE_SYMACT	1
#define NTP_MODE_SYMPAS	2
#define NTP_MODE_CLIENT	3
#define NTP_MODE_SERVER	4
#define NTP_MODE_BCAST	5
#define NTP_MODE_CTRL	6
#define NTP_MODE_PRIV	7

static const value_string mode_types[] = {
	{ NTP_MODE_RSV,		"reserved" },
	{ NTP_MODE_SYMACT,	"symmetric active" },
	{ NTP_MODE_SYMPAS,	"symmetric passive" },
	{ NTP_MODE_CLIENT,	"client" },
	{ NTP_MODE_SERVER,	"server" },
	{ NTP_MODE_BCAST,	"broadcast" },
	{ NTP_MODE_CTRL,	"reserved for NTP control message"},
	{ NTP_MODE_PRIV,	"reserved for private use" },
	{ 0,		NULL}
};

static const value_string info_mode_types[] = {
	{ NTP_MODE_RSV,		"reserved" },
	{ NTP_MODE_SYMACT,	"symmetric active" },
	{ NTP_MODE_SYMPAS,	"symmetric passive" },
	{ NTP_MODE_CLIENT,	"client" },
	{ NTP_MODE_SERVER,	"server" },
	{ NTP_MODE_BCAST,	"broadcast" },
	{ NTP_MODE_CTRL,	"control"},
	{ NTP_MODE_PRIV,	"private" },
	{ 0,		NULL}
};

/* According to rfc, primary (stratum-0 and stratum-1) servers should set
 * their Reference ID (4bytes field) according to following table:
 */
static const struct {
	const char *id;
	const char *data;
} primary_sources[] = {
	/* IANA / RFC 5905 */
	{ "GOES",	"Geostationary Orbit Environment Satellite" },
	{ "GPS\0",	"Global Position System" },
	{ "GAL\0",	"Galileo Positioning System" },
	{ "PPS\0",	"Generic pulse-per-second" },
	{ "IRIG",	"Inter-Range Instrumentation Group" },
	{ "WWVB",	"LF Radio WWVB Ft. Collins, CO 60 kHz" },
	{ "DCF\0",	"LF Radio DCF77 Mainflingen, DE 77.5 kHz" },
	{ "HBG\0",	"LF Radio HBG Prangins, HB 75 kHz" },
	{ "MSF\0",	"LF Radio MSF Anthorn, UK 60 kHz" },
	{ "JJY\0",	"LF Radio JJY Fukushima, JP 40 kHz, Saga, JP 60 kHz" },
	{ "LORC",	"MF Radio LORAN C station, 100 kHz" },
	{ "TDF\0",	"MF Radio Allouis, FR 162 kHz" },
	{ "CHU\0",	"HF Radio CHU Ottawa, Ontario" },
	{ "WWV\0",	"HF Radio WWV Ft. Collins, CO" },
	{ "WWVH",	"HF Radio WWVH Kauai, HI" },
	{ "NIST",	"NIST telephone modem" },
	{ "ACTS",	"NIST telephone modem" },
	{ "USNO",	"USNO telephone modem" },
	{ "PTB\0",	"European telephone modem" },

	/* Unofficial codes */
	{ "LOCL",	"uncalibrated local clock" },
	{ "CESM",	"calibrated Cesium clock" },
	{ "RBDM",	"calibrated Rubidium clock" },
	{ "OMEG",	"OMEGA radionavigation system" },
	{ "DCN\0",	"DCN routing protocol" },
	{ "TSP\0",	"TSP time protocol" },
	{ "DTS\0",	"Digital Time Service" },
	{ "ATOM",	"Atomic clock (calibrated)" },
	{ "VLF\0",	"VLF radio (OMEGA,, etc.)" },
	{ "1PPS",	"External 1 PPS input" },
	{ "FREE",	"(Internal clock)" },
	{ "INIT",	"(Initialization)" },
	{ "\0\0\0\0",	"NULL" },
	{ NULL,		NULL}
};

#define NTP_EXT_R_MASK 0x80

static const value_string ext_r_types[] = {
	{ 0,		"Request" },
	{ 1,		"Response" },
	{ 0,		NULL}
};

#define NTP_EXT_ERROR_MASK 0x40
#define NTP_EXT_VN_MASK 0x3f

static const value_string ext_op_types[] = {
	{ 0,		"NULL" },
	{ 1,		"ASSOC" },
	{ 2,		"CERT" },
	{ 3,		"COOK" },
	{ 4,		"AUTO" },
	{ 5,		"TAI" },
	{ 6,		"SIGN" },
	{ 7,		"IFF" },
	{ 8,		"GQ" },
	{ 9,		"MV" },
	{ 0,		NULL}
};

#define NTPCTRL_R_MASK 0x80

#define ctrl_r_types ext_r_types

#define NTPCTRL_ERROR_MASK 0x40
#define NTPCTRL_MORE_MASK 0x20
#define NTPCTRL_OP_MASK 0x1f

#define NTPCTRL_OP_UNSPEC 0
#define NTPCTRL_OP_READSTAT 1
#define NTPCTRL_OP_READVAR 2
#define NTPCTRL_OP_WRITEVAR 3
#define NTPCTRL_OP_READCLOCK 4
#define NTPCTRL_OP_WRITECLOCK 5
#define NTPCTRL_OP_SETTRAP 6
#define NTPCTRL_OP_ASYNCMSG 7
#define NTPCTRL_OP_UNSETTRAP 31

static const value_string ctrl_op_types[] = {
	{ NTPCTRL_OP_UNSPEC,		"UNSPEC" },
	{ NTPCTRL_OP_READSTAT,		"READSTAT" },
	{ NTPCTRL_OP_READVAR,		"READVAR" },
	{ NTPCTRL_OP_WRITEVAR,		"WRITEVAR" },
	{ NTPCTRL_OP_READCLOCK,		"READCLOCK" },
	{ NTPCTRL_OP_WRITECLOCK,	"WRITECLOCK" },
	{ NTPCTRL_OP_SETTRAP,		"SETTRAP" },
	{ NTPCTRL_OP_ASYNCMSG,		"ASYNCMSG" },
	{ NTPCTRL_OP_UNSETTRAP,		"UNSETTRAP" },
	{ 0,		NULL}
};

#define NTPCTRL_SYSSTATUS_LI_MASK		0xC000
#define NTPCTRL_SYSSTATUS_CLK_MASK		0x3F00
#define NTPCTRL_SYSSTATUS_COUNT_MASK	0x00F0
#define NTPCTRL_SYSSTATUS_CODE_MASK		0x000F

static const value_string ctrl_sys_status_clksource_types[] = {
	{ 0,		"unspecified or unknown" },
	{ 1,		"Calibrated atomic clock (e.g. HP 5061)" },
	{ 2,		"VLF (band 4) or LF (band 5) radio (e.g. OMEGA, WWVB)" },
	{ 3,		"HF (band 7) radio (e.g. CHU, MSF, WWV/H)" },
	{ 4,		"UHF (band 9) satellite (e.g. GOES, GPS)" },
	{ 5,		"local net (e.g. DCN, TSP, DTS)" },
	{ 6,		"UDP/NTP" },
	{ 7,		"UDP/TIME" },
	{ 8,		"eyeball-and-wristwatch" },
	{ 9,		"telephone modem (e.g. NIST)" },
	{ 0,		NULL}
};

static const value_string ctrl_sys_status_event_types[] = {
	{ 0,		"unspecified" },
	{ 1,		"system restart" },
	{ 2,		"system or hardware fault" },
	{ 3,		"system new status word (leap bits or synchronization change)" },
	{ 4,		"system new synchronization source or stratum (sys.peer or sys.stratum change)" },
	{ 5,		"system clock reset (offset correction exceeds CLOCK.MAX)" },
	{ 6,		"system invalid time or date (see NTP spec.)" },
	{ 7,		"system clock exception (see system clock status word)" },
	{ 0,		NULL}
};

#define NTPCTRL_PEERSTATUS_STATUS_MASK		0xF800
#define NTPCTRL_PEERSTATUS_CONFIG_MASK		0x8000
#define NTPCTRL_PEERSTATUS_AUTHENABLE_MASK	0x4000
#define NTPCTRL_PEERSTATUS_AUTHENTIC_MASK	0x2000
#define NTPCTRL_PEERSTATUS_REACH_MASK		0x1000
#define NTPCTRL_PEERSTATUS_RESERVED_MASK	0x0800
#define NTPCTRL_PEERSTATUS_SEL_MASK			0x0700
#define NTPCTRL_PEERSTATUS_COUNT_MASK		0x00F0
#define NTPCTRL_PEERSTATUS_CODE_MASK		0x000F

static const true_false_string tfs_ctrl_peer_status_config = {"configured (peer.config)", "not configured (peer.config)" };
static const true_false_string tfs_ctrl_peer_status_authenable = { "authentication enabled (peer.authenable", "authentication disabled (peer.authenable" };
static const true_false_string tfs_ctrl_peer_status_authentic = { "authentication okay (peer.authentic)", "authentication not okay (peer.authentic)" };
static const true_false_string tfs_ctrl_peer_status_reach = {"reachability okay (peer.reach != 0)", "reachability not okay (peer.reach != 0)" };

static const value_string ctrl_peer_status_selection_types[] = {
	{ 0,		"rejected" },
	{ 1,		"passed sanity checks (tests 1 through 8 in Section 3.4.3)" },
	{ 2,		"passed correctness checks (intersection algorithm in Section 4.2.1)" },
	{ 3,		"passed candidate checks (if limit check implemented)" },
	{ 4,		"passed outlyer checks (clustering algorithm in Section 4.2.2)" },
	{ 5,		"current synchronization source; max distance exceeded (if limit check implemented)" },
	{ 6,		"current synchronization source; max distance okay" },
	{ 7,		"reserved" },
	{ 0,		NULL}
};

static const value_string ctrl_peer_status_event_types[] = {
	{ 0,		"unspecified" },
	{ 1,		"peer IP error" },
	{ 2,		"peer authentication failure (peer.authentic bit was one now zero)" },
	{ 3,		"peer unreachable (peer.reach was nonzero now zero)" },
	{ 4,		"peer reachable (peer.reach was zero now nonzero)" },
	{ 5,		"peer clock exception (see peer clock status word)" },
	{ 0,		NULL}
};

#define NTPCTRL_CLKSTATUS_STATUS_MASK	0xFF00
#define NTPCTRL_CLKSTATUS_CODE_MASK		0x00FF

static const value_string ctrl_clk_status_types[] = {
	{ 0,		"clock operating within nominals" },
	{ 1,		"reply timeout" },
	{ 2,		"bad reply format" },
	{ 3,		"hardware or software fault" },
	{ 4,		"propagation failure" },
	{ 5,		"bad date format or value" },
	{ 6,		"bad time format or value" },
	{ 0,		NULL}
};

#define NTP_CTRL_ERRSTATUS_CODE_MASK	0xFF00

static const value_string ctrl_err_status_types[] = {
	{ 0,		"unspecified" },
	{ 1,		"authentication failure" },
	{ 2,		"invalid message length or format" },
	{ 3,		"invalid opcode" },
	{ 4,		"unknown association identifier" },
	{ 5,		"unknown variable name" },
	{ 6,		"invalid variable value" },
	{ 7,		"administratively prohibited" },
	{ 0,		NULL}
};

static const value_string err_values_types[] = {
	{ 0,		"No error" },
	{ 1,		"incompatible implementation number"},
	{ 2,		"unimplemented request code" },
	{ 3,		"format error" },
	{ 4,		"no data available" },
	{ 5,		"unknown" },
	{ 6,		"unknown" },
	{ 7,		"authentication failure"},
	{ 0,		NULL}
};

#define NTPPRIV_R_MASK 0x80

#define priv_r_types ext_r_types

#define NTPPRIV_MORE_MASK 0x40

#define NTPPRIV_AUTH_MASK 0x80
#define NTPPRIV_SEQ_MASK 0x7f

#define XNTPD 0x03

static const value_string priv_impl_types[] = {
	{ 0,		"UNIV" },
	{ 2,		"XNTPD_OLD (pre-IPv6)" },
	{ 3,		"XNTPD" },
	{ 0,		NULL}
};

#define MON_GETLIST_1 42

static const value_string priv_rc_types[] = {
	{ 0,		"PEER_LIST" },
	{ 1,		"PEER_LIST_SUM" },
	{ 2,		"PEER_INFO" },
	{ 3,		"PEER_STATS" },
	{ 4,		"SYS_INFO" },
	{ 5,		"SYS_STATS" },
	{ 6,		"IO_STATS" },
	{ 7,		"MEM_STATS" },
	{ 8,		"LOOP_INFO" },
	{ 9,		"TIMER_STATS" },
	{ 10,		"CONFIG" },
	{ 11,		"UNCONFIG" },
	{ 12,		"SET_SYS_FLAG" },
	{ 13,		"CLR_SYS_FLAG" },
	{ 16,		"GET_RESTRICT" },
	{ 17,		"RESADDFLAGS" },
	{ 18,		"RESSUBFLAGS" },
	{ 19,		"UNRESTRICT" },
	{ 20,		"MON_GETLIST" },
	{ 21,		"RESET_STATS" },
	{ 22,		"RESET_PEER" },
	{ 23,		"REREAD_KEYS" },
	{ 26,		"TRUSTKEY" },
	{ 27,		"UNTRUSTKEY" },
	{ 28,		"AUTHINFO" },
	{ 29,		"TRAPS" },
	{ 30,		"ADD_TRAP" },
	{ 31,		"CLR_TRAP" },
	{ 32,		"REQUEST_KEY" },
	{ 33,		"CONTROL_KEY" },
	{ 34,		"GET_CTLSTATS" },
	{ 36,		"GET_CLOCKINFO" },
	{ 37,		"SET_CLKFUDGE" },
	{ 38,		"GET_KERNEL" },
	{ 39,		"GET_CLKBUGINFO" },
	{ 42,		"MON_GETLIST_1" },
	{ 43,		"HOSTNAME_ASSOCID" },
	{ 0,		NULL}
};
static value_string_ext priv_rc_types_ext = VALUE_STRING_EXT_INIT(priv_rc_types);

static const range_string stratum_rvals[] = {
	{ 0,	0, "unspecified or invalid" },
	{ 1,	1, "primary reference" },
	{ 2,	15, "secondary reference" },
	{ 16,	16, "unsynchronized" },
	{ 17,	255, "reserved" },
	{ 0,	0, NULL }
};

#define NTP_MD5_ALGO 0
#define NTP_SHA_ALGO 1

static const value_string authentication_types[] = {
	{ NTP_MD5_ALGO,		"MD5" },
	{ NTP_SHA_ALGO,		"SHA" },
	{ 0,		NULL}
};


/*
 * Maximum MAC length : 160 bits MAC + 32 bits Key ID
 */
#define MAX_MAC_LEN	(6 * sizeof (guint32))

static int proto_ntp = -1;

static int hf_ntp_flags = -1;
static int hf_ntp_flags_li = -1;
static int hf_ntp_flags_vn = -1;
static int hf_ntp_flags_mode = -1;
static int hf_ntp_stratum = -1;
static int hf_ntp_ppoll = -1;
static int hf_ntp_precision = -1;
static int hf_ntp_rootdelay = -1;
static int hf_ntp_rootdispersion = -1;
static int hf_ntp_refid = -1;
static int hf_ntp_reftime = -1;
static int hf_ntp_org = -1;
static int hf_ntp_rec = -1;
static int hf_ntp_xmt = -1;
static int hf_ntp_keyid = -1;
static int hf_ntp_mac = -1;
static int hf_ntp_key_type = -1;
static int hf_ntp_key_index = -1;
static int hf_ntp_key_signature = -1;

static int hf_ntp_ext = -1;
static int hf_ntp_ext_flags = -1;
static int hf_ntp_ext_flags_r = -1;
static int hf_ntp_ext_flags_error = -1;
static int hf_ntp_ext_flags_vn = -1;
static int hf_ntp_ext_op = -1;
static int hf_ntp_ext_len = -1;
static int hf_ntp_ext_associd = -1;
static int hf_ntp_ext_tstamp = -1;
static int hf_ntp_ext_fstamp = -1;
static int hf_ntp_ext_vallen = -1;
static int hf_ntp_ext_val = -1;
static int hf_ntp_ext_siglen = -1;
static int hf_ntp_ext_sig = -1;

static int hf_ntpctrl_flags2 = -1;
static int hf_ntpctrl_flags2_r = -1;
static int hf_ntpctrl_flags2_error = -1;
static int hf_ntpctrl_flags2_more = -1;
static int hf_ntpctrl_flags2_opcode = -1;
static int hf_ntpctrl_sequence = -1;
static int hf_ntpctrl_status = -1;
static int hf_ntpctrl_error_status_word = -1;
static int hf_ntpctrl_sys_status_li = -1;
static int hf_ntpctrl_sys_status_clksrc = -1;
static int hf_ntpctrl_sys_status_count = -1;
static int hf_ntpctrl_sys_status_code = -1;
static int hf_ntpctrl_peer_status_b0 = -1;
static int hf_ntpctrl_peer_status_b1 = -1;
static int hf_ntpctrl_peer_status_b2 = -1;
static int hf_ntpctrl_peer_status_b3 = -1;
static int hf_ntpctrl_peer_status_b4 = -1;
static int hf_ntpctrl_peer_status_selection = -1;
static int hf_ntpctrl_peer_status_count = -1;
static int hf_ntpctrl_peer_status_code = -1;
static int hf_ntpctrl_clk_status = -1;
static int hf_ntpctrl_clk_status_code = -1;
static int hf_ntpctrl_associd = -1;
static int hf_ntpctrl_offset = -1;
static int hf_ntpctrl_count = -1;
static int hf_ntpctrl_data = -1;
static int hf_ntpctrl_item = -1;
static int hf_ntpctrl_trapmsg = -1;

static int hf_ntppriv_flags_r = -1;
static int hf_ntppriv_flags_more = -1;
static int hf_ntppriv_auth_seq = -1;
static int hf_ntppriv_auth = -1;
static int hf_ntppriv_seq = -1;
static int hf_ntppriv_impl = -1;
static int hf_ntppriv_reqcode = -1;
static int hf_ntppriv_errcode = -1;
static int hf_ntppriv_numitems = -1;
static int hf_ntppriv_mbz = -1;
static int hf_monlist_item = -1;
static int hf_ntppriv_itemsize = -1;
static int hf_ntppriv_avgint = -1;
static int hf_ntppriv_lsint = -1;
static int hf_ntppriv_count = -1;
static int hf_ntppriv_restr = -1;
static int hf_ntppriv_addr = -1;
static int hf_ntppriv_daddr = -1;
static int hf_ntppriv_flags = -1;
static int hf_ntppriv_port = -1;
static int hf_ntppriv_mode = -1;
static int hf_ntppriv_version = -1;
static int hf_ntppriv_v6_flag = -1;
static int hf_ntppriv_addr6 = -1;
static int hf_ntppriv_daddr6 = -1;

static gint ett_ntp = -1;
static gint ett_ntp_flags = -1;
static gint ett_ntp_ext = -1;
static gint ett_ntp_ext_flags = -1;
static gint ett_ntpctrl_flags2 = -1;
static gint ett_ntpctrl_status = -1;
static gint ett_ntpctrl_data = -1;
static gint ett_ntpctrl_item = -1;
static gint ett_ntppriv_auth_seq = -1;
static gint ett_monlist_item = -1;
static gint ett_ntp_authenticator = -1;

static expert_field ei_ntp_ext = EI_INIT;

static const char *mon_names[12] = {
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec"
};

static const int *ntp_header_fields[] = {
	&hf_ntp_flags_li,
	&hf_ntp_flags_vn,
	&hf_ntp_flags_mode,
	NULL
};

/*
	* dissect peer status word:
	*                      1
	*  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
	* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	* | Status  | Sel | Count | Code  |
	* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	*/
static const int *peer_status_flags[] = {
	&hf_ntpctrl_peer_status_b0,
	&hf_ntpctrl_peer_status_b1,
	&hf_ntpctrl_peer_status_b2,
	&hf_ntpctrl_peer_status_b3,
	&hf_ntpctrl_peer_status_b4,
	&hf_ntpctrl_peer_status_selection,
	&hf_ntpctrl_peer_status_count,
	&hf_ntpctrl_peer_status_code,
	NULL
};

/* parser definitions */
static tvbparse_wanted_t *want;
static tvbparse_wanted_t *want_ignore;

/* NTP_BASETIME is in fact epoch - ntp_start_time */
#define NTP_BASETIME 2208988800u
#define NTP_FLOAT_DENOM 4294967296.0
#define NTP_TS_SIZE 100

/* Modified tvb_ntp_fmt_ts
 * tvb_mip6_fmt_ts - converts MIP6 timestamp to human readable string.
 *      Timestamp
 *
 *         A 64-bit unsigned integer field containing a timestamp.  The
 *          value indicates the number of seconds since January 1, 1970,
 *          00:00 UTC, by using a fixed point format.  In this format, the
 *          integer number of seconds is contained in the first 48 bits of
 *          the field, and the remaining 16 bits indicate the number of
 *          1/65536 fractions of a second.
 *
 * TVB and an offset (IN).
 * returns pointer to filled buffer.  This buffer will be freed automatically once
 * dissection of the next packet occurs.
 */
const char *
tvb_mip6_fmt_ts(tvbuff_t *tvb, gint offset)
{
	guint64		 tempstmp;
	guint32		 tempfrac;
	time_t		 temptime;
	struct tm	*bd;
	double		 fractime;
	char		*buff;

	tempstmp = tvb_get_ntoh48(tvb, offset);
	tempfrac = tvb_get_ntohs(tvb, offset+6);
	tempfrac <<= 16;
	if ((tempstmp == 0) && (tempfrac == 0)) {
		return "NULL";
	}

	temptime = (time_t)(tempstmp /*- NTP_BASETIME*/);
	bd = gmtime(&temptime);
	if(!bd){
		return "Not representable";
	}

	fractime = bd->tm_sec + tempfrac / NTP_FLOAT_DENOM;
	buff = (char *)wmem_alloc(wmem_packet_scope(), NTP_TS_SIZE);
	g_snprintf(buff, NTP_TS_SIZE,
		 "%s %2d, %d %02d:%02d:%07.4f UTC",
		 mon_names[bd->tm_mon],
		 bd->tm_mday,
		 bd->tm_year + 1900,
		 bd->tm_hour,
		 bd->tm_min,
		 fractime);
	return buff;
}
/* tvb_ntp_fmt_ts - converts NTP timestamp to human readable string.
 * TVB and an offset (IN).
 * returns pointer to filled buffer.  This buffer will be freed automatically once
 * dissection of the next packet occurs.
 */
const char *
tvb_ntp_fmt_ts(tvbuff_t *tvb, gint offset)
{
	guint32		 tempstmp, tempfrac;
	time_t		 temptime;
	struct tm	*bd;
	double		 fractime;
	char		*buff;

	tempstmp = tvb_get_ntohl(tvb, offset);
	tempfrac = tvb_get_ntohl(tvb, offset+4);
	if ((tempstmp == 0) && (tempfrac == 0)) {
		return "NULL";
	}

	/* We need a temporary variable here so the unsigned math
	 * works correctly (for years > 2036 according to RFC 2030
	 * chapter 3).
	 */
	temptime = (time_t)(tempstmp - NTP_BASETIME);
	bd = gmtime(&temptime);
	if(!bd){
		return "Not representable";
	}

	fractime = bd->tm_sec + tempfrac / NTP_FLOAT_DENOM;
	buff=(char *)wmem_alloc(wmem_packet_scope(), NTP_TS_SIZE);
	g_snprintf(buff, NTP_TS_SIZE,
		 "%s %2d, %d %02d:%02d:%09.6f UTC",
		 mon_names[bd->tm_mon],
		 bd->tm_mday,
		 bd->tm_year + 1900,
		 bd->tm_hour,
		 bd->tm_min,
		 fractime);
	return buff;
}

/* tvb_ntp_fmt_ts_sec - converts an NTP timestamps second part (32bits) to an human readable string.
* TVB and an offset (IN).
* returns pointer to filled buffer.  This buffer will be freed automatically once
* dissection of the next packet occurs.
*/
const char *
tvb_ntp_fmt_ts_sec(tvbuff_t *tvb, gint offset)
{
	guint32		 tempstmp;
	time_t		 temptime;
	struct tm	*bd;
	char		*buff;

	tempstmp = tvb_get_ntohl(tvb, offset);
	if (tempstmp == 0){
		return "NULL";
	}

	/* We need a temporary variable here so the unsigned math
	* works correctly (for years > 2036 according to RFC 2030
	* chapter 3).
	*/
	temptime = (time_t)(tempstmp - NTP_BASETIME);
	bd = gmtime(&temptime);
	if (!bd){
		return "Not representable";
	}

	buff = (char *)wmem_alloc(wmem_packet_scope(), NTP_TS_SIZE);
	g_snprintf(buff, NTP_TS_SIZE,
		"%s %2d, %d %02d:%02d:%02d UTC",
		mon_names[bd->tm_mon],
		bd->tm_mday,
		bd->tm_year + 1900,
		bd->tm_hour,
		bd->tm_min,
		bd->tm_sec);
	return buff;
}

void
ntp_to_nstime(tvbuff_t *tvb, gint offset, nstime_t *nstime)
{
	guint32		 tempstmp;

	/* We need a temporary variable here so the unsigned math
	 * works correctly (for years > 2036 according to RFC 2030
	 * chapter 3).
	 */
	tempstmp  = tvb_get_ntohl(tvb, offset);
	if (tempstmp)
		nstime->secs = (time_t)(tempstmp - NTP_BASETIME);
	else
		nstime->secs = (time_t)tempstmp; /* 0 */

	nstime->nsecs = (int)(tvb_get_ntohl(tvb, offset+4)/(NTP_FLOAT_DENOM/1000000000.0));
}


static int
dissect_ntp_ext(tvbuff_t *tvb, packet_info *pinfo, proto_tree *ntp_tree, int offset)
{
	proto_tree      *ext_tree, *flags_tree;
	proto_item	*tf, *ext_item;
	guint16		 extlen;
	int		 endoffset;
	guint8		 flags;
	guint32		 vallen, vallen_round, siglen;

	extlen = tvb_get_ntohs(tvb, offset+2);
	tf = proto_tree_add_item(ntp_tree, hf_ntp_ext, tvb, offset, extlen,
	    ENC_NA);
	ext_tree = proto_item_add_subtree(tf, ett_ntp_ext);

	if (extlen < 8) {
		/* Extension length isn't enough for the extension header.
		 * Report the error, and return an offset that goes to
		 * the end of the tvbuff, so we stop dissecting.
		 */
		expert_add_info_format(pinfo, tf, &ei_ntp_ext, "Extension length %u < 8", extlen);
		return tvb_reported_length(tvb);
	}
	if (extlen % 4) {
		/* Extension length isn't a multiple of 4.
		 * Report the error, and return an offset that goes
		 * to the end of the tvbuff, so we stop dissecting.
		 */
		expert_add_info_format(pinfo, tf, &ei_ntp_ext, "Extension length %u isn't a multiple of 4",
				    extlen);
		return tvb_reported_length(tvb);
	}
	endoffset = offset + extlen;

	flags = tvb_get_guint8(tvb, offset);
	tf = proto_tree_add_uint(ext_tree, hf_ntp_ext_flags, tvb, offset, 1,
				 flags);
	flags_tree = proto_item_add_subtree(tf, ett_ntp_ext_flags);
	proto_tree_add_uint(flags_tree, hf_ntp_ext_flags_r, tvb, offset, 1,
			    flags);
	proto_tree_add_uint(flags_tree, hf_ntp_ext_flags_error, tvb, offset, 1,
			    flags);
	proto_tree_add_uint(flags_tree, hf_ntp_ext_flags_vn, tvb, offset, 1,
			    flags);
	offset += 1;

	proto_tree_add_item(ext_tree, hf_ntp_ext_op, tvb, offset, 1, ENC_BIG_ENDIAN);
	offset += 1;

	proto_tree_add_uint(ext_tree, hf_ntp_ext_len, tvb, offset, 2, extlen);
	offset += 2;

	if ((flags & NTP_EXT_VN_MASK) != 2) {
		/* don't care about autokey v1 */
		return endoffset;
	}

	proto_tree_add_item(ext_tree, hf_ntp_ext_associd, tvb, offset, 4,
			    ENC_BIG_ENDIAN);
	offset += 4;

	/* check whether everything up to "vallen" is present */
	if (extlen < MAX_MAC_LEN) {
		/* XXX - report as error? */
		return endoffset;
	}

	proto_tree_add_item(ext_tree, hf_ntp_ext_tstamp, tvb, offset, 4,
			    ENC_BIG_ENDIAN);
	offset += 4;
	proto_tree_add_item(ext_tree, hf_ntp_ext_fstamp, tvb, offset, 4,
			    ENC_BIG_ENDIAN);
	offset += 4;
	/* XXX fstamp can be server flags */

	vallen = tvb_get_ntohl(tvb, offset);
	ext_item = proto_tree_add_uint(ext_tree, hf_ntp_ext_vallen, tvb, offset, 4,
			    vallen);
	offset += 4;
	vallen_round = (vallen + 3) & (-4);
	if (vallen != 0) {
		if ((guint32)(endoffset - offset) < vallen_round) {
			/*
			 * Value goes past the length of the extension
			 * field.
			 */
			expert_add_info_format(pinfo, ext_item, &ei_ntp_ext,
					    "Value length makes value go past the end of the extension field");
			return endoffset;
		}
		proto_tree_add_item(ext_tree, hf_ntp_ext_val, tvb, offset,
				    vallen, ENC_NA);
	}
	offset += vallen_round;

	siglen = tvb_get_ntohl(tvb, offset);
	ext_item = proto_tree_add_uint(ext_tree, hf_ntp_ext_siglen, tvb, offset, 4,
			    siglen);
	offset += 4;
	if (siglen != 0) {
		if (offset + (int)siglen > endoffset) {
			/*
			 * Value goes past the length of the extension
			 * field.
			 */
			expert_add_info_format(pinfo, ext_item, &ei_ntp_ext,
					    "Signature length makes value go past the end of the extension field");
			return endoffset;
		}
		proto_tree_add_item(ext_tree, hf_ntp_ext_sig, tvb,
			offset, siglen, ENC_NA);
	}
	return endoffset;
}

static void
dissect_ntp_std(tvbuff_t *tvb, packet_info *pinfo, proto_tree *ntp_tree)
{
	guint8		 stratum;
	guint8		 ppoll;
	gint8		 precision;
	double		 rootdelay;
	double		 rootdispersion;
	guint32		 refid_addr;
	gchar           *buff;
	int		 i;
	int		 macofs;
	gint		 maclen;

	proto_tree_add_bitmask(ntp_tree, tvb, 0, hf_ntp_flags, ett_ntp_flags, ntp_header_fields, ENC_NA);

	/* Stratum, 1byte field represents distance from primary source
	 */
	proto_tree_add_item(ntp_tree, hf_ntp_stratum, tvb, 1, 1, ENC_NA);
	stratum = tvb_get_guint8(tvb, 1);

	/* Poll interval, 1byte field indicating the maximum interval
	 * between successive messages, in seconds to the nearest
	 * power of two.
	 */
	ppoll = tvb_get_guint8(tvb, 2);
	if ((ppoll >= 4) && (ppoll <= 17)) {
		proto_tree_add_uint_format_value(ntp_tree, hf_ntp_ppoll, tvb, 2, 1,
				   ppoll,
				   "%u (%u sec)",
				   ppoll,
				   1 << ppoll);
	} else {
		proto_tree_add_uint_format_value(ntp_tree, hf_ntp_ppoll, tvb, 2, 1,
				   ppoll,
				   "invalid (%u)",
				   ppoll);
	}

	/* Precision, 1 byte field indicating the precision of the
	 * local clock, in seconds to the nearest power of two.
	 */
	precision = tvb_get_guint8(tvb, 3);
	proto_tree_add_int_format_value(ntp_tree, hf_ntp_precision, tvb, 3, 1,
				   precision,
				   "%8.6f sec",
				   pow(2, precision));

	/* Root Delay is a 32-bit signed fixed-point number indicating
	 * the total roundtrip delay to the primary reference source,
	 * in seconds with fraction point between bits 15 and 16.
	 */
	rootdelay = ((gint16)tvb_get_ntohs(tvb, 4)) +
			(tvb_get_ntohs(tvb, 6) / 65536.0);
	proto_tree_add_double_format_value(ntp_tree, hf_ntp_rootdelay, tvb, 4, 4,
				   rootdelay,
				   "%9.4f sec",
				   rootdelay);

	/* Root Dispersion, 32-bit unsigned fixed-point number indicating
	 * the nominal error relative to the primary reference source, in
	 * seconds with fraction point between bits 15 and 16.
	 */
	rootdispersion = ((gint16)tvb_get_ntohs(tvb, 8)) +
				(tvb_get_ntohs(tvb, 10) / 65536.0);
	proto_tree_add_double_format_value(ntp_tree, hf_ntp_rootdispersion, tvb, 8, 4,
				   rootdispersion,
				   "%9.4f sec",
				   rootdispersion);

	/* Now, there is a problem with secondary servers.  Standards
	 * asks from stratum-2 - stratum-15 servers to set this to the
	 * low order 32 bits of the latest transmit timestamp of the
	 * reference source.
	 * But, all V3 and V4 servers set this to IP address of their
	 * higher level server. My decision was to resolve this address.
	 */
	buff = (gchar *)wmem_alloc(wmem_packet_scope(), NTP_TS_SIZE);
	if (stratum <= 1) {
		g_snprintf (buff, NTP_TS_SIZE, "Unidentified reference source '%.4s'",
			tvb_get_string_enc(wmem_packet_scope(), tvb, 12, 4, ENC_ASCII));
		for (i = 0; primary_sources[i].id; i++) {
			if (tvb_memeql(tvb, 12, primary_sources[i].id, 4) == 0) {
				g_snprintf(buff, NTP_TS_SIZE, "%s",
					primary_sources[i].data);
				break;
			}
		}
	} else {
		int buffpos;
		refid_addr = tvb_get_ipv4(tvb, 12);
		buffpos = g_snprintf(buff, NTP_TS_SIZE, "%s", get_hostname (refid_addr));
		if (buffpos >= NTP_TS_SIZE) {
			buff[NTP_TS_SIZE-4]='.';
			buff[NTP_TS_SIZE-3]='.';
			buff[NTP_TS_SIZE-2]='.';
			buff[NTP_TS_SIZE-1]=0;
		}
	}
	proto_tree_add_bytes_format_value(ntp_tree, hf_ntp_refid, tvb, 12, 4,
					NULL, "%s", buff);

	/* Reference Timestamp: This is the time at which the local clock was
	 * last set or corrected.
	 */
	proto_tree_add_item(ntp_tree, hf_ntp_reftime, tvb, 16, 8, ENC_TIME_NTP|ENC_BIG_ENDIAN);

	/* Originate Timestamp: This is the time at which the request departed
	 * the client for the server.
	 */
	proto_tree_add_item(ntp_tree, hf_ntp_org, tvb, 24, 8, ENC_TIME_NTP|ENC_BIG_ENDIAN);

	/* Receive Timestamp: This is the time at which the request arrived at
	 * the server.
	 */
	proto_tree_add_item(ntp_tree, hf_ntp_rec, tvb, 32, 8, ENC_TIME_NTP|ENC_BIG_ENDIAN);

	/* Transmit Timestamp: This is the time at which the reply departed the
	 * server for the client.
	 */
	proto_tree_add_item(ntp_tree, hf_ntp_xmt, tvb, 40, 8, ENC_TIME_NTP|ENC_BIG_ENDIAN);

	/* MAX_MAC_LEN is the largest message authentication code
	 * (MAC) length.  If we have more data left in the packet
	 * after the header than that, the extra data is NTP4
	 * extensions; parse them as such.
	 */
	macofs = 48;
	while (tvb_reported_length_remaining(tvb, macofs) > (gint)MAX_MAC_LEN)
		macofs = dissect_ntp_ext(tvb, pinfo, ntp_tree, macofs);

	/* When the NTP authentication scheme is implemented, the
	 * Key Identifier and Message Digest fields contain the
	 * message authentication code (MAC) information defined in
	 * Appendix C of RFC-1305. Will print this as hex code for now.
	 */
	if (tvb_reported_length_remaining(tvb, macofs) >= 4)
		proto_tree_add_item(ntp_tree, hf_ntp_keyid, tvb, macofs, 4,
				    ENC_NA);
	macofs += 4;
	maclen = tvb_reported_length_remaining(tvb, macofs);
	if (maclen > 0)
		proto_tree_add_item(ntp_tree, hf_ntp_mac, tvb, macofs,
				    maclen, ENC_NA);
}

static void
dissect_ntp_ctrl(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *ntp_tree)
{
	guint8		 flags2;
	proto_tree	*data_tree, *item_tree, *auth_tree;
	proto_item	*td, *ti;
	guint16		 associd;
	guint16		 datalen;
	guint16		 data_offset;
	int			 length_remaining;

	tvbparse_t	*tt;
	tvbparse_elem_t *element;

	static const int *ntpctrl_flags[] = {
		&hf_ntpctrl_flags2_r,
		&hf_ntpctrl_flags2_error,
		&hf_ntpctrl_flags2_more,
		&hf_ntpctrl_flags2_opcode,
		NULL
	};
	proto_tree_add_bitmask(ntp_tree, tvb, 0, hf_ntp_flags, ett_ntp_flags, ntp_header_fields, ENC_NA);
	proto_tree_add_bitmask(ntp_tree, tvb, 1, hf_ntpctrl_flags2, ett_ntpctrl_flags2, ntpctrl_flags, ENC_NA);
	flags2 = tvb_get_guint8(tvb, 1);

	proto_tree_add_item(ntp_tree, hf_ntpctrl_sequence,    tvb, 2, 2, ENC_BIG_ENDIAN);
	associd = tvb_get_ntohs(tvb, 6);
	/*
	 * further processing of status is only necessary in server responses
	 */
	if (flags2 & NTPCTRL_R_MASK) {
		if (flags2 & NTPCTRL_ERROR_MASK) {
			/*
			 * if error bit is set: dissect error status word
			 *                      1
			 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 * |  Error Code   |   reserved    |
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 */
			static const int *errorstatus[] = {
				&hf_ntpctrl_error_status_word,
				NULL
			};

			/* Check if this is an error response... */
			proto_tree_add_bitmask(ntp_tree, tvb, 4, hf_ntpctrl_status, ett_ntpctrl_status, errorstatus, ENC_BIG_ENDIAN);
		} else {
			/* ...otherwise status word depends on OpCode */
			switch (flags2 & NTPCTRL_OP_MASK) {
			case NTPCTRL_OP_READSTAT:
			case NTPCTRL_OP_READVAR:
			case NTPCTRL_OP_WRITEVAR:
			case NTPCTRL_OP_ASYNCMSG:
				if (associd)
					proto_tree_add_bitmask(ntp_tree, tvb, 4, hf_ntpctrl_status, ett_ntpctrl_status, peer_status_flags, ENC_BIG_ENDIAN);
				else
				{
					/*
					 * dissect system status word:
					 *                      1
					 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
					 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
					 * |LI | ClkSource | Count | Code  |
					 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
					 */
					static const int *systemstatus[] = {
						&hf_ntpctrl_sys_status_li,
						&hf_ntpctrl_sys_status_clksrc,
						&hf_ntpctrl_sys_status_count,
						&hf_ntpctrl_sys_status_code,
						NULL
					};

					proto_tree_add_bitmask(ntp_tree, tvb, 4, hf_ntpctrl_status, ett_ntpctrl_status, systemstatus, ENC_BIG_ENDIAN);
				}
				break;
			case NTPCTRL_OP_READCLOCK:
			case NTPCTRL_OP_WRITECLOCK:
				{
				/*
				 * dissect clock status word:
				 *                      1
				 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
				 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
				 * | Clock Status  |  Event Code   |
				 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
				 */
				static const int *clockstatus[] = {
					&hf_ntpctrl_clk_status,
					&hf_ntpctrl_clk_status_code,
					NULL
				};

				proto_tree_add_bitmask(ntp_tree, tvb, 4, hf_ntpctrl_status, ett_ntpctrl_status, clockstatus, ENC_BIG_ENDIAN);
				}
				break;
			case NTPCTRL_OP_SETTRAP:
			case NTPCTRL_OP_UNSETTRAP:
			default:
				proto_tree_add_item(ntp_tree, hf_ntpctrl_status, tvb, 4, 2, ENC_BIG_ENDIAN);
				break;
			}
		}
	}
	else
	{
		proto_tree_add_item(ntp_tree, hf_ntpctrl_status, tvb, 4, 2, ENC_BIG_ENDIAN);
	}
	proto_tree_add_item(ntp_tree, hf_ntpctrl_associd, tvb, 6, 2, ENC_BIG_ENDIAN);
	proto_tree_add_item(ntp_tree, hf_ntpctrl_offset, tvb, 8, 2, ENC_BIG_ENDIAN);
	datalen = tvb_get_ntohs(tvb, 10);
	proto_tree_add_uint(ntp_tree, hf_ntpctrl_count, tvb, 10, 2, datalen);

	/*
	 * dissect Data part of the NTP control message
	 */
	if (datalen) {
		data_offset = 12;
		td = proto_tree_add_item(ntp_tree, hf_ntpctrl_data, tvb, data_offset, datalen, ENC_NA);
		data_tree = proto_item_add_subtree(td, ett_ntpctrl_data);
		switch(flags2 & NTPCTRL_OP_MASK) {
		case NTPCTRL_OP_READSTAT:
			if (!associd) {
				/*
				 * if associd == 0 then data part contains a list of the form
				 * <association identifier><status word>,
				 */
				while(datalen) {
					ti = proto_tree_add_item(data_tree, hf_ntpctrl_item, tvb, data_offset, 4, ENC_NA);
					item_tree = proto_item_add_subtree(ti, ett_ntpctrl_item);
					proto_tree_add_item(item_tree, hf_ntpctrl_associd, tvb, data_offset, 2, ENC_BIG_ENDIAN);
					data_offset += 2;
					proto_tree_add_bitmask(ntp_tree, tvb, data_offset, hf_ntpctrl_status, ett_ntpctrl_status, peer_status_flags, ENC_BIG_ENDIAN);
					data_offset += 2;
					datalen -= 4;
				}
				break;
			}
			/*
			 * but if associd != 0,
			 * then data part could be the same as if opcode is NTPCTRL_OP_READVAR
			 * --> so, no "break" here!
			 */
		case NTPCTRL_OP_READVAR:
		case NTPCTRL_OP_WRITEVAR:
		case NTPCTRL_OP_READCLOCK:
		case NTPCTRL_OP_WRITECLOCK:
			tt = tvbparse_init(tvb, data_offset, datalen, NULL, want_ignore);
			while( (element = tvbparse_get(tt, want)) != NULL ) {
				tvbparse_tree_add_elem(data_tree, element);
			}
			break;
		case NTPCTRL_OP_ASYNCMSG:
			proto_tree_add_item(data_tree, hf_ntpctrl_trapmsg, tvb, data_offset, datalen, ENC_ASCII|ENC_NA);
			break;
		/* these opcodes doesn't carry any data: NTPCTRL_OP_SETTRAP, NTPCTRL_OP_UNSETTRAP, NTPCTRL_OP_UNSPEC */
		}
	}

	data_offset = 12+datalen;
	length_remaining = tvb_reported_length_remaining(tvb, data_offset);

	/* Check if there is authentication */
	if ((flags2 & NTPCTRL_R_MASK) == 0)
	{
		if (length_remaining > 0)
		{
			auth_tree = proto_tree_add_subtree(ntp_tree, tvb, data_offset, -1, ett_ntp_authenticator, NULL, "Authenticator");
			switch (length_remaining)
			{
			case 20:
				ti = proto_tree_add_uint(auth_tree, hf_ntp_key_type, tvb, data_offset, 0, NTP_MD5_ALGO);
				PROTO_ITEM_SET_GENERATED(ti);
				proto_tree_add_item(auth_tree, hf_ntp_key_index, tvb, data_offset, 4, ENC_BIG_ENDIAN);
				proto_tree_add_item(auth_tree, hf_ntp_key_signature, tvb, data_offset+4, 16, ENC_NA);
				break;
			case 24:
				ti = proto_tree_add_uint(auth_tree, hf_ntp_key_type, tvb, data_offset, 0, NTP_SHA_ALGO);
				PROTO_ITEM_SET_GENERATED(ti);
				proto_tree_add_item(auth_tree, hf_ntp_key_index, tvb, data_offset, 4, ENC_BIG_ENDIAN);
				proto_tree_add_item(auth_tree, hf_ntp_key_signature, tvb, data_offset+4, 20, ENC_NA);
				break;
			}
		}
	}
}

/*
 * Initialize tvb-parser, which is used to dissect data part of NTP control
 * messages
 *
 * Here some constants are defined, which describes character groups used for
 * various purposes. These groups are then used to configure the two global
 * variables "want_ignore" and "want" that we use for dissection
 */
static void
init_parser(void)
{
	/* specify what counts as character */
	tvbparse_wanted_t *want_identifier = tvbparse_chars(-1, 1, 0,
		"abcdefghijklmnopqrstuvwxyz-_ABCDEFGHIJKLMNOPQRSTUVWXYZ.0123456789", NULL, NULL, NULL);
	/* this is the equal sign used in assignments */
	tvbparse_wanted_t *want_equalsign = tvbparse_char(-1, "=", NULL, NULL, NULL);
	/* possible characters allowed for values */
	tvbparse_wanted_t *want_value = tvbparse_set_oneof(0, NULL, NULL, NULL,
		tvbparse_quoted(-1, NULL, NULL, tvbparse_shrink_token_cb, '\"', '\\'),
		tvbparse_quoted(-1, NULL, NULL, tvbparse_shrink_token_cb, '\'', '\\'),
		tvbparse_chars(-1, 1, 0, "abcdefghijklmnopqrstuvwxyz-_ABCDEFGHIJKLMNOPQRSTUVWXYZ.0123456789 ", NULL, NULL, NULL),
		NULL);
	tvbparse_wanted_t *want_comma = tvbparse_until(-1, NULL, NULL, NULL,
		tvbparse_char(-1, ",", NULL, NULL, NULL), TP_UNTIL_SPEND);
	/* the following specifies an assignment of the form identifier=value */
	tvbparse_wanted_t *want_assignment = tvbparse_set_seq(-1, NULL, NULL, NULL,
		want_identifier,
		want_equalsign,
		tvbparse_some(-1, 0, 1, NULL, NULL, NULL, want_value),
		tvbparse_some(-1, 0, 1, NULL, NULL, NULL, want_comma),
		NULL);

	/* we ignore white space characters */
	want_ignore = tvbparse_chars(-1, 1, 0, " \t\r\n", NULL, NULL, NULL);
	/* data part of control messages consists of either identifiers or assignments */
	want = tvbparse_set_oneof(-1, NULL, NULL, NULL,
		want_assignment,
		want_identifier,
		NULL);
}

static void
dissect_ntp_priv(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *ntp_tree)
{
	guint8	impl, reqcode;
	static const int *priv_flags[] = {
		&hf_ntppriv_flags_r,
		&hf_ntppriv_flags_more,
		&hf_ntp_flags_vn,
		&hf_ntp_flags_mode,
		NULL
	};

	static const int *auth_flags[] = {
		&hf_ntppriv_auth,
		&hf_ntppriv_seq,
		NULL
	};

	proto_tree_add_bitmask(ntp_tree, tvb, 0, hf_ntp_flags, ett_ntp_flags, priv_flags, ENC_NA);
	proto_tree_add_bitmask(ntp_tree, tvb, 0, hf_ntppriv_auth_seq, ett_ntppriv_auth_seq, auth_flags, ENC_NA);

	impl = tvb_get_guint8(tvb, 2);
	proto_tree_add_uint(ntp_tree, hf_ntppriv_impl, tvb, 2, 1, impl);

	reqcode = tvb_get_guint8(tvb, 3);
	proto_tree_add_uint(ntp_tree, hf_ntppriv_reqcode, tvb, 3, 1, reqcode);

	if (impl == XNTPD && reqcode == MON_GETLIST_1) {

		guint16		numitems;
		guint16		itemsize;
		guint16		offset;
		guint		i;

		guint32		v6_flag = 0;

		proto_item*     monlist_item;
		proto_tree*     monlist_item_tree;

		proto_tree_add_bits_item(ntp_tree, hf_ntppriv_errcode, tvb, 32, 4, ENC_BIG_ENDIAN);
		proto_tree_add_bits_item(ntp_tree, hf_ntppriv_numitems, tvb, 36, 12, ENC_BIG_ENDIAN);
		proto_tree_add_bits_item(ntp_tree, hf_ntppriv_mbz, tvb, 48, 4, ENC_BIG_ENDIAN);
		proto_tree_add_bits_item(ntp_tree, hf_ntppriv_itemsize, tvb, 52, 12, ENC_BIG_ENDIAN);

		numitems = tvb_get_letohs(tvb, 5) & 0x0FFF;
		itemsize = tvb_get_letohs(tvb, 7) & 0x0FFF;

		for (i = 0; i < numitems; i++) {

			offset = 8 + itemsize * i;

			monlist_item = proto_tree_add_string_format(ntp_tree, hf_monlist_item, tvb, offset,
				itemsize, "Monlist Item", "Monlist item: address: %s:%u",
				tvb_ip_to_str(tvb, offset + 16), tvb_get_ntohs(tvb, offset + 28));
			monlist_item_tree = proto_item_add_subtree(monlist_item, ett_monlist_item);

			proto_tree_add_item(monlist_item_tree, hf_ntppriv_avgint, tvb, offset, 4, ENC_BIG_ENDIAN);
			proto_tree_add_item(monlist_item_tree, hf_ntppriv_lsint, tvb, offset + 4, 4, ENC_BIG_ENDIAN);
			proto_tree_add_item(monlist_item_tree, hf_ntppriv_restr, tvb, offset + 8, 4, ENC_BIG_ENDIAN);
			proto_tree_add_item(monlist_item_tree, hf_ntppriv_count, tvb, offset + 12, 4, ENC_BIG_ENDIAN);
			proto_tree_add_item(monlist_item_tree, hf_ntppriv_addr, tvb, offset + 16, 4, ENC_BIG_ENDIAN);
			proto_tree_add_item(monlist_item_tree, hf_ntppriv_daddr, tvb, offset + 20, 4, ENC_BIG_ENDIAN);
			proto_tree_add_item(monlist_item_tree, hf_ntppriv_flags, tvb, offset + 24, 4, ENC_BIG_ENDIAN);
			proto_tree_add_item(monlist_item_tree, hf_ntppriv_port, tvb, offset + 28, 2, ENC_BIG_ENDIAN);
			proto_tree_add_item(monlist_item_tree, hf_ntppriv_mode, tvb, offset + 30, 1, ENC_BIG_ENDIAN);
			proto_tree_add_item(monlist_item_tree, hf_ntppriv_version, tvb, offset + 31, 1, ENC_BIG_ENDIAN);
			proto_tree_add_item_ret_uint(monlist_item_tree, hf_ntppriv_v6_flag, tvb, offset + 32, 4, ENC_BIG_ENDIAN, &v6_flag);

			if (v6_flag != 0) {
				proto_tree_add_item(monlist_item_tree, hf_ntppriv_addr6, tvb, offset + 36, 16, ENC_NA);
				proto_tree_add_item(monlist_item_tree, hf_ntppriv_daddr6, tvb, offset + 52, 16, ENC_NA);
			}
		}
	}
}

/* dissect_ntp - dissects NTP packet data
 * tvb - tvbuff for packet data (IN)
 * pinfo - packet info
 * proto_tree - resolved protocol tree
 */
static int
dissect_ntp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
	proto_tree      *ntp_tree;
	proto_item      *ti = NULL;
	guint8		 flags;
	void (*dissector)(tvbuff_t *, packet_info *, proto_tree *);

	col_set_str(pinfo->cinfo, COL_PROTOCOL, "NTP");

	col_clear(pinfo->cinfo, COL_INFO);

	flags = tvb_get_guint8(tvb, 0);
	switch (flags & NTP_MODE_MASK) {
	default:
		dissector = dissect_ntp_std;
		break;
	case NTP_MODE_CTRL:
		dissector = dissect_ntp_ctrl;
		break;
	case NTP_MODE_PRIV:
		dissector = dissect_ntp_priv;
		break;
	}

	/* Adding NTP item and subtree */
	ti = proto_tree_add_item(tree, proto_ntp, tvb, 0, -1, ENC_NA);
	ntp_tree = proto_item_add_subtree(ti, ett_ntp);

	/* Show version and mode in info column and NTP root */
	col_add_fstr(pinfo->cinfo, COL_INFO, "%s, %s",
		val_to_str_const((flags & NTP_VN_MASK) >> 3, ver_nums,
				 "Unknown version"),
		val_to_str_const(flags & NTP_MODE_MASK, info_mode_types, "Unknown"));

	proto_item_append_text(ti, " (%s, %s)",
	                       val_to_str_const((flags & NTP_VN_MASK) >> 3, ver_nums,
						"Unknown version"),
	                       val_to_str_const(flags & NTP_MODE_MASK, info_mode_types, "Unknown"));

	/* Dissect according to mode */
	(*dissector)(tvb, pinfo, ntp_tree);
	return tvb_captured_length(tvb);
}

void
proto_register_ntp(void)
{
	static hf_register_info hf[] = {
		{ &hf_ntp_flags, {
			"Flags", "ntp.flags", FT_UINT8, BASE_HEX,
			NULL, 0, "Flags (Leap/Version/Mode)", HFILL }},
		{ &hf_ntp_flags_li, {
			"Leap Indicator", "ntp.flags.li", FT_UINT8, BASE_DEC,
			VALS(li_types), NTP_LI_MASK, "Warning of an impending leap second to be inserted or deleted in the last minute of the current month", HFILL }},
		{ &hf_ntp_flags_vn, {
			"Version number", "ntp.flags.vn", FT_UINT8, BASE_DEC,
			VALS(ver_nums), NTP_VN_MASK, NULL, HFILL }},
		{ &hf_ntp_flags_mode, {
			"Mode", "ntp.flags.mode", FT_UINT8, BASE_DEC,
			VALS(mode_types), NTP_MODE_MASK, NULL, HFILL }},
		{ &hf_ntp_stratum, {
			"Peer Clock Stratum", "ntp.stratum", FT_UINT8, BASE_DEC|BASE_RANGE_STRING,
			RVALS(stratum_rvals), 0, NULL, HFILL }},
		{ &hf_ntp_ppoll, {
			"Peer Polling Interval", "ntp.ppoll", FT_UINT8, BASE_DEC,
			NULL, 0, "Maximum interval between successive messages", HFILL }},
		{ &hf_ntp_precision, {
			"Peer Clock Precision", "ntp.precision", FT_INT8, BASE_DEC,
			NULL, 0, "The precision of the system clock", HFILL }},
		{ &hf_ntp_rootdelay, {
			"Root Delay", "ntp.rootdelay", FT_DOUBLE, BASE_NONE,
			NULL, 0, "Total round-trip delay to the reference clock", HFILL }},
		{ &hf_ntp_rootdispersion, {
			"Root Dispersion", "ntp.rootdispersion", FT_DOUBLE, BASE_NONE,
			NULL, 0, "Total dispersion to the reference clock", HFILL }},
		{ &hf_ntp_refid, {
			"Reference ID", "ntp.refid", FT_BYTES, BASE_NONE,
			NULL, 0, "Particular server or reference clock being used", HFILL }},
		{ &hf_ntp_reftime, {
			"Reference Timestamp", "ntp.reftime", FT_ABSOLUTE_TIME, ABSOLUTE_TIME_UTC,
			NULL, 0, "Time when the system clock was last set or corrected", HFILL }},
		{ &hf_ntp_org, {
			"Origin Timestamp", "ntp.org", FT_ABSOLUTE_TIME, ABSOLUTE_TIME_UTC,
			NULL, 0, "Time at the client when the request departed for the server", HFILL }},
		{ &hf_ntp_rec, {
			"Receive Timestamp", "ntp.rec", FT_ABSOLUTE_TIME, ABSOLUTE_TIME_UTC,
			NULL, 0, "Time at the server when the request arrived from the client", HFILL }},
		{ &hf_ntp_xmt, {
			"Transmit Timestamp", "ntp.xmt", FT_ABSOLUTE_TIME, ABSOLUTE_TIME_UTC,
			NULL, 0, "Time at the server when the response left for the client", HFILL }},
		{ &hf_ntp_keyid, {
			"Key ID", "ntp.keyid", FT_BYTES, BASE_NONE,
			NULL, 0, NULL, HFILL }},
		{ &hf_ntp_mac, {
			"Message Authentication Code", "ntp.mac", FT_BYTES, BASE_NONE,
			NULL, 0, NULL, HFILL }},
		{ &hf_ntp_key_type, {
			"Key type", "ntp.key_type", FT_UINT8, BASE_DEC,
			VALS(authentication_types), 0, "Authentication algorithm used", HFILL }},
		{ &hf_ntp_key_index, {
			"KeyIndex", "ntp.key_index", FT_UINT32, BASE_HEX,
			NULL, 0, NULL, HFILL }},
		{ &hf_ntp_key_signature, {
			"Signature", "ntp.key_signature", FT_BYTES, BASE_NONE,
			NULL, 0, NULL, HFILL }},

		{ &hf_ntp_ext, {
			"Extension", "ntp.ext", FT_NONE, BASE_NONE,
			NULL, 0, NULL, HFILL }},
		{ &hf_ntp_ext_flags, {
			"Flags", "ntp.ext.flags", FT_UINT8, BASE_HEX,
			NULL, 0, "Flags (Response/Error/Version)", HFILL }},
		{ &hf_ntp_ext_flags_r, {
			"Response bit", "ntp.ext.flags.r", FT_UINT8, BASE_DEC,
			VALS(ext_r_types), NTP_EXT_R_MASK, NULL, HFILL }},
		{ &hf_ntp_ext_flags_error, {
			"Error bit", "ntp.ext.flags.error", FT_UINT8, BASE_DEC,
			NULL, NTP_EXT_ERROR_MASK, NULL, HFILL }},
		{ &hf_ntp_ext_flags_vn, {
			"Version", "ntp.ext.flags.vn", FT_UINT8, BASE_DEC,
			NULL, NTP_EXT_VN_MASK, NULL, HFILL }},
		{ &hf_ntp_ext_op, {
			"Opcode", "ntp.ext.op", FT_UINT8, BASE_DEC,
			VALS(ext_op_types), 0, NULL, HFILL }},
		{ &hf_ntp_ext_len, {
			"Extension length", "ntp.ext.len", FT_UINT16, BASE_DEC,
			NULL, 0, NULL, HFILL }},
		{ &hf_ntp_ext_associd, {
			"Association ID", "ntp.ext.associd", FT_UINT32, BASE_DEC,
			NULL, 0, NULL, HFILL }},
		{ &hf_ntp_ext_tstamp, {
			"Timestamp", "ntp.ext.tstamp", FT_UINT32, BASE_HEX,
			NULL, 0, NULL, HFILL }},
		{ &hf_ntp_ext_fstamp, {
			"File Timestamp", "ntp.ext.fstamp", FT_UINT32, BASE_HEX,
			NULL, 0, NULL, HFILL }},
		{ &hf_ntp_ext_vallen, {
			"Value length", "ntp.ext.vallen", FT_UINT32, BASE_DEC,
			NULL, 0, NULL, HFILL }},
		{ &hf_ntp_ext_val, {
			"Value", "ntp.ext.val", FT_BYTES, BASE_NONE,
			NULL, 0, NULL, HFILL }},
		{ &hf_ntp_ext_siglen, {
			"Signature length", "ntp.ext.siglen", FT_UINT32, BASE_DEC,
			NULL, 0, NULL, HFILL }},
		{ &hf_ntp_ext_sig, {
			"Signature", "ntp.ext.sig", FT_BYTES, BASE_NONE,
			NULL, 0, NULL, HFILL }},

		{ &hf_ntpctrl_flags2, {
			"Flags 2", "ntp.ctrl.flags2", FT_UINT8, BASE_HEX,
			NULL, 0, "Flags (Response/Error/More/Opcode)", HFILL }},
		{ &hf_ntpctrl_flags2_r, {
			"Response bit", "ntp.ctrl.flags2.r", FT_UINT8, BASE_DEC,
			VALS(ctrl_r_types), NTPCTRL_R_MASK, NULL, HFILL }},
		{ &hf_ntpctrl_flags2_error, {
			"Error bit", "ntp.ctrl.flags2.error", FT_UINT8, BASE_DEC,
			NULL, NTPCTRL_ERROR_MASK, NULL, HFILL }},
		{ &hf_ntpctrl_flags2_more, {
			"More bit", "ntp.ctrl.flags2.more", FT_UINT8, BASE_DEC,
			NULL, NTPCTRL_MORE_MASK, NULL, HFILL }},
		{ &hf_ntpctrl_flags2_opcode, {
			"Opcode", "ntp.ctrl.flags2.opcode", FT_UINT8, BASE_DEC,
			VALS(ctrl_op_types), NTPCTRL_OP_MASK, NULL, HFILL }},
		{ &hf_ntpctrl_sequence, {
			"Sequence", "ntp.ctrl.sequence", FT_UINT16, BASE_DEC,
			NULL, 0, NULL, HFILL }},
		{ &hf_ntpctrl_status, {
			"Status", "ntp.ctrl.status", FT_UINT16, BASE_HEX,
			NULL, 0, NULL, HFILL }},
		{ &hf_ntpctrl_error_status_word, {
			"Error Status Word", "ntp.ctrl.err_status", FT_UINT16, BASE_DEC,
			VALS(ctrl_err_status_types), NTP_CTRL_ERRSTATUS_CODE_MASK, NULL, HFILL }},
		{ &hf_ntpctrl_sys_status_li, {
			"Leap Indicator", "ntp.ctrl.sys_status.li", FT_UINT16, BASE_DEC,
			VALS(li_types), NTPCTRL_SYSSTATUS_LI_MASK, "Warning of an impending leap second to be inserted or deleted in the last minute of the current month", HFILL }},
		{ &hf_ntpctrl_sys_status_clksrc, {
			"Clock Source", "ntp.ctrl.sys_status.clksrc", FT_UINT16, BASE_DEC,
			VALS(ctrl_sys_status_clksource_types), NTPCTRL_SYSSTATUS_CLK_MASK, NULL, HFILL }},
		{ &hf_ntpctrl_sys_status_count, {
			"System Event Counter", "ntp.ctrl.sys_status.count", FT_UINT16, BASE_DEC,
			NULL, NTPCTRL_SYSSTATUS_COUNT_MASK, NULL, HFILL }},
		{ &hf_ntpctrl_sys_status_code, {
			"System Event Code", "ntp.ctrl.sys_status.code", FT_UINT16, BASE_DEC,
			VALS(ctrl_sys_status_event_types), NTPCTRL_SYSSTATUS_CODE_MASK, NULL, HFILL }},
		{ &hf_ntpctrl_peer_status_b0, {
			"Peer Status", "ntp.ctrl.peer_status.config", FT_BOOLEAN, 16,
			TFS(&tfs_ctrl_peer_status_config), NTPCTRL_PEERSTATUS_CONFIG_MASK, NULL, HFILL }},
		{ &hf_ntpctrl_peer_status_b1, {
			"Peer Status", "ntp.ctrl.peer_status.authenable", FT_BOOLEAN, 16,
			TFS(&tfs_ctrl_peer_status_authenable), NTPCTRL_PEERSTATUS_AUTHENABLE_MASK, NULL, HFILL }},
		{ &hf_ntpctrl_peer_status_b2, {
			"Peer Status", "ntp.ctrl.peer_status.authentic", FT_BOOLEAN, 16,
			TFS(&tfs_ctrl_peer_status_authentic), NTPCTRL_PEERSTATUS_AUTHENTIC_MASK, NULL, HFILL }},
		{ &hf_ntpctrl_peer_status_b3, {
			"Peer Status", "ntp.ctrl.peer_status.reach", FT_BOOLEAN, 16,
			TFS(&tfs_ctrl_peer_status_reach), NTPCTRL_PEERSTATUS_REACH_MASK, NULL, HFILL }},
		{ &hf_ntpctrl_peer_status_b4, {
			"Peer Status: reserved", "ntp.ctrl.peer_status.reserved", FT_UINT16, BASE_DEC,
			NULL, NTPCTRL_PEERSTATUS_RESERVED_MASK, NULL, HFILL }},
		{ &hf_ntpctrl_peer_status_selection, {
			"Peer Selection", "ntp.ctrl.peer_status.selection", FT_UINT16, BASE_DEC,
			VALS(ctrl_peer_status_selection_types), NTPCTRL_PEERSTATUS_SEL_MASK, NULL, HFILL }},
		{ &hf_ntpctrl_peer_status_count, {
			"Peer Event Counter", "ntp.ctrl.peer_status.count", FT_UINT16, BASE_DEC,
			NULL, NTPCTRL_PEERSTATUS_COUNT_MASK, NULL, HFILL }},
		{ &hf_ntpctrl_peer_status_code, {
			"Peer Event Code", "ntp.ctrl.peer_status.code", FT_UINT16, BASE_DEC,
			VALS(ctrl_peer_status_event_types), NTPCTRL_PEERSTATUS_CODE_MASK, NULL, HFILL }},
		{ &hf_ntpctrl_clk_status, {
			"Clock Status", "ntp.ctrl.clock_status.status", FT_UINT16, BASE_DEC,
			VALS(ctrl_clk_status_types), NTPCTRL_CLKSTATUS_STATUS_MASK, NULL, HFILL }},
		{ &hf_ntpctrl_clk_status_code, {
			"Clock Event Code", "ntp.ctrl.clock_status.code", FT_UINT16, BASE_DEC,
			NULL, NTPCTRL_CLKSTATUS_CODE_MASK, NULL, HFILL }},
		{ &hf_ntpctrl_data, {
			"Data", "ntp.ctrl.data", FT_NONE, BASE_NONE,
			NULL, 0, NULL, HFILL }},
		{ &hf_ntpctrl_item, {
			"Item", "ntp.ctrl.item", FT_NONE, BASE_NONE,
			NULL, 0, NULL, HFILL }},
		{ &hf_ntpctrl_associd, {
			"AssociationID", "ntp.ctrl.associd", FT_UINT16, BASE_DEC,
			NULL, 0, NULL, HFILL }},
		{ &hf_ntpctrl_offset, {
			"Offset", "ntp.ctrl.offset", FT_UINT16, BASE_DEC,
			NULL, 0, NULL, HFILL }},
		{ &hf_ntpctrl_count, {
			"Count", "ntp.ctrl.count", FT_UINT16, BASE_DEC,
			NULL, 0, NULL, HFILL }},
		{ &hf_ntpctrl_trapmsg, {
			"Trap message", "ntp.ctrl.trapmsg", FT_STRING, BASE_NONE,
			NULL, 0, NULL, HFILL }},

		{ &hf_ntppriv_flags_r, {
			"Response bit", "ntp.priv.flags.r", FT_UINT8, BASE_DEC,
			VALS(priv_r_types), NTPPRIV_R_MASK, NULL, HFILL }},
		{ &hf_ntppriv_flags_more, {
			"More bit", "ntp.priv.flags.more", FT_UINT8, BASE_DEC,
			NULL, NTPPRIV_MORE_MASK, NULL, HFILL }},
		{ &hf_ntppriv_auth_seq, {
			"Auth, sequence", "ntp.priv.auth_seq", FT_UINT8, BASE_DEC,
			NULL, 0, "Auth bit, sequence number", HFILL }},
		{ &hf_ntppriv_auth, {
			"Auth bit", "ntp.priv.auth", FT_UINT8, BASE_DEC,
			NULL, NTPPRIV_AUTH_MASK, NULL, HFILL }},
		{ &hf_ntppriv_seq, {
			"Sequence number", "ntp.priv.seq", FT_UINT8, BASE_DEC,
			NULL, NTPPRIV_SEQ_MASK, NULL, HFILL }},
		{ &hf_ntppriv_impl, {
			"Implementation", "ntp.priv.impl", FT_UINT8, BASE_DEC,
			VALS(priv_impl_types), 0, NULL, HFILL }},
		{ &hf_ntppriv_reqcode, {
			"Request code", "ntp.priv.reqcode", FT_UINT8, BASE_DEC | BASE_EXT_STRING,
			&priv_rc_types_ext, 0, NULL, HFILL }},
		{ &hf_ntppriv_errcode, {
			"Err", "ntp.priv.err", FT_UINT8, BASE_HEX,
			VALS(err_values_types), 0, NULL, HFILL }},
		{ &hf_ntppriv_numitems, {
			"Number of data items", "ntp.priv.numitems", FT_UINT16, BASE_DEC,
			NULL, 0, NULL, HFILL }},
		{ &hf_ntppriv_mbz, {
			"Reserved", "ntp.priv.reserved", FT_UINT8, BASE_HEX,
			NULL, 0, NULL, HFILL }},
		{ &hf_monlist_item, {
			"Monlist item", "ntp.priv.monlist.item",
		         FT_STRINGZ, BASE_NONE, NULL, 0x00, NULL, HFILL }},
		{ &hf_ntppriv_itemsize, {
			"Size of data item", "ntp.priv.monlist.itemsize", FT_UINT16, BASE_HEX,
			NULL, 0, NULL, HFILL }},
		{ &hf_ntppriv_avgint, {
			"avgint", "ntp.priv.monlist.avgint", FT_UINT32, BASE_DEC,
			NULL, 0, NULL, HFILL }},
		{ &hf_ntppriv_lsint, {
			"lsint", "ntp.priv.monlist.lsint", FT_UINT32, BASE_DEC,
			NULL, 0, NULL, HFILL }},
		{ &hf_ntppriv_restr, {
			"restr", "ntp.priv.monlist.restr", FT_UINT32, BASE_HEX,
			NULL, 0, NULL, HFILL }},
		{ &hf_ntppriv_count, {
			"count", "ntp.priv.monlist.count", FT_UINT32, BASE_DEC,
			NULL, 0, NULL, HFILL }},
		{ &hf_ntppriv_addr, {
			"remote address", "ntp.priv.monlist.remote_address", FT_IPv4, BASE_NONE,
			NULL, 0, NULL, HFILL }},
		{ &hf_ntppriv_daddr, {
			"local address", "ntp.priv.monlist.local_address", FT_IPv4, BASE_NONE,
			NULL, 0, NULL, HFILL }},
		{ &hf_ntppriv_flags, {
			"flags", "ntp.priv.monlist.flags", FT_UINT32, BASE_HEX,
			NULL, 0, NULL, HFILL }},
		{ &hf_ntppriv_port, {
			"port", "ntp.priv.monlist.port", FT_UINT16, BASE_DEC,
			NULL, 0, NULL, HFILL }},
		{ &hf_ntppriv_mode, {
			"mode", "ntp.priv.monlist.mode", FT_UINT8, BASE_DEC,
			NULL, 0, NULL, HFILL }},
		{ &hf_ntppriv_version, {
			"version", "ntp.priv.monlist.version", FT_UINT8, BASE_DEC,
			NULL, 0, NULL, HFILL }},
		{ &hf_ntppriv_v6_flag, {
			"ipv6", "ntp.priv.monlist.ipv6", FT_UINT32, BASE_DEC,
			NULL, 0, NULL, HFILL }},
		{ &hf_ntppriv_addr6, {
			"ipv6 remote addr", "ntp.priv.monlist.addr6", FT_IPv6, BASE_NONE,
			NULL, 0, NULL, HFILL }},
		{ &hf_ntppriv_daddr6, {
			"ipv6 local addr", "ntp.priv.monlist.daddr6", FT_IPv6, BASE_NONE,
			NULL, 0, NULL, HFILL }}
	};
	static gint *ett[] = {
		&ett_ntp,
		&ett_ntp_flags,
		&ett_ntp_ext,
		&ett_ntp_ext_flags,
		&ett_ntpctrl_flags2,
		&ett_ntpctrl_status,
		&ett_ntpctrl_data,
		&ett_ntpctrl_item,
		&ett_ntppriv_auth_seq,
		&ett_monlist_item,
		&ett_ntp_authenticator
	};

	static ei_register_info ei[] = {
		{ &ei_ntp_ext, { "ntp.ext.invalid_length", PI_PROTOCOL, PI_WARN, "Extension invalid length", EXPFILL }},
	};

	expert_module_t* expert_ntp;

	proto_ntp = proto_register_protocol("Network Time Protocol", "NTP",
	    "ntp");
	proto_register_field_array(proto_ntp, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
	expert_ntp = expert_register_protocol(proto_ntp);
	expert_register_field_array(expert_ntp, ei, array_length(ei));

	init_parser();
}

void
proto_reg_handoff_ntp(void)
{
	dissector_handle_t ntp_handle;

	ntp_handle = create_dissector_handle(dissect_ntp, proto_ntp);
	dissector_add_uint("udp.port", UDP_PORT_NTP, ntp_handle);
	dissector_add_uint("tcp.port", TCP_PORT_NTP, ntp_handle);
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
