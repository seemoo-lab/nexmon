#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <getopt.h>

#include "attacks/attacks.h"
#include "osdep.h"
#include "ghosting.h"
#include "fragmenting.h"
#include "channelhopper.h"

#define VERSION "4.2"
#define VERSION_COOL "Awesome! Supports Proof-of-concept of WiFi protocol implementation vulnerability testing"

void *global_cur_options;
struct attacks *global_cur_attack;


char *mdk4_help = "MDK4 " VERSION " - \"" VERSION_COOL "\"\n"
		  "by E7mer, thanks to the author of MDK3 and aircrack-ng community.\n"
		  "MDK4 is a proof-of-concept tool to exploit common IEEE 802.11 protocol weaknesses.\n"
		  "IMPORTANT: It is your responsibility to make sure you have permission from the\n"
		  "network owner before running MDK4 against it.\n\n"
		  "This code is licenced under the GPLv3 or later\n\n"
		  "MDK4 USAGE:\n"
		  "mdk4 <interface> <attack_mode> [attack_options]\n"
		  "mdk4 <interface in> <interface out> <attack_mode> [attack_options]\n\n"
		  "Try mdk4 --fullhelp for all attack options\n"
		  "Try mdk4 --help <attack_mode> for info about one attack only\n\n";


void print_help_and_die(struct attacks *att, int att_cnt, char full, char *add_msg) {
  int i;

  printf("%s\n", mdk4_help);

#ifdef __linux__
  ghosting_print_help();
#endif

  frag_print_help();

  printf("Loaded %d attack modules\n\n", att_cnt);

  for(i=0; i<att_cnt; i++) {
    printf("ATTACK MODE %c: %s\n", att[i].mode_identifier, att[i].attack_name);
    att[i].print_shorthelp();
  }

  if (full) {
    printf("\nFULL OPTIONS:\n");
    for(i=0; i<att_cnt; i++) {
      printf("\nATTACK MODE %c: %s\n", att[i].mode_identifier, att[i].attack_name);
      att[i].print_longhelp();
    }
  }

  if (add_msg) printf("\nERROR: %s\n", add_msg);

  exit(1);
}

void main_loop(struct attacks *att, void *options) {
  struct packet inject;
  unsigned int p_sent = 0, p_sent_ps = 0, ret;
  time_t t_prev = 0;

  while (1) {
    //Get packet
    inject = att->get_packet(options);
    if ((inject.data == NULL) || (inject.len == 0)) 
      continue;

    //Send packet
    if (frag_is_enabled()) ret = frag_send_packet(&inject);
    else ret = osdep_send_packet(&inject);

    if (ret) {
      printf("Injecting packet failed :( Sorry.\n");
      exit(-1);
    }

    p_sent_ps++;
    p_sent++;

    //Show speed and stats
    if((time(NULL) - t_prev) >= 1) {
      t_prev = time(NULL);
      att->print_stats(options);
      printf("\rPackets sent: %6d - Speed: %4d packets/sec\n", p_sent, p_sent_ps);
      fflush(stdout);
      p_sent_ps=0;
    }

    //Perform checks
    att->perform_check(options);
  }
}

int parse_evasion(int argc, char *argv[]) {
  int i = 1;

  while(i < argc) {
    if (i >= argc) break;

    if (! strcmp(argv[i], "--ghost")) {
      parse_ghosting(argv[i + 1]);
      i += 2;
    } else if (! strcmp(argv[i], "--frag")) {
      parse_frag(argv[i + 1]);
      i += 2;
    } else return (i - 1);
  }

  return (i - 1);
}

int main(int argc, char *argv[]) {
  struct attacks *a, *cur_attack = NULL;
  void *cur_options;
  int i, att_cnt;
  int dual_interface = 0;

  a = load_attacks(&att_cnt);

  if (geteuid() != 0) print_help_and_die(a, att_cnt, 0, "mdk4 requires root privileges.");

  if (argc < 2) print_help_and_die(a, att_cnt, 0, NULL);

  if (! strcmp(argv[1], "--fullhelp")) print_help_and_die(a, att_cnt, 1, NULL);

  if (argc < 3) print_help_and_die(a, att_cnt, 0, NULL);

  if (strlen(argv[2]) != 1){
	if(argc > 3){
		if(strlen(argv[3]) != 1){
			print_help_and_die(a, att_cnt, 0, "Attack Mode is only a single character!\n");
		}else{
			dual_interface = 1;
		}
	}else{
		print_help_and_die(a, att_cnt, 0, "Attack Mode is only a single character!\n");
	}

  }

  for(i=0; i<att_cnt; i++) {
	if (argv[2+dual_interface][0] == a[i].mode_identifier) cur_attack = a + i;
  }

  if (cur_attack == NULL) print_help_and_die(a, att_cnt, 0, "Invalid Attack Mode\n");

  if (!strcmp(argv[1], "--help")) { cur_attack->print_longhelp(); return 0; }


  if (osdep_start(argv[1], argv[1+dual_interface])) {
	printf("Starting OSDEP failed\n");
	return 2;
  }

  /* drop privileges */
  setuid(getuid());

  for(i=0; i<att_cnt; i++) free(a[i].attack_name); //Make Valgrind smile :)

  i = 2 + parse_evasion(argc - 2 - dual_interface, argv + 2 + dual_interface) + dual_interface;

  cur_options = cur_attack->parse_options(argc - i, argv + i);
  if (!cur_options) return 1;

  srandom(time(NULL));	//Fresh numbers each run

  global_cur_options = cur_options;
  global_cur_attack = cur_attack;

  //Parsing done, start attacks
  main_loop(cur_attack, cur_options);

  return 0;
}
