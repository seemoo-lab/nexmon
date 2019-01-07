#ifndef HAVE_MYAUTH_DOS_H
#define HAVE_MYAUTH_DOS_H

#include "attacks_authflood.h"



struct attacks load_auth_dos();

void *auth_dos_parse(char* ap_mac, int intelligent);

struct packet auth_dos_getpacket(void *options);

void auth_dos_print_stats(void *options);

void auth_dos_perform_check(void *options);

#endif