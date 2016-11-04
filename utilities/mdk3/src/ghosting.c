#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "ghosting.h"
#include "osdep.h"

#ifdef __linux__

unsigned int ghosting_period = 100;
int ghosting_maxrate, ghosting_minpower;

/*  ZERO_CHAOS says:
 *    If you want to make the WIDS vendors hate you,
 *    change tx power of the card while injecting, so you can evade location tracking.
 *    If you turn the radio's power up and down every few ms, the trackers will have a much harder time finding you
 *    (basicly you will hop all over the place depending on sensor position). At least madwifi can do it.
 * 
 *    Also change speed every few ms, not a fantastic evasion technique but it may cause more location tracking oddity.
 */

char *ghost_help ="###### This version supports IDS Evasion (Ghosting) ######\n"
		  "# Just append  --ghost <period>,<max_rate>,<min_txpower> #\n"
		  "# after your attack mode identifier to enable ghosting!  #\n"
		  "# <period>      : How often (in ms) to switch rate/power #\n"
		  "# <max_rate>    : Maximum Bitrate to use in MBit         #\n"
		  "# <min_txpower> : Minimum TX power in dBm to use         #\n"
		  "# NOTE: Does not fully work with every driver, YMMV...   #\n"
		  "##########################################################\n";

void ghosting_print_help() {
  printf("%s\n", ghost_help);
}

void txpower_ghosting_thread() {
  while(1) {
    osdep_random_txpower(ghosting_minpower);
    usleep(1000 * ghosting_period);
  }
}

void bitrate_ghosting_thread() {
  int maxrate = ghosting_maxrate * 1000000;
  long rnd;
  
  if (maxrate == 5000000) maxrate = 5500000; //5.5 MBit crap :(
  
  while(1) {
    do {
      rnd = random() % VALID_RATE_COUNT;
    } while (VALID_BITRATES[rnd] > maxrate);

    osdep_set_rate(VALID_BITRATES[rnd]);
    
    usleep(1000 * ghosting_period);
  }
}

//If you set rate or power to -1, ghosting will be disabled for this!
//So If you just want to change your tx power every 100 ms from 10 dBm to your cards max:
//start_ghosting(100, -1, 10);
void start_ghosting(unsigned int period, int max_bitrate, int min_tx_power) {
  pthread_t rateghost, powerghost;
  
  //Gotta get stuff away from the stack:
  ghosting_period = period;
  ghosting_maxrate = max_bitrate;
  ghosting_minpower = min_tx_power;
  
  osdep_init_txpowers();  //don't forget to init stuff!

  printf("Ghosting has been activated: ");
  
  if (max_bitrate < 1) {
    printf("You're a funny guy... Your maximum bitrate is less than 1....\n");
    return;
  }
  
  if (min_tx_power > osdep_get_max_txpower()) {
    printf("Your minimum TX power is greater than your cards maximum. You want to fry stuff?\n");
    return;
  }
  
  if (max_bitrate != -1) {
    printf("Change Bitrate from 1 to %d MBit, ", max_bitrate);
    pthread_create(&rateghost, NULL, (void *) bitrate_ghosting_thread, (void *) NULL);
  }
  
  if (min_tx_power != -1) {
    printf("Change TX Power from %d to %d dBm, ", min_tx_power, osdep_get_max_txpower());
    pthread_create(&powerghost, NULL, (void *) txpower_ghosting_thread, (void *) NULL);
  }
  
  printf("every %d milliseconds\n", period);
}

void parse_ghosting(const char *input) {
  int parseok;
  int per, rate, pow;
  
  parseok = sscanf(input, "%d,%d,%d", &per, &rate, &pow);
  
  if (parseok != 3) {
    printf("Your ghosting parameters are unparseable...\n");
    exit(-1);
  }
  
  start_ghosting(per, rate, pow);
}
#endif
