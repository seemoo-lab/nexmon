#ifndef HAVE_DUMPFILE_H
#define HAVE_DUMPFILE_H

#include "packet.h"

void dump_packet(struct packet *pkt);

void start_dump(char *filename);

void stop_dump();

#endif
