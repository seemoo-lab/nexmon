/* packet-llcgprs.c
 * Routines for Logical Link Control GPRS dissection ETSI 4.64(TS 101 351 V8.7.0)
 * Copyright 2000, Josef Korelus <jkor@quick.cz>
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
#include <epan/prefs.h>
#include <epan/expert.h>
#include <wiretap/wtap.h>

void proto_register_llcgprs(void);
void proto_reg_handoff_llcgprs(void);

#define I_FORMAT	1
#define S_FORMAT	2
#define UI_FORMAT	3
#define U_FORMAT	4
#define I_SACK		5
#define S_SACK		6
#define UI_MASK_FMT    0xe000
#define UI_MASK_SPB    0x1800
#define UI_MASK_NU     0x07fc
#define UI_MASK_E      0x0002
#define UI_MASK_PM     0x0001

#define N202           4U
#define CRC_LENGTH     3U
#define UI_HDR_LENGTH  3U

/* Initialize the protocol and registered fields */
static int proto_llcgprs       	  = -1;
static int hf_llcgprs_pd       	  = -1;
static int hf_llcgprs_fcs         = -1;
static int hf_llcgprs_cr       	  = -1;
static int hf_llcgprs_sapi     	  = -1;
static int hf_llcgprs_sapib    	  = -1;
static int hf_llcgprs_U_fmt    	  = -1; /* 3 upper bits in controlfield (UI format) */
static int hf_llcgprs_sp_bits  	  = -1; /* Spare bits in control field */
static int hf_llcgprs_NU       	  = -1; /* Transmited unconfirmed sequence number */
static int hf_llcgprs_E_bit    	  = -1; /* Encryption mode bit */
static int hf_llcgprs_PM_bit   	  = -1;
static int hf_llcgprs_Un       	  = -1;
static int hf_llcgprs_As      	  = -1;
static int hf_llcgprs_ucom     	  = -1;
static int hf_llcgprs_PF          = -1;
static int hf_llcgprs_S_fmt       = -1;
static int hf_llcgprs_NR          = -1;
static int hf_llcgprs_sjsd        = -1;
static int hf_llcgprs_k 	  = -1;
static int hf_llcgprs_isack_ns 	  = -1;
static int hf_llcgprs_isack_nr 	  = -1;
static int hf_llcgprs_isack_sfb   = -1;
static int hf_llcgprs_rbyte 	  = -1;
static int hf_llcgprs_kmask 	  = -1;
static int hf_llcgprs_ifmt 	  = -1;
static int hf_llcgprs_Ai 	  = -1;
static int hf_llcgprs_izerobit 	  = -1;
static int hf_llcgprs_sspare 	  = -1;
static int hf_llcgprs_xid_xl 	  = -1;
static int hf_llcgprs_xid_type 	  = -1;
static int hf_llcgprs_xid_len1 	  = -1;
static int hf_llcgprs_xid_len2 	  = -1;
static int hf_llcgprs_xid_spare   = -1;
static int hf_llcgprs_xid_byte 	  = -1;
static int hf_llcgprs_frmr_cf 	  = -1;
static int hf_llcgprs_frmr_spare  = -1;
static int hf_llcgprs_frmr_vs 	  = -1;
static int hf_llcgprs_frmr_vr 	  = -1;
static int hf_llcgprs_frmr_cr 	  = -1;
static int hf_llcgprs_frmr_w4 	  = -1;
static int hf_llcgprs_frmr_w3 	  = -1;
static int hf_llcgprs_frmr_w2 	  = -1;
static int hf_llcgprs_frmr_w1 	  = -1;
static int hf_llcgprs_tom_rl 	  = -1;
static int hf_llcgprs_tom_pd 	  = -1;
static int hf_llcgprs_tom_header  = -1;
static int hf_llcgprs_tom_data 	  = -1;

/* Unnumbered Commands and Responses (U Frames) */
#define U_DM	0x01
#define U_DISC	0x04
#define U_UA	0x06
#define U_SABM	0x07
#define U_FRMR	0x08
#define U_XID	0x0B
#define U_NULL	0x00

/* SAPI value constants */
#define SAPI_LLGMM	0x01
#define SAPI_TOM2	0x02
#define SAPI_LL3	0x03
#define SAPI_LL5	0x05
#define SAPI_LLSMS	0x07
#define SAPI_TOM8	0x08
#define SAPI_LL9	0x09
#define SAPI_LL11	0x0B

/* Initialize the subtree pointers */
static gint ett_llcgprs = -1;
static gint ett_llcgprs_adf = -1;
static gint ett_llcgprs_ctrlf = -1;
static gint ett_llcgprs_ui = -1;
static gint ett_llcgprs_sframe = -1;

static expert_field ei_llcgprs_no_info_field = EI_INIT;

static dissector_handle_t sndcp_xid_handle;

static gboolean ignore_cipher_bit = FALSE;

static dissector_table_t llcgprs_subdissector_table;

static const value_string sapi_t[] = {
	{  0, "Reserved"},
	{  1, "GPRS Mobility Management" },
	{  2, "Tunneling of messages 2" },
	{  3, "User data 3"},
	{  4, "Reserved" },
	{  5, "User data 5" },
	{  6, "Reserved" },
	{  7, "SMS" },
	{  8, "Tunneling of messages 8" },
	{  9, "User data 9" },
	{ 10, "Reserved" },
	{ 11, "User data 11" },
	{ 12, "Reserved" },
	{ 13, "Reserved" },
	{ 14, "Reserved" },
	{ 15, "Reserved" },
	{  0, NULL },
};
static value_string_ext sapi_t_ext = VALUE_STRING_EXT_INIT(sapi_t);

static const value_string sapi_abrv[] = {
	{  0, "Reserved 0"},
	{  1, "LLGMM" },
	{  2, "TOM2" },
	{  3, "LL3"},
	{  4, "Reserved 4" },
	{  5, "LL5" },
	{  6, "Reserved 6" },
	{  7, "LLSMS" },
	{  8, "TOM8" },
	{  9, "LL9" },
	{ 10, "Reserved 10" },
	{ 11, "LL11" },
	{ 12, "Reserved 12" },
	{ 13, "Reserved 13" },
	{ 14, "Reserved 14" },
	{ 15, "Reserved 15" },
	{ 0, NULL }
};
static value_string_ext sapi_abrv_ext = VALUE_STRING_EXT_INIT(sapi_abrv);

static const true_false_string a_bit = {
	"To solicit an acknowledgement from the peer LLE. ",
	"The peer LLE is not requested to send an acknowledgment."
};

static const true_false_string pd_bit = {
	"Invalid frame PD=1",
	"OK"
};
static const true_false_string e_bit = {
	" encrypted frame",
	" non encrypted frame"
};
static const true_false_string pm_bit = {
	"FCS covers the frame header and information fields",
	"FCS covers only the frame header and first N202 octets of the information field"
};
static const true_false_string cr_bit = {
	"DownLink/UpLink = Command/Response",
	"DownLink/UpLink = Response/Command"
};
/* bits are swaped comparing with "Table 3" in ETSI document */
static const value_string pme[] = {
	{ 0, "unprotected,non-ciphered information" },
	{ 1, "protected, non-ciphered information" },
	{ 2, "unprotected,ciphered information"},
	{ 3, "protected, ciphered information" },
	{ 0, NULL }
};

static const value_string xid_param_type_str[] = {
	{ 0x0, "Version (LLC version number)" },
	{ 0x1, "IOV-UI (ciphering Input offset value for UI frames)" },
	{ 0x2, "IOV-I (ciphering Input offset value for I frames)" },
	{ 0x3, "T200 (retransmission timeout)" },
	{ 0x4, "N200 (max number of retransmissions)" },
	{ 0x5, "N201-U (max info field length for U and UI frames)" },
	{ 0x6, "N201-I (max info field length for I frames)" },
	{ 0x7, "mD (I frame buffer size in the DL direction)" },
	{ 0x8, "mU (I frame buffer size in the UL direction)" },
	{ 0x9, "kD (window size in the DL direction)" },
	{ 0xA, "kU (window size in the UL direction)" },
	{ 0xB, "Layer-3 Parameters" },
	{ 0xC, "Reset" },
	{ 0, NULL }
};
static value_string_ext xid_param_type_str_ext = VALUE_STRING_EXT_INIT(xid_param_type_str);

static const value_string tompd_formats[] = {
	{ 0x0, "Not specified" },
	{ 0x1, "TIA/EIA-136" },
	{ 0x2, "RRLP" },
	{ 0x3, "Reserved value 3" },
	{ 0x4, "Reserved value 4" },
	{ 0x5, "Reserved value 5" },
	{ 0x6, "Reserved value 6" },
	{ 0x7, "Reserved value 7" },
	{ 0x8, "Reserved value 8" },
	{ 0x9, "Reserved value 9" },
	{ 0xA, "Reserved value 10" },
	{ 0xB, "Reserved value 11" },
	{ 0xC, "Reserved value 12" },
	{ 0xD, "Reserved value 13" },
	{ 0xE, "Reserved value 14" },
	{ 0xF, "Reserved for extension" },
	{ 0, NULL }
};

static const value_string cr_formats_unnumb[]= {
	{ 0x1, "DM-response" },
	{ 0x4, "DISC-command" },
	{ 0x6, "UA-response" },
	{ 0x7, "SABM" },
	{ 0x8, "FRMR" },
	{ 0xb, "XID" },
	{ 0, NULL }
};
static const value_string cr_formats_ipluss[] = {
	{ 0x0, "RR" },
	{ 0x1, "ACK" },
	{ 0x2, "RNR" },
	{ 0x3, "SACK" },
	{ 0, NULL }
};

/* CRC24 table - FCS */
guint32 tbl_crc24[256] = {
	0x00000000, 0x00d6a776, 0x00f64557, 0x0020e221, 0x00b78115, 0x00612663, 0x0041c442, 0x00976334,
	0x00340991, 0x00e2aee7, 0x00c24cc6, 0x0014ebb0, 0x00838884, 0x00552ff2, 0x0075cdd3, 0x00a36aa5,
	0x00681322, 0x00beb454, 0x009e5675, 0x0048f103, 0x00df9237, 0x00093541, 0x0029d760, 0x00ff7016,
	0x005c1ab3, 0x008abdc5, 0x00aa5fe4, 0x007cf892, 0x00eb9ba6, 0x003d3cd0, 0x001ddef1, 0x00cb7987,
	0x00d02644, 0x00068132, 0x00266313, 0x00f0c465, 0x0067a751, 0x00b10027, 0x0091e206, 0x00474570,
	0x00e42fd5, 0x003288a3, 0x00126a82, 0x00c4cdf4, 0x0053aec0, 0x008509b6, 0x00a5eb97, 0x00734ce1,
	0x00b83566, 0x006e9210, 0x004e7031, 0x0098d747, 0x000fb473, 0x00d91305, 0x00f9f124, 0x002f5652,
	0x008c3cf7, 0x005a9b81, 0x007a79a0, 0x00acded6, 0x003bbde2, 0x00ed1a94, 0x00cdf8b5, 0x001b5fc3,
	0x00fb4733, 0x002de045, 0x000d0264, 0x00dba512, 0x004cc626, 0x009a6150, 0x00ba8371, 0x006c2407,
	0x00cf4ea2, 0x0019e9d4, 0x00390bf5, 0x00efac83, 0x0078cfb7, 0x00ae68c1, 0x008e8ae0, 0x00582d96,
	0x00935411, 0x0045f367, 0x00651146, 0x00b3b630, 0x0024d504, 0x00f27272, 0x00d29053, 0x00043725,
	0x00a75d80, 0x0071faf6, 0x005118d7, 0x0087bfa1, 0x0010dc95, 0x00c67be3, 0x00e699c2, 0x00303eb4,
	0x002b6177, 0x00fdc601, 0x00dd2420, 0x000b8356, 0x009ce062, 0x004a4714, 0x006aa535, 0x00bc0243,
	0x001f68e6, 0x00c9cf90, 0x00e92db1, 0x003f8ac7, 0x00a8e9f3, 0x007e4e85, 0x005eaca4, 0x00880bd2,
	0x00437255, 0x0095d523, 0x00b53702, 0x00639074, 0x00f4f340, 0x00225436, 0x0002b617, 0x00d41161,
	0x00777bc4, 0x00a1dcb2, 0x00813e93, 0x005799e5, 0x00c0fad1, 0x00165da7, 0x0036bf86, 0x00e018f0,
	0x00ad85dd, 0x007b22ab, 0x005bc08a, 0x008d67fc, 0x001a04c8, 0x00cca3be, 0x00ec419f, 0x003ae6e9,
	0x00998c4c, 0x004f2b3a, 0x006fc91b, 0x00b96e6d, 0x002e0d59, 0x00f8aa2f, 0x00d8480e, 0x000eef78,
	0x00c596ff, 0x00133189, 0x0033d3a8, 0x00e574de, 0x007217ea, 0x00a4b09c, 0x008452bd, 0x0052f5cb,
	0x00f19f6e, 0x00273818, 0x0007da39, 0x00d17d4f, 0x00461e7b, 0x0090b90d, 0x00b05b2c, 0x0066fc5a,
	0x007da399, 0x00ab04ef, 0x008be6ce, 0x005d41b8, 0x00ca228c, 0x001c85fa, 0x003c67db, 0x00eac0ad,
	0x0049aa08, 0x009f0d7e, 0x00bfef5f, 0x00694829, 0x00fe2b1d, 0x00288c6b, 0x00086e4a, 0x00dec93c,
	0x0015b0bb, 0x00c317cd, 0x00e3f5ec, 0x0035529a, 0x00a231ae, 0x007496d8, 0x005474f9, 0x0082d38f,
	0x0021b92a, 0x00f71e5c, 0x00d7fc7d, 0x00015b0b, 0x0096383f, 0x00409f49, 0x00607d68, 0x00b6da1e,
	0x0056c2ee, 0x00806598, 0x00a087b9, 0x007620cf, 0x00e143fb, 0x0037e48d, 0x001706ac, 0x00c1a1da,
	0x0062cb7f, 0x00b46c09, 0x00948e28, 0x0042295e, 0x00d54a6a, 0x0003ed1c, 0x00230f3d, 0x00f5a84b,
	0x003ed1cc, 0x00e876ba, 0x00c8949b, 0x001e33ed, 0x008950d9, 0x005ff7af, 0x007f158e, 0x00a9b2f8,
	0x000ad85d, 0x00dc7f2b, 0x00fc9d0a, 0x002a3a7c, 0x00bd5948, 0x006bfe3e, 0x004b1c1f, 0x009dbb69,
	0x0086e4aa, 0x005043dc, 0x0070a1fd, 0x00a6068b, 0x003165bf, 0x00e7c2c9, 0x00c720e8, 0x0011879e,
	0x00b2ed3b, 0x00644a4d, 0x0044a86c, 0x00920f1a, 0x00056c2e, 0x00d3cb58, 0x00f32979, 0x00258e0f,
	0x00eef788, 0x003850fe, 0x0018b2df, 0x00ce15a9, 0x0059769d, 0x008fd1eb, 0x00af33ca, 0x007994bc,
	0x00dafe19, 0x000c596f, 0x002cbb4e, 0x00fa1c38, 0x006d7f0c, 0x00bbd87a, 0x009b3a5b, 0x004d9d2d
};

#define INIT_CRC24	0xffffff

static guint32 crc_calc(guint32 fcs, tvbuff_t *tvb, guint len)
{
	const guchar *cp;

	cp = tvb_get_ptr(tvb, 0, len);
	while (len--)
		fcs = (fcs >> 8) ^ tbl_crc24[(fcs ^ *cp++) & 0xff];
	return fcs;
}

typedef enum {
	FCS_VALID,
	FCS_NOT_VALID,
	FCS_NOT_VALID_DUE_TO_CIPHERING,
	FCS_NOT_COMPUTED
} fcs_status_t;


/* sub-dissector for XID data */
static void
llc_gprs_dissect_xid(tvbuff_t *tvb, packet_info *pinfo, proto_item *llcgprs_tree)
{
	guint8 xid_param_len = 0, byte1 = 0, byte2 = 0, tmp = 0;
	guint item_len = 0;
	guint location = 0;
	guint loop_counter = 0;
	proto_tree *uinfo_tree = NULL;
	proto_tree *xid_tree = NULL;
	guint info_len;

	info_len = tvb_reported_length(tvb);

	xid_tree = proto_tree_add_subtree_format(llcgprs_tree, tvb, 0, info_len,
				ett_llcgprs_ui, NULL, "Information Field: Length = %u", info_len);

	while (location < info_len)
	{
		/* parse the XID parameters */
		byte1 = tvb_get_guint8(tvb, location);

		if (byte1 & 0x80)
		{
			guint8 xid_param_len_high = 0;
			guint8 xid_param_len_low = 0;
			byte2 = tvb_get_guint8(tvb, location + 1);

			/* XL bit is set - length is continued in second byte */
			xid_param_len_high = byte1 & 0x03;
			xid_param_len_low = byte2 & 0xFC;

			/* bit shift the rest of the length */
			xid_param_len_low = xid_param_len_low >> 2;
			xid_param_len_low = xid_param_len_low & 0x3F;

			xid_param_len_high = xid_param_len_high << 6;
			xid_param_len_high = xid_param_len_high & 0xC0;

			/* combine the two */
			xid_param_len = xid_param_len_high | xid_param_len_low;
			item_len = xid_param_len + 2;

		}
		else
		{
			xid_param_len = byte1 & 0x3;
			item_len = xid_param_len + 1;
		}

		tmp =  byte1 & 0x7C;
		tmp = tmp >> 2;

		if (tmp == 0xB) /* L3 XID parameters, call the SNDCP-XID dissector */
		{
			tvbuff_t	*sndcp_xid_tvb;
			guint8 sndcp_xid_offset;

			uinfo_tree = proto_tree_add_subtree(xid_tree, tvb, location, item_len,
				ett_llcgprs_ui, NULL, "XID parameter Type: L3 parameters");
			proto_tree_add_uint(uinfo_tree, hf_llcgprs_xid_xl, tvb, location, 1, byte1);
			proto_tree_add_uint(uinfo_tree, hf_llcgprs_xid_type, tvb, location, 1, byte1);
			proto_tree_add_uint(uinfo_tree, hf_llcgprs_xid_len1, tvb, location, 1, byte1);
			if (byte1 & 0x80) {
				proto_tree_add_uint(uinfo_tree, hf_llcgprs_xid_len2, tvb, location+1, 1, byte2);
				proto_tree_add_uint(uinfo_tree, hf_llcgprs_xid_spare, tvb, location+1, 1, byte2);
				sndcp_xid_offset = 2;
			}
			else
			{
				sndcp_xid_offset = 1;
			}

			if (xid_param_len) {

				sndcp_xid_tvb = tvb_new_subset_length (tvb, location+sndcp_xid_offset, xid_param_len);

				if(sndcp_xid_handle)
				{
					call_dissector(sndcp_xid_handle, sndcp_xid_tvb, pinfo, uinfo_tree);
				}
			}

			location += item_len;
		}
		else
		{
			if (( xid_param_len > 0 ) && ( xid_param_len <=4 ))
			{
				guint32 value = 0;
				guint8 i;
				for (i=1;i<=xid_param_len;i++)
				{
					value <<= 8;
					value |= (guint32)tvb_get_guint8(tvb, location+i );
				}
				uinfo_tree = proto_tree_add_subtree_format(xid_tree, tvb, location, item_len,
					ett_llcgprs_ui, NULL, "XID Parameter Type: %s - Value: %u",
					val_to_str_ext_const(tmp, &xid_param_type_str_ext, "Reserved Type:%X"), value);
			}
			else
			{
				uinfo_tree = proto_tree_add_subtree_format(xid_tree, tvb, location, item_len,
					ett_llcgprs_ui, NULL, "XID Parameter Type: %s",
					val_to_str_ext_const(tmp, &xid_param_type_str_ext, "Reserved Type:%X"));
			}
			proto_tree_add_uint(uinfo_tree, hf_llcgprs_xid_xl, tvb, location,
				1, byte1);
			proto_tree_add_uint(uinfo_tree, hf_llcgprs_xid_type, tvb, location,
				1, byte1);
			proto_tree_add_uint(uinfo_tree, hf_llcgprs_xid_len1, tvb, location,
				1, byte1);

			if (byte1 & 0x80) {
				/* length continued into byte 2 */
				proto_tree_add_uint(uinfo_tree, hf_llcgprs_xid_len2, tvb, location,
					1, byte2);
				proto_tree_add_uint(uinfo_tree, hf_llcgprs_xid_spare, tvb, location,
					1, byte2);

				/* be sure to account for the second byte of length */
				location++;
			}

			location++;
			for (loop_counter = 0; loop_counter < xid_param_len; loop_counter++)
			{
				/* grab the information in the XID param */
				byte2 = tvb_get_guint8(tvb, location);
				proto_tree_add_uint(uinfo_tree, hf_llcgprs_xid_byte, tvb, location,
					1, byte2);
				location++;
			}
		}
	}
}


static int
dissect_llcgprs(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
	guint offset = 0;
	guint8 addr_fld=0, sapi=0, ctrl_fld_fb=0, frame_format, tmp=0;
	guint16 epm = 0, nu=0, ctrl_fld_ui_s=0;
	proto_item *ti, *addres_field_item;
	proto_tree *llcgprs_tree=NULL , *ad_f_tree =NULL, *ctrl_f_tree=NULL, *ui_tree=NULL;
	tvbuff_t *next_tvb;
	guint length, captured_length;
	guint crc_length = 0, llc_data_length;
	guint32 fcs=0;
	guint32 fcs_calc=0;
	fcs_status_t fcs_status;
	guint16 ns = 0;
	guint16 nr = 0;
	guint8 k = 0;
	guint8 m_bits = 0;
	guint info_len;
	proto_tree *uinfo_tree = NULL;
	gboolean ciphered_ui_frame = FALSE;

	col_set_str(pinfo->cinfo, COL_PROTOCOL, "GPRS-LLC");

	/* llc_data_length includes the header and the payload */
	llc_data_length = length = tvb_reported_length(tvb);
	if (length >= CRC_LENGTH) {
		/*
		 * The packet is big enough to have a CRC.
		 */
		llc_data_length -= CRC_LENGTH;
	}
	else
	{
		/*
		 * The packet isn't big enough to have a CRC; claim
		 * the data length is 0.
		 */
		llc_data_length = 0;
	}
	captured_length = tvb_captured_length(tvb);

	/*
	 * Address field.
	 */
	addr_fld = tvb_get_guint8(tvb, offset);
	offset++;

	if (addr_fld > 128 )
	{
		col_set_str(pinfo->cinfo, COL_INFO, "Invalid packet - Protocol Discriminator bit is set to 1");
		return offset;
	}

	sapi = addr_fld & 0xF;

	col_add_fstr(pinfo->cinfo, COL_INFO, "SAPI: %s", val_to_str_ext_const(sapi, &sapi_abrv_ext, "Unknown (%d)"));

	/* In the interest of speed, if "tree" is NULL, don't do any work not
		necessary to generate protocol tree items. */
	if (tree)
	{
		ti = proto_tree_add_protocol_format(tree, proto_llcgprs, tvb, 0, -1,
						    "MS-SGSN LLC (Mobile Station - Serving GPRS Support Node Logical Link Control)  SAPI: %s",
						    val_to_str_ext_const(sapi, &sapi_t_ext, "Unknown (%u)"));

		llcgprs_tree = proto_item_add_subtree(ti, ett_llcgprs);

		/* add an item to the subtree, see section 1.6 for more information */
		addres_field_item = proto_tree_add_uint_format(llcgprs_tree, hf_llcgprs_sapi,
		     tvb, 0, 1, sapi, "Address field  SAPI: %s", val_to_str_ext_const(sapi, &sapi_abrv_ext, "Unknown (%d)"));

		ad_f_tree = proto_item_add_subtree(addres_field_item, ett_llcgprs_adf);
		proto_tree_add_boolean(ad_f_tree, hf_llcgprs_pd, tvb, 0, 1, addr_fld );
		proto_tree_add_boolean(ad_f_tree, hf_llcgprs_cr, tvb, 0, 1, addr_fld );
		proto_tree_add_uint(ad_f_tree, hf_llcgprs_sapib, tvb, 0, 1, addr_fld );
	}

	/*
	 * Control field.
	 */
	ctrl_fld_fb = tvb_get_guint8(tvb, offset);
	if (ctrl_fld_fb < 0xC0)
	{
		frame_format = (ctrl_fld_fb < 0x80)? I_FORMAT : S_FORMAT;
	}
	else
	{
		frame_format = (ctrl_fld_fb < 0xe0 )? UI_FORMAT : U_FORMAT;
	}

	switch (frame_format)
	{
	case I_FORMAT:
		col_append_str(pinfo->cinfo, COL_INFO, ", I, ");

		ns = tvb_get_ntohs(tvb, offset);
		ns = (ns >> 4)& 0x01FF;
		nr = ctrl_fld_ui_s = tvb_get_ntohs(tvb, offset + 1);
		nr = (nr >> 2) & 0x01FF;

		epm = ctrl_fld_ui_s & 0x3;

		col_append_str(pinfo->cinfo, COL_INFO, val_to_str(epm, cr_formats_ipluss, "Unknown (%d)"));
		col_append_fstr(pinfo->cinfo, COL_INFO, ", N(S) = %u", ns);
		col_append_fstr(pinfo->cinfo, COL_INFO, ", N(R) = %u", nr);

		if (tree)
		{
			guint32 tmpx;

			ctrl_f_tree = proto_tree_add_subtree_format(llcgprs_tree, tvb, offset,
							      3, ett_llcgprs_sframe, NULL, "Information format: %s: N(S) = %u,  N(R) = %u",
							      val_to_str(epm, cr_formats_ipluss, "Unknown (%d)"), ns, nr);

			/* retrieve the second octet */
			tmpx = tvb_get_ntohs(tvb, offset)  << 16;
			proto_tree_add_uint(ctrl_f_tree, hf_llcgprs_ifmt, tvb, offset, 3, tmpx);
			proto_tree_add_boolean(ctrl_f_tree, hf_llcgprs_Ai, tvb, offset, 3, tmpx);
			proto_tree_add_uint(ctrl_f_tree, hf_llcgprs_izerobit, tvb, offset, 3, tmpx);

			tmpx = ns << 12;
			proto_tree_add_uint(ctrl_f_tree, hf_llcgprs_isack_ns, tvb, offset, 3, tmpx);

			tmpx = nr << 2;
			proto_tree_add_uint(ctrl_f_tree, hf_llcgprs_isack_nr, tvb, offset, 3, tmpx);
			proto_tree_add_uint(ctrl_f_tree, hf_llcgprs_isack_sfb, tvb, offset, 3, ctrl_fld_ui_s);
		}

		/* advance to either R Bitmap or Payload */
		offset += 3;

		/* check to see if epm is SACK - meaning this is an ISACK frame */
		if (epm == 0x03)
		{
			guint8 kmask;
			/* SACK Frame */
			k = kmask = tvb_get_guint8(tvb, offset);
			k = k & 0x1F;

			/* advance past the k field */
			offset++;

			/* the real value of k is actually k + 1 */
			/* account for the off by one representation */
			k++;

			col_append_fstr(pinfo->cinfo, COL_INFO, ", k = %u", k);

			if (tree)
			{
				guint8 loop_count = 0;
				guint8 r_byte = 0;
				guint location = offset;

				ctrl_f_tree = proto_tree_add_subtree_format(llcgprs_tree, tvb, (offset-1),
								      (k+1), ett_llcgprs_sframe, NULL, "SACK FRAME: k = %u", k);

				proto_tree_add_uint(ctrl_f_tree, hf_llcgprs_kmask, tvb, offset-1, 1, kmask);
				proto_tree_add_uint(ctrl_f_tree, hf_llcgprs_k, tvb, offset-1, 1, k);

				/* display the R Bitmap */
				for (loop_count = 0; loop_count < k; loop_count++)
				{
					r_byte = tvb_get_guint8(tvb, location);
					proto_tree_add_uint(ctrl_f_tree, hf_llcgprs_rbyte, tvb, location, 1, r_byte);
					location++;
				}
			}

			/* step past the R Bitmap */
			offset += k;
		}

		crc_length = llc_data_length;
		break;

	case S_FORMAT:
		col_append_str(pinfo->cinfo, COL_INFO, ", S, ");

		nu = ctrl_fld_ui_s = tvb_get_ntohs(tvb, offset);
		epm = ctrl_fld_ui_s & 0x3;
		nu = (nu >>2)&0x01FF;

		col_append_str(pinfo->cinfo, COL_INFO, val_to_str(epm, cr_formats_ipluss, "Unknown (%d)"));
		col_append_fstr(pinfo->cinfo, COL_INFO, ", N(R) = %u", nu);

		if (tree)
		{
			static const int * s_formats[] = {
				&hf_llcgprs_S_fmt,
				&hf_llcgprs_As,
				&hf_llcgprs_sspare,
				&hf_llcgprs_NR,
				&hf_llcgprs_sjsd,
				NULL
			};

			proto_tree_add_bitmask_text(llcgprs_tree, tvb, offset, 2,
							      "Supervisory format: ", NULL, ett_llcgprs_sframe, s_formats, ENC_BIG_ENDIAN, 0);
		}
		offset += 2;

		if ((ctrl_fld_ui_s & 0x03) == 0x03)
			/* It is a SACK frame */
		{
			/* TODO: length is fudged - it is not correct */
			guint32 sack_length = llc_data_length - offset;

			if (tree)
			{
				guint loop_count;
				guint8 r_byte;
				guint16 location = offset;
				ctrl_f_tree = proto_tree_add_subtree_format(llcgprs_tree, tvb, offset, sack_length,
								      ett_llcgprs_sframe, NULL, "SACK FRAME: length = %u", sack_length);
				/* display the R Bitmap */
				for (loop_count = 0; loop_count < sack_length; loop_count++)
				{
					r_byte = tvb_get_guint8(tvb, location);
					proto_tree_add_uint(ctrl_f_tree, hf_llcgprs_rbyte, tvb,
							    location, 1, r_byte);
					location++;
				}

				/* step past the r bitmap */
				offset += sack_length;
			}
		}

		crc_length = llc_data_length;
		break;

	case UI_FORMAT:
		col_append_str(pinfo->cinfo, COL_INFO, ", UI, ");

		nu = ctrl_fld_ui_s = tvb_get_ntohs(tvb, offset);
		epm = ctrl_fld_ui_s & 0x3;
		nu = (nu >>2)&0x01FF;

		/* If the frame is ciphered, the calculated FCS will not be valid (unless it has been unciphered) */
		if (epm & UI_MASK_E)
		{
			ciphered_ui_frame = TRUE;
		}
		if ((epm & UI_MASK_PM)== 0)
		{
			/* FCS covers at maximum the LLC header and N202 bytes */
			crc_length = MIN(UI_HDR_LENGTH + N202, llc_data_length);
		}
		else
		{
			crc_length = llc_data_length;
		}

		col_append_str(pinfo->cinfo, COL_INFO, val_to_str(epm, pme, "Unknown (%d)"));
		col_append_fstr(pinfo->cinfo, COL_INFO, ", N(U) = %u", nu);

		if (tree)
		{
			static const int * i_formats[] = {
				&hf_llcgprs_U_fmt,
				&hf_llcgprs_sp_bits,
				&hf_llcgprs_NU,
				&hf_llcgprs_E_bit,
				&hf_llcgprs_PM_bit,
				NULL
			};

			proto_tree_add_bitmask_text(llcgprs_tree, tvb, offset, 2,
							      "Unconfirmed Information format - UI: ", NULL, ett_llcgprs_ctrlf, i_formats, ENC_BIG_ENDIAN, 0);
		}
		offset += 2;
		break;

	case U_FORMAT:
		col_append_str(pinfo->cinfo, COL_INFO, ", U, ");

		tmp =  ctrl_fld_fb & 0xf;

		col_append_str(pinfo->cinfo, COL_INFO,
			       val_to_str(tmp, cr_formats_unnumb, "Unknown/invalid code:%X"));

		ui_tree = proto_tree_add_subtree_format(llcgprs_tree, tvb, offset, (llc_data_length-1),
						    ett_llcgprs_ui, NULL, "Unnumbered frame: %s",
						    val_to_str(tmp, cr_formats_unnumb, "Unknown/invalid code:%X"));

		proto_tree_add_uint(ui_tree, hf_llcgprs_Un, tvb, offset, 1, ctrl_fld_fb);
		proto_tree_add_boolean(ui_tree, hf_llcgprs_PF, tvb, offset, 1, ctrl_fld_fb);
		proto_tree_add_uint(ui_tree, hf_llcgprs_ucom, tvb, offset, 1, ctrl_fld_fb);
		offset += 1;
		crc_length = llc_data_length;
		break;
	}

	/*
	 * FCS.
	 */
	if (captured_length >= length && length >= CRC_LENGTH)
	{
		/*
		 * We have all the packet data, including the full FCS,
		 * so we can compute the FCS.
		 */
		fcs_calc = crc_calc ( INIT_CRC24 , tvb, crc_length );
		fcs_calc = ~fcs_calc;
		fcs_calc &= 0xffffff;

		fcs = tvb_get_letoh24(tvb, llc_data_length);
		if ( fcs_calc == fcs )
		{
			fcs_status = FCS_VALID;
			proto_tree_add_uint_format_value(llcgprs_tree, hf_llcgprs_fcs, tvb, llc_data_length, CRC_LENGTH,
				fcs_calc&0xffffff, "0x%06x (correct)", fcs_calc&0xffffff);
		}
		else
		{
			if (ciphered_ui_frame)
			{
				fcs_status = FCS_NOT_VALID_DUE_TO_CIPHERING;
				proto_tree_add_uint_format_value(llcgprs_tree, hf_llcgprs_fcs, tvb, llc_data_length, CRC_LENGTH,
					fcs, "0x%06x  (incorrect, maybe due to ciphering, calculated 0x%06x)", fcs, fcs_calc );
			}
			else
			{
				fcs_status = FCS_NOT_VALID;
				proto_tree_add_uint_format_value(llcgprs_tree, hf_llcgprs_fcs, tvb, llc_data_length, CRC_LENGTH,
					fcs, "0x%06x  (incorrect, should be 0x%06x)", fcs, fcs_calc );
			}
		}
	}
	else
	{
		/* We don't have enough data to compute the FCS. */
		fcs_status = FCS_NOT_COMPUTED;
		proto_tree_add_uint_format_value(llcgprs_tree, hf_llcgprs_fcs, tvb, 0, 0, 0,
			"FCS: Not enough data to compute the FCS");
	}

	/*
	 * Information field.
	 */
	switch (frame_format)
	{
	case I_FORMAT:
		if ((sapi == SAPI_TOM2) || (sapi == SAPI_TOM8))
		{
			/* if SAPI is TOM do other parsing */
			if (tree)
			{
				guint8 tom_byte = 0;
				guint8 remaining_length = 0;
				guint8 tom_pd = 0;
				int loop_counter = 0;

				tom_byte = tvb_get_guint8(tvb, offset);
				remaining_length = (tom_byte >> 4) & 0x0F;
				tom_pd = tom_byte & 0x0F;

				ctrl_f_tree = proto_tree_add_subtree_format(llcgprs_tree, tvb, offset,
								      (llc_data_length-offset), ett_llcgprs_sframe, NULL,
								      "TOM Envelope - Protocol: %s",
								      val_to_str(tom_pd, tompd_formats, "Unknown (%d)"));

				proto_tree_add_uint(ctrl_f_tree, hf_llcgprs_tom_rl, tvb, offset, 1, tom_byte);
				proto_tree_add_uint(ctrl_f_tree, hf_llcgprs_tom_pd, tvb, offset, 1, tom_byte);

				/* step past the TOM header first byte */
				offset++;

				/* TOM remaining length field value 0x0F is reserved for extension */
				if (remaining_length != 0x0F)
				{
					/* parse the rest of the TOM header */
					for (loop_counter = 0; loop_counter < remaining_length; loop_counter++)
					{
						tom_byte = tvb_get_guint8(tvb, offset);

						proto_tree_add_uint(ctrl_f_tree, hf_llcgprs_tom_header, tvb,
								    offset, 1, tom_byte);

						/* step to the next byte */
						offset++;
					}

					remaining_length = llc_data_length - offset;

					/* parse the TOM message capsule */
					for (loop_counter = 0; loop_counter < remaining_length; loop_counter++)
					{
						tom_byte = tvb_get_guint8(tvb, offset);

						proto_tree_add_uint(ctrl_f_tree, hf_llcgprs_tom_data, tvb,
								    offset, 1, tom_byte);

						/* step to the next byte */
						offset++;
					}
				}
			}
		}
		else
		{
			/* otherwise - call a subdissector */
			next_tvb = tvb_new_subset_length(tvb, offset, (llc_data_length-offset));
			if (!dissector_try_uint(llcgprs_subdissector_table, sapi, next_tvb, pinfo, tree))
				/* if no subdissector is found, call the data dissector */
			{
				call_data_dissector(next_tvb, pinfo, tree);
			}
		}
		break;

	case S_FORMAT:
		/* should parse the rest of the supervisory message based on type */
		/* if SAPI is TOM do other parsing */
		if ((sapi == SAPI_TOM2) || (sapi == SAPI_TOM8))
		{
			if (tree)
			{
				guint8 tom_byte = 0;
				guint8 remaining_length = 0;
				guint8 tom_pd = 0;
				int loop_counter = 0;

				tom_byte = tvb_get_guint8(tvb, offset);
				remaining_length = (tom_byte >> 4) & 0x0F;
				tom_pd = tom_byte & 0x0F;

				ctrl_f_tree = proto_tree_add_subtree_format(llcgprs_tree, tvb, offset,
								      (llc_data_length-offset), ett_llcgprs_sframe, NULL,
								      "TOM Envelope - Protocol: %s",
								      val_to_str(tom_pd, tompd_formats, "Unknown (%d)"));

				proto_tree_add_uint(ctrl_f_tree, hf_llcgprs_tom_rl, tvb, offset, 1, tom_byte);
				proto_tree_add_uint(ctrl_f_tree, hf_llcgprs_tom_pd, tvb, offset, 1, tom_byte);

				/* step past the TOM header first byte */
				offset++;

				/* TOM remaining length field value 0x0F is reserved for extension */
				if (remaining_length != 0x0F)
				{
					/* parse the rest of the TOM header */
					for (loop_counter = 0; loop_counter < remaining_length; loop_counter++)
					{
						tom_byte = tvb_get_guint8(tvb, offset);
						proto_tree_add_uint(ctrl_f_tree, hf_llcgprs_tom_header, tvb,
								    offset, 1, tom_byte);

						/* step to the next byte */
						offset++;
					}

					/* Amount of frame left from offset to crc */
					remaining_length = llc_data_length - offset;

					/* parse the TOM message capsule */
					for (loop_counter = 0; loop_counter < remaining_length; loop_counter++)
					{
						tom_byte = tvb_get_guint8(tvb, offset);
						proto_tree_add_uint(ctrl_f_tree, hf_llcgprs_tom_data, tvb,
								    offset, 1, tom_byte);

						/* step to the next byte */
						offset++;
					}
				}
			}
		}
		else if (llc_data_length>offset)
		{
			/* otherwise - call a subdissector */
			next_tvb = tvb_new_subset_length(tvb, offset, (llc_data_length-offset));
			if (!dissector_try_uint(llcgprs_subdissector_table, sapi, next_tvb, pinfo, tree))
			{
				call_data_dissector(next_tvb, pinfo, tree);
			}
		}
		break;

	case UI_FORMAT:
		next_tvb = tvb_new_subset_length(tvb, offset, (llc_data_length-offset));

		if ((ignore_cipher_bit && (fcs_status == FCS_VALID)) || !(epm & 0x2))
		{
			/* Either we're ignoring the cipher bit
			 * (because the bit is set but the
			 * data is unciphered), and the data has
			 * a valid FCS, or the cipher
			 * bit isn't set (indicating that the
			 * data is unciphered).  Try dissecting
			 * it with a subdissector. */

			/* if SAPI is TOM do other parsing */
			if ((sapi == SAPI_TOM2) || (sapi == SAPI_TOM8))
			{
				if (tree)
				{
					guint8 tom_byte = 0;
					guint8 remaining_length = 0;
					guint8 tom_pd = 0;
					int loop_counter = 0;

					tom_byte = tvb_get_guint8(tvb, offset);
					remaining_length = (tom_byte >> 4) & 0x0F;
					tom_pd = tom_byte & 0x0F;

					ctrl_f_tree = proto_tree_add_subtree_format(llcgprs_tree, tvb, offset,
									      (llc_data_length-offset), ett_llcgprs_sframe, NULL,
									      "TOM Envelope - Protocol: %s",
									      val_to_str(tom_pd, tompd_formats, "Unknown (%d)"));

					proto_tree_add_uint(ctrl_f_tree, hf_llcgprs_tom_rl, tvb, offset, 1, tom_byte);
					proto_tree_add_uint(ctrl_f_tree, hf_llcgprs_tom_pd, tvb, offset, 1, tom_byte);

					/* step past the TOM header first byte */
					offset++;

					/* TOM remaining length field value 0x0F is reserved for extension */
					if (remaining_length != 0x0F)
					{
						/* parse the rest of the TOM header */
						for (loop_counter = 0; loop_counter < remaining_length; loop_counter++)
						{
							tom_byte = tvb_get_guint8(tvb, offset);

							proto_tree_add_uint(ctrl_f_tree, hf_llcgprs_tom_header, tvb,
									    offset, 1, tom_byte);

							/* step to the next byte */
							offset++;
						}

						/* Amount of frame left from offset to crc */
						remaining_length = llc_data_length - offset;

						/* parse the TOM message capsule */
						for (loop_counter = 0; loop_counter < remaining_length; loop_counter++)
						{
							tom_byte = tvb_get_guint8(tvb, offset);

							proto_tree_add_uint(ctrl_f_tree, hf_llcgprs_tom_data, tvb,
									    offset, 1, tom_byte);

							/* step to the next byte */
							offset++;
						}
					}
				}
			}
			else if (llc_data_length>offset)
			{
				/* otherwise - call a subdissector */
				if (!dissector_try_uint(llcgprs_subdissector_table, sapi, next_tvb, pinfo, tree))
				{
					call_data_dissector(next_tvb, pinfo, tree);
				}
			}
		}
		else
		{
			/* ciphered information - just parse it as data */
			call_data_dissector(next_tvb, pinfo, tree);
		}
		break;

	case U_FORMAT:
		m_bits = ctrl_fld_fb & 0x0F;

		info_len = llc_data_length - offset;

		switch (m_bits)
		{
		case U_DM:
		case U_DISC:
		case U_NULL:
			/* These frames SHOULD NOT have an info field */
			proto_tree_add_expert(llcgprs_tree, pinfo, &ei_llcgprs_no_info_field, tvb, offset, (llc_data_length-2));
			break;
		case U_UA:
			/* This frame MAY or MAY NOT have an info field */
			/* Info field, if it exists, consists of XID parameters */
			if (tree)
			{
				tvbuff_t	*xid_tvb;
				xid_tvb = tvb_new_subset_length (tvb, offset, info_len);

				llc_gprs_dissect_xid(xid_tvb, pinfo, llcgprs_tree);
			}
			break;
		case U_SABM:
		case U_XID:
			/* These frames do have info fields consisting of XID parameters */
			/* Info field consists of XID parameters */
			if (tree)
			{
				tvbuff_t	*xid_tvb;
				xid_tvb = tvb_new_subset_length (tvb, offset, info_len);

				llc_gprs_dissect_xid(xid_tvb, pinfo, llcgprs_tree);
			}
			break;
		case U_FRMR:
			/* This frame has a special format info field */
			if (tree)
			{
				guint32 fld_vars = 0;
				guint16 cf_byte = 0;
				int loop_counter = 0;
				int location = 0;

				ui_tree = proto_tree_add_subtree_format(llcgprs_tree, tvb, offset, (llc_data_length-2),
							    ett_llcgprs_ui, NULL, "Information Field: Length = %u", info_len);

				uinfo_tree = proto_tree_add_subtree(ui_tree, tvb, offset, 6,
								  ett_llcgprs_ui, NULL, "Rejected Frame Control Field");

				location = offset;
				for (loop_counter = 0; loop_counter < 3; loop_counter++)
				{
					/* display the rejected frame control field */
					cf_byte = tvb_get_ntohs(tvb, location);
					proto_tree_add_uint(uinfo_tree, hf_llcgprs_frmr_cf, tvb, location,
							    2, cf_byte);

					location += 2;
				}

				uinfo_tree = proto_tree_add_subtree(ui_tree, tvb, location, 4,
								  ett_llcgprs_ui, NULL, "Information Field Data");

				fld_vars = tvb_get_ntohl(tvb, location);
				proto_tree_add_uint(uinfo_tree, hf_llcgprs_frmr_spare, tvb, location,
						    4, fld_vars);
				proto_tree_add_uint(uinfo_tree, hf_llcgprs_frmr_vs, tvb, location,
						    2, fld_vars);
				proto_tree_add_uint(uinfo_tree, hf_llcgprs_frmr_vr, tvb, (location + 1),
						    2, fld_vars);
				proto_tree_add_uint(uinfo_tree, hf_llcgprs_frmr_cr, tvb, (location + 2),
						    1, fld_vars);
				proto_tree_add_uint(uinfo_tree, hf_llcgprs_frmr_w4, tvb, (location + 3),
						    1, fld_vars);
				proto_tree_add_uint(uinfo_tree, hf_llcgprs_frmr_w3, tvb, (location + 3),
						    1, fld_vars);
				proto_tree_add_uint(uinfo_tree, hf_llcgprs_frmr_w2, tvb, (location + 3),
						    1, fld_vars);
				proto_tree_add_uint(uinfo_tree, hf_llcgprs_frmr_w1, tvb, (location + 3),
						    1, fld_vars);
			}
			break;
		default:
			break;
		}
		break;
	}

	return tvb_captured_length(tvb);
}


/* Register the protocol with Wireshark */

void
proto_register_llcgprs(void)
{
/* Setup list of header fields  See Section 1.6.1 for details */
	static hf_register_info hf[] = {
		{ &hf_llcgprs_sapi,
		  { "SAPI", "llcgprs.sapi", FT_UINT8, BASE_DEC|BASE_EXT_STRING,
		    &sapi_abrv_ext, 0x0, "Service Access Point Identifier", HFILL }},

		{ &hf_llcgprs_pd,
		  { "Protocol Discriminator_bit", "llcgprs.pd", FT_BOOLEAN, 8,
		    TFS(&pd_bit), 0x80, "Protocol Discriminator bit (should be 0)", HFILL }},

		{ &hf_llcgprs_fcs,
		  { "FCS", "llcgprs.fcs", FT_UINT24, BASE_HEX,
		    NULL, 0, NULL, HFILL }},

		{ &hf_llcgprs_sjsd,
		  { "Supervisory function bits", "llcgprs.s1s2", FT_UINT16, BASE_HEX,
		    VALS(cr_formats_ipluss), 0x3, NULL, HFILL }},

		{ &hf_llcgprs_cr,
		  { "Command/Response bit", "llcgprs.cr", FT_BOOLEAN, 8,
		    TFS(&cr_bit), 0x40, NULL, HFILL }},

		{ &hf_llcgprs_sapib,
		  { "SAPI", "llcgprs.sapib", FT_UINT8, BASE_DEC|BASE_EXT_STRING ,
		    &sapi_t_ext, 0xf, "Service Access Point Identifier", HFILL }},

		{ &hf_llcgprs_U_fmt,
		  { "UI format", "llcgprs.ui", FT_UINT16, BASE_HEX,
		    NULL, UI_MASK_FMT, "UI frame format", HFILL }},

		{ &hf_llcgprs_Un,
		  { "U format", "llcgprs.u", FT_UINT8, BASE_HEX,
		    NULL, 0xe0, "U frame format", HFILL }},

		{ &hf_llcgprs_sp_bits,
		  { "Spare bits", "llcgprs.ui_sp_bit", FT_UINT16, BASE_HEX,
		    NULL, UI_MASK_SPB, NULL, HFILL }},

		{ &hf_llcgprs_NU,
		  { "N(U)", "llcgprs.nu", FT_UINT16, BASE_DEC,
		    NULL, UI_MASK_NU, "Transmitted unconfirmed sequence number", HFILL }},

		{ &hf_llcgprs_E_bit,
		  { "E bit", "llcgprs.e", FT_BOOLEAN, 16,
		    TFS(&e_bit), UI_MASK_E, "Encryption mode bit", HFILL }},

		{ &hf_llcgprs_PM_bit,
		  { "PM bit", "llcgprs.pm", FT_BOOLEAN, 16,
		    TFS(&pm_bit), UI_MASK_PM, "Protected mode bit", HFILL }},

		{ &hf_llcgprs_As,
		  { "Ackn request bit", "llcgprs.as", FT_BOOLEAN, 16,
		    TFS(&a_bit), 0x2000 , "Acknowledgement request bit A", HFILL }},

		{ &hf_llcgprs_PF,
		  { "P/F bit", "llcgprs.pf", FT_BOOLEAN, 8,
		    NULL, 0x10, "Poll/Final bit", HFILL }},

		{ &hf_llcgprs_ucom,
		  { "Command/Response", "llcgprs.ucom", FT_UINT8, BASE_HEX,
		    VALS(cr_formats_unnumb), 0xf, "Commands and Responses", HFILL }},

		{ &hf_llcgprs_NR,
		  { "Receive sequence number", "llcgprs.nr", FT_UINT16, BASE_DEC,
		    NULL, UI_MASK_NU, "Receive sequence number N(R)", HFILL }},

		{ &hf_llcgprs_S_fmt,
		  { "S format", "llcgprs.s", FT_UINT16, BASE_HEX,
		    NULL, 0xc000, "Supervisory format S", HFILL }},

		{ &hf_llcgprs_kmask,
		  { "ignored", "llcgprs.kmask", FT_UINT8, BASE_DEC,
		    NULL, 0xE0, NULL, HFILL }},

		{ &hf_llcgprs_k,
		  { "k", "llcgprs.k", FT_UINT8, BASE_DEC,
		    NULL, 0x1F, "k counter", HFILL }},

		{ &hf_llcgprs_isack_ns,
		  { "N(S)", "llcgprs.sackns", FT_UINT24, BASE_DEC,
		    NULL, 0x1FF000, NULL, HFILL }},

		{ &hf_llcgprs_isack_nr,
		  { "N(R)", "llcgprs.sacknr", FT_UINT24, BASE_DEC,
		    NULL, 0x0007FC, NULL, HFILL }},

		{ &hf_llcgprs_isack_sfb,
		  { "Supervisory function bits", "llcgprs.sacksfb", FT_UINT24, BASE_HEX,
		    VALS(cr_formats_ipluss), 0x000003, NULL, HFILL }},

		{ &hf_llcgprs_ifmt,
		  { "I Format", "llcgprs.ifmt", FT_UINT24, BASE_HEX,
		    NULL, 0x800000, "I Fmt Bit", HFILL }},

		{ &hf_llcgprs_Ai,
		  { "Ackn request bit", "llcgprs.ai", FT_BOOLEAN, 24,
		    TFS(&a_bit), 0x400000, "Acknowledgement request bit A", HFILL }},

		{ &hf_llcgprs_izerobit,
		  { "Spare", "llcgprs.iignore", FT_UINT24, BASE_DEC,
		    NULL, 0x200000, "Ignore Bit", HFILL }},

		{ &hf_llcgprs_sspare,
		  { "Spare", "llcgprs.sspare", FT_UINT16, BASE_DEC,
		    NULL, 0x1800, "Ignore Bit", HFILL }},

		{ &hf_llcgprs_rbyte,
		  { "R Bitmap Bits", "llcgprs.sackrbits", FT_UINT8, BASE_HEX,
		    NULL, 0xFF, "R Bitmap", HFILL }},

		/* XID Parameter Parsing Info */
		{ &hf_llcgprs_xid_xl,
		  { "XL Bit", "llcgprs.xidxl", FT_UINT8, BASE_HEX,
		    NULL, 0x80, "XL", HFILL }},

		{ &hf_llcgprs_xid_type,
		  { "Type", "llcgprs.xidtype", FT_UINT8, BASE_DEC,
		    NULL, 0x7C, NULL, HFILL }},

		{ &hf_llcgprs_xid_len1,
		  { "Length", "llcgprs.xidlen1", FT_UINT8, BASE_DEC,
		    NULL, 0x03, NULL, HFILL }},

		{ &hf_llcgprs_xid_len2,
		  { "Length continued", "llcgprs.xidlen2", FT_UINT8, BASE_DEC,
		    NULL, 0xFC, NULL, HFILL }},

		{ &hf_llcgprs_xid_spare,
		  { "Spare", "llcgprs.xidspare", FT_UINT8, BASE_HEX,
		    NULL, 0x03, "Ignore", HFILL }},

		{ &hf_llcgprs_xid_byte,
		  { "Parameter Byte", "llcgprs.xidbyte", FT_UINT8, BASE_HEX,
		    NULL, 0xFF, "Data", HFILL }},

		/* FRMR Parsing Information */
		{ &hf_llcgprs_frmr_cf,
		  { "Control Field Octet", "llcgprs.frmrrfcf", FT_UINT16, BASE_DEC,
		    NULL, 0xFFFF, "Rejected Frame CF", HFILL }},

		{ &hf_llcgprs_frmr_spare,
		  { "X", "llcgprs.frmrspare", FT_UINT32, BASE_HEX,
		    NULL, 0xF00400F0, "Filler", HFILL }},

		{ &hf_llcgprs_frmr_vs,
		  { "V(S)", "llcgprs.frmrvs", FT_UINT32, BASE_DEC,
		    NULL, 0x0FF80000, "Current send state variable", HFILL }},

		{ &hf_llcgprs_frmr_vr,
		  { "V(R)", "llcgprs.frmrvr", FT_UINT32, BASE_DEC,
		    NULL, 0x0003FE00, "Current receive state variable", HFILL }},

		{ &hf_llcgprs_frmr_cr,
		  { "C/R", "llcgprs.frmrcr", FT_UINT32, BASE_DEC,
		    NULL, 0x00000100, "Rejected command response", HFILL }},

		{ &hf_llcgprs_frmr_w4,
		  { "W4", "llcgprs.frmrw4", FT_UINT32, BASE_DEC,
		    NULL, 0x00000008, "LLE was in ABM when rejecting", HFILL }},

		{ &hf_llcgprs_frmr_w3,
		  { "W3", "llcgprs.frmrw3", FT_UINT32, BASE_DEC,
		    NULL, 0x00000004, "Undefined control field", HFILL }},

		{ &hf_llcgprs_frmr_w2,
		  { "W2", "llcgprs.frmrw2", FT_UINT32, BASE_DEC,
		    NULL, 0x00000002, "Info exceeded N201", HFILL }},

		{ &hf_llcgprs_frmr_w1,
		  { "W1", "llcgprs.frmrw1", FT_UINT32, BASE_DEC,
		    NULL, 0x00000001, "Invalid - info not permitted", HFILL }},

		{ &hf_llcgprs_tom_rl,
		  { "Remaining Length of TOM Protocol Header", "llcgprs.romrl", FT_UINT8, BASE_DEC,
		    NULL, 0xF0, "RL", HFILL }},

		{ &hf_llcgprs_tom_pd,
		  { "TOM Protocol Discriminator", "llcgprs.tompd", FT_UINT8, BASE_HEX,
		    NULL, 0x0F, "TPD", HFILL }},

		{ &hf_llcgprs_tom_header,
		  { "TOM Header Byte", "llcgprs.tomhead", FT_UINT8, BASE_HEX,
		    NULL, 0xFF, "thb", HFILL }},

		{ &hf_llcgprs_tom_data,
		  { "TOM Message Capsule Byte", "llcgprs.tomdata", FT_UINT8, BASE_HEX,
		    NULL, 0xFF, "tdb", HFILL }},
	};

/* Setup protocol subtree array */
	static gint *ett[] = {
		&ett_llcgprs,
		&ett_llcgprs_adf,
		&ett_llcgprs_ctrlf,
		&ett_llcgprs_ui,
		&ett_llcgprs_sframe,
	};

	static ei_register_info ei[] = {
		{ &ei_llcgprs_no_info_field, { "llcgprs.no_info_field", PI_PROTOCOL, PI_WARN, "No Information Field", EXPFILL }},
	};

	module_t *llcgprs_module;
	expert_module_t* expert_llcgprs;

/* Register the protocol name and description */
	proto_llcgprs = proto_register_protocol("Logical Link Control GPRS",
	    "GPRS-LLC", "llcgprs");
	llcgprs_subdissector_table = register_dissector_table("llcgprs.sapi", "GPRS LLC SAPI", proto_llcgprs, FT_UINT8, BASE_HEX);

/* Required function calls to register the header fields and subtrees used */
	proto_register_field_array(proto_llcgprs, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
	expert_llcgprs = expert_register_protocol(proto_llcgprs);
	expert_register_field_array(expert_llcgprs, ei, array_length(ei));
	register_dissector("llcgprs", dissect_llcgprs, proto_llcgprs);

	llcgprs_module = prefs_register_protocol ( proto_llcgprs, NULL );
	prefs_register_bool_preference ( llcgprs_module, "autodetect_cipher_bit",
	    "Autodetect cipher bit",
	    "Whether to autodetect the cipher bit (because it might be set on unciphered data)",
	    &ignore_cipher_bit );
}


void
proto_reg_handoff_llcgprs(void)
{
	dissector_handle_t gprs_llc_handle;

	/* make sure that the top level can call this dissector */
	gprs_llc_handle = find_dissector("llcgprs");
	dissector_add_uint("wtap_encap", WTAP_ENCAP_GPRS_LLC, gprs_llc_handle);

	sndcp_xid_handle  = find_dissector_add_dependency("sndcpxid", proto_llcgprs);
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
