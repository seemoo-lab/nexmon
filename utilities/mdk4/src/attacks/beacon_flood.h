#ifndef HAVE_BEACON_FLOOD_H
#define HAVE_BEACON_FLOOD_H

#include "attacks.h"

void beacon_flood_shorthelp();

void beacon_flood_longhelp();

struct attacks load_beacon_flood();

void *beacon_flood_parse(int argc, char *argv[]);

struct packet beacon_flood_getpacket(void *options);

void beacon_flood_print_stats(void *options);

void beacon_flood_perform_check(void *options);

#endif