#ifndef HAVE_BEACON_FLOOD_H
#define HAVE_BEACON_FLOOD_H

#include "attacks_beaconflood.h"


struct attacks load_beacon_flood();

void *beacon_flood_parse();

struct packet beacon_flood_getpacket(void *options);

void beacon_flood_print_stats(void *options);

void beacon_flood_perform_check(void *options);

#endif