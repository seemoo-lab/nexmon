#ifndef HAVE_PROBING_H
#define HAVE_PROBING_H

#include "attacks.h"

void probing_shorthelp();

void probing_longhelp();

struct attacks load_probing();

void *probing_parse(int argc, char *argv[]);

struct packet probing_getpacket(void *options);

void probing_print_stats(void *options);

void probing_perform_check(void *options);

#endif
