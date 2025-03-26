/*
 * Copyright (c) 2003, 2004 David Young.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of David Young may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY DAVID YOUNG ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL DAVID
 * YOUNG BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */

/*
 * Modifications to fit into the linux IEEE 802.11 stack,
 * Mike Kershaw (dragorn@kismetwireless.net)
 */

#ifndef IEEE80211RADIOTAP_H
#define IEEE80211RADIOTAP_H

#include <types.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

extern const struct ieee80211_radiotap_namespace radiotap_ns;

/* Base version of the radiotap packet header data */
#define PKTHDR_RADIOTAP_VERSION     0

struct tsf {
    uint32 tsf_l;
    uint32 tsf_h;
} __attribute__((packed));

struct xchan {
    uint32 xchannel_flags;
    uint16 xchannel_freq;
    uint8 xchannel_channel;
    uint8 xchannel_maxpower;
} __attribute__((packed));

/* A generic radio capture format is desirable. There is one for
 * Linux, but it is neither rigidly defined (there were not even
 * units given for some fields) nor easily extensible.
 *
 * I suggest the following extensible radio capture format. It is
 * based on a bitmap indicating which fields are present.
 *
 * I am trying to describe precisely what the application programmer
 * should expect in the following, and for that reason I tell the
 * units and origin of each measurement (where it applies), or else I
 * use sufficiently weaselly language ("is a monotonically nondecreasing
 * function of...") that I cannot set false expectations for lawyerly
 * readers.
 */

/*
 * The radio capture header precedes the 802.11 header.
 * All data in the header is little endian on all platforms.
 */
struct ieee80211_radiotap_header {
    uint8 it_version;      /* Version 0. Only increases
                 * for drastic changes,
                 * introduction of compatible
                 * new fields does not count.
                 */
    uint8 it_pad;
    uint16 it_len;      /* length of the whole
                 * header in bytes, including
                 * it_version, it_pad,
                 * it_len, and data fields.
                 */
    uint32 it_present;  /* A bitmap telling which
                 * fields are present. Set bit 31
                 * (0x80000000) to extend the
                 * bitmap by another 32 bits.
                 * Additional extensions are made
                 * by setting bit 31.
                 */
} __packed;

/* Name                                 Data type    Units
 * ----                                 ---------    -----
 *
 * IEEE80211_RADIOTAP_TSFT              __le64       microseconds
 *
 *      Value in microseconds of the MAC's 64-bit 802.11 Time
 *      Synchronization Function timer when the first bit of the
 *      MPDU arrived at the MAC. For received frames, only.
 *
 * IEEE80211_RADIOTAP_CHANNEL           2 x __le16   MHz, bitmap
 *
 *      Tx/Rx frequency in MHz, followed by flags (see below).
 *
 * IEEE80211_RADIOTAP_FHSS              __le16       see below
 *
 *      For frequency-hopping radios, the hop set (first byte)
 *      and pattern (second byte).
 *
 * IEEE80211_RADIOTAP_RATE              u8           500kb/s
 *
 *      Tx/Rx data rate
 *
 * IEEE80211_RADIOTAP_DBM_ANTSIGNAL     s8           decibels from
 *                                                   one milliwatt (dBm)
 *
 *      RF signal power at the antenna, decibel difference from
 *      one milliwatt.
 *
 * IEEE80211_RADIOTAP_DBM_ANTNOISE      s8           decibels from
 *                                                   one milliwatt (dBm)
 *
 *      RF noise power at the antenna, decibel difference from one
 *      milliwatt.
 *
 * IEEE80211_RADIOTAP_DB_ANTSIGNAL      u8           decibel (dB)
 *
 *      RF signal power at the antenna, decibel difference from an
 *      arbitrary, fixed reference.
 *
 * IEEE80211_RADIOTAP_DB_ANTNOISE       u8           decibel (dB)
 *
 *      RF noise power at the antenna, decibel difference from an
 *      arbitrary, fixed reference point.
 *
 * IEEE80211_RADIOTAP_LOCK_QUALITY      __le16       unitless
 *
 *      Quality of Barker code lock. Unitless. Monotonically
 *      nondecreasing with "better" lock strength. Called "Signal
 *      Quality" in datasheets.  (Is there a standard way to measure
 *      this?)
 *
 * IEEE80211_RADIOTAP_TX_ATTENUATION    __le16       unitless
 *
 *      Transmit power expressed as unitless distance from max
 *      power set at factory calibration.  0 is max power.
 *      Monotonically nondecreasing with lower power levels.
 *
 * IEEE80211_RADIOTAP_DB_TX_ATTENUATION __le16       decibels (dB)
 *
 *      Transmit power expressed as decibel distance from max power
 *      set at factory calibration.  0 is max power.  Monotonically
 *      nondecreasing with lower power levels.
 *
 * IEEE80211_RADIOTAP_DBM_TX_POWER      s8           decibels from
 *                                                   one milliwatt (dBm)
 *
 *      Transmit power expressed as dBm (decibels from a 1 milliwatt
 *      reference). This is the absolute power level measured at
 *      the antenna port.
 *
 * IEEE80211_RADIOTAP_FLAGS             u8           bitmap
 *
 *      Properties of transmitted and received frames. See flags
 *      defined below.
 *
 * IEEE80211_RADIOTAP_ANTENNA           u8           antenna index
 *
 *      Unitless indication of the Rx/Tx antenna for this packet.
 *      The first antenna is antenna 0.
 *
 * IEEE80211_RADIOTAP_RX_FLAGS          __le16       bitmap
 *
 *     Properties of received frames. See flags defined below.
 *
 * IEEE80211_RADIOTAP_TX_FLAGS          __le16       bitmap
 *
 *     Properties of transmitted frames. See flags defined below.
 *
 * IEEE80211_RADIOTAP_RTS_RETRIES       u8           data
 *
 *     Number of rts retries a transmitted frame used.
 *
 * IEEE80211_RADIOTAP_DATA_RETRIES      u8           data
 *
 *     Number of unicast retries a transmitted frame used.
 *
 * IEEE80211_RADIOTAP_XCHANNEL          uint32_t        bitmap
 *                                      uint16_t        MHz
 *                                      uint8_t         channel number
 *                                      int8_t          .5 dBm
 *
 *      Extended channel specification: flags (see below) followed by
 *      frequency in MHz, the corresponding IEEE channel number, and
 *      finally the maximum regulatory transmit power cap in .5 dBm
 *      units.  This property supersedes IEEE80211_RADIOTAP_CHANNEL
 *      and only one of the two should be present.
 *
 * IEEE80211_RADIOTAP_MCS               u8, u8, u8      unitless
 *
 *     Contains a bitmap of known fields/flags, the flags, and
 *     the MCS index.
 *
 * IEEE80211_RADIOTAP_AMPDU_STATUS      u32, u16, u8, u8    unitless
 *
 *  Contains the AMPDU information for the subframe.
 *
 * IEEE80211_RADIOTAP_VHT               u16, u8, u8, u8[4], u8, u8, u16
 *
 *  Contains VHT information about this frame.
 *
 * IEEE80211_RADIOTAP_TIMESTAMP     u64, u16, u8, u8    variable
 *
 *  Contains timestamp information for this frame.
 */
enum ieee80211_radiotap_type {
    IEEE80211_RADIOTAP_TSFT = 0,
    IEEE80211_RADIOTAP_FLAGS = 1,
    IEEE80211_RADIOTAP_RATE = 2,
    IEEE80211_RADIOTAP_CHANNEL = 3,
    IEEE80211_RADIOTAP_FHSS = 4,
    IEEE80211_RADIOTAP_DBM_ANTSIGNAL = 5,
    IEEE80211_RADIOTAP_DBM_ANTNOISE = 6,
    IEEE80211_RADIOTAP_LOCK_QUALITY = 7,
    IEEE80211_RADIOTAP_TX_ATTENUATION = 8,
    IEEE80211_RADIOTAP_DB_TX_ATTENUATION = 9,
    IEEE80211_RADIOTAP_DBM_TX_POWER = 10,
    IEEE80211_RADIOTAP_ANTENNA = 11,
    IEEE80211_RADIOTAP_DB_ANTSIGNAL = 12,
    IEEE80211_RADIOTAP_DB_ANTNOISE = 13,
    IEEE80211_RADIOTAP_RX_FLAGS = 14,
    IEEE80211_RADIOTAP_TX_FLAGS = 15,
    IEEE80211_RADIOTAP_RTS_RETRIES = 16,
    IEEE80211_RADIOTAP_DATA_RETRIES = 17,
    IEEE80211_RADIOTAP_XCHANNEL = 18,    
    IEEE80211_RADIOTAP_MCS = 19,
    IEEE80211_RADIOTAP_AMPDU_STATUS = 20,
    IEEE80211_RADIOTAP_VHT = 21,
    IEEE80211_RADIOTAP_TIMESTAMP = 22,
    IEEE80211_RADIOTAP_HE = 23,

    /* valid in every it_present bitmap, even vendor namespaces */
    IEEE80211_RADIOTAP_RADIOTAP_NAMESPACE = 29,
    IEEE80211_RADIOTAP_VENDOR_NAMESPACE = 30,
    IEEE80211_RADIOTAP_EXT = 31
};

/* Channel flags. */
#define IEEE80211_CHAN_PRIV0    0x00000001 /* driver private bit 0 */
#define IEEE80211_CHAN_PRIV1    0x00000002 /* driver private bit 1 */
#define IEEE80211_CHAN_PRIV2    0x00000004 /* driver private bit 2 */
#define IEEE80211_CHAN_PRIV3    0x00000008 /* driver private bit 3 */
#define IEEE80211_CHAN_TURBO    0x00000010 /* Turbo channel */
#define IEEE80211_CHAN_CCK  0x00000020 /* CCK channel */
#define IEEE80211_CHAN_OFDM 0x00000040 /* OFDM channel */
#define IEEE80211_CHAN_2GHZ 0x00000080 /* 2 GHz spectrum channel. */
#define IEEE80211_CHAN_5GHZ 0x00000100 /* 5 GHz spectrum channel */
#define IEEE80211_CHAN_PASSIVE  0x00000200 /* Only passive scan allowed */
#define IEEE80211_CHAN_DYN  0x00000400 /* Dynamic CCK-OFDM channel */
#define IEEE80211_CHAN_GFSK 0x00000800 /* GFSK channel (FHSS PHY) */
#define IEEE80211_CHAN_GSM  0x00001000 /* 900 MHz spectrum channel */
#define IEEE80211_CHAN_STURBO   0x00002000 /* 11a static turbo channel only */
#define IEEE80211_CHAN_HALF 0x00004000 /* Half rate channel */
#define IEEE80211_CHAN_QUARTER  0x00008000 /* Quarter rate channel */
#define IEEE80211_CHAN_HT20 0x00010000 /* HT 20 channel */
#define IEEE80211_CHAN_HT40U    0x00020000 /* HT 40 channel w/ ext above */
#define IEEE80211_CHAN_HT40D    0x00040000 /* HT 40 channel w/ ext below */
#define IEEE80211_CHAN_DFS  0x00080000 /* DFS required */
#define IEEE80211_CHAN_4MSXMIT  0x00100000 /* 4ms limit on frame length */
#define IEEE80211_CHAN_NOADHOC  0x00200000 /* adhoc mode not allowed */
#define IEEE80211_CHAN_NOHOSTAP 0x00400000 /* hostap mode not allowed */
#define IEEE80211_CHAN_11D  0x00800000 /* 802.11d required */
#define IEEE80211_CHAN_VHT20    0x01000000 /* VHT20 channel */
#define IEEE80211_CHAN_VHT40U   0x02000000 /* VHT40 channel, ext above */
#define IEEE80211_CHAN_VHT40D   0x04000000 /* VHT40 channel, ext below */
#define IEEE80211_CHAN_VHT80    0x08000000 /* VHT80 channel */
#define IEEE80211_CHAN_VHT80_80 0x10000000 /* VHT80+80 channel */
#define IEEE80211_CHAN_VHT160   0x20000000 /* VHT160 channel */

/* For IEEE80211_RADIOTAP_FLAGS */
#define IEEE80211_RADIOTAP_F_CFP    0x01    /* sent/received
                         * during CFP
                         */
#define IEEE80211_RADIOTAP_F_SHORTPRE   0x02    /* sent/received
                         * with short
                         * preamble
                         */
#define IEEE80211_RADIOTAP_F_WEP    0x04    /* sent/received
                         * with WEP encryption
                         */
#define IEEE80211_RADIOTAP_F_FRAG   0x08    /* sent/received
                         * with fragmentation
                         */
#define IEEE80211_RADIOTAP_F_FCS    0x10    /* frame includes FCS */
#define IEEE80211_RADIOTAP_F_DATAPAD    0x20    /* frame has padding between
                         * 802.11 header and payload
                         * (to 32-bit boundary)
                         */
#define IEEE80211_RADIOTAP_F_BADFCS 0x40    /* bad FCS */

/* For IEEE80211_RADIOTAP_RX_FLAGS */
#define IEEE80211_RADIOTAP_F_RX_BADPLCP 0x0002  /* frame has bad PLCP */

/* For IEEE80211_RADIOTAP_TX_FLAGS */
#define IEEE80211_RADIOTAP_F_TX_FAIL    0x0001  /* failed due to excessive
                         * retries */
#define IEEE80211_RADIOTAP_F_TX_CTS 0x0002  /* used cts 'protection' */
#define IEEE80211_RADIOTAP_F_TX_RTS 0x0004  /* used rts/cts handshake */
#define IEEE80211_RADIOTAP_F_TX_NOACK   0x0008  /* don't expect an ack */


/* For IEEE80211_RADIOTAP_MCS */
#define IEEE80211_RADIOTAP_MCS_HAVE_BW      0x01
#define IEEE80211_RADIOTAP_MCS_HAVE_MCS     0x02
#define IEEE80211_RADIOTAP_MCS_HAVE_GI      0x04
#define IEEE80211_RADIOTAP_MCS_HAVE_FMT     0x08
#define IEEE80211_RADIOTAP_MCS_HAVE_FEC     0x10
#define IEEE80211_RADIOTAP_MCS_HAVE_STBC    0x20

#define IEEE80211_RADIOTAP_MCS_BW_MASK      0x03
#define     IEEE80211_RADIOTAP_MCS_BW_20    0
#define     IEEE80211_RADIOTAP_MCS_BW_40    1
#define     IEEE80211_RADIOTAP_MCS_BW_20L   2
#define     IEEE80211_RADIOTAP_MCS_BW_20U   3
#define IEEE80211_RADIOTAP_MCS_SGI      0x04
#define IEEE80211_RADIOTAP_MCS_FMT_GF       0x08
#define IEEE80211_RADIOTAP_MCS_FEC_LDPC     0x10
#define IEEE80211_RADIOTAP_MCS_STBC_MASK    0x60
#define     IEEE80211_RADIOTAP_MCS_STBC_1   1
#define     IEEE80211_RADIOTAP_MCS_STBC_2   2
#define     IEEE80211_RADIOTAP_MCS_STBC_3   3

#define IEEE80211_RADIOTAP_MCS_STBC_SHIFT   5

/* For IEEE80211_RADIOTAP_AMPDU_STATUS */
#define IEEE80211_RADIOTAP_AMPDU_REPORT_ZEROLEN     0x0001
#define IEEE80211_RADIOTAP_AMPDU_IS_ZEROLEN     0x0002
#define IEEE80211_RADIOTAP_AMPDU_LAST_KNOWN     0x0004
#define IEEE80211_RADIOTAP_AMPDU_IS_LAST        0x0008
#define IEEE80211_RADIOTAP_AMPDU_DELIM_CRC_ERR      0x0010
#define IEEE80211_RADIOTAP_AMPDU_DELIM_CRC_KNOWN    0x0020

/* For IEEE80211_RADIOTAP_VHT */
#define IEEE80211_RADIOTAP_VHT_KNOWN_STBC           0x0001
#define IEEE80211_RADIOTAP_VHT_KNOWN_TXOP_PS_NA         0x0002
#define IEEE80211_RADIOTAP_VHT_KNOWN_GI             0x0004
#define IEEE80211_RADIOTAP_VHT_KNOWN_SGI_NSYM_DIS       0x0008
#define IEEE80211_RADIOTAP_VHT_KNOWN_LDPC_EXTRA_OFDM_SYM    0x0010
#define IEEE80211_RADIOTAP_VHT_KNOWN_BEAMFORMED         0x0020
#define IEEE80211_RADIOTAP_VHT_KNOWN_BANDWIDTH          0x0040
#define IEEE80211_RADIOTAP_VHT_KNOWN_GROUP_ID           0x0080
#define IEEE80211_RADIOTAP_VHT_KNOWN_PARTIAL_AID        0x0100

#define IEEE80211_RADIOTAP_VHT_FLAG_STBC            0x01
#define IEEE80211_RADIOTAP_VHT_FLAG_TXOP_PS_NA          0x02
#define IEEE80211_RADIOTAP_VHT_FLAG_SGI             0x04
#define IEEE80211_RADIOTAP_VHT_FLAG_SGI_NSYM_M10_9      0x08
#define IEEE80211_RADIOTAP_VHT_FLAG_LDPC_EXTRA_OFDM_SYM     0x10
#define IEEE80211_RADIOTAP_VHT_FLAG_BEAMFORMED          0x20

#define IEEE80211_RADIOTAP_CODING_LDPC_USER0            0x01
#define IEEE80211_RADIOTAP_CODING_LDPC_USER1            0x02
#define IEEE80211_RADIOTAP_CODING_LDPC_USER2            0x04
#define IEEE80211_RADIOTAP_CODING_LDPC_USER3            0x08

/* For IEEE80211_RADIOTAP_TIMESTAMP */
#define IEEE80211_RADIOTAP_TIMESTAMP_UNIT_MASK          0x000F
#define IEEE80211_RADIOTAP_TIMESTAMP_UNIT_MS            0x0000
#define IEEE80211_RADIOTAP_TIMESTAMP_UNIT_US            0x0001
#define IEEE80211_RADIOTAP_TIMESTAMP_UNIT_NS            0x0003
#define IEEE80211_RADIOTAP_TIMESTAMP_SPOS_MASK          0x00F0
#define IEEE80211_RADIOTAP_TIMESTAMP_SPOS_BEGIN_MDPU        0x0000
#define IEEE80211_RADIOTAP_TIMESTAMP_SPOS_PLCP_SIG_ACQ      0x0010
#define IEEE80211_RADIOTAP_TIMESTAMP_SPOS_EO_PPDU       0x0020
#define IEEE80211_RADIOTAP_TIMESTAMP_SPOS_EO_MPDU       0x0030
#define IEEE80211_RADIOTAP_TIMESTAMP_SPOS_UNKNOWN       0x00F0

#define IEEE80211_RADIOTAP_TIMESTAMP_FLAG_64BIT         0x00
#define IEEE80211_RADIOTAP_TIMESTAMP_FLAG_32BIT         0x01
#define IEEE80211_RADIOTAP_TIMESTAMP_FLAG_ACCURACY      0x02

/*
 * Radiotap parsing functions -- for controlled injection support
 *
 * Implemented in net/wireless/radiotap.c
 * Documentation in Documentation/networking/radiotap-headers.txt
 */

struct radiotap_align_size {
    uint8_t align:4, size:4;
};

struct ieee80211_radiotap_namespace {
    const struct radiotap_align_size *align_size;
    int n_bits;
    uint32_t oui;
    uint8_t subns;
};

struct ieee80211_radiotap_vendor_namespaces {
    const struct ieee80211_radiotap_namespace *ns;
    int n_ns;
};

/**
 * struct ieee80211_radiotap_iterator - tracks walk thru present radiotap args
 * @this_arg_index: index of current arg, valid after each successful call
 *  to ieee80211_radiotap_iterator_next()
 * @this_arg: pointer to current radiotap arg; it is valid after each
 *  call to ieee80211_radiotap_iterator_next() but also after
 *  ieee80211_radiotap_iterator_init() where it will point to
 *  the beginning of the actual data portion
 * @this_arg_size: length of the current arg, for convenience
 * @current_namespace: pointer to the current namespace definition
 *  (or internally %NULL if the current namespace is unknown)
 * @is_radiotap_ns: indicates whether the current namespace is the default
 *  radiotap namespace or not
 *
 * @_rtheader: pointer to the radiotap header we are walking through
 * @_max_length: length of radiotap header in cpu byte ordering
 * @_arg_index: next argument index
 * @_arg: next argument pointer
 * @_next_bitmap: internal pointer to next present u32
 * @_bitmap_shifter: internal shifter for curr u32 bitmap, b0 set == arg present
 * @_vns: vendor namespace definitions
 * @_next_ns_data: beginning of the next namespace's data
 * @_reset_on_ext: internal; reset the arg index to 0 when going to the
 *  next bitmap word
 *
 * Describes the radiotap parser state. Fields prefixed with an underscore
 * must not be used by users of the parser, only by the parser internally.
 */

struct ieee80211_radiotap_iterator {
    struct ieee80211_radiotap_header *_rtheader;
    const struct ieee80211_radiotap_vendor_namespaces *_vns;
    const struct ieee80211_radiotap_namespace *current_namespace;

    unsigned char *_arg, *_next_ns_data;
    unsigned int *_next_bitmap;

    unsigned char *this_arg;
    int this_arg_index;
    int this_arg_size;

    int is_radiotap_ns;

    int _max_length;
    int _arg_index;
    uint32_t _bitmap_shifter;
    int _reset_on_ext;
};

struct nexmon_radiotap_header {
    struct ieee80211_radiotap_header header;
    struct tsf tsf;
    char flags;
    unsigned char data_rate;
    unsigned short chan_freq;
    unsigned short chan_flags;
    char dbm_antsignal;
    char dbm_antnoise;
#ifdef RADIOTAP_MCS
    char mcs[3];
    char PAD;
#endif
#ifdef RADIOTAP_VHT
    unsigned short vht_known;
    unsigned char vht_flags;
    unsigned char vht_bandwidth;
    unsigned char vht_mcs_nss[4];
    unsigned char vht_coding;
    unsigned char vht_group_id;
    unsigned short vht_partial_aid;
#endif
#ifdef RADIOTAP_VENDOR
    unsigned char vendor_oui[3];
    unsigned char vendor_sub_namespace;
    unsigned short vendor_skip_length;
#endif
} __attribute__((packed));

extern int
ieee80211_radiotap_iterator_init(struct ieee80211_radiotap_iterator *iterator,
                 struct ieee80211_radiotap_header *radiotap_header,
                 int max_length,
                 const struct ieee80211_radiotap_vendor_namespaces *vns);

extern int
ieee80211_radiotap_iterator_next(struct ieee80211_radiotap_iterator *iterator);

#endif              /* IEEE80211_RADIOTAP_H */
