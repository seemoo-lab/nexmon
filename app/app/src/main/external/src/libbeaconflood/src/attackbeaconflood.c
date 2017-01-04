#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <getopt.h>
#include <jni.h>

#include "attacks/attacks_beaconflood.h"
#include "osdep.h"
#include "ghosting.h"
#include "fragmenting.h"
#include "attackbeaconflood.h"
#include "infomessage.h"
#include "beaconfloodstop.h"

#define VERSION "v7"
#define VERSION_COOL "OMG! He cleaned his code!"

char info_buffer[1024];

void main_loop(struct attacks *att, void *options) {
  struct packet inject;
  unsigned int p_sent = 0, p_sent_ps = 0, ret;
  time_t t_prev = 0;
  while (1) {
    //Get packet
    inject = att->get_packet(options);
    if ((inject.data == NULL) || (inject.len == 0)) break;

    send_packet_to_java(inject.data, inject.len);

    if(stop_attack)
      break;

    p_sent_ps++;
    p_sent++;
    
    //Show speed and stats
    if((time(NULL) - t_prev) >= 1) {
      t_prev = time(NULL);
      att->print_stats(options);

      info_message_stats(p_sent_ps, p_sent);
      printf("\rPackets sent: %6d - Speed: %4d packets/sec", p_sent, p_sent_ps);
      fflush(stdout);
      p_sent_ps=0;
    }
    
    //Perform checks
    att->perform_check(options);
  }
}


int attack_beacon_flood(JNIEnv* pEnv, jobject pThis, jmethodID pMid, jmethodID pMid_stats, char* interface_name, int myid) {
  struct attacks *a, *cur_attack = NULL;
  void *cur_options;
  int i, att_cnt;
  stop_attack = 0;
  
  setup_info_message(pEnv, pThis, pMid, pMid_stats, myid);
  snprintf(info_buffer, sizeof(info_buffer), "Attack Beacon Flood started...\n");
  info_message(info_buffer, UPDATE_RUNNING);

  setup_send_env(pEnv);

  a = load_attacks(&att_cnt);
  cur_attack = a;
  
  /*if (osdep_start(interface_name)) {
    printf("Starting OSDEP on %s failed\n", interface_name);
    snprintf(info_buffer, sizeof(info_buffer), "Starting OSDEP on %s failed\n", interface_name);
    info_message(info_buffer, UPDATE_ERROR);
    return 2;
  }*/
  
  /* drop privileges */
  setuid(getuid());

  i = 2;
  
  cur_options = cur_attack->parse_options();
  if (!cur_options) return 1;

  srandom(time(NULL));	//Fresh numbers each run
  
  //Parsing done, start attacks
  main_loop(cur_attack, cur_options);
  
  return 0;
}
