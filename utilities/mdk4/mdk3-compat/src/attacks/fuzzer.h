#ifndef FUZZER_H_
#define FUZZER_H_

#include "attacks.h"

void fuzz_shorthelp();

void fuzz_longhelp();

struct attacks load_fuzz();

void *fuzz_parse(int argc, char *argv[]);

struct packet fuzz_getpacket(void *options);

void fuzz_print_stats(void *options);

void fuzz_perform_check(void *options);

#endif
