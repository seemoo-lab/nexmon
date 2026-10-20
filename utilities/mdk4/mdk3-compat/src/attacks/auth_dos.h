#ifndef HAVE_AUTH_DOS_H
#define HAVE_AUTH_DOS_H

#include "attacks.h"

void auth_dos_shorthelp();

void auth_dos_longhelp();

struct attacks load_auth_dos();

void *auth_dos_parse(int argc, char *argv[]);

struct packet auth_dos_getpacket(void *options);

void auth_dos_print_stats(void *options);

void auth_dos_perform_check(void *options);

#endif