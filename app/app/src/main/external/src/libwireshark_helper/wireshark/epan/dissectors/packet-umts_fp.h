/* packet-fp.h
 *
 * Martin Mathieson
 * $Id: packet-umts_fp.h 35188 2010-12-15 01:45:43Z martinm $
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

/* Channel types */
#define CHANNEL_RACH_FDD      1
#define CHANNEL_RACH_TDD      2
#define CHANNEL_FACH_FDD      3
#define CHANNEL_FACH_TDD      4
#define CHANNEL_DSCH_FDD      5
#define CHANNEL_DSCH_TDD      6
#define CHANNEL_USCH_TDD_384  8
#define CHANNEL_USCH_TDD_128  24
#define CHANNEL_PCH           9
#define CHANNEL_CPCH          10
#define CHANNEL_BCH           11
#define CHANNEL_DCH           12
#define CHANNEL_HSDSCH        13
#define CHANNEL_IUR_CPCHF     14
#define CHANNEL_IUR_FACH      15
#define CHANNEL_IUR_DSCH      16
#define CHANNEL_EDCH          17
#define CHANNEL_RACH_TDD_128  18
#define CHANNEL_HSDSCH_COMMON 19
#define CHANNEL_HSDSCH_COMMON_T3 20
#define CHANNEL_EDCH_COMMON      21

enum fp_interface_type
{
    IuB_Interface,
    IuR_Interface
};

enum division_type
{
    Division_FDD=1,
    Division_TDD_384=2,
    Division_TDD_128=3,
    Division_TDD_768=4
};

enum fp_hsdsch_entity
{
    entity_not_specified=0,
    hs=1,
    ehs=2
};

enum fp_link_type
{
    FP_Link_Unknown,
    FP_Link_ATM,
    FP_Link_Ethernet
};

/* Info attached to each FP packet */
typedef struct fp_info
{
    enum fp_interface_type iface_type;
    enum division_type     division;
    guint8  release;                     /* e.g. 99, 4, 5, 6, 7 */
    guint16 release_year;                /* e.g. 2001 */
    guint8  release_month;               /* e.g. 12 for December */
    gboolean is_uplink;
    gint channel;                       /* see definitions above */
    guint8  dch_crc_present;            /* 0=No, 1=Yes, 2=Unknown */
    gint paging_indications;
    gint num_chans;
#define MAX_FP_CHANS  64
    gint chan_tf_size[MAX_FP_CHANS];
    gint chan_num_tbs[MAX_FP_CHANS];

#define MAX_EDCH_DDIS 16
    gint   no_ddi_entries;
    guint8 edch_ddi[MAX_EDCH_DDIS];
    guint  edch_macd_pdu_size[MAX_EDCH_DDIS];
    guint8 edch_type;  /* 1 means T2 */

    gint cur_tb;	/* current transport block (required for dissecting of single TBs */
    gint cur_chan;  /* current channel, required to retrieve the correct channel configuration for UMTS MAC */

    guint16 srcport, destport;

    enum   fp_hsdsch_entity hsdsch_entity;
    enum   fp_link_type link_type;
} fp_info;

