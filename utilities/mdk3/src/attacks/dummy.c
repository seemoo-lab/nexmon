#include <stdio.h>
#include <string.h>

#include "dummy.h"

#define DUMMY_MODE 'D'
#define DUMMY_NAME "An empty dummy attack"

// A starting point to build new attack modules for mdk3

// IMPORTANT:
// In order to include your attack into mdk3, you have to add it to attacks.h!

struct dummy_options {
  int option_count;
};

void dummy_shorthelp()
{
  printf("  dummy call short help\n");
}

void dummy_longhelp()
{
  printf("  dummy call 2 LONG HELP\n");
}

void *dummy_parse(int argc, char *argv[]) {
  int i;
  struct dummy_options *dopt = malloc(sizeof(struct dummy_options));
  
  for (i=0; i<argc; i++)
    printf("parse: %d: %s\n", i, argv[i]);
  
  if (argc < 3) {
    printf("Missing arguments (need at least 3)!\n");
    return NULL;
  }
  
  dopt->option_count = argc;
  
  return (void *) dopt;
}

void dummy_check(void *options) {
  options = options;
  printf("Implement check here\n");
}

struct packet dummy_getpacket(void *options) {
  options = options;
  struct packet pkt;
  
  printf("Build your packet here and return it. NULL data makes mdk3 exit\n");
   
  pkt.len = 0;
  
  return pkt;
}

void dummy_stats(void *options) {
  options = options;
  
  printf("This is called every second to display statistics\n");
}

struct attacks load_dummy() {
  struct attacks this_attack;
  char *dummy_name = malloc(strlen(DUMMY_NAME) + 1);
  strcpy(dummy_name, DUMMY_NAME);

  this_attack.print_shorthelp = (fp) dummy_shorthelp;
  this_attack.print_longhelp = (fp) dummy_longhelp;
  this_attack.parse_options = (fpo) dummy_parse;
  this_attack.mode_identifier = DUMMY_MODE;
  this_attack.attack_name = dummy_name;
  this_attack.perform_check = (fps) dummy_check;
  this_attack.get_packet = (fpp) dummy_getpacket;
  this_attack.print_stats = (fps) dummy_stats;
  
  return this_attack;
}
