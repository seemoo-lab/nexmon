#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "fragmenting.h"
#include "mac_addr.h"
#include "osdep.h"

int frag_min = 0, frag_max = 0, std_comp = 0, percentage = 0;

char *frag_help = "#### This version supports IDS Evasion  (Fragmenting) ####\n"
                  "# Just append  --frag <min_frags>,<max_frags>,<percent>  #\n"
                  "# after your attack mode identifier to fragment all      #\n"
                  "# outgoing packets, possibly avoiding lots of IDS!       #\n"
                  "# <min_frags> : Minimum fragments to split packets into  #\n"
                  "# <max_frags> : Maximum amount of fragments to create    #\n"
                  "# <percent>   : Percantage of packets to fragment        #\n"
                  "# NOTE: May not fully work with every driver, YMMV...    #\n"
                  "# HINT: Set max_frags to 0 to enable standard compliance #\n"
                  "##########################################################\n";

void frag_print_help() {
  printf("%s\n", frag_help);
}

int frag_is_enabled() {
  return frag_min;
}

int frag_send_frag(struct packet *pkt, int start, int size, uint8_t fragno, int last_frag) {
  struct packet inject;
  struct ieee_hdr *hdr = (struct ieee_hdr *) inject.data;
  int payload_start = sizeof(struct ieee_hdr);

  memcpy(inject.data, pkt->data, pkt->len);
  if (hdr->type == 0x88) payload_start += 2; //handle QoS frames

  set_fragno(&inject, fragno, last_frag);

  memmove(inject.data + payload_start, inject.data + start + payload_start, size);
  inject.len = payload_start + size;

  return osdep_send_packet(&inject);
}

int frag_send_packet(struct packet *pkt) {
  struct ieee_hdr *hdr = (struct ieee_hdr *) pkt->data;
  struct ether_addr *dst = get_destination(pkt);
  int q = 0, rnd, payload_size, fragsize, i, last_frag = 0;
  float fragfsize, fi;
  uint8_t fragno = 0;

  //Check if already fragged => inject normally
  if (get_fragno(pkt)) return osdep_send_packet(pkt);

  //Encrypted packets cannot be fragmented, since each fragment is encrypted individually
  if (hdr->flags & 0xB0) return osdep_send_packet(pkt);

  //Std Comp: Only frag Unicast packets!
  //"The MAC may fragment and reassemble individually addressed MSDUs"
  if (std_comp && MAC_IS_BCAST(*dst)) return osdep_send_packet(pkt);

  if (hdr->type == 0x88) q = 2; //handle QoS frames
  payload_size = pkt->len -(sizeof(struct ieee_hdr) + q);

  //Check if packet is fraggable (has payload), no => inject normally
  if (pkt->len <= sizeof(struct ieee_hdr) + q) return osdep_send_packet(pkt);

  //Make a choice if packet is in the frag percentage
  rnd = random() % 100;
  if (rnd > percentage) return osdep_send_packet(pkt);

  //Select fragment count
  rnd = random() % (frag_max - frag_min + 1);
  rnd += frag_min;

  //Check if payload is longer than frag count, no => inject bytewise
  //Std Comp. Payload TWICE the frag count
  if (std_comp) {
    if ((2 * rnd) > payload_size) rnd = payload_size / 2;
  } else {
    if (rnd > payload_size) rnd = payload_size;
  }

  //Inject frags
  if (std_comp) {
    //Be compliant!
    //"The length of each fragment shall be an equal number of octets for all fragments except the last."
    fragsize = payload_size / rnd;
    if (fragsize % 2) fragsize++; //"The length of each fragment shall be an even number of octets, except for the last fragment."
    if ((fragsize * 15) < payload_size) fragsize += 2; //We can only send 15 frags max

    for (i=0; i<payload_size; i+=fragsize) {
      if ((i + fragsize) > payload_size) fragsize = (payload_size - i); //Adjust size of last fragment
      if (frag_send_frag(pkt, i, fragsize, fragno++, ((i + fragsize) == payload_size))) return -1;
    }
  } else {
    //VIOLATE THE STANDARD NOW:
    //"The MAC may fragment and reassemble individually addressed MSDUs"
    //"The length of each fragment shall be an equal number of octets for all fragments except the last."
    //"The length of each fragment shall be an even number of octets, except for the last fragment."
    fragfsize = (float) payload_size / (float) rnd;

    i = 0;
    for (fi=fragfsize; fi <= ((float)payload_size) + 0.5; fi+=fragfsize) {
      fragsize = (int) floorf(fi) - i;

      if (fragno == (rnd - 1)) { //Adjust size of last fragment because floats aren't exact
        last_frag = 1;
        fragsize = payload_size - i;
      }

      if (frag_send_frag(pkt, i, fragsize, fragno++, last_frag)) return -1;
      i = (int) floorf(fi); //Store where last fragment ended
    }
  }

  return 0;
}

void start_fragging(int min, int max, int perc) {

  if (max == 0) {
    printf("Standard Compliant Fragmentation activated! Will not fragment Broadcasts!\n");
    std_comp = 1;
    max = 15;
  }

  if ((min < 1) || (max < 1)) {
    printf("NOT funny: Packets must be sent in at least one fragment! Raising fragment count\n");
    if (min < 1) min = 1;
    if (max < 1) max = 1;
  }

  if ((min > 15) || (max > 15)) {
    printf("IEEE 802.11 only supports up to 15 fragments, lowering fragment count\n");
    if (min > 15) min = 15;
    if (max > 15) max = 15;
  }

  if (min > max) {
    printf("Your fragmenting minimum is greater than the maximum, raising maximum to 15\n");
    max = 15;
  }

  frag_min = min;
  frag_max = max;

  if (perc < 1) { printf("Adjusting percentage to 1\n"); perc = 1; }
  else if (perc > 100) { printf("Adjusting percentage to 100\n"); perc = 100; }
  percentage = perc;

  printf("IDS Evasion via Fragmentation is enabled and set to select %d to %d fragments for %d%% of all injected packets.\n", min, max, perc);
}

void parse_frag(const char *input) {
  int parseok;
  int min, max, perc;

  parseok = sscanf(input, "%d,%d, %d", &min, &max, &perc);

  if (parseok != 3) {
    printf("Your fragmenting parameters are unparseable...\n");
    exit(-1);
  }

  start_fragging(min, max, perc);
}
