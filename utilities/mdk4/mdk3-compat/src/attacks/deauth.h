#ifndef HAVE_DEAUTH_H
#define HAVE_DEAUTH_H

#include "attacks.h"

void deauth_shorthelp();

void deauth_longhelp();

struct attacks load_deauth();

void *deauth_parse(int argc, char *argv[]);

struct packet deauth_getpacket(void *options);

void deauth_print_stats(void *options);

void deauth_perform_check(void *options);

#endif