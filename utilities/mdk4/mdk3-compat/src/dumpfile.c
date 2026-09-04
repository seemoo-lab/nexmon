#include <sys/time.h>
#include <pcap.h>

#include "dumpfile.h"

pcap_t *dumphandle = NULL;
pcap_dumper_t *dumper = NULL;

void dump_packet(struct packet *pkt) {
  struct timeval now;
  struct pcap_pkthdr hdr;
  
  if (! dumper) {
    printf("You need to start a dumpfile before dumping packets.\n");
    return;
  }
  
  gettimeofday(&now, NULL);
  hdr.ts = now;
  hdr.caplen = pkt->len;
  hdr.len = pkt->len;
  
  pcap_dump((u_char *) dumper, &hdr, pkt->data);
  pcap_dump_flush(dumper);
}

void start_dump(char *filename) {
  
  if (dumper) {
    printf("Dumpfile is already opened, ignoring request\n");
    return;
  }
  
  dumphandle = pcap_open_dead(DLT_IEEE802_11, BUFSIZ);
  dumper = pcap_dump_open(dumphandle, filename);
}

void stop_dump() {
  if (! dumper) return;
  pcap_dump_close(dumper);
  pcap_close(dumphandle);
}