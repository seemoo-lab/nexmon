#ifndef HAVE_PACKET_H
#define HAVE_PACKET_H

#include <inttypes.h>

#include "osdep/byteorder.h"
#include "mac_addr.h"

#define MANAGMENT_FRAME 0x00
#define CONTROL_FRAME   0x04
#define DATA_FRAME      0x08
#define EXTENSION_FRAME 0x0C

#define IEEE80211_TYPE_ASSOCREQ				0x00            // association request
#define IEEE80211_TYPE_ASSOCRES				0x10            // association response
#define IEEE80211_TYPE_REASSOCREQ			0x20			// reassociation request
#define IEEE80211_TYPE_REASSOCRES			0x30			// reassociation response
#define IEEE80211_TYPE_PROBEREQ				0x40			// probe request
#define IEEE80211_TYPE_PROBERES				0x50			// probe response
#define IEEE80211_TYPE_TIMADVERT			0x60			// timing advertisement
#define IEEE80211_TYPE_000111				0x70			// reserved
#define IEEE80211_TYPE_BEACON				0x80			// beacon
#define IEEE80211_TYPE_ATIM					0x90			// ATIM
#define IEEE80211_TYPE_DISASSOC				0xA0			// disassociation
#define IEEE80211_TYPE_AUTH					0xB0			// authentication
#define IEEE80211_TYPE_DEAUTH				0xC0			// deauthentication
#define IEEE80211_TYPE_ACTION				0xD0			// action
#define IEEE80211_TYPE_ACTIONNOACK   		0xE0			// action no ack
#define IEEE80211_TYPE_001111				0xF0			// reserved

#define IEEE80211_TYPE_010000				0x04			// reserved
#define IEEE80211_TYPE_010001				0x14			// reserved
#define IEEE80211_TYPE_010010				0x24			// reserved
#define IEEE80211_TYPE_010011				0x34			// reserved
#define IEEE80211_TYPE_BEAMFORMING			0x44			// beamforming report poll
#define IEEE80211_TYPE_VHT					0x54			// vht ndp announcement
#define IEEE80211_TYPE_CTRLFRMEXT			0x64			// control frame extension
#define IEEE80211_TYPE_CTRLWRAP			    0x74			// control wrapper
#define IEEE80211_TYPE_BLOCKACKREQ			0x84			// block ack request
#define IEEE80211_TYPE_BLOCKACK 			0x94			// block ack
#define IEEE80211_TYPE_PSPOLL				0xA4			// ps-poll
#define IEEE80211_TYPE_RTS					0xB4			// rts
#define IEEE80211_TYPE_CTS					0xC4			// cts
#define IEEE80211_TYPE_ACK 					0xD4			// ack
#define IEEE80211_TYPE_CFEND				0xE4			// cf-end
#define IEEE80211_TYPE_CFENDACK				0xF4			// cf-end + cf-ack

#define IEEE80211_TYPE_DATA					0x08 			// data
#define IEEE80211_TYPE_DATACFACK			0x18			// data + cf-ack
#define IEEE80211_TYPE_DATACFPOLL			0x28			// data + cf-poll
#define IEEE80211_TYPE_DATACFACKPOLL		0x38			// data + cf-ack + cf-poll
#define IEEE80211_TYPE_NULL					0x48 			// null func
#define IEEE80211_TYPE_CFACK  				0x58			// cf-ack
#define IEEE80211_TYPE_CFPOLL   			0x68			// cf-poll
#define IEEE80211_TYPE_CFACKPOLL 			0x78 			// cf-ack + cf-poll
#define IEEE80211_TYPE_QOSDATA				0x88			// qos data
#define IEEE80211_TYPE_QOSDATACFACK			0x98			// qos data + cf-ack
#define IEEE80211_TYPE_QOSDATACFPOLL		0xA8			// qos data + cf-poll
#define IEEE80211_TYPE_QOSDATACFACKPOLL		0xB8			// qos data + cf-ack + cf-poll
#define IEEE80211_TYPE_QOSNULL				0xC8            // qos null func
#define IEEE80211_TYPE_QOSCFACK				0xD8			// qos cf-ack
#define IEEE80211_TYPE_QOSCFPOLL 			0xE8			// qos cf-poll
#define IEEE80211_TYPE_QOSCFACKPOLL 		0xF8			// qos cf-ack + cf-poll

#define IEEE80211_TYPE_DMGBEACON			0x0C            // DMG beacon
#define IEEE80211_TYPE_110001				0x1C            // reserved
#define IEEE80211_TYPE_110010				0x2C            // reserved
#define IEEE80211_TYPE_110011				0x3C            // reserved
#define IEEE80211_TYPE_110100				0x4C            // reserved
#define IEEE80211_TYPE_110101				0x5C            // reserved
#define IEEE80211_TYPE_110110				0x6C            // reserved
#define IEEE80211_TYPE_110111				0x7C            // reserved
#define IEEE80211_TYPE_111000				0x8C            // reserved
#define IEEE80211_TYPE_111001				0x9C            // reserved
#define IEEE80211_TYPE_111010				0xAC            // reserved
#define IEEE80211_TYPE_111011				0xBC            // reserved
#define IEEE80211_TYPE_111100				0xCC            // reserved
#define IEEE80211_TYPE_111101				0xDC            // reserved
#define IEEE80211_TYPE_111110				0xEC            // reserved
#define IEEE80211_TYPE_111111				0xFC            // reserved

#define DEFAULT_BEACON_INTERVAL	0x64
#define DEFAULT_11B_RATES	"\x01\x04\x82\x84\x8b\x96"
#define DEFAULT_11G_RATES	"\x32\x08\x0c\x12\x18\x24\x30\x48\x60\x6c"
#define DEFAULT_WPA_TKIP_TAG	"\xDD\x18\x00\x50\xF2\x01\x01\x00\x00\x50\xF2\x02\x01\x00\x00\x50\xF2\x02\x01\x00\x00\x50\xF2\x02\x00\x00"
#define DEFAULT_WPA_AES_TAG	"\xDD\x18\x00\x50\xF2\x01\x01\x00\x00\x50\xF2\x04\x01\x00\x00\x50\xF2\x04\x01\x00\x00\x50\xF2\x02\x00\x00"

#define AUTH_ALGORITHM_OPEN	0x0000
#define AUTH_STATUS_SUCCESS	0x0000
#define AUTH_DEFAULT_DURATION	314

#define DEAUTH_REASON_UNSPEC	0x0001
#define DEAUTH_REASON_LEAVING	0x0003
#define DISASSOC_REASON_APFULL	0x0005
#define DISASSOC_REASON_LEAVING	0x0008

#define DEFAULT_LISTEN_INTERVAL	0x0001

#define BEACON_TAGTYPE_SSID	0x00
#define BEACON_TAGTYPE_MESHID   0x72

#define LLC_SNAP		0xAA
#define LLC_UNNUMBERED		0x03

#define RSN_TYPE_KEY		0x03
#define RSN_DESCRIPTOR_KEY	0x02

#define MESH_ACTION_CATEGORY	0x0D
#define MESH_ACTION_PATHSEL	0x01
#define MESH_TAG_PREQ		0x82
#define MESH_TAG_PREP		0x83

#define MAX_PACKET_SIZE 2048

struct packet {
  unsigned char data[MAX_PACKET_SIZE];
  unsigned int len;
};

struct ieee_hdr {
  uint8_t type;
  uint8_t flags;
  uint16_t duration;
  struct ether_addr addr1;
  struct ether_addr addr2;
  struct ether_addr addr3;
  uint16_t frag_seq;
} __attribute__((packed));

struct beacon_fixed {
  uint64_t timestamp;
  uint16_t interval;
  uint16_t capabilities;
} __attribute__((packed));

struct auth_fixed {
  uint16_t algorithm;
  uint16_t seq;
  uint16_t status;
} __attribute__((packed));

struct assoc_fixed {
  uint16_t capabilities;
  uint16_t interval;
} __attribute__((packed));

struct llc_header {
  uint8_t dsap;
  uint8_t ssap;
  uint8_t control;
  uint8_t encap[3];
  uint16_t type;
} __attribute__((packed));

struct rsn_auth {
  uint8_t version;
  uint8_t type;
  uint16_t length;
  uint8_t descriptor;
  uint16_t key_info;
  uint16_t key_length;
  uint64_t replay_counter;
  uint8_t nonce[32];
  uint8_t key_iv[16];
  uint64_t key_rsc;
  uint64_t key_id;
  uint8_t key_mic[16];
  uint16_t wpa_length;
} __attribute__((packed));

struct action_fixed {
  uint8_t category;
  uint8_t action_code;
  uint8_t tag;
  uint8_t taglen;
} __attribute__((packed));

struct mesh_preq {
  uint8_t flags;
  uint8_t hop_count;
  uint8_t ttl;
  uint32_t discovery_id;
  struct ether_addr originator;
  uint32_t orig_seq;
  uint32_t lifetime;
  uint32_t metric;
  uint8_t target_count;
  uint8_t target_flags;
  struct ether_addr target;
  uint32_t target_seq;
} __attribute__((packed));

struct mesh_prep {
  uint8_t flags;
  uint8_t hop_count;
  uint8_t ttl;
  struct ether_addr target;
  uint32_t target_seq;
  uint32_t lifetime;
  uint32_t metric;
  struct ether_addr originator;
  uint32_t orig_seq;
} __attribute__((packed));

struct cts {
  uint8_t type;
  uint8_t flags;
  uint16_t duration;
  struct ether_addr dest;
} __attribute__((packed));

//dsflags: 'a' = AdHoc, Beacon   'f' = From DS   't' = To DS   'w' = WDS (intra DS)
//Set recv to SE_NULLMAC if you don't create WDS packets. (its ignored anyway)
void create_ieee_hdr(struct packet *pkt, uint8_t type, char dsflags, uint16_t duration, struct ether_addr destination, struct ether_addr source, struct ether_addr bssid_or_transm, struct ether_addr recv, uint8_t fragment);

struct ether_addr *get_bssid(struct packet *pkt);

struct ether_addr *get_source(struct packet *pkt);

struct ether_addr *get_destination(struct packet *pkt);

struct ether_addr *get_transmitter(struct packet *pkt);

struct ether_addr *get_receiver(struct packet *pkt);

//encryption: 'n' = None   'w' = WEP   't' = TKIP (WPA)   'a' = AES (WPA2)
//If bitrate is 54, you'll get an bg network, b only otherwise
struct packet create_beacon(struct ether_addr bssid, char *ssid, uint8_t channel, char encryption, unsigned char bitrate, char adhoc);

struct packet create_auth(struct ether_addr bssid, struct ether_addr client, uint16_t seq);

struct packet create_probe(struct ether_addr source, char *ssid, unsigned char bitrate);

struct packet create_deauth(struct ether_addr destination, struct ether_addr source, struct ether_addr bssid);

struct packet create_disassoc(struct ether_addr destination, struct ether_addr source, struct ether_addr bssid);

//Capabilities and SSID should match AP, so just copy them from one of its beacon frames
struct packet create_assoc_req(struct ether_addr client, struct ether_addr bssid, uint16_t capabilities, char *ssid, unsigned char bitrate);

struct packet create_cts(struct ether_addr destination, uint16_t duration);

//Copy SSID or MeshID from Beacon Frame into String. Must free afterwards! Returns NULL on Errors (no beacon frame, no SSID tag found)
//SSID len is also reported, because on hidden SSIDs, strlen() doesn't work, since the SSID is all NULLBYTES!
//If you don't need that info, set ssidlen to NULL!
char *get_ssid(struct packet *pkt, unsigned char *ssidlen);
char *get_meshid(struct packet *pkt, unsigned char *meshidlen);

uint16_t get_capabilities(struct packet *pkt);

//Append data to packet
void append_data(struct packet *pkt, unsigned char *data, int len);

//Append country code to packet
void add_country_set(struct packet *pkt, char *country);

//Adds LLC header to a packet created with create_ieee_hdr(). You can use this to build unencrypted data frames or EAP packets.
void add_llc_header(struct packet *pkt, uint16_t llc_type);

//Adds EAP/WPA packet behind the LLC Header to create WPA Login packets
void add_eapol(struct packet *pkt, uint16_t wpa_length, uint8_t *wpa_element, uint8_t wpa_1or2, uint8_t rsn_version, uint64_t rsn_replay);

void increase_seqno(struct packet *pkt);
uint16_t get_seqno(struct packet *pkt);
//If pkt is NULL in set_seqno, the sequence number for the next call to create_ieee_hdr will be seqno + 1!
uint16_t get_next_seqno();
void set_seqno(struct packet *pkt, uint16_t seqno);

uint8_t get_fragno(struct packet *pkt);
void set_fragno(struct packet *pkt, uint8_t frag, int last_frag);

void add_ssid_set(struct packet *pkt, char *ssid);
void add_rate_sets(struct packet *pkt, char b_rates, char g_rates);

#endif
