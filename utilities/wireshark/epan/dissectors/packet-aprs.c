/* packet-aprs.c
 * Routines for Amateur Packet Radio protocol dissection
 * Copyright 2007,2008,2009,2010,2012 R.W. Stearn <richard@rns-stearn.demon.co.uk>
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

/*
 * This dissector is for APRS (Automatic Packet Reporting System)
 *
 * Information was drawn from:
 *    http://www.aprs.org/
 *    Specification: http://www.aprs.org/doc/APRS101.PDF
 *
 * Inspiration on how to build the dissector drawn from
 *   packet-sdlc.c
 *   packet-x25.c
 *   packet-lapb.c
 *   paket-gprs-llc.c
 *   xdlc.c
 * with the base file built from README.developers.
 */

#include "config.h"

#include <math.h>

#include <epan/packet.h>
#include <epan/prefs.h>
#include <epan/to_str.h>

#define STRLEN	100

void proto_register_aprs(void);

static int proto_aprs			= -1;

/* aprs timestamp items */
static int hf_aprs_dhm			= -1;
static int hf_aprs_hms			= -1;
static int hf_aprs_mdhm			= -1;
static int hf_aprs_tz			= -1;

/* aprs position items */
/* static int hf_aprs_position		= -1; */
static int hf_aprs_lat			= -1;
static int hf_aprs_long			= -1;

/* aprs msg items */
static int hf_aprs_msg			= -1;
static int hf_aprs_msg_rng		= -1;
static int hf_aprs_msg_cse		= -1;
static int hf_aprs_msg_spd		= -1;
static int hf_aprs_msg_dir		= -1;
static int hf_aprs_msg_brg		= -1;
static int hf_aprs_msg_nrq		= -1;

/* aprs compression type items */
static int hf_aprs_compression_type	= -1;
static int hf_aprs_ct_gps_fix		= -1;
static int hf_aprs_ct_nmea_src		= -1;
static int hf_aprs_ct_origin		= -1;

/* phg msg items */
static int hf_aprs_msg_phg_p		= -1;
static int hf_aprs_msg_phg_h		= -1;
static int hf_aprs_msg_phg_g		= -1;
static int hf_aprs_msg_phg_d		= -1;

/* dfs msg items */
static int hf_aprs_msg_dfs_s		= -1;
static int hf_aprs_msg_dfs_h		= -1;
static int hf_aprs_msg_dfs_g		= -1;
static int hf_aprs_msg_dfs_d		= -1;

/* weather items */
static int hf_aprs_weather_dir		= -1;
static int hf_aprs_weather_spd		= -1;
static int hf_aprs_weather_peak		= -1;
static int hf_aprs_weather_temp		= -1;
static int hf_aprs_weather_rain_1	= -1;
static int hf_aprs_weather_rain_24	= -1;
static int hf_aprs_weather_rain		= -1;
static int hf_aprs_weather_humidty	= -1;
static int hf_aprs_weather_press	= -1;
static int hf_aprs_weather_luminosity	= -1;
static int hf_aprs_weather_snow		= -1;
static int hf_aprs_weather_raw_rain	= -1;
static int hf_aprs_weather_software	= -1;
static int hf_aprs_weather_unit		= -1;

/* aod msg items */
static int hf_aprs_msg_aod_t		= -1;
static int hf_aprs_msg_aod_c		= -1;

/* mic-e msg items */
static int hf_aprs_mic_e_dst		= -1;
static int hf_aprs_mic_e_long_d		= -1;
static int hf_aprs_mic_e_long_m		= -1;
static int hf_aprs_mic_e_long_h		= -1;
static int hf_aprs_mic_e_spd_sp		= -1;
static int hf_aprs_mic_e_spd_dc		= -1;
static int hf_aprs_mic_e_spd_se		= -1;
static int hf_aprs_mic_e_telemetry	= -1;
static int hf_aprs_mic_e_status		= -1;

/* Storm items */
static int hf_aprs_storm_dir		= -1;
static int hf_aprs_storm_spd		= -1;
static int hf_aprs_storm_type		= -1;
static int hf_aprs_storm_sws		= -1;
static int hf_aprs_storm_pwg		= -1;
static int hf_aprs_storm_cp		= -1;
static int hf_aprs_storm_rhw		= -1;
static int hf_aprs_storm_rtsw		= -1;
static int hf_aprs_storm_rwg		= -1;

/* aprs sundry items */
static int hf_aprs_dti			= -1;
static int hf_aprs_sym_id		= -1;
static int hf_aprs_sym_code		= -1;
static int hf_aprs_comment		= -1;
static int hf_aprs_storm		= -1;

/* aprs main catgories items */
static int hf_ultimeter_2000		= -1;
static int hf_aprs_status		= -1;
static int hf_aprs_object		= -1;
static int hf_aprs_item			= -1;
static int hf_aprs_query		= -1;
static int hf_aprs_telemetry		= -1;
static int hf_aprs_raw			= -1;
static int hf_aprs_station		= -1;
static int hf_aprs_message		= -1;
static int hf_aprs_agrelo		= -1;
static int hf_aprs_maidenhead		= -1;
static int hf_aprs_weather		= -1;
static int hf_aprs_invalid_test		= -1;
static int hf_aprs_user_defined		= -1;
static int hf_aprs_third_party		= -1;
static int hf_aprs_mic_e_0_current	= -1;
static int hf_aprs_mic_e_0_old		= -1;
static int hf_aprs_mic_e_old		= -1;
static int hf_aprs_mic_e_current	= -1;
static int hf_aprs_peet_1		= -1;
static int hf_aprs_peet_2		= -1;
static int hf_aprs_map_feature		= -1;
static int hf_aprs_shelter_data		= -1;
static int hf_aprs_space_weather	= -1;


static gboolean gPREF_APRS_LAX = FALSE;

static gint ett_aprs		= -1;
static gint ett_aprs_msg	= -1;
static gint ett_aprs_ct		= -1;
static gint ett_aprs_weather	= -1;
static gint ett_aprs_storm	= -1;
static gint ett_aprs_mic_e	= -1;


const value_string ctype_vals[] = {
	{ 0,   "Compressed" },
	{ 1,   "TNC BText" },
	{ 2,   "Software (DOS/Mac/Win/+SA)" },
	{ 3,   "[tbd]" },
	{ 4,   "KPC3" },
	{ 5,   "Pico" },
	{ 6,   "Other tracker [tbd]" },
	{ 7,   "Digipeater conversion" },
	{ 0,            NULL }
};

const value_string nmea_vals[] = {
	{ 0,   "other" },
	{ 1,   "GLL" },
	{ 2,   "GGA" },
	{ 3,   "RMC" },
	{ 0,            NULL }
};

const value_string gps_vals[] = {
	{ 0,   "old (last)" },
	{ 1,   "current" },
	{ 0,            NULL }
};

/* sorted */
static const value_string aprs_description[] = {
	{ 0x1c, "Current Mic-E Data (Rev 0 beta)" },
	{ 0x1d, "Old Mic-E Data (Rev 0 beta)" },
	{ '#',  "Peet Bros U-II Weather Station" },
	{ '$',  "Raw GPS data or Ultimeter 2000" },
	{ '%',  "Agrelo DFJr / MicroFinder" },
	{ '&',  "[Reserved - Map Feature]" },
	{ '\'', "Old Mic-E Data (current data for TM-D700)" },
	{ ')',  "Item" },
	{ '*',  "Peet Bros U-II Weather Station" },
	{ '+',  "[Reserved - Shelter data with time]" },
	{ ',',  "Invalid data or test data" },
	{ '.',  "[Reserved - Space weather]" },
	{ '/',  "Position + timestamp" },
	{ ':',  "Message" },
	{ ';',  "Object" },
	{ '<',  "Station Capabilities" },
	{ '=',  "Position + APRS data extension" },
	{ '>',  "Status" },
	{ '?',  "Query" },
	{ '@',  "Position + timestamp + APRS data extension" },
	{ 'T',  "Telemetry data" },
	{ '[',  "Maidenhead grid locator beacon (obsolete)" },
	{ '_',  "Weather Report (without position)" },
	{ '`',  "Current Mic-E Data (not used in TM-D700)" },
	{ '{',  "User-Defined APRS packet format" },
	{ '}',  "Third-party traffic" },
	{ 0,            NULL }
};
static value_string_ext aprs_description_ext = VALUE_STRING_EXT_INIT(aprs_description);

/* MIC-E destination field code table */
typedef struct
	{
	guint8 key;
	char   digit;
	int    msg;
	char   n_s;
	int    long_offset;
	char   w_e;
	} mic_e_dst_code_table_s;

static const mic_e_dst_code_table_s dst_code[] =
	{
	{ '0' << 1, '0', 0, 'S',   0, 'E' },
	{ '1' << 1, '1', 0, 'S',   0, 'E' },
	{ '2' << 1, '2', 0, 'S',   0, 'E' },
	{ '3' << 1, '3', 0, 'S',   0, 'E' },
	{ '4' << 1, '4', 0, 'S',   0, 'E' },
	{ '5' << 1, '5', 0, 'S',   0, 'E' },
	{ '6' << 1, '6', 0, 'S',   0, 'E' },
	{ '7' << 1, '7', 0, 'S',   0, 'E' },
	{ '8' << 1, '8', 0, 'S',   0, 'E' },
	{ '9' << 1, '9', 0, 'S',   0, 'E' },
	{ 'A' << 1, '0', 1, '?',   0, '?' },
	{ 'B' << 1, '1', 1, '?',   0, '?' },
	{ 'C' << 1, '2', 1, '?',   0, '?' },
	{ 'D' << 1, '3', 1, '?',   0, '?' },
	{ 'E' << 1, '4', 1, '?',   0, '?' },
	{ 'F' << 1, '5', 1, '?',   0, '?' },
	{ 'G' << 1, '6', 1, '?',   0, '?' },
	{ 'H' << 1, '7', 1, '?',   0, '?' },
	{ 'I' << 1, '8', 1, '?',   0, '?' },
	{ 'J' << 1, '9', 1, '?',   0, '?' },
	{ 'K' << 1, ' ', 1, '?',   0, '?' },
	{ 'L' << 1, ' ', 0, 'S',   0, 'E' },
	{ 'P' << 1, '0', 1, 'N', 100, 'W' },
	{ 'Q' << 1, '1', 1, 'N', 100, 'W' },
	{ 'R' << 1, '2', 1, 'N', 100, 'W' },
	{ 'S' << 1, '3', 1, 'N', 100, 'W' },
	{ 'T' << 1, '4', 1, 'N', 100, 'W' },
	{ 'U' << 1, '5', 1, 'N', 100, 'W' },
	{ 'V' << 1, '6', 1, 'N', 100, 'W' },
	{ 'W' << 1, '7', 1, 'N', 100, 'W' },
	{ 'X' << 1, '8', 1, 'N', 100, 'W' },
	{ 'Y' << 1, '9', 1, 'N', 100, 'W' },
	{ 'Z' << 1, ' ', 1, 'N', 100, 'W' },
	{        0, '_', 3, '?',   3, '?' },
	};


/* MIC-E message table */
typedef struct
	{
	const char *std;
	const char *custom;
	} mic_e_msg_table_s;

static const mic_e_msg_table_s mic_e_msg_table[] =
	{
	{ "Emergency",  "Emergency" },
	{ "Priority",   "Custom 6" },
	{ "Special",    "Custom 5" },
	{ "Committed",  "Custom 4" },
	{ "Returning",  "Custom 3" },
	{ "In Service", "Custom 2" },
	{ "En Route",   "Custom 1" },
	{ "Off Duty",   "Custom 0" }
	};

/* Code to actually dissect the packets */

static int
dissect_aprs_compression_type(	tvbuff_t	 *tvb,
				int		  offset,
				proto_tree	 *parent_tree
				)
{
	proto_tree *tc;
	proto_tree *compression_tree;
	int	    new_offset;
	int	    data_len;
	guint8	    compression_type;


	data_len = 1;
	new_offset = offset + data_len;

	if ( parent_tree )
		{
		compression_type = tvb_get_guint8( tvb, offset ) - 33;

		tc = proto_tree_add_uint( parent_tree, hf_aprs_compression_type, tvb, offset, data_len,
					  compression_type );
		compression_tree = proto_item_add_subtree( tc, ett_aprs_ct );

		proto_tree_add_item( compression_tree, hf_aprs_ct_gps_fix,  tvb, offset, data_len, ENC_BIG_ENDIAN );
		proto_tree_add_item( compression_tree, hf_aprs_ct_nmea_src, tvb, offset, data_len, ENC_BIG_ENDIAN );
		proto_tree_add_item( compression_tree, hf_aprs_ct_origin,   tvb, offset, data_len, ENC_BIG_ENDIAN );
		}

	return new_offset;
}

static int
dissect_aprs_msg(	tvbuff_t	  *tvb,
			int		   offset,
			proto_tree	  *parent_tree,
			int		   wind,
			int 		   brg_nrq
			)
{
	proto_tree *msg_tree = NULL;
	guint8	    ch;


	if ( parent_tree )
		{
		proto_tree *tc;
		tc = proto_tree_add_item( parent_tree, hf_aprs_msg, tvb, offset, 7, ENC_ASCII|ENC_NA );
		msg_tree = proto_item_add_subtree( tc, ett_aprs_msg );
		}

	ch = tvb_get_guint8( tvb, offset );

	if ( g_ascii_isdigit( ch ) )
		{
		if ( wind )
			proto_tree_add_item( msg_tree, hf_aprs_msg_dir, tvb, offset, 3, ENC_ASCII|ENC_NA );
		else
			proto_tree_add_item( msg_tree, hf_aprs_msg_cse, tvb, offset, 3, ENC_ASCII|ENC_NA );
		offset += 3;
		/* verify the separator */
		offset += 1;
		proto_tree_add_item( msg_tree, hf_aprs_msg_spd, tvb, offset, 3, ENC_ASCII|ENC_NA );
		offset += 3;
		}
	else
		{
		switch ( ch )
			{
			case 'D' :	/* dfs */
				offset += 3;
				proto_tree_add_item( msg_tree, hf_aprs_msg_dfs_s, tvb, offset, 1, ENC_ASCII|ENC_NA );
				offset += 1;
				proto_tree_add_item( msg_tree, hf_aprs_msg_dfs_h, tvb, offset, 1, ENC_ASCII|ENC_NA );
				offset += 1;
				proto_tree_add_item( msg_tree, hf_aprs_msg_dfs_g, tvb, offset, 1, ENC_ASCII|ENC_NA );
				offset += 1;
				proto_tree_add_item( msg_tree, hf_aprs_msg_dfs_d, tvb, offset, 1, ENC_ASCII|ENC_NA );
				break;
			case 'P' :	/* phgd */
				offset += 3;
				proto_tree_add_item( msg_tree, hf_aprs_msg_phg_p, tvb, offset, 1, ENC_ASCII|ENC_NA );
				offset += 1;
				proto_tree_add_item( msg_tree, hf_aprs_msg_phg_h, tvb, offset, 1, ENC_ASCII|ENC_NA );
				offset += 1;
				proto_tree_add_item( msg_tree, hf_aprs_msg_phg_g, tvb, offset, 1, ENC_ASCII|ENC_NA );
				offset += 1;
				proto_tree_add_item( msg_tree, hf_aprs_msg_phg_d, tvb, offset, 1, ENC_ASCII|ENC_NA );
				break;
			case 'R' :	/* rng */
				proto_tree_add_item( msg_tree, hf_aprs_msg_rng, tvb, offset, 7, ENC_ASCII|ENC_NA );
				break;
			case 'T' :	/* aod */
				offset += 1;
				proto_tree_add_item( msg_tree, hf_aprs_msg_aod_t, tvb, offset, 2, ENC_ASCII|ENC_NA );
				offset += 2;
				/* step over the /C */
				offset += 2;
				proto_tree_add_item( msg_tree, hf_aprs_msg_aod_c, tvb, offset, 2, ENC_ASCII|ENC_NA );
				break;
			default  :	/* wtf */
				break;
			}
		}
	if ( brg_nrq )
		{
		proto_tree_add_item( msg_tree, hf_aprs_msg_brg, tvb, offset, 3, ENC_ASCII|ENC_NA );
		offset += 3;
		/* verify the separator */
		offset += 1;
		proto_tree_add_item( msg_tree, hf_aprs_msg_nrq, tvb, offset, 3, ENC_ASCII|ENC_NA );
		offset += 3;
		}

	return offset;
}

static int
dissect_aprs_compressed_msg(	tvbuff_t *tvb,
				int offset,
				proto_tree *parent_tree
				)
{
	proto_tree *tc;
	proto_tree *msg_tree;
	int	    new_offset;
	int	    data_len;
	guint8	    ch;
	guint8	    course;
	double	    speed;
	double	    range;
	gchar	   *info_buffer;


	data_len = 2;
	new_offset = offset + data_len;

	if ( parent_tree )
		{
		tc = proto_tree_add_item( parent_tree, hf_aprs_msg, tvb, offset, data_len, ENC_ASCII|ENC_NA );
		msg_tree = proto_item_add_subtree( tc, ett_aprs_msg );

		ch = tvb_get_guint8( tvb, offset );
		if ( ch != ' ' )
			{
			if ( ch == '{' )
				{ /* Pre-Calculated Radio Range */
				offset += 1;
				ch = tvb_get_guint8( tvb, offset );
				range = exp( log( 1.08 ) * (ch - 33) );
				info_buffer = wmem_strdup_printf( wmem_packet_scope(), "%7.2f", range );
				proto_tree_add_string( msg_tree, hf_aprs_msg_rng, tvb, offset, 1, info_buffer );
				}
			else
				if ( ch >= '!' && ch <= 'z' )
					{ /* Course/Speed */
					course = (ch - 33) * 4;
					info_buffer = wmem_strdup_printf( wmem_packet_scope(), "%d", course );
					proto_tree_add_string( msg_tree, hf_aprs_msg_cse,
							       tvb, offset, 1, info_buffer );
					offset += 1;
					ch = tvb_get_guint8( tvb, offset );
					speed = exp( log( 1.08 ) * (ch - 33) );
					info_buffer = wmem_strdup_printf( wmem_packet_scope(), "%7.2f", speed );
					proto_tree_add_string( msg_tree, hf_aprs_msg_spd,
							       tvb, offset, 1, info_buffer );
					}

			}
		}

	return new_offset;
}


static const mic_e_dst_code_table_s *
dst_code_lookup( guint8 ch )
{
	guint indx;

	indx = 0;
	while ( indx < ( sizeof( dst_code ) / sizeof( mic_e_dst_code_table_s ) )
			&& dst_code[ indx ].key != ch
			&& dst_code[ indx ].key > 0 )
		indx++;
	return &( dst_code[ indx ] );
}

static int
d28_to_deg( guint8 code, int long_offset )
{
	int value;

	value = code - 28 + long_offset;
	if ( value >= 180 && value <= 189 )
		value -= 80;
	else
		if ( value >= 190 && value <= 199 )
			value -= 190;
	return value;
}

static int
d28_to_min( guint8 code )
{
	int value;

	value = code - 28;
	if ( value >= 60 )
		value -= 60;
	return value;
}

static int
dissect_mic_e(	tvbuff_t    *tvb,
		int	     offset,
		packet_info *pinfo,
		proto_tree  *parent_tree,
		int	     hf_mic_e_idx
		)
{
	proto_tree *tc;
	proto_tree *mic_e_tree;
	int	    new_offset;
	int	    data_len;
	char    *info_buffer;
	char    latitude[7] = { '?', '?', '?', '?', '.', '?', '?' };
	int	    msg_a;
	int	    msg_b;
	int	    msg_c;
	char    n_s;
	int	    long_offset;
	char    w_e;
	int	    cse;
	int	    spd;
	guint8  ssid;
	const guint8 *addr;
	const mic_e_dst_code_table_s *dst_code_entry;

	data_len    = tvb_reported_length_remaining( tvb, offset );
	new_offset  = offset + data_len;

	info_buffer = (char *)wmem_alloc( wmem_packet_scope(), STRLEN );

	msg_a = 0;
	msg_b = 0;
	msg_c = 0;

	n_s = '?';
	long_offset = 0;
	w_e = '?';
	ssid = 0;

	if ( pinfo->dst.type == AT_AX25 && pinfo->dst.len == AX25_ADDR_LEN )
		{
		/* decode the AX.25 destination address */
		addr = (const guint8 *)pinfo->dst.data;

		dst_code_entry = dst_code_lookup( addr[ 0 ] );
		latitude[ 0 ] = dst_code_entry->digit;
		msg_a = dst_code_entry->msg & 0x1;

		dst_code_entry = dst_code_lookup( addr[ 1 ] );
		latitude[ 1 ] = dst_code_entry->digit;
		msg_b = dst_code_entry->msg & 0x1;

		dst_code_entry = dst_code_lookup( addr[ 2 ] );
		latitude[ 2 ] = dst_code_entry->digit;
		msg_c = dst_code_entry->msg & 0x1;

		dst_code_entry = dst_code_lookup( addr[ 3 ] );
		latitude[ 3 ] = dst_code_entry->digit;
		n_s = dst_code_entry->n_s;

		latitude[ 4 ] = '.';

		dst_code_entry = dst_code_lookup( addr[ 4 ] );
		latitude[ 5 ] = dst_code_entry->digit;
		long_offset = dst_code_entry->long_offset;

		dst_code_entry = dst_code_lookup( addr[ 5 ] );
		latitude[ 6 ] = dst_code_entry->digit;
		w_e = dst_code_entry->w_e;

		ssid = (addr[ 6 ] >> 1) & 0x0f;
		}

	/* decode the mic-e info fields */
	spd = ((tvb_get_guint8( tvb, offset + 3 ) - 28) * 10) + ((tvb_get_guint8( tvb, offset + 4 ) - 28) / 10);
	if ( spd >= 800 )
		spd -= 800;

	cse = (((tvb_get_guint8( tvb, offset + 4 ) - 28) % 10) * 100) + ((tvb_get_guint8( tvb, offset + 5 ) - 28) * 10);
	if ( cse >= 400 )
		cse -= 400;

	g_snprintf( info_buffer, STRLEN,
				"Lat: %7.7s%c Long: %03d%02d.%02d%c, Cse: %d, Spd: %d, SSID: %d, Msg %s",
				latitude,
				n_s,
				d28_to_deg( tvb_get_guint8( tvb, offset ), long_offset ),
				d28_to_min( tvb_get_guint8( tvb, offset + 1 ) ),
				tvb_get_guint8( tvb, offset + 2 ) - 28,
				w_e,
				cse,
				spd,
				ssid,
				mic_e_msg_table[ (msg_a << 2) + (msg_b << 1) + msg_c ].std
				);

	col_set_str( pinfo->cinfo, COL_INFO, "MIC-E " );
	col_append_str( pinfo->cinfo, COL_INFO, info_buffer );

	if ( parent_tree )
		{
		tc = proto_tree_add_string( parent_tree, hf_mic_e_idx, tvb, offset, data_len, info_buffer );
		mic_e_tree = proto_item_add_subtree( tc, ett_aprs_mic_e );

		g_snprintf( info_buffer, STRLEN,
				"Lat %7.7s, Msg A %d, Msg B %d, Msg C %d, N/S %c, Long off %3d, W/E %c, SSID %d",
				latitude,
				msg_a,
				msg_b,
				msg_c,
				n_s,
				long_offset,
				w_e,
				ssid
				);

		proto_tree_add_string( mic_e_tree, hf_aprs_mic_e_dst,     tvb,      0, 0, info_buffer ); /* ?? */

		proto_tree_add_item( mic_e_tree, hf_aprs_mic_e_long_d,    tvb, offset, 1, ENC_BIG_ENDIAN );
		offset += 1;

		proto_tree_add_item( mic_e_tree, hf_aprs_mic_e_long_m,    tvb, offset, 1, ENC_BIG_ENDIAN );
		offset += 1;

		proto_tree_add_item( mic_e_tree, hf_aprs_mic_e_long_h,    tvb, offset, 1, ENC_BIG_ENDIAN );
		offset += 1;

		proto_tree_add_item( mic_e_tree, hf_aprs_mic_e_spd_sp,    tvb, offset, 1, ENC_BIG_ENDIAN );
		offset += 1;

		proto_tree_add_item( mic_e_tree, hf_aprs_mic_e_spd_dc,    tvb, offset, 1, ENC_BIG_ENDIAN );
		offset += 1;

		proto_tree_add_item( mic_e_tree, hf_aprs_mic_e_spd_se,    tvb, offset, 1, ENC_BIG_ENDIAN );
		offset += 1;

		proto_tree_add_item( mic_e_tree, hf_aprs_sym_code,  tvb, offset, 1, ENC_ASCII|ENC_NA );
		offset += 1;

		proto_tree_add_item( mic_e_tree, hf_aprs_sym_id,    tvb, offset, 1, ENC_ASCII|ENC_NA );
		offset += 1;

		if ( offset < new_offset )
			{
			guint8 c = tvb_get_guint8(tvb, offset);
			if ( (c == ',') || (c == 0x1d) )
				proto_tree_add_item( mic_e_tree, hf_aprs_mic_e_telemetry,
						     tvb, offset, -1, ENC_NA );
			else
				proto_tree_add_item( mic_e_tree, hf_aprs_mic_e_status,
						     tvb, offset, -1, ENC_ASCII|ENC_NA );
			}

		}

	return new_offset;
}

static int
dissect_aprs_storm(	tvbuff_t   *tvb,
			int	    offset,
			proto_tree *parent_tree
			)
{
	proto_tree  *storm_tree;
	proto_tree *tc;

	tc = proto_tree_add_item( parent_tree, hf_aprs_storm, tvb, offset, -1, ENC_ASCII|ENC_NA );
	storm_tree = proto_item_add_subtree( tc, ett_aprs_storm );

	proto_tree_add_item( storm_tree, hf_aprs_storm_dir,  tvb, offset, 3, ENC_ASCII|ENC_NA );
	offset += 3;
	offset += 1;
	proto_tree_add_item( storm_tree, hf_aprs_storm_spd,  tvb, offset, 3, ENC_ASCII|ENC_NA );
	offset += 3;
	proto_tree_add_item( storm_tree, hf_aprs_storm_type, tvb, offset, 3, ENC_ASCII|ENC_NA );
	offset += 3;
	proto_tree_add_item( storm_tree, hf_aprs_storm_sws,  tvb, offset, 4, ENC_ASCII|ENC_NA );
	offset += 4;
	proto_tree_add_item( storm_tree, hf_aprs_storm_pwg,  tvb, offset, 4, ENC_ASCII|ENC_NA );
	offset += 4;
	proto_tree_add_item( storm_tree, hf_aprs_storm_cp,   tvb, offset, 5, ENC_ASCII|ENC_NA );
	offset += 5;
	proto_tree_add_item( storm_tree, hf_aprs_storm_rhw,  tvb, offset, 4, ENC_ASCII|ENC_NA );
	offset += 4;
	proto_tree_add_item( storm_tree, hf_aprs_storm_rtsw, tvb, offset, 4, ENC_ASCII|ENC_NA );
	offset += 4;
	proto_tree_add_item( storm_tree, hf_aprs_storm_rwg,  tvb, offset, 4, ENC_ASCII|ENC_NA );
	offset += 4;

	return offset;
}

static int
dissect_aprs_weather(	tvbuff_t   *tvb,
			int	    offset,
			proto_tree *parent_tree
			)
{
	proto_tree  *tc;
	proto_tree  *weather_tree;
	int	     new_offset;
	int	     data_len;
	guint8	     ch;


	data_len    = tvb_reported_length_remaining( tvb, offset );
	new_offset  = offset + data_len;

	tc = proto_tree_add_item( parent_tree, hf_aprs_weather, tvb, offset, data_len, ENC_ASCII|ENC_NA );
	weather_tree = proto_item_add_subtree( tc, ett_aprs_weather );

	ch = tvb_get_guint8( tvb, offset );
	if ( g_ascii_isdigit( ch ) )
		{
		proto_tree_add_item( weather_tree, hf_aprs_weather_dir, tvb, offset, 3, ENC_ASCII|ENC_NA );
		offset += 3;
		/* verify the separator */
		offset += 1;
		proto_tree_add_item( weather_tree, hf_aprs_weather_spd, tvb, offset, 3, ENC_ASCII|ENC_NA );
		offset += 3;
		}

	if ( parent_tree )
		{
		while ( offset < new_offset )
			{
			ch = tvb_get_guint8( tvb, offset );
			switch ( ch )
				{
				case 'c' :
					proto_tree_add_item( weather_tree, hf_aprs_weather_dir,
						tvb, offset, 4, ENC_ASCII|ENC_NA );
					offset += 4;
					break;
				case 's' :
					proto_tree_add_item( weather_tree, hf_aprs_weather_spd,
						tvb, offset, 4, ENC_ASCII|ENC_NA );
					offset += 4;
					break;
				case 'g' :
					proto_tree_add_item( weather_tree, hf_aprs_weather_peak,
						tvb, offset, 4, ENC_ASCII|ENC_NA );
					offset += 4;
					break;
				case 't' :
					proto_tree_add_item( weather_tree, hf_aprs_weather_temp,
						tvb, offset, 4, ENC_ASCII|ENC_NA );
					offset += 4;
					break;
				case 'r' :
					proto_tree_add_item( weather_tree, hf_aprs_weather_rain_1,
						tvb, offset, 4, ENC_ASCII|ENC_NA );
					offset += 4;
					break;
				case 'P' :
					proto_tree_add_item( weather_tree, hf_aprs_weather_rain_24,
						tvb, offset, 4, ENC_ASCII|ENC_NA );
					offset += 4;
					break;
				case 'p' :
					proto_tree_add_item( weather_tree, hf_aprs_weather_rain,
						tvb, offset, 4, ENC_ASCII|ENC_NA );
					offset += 4;
					break;
				case 'h' :
					proto_tree_add_item( weather_tree, hf_aprs_weather_humidty,
						tvb, offset, 3, ENC_ASCII|ENC_NA );
					offset += 3;
					break;
				case 'b' :
					proto_tree_add_item( weather_tree, hf_aprs_weather_press,
						tvb, offset, 6, ENC_ASCII|ENC_NA );
					offset += 6;
					break;
				case 'l' :
				case 'L' :
					proto_tree_add_item( weather_tree, hf_aprs_weather_luminosity,
						tvb, offset, 4, ENC_ASCII|ENC_NA );
					offset += 4;
					break;
				case 'S' :
					proto_tree_add_item( weather_tree, hf_aprs_weather_snow,
						tvb, offset, 4, ENC_ASCII|ENC_NA );
					offset += 4;
					break;
				case '#' :
					proto_tree_add_item( weather_tree, hf_aprs_weather_raw_rain,
						tvb, offset, 4, ENC_ASCII|ENC_NA );
					offset += 4;
					break;
				default  : {
					gint lr;
					/* optional: software type/unit: see if present */
					lr = new_offset - offset;
#if 0 /* fcn'al change: defer */
					/*
					 * XXX - ASCII or UTF-8?
					 * See http://www.aprs.org/aprs12/utf-8.txt
					 */
					if ( ((lr < 3) || (lr > 5)) ||
						( lr != strspn( tvb_get_string_enc( wmem_packet_scope(), tvb, offset, lr, ENC_ASCII|ENC_NA ), "a-zA-Z0-9-_" ) ) )
						{
						new_offset = offset;  /* Assume rest is a comment: force exit from while */
						break;  /* from switch */
						}
#endif
					proto_tree_add_item( weather_tree, hf_aprs_weather_software,
						tvb, offset, 1, ENC_ASCII|ENC_NA );
					offset += 1;
					proto_tree_add_item( weather_tree, hf_aprs_weather_unit,
						tvb, offset, lr-1, ENC_ASCII|ENC_NA );
					offset = new_offset;
					break;
					}
				} /* switch */
			} /* while */
		} /* if (parent_tree) */
	return new_offset;
}

static int
aprs_timestamp( proto_tree *aprs_tree, tvbuff_t *tvb, int offset )
{
	int	data_len;
	const char *tzone;
	guint8  ch;

	data_len = 8;
	tzone = "zulu";

	ch= tvb_get_guint8( tvb, offset + 6 );
	if ( g_ascii_isdigit( ch ) )
		{ /* MDHM */
		proto_tree_add_item( aprs_tree, hf_aprs_mdhm, tvb, offset, data_len, ENC_ASCII|ENC_NA );
		proto_tree_add_string( aprs_tree, hf_aprs_tz, tvb, offset, data_len, tzone );
		}
	else
		{
		data_len -= 1;
		if ( ch  == 'h' )
			{ /* HMS */
			proto_tree_add_item( aprs_tree, hf_aprs_hms,  tvb, offset, data_len, ENC_ASCII|ENC_NA );
			proto_tree_add_string( aprs_tree, hf_aprs_tz, tvb, offset, data_len, tzone );
			}
		else
			{ /* DHM */
			switch ( ch )
				{
				case 'z' : tzone = "zulu";    break;
				case '/' : tzone = "local";   break;
				default  : tzone = "unknown"; break;
				}
			proto_tree_add_item( aprs_tree, hf_aprs_dhm,  tvb, offset, data_len, ENC_ASCII|ENC_NA );
			proto_tree_add_string( aprs_tree, hf_aprs_tz, tvb, offset + 6, 1, tzone );
			}
		}

	return offset + data_len;
}

static int
aprs_latitude_compressed( proto_tree *aprs_tree, tvbuff_t *tvb, int offset )
{
	if ( aprs_tree )
		{
		char *info_buffer;
		int   temp;

		info_buffer = (char *)wmem_alloc( wmem_packet_scope(), STRLEN );

		temp = ( tvb_get_guint8( tvb, offset + 0 ) - 33 );
		temp = ( tvb_get_guint8( tvb, offset + 1 ) - 33 ) + ( temp * 91 );
		temp = ( tvb_get_guint8( tvb, offset + 2 ) - 33 ) + ( temp * 91 );
		temp = ( tvb_get_guint8( tvb, offset + 3 ) - 33 ) + ( temp * 91 );

		g_snprintf( info_buffer, STRLEN, "%6.2f", 90.0 - (temp / 380926.0) );
		proto_tree_add_string( aprs_tree, hf_aprs_lat, tvb, offset, 4, info_buffer );
		}
	return offset + 4;
}

static int
aprs_longitude_compressed( proto_tree *aprs_tree, tvbuff_t *tvb, int offset )
{
	if ( aprs_tree )
		{
		char *info_buffer;
		int   temp;

		info_buffer = (char *)wmem_alloc( wmem_packet_scope(), STRLEN );

		temp = ( tvb_get_guint8( tvb, offset + 0 ) - 33 );
		temp = ( tvb_get_guint8( tvb, offset + 1 ) - 33 ) + ( temp * 91 );
		temp = ( tvb_get_guint8( tvb, offset + 2 ) - 33 ) + ( temp * 91 );
		temp = ( tvb_get_guint8( tvb, offset + 3 ) - 33 ) + ( temp * 91 );

		g_snprintf( info_buffer, STRLEN, "%7.2f", (temp / 190463.0) - 180.0 );
		proto_tree_add_string( aprs_tree, hf_aprs_long, tvb, offset, 4, info_buffer );
		}
	return offset + 4;
}

static int
aprs_status( proto_tree *aprs_tree, tvbuff_t *tvb, int offset )
{
	int data_len;

	data_len = tvb_reported_length_remaining( tvb, offset );

	if ( ( data_len > 7 ) && ( tvb_get_guint8( tvb, offset+6 ) == 'z' ) )
		{
		proto_tree_add_item( aprs_tree, hf_aprs_dhm, tvb, offset, 6, ENC_ASCII|ENC_NA );
		offset	 += 6;
		data_len -= 6;
		proto_tree_add_string( aprs_tree, hf_aprs_tz, tvb, offset, 1, "zulu" );
		offset	 += 1;
		data_len -= 1;
		}
	proto_tree_add_item( aprs_tree, hf_aprs_status, tvb, offset, data_len, ENC_ASCII|ENC_NA );

	return offset + data_len;
}

static int
aprs_item( proto_tree *aprs_tree, tvbuff_t *tvb, int offset )
{
	char *info_buffer;
	int   data_len;
	char *ch_ptr;

	data_len    = 10;

	/*
	 * XXX - ASCII or UTF-8?
	 * See http://www.aprs.org/aprs12/utf-8.txt
	 */
	info_buffer = tvb_get_string_enc( wmem_packet_scope(), tvb, offset, data_len, ENC_ASCII|ENC_NA );

	ch_ptr = strchr( info_buffer, '!' );
	if ( ch_ptr != NULL )
		{
		data_len = (int)(ch_ptr - info_buffer + 1);
		*ch_ptr = '\0';
		}
	else
		{
		ch_ptr = strchr( info_buffer, '!' );
		if ( ch_ptr != NULL )
			{
			data_len = (int)(ch_ptr - info_buffer + 1);
			*ch_ptr = '\0';
			}
		}
	proto_tree_add_string( aprs_tree, hf_aprs_item, tvb, offset, data_len, info_buffer );

	return offset + data_len;
}

static int
aprs_3rd_party( proto_tree *aprs_tree, tvbuff_t *tvb, int offset, int data_len )
{
	/* If the type of the hf[] entry pointed to by hfindex is FT_BYTES or FT_STRING */
	/*  then  data_len == -1 is allowed and means "remainder of the tvbuff"         */
	if ( data_len == -1 )
		{
		data_len = tvb_reported_length_remaining( tvb, offset );
#if 0 /* fcn'al change: defer */
		if ( data_len <= 0 )
			return offset;  /* there's no data */
#endif
		}
	proto_tree_add_item( aprs_tree, hf_aprs_third_party, tvb, offset, data_len, ENC_NA );
	/* tnc-2 */
	/* aea */
	return offset + data_len;
}

static int
aprs_default_string( proto_tree *aprs_tree, tvbuff_t *tvb, int offset, int data_len, int hfindex )
{
	/* Assumption: hfindex points to an hf[] entry with type FT_STRING; should be validated ? */
	/* If the type of the hf[] entry pointed to by hfindex is FT_STRING      */
	/*  then  data_len == -1 is allowed and means "remainder of the tvbuff"  */
	if ( data_len == -1 )
		{
		data_len = tvb_reported_length_remaining( tvb, offset );
#if 0 /* fcn'al change: defer */
		if ( data_len <= 0 )
			return offset;  /* there's no data */
#endif
		}
	proto_tree_add_item( aprs_tree, hfindex, tvb, offset, data_len, ENC_ASCII|ENC_NA );
	return offset + data_len;
}

static int
aprs_default_bytes( proto_tree *aprs_tree, tvbuff_t *tvb, int offset, int data_len, int hfindex )
{
	/* Assumption: hfindex points to an hf[] entry with type FT_BYTES; should be validated ? */
	/* If the type of the hf[] entry pointed to by hfindex is FT_BYTES      */
	/*  then  data_len == -1 is allowed and means "remainder of the tvbuff" */
	if ( data_len == -1 )
		{
		data_len = tvb_reported_length_remaining( tvb, offset );
#if 0 /* fcn'al change: defer */
		if ( data_len <= 0 )
			return offset;  /* there's no data */
#endif
		}
	proto_tree_add_item( aprs_tree, hfindex, tvb, offset, data_len, ENC_NA );
	return offset + data_len;
}

static int
aprs_position( proto_tree *aprs_tree, tvbuff_t *tvb, int offset, gboolean with_msg )
{
	guint8	 symbol_table_id    = 0;
	guint8	 symbol_code	    = 0;
	gboolean probably_a_msg	    = FALSE;
	gboolean probably_not_a_msg = FALSE;

	if ( g_ascii_isdigit( tvb_get_guint8( tvb, offset ) ) )
		{
		offset		= aprs_default_string( aprs_tree, tvb, offset, 8, hf_aprs_lat );
		symbol_table_id = tvb_get_guint8( tvb, offset );
		offset		= aprs_default_string( aprs_tree, tvb, offset, 1, hf_aprs_sym_id );
		offset		= aprs_default_string( aprs_tree, tvb, offset, 9, hf_aprs_long );
		symbol_code	= tvb_get_guint8( tvb, offset );
		offset		= aprs_default_string( aprs_tree, tvb, offset, 1, hf_aprs_sym_code );
		if ( gPREF_APRS_LAX )
			{
			switch ( tvb_get_guint8( tvb, offset ) )
				{
				case 'D'	: probably_a_msg = TRUE;     break;
				case 'P'	: probably_a_msg = TRUE;     break;
				case 'R'	: probably_a_msg = TRUE;     break;
				case 'T'	: probably_a_msg = TRUE;     break;
				default		: probably_not_a_msg = TRUE; break;
				}
			}
		if ( with_msg || probably_a_msg || ! probably_not_a_msg )
			offset = dissect_aprs_msg(	tvb,
							offset,
							aprs_tree,
							( symbol_code == '_' ),
							( symbol_table_id == '/' && symbol_code == '\\' )
							);
		}
	else
		{
		symbol_table_id = tvb_get_guint8( tvb, offset );
		offset = aprs_default_string( aprs_tree, tvb, offset, 1, hf_aprs_sym_id );
		offset = aprs_latitude_compressed( aprs_tree, tvb, offset );
		offset = aprs_longitude_compressed( aprs_tree, tvb, offset );
		symbol_code = tvb_get_guint8( tvb, offset );
		offset = aprs_default_string( aprs_tree, tvb, offset, 1, hf_aprs_sym_code );
		offset = dissect_aprs_compressed_msg(	tvb,
							offset,
							aprs_tree
							);
		offset = dissect_aprs_compression_type(	tvb,
							offset,
							aprs_tree
							);
		if ( symbol_table_id == '/' && symbol_code == '\\' )
			offset = aprs_default_string( aprs_tree, tvb, offset, 8, hf_aprs_msg_brg );
		}

	if ( symbol_code == '_' )
		offset = dissect_aprs_weather(	tvb,
						offset,
						aprs_tree
						);
	if ( ( symbol_table_id == '/' && symbol_code == '@' ) || ( symbol_table_id == '\\' && symbol_code == '@' ) )
		offset = dissect_aprs_storm(	tvb,
						offset,
						aprs_tree
						);

	return offset;
}

static int
dissect_aprs( tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree, void *data _U_ )
{
	proto_item    *ti;
	proto_tree    *aprs_tree;

	int	       offset;
	guint8	       dti;
	wmem_strbuf_t *sb;

	col_set_str( pinfo->cinfo, COL_PROTOCOL, "APRS" );
	col_clear( pinfo->cinfo, COL_INFO );

	offset	 = 0;

	dti	 = tvb_get_guint8( tvb, offset );

	sb = wmem_strbuf_new_label(wmem_packet_scope());

	if (dti != '!')
		wmem_strbuf_append(sb, val_to_str_ext_const(dti, &aprs_description_ext, ""));

	switch ( dti )
		{
		case '!':
			/* Position or Ultimeter 2000 WX Station */
			if ( tvb_get_guint8( tvb, offset + 1 ) == '!' )
				{
				wmem_strbuf_append(sb, "Ultimeter 2000 WX Station");
				}
			else
				{
				/* Position "without APRS messaging" */
				wmem_strbuf_append(sb, "Position (");
				wmem_strbuf_append(sb, tvb_format_text(tvb, offset + 1, 8));	/* Lat */
				wmem_strbuf_append(sb, " ");
				wmem_strbuf_append(sb, tvb_format_text(tvb, offset + 1 + 8 + 1, 9));	/* Long */
				wmem_strbuf_append(sb, " ");
				wmem_strbuf_append(sb, tvb_format_text(tvb, offset + 1 + 8, 1));	/* Symbol table id */
				wmem_strbuf_append(sb, tvb_format_text(tvb, offset + 1 + 8 + 1 + 9, 1));	/* Symbol Code */
				}
			break;

		case '=':
			/* Position "with APRS messaging" + Ext APRS message */
			wmem_strbuf_append(sb, " (");
			wmem_strbuf_append(sb, tvb_format_text(tvb, offset + 1, 8));	/* Lat */
			wmem_strbuf_append(sb, " ");
			wmem_strbuf_append(sb, tvb_format_text(tvb, offset + 1 + 8 + 1, 9));	/* Long */
			wmem_strbuf_append(sb, " ");
			wmem_strbuf_append(sb, tvb_format_text(tvb, offset + 1 + 8, 1));	/* Symbol table id */
			wmem_strbuf_append(sb, tvb_format_text(tvb, offset + 1 + 8 + 1 + 9, 1));	/* Symbol Code */
			break;

		case '/':
			/* Position + timestamp "without APRS messaging" */
			wmem_strbuf_append(sb, " (");
			wmem_strbuf_append(sb, tvb_format_text(tvb, offset + 1, 7));	/* Timestamp */
			wmem_strbuf_append(sb, " ");
			wmem_strbuf_append(sb, tvb_format_text(tvb, offset + 1 + 7 + 1, 8));    /*??*/	/* Lat */
			wmem_strbuf_append(sb, " ");
			wmem_strbuf_append(sb, tvb_format_text(tvb, offset + 1 + 7 + 8 + 1, 9));	/* Long */
			wmem_strbuf_append(sb, " ");
			wmem_strbuf_append(sb, tvb_format_text(tvb, offset + 1 + 7, 1));	/* Symbol table id */
			wmem_strbuf_append(sb, tvb_format_text(tvb, offset + 1 + 7 + 1 + 9, 1));	/* Symbol Code */
			break;

		case '@':
			/* Position + timestamp "with APRS messaging" + Ext APRS message */
			wmem_strbuf_append(sb, " (");
			wmem_strbuf_append(sb, tvb_format_text(tvb, offset + 1, 7));	/* Timestamp */
			wmem_strbuf_append(sb, " ");
			wmem_strbuf_append(sb, tvb_format_text(tvb, offset + 1 + 7 + 1, 8));    /*??*/	/* Lat */
			wmem_strbuf_append(sb, " ");
			wmem_strbuf_append(sb, tvb_format_text(tvb, offset + 1 + 7 + 8 + 1, 9));	/* Long */
			wmem_strbuf_append(sb, " ");
			wmem_strbuf_append(sb, tvb_format_text(tvb, offset + 1 + 7, 1));	/* Symbol table id */
			wmem_strbuf_append(sb, tvb_format_text(tvb, offset + 1 + 7 + 1 + 9, 1));	/* Symbol Code */
			break;
		}

	col_add_str( pinfo->cinfo, COL_INFO, wmem_strbuf_get_str(sb) );

	/* create display subtree for the protocol */
	ti = proto_tree_add_protocol_format( parent_tree , proto_aprs, tvb, 0, -1, "%s", wmem_strbuf_get_str(sb) );
	aprs_tree = proto_item_add_subtree( ti, ett_aprs );

	proto_tree_add_item( aprs_tree, hf_aprs_dti, tvb, offset, 1, ENC_ASCII|ENC_NA );
	offset += 1;

	switch ( dti )
		{
		case '<'	: /* Station Capabilities */
			offset = aprs_default_string( aprs_tree, tvb, offset, -1, hf_aprs_station );
			break;
		case '>'	: /* Status */
			offset = aprs_status( aprs_tree, tvb, offset );
			break;
		case '?'	: /* Query */
			offset = aprs_default_string( aprs_tree, tvb, offset, -1, hf_aprs_query );
			break;
		case '$'	: /* Raw GPS data or Ultimeter 2000 */
			if ( tvb_get_guint8( tvb, offset ) == 'U' )
				offset = aprs_default_string( aprs_tree, tvb, offset, -1, hf_ultimeter_2000 );
			else
				offset = aprs_default_string( aprs_tree, tvb, offset, -1, hf_aprs_raw );
			break;
		case '%'	: /* Agrelo DFJr / MicroFinder */
			offset = aprs_default_bytes( aprs_tree, tvb, offset, -1, hf_aprs_agrelo );
			break;
		case 'T'	: /* Telemetry data */
			offset = aprs_default_string( aprs_tree, tvb, offset, -1, hf_aprs_telemetry );
			break;
		case '['	: /* Maidenhead grid locator beacon (obsolete) */
			offset = aprs_default_bytes( aprs_tree, tvb, offset, -1, hf_aprs_maidenhead );
			break;
		case '_'	: /* Weather Report (without position) */
			offset = aprs_timestamp( aprs_tree, tvb, offset );
			offset = dissect_aprs_weather( tvb,
				offset,
				aprs_tree
				);
			break;
		case ','	: /* Invalid data or test data */
			offset = aprs_default_bytes( aprs_tree, tvb, offset, -1, hf_aprs_invalid_test );
			break;
		case '{'	: /* User-Defined APRS packet format */
			offset = aprs_default_bytes( aprs_tree, tvb, offset, -1, hf_aprs_user_defined );
			break;
		case '}'	: /* Third-party traffic */
			offset = aprs_3rd_party( aprs_tree, tvb, offset, -1 );
			break;
		case ':'	: /* Message */
			offset = aprs_default_string( aprs_tree, tvb, offset, -1, hf_aprs_message );
			break;
		case 0x1c	: /* Current Mic-E Data (Rev 0 beta) */
			offset = dissect_mic_e(	tvb,
						offset,
						pinfo,
						aprs_tree,
						hf_aprs_mic_e_0_current
						);
			break;
		case 0x1d	: /* Old Mic-E Data (Rev 0 beta) */
			offset = dissect_mic_e(	tvb,
						offset,
						pinfo,
						aprs_tree,
						hf_aprs_mic_e_0_old
						);
			break;
		case '\''	: /* Old Mic-E Data (but Current data for TM-D700) */
			offset = dissect_mic_e(	tvb,
						offset,
						pinfo,
						aprs_tree,
						hf_aprs_mic_e_old
						);
			break;
		case '`'	: /* Current Mic-E Data (not used in TM-D700) */
			offset = dissect_mic_e(	tvb,
						offset,
						pinfo,
						aprs_tree,
						hf_aprs_mic_e_current
						);
			break;
		case '#'	: /* Peet Bros U-II Weather Station */
			offset = aprs_default_bytes( aprs_tree, tvb, offset, -1, hf_aprs_peet_1 );
			break;
		case '*'	: /* Peet Bros U-II Weather Station */
			offset = aprs_default_bytes( aprs_tree, tvb, offset, -1, hf_aprs_peet_2 );
			break;
		case '&'	: /* [Reserved - Map Feature] */
			offset = aprs_default_bytes( aprs_tree, tvb, offset, -1, hf_aprs_map_feature );
			break;
		case '+'	: /* [Reserved - Shelter data with time] */
			offset = aprs_default_bytes( aprs_tree, tvb, offset, -1, hf_aprs_shelter_data );
			break;
		case '.'	: /* [Reserved - Space weather] */
			offset = aprs_default_bytes( aprs_tree, tvb, offset, -1, hf_aprs_space_weather );
			break;
		case ')'	: /* Item */
			offset = aprs_item( aprs_tree, tvb, offset );
			offset = aprs_position( aprs_tree, tvb, offset, TRUE );
			offset = aprs_default_string( aprs_tree, tvb, offset, -1, hf_aprs_comment );
			break;
		case ';'	: /* Object */
			offset = aprs_default_string( aprs_tree, tvb, offset, 10, hf_aprs_object );
			offset = aprs_timestamp( aprs_tree, tvb, offset );
			offset = aprs_position( aprs_tree, tvb, offset, TRUE );
			offset = aprs_default_string( aprs_tree, tvb, offset, -1, hf_aprs_comment );
			break;
		case '!'	: /* Position or Ultimeter 2000 WX Station */
			if ( tvb_get_guint8( tvb, offset ) == '!' )
				offset = aprs_default_string( aprs_tree, tvb, offset, -1, hf_ultimeter_2000 );
			else
				{
				offset = aprs_position( aprs_tree, tvb, offset, FALSE );
				offset = aprs_default_string( aprs_tree, tvb, offset, -1, hf_aprs_comment );
				}
			break;
		case '='	: /* Position + Ext APRS message */
			offset = aprs_position( aprs_tree, tvb, offset, TRUE );
			offset = aprs_default_string( aprs_tree, tvb, offset, -1, hf_aprs_comment );
			break;
		case '/'	: /* Position + timestamp */
			offset = aprs_timestamp( aprs_tree, tvb, offset );
			offset = aprs_position( aprs_tree, tvb, offset, FALSE );
			offset = aprs_default_string( aprs_tree, tvb, offset, -1, hf_aprs_comment );
			break;
		case '@'	: /* Position + timestamp + Ext APRS message */
			offset = aprs_timestamp( aprs_tree, tvb, offset );
			offset = aprs_position( aprs_tree, tvb, offset, TRUE );
			offset = aprs_default_string( aprs_tree, tvb, offset, -1, hf_aprs_comment );
			break;
		default	: break;
		}
	return offset;
}

void
proto_register_aprs( void )
{
	module_t *aprs_module;

	static hf_register_info hf[] = {
		{ &hf_aprs_dti,
			{ "Data Type Indicator",		"aprs.dti",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_sym_code,
			{ "Symbol code",		"aprs.sym_code",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_sym_id,
			{ "Symbol table ID",		"aprs.sym_id",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},

/* Position */
#if 0
		{ &hf_aprs_position,
			{ "Position",		"aprs.position",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
#endif
		{ &hf_aprs_lat,
			{ "Latitude",		"aprs.position.lat",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_long,
			{ "Longitude",		"aprs.position.long",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},

/* APRS Messages */
		{ &hf_aprs_comment,
			{ "Comment",			"aprs.comment",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_ultimeter_2000,
			{ "Ultimeter 2000",		"aprs.ultimeter_2000",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_status,
			{ "Status",			"aprs.status",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_object,
			{ "Object",			"aprs.object",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_item,
			{ "Item",			"aprs.item",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_query,
			{ "Query",			"aprs.query",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_telemetry,
			{ "Telemetry",			"aprs.telemetry",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_raw,
			{ "Raw",			"aprs.raw",
			FT_STRING, BASE_NONE, NULL, 0x0,
			"Raw NMEA position report format", HFILL }
		},
		{ &hf_aprs_station,
			{ "Station",			"aprs.station",
			FT_STRING, BASE_NONE, NULL, 0x0,
			"Station capabilities", HFILL }
		},
		{ &hf_aprs_message,
			{ "Message",			"aprs.message",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_agrelo,
			{ "Agrelo",			"aprs.agrelo",
			FT_BYTES, BASE_NONE, NULL, 0x0,
			"Agrelo DFJr / MicroFinder", HFILL }
		},
		{ &hf_aprs_maidenhead,
			{ "Maidenhead",			"aprs.maidenhead",
			FT_BYTES, BASE_NONE, NULL, 0x0,
			"Maidenhead grid locator beacon (obsolete)", HFILL }
		},
		{ &hf_aprs_invalid_test,
			{ "Invalid or test",		"aprs.invalid_test",
			FT_BYTES, BASE_NONE, NULL, 0x0,
			"Invalid data or test data", HFILL }
		},
		{ &hf_aprs_user_defined,
			{ "User-Defined",		"aprs.user_defined",
			FT_BYTES, BASE_NONE, NULL, 0x0,
			"User-Defined APRS packet format", HFILL }
		},
		{ &hf_aprs_third_party,
			{ "Third-party",		"aprs.third_party",
			FT_BYTES, BASE_NONE, NULL, 0x0,
			"Third-party traffic", HFILL }
		},
		{ &hf_aprs_peet_1,
			{ "Peet U-II (1)",		"aprs.peet_1",
			FT_BYTES, BASE_NONE, NULL, 0x0,
			"Peet Bros U-II Weather Station", HFILL }
		},
		{ &hf_aprs_peet_2,
			{ "Peet U-II (2)",		"aprs.peet_2",
			FT_BYTES, BASE_NONE, NULL, 0x0,
			"Peet Bros U-II Weather Station", HFILL }
		},
		{ &hf_aprs_map_feature,
			{ "Map Feature",		"aprs.map_feature",
			FT_BYTES, BASE_NONE, NULL, 0x0,
			"[Reserved - Map Feature", HFILL }
		},
		{ &hf_aprs_shelter_data,
			{ "Shelter data",		"aprs.shelter_data",
			FT_BYTES, BASE_NONE, NULL, 0x0,
			"[Reserved - Shelter data with time]", HFILL }
		},
		{ &hf_aprs_space_weather,
			{ "Space weather",		"aprs.space_weather",
			FT_BYTES, BASE_NONE, NULL, 0x0,
			"[Reserved - Space weather]", HFILL }
		},
		{ &hf_aprs_storm,
			{ "Storm",			"aprs.storm",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},

/* Time stamp */
		{ &hf_aprs_dhm,
			{ "Day/Hour/Minute",		"aprs.dhm",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_hms,
			{ "Hour/Minute/Second",		"aprs.hms",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_mdhm,
			{ "Month/Day/Hour/Minute",		"aprs.mdhm",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_tz,
			{ "Time Zone",		"aprs.tz",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},

/* Compressed Msg */
		{ &hf_aprs_compression_type,
			{ "Compression type",		"aprs.ct",
			FT_UINT8, BASE_HEX, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_ct_gps_fix,
			{ "GPS fix type",		"aprs.ct.gps_fix",
			FT_UINT8, BASE_HEX, VALS(gps_vals), 0x20,
			NULL, HFILL }
		},
		{ &hf_aprs_ct_nmea_src,
			{ "NMEA source",		"aprs.ct.nmea_src",
			FT_UINT8, BASE_HEX, VALS(nmea_vals), 0x18,
			NULL, HFILL }
		},
		{ &hf_aprs_ct_origin,
			{ "Compression origin",		"aprs.ct.origin",
			FT_UINT8, BASE_HEX, VALS(ctype_vals), 0x07,
			NULL, HFILL }
		},

/* Ext Msg */
		{ &hf_aprs_msg,
			{ "Extended message",			"aprs.msg",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_msg_rng,
			{ "Range",			"aprs.msg.rng",
			FT_STRING, BASE_NONE, NULL, 0x0,
			"Pre-calculated radio range", HFILL }
		},
		{ &hf_aprs_msg_cse,
			{ "Course",			"aprs.msg.cse",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_msg_spd,
			{ "Speed",			"aprs.msg.spd",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_msg_dir,
			{ "Wind direction",		"aprs.msg.dir",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_msg_brg,
			{ "Bearing",			"aprs.msg.brg",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_msg_nrq,
			{ "Number/Range/Quality",			"aprs.msg.nrq",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},

/* Msg PHGD */
		{ &hf_aprs_msg_phg_p,
			{ "Power",		"aprs.msg.phg.p",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_msg_phg_h,
			{ "Height",		"aprs.msg.phg.h",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_msg_phg_g,
			{ "Gain",		"aprs.msg.phg.g",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_msg_phg_d,
			{ "Directivity",		"aprs.msg.phg.d",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},

/* Msg DFS */
		{ &hf_aprs_msg_dfs_s,
			{ "Strength",			"aprs.msg.dfs.s",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_msg_dfs_h,
			{ "Height",			"aprs.msg.dfs.h",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_msg_dfs_g,
			{ "Gain",			"aprs.msg.dfs.g",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_msg_dfs_d,
			{ "Directivity",		"aprs.msg.dfs.d",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},

/* Msg AOD */
		{ &hf_aprs_msg_aod_t,
			{ "Type",			"aprs.msg.aod.t",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_msg_aod_c,
			{ "Colour",			"aprs.msg.aod.c",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},

/* Weather */
		{ &hf_aprs_weather,
			{ "Weather report",			"aprs.weather",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_weather_dir,
			{ "Wind direction",		"aprs.weather.dir",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_weather_spd,
			{ "Wind speed",			"aprs.weather.speed",
			FT_STRING, BASE_NONE, NULL, 0x0,
			"Wind speed (1 minute)", HFILL }
		},
		{ &hf_aprs_weather_peak,
			{ "Peak wind speed",		"aprs.weather.peak",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_weather_temp,
			{ "Temperature (F)",		"aprs.weather.temp",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_weather_rain_1,
			{ "Rain (last 1 hour)",			"aprs.weather.1_hour",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_weather_rain_24,
			{ "Rain (last 24 hours)",		"aprs.weather.24_hour",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_weather_rain,
			{ "Rain",			"aprs.weather.rain",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_weather_humidty,
			{ "Humidity",			"aprs.weather.humidity",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_weather_press,
			{ "Pressure",			"aprs.weather.pressure",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_weather_luminosity,
			{ "Luminosity",			"aprs.weather.luminosity",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_weather_snow,
			{ "Snow",			"aprs.weather.snow",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_weather_raw_rain,
			{ "Raw rain",			"aprs.weather.raw_rain",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_weather_software,
			{ "Software",			"aprs.weather.software",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_weather_unit,
			{ "Unit",			"aprs.weather.unit",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},

/* MIC-E */
		{ &hf_aprs_mic_e_0_current,
			{ "Current Mic-E (Rev 0)",	"aprs.mic_e_0_current",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_mic_e_0_old,
			{ "Old Mic-E (Rev 0)",		"aprs.mic_e_0_old",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_mic_e_old,
			{ "Old Mic-E",			"aprs.mic_e_old",
			FT_STRING, BASE_NONE, NULL, 0x0,
			"Old Mic-E Data (but Current data for TM-D700)", HFILL }
		},
		{ &hf_aprs_mic_e_current,
			{ "Current Mic-E",		"aprs.mic_e_current",
			FT_STRING, BASE_NONE, NULL, 0x0,
			"Current Mic-E Data (not used in TM-D700)", HFILL }
		},
		{ &hf_aprs_mic_e_dst,
			{ "Destination Address",		"aprs.mic_e.dst",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_mic_e_long_d,
			{ "Longitude degrees",		"aprs.mic_e.long_d",
			FT_UINT8, BASE_HEX, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_mic_e_long_m  ,
			{ "Longitude minutes",		"aprs.mic_e.long_m",
			FT_UINT8, BASE_HEX, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_mic_e_long_h,
			{ "Longitude hundreths of minutes",	"aprs.mic_e.long_h",
			FT_UINT8, BASE_HEX, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_mic_e_spd_sp,
			{ "Speed (hundreds & tens)",		"aprs.mic_e.speed_sp",
			FT_UINT8, BASE_HEX, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_mic_e_spd_dc,
			{ "Speed (tens), Course (hundreds)",		"aprs.mic_e.speed_dc",
			FT_UINT8, BASE_HEX, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_mic_e_spd_se,
			{ "Course (tens & units)",	"aprs.mic_e.speed_se",
			FT_UINT8, BASE_HEX, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_mic_e_telemetry,
			{ "Telemetry",	"aprs.mic_e.telemetry",
			FT_BYTES, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_mic_e_status,
			{ "Status",	"aprs.mic_e.status",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_storm_dir,
			{ "Direction",	"aprs.storm.direction",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_storm_spd,
			{ "Speed (knots)",	"aprs.storm.speed",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_storm_type,
			{ "Type",	"aprs.storm.type",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_storm_sws,
			{ "Sustained wind speed (knots)",	"aprs.storm.sws",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_storm_pwg,
			{ "Peak wind gusts (knots)",	"aprs.storm.pwg",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_storm_cp,
			{ "Central pressure (millibars/hPascal)",	"aprs.storm.central_pressure",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_storm_rhw,
			{ "Radius Hurricane Winds (nautical miles)",	"aprs.storm.radius_hurricane_winds",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_storm_rtsw,
			{ "Radius Tropical Storm Winds (nautical miles)",	"aprs.storm.radius_tropical_storms_winds",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_aprs_storm_rwg,
			{ "Radius Whole Gale (nautical miles)",	"aprs.storm.radius_whole_gale",
			FT_STRING, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		}
	};

	static gint *ett[] = {
		&ett_aprs,
		&ett_aprs_msg,
		&ett_aprs_ct,
		&ett_aprs_weather,
		&ett_aprs_storm,
		&ett_aprs_mic_e,
	};

	proto_aprs = proto_register_protocol("Automatic Position Reporting System", "APRS", "aprs");

	register_dissector( "aprs", dissect_aprs, proto_aprs);

	proto_register_field_array( proto_aprs, hf, array_length(hf ) );
	proto_register_subtree_array( ett, array_length( ett ) );

	aprs_module = prefs_register_protocol( proto_aprs, NULL);

	prefs_register_bool_preference(aprs_module, "showaprslax",
		"Allow APRS violations.",
		"Attempt to display common APRS protocol violations correctly",
		&gPREF_APRS_LAX );

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
