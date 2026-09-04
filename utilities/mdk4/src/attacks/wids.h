#ifndef WIDS_H_
#define WIDS_H_

#include "attacks.h"

void wids_shorthelp();

void wids_longhelp();

struct attacks load_wids();

void *wids_parse(int argc, char *argv[]);

struct packet wids_getpacket(void *options);

void wids_print_stats(void *options);

void wids_perform_check(void *options);

#endif
