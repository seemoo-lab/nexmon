#ifndef FRAGMENTING_H_
#define FRAGMENTING_H_

#include "packet.h"

void frag_print_help();
void parse_frag(const char *input);
int frag_is_enabled();
int frag_send_packet(struct packet *pkt);

#endif
