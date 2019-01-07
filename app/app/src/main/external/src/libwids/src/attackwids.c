#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <getopt.h>
#include <jni.h>

#include "attacks/attacks_wids.h"
#include "osdep.h"
#include "ghosting.h"
#include "fragmenting.h"
#include "attackwids.h"
#include "infomessage.h"
#include "widsstop.h"

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
    
    //Send packet
    if (frag_is_enabled()) ret = frag_send_packet(&inject);
    else ret = osdep_send_packet(&inject);

    if (ret) {
      snprintf(info_buffer, sizeof(info_buffer), "Injecting packet failed :( Sorry.\n");
      info_message(info_buffer, UPDATE_ERROR);
      printf("Injecting packet failed :( Sorry.\n");
      return;
    }
    if(stop_attackwids)
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


int attack_wids(JNIEnv* pEnv, jobject pThis, jmethodID pMid, jmethodID pMid_stats, char* interface_name, char* ap_essid, int use_chan_hopping, int use_zero_chaos, int myid) {
  struct attacks *cur_attack = NULL;
  void *cur_options;
  int att_cnt;
  //stop_attackauthflood = 0;
  stop_attackwids = 0;
  (*pEnv)->GetJavaVM(pEnv, &jvm);  
  
  setup_info_message(pEnv, pThis, pMid, pMid_stats, myid);
  snprintf(info_buffer, sizeof(info_buffer), "Attack Auth Flood started...\n");
  info_message(info_buffer, UPDATE_RUNNING);

  cur_attack = load_attacks(&att_cnt);
  
  
  if (osdep_start(interface_name)) {
    printf("Starting OSDEP on %s failed\n", interface_name);
    snprintf(info_buffer, sizeof(info_buffer), "Starting OSDEP on %s failed\n", interface_name);
    info_message(info_buffer, UPDATE_ERROR);
    return 2;
  }
  
  /* drop privileges */
  setuid(getuid());

  
  cur_options = cur_attack->parse_options(ap_essid, use_chan_hopping, use_zero_chaos);
  if (!cur_options) return 1;

  srandom(time(NULL));  //Fresh numbers each run
  
  //Parsing done, start attacks
  main_loop(cur_attack, cur_options);

  (*pEnv)->DeleteGlobalRef(pEnv, cb_obj);

  return 0;
}
