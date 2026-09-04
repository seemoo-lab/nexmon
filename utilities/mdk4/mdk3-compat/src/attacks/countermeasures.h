#ifndef HAVE_COUNTERMEASURES_H
#define HAVE_COUNTERMEASURES_H

#include "attacks.h"

void countermeasures_shorthelp();

void countermeasures_longhelp();

struct attacks load_countermeasures();

void *countermeasures_parse(int argc, char *argv[]);

struct packet countermeasures_getpacket(void *options);

void countermeasures_print_stats(void *options);

void countermeasures_perform_check(void *options);

#endif