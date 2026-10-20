#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "probing.h"
#include "../osdep.h"
#include "../helpers.h"
#include "../brute.h"

#define PROBING_MODE 'p'
#define PROBING_NAME "SSID Probing and Bruteforcing"

struct probing_options {
  struct ether_addr *target;
  char *filename;
  char *ssid;
  unsigned int speed;
  char *charsets;
  char *proceed;
};

//Global things, shared by packet creation and stats printing
unsigned int probes, answers;
char *filessid = NULL;

void probing_shorthelp()
{
  printf("  Probes APs and checks for answer, useful for checking if SSID has\n");
  printf("  been correctly decloaked and if AP is in your sending range.\n");
  printf("  Bruteforcing of hidden SSIDs with or without a wordlist is also available.\n");
}


void probing_longhelp()
{
  printf( "  Probes APs and checks for answer, useful for checking if SSID has\n"
	  "  been correctly decloaked and if AP is in your sending range.\n"
	  "  Bruteforcing of hidden SSIDs with or without a wordlist is also available.\n"
	  "      -e <ssid>\n"
	  "         SSID to probe for\n"
	  "      -f <filename>\n"
	  "         Read SSIDs from file for bruteforcing hidden SSIDs\n"
	  "      -t <bssid>\n"
	  "         Set MAC address of target AP\n"
	  "      -s <pps>\n"
	  "         Set speed (Default: 400)\n"
	  "      -b <character sets>\n"
	  "         Use full Bruteforce mode (recommended for short SSIDs only!)\n"
	  "         You can select multiple character sets at once:\n"
	  "         * n (Numbers:   0-9)\n"
	  "         * u (Uppercase: A-Z)\n"
	  "         * l (Lowercase: a-z)\n"
	  "         * s (Symbols: ASCII)\n"
	  "      -p <word>\n"
	  "         Continue bruteforcing, starting at <word>.\n");
}


void *probing_parse(int argc, char *argv[]) {
  int opt;
  struct probing_options *popt = malloc(sizeof(struct probing_options));

  popt->target = NULL;
  popt->filename = NULL;
  popt->ssid = NULL;
  popt->speed = 400;
  popt->charsets = NULL;
  popt->proceed = NULL;
  
  while ((opt = getopt(argc, argv, "e:f:t:s:b:p:r")) != -1) {
    switch (opt) {
      case 'e':
	if (popt->filename || popt->charsets || popt->proceed) { 
	  printf("Select only one mode please (either -e, -f or -b), not two of them!\n"); return NULL; }
	popt->ssid = malloc(strlen(optarg) + 1);
	strcpy(popt->ssid, optarg);
      break;
      case 'f':
	if (popt->ssid || popt->charsets || popt->proceed) { 
	  printf("Select only one mode please (either -e, -f or -b), not two of them!\n"); return NULL; }
	popt->filename = malloc(strlen(optarg) + 1);
	strcpy(popt->filename, optarg);
      break;
      case 's':
	popt->speed = (unsigned int) atoi(optarg);
      break;
      case 't':
	if (popt->ssid) { 
	  printf("Targets (-t) are not needed for this Probing mode\n"); return NULL; }
	popt->target = malloc(sizeof(struct ether_addr));
	*(popt->target) = parse_mac(optarg);
      break;
      case 'b':
	if (popt->filename || popt->ssid) { 
	  printf("Select only one mode please (either -e, -f or -b), not two of them!\n"); return NULL; }
	popt->charsets = malloc(strlen(optarg) + 1);
	strcpy(popt->charsets, optarg);
      break;
      case 'p':
	if (popt->ssid || popt->filename) { 
	  printf("Select only one mode please (either -e, -f or -b), not two of them!\n"); return NULL; }
	popt->proceed = malloc(strlen(optarg) + 1);
	strcpy(popt->proceed, optarg);
      break;
      default:
	probing_longhelp();
	printf("\n\nUnknown option %c\n", opt);
	return NULL;
    }
  }
  
  if ((! popt->target) && popt->charsets) {
    printf("Bruteforce modes need a target MAC address (-t)\n");
    return NULL;
  }
  
  if ((! popt->target) && popt->filename) {
    printf("No target (-t) specified, will display ALL responses!\n");
  }
  
  if ((! popt->charsets) && popt->proceed) {
    printf("You need to specify a character set (-b)\n");
    return NULL;
  }
  
  if (!popt->filename && !popt->ssid && !popt->charsets) {
    probing_longhelp();
    printf("\nOptions are completely missing.\n");
    return NULL;
  }
  
  return (void *) popt;
}


unsigned int get_ssid_len(struct ether_addr target) {
  struct ieee_hdr *hdr;
  struct packet pkt;
  struct ether_addr *ap;
  char *ssid;
  unsigned char ssidlen;
  
  printf("Waiting for a beacon frame from target to get its SSID length.\n");
  
  while(1) {
    pkt = osdep_read_packet();
    if (pkt.len == 0) exit(-1);
    hdr = (struct ieee_hdr *) pkt.data;
    
    if (hdr->type == IEEE80211_TYPE_BEACON) {
      ap = get_source(&pkt);
      if (MAC_MATCHES(*ap, target)) {
	ssid = get_ssid(&pkt, &ssidlen);
	if (ssidlen < 2) { free(ssid); return 0; }  //SSID lengths 0 and 1 are known to be "full hidden", ie no length info :(
	if (strlen(ssid) == ssidlen) {
	  printf("WARNING: SSID DOES NOT SEEM TO BE HIDDEN, SSID IS %s\n", ssid);
	  printf("mdk3 will still continue, but its unlikely that this SSID is wrong\n");
	}
	free(ssid);
	return ssidlen;
      }
    }
  }
}


struct packet probing_getpacket(void *options) {
  struct probing_options *popt = (struct probing_options *) options;
  struct packet pkt;
  struct ether_addr src;
  static unsigned int ssidlen = 0, havessidlen = 0, brutelen = 1;
  
  if (! havessidlen && popt->target) {
    ssidlen = get_ssid_len(*(popt->target));
    printf("SSID length is %d\n", ssidlen);
    havessidlen = 1;
  }
  
  sleep_till_next_packet(popt->speed);
  src = generate_mac(MAC_KIND_CLIENT);
  
  if (popt->ssid) {
    pkt = create_probe(src, popt->ssid, 54);
    probes++;
    return pkt;
  }
  if (popt->filename) {
    if (filessid) free(filessid);
    do {
      filessid = read_next_line(popt->filename, 0);
      if (!filessid) {
	printf("\nWordlist completed.\n");
	sleep(3);	//Waiting for some leftover packets in queue
	exit(0);
      }
      if (!popt->target) break;
      if (!ssidlen) break;
    } while (strlen(filessid) != ssidlen);
    pkt = create_probe(src, filessid, 54);
    return pkt;
  }
  if (popt->charsets) {
    if (ssidlen && popt->proceed && (ssidlen != strlen(popt->proceed))) {
      printf("SSID length and length of bruteforcer start word are not equal. That won't work ;)\n");
      pkt.len = 0; return pkt;
    }
    if (ssidlen) {
      popt->proceed = get_brute_word(popt->charsets, popt->proceed, ssidlen);
    } else {
      popt->proceed = get_brute_word(popt->charsets, popt->proceed, brutelen);
      if (popt->proceed == NULL) {
	brutelen++;
	popt->proceed = get_brute_word(popt->charsets, NULL, brutelen);
      }
    }
    if (popt->proceed == NULL) { //Keyspace exhausted
      printf("\nKeyspace exhausted.\n");
      sleep(3);
      pkt.len = 0; return pkt;
    }
    pkt = create_probe(src, popt->proceed, 54);
    return pkt;
  }
  
  pkt.len = 0; return pkt;
}


void probing_print_stats(void *options) {
  struct probing_options *popt = (struct probing_options *) options;
  unsigned int perc;
  
  if (popt->ssid) {
    perc = ((answers * 100) / probes);
    printf("\rAP responded on %d of %d probes (%d percent)                  \n", answers, probes, perc);
    answers = probes = 0;
  }
  
  if (popt->filename) {
    printf("\rTrying SSID: %s                                           \n", filessid);
  }
  
  if (popt->charsets) {
    printf("\rTrying SSID: %s                                           \n", popt->proceed);
  }
}


void probing_sniffer(void *options) {
  struct probing_options *popt = (struct probing_options *) options;
  struct packet pkt;
  struct ether_addr *dup, dupdetect, *ap;
  struct ieee_hdr *hdr;
  char *ssid;
  
  while(1) {
    pkt = osdep_read_packet();
    if (pkt.len == 0) exit(-1);
    hdr = (struct ieee_hdr *) pkt.data;
    
    if (popt->ssid) { //Skip duplicates only in non-bruteforce modes
      dup = get_destination(&pkt);
      if (MAC_MATCHES(dupdetect, *dup)) continue;  //Duplicate ignored
      MAC_COPY(dupdetect, *dup);
      
      if (hdr->type == IEEE80211_TYPE_PROBERES) answers++;
    }
    
    if (popt->filename || popt->charsets) {
      ap = get_source(&pkt);
      if (hdr->type == IEEE80211_TYPE_PROBERES) {
	if (popt->target && MAC_MATCHES(*ap, *(popt->target))) {
	  ssid = get_ssid(&pkt, NULL);
	  printf("\rProbe Response from target AP with SSID %s                \n", ssid);
	  printf("Job's done, have a nice day :)\n");
	  free(ssid);
	  exit(0);
	} else if (! popt->target) {
	  printf("\rProbe response from "); print_mac(*ap);
	  ssid = get_ssid(&pkt, NULL);
	  printf(" with SSID %s                \n", ssid);
	  free(ssid);
	}
      }
    }
  }
}

void probing_perform_check(void *options) {
  static pthread_t *sniffer = NULL;
  
  if (!sniffer) {
    sniffer = malloc(sizeof(pthread_t));
    pthread_create(sniffer, NULL, (void *) probing_sniffer, (void *) options);
  }
}


struct attacks load_probing() {
  struct attacks this_attack;
  char *probing_name = malloc(strlen(PROBING_NAME) + 1);
  strcpy(probing_name, PROBING_NAME);

  this_attack.print_shorthelp = (fp) probing_shorthelp;
  this_attack.print_longhelp = (fp) probing_longhelp;
  this_attack.parse_options = (fpo) probing_parse;
  this_attack.get_packet = (fpp) probing_getpacket;
  this_attack.print_stats = (fps) probing_print_stats;
  this_attack.perform_check = (fps) probing_perform_check;
  this_attack.mode_identifier = PROBING_MODE;
  this_attack.attack_name = probing_name;

  return this_attack;
}
