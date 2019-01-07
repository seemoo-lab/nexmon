/* packet-ieee80211-wlancap.c
 * Routines for AVS linux-wlan monitoring mode header dissection
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * Copied from README.developer
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
#include <epan/capture_dissectors.h>
#include <wiretap/wtap.h>
#include <wsutil/pint.h>
#include <wsutil/frequency-utils.h>

#include "packet-ieee80211.h"

void proto_register_ieee80211_wlancap(void);
void proto_reg_handoff_ieee80211_wlancap(void);

static dissector_handle_t ieee80211_radio_handle;

static int proto_wlancap = -1;

/* AVS WLANCAP radio header */
static int hf_wlancap_magic = -1;
static int hf_wlancap_version = -1;
static int hf_wlancap_length = -1;
static int hf_wlancap_mactime = -1;
static int hf_wlancap_hosttime = -1;
static int hf_wlancap_phytype = -1;
static int hf_wlancap_hop_set = -1;
static int hf_wlancap_hop_pattern = -1;
static int hf_wlancap_hop_index = -1;
static int hf_wlancap_channel = -1;
static int hf_wlancap_channel_frequency = -1;
static int hf_wlancap_data_rate = -1;
static int hf_wlancap_antenna = -1;
static int hf_wlancap_priority = -1;
static int hf_wlancap_ssi_type = -1;
static int hf_wlancap_normrssi_antsignal = -1;
static int hf_wlancap_dbm_antsignal = -1;
static int hf_wlancap_rawrssi_antsignal = -1;
static int hf_wlancap_normrssi_antnoise = -1;
static int hf_wlancap_dbm_antnoise = -1;
static int hf_wlancap_rawrssi_antnoise = -1;
static int hf_wlancap_preamble = -1;
static int hf_wlancap_encoding = -1;
static int hf_wlancap_sequence = -1;
static int hf_wlancap_drops = -1;
static int hf_wlancap_receiver_addr = -1;
static int hf_wlancap_padding = -1;

static gint ett_wlancap = -1;

static dissector_handle_t wlancap_handle;

gboolean
capture_wlancap(const guchar *pd, int offset, int len, capture_packet_info_t *cpinfo, const union wtap_pseudo_header *pseudo_header _U_)
{
  guint32 length;

  if (!BYTES_ARE_IN_FRAME(offset, len, sizeof(guint32)*2))
    return FALSE;

  length = pntoh32(pd+sizeof(guint32));

  if (!BYTES_ARE_IN_FRAME(offset, len, length))
    return FALSE;

  offset += length;

  /* 802.11 header follows */
  return capture_ieee80211(pd, offset, len, cpinfo, pseudo_header);
}

/*
 * AVS linux-wlan-based products use a new sniff header to replace the
 * old Prism header.  This one has additional fields, is designed to be
 * non-hardware-specific, and more importantly, version and length fields
 * so it can be extended later without breaking anything.
 *
 * Support by Solomon Peachy
 *
 * Description, from the capturefrm.txt file in the linux-wlan-ng 0.2.9
 * release (linux-wlan-ng-0.2.9/doc/capturefrm.txt):
 *
AVS Capture Frame Format
Version 2.1.1

1. Introduction
The original header format for "monitor mode" or capturing frames was
a considerable hack.  The document covers a redesign of that format.

  Any questions, corrections, or proposed changes go to info@linux-wlan.com

2. Frame Format
All sniff frames follow the same format:

        Offset  Name            Size            Description
        --------------------------------------------------------------------
        0       CaptureHeader                   AVS capture metadata header
        64      802.11Header    [10-30]         802.11 frame header
        ??      802.11Payload   [0-2312]        802.11 frame payload
        ??      802.11FCS       4               802.11 frame check sequence

Note that the header and payload are variable length and the payload
may be empty.

If the hardware does not supply the FCS to the driver, then the frame shall
have a FCS of 0xFFFFFFFF.

3. Byte Order
All multibyte fields of the capture header are in "network" byte
order.  The "host to network" and "network to host" functions should
work just fine.  All the remaining multibyte fields are ordered
according to their respective standards.

4. Capture Header Format
The following fields make up the AVS capture header:

        Offset  Name            Type
        ------------------------------
        0       version         uint32
        4       length          uint32
        8       mactime         uint64
        16      hosttime        uint64
        24      phytype         uint32
        28      frequency       uint32
        32      datarate        uint32
        36      antenna         uint32
        40      priority        uint32
        44      ssi_type        uint32
        48      ssi_signal      int32
        52      ssi_noise       int32
        56      preamble        uint32
        60      encoding        uint32
        64      sequence        uint32
        68      drops           uint32
        72      receiver_addr   uint8[6]
        78      padding         uint8[2]
        ------------------------------
        80

The following subsections detail the fields of the capture header.

4.1 version
The version field identifies this type of frame as a subtype of
ETH_P_802111_CAPTURE as received by an ARPHRD_IEEE80211_PRISM or
an ARPHRD_IEEE80211_CAPTURE device.  The value of this field shall be
0x80211002.  As new revisions of this header are necessary, we can
increment the version appropriately.

4.2 length
The length field contains the length of the entire AVS capture header,
in bytes.

4.3 mactime
Many WLAN devices supply a relatively high resolution frame reception
time value.  This field contains the value supplied by the device.  If
the device does not supply a receive time value, this field shall be
set to zero.  The units for this field are microseconds.

If possible, this time value should be absolute, representing the number
of microseconds elapsed since the UNIX epoch.

4.4 hosttime
The hosttime field is set to the current value of the host maintained
clock variable when the frame is received by the host.

If possible, this time value should be absolute, representing the number
of microseconds elapsed since the UNIX epoch.

4.5 phytype
The phytype field identifies what type of PHY is employed by the WLAN
device used to capture this frame.  The valid values are:

        PhyType                         Value
        -------------------------------------
        phytype_fhss_dot11_97            1
        phytype_dsss_dot11_97            2
        phytype_irbaseband               3
        phytype_dsss_dot11_b             4
        phytype_pbcc_dot11_b             5
        phytype_ofdm_dot11_g             6
        phytype_pbcc_dot11_g             7
        phytype_ofdm_dot11_a             8
        phytype_dss_ofdm_dot11_g         9

4.6 frequency

This represents the frequency or channel number of the receiver at the
time the frame was received.  It is interpreted as follows:

For frequency hopping radios, this field is broken in to the
following subfields:

        Byte    Subfield
        ------------------------
        Byte0   Hop Set
        Byte1   Hop Pattern
        Byte2   Hop Index
        Byte3   reserved

For non-hopping radios, the frequency is interpreted as follows:

       Value                Meaning
    -----------------------------------------
       < 256           Channel number (using externally-defined
                         channelization)
       < 10000         Center frequency, in MHz
      >= 10000         Center frequency, in KHz

4.7 datarate
The data rate field contains the rate at which the frame was received
in units of 100kbps.

4.8 antenna
For WLAN devices that indicate the receive antenna for each frame, the
antenna field shall contain an index value into the dot11AntennaList.
If the device does not indicate a receive antenna value, this field
shall be set to zero.

4.9 priority
The priority field indicates the receive priority of the frame.  The
value is in the range [0-15] with the value 0 reserved to indicate
contention period and the value 6 reserved to indicate contention free
period.

4.10 ssi_type
The ssi_type field is used to indicate what type of signal strength
information is present: "None", "Normalized RSSI" or "dBm".  "None"
indicates that the underlying WLAN device does not supply any signal
strength at all and the ssi_* values are unset.  "Normalized RSSI"
values are integers in the range [0-1000] where higher numbers
indicate stronger signal.  "dBm" values indicate an actual signal
strength measurement quantity and are usually in the range [-108 - 10].
The following values indicate the three types:

        Value   Description
        ---------------------------------------------
        0       None
        1       Normalized RSSI
        2       dBm
        3       Raw RSSI

4.11 ssi_signal
The ssi_signal field contains the signal strength value reported by
the WLAN device for this frame.  Note that this is a signed quantity
and if the ssi_type value is "dBm" that the value may be negative.

4.12 ssi_noise
The ssi_noise field contains the noise or "silence" value reported by
the WLAN device.  This value is commonly defined to be the "signal
strength reported immediately prior to the baseband processor lock on
the frame preamble".  If the hardware does not provide noise data, this
shall equal 0xffffffff.

4.12 preamble
For PHYs that support variable preamble lengths, the preamble field
indicates the preamble type used for this frame.  The values are:

        Value   Description
        ---------------------------------------------
        0       Undefined
        1       Short Preamble
        2       Long Preamble

4.13 encoding
This specifies the encoding of the received packet.  For PHYs that support
multiple encoding types, this will tell us which one was used.

        Value   Description
        ---------------------------------------------
        0       Unknown
        1       CCK
        2       PBCC
        3       OFDM
        4       DSSS-OFDM
        5       BPSK
        6       QPSK
        7       16QAM
        8       64QAM

4.14 sequence
This is a receive frame sequence counter.  The sniff host shall
increment this by one for every valid frame received off the medium.
By watching for gaps in the sequence numbers we can determine when
packets are lost due to unreliable transport, rather than a frame never
being received to begin with.

4.15 drops
This is a counter of the number of known frame drops that occurred.  This
is particularly useful when the system or hardware cannot keep up with
the sniffer load.

4.16 receiver_addr
This specifies the MAC address of the receiver of this frame.
It is six octets in length.  This field is followed by two octets of
padding to keep the structure 32-bit word aligned.

================================

Changes: v2->v2.1

 * Added contact e-mail address to introduction
 * Added sniffer_addr, drop count, and sequence fields, bringing total
   length to 80 bytes
 * Bumped version to 0x80211002
 * Mactime is specified in microseconds, not nanoseconds
 * Added 64QAM, 16QAM, BPSK, QPSK encodings

================================

Changes: v2.1->v2.1.1

 * Renamed 'channel' to 'frequency'
 * Clarified the interpretation of the frequency/channel field.
 * Renamed 'sniffer address' to 'receiver address'
 * Clarified timestamp fields.
 */

/*
 * Signal/noise strength type values.
 */
#define SSI_NONE        0       /* no SSI information */
#define SSI_NORM_RSSI   1       /* normalized RSSI - 0-1000 */
#define SSI_DBM         2       /* dBm */
#define SSI_RAW_RSSI    3       /* raw RSSI from the hardware */

static int
dissect_wlancap(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
    proto_tree *wlan_tree = NULL;
    proto_item *ti;
    tvbuff_t *next_tvb;
    int offset;
    guint32 version;
    guint32 length;
    guint32 channel;
    guint   frequency;
    gint    calc_channel;
    guint32 datarate;
    guint32 ssi_type;
    gint32  dbm;
    guint32 antnoise;
    struct ieee_802_11_phdr phdr;

    /* We don't have any 802.11 metadata yet. */
    memset(&phdr, 0, sizeof(phdr));
    phdr.fcs_len = -1;
    phdr.decrypted = FALSE;
    phdr.datapad = FALSE;
    phdr.phy = PHDR_802_11_PHY_UNKNOWN;

    col_set_str(pinfo->cinfo, COL_PROTOCOL, "WLAN");
    col_clear(pinfo->cinfo, COL_INFO);
    offset = 0;

    version = tvb_get_ntohl(tvb, offset) - WLANCAP_MAGIC_COOKIE_BASE;

    length = tvb_get_ntohl(tvb, offset+4);

    col_add_fstr(pinfo->cinfo, COL_INFO, "AVS WLAN Capture v%x, Length %d",version, length);

    if (version > 2) {
      goto skip;
    }

    /* Dissect the AVS header */
    if (tree) {
      ti = proto_tree_add_item(tree, proto_wlancap, tvb, 0, length, ENC_NA);
      wlan_tree = proto_item_add_subtree(ti, ett_wlancap);
      proto_tree_add_item(wlan_tree, hf_wlancap_magic, tvb, offset, 4, ENC_BIG_ENDIAN);
      proto_tree_add_item(wlan_tree, hf_wlancap_version, tvb, offset, 4, ENC_BIG_ENDIAN);
    }
    offset+=4;
    if (tree)
      proto_tree_add_item(wlan_tree, hf_wlancap_length, tvb, offset, 4, ENC_BIG_ENDIAN);
    offset+=4;
    phdr.has_tsf_timestamp = TRUE;
    phdr.tsf_timestamp = tvb_get_ntoh64(tvb, offset);
    if (tree)
      proto_tree_add_item(wlan_tree, hf_wlancap_mactime, tvb, offset, 8, ENC_BIG_ENDIAN);
    offset+=8;
    if (tree)
      proto_tree_add_item(wlan_tree, hf_wlancap_hosttime, tvb, offset, 8, ENC_BIG_ENDIAN);
    offset+=8;
    switch (tvb_get_ntohl(tvb, offset)) {

    case 1:
        phdr.phy = PHDR_802_11_PHY_11_FHSS;
        break;

    case 2:
        phdr.phy = PHDR_802_11_PHY_11_DSSS;
        break;

    case 3:
        phdr.phy = PHDR_802_11_PHY_11_IR;
        break;

    case 4:
        phdr.phy = PHDR_802_11_PHY_11B;
        break;

    case 5:
        /* 11b PBCC? */
        phdr.phy = PHDR_802_11_PHY_11B;
        break;

    case 6:
        phdr.phy = PHDR_802_11_PHY_11G; /* pure? */
        break;

    case 7:
        /* 11a PBCC? */
        phdr.phy = PHDR_802_11_PHY_11A;
        break;

    case 8:
        phdr.phy = PHDR_802_11_PHY_11A;
        break;

    case 9:
        phdr.phy = PHDR_802_11_PHY_11G; /* mixed? */
        break;
    }
    if (tree)
      proto_tree_add_item(wlan_tree, hf_wlancap_phytype, tvb, offset, 4, ENC_BIG_ENDIAN);
    offset+=4;

    if (phdr.phy == PHDR_802_11_PHY_11_FHSS) {
      phdr.phy_info.info_11_fhss.has_hop_set = TRUE;
      phdr.phy_info.info_11_fhss.hop_set = tvb_get_guint8(tvb, offset);
      if (tree)
        proto_tree_add_item(wlan_tree, hf_wlancap_hop_set, tvb, offset, 1, ENC_NA);
      phdr.phy_info.info_11_fhss.has_hop_pattern = TRUE;
      phdr.phy_info.info_11_fhss.hop_pattern = tvb_get_guint8(tvb, offset + 1);
      if (tree)
        proto_tree_add_item(wlan_tree, hf_wlancap_hop_pattern, tvb, offset + 1, 1, ENC_NA);
      phdr.phy_info.info_11_fhss.has_hop_index = TRUE;
      phdr.phy_info.info_11_fhss.hop_index = tvb_get_guint8(tvb, offset + 2);
      if (tree)
        proto_tree_add_item(wlan_tree, hf_wlancap_hop_index, tvb, offset + 2, 1, ENC_NA);
    } else {
      channel = tvb_get_ntohl(tvb, offset);
      if (channel < 256) {
        col_add_fstr(pinfo->cinfo, COL_FREQ_CHAN, "%u", channel);
        phdr.has_channel = TRUE;
        phdr.channel = channel;
        if (tree)
          proto_tree_add_uint(wlan_tree, hf_wlancap_channel, tvb, offset, 4, channel);
        frequency = ieee80211_chan_to_mhz(channel, (phdr.phy != PHDR_802_11_PHY_11A));
        if (frequency != 0) {
          phdr.has_frequency = TRUE;
          phdr.frequency = frequency;
        }
      } else if (channel < 10000) {
        col_add_fstr(pinfo->cinfo, COL_FREQ_CHAN, "%u MHz", channel);
        phdr.has_frequency = TRUE;
        phdr.frequency = channel;
        if (tree)
          proto_tree_add_uint_format(wlan_tree, hf_wlancap_channel_frequency, tvb, offset,
                                     4, channel, "Frequency: %u MHz", channel);
        calc_channel = ieee80211_mhz_to_chan(channel);
        if (calc_channel != -1) {
          phdr.has_channel = TRUE;
          phdr.channel = calc_channel;
        }
      } else {
        col_add_fstr(pinfo->cinfo, COL_FREQ_CHAN, "%u KHz", channel);
        if (tree)
          proto_tree_add_uint_format(wlan_tree, hf_wlancap_channel_frequency, tvb, offset,
                                     4, channel, "Frequency: %u KHz", channel);
      }
    }
    offset+=4;
    datarate = tvb_get_ntohl(tvb, offset);
    if (datarate < 100000) {
      /* In units of 100 Kb/s; convert to b/s */
      datarate *= 100000;
    }

    col_add_fstr(pinfo->cinfo, COL_TX_RATE, "%u.%u",
                   datarate / 1000000,
                   ((datarate % 1000000) > 500000) ? 5 : 0);
    if (datarate != 0) {
      /* 0 is obviously bogus; it may be used for "unknown" */
      /* Can this be expressed in .5 MHz units? */
      if ((datarate % 500000) == 0) {
        /* Yes. */
        phdr.has_data_rate = TRUE;
        phdr.data_rate = datarate / 500000;
      }
    }
    if (tree) {
      proto_tree_add_uint64_format_value(wlan_tree, hf_wlancap_data_rate, tvb, offset, 4,
                                   datarate,
                                   "%u.%u Mb/s",
                                   datarate/1000000,
                                   ((datarate % 1000000) > 500000) ? 5 : 0);
    }
    offset+=4;
    if (tree)
      proto_tree_add_item(wlan_tree, hf_wlancap_antenna, tvb, offset, 4, ENC_BIG_ENDIAN);
    offset+=4;
    if (tree)
      proto_tree_add_item(wlan_tree, hf_wlancap_priority, tvb, offset, 4, ENC_BIG_ENDIAN);
    offset+=4;
    ssi_type = tvb_get_ntohl(tvb, offset);
    if (tree)
      proto_tree_add_uint(wlan_tree, hf_wlancap_ssi_type, tvb, offset, 4, ssi_type);
    offset+=4;
    switch (ssi_type) {

    case SSI_NONE:
    default:
      /* either there is no SSI information, or we don't know what type it is */
      break;

    case SSI_NORM_RSSI:
      /* Normalized RSSI */
      col_add_fstr(pinfo->cinfo, COL_RSSI, "%u (norm)", tvb_get_ntohl(tvb, offset));
      if (tree)
        proto_tree_add_item(wlan_tree, hf_wlancap_normrssi_antsignal, tvb, offset, 4, ENC_BIG_ENDIAN);
      break;

    case SSI_DBM:
      /* dBm */
      dbm = tvb_get_ntohl(tvb, offset);
      phdr.has_signal_dbm = TRUE;
      phdr.signal_dbm = dbm;
      col_add_fstr(pinfo->cinfo, COL_RSSI, "%d dBm", dbm);
      if (tree)
        proto_tree_add_item(wlan_tree, hf_wlancap_dbm_antsignal, tvb, offset, 4, ENC_BIG_ENDIAN);
      break;

    case SSI_RAW_RSSI:
      /* Raw RSSI */
      col_add_fstr(pinfo->cinfo, COL_RSSI, "%u (raw)", tvb_get_ntohl(tvb, offset));
      if (tree)
        proto_tree_add_item(wlan_tree, hf_wlancap_rawrssi_antsignal, tvb, offset, 4, ENC_BIG_ENDIAN);
      break;
    }
    offset+=4;
    antnoise = tvb_get_ntohl(tvb, offset);
    /* 0xffffffff means "hardware does not provide noise data" */
    if (antnoise != 0xffffffff) {
      switch (ssi_type) {

      case SSI_NONE:
      default:
        /* either there is no SSI information, or we don't know what type it is */
        break;

      case SSI_NORM_RSSI:
        /* Normalized RSSI */
        if (tree)
          proto_tree_add_uint(wlan_tree, hf_wlancap_normrssi_antnoise, tvb, offset, 4, antnoise);
        break;

      case SSI_DBM:
        /* dBm */
        if (antnoise != 0) {
          /* The spec says use 0xffffffff, but some drivers appear to use 0. */
          phdr.has_noise_dbm = TRUE;
          phdr.noise_dbm = antnoise;
        }
        if (tree)
          proto_tree_add_int(wlan_tree, hf_wlancap_dbm_antnoise, tvb, offset, 4, antnoise);
        break;

      case SSI_RAW_RSSI:
        /* Raw RSSI */
        if (tree)
          proto_tree_add_uint(wlan_tree, hf_wlancap_rawrssi_antnoise, tvb, offset, 4, antnoise);
        break;
      }
    }
    offset+=4;
    switch (tvb_get_ntohl(tvb, offset)) {

    case 0:
      /* Undefined, so we don't know if there's a short preamble */
      break;

    case 1:
      /*
       * Short preamble.
       */
      switch (phdr.phy) {

      case PHDR_802_11_PHY_11B:
        phdr.phy_info.info_11b.has_short_preamble = TRUE;
        phdr.phy_info.info_11b.short_preamble = TRUE;
        break;

      case PHDR_802_11_PHY_11G:
        phdr.phy_info.info_11g.has_short_preamble = TRUE;
        phdr.phy_info.info_11g.short_preamble = TRUE;
        break;
      }
      break;

    case 2:
      /*
       * Long preamble.
       * We assume this is present only for PHYs that support variable
       * preamble lengths.
       */
      switch (phdr.phy) {

      case PHDR_802_11_PHY_11B:
        phdr.phy_info.info_11b.has_short_preamble = TRUE;
        phdr.phy_info.info_11b.short_preamble = FALSE;
        break;

      case PHDR_802_11_PHY_11G:
        phdr.phy_info.info_11g.has_short_preamble = TRUE;
        phdr.phy_info.info_11g.short_preamble = FALSE;
        break;
      }
      break;

    default:
      /* Invalid, so we don't know if there's a short preamble. */
      break;
    }
    if (tree)
      proto_tree_add_item(wlan_tree, hf_wlancap_preamble, tvb, offset, 4, ENC_BIG_ENDIAN);
    offset+=4;
    if (tree)
      proto_tree_add_item(wlan_tree, hf_wlancap_encoding, tvb, offset, 4, ENC_BIG_ENDIAN);
    offset+=4;
    if (version > 1) {
      if (tree)
        proto_tree_add_item(wlan_tree, hf_wlancap_sequence, tvb, offset, 4, ENC_BIG_ENDIAN);
      offset+=4;
      if (tree)
        proto_tree_add_item(wlan_tree, hf_wlancap_drops, tvb, offset, 4, ENC_BIG_ENDIAN);
      offset+=4;
      if (tree)
        proto_tree_add_item(wlan_tree, hf_wlancap_receiver_addr, tvb, offset, 6, ENC_NA);
      offset+=6;
      if (tree)
        proto_tree_add_item(wlan_tree, hf_wlancap_padding, tvb, offset, 2, ENC_NA);
      /*offset+=2;*/
    }


 skip:
    offset = length;

    /* dissect the 802.11 header next */
    next_tvb = tvb_new_subset_remaining(tvb, offset);
    call_dissector_with_data(ieee80211_radio_handle, next_tvb, pinfo, tree, (void *)&phdr);
    return tvb_captured_length(tvb);
}

static const value_string phy_type[] = {
    { 0, "Unknown" },
    { 1, "FHSS 802.11 '97" },
    { 2, "DSSS 802.11 '97" },
    { 3, "IR Baseband" },
    { 4, "DSSS 802.11b" },
    { 5, "PBCC 802.11b" },
    { 6, "OFDM 802.11g" },
    { 7, "PBCC 802.11g" },
    { 8, "OFDM 802.11a" },
    { 0, NULL }
};

static const value_string encoding_type[] = {
    { 0, "Unknown" },
    { 1, "CCK" },
    { 2, "PBCC" },
    { 3, "OFDM" },
    { 4, "DSS-OFDM" },
    { 5, "BPSK" },
    { 6, "QPSK" },
    { 7, "16QAM" },
    { 8, "64QAM" },
    { 0, NULL }
};

static const value_string ssi_type[] = {
    { SSI_NONE, "None" },
    { SSI_NORM_RSSI, "Normalized RSSI" },
    { SSI_DBM, "dBm" },
    { SSI_RAW_RSSI, "Raw RSSI" },
    { 0, NULL }
};

static const value_string preamble_type[] = {
    { 0, "Unknown" },
    { 1, "Short" },
    { 2, "Long" },
    { 0, NULL }
};

static hf_register_info hf_wlancap[] = {
    {&hf_wlancap_magic,
     {"Header magic", "wlancap.magic", FT_UINT32, BASE_HEX, NULL, 0xFFFFFFF0,
      NULL, HFILL }},

    {&hf_wlancap_version,
     {"Header revision", "wlancap.version", FT_UINT32, BASE_DEC, NULL, 0xF,
      NULL, HFILL }},

    {&hf_wlancap_length,
     {"Header length", "wlancap.length", FT_UINT32, BASE_DEC, NULL, 0x0,
      NULL, HFILL }},

    {&hf_wlancap_mactime,
     {"MAC timestamp", "wlancap.mactime", FT_UINT64, BASE_DEC, NULL, 0x0,
      "Value in microseconds of the MAC's Time Synchronization Function timer when the first bit of the MPDU arrived at the MAC", HFILL }},

    {&hf_wlancap_hosttime,
     {"Host timestamp", "wlancap.hosttime", FT_UINT64, BASE_DEC, NULL, 0x0,
      NULL, HFILL }},

    {&hf_wlancap_phytype,
     {"PHY type", "wlancap.phytype", FT_UINT32, BASE_DEC, VALS(phy_type), 0x0,
      NULL, HFILL }},

    {&hf_wlancap_hop_set,
     {"Hop set", "wlancap.fhss.hop_set", FT_UINT8, BASE_HEX, NULL, 0x0,
      NULL, HFILL }},

    {&hf_wlancap_hop_pattern,
     {"Hop pattern", "wlancap.fhss.hop_pattern", FT_UINT8, BASE_HEX, NULL, 0x0,
      NULL, HFILL }},

    {&hf_wlancap_hop_index,
     {"Hop index", "wlancap.fhss.hop_index", FT_UINT8, BASE_HEX, NULL, 0x0,
      NULL, HFILL }},

    {&hf_wlancap_channel,
     {"Channel", "wlancap.channel", FT_UINT8, BASE_DEC, NULL, 0x0,
      "802.11 channel number that this frame was sent/received on", HFILL }},

    {&hf_wlancap_channel_frequency,
     {"Channel frequency", "wlancap.channel_frequency", FT_UINT32, BASE_DEC, NULL, 0x0,
      "Channel frequency in megahertz that this frame was sent/received on", HFILL }},

    {&hf_wlancap_data_rate,
     {"Data Rate", "wlancap.data_rate", FT_UINT64, BASE_DEC, NULL, 0x0,
      "Data rate (b/s)", HFILL }},

    {&hf_wlancap_antenna,
     {"Antenna", "wlancap.antenna", FT_UINT32, BASE_DEC, NULL, 0x0,
      "Antenna number this frame was sent/received over (starting at 0)", HFILL } },

    {&hf_wlancap_priority,
     {"Priority", "wlancap.priority", FT_UINT32, BASE_DEC, NULL, 0x0,
      NULL, HFILL }},

    {&hf_wlancap_ssi_type,
     {"SSI Type", "wlancap.ssi_type", FT_UINT32, BASE_DEC, VALS(ssi_type), 0x0,
      NULL, HFILL }},

    {&hf_wlancap_normrssi_antsignal,
     {"Normalized RSSI Signal", "wlancap.normrssi_antsignal", FT_UINT32, BASE_DEC, NULL, 0x0,
      "RF signal power at the antenna, normalized to the range 0-1000", HFILL }},

    {&hf_wlancap_dbm_antsignal,
     {"SSI Signal (dBm)", "wlancap.dbm_antsignal", FT_INT32, BASE_DEC, NULL, 0x0,
      "RF signal power at the antenna from a fixed, arbitrary value in decibels from one milliwatt", HFILL }},

    {&hf_wlancap_rawrssi_antsignal,
     {"Raw RSSI Signal", "wlancap.rawrssi_antsignal", FT_UINT32, BASE_DEC, NULL, 0x0,
      "RF signal power at the antenna, reported as RSSI by the adapter", HFILL }},

    {&hf_wlancap_normrssi_antnoise,
     {"Normalized RSSI Noise", "wlancap.normrssi_antnoise", FT_UINT32, BASE_DEC, NULL, 0x0,
      "RF noise power at the antenna, normalized to the range 0-1000", HFILL }},

    {&hf_wlancap_dbm_antnoise,
     {"SSI Noise (dBm)", "wlancap.dbm_antnoise", FT_INT32, BASE_DEC, NULL, 0x0,
      "RF noise power at the antenna from a fixed, arbitrary value in decibels per one milliwatt", HFILL }},

    {&hf_wlancap_rawrssi_antnoise,
     {"Raw RSSI Noise", "wlancap.rawrssi_antnoise", FT_UINT32, BASE_DEC, NULL, 0x0,
      "RF noise power at the antenna, reported as RSSI by the adapter", HFILL }},

    {&hf_wlancap_preamble,
     {"Preamble", "wlancap.preamble", FT_UINT32, BASE_DEC, VALS(preamble_type), 0x0,
      NULL, HFILL }},

    {&hf_wlancap_encoding,
     {"Encoding Type", "wlancap.encoding", FT_UINT32, BASE_DEC, VALS(encoding_type), 0x0,
      NULL, HFILL }},

    {&hf_wlancap_sequence,
     {"Receive sequence", "wlancap.sequence", FT_UINT32, BASE_DEC, NULL, 0x0,
      NULL, HFILL }},

    {&hf_wlancap_drops,
     {"Known Dropped Frames", "wlancap.drops", FT_UINT32, BASE_DEC, NULL, 0x0,
      NULL, HFILL }},

    {&hf_wlancap_receiver_addr,
     {"Receiver Address", "wlancap.receiver_addr", FT_ETHER, BASE_NONE, NULL, 0x0,
      "Receiver Hardware Address", HFILL }},

    {&hf_wlancap_padding,
     {"Padding", "wlancap.padding", FT_BYTES, BASE_NONE, NULL, 0x0,
      NULL, HFILL }}
};

static gint *tree_array[] = {
  &ett_wlancap
};

void proto_register_ieee80211_wlancap(void)
{
  proto_wlancap = proto_register_protocol("AVS WLAN Capture header",
                                          "AVS WLANCAP", "wlancap");
  proto_register_field_array(proto_wlancap, hf_wlancap,
                             array_length(hf_wlancap));
  register_dissector("wlancap", dissect_wlancap, proto_wlancap);

  wlancap_handle = create_dissector_handle(dissect_wlancap, proto_wlancap);
  dissector_add_uint("wtap_encap", WTAP_ENCAP_IEEE_802_11_AVS,
                     wlancap_handle);
  proto_register_subtree_array(tree_array, array_length(tree_array));
}

void proto_reg_handoff_ieee80211_wlancap(void)
{
  ieee80211_radio_handle = find_dissector_add_dependency("wlan_radio", proto_wlancap);
  register_capture_dissector("wtap_encap", WTAP_ENCAP_IEEE_802_11_AVS, capture_wlancap, proto_wlancap);
}

/*
 * Editor modelines
 *
 * Local Variables:
 * c-basic-offset: 2
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * ex: set shiftwidth=2 tabstop=8 expandtab:
 * :indentSize=2:tabSize=8:noTabs=true:
 */
