#ifndef HAVE_ATTACKS_H
#define HAVE_ATTACKS_H

#include "beacon_flood.h"
#include "auth_dos.h"
#include "probing.h"
#include "deauth.h"
#include "countermeasures.h"
#include "eapol.h"
#include "ieee80211s.h"
#include "wids.h"
#include "fuzzer.h"

#include "../packet.h"

typedef void		(*fp) (void);
typedef char		(*fpc)(void);
typedef void*		(*fpo)(int, char **);
typedef struct packet 	(*fpp)(void *);
typedef void		(*fps)(void *);

struct attacks {
  /* void  */ fp print_shorthelp;/* (void) */
  /* void  */ fp print_longhelp; /* (void) */
  /* void* */ fpo parse_options; /* (int argc, char *argv[]) - Each attack parses its own options and returns a pointer to some struct,
				 //                            that contains their parsed result. If parsing fail, return NULL, mdk will exit! */
  /* packet*/ fpp get_packet;	 /* (void *) - This is called to get a packet from an attack, supplying the options returned by parse_options as void * */
				 //            Should return the packet mdk3 should send. Use usleep in this call to adjust injection speed
				 //            If the packet.data is NULL, and/or packet.len is 0, mdk3 will exit the sending loop
  /* void  */ fps print_stats;	 /* (void *) - This is called each second to enable attack to print its stats; void * to options supplied */
  /* void  */ fps perform_check; /* (void *) - Called after each injected packet, implement logic to check for success here */

  char mode_identifier; /* A character to identify the mode: mdk3 <interface> MODE */
  char *attack_name; /* The name of this attack */
};

struct attacks *load_attacks(int *count);

int attack_count;

#endif
