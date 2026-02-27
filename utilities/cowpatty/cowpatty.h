/*
 * coWPAtty - Brute-force dictionary attack against WPA-PSK.
 *
 * Copyright (c) 2004-2005, Joshua Wright <jwright@hasborg.com>
 *
 * $Id: cowpatty.h,v 4.3 2008-11-12 14:22:27 jwright Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. See COPYING for more
 * details.
 *
 * coWPAtty is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*
 * Significant code is graciously taken from the following:
 * wpa_supplicant by Jouni Malinen.  This tool would have been MUCH more
 * difficult for me if not for this code.  Thanks Jouni.
 */

#include "common.h"

/* IEEE 802.11 frame information */
#define DOT11HDR_A3_LEN 24
#define DOT11_FC_TYPE_MGMT 0
#define DOT11_FC_TYPE_CTRL 1
#define DOT11_FC_TYPE_DATA 2

#define DOT11_FC_SUBTYPE_DATA            0
#define DOT11_FC_SUBTYPE_DATACFACK       1
#define DOT11_FC_SUBTYPE_DATACFPOLL      2
#define DOT11_FC_SUBTYPE_DATACFACKPOLL   3
#define DOT11_FC_SUBTYPE_DATANULL        4
#define DOT11_FC_SUBTYPE_CFACK           5
#define DOT11_FC_SUBTYPE_CFACKPOLL       6
#define DOT11_FC_SUBTYPE_CFACKPOLLNODATA 7
#define DOT11_FC_SUBTYPE_QOSDATA         8
/* 9 - 11 reserved as of 11/7/2005 - JWRIGHT */
#define DOT11_FC_SUBTYPE_QOSNULL         12

struct dot11hdr {
	union {
		struct {
			uint8_t		version:2;
			uint8_t		type:2;
			uint8_t		subtype:4;
			uint8_t		to_ds:1;
			uint8_t		from_ds:1;
			uint8_t		more_frag:1;
			uint8_t		retry:1;
			uint8_t		pwrmgmt:1;
			uint8_t		more_data:1;
			uint8_t		protected:1;
			uint8_t		order:1;
		} __attribute__ ((packed)) fc;

		uint16_t	fchdr;
	} u1;

	uint16_t	duration;
	uint8_t		addr1[6];
	uint8_t		addr2[6];
	uint8_t		addr3[6];

	union {
		struct {
			uint16_t	fragment:4;
			uint16_t	sequence:12;
		} __attribute__ ((packed)) seq;

		uint16_t	seqhdr;
	} u2;

} __attribute__ ((packed));


/* IEEE 802.1X frame information */

struct ieee802_1x_hdr {
	u8 version;
	u8 type;
	u16 length;
	/* followed by length octets of data */
} __attribute__ ((packed));

/* The 802.1x header indicates a version, type and length */
struct ieee8021x {
	u8 version;
	u8 type;
	u16 length;
} __attribute__ ((packed));

#define MAXPASSLEN 64
#define MEMORY_DICT 0
#define STDIN_DICT 1
#define EAPDOT1XOFFSET 4
#define BIT(n) (1 << (n))
#define WPA_KEY_INFO_TYPE_MASK (BIT(0) | BIT(1) | BIT(2))
#define WPA_KEY_INFO_TYPE_HMAC_MD5_RC4 BIT(0)
#define WPA_KEY_INFO_TYPE_HMAC_SHA1_AES BIT(1)
#define WPA_KEY_INFO_KEY_TYPE BIT(3)	/* 1 = Pairwise, 0 = Group key */
/* bit4..5 is used in WPA, but is reserved in IEEE 802.11i/RSN */
#define WPA_KEY_INFO_KEY_INDEX_MASK (BIT(4) | BIT(5))
#define WPA_KEY_INFO_KEY_INDEX_SHIFT 4
#define WPA_KEY_INFO_INSTALL BIT(6)	/* pairwise */
#define WPA_KEY_INFO_TXRX BIT(6)	/* group */
#define WPA_KEY_INFO_ACK BIT(7)
#define WPA_KEY_INFO_MIC BIT(8)
#define WPA_KEY_INFO_SECURE BIT(9)
#define WPA_KEY_INFO_ERROR BIT(10)
#define WPA_KEY_INFO_REQUEST BIT(11)
#define WPA_KEY_INFO_ENCR_KEY_DATA BIT(12)	/* IEEE 802.11i/RSN only */
#define WPA_NONCE_LEN 32
#define WPA_REPLAY_COUNTER_LEN 8

struct wpa_eapol_key {
	u8 type;
	u16 key_info;
	u16 key_length;
	u8 replay_counter[WPA_REPLAY_COUNTER_LEN];
	u8 key_nonce[WPA_NONCE_LEN];
	u8 key_iv[16];
	u8 key_rsc[8];
	u8 key_id[8];		/* Reserved in IEEE 802.11i/RSN */
	u8 key_mic[16];
	u16 key_data_length;
/*    u8 key_data[0]; */
} __attribute__ ((packed));

struct wpa_ptk {
	u8 mic_key[16];		/* EAPOL-Key MIC Key (MK) */
	u8 encr_key[16];	/* EAPOL-Key Encryption Key (EK) */
	u8 tk1[16];		/* Temporal Key 1 (TK1) */
	union {
		u8 tk2[16];	/* Temporal Key 2 (TK2) */
		struct {
			u8 tx_mic_key[8];
			u8 rx_mic_key[8];
		} auth;
	} u;
} __attribute__ ((packed));

struct user_opt {
	char ssid[256];
	char dictfile[256];
	char pcapfile[256];
	char hashfile[256];
	u8 nonstrict;
    u8 checkonly;
	u8 verbose;
    u8 unused;
};

struct capture_data {
	char pcapfilename[256];
	int pcaptype;
	int dot1x_offset;
	int l2type_offset;
	int dstmac_offset;
	int srcmac_offset;
};

struct crack_data {
	u8 aa[6];
	u8 spa[6];
	u8 snonce[32];
	u8 anonce[32];
	u8 eapolframe[99];
	u8 eapolframe2[125];
	u8 keymic[16];
	u8 aaset;
	u8 spaset;
	u8 snonceset;
	u8 anonceset;
	u8 keymicset;
	u8 eapolframeset;
	u8 replay_counter[8];

	int ver; /* Hashing algo, MD5 or AES-CBC-MAC */
	int eapolframe_size;
};

struct hashdb_head {
	uint32_t magic;
	uint8_t reserved1[3];
	uint8_t ssidlen;
	uint8_t ssid[32];
};

struct hashdb_rec {
	uint8_t rec_size;
	char *word;
	uint8_t pmk[32];
} __attribute__ ((packed));
