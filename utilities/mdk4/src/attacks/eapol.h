#ifndef HAVE_EAPOL_H
#define HAVE_EAPOL_H

#include "attacks.h"

void eapol_shorthelp();

void eapol_longhelp();

struct attacks load_eapol();

void *eapol_parse(int argc, char *argv[]);

struct packet eapol_getpacket(void *options);

void eapol_print_stats(void *options);

void eapol_perform_check(void *options);

#endif