#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "packet.h"

static uint16_t seqno = 0;

void create_ieee_hdr(struct packet *pkt, uint8_t type, char dsflags, uint16_t duration, struct ether_addr destination, struct ether_addr source, struct ether_addr bssid_or_transm, struct ether_addr recv, uint8_t fragment) {
  struct ieee_hdr *hdr = (struct ieee_hdr *) pkt->data;

  //If fragment, do not increase sequence
  if (!fragment) seqno++; seqno %= 0x1000;
  
  if (fragment > 0x0F) {
    printf("WARNING: Fragment number exceeded maximum of 15, resetting to 0.\n");
    fragment = 0;
  }
  
  hdr->type = type;
  
  hdr->flags = 0x00;
  //if (wep) hdr->flags |= 0x40; //If somebody needs WEP, here it is :D
  
  switch (dsflags) {
    case 'a':	//Ad Hoc, Beacons:    ToDS 0 FromDS 0  Addr: DST, SRC, BSS
      MAC_COPY(hdr->addr1, destination);
      MAC_COPY(hdr->addr2, source);
      MAC_COPY(hdr->addr3, bssid_or_transm);
      break;
    case 'f':	//From AP to station: ToDS 0 FromDS 1  Addr: DST, BSS, SRC
      hdr->flags |= 0x02;
      MAC_COPY(hdr->addr1, destination);
      MAC_COPY(hdr->addr2, bssid_or_transm);
      MAC_COPY(hdr->addr3, source);
      break;
    case 't':	//From station to AP: ToDS 1 FromDS 1  Addr: BSS, SRC, DST
      hdr->flags |= 0x01;
      MAC_COPY(hdr->addr1, bssid_or_transm);
      MAC_COPY(hdr->addr2, source);
      MAC_COPY(hdr->addr3, destination);
      break;
    case 'w':	//WDS:                ToDS 1 FromDS 1  Addr: RCV, TRN, DST ... SRC
      hdr->flags |= 0x03;
      MAC_COPY(hdr->addr1, recv);
      MAC_COPY(hdr->addr2, bssid_or_transm);
      MAC_COPY(hdr->addr3, destination);
      memcpy((pkt->data) + (sizeof(struct ieee_hdr)), source.ether_addr_octet, ETHER_ADDR_LEN);
      break;
    default:
      printf("ERROR: DS Flags invalid, use only a, f, t or w! Frame will have no MAC adresses!\n");
  }
  
  hdr->duration = htole16(duration);
  
  hdr->frag_seq = htole16(fragment | (seqno << 4));
  
  //TODO: Maybe we need to add support for other frame types beside DATA and Beacon.
  //      A good idea would also be QoS Data support
  
  pkt->len = sizeof(struct ieee_hdr);
  if ((hdr->flags & 0x03) == 0x03) pkt->len += 6;	//Extra MAC in WDS packets
}

struct ether_addr *get_addr(struct packet *pkt, char type) {
  uint8_t dsflags;
  struct ieee_hdr *hdr;
  struct ether_addr *src = NULL, *dst = NULL, *bss = NULL, *trn = NULL;
  
  if(! pkt) {
    printf("BUG: Got NULL packet!\n");
    return NULL;
  }
  
  hdr = (struct ieee_hdr *) pkt->data;
  dsflags = hdr->flags & 0x03;
  
  switch (dsflags) {
    case 0x00:
      dst = &(hdr->addr1);
      src = &(hdr->addr2);
      bss = &(hdr->addr3);
      break;
    case 0x01:
      bss = &(hdr->addr1);
      src = &(hdr->addr2);
      dst = &(hdr->addr3);
      break;
    case 0x02:
      dst = &(hdr->addr1);
      bss = &(hdr->addr2);
      src = &(hdr->addr3);
      break;
    case 0x03:
      bss = &(hdr->addr1);
      trn = &(hdr->addr2);
      dst = &(hdr->addr3);
      src = (struct ether_addr *) &(pkt->data) + (sizeof(struct ieee_hdr));
      break;
  }
  
  switch (type) {
    case 'b':
      return bss;
    case 'd':
      return dst;
    case 's':
      return src;
    case 't':
      return trn;
  }
  
  return NULL;
}

struct ether_addr *get_bssid(struct packet *pkt) {
  return get_addr(pkt, 'b');
}

struct ether_addr *get_source(struct packet *pkt) {
  return get_addr(pkt, 's');
}

struct ether_addr *get_destination(struct packet *pkt) {
  return get_addr(pkt, 'd');
}

struct ether_addr *get_transmitter(struct packet *pkt) {
  return get_addr(pkt, 't');
}

struct ether_addr *get_receiver(struct packet *pkt) {
  return get_addr(pkt, 'b');
}

void add_ssid_set(struct packet *pkt, char *ssid) {
  pkt->data[pkt->len] = 0x00;	//SSID parameter set
  pkt->data[pkt->len+1] = (uint8_t) strlen(ssid);	//SSID len
  memcpy(pkt->data + pkt->len + 2, ssid, strlen(ssid));	//Copy the SSID
  
  pkt->len += strlen(ssid) + 2;
}

void add_rate_sets(struct packet *pkt, char b_rates, char g_rates) {
  if (b_rates) {
    memcpy(pkt->data + pkt->len, DEFAULT_11B_RATES, 6);	//11 MBit
    pkt->len += 6;
  }
  if (g_rates) {
    memcpy(pkt->data + pkt->len, DEFAULT_11G_RATES, 10);	//54 MBit
    pkt->len += 10;
  }
}

void add_channel_set(struct packet *pkt, uint8_t channel) {
  pkt->data[pkt->len] = 0x03;	//Channel set
  pkt->data[pkt->len+1] = 0x01;	//One channel
  pkt->data[pkt->len+2] = channel;
  pkt->len += 3;
}

struct packet create_beacon(struct ether_addr bssid, char *ssid, uint8_t channel, char encryption, unsigned char bitrate, char adhoc) {
  struct packet beacon;
  struct beacon_fixed *bf;
  static uint64_t internal_timestamp = 0;
  struct ether_addr bc;
  
  MAC_SET_BCAST(bc);
  create_ieee_hdr(&beacon, IEEE80211_TYPE_BEACON, 'a', 0, bc, bssid, bssid, SE_NULLMAC, 0);

  bf = (struct beacon_fixed *) (beacon.data + beacon.len);
  
  internal_timestamp += 0x400 * DEFAULT_BEACON_INTERVAL;
  bf->timestamp = htole64(internal_timestamp);
  bf->interval = htole16(DEFAULT_BEACON_INTERVAL);
  bf->capabilities = 0x0000;
  if (adhoc) { bf->capabilities |= 0x0002; } else { bf->capabilities |= 0x0001; }
  if (encryption != 'n') bf->capabilities |= 0x0010;

  beacon.len += sizeof(struct beacon_fixed);

  add_ssid_set(&beacon, ssid);
  add_rate_sets(&beacon, 1, (bitrate == 54));
  add_channel_set(&beacon, channel);

  if (encryption == 't') {
    memcpy(beacon.data + beacon.len, DEFAULT_WPA_TKIP_TAG, 26);
    beacon.len += 26;
  }
  if (encryption == 'a') {
    memcpy(beacon.data + beacon.len, DEFAULT_WPA_AES_TAG, 26);
    beacon.len += 26;
  }
  
  return beacon;
}

struct packet create_auth(struct ether_addr bssid, struct ether_addr client, uint16_t seq) {
  struct packet auth;
  struct auth_fixed *af;

  create_ieee_hdr(&auth, IEEE80211_TYPE_AUTH, 'a', AUTH_DEFAULT_DURATION, bssid, client, bssid, SE_NULLMAC, 0);
  
  af = (struct auth_fixed *) (auth.data + auth.len);
  
  af->algorithm = htole16(AUTH_ALGORITHM_OPEN);
  af->seq = htole16(seq);
  af->status = htole16(AUTH_STATUS_SUCCESS);
  
  auth.len = 30;
  return auth;
}

struct packet create_probe(struct ether_addr source, char *ssid, unsigned char bitrate) {
  struct packet probe;
  struct ether_addr bc;
  
  MAC_SET_BCAST(bc);
  create_ieee_hdr(&probe, IEEE80211_TYPE_PROBEREQ, 'a', 0, bc, source, bc, SE_NULLMAC, 0);

  add_ssid_set(&probe, ssid);
  add_rate_sets(&probe, 1, (bitrate == 54));
  
  return probe;
}

struct packet create_deauth(struct ether_addr destination, struct ether_addr source, struct ether_addr bssid) {
  struct packet deauth;
  uint16_t *reason;

  create_ieee_hdr(&deauth, IEEE80211_TYPE_DEAUTH, 'a', AUTH_DEFAULT_DURATION, destination, source, bssid, SE_NULLMAC, 0);
  
  reason = (uint16_t *) (deauth.data + deauth.len);
  
  if (MAC_MATCHES(source, bssid)) {
    *reason = htole16(DEAUTH_REASON_UNSPEC);	//AP to Station deauth is with unspecified reason
  } else {
    *reason = htole16(DEAUTH_REASON_LEAVING);	//Station to AP deauth is with reason "I am leavin the network"
  }
  
  deauth.len += 2;
  return deauth;
}

struct packet create_disassoc(struct ether_addr destination, struct ether_addr source, struct ether_addr bssid) {
  struct packet disassoc;
  uint16_t *reason;

  create_ieee_hdr(&disassoc, IEEE80211_TYPE_DISASSOC, 'a', AUTH_DEFAULT_DURATION, destination, source, bssid, SE_NULLMAC, 0);
  
  reason = (uint16_t *) (disassoc.data + disassoc.len);
  
  if (MAC_MATCHES(source, bssid)) {
    *reason = htole16(DISASSOC_REASON_APFULL);	//AP to Station: I kick you because I am crowded!
  } else {
    *reason = htole16(DISASSOC_REASON_LEAVING);	//Station to AP: Bye bye, I am leaving the network!
  }
  
  disassoc.len += 2;
  return disassoc;
}

struct packet create_assoc_req(struct ether_addr client, struct ether_addr bssid, uint16_t capabilities, char *ssid, unsigned char bitrate) {
  struct packet assoc;
  struct assoc_fixed *af;

  create_ieee_hdr(&assoc, IEEE80211_TYPE_ASSOCREQ, 'a', AUTH_DEFAULT_DURATION, bssid, client, bssid, SE_NULLMAC, 0);
  af = (struct assoc_fixed *) (assoc.data + assoc.len);

  af->capabilities = htole16(capabilities);
  af->interval = htole16(DEFAULT_LISTEN_INTERVAL);
  assoc.len += sizeof(struct assoc_fixed);
  
  add_ssid_set(&assoc, ssid);
  add_rate_sets(&assoc, 1, (bitrate == 54));
  
  return assoc;
}

struct packet create_cts(struct ether_addr destination, uint16_t duration) {
  struct packet pkt;
  struct cts *cts;

  cts = (struct cts *) pkt.data;

  cts->dest = destination;
  cts->duration = duration;
  cts->flags = 0;
  cts->type = IEEE80211_TYPE_CTS;

  pkt.len = sizeof(struct cts);

  return pkt;
}

char *get_id_type(struct packet *pkt, unsigned char *ssidlen, unsigned char type) {
  char *ssid = NULL;
  struct ieee_hdr *hdr = (struct ieee_hdr *) (pkt->data);
  unsigned char *tags = pkt->data + sizeof(struct ieee_hdr) + sizeof(struct beacon_fixed);
  
  if ((hdr->type != IEEE80211_TYPE_BEACON) && (hdr->type != IEEE80211_TYPE_PROBERES)) return NULL;
  //Thats neither a beacon nor a probe response, therefor it has no SSID
 
  while (tags < (pkt->data + pkt->len)) {
    if (tags[0] == type) {
      ssid = malloc(tags[1] + 1);
      if (ssidlen) *ssidlen = tags[1];
      memcpy(ssid, tags + 2, tags[1]);
      ssid[tags[1]] = 0x00;
      return ssid;
    }
    tags += tags[1] + 2;
  }
  
  //No ID found in Beacon Frame!
  return NULL;
}

char *get_ssid(struct packet *pkt, unsigned char *ssidlen) {
  return get_id_type(pkt, ssidlen, BEACON_TAGTYPE_SSID);
}

char *get_meshid(struct packet *pkt, unsigned char *meshidlen) {
  return get_id_type(pkt, meshidlen, BEACON_TAGTYPE_MESHID);  
}

uint16_t get_capabilities(struct packet *pkt) {
  struct ieee_hdr *hdr = (struct ieee_hdr *) (pkt->data);
  struct beacon_fixed *bf = (struct beacon_fixed *) (pkt->data + sizeof(struct ieee_hdr));
  
  if (hdr->type != IEEE80211_TYPE_BEACON) return 0; //Thats not a beacon, therefor it has no capabilities

  return le16toh(bf->capabilities);
}

void append_data(struct packet *pkt, unsigned char *data, int len) {
  memcpy(pkt->data + pkt->len, data, len);
  
  pkt->len += len;
}

void add_llc_header(struct packet *pkt, uint16_t llc_type) {
  struct llc_header *llc;

  llc = (struct llc_header *) (pkt->data + sizeof(struct ieee_hdr));
  llc->dsap = LLC_SNAP; llc->ssap = LLC_SNAP;
  llc->control = LLC_UNNUMBERED;
  llc->encap[0] = 0x00; llc->encap[1] = 0x00; llc->encap[2] = 0x00;
  llc->type = htobe16(llc_type);

  pkt->len += 8;
}

void add_eapol(struct packet *pkt, uint16_t wpa_length, uint8_t *wpa_element, uint8_t wpa_1or2, uint8_t rsn_version, uint64_t rsn_replay) {
  struct rsn_auth *rsn;
  uint32_t t;

  rsn = (struct rsn_auth *) (pkt->data + sizeof(struct ieee_hdr) + sizeof(struct llc_header));
  rsn->version = rsn_version;
  rsn->type = RSN_TYPE_KEY;
  rsn->length = htobe16(sizeof(struct rsn_auth) + wpa_length - 4);
  rsn->descriptor = RSN_DESCRIPTOR_KEY;
  rsn->key_info = htobe16(0x0108);	// Key MIC flag + Pairwise flag
  if (wpa_1or2 == 1) rsn->key_info |= htobe16(0x0001);
  if (wpa_1or2 == 2) rsn->key_info |= htobe16(0x0002);
  rsn->key_length = htobe16(0);
  rsn->replay_counter = htobe64(rsn_replay);
  for (t=0; t<32; t++) rsn->nonce[t] = random();
  memset(rsn->key_iv, 0x00, 16);
  rsn->key_rsc = htobe64(0);
  rsn->key_id = htobe64(0);
  for (t=0; t<16; t++) rsn->key_mic[t] = random();
  rsn->wpa_length = htobe16(wpa_length);
  memcpy(pkt->data + sizeof(struct ieee_hdr) + sizeof(struct llc_header) + sizeof(struct rsn_auth), wpa_element, wpa_length);

  pkt->len += sizeof(struct rsn_auth) + wpa_length;
}

void increase_seqno(struct packet *pkt) {
  uint16_t frgseq;
  struct ieee_hdr *hdr = (struct ieee_hdr *) (pkt->data);
  
  frgseq = letoh16(hdr->frag_seq);
  
  frgseq += 0x10;	//Lower 4 bytes are fragment number
  
  hdr->frag_seq = htole16(frgseq);
}

uint16_t get_seqno(struct packet *pkt) {
  uint16_t seq;
  struct ieee_hdr *hdr = (struct ieee_hdr *) (pkt->data);

  seq = letoh16(hdr->frag_seq);
  seq >>= 4;

  return seq;
}

uint8_t get_fragno(struct packet *pkt) {
  uint16_t seq;
  struct ieee_hdr *hdr = (struct ieee_hdr *) (pkt->data);

  seq = letoh16(hdr->frag_seq);

  return (seq & 0xF);
}

void set_seqno(struct packet *pkt, uint16_t seq) {
  struct ieee_hdr *hdr;
  uint16_t frgseq;

  if (!pkt) {
    seqno = seq;
    return;
  }

  hdr = (struct ieee_hdr *) (pkt->data);
  frgseq = letoh16(hdr->frag_seq);

  frgseq &= 0x000F;       //Clear seq, but keep fragment intact;
  frgseq |= (seq << 4); //Add seq

  hdr->frag_seq = htole16(frgseq);
}

void set_fragno(struct packet *pkt, uint8_t frag, int last_frag) {
  struct ieee_hdr *hdr = (struct ieee_hdr *) (pkt->data);
  uint16_t seq = letoh16(hdr->frag_seq);

  if (last_frag) hdr->flags &= 0xFB;
  else hdr->flags |= 0x04;

  seq &= 0xFFF0; //Clear frag bits
  seq |= frag;

  hdr->frag_seq = htole16(seq);
}
