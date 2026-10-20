#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

#include "fuzzer.h"
#include "../osdep.h"
#include "../channelhopper.h"
#include "../helpers.h"

#define FUZZ_MODE 'f'
#define FUZZ_NAME "Packet Fuzzer"
#define FUZZ_MAX_SIZE 1500

struct fuzz_options {
  char *sources;
  char *modifiers;
  int speed;
};

//Global things, shared by packet creation and stats printing
char *source, *modifier = NULL;
struct packet injected, sniffed;
pthread_mutex_t packet_mutex = PTHREAD_MUTEX_INITIALIZER;

void fuzz_shorthelp()
{
  printf("  A simple packet fuzzer with multiple packet sources\n");
  printf("  and a nice set of modifiers. Be careful!\n");
}

void fuzz_longhelp()
{
  printf( "  A simple packet fuzzer with multiple packet sources\n"
          "  and a nice set of modifiers. Be careful!\n"
          "  mdk3 randomly selects the given sources and one or multiple modifiers.\n"
          "      -s <sources>\n"
          "         Specify one or more of the following packet sources:\n"
          "         a - Sniff packets from the air\n"
          "         b - Create valid beacon frames with random SSIDs and properties\n"
          "         c - Create CTS frames to broadcast (you can also use this for a CTS DoS)\n"
          "         p - Create broadcast probe requests\n"
          "      -m <modifiers>\n"
          "         Select at least one of the modifiers here:\n"
          "         n - No modifier, do not modify packets\n"
          "         b - Set destination address to broadcast\n"
          "         m - Set source address to broadcast\n"
          "         s - Shotgun: randomly overwrites a couple of bytes\n"
          "         t - append random bytes (creates broken tagged parameters in beacons/probes)\n"
          "         c - Cut packets short, preferably somewhere in headers or tags\n"
          "         d - Insert random values in Duration and Flags fields\n"
          "      -c [chan,chan,...,chan[:speed]]\n"
          "         Enable channel hopping. When -c h is given, mdk3 will hop an all\n"
          "         14 b/g channels. Channel will be changed every 3 seconds,\n"
          "         if speed is not specified. Speed value is in milliseconds!\n"
          "      -p <pps>\n"
          "         Set speed in packets per second (Default: 250)\n");
}

int like_options(char *options, char *valid) {
  unsigned int i;

  for (i=0; i<strlen(options); i++) {
    if (! strchr(valid, options[i])) return 0;
  }

  return 1;
}

void *fuzz_parse(int argc, char *argv[]) {
  int opt, speed;
  char *speedstr;
  struct fuzz_options *fopt = malloc(sizeof(struct fuzz_options));

  fopt->sources = NULL;
  fopt->modifiers = NULL;
  fopt->speed = 250;

  while ((opt = getopt(argc, argv, "s:m:c:p:")) != -1) {
    switch (opt) {
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
      case 'p':
        fopt->speed = (unsigned int) atoi(optarg);
      break;
      case 's':
        if (like_options(optarg, "abcp")) fopt->sources = optarg;
      break;
      case 'm':
        if (like_options(optarg, "nbmstcd")) fopt->modifiers = optarg;
      break;
      default:
        fuzz_longhelp();
        printf("\n\nUnknown option %c\n", opt);
        return NULL;
    }
  }

  if ((! fopt->sources) || (! fopt->modifiers)) {
    fuzz_longhelp();
    printf("\n\nSources and Modifiers must be specified!\n");
    return NULL;
  }

  return (void *) fopt;
}

void fuzz_sniffer(void *options) {
  struct fuzz_options *fopt = (struct fuzz_options *) options;
  struct packet sniff;

  sniffed.len = 0;

  if (! strchr(fopt->sources, 'a')) return;  //Sniffer source not selected, skip

  while(1) {
    sniff = osdep_read_packet();

    pthread_mutex_lock(&packet_mutex);

    if ((sniff.len != injected.len) || memcmp(sniff.data, injected.data, sniff.len)) {
      sniffed = sniff;
    }

    pthread_mutex_unlock(&packet_mutex);
  }
}

struct packet fuzz_getpacket(void *options) {
  static pthread_t *sniffer = NULL;
  struct fuzz_options *fopt = (struct fuzz_options *) options;
  struct packet pkt;
  unsigned int mod, modcount, i, j, k, src = fopt->sources[random() % strlen(fopt->sources)];
  char *ssid, encs[5] = "nwta";
  unsigned char *extra;
  struct ether_addr dest;
  struct ieee_hdr *hdr;

  if (! sniffer) {
    sniffer = malloc(sizeof(pthread_t));
    pthread_create(sniffer, NULL, (void *) fuzz_sniffer, (void *) fopt);
  }

  sleep_till_next_packet(fopt->speed);
  pkt.len = 0;

  while (pkt.len == 0) {
    switch(src) {
      case 'a':
        pthread_mutex_lock(&packet_mutex);
        pkt = sniffed;
        pthread_mutex_unlock(&packet_mutex);
        source = "sniffer";
      break;
      case 'b':
        ssid = generate_ssid(1);
        pkt = create_beacon(generate_mac(MAC_KIND_AP), ssid, osdep_get_channel(), encs[random() % 4], (random() % 2) ? 54 : 11, random() % 2);
        free(ssid);
        source = "beacon";
      break;
      case 'c':
        MAC_SET_BCAST(dest);
        pkt = create_cts(dest, AUTH_DEFAULT_DURATION);
        source = "cts";
      break;
      case 'p':
        pkt = create_probe(generate_mac(MAC_KIND_CLIENT), "" /* BCast Probe = Empty SSID */, 54);
        source = "probe";
      break;
      default:
        pkt.len = 0;
    }
  }

  modcount = (random() % strlen(fopt->modifiers)) + 1;
  if (modifier) free(modifier);
  modifier = malloc(modcount + 1);
  memset(modifier, 0x00, modcount + 1);

  for (i=0; i<modcount; i++) {
    mod = fopt->modifiers[random() % strlen(fopt->modifiers)];
    modifier[i] = mod;

    switch(mod) {
      case 'n':
      break;
      case 'b': //dest to bcast
        memset(get_destination(&pkt)->ether_addr_octet, '\xFF', 6);
      break;
      case 'm': //src to bcast
        memset(get_source(&pkt)->ether_addr_octet, '\xFF', 6);
      break;
      case 's': //shotgun
        k = ((random() % pkt.len) / 4) + 1;
        for (j=0; j<k; j++)
          pkt.data[random() % pkt.len] = random();
      break;
      case 't': //add broken tagged parameters
        k = random() % (FUZZ_MAX_SIZE - pkt.len);
        if (k < FUZZ_MAX_SIZE) {
          extra = malloc(k);
          for (j=0; j<k; j++) extra[j] = random();
          append_data(&pkt, extra, k);
          free(extra);
        }
      break;
      case 'c':
        pkt.len = (random() % pkt.len) + 1;
      break;
      case 'd':
        hdr = (struct ieee_hdr *) pkt.data;
        hdr->duration = random();
        hdr->flags = random();
      break;
      default: //WTF
        pkt.len = 0; return pkt;
    }
  }

  pthread_mutex_lock(&packet_mutex);
  injected = pkt;
  pthread_mutex_unlock(&packet_mutex);
  return pkt;
}

void fuzz_print_stats(void *options) {
  options = options;

  printf("\rSelected packet source: %10s  Used modifiers: %s\n", source, modifier);
}

void fuzz_perform_check(void *options) {
  //Nothing to check
  options = options; //Avoid unused warning
}

struct attacks load_fuzz() {
  struct attacks this_attack;
  char *fuzz_name = malloc(strlen(FUZZ_NAME) + 1);
  strcpy(fuzz_name, FUZZ_NAME);

  this_attack.print_shorthelp = (fp) fuzz_shorthelp;
  this_attack.print_longhelp = (fp) fuzz_longhelp;
  this_attack.parse_options = (fpo) fuzz_parse;
  this_attack.get_packet = (fpp) fuzz_getpacket;
  this_attack.print_stats = (fps) fuzz_print_stats;
  this_attack.perform_check = (fps) fuzz_perform_check;
  this_attack.mode_identifier = FUZZ_MODE;
  this_attack.attack_name = fuzz_name;

  return this_attack;
}
