#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#include "helpers.h"
#include "mac_addr.h"
#include "linkedlist.h"
#include "greylist.h"
#include "dumpfile.h"
#include "brute.h"

void print_chars_decimal(char *values, int count) {
  int i;
  
  for(i=0; i<count; i++) printf("%d ", values[i]);
}

void print_strings(char **values, int count) {
  int i;
  
  for(i=0; i<count; i++) printf("\"%s\" ", values[i]);
}

void print_clist(struct clist *cl) {
  if (!cl) return;
  
  struct clist *first = cl;
  
  do {
    printf("    CList at %X contains: Status %d, Data %s\n", (unsigned int) cl, cl->status, cl->data);
    cl = cl->next;
  } while (cl != first);
}

void test_helpers() {
  char chan[8];
  char *ssid[8];
  int i;
  char *line;
  
  printf("Testing generic helpers:\n");
  
  for(i=0; i<8; i++) chan[i] = generate_channel();
  printf("Random channels: "); print_chars_decimal(chan, 8);
  for(i=0; i<8; i++) {
    assert(chan[i] > 0);
    assert(chan[i] < 15);
  }
  
  for(i=0; i<8; i++) ssid[i] = generate_ssid(0);
  printf("\nRandom SSIDs: "); print_strings(ssid, 8);
  for(i=0; i<8; i++) {
    assert(strlen(ssid[i]) > 0);
    assert(strlen(ssid[i]) < 33);
    free(ssid[i]);
  }
  
  printf("\nWho are my authors?:\n");
  line = read_next_line("./AUTHORS", 1);
  while (line) {
    printf("  READING: %s\n", line);
    free(line);
    line = read_next_line("./AUTHORS", 0);
  }
  
  printf("\n\n");
}

void show_some_macs(struct ether_addr mac, struct ether_addr mac2) {
  int i;
  
  printf("\n  First MAC: "); print_mac(get_next_mac(mac, &mac2));
  printf("\n  Second MAC: "); print_mac(get_next_mac(mac, &mac2));
  for(i=0; i<99998; i++) get_next_mac(mac, &mac2);
  printf("\n  MAC 100000: "); print_mac(get_next_mac(mac, &mac2));
  for(i=0; i<16677214; i++) get_next_mac(mac, &mac2);
  printf("\n  Many MACs later: "); print_mac(get_next_mac(mac, &mac2));
  if (MAC_IS_BCAST(mac2)) printf("\n  Ran out of MAC addresses (correct in semi-auto and manual).");
  for(i=0; i<123456; i++) get_next_mac(mac, &mac2);
  printf("\n  New base: "); print_mac(get_next_mac(mac, &mac2));}

void test_mac_addr() {
  struct ether_addr mac, mac2;
  char parse1[18] = "aa:bB:Cc:DD:00:0f";
  char parse2[13] = "aabbCCdDEef9";
  
  printf("Testing MAC Address parsers and generators:\n");
  
  MAC_SET_NULL(mac);
  printf("Null MAC: "); print_mac(mac);
  assert(MAC_IS_NULL(mac));
  
  MAC_SET_BCAST(mac);
  printf("\nBroadcast MAC: "); print_mac(mac);
  assert(MAC_IS_BCAST(mac));
  
  printf("\nParsing %s: ", parse1); print_mac(parse_mac(parse1));
  printf("\nParsing %s: ", parse2); print_mac(parse_mac(parse2));

  printf("\nRandom MAC: "); print_mac(generate_mac(MAC_KIND_RANDOM));
  printf("\nRandom valid client MAC: "); print_mac(generate_mac(MAC_KIND_CLIENT));
  printf("\nRandom valid AP MAC: "); print_mac(generate_mac(MAC_KIND_AP));

  printf("\nMAC filter Bruteforcer in auto-mode:");
  MAC_SET_NULL(mac); MAC_SET_NULL(mac2);
  show_some_macs(mac, mac2);
  printf("\nMAC filter Bruteforcer in semi-auto mode starting with F0:EE:DD:");
  mac.ether_addr_octet[0] = 0xF0;
  mac.ether_addr_octet[1] = 0xEE;
  mac.ether_addr_octet[2] = 0xDD;
  MAC_SET_NULL(mac2);
  show_some_macs(mac, mac2); 
  printf("\nMAC filter Bruteforcer in manual mode starting with F0:EE:DD:AA:00:85");
  mac2.ether_addr_octet[0] = 0xAA;
  mac2.ether_addr_octet[1] = 0x00;
  mac2.ether_addr_octet[2] = 0x85;
  show_some_macs(mac, mac2); 
  
  printf("\n\n");
}

void test_linkedlist() {
  struct clist *cl = NULL;
  char tdata[10] = "testdata";
  char *rdata;
  int i;
  
  printf("Testing Circular Linked Lists:\n");
  
  printf("Test A: Data CList\n");
  printf("  Searching status in empty list: %X\n", (unsigned int) search_status(cl, 0));
  printf("  Searching \"%s\" in empty list: %X\n", tdata, (unsigned int) search_data(cl, (u_char *) tdata, strlen(tdata)));
  printf("  Adding random data to list.\n");
  for (i=0; i<5; i++) {
    rdata = generate_ssid(0);
    cl = add_to_clist(cl, (u_char *) rdata, random(), strlen(rdata)+1);
    free(rdata);
  }
  printf("  Adding \"%s\"\n", tdata);
  cl = add_to_clist(cl, (u_char *) tdata, 0, strlen(tdata)+1);
  printf("  Adding more random data to list.\n");
  for (i=0; i<15; i++) {
    rdata = generate_ssid(0);
    cl = add_to_clist(cl, (u_char *) rdata, random(), strlen(rdata)+1);
    free(rdata);
  }
  printf("  CList DUMP:\n");
  print_clist(cl);
  printf("  Searching status in full list: %X\n", (unsigned int) search_status(cl, 0));
  printf("  Searching \"%s\" in full list: %X\n", tdata, (unsigned int) search_data(cl, (u_char *) tdata, strlen(tdata)));

  printf("Test B: WIDS AP CList - not implemented\n");
  printf("Test C: WIDS Client CList - not implemented\n");
}

void test_greylist() {
  struct ether_addr target = parse_mac("000011112222");
  
  printf("Testing Greylist\n");
  
  //Using an example file that is not well formed ;)
  load_greylist(1, "./useful_files/fakeap-example.txt");
  
  if (is_blacklisted(target)) {
    printf(" Target MAC has been found blacklisted\n");
  } else {
    printf(" Target MAC is NOT blacklisted!\n");
  }
  
  printf("Turning list into a whitelist:\n");
  load_greylist(0, NULL);
  
  if (is_blacklisted(target)) {
    printf(" Target MAC has been found blacklisted\n");
  } else {
    printf(" Target MAC is NOT blacklisted!\n");
  }
}

void test_packet() {
  struct packet pkt;
  int i;
  uint16_t caps;
  struct ether_addr bssid, station;
  char *ssid;
  char enc[4] = {'n', 'w', 't', 'a'};
  
  printf("Opening testdump.cap\n");
  
  start_dump("testdump.cap");
  
  printf("Creating random beacons :)\n");
  for(i=0; i<20; i++) {
    bssid = generate_mac(MAC_KIND_AP);
    ssid = generate_ssid(0);
    pkt = create_beacon(bssid, ssid, (uint8_t) (random() % 14), enc[random() % 4], (random() % 2) * 54, random() % 2);
    dump_packet(&pkt);
    free(ssid);
    ssid = get_ssid(&pkt, NULL);
    caps = get_capabilities(&pkt);
    printf("SSID found in beacon: %s\n", ssid);
    printf("Capabilities: %04X\n", caps);
    free(ssid);
  }
  printf("Creating random auths :)\n");
  for(i=0; i<20; i++) {
    bssid = generate_mac(MAC_KIND_AP);
    station = generate_mac(MAC_KIND_CLIENT);
    pkt = create_auth(bssid, station, (random() % 2) + 1);
    dump_packet(&pkt);
  }
  printf("Creating random probes :)\n");
  for(i=0; i<20; i++) {
    station = generate_mac(MAC_KIND_CLIENT);
    ssid = generate_ssid(0);
    pkt = create_probe(station, ssid, (random() % 2) * 54);
    dump_packet(&pkt);
    free(ssid);
  }
  printf("Creating random kicks :)\n");
  for(i=0; i<20; i++) {
    station = generate_mac(MAC_KIND_CLIENT);
    bssid = generate_mac(MAC_KIND_AP);    
    pkt = create_deauth(bssid, station, bssid);
    dump_packet(&pkt);
    pkt = create_deauth(station, bssid, bssid);
    dump_packet(&pkt);
    pkt = create_disassoc(bssid, station, bssid);
    dump_packet(&pkt);
    pkt = create_disassoc(station, bssid, bssid);
    dump_packet(&pkt);
  }
  printf("Creating random associations :)\n");
  for(i=0; i<20; i++) {
    bssid = generate_mac(MAC_KIND_AP);
    ssid = generate_ssid(0);
    station = generate_mac(MAC_KIND_CLIENT);
    pkt = create_assoc_req(station, bssid, 0x0431, ssid, 54);
    dump_packet(&pkt);
    free(ssid);
  }
  printf("done!\n");
}

void test_brute() {
  char word[3] = { 'x', 'x', 0x00 }; //Can't init with "xx", would be READ ONLY!
  int sl;
  char *fresh = NULL;
  
  printf("Words after %s, using lowercase and numbers:\n", word);
  sl = strlen(word);
  
  while(get_brute_word("ln", word, sl)) {
    printf("%s, ", word);
    fflush(stdout);
  }
  
  printf("Keyspace exhausted!\n");
  
  printf("Fresh 2 char words:\n");
  while((fresh = get_brute_word("u", fresh, 2))) { //Yep, use assignment as truth value.
    printf("%s, ", fresh);
    fflush(stdout);    
  }
  printf("Keyspace exhausted!\n");
  
  free(fresh);
}

int main() {
  
  printf("mdk3 Implementation Tests\n\n");
  
  srandom(time(NULL));	//Fresh numbers each run
  
  test_helpers();
  test_linkedlist();
  test_greylist();
  test_packet();
  test_mac_addr();
  test_brute();

  stop_dump();
  return 0;
}
  
