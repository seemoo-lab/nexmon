#ifndef MYWIDS_H_
#define MYWIDS_H_

#include "attacks_wids.h"



struct attacks load_wids();

void *wids_parse(char* myessid, int use_chan_hopping, int use_zero_chaos);

struct packet wids_getpacket(void *options);

void wids_print_stats(void *options);

void wids_perform_check(void *options);

#endif