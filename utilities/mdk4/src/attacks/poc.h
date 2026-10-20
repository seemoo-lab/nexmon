#ifndef _POC_H
#define _POC_H

#include "../packet.h"
#include "attacks.h"

#define POC_MODE 'x'

struct poc_options {
  char vendor[255];
  struct ether_addr bssid;
  struct ether_addr source_mac;
  struct ether_addr target_mac;
  uint16_t seq_ctrl;
	uint16_t recv_seq_ctrl;
  uint16_t data_seq_ctrl;
  uint16_t recv_data_seq_ctrl;
  unsigned int speed;

};

struct poc_packet{
    char vendor[255];
    int pkt_cnt;
    struct packet *pkts;
};

struct attacks load_poc();

#endif