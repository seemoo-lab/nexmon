#ifndef HAVE_GHOSTING_H
#define HAVE_GHOSTING_H

#ifdef __linux__

//This does the job for you
void parse_ghosting(const char *input);

//If you set rate or power to -1, ghosting will be disabled for this!
//So If you just want to change your tx power every 100 ms from 10 dBm to your cards max:
//start_ghosting(100, -1, 10);
void start_ghosting(unsigned int period, int max_bitrate, int min_tx_power);

void ghosting_print_help();

#endif

#endif
