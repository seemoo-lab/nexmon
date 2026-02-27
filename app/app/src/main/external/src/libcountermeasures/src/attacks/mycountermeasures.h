#ifndef HAVE_MYCOUNTERMEASURES_H
#define HAVE_MYCOUNTERMEASURES_H

#include "attacks_countermeasures.h"


struct attacks load_countermeasures();

void *countermeasures_parse(char* ap_mac, int burst_pause_time, int packets_per_burst, int packets_per_sec, int use_qos_exploit);

struct packet countermeasures_getpacket(void *options);

void countermeasures_print_stats(void *options);

void countermeasures_perform_check(void *options);

#endif